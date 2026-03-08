#ifndef CONFIG_H
#define CONFIG_H

/* config.h.  Manual config for MinGW (GCC on Windows). */

#ifdef _MSC_VER
#  warning "mingw/config.h shouldn't be included for your development environment."
#  error   "Please make sure the mingw/ directory is removed from your build path."
#endif

/* Define to the attribute for default visibility. */
#define DEFAULT_VISIBILITY /**/

/* Define to 1 to start with debug message logging enabled. */
/* #undef ENABLE_DEBUG_LOGGING */

/* Define to 1 to enable message logging. */
#define ENABLE_LOGGING 1

/* Define to 1 if compiling for a Windows platform. */
#define PLATFORM_WINDOWS 1

/* Define to the attribute for enabling parameter checks on printf-like
   functions. */
#define PRINTF_FORMAT(a, b) __attribute__((__format__(__printf__, a, b)))

/* Define to 1 if the system has the type `struct timespec'. */
#define HAVE_STRUCT_TIMESPEC 1

/* Define to 1 to output logging messages to the systemwide log. */
/* #undef USE_SYSTEM_LOGGING_FACILITY */

/* Qt defines NOMINMAX which removes min/max from <windows.h>.
   Re-define them here as macros so libusb C sources compile correctly. */
#ifndef min
#  define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#  define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#endif /* CONFIG_H */
