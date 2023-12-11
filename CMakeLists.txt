cmake_minimum_required(VERSION 3.14.0)

# The NEW behavior for this policy is to set VERSION as documented by the project() command.
cmake_policy(SET CMP0048 NEW)

project(Fespp
	VERSION 3.1.0
	DESCRIPTION "Paraview plugin for importing EPC documents"
	HOMEPAGE_URL "https://github.com/F2I-Consulting/fespp"
	LANGUAGES C CXX
)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/modules)

# These five lines are required in order to set up installation directories
# (which also control build directory locations) and enable shared builds
# (CMake's default is for a static build).
include(GNUInstallDirs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(BUILD_SHARED_LIBS ON)

# Find ParaView. This will bring in ParaView's CMake API and imported targets.
find_package(ParaView 5.10.0 REQUIRED pqCore)

# Scan the plugin file in order to set up internal data structures for building plugins
set("_paraview_plugin_default_${CMAKE_PROJECT_NAME}" ON)
paraview_plugin_scan(
  PLUGIN_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/Plugin/paraview.plugin"
  PROVIDES_PLUGINS  plugins
  REQUIRES_MODULES  required_modules)

foreach (module IN LISTS required_modules)
  if (NOT TARGET "${module}")
    message("Missing required module: ${module}")
    return ()
  endif ()
endforeach ()

# Build the plugins discovered during the scan.
paraview_plugin_build(
  RUNTIME_DESTINATION "${CMAKE_INSTALL_BINDIR}"
  LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY_SUBDIRECTORY "${PARAVIEW_PLUGIN_SUBDIR}"
  PLUGINS ${plugins})
