/*
  common routines

  Copyright (c) 2006-2010 Cheng Zhang

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  Usage:

  1.  It is designed quick programming.
      For simple use, just include this file and all functions will be available.
      But there might be compiler warnings for unused functions.

  2.  You should be able to include this file multiple times in a single file
      without a problem (otherwise a bug!).

  3.  Every function is static by default. If you want to export functions,
      e.g., to make it easier to debug, or to avoid warning of unused functions,
      define ZCOM_XFUNCS before the first inclusion.

  4.  The file combines many modules. To specify a particular module,
        #define ZCOM_PICK
        #define ZCOM_RNG
      before including this file. 

  5.  If the compiler supports keywords inline and restrict,
        #define ZCINLINE inline
        #define ZCRESRICT restrict
      before including this file.
      
  6.  Define ZCHAVEVAM if the compiler supports variable-argument macros.

  7.  The def module defines `real' as a double, to override
        typedef float real;
        #define ZCHAVEREAL 1
      before including this file (or equivalently HAVE_REAL)
*/

/* macros for selectively including functions, advantages: 
 * 1. reduces the # of warnings for unused functions
 * 2. accelerates the compiling
 * 3. avoids multiple inclusions
 * By default, ZCOM_PICK is undefined, so everything is used. */
#ifdef ZCOM_NONE  /* equivalent to ZCOM_PICK */
#define ZCOM_PICK
#endif

#ifndef ZCOM_PICK
#endif

/* build dependencies */

/* manage storage class: static is the safer choice
   to avoid naming conclict.  Example:
   both m.c and n.c include this file,
   m.c --> m.o, n.c --> n.o, m.o+n.o --> a.out
   static is the only way to avoid naming conflict in this case.

   In case that this file is included multiple times,
   ZCOM_XFUNCS should be defined before the first inclusion,
   otherwise it won't be effective in deciding storage class. */
#ifndef ZCSTRCLS
  #ifndef ZCOM_XFUNCS
    #define ZCSTRCLS static
  #else
    #define ZCSTRCLS
  #endif
#endif

/* inline keyword */
#ifndef ZCINLINE
  #if defined(__GNUC__) || defined(__xlC__)
    #define ZCINLINE ZCSTRCLS __inline__
  #elif defined(_MSC_VER) || defined(__BORLANDC__)
    #define ZCINLINE __inline ZCSTRCLS
  #elif defined(__STDC_VERSION__) && (STDC_VERSION__ >= 199901L)
    #define ZCINLINE ZCSTRCLS inline
  #else
    #define ZCINLINE ZCSTRCLS
  #endif
#endif

/* restrict keyword */
#ifndef ZCRESTRICT
  #if (defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__xlC__))
    #define ZCRESTRICT __restrict
  #elif defined(__STDC_VERSION__) && (STDC_VERSION__ >= 199901L)
    #define ZCRESTRICT restrict
  #else
    #define ZCRESTRICT
  #endif
#endif

/* macros with variable-length arguments */
#ifndef ZCHAVEVAM
  #if (  (defined(__GNUC__) && (__GNUC__ >= 3))   \
      || (defined(__xlC__)  && (__xlC__ >= 0x0700)) \
      || (defined(_MSC_VER) && (_MSC_VER >= 1400)) ) 
    #define ZCHAVEVAM 1
  #endif
#endif

#ifdef __INTEL_COMPILER
  #pragma warning(disable:981) /* unspecified order warning */
  #pragma warning(disable:177) /* unreferenced function */
#endif

#ifdef _MSC_VER
  #pragma warning(disable:4127) /* conditional expression constant */
  #pragma warning(disable:4505) /* unreferenced function */
  #pragma warning(disable:4514) /* unreferenced inline */
  #pragma warning(disable:4710) /* not inlined */
#endif

/* In addition to ZCOM_ABC, we have to define another macro ZCOM_ABC__
 * in order to avoid multiple inclusion a single ZCOM_ABC__ won't do,
 * because different module-set may be selected */

