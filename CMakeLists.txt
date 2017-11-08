cmake_minimum_required (VERSION 2.6)

project (iq_amp_control)

IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  SET(CMAKE_INSTALL_PREFIX /usr/local CACHE PATH TesTest FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

add_executable(ir_event ./src/ir_event.c)
target_link_libraries (ir_event mpdclient pthread)

add_executable(mechanical_controls ./src/mechanical_controls.c)
target_link_libraries (mechanical_controls mpdclient pigpio pthread)

INSTALL(TARGETS ir_event RUNTIME DESTINATION bin)
INSTALL(DIRECTORY files/systemd/ DESTINATION /lib/systemd/system FILES_MATCHING PATTERN "*.service")
INSTALL(DIRECTORY files/remotes/ DESTINATION /etc/rc_keymaps FILES_MATCHING PATTERN "*")

INSTALL(TARGETS mechanical_controls RUNTIME DESTINATION bin)

SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libevent-2.0-5 (>= 2.0.21-stable-3), libmpdclient2 (>= 2.9-1), pigpio (>= 1.60-1)")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "IQ Amp Controls: Control MusicPD via IR events and mechanical inputs")
SET(CPACK_PACKAGE_CONTACT "Christoph Appel")
SET(CPACK_PACKAGING_INSTALL_PREFIX /usr/local)
SET(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/CMake/debian/postinst;${CMAKE_CURRENT_SOURCE_DIR}/CMake/debian/prerm;${CMAKE_CURRENT_SOURCE_DIR}/CMake/debian/postrm;" )

SET(CPACK_GENERATOR "DEB")

INCLUDE(CPack)
