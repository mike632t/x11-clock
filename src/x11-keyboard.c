/*
 * x11-keyboard.c - An X11 keyboard class.
 *
 * Copyright(C) 2021   MT
 *
 * A  very simple keyboard 'class' to translate keystrokes into basic ASCII
 * character codes using the key definitions in keysymdef.h.
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
 * 24 Feb 26    0.1  - Initial version  (derived from x11-calc) - MT
 *
 */

#include <stdio.h>     /* fprintf(), etc. */
#include <stdlib.h>    /* malloc(), etc. */
#include <ctype.h>     /* is alpha(), etc. */

#include <X11/Xlib.h>  /* X11 definitions - required before Xutil */
#include <X11/Xutil.h> /* XSizeHints etc. */
#include <X11/keysym.h>

#include "x11-keyboard.h"

#include "gcc-debug.h"

/*
 * key_decode(keyboard, display, keycode, keystate, numlock)
 *
 * Attempts to translate a key code into an ASCII character.
 *
 */

static void v_key_decode(okeyboard *h_keyboard, Display *x_display, XKeyEvent *x_event)  /* Static function (not exposed when linking) */
{
   static XComposeStatus x_compose_status;
   KeySym x_keysym;
   char x_key_buffer[4];

   XLookupString(x_event, x_key_buffer,sizeof(x_key_buffer), &x_keysym, &x_compose_status);  /* Get key and key state but don't return the name string */
   h_keyboard->keysym = x_keysym;
   h_keyboard->key = x_key_buffer[0];
   h_keyboard->name = XKeysymToString(x_keysym);
   debug (
      if (h_keyboard->name) printf("Name = %-12s ", h_keyboard->name); else printf("Key = %-24s  ", "Unknown");
      if (isprint(h_keyboard->key)) printf("Char = `%c`  ", h_keyboard->key); else printf("Char = %-4d ", h_keyboard->key);
      printf("State = %-6d", x_event->state);
      printf("Key = %6d  ASCII = %-4d  Code = %-4d", (int)x_keysym, (int)x_keysym & 0x7f, (int)x_keysym & 0x1f);
      if (x_keysym == XK_BackSpace) printf("Backspace")
      );
}

/*
 * get_numlock_mask(display)
 *
 * Since the mask used to represent the state of the NumLock key is not the
 * same on every system we need to search the keyboard modifier map to find
 * which one corresponds to the NumLock key.
 *
 */

static unsigned int u_get_numlock_mask(Display *x_display)
{
   XModifierKeymap *h_modmap;
   KeyCode u_keycode;
   unsigned int u_mask = 0;
   int i_count, i_counter, i_offset;

   /* Get the keycode that represents XK_Num_Lock on this keyboard.
    * This tells us which physical key is the NumLock key.
    */
   u_keycode = XKeysymToKeycode(x_display, XK_Num_Lock);

   /* If the keyboard/layout has no NumLock key, we cannot map it. */
   if (u_keycode != 0) {

      /* Get  the key modifier map which contains 8 modifier slots (one for
       * Shift, Lock, Control, and Mod1 - Mod5), with slot listing all  the
       * keycodes that activate that modifier */

      h_modmap = XGetModifierMapping(x_display);

      if (h_modmap != NULL)
      {
         for (i_count = 0; i_count < 8; i_count++)  /* Search each slot */
         {
            for (i_counter = 0; i_counter < h_modmap->max_keypermod; i_counter++)  /* Scan each key code in each modifier map */
            {
               i_offset = i_count * h_modmap->max_keypermod + i_counter;  /* Find the offset into the flat modifier map */
               if (h_modmap->modifiermap[i_offset] == u_keycode)  /* If we find an entry that matches the NumLock keycode, then we have found it */
               {
                  /* Convert slot index into the correct modifier mask bit
                   * Slot 0 = ShiftMask,
                   * Slot 1 = LockMask,
                   * Slot 2 = ControlMask,
                   * slot 3 = Mod1Mask,
                   * Slot 4 = Mod2Mask, etc
                   */
                  u_mask = (unsigned int)(1u << i_count);  /* Set the mask */
               }
            }
         }
         XFreeModifiermap(h_modmap);  /* Free the modifier map */
      }
   }
   return u_mask;  /* Return the NumLockMask */
}

/*
 * key_pressed(keyboard, display, keycode, keystate, numlock)
 *
 * Calls key_decode() to update keycode when key is pressed.
 *
 */

void v_key_pressed(okeyboard *h_keyboard, Display *x_display, XKeyEvent *x_event)
{
   v_key_decode(h_keyboard, x_display, x_event);
}

/*
 * key_released (keyboard, keycode)
 *
 * Updates the keyboard state when a key is released.
 *
 */
void v_key_released(okeyboard *h_keyboard, Display *x_display, XKeyEvent *x_event)
{
   v_key_decode(h_keyboard, x_display, x_event);
}

/*
 * keyboard_create (index)
 *
 * Allocates storage for a new keyboard object, sets the default key states
 * and  returns a pointer to the keyboard object (or NULL to indicate  that
 * an error occurred when allocating memory).
 *
 */

okeyboard *h_keyboard_create(Display *x_display) {
   okeyboard *h_keyboard; /* Pointer to keyboard structure. */
   if ((h_keyboard = malloc (sizeof(*h_keyboard))) != NULL){
      h_keyboard->key = '\000';
      h_keyboard->name = NULL;
      h_keyboard->keysym = 0x0000;
      h_keyboard->NumLockMask = u_get_numlock_mask(x_display);
   }
   else
      h_keyboard = NULL;
   return(h_keyboard);
}
