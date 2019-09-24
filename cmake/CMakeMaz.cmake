#
#
macro(maz_simple_info)
    message(STATUS "")
    message(STATUS "INFO: CYGWIN=${CYGWIN} UNIX=${UNIX} WIN32=${WIN32}")
    message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
    message(STATUS "")
    message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
    message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
    message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
    message(STATUS "")
    message(STATUS "CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")
    message(STATUS "CMAKE_SHARED_LINKER_FLAGS: ${CMAKE_SHARED_LINKER_FLAGS}")
    message(STATUS "CMAKE_STATIC_LINKER_FLAGS: ${CMAKE_STATIC_LINKER_FLAGS}")
    message(STATUS "CMAKE_CXX_FLAGS_RELWITHDEBINFO: ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    message(STATUS "CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")
    message(STATUS "CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
    message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
    message(STATUS "")
    message(STATUS "CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO: ${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO}")
    message(STATUS "CMAKE_EXE_LINKER_FLAGS_RELEASE: ${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
    message(STATUS "CMAKE_EXE_LINKER_FLAGS_DEBUG: ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
    message(STATUS "")
endmacro()

#
#
macro(maz_compile_info)
    message(STATUS "")
    message(STATUS "Using LIB paths ${LIB_PATHS}")
    message(STATUS "CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")
    message(STATUS "CMAKE_SHARED_LINKER_FLAGS: ${CMAKE_SHARED_LINKER_FLAGS}")
    message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
    message(STATUS "")
    message(STATUS "INFO: Static linking with ${Static_LIBRARIES}")
    message(STATUS "INFO: Dynamic linking with ${Dynamic_LIBRARIES}")
    message(STATUS "")
endmacro()

#
#
macro(maz_default_settings)

    option(CPU_NATIVE "Compile for native CPU" OFF)
    option(STATIC_ANALYSIS "Static analysis" OFF)
    option(UBSAN_ANALYSIS "UBSAN_ANALYSIS analysis" OFF)
    option(LINKER_OPTIM "Remove unused functions if possible etc." OFF)

    if ("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "x64")
        set(MAZPLATFORM "x64")
    else()
        set(MAZPLATFORM "x32")
    endif()
    message(STATUS "Building platform: ${MAZPLATFORM}")

    set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/../${MAZ_OUTPUT_PATH}")
    message(STATUS "EXECUTABLE_OUTPUT_PATH: ${EXECUTABLE_OUTPUT_PATH}")
    set(LIBRARY_OUTPUT_PATH  "${EXECUTABLE_OUTPUT_PATH}")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}")

endmacro()

macro(configure_msvc_runtime)
    if (MSVC)
        add_definitions(-DNOGDI)
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
        add_definitions(-DNOMINMAX)

        set(variables
            CMAKE_C_FLAGS_DEBUG
            CMAKE_C_FLAGS_MINSIZEREL
            CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_RELWITHDEBINFO
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_MINSIZEREL
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_RELWITHDEBINFO
        )

        message(STATUS "Initial build flags:")
        foreach(variable ${variables})
            message(STATUS "  '${variable}': ${${variable}}")
        endforeach()

        foreach(variable ${variables})
            set(${variable} "${CMAKE_CXX_FLAGS} ${${variable}} /MP /utf-8")
        endforeach()


        foreach(variable ${variables})
            if(${variable} MATCHES "/MT")
                string(REGEX REPLACE "/MT" "/MD" ${variable} "${${variable}}")
            endif()
        endforeach()

        message(STATUS "Current build flags:")
        foreach(variable ${variables})
            message(STATUS "  '${variable}': ${${variable}}")
        endforeach()

        message(STATUS "")
    endif()
endmacro()

#
#
macro(maz_configure_compilers_basic)

    if (MSVC)
        configure_msvc_runtime()
        set(MAZRUNTIME "mtdll")
        message(STATUS "Building runtime: ${MAZRUNTIME}")
        # /GL - Link time code generation (whole program optimization)
        # /Gy - Function-level linking
        # use always with windows for now even if LINKER_OPTIM is suitable
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /GL /Gy")
        set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /LTCG")
        set(CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO} /LTCG")
    endif()

    if (CYGWIN)
        add_definitions(-D_GLIBCXX_USE_C99)
        add_definitions(-D__CYGWIN__)
    endif()

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        if (STATIC_ANALYSIS)
            message(STATUS "Static analysis")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --analyze")
        endif()
        if (UBSAN_ANALYSIS)
            message(STATUS "UBSan analysis")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined,address,integer,nullability -fno-omit-frame-pointer")
        endif()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -fPIC")
    endif()


    if (CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        # -pipe
        # Use pipes rather than temporary files for communication between the various
        # stages of compilation. This fails to work on some systems where the assembler is unable
        # to read from a pipe; but the GNU assembler has no trouble.
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64 -pipe -fmessage-length=0 -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=1")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unknown-pragmas -Wno-unused-parameter -Wno-missing-field-initializers -Wno-ignored-qualifiers")
        if (CMAKE_COMPILER_IS_GNUCXX)
            # not implemented in all versions - set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-misleading-indentation")
            if (LINKER_OPTIM AND CMAKE_COMPILER_IS_GNUCXX)
                message(STATUS "Linking with optimizations")
                set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO} -flto")
                set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -flto")
                set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -flto")
                set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
            endif()
        endif()

        if(CPU_NATIVE)
            message(STATUS "Compiling for native CPU")
            # use  -mno-avx2 because of lambda containers not supporting it
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -mno-avx2")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native -mno-avx2")
        endif()

        if(COVERAGE)
            message(STATUS "Compiling with coverage")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
        endif()

    endif()

endmacro()
