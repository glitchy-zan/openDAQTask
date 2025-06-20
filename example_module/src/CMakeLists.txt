set(LIB_NAME example_module)
set(MODULE_HEADERS_DIR ../include/${TARGET_FOLDER_NAME})

set(SRC_Include common.h
                module_dll.h
                example_module.h
                example_fb.h
                example_iir_filter_fb.h
)

set(SRC_Srcs module_dll.cpp
             example_module.cpp
             example_fb.cpp
             example_iir_filter_fb.cpp
)

prepend_include(${TARGET_FOLDER_NAME} SRC_Include)

source_group("module" FILES ${MODULE_HEADERS_DIR}/example_module.h
                            ${MODULE_HEADERS_DIR}/example_fb.h
                            ${MODULE_HEADERS_DIR}/module_dll.h
                            ${MODULE_HEADERS_DIR}/example_iir_filter_fb.h
                            module_dll.cpp
                            example_module.cpp
                            example_fb.cpp
                            example_iir_filter_fb.cpp
)


add_library(${LIB_NAME} SHARED ${SRC_Include}
                               ${SRC_Srcs}
)

add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

if (MSVC)
    target_compile_options(${LIB_NAME} PRIVATE /bigobj)
endif()

target_link_libraries(${LIB_NAME} PUBLIC daq::opendaq
)

target_include_directories(${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
                                              $<INSTALL_INTERFACE:include>
)

opendaq_set_module_properties(${LIB_NAME} ${PROJECT_VERSION_MAJOR})
create_version_header(${LIB_NAME})
