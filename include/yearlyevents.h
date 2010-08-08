/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| YEARLYEVENTS.H  -  Table of yearly events.                                  |
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
| Module originally designed and written by:  J.P.Boggis 03/06/2000.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: yearlyevents.h,v 1.4 2004/12/07 19:38:07 jpboggis Exp $

*/


/* ---->  Yearly events  <---- */
/*  NT = Notify connected users when event occurs  */
/*  TD = Take user time difference into account    */
/*            Name:                        Day Mth Hr  Min Date Format   NT TD  Banner:  */
struct yearly_event_data yearly_events[] = {
       {NULL, "New Year",                  01, 01, 00, 00, LONGDATEFMT,  1, 1,  "%r%lHappy New Year!  Welcome to the year %y%l%{@?wordno 4 {@?realtime = date}}%r%l!"},
       {NULL, "Record Peak of 122 Users",  05, 03, 00, 00, LONGDATEFMT,  0, 0,  "%r%lThe original TCZ MUD reached a record peak of %w%l122 users simultaneously%r%l on %y%l5th March 1996%r%l."},
       {NULL, "TCZ 2000 Project",          11, 04, 00, 00, LONGDATEFMT,  1, 0,  "%r%lThe %w%lTCZ 2000 Project%r%l celebrates its anniversary!  -  The TCZ 2000 Project commenced on %y%lSunday 11th April 1999%r%l to ensure the future of the TCZ source code.  It celebrates its %w%l%{@?rank {@eval 0:(real ({@?time} - {@?datetime 11/04/1999}) / 31557600)}}%x birthday."},
       {NULL, "TCZ Research Project",      23, 05, 00, 00, FULLDATEFMT,  1, 0,  "%r%lAfter almost one year of downtime, the original TCZ MUD database was re-opened on %y%lWednesday 23rd May 2001%r%l as part of an official research project of the %w%lInformation, Media and Communication (IMC) Research Group%r%l in the %w%lDepartment of Computer Science, Queen Mary, University of London%r%l."},
       {NULL, "TCZ Closed Down",           30, 05, 00, 00, FULLDATEFMT,  0, 0,  "%r%lOn %y%lWednesday 30th May 2000%r%l, the original TCZ MUD was finally closed after over six years of running.  It was re-opened again on %y%lWednesday 23rd May 2001%r%l as an official research project of the %w%lInformation, Media and Communication (IMC) Research Group%r%l in the %w%lDepartment of Computer Science, Queen Mary, University of London%r%l."},
       {NULL, "One Million Connections",   22, 06, 00, 00, LONGDATEFMT,  0, 0,  "%r%lThe original TCZ MUD reached a total of %w%lone million%r%l (Non-unique) connections on %y%lSunday 22nd June 1997%r%l."},
       {NULL, "Independence Day",          04, 07, 00, 00, LONGDATEFMT,  1,  1,  "%r%lHappy Independence Day to all our American users %l - %l Enjoy the fireworks!"},
       {NULL, "Halloween",                 31, 10, 00, 00, LONGDATEFMT,  1,  1,  "%r%lHappy Halloween!  Watch out for those spooks, ghosts and ghouls!"},
       {NULL, "Bonfire Night",             05, 11, 00, 00, LONGDATEFMT,  1,  1,  "%r%lRemember, remember the %y%l5th of November%x!  Have a great bonfire night, and enjoy the fireworks!"},
       {NULL, "Peak of 100 Users",         28, 11, 17, 26, FULLDATEFMT,  0, 0,  "%r%lThe original TCZ MUD reached a peak of %w%l100 users simultaneously%r%l for the first time on %y%l28th November 1995%r%l at %y%l5:26pm (GMT)%r%l."},
       {NULL, "TCZ's Birthday",            28, 11, 18, 00, FULLDATEFMT,  1, 0,  "%r%lHappy Birthday %y%lThe Chatting Zone%r%l!  -  The original TCZ MUD was first opened to the public on %y%lMonday 28th November 1994%r%l at %y%l6pm (GMT)%r%l.  Today it celebrates its %y%l%{@?rank {@eval {@?wordno 4 {@?realtime = date}} - 1994}}%r%l birthday."},
       {NULL, "TCZ Public GPL Release",    02, 12, 17, 39, FULLDATEFMT,  1, 0,  "%r%lThe %y%lTCZ source code%r%l was released publicly under the %w%lGNU General Public License%r%l (%w%l%(GPL%)%r%l) on %y%l02/12/2004%r%l (See '%w%l%<gpl%>%r%l' and visit %y%l%u%{@?link \"\" \"http://www.sourceforge.net/projects/tcz\" \"Click to visit the TCZ project web site...\"}%r%l"},
       {NULL, "TCZ Development Commenced", 21, 12, 00, 00, LONGDATEFMT,  1, 0,  "%r%lHappy Birthday %w%lTCZ%r%l source code!  -  Development of the TCZ source code first commenced on %y%lTuesday 21st December 1993%r%l by %m%lJ.P.Boggis%r%l.  It celebrates its %w%l%{@?rank {@eval {@?wordno 4 {@?realtime = date}} - 1993}}%x birthday."},
       {NULL, "Christmas",                 25, 12, 00, 00, LONGDATEFMT,  1, 1,  "%r%lMerry Christmas and a Happy New Year!"},
       {NULL, NULL,                        00, 00, 00, 00, FULLDATEFMT,  0, 0,  NULL},
};
