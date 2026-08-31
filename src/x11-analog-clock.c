/*
 * x11-analog-clock.c - Yet another analogue clock!
 *
 * Copyright(C) 2014 - MT
 *
 * A relatively simple example program for X windows that displays the time 
 * using an analogue clock face.
 * 
 * Includes a generic error handler but uses on a separate delay routine to
 * provide a cross platform solution.
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
 * 15 Jun 14   0.1   - Initial version - MT
 * 12 Jan 26         - Calculates  position of second hand using a floating
 *                     point value for the number seconds resulting in much 
 *                     smoother movement - MT
 * 16 Jan 26         - Use mouse button to pause display updates - MT
 * 18 Jan 26         - Dynamically  scale line widths depending on the size 
 *                     of the clock face - MT
 *                   - Don't draw a border round the clock face - MT
 *                   - Checks colour depth - MT
 *                   - Compiles on Linux Tru64 and VMS - MT
 * 21 Jan 26         - Maintains  compatibility with older systems by using 
 *                     time() by default. Define POSIX to update the second
 *                     hand more frequently on systems with clock_gettime()
 *                     support - MT
 * 17 Feb 26  (0009) - Allow window to be resized using keyboard - MT
 * 18 Feb 26         - More extensive error handling - MT
 * 21 Feb 26         - Window size was calculated incorrectly - MT
 *                   - Allocating pixmap returns correct status - MT
 * 07 Mar 26         - Restructured the code and moved the common functions 
 *                     into separate modules - MT 
 * 10 Mar 26   0.2   - Added command line options to allow the users define 
 *                     the window size and position - MT
 *                   - Adds an alternative command line parser that gives a
 *                     more appropriate interface on non-UNIX systems - MT
 * 
 * To Do             - 
 * 
 */

#define  NAME           "x11-analog-clock"
#define  VERSION        "0.2"
#define  BUILD          "0016"
#define  DATE           "10 Mar 26"
#define  AUTHOR         "MT"

#define  NODEBUG
#define  NOBORDER
#define  POSIX

#define  WIDTH          320   /* Default initial window size */
#define  HEIGHT         320
#define  PADDING        8     /* Padding between edge of window and clock face */
#define  MINIMUM        64    /* Do not use a minimum size less than 32 as that gets pretty hard to see! */

#include <errno.h>      /* errno */

#include <stdarg.h>     /* vargs(), va_list */
#include <string.h>     /* strlen(), etc */
#include <stdio.h>      /* fprintf(), stderr */
#include <stdlib.h>     /* exit(), getenv() */

#include <ctype.h>      /* isalpha(), toupper()etc */

#include <time.h>       /* time(), localtime() */
#include <math.h>       /* sin(), cos() */

#include <X11/Xlib.h>   /* XOpenDisplay(), True/False etc */
#include <X11/Xutil.h>  /* XSizeHints etc */
#include <X11/Xlib.h>
#include <X11/Xutil.h>  /* XSizeHints */
#include <X11/keysym.h>

#include "gcc-debug.h"  /* debug() */
#include "gcc-error.h"  /* error() */
#include "gcc-wait.h"   /* wait() */

#include "x11-analog-clock.h"
#include "x11-clock-messages.h"

#define  PI             3.14159265358979323846


static void v_version(void)  /* Display version information */
{
   fprintf(stdout, "%s: Version %s.%s %s", NAME, VERSION, BUILD, COMMIT_ID);
#if defined(__compiler__)
   if (strlen(__compiler__)) fprintf(stdout, " %s", __compiler__);  /* Include compiler version if defined - it could be defined as a null string */
#endif
   if (__DATE__[4] == ' ') fprintf(stdout, " 0"); else fprintf(stdout, " %c", __DATE__[4]);
   fprintf(stdout, "%c %c%c%c %s %s\n", __DATE__[5],
      __DATE__[0], __DATE__[1], __DATE__[2], &__DATE__[9], __TIME__ );
}


static unsigned long l_NamedColourPixel(Display *x_display, int screen, Colormap x_colourmap, const char *s_name, unsigned long l_default)
{
   XColor x_colour;

   x_colour.pixel = l_default;  /*  Set default */
   if (XParseColor(x_display, x_colourmap, s_name, &x_colour))  /* Check if the colour name if valid */
   {
      if (!XAllocColor(x_display, x_colourmap, &x_colour))  /* Attempt to allocate nearest matching colour */
         i_error(stderr, NAME, EXIT_WARN, h_err_colour, s_name);
   }
   else  /* Not a valid colour */
      i_error(stderr, NAME, EXIT_WARN, h_err_invalid_colour, s_name);
   return x_colour.pixel;
}


static void v_draw_hand(Display *x_display, Drawable x_buffer, GC gc_hand, int center_x, int center_y, int length, int i_thickness, double angle)  /* Draw a clock hand (line with small circle at centre) */
{
   int x2 = center_x + (int)(length * cos(angle));  /* End point X */
   int y2 = center_y + (int)(length * sin(angle));  /* End point Y */

   XSetLineAttributes(x_display, gc_hand, (unsigned int)i_thickness, LineSolid, CapRound, JoinRound);  /* Solid line with rounded ends - looks pretty good */
   XDrawLine(x_display, x_buffer, gc_hand, center_x, center_y, x2, y2);  /* Draw hand */
   XFillArc(x_display, x_buffer, gc_hand, center_x - i_thickness, center_y - i_thickness, i_thickness * 2, i_thickness * 2, 0, 360 * 64);  /* Draw a circle at the centre */
}


static int i_draw_dial(Display *x_display, Drawable x_pixmap, GC gc_colour, GC gc_background, int i_window_width, int i_window_height)  /* Draw the clock face (tick marks, optional border) */
{
   double f_angle;
   int i_centre_x, i_centre_y, i_radius;
   int i_radius_inner, i_radius_outer;
   int i_radius_tick;
   int i_thickness;
   int i_x1, i_y1, i_x2, i_y2;
   int i_count;
   int i_status = 0;

   i_radius = MIN(i_window_width, i_window_height);
   i_radius = i_radius / 2 - PADDING;  /* Radius is half the height/width allowing for a border */
   i_centre_x = (int)(i_window_width / 2);
   i_centre_y = (int)(i_window_height / 2);
   if (i_radius > 0)  /* Check size is greater then zero otherwise there is nothing to do */
   {
      XFillArc(x_display, x_pixmap, gc_background, i_centre_x - i_radius, i_centre_y - i_radius, i_radius * 2, i_radius * 2, 0, 360 * 64);  /* Fill in whole clock face */
      i_thickness = (i_radius / 64);
      i_radius_inner = i_radius - (i_radius / 16);  /* Inner radius used for long tick marks (hours) */
      i_radius_outer = i_radius - (i_radius / 24);  /* Outer radius used for short tick marks (minutes) */
#if defined(BORDER)  /* Draw border round clock face */
      i_radius = i_radius - i_thickness / 2;  /* Bit of a fudge but it seems to work ! */
      XSetLineAttributes(x_display, gc_colour, i_thickness, LineSolid, CapRound, JoinRound);  /* Set border thickness */
      XDrawArc(x_display, x_pixmap, gc_colour, i_centre_x - i_radius, i_centre_y - i_radius, i_radius * 2 , i_radius * 2, 0, 360 * 64);  /* Draw border round clock face */
      i_radius = i_radius + i_thickness / 2;
#endif

      for (i_count = 0; i_count < 60; ++i_count)
      {
         if ((i_count % 5) == 0) /* Draw tick mark for each hour */
         {
            i_thickness = (i_radius + 24) / 48;
            XSetLineAttributes(x_display, gc_colour, i_thickness, LineSolid, CapRound, JoinRound);
            i_radius_tick = i_radius_inner;
         }
         else  /* Draw tick mark for each minute */
         {
            i_thickness = (i_radius + 32) / 64;
            XSetLineAttributes(x_display, gc_colour, i_thickness, LineSolid, CapRound, JoinRound);
            i_radius_tick = i_radius_outer;
         }

         f_angle = (i_count * 6.0 - 90.0) * PI / 180.0;  /* One tick every 6 degrees (converted to radians) */
         i_x1 = i_centre_x + (int)(cos(f_angle) * (double)(i_radius - i_radius / 64) + 0.5);  /* Compute coordinates of each end of the line */
         i_y1 = i_centre_y + (int)(sin(f_angle) * (double)(i_radius - i_radius / 64) + 0.5);
         i_x2 = i_centre_x + (int)(cos(f_angle) * (double)i_radius_tick + 0.5);
         i_y2 = i_centre_y + (int)(sin(f_angle) * (double)i_radius_tick + 0.5);

         if ((i_count % 5) == 0) /* Draw tick mark for each hour */
            XDrawLine(x_display, x_pixmap, gc_colour, i_x1, i_y1, i_x2, i_y2);
         else  /* Draw tick mark for each minute */
            if (i_thickness > 1) XDrawLine(x_display, x_pixmap, gc_colour, i_x1, i_y1, i_x2, i_y2);
      }
   }
   return i_status;
}


static int i_draw_hands(Display *x_display, Drawable x_pixmap, GC gc_colour, GC gc_foreground, int i_window_width, int i_window_height)  /* Draw the clock hands */
{
#if (defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__unix__)) && defined(POSIX)
   struct timespec t_now;
#else
   time_t t_now;
#endif
   struct tm *t_time;
   float f_seconds, f_minutes, f_hours;
   double d_second_angle, d_minute_angle, d_hour_angle;
   int i_centre_x, i_centre_y, i_radius;
   int i_status = 0;

   i_radius = MIN(i_window_width, i_window_height);
   i_radius = i_radius / 2 - PADDING;  /* Radius is half the height/width allowing for a border */
   i_centre_x = (int)(i_window_width / 2);
   i_centre_y = (int)(i_window_height / 2);

   if (i_radius > 0)  /* Check size is greater then zero otherwise there is nothing to do */
   {
#if (defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__unix__)) && defined(POSIX)
      if (clock_gettime(CLOCK_REALTIME, &t_now))  /* Get current time */
         i_status = i_error(stderr, NAME, EXIT_WARN, h_err_time);  
      else
      {
         if ((t_time = localtime(&t_now.tv_sec)))
         {
            f_seconds = (float)t_time->tm_sec + (float)t_now.tv_nsec / 1e9f;  /* Convert from nano seconds */
#else
      {
         t_now = time(NULL);  /* Get current time */
         if ((t_time = localtime(&t_now)))
         {
            f_seconds = (float)t_time->tm_sec;
#endif
            f_minutes = (float)t_time->tm_min + (f_seconds / 60.0);
            f_hours = (float)(t_time->tm_hour % 12) + (f_minutes / 60.0);
            
            d_second_angle = (f_seconds * 6.0 * PI / 180.0) - PI / 2;  /* Calculate angle of each hand (in radians, starting from the top, going clockwise) */ 
            d_minute_angle = (f_minutes * 6.0 * PI / 180.0) - PI / 2;
            d_hour_angle = (f_hours * 30.0 * PI / 180.0) - PI / 2;

            /** XFillArc(x_display, x_pixmap, gc_dial, i_centre_x - i_length, i_centre_y - i_length, i_length * 2, i_length * 2, 0, 360 * 64);  /* Fill in clock face */

            v_draw_hand(x_display, x_pixmap, gc_colour, i_centre_x, i_centre_y, (int)(i_radius * 0.6), i_radius / 16, d_hour_angle);  /* Length of hour hand is 60% of radius */
            v_draw_hand(x_display, x_pixmap, gc_colour, i_centre_x, i_centre_y, (int)(i_radius * 0.75), i_radius / 24, d_minute_angle);  /* Length of minute hand is 70% of radius */
            v_draw_hand(x_display, x_pixmap, gc_foreground, i_centre_x, i_centre_y, (int)(i_radius * 0.85), i_radius / 32, d_second_angle);  /* Length of second hand is 80% of radius */
         }
         else
            i_status = i_error(stderr, NAME, EXIT_WARN, h_err_localtime);
      }
   }
   return i_status;
}


int main(int i_argc, char *p_argv[])
{
   Display *x_display;
   Window x_window, x_root;
   Pixmap x_buffer;
   XEvent x_event;
   XSizeHints *h_size_hint;
   Atom wmDelete;
   Colormap x_colourmap;
   GC gc_background, gc_foreground, gc_highlight, gc_colour;

   XComposeStatus x_key_state;
   KeySym x_key;
   char s_key[16];  /* Holds the character string produced by a key (a letter, compose sequence, or escape sequence) */

   unsigned int i_screen_width, i_screen_height;
   unsigned int i_window_width, i_window_height, i_window_border, i_colour_depth;
   int i_window_left, i_window_top;
   int i_screen;
   int i_status, i_count, i_index, i_offset, i_result;
   int b_geometry = False;
   int b_quit = False;
   int b_abort = False;
   int b_resize = True;  /* Resizes window immediately after it is created */
   
   if ((x_display = XOpenDisplay(NULL)))  /* Open the display */
   {
      i_screen = DefaultScreen(x_display);  /* Get default screen index */
      i_screen_width = DisplayWidth(x_display, i_screen);
      i_screen_height = DisplayHeight(x_display, i_screen);
      i_window_width = WIDTH;
      i_window_height = HEIGHT;
      i_window_left = (i_screen_width - i_window_width) / 2;  /* Default window position */
      i_window_top = (i_screen_height - i_window_height) / 2;
      i_colour_depth = DefaultDepth(x_display, i_screen);
      i_status = 0;

#if (defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__))  /* Parse UNIX style command line options */
      for (i_count = 1; i_count < i_argc && !(i_status || b_quit || b_abort); i_count++)
      {
         if (p_argv[i_count][0] == '-')
         {
            i_index = 1;
            while ((p_argv[i_count][i_index] != 0) && !(i_status || b_quit || b_abort))
            {
               switch (p_argv[i_count][i_index])
               {
               case 'v':  /* Display version */
                  v_version();  /* Display version information */
                  v_print(stdout, h_msg_licence, &__DATE__[7], AUTHOR);
                  b_abort = True;  /* or exit(EXIT_SUCCESS) */
                  break;
               case '-':  /* '--' terminates command line processing */
                  i_index = strlen(p_argv[i_count]);
                  if (i_index == 2)
                    b_quit = True;  /* '--' terminates command line processing */
                  else
                  {
                     if (!strncmp(p_argv[i_count], "--geometry=", 11))  /* Just check the first 11 characters match */
                     {
                        const char *c_geometry = p_argv[i_count] + 11;
                        
                        if ((i_result = XParseGeometry(c_geometry,  &i_window_left, &i_window_top, &i_window_width, &i_window_height)))
                        {
                           if (!(i_result & XValue) && !(i_result & YValue))  /* If neither positional parameter was specified centre the window on the screen (again) since the default position will have been changed */
                           {
                              i_window_left = (i_screen_width - i_window_width) / 2;
                              i_window_top = (i_screen_height - i_window_height) / 2;
                           }
                           else
                              b_geometry = True;
                        }
                        else
                           i_status = i_error(stderr, NAME, EINVAL, h_err_geometry, c_geometry);
                     }
                     else if (!strncmp(p_argv[i_count], "--geometry", i_index))
                     {
                        if (i_count + 1 < i_argc)
                        {
                           const char *c_geometry = p_argv[i_count + 1];
                           if ((i_result = XParseGeometry(c_geometry,  &i_window_left, &i_window_top, &i_window_width, &i_window_height)))
                           {
                              if (!(i_result & XValue) && !(i_result & YValue))  /* If neither positional parameter was specified centre the window on the screen (again) since the default position will have been changed */
                              {
                                 i_window_left = (i_screen_width - i_window_width) / 2;
                                 i_window_top = (i_screen_height - i_window_height) / 2;
                              }
                              else
                                 b_geometry = True;
                              if (i_count + 2 < i_argc)  /* Remove the parameter from the arguments */
                                 for (i_offset = i_count + 1; i_offset < i_argc - 1; i_offset++)
                                    p_argv[i_offset] = p_argv[i_offset + 1];
                              i_argc--;
                           }
                           else
                              i_status = i_error(stderr, NAME, EINVAL, h_err_geometry, c_geometry);
                        }
                        else
                           i_status = i_error(stderr, NAME, EINVAL, h_err_missing_argument, p_argv[i_count]);
                     }
                     else if (!strncmp(p_argv[i_count], "--version", i_index))
                     {
                        v_version();  /* Display version information */
                        v_print(stdout, h_msg_licence, &__DATE__[7], AUTHOR);
                        b_abort = True;  /* or exit(EXIT_SUCCESS) */
                     }
                     else if (!strncmp(p_argv[i_count], "--help", i_index))
                     {
                        v_print(stdout, h_msg_usage, NAME);
                        b_abort = True;  /* or exit(EXIT_SUCCESS) */
                     }
                     else  /* If we get here then the we have an invalid long option */
                        i_status = i_error(stderr, NAME, EINVAL, h_err_unrecognised_option, p_argv[i_count]);
                  }
                  i_index--;  /* Leave index pointing at end of string (so argv[i_count][i_index] = 0) */
                  break;
               default:  /* If we get here the single letter option is unknown */
                  i_status = i_error(stderr, NAME, EINVAL, h_err_invalid_option, p_argv[i_count][i_index]);
                  i_index = strlen(p_argv[i_count]) - 1;
               }
               i_index++;  /* Parse next letter in option */
            }
            if ((p_argv[i_count][1] != 0) && (!i_status))
            {
               for (i_offset = i_count; i_offset < i_argc - 1; i_offset++)
                  p_argv[i_offset] = p_argv[i_offset + 1];
               i_argc--; i_count--;
            }
         }
      }
      
#else  /* Parse DEC style command line options */
      for (i_count = 1; i_count < i_argc && !(i_status || b_quit || b_abort); i_count++)
      {
         if (p_argv[i_count][0] == '/')
         {
            for (i_index = 0; p_argv[i_count][i_index]; i_index++)
               p_argv[i_count][i_index] = (char)toupper((unsigned char)p_argv[i_count][i_index]) ;  /* Convert all options to uppercase before parsing */
            if (!strncmp(p_argv[i_count], "/GEOMETRY", i_index))  
            {
               if (i_count + 1 < i_argc)
               {
                  const char *c_geometry = p_argv[i_count + 1];
                  if ((i_result = XParseGeometry(c_geometry,  &i_window_left, &i_window_top, &i_window_width, &i_window_height)))
                  {
                     if (!(i_result & XValue) && !(i_result & YValue))  /* If neither positional parameter was specified centre the window on the screen (again) since the default position will have been changed */
                     {
                        i_window_left = (i_screen_width - i_window_width) / 2;
                        i_window_top = (i_screen_height - i_window_height) / 2;
                     }
                     else
                        b_geometry = True;
                     if (i_count + 2 < i_argc)  /* Remove the parameter from the arguments */
                        for (i_offset = i_count + 1; i_offset < i_argc - 1; i_offset++)
                           p_argv[i_offset] = p_argv[i_offset + 1];
                     i_argc--;
                  }
                  else
                     i_status = i_error(stderr, NAME, EINVAL, h_err_geometry, c_geometry);
               }
               else
                  i_status = i_error(stderr, NAME, EINVAL, h_err_missing_argument, p_argv[i_count]);
            }
            else if (!strncmp(p_argv[i_count], "/VERSION", i_index))
            {
               v_version();  /* Display version information */
               v_print(stdout, h_msg_licence, &__DATE__[7], AUTHOR);
               b_abort = True;  /* or exit(EXIT_SUCCESS) */
            }
            else if ((!strncmp(p_argv[i_count], "/HELP", i_index)) | (!strncmp(p_argv[i_count], "/?", i_index)))
            {
               v_print(stdout, h_msg_usage, NAME);
               b_abort = True;  /* or exit(EXIT_SUCCESS) */
            }
            else  /* If we get here then the we have an invalid option */
               i_status = i_error(stderr, NAME, EINVAL, h_err_invalid_option, p_argv[i_count][i_index]);
            if (p_argv[i_count][1] != 0)
            {
               for (i_index = i_count; i_index < i_argc - 1; i_index++)
                  p_argv[i_index] = p_argv[i_index + 1];
               i_argc--; i_count--;
            }
         }
      }
#endif

      if (i_argc > 1) i_status = i_error(stderr, NAME, EINVAL, h_err_invalid_operand);  /* There shouldn't any command line parameters */
      
      if (!(i_status || b_abort))  /* Check it see if there was an error parsing the command line */
      {
         v_version();  /* Display version */
         debug(printf("%ux%u+%d+%d", i_window_width, i_window_height, i_window_left, i_window_top));
         i_screen = DefaultScreen(x_display);  /* Get default screen index */
         if ((x_window = XCreateSimpleWindow(x_display, RootWindow(x_display, i_screen), 0, 0, i_window_width, i_window_height, 4, BlackPixel(x_display, i_screen), WhitePixel(x_display, i_screen))))  /* Create window */ 
         {
            XSelectInput(x_display, x_window, FocusChangeMask | ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask);  /* Select the events we want to receive */
            wmDelete = XInternAtom(x_display, "WM_DELETE_WINDOW", False);  /* Close button atom */
            if (XSetWMProtocols(x_display, x_window, &wmDelete, 1))
            {
               if ((h_size_hint = XAllocSizeHints()))  /* Allocate size hints */
               {
                  /** h_size_hint->flags = PSize | PMinSize | PMaxSize;  /* Fixed size window - remove PMaxSize to make it resizable */
                  /**h_size_hint->flags = PSize | PPosition;  /* Set size and position */
                  /** h_size_hint->x = LEFT; h_size_hint->y = TOP; /* Set position */

                  h_size_hint->flags = PSize; /* Initial window size only allows program to resize window */
                  h_size_hint->max_width  = h_size_hint->min_width  = h_size_hint->width  = i_window_width;  /* Set initial width */
                  h_size_hint->max_height = h_size_hint->min_height = h_size_hint->height = i_window_height;  /* Set initial height */
                  if (b_geometry)  /* Update flags if geometry specified on command line otherwise let window manager position window */
                  {
                     h_size_hint->flags = h_size_hint->flags | USPosition;  /* Update flags */
                     h_size_hint->x = i_window_left;
                     h_size_hint->y = i_window_top;
                  } 
                  XSetStandardProperties(x_display, x_window, NAME, NAME, None, NULL, 0, h_size_hint);  /* Deprecated but works on current and legacy systems (Icon name doesn't have to be the same as the window name)  */
                  XSetWMNormalHints(x_display, x_window, h_size_hint);  /* Set the window size */
                  /** if (!(XStoreName(x_display, x_window, NAME))) v_error("Unable to set window name\n"); /* Set text shown in title bar name */
                  /** if (!XSetIconName(x_display, x_window, NAME)) v_error("Unable to set icon name\n"); /* Set text shown when window in minimized */
                  if (b_geometry) XMoveWindow(x_display, x_window, h_size_hint->x, h_size_hint->y);  /* Move window */
                  XFree(h_size_hint);  /* Free size hint */
                  x_root = RootWindow(x_display, i_screen);  /* Use a separate variable to ensure we pass a pointer to the root window */
                  if (XGetGeometry(x_display, x_window, &x_root, &i_window_left, &i_window_top, &i_window_width, &i_window_height, &i_window_border, &i_colour_depth))  /* Get window geometry */
                  {
                     XMapWindow(x_display, x_window);  /* Show window on display */
                     XRaiseWindow(x_display, x_window);  /* Raise window - ensures expose event is raised? */
                     x_colourmap = DefaultColormap(x_display, i_screen);  /*Create colour map */
                     if ((gc_background = XCreateGC(x_display, x_window, 0, NULL)))  /* Create a graphics content for the background */
                     {
                        if ((gc_foreground = XCreateGC(x_display, x_window, 0, NULL)))  /* Create a graphics content for the hands */
                        {
                           if ((gc_highlight = XCreateGC(x_display, x_window, 0, NULL)))  /* Create a graphics content for the second hand */
                           {
                              if ((gc_colour = XCreateGC(x_display, x_window, 0, NULL)))  /* Create a graphics content for the clock face */
                              {
                                 XSetForeground(x_display, gc_background, WhitePixel(x_display, i_screen));  /* Set background colour */
                                 XSetForeground(x_display, gc_colour, l_NamedColourPixel(x_display, i_screen, x_colourmap, "FloralWhite", WhitePixel(x_display, i_screen)));  /* Set foreground colour used by the clock face */
                                 XSetForeground(x_display, gc_foreground, l_NamedColourPixel(x_display, i_screen, x_colourmap, "Gray49", BlackPixel(x_display, i_screen)));  /* Set foreground colour used by main hands and dial */
                                 XSetForeground(x_display, gc_highlight, l_NamedColourPixel(x_display, i_screen, x_colourmap, "OrangeRed", BlackPixel(x_display, i_screen)));  /* Set foreground colour used by second hand */
                                 if ((x_buffer = XCreatePixmap(x_display, x_window, WIDTH, HEIGHT, i_colour_depth)))  /* Create pixmap to use for display  buffer */
                                 {
                                    b_abort = False;
                                    while (!b_abort)
                                    {
                                       while (XPending(x_display))
                                       {
                                          XNextEvent(x_display, &x_event);  /* Fetch next event */
                                          switch (x_event.type)
                                          {
                                             case KeyPress:  /* Keyboard input */
                                                XLookupString(&x_event.xkey, s_key, sizeof(s_key), &x_key, &x_key_state);
                                                debug(
                                                   const char* s_name = XKeysymToString(x_key);
                                                   if (s_name) printf("Key = %s", s_name); else printf("Key = unknown")
                                                   );
                                                if (x_key == XK_Escape || x_key == XK_space) b_abort = True;  /* Exit on Escape or Space */
                                                break;
                                             case ClientMessage:
                                                if ((Atom)x_event.xclient.data.l[0] == wmDelete) b_abort = True;  /* User closed window */
                                                break;
                                             case DestroyNotify:
                                                b_abort = True;  /* Window closed */
                                                break;
                                             case ConfigureNotify:  /* Window resized */  
                                                if (i_window_width != x_event.xconfigure.width || i_window_height != x_event.xconfigure.height)  /* Important - Don't do anything unless size changes */
                                                {
                                                   debug(printf("Window Resized"));
                                                   i_window_width = x_event.xconfigure.width;  /* Get new window size */
                                                   i_window_height = x_event.xconfigure.height;
                                                   b_resize = True;  /* Set flag to resize window - don't resize window here as it will raise another configure notify event! */
                                                }
                                                break;
                                          }  
                                       }  /* Done processing window events */

                                       if (b_resize)  /* Resize the window outside the event loop */
                                       {
                                          XResizeWindow(x_display, x_window, i_window_width, i_window_height);
                                          XFreePixmap(x_display, x_buffer);  /* Free existing buffer */
                                          if (!(x_buffer = XCreatePixmap(x_display, x_window, i_window_width, i_window_height, i_colour_depth)))  /* Create a new one to match the new windows size */
                                             b_abort = i_error(stderr, NAME, EXIT_WARN, h_err_pixmap); 
                                          b_resize = False;
                                       }

                                       if (!b_abort)
                                       {
                                          XFillRectangle(x_display, x_buffer, gc_background, 0, 0, i_window_width, i_window_height);  /* Fill the buffer with the window background colour */
                                          
                                          b_abort = i_draw_dial(x_display, x_buffer, gc_foreground, gc_colour, i_window_width, i_window_height);
                                          if (!b_abort)
                                          {
                                             b_abort = i_draw_hands(x_display, x_buffer, gc_foreground, gc_highlight, i_window_width, i_window_height);
                                             
                                             i_count = 10;
                                             while (!(XPending(x_display)) && i_count-- )  /* Don't pause if there are events pending */
                                                i_wait(15);  

                                             XCopyArea(x_display, x_buffer, x_window, gc_background, 0, 0, i_window_width, i_window_height, 0, 0);  /* Copy buffer to window */
                                             XSync(x_display, False);
                                          }
                                       }
                                    }
                                    XFreePixmap(x_display, x_buffer);
                                 }
                                 else
                                    i_error(stderr, NAME, EXIT_WARN, h_err_pixmap); 
                                 XFreeGC(x_display, gc_colour);
                              }
                              else
                                 i_error(stderr, NAME, EXIT_WARN, h_err_context);
                              XFreeGC(x_display, gc_highlight);
                           }
                           else
                              i_error(stderr, NAME, EXIT_WARN, h_err_context);
                           XFreeGC(x_display, gc_foreground);
                        }
                        else
                           i_error(stderr, NAME, EXIT_WARN, h_err_context);
                        XFreeGC(x_display, gc_background);
                     }
                     else
                        i_error(stderr, NAME, EXIT_WARN, h_err_context);
                  }
                     else
                        i_error(stderr, NAME, EXIT_WARN, h_err_display_properties);
               }
               else
                  i_error(stderr, NAME, EXIT_WARN, h_err_hint);
            }
            else
                i_error(stderr, NAME, EXIT_WARN, h_err_protocols);
            XDestroyWindow(x_display, x_window);
         }
         else
            i_error(stderr, NAME, EXIT_WARN, h_err_window);
         XCloseDisplay(x_display);
      }
   }
   else
      i_error(stderr, NAME, EXIT_WARN, h_err_display);
   exit(EXIT_SUCCESS);
}
