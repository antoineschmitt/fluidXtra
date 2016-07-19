# Install script for directory: C:/Users/quantique1/Documents/fluidXtra/fluidXtra/fluidsynth-1.1.5/include/fluidsynth

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files/FluidSynth")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/fluidsynth" TYPE FILE FILES
    "C:/Users/quantique1/Documents/fluidXtra/fluidXtra/fluidsynth-1.1.5/include/fluidsynth/audio.h"
    "C:/Users/quantique1/Documents/fluidXtra/fluidXtra/fluidsynth-1.1.5/include/fluidsynth/event.h"
    "C:/Users/quantique1/Documents/fluidXtra/fluidXtra/fluidsynth-1.1.5/include/fluidsynth/gen.h"
    "C:/Users/quantique1/Documents/fluidXtra/fluidXtra/fluidsynth-1.1.5/include/fluidsynth/log.h"
    "C:/Users/quantique1/Documents/fluidXtra/fluidXtra/fluidsynth-1.1.5/include/fluidsynth/midi.h"
    "C:/Users/quantique1/Documents/fluidXtra/fluidXtra/fluidsynth-1.1.5/include/fluidsynth/misc.h"
    "C:/Users/quantique1/Documents/fluidXtra/fluidXtra/fluidsynth-1.1.5/include/fluidsynth/mod.h"
    "C:/Users/quantique1/Documents/fluidXtra/fluidXtra/fluidsynth-1.1.5/include/fluidsynth/ramsfont.h"
    "C:/Users/quantique1/Documents/fluidXtra/fluidXtra/fluidsynth-1.1.5/include/fluidsynth/seq.h"
    "C:/Users/quantique1/Documents/fluidXtra/fluidXtra/fluidsynth-1.1.5/include/fluidsynth/seqbind.h"
    "C:/Users/quantique1/Documents/fluidXtra/fluidXtra/fluidsynth-1.1.5/include/fluidsynth/settings.h"
    "C:/Users/quantique1/Documents/fluidXtra/fluidXtra/fluidsynth-1.1.5/include/fluidsynth/sfont.h"
    "C:/Users/quantique1/Documents/fluidXtra/fluidXtra/fluidsynth-1.1.5/include/fluidsynth/shell.h"
    "C:/Users/quantique1/Documents/fluidXtra/fluidXtra/fluidsynth-1.1.5/include/fluidsynth/synth.h"
    "C:/Users/quantique1/Documents/fluidXtra/fluidXtra/fluidsynth-1.1.5/include/fluidsynth/types.h"
    "C:/Users/quantique1/Documents/fluidXtra/fluidXtra/fluidsynth-1.1.5/include/fluidsynth/voice.h"
    "C:/Users/quantique1/Documents/fluidXtra/fluidXtra/fluidsynth-1.1.5/build_win/include/fluidsynth/version.h"
    )
endif()

