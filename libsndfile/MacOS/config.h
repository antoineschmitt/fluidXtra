/* src/config.h.  Generated automatically by configure.  */
/* src/config.h.in.  Generated automatically from configure.in by autoheader.  */

/* Define to `long' if <sys/types.h> doesn't define.  */
/* #undef off_t */

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define to `long' if <sys/types.h> doesn't define.  */
/* #undef off_t */

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

#ifdef __MWERKS__
	#ifdef _WIN32
		#define __BIG_ENDIAN__ 0 /* Set to 1 if the processor is big endian, otherwise set to 0.  */
		#define __LITTLE_ENDIAN__ 1
		#define CAN_READ_WRITE_x86_IEEE 1 /* Set to 1 if the processor can read and write Intel x86 32 bit floats.  */
	#else
		#ifndef  __BIG_ENDIAN__
			#define __BIG_ENDIAN__ 1
		#endif
		#define __LITTLE_ENDIAN__ 0
		#define CAN_READ_WRITE_x86_IEEE 0
	#endif
#endif

#ifdef _DIRECTOR_XCODE
	#ifdef __ppc__
		#define __BIG_ENDIAN__ 1
		#define __LITTLE_ENDIAN__ 0
		#define CAN_READ_WRITE_x86_IEEE 0
	#endif
	#ifdef __i386__
		#define __BIG_ENDIAN__ 0
		#define __LITTLE_ENDIAN__ 1
		#define CAN_READ_WRITE_x86_IEEE 1
	#endif
#endif
	


/* The number of bytes in a double.  */
#define SIZEOF_DOUBLE 8

/* The number of bytes in a float.  */
#define SIZEOF_FLOAT 4

/* The number of bytes in a int.  */
#define SIZEOF_INT 4

/* The number of bytes in a short.  */
#define SIZEOF_SHORT 2

/* The number of bytes in a void*.  */
#define SIZEOF_VOIDP 4

/* Define if you have the fclose function.  */
#define HAVE_FCLOSE 1

/* Define if you have the fopen function.  */
#define HAVE_FOPEN 1

/* Define if you have the fread function.  */
#define HAVE_FREAD 1

/* Define if you have the free function.  */
#define HAVE_FREE 1

/* Define if you have the fseek function.  */
#define HAVE_FSEEK 1

/* Define if you have the ftell function.  */
#define HAVE_FTELL 1

/* Define if you have the fwrite function.  */
#define HAVE_FWRITE 1

/* Define if you have the malloc function.  */
#define HAVE_MALLOC 1

/* Name of package */
#define PACKAGE "libsndfile"

/* Version number of package */
#define VERSION "0.0.16"

