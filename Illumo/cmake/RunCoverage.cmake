if(NOT DEFINED TEST_BINARY OR NOT EXISTS "${TEST_BINARY}")
	message(FATAL_ERROR "TEST_BINARY is missing or does not exist: ${TEST_BINARY}")
endif()
if(NOT DEFINED BINARY_DIR OR NOT IS_DIRECTORY "${BINARY_DIR}")
	message(FATAL_ERROR "BINARY_DIR is missing or does not exist: ${BINARY_DIR}")
endif()
if(NOT DEFINED CTEST_COMMAND OR NOT DEFINED LLVM_PROFDATA OR NOT DEFINED LLVM_COV)
	message(FATAL_ERROR "Coverage tool paths were not supplied")
endif()
if(NOT DEFINED MINIMUM_LINE_COVERAGE)
	set(MINIMUM_LINE_COVERAGE 85)
endif()

file(TO_CMAKE_PATH "${BINARY_DIR}" NORMALIZED_BINARY_DIR)
set(PROFILE_DIR "${NORMALIZED_BINARY_DIR}/coverage-profiles")
set(REPORT_DIR "${NORMALIZED_BINARY_DIR}/coverage-html")
string(FIND "${PROFILE_DIR}" "${NORMALIZED_BINARY_DIR}/" PROFILE_PREFIX)
string(FIND "${REPORT_DIR}" "${NORMALIZED_BINARY_DIR}/" REPORT_PREFIX)
if(NOT PROFILE_PREFIX EQUAL 0 OR NOT REPORT_PREFIX EQUAL 0)
	message(FATAL_ERROR "Coverage output directories must remain inside BINARY_DIR")
endif()

file(REMOVE_RECURSE "${PROFILE_DIR}" "${REPORT_DIR}")
file(MAKE_DIRECTORY "${PROFILE_DIR}" "${REPORT_DIR}")

execute_process(
	COMMAND "${CMAKE_COMMAND}" -E env
		"LLVM_PROFILE_FILE=${PROFILE_DIR}/Illumo-%p.profraw"
		"${CTEST_COMMAND}" --test-dir "${BINARY_DIR}" -C "${CONFIG}"
		-L Illumo -j 4 --output-on-failure
	RESULT_VARIABLE TEST_RESULT
)
if(NOT TEST_RESULT EQUAL 0)
	message(FATAL_ERROR "Granular Illumo coverage tests failed")
endif()

file(GLOB PROFILE_FILES "${PROFILE_DIR}/Illumo-*.profraw")
list(LENGTH PROFILE_FILES PROFILE_COUNT)
if(PROFILE_COUNT EQUAL 0)
	message(FATAL_ERROR "No LLVM raw profiles were produced")
endif()

set(PROFILE_DATA "${NORMALIZED_BINARY_DIR}/Illumo.profdata")
execute_process(
	COMMAND "${LLVM_PROFDATA}" merge -sparse ${PROFILE_FILES} -o "${PROFILE_DATA}"
	RESULT_VARIABLE MERGE_RESULT
	ERROR_VARIABLE MERGE_ERROR
)
if(NOT MERGE_RESULT EQUAL 0)
	message(FATAL_ERROR "llvm-profdata failed: ${MERGE_ERROR}")
endif()

set(IGNORE_REGEX [=[(Source[/\\]Tests|thirdparty|Program Files|scoop|Microsoft Visual Studio|Windows Kits|Rendering[/\\]OpenGL|Rendering[/\\]Mock)]=])
execute_process(
	COMMAND "${LLVM_COV}" report "${TEST_BINARY}"
		"--instr-profile=${PROFILE_DATA}"
		"--ignore-filename-regex=${IGNORE_REGEX}"
	RESULT_VARIABLE REPORT_RESULT
	OUTPUT_VARIABLE COVERAGE_REPORT
	ERROR_VARIABLE REPORT_ERROR
)
if(NOT REPORT_RESULT EQUAL 0)
	message(FATAL_ERROR "llvm-cov report failed: ${REPORT_ERROR}")
endif()

message("${COVERAGE_REPORT}")
string(REGEX MATCH "TOTAL[^\r\n]*" TOTAL_LINE "${COVERAGE_REPORT}")
if(TOTAL_LINE STREQUAL "")
	message(FATAL_ERROR "Unable to find TOTAL in llvm-cov report")
endif()
string(REGEX REPLACE "[ \t]+" ";" TOTAL_FIELDS "${TOTAL_LINE}")
list(LENGTH TOTAL_FIELDS TOTAL_FIELD_COUNT)
if(TOTAL_FIELD_COUNT LESS 10)
	message(FATAL_ERROR "Unable to parse llvm-cov TOTAL line: ${TOTAL_LINE}")
endif()
list(GET TOTAL_FIELDS 9 LINE_COVERAGE_WITH_PERCENT)
string(REPLACE "%" "" LINE_COVERAGE "${LINE_COVERAGE_WITH_PERCENT}")
if(LINE_COVERAGE LESS MINIMUM_LINE_COVERAGE)
	message(FATAL_ERROR
		"Production line coverage ${LINE_COVERAGE}% is below ${MINIMUM_LINE_COVERAGE}%")
endif()

execute_process(
	COMMAND "${LLVM_COV}" show "${TEST_BINARY}"
		"--instr-profile=${PROFILE_DATA}"
		"--ignore-filename-regex=${IGNORE_REGEX}"
		-format=html
		"-output-dir=${REPORT_DIR}"
	RESULT_VARIABLE HTML_RESULT
	ERROR_VARIABLE HTML_ERROR
)
if(NOT HTML_RESULT EQUAL 0)
	message(FATAL_ERROR "llvm-cov HTML report failed: ${HTML_ERROR}")
endif()

message(STATUS "Illumo production line coverage: ${LINE_COVERAGE}%")
message(STATUS "Coverage gate passed: ${MINIMUM_LINE_COVERAGE}% minimum")
message(STATUS "HTML report: ${REPORT_DIR}/index.html")
