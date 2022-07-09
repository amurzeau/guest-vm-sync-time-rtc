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
if(GIT_VERSION AND GIT_VERSION MATCHES "^v")
	string(REGEX REPLACE "^v([0-9]+\\.[0-9]+(\\.[0-9]+)?).*\$" "\\1" GIT_VERSION_TRIMMED ${GIT_VERSION})
else()
	set(GIT_VERSION "unknown-rev")
	set(GIT_VERSION_TRIMMED "0.0.0")
endif()

message(STATUS "Version: ${GIT_VERSION}")

project(guest-vm-sync-time-rtc VERSION ${GIT_VERSION_TRIMMED})

add_executable(${PROJECT_NAME} main.c)
target_compile_definitions(${PROJECT_NAME} PRIVATE _DEFAULT_SOURCE)
target_compile_definitions(${PROJECT_NAME} PRIVATE "VERSION=\"${GIT_VERSION}\"")

install(TARGETS ${PROJECT_NAME} DESTINATION usr/sbin/)
install(FILES guest-vm-sync-time-with-rtc.service DESTINATION lib/systemd/system/)

set(CMAKE_PROJECT_HOMEPAGE_URL "https://github.com/amurzeau/guest-vm-sync-time-rtc")
set(CPACK_GENERATOR TGZ DEB)
set(CPACK_PACKAGE_CONTACT "Alexis Murzeau <amubtdx@gmail.com>")
set(CPACK_PACKAGE_DESCRIPTION_FILE ${CMAKE_CURRENT_LIST_DIR}/debian_files/description)
set(CPACK_PACKAGING_INSTALL_PREFIX "/")
set(CPACK_DEBIAN_PACKAGE_RELEASE "1~upstream1")
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_DEBIAN_PACKAGE_SECTION "admin")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    ${CMAKE_CURRENT_LIST_DIR}/debian_files/postinst
    ${CMAKE_CURRENT_LIST_DIR}/debian_files/prerm
    ${CMAKE_CURRENT_LIST_DIR}/debian_files/postrm
)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)
set(CPACK_DEBIAN_DEBUGINFO_PACKAGE ON)

include(CPack)
