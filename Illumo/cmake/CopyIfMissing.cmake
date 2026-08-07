if(NOT DEFINED SOURCE OR NOT DEFINED DESTINATION)
    message(FATAL_ERROR "CopyIfMissing.cmake requires SOURCE and DESTINATION")
endif()

if(NOT EXISTS "${DESTINATION}")
    get_filename_component(destinationDirectory "${DESTINATION}" DIRECTORY)
    file(MAKE_DIRECTORY "${destinationDirectory}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy "${SOURCE}" "${DESTINATION}"
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()
