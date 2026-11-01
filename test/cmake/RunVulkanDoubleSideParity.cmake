foreach(required NODE PROBE PARITY_DIR UPSTREAM OUTPUT_ROOT)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
        message(FATAL_ERROR "Missing ${required}")
    endif()
endforeach()
file(MAKE_DIRECTORY "${OUTPUT_ROOT}")
string(TIMESTAMP run_stamp "%Y%m%dT%H%M%S" UTC)
string(RANDOM LENGTH 16 ALPHABET "0123456789abcdef" run_random)
set(run_output "${OUTPUT_ROOT}/${run_stamp}-${run_random}")
if(NOT DEFINED INPUT OR "${INPUT}" STREQUAL "")
    set(INPUT "${PARITY_DIR}/vulkan-double-side-fixtures.json")
endif()
execute_process(COMMAND "${NODE}" "${PARITY_DIR}/compare-vulkan-double-side.mjs"
    --probe "${PROBE}" --upstream "${UPSTREAM}" --output "${run_output}"
    --input "${INPUT}"
    RESULT_VARIABLE status)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "Vulkan double-side callback parity failed (${status}); evidence ${run_output}")
endif()
