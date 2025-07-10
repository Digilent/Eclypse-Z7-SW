# Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc.  All rights reserved.
# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.16)

include(${CMAKE_CURRENT_SOURCE_DIR}/Empty_applicationExample.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/UserConfig.cmake)

set(APP_NAME mm2s_single_transfer_test)
project(${APP_NAME})

enable_language(C ASM CXX)
find_package(common)

# Needs to be changed
# set(BASE_PATH_DRIVERS "<base-path-to-drivers>/eclypse-z7/sw/ws/<platform-name>/hw/sdt/drivers/")

collect(PROJECT_LIB_DEPS xilstandalone;xiltimer)
collect(PROJECT_LIB_DEPS xil)
collect(PROJECT_LIB_DEPS gcc)
collect(PROJECT_LIB_DEPS c)

# Src dir source files
collect(PROJECT_LIB_SOURCES main.c)
collect(PROJECT_LIB_SOURCES platform.c)
collect(PROJECT_LIB_SOURCES awg_calibration.c)
collect(PROJECT_LIB_SOURCES mm2s_transfer.c)
collect(PROJECT_LIB_SOURCES test_stream_sink.c)
collect(PROJECT_LIB_SOURCES zmod_awg_axi_configuration.c)

# Drivers source files
collect(PROJECT_LIB_SOURCES ${BASE_PATH_DRIVERS}AxiStreamSinkMonitor_v1_0/src/AxiStreamSinkMonitor.c)
collect(PROJECT_LIB_SOURCES ${BASE_PATH_DRIVERS}AxiStreamSourceMonitor_v1_0/src/AxiStreamSourceMonitor.c)
collect(PROJECT_LIB_SOURCES ${BASE_PATH_DRIVERS}ManualTrigger_v1_0/src/ManualTrigger.c)
collect(PROJECT_LIB_SOURCES ${BASE_PATH_DRIVERS}TriggerControl_v1_0/src/TriggerControl.c)
collect(PROJECT_LIB_SOURCES ${BASE_PATH_DRIVERS}UserRegisters_v1_0/src/UserRegisters.c)
collect(PROJECT_LIB_SOURCES ${BASE_PATH_DRIVERS}ZmodAwgAxiConfiguration_v1_0/src/ZmodAwgAxiConfiguration.c)

# Dpmutil lib source files
collect(PROJECT_LIB_SOURCES dpmutil/dpmutil.c)
collect(PROJECT_LIB_SOURCES dpmutil/I2CHAL.c)
collect(PROJECT_LIB_SOURCES dpmutil/PlatformMCU.c)
collect(PROJECT_LIB_SOURCES dpmutil/syzygy.c)
collect(PROJECT_LIB_SOURCES dpmutil/Zmod.c)
collect(PROJECT_LIB_SOURCES dpmutil/ZmodADC.c)
collect(PROJECT_LIB_SOURCES dpmutil/ZmodDAC.c)
collect(PROJECT_LIB_SOURCES dpmutil/ZmodDigitizer.c)

collector_list(_sources PROJECT_LIB_SOURCES)
foreach(source ${_sources})
    get_filename_component(ext ${source} EXT)
    list(APPEND src_ext ${ext})
endforeach()

find_project_type("${src_ext}" PROJECT_TYPE)

if("${PROJECT_TYPE}" STREQUAL "c++")
    collect(PROJECT_LIB_DEPS stdc++)
endif()
    collector_list(_deps PROJECT_LIB_DEPS)
list (APPEND _deps ${USER_LINK_LIBRARIES})

if("${PROJECT_TYPE}" STREQUAL "c++")
    string(REPLACE ";" ",-l" _deps "${_deps}")
endif()

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
    set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

linker_gen("${CMAKE_CURRENT_SOURCE_DIR}/linker_files/")

string(APPEND CMAKE_C_FLAGS ${USER_COMPILE_OPTIONS})
string(APPEND CMAKE_CXX_FLAGS ${USER_COMPILE_OPTIONS})
string(APPEND CMAKE_C_LINK_FLAGS ${USER_LINK_OPTIONS})
string(APPEND CMAKE_CXX_LINK_FLAGS ${USER_LINK_OPTIONS})

add_dependency_on_bsp(_sources)

add_executable(${APP_NAME}.elf ${_sources})
set_target_properties(${APP_NAME}.elf PROPERTIES LINK_DEPENDS ${USER_LINKER_SCRIPT})
target_link_libraries(${APP_NAME}.elf
                      -Wl,-T -Wl,\"${USER_LINKER_SCRIPT}\"
                      -L\"${CMAKE_SOURCE_DIR}/\"
                      -L\"${CMAKE_LIBRARY_PATH}/\"
                      -L\"${USER_LINK_DIRECTORIES}/\"
                      -Wl,--start-group,-l${_deps} -Wl,--end-group
                      )
target_compile_definitions(${APP_NAME}.elf PUBLIC ${USER_COMPILE_DEFINITIONS})
target_include_directories(${APP_NAME}.elf PUBLIC
                           ${USER_INCLUDE_DIRECTORIES}
                           "dmputil"
                           ${BASE_PATH_DRIVERS}AxiStreamSinkMonitor_v1_0/src
                           ${BASE_PATH_DRIVERS}AxiStreamSourceMonitor_v1_0/src
                           ${BASE_PATH_DRIVERS}ManualTrigger_v1_0/src
                           ${BASE_PATH_DRIVERS}TriggerControl_v1_0/src
                           ${BASE_PATH_DRIVERS}UserRegisters_v1_0/src
                           ${BASE_PATH_DRIVERS}ZmodAwgAxiConfiguration_v1_0/src                 
                           )
print_elf_size(CMAKE_SIZE ${APP_NAME})
