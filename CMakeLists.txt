cmake_minimum_required(VERSION 3.21)
project(XionganTrafficNativeCpp VERSION 1.0.0 LANGUAGES CXX)

# This native performance build is intentionally Release-only. Keeping a
# single configuration prevents Visual Studio's CMake workspace from silently
# falling back to an incompatible Debug TESSNG/Qt runtime.
if(MSVC AND CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING
        "Supported build configurations" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

# 与 TESSNG 4.1.3 官方 C++ 示例相同：Qt、头文件、Debug/Release 库分别配置。
# 命令行可通过 -DQT_ROOT=... -DTESSNG_SDK_ROOT=... 覆盖本机默认路径。
set(QT_ROOT "F:/Qt/5.15.2/msvc2019_64" CACHE PATH "Qt 5.15.2 MSVC x64 root")
set(TESSNG_SDK_ROOT "F:/TESSNG_4.1.3" CACHE PATH "TESSNG 4.1.3 SDK root")

if(DEFINED ENV{QT_ROOT} AND NOT "$ENV{QT_ROOT}" STREQUAL "")
    set(QT_ROOT "$ENV{QT_ROOT}" CACHE PATH "Qt 5.15.2 MSVC x64 root" FORCE)
endif()
if(DEFINED ENV{TESSNG_ROOT} AND NOT "$ENV{TESSNG_ROOT}" STREQUAL "")
    set(TESSNG_SDK_ROOT "$ENV{TESSNG_ROOT}" CACHE PATH "TESSNG 4.1.3 SDK root" FORCE)
endif()

list(PREPEND CMAKE_PREFIX_PATH "${QT_ROOT}")
find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets)

set(TESSNG_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(TESSNG_LIB_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/lib/debug/TessInterfaces.lib")
set(TESSNG_LIB_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/lib/release/TessInterfaces.lib")
set(TESSNG_CPP_TEMPLATE_ROOT
    "${TESSNG_SDK_ROOT}/SecondDev/TESS_CppAPI_BATCH_COMMANDS_EXAMPLE")
set(TESSNG_RUNTIME_DEBUG
    "${TESSNG_CPP_TEMPLATE_ROOT}/x64/Debug")
set(TESSNG_RUNTIME_RELEASE "${TESSNG_SDK_ROOT}")

foreach(required_path
        "${TESSNG_INCLUDE_DIR}/tessinterface.h"
        "${TESSNG_INCLUDE_DIR}/Plugin/tessplugin.h"
        "${TESSNG_LIB_DEBUG}"
        "${TESSNG_LIB_RELEASE}")
    if(NOT EXISTS "${required_path}")
        message(FATAL_ERROR "Missing TESSNG SDK file: ${required_path}")
    endif()
endforeach()

if(NOT EXISTS "${QT_ROOT}/bin/qmake.exe")
    message(FATAL_ERROR "QT_ROOT is invalid: ${QT_ROOT}")
endif()
if(NOT EXISTS "${TESSNG_RUNTIME_RELEASE}/TessInterfaces.dll")
    message(FATAL_ERROR "Release runtime is incomplete: ${TESSNG_RUNTIME_RELEASE}")
endif()

add_executable(XionganTrafficNativeCpp WIN32
    src/main.cpp
    src/MyPlugin.cpp
    src/MyNet.cpp
    src/MySimulator.cpp
    src/core/ProjectModel.cpp
    src/core/TrafficAggregator.cpp
    src/core/SignalEngine.cpp
    src/core/RoutingEngine.cpp
    src/core/DemandEngine.cpp
    src/core/CooperativeLaneBalancer.cpp
    src/core/MetricsRecorder.cpp
    src/ui/ControlPanel.cpp
    src/ui/IntersectionDetailPanel.cpp
    src/ui/IntersectionInteractionLayer.cpp
    src/ui/TelemetryPanel.cpp
    src/MyPlugin.h
    src/MyNet.h
    src/MySimulator.h
    src/core/ProjectModel.h
    src/core/TrafficAggregator.h
    src/core/SignalEngine.h
    src/core/RoutingEngine.h
    src/core/DemandEngine.h
    src/core/CooperativeLaneBalancer.h
    src/core/MetricsRecorder.h
    src/ui/ControlPanel.h
    src/ui/IntersectionDetailPanel.h
    src/ui/IntersectionInteractionLayer.h
    src/ui/TelemetryPanel.h
)

target_include_directories(XionganTrafficNativeCpp PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${TESSNG_INCLUDE_DIR}"
)

target_link_libraries(XionganTrafficNativeCpp PRIVATE
    Qt5::Core Qt5::Gui Qt5::Widgets
    debug "${TESSNG_LIB_DEBUG}"
    optimized "${TESSNG_LIB_RELEASE}"
)

if(MSVC)
    target_compile_options(XionganTrafficNativeCpp PRIVATE
        /utf-8
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Release>:/GL>
        $<$<CONFIG:Release>:/Gy>
        $<$<CONFIG:Release>:/Gw>
    )
    target_link_options(XionganTrafficNativeCpp PRIVATE
        $<$<CONFIG:Release>:/LTCG>
        $<$<CONFIG:Release>:/OPT:REF>
        $<$<CONFIG:Release>:/OPT:ICF>
    )
endif()

set_target_properties(XionganTrafficNativeCpp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin/Release"
    PDB_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/pdb"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VS_DEBUGGER_ENVIRONMENT
        "PATH=$<TARGET_FILE_DIR:XionganTrafficNativeCpp>;$<$<CONFIG:Debug>:${TESSNG_RUNTIME_DEBUG}>$<$<CONFIG:Release>:${TESSNG_RUNTIME_RELEASE}>;${QT_ROOT}/bin;%PATH%\nQT_PLUGIN_PATH=$<IF:$<CONFIG:Debug>,${QT_ROOT}/plugins,$<TARGET_FILE_DIR:XionganTrafficNativeCpp>/plugins>\nQML2_IMPORT_PATH=$<TARGET_FILE_DIR:XionganTrafficNativeCpp>\nQT_ROOT=${QT_ROOT}\nTESSNG_ROOT=${TESSNG_SDK_ROOT}"
)

# tessng() does not locate TESS_WIN.dll only through PATH.  It resolves the
# native TESS NG modules from QCoreApplication::applicationDirPath(), so the
# matching Debug/Release runtime must be beside our executable.  The official
# example also runs with this layout (its exe and TESS_WIN.dll share x64/<cfg>).
# Deploy the matching runtime because TESS_WIN loads further TESS_*.dll,
# Qt/QML modules and data files dynamically after the first entry point.

# 生成 Visual Studio 解决方案时，将仿真程序而非自检程序设为启动项。
set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    PROPERTY VS_STARTUP_PROJECT XionganTrafficNativeCpp)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src"
    FILES
        src/main.cpp
        src/MyPlugin.cpp src/MyPlugin.h
        src/MyNet.cpp src/MyNet.h
        src/MySimulator.cpp src/MySimulator.h
        src/core/ProjectModel.cpp src/core/ProjectModel.h
        src/core/TrafficAggregator.cpp src/core/TrafficAggregator.h
        src/core/SignalEngine.cpp src/core/SignalEngine.h
        src/core/RoutingEngine.cpp src/core/RoutingEngine.h
        src/core/DemandEngine.cpp src/core/DemandEngine.h
        src/core/CooperativeLaneBalancer.cpp src/core/CooperativeLaneBalancer.h
        src/core/MetricsRecorder.cpp src/core/MetricsRecorder.h
        src/ui/ControlPanel.cpp src/ui/ControlPanel.h
        src/ui/IntersectionDetailPanel.cpp src/ui/IntersectionDetailPanel.h
        src/ui/IntersectionInteractionLayer.cpp src/ui/IntersectionInteractionLayer.h
        src/ui/TelemetryPanel.cpp src/ui/TelemetryPanel.h
)

add_custom_command(TARGET XionganTrafficNativeCpp POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
        "-DDEPLOY_CONFIG=$<CONFIG>"
        "-DDEBUG_SOURCE=${TESSNG_RUNTIME_DEBUG}"
        "-DRELEASE_SOURCE=${TESSNG_RUNTIME_RELEASE}"
        "-DDESTINATION=$<TARGET_FILE_DIR:XionganTrafficNativeCpp>"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/DeployTessRuntime.cmake"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
        "$<TARGET_FILE_DIR:XionganTrafficNativeCpp>/config.json"
    COMMENT "Deploying the matching TESSNG 4.1.3 runtime beside the executable"
)

enable_testing()
add_executable(NativeCoreSelfTest
    tests/NativeCoreSelfTest.cpp
    src/core/ProjectModel.cpp
    src/core/TrafficAggregator.cpp
    src/core/SignalEngine.cpp
    src/core/RoutingEngine.cpp
    src/core/DemandEngine.cpp
    src/core/MetricsRecorder.cpp
)
target_include_directories(NativeCoreSelfTest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${TESSNG_INCLUDE_DIR}"
)
target_link_libraries(NativeCoreSelfTest PRIVATE
    Qt5::Core Qt5::Gui Qt5::Widgets
    optimized "${TESSNG_LIB_RELEASE}"
    debug "${TESSNG_LIB_DEBUG}"
)
if(MSVC)
    target_compile_options(NativeCoreSelfTest PRIVATE
        /utf-8 $<$<CONFIG:Release>:/O2>)
endif()
add_test(NAME NativeCoreSelfTest
    COMMAND NativeCoreSelfTest "${CMAKE_CURRENT_SOURCE_DIR}"
)
