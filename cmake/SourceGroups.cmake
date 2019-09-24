# Use solution folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake Targets")

set(_CPP ".*\\.cpp")
set(CPP "${_CPP}$")

set(_CC ".*\\.cc")
set(CC "${_CC}$")

set(_H ".*\\.h")
set(H "${_H}$")

set(_HPP ".*\\.hpp")
set(HPP "${_HPP}$")

set(H_CPP "(${H}|${HPP}|${CPP}|${CC})")

set(SSRC ${CMAKE_SOURCE_DIR})
