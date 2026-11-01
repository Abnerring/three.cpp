foreach(required NODE PROBE PARITY_DIR UPSTREAM OUTPUT_ROOT)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "Missing ${required}")
    endif()
endforeach()
string(TIMESTAMP stamp "%Y%m%dT%H%M%SZ" UTC)
string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef suffix)
set(output "${OUTPUT_ROOT}-${stamp}-${suffix}")
execute_process(COMMAND "${NODE}" --expose-gc "${PARITY_DIR}/compare-function-overloading-extra.mjs"
    --probe "${PROBE}" --upstream "${UPSTREAM}" --output "${output}"
    RESULT_VARIABLE code)
if(NOT code EQUAL 0)
    message(FATAL_ERROR "Strict FunctionOverloading extra parity failed; original evidence retained at ${output}")
endif()
