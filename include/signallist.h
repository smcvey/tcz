/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| SIGNALLIST.H  -  List of signal names and descriptions (Similar to          |
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
|                        https://github.com/smcvey/tcz                        |
`-----------------------------------------------------------------------------'

*/

#ifndef __SIGNALLIST_H
#define __SIGNALLIST_H

/* ---->  Signal data structure  <---- */
struct signal_data {
       const char *name;  /*  Short abbreviated name of signal  */
       const char *desc;  /*  Description of signal             */
};

extern struct signal_data signallist[];

/* ---->  Signal name/description macros  <---- */
#define SignalName(signal) (((signal > 0) && (signal <= 30)) ? signallist[signal].name:"Unknown")
#define SignalDesc(signal) (((signal > 0) && (signal <= 30)) ? signallist[signal].desc:"Unknown signal")

#endif  /*  __SIGNALLIST_H  */
