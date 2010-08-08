/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| LOGFILES_TABLE.H  -  Table of system log files, accessible using the '@log' |
|                      comand on TCZ.                                         |
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
| Module originally designed and written by:  J.P.Boggis 11/03/2000.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: logfile_table.h,v 1.1.1.1 2004/12/02 17:43:25 jpboggis Exp $

*/


/* ---->  Log files (Filename, Log name, Maximum size (If 0, log file is disabled), Size to truncate to (If maximum size exceeded), Keep open (1 = Always open, 0 = Close when not in use), Access level (As for privilege()) and active file descriptor (Must be NULL in initialisation))  <---- */
struct log_data logfs[] = {

/*      Path  */
/*      |                        Log File Name                                  */
/*      |                        |                Maximum Size (0 = Disable)    */
/*      |                        |                |      Reset Size             */
/*      |                        |                |      |     Always keep logfile open  */
/*      |                        |                |      |     |                */
/*      |                        |                |      |     |  Access Level  */
/*      |                        |                |      |     |  |             */
       {"log/admin.log",         "Admin",         5000,  1000, 0, 4, NULL},
       {"log/assist.log",        "Assist",        2000,  1000, 0, 4, NULL},
       {"log/ban.log",           "Ban",           2000,  1000, 0, 4, NULL},
       {"log/bbs.log",           "BBS",           2000,  1000, 0, 4, NULL},
       {"log/boot.log",          "Boot",          2000,  1000, 0, 4, NULL},
       {"log/bugs.log",          "Bugs",          2000,  1000, 0, 4, NULL},
       {"log/combat.log",        "Combat",        2000,  1000, 0, 4, NULL},
       {"log/command.log",       "Command",       25000, 1000, 1, 0, NULL},
       {"log/comments.log",      "Comments",      2000,  1000, 0, 4, NULL},
       {"log/complaints.log",    "Complaints",    2000,  1000, 0, 4, NULL},
       {"log/connect.log",       "Connect",       10000, 1000, 1, 4, NULL},
       {"log/create.log",        "Create",        2000,  1000, 0, 4, NULL},
       {"log/destroy.log",       "Destroy",       5000,  1000, 0, 4, NULL},
       {"log/dump.log",		 "Dump",	  2000,  1000, 0, 4, NULL},
       {"log/duplicate.log",     "Duplicate",     2000,  1000, 0, 4, NULL},
       {"log/email.log",         "Email",         2000,  1000, 0, 4, NULL},
       {"log/emergency.log",     "Emergency",     5000,  1000, 0, 4, NULL},
       {"log/execution.log",     "Execution",     5000,  1000, 0, 4, NULL},
       {"log/flags.log",         "Flags",         2000,  1000, 0, 4, NULL},
       {"log/force.log",         "Force",         2000,  1000, 0, 4, NULL},
       {"log/hack.log",          "Hack",          5000,  1000, 0, 4, NULL},
       {"log/html.log",          "HTML",          5000,  1000, 1, 4, NULL},
       {"log/logentry.log",      "LogEntry",      2000,  1000, 0, 4, NULL},
       {"log/maintenance.log",   "Maintenance",   2000,  1000, 0, 4, NULL},
       {"log/miscellaneous.log", "Miscellaneous", 2000,  1000, 0, 4, NULL},
       {"log/monitor.log",       "Monitor",       5000,  1000, 1, 4, NULL},
       {"log/name.log",          "Name",          2000,  1000, 0, 4, NULL},
       {"log/options.log",       "Options",       2000,  1000, 0, 4, NULL},
       {"log/owner.log",         "Owner",         2000,  1000, 0, 4, NULL},
       {"log/password.log",      "Password",      2000,  1000, 0, 4, NULL},
       {"log/request.log",       "Request",       5000,  1000, 0, 4, NULL},
       {"log/research.log",      "Research",      5000,  1000, 0, 0, NULL},
       {"log/restart.log",       "Restart",       2000,  1000, 0, 4, NULL},
       {"log/sanity.log",        "Sanity",        5000,  1000, 0, 4, NULL},
       {"log/server.log",        "Server",        5000,  1000, 0, 4, NULL},
       {"log/shout.log",         "Shout",         2000,  1000, 0, 4, NULL},
       {"log/site.log",          "Site",          2000,  1000, 0, 4, NULL},
       {"log/stats.log",         "Stats",         2000,  1000, 0, 4, NULL},
       {"log/suggestions.log",   "Suggestions",   2000,  1000, 0, 4, NULL},
       {"log/summon.log",        "Summon",        2000,  1000, 0, 4, NULL},
       {"log/termcap.log",       "Termcap",       1000,  500,  0, 4, NULL},
       {"log/transaction.log",   "Transaction",   2000,  1000, 0, 4, NULL},
       {"log/warn.log",          "Warn",          2000,  1000, 0, 4, NULL},
       {"log/welcome.log",       "Welcome",       2000,  1000, 0, 4, NULL},
};
