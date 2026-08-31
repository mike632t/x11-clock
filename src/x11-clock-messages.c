/*
 * x11-calc-text.c
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
 * To Do             - Add command line options.
 * 
 * 
 */
 
#include <stdlib.h>     /* NULL */

const char *h_err_geometry = "invalid geometry: %s (expected: WxH[+X+Y])";
const char *h_err_invalid_operand = "invalid operand(s)";
const char *h_err_invalid_option = "invalid option -- '%c'";
const char *h_err_duplicate_option = "duplicate option -- '%c'";
const char *h_err_missing_argument = "option requires an argument -- '%s'";
const char *h_err_unrecognised_option = "unrecognised option '%s'";
const char *h_err_invalid_number = "not a number -- '%s'";
const char *h_err_numeric_range = "out of range -- '%s'";
const char *h_err_invalid_argument = "expected argument not -- '%c'";

const char *h_err_display = "Cannot connect to X server '%s'";
const char *h_err_window = "Unable to create window";
const char *h_err_protocols = "Unable to set window manager protocols";
const char *h_err_hint = "Unable to allocate size hint";
const char *h_err_display_properties = "Unable to get display properties";
const char *h_err_display_colour = "Requires a %d-bit colour display";
const char *h_err_pixmap = "Can't create pixmap";

const char *h_err_context = "Unable to create graphics context";
const char *h_err_colour = "Failed to allocate colour '%s'";
const char *h_err_invalid_colour = "Invalid colour name '%s'";
const char *h_err_time = "Can't get time";
const char *h_err_localtime = "Can't get local time";

const char *h_msg_licence[] =
{
   "Copyright(C) %s %s\n",
   "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n",
   "This is free software: you are free to change and redistribute it.\n",
   "There is NO WARRANTY, to the extent permitted by law.\n",
   NULL
};

