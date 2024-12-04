from ._module_server_impl import (
    ServiceDescription,
    ModuleDescription,
    PfdlType,
    Parameter,
    run_module_server,
)

import json
from typing import Callable, Union
import traceback

PfdlTypes = Union[type(None), bool, float, str]

ServiceCallable = Callable[..., PfdlTypes]

PFDL_TYPE_DICT = {
    "boolean": PfdlType.boolean,
    "number": PfdlType.number,
    "string": PfdlType.string,
}


def run(json_file: str, callback: ServiceCallable, to_registry: bool):
    with open(json_file, "r") as f:
        config = json.load(f)

    def parse_params(p):
        return [Parameter(n, PFDL_TYPE_DICT[t]) for n, t in p.items()]

    def check_type(o):
        if not isinstance(o, PfdlTypes):
            raise ValueError(f"Return type must be an instance of: {PfdlTypes}")
        return o

    def cb_wrapper(args):
        try:
            kwargs = {a.name: a.value for a in args}
            return check_type(callback(**kwargs))
        except Exception:
            traceback.print_exc()
            return None

    service = ServiceDescription()
    service.name = config["service_name"]
    if len(config["input_params"]) < 1:
        raise TypeError("Service must provide at least one input parameter")

    service.input_params = parse_params(config["input_params"])
    service.output_param = parse_params(config["output_param"])[0]
    service.callback = cb_wrapper

    module = ModuleDescription()
    module.type_name = config["module_type"]
    module.namespace_name = config["namespace"]
    module.services = [service]

    run_module_server(module, json_file, to_registry)
