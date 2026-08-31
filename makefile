#
#  makefile
#
#  Copyright(C) 2026 - MT
#
#  Top-level make file.
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
#  09 Oct 21         - Initial version - MT
#  29 Aug 26         - Reworked  to provide a common set of cross  platform 
#                      make files - MT
#
#  To Do             - Fix backup.
#

PROJECT	= x11-clock

BIN	= bin
SRC	= src
IMG	= img

# Program-specific source files.
SOURCES	= x11-clock.c x11-analog-clock.c x11-desktop-clock

# Source files shared by all programs.
COMMON	= gcc-wait.c gcc-error.c x11-keyboard.c x11-clock-messages.c

# Derived object-file lists.
OBJECTS	= $(SOURCES:.c=.o)
MODULES	= $(COMMON:.c=.o)

# Executable names are derived from the program source names.
PROGRAMS= $(SOURCES:.c=)

# Additional files included by the backup target.
FILES	= README.md LICENSE

.PHONY: default $(PROGRAMS) all common clean backup

# Default target: build all programs incrementally.
default: $(PROGRAMS)

# All programs depend on the common modules. This dependency ensures that
# common modules are built once before the individual program builds,
# including when make is run in parallel.
$(PROGRAMS): common
	@cd $(SRC); $(MAKE) -s PROGRAM=$@ SOURCES="$(SOURCES)" COMMON="$(COMMON)" all

# Explicit full rebuild: clean first, then build the common modules and
# finally all programs.
all: clean common
	@$(MAKE) -s $(PROGRAMS)

# Build/update only the common modules.
common:
	@cd $(SRC); $(MAKE) -s COMMON="$(COMMON)" common

# Remove generated object files and _any_ files in BIN.
clean:
	@rm -f $(SRC)/*.o
	@rm -f $(BIN)/*  

# Create a source/build-system backup archive.
backup:
	@_date=`date +'%Y%m%d%H%M'`;_branch="`command -v git >/dev/null 2>&1 && git rev-parse --abbrev-ref HEAD 2>/dev/null || echo ""`"; \
	if [ -z "$$_branch" ]; then \
		archive="$(PROJECT)-$$_date.tar"; \
	else \
		archive="$(PROJECT)-$$_branch-$$_date.tar"; \
	fi; \
	echo tar -cpf ../$$archive $(FILES) -C src $(SOURCES) $(COMMON); \
	cd .. && ls --color $$archive 2>/dev/null || ls ../$$archive 2>/dev/null || true
