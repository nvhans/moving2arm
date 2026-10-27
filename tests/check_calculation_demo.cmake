if(NOT DEFINED PROGRAM)
    message(FATAL_ERROR "PROGRAM must name a calculation demo executable")
endif()

execute_process(COMMAND ${EMULATOR} "${PROGRAM}"
    RESULT_VARIABLE result OUTPUT_VARIABLE actual ERROR_VARIABLE errors TIMEOUT 20)
if(NOT "${result}" STREQUAL "0")
    message(FATAL_ERROR "Calculation demo failed (${result}):\n${errors}")
endif()
string(REPLACE "\r\n" "\n" actual "${actual}")
set(expected [=[Element 0: 1.00 is not larger than 1.00
Element 1: 4.00 is larger than 2.00
Element 2: 9.00 is larger than 3.00
Element 3: 16.00 is larger than 4.00
Addition Result: 2.000000 6.000000 12.000000 20.000000
Multiplication Result: 2.000000 12.000000 36.000000 80.000000
Square Root Result: 1.414214 3.464102 6.000000 8.944272
]=])
if(NOT "${actual}" STREQUAL "${expected}")
    message(FATAL_ERROR "Calculation demo output differs.\nExpected:\n${expected}\nActual:\n${actual}")
endif()
