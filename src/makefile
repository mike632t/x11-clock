#
#  src/makefile
#
#  Copyright(C) 2026 - MT
#
#  Operating-system makefile dispatcher.
#
#  The operating-system name is obtained from uname. NetBSD and FreeBSD are
#  both handled by the common BSD makefile.
#
#  The platform-specific makefile supplies compiler and linker settings and
#  then include makefile.all for the common build rules.
#
#  This  program is free software: you can redistribute it and/or modify it
#  under  the terms of the GNU General Public License as published  by  the
#  Free  Software Foundation, either version 3 of the License, or (at  your
#  option) any later version.
#
#  This  program  is distributed in the hope that it will  be  useful,  but
#  WITHOUT   ANY   WARRANTY;   without even   the   implied   warranty   of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
#  Public License for more details.
#
#  You  should have received a copy of the GNU General Public License along
#  with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#  Note separator (tab) at the beginning of the line CANNOT be a space..!
#
#  29 Aug 26         - Initial version - MT
#

all:
	@_os="`uname -s | tr '[A-Z]' '[a-z]'`"; \
	case "$$_os" in netbsd|freebsd) echo '**** bsd'; _os=bsd ;; esac;\
	$(MAKE) -s -f "makefile.$$_os" PROGRAM=$(PROGRAM) SOURCES="$(SOURCES)" COMMON="$(COMMON)" $@

.DEFAULT:
	@_os="`uname -s | tr '[A-Z]' '[a-z]'`"; \
	case "$$_os" in netbsd|freebsd) echo '**** bsd'; _os=bsd ;; esac;\
	$(MAKE) -s -f "makefile.$$_os" PROGRAM=$(PROGRAM) SOURCES="$(SOURCES)" COMMON="$(COMMON)" $@
