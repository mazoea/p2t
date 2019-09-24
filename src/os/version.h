#ifndef _VERSION_H_
#define _VERSION_H_

#if defined(_M_X64) || defined(__INT64_MAX__)
#define _PROG_ARCH_ "x64"
#else
#define _PROG_ARCH_ "x32"
#endif

#if defined(_WIN32) || defined(WIN64)
#ifndef _WINDOWS
#define _WINDOWS
#endif
#endif

#define _PROG_VERSION_ "3.0.1"
#define _LIB_VERSION_STR "[" _PROG_VERSION_ _PROG_ARCH_ " - " __TIME__ " " __DATE__ "]"
#define PR_INFO " by Mazoea s.r.o."

#define _COMMIT_VERSION_ "[XXX 2016-01-01 00:00:00 +0100 by jm++]"

// helper macros
#ifndef STRINGIFY
#define DO_EXPAND(VAL) VAL##1
#define EXPAND(VAL) DO_EXPAND(VAL)
#define STRINGIFY(x) #x
#endif

#ifndef _T1LIB_VERSION_
  #define _T1LIB_VERSION_ "not available"
#endif
#ifndef _FREETYPELIB_VERSION_
  #define _FREETYPELIB_VERSION_ "not available"
#endif
#define _LIBS_VERSION_ "t1lib: " _T1LIB_VERSION_ ", freetype: " _FREETYPELIB_VERSION_

#pragma message( "Using: " _LIBS_VERSION_ )

namespace maz {

    inline const char* version()
    {
        return _LIB_VERSION_STR " " _COMMIT_VERSION_ " ";
    }
} // namespace maz

#endif // _VERSION_H_
