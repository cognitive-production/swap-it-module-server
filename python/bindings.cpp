#include <nanobind/nanobind.h>
#include <nanobind/stl/function.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>
#include <swapit/module_server.h>

namespace nb = nanobind;
using namespace swapit;

NB_MODULE(_module_server_impl, m) {
    nb::enum_<PfdlType>(m, "PfdlType")
        .value("number", PfdlType::number)
        .value("boolean", PfdlType::boolean)
        .value("string", PfdlType::string);

    nb::class_<Parameter>(m, "Parameter")
        .def(nb::init<>())
        .def(nb::init<const std::string &, PfdlType>())
        .def_rw("name", &Parameter::name)
        .def_rw("type", &Parameter::type);

    nb::class_<Argument>(m, "Argument")
        .def(nb::init<>())
        .def(nb::init<const std::string &, PfdlVariant>())
        .def_rw("name", &Argument::name)
        .def_rw("value", &Argument::value);

    nb::class_<ServiceDescription>(m, "ServiceDescription")
        .def(nb::init<>())
        .def_rw("name", &ServiceDescription::name)
        .def_rw("input_params", &ServiceDescription::input_params)
        .def_rw("output_param", &ServiceDescription::output_param)
        // .def_prop_rw(
        //     "callback", [](ServiceDescription &s) { return s.callback; },
        //     [](ServiceDescription &s, const ServiceCallback &cb) {
        //         s.callback =
        //             [cb = cb](
        //                 const std::vector<Argument> &args) ->
        //                 std::optional<PfdlVariant> {
        //             try {
        //                 std::cout << "Hello" << std::endl;
        //                 return cb(args);
        //             } catch(const nb::cast_error &e) {
        //                 std::cout << "Cast Error" << std::endl;
        //                 return std::nullopt;
        //             } catch(const std::exception &e) {
        //                 std::cerr << "Exception was thrown\n"
        //                           << "what(): " << e.what() << "\n"
        //                           << "type: " << typeid(e).name() << std::endl;
        //                 return std::nullopt;
        //             }
        //         };
        //     });
        .def_rw("callback", &ServiceDescription::callback);

    nb::class_<ModuleDescription>(m, "ModuleDescription")
        .def(nb::init<>())
        .def_rw("namespace_name", &ModuleDescription::namespace_name)
        .def_rw("type_name", &ModuleDescription::type_name)
        .def_rw("services", &ModuleDescription::services);

    m.def("run_module_server", &run_module_server, "Run a module server",
          nb::arg("descr"), nb::arg("json_file"), nb::arg("to_registry"),
          nb::call_guard<nb::gil_scoped_release>());
}