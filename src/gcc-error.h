/*
 * x11-clock-error.h
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
 
#define  EXIT_WARN   2 
#define  EXIT_INFO   3

int i_error(FILE *h_file, const char *s_name, int i_severity, const char *s_format, ...);
void v_print(FILE *h_file, const char **s_text, ...);
