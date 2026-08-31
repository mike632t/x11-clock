/*
 * gcc-debug.h
 *
 * Copyright(C) 2013   MT
 *
 * Define debug macro.
 *
 * This  program is free software: you can redistribute it and/or modify it
 * under  the terms of the GNU General Public License as published  by  the
 * Free  Software Foundation, either version 3 of the License, or (at  your
 * option) any later version.
 *
 * This  program  is distributed in the hope that it will  be  useful,  but
 * WITHOUT   ANY   WARRANTY;   without even   the   implied   warranty   of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You  should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * 17 Aug 13         - Initial version - MT
 * 07 Dec 18         - Updated  allow debugging to be turned on and off  by
 *                     setting  DEBUG true or false - MT
 * 22 Aug 21         - Prints Current filename and line number - MT
 *                   - Added verbose macro - MT
 * 03 Jan 22         - Changed both macros to allow DEBUG and VERBOSE to be
 *                     defined from the command line - MT
 * 09 Jun 25         - Changed to use stdout instead of stderr - MT
 * 25 Jul 25         - Added profile and error macros - MT
 *  1 Aug 25         - All macro definitions are now guarded by debug - MT
 * 27 Jan 26         - Put code in it's own block - MT
 * 26 Feb 26         - Added timestamps to debug output - MT
 *  7 Mar 26         - Tidied up formatting - MT
 *
 */

#ifndef debug /* Don't redefine macro if already defined. */

#if defined(DEBUG)

#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__) /* UNIX like operating systems */
#include <sys/time.h>
#include <time.h>

#define debug(code) do \
{ \
    { \
        struct timeval tv; \
        char *p; \
        gettimeofday(&tv, NULL); \
        p = ctime(&tv.tv_sec); \
        fprintf(stdout, "Debug %.*s.%03ld : %s line %d : ", \
            8, p+11, tv.tv_usec/1000, __FILE__, __LINE__); \
    } \
    { code; } \
    fprintf(stdout, "\n"); \
} while(0)
#else
#define debug(code) do \
{ \
    fprintf(stdout, "Debug\t: %s line : %d : ", __FILE__, __LINE__); \
    { code; } \
    fprintf(stdout, "\n"); \
} while(0)
#endif
#else
#define debug(code)
#endif


#if defined(VERBOSE)
#define verbose(code) do \
{ \
    fprintf(stdout, "Verbose\t: %s line : %d : ", __FILE__, __LINE__); \
    { code; } \
    fprintf(stdout, "\n"); \
} while(0)
#else
#define verbose(code)
#endif

#if defined(DEBUG)  /* Execute code if DEBUG is True */ 

#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)

#define profile(__code) do \
{  /* Time how long code takes to execute */ \
   struct timespec __t_start, __t_end; \
   double __d_elapsed; \
   clock_gettime(CLOCK_MONOTONIC, &__t_start); \
   __code; \
   clock_gettime(CLOCK_MONOTONIC, &__t_end); \
   __d_elapsed = (__t_end.tv_sec - __t_start.tv_sec) + \
                    (__t_end.tv_nsec - __t_start.tv_nsec) / 1e9; \
   fprintf(stdout, "%0.9fs\n", __d_elapsed); \
} while(0)
#else
#define profile(code)
#endif
#else
#define profile(code)
#endif

#endif
