#========================================================================
#
# goo/CMakeLists.txt
#
# CMake script for the goo library.
#
# Copyright 2015 Glyph & Cog, LLC
#
#========================================================================

file(GLOB _HEADER_FILES *.h)
file(GLOB _SOURCE_FILES *.cc *.c)
add_library(goo OBJECT ${_HEADER_FILES} ${_SOURCE_FILES})