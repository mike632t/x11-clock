/*
 * x11-clock.c - A minimal clock for X11
 *
 * Copyright(C) 2019 - MT
 *
 * Converted  to  C  from the original C++ version (xclock.cc)  written  by
 * Frank Hale (frankhale@gmail.com) on 24 Jan 05.
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
 * 24 Sep 20   0.1   - Initial version - MT
 *                   - Compiles on Linux Tru64 and VMS - MT
 * 18 Mar 26   0.2   - Window can be moved using the mouse - MT
 *
 */

#include <X11/Xlib.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#include "gcc-wait.h"

#define  HEIGHT      24
#define  WIDTH       128
#define  BORDER      0

int text_width(Display *x_display, int i_screen, XFontStruct *x_font, char *s_text)
{
   int i_screen_width = XTextWidth(x_font, s_text, strlen(s_text));
   return i_screen_width;
}


int main()
{
   struct tm *t_time;
   time_t t_now;
   char s_time[9];  /* Include terminator */

   Display *x_display;
   XSetWindowAttributes x_window_attributes;
   XFontStruct *x_font;
   Window x_window;
   XEvent x_event;
   unsigned int i_screen_width, i_screen_height;
   unsigned int i_window_width, i_window_height;
   unsigned int i_window_border;
   int i_screen;
   int i_window_left, i_window_top;
   int i_mouse_left, i_mouse_top;
   int b_button_down = False;
   int b_continue = True;

   if ((x_display = XOpenDisplay(NULL)))
   {
      i_window_width = WIDTH;
      i_window_height = HEIGHT;
      i_window_border = BORDER;

      i_screen = DefaultScreen(x_display);
      i_screen_width = DisplayWidth(x_display, i_screen);
      i_screen_height = DisplayHeight(x_display, i_screen);
      i_window_left = (i_screen_width - i_window_width) / 2;  /* Default window position */
      i_window_top = (i_screen_height - i_window_height) / 2;
      x_window = XCreateSimpleWindow(x_display, RootWindow(x_display, i_screen), (i_screen_width - i_window_width) / 2, (i_screen_height - i_window_height) / 2, 
         i_window_width, i_window_height, i_window_border, BlackPixel(x_display, i_screen), WhitePixel(x_display, i_screen));
      x_window_attributes.override_redirect=True;
      XChangeWindowAttributes(x_display, x_window, CWOverrideRedirect, &x_window_attributes); /* Removes border from window */

      XMapWindow(x_display, x_window);
      XSelectInput (x_display, x_window, ExposureMask|ButtonPressMask|KeyPressMask);
      x_font = XQueryFont(x_display, XGContextFromGC(DefaultGC(x_display, i_screen)));

      while(b_continue)
      {
         while(XPending(x_display))
         {
            XNextEvent(x_display, &x_event);
            switch  (x_event.type)
            {
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
               case KeyPress:
                  b_continue = False;
                  break;
            }
         }
         t_now = time (NULL);
         t_time = localtime(&t_now);
         /** s_time = ctime(&t_now) + 11;  /* Skip date */
         sprintf(s_time, "%02d:%02d:%02d", t_time->tm_hour, t_time->tm_min, t_time->tm_sec);
         XClearWindow(x_display, x_window);
         XDrawString(x_display, x_window, DefaultGC(x_display, i_screen), (WIDTH - text_width(x_display, i_screen, x_font, s_time)) / 2, 15, s_time, strlen(s_time));

         XSync (x_display, False);
         i_wait(150);
      }
      XDestroyWindow(x_display, x_window);
      XCloseDisplay(x_display);
   }
   else
      fprintf(stderr,"Can't open display\n");
   return(EXIT_SUCCESS);
}
