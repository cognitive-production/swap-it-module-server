..
    Licensed under the MIT License.
    For details on the licensing terms, see the LICENSE file.
    SPDX-License-Identifier: MIT

    Copyright 2023-2024 (c) Fraunhofer IOSB (Author: Florian Düwel)


Object Types
==============

ModuleType
-----------------

The ModulType is an abstract ObjectType from which resource specific SubType can be derived. Each Subtype should represent
a specific class of modules in a generic way.

.. figure:: /images/ModuleType.PNG
   :alt: Definition of a resource specific SubType of the ModuleType

Capabilities
....................

Capabilities are restrictions of a resource, which are used to find suitable resource to execute services on the field level.
The CapabilityObject has no pre-defined references to variables, however, the common information model introduces the abstract **Capability_Struct_Data_Type**
as Subpertype for the **Capability_Struct_Boolean**, **Capability_Struct_Number** and **Capability_Struct_String**.
The capability node only allows variables as children, and these variables must point to a datatype, derived from the **Capability_Struct_Data_Type**.

Properties
..........

The PropertyObject provides static information of a resource and there are no restriction for further children. As starting point,
the PropertyObject references the an Asset- and AssetClass Object, as well as a Stationary Variable,
indicating whether a resource is able to move itself (e.g an autonomous guided vehicle). Properties dependent to a service or process execution, so that
it is user's decision whether to declare them or not.

Queue
..........

Contains a ServiceQueue Object that has a TypeDefinition of the QueueObjectType. This Type adds Methods to add, remove or change the
state of a queue entry. A corresponding functionality is provided with the open62541 server template ()

RegistrySubscription
....................

The variable Subscription_Objects is an array of BrowseNames of server Objects. The Objects specified within this list is mapped to a Device Registry
when a server registers itself and makes itself available to execute services. Important Objects for the this list are the Objects Capabilities and Queue.
Capabilities are required to filter suitable resource from the Device Registry. The Queue is required for the default resource assignment implemented in any Process agent.

Services
..........

The ModuleType predefines two services for each Module, which can be invoked to register or unregister a module within a DeviceRegsitry,
so that a server gets visible for process agents. Besides, the Services Object contains a registered variable that indicates whether the agent is already registered in a device registry or not.
When instantiating a SubType of the ModuleType, it has to be ensured that the resource's
service is added to the Services Object as a callable OPC UA Method.

State
..........
The state Object features two variables. The AssetState that with a variable type AssetStateType that indicates the current execution state of the resource. This variable is used by process agents to check,
whether the resource is available for service execution or not. In addition, the process agent interacts with this variable during service executions. The second variable is a location variable that indicates the position
of the resource within the shop floor, so that transport resources can find and drive to the resource.

ServiceFinishedEventType
----------------------------------
The abstract ServiceFinishedEventType is the base type for all service finished events that transmit the completion of a service execution to a process agent in case of a asynchronous service result.
The ServiceFinishedEventType contains three properties, the ServiceExecutionResult, that indicates whether the service execution was suceessful or not, a service uuid and a task uuid which are used for process agents to identify
the service and the task in which context the service was executed. these informations are important, since multiple process agent can be connected to a single resource and queue their services in the resource and wait for their specific
ServiceFinishedEvent.

.. figure:: /images/event.PNG
   :alt: alternate text