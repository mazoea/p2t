// vim:tabstop=4:shiftwidth=4:noexpandtab:textwidth=80

#ifndef _COMPILER_H_
#define _COMPILER_H_

// default definitions for compiler specific attributes 
#define UNUSED_PARAM
#define WARN_UNUSED_RESULT
#define DEPRECATED
#define DONOTUSE

//
// GCC specific stuff
//
#ifdef __GNUC__

// Note that gcc >= 4.3 reports error when macro is redefined, so 
// we need to undefine already existing macros

#undef UNUSED_PARAM
#define UNUSED_PARAM __attribute__((unused))

#undef WARN_UNUSED_RESULT 
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))

#undef DEPRECATED
#define DEPRECATED __attribute__((deprecated))

#define USE_GCC_PRAGMAS

/* There is a bug in the version of gcc which ships with MacOS X 10.2 */
#if defined(__APPLE__) && defined(__MACH__)
#  include <AvailabilityMacros.h>
#endif
#ifdef MAC_OS_X_VERSION_MAX_ALLOWED
#  if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_2
#    undef USE_GCC_PRAGMAS
#  endif
#endif

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#endif // GCC specific stuff

#if defined(_MSC_VER)
#	if _MSC_VER >= 1400
#		undef DEPRECATED
#		define DEPRECATED // must be at the beginning __declspec(deprecated) 
#	endif
#endif

#endif

