/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| LOGFILES.H  -  Constants for TCZ system log files.                          |
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
| Module originally designed and written by:  J.P.Boggis 03/06/1999.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: logfiles.h,v 1.1.1.1 2004/12/02 17:43:25 jpboggis Exp $

*/


#define USER_LOG_FILE_SIZE   1000                      /*  Maximum size of user log files  */
#define USER_LOG_FILE_RESET  (USER_LOG_FILE_SIZE / 2)  /*  Size to truncate to (If maximum size is exceeded)  */

#define UserLog(x)           (((x) > 0) ? (0 - (x)):(1 - INFINITY))


/* ---->  Log files (Log file index numbers must be consecutive,     <---- */
/*                  starting from 0, and must match entries in table       */
/*                  defined in include/logfile_table.h)                    */

#define LOG_COUNT       44  /*  Last log file index number + 1  */

#define ADMIN_LOG       0
#define ASSIST_LOG      1
#define BAN_LOG         2
#define BBS_LOG         3
#define BOOT_LOG        4
#define BUG_LOG         5
#define COMBAT_LOG      6
#define COMMAND_LOG     7
#define COMMENT_LOG     8
#define COMPLAINT_LOG   9
#define CONNECT_LOG     10
#define CREATE_LOG      11
#define DESTROY_LOG     12
#define DUMP_LOG	13
#define DUPLICATE_LOG   14
#define EMAIL_LOG       15
#define EMERGENCY_LOG   16
#define EXECUTION_LOG   17
#define FLAGS_LOG       18
#define FORCE_LOG       19
#define HACK_LOG        20
#define HTML_LOG        21
#define LOGENTRY_LOG    22
#define MAINTENANCE_LOG 23
#define MISC_LOG        24
#define MONITOR_LOG     25
#define NAME_LOG        26
#define OPTIONS_LOG     27
#define OWNER_LOG       28
#define PASSWORD_LOG    29
#define REQUEST_LOG     30
#define RESEARCH_LOG	31
#define RESTART_LOG     32
#define SANITY_LOG      33
#define SERVER_LOG      34
#define SHOUT_LOG       35
#define SITE_LOG        36
#define STATS_LOG       37
#define SUGGESTION_LOG  38
#define SUMMON_LOG      39
#define TERMCAP_LOG     40
#define TRANSACTION_LOG 41
#define WARN_LOG        42
#define WELCOME_LOG     43
