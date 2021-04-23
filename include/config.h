/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| CONFIG.H  -  TCZ configuration and parameters.  Check these options in      |
|              detail before attempting to compile the source code for the    |
|              first time, to ensure they are correct for your system and     |
|              preferences.                                                   |
|                                                                             |
|              NOTE:  Quite a lot of server parameters can be set at run-time |
|                     without re-compiling the source code (Either using a    |
|                     configuration file and/or command-line options.)        |
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
| Module originally modified for TCZ by:  J.P.Boggis 21/12/1993.              |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                        https://github.com/smcvey/tcz                        |
`-----------------------------------------------------------------------------'

*/


#ifndef __CONFIG_H
#define __CONFIG_H


/* ---->  Code base  <---- */
#define CODEBASE  "TCZ"                             /*  Name of code base               */
#define CODEDESC  "Official"                        /*  Brief description of code base  */


/* ---->  Operating System  <---- */
#define LINUX                /*  Compile for Linux  */
#undef  CYGWIN32             /*  Compile for CygWin32 (Win95/Win98/WinNT)  */
#undef  SOLARIS              /*  Compile for Solaris (Not tested)  */
#undef  SUNOS                /*  Compile for SunOS (Not tested)  */


/* ---->  Compile options  <---- */
#undef  BETA                 /*  Enables unfinished beta code which is

                                 currently being worked on.  Only recommended
                                 for a TCZ compiled for testing purposes only.
                                 If LOCAL is defined, BETA is assumed by
                                 default  */

#undef  DEMO                 /*  Compile restricted demonstration TCZ,
                                 suitable for distribution for testing
                                 or evaluation purposes.  Only allows
                                 connections from 127.0.0.1 (Localhost.)  */


/* ---->  Configuration options  <---- */
#define AUTO_TIME_ADJUST     /*  If a significant change in time on the
                                 host server occurs (E.g:  Daylight saving),
                                 TCZ will detect this and adjust its internal
                                 times to compensate, preventing users from
                                 getting logged off for being idle, alarms
                                 prematurely executing, etc.  */

#undef  BBS_LIST_VOTES       /*  Enables Mortals to list who has voted
                                 for/against their messages, the topic
                                 owner can list the votes for/against ANY
                                 message in their topic(s) and Admin can
                                 list the votes for/against messages owned
                                 by lower level characters.  */

#define CHIMING_CLOCK        /*  Enables the hourly chiming clock feature
                                 (NOTE:  The clock message (Sent to every
                                 connected user every hour) can be modified
                                 to suit your taste by changing the
                                 CLOCK_PREFIX, CLOCK_MIDDLE and CLOCK_SUFFIX
                                 definitions appropriately.)  */

#define DATABASE_DUMP        /*  Enables the ability for TCZ to dump the
                                 database in memory to disk.  If this is not
                                 enabled, '@dump' and automatic database
                                 dumping will be disabled.  Disabled by
                                 default if DEMO is defined.  */

#define DB_COMPRESSION       /*  Enables compression of database files stored
                                 on disk (Using 'gzip' or alternatively,
                                 'compress'), saving on disk space.  This
                                 option is recommended for large databases,
                                 however it may increase the time taken to
                                 restart/shutdown TCZ.  */

#define DB_COMPRESSION_GZIP  /*  Enables use of 'gzip' instead of 'compress'
                                 when DB_COMPRESSION is defined.  'gzip'
                                 obtains better compression than 'compress'
                                 and is probably more efficient.  */
  
#define DB_FORK              /*  Enables use of fork() to spawn separate
				 dedicated dumping process rather than
                                 dumping the database in the background
                                 using Zero Memory Database Dumping
                                 (Internal background database dumping.)
                                 
                                 This can be demanding on memory if the
                                 operating system does not support
                                 copy-on-write pages with fork() (Linux
                                 supports this.)

                                 Because the dumping process is separate from
                                 the main TCZ process, it can write data to
                                 the dump file continuously, resulting in a
                                 faster dump that is free from inconsistencies
                                 (No changes take place to the dumping process
                                 DB while it is being dumped.)

                                 It is also more resilient, seeing as the
                                 child process can continue if the main TCZ
                                 process crashes.  On restart, TCZ will wait
                                 up to 10 minutes (Default) for the dumping
                                 process to complete.  */

#undef EMAIL_FORWARDING     /*  When enabled, TCZ will write forwarding
                                 E-mail addresses for all characters on TCZ
                                 who have set their E-mail address.  This can
                                 then be placed in '/etc/aliases', allowing
                                 users of TCZ to be sent E-mail using
                                 'tcz.user.name@tczserver.domain', e.g:
                                 'jc.digita@thechattingzone.uk'.  */

#define EXPERIENCED_HELP     /*  When, Assistants and Experienced Builders
                                 will be allowed to use the 'welcome' and
                                 'assist' commands to welcome and assist
                                 users when insufficient Admin are connected
                                 or respond too slowly.  */

#define EXTRA_COMPRESSION    /*  When enabled, a more advanced compression
                                 routine will be used on the database to
                                 reduce system memory usage when TCZ is
                                 running.  This compression routine can
                                 gain typical compression rates of around
                                 70%, whereas the more simple compression
                                 routine (Which will be used if this option
                                 is not defined) gains a flat 12.5%  */

#define GUARDIAN_ALARM	     /*  When enabled, TCZ will automatically
				 shutdown if command processing takes
                                 longer than GUARDIAN_ALARM_TIME.  This
                                 safe-guards against infinite loops that
                                 can cause TCZ to 'hang' indefinitely
                                 due to bugs in new code.  */

#define HOME_ROOMS           /*  When enabled, users will automatically be
                                 given their own home room when they create
                                 their character.  */

#define KEEP_POSSESSIONS     /*  When defined, possessions (Objects which
                                 are being carrying) will not be sent to
                                 their respective homes when the user carrying
                                 them goes home (Using the 'home' command.)  */

#undef  MORTAL_SHOUT         /*  When defined, Mortals will be allowed to use
                                 the 'shout' command (By default Admin only),
                                 providing their total time connected is
                                 greater than MORTAL_SHOUT_CONSTRAINT hours.
                                 Also, Druids/Apprentice Wizards may use
                                 'shout ;<MESSAGE>' to do broadcast messages
                                 (Usually Druid/Wizard and above only.)  */

#define NOTIFY_WELCOME       /*  When defined, connected Admin will be
                                 automatically asked to welcome new users
                                 (Using the 'welcome' command.)  */

#undef  QUIET_WHISPERS       /*  When defined, the 'whispers' command will be
                                 "quiet", so other users in the same room
                                 don't see '<NAME> whispers something to
                                 <NAME>.'  */

#undef  REFRESH_SOCKETS      /*  When defined, sockets for incoming Telnet
                                 connections will be closed and re-opened
                                 if no new connections are received
                                 for REFRESH_SOCKET_INTERVAL minutes.

                                 NOTE:  This appears to conflict with the pipes
                                        used for dumping compressed databases,
                                        so should be disabled for now if
                                        DB_COMPRESSION is defined.

                                        The cause is unknown at present.  */

#define RESTRICT_MEMORY      /*  When defined, maximum memory usage by TCZ
                                 will be restricted.  The stack will be
                                 restricted to LIMIT_STACK and the data/
                                 resident set size will be limited by
                                 LIMIT_MEMORY_PERCENTAGE, LIMIT_MEMORY_LOWER
                                 and LIMIT_MEMORY_UPPER.

                                 RESTRICT_MEMORY is used to guard against
                                 infinitely recursive function calls and
                                 excessive database expansion.  */

#define SERVERINFO	     /*  When defined, code for automagically
				 figuring out various network addresses and
				 reading your kernel network configuration
				 will be incorporated, so that you no-longer
				 have to specify these at start up.  */

#define PAGE_DISCLAIMER      /*  When defined, the disclaimer will be paged
                                 using a simple 'more' pager, allowing a long
                                 disclaimer (More than 1x 80x24 screen) to be
                                 read, regardless of Telnet client.  */

#undef  PAGE_TITLESCREENS    /*  When defined, the title screens (Shown when
                                 logging in via Telnet) will be paged using
                                 a simple 'more' pager, allowing long title
                                 screens (More than 1x 80x24 screen) to be
                                 read, regardless of Telnet client.  */

#define SOCKETS              /*  When defined, code for IP sockets will be
				 incorporated, allowing multiple connections
				 via Telnet.  */

#define TCZ_EXEC_BACKUP	     /*  When defined, the TCZ executable will be
				 copied to 'bin/tcz.<TELNET PORT>',
                                 allowing debug of core dump if the
                                 executable changes due to code
				 recompile, etc.  If this file exists
                                 prior to making copy, it will be renamed
                                 to 'bin/tcz.<TELNET PORT>.core'
                                 (This executable should be used when
				 debugging the core dump, if TCZ is setup
			         to automatically restart.)  */

#define UPS_SUPPORT          /*  When defined, TCZ will automatically warn
                                 users and start dumping the database (If
                                 a dump is not already in progress) when
                                 the signal SIGPWR is received.  If the
                                 power is resumed and a further SIGPWR
                                 signal is received, the users will be
                                 informed of this.  */                                   

#define USER_LOG_FILES       /*  When defined, each Builder and above will
                                 have their own personal log file, into
                                 which they can log entries using '@logentry',
                                 plus failed commands typed by users in their
                                 rooms/areas and connections/disconnections
                                 of their character will be logged to their
                                 personal log file.  */

#define USE_PROC             /*  When defined, the /proc filesystem will be
                                 used to gather resource usage information
                                 when '@stats resource' is used.  This gives
                                 more detailed and meaningful information than
                                 the information returned by getrusage(), which
                                 will be used alternatively if USE_PROC is not
                                 defined.  If you have the /proc filesystem,
                                 you should use it rather than getrusage()  -
                                 On some systems (Such as Linux), the
                                 information returned by getrusage() is
                                 not fully implemented.  */

#define WARN_DUPLICATES      /*  When defined, the E-mail addresses of new
                                 characters will be checked against the E-mail
                                 addresses of all other characters in the
                                 database, warning connected Admin and logging
                                 to 'Admin' log file if more than one character
                                 with E-mail address (Or similar E-mail
                                 address) exists.  */


/* ---->  Version, compilation date and server/site details  <---- */
#define TCZ_ADMIN_EMAIL       "admin@tcz.mud.host"             /*  Default admin. E-mail address (Can be overriden in config file.)  */
#define TCZ_SHORT_NAME        "TCZ"                            /*  Default TCZ server name (Short version)  */
#define TCZ_FULL_NAME         "New TCZ-based MUD"              /*  Default TCZ server name  */
#define TCZ_LOCATION          "Not Specified"                  /*  Default location of TCZ server  */
#define TCZ_REVISION          1                                /*  Current minor revision number of TCZ server  */
#define TCZ_VERSION           "4.4"                            /*  TCZ server version number  */
#define HTML_HOME_URL         "https://github.com/smcvey/tcz"  /*  URL of TCZ home pages  */


/* ---->  Networking  <---- */
#define IPADDR(ip1,ip2,ip3,ip4)      ((ip1 << 24) | (ip2 << 16) | (ip3 << 8) | ip4)
#define MAX_PENDING_NEW_CONNECTIONS  64                 /*  Maximum number of pending new connections allowed on each socket  */
#define REFRESH_SOCKET_INTERVAL      10                 /*  Refresh (In minutes) of sockets for incoming new connections (If no new connections after this time, socket will be closed and re-opened)  */
#define GUEST_LOGIN_ADDRESS          IPADDR(127,0,0,1)  /*  Guest Telnet login service address (127.0.0.1, local loop-back of server machine)  */
#define TELNETPORT                   8342               /*  Default server port number for direct Telnet connections (8342)  */


/* ---->  Internet address and port number of machine on which TCZ server is running  <---- */
#define TCZ_SERVER_NAME       "localhost"               /*  TCZ server DNS name         */
#define TCZ_SERVER_IP         IPADDR(127,0,0,1)         /*  TCZ server IP address       */
#define TCZ_SERVER_NETMASK    IPADDR(255,0,0,0)         /*  TCZ server network mask     */
#define TCZ_SERVER_NETWORK    IPADDR(127,0,0,0)         /*  TCZ server network address  */


/* ---->  Default welcome (If no 'motd.tcz' file) and leave messages, etc.  <---- */
#define EMERGENCY_SHUTDOWN    "\n\n"ANSI_LGREEN"%s fades...\n\n"ANSI_LRED \
                              ".----------------------------------------------------------------------------.\n" \
                              "|                             `                                              |\n" \
                              "|                        ----====================----                        |\n" \
                              "|    [OH NO!]    ***=--->   "ANSI_WYELLOW"EMERGENCY  SHUTDOWN!!!"ANSI_LRED"   <---=***    [OH NO!]    |\n" \
                              "|                        ----====================----                        |\n" \
                              "|                                                                            |\n" \
                              "`----------------------------------------------------------------------------'\n\n"ANSI_LWHITE

#define BIRTHDAY_MESSAGE      ANSI_DCYAN"\n" \
                              "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n\n" \
                              ANSI_LWHITE"              .--.  .--..--------..--------..--------..--.  .--.\n" \
                              ANSI_DWHITE"              |  |  |  ||  .--.  ||  .--.  ||  .--.  ||  |  |  |\n" \
                              ANSI_LGREEN"  .--------.  "ANSI_LCYAN"|  `--'  ||  `--'  ||  `--'  ||  `--'  ||  `--'  |  "ANSI_LGREEN".--------.\n" \
                              ANSI_DGREEN"  `--------'  "ANSI_DCYAN"|  .--.  ||  .--.  ||  .-----'|  .-----'`--.  .--'  "ANSI_DGREEN"`--------'\n" \
                               ANSI_LBLUE"              |  |  |  ||  |  |  ||  |      |  |         |  |\n" \
                               ANSI_DBLUE"              `--'  `--'`--'  `--'`--'      `--'         `--'\n" \
                              ANSI_LWHITE"  .-------. .--..-------. .--------..--.  .--..--------..--------..--.  .--.\n" \
                              ANSI_DWHITE"  |  .-.  | |  ||  .-.  | `--.  .--'|  |  |  |`.  .-.  ||  .--.  ||  |  |  |\n" \
                               ANSI_LCYAN"  |  `-'  `.|  ||  `-'  `.   |  |   |  `--'  | |  | |  ||  `--'  ||  `--'  |\n" \
                               ANSI_DCYAN"  |  .--.  ||  ||  .--.  |   |  |   |  .--.  | |  | |  ||  .--.  |`--.  .--'\n" \
                               ANSI_LBLUE"  |  `--'  ||  ||  |  |  |   |  |   |  |  |  |.'  `-'  ||  |  |  |   |  |\n" \
                               ANSI_DBLUE"  `--------'`--'`--'  `--'   `--'   `--'  `--'`--------'`--'  `--'   `--'\n" \
                              ANSI_DCYAN"\n-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n\n"

#define SYSTEM_SHUTDOWN       "\n\n"ANSI_LGREEN"%s fades...\n\n"ANSI_LRED \
                              ".----------------------------------------------------------------------------.\n" \
                              "|                                                                            |\n" \
                              "|                        ----====================----                        |\n" \
                              "|                  ***=--->   "ANSI_WYELLOW"SYSTEM SHUTDOWN!!!"ANSI_LRED"   <---=***                  |\n" \
                              "|                        ----====================----                        |\n" \
                              "|                                                                            |\n" \
                              "`----------------------------------------------------------------------------'\n\n"ANSI_LWHITE

#define WELCOME_MESSAGE       "\n-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n %s (TCZ v"TCZ_VERSION")  -  (C)Copyright J.P.Boggis 1993 - %d.\n-------------------------------------------------------------------------------\n" \
                              " TCZ is free software, which is distributed under version 2 of the GNU General\n" \
                              " Public License (See 'help gpl' in TCZ, or visit http://www.gnu.org)  For more\n" \
                              " information about the TCZ, please visit:\n\n" \
			      "                    https://github.com/smcvey/tcz\n" \
			      "-------------------------------------------------------------------------------\n" \
                              " Type:  INFO  -  For more information about TCZ.\n" \
                              "        NEW   -  To create a new character.\n" \
                              "        WHO   -  To find out who is currently connected.\n" \
                              "        QUIT  -  To disconnect (In capital letters only.)\n" \
                              "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n\n"

#define OUTPUT_FLUSHED        ANSI_DCYAN"<OUTPUT FLUSHED>\n"ANSI_DWHITE
#define LEAVE_MESSAGE         ANSI_LGREEN"\nThank you for visiting "ANSI_LYELLOW"%s"ANSI_LGREEN"  -  See you again soon!\n\n"
#define CLOCK_PREFIX          ANSI_LGREEN"\007You hear the town clock strike "
#define CLOCK_MIDDLE          ", it's "
#define CLOCK_SUFFIX          " o'clock."
#define AUTO_AFK              "I have been idle for more than %d minute%s and have been automatically sent AFK by %s."

#ifdef KEEP_POSSESSIONS
   #define HOME_MESSAGE       "\n"ANSI_LMAGENTA"You shoot up into the sky at a terrific rate...\n"ANSI_LYELLOW"AAAAAAAAAaaaaaaaaaaaaiiiiiiiiiiieeeeeeeee!!!\n"ANSI_LGREEN"Bleeeeeeeeeeeeeeeeeeeeeeeeeeeeuuuuuugh!\n\n"ANSI_LCYAN"Next time I think I'll walk!\n"ANSI_LWHITE"You hit the floor of your home with a loud thump!\n"
#else
   #define HOME_MESSAGE       "\n"ANSI_LMAGENTA"You shoot up into the sky at a terrific rate...\n"ANSI_LYELLOW"AAAAAAAAAaaaaaaaaaaaaiiiiiiiiiiieeeeeeeee!!!\n"ANSI_LGREEN"Bleeeeeeeeeeeeeeeeeeeeeeeeeeeeuuuuuugh!\n\n"ANSI_LCYAN"Next time I think I'll walk!\n"ANSI_LWHITE"You hit the floor of your home with a loud thump, minus your possessions!\n"
#endif


/* ---->  Standard character settings  <---- */
#define STANDARD_CHARACTER_SCRHEIGHT   24        /*  Starting screen height (24 line screen assumed)  */
#define STANDARD_CHARACTER_SCRWIDTH    80        /*  Starting screen width  (80 column screen assumed)  */
#define STANDARD_CHARACTER_STRENGTH    2500      /*  Standard strength of character (Determines weight of possessions they can carry.)  */


/* ---->  Standard object volume  <---- */
#define STANDARD_CHARACTER_VOLUME      75        /*  Starting volume of character (Litres)  */
#define STANDARD_THING_VOLUME          1         /*  Starting volume of thing (Litres)  */
#define STANDARD_ROOM_VOLUME           10000     /*  Starting volume of room (Litres)  */
#define STANDARD_VOLUME                0         /*  Starting volume for all other objects (Litres)  */


/* ---->  Standard object mass  <---- */
#define STANDARD_CHARACTER_MASS        70            /*  Starting mass of character (Kg)  */
#define STANDARD_THING_MASS            1             /*  Starting mass of thing (Kg)  */
#define STANDARD_ROOM_MASS             TCZ_INFINITY  /*  Starting mass of room (Kg)  */
#define STANDARD_MASS                  0             /*  Starting mass of all other objects (Kg)  */


/* ---->  Reserved database objects  <---- */
#define ROOMZERO                       0         /*  Used as a junk pile for junked objects and objects which were attached to a (Now) destroyed object  */
#define ROOT                           1         /*  'Super User' (Supreme Being)  */
#define START_LOCATION                 3         /*  Starting location for newly created characters  */
#define GLOBAL_COMMANDS                4         /*  Global compound commands location  */


/* ---->  Friends/enemies list constraints  <---- */
#define MAX_FRIENDS_MORTAL             300       /*  Maximum friends a Mortal can have in their friends/enemies list (Each friend/enemy uses 9 bytes)  */
#define MAX_FRIENDS_ADMIN              500       /*  Maximum friends Apprentice Wizard or above can have in their friends/enemies list                 */


/* ---->  Alias constraints  <---- */
#define MAX_ALIASES_MORTAL             100       /*  Maximum unique aliases a Mortal may have  */
#define MAX_ALIASES_ADMIN              200       /*  Maximum unique aliases Apprentice Wizard or above may have (Elder Wizards and above are unrestricted (With exception of MAX_ALIASES))  */
#define MAX_ALIAS_LENGTH               64        /*  Maximum length of alias name  */
#define MAX_ALIASES                    65000     /*  Absolute maximum number of individual (Non-unique) aliases  */


/* ---->  Mail system constraints  <---- */
#define MAIL_LIMIT_GROUP_MORTAL        25        /*  Maximum number of users a Mortal can mail in one go (Group mail)  */
#define MAIL_LIMIT_MORTAL              50        /*  Standard mail limit for Mortals  */
#define MAIL_LIMIT_ADMIN               100       /*  Standard mail limit for Admin  */
#define MAIL_TIME_LIMIT                50        /*  Mail which was last read MAIL_TIME_LIMIT days ago or more will be automatically deleted  */
#define MAX_MAIL_LIMIT                 255       /*  Maximum mail limit that can be set (By Elders+)  (Anything up to 255)  */


/* ---->  BBS constraints  <---- */
#define BBS_MAX_MESSAGES_MORTAL        100       /*  Highest value allowed for topic message limit (Mortal)  */
#define BBS_MAX_MESSAGES_ADMIN         250       /*  Highest value allowed for topic message limit (Admin  -  Elders and above may set any limit up to BBS_MAX_MESSAGES)  */

#define BBS_DEFAULT_MAX_SUBTOPICS      16        /*  Default maximum number of sub-topics for a new topic  */
#define BBS_DEFAULT_MAX_MESSAGES       100       /*  Default maximum number of messages for a new topic (Up to 999)  */
#define BBS_TOPIC_TIME_LIMIT           0         /*  Default time limit (In days) for messages in new topic (0 = None)  */
#define BBS_VOTE_CONSTRAINT            1         /*  Total time (Hours) Mortal must have to vote for messages  */
#define BBS_READERS_EXPIRY             14        /*  Time (In days, since message posting date) after which readers list of message will be cleared and message set as read by everyone  */
#define BBS_CYCLIC_REGION              30.0      /*  Percentage of messages in a topic on the BBS from which old messages will be cyclicly deleted when topic is full  */
#define BBS_MAX_SUBTOPICS              100       /*  Maximum number of sub-topics allowed per topic (Deities and above can set above this limit)  */
#define BBS_MAX_MESSAGES               999       /*  Maximum number of messages ever allowed in a topic (Up to 999)  */
#define BBS_VOTE_EXPIRY                7         /*  Maximum expiry time (In days) that can be set for a vote on a message  */
#define BBS_MAX_READERS                250       /*  Maximum number of readers before reader list is cleared and message is marked as being read by everyone  */
#define BBS_MAX_LATEST                 60        /*  Maximum number of messages displayed using 'latest' BBS commannd  */
#define BBS_MAX_TOPICS                 1000      /*  Maximum number of topics allowed (Any number up to 65535)  */
#define BBS_MAX_VOTES                  250       /*  Maximum number of votes allowed per message (Any number)  */


/* ---->  Channel restrictions  <---- */
#define CHANNEL_NAME_LENGTH            20        /*  Maximum length of channel name   */
#define CHANNEL_BANNER_LENGTH          20        /*  Maximum length of channel banner (Excluding ANSI colour codes)  */
#define CHANNEL_BANNER_MAX             64        /*  Maximum length of channel banner (Including ANSI colour codes)  */
#define CHANNEL_TITLE_LENGTH           128       /*  Maximum length of channel title (Excluding ANSI colour codes)  */
#define CHANNEL_TITLE_MAX              256       /*  Maximum length of channel title (Including ANSI colour codes)  */
#define CHANNEL_MAX                    16        /*  Maximum number of temporary channels Mortal user may create  */


/* ---->  Character/mail/BBS automatic maintenance timing and parameters  <---- */
#define CHAR_MAINTENANCE_DAY_OFFSET    3         /*  Day offset from EPOCH day-of-week (Thursday) to start unused character maintenance (3 == Sunday)  */
#define OBJ_MAINTENANCE_DAY_OFFSET     2         /*  Day offset from EPOCH day-of-week (Thursday) to start unused/junked object maintenance (2 == Saturday)  */
#define CHAR_MAINTENANCE_HOUR          5         /*  Time each week to start unused character maintenance  */
#define MAIL_CHECK_DAY_OFFSET          2         /*  Day offset from EPOCH day-of-week (Thursday) to start unread mail maintenance (2 == Saturday)  */
#define OBJ_MAINTENANCE_HOUR           5         /*  Time to start unused object maintenance  */
#define REQUEST_EXPIRY_HOUR            3         /*  Time (Each day) to check new character request queue for expired requests  */
#define MAIL_CHECK_HOUR                5         /*  Time each week to start unread mail maintenance  */
#define BBS_CHECK_HOUR                 4         /*  Time (Each day) to check BBS for out-of-date messages (Time restricted topics only)  */

#define OBJ_MAINTENANCE                365       /*  Default unused object destroy time (Days)  */
#define JUNK_MAINTENANCE               30        /*  Default junk object (Objects located in #0 (ROOMZERO)) destroy time (Days)  */
#define CHAR_MAINTENANCE_MORONS        14        /*  Default unused Moron destroy time (Days)  */
#define CHAR_MAINTENANCE_NEWBIES       28        /*  Default unused newbie (Character with < 1 day total time connected) destroy time (Days)  */
#define CHAR_MAINTENANCE_MORTALS       100       /*  Default unused Mortal destroy time (Days)  */
#define CHAR_MAINTENANCE_BUILDERS      200       /*  Default unused Builder destroy time (Days)  */
#define PASSWORD_EXPIRY                90        /*  Time (In days) after which a user's password expires  */
#define REQUEST_EXPIRY                 28        /*  Time (In days) after which new character requests expire  */


/* ---->  Combat  <---- */
#define HEALTH_REPLENISH_INTERVAL      6         /*  Health replenishing interval (In seconds)  */
#define HEALTH_REPLENISH_AMOUNT        0.1       /*  Amount of health per replenish             */


/* ---->  Financial  <---- */
#define WITHDRAWAL_RESTRICTION         60        /*  Total connect time (In minutes) required before new user can make withdrawals from their bank account  */
#define DEFAULT_RESTRICTION            10        /*  Default payment restriction (User cannot pay more than this from within compound command)  */
#define PAYMENT_RESTRICTION            0         /*  Total connect time (In minutes) required before new user can make payments to other users  */
#define RAISE_RESTRICTION              100.0     /*  Total number of credits user must be below to be given credit ('@credit')  */ 
#define DEFAULT_BALANCE                500.0     /*  Default bank balance of new character  */
#define DEFAULT_CREDIT                 50.0      /*  Default credit in pocket of a new character  */
#define HOURLY_PAYMENT                 25        /*  Hourly payment (Whilst connected)  */
#define DOUBLE_PAYMENT                 8         /*  Hours of connect time after which hourly payment doubles  */
#define ADMIN_PAYMENT                  2         /*  Multiplier for hourly payments to administrators  */
#define DEFAULT_DEBIT                  20.0      /*  Default debit rate     */
#define RAISE_MAXIMUM                  1000.0    /*  Maximum number of credits (Total) user may be given ('@credit')  */
#define PAYMENT_GUARD                  10.0      /*  Default maximum payment which user can make from within a compound command  */


/* ---->  Profile constraints (Maximum lengths)  <---- */
#define PROFILE_QUALIFICATIONS         512       /*  Qualifications     */
#define PROFILE_ACHIEVEMENTS           512       /*  Achievements       */
#define PROFILE_NATIONALITY            128       /*  Nationality        */
#define PROFILE_OCCUPATION             256       /*  Occupation         */
#define PROFILE_INTERESTS              512       /*  Interests          */
#define PROFILE_COMMENTS               1024      /*  Comments           */
#define PROFILE_DISLIKES               512       /*  Dislikes           */
#define PROFILE_COUNTRY                128       /*  Country            */
#define PROFILE_HOBBIES                512       /*  Hobbies/interests  */
#define PROFILE_PICTURE                256       /*  URL of picture     */
#define PROFILE_DRINK                  512       /*  Favourite drinks   */
#define PROFILE_LIKES                  512       /*  Likes              */
#define PROFILE_MUSIC                  512       /*  Favourite music    */
#define PROFILE_OTHER                  1024      /*  Other information  */
#define PROFILE_SPORT                  512       /*  Favourite sport    */
#define PROFILE_CITY                   128       /*  City/town          */
#define PROFILE_EYES                   128       /*  Eye colour         */
#define PROFILE_FOOD                   512       /*  Favourite food     */
#define PROFILE_HAIR                   128       /*  Hair colour        */
#define PROFILE_IRL                    256       /*  Real name          */


/* ---->  Welcome/Assist constraints  <---- */
#define WELCOME_RESPONSE_TIME          2         /*  Time in minutes after which Assistants/Experienced Builders may respond to a welcome, if not responded to by Admin within that time  */
#define ASSIST_RESPONSE_TIME           2         /*  Time in minutes after which Assistants/Experienced Builders may respond to a welcome, if not responded to by Admin within that time  */
#define ASSIST_MINIMUM                 6         /*  Minimum number of users to be notified (If less than this, involve Assistants/Experienced)  */
#define ASSIST_TIME                    5         /*  Time (In minutes) user must wait before asking for assistance again (Stops unneccessary repetitive use of the 'assist' command)  */


/* ---->  Timings and constraints to guard against page/tell/mail/gripe 'bombing' (Or 'spamming')  <---- */
#define MAX_LOGIN_IDLE_TIME            5         /*  Amount of time (In minutes) a user at the Telnet login screen may idle before being automatically disconnected  */
#define TRANSACTION_TIME               5         /*  Amount of time (In seconds) to wait before notifying connected user of transaction (Payment)  */
#define DISCLAIMER_TIME                90        /*  Number of days after which user must read and accept the terms and conditions of the disclaimer again  */
#define PAGE_TIME_MORON                10        /*  Amount of time (In seconds) a Moron must wait between pages to a particular user  */
#define TIME_VARIATION                 10        /*  Maximum allowed time variation (In minutes)  -  Used to handle daylight saving, change of time on server, etc.  */
#define EMERGENCY_TIME                 5         /*  Time (In minutes) that commands will be logged for when 'emergency' is typed in TCZ  */
#define WAKE_IDLE_TIME                 2         /*  Amount of time (In minutes) user must be idle to be woken  */
#define MAX_IDLE_TIME                  60        /*  Amount of time (In minutes) a connected user may idle before being automatically disconnected  */
#define WARN_IDLE_TIME                 MAX((MAX_IDLE_TIME) - 15, 5)         /* Amount of time (In minutes) before showing user an idle warning message */
#define CONNECT_TIME                   30        /*  Amount of time (In seconds) to wait before notifying connected user of failed connection attempts  */
#define WHISPER_TIME                   1         /*  Amount of time (In seconds) a user must wait between whispers to a particular user  */
#define COMMENT_TIME                   60        /*  Amount of time (In seconds) a user must wait before they can make another complaint/comment/suggestion/bug report  */
#define NEWBIE_TIME                    7200      /*  Amount of total connect time (In seconds) a user is still considered a 'Newbie'.  */
#define IDLE_TIME                      5         /*  Allowed idle time (In minutes) before character's time spent idle is updated  */
#define MAIL_TIME                      15        /*  Amount of time (In seconds) a user must wait between sending mail to a particular user  */
#define NAME_TIME                      900       /*  Time before characters can change their name again  */
#define PAGE_TIME                      1         /*  Amount of time (In seconds) a user must wait between pages to a particular user  */
#define POST_TIME                      5         /*  Amount of time (In seconds) a user must wait between posting messages to the BBS  */
#define WAKE_TIME                      30        /*  Amount of time (In seconds) a user must wait between waking a particular user  */
#define AFK_TIME                       10        /*  Default idle time in minutes, after which a character will automatically be sent AFK (Away From Keyboard), for security  */


/* ---->  TCZ server statistics ('@stats tcz')  <---- */
#define STAT_HISTORY                   14        /*  Max. number of days to keep record of  */
#define STAT_TOTAL                     STAT_HISTORY
#define STAT_MAX                       STAT_HISTORY + 1


/* ---->  Execution time limits (In seconds) for compound commands  <---- */
#define STANDARD_EXEC_TIME_STARTUP     (1 * MINUTE)   /*  Default execution time limit for '.startup'/'.shutdown' compound commands in #4  */
#define MAX_EXEC_TIME_UPPER_ADMIN      10             /*  Maximum execution time limit for Elder Wizards/Druids and above  */
#define MAX_EXEC_TIME_LOWER_ADMIN      5              /*  Maximum execution time limit for Apprentice Wizards/Druids and above  */
#define MAX_EXEC_TIME_STARTUP          (10 * MINUTE)  /*  Maximum execution time limit for '.startup'/'.shutdown' compound commands in #4  */
#define MAX_EXEC_TIME_MORTAL           3              /*  Maximum execution time limit for Mortals  */
#define GUARDIAN_ALARM_TIME            (10 * MINUTE)  /*  Default guardian alarm time limit  */
#define STANDARD_EXEC_TIME             3              /*  Default execution time limit  */
#define MAX_EXEC_TIME                  60             /*  Absolute maximum execution time limit  */


/* ---->  Maximum compound command nesting allowed (I.e:  Calling within each other)  <---- */
#define MAX_CMD_NESTED_MORTAL          50        /*  Anyone below Apprentice Wizard status  */
#define MAX_CMD_NESTED_ADMIN           100       /*  Apprentice Wizards and above           */


/* ---->  Maximum number of nested commands (Ones in {}'s) allowed  <---- */
#define MAX_NESTED_MORTAL              16        /*  Anyone below Apprentice Wizard status  */
#define MAX_NESTED_ADMIN               32        /*  Apprentice Wizards and above           */
#define MAX_BRACKETS                   16        /*  Max. nested brackets allowed in @eval  */


/* ---->  Maximum number of temporary variables which can be defined at any one time  <---- */
#define MAX_TEMP_MORTAL                100       /*  Anyone below Apprentice Wizard status  */
#define MAX_TEMP_ADMIN                 250       /*  Apprentice Wizards and above (Elders and above are unrestricted)  */


/* ---->  Database dumping  <---- */
#define DUMP_CYCLE_INTERVAL            3         /*  Default dump cycle wait period (In seconds)   */
#define DUMP_CYCLE_DATA                128       /*  Default data dumped per cycle (In Kilobytes)  */
#define DUMP_INTERVAL                  900       /*  Default time (In seconds) between database dumps (15 mins  -  Can be set using '@admin')  */
#define DUMP_WAIT		       10        /*  Default time (In minutes) to wait at start-up for child dumping process to complete (Applicable only if DB_FORK is #define'd)  */

#define NEWDATABASE                    "new"     /*  Default new database name (When -g generate option is used.)  */
#define DATABASE                       "main"    /*  Default database name  */


/* ---->  Memory restrictions  <---- */
#define LIMIT_MEMORY_PERCENTAGE        0.50      /*  Maximum memory expansion allowed (Over initial size at start-up), as percentage (0.50 = 50%)  */
#define LIMIT_MEMORY_MINIMUM           16        /*  Minimum memory expansion allowed (Mb) (If LIMIT_MEMORY_PERCENTAGE gives value less than this, expansion will be set to this limit.)  */
#define LIMIT_STACK                    7000      /*  Maximum size of stack (Kb)  */


/* ---->  E-mail addresses/E-mail forwarding  <---- */
#define EMAIL_FORWARD_QUANTITY         100       /*  Number of E-mail addresses to dump in one go  */
#define EMAIL_FORWARD_INTERVAL         30        /*  Interval (In seconds) to wait before dumping next EMAIL_FORWARD_QUANITY E-mail addresses  */
#define EMAIL_FORWARD_HOUR             22        /*  Time (Each day) to write list of E-mail aliases to 'lib/forward.tcz'  */
#define EMAIL_ADDRESSES                5         /*  Number of E-mail addresses per character (2nd address is private, the rest are public)  */


/* ---->  Name of forwarding E-mail server (Appended to 'tcz.user.name@')  <---- */
#ifdef DEMO
   #define EMAIL_FORWARD_NAME          "unavailable"
#else
   #define EMAIL_FORWARD_NAME          "localhost"
#endif


/* ---->  Title constants  <---- */
#define AUTO_AFK_TITLE	               ANSI_LMAGENTA" has been sent "ANSI_LRED""ANSI_UNDERLINE"AFK"ANSI_LMAGENTA" due to idling."
#define BIRTHDAY_TITLE		       " is %ld today  "ANSI_DCYAN"-  "ANSI_LRED""ANSI_BLINK"H"ANSI_LGREEN""ANSI_BLINK"A"ANSI_LYELLOW""ANSI_BLINK"P"ANSI_LBLUE""ANSI_BLINK"P"ANSI_LMAGENTA""ANSI_BLINK"Y "ANSI_LCYAN""ANSI_BLINK"B"ANSI_LWHITE""ANSI_BLINK"I"ANSI_LRED""ANSI_BLINK"R"ANSI_LGREEN""ANSI_BLINK"T"ANSI_LYELLOW""ANSI_BLINK"H"ANSI_LBLUE""ANSI_BLINK"D"ANSI_LMAGENTA""ANSI_BLINK"A"ANSI_LCYAN""ANSI_BLINK"Y"ANSI_LWHITE""ANSI_BLINK"!"
#define BANNED_TITLE                   ANSI_LRED" is currently "ANSI_LMAGENTA""ANSI_UNDERLINE"banned"ANSI_LRED" from "ANSI_LYELLOW"%s"ANSI_LRED"."
#define GUEST_TITLE                    ANSI_LCYAN"is a guest to "ANSI_LYELLOW"%s"ANSI_LCYAN"."
#define LOST_TITLE		       ANSI_DCYAN" has lost %s connection."
#define ROOT_TITLE                     "%%g%%lis the administrator of %%y%%l%s%%g%%l."
#define AFK_TITLE	               ANSI_LMAGENTA" is "ANSI_LRED""ANSI_UNDERLINE"AFK"ANSI_LMAGENTA" and idle."
#define NEW_TITLE                      ANSI_LGREEN"is new to "ANSI_LYELLOW"%s"ANSI_LGREEN"!"


/* ---->  Server  <---- */
#define EVENT_PROCESSING_LIMIT         1         /*  Maximum number of pending events that can be processed per call of tcz_time_sync()  */
#define RESERVED_DESCRIPTORS           13        /*  Reserved file descriptors (For log files, database dump, etc.)  */
#define IDLE_STATE_INTERVAL            30        /*  Time (In minutes) of inactivity before entering semi-idle state  */
#define INPUT_MAX_EDITOR               256       /*  Maximum number of queued user commands (Per user) while using the editor (Higher to allow cut 'n' paste of text)  */
#define IDLE_STATE_DELAY               5         /*  Idle state delay (In seconds)  */
#define OUTPUT_MAX                     17408     /*  Maximum amount of queued user output in bytes (Per user)  */
#define INPUT_MAX                      64        /*  Maximum number of queued user commands (Per user)  */
#define KEEPALIVE                      5         /*  Time (In minutes) to keep connection open, if closed improperly  */
#define PRIORITY                       0         /*  Default scheduling priority (-20 = Highest, 20 = Lowest)  -  Needs to be ran as root for priority < 0  */


/* ---->  Miscellaneous  <---- */
#define LOG_BACKTRACK_CONSTRAINT       4096      /*  Maximum number of lines within log files that can be back-tracked via the '@log' command  */
#define MORTAL_SHOUT_CONSTRAINT        24        /*  Number of hours total time connected a Mortal must have to be able to shout messages (Mortals can only shout if TCZ is compiled with -DMORTAL_SHOUT)  */
#define CHAT_INVITE_QUEUE_SIZE         10        /*  Size of invite queue for each chatting channel  */
#define MATCH_RECURSION_LIMIT          100       /*  Maximum recursion limit for matching routines (match.c)  */
#define WARN_LOGIN_INTERVAL            15        /*  Interval (Minutes) between giving warnings of failed logins  */
#define MAX_STORED_MESSAGES            8         /*  Max. stored pages/tells per user  */
#define DESTROY_QUEUE_SIZE             100       /*  Size of '@undestroy' object queue  */
#define PROGRESS_UNITS                 4         /*  Number of units ('.') per megabyte on progress meter  */
#define MAX_LIST_LIMIT                 100       /*  Maximum number of users who can be specified in a ','/';' separated list (Used by 'page', 'tell', friends/enemies code, etc.)  */
#define RETURN_BUFFERS                 16        /*  Number of return buffers for function that can be called multiple times in same instance  */
#define DST_THRESHOLD                  10        /*  Daylight Saving Time detection threshold (In seconds)  -  1 hour +/- threshold  */
#define ARRAY_LIMIT                    1000      /*  Max. number of dynamic array elements that can be created in one go  */
#define PARAMETERS                     8         /*  Maximum number of parameters for unparse_parameters()  */
#define ENTITIES                       7         /*  Maximum entities in interval() routine  */


/* ---->  Default time/date formats (See 'help dateformat' on TCZ)  <---- */
#define DATESEPARATOR                  ", "                /*  Default date separator  */
#define SHORTDATEFMT                   "dd/mm/yyyy"        /*  Default short date format  */
#define LONGDATEFMT                    "WW D MM yyyy"      /*  Default long date format  */
#define TIMEFMT                        "h:nn.ss pp"        /*  Default time format  */
#define DATEORDER                      0                   /*  Default date order (0 = Date first, 1 = Time first.)  */
#define FULLDATEFMT                    LONGDATEFMT""DATESEPARATOR""TIMEFMT  /*  Default date & time format  */


/* ---->  Colours used for character ranks  <---- */
#define DEITY_COLOUR             ANSI_LMAGENTA   /*  Deities               */
#define ELDER_COLOUR             ANSI_LGREEN     /*  Elder Wizards         */
#define ELDER_DRUID_COLOUR       ANSI_DGREEN     /*  Elder Druids          */
#define WIZARD_COLOUR            ANSI_LCYAN      /*  Wizards               */
#define DRUID_COLOUR             ANSI_DCYAN      /*  Druids                */
#define APPRENTICE_COLOUR        ANSI_LYELLOW    /*  Apprentice Wizards    */
#define APPRENTICE_DRUID_COLOUR  ANSI_DYELLOW    /*  Apprentice Druids     */
#define RETIRED_COLOUR           ANSI_LRED       /*  Retired Wizards       */
#define RETIRED_DRUID_COLOUR     ANSI_DRED       /*  Retired Druids        */
#define EXPERIENCED_COLOUR       ANSI_LRED       /*  Experienced Builders  */
#define ASSISTANT_COLOUR         ANSI_DRED       /*  Assistants            */
#define BUILDER_COLOUR           ANSI_LWHITE     /*  Builders              */
#define MORTAL_COLOUR            ANSI_LWHITE     /*  Mortals               */
#define MORON_COLOUR             ANSI_LBLUE      /*  Morons                */


/* ---->  Memory compression constants (When EXTRA_COMPRESSION is #define'd)  <---- */
#define COMPRESSION_QUEUE_SIZE   1000000         /*  Absolute maximum number of entries allowed in compression queue (Used when initialising compression table)  */
#define COMPRESSION_TABLE_SIZE   4094            /*  Size of compression table (Any value up to 4094)  */
#define COMPRESSION_MAX_WORDLEN  30              /*  Maximum length (In characters) of compressable word  */


/* ---->  Database compression constants (When DB_COMPRESSION is #define'd)  <---- */
#ifdef DB_COMPRESSION_GZIP
   #define DB_COMPRESS           "gzip"          /*  Compression program    */
   #define DB_DECOMPRESS         "gunzip"        /*  Decompression program  */
   #define DB_EXTENSION          ".gz"           /*  Filename extension     */
#else
   #define DB_COMPRESS           "compress"      /*  Compression program    */
   #define DB_DECOMPRESS         "decompress"    /*  Decompression program  */
   #define DB_EXTENSION          ".Z"            /*  Filename extension     */
#endif


/* ---->  Names of various files used by the TCZ server  <---- */
#define GENERIC_TUTORIAL_FILE    "lib/generic.tutorial.tcz"  /*  Generic tutorials file  */
#define LOCAL_TUTORIAL_FILE      "lib/local.tutorial.tcz"    /*  Local tutorials file  */
#define GENERIC_HELP_FILE        "lib/generic.help.tcz"      /*  Generic help topics file  */
#define LOCAL_HELP_FILE          "lib/local.help.tcz"        /*  Local help topics file  */
#define COLOURMAP_FILE           "lib/colourmap.tcz"         /*  Map colour map (Defines ANSI colours for ASCII map)  */
#define FORWARD_FILE             "lib/forward.tcz"           /*  File to dump E-mail addresses of all characters in the database to on a daily basis (For E-mail forwarding)  */
#define PROC_MEMINFO             "/proc/meminfo"             /*  Memory usage information   */
#define TERMCAP_FILE             "lib/termcap"               /*  Location of system (Or custom) terminal capabilities file  */
#define CONFIG_FILE              "lib/default.tcz"           /*  Default configuration file  */
#define PROC_UPTIME              "/proc/uptime"              /*  System uptime information  */
#define EMAIL_FILE               "lib/email.tcz"             /*  File to dump E-mail addresses of all characters in the database to ('@dump email addresses')  */
#define TITLE_FILE               "lib/title%d.tcz"           /*  Prototype for loading title screens, displayed when users first connect via Telnet.  */
#define PROC_STATM               "/proc/%d/statm"            /*  Prototype for process memory statistics information ('%d' is replaced with PID of TCZ.)  */
#define DISC_FILE                "lib/disclaimer.tcz"        /*  Disclaimer  */
#define PROC_STAT                "/proc/%d/stat"             /*  Prototype for process statistics information ('%d' is replaced with PID of TCZ.)  */
#define SITE_FILE                "lib/sites.tcz"             /*  Internet sites file  */
#define MAP_FILE                 "lib/map.tcz"               /*  Map (ASCII only)  */


#define CONVERSE_CMD_TOKEN       '.'             /*  Converse command execution token  */
#define PROMPT_CMD_TOKEN         '.'             /*  '@prompt' command execution token  */
#define EDIT_CMD_TOKEN           '.'             /*  Edit command execution token  */
#define ABSOLUTE_TOKEN           '|'             /*  Absolute command look-up token  */
#define LIST_SEPARATOR           ';'             /*  Default list seperator  */
#define COMMAND_TOKEN            '@'             /*  Built-in command look-up token  */
#define LOOKUP_TOKEN             '*'             /*  Character name look-up token  */
#define NUMBER_TOKEN             '#'             /*  Object #ID look-up token  */
#define MATCH_EXACT              '!'             /*  Match exact object name token  */
#define MATCH_TOKEN              '@'             /*  Match specific object type(s) token  */
#define FLAG_TOKEN               '@'             /*  Flag boolean token  */
#define NOT_TOKEN                '!'             /*  NOT boolean token  */
#define AND_TOKEN                '&'             /*  AND boolean token  */
#define OR_TOKEN                 '|'             /*  OR boolean token  */

#define ALT_CHANNEL_TOKEN        "_"             /*  Alternative 'chat' channel short-hand token  */
#define ALT_REMOTE_TOKEN         ","             /*  Alternative 'remote' short-hand token  */
#define ALT_THINK_TOKEN          "="             /*  Alternative 'think' short-hand token  */
#define BROADCAST_TOKEN          ";"             /*  'shout' broadcast short-hand token  */
#define ALT_POSE_TOKEN           ";"             /*  Alternative 'pose' short-hand token  */
#define ALT_SAY_TOKEN1           "\'"            /*  Alternative 'say' short-hand token  */
#define ALT_SAY_TOKEN2           "`"             /*  Alternative 'say' short-hand token  */
#define ALT_TELL_TOKEN           "."             /*  Alternative 'tell' short-hand token  */
#define CHANNEL_TOKEN            "-"             /*  'chat' channel short-hand token  */
#define REMOTE_TOKEN             "<"             /*  'remote' short-hand token  */
#define SHOUT_TOKEN              "!"             /*  'shout' short-hand token  */
#define THINK_TOKEN              "+"             /*  'think' short-hand token  */
#define ECHO_TOKEN               "~"             /*  '@echo' short-hand token  */
#define POSE_TOKEN               ":"             /*  'pose' short-hand token  */
#define TELL_TOKEN               ">"             /*  'tell' short-hand token  */
#define ASK_TOKEN                "?"             /*  'ask' short-hand token  */
#define SAY_TOKEN                "\""            /*  'say' short-hand token  */

#define TOKEN(x) ((x == *SAY_TOKEN) || (x == *ALT_SAY_TOKEN1) || (x == *POSE_TOKEN) || (x == *TELL_TOKEN) || (x == *THINK_TOKEN) || (x == *ASK_TOKEN) || (x == *CHANNEL_TOKEN) || (x == *ALT_SAY_TOKEN2) || (x == *ALT_POSE_TOKEN) || (x == *ALT_TELL_TOKEN) || (x == *ALT_THINK_TOKEN) || (x == *REMOTE_TOKEN) || (x == *ALT_REMOTE_TOKEN) || (x == *BROADCAST_TOKEN) || (x == *SHOUT_TOKEN) || (x == *ECHO_TOKEN) || (x == *ALT_CHANNEL_TOKEN))


/* ---->  System specific configuration  <---- */
#ifdef LINUX

   /* ---->  Linux specific configuration  <---- */
   #define DB_COMPRESSION_GZIP
   #undef  REFRESH_SOCKETS
   #define DB_COMPRESSION
   #define USE_PROC
#endif 

#ifdef CYGWIN32

   /* ---->  Cygwin32 specific configuration (Win95/Win98/WinNT)  <---- */
   #undef  DB_COMPRESSION_GZIP
   #undef  EMAIL_FORWARDING
   #undef  REFRESH_SOCKETS
   #undef  RESTRICT_MEMORY
   #undef  DB_COMPRESSION
   #undef  UPS_SUPPORT
   #undef  SERVERINFO
   #undef  USE_PROC

   #define srand48(value) srand(value)  /* No srand48()  */
   #define lrand48()      rand()        /* No lrand48()  */
   #define SIGIOT         6             /* SIGIOT not defined  */
#endif

#ifdef SUNOS

   /* ---->  SunOS specific configuration  <---- */
#endif

#ifdef SOLARIS

   /* ---->  Solaris specific configuration  <---- */
#endif

#ifdef DEMO
   #undef EMAIL_FORWARDING
   #undef NOTIFY_WELCOME
   #undef PANIC_DUMP
   #undef WARN_DUPLICATES
   #undef EXPERIENCED_HELP
   #undef DATABASE_DUMP
   #undef LOG_FAILED_COMMANDS
   #undef LOG_COMMANDS
   #undef LOG_ALARMS
   #undef LOG_FUSES
   #undef LOG_HACKS
#endif


#endif  /* __CONFIG_H  */
