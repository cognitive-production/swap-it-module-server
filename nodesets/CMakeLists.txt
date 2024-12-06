set(NODESET_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/generated_src")
set(UA_GENERATED_OUTPUT_DIR "${NODESET_INCLUDE_DIR}/nodesets")

# common nodeset
set(NODESET_COMMON_DIR "${CMAKE_CURRENT_LIST_DIR}/common")

ua_generate_nodeset_and_datatypes(
    NAME "common"
    FILE_BSD "${NODESET_COMMON_DIR}/SWAP.Fraunhofer.Common.Model.Types.bsd"
    FILE_CSV "${NODESET_COMMON_DIR}/CommonModelDesign.csv"
    FILE_NS "${NODESET_COMMON_DIR}/SWAP.Fraunhofer.Common.Model.NodeSet2.xml"
    NAMESPACE_MAP "2:http://common.swap.fraunhofer.de"
    OUTPUT_DIR ${UA_GENERATED_OUTPUT_DIR}
    INTERNAL
)

# pfdl_types nodeset
set(NODESET_PFDL_DIR "${CMAKE_CURRENT_LIST_DIR}/pfdl")

ua_generate_nodeset_and_datatypes(
    NAME "pfdl"
    FILE_BSD "${NODESET_PFDL_DIR}/Pfdl.Types.bsd"
    FILE_CSV "${NODESET_PFDL_DIR}/Pfdl.NodeIds.csv"
    FILE_NS "${NODESET_PFDL_DIR}/Pfdl.NodeSet2.xml"
    NAMESPACE_MAP "3:http://cps.iwu.fraunhofer.de/UA/Pfdl/"
    OUTPUT_DIR ${UA_GENERATED_OUTPUT_DIR}
    INTERNAL
)

set(UA_GENERATED_SOURCES
    ${UA_NODESET_COMMON_SOURCES}
    ${UA_TYPES_COMMON_SOURCES}
    ${UA_NODESET_PFDL_SOURCES}
    ${UA_TYPES_PFDL_SOURCES}
)

set(UA_GENERATED_DEPS
    open62541-generator-ids-common
    open62541-generator-ids-pfdl
)

add_library(nodesets STATIC ${UA_GENERATED_SOURCES})
set_property(TARGET nodesets PROPERTY POSITION_INDEPENDENT_CODE ON)

add_dependencies(nodesets ${UA_GENERATED_DEPS})
target_link_libraries(nodesets PRIVATE open62541)
target_include_directories(nodesets PUBLIC ${NODESET_INCLUDE_DIR})
