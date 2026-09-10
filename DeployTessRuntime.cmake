# Deploy the runtime layout expected by tessng().
#
# Debug uses the SDK's matching debug binaries. Release uses the installed
# TESSNG 4.1.3 runtime, which contains the complete Qt/QML/3D dependency set.

foreach(required DEPLOY_CONFIG DEBUG_SOURCE RELEASE_SOURCE DESTINATION)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
        message(FATAL_ERROR "DeployTessRuntime: missing ${required}")
    endif()
endforeach()

file(MAKE_DIRECTORY "${DESTINATION}")

if(DEPLOY_CONFIG STREQUAL "Debug")
    file(GLOB runtime_files LIST_DIRECTORIES false "${DEBUG_SOURCE}/*")
else()
    file(GLOB runtime_files LIST_DIRECTORIES false
        "${RELEASE_SOURCE}/*.dll"
        "${RELEASE_SOURCE}/*.qm"
        "${RELEASE_SOURCE}/*.db"
        "${RELEASE_SOURCE}/*.xml"
    )
endif()

foreach(runtime_file IN LISTS runtime_files)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${runtime_file}" "${DESTINATION}"
        RESULT_VARIABLE copy_result
    )
    if(NOT copy_result EQUAL 0)
        message(FATAL_ERROR "Failed to deploy ${runtime_file}")
    endif()
endforeach()

if(NOT DEPLOY_CONFIG STREQUAL "Debug")
    foreach(runtime_dir
            Cert
            plugins
            QtLocation
            QtPositioning
            QtQml
            QtQuick
            QtQuick.2
            tessplugins)
        if(EXISTS "${RELEASE_SOURCE}/${runtime_dir}")
            file(COPY "${RELEASE_SOURCE}/${runtime_dir}"
                 DESTINATION "${DESTINATION}")
        endif()
    endforeach()
endif()
