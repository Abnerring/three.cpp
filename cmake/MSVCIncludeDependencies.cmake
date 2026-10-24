# Newer MSVC emits UTF-8 /showIncludes text even on a Chinese Windows host.
# CMake versions decoding compiler detection with the system code page can
# cache a corrupt prefix. Ninja then silently records no header dependencies.
if(NOT MSVC OR NOT CMAKE_GENERATOR MATCHES "Ninja")
    return()
endif()
set(_threecpp_include_probe_dir "${CMAKE_BINARY_DIR}/CMakeFiles/threecpp-msvc-includes")
file(MAKE_DIRECTORY "${_threecpp_include_probe_dir}")
set(_threecpp_include_probe_header "${_threecpp_include_probe_dir}/dependency.h")
file(WRITE "${_threecpp_include_probe_header}" "#pragma once\n")
file(WRITE "${_threecpp_include_probe_dir}/dependency.cpp" "#include \"dependency.h\"\n")
execute_process(
    COMMAND "${CMAKE_CXX_COMPILER}" /nologo /showIncludes /Zs "${_threecpp_include_probe_dir}/dependency.cpp"
    OUTPUT_VARIABLE _threecpp_include_probe_output
    ERROR_VARIABLE _threecpp_include_probe_error
    RESULT_VARIABLE _threecpp_include_probe_result
    ENCODING UTF-8)
if(_threecpp_include_probe_result EQUAL 0)
    file(TO_NATIVE_PATH "${_threecpp_include_probe_header}" _threecpp_include_native_header)
    string(REPLACE "\r" "" _threecpp_include_probe_output "${_threecpp_include_probe_output}${_threecpp_include_probe_error}")
    string(REPLACE "\n" ";" _threecpp_include_probe_lines "${_threecpp_include_probe_output}")
    foreach(_threecpp_include_line IN LISTS _threecpp_include_probe_lines)
        string(FIND "${_threecpp_include_line}" "${_threecpp_include_native_header}" _threecpp_include_path_offset)
        if(_threecpp_include_path_offset GREATER 0)
            string(SUBSTRING "${_threecpp_include_line}" 0 ${_threecpp_include_path_offset} _threecpp_include_prefix)
            # A legacy compiler emitting the system encoding keeps CMake's
            # original detection rather than a lossy UTF-8 replacement.
            if(NOT _threecpp_include_prefix MATCHES "�")
                set(CMAKE_CXX_CL_SHOWINCLUDES_PREFIX "${_threecpp_include_prefix}")
                set(CMAKE_CL_SHOWINCLUDES_PREFIX "${_threecpp_include_prefix}")
            endif()
            break()
        endif()
    endforeach()
endif()
