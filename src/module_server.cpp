#include "swapit/module_server.h"

#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <csignal>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <sstream>
#include <thread>

#include <nodesets/common_nodeids.h>
#include <nodesets/namespace_common_generated.h>
#include <nodesets/namespace_pfdl_generated.h>
#include <nodesets/pfdl_nodeids.h>
#include <nodesets/types_common_generated.h>
#include <nodesets/types_pfdl_generated.h>
#include <unordered_map>

extern "C" {
#include <swap_it.h>
}

namespace swapit {

namespace {

// error

class BadStatusError : public std::exception {
  public:
    explicit BadStatusError(UA_StatusCode status = UA_STATUSCODE_BAD) : status_(status) {}

    const char *
    what() const noexcept override {
        return UA_StatusCode_name(status_);
    }

    UA_StatusCode
    status() const noexcept {
        return status_;
    }

  private:
    UA_StatusCode status_;
};

void
throw_if_bad(UA_StatusCode status) {
    if(UA_StatusCode_isBad(status)) {
        throw BadStatusError(status);
    }
}

// worker

template <typename T> class Queue {
  public:
    Queue() = default;

    void
    enqueue(T f) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(f));
    }

    std::optional<T>
    dequeue() {
        std::lock_guard<std::mutex> lock(mutex_);
        if(queue_.empty()) {
            return std::nullopt;
        }
        auto f = std::move(queue_.front());
        queue_.pop();
        return f;
    }

  private:
    std::queue<T> queue_;
    std::mutex mutex_;
};

class WorkerThread {
  public:
    WorkerThread(std::chrono::milliseconds timeout = std::chrono::milliseconds(100))
        : timeout_(timeout), is_running_(true), thread_(&WorkerThread::loop, this) {}

    ~WorkerThread() {
        is_running_ = false;
        if(thread_.joinable()) {
            thread_.join();
        }
    }

    void
    enqueue(std::function<void()> f) {
        queue_.enqueue(std::move(f));
    }

  private:
    void
    loop() {
        while(is_running_) {
            auto f = queue_.dequeue();
            if(f.has_value()) {
                std::invoke(*f);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }

  private:
    Queue<std::function<void()>> queue_;
    std::atomic<bool> is_running_;
    std::chrono::milliseconds timeout_;
    std::thread thread_;
};

// string

UA_String
ua_string(const std::string &s) {
    return UA_STRING((char *)s.c_str());
}

UA_LocalizedText
ua_localized_text(const std::string &s, const std::string &l = "en-US") {
    return UA_LOCALIZEDTEXT((char *)l.c_str(), (char *)s.c_str());
}

UA_QualifiedName
ua_qualified_name(UA_UInt16 ns, const std::string &s) {
    return UA_QUALIFIEDNAME(ns, (char *)s.c_str());
}

UA_ByteString
ua_byte_string(const std::vector<uint8_t> &b) {
    return {b.size(), (UA_Byte *)b.data()};
}

std::string
to_string(const UA_String &s) {
    return std::string(s.data, s.data + s.length);
}

// misc

UA_NodeId
default_node_id(UA_UInt16 ns) {
    return {ns, UA_NODEIDTYPE_NUMERIC, 0};
}

void
make_mandatory(UA_Server *server, const UA_NodeId node_id) {
    throw_if_bad(UA_Server_addReference(
        server, node_id, UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true));
}

// variant

PfdlType
get_type(const PfdlVariant &var) {
    return static_cast<PfdlType>(var.index());
}

// const UA_DataType *
// get_data_type(PfdlType t) {
//     switch(t) {
//         case PfdlType::boolean:
//             return &UA_TYPES[UA_TYPES_BOOLEAN];
//         case PfdlType::number:
//             return &UA_TYPES[UA_TYPES_DOUBLE];
//         case PfdlType::string:
//             return &UA_TYPES[UA_TYPES_STRING];
//         default:
//             throw std::logic_error("Unexpected enum class value.");
//     }
// }

const UA_DataType *
get_struct_data_type(PfdlType t) {
    switch(t) {
        case PfdlType::boolean:
            return &UA_TYPES_PFDL[UA_TYPES_PFDL_PFDLBOOLEAN];
        case PfdlType::number:
            return &UA_TYPES_PFDL[UA_TYPES_PFDL_PFDLNUMBER];
        case PfdlType::string:
            return &UA_TYPES_PFDL[UA_TYPES_PFDL_PFDLSTRING];
        default:
            throw std::logic_error("Unexpected enum class value.");
    }
}

template <typename T>
T
ua_variant_get(const UA_Variant *v) {
    return *static_cast<T *>(v->data);
}

template <typename T>
const T &
variant_get(const PfdlVariant &v) {
    try {
        return std::get<T>(v);
    } catch(const std::bad_variant_access &e) {
        throw BadStatusError(UA_STATUSCODE_BADTYPEMISMATCH);
    }
}

std::optional<PfdlVariant>
to_variant(const UA_Variant *v, PfdlType t) {
    if(!UA_Variant_hasScalarType(v, get_struct_data_type(t))) {
        return std::nullopt;
    }
    switch(t) {
        case PfdlType::boolean:
            return ua_variant_get<UA_PfdlBoolean>(v).value;
        case PfdlType::number:
            return ua_variant_get<UA_PfdlNumber>(v).value;
        case PfdlType::string: {
            return to_string(ua_variant_get<UA_PfdlString>(v).value);
        }
    }
    return std::nullopt;
}

// namespaces

std::vector<std::string>
read_namespace_array(UA_Server *server) {
    UA_Variant ns_var;
    throw_if_bad(UA_Server_readValue(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY), &ns_var));
    if(!UA_Variant_hasArrayType(&ns_var, &UA_TYPES[UA_TYPES_STRING])) {
        throw BadStatusError(UA_STATUSCODE_BADUNEXPECTEDERROR);
    }

    std::vector<std::string> ns_array;
    UA_String *ns = static_cast<UA_String *>(ns_var.data);
    for(size_t i = 0; i < ns_var.arrayLength; ++i) {
        ns_array.push_back(to_string(ns[i]));
    }
    return ns_array;
}

template <typename AddGenerated>
UA_UInt16
add_generated_namespace(UA_Server *server, AddGenerated add_ns) {
    throw_if_bad(add_ns(server));

    std::vector<std::string> ns_array = read_namespace_array(server);
    if(ns_array.empty()) {
        throw BadStatusError(UA_STATUSCODE_BADUNEXPECTEDERROR);
    }
    size_t nsi;
    throw_if_bad(UA_Server_getNamespaceByName(server, ua_string(ns_array.back()), &nsi));
    return static_cast<UA_UInt16>(nsi);
};

struct Namespaces {
    UA_UInt16 common;
    UA_UInt16 pfdl;
    UA_UInt16 module;
};

Namespaces
add_namespaces(UA_Server *server, const std::string &module_ns) {
    Namespaces ns;
    ns.common = add_generated_namespace(server, namespace_common_generated);
    ns.pfdl = add_generated_namespace(server, namespace_pfdl_generated);
    ns.module = UA_Server_addNamespace(server, module_ns.c_str());
    return ns;
}

// module type

void
make_variables_writable(UA_Server *server, UA_UInt16 common_ns) {
    using NodeIterCallback =
        std::function<UA_StatusCode(UA_NodeId child_id, UA_Boolean is_inverse,
                                    UA_NodeId reference_type_id, void *handle)>;

    auto node_iter_native_callback = [](UA_NodeId child_id, UA_Boolean is_inverse,
                                        UA_NodeId reference_type_id,
                                        void *handle) -> UA_StatusCode {
        NodeIterCallback *node_iter = static_cast<NodeIterCallback *>(handle);
        return std::invoke(*node_iter, child_id, is_inverse, reference_type_id, handle);
    };

    NodeIterCallback node_iter_callback = [server, node_iter_native_callback, common_ns](
                                              UA_NodeId child_id, UA_Boolean is_inverse,
                                              UA_NodeId reference_type_id,
                                              void *handle) -> UA_StatusCode {
        UA_StatusCode status = UA_STATUSCODE_GOOD;
        if(is_inverse) {
            return status;
        }

        if(child_id.namespaceIndex != common_ns) {
            return status;
        }

        UA_NodeClass node_class;
        status = UA_Server_readNodeClass(server, child_id, &node_class);
        if(UA_StatusCode_isBad(status)) {
            return status;
        }

        if(node_class == UA_NODECLASS_OBJECT) {
            return UA_Server_forEachChildNodeCall(server, child_id,
                                                  node_iter_native_callback, handle);
        }

        if(node_class != UA_NODECLASS_VARIABLE) {
            return status;
        }

        UA_UInt32 access_level = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        status = UA_Server_writeAccessLevel(server, child_id, access_level);

        return status;
    };

    throw_if_bad(UA_Server_forEachChildNodeCall(
        server, UA_NODEID_NUMERIC(common_ns, UA_COMMONID_MODULETYPE),
        node_iter_native_callback, &node_iter_callback));
}

UA_NodeId
create_module_type_object(UA_Server *server, const Namespaces &ns,
                          const std::string &name) {
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.displayName = ua_localized_text(name);

    UA_NodeId id;
    throw_if_bad(
        UA_Server_addObjectTypeNode(server, default_node_id(ns.module),
                                    UA_NODEID_NUMERIC(ns.common, UA_COMMONID_MODULETYPE),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                    ua_qualified_name(ns.module, name), attr, NULL, &id));
    return id;
}

UA_NodeId
create_object_copy(UA_Server *server, const UA_NodeId &src_id,
                   const UA_NodeId &parent_id) {
    UA_QualifiedName browse_name = {};
    throw_if_bad(UA_Server_readBrowseName(server, src_id, &browse_name));
    browse_name.namespaceIndex = parent_id.namespaceIndex;

    UA_LocalizedText display_name = {};
    throw_if_bad(UA_Server_readDisplayName(server, src_id, &display_name));
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = display_name;

    UA_NodeId id;
    throw_if_bad(UA_Server_addObjectNode(
        server, default_node_id(parent_id.namespaceIndex), parent_id,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), browse_name,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), attr, NULL, &id));
    return id;
}

UA_NodeId
create_services_object(UA_Server *server, UA_UInt16 common_ns, const UA_NodeId &type_id) {
    UA_NodeId src_id = UA_NODEID_NUMERIC(common_ns, UA_COMMONID_MODULETYPE_SERVICES);
    UA_NodeId id = create_object_copy(server, src_id, type_id);
    make_mandatory(server, id);
    return id;
};

// event

UA_NodeId
create_service_event_type(UA_Server *server, const Namespaces &ns,
                          const std::string &name) {
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.displayName = ua_localized_text(name);

    UA_NodeId id;
    throw_if_bad(UA_Server_addObjectTypeNode(
        server, default_node_id(ns.module),
        UA_NODEID_NUMERIC(ns.common, UA_COMMONID_SERVICEFINISHEDEVENTTYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE), ua_qualified_name(ns.module, name),
        attr, NULL, &id));
    return id;
}

UA_NodeId
create_service_event_result(UA_Server *server, UA_UInt16 module_ns,
                            const Parameter &output_param,
                            const UA_NodeId &event_type_id) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = ua_localized_text(output_param.name);
    attr.dataType = get_struct_data_type(output_param.type)->typeId;
    attr.valueRank = UA_VALUERANK_SCALAR;

    UA_NodeId id;
    throw_if_bad(
        UA_Server_addVariableNode(server, default_node_id(module_ns), event_type_id,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                  ua_qualified_name(module_ns, output_param.name),
                                  UA_NODEID_NULL, attr, NULL, &id));

    make_mandatory(server, id);
    return id;
}

UA_NodeId
create_event(UA_Server *server, const UA_NodeId &type_id) {
    UA_NodeId event_id;
    throw_if_bad(UA_Server_createEvent(server, type_id, &event_id));

    UA_DateTime time = UA_DateTime_now();
    throw_if_bad(UA_Server_writeObjectProperty_scalar(server, event_id,
                                                      ua_qualified_name(0, "Time"), &time,
                                                      &UA_TYPES[UA_TYPES_DATETIME]));
    return event_id;
}

void
write_exec_result(UA_Server *server, const UA_NodeId &event_id,
                  const UA_NodeId &exec_result_id, bool success) {
    UA_QualifiedName name;
    throw_if_bad(UA_Server_readBrowseName(server, exec_result_id, &name));

    UA_ServiceExecutionStatus status =
        success ? UA_SERVICEEXECUTIONSTATUS_SERVICE_EXECUTION_SUCCESS
                : UA_SERVICEEXECUTIONSTATUS_SERVICE_EXECUTION_FAIL;

    return throw_if_bad(UA_Server_writeObjectProperty_scalar(
        server, event_id, name, &status,
        &UA_TYPES_COMMON[UA_TYPES_COMMON_SERVICEEXECUTIONSTATUS]));
}

void
write_result(UA_Server *server, const UA_NodeId &event_id, const UA_NodeId &result_id,
             const PfdlVariant &result) {
    UA_QualifiedName name;
    throw_if_bad(UA_Server_readBrowseName(server, result_id, &name));

    auto write_property = [server, event_id, name](void *data, const UA_DataType *type) {
        throw_if_bad(
            UA_Server_writeObjectProperty_scalar(server, event_id, name, data, type));
    };

    PfdlType type = get_type(result);
    switch(type) {
        case PfdlType::boolean: {
            UA_PfdlBoolean b = {variant_get<bool>(result)};
            write_property(&b, get_struct_data_type(type));
            break;
        }
        case PfdlType::number: {
            UA_PfdlNumber n = {variant_get<double>(result)};
            write_property(&n, get_struct_data_type(type));
            break;
        }
        case PfdlType::string: {
            UA_PfdlString s = {ua_string(variant_get<std::string>(result))};
            write_property(&s, get_struct_data_type(type));
            break;
        }
    }
}

class ServiceEvent {
  public:
    static ServiceEvent
    create(UA_Server *server, const Namespaces &ns, const std::string &name,
           const Parameter &output_param) {
        ServiceEvent event;
        event.type_id_ = create_service_event_type(server, ns, name);
        event.exec_result_id_ = UA_NODEID_NUMERIC(
            ns.common, UA_COMMONID_SERVICEFINISHEDEVENTTYPE_SERVICEEXECUTIONRESULT);
        event.result_id_ =
            create_service_event_result(server, ns.module, output_param, event.type_id_);
        return event;
    }

    void
    emit(UA_Server *server, const std::optional<PfdlVariant> &result) const noexcept {
        const std::string fail_msg = "Failed to emit ServiceFinishedEvent!";
        try {
            const UA_NodeId event_id = create_event(server, type_id_);
            write_exec_result(server, event_id, exec_result_id_, result.has_value());
            if(result.has_value()) {
                write_result(server, event_id, result_id_, *result);
            }
            throw_if_bad(UA_Server_triggerEvent(
                server, event_id, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), NULL, true));
        } catch(const BadStatusError &e) {
            std::cerr << fail_msg << " Failed with Status: " << e.what() << std::endl;
        } catch(...) {
            std::cerr << fail_msg << " An unknown exception occured." << std::endl;
        }
    }

  private:
    UA_NodeId type_id_;
    UA_NodeId exec_result_id_;
    UA_NodeId result_id_;
};

// service

std::vector<Argument>
convert_arguments(const std::vector<UA_Variant> &input,
                  const std::vector<Parameter> &params) {
    if(input.size() != params.size()) {
        throw BadStatusError(UA_STATUSCODE_BADINVALIDARGUMENT);
    }

    std::vector<Argument> args(params.size());
    for(size_t i = 0; i < params.size(); ++i) {
        std::optional<PfdlVariant> var = to_variant(&input[i], params[i].type);
        if(!var.has_value()) {
            throw BadStatusError(UA_STATUSCODE_BADINVALIDARGUMENT);
        }
        args[i] = {params[i].name, *var};
    }
    return args;
}

UA_Variant
create_sync_result(const std::string &msg, UA_StatusCode status) {
    UA_ServiceExecutionAsyncResultDataType result = {};
    result.serviceResultMessage = ua_string(msg);
    result.serviceResultCode = status;
    result.expectedServiceExecutionDuration = 0.0;
    result.serviceTriggerResult =
        status == UA_STATUSCODE_BADINVALIDARGUMENT
            ? UA_SERVICETRIGGERRESULT_SERVICE_RESULT_INVALID_PARAMETER
            : UA_SERVICETRIGGERRESULT_SERVICE_RESULT_ACCEPTED;

    UA_Variant v = {};
    throw_if_bad(UA_Variant_setScalarCopy(
        &v, &result,
        &UA_TYPES_COMMON[UA_TYPES_COMMON_SERVICEEXECUTIONASYNCRESULTDATATYPE]));
    return v;
}

UA_NodeId
create_service_method(UA_Server *server, UA_UInt16 module_ns, const std::string &name,
                      const std::vector<Parameter> &params, const UA_NodeId &parent_id) {
    UA_MethodAttributes attr = UA_MethodAttributes_default;
    std::vector<UA_Argument> input_args;
    input_args.reserve(params.size());
    for(const auto &[n, t] : params) {
        UA_Argument arg = {};
        arg.name = ua_string(n);
        arg.dataType = get_struct_data_type(t)->typeId;
        arg.valueRank = UA_VALUERANK_SCALAR;
        input_args.push_back(arg);
    }

    UA_Argument output_arg = {};
    output_arg.name = ua_string("res");
    output_arg.dataType =
        UA_TYPES_COMMON[UA_TYPES_COMMON_SERVICEEXECUTIONASYNCRESULTDATATYPE].typeId,
    output_arg.valueRank = UA_VALUERANK_SCALAR;

    UA_NodeId method_id;
    throw_if_bad(UA_Server_addMethodNode(server, default_node_id(module_ns), parent_id,
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                         ua_qualified_name(module_ns, name), attr, NULL,
                                         input_args.size(), input_args.data(), 1,
                                         &output_arg, NULL, &method_id));
    make_mandatory(server, method_id);
    return method_id;
}

class AsyncService {
  public:
    static AsyncService
    create(UA_Server *server, const Namespaces &ns, const ServiceDescription &descr) {
        AsyncService service;
        service.params_ = descr.input_params;
        service.callback_ = descr.callback;
        service.event_ = ServiceEvent::create(server, ns, descr.name, descr.output_param);
        return service;
    }

    UA_Variant
    operator()(UA_Server *server, WorkerThread &worker,
               const std::vector<UA_Variant> &input) const {
        UA_StatusCode status = UA_STATUSCODE_GOOD;
        std::stringstream msg;
        try {
            std::vector<Argument> args = convert_arguments(input, params_);
            worker.enqueue(
                [this, server, args = std::move(args)] { async_callback(server, args); });
            msg << "Executing async Service.";
        } catch(const BadStatusError &e) {
            status = e.status();
            msg << "Execution failed with Status: " << e.what() << ".";
        }
        return create_sync_result(msg.str(), status);
    }

  private:
    void
    async_callback(UA_Server *server, std::vector<Argument> args) const noexcept {
        std::optional<PfdlVariant> result;
        try {
            std::cout << "Starting Service execution" << std::endl;
            result = callback_(args);
            std::cout << "Service finished with "
                      << (result.has_value() ? "SUCCESS" : "ERROR") << "." << std::endl;
        } catch(...) {
            std::cerr << "An unknown exception occured during Service execution."
                      << std::endl;
        }
        event_.emit(server, result);
    }

  private:
    std::vector<Parameter> params_;
    ServiceCallback callback_;
    ServiceEvent event_;
};

struct ServiceDefinition {
    UA_NodeId method_id;
    AsyncService service;
};

ServiceDefinition
create_service(UA_Server *server, const Namespaces &ns, const UA_NodeId &parent_id,
               const ServiceDescription &descr) {
    ServiceDefinition def;
    def.method_id = create_service_method(server, ns.module, descr.name,
                                          descr.input_params, parent_id);
    def.service = AsyncService::create(server, ns, descr);
    return def;
}

// service store

class ServiceStore {
  public:
    ServiceStore() = default;

    void
    add(ServiceDefinition &&def) {
        services_[UA_NodeId_hash(&def.method_id)] = def.service;
    }

    const AsyncService &
    get(const UA_NodeId &method_id) const {
        auto it = services_.find(UA_NodeId_hash(&method_id));
        if(it == services_.end()) {
            throw BadStatusError(UA_STATUSCODE_BADNOENTRYEXISTS);
        }
        return it->second;
    }

  private:
    std::unordered_map<UA_UInt32, AsyncService> services_;
};

// module server

class ModuleServer {
  public:
    ModuleServer(const ModuleDescription &descr);

    UA_Variant
    call_async_service(const UA_NodeId &method_id, const std::vector<UA_Variant> &input) {
        const AsyncService &service = services_.get(method_id);
        return service(server_.get(), worker_, input);
    }

    UA_Server *
    server() {
        return server_.get();
    }

  private:
    // Keep member order!
    std::unique_ptr<UA_Server, decltype(&UA_Server_delete)> server_;
    ServiceStore services_;
    WorkerThread worker_;
};

void
set_server_context(UA_Server *server, ModuleServer *module_server) {
    throw_if_bad(UA_Server_setNodeContext(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                                          module_server));
}

ModuleServer &
get_server_context(UA_Server *server) {
    void *node_context;
    throw_if_bad(UA_Server_getNodeContext(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                                          &node_context));
    if(!node_context) {
        throw BadStatusError(UA_STATUSCODE_BADINTERNALERROR);
    }
    return *static_cast<ModuleServer *>(node_context);
}

UA_StatusCode
service_callback(UA_Server *server, const UA_NodeId *session_id, void *session_handle,
                 const UA_NodeId *method_id, void *method_context,
                 const UA_NodeId *object_id, void *object_context, size_t input_size,
                 const UA_Variant *input, size_t output_size,
                 UA_Variant *output) noexcept {
    UA_StatusCode status = UA_STATUSCODE_GOOD;
    try {
        *output = get_server_context(server).call_async_service(
            *method_id, std::vector<UA_Variant>(input, input + input_size));
    } catch(const BadStatusError &e) {
        status = e.status();
    } catch(...) {
        status = UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    return status;
}

void
init_module(UA_Server *server, ServiceStore &services_, const ModuleDescription &descr) {
    Namespaces ns = add_namespaces(server, descr.namespace_name);
    make_variables_writable(server, ns.common);
    const UA_NodeId type_id = create_module_type_object(server, ns, descr.type_name);
    const UA_NodeId services_id = create_services_object(server, ns.common, type_id);
    for(const auto &s : descr.services) {
        services_.add(create_service(server, ns, services_id, s));
    }
};

ModuleServer::ModuleServer(const ModuleDescription &descr)
    : server_(UA_Server_new(), &UA_Server_delete) {
    if(!server()) {
        throw BadStatusError();
    }
    set_server_context(server(), this);
    init_module(server(), services_, descr);
}

// server template

std::vector<uint8_t>
read_binary_file(const std::string &json_file) {
    std::ifstream ifs(json_file, std::ios::binary);
    if(!ifs.is_open()) {
        std::cerr << "Failed to open file: " << json_file << "." << std::endl;
        throw BadStatusError();
    }
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(ifs), {});
}

bool is_running = true;
void
run_template_server(UA_Server *server, const std::vector<uint8_t> &json_bytes,
                    bool to_registry) {

    auto signal_handler = [](int sig) { is_running = false; };
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    // UA_Server_run(server, &is_running);

    UA_service_server_interpreter config = {};
    throw_if_bad(UA_server_swap_it(server, ua_byte_string(json_bytes), service_callback,
                                   false, &is_running, to_registry, &config));

    while(is_running) {
        UA_Server_run_iterate(server, true);
    }

    clear_swap_server(&config, to_registry, server);
}

}  // namespace

void
run_module_server(ModuleDescription descr, const std::string &json_file,
                  bool to_registry) {
    try {
        auto json_bytes = read_binary_file(json_file);
        ModuleServer module_server(descr);
        run_template_server(module_server.server(), json_bytes, to_registry);
    } catch(const BadStatusError &e) {
        std::cerr << "An error occured during execution. Status: " << e.what()
                  << std::endl;
    } catch(...) {
        std::cerr << "An unknown exception occured during execution." << std::endl;
    }
}

}  // namespace swapit