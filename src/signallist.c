/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| SIGNALLIST.C  -  List of signal names and descriptions (Similar to          |
|                  strsignal() / sys_siglist[] functionality found in         |
|                  some C compiler implementations (Such as GNU/Linux.)       |
|--------------------------[ Copyright Information ]--------------------------|
| This program is free software; you can redistribute it and/or modify        |
| it under the terms of the GNU General Public License as published by        |
| the Free Software Foundation; either version 2 of the License, or           |
| (at your option) any later version.                                         |
|                                                                             |
| This program is distributed in the hope that it will be useful,             |
| but WITHOUT ANY WARRANTY; without even the implied warranty of              |
| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               |
| GNU General Public License for more details.                                |
|                                                                             |
| You should have received a copy of the GNU General Public License           |
| along with this program; if not, write to the Free Software                 |
| Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA   |
|-----------------------[ Credits & Acknowledgements ]------------------------|
| For full details of authors and contributers to TCZ, please see the files   |
| MODULES and CONTRIBUTERS.  For copyright and license information, please    |
| see LICENSE and COPYRIGHT.                                                  |
|                                                                             |
| Module originally designed and written by:  J.P.Boggis 26/08/2000.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

*/

#include <stdlib.h>

#include "signallist.h"

/* ---->  List of signals  <---- */
struct signal_data signallist[] =
{
       {"Unknown",   "Unknown signal"},
       {"SIGHUP",    "Hangup"},
       {"SIGINT",    "Interrupt"},
       {"SIGQUIT",   "Quit"},
       {"SIGILL",    "Illegal instruction"},
       {"SIGTRAP",   "Trace/breakpoint trap"},
       {"SIGABRT",   "Aborted"},
       {"SIGBUS",    "Bus error"},
       {"SIGFPE",    "Arithmetic exception"},
       {"SIGKILL",   "Killed"},
       {"SIGUSR1",   "User defined signal #1"},
       {"SIGSEGV",   "Segmentation fault"},
       {"SIGUSR2",   "User defined signal #2"},
       {"SIGPIPE",   "Broken pipe"},
       {"SIGALRM",   "Alarm clock"},
       {"SIGTERM",   "Terminated"},
       {"Unknown",   "Unknown signal"},
       {"SIGCHLD",   "Child status changed"},
       {"SIGCONT",   "Continued"},
       {"SIGSTOP",   "Stopped (Signal)"},
       {"SIGTSTP",   "Stopped (User)"},
       {"SIGTTIN",   "Stopped (TTY input)"},
       {"SIGTTOU",   "Stopped (TTY output)"},
       {"SIGURG",    "Urgent I/O condition"},
       {"SIGXCPU",   "CPU time limit exceeded"},
       {"SIGXFSZ",   "File size limit exceeded"},
       {"SIGVTALRM", "Virtual timer expired"},
       {"SIGPROF",   "Profiling timer expired"},
       {"SIGWINCH",  "Window size changed"},
       {"SIGIO",     "I/O possible"},
       {"SIGPWR",    "Power fail/restart"},
       {NULL,        NULL}
};
