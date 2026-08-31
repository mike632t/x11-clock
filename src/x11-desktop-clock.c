/*
 * x11-desktop-clock.c - Yet another analogue clock!
 *
 * Copyright(C) 2026 - MT
 *
 * An  analogue  clock implemented in X11 using a transparent  border  less
 * window to create a desktop widget...
 * 
 * The alpha channel (high byte) controls the 'transparency' of each colour 
 * which actually just controls how the colour and the background are mixed
 * to give the illusion of transparency.    
 * 
 * Deliberately written in a 'top down' style in part because it that style
 * seems to fit with the way X11 works.  
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
 * 17 Feb 26         - Allow window to be resized using keyboard - MT
 * 18 Feb 26         - More extensive error handling - MT
 * 21 Feb 26         - Uses a border less window and transparent background
 *                     (which requires a 32-bit colour display) to draw the
 *                     clock - MT
 * 21 Feb 26         - Window size was calculated incorrectly - MT
 *                   - Allocating pixmap returns correct status - MT
 *                   - Don't resize the window while processing events - MT
 *                   - Allows the window can be drawn either on the desktop 
 *                     or on top of all the other windows - MT 
 * 23 Feb 26         - Added a command line parser allowing the user to set 
 *                     the size and position of the window and if it should
 *                     be shown on the desktop or stay on top - MT
 *                   - XCloseDisplay() now called in the right place - MT
 * 24 Feb 26   0.2   - Moved error handling, messages and keyboard routines
 *                     to separate modules - MT
 *                   - Avoids triggering any window events while processing 
 *                     an event - MT
 *                   - Uses a bitmap mask to define a circular window - MT
 *                   - Using hints to fix minimum windows size doesn't work
 *                     and causes conflicts between the application and the
 *                     window manager - MT
 * 26 Feb 26         - Checks that the window is bigger then a minimum size
 *                     when creating or resizing the window - MT
 * 28 Feb 26  (0023) - Renamed error handler module - MT
 *                   - Initialize status properly - MT
 * 03 Mar 26         - Separated clock drawing code into two functions - MT 
 *                   - Reinstated compatibility with older systems that are 
 *                     not able to use clock_gettime() - MT
 * 06 Mar 26         - Resize window only if it has focus - MT
 * 
 * To Do             - Add option to hide second hand.
 *                   - Hide second hand automatically depending on the size
 *                     of the window.
 *                   - Save size and position on exit.
 * 
 */

#define  NAME           "x11-desktop-clock"
#define  VERSION        "0.2"
#define  BUILD          "0026"
#define  DATE           "03 Mar 26"
#define  AUTHOR         "MT"

#define  NOBORDER
#define  POSIX

#define  WIDTH          256   /* Initial window size */
#define  HEIGHT         256
#define  PADDING        0     /* Don't need any padding between edge of window and clock face */
#define  MINIMUM        64    /* Do not use a minimum size less than 16 that gets pretty hard to see! */

#include <errno.h>      /* errno */
#include <stdarg.h>     /* vargs(), va_list */
#include <string.h>     /* strlen(), etc */
#include <stdio.h>      /* fprintf(), stderr */
#include <stdlib.h>     /* exit(), getenv() */
#include <ctype.h>      /* tolower(), isalpha() */

#include <time.h>       /* time(), localtime() */
#include <math.h>       /* sin(), cos() */

#include <X11/Xlib.h>   /* XOpenDisplay(), True/False etc */
#include <X11/Xutil.h>  /* XSizeHints etc */
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>  /* XSizeHints */
#include <X11/keysym.h>
#include <X11/extensions/shape.h>

#include "gcc-debug.h"  /* debug() */
#include "gcc-error.h"  /* error() */
#include "gcc-wait.h"   /* wait() */

#include "x11-keyboard.h"
#include "x11-desktop-clock.h"
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


static void v_apply_circular_mask(Display *x_display, Window x_window, int i_window_width, int i_window_height)
{
    Pixmap x_shape_mask = XCreatePixmap(x_display, x_window, i_window_width, i_window_height, 1);
    GC gc_shape = XCreateGC(x_display, x_shape_mask, 0, NULL);
    int i_radius = (MIN(i_window_width, i_window_height) - PADDING) / 2;
    int i_centre_left = i_window_width / 2;
    int i_centre_top = i_window_height / 2;
    
    XSetForeground(x_display, gc_shape, 0);
    XFillRectangle(x_display, x_shape_mask, gc_shape, 0, 0, i_window_width, i_window_height);
    XSetForeground(x_display, gc_shape, 1);
    XFillArc(x_display, x_shape_mask, gc_shape, i_centre_left - i_radius, i_centre_top - i_radius, i_radius * 2, i_radius * 2, 0, 360 * 64);
    XShapeCombineMask(x_display, x_window, ShapeBounding, 0, 0, x_shape_mask, ShapeSet);
    XFreeGC(x_display, gc_shape);
    XFreePixmap(x_display, x_shape_mask);
}


static unsigned long l_ColourPixel(Display *x_display, int i_screen, Colormap x_colourmap, const char *s_name)  /* Convert a named colour to a pixel value */
{
   XColor x_screen_colour, x_exact_colour;

   if (!XAllocNamedColor(x_display, x_colourmap, s_name, &x_screen_colour, &x_exact_colour)) 
   {
      i_error(stderr, NAME, EXIT_WARN, h_err_colour, s_name);
      x_screen_colour.pixel = BlackPixel(x_display, i_screen);  /* Fall back to black */ 
   }

   return x_screen_colour.pixel;  /* Return pixel colour */
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


static Window x_current_window(Display *x_display)  /* Returns a pointer to the window that currently has focus */
{
    Window x_window;
    int x_current;
    XGetInputFocus(x_display, &x_window, &x_current);  /* Get the window that currently has input focus */
    return x_window;
}


static int i_resize_window(Display *x_display, Window x_window, Pixmap *x_pixmap, int *i_window_left, int *i_window_top, 
   unsigned int *i_window_width, unsigned int *i_window_height, unsigned int i_original_width, unsigned int i_original_height, 
   unsigned int i_screen_width, unsigned int i_screen_height, int *i_scale, int i_colour_depth) 
{
   int i_status = 0;
   int i_temp;
   
   /* Update window size based on scale */
   i_temp = *i_window_width;
   *i_window_width = MAX(i_original_width * (7 + *i_scale) / 8, MINIMUM);  /* Resize width based on scale */
   *i_window_left = *i_window_left - (*i_window_width - i_temp) / 2;  /* Keep the centre of the window in the same place */

   i_temp = *i_window_height;
   *i_window_height = MAX(i_original_height * (7 + *i_scale) / 8, MINIMUM);  /* Resize height based on scale */
   *i_window_top = *i_window_top - (*i_window_height - i_temp) / 2;  /* Keep the centre of the window in the same place */

   XFreePixmap(x_display, *x_pixmap);  /* Free existing buffer */

   if (!(*x_pixmap = XCreatePixmap(x_display, x_window, *i_window_width, *i_window_height, i_colour_depth)))  /* Create a new buffer to match the new window size */
      i_status = i_error(stderr, NAME, EXIT_WARN, h_err_pixmap); 
   XMoveResizeWindow(x_display, x_window, *i_window_left, *i_window_top, *i_window_width, *i_window_height);  /* Resize window */
   v_apply_circular_mask(x_display, x_window, *i_window_width, *i_window_height);  /* Apply any window mask */
   return (i_status);
}


int main(int argc, const char *argv[])
{
   Display *x_display;
   Window x_window, x_root;
   Pixmap x_buffer;
   XEvent x_event;
   XSizeHints *h_size_hint;
   Atom x_wm_type, x_dock, x_wm_state, x_below;  
   Atom wmDelete;
   Colormap x_colourmap;
   GC gc_background, gc_foreground, gc_highlight, gc_colour;
   
   XVisualInfo x_visual_info;
   XSetWindowAttributes x_window_attributes;

   okeyboard *h_keyboard;
   
   long unsigned int l_background, l_foreground, l_highlight, i_colour;
   unsigned int i_screen_width, i_screen_height;
   unsigned int i_window_width, i_window_height, i_window_border, i_colour_depth;
   unsigned int i_original_width, i_original_height;
   int i_screen;
   int i_window_left, i_window_top;
   int i_mouse_left, i_mouse_top;
   int i_count, i_offset, i_position;
   int i_scale = 1;
   int i_status = 0;
   int i_result;
   int i_temp;
   int b_topmost = False;
   int b_abort = False;
   int b_button_down = False;
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
      
      for (i_count = 1; i_count < argc && (b_abort != True); i_count++)
      {
         if (argv[i_count][0] == '-')
         {
            i_position = 1;
            while ((argv[i_count][i_position] != 0) && (b_abort != True))
            {
               switch (argv[i_count][i_position])
               {
               case 'v':  /* Display version */
                  v_version();  /* Display version information */
                  v_print(stdout, h_msg_licence, &__DATE__[7], AUTHOR);
                  b_abort = True;  /* or exit(EXIT_SUCCESS) */
                  break;
               case 't':  /* Position window on top */
                  b_topmost = True;
                  break;
               case '-':  /* '--' terminates command line processing */
                  i_position = strlen(argv[i_count]);
                  if (i_position == 2)
                    b_abort = True;  /* '--' terminates command line processing */
                  else
                  {
                     if (!strncmp(argv[i_count], "--geometry=", 11))  /* Just check the first 11 characters match */
                     {
                        const char *c_geometry = argv[i_count] + 11;
                        
                        if ((i_result = XParseGeometry(c_geometry,  &i_window_left, &i_window_top, &i_window_width, &i_window_height)))
                        {
                           if (!(i_result & XValue) && !(i_result & YValue))  /* If neither positional parameter was specified centre the window on the screen (again) since the default position will have been changed */
                           {
                              i_window_left = (i_screen_width - i_window_width) / 2;
                              i_window_top = (i_screen_height - i_window_height) / 2;
                           }
                        }
                        else
                           i_status = i_error(stderr, NAME, EINVAL, h_err_geometry, c_geometry);
                     }
                     else if (!strncmp(argv[i_count], "--geometry", i_position))
                     {
                        if (i_count + 1 < argc)
                        {
                           const char *c_geometry = argv[i_count + 1];
                           if ((i_result = XParseGeometry(c_geometry,  &i_window_left, &i_window_top, &i_window_width, &i_window_height)))
                           {
                              if (!(i_result & XValue) && !(i_result & YValue))  /* If neither positional parameter was specified centre the window on the screen (again) since the default position will have been changed */
                              {
                                 i_window_left = (i_screen_width - i_window_width) / 2;
                                 i_window_top = (i_screen_height - i_window_height) / 2;
                              }
                              if (i_count + 2 < argc)  /* Remove the parameter from the arguments */
                                 for (i_offset = i_count + 1; i_offset < argc - 1; i_offset++)
                                    argv[i_offset] = argv[i_offset + 1];
                              argc--;
                           }
                           else
                              i_status = i_error(stderr, NAME, EINVAL, h_err_geometry, c_geometry);
                        }
                        else
                           i_status = i_error(stderr, NAME, EINVAL, h_err_missing_argument, argv[i_count]);
                     }
                     else if (!strncmp(argv[i_count], "--topmost", i_position))/* Position window on top */
                     {
                        b_topmost = True;
                     }
                     else if (!strncmp(argv[i_count], "--version", i_position))
                     {
                        v_version();  /* Display version information */
                        v_print(stdout, h_msg_licence, &__DATE__[7], AUTHOR);
                        b_abort = True;  /* or exit(EXIT_SUCCESS) */
                     }
                     else if (!strncmp(argv[i_count], "--help", i_position))
                     {
                        v_print(stdout, h_msg_usage, NAME);
                        b_abort = True;  /* or exit(EXIT_SUCCESS) */
                     }
                     else  /* If we get here then the we have an invalid long option */
                        i_status = i_error(stderr, NAME, EINVAL, h_err_unrecognised_option, argv[i_count]);
                  }
                  i_position--;  /* Leave index pointing at end of string (so argv[i_count][i_position] = 0) */
                  break;
               default:  /* If we get here the single letter option is unknown */
                  i_status = i_error(stderr, NAME, EINVAL, h_err_invalid_option, argv[i_count][i_position]);
                  i_position = strlen(argv[i_count]) - 1;
               }
               i_position++;  /* Parse next letter in option */
            }
            if ((argv[i_count][1] != 0) && (! i_status))
            {
               for (i_offset = i_count; i_offset < argc - 1; i_offset++)
                  argv[i_offset] = argv[i_offset + 1];
               argc--; i_count--;
            }
         }
      }      
      
      if (! (i_status || b_abort))  /* Check it see if there was an error parsing the command line */
      {
         v_version();  /* Display version */
         i_window_width = MAX(i_window_width, MINIMUM);  /* Initial windows size cannot be less than MINIMUM */
         i_window_height = MAX(i_window_height, MINIMUM);
         i_original_width = i_window_width;  /* Save windows size */
         i_original_height = i_window_height;
         h_keyboard = h_keyboard_create(x_display);
         x_root = RootWindow(x_display, i_screen);  /* Use a separate variable to ensure we pass a pointer to the root window */
         
         if (XMatchVisualInfo(x_display, i_screen, 32, TrueColor, &x_visual_info))  /* Try to locate a 32-bit ARGB visual */
         {
            x_window_attributes.colormap = XCreateColormap(x_display, x_root, x_visual_info.visual, AllocNone);  /* Prepare window attributes */
            x_window_attributes.border_pixel = BlackPixel(x_display, i_screen);
            x_window_attributes.background_pixel = BlackPixel(x_display, i_screen);
            x_window_attributes.override_redirect = b_topmost;  /* Override the window manager to prevent it from managing the window */
            if ((x_window = XCreateWindow(x_display, x_root, i_window_left, i_window_top, i_window_width, i_window_height, 0,
               x_visual_info.depth, InputOutput, x_visual_info.visual,
               CWColormap | CWBorderPixel | CWBackPixel | CWOverrideRedirect, &x_window_attributes)))  /* Create window */
            {
               if (!(b_topmost))  /* Update window properties if it is a managed window to keep it below all other windows and out of the task bar */
               {
                  x_wm_type = XInternAtom(x_display, "_NET_WM_WINDOW_TYPE", False);
                  x_dock = XInternAtom(x_display, "_NET_WM_WINDOW_TYPE_DOCK", False);
                  XChangeProperty(x_display, x_window, x_wm_type, XA_ATOM, 32, PropModeReplace, (unsigned char *) &x_dock, 1);
                  x_wm_state = XInternAtom(x_display, "_NET_WM_STATE", False);
                  x_below = XInternAtom(x_display, "_NET_WM_STATE_BELOW", False);
                  XChangeProperty(x_display, x_window, x_wm_state, XA_ATOM, 32, PropModeReplace, (unsigned char *) &x_below, 1);
               }
               XSelectInput(x_display, x_window, FocusChangeMask | ExposureMask | KeyPressMask | KeyReleaseMask |
                  ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask);  /* Select the events we want to receive */
               wmDelete = XInternAtom(x_display, "WM_DELETE_WINDOW", False);  /* Close button atom */
               if (XSetWMProtocols(x_display, x_window, &wmDelete, 1))
               {
                  if ((h_size_hint = XAllocSizeHints()))  /* Allocate size hints */
                  {
                     h_size_hint->flags = PSize; /* Initial window size only allows program to resize window */
                     h_size_hint->max_width  = h_size_hint->min_width  = h_size_hint->width  = i_window_width;  /* Set initial width */
                     h_size_hint->max_height = h_size_hint->min_height = h_size_hint->height = i_window_height;  /* Set initial height */
                     XSetStandardProperties(x_display, x_window, NAME, NAME, None, NULL, 0, h_size_hint);  /* Deprecated but works on current and legacy systems (Icon name doesn't have to be the same as the window name)  */
                     XFree(h_size_hint);  /* Free size hint */
                     if (XGetGeometry(x_display, x_window, &x_root, &i_window_left, &i_window_top, &i_window_width, 
                        &i_window_height, &i_window_border, &i_colour_depth))  /* Get window geometry */
                     {
                        XMapWindow(x_display, x_window);  /* Show window on display */
                        XRaiseWindow(x_display, x_window);  /* Raise window - ensures expose event is raised? */
                        x_colourmap = DefaultColormap(x_display, i_screen);  /*Create colour map */
                        if ((gc_background = XCreateGC(x_display, x_window, 0, NULL)))  /* Create graphics content for the window background */
                        {
                           XSetBackground(x_display, gc_background, 0x00000000);
                           if ((gc_foreground = XCreateGC(x_display, x_window, 0, NULL)))  /* Create graphics content for the hands */
                           {
                              if ((gc_highlight = XCreateGC(x_display, x_window, 0, NULL)))  /* Create graphics content for the second hand */
                              {
                                 if ((gc_colour = XCreateGC(x_display, x_window, 0, NULL)))  /* Create graphics content for the shaded background to the clock face */
                                 {
                                    l_background = 0x00000000; /* Must clear _all_ bits for a transparent window  */
                                    i_colour = (l_ColourPixel(x_display, i_screen, x_colourmap, "Black")) | 0x10000000;
                                    l_foreground = (l_ColourPixel(x_display, i_screen, x_colourmap, "DarkGrey")) | 0xff000000;  /* The alpha channel byte controls 'transparency'  */
                                    l_highlight = (l_ColourPixel(x_display, i_screen, x_colourmap, "OrangeRed")) | 0xff000000;
                                    XSetForeground(x_display, gc_background, l_background) ;  /* Set window background colour */
                                    XSetForeground(x_display, gc_colour, i_colour);  /* Set foreground colour used by the clock face */
                                    XSetForeground(x_display, gc_foreground, l_foreground);  /* Set foreground colour used by main hands and dial */
                                    XSetForeground(x_display, gc_highlight, l_highlight);  /* Set foreground colour used by second hand with alpha */
                                    
                                    if ((x_buffer = XCreatePixmap(x_display, x_window, i_window_width, i_window_height, i_colour_depth)))  /* Create pixmap to use for display  buffer */
                                    {
                                       b_abort = False;
                                       while (!b_abort)
                                       {
                                          while (XPending(x_display))
                                          {
                                             XNextEvent(x_display, &x_event);  /* Fetch next event */
                                             switch (x_event.type)
                                             {
                                                case KeyPress :
                                                   if (!((x_event.xkey.state & KeyboardMask) & ~(h_keyboard->NumLockMask | ShiftMask | ControlMask)))  /* Guard against invalid modifiers */
                                                   {
                                                      v_key_pressed(h_keyboard, x_display, &x_event.xkey);  /* Translate a key code into a character */
                                                      if (x_event.xkey.state & ControlMask)  /* Process any control keys */
                                                      {
                                                         switch (h_keyboard->key)
                                                         {
                                                            case (XK_0 & 0x7f):  /* Ctrl-0 to reset window size */
                                                               i_scale = 1;  /* Reset size */
                                                               i_temp = i_window_width;
                                                               i_window_width  = i_original_width;
                                                               i_window_left =  i_window_left - (i_window_width - i_temp) / 2;  /* Keep the centre of window in the same place */
                                                               i_temp = i_window_height;
                                                               i_window_height = i_original_height;
                                                               i_window_top =  i_window_top - (i_window_height - i_temp) / 2;  /* Keep the centre of window in the same place */
                                                               b_resize = True;  /* Set flag to resize window */
                                                               break;
                                                            case (XK_plus & 0x7f):  /* Ctrl-Plus to increase window size */
                                                               if (MIN(i_original_width, i_original_height) * (8 + i_scale) / 8 < MIN(i_screen_width, i_screen_height))  /* Don't let it get bigger then the screen */
                                                               {
                                                                  i_scale++;  /* Grow by +12.5% */
                                                                  b_resize = True;
                                                               }                                                            
                                                               break;
                                                            case (XK_minus & 0x7f):  /* Ctrl-Minus to decrease windows size */
                                                               if (MIN(i_original_width, i_original_height) * (6 + i_scale) / 8 > 3 * PADDING)   /* Don't let window size become zero ! */
                                                               {
                                                                  i_scale--;  /* Shrink by 12.5% */
                                                                  b_resize = True;
                                                               }
                                                               break;
                                                            case (XK_bracketleft & 0x7f):  /* Ctrl-[ is Escape */
                                                               b_abort = True;  /* Exit on Escape */
                                                               break;
                                                         }
                                                      }
                                                      else
                                                         if (h_keyboard->key == (XK_Escape & 0x7f)) b_abort = True;  /* Exit on Escape */
                                                   }
                                                   break;
                                                case ButtonPress:
                                                   switch (x_event.xbutton.button)
                                                   {
                                                      case Button1:
                                                         b_button_down = True;
                                                         i_mouse_left = x_event.xbutton.x_root;
                                                         i_mouse_top  = x_event.xbutton.y_root;
                                                         XSetInputFocus(x_display, x_window, RevertToParent, CurrentTime);  /* Explicitly set input focus when user clicks on the window */
                                                         XGrabPointer(x_display, x_window, True, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None, CurrentTime);  /* Grab the pointer so we keep getting motion events */
                                                         break;
                                                   }
                                                   break;
                                                case ButtonRelease:
                                                   switch (x_event.xbutton.button)
                                                   {
                                                      case Button1:
                                                         b_button_down = False;
                                                         XUngrabPointer(x_display, CurrentTime);  /* Release the pointer when drag ends */
                                                         break;
                                                      case Button4:
                                                         if (!((x_event.xkey.state & KeyboardMask) & ~(h_keyboard->NumLockMask | ShiftMask | ControlMask)))  /* Guard against invalid modifiers */
                                                            if (x_event.xkey.state & ControlMask && (x_current_window(x_display) == x_window))  /* Resize only if control key is pressed and window has focus */
                                                               if (MIN(i_original_width, i_original_height) * (8 + i_scale) / 8 < MIN(i_screen_width, i_screen_height))  /* Don't let it get bigger then the screen */
                                                               {
                                                                  i_scale++;  /* Grow by +12.5% */
                                                                  b_resize = True;
                                                               } 
                                                         break;
                                                      case Button5:
                                                         if (!((x_event.xkey.state & KeyboardMask) & ~(h_keyboard->NumLockMask | ShiftMask | ControlMask)))  /* Guard against invalid modifiers */
                                                            if (x_event.xkey.state & ControlMask && (x_current_window(x_display) == x_window))  /* Resize only if control key is pressed and window has focus */
                                                               if (MIN(i_original_width, i_original_height) * (6 + i_scale) / 8 > 3 * PADDING)   /* Don't let window size become zero ! */
                                                               {
                                                                  i_scale--;  /* Shrink by 12.5% */
                                                                  b_resize = True;
                                                               }
                                                         break;
                                                   }
                                                   break;
                                                case MotionNotify:
                                                   if (b_button_down) 
                                                   {
                                                      int dx = x_event.xmotion.x_root - i_mouse_left;
                                                      int dy = x_event.xmotion.y_root - i_mouse_top;

                                                      XMoveWindow(x_display, x_window, i_window_left + dx, i_window_top + dy);  /* Move window relative to current position */
                                                      i_window_left += dx;  /* Update stored position */
                                                      i_window_top  += dy;
                                                      i_mouse_left = x_event.xmotion.x_root;  /* Update mouse position start so motion is incremental */
                                                      i_mouse_top  = x_event.xmotion.y_root;
                                                   }
                                                   break;
                                                case ClientMessage:
                                                   if ((Atom)x_event.xclient.data.l[0] == wmDelete) b_abort = True;  /* User closed window */
                                                   break;
                                                case DestroyNotify:
                                                   b_abort = True;  /* Window closed */
                                                   break;
                                                case ConfigureNotify:  /* Window resized */   
                                                   if (i_window_width != x_event.xconfigure.width || i_window_height != x_event.xconfigure.height)  /* Don't do anything unless size changes  */
                                                   {
                                                      i_window_width = x_event.xconfigure.width;  /* Get new window size */
                                                      i_window_height = x_event.xconfigure.height;
                                                      b_resize = True;  /* Set flag to resize window - don't resize window here as it will raise another configure notify event! */
                                                   }
                                                   break;
                                             }  
                                          }  /* Done processing window events */

                                          if (b_resize)  /* Resize the window outside the event loop */
                                          {
                                             i_status = i_resize_window(x_display, x_window, &x_buffer, &i_window_left, &i_window_top, &i_window_width, &i_window_height, 
                                                i_original_width, i_original_height, i_screen_width, i_screen_height, &i_scale, i_colour_depth);
                                             b_resize = False;
                                          }

                                          if (i_status == 0)  /* Check status as it gets reset below otherwise */
                                          {
                                             XFillRectangle(x_display, x_buffer, gc_background, 0, 0, i_window_width, i_window_height);  /* Fill the buffer with the window background colour */
                                             
                                             i_status = i_draw_dial(x_display, x_buffer, gc_foreground, gc_colour, i_window_width, i_window_height);
                                             if (i_status == 0)
                                             {
                                                i_status = i_draw_hands(x_display, x_buffer, gc_foreground, gc_highlight, i_window_width, i_window_height);
                                                
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
                                       i_status = i_error(stderr, NAME, EXIT_WARN, h_err_pixmap); 
                                    XFreeGC(x_display, gc_colour);
                                 }
                                 else
                                    i_status = i_error(stderr, NAME, EXIT_WARN, h_err_context);
                                 XFreeGC(x_display, gc_highlight);
                              }
                              else
                                 i_status = i_error(stderr, NAME, EXIT_WARN, h_err_context);
                              XFreeGC(x_display, gc_foreground);
                           }
                           else
                              i_status = i_error(stderr, NAME, EXIT_WARN, h_err_context);
                           XFreeGC(x_display, gc_background);
                        }
                        else
                           i_status = i_error(stderr, NAME, EXIT_WARN, h_err_context);
                     }
                     else
                        i_status = i_error(stderr, NAME, EXIT_WARN, h_err_display_properties);
                  }
                  else
                     i_status = i_error(stderr, NAME, EXIT_WARN, h_err_hint);
               }
               else
                  i_status = i_error(stderr, NAME, EXIT_WARN, h_err_protocols);
               XDestroyWindow(x_display, x_window);
            }
            else
               i_status = i_error(stderr, NAME, EXIT_WARN, h_err_window);
         }
         else
            i_status = i_error(stderr, NAME, EXIT_WARN, h_err_display_colour, i_colour_depth);
      }
      XCloseDisplay(x_display);
   }
   else
      i_status = i_error(stderr, NAME, EXIT_WARN, h_err_display);
   exit(i_status);
}
