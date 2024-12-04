#pragma once

#include <functional>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace swapit {

using PfdlVariant = std::variant<bool, double, std::string>;

enum class PfdlType : int { boolean = 0, number, string };

struct Parameter {
    std::string name;
    PfdlType type;
};

struct Argument {
    std::string name;
    PfdlVariant value;
};

using ServiceCallback =
    std::function<std::optional<PfdlVariant>(const std::vector<Argument> &args)>;

struct ServiceDescription {
    std::string name;
    std::vector<Parameter> input_params;
    Parameter output_param;
    ServiceCallback callback;
};

struct ModuleDescription {
    std::string namespace_name;
    std::string type_name;
    std::vector<ServiceDescription> services;
};

void
run_module_server(ModuleDescription descr, const std::string &json_file,
                  bool to_registry);

}  // namespace swapit