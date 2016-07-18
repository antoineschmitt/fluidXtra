# Install script for directory: /Users/antoine/Documents/Code/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth

# Set the install prefix
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "/usr/local")
ENDIF(NOT DEFINED CMAKE_INSTALL_PREFIX)
STRING(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
IF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  IF(BUILD_TYPE)
    STRING(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  ELSE(BUILD_TYPE)
    SET(CMAKE_INSTALL_CONFIG_NAME "Release")
  ENDIF(BUILD_TYPE)
  MESSAGE(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
ENDIF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)

# Set the component getting installed.
IF(NOT CMAKE_INSTALL_COMPONENT)
  IF(COMPONENT)
    MESSAGE(STATUS "Install component: \"${COMPONENT}\"")
    SET(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  ELSE(COMPONENT)
    SET(CMAKE_INSTALL_COMPONENT)
  ENDIF(COMPONENT)
ENDIF(NOT CMAKE_INSTALL_COMPONENT)

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/fluidsynth" TYPE FILE FILES
    "/Users/antoine/Documents/Code/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/audio.h"
    "/Users/antoine/Documents/Code/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/event.h"
    "/Users/antoine/Documents/Code/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/gen.h"
    "/Users/antoine/Documents/Code/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/log.h"
    "/Users/antoine/Documents/Code/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/midi.h"
    "/Users/antoine/Documents/Code/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/misc.h"
    "/Users/antoine/Documents/Code/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/mod.h"
    "/Users/antoine/Documents/Code/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/ramsfont.h"
    "/Users/antoine/Documents/Code/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/seq.h"
    "/Users/antoine/Documents/Code/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/seqbind.h"
    "/Users/antoine/Documents/Code/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/settings.h"
    "/Users/antoine/Documents/Code/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/sfont.h"
    "/Users/antoine/Documents/Code/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/shell.h"
    "/Users/antoine/Documents/Code/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/synth.h"
    "/Users/antoine/Documents/Code/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/types.h"
    "/Users/antoine/Documents/Code/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/voice.h"
    "/Users/antoine/Documents/Code/fluidXtra/src/fluidsynth-1.1.5/build_mac/include/fluidsynth/version.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

