#========================================================================
#
# splash/CMakeLists.txt
#
# CMake script for the splash library.
#
# Copyright 2015 Glyph & Cog, LLC
#
#========================================================================

include_directories("${PROJECT_SOURCE_DIR}/src/xpdf")
include_directories("${PROJECT_SOURCE_DIR}/src/xpdf/goo")
include_directories("${PROJECT_SOURCE_DIR}/src/xpdf/fofi")

file(GLOB _HEADER_FILES *.h)
file(GLOB _SOURCE_FILES *.cc)
add_library(splash OBJECT ${_HEADER_FILES} ${_SOURCE_FILES})