#========================================================================
#
# fofi/CMakeLists.txt
#
# CMake script for the fofi library.
#
# Copyright 2015 Glyph & Cog, LLC
#
#========================================================================

include_directories("${PROJECT_SOURCE_DIR}/src/xpdf/goo")

file(GLOB _HEADER_FILES *.h)
file(GLOB _SOURCE_FILES *.cc)
add_library(fofi OBJECT ${_HEADER_FILES} ${_SOURCE_FILES})