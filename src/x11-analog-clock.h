/*
 * x11-clock.h
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

#if !defined(COMMIT_ID)
#define COMMIT_ID "[Commit ID: $Format:%h$]"
#endif

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

const char *h_msg_usage[] =
{
   "Usage: %s [OPTION]...\n",
   "An analog clock for X11.\n",
   "\n",
   "  -v  --version            show version and exit\n",
   "      --geometry hxw+x+y   specify initial window position\n",
   "      --help               show this help and exit\n\n",
   NULL
};
