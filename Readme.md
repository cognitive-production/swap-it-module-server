# Module Server

This python package provides a python interface to the [swap-it-open62541-server-template](https://github.com/FraunhoferIOSB/swap-it-open62541-server-template). It allows the deployment of a precompiled resource server for the swap-it-software architecture based on a json configuration file.

## Build
### Linux
Build and install the dependencies [open62541](https://github.com/open62541/open62541) and the [server-template](https://github.com/FraunhoferIOSB/swap-it-open62541-server-template). 
Init submodules:
``` shell
git submodule update --init --recursive
```
Install [scikit-build-core](https://github.com/scikit-build/scikit-build-core):
```
pip install scikit-build-core==0.2.2
```
Install [ninja-build](https://ninja-build.org/):
```
apt install ninja-build
```
Run:
``` shell
pip install .
```

## Usage
### Configuration
Create a json config file according to https://github.com/FraunhoferIOSB/swap-it-open62541-server-template/tree/main. Extend the file as follows:
``` json5
// config.json
{
    // ...
    // namespace uri of the server module
    "namespace": "https://cps.iwu.fraunhofer.de/UA/CpsDemo", 

    // Define the parameters of the service as "name": "type" pairs.
    // Valid types are: boolean, number and string
    // [1...n] input parameter(s)
    "input_params": {
        "order": "string",
        "speed": "number",
        "plot": "boolean"
    },
    // 1 output parameter
    "output_param": {
        "success": "boolean"
    }
}
```
### Service Implementation
Create the implementation of the service and run the module server:
``` python
# server.py
# Import server module
import swapit_module_server

# Create the service implementation
# The "input_params" are passed to the function as keyword arguments:
# use either **kwargs
def cb(**kwargs) -> bool:
    return True

# or explicit arguments
def cb(order: str, speed: float, plot: bool) -> bool:
    return True

# The return value must correspond to the "output_param" of the config, 
# i.e. boolean -> bool

# run the server
swapit_module_server.run("config.json", cb, True)
```