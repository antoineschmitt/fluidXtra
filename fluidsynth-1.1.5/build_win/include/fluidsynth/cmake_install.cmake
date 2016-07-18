# Install script for directory: C:/Documents and Settings/Antoine/Mes documents/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth

# Set the install prefix
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "C:/Program Files/FluidSynth")
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
    "C:/Documents and Settings/Antoine/Mes documents/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/audio.h"
    "C:/Documents and Settings/Antoine/Mes documents/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/event.h"
    "C:/Documents and Settings/Antoine/Mes documents/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/gen.h"
    "C:/Documents and Settings/Antoine/Mes documents/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/log.h"
    "C:/Documents and Settings/Antoine/Mes documents/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/midi.h"
    "C:/Documents and Settings/Antoine/Mes documents/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/misc.h"
    "C:/Documents and Settings/Antoine/Mes documents/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/mod.h"
    "C:/Documents and Settings/Antoine/Mes documents/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/ramsfont.h"
    "C:/Documents and Settings/Antoine/Mes documents/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/seq.h"
    "C:/Documents and Settings/Antoine/Mes documents/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/seqbind.h"
    "C:/Documents and Settings/Antoine/Mes documents/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/settings.h"
    "C:/Documents and Settings/Antoine/Mes documents/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/sfont.h"
    "C:/Documents and Settings/Antoine/Mes documents/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/shell.h"
    "C:/Documents and Settings/Antoine/Mes documents/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/synth.h"
    "C:/Documents and Settings/Antoine/Mes documents/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/types.h"
    "C:/Documents and Settings/Antoine/Mes documents/fluidXtra/src/fluidsynth-1.1.5/include/fluidsynth/voice.h"
    "C:/Documents and Settings/Antoine/Mes documents/fluidXtra/src/fluidsynth-1.1.5/build_win/include/fluidsynth/version.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

