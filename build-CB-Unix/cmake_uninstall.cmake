if (NOT EXISTS "/home/point/OpenSource/Cataclysm-DDA/build-CB-Unix/install_manifest.txt")
    message(FATAL_ERROR
            "Cannot find install manifest: \"/home/point/OpenSource/Cataclysm-DDA/build-CB-Unix/install_manifest.txt\"")
endif ()

file(READ "/home/point/OpenSource/Cataclysm-DDA/build-CB-Unix/install_manifest.txt" FILE_LIST)
string(REGEX REPLACE "\n" ";" FILE_LIST "${FILE_LIST}")
foreach (FILE_NAME ${FILE_LIST})
    message(STATUS "Uninstalling \"$ENV{DESTDIR}${FILE_NAME}\"")
    if (EXISTS "$ENV{DESTDIR}${FILE_NAME}")
        exec_program(
                "/usr/bin/cmake" ARGS "-E remove \"$ENV{DESTDIR}${FILE_NAME}\""
                OUTPUT_VARIABLE RM_OUT
                RETURN_VALUE RM_RETVAL)
        if ("${RM_RETVAL}" STREQUAL 0)
        else ()
            message(FATAL_ERROR "Problem when removing \"$ENV{DESTDIR}${FILE_NAME}\"")
        endif ()
    else ()
        message(STATUS "File \"$ENV{DESTDIR}${FILE_NAME}\" does not exist.")
    endif ()
endforeach ()

