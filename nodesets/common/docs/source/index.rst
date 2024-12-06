..
    Licensed under the MIT License.
    For details on the licensing terms, see the LICENSE file.
    SPDX-License-Identifier: MIT

    Copyright 2023-2024 (c) Fraunhofer IOSB (Author: Florian Düwel)


Common Information Model
========================

The common information model is an OCP UA information model to make OPC UA Server compatible with the SWAP-IT software. In this context,
the common information model provides all OPC UA BaseTypes that a resource requires to interact with an Execution Engine and thus, become available
for PFDL-based Process executions. Based on the common model, an resource specific information model has to be derived for each resource class, which
defines a SubType of the ModuleType. All references to Object and Variables within the Moduletype become mandatory for the SubType. In addition, a
resource specific Method Node has to be added to the ServiceObject of the SubType, where the BrowseName of the Method corresponds to the resource's service name within PFDL process descriptions.
This SubType has to be instantiated into the OPC UA Server that interacts with the resource. The figure below only illustrates all referenced Objects from the ModuleType. Further references from these
Objects are not part of the figure and are explained in more detail in the following sections
All graphical representations in this documentation are based on the graphical notation defined in OPC UA Specification Part 3.



.. figure:: /images/ModuleType.PNG
   :alt: Definition of a resource specific SubType of the ModuleType


.. toctree::
   :maxdepth: 2

   custom_data_types
   object_types
   related_projects
   glossary
   contact

