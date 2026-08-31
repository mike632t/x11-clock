$  _verify := 'f$verify(0,0)
$!
$! Copyright(C) 2013   MT
$!
$! DCL command file to compile and link an X11 program.
$!
$! This  program is free software: you can redistribute it and/or modify it
$! under  the terms of the GNU General Public License as published  by  the
$! Free  Software Foundation, either version 3 of the License, or (at  your
$! option) any later version.
$!
$! This  program  is distributed in the hope that it will  be  useful,  but
$! WITHOUT   ANY   WARRANTY;   without even   the   implied   warranty   of
$! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
$! Public License for more details.
$!
$! You  should have received a copy of the GNU General Public License along
$! with this program.  If not, see <http://www.gnu.org/licenses/>.
$!
$! 23 Jul 13         - Initial version - MT
$! 22 Jan 26         - Updated for VMS on Alpha, Itanium and x86 - MT
$!
$  _message_status = f$environment("MESSAGE")
$  on error then goto _done
$  on control_Y then goto _done
$  _program = "x11-analog-clock"
$  _count = 0
$  if "''P1'" .eqs "" then goto _next
$  if "''P1'" .eqs "ALL" then goto _next
$  if "''P1'" .eqs "CLEAN" then goto _clean
$  write sys$error "Invalid command"
$  goto _done
$_clean:
$  if f$search("''_program'.exe") .nes. "" then delete '_program.exe;* /nolog /noconfirm
$  if f$search("''_program'.obj") .nes. "" then delete '_program.obj;* /nolog /noconfirm
$  goto _done
$_next:
$  cc '_program, gcc-wait, gcc-error, x11-clock-messages
$!  link '_program, gcc-wait, gcc-error, x11-clock-messages, x11-lib.opt/opt ! VAX
$  link '_program, gcc-wait, gcc-error, x11-clock-messages, x11-lib.opt/opt ! Alpha, Itanium, x86
$  if f$search("*.obj") .nes. "" then delete *.obj;* /nolog /noconfim
$  goto _done
$_invalid:
$  write sys$output "Error"
$_done:
$  set message 'Message_Status'
$  exit

