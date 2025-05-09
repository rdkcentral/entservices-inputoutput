# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
set(PLUGIN_NAME HdmiCecSource)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

set(PLUGIN_HDMICECSOURCE_STARTUPORDER "" CACHE STRING "To configure startup order of HdmiCecSource plugin")
set_source_files_properties(HdmiCecSource.cpp PROPERTIES COMPILE_FLAGS "-fexceptions")

find_package(${NAMESPACE}Plugins REQUIRED)

add_library(${MODULE_NAME} SHARED
        HdmiCecSource.cpp
        Module.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

find_package(DS)
find_package(IARMBus)
find_package(CEC)

target_include_directories(${MODULE_NAME} PUBLIC ${URL_INCLUDE_DIR})
target_include_directories(${MODULE_NAME} PUBLIC ${DS_INCLUDE_DIRS})
target_include_directories(${MODULE_NAME} PUBLIC ${DSHAL_INCLUDE_DIRS})
target_include_directories(${MODULE_NAME} PUBLIC ${DSRPC_INCLUDE_DIRS})
target_include_directories(${MODULE_NAME} PUBLIC ${CEC_INCLUDE_DIRS})
target_include_directories(${MODULE_NAME} PUBLIC ${OSAL_INCLUDE_DIRS})
target_include_directories(${MODULE_NAME} PUBLIC ${CEC_HOST_INCLUDE_DIRS})
target_include_directories(${MODULE_NAME} PUBLIC ${CEC_IARM_INCLUDE_DIRS})
target_include_directories(${MODULE_NAME} PUBLIC ${IARMBUS_INCLUDE_DIRS})
target_include_directories(${MODULE_NAME} PUBLIC ${IARMIR_INCLUDE_DIRS})
target_include_directories(${MODULE_NAME} PUBLIC ${IARMRECEIVER_INCLUDE_DIRS})
target_include_directories(${MODULE_NAME} PUBLIC ${IARMPWR_INCLUDE_DIRS})
target_include_directories(${MODULE_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/helpers)

file(GLOB libraries ${DS_LIBRARIES}/*.so)
foreach(lib ${libraries})
        target_link_libraries(${MODULE_NAME} PUBLIC ${NAMESPACE}Plugins::${NAMESPACE}Plugins ${lib})
endforeach(lib)

file(GLOB libraries ${IARM_LIBRARIES}/*.so)
foreach(lib ${libraries})
        target_link_libraries(${MODULE_NAME} PUBLIC ${NAMESPACE}Plugins::${NAMESPACE}Plugins ${lib})
endforeach(lib)

file(GLOB libraries ${CEC_LIBRARIES}/*.so)
foreach(lib ${libraries})
        target_link_libraries(${MODULE_NAME} PUBLIC ${NAMESPACE}Plugins::${NAMESPACE}Plugins ${lib})
endforeach(lib)

if (NOT RDK_SERVICES_TEST)
    target_compile_options(${MODULE_NAME} PRIVATE -Wno-error=deprecated)
endif ()

install(TARGETS ${MODULE_NAME}
        DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
