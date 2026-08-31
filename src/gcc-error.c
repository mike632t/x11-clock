/*
 * x11-clock-error.c
 *
 * Copyright(C) 2026 - MT
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
 * 22 Feb 26         - Initial version - MT
 * 
 */

#include <errno.h>      /* errno */
#include <stdarg.h>     /* vargs(), va_list */
#include <string.h>     /* strlen(), etc */
#include <stdio.h>      /* fprintf(), stderr */
#include <stdlib.h>     /* exit(), getenv() */

#include "gcc-debug.h"  /* debug() */

int i_error(FILE *h_file, const char *s_name, int i_severity, const char *s_format, ...)  /* Prints an error message */
{
   va_list t_args;
   va_start(t_args, s_format);
   fprintf(h_file, "%s: ", s_name);  /* Always display program name */
   vfprintf(h_file, s_format, t_args);  /* Print formatted error message */
   va_end(t_args);
   fprintf(h_file, "\r\n");  /* Add a newline */
   fflush(h_file);
   return i_severity;  /* Return default error code */
}


void v_print(FILE *h_file, const char **s_text, ...)  /* Print multiple lines of text */
{
    va_list t_args;
    int i_count;

    va_start(t_args, s_text);
    for (i_count = 0; s_text[i_count] != NULL; i_count++)
    {
        if (i_count == 0)
            vfprintf(h_file, s_text[i_count], t_args);  /* Display first line using arguments */
        else
            fprintf(h_file, "%s", s_text[i_count]);  /* Display remainder of the text */
    }
    va_end(t_args);
}

