cmake_minimum_required(VERSION 3.13)


find_package(Git QUIET)
if(GIT_FOUND)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_VERSION
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
endif()
if(GIT_VERSION)
	string(REGEX REPLACE "^v?([0-9]+\\.[0-9]+(\\.[0-9]+)?).*\$" "\\1" GIT_VERSION_TRIMMED ${GIT_VERSION})
else()
	set(GIT_VERSION "unknown-rev")
	set(GIT_VERSION_TRIMMED "0.0.0")
endif()

message(STATUS "Version: ${GIT_VERSION}")

project(guest-vm-sync-time-rtc VERSION ${GIT_VERSION_TRIMMED})

add_executable(${PROJECT_NAME} main.c)
target_compile_definitions(${PROJECT_NAME} PRIVATE _DEFAULT_SOURCE)


set(CPACK_GENERATOR TGZ)
set(CPACK_STRIP_FILES ON)

include(CPack)
