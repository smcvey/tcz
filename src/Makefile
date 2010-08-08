# .---------------------------------------------------------------------------.
# | The Chatting Zone (TCZ)                              J.P.Boggis 1993-2004 |
# | ~~~~~~~~~~~~~~~~~~~~~~~                              ~~~~~~~~~~~~~~~~~~~~ |
# |--------------------------[ Module Description ]---------------------------|
# | MAKEFILE:  Before attempting to compile TCZ for the first time, please    |
# |            edit include/config.h to suit your operating system and        |
# |            requirements.                                                  |
# |-------------------------[ Copyright Information ]-------------------------|
# | This program is free software; you can redistribute it and/or modify      |
# | it under the terms of the GNU General Public License as published by      |
# | the Free Software Foundation; either version 2 of the License, or         |
# | (at your option) any later version.                                       |
# |                                                                           |
# | This program is distributed in the hope that it will be useful,           |
# | but WITHOUT ANY WARRANTY; without even the implied warranty of            |
# | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             |
# | GNU General Public License for more details.                              |
# |                                                                           |
# | You should have received a copy of the GNU General Public License         |
# | along with this program; if not, write to the Free Software               |
# | Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA |
# |----------------------[ Credits & Acknowledgements ]-----------------------|
# | For full details of authors and contributers to TCZ, please see the files |
# | MODULES and CONTRIBUTERS.  For copyright and license information, please  |
# | see LICENSE and COPYRIGHT.                                                |
# |                                                                           |
# | Makefile originally written by:  Simon A. Boggis 14/07/2000.              |
# |------------------------[ The Chatting Zone (TCZ) ]------------------------|
# |               For more information about TCZ, please visit:               |
# |                  http://www.sourceforge.net/projects/tcz                  |
# `---------------------------------------------------------------------------'
#
#   $Id: Makefile,v 1.1.1.1 2004/12/02 17:43:09 jpboggis Exp $
#

# Export all our variables
export

# The shell we expect to use to process commands
SHELL = /bin/sh

# Blank, then set recognised suffixes to avoid confusion
.SUFFIXES:
.SUFFIXES: .c .o

###############################################################################
# Programs to use							      #
###############################################################################

# C compiler
CC = gcc

# Install program
INSTALL = install
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL)

###############################################################################
# Directories								      #
###############################################################################

# Top level dir for install of files
PREFIX = /usr/local

# Top level dir for install of executable files
EXEC_PREFIX = $(PREFIX)

# Subdirectory containing executable files
BINDIR = $(EXEC_PREFIX)/bin

# Subdirectory containing system configuration files
SYSCONFDIR = $(PREFIX)/etc

# Source code directory
SRCDIR = ../src

# Include directory
INCLDIR = ../include

###############################################################################
# Names of files							      #
###############################################################################

# Name of the executable
BIN = ../bin/tcz

# Example configuration file -  to help users create their own
# No config file for now
#EXAMPLECONF = EXAMPLE.default.conf

# Settings for installation of executables
INSTALL_USR = root
INSTALL_GRP = root
INSTALL_MODE = 0755

# Settings for installation of data files (e.g. config files)
INSTALL_DATA_USR = $(INSTALL_USR)
INSTALL_DATA_GRP = $(INSTALL_GRP)
INSTALL_DATA_MODE = 0644

# Extra flags to make: mostly debugging stuff
#
# Can be specified on command line (this overrides definitions in this file) -
#
# make FOO=1 BAR=1 

# Use a malloc debugger?
ELECTRICFENCE = 0

###############################################################################
# Compiler flags							      #
###############################################################################

# Compiler options:
#
# Possible flags for the above (NOT an exhaustive list!):
# -0		optimise code (this will probably mess up gdb output)
# -m386		produce 386 optimised code (will run on others fine)
# -m486		produce 486 optimised code 
# -Wall		All Warnings
# -DLINUX 	We are using Linux
#
# You can mess around with these, they shouldn't stop sucessful compilation

CFLAGS = -ggdb -Wall# -O

# Other Important flags:
#
# Flags in ALL_CFLAGS are needed, and WILL affect/prevent sucessful compilation 
# and running - be careful with them
#
# -DANSI	We are (hopefully!) using ANSI C pass options which have
#		variable values like this:
#
# -DOPTION=\"$(SOME_VALUE)\"

ALL_CFLAGS =

# Optional debugging macro definitions:
#
# These allow you to switch various debugging / testing features on and off
#

DEFS = 

###############################################################################
# Where to look for include files					      #
###############################################################################

INCLS = -I. -I$(SRCDIR) -I$(INCLDIR)

###############################################################################
# Put the flags together now						      #
###############################################################################

ALL_CFLAGS += $(DEFS) $(INCLS) $(CFLAGS) 

###############################################################################
# Libraries								      #
###############################################################################

# Normal/local libraries we need to use
#
# LIBS = -lm                                     # CygWin32
# LIBS = -lm                                     # Linux (Pre-glibc)
# LIBS = -lm lcrypt			         # Linux (glibc)
# LIBS = -L/usr/ucblib -lsocket -lnsl -lucb -lm  # Solaris

LIBS = -lm -lcrypt

# Add a malloc debugger, if asked for
ifeq ($(ELECTRICFENCE),1)
    LIBS += -lefence
endif

###############################################################################
# Put libraries together now						      #
###############################################################################

ALL_LIBS = $(LIBS)

###############################################################################
# Install program flags							      #
###############################################################################

# Settings for installation of executables
INSTALL_PROGRAM_FLAGS = -g $(INSTALL_GRP) -m $(INSTALL_MODE) -o $(INSTALL_USR)

# Settings for installation of data files (e.g. config files)
INSTALL_DATA_FLAGS =  -g $(INSTALL_DATA_GRP) -m $(INSTALL_DATA_MODE) \
			 -o $(INSTALL_DATA_USR)

###############################################################################
# DON'T EDIT BELOW THIS LINE UNLESS YOU KNOW EXACTLY WHAT YOU ARE DOING !     #
###############################################################################

# Master source file list
SRC = \
        admin.c \
	alias.c \
        array.c \
        bbs.c \
        banish.c \
	boolexp.c \
	calculate.c \
        channels.c \
	character.c \
	combat.c \
	command.c \
	communication.c \
        compression.c \
	container.c \
	create.c \
	db.c \
	destroy.c \
        edit.c \
	event.c \
        finance.c \
	fields.c \
	flag_map.c \
	friend_flags.c \
        friends.c \
	global.c \
        group.c \
        help.c \
        html.c \
	interface.c \
        lists.c \
	logfiles.c \
	look.c \
        mail.c \
        map.c \
	match.c \
	match_array.c \
	modules.c \
	move.c \
	objectlists.c \
	options.c \
	output.c \
        pager.c \
        pagetell.c \
	predicates.c \
	preferences.c \
	primary_flags.c \
	prompt.c \
        qmwlogsocket.c \
        query.c \
	quota.c \
        request.c \
        sanity.c \
        search.c \
        search_arrays.c \
	secondary_flags.c \
        selection.c \
        server.c \
	serverinfo.c \
	signallist.c \
        sites.c \
	set.c \
	statistics.c \
	stringutils.c \
	substitute.c \
        tcz.c \
	temp.c \
        termcap.c \
	unparse.c \
        userlist.c \
        yearlyevents.c

# Generate the object list from the source list
OBJS = $(SRC:.c=.o)

# If we didn't say we wanted to make anything, default to 'all'
all:
	@$(MAKE) -f Makefile.rules $@

# Tell make that it'll find all other rules in another makefile
%:
	@$(MAKE) -f Makefile.rules $@

###############################################################################
# END OF MAKEFILE 							      #
###############################################################################
