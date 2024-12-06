..
    Licensed under the MIT License.
    For details on the licensing terms, see the LICENSE file.
    SPDX-License-Identifier: MIT

    Copyright 2023-2024 (c) Fraunhofer IOSB (Author: Florian Düwel)

===========
Structures
===========

Capability_Struct_Data_Type
=================================
Source DataType to express resource Capabilities. It introduces the Field RelationalOperator that has Enumeration OperatorType as DataType. The
DataType itself should never be used for a variable's attribute, however it is the source for a set of
structures which can be used to express capabilities of a resource.

.. figure:: /images/capabilityStruct.PNG
   :alt: alternate text

Capability_Struct_Boolean
---------------------------
Expresses a boolean Capability of a resource and requires the OperatorType to be either "IsTrue" or "IsFalse"

.. figure:: /images/capabilitystructboolean.PNG
   :alt: alternate text

Capability_Struct_Number
---------------------------
Expresses a numeric Capability of a resource and requires the OperatorType to be either "Greater", "Smaller", "Equal", "GreaterOrEqual" or "SmallerOrEqual"

.. figure:: /images/capabilitystructNumber.PNG
   :alt: alternate text

Capability_Struct_String
---------------------------
Expresses a string-based Capability and requires the OperatorType to be "EqualString"

.. figure:: /images/capabilitystructString.PNG
   :alt: alternate text

QueueDataType
==============
The queue data type contains several fields to identify a product within an order, the corresponding service of the process agent as well as a client identifier to identify the client of a process agent, that added th queue element.
In addition, each queue element has an individual state field that indicates the execution status of the queue element an lastly, a Array to store service parameters of the queue element that can be used for the resource parameterization before the
execution of the queue entry.

.. figure:: /images/QueueDataType.PNG
   :alt: alternate text

ServiceExecutionResult
============================
The SWAP-IT software modules enable a synchronous or an asynchronous service result, which means, the service result can be either transmitted directly from the service method in a synchronous case, or at a later
point in time with a service finished event in a asynchronous case. Depending on the case, the output parameter of the service method is either of the data type ServiceFinishedSyncDataType or ServiceFinishedAsyncDataType.

ServiceFinishedSyncDataType
----------------------------
.. figure:: /images/ServiceSync.PNG
   :alt: alternate text

ServiceFinishedAsyncDataType
-----------------------------
.. figure:: /images/ServiceAsync.PNG
   :alt: alternate text