..
    Licensed under the MIT License.
    For details on the licensing terms, see the LICENSE file.
    SPDX-License-Identifier: MIT

    Copyright 2023-2024 (c) Fraunhofer IOSB (Author: Florian Düwel)



Glossary
========

    PFDL Scheduler
        Generates a petri net out of a PFDL description of a process and schedules the specified services and tasks.

    Execution Engine
        Interface between the PFDL scheduler and field level resources that executes the scheduled services on the field level. In addition, it handles the parameter flow between services and tasks.

    Process Agent
        Agent consisting of a PFDL scheduler and an Execution Engine. It drives the execution of the process specified within a PFDL.

    Device Registry
        Registry Module where resource OPC UA server, representing field level devices, can register themself and thus, make themself available to execute PFDL services. In addition,
        the Device Registry has a build-in functionality to filter suitable resources for a service execution based on a resource's capabilities.

    Assignment Agent
        Entity that interacts with an Execution Engine and a Device Registry to assign a service to one concrete resource out of a set of possible resources.

    OPC UA Information Model
        Contains all nodes and references to map a specific entity to an OPC UA server.

    PFDL
      The Production Flow Description Language is a domain specific language that describes a production order. It is parsed by the scheduler and transformed into a structure to schedule the execution.