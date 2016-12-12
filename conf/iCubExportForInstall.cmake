# Copyright: (C) 2010 RobotCub Consortium
# Authors: Lorenzo Natale
# CopyPolicy: Released under the terms of the GNU GPL v2.0.
#
# Code to export the installd project
#
# Files:
# EXPORT_CONFIG_FILE: in build directory, store libraries and dependenecies
# automatically generated with CMake export command.
# EXPORT_INCLUDE_FILE: in build directory, store include directories for each library,
# this is installed in CMAKE_INSTALL_PREFIX
# INSTALL_CONFIG_FILE: tmp file, it is generated from a template and will
# become the real icub-config.cmake that is installed in CMAKE_INSTALL_PREFIX
# INSTALL_CONFIG_TEMPLATE: INSTALL_CONFIG_FILE template
#
# In the template the variable $ICUB_INCLUDE_DIRS is substituted with the
# correct value which is a combination of:
# - ${CMAKE_INSTALL_PREFIX}/include (this is the general include directory where header files
# are installed
# - a list of external directories, as specified in the target's EXTERNAL_INCLUDE_DIRS property
# set in icub_export_library inside iCubHelpers.cmake
#
#
#### prepare config file for installation
get_property(ICUB_TARGETS GLOBAL PROPERTY ICUB_TARGETS)

#message(STATUS "Now exporting targets for installed builds: ${ICUB_TARGETS}")

set(EXPORT_CONFIG_FILE icub-export-install.cmake)
set(EXPORT_INCLUDE_FILE icub-export-inst-includes.cmake)

set(INSTALL_CONFIG_FILE icub-config-for-install.cmake)
set(INSTALL_CONFIG_TEMPLATE "conf/template/icub-config-install.cmake.in")

set(ICUB_MODULE_PATH ${CMAKE_INSTALL_PREFIX}/share/iCub/cmake)

set(ICUB_LINK_FLAGS ${IPOPT_LINK_FLAGS})

file(APPEND ${CMAKE_BINARY_DIR}/${EXPORT_INCLUDE_FILE} "# This file is automatically generated, see conf/iCubExportForInstall.cmake\n")
file(APPEND ${CMAKE_BINARY_DIR}/${EXPORT_INCLUDE_FILE} "###################\n")
file(APPEND ${CMAKE_BINARY_DIR}/${EXPORT_INCLUDE_FILE} "# List of include directories for exported targets\n\n")

set(include_dirs "")
foreach (t ${ICUB_TARGETS})
  get_property(target_INCLUDE_DIRS TARGET ${t} PROPERTY EXTERNAL_INCLUDE_DIRS)
  set(include_dirs ${include_dirs} ${target_INCLUDE_DIRS})

  if (ICUB_VERBOSE)
	message(STATUS "Header files for ${t}: ${target_INCLUDE_DIRS}")
  endif()

  file(APPEND ${CMAKE_BINARY_DIR}/${EXPORT_INCLUDE_FILE} "unset(${t}_INCLUDE_DIRS CACHE)\n")
  if (target_INCLUDE_DIRS)
      file(APPEND ${CMAKE_BINARY_DIR}/${EXPORT_INCLUDE_FILE} "set(${t}_INCLUDE_DIRS \"${CMAKE_INSTALL_PREFIX}/include\" \"${target_INCLUDE_DIRS}\")\n")
  else()
      file(APPEND ${CMAKE_BINARY_DIR}/${EXPORT_INCLUDE_FILE} "set(${t}_INCLUDE_DIRS \"${CMAKE_INSTALL_PREFIX}/include\")\n")
 endif()
endforeach(t)

if(include_dirs)
   LIST(REMOVE_DUPLICATES include_dirs)
endif(include_dirs)

set(ICUB_INCLUDE_DIRS ${CMAKE_INSTALL_PREFIX}/include ${include_dirs})
file(APPEND ${CMAKE_BINARY_DIR}/${EXPORT_INCLUDE_FILE} "set(ICUB_INCLUDE_DIRS \"${CMAKE_INSTALL_PREFIX}/include\" \"${include_dirs}\" CACHE STRING \"include dir for target ${t}\")\n\n")

#message(STATUS "Header files global directory: ${ICUB_INCLUDE_DIRS}")

### now write to file the list of dependencies with which iCub was compiled
get_property(dependency_flags GLOBAL PROPERTY ICUB_DEPENDENCIES_FLAGS)
foreach (t ${dependency_flags})
		file(APPEND ${CMAKE_BINARY_DIR}/${EXPORT_INCLUDE_FILE} "unset(${t} CACHE)\n")
		file(APPEND ${CMAKE_BINARY_DIR}/${EXPORT_INCLUDE_FILE} "set(${t} \"${${t}}\")\n")
endforeach()

install(EXPORT icub-targets DESTINATION lib/ICUB FILE ${EXPORT_CONFIG_FILE} COMPONENT Development)

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/${INSTALL_CONFIG_TEMPLATE}
  ${CMAKE_BINARY_DIR}/${INSTALL_CONFIG_FILE} @ONLY IMMEDIATE)

install(FILES ${CMAKE_BINARY_DIR}/${INSTALL_CONFIG_FILE} DESTINATION lib/ICUB RENAME icub-config.cmake COMPONENT Development)
install(FILES ${CMAKE_BINARY_DIR}/${EXPORT_INCLUDE_FILE} DESTINATION lib/ICUB RENAME ${EXPORT_INCLUDE_FILE} COMPONENT Development)


