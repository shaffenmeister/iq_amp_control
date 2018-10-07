cmake_minimum_required (VERSION 2.6)

project (iq_amp_control_mpd)

SET(iq_amp_control_mpd_VERSION_MAJOR 0)
SET(iq_amp_control_mpd_VERSION_MINOR 2)

IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  SET(CMAKE_INSTALL_PREFIX /usr/local CACHE PATH TesTest FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

add_executable(ir_event_mpd ./src/ir_event_mpd.c)
target_link_libraries (ir_event_mpd mpdclient pthread)

add_executable(mechanical_controls_mpd ./src/mechanical_controls_mpd.c)
target_link_libraries (mechanical_controls_mpd mpdclient pigpio pthread)

INSTALL(TARGETS ir_event_mpd RUNTIME DESTINATION bin)
INSTALL(DIRECTORY files/systemd/ DESTINATION /lib/systemd/system FILES_MATCHING PATTERN "*.service")
INSTALL(DIRECTORY files/remotes/ DESTINATION /etc/rc_keymaps FILES_MATCHING PATTERN "*")

INSTALL(TARGETS mechanical_controls_mpd RUNTIME DESTINATION bin)

SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libevent-2.0-5 (>= 2.0.21-stable-3), libmpdclient2 (>= 2.9-1), pigpio (>= 1.60-1)")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "IQ Amp Controls: Control MusicPD via IR events and mechanical inputs")
SET(CPACK_PACKAGE_CONTACT "Christoph Appel")
SET(CPACK_PACKAGING_INSTALL_PREFIX /usr/local)
SET(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/CMake/debian/postinst;${CMAKE_CURRENT_SOURCE_DIR}/CMake/debian/prerm;${CMAKE_CURRENT_SOURCE_DIR}/CMake/debian/postrm;" )
SET(CPACK_PACKAGE_VERSION_MAJOR "${iq_amp_control_mpd_VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${iq_amp_control_mpd_VERSION_MINOR}")

SET(CPACK_GENERATOR "DEB")

INCLUDE(CPack)
