/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| OPTIONS.C  -  Handles system command-line arguments and configuration       |
|               files.                                                        |
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
| Module originally designed and written by:  J.P.Boggis 24/12/1999.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: options.c,v 1.4 2005/06/29 21:35:30 tcz_monster Exp $

*/


#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "prompts.h"
#include "config.h"
#include "db.h"

#include "object_types.h"
#include "flagset.h"

#ifdef DB_FORK
#include <signal.h>
#endif


/* ---->  Valid long options (--option)  <---- */
/*        (Name, Has Arg (0 = No, 1 = Yes, 2 = Optional), Flag, Value)  */
static struct option options[] = {
   {"admin",          1, 0, 'E'},  /*  Admin. E-mail address  */
   {"adminemail",     1, 0, 'E'},  /*  Admin. E-mail address  */
   {"advanced",       0, 0, 'a'},  /*  Display advanced options  */
   {"autodump",       2, 0, 'A'},  /*  Automatic database dumping  */
   {"automaticdump",  2, 0, 'A'},  /*  Automatic database dumping  */
   {"backdoor",       1, 0, 'b'},  /*  Backdoor password  */
   {"check",          2, 0, 'C'},  /*  Force full database consistency check  */
   {"compress",       2, 0, 'z'},  /*  Database compression in memory  */
   {"compressdisk",   2, 0, 'Z'},  /*  Database compression on disk  */
   {"compressmem",    2, 0, 'z'},  /*  Database compression in memory  */
   {"compressmemory", 2, 0, 'z'},  /*  Database compression in memory  */
   {"compression",    2, 0, 'z'},  /*  Database compression in memory  */
   {"config",         1, 0, 'c'},  /*  Config file  */
   {"console",        2, 0, 'V'},  /*  Display log file entries on console  */
   {"consolelog",     2, 0, 'V'},  /*  Display log file entries on console  */
   {"consolelogging", 2, 0, 'V'},  /*  Display log file entries on console  */
   {"core",           2, 0, 'K'},  /*  Core dump  */
   {"coredump",       2, 0, 'K'},  /*  Core dump  */
   {"current",        0, 0, 'O'},  /*  Display current options  */
   {"database",       1, 0, 'd'},  /*  Database name  */
   {"databasecheck",  2, 0, 'C'},  /*  Force full database consistency check  */
   {"data",           1, 0, 'D'},  /*  HTML Interface data URL  */
   {"dataurl",        1, 0, 'D'},  /*  HTML Interface data URL  */
   {"debug",          2, 0, 'G'},  /*  Run in debugging/development mode  */
   {"debugging",      2, 0, 'G'},  /*  Run in debugging/development mode  */
   {"develop",        2, 0, 'G'},  /*  Run in debugging/development mode  */
   {"developer",      2, 0, 'G'},  /*  Run in debugging/development mode  */
   {"dir",            1, 0, 'p'},  /*  Path  */
   {"directory",      1, 0, 'p'},  /*  Path  */
   {"dns",            2, 0, 'n'},  /*  DNS Lookups  */
   {"DNS",            2, 0, 'n'},  /*  DNS Lookups  */
   {"dump",           2, 0, 'A'},  /*  Automatic database dumping  */
   {"dumping",        2, 0, 'A'},  /*  Automatic database dumping  */
   {"email",          1, 0, 'e'},  /*  E-mail forwarding domain name  */
   {"e-mail",         1, 0, 'e'},  /*  E-mail forwarding domain name  */
   {"emailforward",   1, 0, 'e'},  /*  E-mail forwarding domain name  */
   {"emergency",      2, 0, 'P'},  /*  Emergency database dump  */
   {"emergencydump",  2, 0, 'P'},  /*  Emergency database dump  */
   {"encryption",     2, 0, 'S'},  /*  SSL (Secure Sockets Layer) connections  */
   {"guardian",       2, 0, 'x'},  /*  Guardian alarm  */
   {"guardianalarm",  2, 0, 'x'},  /*  Guardian alarm  */
   {"generate",       1, 0, 'g'},  /*  Generate new empty database  */
   {"help",           0, 0, 'h'},  /*  Display help  */
   {"html",           1, 0, 'w'},  /*  HTML port  */
   {"htmlport",       1, 0, 'w'},  /*  HTML port  */
   {"HTML",           1, 0, 'w'},  /*  HTML port  */
   {"home",           1, 0, 'W'},  /*  Web site URL  */
   {"homeurl",        1, 0, 'W'},  /*  Web site URL  */
   {"homepage",       1, 0, 'W'},  /*  Web site URL  */
   {"fork",           2, 0, 'F'},  /*  Forked database dump  */
   {"forked",         2, 0, 'F'},  /*  Forked database dump  */
   {"forkdump",       2, 0, 'F'},  /*  Forked database dump  */
   {"forkeddump",     2, 0, 'F'},  /*  Forked database dump  */
   {"forward",        1, 0, 'e'},  /*  E-mail forwarding name  */
   {"forwarding",     1, 0, 'e'},  /*  E-mail forwarding name  */
   {"full",           1, 0, 'y'},  /*  Full name of server  */
   {"fullname",       1, 0, 'y'},  /*  Full name of server  */
   {"images",         2, 0, 'i'},  /*  Serve HTML images internally  */
   {"image",          2, 0, 'i'},  /*  Serve HTML images internally  */
   {"information",    0, 0, 'v'},  /*  Display TCZ version number  */
   {"info",           0, 0, 'v'},  /*  Display TCZ version number  */
   {"internal",       2, 0, 'i'},  /*  Serve HTML images internally  */
   {"internally",     2, 0, 'i'},  /*  Serve HTML images internally  */
   {"kill",           0, 0, 'k'},  /*  Signal handling behaviour  */
   {"local",          2, 0, 'l'},  /*  Run locally (127.0.0.1)  */
   {"localhost",      2, 0, 'l'},  /*  Run locally (127.0.0.1)  */
   {"location",       1, 0, 'H'},  /*  Location of server  */
   {"log",            1, 0, 'L'},  /*  Logging level  */
   {"login",          2, 0, 'u'},  /*  User logins  */
   {"logins",         2, 0, 'u'},  /*  User logins  */
   {"loglevel",       1, 0, 'L'},  /*  Logging level  */
   {"logging",        1, 0, 'L'},  /*  Logging level  */
   {"logginglevel",   1, 0, 'L'},  /*  Logging level  */
   {"message",        1, 0, 'm'},  /*  MOTD login message  */
   {"motd",           1, 0, 'm'},  /*  MOTD login message  */
   {"nice",           1, 0, 'N'},  /*  Nice level  */
   {"nicelevel",      1, 0, 'N'},  /*  Nice level  */
   {"options",        0, 0, 'o'},  /*  Display start-up options  */
   {"panic",          2, 0, 'P'},  /*  Emergency database dump  */
   {"panicdump",      2, 0, 'P'},  /*  Emergency database dump  */
   {"path",           1, 0, 'p'},  /*  Path  */
   {"port",           1, 0, 't'},  /*  Telnet port  */
   {"quit",           2, 0, 'Q'},  /*  Exit after processing options  */
   {"telnet",         1, 0, 't'},  /*  Telnet port  */
   {"telnetport",     1, 0, 't'},  /*  Telnet port  */
   {"secure",         2, 0, 'S'},  /*  SSL (Secure Sockets Layer) connections  */
   {"securesockets",  2, 0, 'S'},  /*  SSL (Secure Sockets Layer) connections  */
   {"server",         1, 0, 's'},  /*  Server address  */
   {"serverinfo",     2, 0, 'I'},  /*  Automatically lookup server info  */
   {"signals",        0, 0, 'k'},  /*  Signal handling behaviour  */
   {"site",           1, 0, 'W'},  /*  Web site URL  */
   {"short",          1, 0, 'Y'},  /*  Short abbreviated name of server  */
   {"shortname",      1, 0, 'Y'},  /*  Short abbreviated name of server  */
   {"shutdown",       2, 0, 'X'},  /*  Immediate shutdown (After start-up)  */
   {"ssl",            2, 0, 'S'},  /*  SSL (Secure Sockets Layer) connections  */
   {"tczfull",        1, 0, 'y'},  /*  Full name  */
   {"tczfullname",    1, 0, 'y'},  /*  Full name  */
   {"tczshort",       1, 0, 'Y'},  /*  Short abbreviated name of server  */
   {"tczshortname",   1, 0, 'Y'},  /*  Short abbreviated name of server  */
   {"url",            1, 0, 'D'},  /*  HTML Interface data URL  */
   {"user",           2, 0, 'U'},  /*  User log files  */
   {"userlog",        2, 0, 'U'},  /*  User log files  */
   {"userlogs",       2, 0, 'U'},  /*  User log files  */
   {"userlogfile",    2, 0, 'U'},  /*  User log files  */
   {"userlogfiles",   2, 0, 'U'},  /*  User log files  */
   {"userlogin",      2, 0, 'u'},  /*  User logins  */
   {"userlogins",     2, 0, 'u'},  /*  User logins  */
   {"version",        0, 0, 'v'},  /*  Display TCZ version number  */
   {"web",            1, 0, 'w'},  /*  HTML port  */
   {"webport",        1, 0, 'w'},  /*  HTML port  */
   {"website",        1, 0, 'W'},  /*  Web site URL  */
   {"working",        1, 0, 'p'},  /*  Path  */
   {0,                0, 0, 0}
};


/* ---->  Description of logging levels  <---- */
static const char *loglevels[]={
   "Disabled",
   "Normal (Failed commands logged)",
   "Verbose (Top-level commands logged, user privacy upheld)",
   "Low Debug (Top-level commands logged, user privacy breeched)",
   "Medium Debug (Sub-commands in compound commands logged)",
   "High Debug (Fuse and alarm execution logged)",
   "IO Debug (Telnet/HTML descriptor input/output logged)",
};

static struct option_list_data *option_list = NULL;


/* ---->  {J.P.Boggis 27/02/2000}  Boolean (Yes/No) argument  <---- */
int option_boolean(const char *optarg,int defaultvalue)
{
    if(!Blank(optarg)) {
       if(!(string_prefix("Yes",optarg) || string_prefix("On",optarg) || (*optarg == '1'))) {
          if(!(string_prefix("No",optarg) || string_prefix("Off",optarg) || (*optarg == '0'))) {
             return(defaultvalue);
	  } else return(0);
       } else return(1);
    }
    return(defaultvalue);
}

/* ---->  {J.P.Boggis 27/02/2000}  Admin. E-mail address  <---- */
const char *option_adminemail(OPTCONTEXT)
{
      if(status) return(tcz_admin_email);

      if(!Blank(value)) {
         if(strlen(value) <= 256) {
	    if(strcasecmp(value,tcz_admin_email))
	       writelog(OPTIONS_LOG,0,title,"Admin E-mail address changed to '%s'.",value);

            if(Validchar(player)) {
               if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Admin E-mail address changed to '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",value);
               writelog(ADMIN_LOG,0,title,"%s(#%d) changed the Admin E-mail address option to '%s'.",getname(player),player,value);
	    }

	    FREENULL(tcz_admin_email);
	    tcz_admin_email = alloc_string(value);
            return(tcz_admin_email);
	 } else if(Validchar(player)) {
            output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Admin. E-mail address is too long (Limit is 256 characters.)");
	 } else writelog(OPTIONS_LOG,0,title,"Sorry, Admin. E-mail address is too long (Limit is 256 characters.)");
      } else if(Validchar(player)) {
         output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Admin. E-mail address cannot be blank.");
      } else writelog(OPTIONS_LOG,0,title,"Sorry, Admin. E-mail address cannot be blank.");

      (*error)++, (*critical)++;
      return(tcz_admin_email);
}

/* ---->  {J.P.Boggis 27/02/2000}  Backdoor password  <---- */
const char *option_backdoor(OPTCONTEXT)
{
      static const char *backdoor = NULL;

      if(status) return(backdoor);

      if(Blank(value) || (strlen(value) >= 6)) {
         if(Blank(value) || (strlen(value) <= 32)) {
            writelog(OPTIONS_LOG,1,title,"Backdoor password is %sabled.",(value) ? "en":"dis");
            FREENULL(backdoor);
            backdoor = alloc_string(value);
            return(backdoor);
	 } else writelog(OPTIONS_LOG,1,title,"Sorry, backdoor password is too long (Limit is 32 characters.)"), (*error)++;
      } else writelog(OPTIONS_LOG,1,title,"Sorry, backdoor password is too short (Must be 6 or more characters.)"), (*error)++;

      (*error)++, (*critical)++;
      return(backdoor);
}

/* ---->  {J.P.Boggis 27/02/2000}  Force full database consistency check  <---- */
int option_check(OPTCONTEXT)
{
    static int check = 0;
    int    new;

    if(status) return(check);

    new = option_boolean(value,1);
    if(new != check) writelog(OPTIONS_LOG,0,title,"Full consistency check will%s be performed on the database.",(new) ? "":" not");
    check = new;
    return(check);
}

/* ---->  {J.P.Boggis 22/07/2000}  Run in debugging/developer mode  <---- */
int option_debug(OPTCONTEXT)
{
    static int debug = 0;
    int    new;

    if(status) return(debug);

    if(Validchar(player) && Blank(value)) {
       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the new setting for '"ANSI_LWHITE"Run in debugging/developer mode"ANSI_LGREEN"'.");
       (*error)++, (*critical)++;
       return(debug);
    }

    new = option_boolean(value,1);
    if(new != debug) writelog(OPTIONS_LOG,0,title,"%s will%s run in debugging/developer mode.",tcz_short_name,(new) ? "":" not");
    debug = new;

    if(Validchar(player)) {
       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s "ANSI_LWHITE"will%s"ANSI_LGREEN" run in debugging/developer mode.",tcz_short_name,(new) ? "":" not");
       writelog(ADMIN_LOG,0,title,"%s(#%d) changed the debugging/developer option to %s.",getname(player),player,(new) ? "ON":"OFF");
    }
    return(debug);
}

/* ---->  {J.P.Boggis 27/02/2000}  Database compression (Disk)  <---- */
int option_compress_disk(OPTCONTEXT)
{
#ifdef DB_COMPRESSION
    static int compress_disk = 1;
    int    new;
#else
    static int compress_disk = 0;
#endif

    if(status) return(compress_disk);

#ifdef DB_COMPRESSION_GZIP
    #define DB_COMPRESSION_FORMAT "gzip"
#else
    #define DB_COMPRESSION_FORMAT "compress"
#endif

#ifdef DB_COMPRESSION
    new = option_boolean(value,1);
    if(new != compress_disk) writelog(OPTIONS_LOG,0,title,"The database will%s be compressed when dumped to disk ('"DB_COMPRESSION_FORMAT"' format.)",(new) ? "":" not");
    compress_disk = new;
#else
    writelog(OPTIONS_LOG,0,title,"Sorry, database disk compression option is not supported (#define DB_COMPRESSION)");
    (*error)++, (*critical)++;
#endif
    return(compress_disk);
}

/* ---->  {J.P.Boggis 27/02/2000}  Database compression (Memory)  <---- */
int option_compress_memory(OPTCONTEXT)
{
#ifdef EXTRA_COMPRESSION
    static int compress_memory = 1;
    int    new;
#else
    static int compress_memory = 0;
#endif

    if(status) return(compress_memory);

#ifdef EXTRA_COMPRESSION
    new = option_boolean(value,1);
    if(new != compress_memory) writelog(OPTIONS_LOG,0,title,"The database will%s be compressed in memory.",(new) ? "":" not");
    compress_memory = new;
#else
    writelog(OPTIONS_LOG,0,title,"Sorry, database memory compression option is not supported (#define EXTRA_COMPRESSION)");
    (*error)++, (*critical)++;
#endif
    return(compress_memory);
}

/* ---->  {J.P.Boggis 01/07/2000}  Display log file entries on console  <---- */
int option_console(OPTCONTEXT)
{
    static int console = 0;
    int    new;

    if(status) return(console);

    if(Validchar(player) && Blank(value)) {
       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the new setting for '"ANSI_LWHITE"Display log entries on console"ANSI_LGREEN"'.");
       (*error)++, (*critical)++;
       return(console);
    }

    new = option_boolean(value,1);
    if(new != console) writelog(OPTIONS_LOG,0,title,"All log file entries will%s be displayed on the console (stderr.)",(new) ? "":" not");
    console = new;

    if(Validchar(player)) {
       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"All log file entries "ANSI_LWHITE"will%s"ANSI_LGREEN" be displayed on the server console ("ANSI_LYELLOW"stderr"ANSI_LGREEN".)",(new) ? "":" not");
       writelog(ADMIN_LOG,0,title,"%s(#%d) changed the console logging option to %s.",getname(player),player,(new) ? "ON":"OFF");
    }
    return(console);
}

/* ---->  {J.P.Boggis 05/03/2000}  Core dumping  <---- */
int option_coredump(OPTCONTEXT)
{
    static int coredump = 1;
    int    new;

    if(status) return(coredump);

    if(Validchar(player) && Blank(value)) {
       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the new setting for '"ANSI_LWHITE"Core dump on crash"ANSI_LGREEN"'.");
       (*error)++, (*critical)++;
       return(coredump);
    }

    new = option_boolean(value,1);
    if(new != coredump) writelog(OPTIONS_LOG,0,title,"A core dump will%s be produced if %s crashes.",(new) ? "":" not",tcz_short_name);
    coredump = new;

    if(Validchar(player)) {
       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"A core dump "ANSI_LWHITE"will%s"ANSI_LGREEN" be produced if %s crashes.",(new) ? "":" not",tcz_short_name);
       writelog(ADMIN_LOG,0,title,"%s(#%d) changed the core dump option to %s.",getname(player),player,(new) ? "ON":"OFF");
    }
    return(coredump);
}

/* ---->  {J.P.Boggis 27/02/2000}  Database name  <---- */
const char *option_database(OPTCONTEXT)
{
      static const char *database = NULL;

      if(!database) database = alloc_string(DATABASE);
      if(status) return(database);

      if(Blank(value)) value = DATABASE;
      if(strlen(value) <= 64) {
         if(!database || strcasecmp(value,database))
            writelog(OPTIONS_LOG,0,title,"Using database name '%s'.",value);
         FREENULL(database);
         database = alloc_string(value);
         return(database);
      } else writelog(OPTIONS_LOG,0,title,"Sorry, database name is too long (Limit is 64 characters.)");

      (*error)++, (*critical)++;
      return(database);
}

/* ---->  {J.P.Boggis 27/02/2000}  HTML Interface data URL  <---- */
const char *option_dataurl(OPTCONTEXT)
{
      if(status) return(html_data_url);

#ifdef HTML_INTERFACE
      if(!Blank(value)) {
         if(strlen(value) <= 512) {
	    if(value[strlen(value) - 1] != '/')
	       sprintf(scratch_return_string,"%s/",value);
		  else strcpy(scratch_return_string,value);

	    if(strcasecmp(scratch_return_string,html_data_url))
	       writelog(OPTIONS_LOG,0,title,"HTML Interface data URL changed to '%s'.",scratch_return_string);

            if(Validchar(player)) {
               if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"HTML Interface data URL changed to '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",scratch_return_string);
               writelog(ADMIN_LOG,0,title,"%s(#%d) changed the HTML Interface data URL option to '%s'.",getname(player),player,scratch_return_string);
	    }
      
	    FREENULL(html_data_url);
	    html_data_url = alloc_string(scratch_return_string);
            return(html_data_url);
	 } else if(Validchar(player)) {
            output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, HTML Interface data URL is too long (Limit is 512 characters.)");
	 } else writelog(OPTIONS_LOG,0,title,"Sorry, HTML Interface data URL is too long (Limit is 512 characters.)");
      } else if(Validchar(player)) {
         output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, HTML Interface data URL cannot be blank.");
      } else writelog(OPTIONS_LOG,0,title,"Sorry, HTML Interface data URL cannot be blank.");
#else
      if(Validchar(player)) {
         output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, HTML Interface data URL option is not supported.");
      } else writelog(OPTIONS_LOG,0,title,"Sorry, HTML Interface data URL option is not supported (#define HTML_INTERFACE)");
#endif

      (*error)++, (*critical)++;
      return(html_data_url);
}

/* ---->  {J.P.Boggis 19/03/2000}  DNS Lookups  <---- */
int option_dns(OPTCONTEXT)
{
    static int dnslookup = 0;
    int    new;

    if(status) return(dnslookup);

    if(Validchar(player) && Blank(value)) {
       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the new setting for '"ANSI_LWHITE"DNS lookups"ANSI_LGREEN"'.");
       (*error)++, (*critical)++;
       return(dnslookup);
    }

    new = option_boolean(value,1);
    if(new != dnslookup) writelog(OPTIONS_LOG,0,title,"Host DNS of users will%s be looked up.",(new) ? "":" not");
    dnslookup = new;

    if(Validchar(player)) {
       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Host DNS of users "ANSI_LWHITE"will%s"ANSI_LGREEN" be looked up.",(new) ? "":" not");
       writelog(ADMIN_LOG,0,title,"%s(#%d) changed the DNS lookup option to %s.",getname(player),player,(new) ? "ON":"OFF");
    }
    return(dnslookup);
}

/* ---->  {J.P.Boggis 28/02/2000}  Automatic database dumping  <---- */
int option_dumping(OPTCONTEXT)
{
#ifdef DATABASE_DUMP
    static int dumping = 1;
    int    new;
#else
    static int dumping = 0;
#endif

    if(status) return(dumping);

#ifdef DATABASE_DUMP
    if(Validchar(player) && Blank(value)) {
       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the new setting for '"ANSI_LWHITE"Automatically dump database"ANSI_LGREEN"'.");
       (*error)++, (*critical)++;
       return(dumping);
    }

    new = option_boolean(value,1);
    if(new != dumping) writelog(OPTIONS_LOG,0,title,"The database will%s be automatically dumped to disk while %s is running.",(new) ? "":" not",tcz_short_name);
    dumping = new;

    if(Validchar(player)) {
       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"The database "ANSI_LWHITE"will%s"ANSI_LGREEN" be automatically dumped to disk while %s is running.",(dumping) ? "":" not",tcz_short_name);
       writelog(ADMIN_LOG,0,title,"%s(#%d) changed the database dumping option to %s.",getname(player),player,(dumping) ? "ON":"OFF");
    }
#else
    if(Validchar(player)) {
       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, automatic database dumping option is not supported.");
    } else writelog(OPTIONS_LOG,0,title,"Sorry, automatic database dumping option is not supported (#define DATABASE_DUMP)");
    (*error)++, (*critical)++;
#endif
    return(dumping);
}

/* ---->  {J.P.Boggis 27/02/2000}  E-mail forwarding domain name  <---- */
const char *option_emailforward(OPTCONTEXT)
{
      if(status) return(email_forward_name);

#ifdef EMAIL_FORWARDING
      if(!Blank(value)) {
         if(option_boolean(value,1)) {
	    if(strlen(value) <= 128) {
	       if(Blank(email_forward_name) || strcasecmp(value,email_forward_name))
		  writelog(OPTIONS_LOG,0,title,"E-mail forwarding domain name changed to '%s' (user.name@%s)",value,value);

               if(Validchar(player)) {
                  if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"E-mail forwarding domain name changed to '"ANSI_LWHITE"%s"ANSI_LGREEN"' ("ANSI_LYELLOW"user.name@%s"ANSI_LGREEN")",value,value);
                  writelog(ADMIN_LOG,0,title,"%s(#%d) changed the E-mail forwarding domain name option to '%s' (user.name@%s)",getname(player),player,value,value);
	       }

	       FREENULL((char *) email_forward_name);
	       email_forward_name = alloc_string(value);
	       return(email_forward_name);
	    } else writelog(OPTIONS_LOG,0,title,"Sorry, E-mail forwarding domain name is too long (Limit is 128 characters.)");
	 } else {
            writelog(OPTIONS_LOG,0,title,"E-mail forwarding disabled.");
            if(Validchar(player)) {
               if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"E-mail forwarding disabled.");
               writelog(ADMIN_LOG,0,title,"%s(#%d) disabled E-mail forwarding.",getname(player),player);
	    }

            FREENULL((char *) email_forward_name);
	    return(email_forward_name);
	 }
      } else if(Validchar(player)) {
         output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, E-mail forwarding domain name cannot be blank.");
      } else writelog(OPTIONS_LOG,0,title,"Sorry, E-mail forwarding domain name cannot be blank.");
#else
      if(Validchar(player)) {
         output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, E-mail forwarding domain name option is not supported.");
      } else writelog(OPTIONS_LOG,0,title,"Sorry, E-mail forwarding domain name option is not supported (#define EMAIL_FORWARDING)");
#endif

      (*error)++, (*critical)++;
      return(email_forward_name);
}

/* ---->  {J.P.Boggis 05/03/2000}  Emergency (Panic) database  <---- */
int option_emergency(OPTCONTEXT)
{
    static int panic = 0;
    int    new;

    if(status) return(panic);

    if(Validchar(player) && Blank(value)) {
       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the new setting for '"ANSI_LWHITE"Emergency database dump on crash"ANSI_LGREEN"'.");
       (*error)++, (*critical)++;
       return(panic);
    }

    new = option_boolean(value,1);
    if(new != panic) writelog(OPTIONS_LOG,0,title,"An emergency (Panic) database dump will%s be performed if %s crashes.",(new) ? "":" not",tcz_short_name);

    /* ---->  Handle SIGSEGV/SIGBUS  <---- */
    if(Validchar(player) && (new != panic)) {
       if(new) {
          signal(SIGBUS,server_signal_handler);
          signal(SIGSEGV,server_signal_handler);
       } else {
          signal(SIGBUS,SIG_DFL);
          signal(SIGSEGV,SIG_DFL);
       }
    }
    panic = new;

    if(Validchar(player)) {
       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"An emergency (Panic) database dump "ANSI_LWHITE"will%s"ANSI_LGREEN" be performed if %s crashes.",(new) ? "":" not",tcz_short_name);
       writelog(ADMIN_LOG,0,title,"%s(#%d) changed the emergency database dump option to %s.",getname(player),player,(new) ? "ON":"OFF");
    }
    return(panic);
}

/* ---->  {J.P.Boggis 05/03/2000}  Forked database dump  <---- */
int option_forkdump(OPTCONTEXT)
{
#ifdef DB_FORK
    static int forkdump = 1;
    int    new;
#else
    static int forkdump = 0;
#endif

    if(status) return(forkdump);

#ifdef DB_FORK
    if(Validchar(player) && Blank(value)) {
       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the new setting for '"ANSI_LWHITE"Forked database dump"ANSI_LGREEN"'.");
       (*error)++, (*critical)++;
       return(forkdump);
    }

    new = option_boolean(value,1);
    if(new != forkdump) writelog(OPTIONS_LOG,0,title,"The automatic database dump will%s be forked as a separate process.",(new) ? "":" not");

    /* ---->  Handle forked database dump child process signal  <---- */
    if(Validchar(player) && (new != forkdump)) {
       if(new) signal(SIGCHLD,server_SIGCHLD_handler);
          else signal(SIGCHLD,SIG_DFL);
    }
    forkdump = new;

    if(Validchar(player)) {
       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"The automatic database dump "ANSI_LWHITE"will%s"ANSI_LGREEN" be forked as a separate process.",(forkdump) ? "":" not");
       writelog(ADMIN_LOG,0,title,"%s(#%d) changed the forked database dump option to %s.",getname(player),player,(forkdump) ? "ON":"OFF");
    }
#else
    if(Validchar(player)) {
       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, forked database dump option is not supported.");
    } else writelog(OPTIONS_LOG,0,title,"Sorry, forked database dump option is not supported (#define DB_FORK)");
    (*error)++, (*critical)++;
#endif
    return(forkdump);
}

/* ---->  {J.P.Boggis 17/04/2000}  Full name of server  <---- */
const char *option_fullname(OPTCONTEXT)
{
      char buffer[TEXT_SIZE];

      if(status) return(tcz_full_name);

      if(!Blank(value)) {
         if(strlen(value) <= 64) {
            filter_spaces(buffer,value,1);
	    if(strcasecmp(buffer,tcz_full_name))
	       writelog(OPTIONS_LOG,0,title,"Server full name changed to '%s'.",buffer);

            if(Validchar(player)) {
               if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Server full name changed to '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",buffer);
               writelog(ADMIN_LOG,0,title,"%s(#%d) changed the server full name option to '%s'.",getname(player),player,buffer);
	    }

	    FREENULL(tcz_full_name);
	    tcz_full_name = alloc_string(buffer);
            return(tcz_full_name);
	 } else if(Validchar(player)) {
            output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, full name of server is too long (Limit is 64 characters.)");
	 } else writelog(OPTIONS_LOG,0,title,"Sorry, full name of server is too long (Limit is 64 characters.)");
      } else if(Validchar(player)) {
         output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, full name of server cannot be blank.");
      } else writelog(OPTIONS_LOG,0,title,"Sorry, full name of server cannot be blank.");

      (*error)++, (*critical)++;
      return(tcz_full_name);
}

/* ---->  {J.P.Boggis 26/07/2001}  Guardian alarm  <---- */
int option_guardian(OPTCONTEXT)
{
#ifdef GUARDIAN_ALARM
    static int guardian_alarm = 1;
    int    new;
#else
    static int guardian_alarm = 0;
#endif

    if(status) return(guardian_alarm);

#ifdef GUARDIAN_ALARM
    if(Validchar(player) && Blank(value)) {
       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the new setting for '"ANSI_LWHITE"Guardian alarm"ANSI_LGREEN"'.");
       (*error)++, (*critical)++;
       return(guardian_alarm);
    }

    new = option_boolean(value,1);
    if(new != guardian_alarm) writelog(OPTIONS_LOG,0,title,"Guardian alarm is %sabled.",(new) ? "en":"dis");
    guardian_alarm = new;

    if(Validchar(player)) {
       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Guardian alarm is "ANSI_LWHITE"%sabled"ANSI_LGREEN".",(new) ? "en":"dis");
       writelog(ADMIN_LOG,0,title,"%s(#%d) changed the guardian alarm option to %s.",getname(player),player,(new) ? "ON":"OFF");
    }
#else
    if(Validchar(player)) {
       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, guardian alarm option is not supported.");
    } else writelog(OPTIONS_LOG,0,title,"Sorry, guardian alarm option is not supported (#define GUARDIAN_ALARM)");
    (*error)++, (*critical)++;
#endif

    return(guardian_alarm);
}

/* ---->  {J.P.Boggis 27/02/2000}  Generate new database  <---- */
const char *option_generate(OPTCONTEXT)
{
      static const char *generate = NULL;
      static int newdb = 0;

      if(!generate) generate = alloc_string(NEWDATABASE);
      if(status) {
         if(!newdb) return(NULL);
         return(generate);
      }

      if(Blank(value)) value = NEWDATABASE;
      if(strlen(value) <= 64) {
         if(!generate || strcasecmp(value,generate))
            writelog(OPTIONS_LOG,0,title,"New database will be generated with the name '%s'.",value);
         FREENULL(generate);
         generate = alloc_string(value);
         newdb    = 1;
         return(generate);
      } else writelog(OPTIONS_LOG,0,title,"Sorry, new database name is too long (Limit is 64 characters.)");

      (*error)++, (*critical)++;
      return(generate);
}

/* ---->  {J.P.Boggis 28/02/2000}  HTML port  <---- */
int option_htmlport(OPTCONTEXT)
{
    if(status) return(htmlport);

#ifdef HTML_INTERFACE
    if(!Blank(value)) {
       if(isdigit(*value) || (*value == '-')) { 
	  int port = atoi(value);
	  if(port >= 0) {
	     if(port) {
		if(port != htmlport)
		   writelog(OPTIONS_LOG,0,title,"HTML port changed to %d.",port);
	     } else writelog(OPTIONS_LOG,0,title,"HTML port disabled.");
	     htmlport = port;
	     return(htmlport);
	  } else writelog(OPTIONS_LOG,0,title,"Sorry, HTML port cannot be negative.");
       } else writelog(OPTIONS_LOG,0,title,"Sorry, HTML port must be numeric.");
    } else writelog(OPTIONS_LOG,0,title,"Sorry, HTML port cannot be blank.");
#else
    if(Validchar(player)) {
       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, HTML port option is not supported.");
    } else writelog(OPTIONS_LOG,0,title,"Sorry, HTML port option is not supported (#define HTML_INTERFACE)");
#endif

    (*error)++, (*critical)++;
    return(htmlport);
}


extern struct html_image_data *html_images;


/* ---->  {J.P.Boggis 11/05/2000}  Serve HTML images internally  <---- */
int option_images(OPTCONTEXT)
{
    int new;

    if(status) return(html_internal_images);

#ifdef HTML_INTERFACE
    if(Validchar(player) && Blank(value)) {
       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the new setting for '"ANSI_LWHITE"Serve HTML images internally"ANSI_LGREEN"'.");
       (*error)++, (*critical)++;
       return(html_internal_images);
    }

    new = option_boolean(value,1);

    if((player == NOTHING) || !new || html_images) {
       if(new != html_internal_images) writelog(OPTIONS_LOG,0,title,"HTML Interface images will%s be served internally.",(new) ? "":" not");
       html_internal_images = new;

       if(Validchar(player)) {
          html_init_smileys();
          if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"HTML Interface images "ANSI_LWHITE"will%s"ANSI_LGREEN" be served internally.",(new) ? "":" not");
          writelog(ADMIN_LOG,0,title,"%s(#%d) changed the HTML images option to %s.",getname(player),player,(new) ? "ON":"OFF");
       }
    } else {
       if(Validchar(player)) {
          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, no HTML images have been loaded at start-up  -  Unable to serve HTML images internally.");
       } else writelog(OPTIONS_LOG,0,title,"Sorry, no HTML images have been loaded at start-up  -  Unable to serve HTML images internally.");
       (*error)++, (*critical)++;
    }
#else
    if(Validchar(player)) {
       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, HTML Interface images option is not supported.");
    } else writelog(OPTIONS_LOG,0,title,"Sorry, HTML Interface images option is not supported (#define HTML_INTERFACE)");
    (*error)++, (*critical)++;
#endif

    return(html_internal_images);
}

/* ---->  {J.P.Boggis 27/02/2000}  Run locally (127.0.0.1, localhost)  <---- */
int option_local(OPTCONTEXT)
{
    static int runlocal = 0;
    int    new;

    if(status) return(runlocal);

    new = option_boolean(value,1);
    if(new && (new != runlocal)) {

       /* ---->  Change local settings  <---- */
       runlocal = new;
       FREENULL(tcz_server_name);
       FREENULL(html_data_url);
       FREENULL(html_home_url);

       html_internal_images = 1;
       tcz_server_netmask   = IPADDR(255,0,0,0);
       tcz_server_network   = IPADDR(127,0,0,0);
       tcz_server_name      = alloc_string("localhost");
       tcz_server_ip        = IPADDR(127,0,0,1);
       html_data_url        = alloc_string("http://localhost/tczhtml/");
       html_home_url        = alloc_string("http://localhost/");

       /* ---->  Display settings  <---- */
       writelog(OPTIONS_LOG,0,title,"Server will%s run locally.",(runlocal) ? "":" not");
       writelog(OPTIONS_LOG,0,title,"Server name:  %s (%s)",tcz_server_name,ip_to_text(tcz_server_ip,SITEMASK,scratch_return_string));
       writelog(OPTIONS_LOG,0,title,"HTML Interface data URL:  %s",html_data_url);
       writelog(OPTIONS_LOG,0,title,"Web Site URL:  %s",html_home_url);
    }
    return(runlocal);
}

/* ---->  {J.P.Boggis 28/04/2000}  Location of server  <---- */
const char *option_location(OPTCONTEXT)
{
      char buffer[TEXT_SIZE];

      if(status) return(tcz_location);

      if(!Blank(value)) {
         if(strlen(value) <= 128) {
            filter_spaces(buffer,value,1);
	    if(strcasecmp(buffer,tcz_location))
	       writelog(OPTIONS_LOG,0,title,"Location of server changed to '%s'.",buffer);

            if(Validchar(player)) {
               if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Location of server changed to '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",buffer);
               writelog(ADMIN_LOG,0,title,"%s(#%d) changed the location of server option to '%s'.",getname(player),player,buffer);
	    }

	    FREENULL(tcz_location);
	    tcz_location = alloc_string(buffer);
            return(tcz_location);
	 } else if(Validchar(player)) {
            output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, location of server is too long (Limit is 128 characters.)");
	 } else writelog(OPTIONS_LOG,0,title,"Sorry, location of server is too long (Limit is 128 characters.)");
      } else if(Validchar(player)) {
         output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, location of server cannot be blank.");
      } else writelog(OPTIONS_LOG,0,title,"Sorry, location of server cannot be blank.");

      (*error)++, (*critical)++;
      return(tcz_location);
}

/* ---->  {J.P.Boggis 27/02/2000}  User logins  <---- */
int option_logins(OPTCONTEXT)
{
    static int logins = 1;
    int    new;

    if(status) return(logins);

    new = option_boolean(value,1);
    if(new != logins) writelog(OPTIONS_LOG,0,title,"User logins will%s be allowed.",(new) ? "":" not");
    logins = new;
    return(logins);
}

/* ---->  {J.P.Boggis 18/03/2000}  Logging level  <---- */
int option_loglevel(OPTCONTEXT)
{
    static int maxlevel = 6;
    static int loglevel = 2;
    int    showlevels   = 0;

    if(status) return(loglevel);

    if(!Blank(value)) {
       if(isdigit(*value) || (*value == '-')) { 
	  int level = atoi(value);
	  if(level >= 0) {
	      if(level <= maxlevel) {
		  if(level != loglevel)
		      writelog(OPTIONS_LOG,0,title,"Logging level changed to %d (%s)",level,loglevels[level]);

                if(Validchar(player)) {
                   if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Logging level changed to "ANSI_LWHITE"%d"ANSI_LGREEN" ("ANSI_LYELLOW"%s"ANSI_LGREEN")",level,loglevels[level]);
                   writelog(ADMIN_LOG,0,title,"%s(#%d) changed the logging level option to %d (%s.)",getname(player),player,level,loglevels[level]);
                   logfile_close();
		}

		loglevel = level;
                if(Validchar(player)) logfile_open(0);
		return(loglevel);
	     } else if(Validchar(player)) {
                output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, logging level cannot be greater than %d.",maxlevel), showlevels = 1;
             } else writelog(OPTIONS_LOG,0,title,"Sorry, logging level cannot be greater than %d.",maxlevel), showlevels = 1;
	  } else if(Validchar(player)) {
             output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, logging level cannot be negative."), showlevels = 1;
	  } else writelog(OPTIONS_LOG,0,title,"Sorry, logging level cannot be negative."), showlevels = 1;
       } else if(Validchar(player)) {
          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, logging level must be numeric."), showlevels = 1;
       } else writelog(OPTIONS_LOG,0,title,"Sorry, logging level must be numeric."), showlevels = 1;
    } else showlevels = 1;

    /* ---->  Show available log levels and description of each  <---- */
    if(showlevels) {
       int loop;

       if(Validchar(player)) {
          struct descriptor_data *p = getdsc(player);

          tilde_string(player,"Available logging levels:",ANSI_LCYAN,ANSI_DCYAN,0,1,5);
          for(loop = 0; loop <= maxlevel; loop++)
              output(p,player,0,1,4,ANSI_LYELLOW"%d "ANSI_DCYAN"- "ANSI_LWHITE"%s",loop,loglevels[loop]);
          output(p,player,0,1,0,"\n"ANSI_LCYAN"NOTE:  \x05\x07"ANSI_LWHITE"Logging levels "ANSI_LYELLOW"2 "ANSI_LWHITE"- "ANSI_LYELLOW"%d"ANSI_LWHITE" inherit all the options of the logging levels below them (E.g:  Logging level "ANSI_LCYAN"4"ANSI_LWHITE" inherits the options of "ANSI_LCYAN"2"ANSI_LWHITE" and "ANSI_LCYAN"3"ANSI_LWHITE".)\n",maxlevel);
       } else {
          fputs("\nAvailable logging levels:\n~~~~~~~~~~~~~~~~~~~~~~~~~\n",stderr);
          for(loop = 0; loop <= maxlevel; loop++)
              fprintf(stderr,"%d - %s\n",loop,loglevels[loop]);
          fprintf(stderr,"\nNOTE:  Logging levels 2 - %d inherit all the options of the logging levels\n       below them (E.g:  Logging level 4 inherits the options of 2 and 3.)\n\n",maxlevel);
       }
    }

    (*error)++, (*critical)++;
    return(loglevel);
}

/* ---->  {J.P.Boggis 11/05/2000}  MOTD login message  <---- */
const char *option_motd(OPTCONTEXT)
{
      static const char *motd = NULL;
      char   subst[BUFFER_LEN];
      char   buffer[TEXT_SIZE];
      char   *result;

      if(status) return(motd);

      if(strlen(value) <= TEXT_SIZE) {
	 filter_spaces(buffer,value,0);
	 result = punctuate(buffer,2,'.');

	 if(!motd || strcasecmp(result,motd)) {
            if(!Blank(result)) writelog(OPTIONS_LOG,0,title,"MOTD login message changed to '%s'.",result);
               else writelog(OPTIONS_LOG,0,title,"MOTD login message reset.");
	 }

	 if(Validchar(player)) {
	    if(!in_command) {
               if(!Blank(result)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"MOTD login message changed to:\n\n"ANSI_LWHITE"%s\n",substitute(player,subst,result,0,ANSI_LWHITE,NULL,0));
                  else output(getdsc(player),player,0,1,0,ANSI_LGREEN"MOTD login message reset.");
	    }

            if(!Blank(result))
	       writelog(ADMIN_LOG,0,title,"%s(#%d) changed the MOTD login message option to '%s'.",getname(player),player,result);
                  else writelog(ADMIN_LOG,0,title,"%s(#%d) reset the MOTD login message option.",getname(player),player);
	 }

	 FREENULL(motd);
	 motd = alloc_string(result);
	 return(motd);
      } else if(Validchar(player)) {
	 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, MOTD login message is too long (Limit is %d characters.)",TEXT_SIZE);
      } else writelog(OPTIONS_LOG,0,title,"Sorry, MOTD login message is too long (Limit is %d characters.)",TEXT_SIZE);

      (*error)++, (*critical)++;
      return(motd);
}

/* ---->  {J.P.Boggis 05/03/2000}  Nice level  <---- */
int option_nice(OPTCONTEXT)
{
    static int priority = PRIORITY;

    if(status) return(priority);

#ifdef RESTRICT_MEMORY
    if(!Blank(value)) {
       if(isdigit(*value) || (*value == '-')) { 
	  int newpriority = atoi(value);

          if(newpriority != priority)
      	     writelog(OPTIONS_LOG,0,title,"Nice level changed to %d.",newpriority);
	  priority = newpriority;
	  return(priority);
       } else writelog(OPTIONS_LOG,0,title,"Sorry, nice level must be numeric.");
    } else writelog(OPTIONS_LOG,0,title,"Sorry, nice level cannot be blank.");
    (*error)++, (*critical)++;
#else 
    writelog(OPTIONS_LOG,0,title,"Sorry, nice level option is not supported (#define RESTRICT_MEMORY)");
    (*error)++, (*critical)++;
#endif
    return(priority);
}

/* ---->  {J.P.Boggis 27/02/2000}  Change path  <---- */
const char *option_path(OPTCONTEXT)
{
      static char workdir[BUFFER_LEN];

      getcwd(workdir,BUFFER_LEN - 1);
      if(status) return(workdir);

      if(!Blank(value)) {
          if(!chdir(value))
             writelog(OPTIONS_LOG,0,title,"Working directory path changed to '%s'.",value);
	        else writelog(OPTIONS_LOG,0,title,"Unable to change working directory path to '%s' (%s.)",value,strerror(errno));
          getcwd(workdir,BUFFER_LEN - 1);
          return(workdir);
      } else writelog(OPTIONS_LOG,0,title,"Sorry, working directory path cannot be blank.");

      (*error)++, (*critical)++;
      return(workdir);
}

/* ---->  {J.P.Boggis 28/02/2000}  Quit after processing options  <---- */
int option_quit(OPTCONTEXT)
{
    static int option_quit = 0;

    if(status) return(option_quit);

    option_quit = option_boolean(value,1);
    return(option_quit);
}

/* ---->  {J.P.Boggis 28/02/2000}  Server address  <---- */
const char *option_server(OPTCONTEXT)
{
      if(status) return(tcz_server_name);

      if(!Blank(value)) {
         if(strlen(value) <= 128) {
	    if(strcasecmp(value,tcz_server_name))
	       writelog(OPTIONS_LOG,0,title,"Server address changed to '%s'.",value);

            if(Validchar(player)) {
               if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Server address changed to '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",value);
               writelog(ADMIN_LOG,0,title,"%s(#%d) changed the server address option to '%s'.",getname(player),player,value);
	    }

	    FREENULL(tcz_server_name);
	    tcz_server_name = alloc_string(value);
            return(tcz_server_name);
	 } else if(Validchar(player)) {
            output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, server address is too long (Limit is 128 characters.)");
	 } else writelog(OPTIONS_LOG,0,title,"Sorry, server address is too long (Limit is 128 characters.)");
      } else if(Validchar(player)) {
         output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, server address cannot be blank.");
      } else writelog(OPTIONS_LOG,0,title,"Sorry, server address cannot be blank.");

      (*error)++, (*critical)++;
      return(tcz_server_name);
}

/* ---->  {J.P.Boggis 27/02/2000}  Automatic lookup of server info  <---- */
int option_serverinfo(OPTCONTEXT)
{
#ifdef SERVERINFO
    static int useserverinfo = 1;
    int    new;
#else
    static int useserverinfo = 0;
#endif

    if(status) return(useserverinfo);

#ifdef SERVERINFO
    new = option_boolean(value,1);
    if(new != useserverinfo) writelog(OPTIONS_LOG,0,title,"Server information will%s be automatically looked up.",(new) ? "":" not");
    useserverinfo = new;
#else
    writelog(OPTIONS_LOG,0,title,"Sorry, server information option is not supported (#define SERVERINFO)");
    (*error)++, (*critical)++;
#endif
    return(useserverinfo);
}

/* ---->  {J.P.Boggis 17/04/2000}  Short abbreviated name of server  <---- */
const char *option_shortname(OPTCONTEXT)
{
      char buffer[TEXT_SIZE];

      if(status) return(tcz_short_name);

      if(!Blank(value)) {
         if(strlen(value) <= 32) {
            filter_spaces(buffer,value,1);
	    if(strcasecmp(buffer,tcz_short_name))
	       writelog(OPTIONS_LOG,0,title,"Server short abbreviated name changed to '%s'.",buffer);

            if(Validchar(player)) {
               if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Server short abbreviated name changed to '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",buffer);
               writelog(ADMIN_LOG,0,title,"%s(#%d) changed the server short abbreviated name to '%s'.",getname(player),player,buffer);
	    }

            /* ---->  Change short abbreviated name of server  <---- */ 
	    FREENULL(tcz_short_name);
	    tcz_short_name = alloc_string(buffer);

            /* ---->  Change default prompt  <---- */
            sprintf(buffer,TELNET_TCZ_PROMPT,tcz_short_name);
            tcz_prompt = alloc_string(buffer);
            return(tcz_short_name);
	 } else if(Validchar(player)) {
            output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, short abbreviated name of server is too long (Limit is 32 characters.)");
	 } else writelog(OPTIONS_LOG,0,title,"Sorry, short abbreviated name of server is too long (Limit is 32 characters.)");
      } else if(Validchar(player)) {
         output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, short abbreviated name of server cannot be blank.");
      } else writelog(OPTIONS_LOG,0,title,"Sorry, short abbreviated name of server cannot be blank.");

      (*error)++, (*critical)++;
      return(tcz_short_name);
}

/* ---->  {J.P.Boggis 28/02/2000}  Immediate shutdown  <---- */
int option_shutdown(OPTCONTEXT)
{
    static int ishutdown = 0;
    int    new;

    if(status) return(ishutdown);

    new = option_boolean(value,1);
    if(new != ishutdown) writelog(OPTIONS_LOG,0,title,"The server will%s shutdown immediately after start-up.",(new) ? "":" not");
    ishutdown = new;
    return(ishutdown);
}

/* ---->  {J.P.Boggis 26/03/2000}  SSL (Secure Sockets Layer) connections  <---- */
int option_ssl(OPTCONTEXT)
{
#ifdef SSL_SOCKETS
    static int ssl = 1;
    int    new;
#else
    static int ssl = 0;
#endif

    if(status) return(ssl);

#ifdef SSL_SOCKETS
    new = option_boolean(value,1);
    if(new != ssl) writelog(OPTIONS_LOG,0,title,"SSL (Secure Sockets Layer) connections will%s be allowed.",(new) ? "":" not");
    ssl = new;
#else
    writelog(OPTIONS_LOG,0,title,"Sorry, SSL (Secure Sockets Layer) connections option is not supported (#define SSL)");
    (*error)++, (*critical)++;
#endif
    return(ssl);
}

/* ---->  {J.P.Boggis 28/02/2000}  Telnet port  <---- */
int option_telnetport(OPTCONTEXT)
{
    if(status) return(telnetport);

    if(!Blank(value)) {
       if(isdigit(*value) || (*value == '-')) { 
	  int port = atoi(value);
	  if(port >= 0) {
	     if(port) {
		if(port != telnetport)
		   writelog(OPTIONS_LOG,0,title,"Telnet port changed to %d.",port);
	     } else writelog(OPTIONS_LOG,0,title,"Telnet port disabled.");
	     telnetport = port;
	     return(telnetport);
	  } else writelog(OPTIONS_LOG,0,title,"Sorry, telnet port cannot be negative.");
       } else writelog(OPTIONS_LOG,0,title,"Sorry, telnet port must be numeric.");
    } else writelog(OPTIONS_LOG,0,title,"Sorry, telnet port cannot be blank.");

    (*error)++, (*critical)++;
    return(telnetport);
}

/* ---->  {J.P.Boggis 12/03/2000}  User log files  <---- */
int option_userlogs(OPTCONTEXT)
{
#ifdef USER_LOG_FILES
    static int userlogs = 1;
    int    new;
#else
    static int userlogs = 0;
#endif

    if(status) return(userlogs);

#ifdef USER_LOG_FILES
    if(Validchar(player) && Blank(value)) {
       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the new setting for '"ANSI_LWHITE"User log files"ANSI_LGREEN"'.");
       (*error)++, (*critical)++;
       return(userlogs);
    }

    new = option_boolean(value,1);

    if(new != userlogs) writelog(OPTIONS_LOG,0,title,"User log files are %sabled.",(new) ? "en":"dis");
    userlogs = new;

    if(Validchar(player)) {
       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"User log files are "ANSI_LWHITE"%sabled"ANSI_LGREEN".",(new) ? "en":"dis");
       writelog(ADMIN_LOG,0,title,"%s(#%d) changed the user log files option to %s.",getname(player),player,(new) ? "ON":"OFF");
    }
#else
    if(Validchar(player)) {
       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, user log files option is not supported.");
    } else writelog(OPTIONS_LOG,0,title,"Sorry, user log files option is not supported (#define USER_LOG_FILES)");
    (*error)++, (*critical)++;
#endif
    return(userlogs);
}

/* ---->  {J.P.Boggis 28/02/2000}  Web Site URL  <---- */
const char *option_website(OPTCONTEXT)
{
      if(status) return(html_home_url);

      if(!Blank(value)) {
         if(strlen(value) <= 512) {
	    if(strcasecmp(value,html_home_url))
	       writelog(OPTIONS_LOG,0,title,"Web site URL changed to '%s'.",value);

            if(Validchar(player)) {
               if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Web site URL changed to '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",value);
               writelog(ADMIN_LOG,0,title,"%s(#%d) changed the web site URL option to '%s'.",getname(player),player,value);
	    }

	    FREENULL(html_home_url);
	    html_home_url = alloc_string(value);
            return(html_home_url);
	 } else if(Validchar(player)) {
            output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, web site URL is too long (Limit is 512 characters.)");
	 } else writelog(OPTIONS_LOG,0,title,"Sorry, web site URL is too long (Limit is 512 characters.)");
      } else if(Validchar(player)) {
         output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, web site URL cannot be blank.");
      } else writelog(OPTIONS_LOG,0,title,"Sorry, web site URL cannot be blank.");

      (*error)++, (*critical)++;
      return(html_home_url);
}

/* ---->  {J.P.Boggis 28/02/2000}  Display current options  <---- */
void option_options(struct descriptor_data *d,unsigned char options)
{
     int twidth = output_terminal_width((d) ? d->player:NOTHING);

     if(d) {
        if(options) {
           if(IsHtml(d)) {
              html_anti_reverse(d,1);
	      output(d,d->player,1,2,0,"<BR><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">");
	      output(d,d->player,1,2,0,"<TR BGCOLOR="HTML_TABLE_CYAN"><TH COLSPAN=2><FONT SIZE=4 COLOR="HTML_LCYAN"><I>Current %s Server Options (Set at start-up):</I></FONT></TH></TR>",tcz_short_name);
	   } else {
              if(Validchar(d->player) && !in_command && d && !d->pager && !IsHtml(d) && More(d->player)) pager_init(d);
	      output(d,d->player,0,1,0,ANSI_LCYAN"\n Current %s Server Options (Set at start-up):",tcz_short_name);
	      output(d,d->player,0,1,0,separator(twidth,0,'-','='));
	   }
	} else return;
     } else fputs("\n          Current Option Settings:\n          ~~~~~~~~~~~~~~~~~~~~~~~~\n",stderr);

     if(d) output(d,d->player,2,1,37,"%s"ANSI_LYELLOW""ANSI_UNDERLINE"Admin. "ANSI_LGREEN"E-mail address:%s"ANSI_LWHITE"%s%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"             ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_adminemail(OPTSTATUS),IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"            Admin. E-mail address (E):  %s\n",option_adminemail(OPTSTATUS));

#ifdef SERVERINFO
     if(d) output(d,d->player,2,1,37,"%s"ANSI_LGREEN"Auto lookup server information:%s"ANSI_LWHITE"%sabled.%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"    ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_serverinfo(OPTSTATUS) ? "En":"Dis",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"   Auto lookup server information (I):  %sabled.\n",option_serverinfo(OPTSTATUS) ? "En":"Dis");
#endif

#ifdef DATABASE_DUMP
     if(d) output(d,d->player,2,1,37,"%s"ANSI_LGREEN"Automatically "ANSI_LYELLOW""ANSI_UNDERLINE"dump"ANSI_LGREEN" database:%s"ANSI_LWHITE"%sabled.%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"       ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_dumping(OPTSTATUS) ? "En":"Dis",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"      Automatically dump database (A):  %sabled.\n",option_dumping(OPTSTATUS) ? "En":"Dis");
#endif

     if(d) output(d,d->player,2,1,37,"%s"ANSI_LGREEN"Backdoor password:%s"ANSI_LWHITE"%sabled.%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"                 ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_backdoor(OPTSTATUS) ? "En":"Dis",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"                Backdoor password (B):  %sabled.\n",option_backdoor(OPTSTATUS) ? "En":"Dis");

#ifdef DB_COMPRESSION
     if(d) output(d,d->player,2,1,37,"%s"ANSI_LGREEN"Compress database on disk:%s"ANSI_LWHITE"%sabled.%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"         ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_compress_disk(OPTSTATUS) ? "En":"Dis",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"        Compress database on disk (Z):  %sabled.\n",option_compress_disk(OPTSTATUS) ? "En":"Dis");
#endif

#ifdef EXTRA_COMPRESSION
     if(d) output(d,d->player,2,1,37,"%s"ANSI_LGREEN"Compress database in memory:%s"ANSI_LWHITE"%sabled.%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"       ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_compress_memory(OPTSTATUS) ? "En":"Dis",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"      Compress database in memory (z):  %sabled.\n",option_compress_memory(OPTSTATUS) ? "En":"Dis");
#endif

     if(d) output(d,d->player,2,1,37,"%s"ANSI_LYELLOW""ANSI_UNDERLINE"Core"ANSI_LGREEN" dump on crash:%s"ANSI_LWHITE"%sabled.%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"                ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_coredump(OPTSTATUS) ? "En":"Dis",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"               Core dump on crash (K):  %sabled.\n",option_coredump(OPTSTATUS) ? "En":"Dis");

     if(d) output(d,d->player,2,1,37,"%s"ANSI_LGREEN"Database name:%s"ANSI_LWHITE"%s%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"                     ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_database(OPTSTATUS),IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"                    Database name (d):  %s\n",option_database(OPTSTATUS));

     if(d) output(d,d->player,2,1,37,"%s"ANSI_LYELLOW""ANSI_UNDERLINE"DNS"ANSI_LGREEN" lookups:%s"ANSI_LWHITE"%sabled.%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"                       ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_dns(OPTSTATUS) ? "En":"Dis",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"                      DNS lookups (n):  %sabled.\n",option_dns(OPTSTATUS) ? "En":"Dis");

     if(d) output(d,d->player,2,1,37,"%s"ANSI_LGREEN" Display log entries on "ANSI_LYELLOW""ANSI_UNDERLINE"console"ANSI_LGREEN":%s"ANSI_LWHITE"%sabled.%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"   ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_console(OPTSTATUS) ? "En":"Dis",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"   Display log entries on console (V):  %sabled.\n",option_console(OPTSTATUS) ? "En":"Dis");

     if(d) output(d,d->player,2,1,37,"%s"ANSI_LYELLOW""ANSI_UNDERLINE"Emergency"ANSI_LGREEN" database dump on crash:%s"ANSI_LWHITE"%sabled.%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"  ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_emergency(OPTSTATUS) ? "En":"Dis",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr," Emergency database dump on crash (P):  %sabled.\n",option_emergency(OPTSTATUS) ? "En":"Dis");

#ifdef EMAIL_FORWARDING
     if(d) output(d,d->player,2,1,37,"%s"ANSI_LGREEN"E-mail "ANSI_LYELLOW""ANSI_UNDERLINE"forwarding"ANSI_LGREEN" domain name:%s"ANSI_LWHITE"%s%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"     ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_emailforward(OPTSTATUS) ? option_emailforward(OPTSTATUS):"Disabled.",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"    E-mail forwarding domain name (e):  %s\n",option_emailforward(OPTSTATUS) ? option_emailforward(OPTSTATUS):"Disabled.");
#endif

     if(d) output(d,d->player,2,1,37,"%s"ANSI_LGREEN"Forced database consistency check:%s"ANSI_LWHITE"%sabled.%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":" ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_check(OPTSTATUS) ? "En":"Dis",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"Forced database consistency check (C):  %sabled.\n",option_check(OPTSTATUS) ? "En":"Dis");

#ifdef DB_FORK
     if(d) output(d,d->player,2,1,37,"%s"ANSI_LYELLOW""ANSI_UNDERLINE"Forked"ANSI_LGREEN" database dump:%s"ANSI_LWHITE"%sabled.%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"              ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_forkdump(OPTSTATUS) ? "En":"Dis",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"             Forked database dump (F):  %sabled.\n",option_forkdump(OPTSTATUS) ? "En":"Dis");
#endif

     if(d) output(d,d->player,2,1,37,"%s"ANSI_LYELLOW""ANSI_UNDERLINE"Full"ANSI_LGREEN" name of server:%s"ANSI_LWHITE"%s%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"               ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_fullname(OPTSTATUS),IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"              Full name of server (y):  %s\n",option_fullname(OPTSTATUS));

#ifdef GUARDIAN_ALARM
     if(d) output(d,d->player,2,1,37,"%s"ANSI_LYELLOW""ANSI_UNDERLINE"Guardian"ANSI_LGREEN" alarm:%s"ANSI_LWHITE"%sabled.%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"                    ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_guardian(OPTSTATUS) ? "En":"Dis",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"                   Guardian alarm (x):  %sabled.\n",option_guardian(OPTSTATUS) ? "En":"Dis");
#endif

     if(!d) {
        fprintf(stderr,"            Generate new database (g):  %sabled.\n",option_generate(OPTSTATUS) ? "En":"Dis");
        if(option_generate(OPTSTATUS))
           fprintf(stderr,"          Generated database name (g):  %s\n",option_generate(OPTSTATUS));
     }

#ifdef HTML_INTERFACE
     if(d) output(d,d->player,2,1,37,"%s"ANSI_LGREEN"HTML Interface "ANSI_LYELLOW""ANSI_UNDERLINE"data"ANSI_LGREEN" URL:%s"ANSI_LWHITE"%s%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"           ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_dataurl(OPTSTATUS),IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"          HTML Interface data URL (D):  %s\n",option_dataurl(OPTSTATUS));
#endif

#ifdef HTML_INTERFACE
     if(d) output(d,d->player,2,1,37,"%s"ANSI_LGREEN"HTML port:%s"ANSI_LWHITE"%d.%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"                         ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_htmlport(OPTSTATUS),IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"                        HTML port (w):  %d.\n",option_htmlport(OPTSTATUS));
#endif

     if(!d) fprintf(stderr,"Immediate shutdown after start-up (X):  %sabled.\n",option_shutdown(OPTSTATUS) ? "En":"Dis");

     if(d) output(d,d->player,2,1,37,"%s"ANSI_LYELLOW""ANSI_UNDERLINE"Location"ANSI_LGREEN" of server:%s"ANSI_LWHITE"%s%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"                ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_location(OPTSTATUS),IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"               Location of server (H):  %s\n",option_location(OPTSTATUS));

     if(d) output(d,d->player,2,1,37,"%s"ANSI_LYELLOW""ANSI_UNDERLINE"Logging"ANSI_LGREEN" level:%s"ANSI_LWHITE"%d \016&nbsp;\016 (%s)%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"                     ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_loglevel(OPTSTATUS),loglevels[option_loglevel(OPTSTATUS)],IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"                    Logging level (L):  %d (%s)\n",option_loglevel(OPTSTATUS),loglevels[option_loglevel(OPTSTATUS)]);

     if(d) output(d,d->player,2,1,37,"%s"ANSI_LYELLOW""ANSI_UNDERLINE"MOTD"ANSI_LGREEN" login message:%s"ANSI_LWHITE"%s%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"                ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",!Blank(option_motd(OPTSTATUS)) ? option_motd(OPTSTATUS):"Disabled.",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"               MOTD login message (m):  %s\n",!Blank(option_motd(OPTSTATUS)) ? option_motd(OPTSTATUS):"Disabled.");

     if(!d) fprintf(stderr,"    Quit after processing options (Q):  %sabled.\n",option_quit(OPTSTATUS) ? "En":"Dis");

     if(d) output(d,d->player,2,1,37,"%s"ANSI_LGREEN"Run at nice level:%s"ANSI_LWHITE"%d.%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"                 ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_nice(OPTSTATUS),IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"                Run at nice level (N):  %d.\n",option_nice(OPTSTATUS));

     if(d) output(d,d->player,2,1,37,"%s"ANSI_LGREEN"Run in "ANSI_LYELLOW""ANSI_UNDERLINE"debug"ANSI_LGREEN"ging/developer mode:%s"ANSI_LWHITE"%sabled.%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"   ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_debug(OPTSTATUS) ? "En":"Dis",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"  Run in debugging/developer mode (G):  %sabled.\n",option_debug(OPTSTATUS) ? "En":"Dis");

     if(d) output(d,d->player,2,1,37,"%s"ANSI_LGREEN"Run locally:%s"ANSI_LWHITE"%sabled.%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"                       ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_local(OPTSTATUS) ? "En":"Dis",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"                      Run locally (l):  %sabled.\n",option_local(OPTSTATUS) ? "En":"Dis");

#ifdef HTML_INTERFACE
     if(d) output(d,d->player,2,1,37,"%s"ANSI_LGREEN"Serve HTML images "ANSI_LYELLOW""ANSI_UNDERLINE"internal"ANSI_LGREEN"ly:%s"ANSI_LWHITE"%sabled.%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"      ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_images(OPTSTATUS) ? "En":"Dis",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"     Serve HTML images internally (i):  %sabled.\n",option_images(OPTSTATUS) ? "En":"Dis");
#endif

     if(d) output(d,d->player,2,1,37,"%s"ANSI_LYELLOW""ANSI_UNDERLINE"Server"ANSI_LGREEN" address:%s"ANSI_LWHITE"%s%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"                    ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_server(OPTSTATUS),IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"                   Server address (s):  %s\n",option_server(OPTSTATUS));

     if(d) output(d,d->player,2,1,37,"%s"ANSI_LYELLOW""ANSI_UNDERLINE"Short"ANSI_LGREEN" name of server:%s"ANSI_LWHITE"%s%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"              ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_shortname(OPTSTATUS),IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"             Short name of server (Y):  %s\n",option_shortname(OPTSTATUS));

#ifdef SSL_SOCKETS
     if(d) output(d,d->player,2,1,37,"%s"ANSI_LGREEN"SSL (Secure Sockets Layer):%s"ANSI_LWHITE"%sabled.%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"        ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_ssl(OPTSTATUS) ? "En":"Dis",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"       SSL (Secure Sockets Layer) (S):  %sabled.\n",option_ssl(OPTSTATUS) ? "En":"Dis");
#endif

     if(d) output(d,d->player,2,1,37,"%s"ANSI_LGREEN"Telnet port:%s"ANSI_LWHITE"%d.%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"                       ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_telnetport(OPTSTATUS),IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"                      Telnet port (t):  %d.\n",option_telnetport(OPTSTATUS));

     if(d) output(d,d->player,2,1,37,"%s"ANSI_LGREEN"User logins:%s"ANSI_LWHITE"%sabled.%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"                       ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_logins(OPTSTATUS) ? "En":"Dis",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"                      User logins (u):  %sabled.\n",option_logins(OPTSTATUS) ? "En":"Dis");

#ifdef USER_LOG_FILES
     if(d) output(d,d->player,2,1,37,"%s"ANSI_LYELLOW""ANSI_UNDERLINE"User"ANSI_LGREEN" log files:%s"ANSI_LWHITE"%sabled.%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"                    ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_userlogs(OPTSTATUS) ? "En":"Dis",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"                   User log files (U):  %sabled.\n",option_userlogs(OPTSTATUS) ? "En":"Dis");
#endif

     if(d) output(d,d->player,2,1,37,"%s"ANSI_LGREEN"Web "ANSI_LYELLOW""ANSI_UNDERLINE"Site"ANSI_LGREEN" URL:%s"ANSI_LWHITE"%s%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"                      ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_website(OPTSTATUS),IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"                     Web Site URL (W):  %s\n",option_website(OPTSTATUS));

     if(d) output(d,d->player,2,1,37,"%s"ANSI_LGREEN"Working directory path:%s"ANSI_LWHITE"%s%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN">\016":"            ",IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016":"  ",option_path(OPTSTATUS),IsHtml(d) ? "\016</TD></TR>\016":"\n");
        else fprintf(stderr,"           Working directory path (p):  %s\n",option_path(OPTSTATUS));
 
     if(d) {
        if(!IsHtml(d)) output(d,d->player,0,1,0,separator(twidth,0,'-','-'));
        if(d) output(d,d->player,2,1,1,"%s"ANSI_LWHITE"To change the settings of the above server options, type '"ANSI_LGREEN"@admin options <OPTION> = <SETTING>"ANSI_LWHITE"' \016&nbsp;\016 ("ANSI_LCYAN"NOTE: \016&nbsp;\016 "ANSI_LWHITE"Some options cannot be changed.)%s",IsHtml(d) ? "\016<TR><TH COLSPAN=2 ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><I>\016":" ",IsHtml(d) ? "\016</I></TH></TR>\016":"\n");
     }

     if(d) {
        if(IsHtml(d)) {
           output(d,d->player,1,2,0,"</TABLE><BR>");
           html_anti_reverse(d,0);
	} else output(d,d->player,0,1,0,separator(twidth,1,'-','='));
        setreturn(OK,COMMAND_SUCC);
     } else fputs("\n",stderr);
}

/* ---->  Get list of options into dynamic array  <---- */
void option_get_list(int argc,char **argv)
{
     struct option_list_data *tail = NULL,*new;
     int    index = 0;
     char   opt;

     opterr = 0;
     while((opt = getopt_long(argc,argv,"aA::b:c:C::d:D:e:E:F::g:G::hH:i::I::kK::l::L:m:n::N:oOp:P::Q::s:S::t:u::U::vV::w:W:y:Y:x::X::z::Z::",options,&index)) != -1) {
           MALLOC(new,struct option_list_data);
           new->next    = NULL;
           new->opt     = opt;
           new->optarg  = alloc_string(optarg);
           new->longopt = alloc_string(argv[optind - ((opt == '?') ? 1:0)]);

           if(option_list) {
              tail->next = new;
              tail       = new;
	   } else option_list = tail = new;
     }
}

/* ---->  Free dynamic array of options  <---- */
void option_free_list(void)
{
     struct option_list_data *next;

     if(!option_list) return;
     for(; option_list; option_list = next) {
         next = option_list->next;
         FREENULL(option_list->longopt);
         FREENULL(option_list->optarg);
         FREENULL(option_list);
     }
}

/* ---->  {J.P.Boggis 19/03/2000}  Check command line options  <---- */
int option_option(dbref player,struct option_list_data *opt,const char *title,const char *pname,int *error,int *critical,int instance)
{
    switch(opt->opt) {
	   case 'a':

		/* ---->  Display advanced options  <---- */
		if(instance == OPTION_ADMINOPTIONS) {
                   if(Validchar(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the list of advanced start-up options cannot be displayed using '"ANSI_LYELLOW"@admin options"ANSI_LGREEN"'.");
		} else if(instance == OPTION_CONFIGFILE)
                   writelog(OPTIONS_LOG,0,title,"Sorry, the list of advanced start-up options cannot be displayed from within a configuration file.");
		break;
	   case 'A':

		/* ---->  Automatic database dumping  <---- */
		option_dumping(player,title,opt->optarg,0,error,critical);
		break;
	   case 'b':

		/* ---->  Backdoor password  <---- */
		if(instance == OPTION_ADMINOPTIONS) {
                   if(Validchar(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the backdoor password cannot be changed using '"ANSI_LYELLOW"@admin options"ANSI_LGREEN"'.");
		} else option_backdoor(player,title,opt->optarg,0,error,critical);
		break;
	   case 'c':

		/* ---->  Config file (Dealt with earlier by option_configfile())  <---- */
		if(instance == OPTION_ADMINOPTIONS) {
                   if(Validchar(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, configuration files cannot be processed using '"ANSI_LYELLOW"@admin options"ANSI_LGREEN"'.");
		} else if(instance == OPTION_CONFIGFILE)
                   writelog(OPTIONS_LOG,0,title,"Sorry, configuration files cannot be processed from within a configuration file.");
		break;
	   case 'C':

		/* ---->  Force full database consistency check  <---- */
		if(instance == OPTION_ADMINOPTIONS) {
                   if(Validchar(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a full database consistency check cannot be forced using '"ANSI_LYELLOW"@admin options"ANSI_LGREEN"' (Use '"ANSI_LYELLOW"@sanity"ANSI_LGREEN"' instead.)");
		} else option_check(player,title,opt->optarg,0,error,critical);
		break;
	   case 'd':

		/* ---->  Database name  <---- */
		if(instance == OPTION_ADMINOPTIONS) {
                   if(Validchar(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the database name cannot be changed using '"ANSI_LYELLOW"@admin options"ANSI_LGREEN"'.");
		} else option_database(player,title,opt->optarg,0,error,critical);
		break;
	   case 'D':

		/* ---->  HTML Interface data URL  <---- */
		option_dataurl(player,title,opt->optarg,0,error,critical);
		break;
	   case 'e':

		/* ---->  E-mail forwarding domain name  <---- */
		option_emailforward(player,title,opt->optarg,0,error,critical);
		break;
	   case 'E':

		/* ---->  Admin. E-mail address  <---- */
		option_adminemail(player,title,opt->optarg,0,error,critical);
		break;
	   case 'F':

		/* ---->  Forked database dump  <---- */
		option_forkdump(player,title,opt->optarg,0,error,critical);
		break;
	   case 'g':

		/* ---->  Generate new database  <---- */
		if(instance == OPTION_ADMINOPTIONS) {
                   if(Validchar(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a new database cannot be generated using '"ANSI_LYELLOW"@admin options"ANSI_LGREEN"'.");
		} else option_generate(player,title,opt->optarg,0,error,critical);
		break;
	   case 'G':

		/* ---->  Run in debugging/developer mode  <---- */
		option_debug(player,title,opt->optarg,0,error,critical);
		break;
	   case 'h':

		/* ---->  Display help  <---- */
		if(instance == OPTION_ADMINOPTIONS) {
                   if(Validchar(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, start-up options help cannot be displayed using '"ANSI_LYELLOW"@admin options"ANSI_LGREEN"'.");
		} else if(instance == OPTION_CONFIGFILE)
                   writelog(OPTIONS_LOG,0,title,"Sorry, start-up options help cannot be displayed from within a configuration file.");
		break;
	   case 'H':

		/* ---->  Location of server  <---- */
		option_location(player,title,opt->optarg,0,error,critical);
		break;
	   case 'i':

		/* ---->  Serve HTML images internally  <---- */
		option_images(player,title,opt->optarg,0,error,critical);
		break;
	   case 'I':

		/* ---->  Automatically lookup server information  <---- */
		if(instance == OPTION_ADMINOPTIONS) {
                   if(Validchar(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, automatic lookup of server information cannot be changed using '"ANSI_LYELLOW"@admin options"ANSI_LGREEN"'.");
		} else option_serverinfo(player,title,opt->optarg,0,error,critical);
		break;
	   case 'k':

		/* ---->  Signal handling behaviour  <---- */
		if(instance == OPTION_ADMINOPTIONS) {
                   if(Validchar(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the list of signals handled by %s cannot be displayed using '"ANSI_LYELLOW"@admin options"ANSI_LGREEN"'.",tcz_short_name);
		} else if(instance == OPTION_CONFIGFILE)
                   writelog(OPTIONS_LOG,0,title,"Sorry, the list of signals cannot be displayed from within a configuration file.");		
		break;
	   case 'l':

		/* ---->  Run locally (127.0.0.1, localhost)  <---- */
		if(instance == OPTION_ADMINOPTIONS) {
                   if(Validchar(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %s server cannot be ran locally using '"ANSI_LYELLOW"@admin options"ANSI_LGREEN"'.",tcz_short_name);
		} else option_local(player,title,opt->optarg,0,error,critical);
		break;
	   case 'L':

		/* ---->  Logging level  <---- */
		option_loglevel(player,title,opt->optarg,0,error,critical);
		break;
	   case 'K':

		/* ---->  Core dump  <---- */
		option_coredump(player,title,opt->optarg,0,error,critical);
		break;
	   case 'm':

		/* ---->  MOTD login message  <---- */
		option_motd(player,title,opt->optarg,0,error,critical);
		break;
	   case 'n':

		/* ---->  DNS lookups  <---- */
		option_dns(player,title,opt->optarg,0,error,critical);
		break;
	   case 'N':

		/* ---->  Nice level  <---- */
		if(instance == OPTION_ADMINOPTIONS) {
                   if(Validchar(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %s server nice level cannot be changed using '"ANSI_LYELLOW"@admin options"ANSI_LGREEN"'.",tcz_short_name);
		} else option_nice(player,title,opt->optarg,0,error,critical);
		break;
	   case 'o':

		/* ---->  Display start-up options  <---- */
		if(instance == OPTION_ADMINOPTIONS) {
                   if(Validchar(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please use '"ANSI_LYELLOW"@admin options"ANSI_LGREEN"' without any other parameters to display the current server option settings.");
		} else if(instance == OPTION_CONFIGFILE)
                   writelog(OPTIONS_LOG,0,title,"Sorry, the start-up options cannot be displayed from within a configuration file.");
		break;
	   case 'O':

		/* ---->  Display current options  <---- */
		if(instance == OPTION_ADMINOPTIONS) {
                   if(Validchar(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please use '"ANSI_LYELLOW"@admin options"ANSI_LGREEN"' without any other parameters to display the current server option settings.");
		} else option_options(NULL,0);
		break;
	   case 'p':

		/* ---->  Change working directory path  <---- */
		if(instance == OPTION_ADMINOPTIONS) {
                   if(Validchar(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the working directory path cannot be changed using '"ANSI_LYELLOW"@admin options"ANSI_LGREEN"'.");
		} else if(instance == OPTION_CONFIGFILE)
                   writelog(OPTIONS_LOG,0,title,"Sorry, the working directory path cannot be changed from within a configuration file (Please use the '-p'/'--path' command-line option.)");
		break;
	   case 'P':

		/* ---->  Emergency database dump  <---- */
		option_emergency(player,title,opt->optarg,0,error,critical);
		break;
	   case 'Q':

		/* ---->  Quit after processing options  <---- */
		if(instance == OPTION_ADMINOPTIONS) {
                   if(Validchar(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, exit after processing start-up options cannot be changed using '"ANSI_LYELLOW"@admin options"ANSI_LGREEN"'.");
		} else option_quit(player,title,opt->optarg,0,error,critical);
		break;
	   case 's':

		/* ---->  Server address  <---- */
		option_server(player,title,opt->optarg,0,error,critical);
		break;
	   case 'S':

		/* ---->  SSL (Secure Sockets Layer)  <---- */
		if(instance == OPTION_ADMINOPTIONS) {
                   if(Validchar(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, SSL (Secure Sockets Layer) connections cannot be changed using '"ANSI_LYELLOW"@admin options"ANSI_LGREEN"'.");
		} else option_ssl(player,title,opt->optarg,0,error,critical);
		break;
	   case 't':

		/* ---->  Telnet port  <---- */
		if(instance == OPTION_ADMINOPTIONS) {
                   if(Validchar(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the Telnet port cannot be changed using '"ANSI_LYELLOW"@admin options"ANSI_LGREEN"'.");
		} else option_telnetport(player,title,opt->optarg,0,error,critical);
		break;
	   case 'u':

		/* ---->  User logins  <---- */
		if(instance == OPTION_ADMINOPTIONS) {
                   if(Validchar(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, user logins cannot be changed using '"ANSI_LYELLOW"@admin options"ANSI_LGREEN"' (Use '"ANSI_LYELLOW"@admin connections/creations"ANSI_LGREEN"' instead.)");
		} else option_logins(player,title,opt->optarg,0,error,critical);
		break;
	   case 'U':

		/* ---->  User log files  <---- */
		option_userlogs(player,title,opt->optarg,0,error,critical);
		break;
           case 'v':

		/* ---->  Display version information  <---- */
		if(instance == OPTION_ADMINOPTIONS) {
                   if(Validchar(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s version information cannot be displayed using '"ANSI_LYELLOW"@admin options"ANSI_LGREEN"' (Type '"ANSI_LWHITE"version"ANSI_LGREEN"' to view this information.)",tcz_short_name);
		} else if(instance == OPTION_CONFIGFILE)
                   writelog(OPTIONS_LOG,0,title,"Sorry, %s version information cannot be displayed from within a configuration file.",tcz_short_name);
		break;
	   case 'V':

		/* ---->  Display log file entries on console  <---- */
		option_console(player,title,opt->optarg,0,error,critical);
		break;
	   case 'w':

		/* ---->  HTML port  <---- */
		if(instance == OPTION_ADMINOPTIONS) {
                   if(Validchar(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the HTML Interface port cannot be changed using '"ANSI_LYELLOW"@admin options"ANSI_LGREEN"'.");
		} else option_htmlport(player,title,opt->optarg,0,error,critical);
		break;
	   case 'W':

		/* ---->  Web site URL  <---- */
		option_website(player,title,opt->optarg,0,error,critical);
		break;
	   case 'x':

		/* ---->  Guardian alarm  <---- */
		option_guardian(player,title,opt->optarg,0,error,critical);
	        break;
	   case 'X':

		/* ---->  Immediate shutdown (After start-up)  <---- */
		if(instance == OPTION_ADMINOPTIONS) {
                   if(Validchar(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, immediate %s server shutdown cannot be changed using '"ANSI_LYELLOW"@admin options"ANSI_LGREEN"' (Use '"ANSI_LYELLOW"@shutdown"ANSI_LGREEN"' to shutdown %s.)",tcz_short_name,tcz_short_name);
		} else option_shutdown(player,title,opt->optarg,0,error,critical);
		break;
	   case 'y':

		/* ---->  Full name of server  <---- */
		option_fullname(player,title,opt->optarg,0,error,critical);
		break;
	   case 'Y':

		/* ---->  Short abbreviated name of server  <---- */
		option_shortname(player,title,opt->optarg,0,error,critical);
		break;
	   case 'z':

		/* ---->  Disable database compression in memory  <---- */
		if(instance == OPTION_ADMINOPTIONS) {
                   if(Validchar(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, database memory compression cannot be changed using '"ANSI_LYELLOW"@admin options"ANSI_LGREEN"'.");
		} else option_compress_memory(player,title,opt->optarg,0,error,critical);
		break;
	   case 'Z':

		/* ---->  Disable database compression on disk  <---- */
		if(instance == OPTION_ADMINOPTIONS) {
                   if(Validchar(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, database disk compression cannot be changed using '"ANSI_LYELLOW"@admin options"ANSI_LGREEN"'.");
		} else option_compress_disk(player,title,opt->optarg,0,error,critical);
		break;
	   default:

		/* ---->  Unknown/invalid option  <---- */
		(*error)++, (*critical)++;
		if(instance == OPTION_ADMINOPTIONS) {
                   if(Validchar(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, unknown server option '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Type '"ANSI_LYELLOW"@admin options"ANSI_LGREEN"' for a list.)",opt->longopt);
		} else if(instance == OPTION_COMMANDLINE)
  		   fprintf(stderr,"%s:  Unknown/invalid option '%s' or required parameter omitted (Type '%s --help' for help.)\n",title,opt->longopt,pname);
		break;
    }
    return(!(*critical));
}

/* ---->  {J.P.Boggis 01/05/2000}  Lookup option by name and optionally execute handler code from option_option()   <---- */
int option_match(dbref player,const char *title,const char *option,const char *value,const char *pname,int handler,int instance)
{
    struct option_list_data opt;
    int    error,critical;
    int    count,result;

    /* ---->  Match option in options list  <---- */
    if(!Blank(option)) {
       for(count = 0; options[count].name && strcasecmp(option,options[count].name); count++);
       if(options[count].name) {
	  if(handler) {
	     opt.longopt = options[count].name;
	     opt.optarg  = (value) ? value:"";
	     opt.next    = NULL;
	     opt.opt     = options[count].val;

	     result = option_option(player,&opt,title,pname,&error,&critical,instance);
	     return(result);
	  }
	  return(1);
       }
    }

    /* ---->  Matching option not found  <---- */
    if(Validchar(player))
       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, unknown server option '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Type '"ANSI_LYELLOW"@admin options"ANSI_LGREEN"' for a list.)",option);
    return(0);
}

/* ---->  {J.P.Boggis 27/02/2000}  Get options from config file  <---- */
int option_config(const char *filename,const char *pname,int defaultconfig)
{
    char  buffer[BUFFER_LEN],fname[TEXT_SIZE];
    char  *ptr,*ptr2,*option,*value;
    const char *fptr;
    FILE  *config;
    int   comment;

    if(!Blank(filename)) {

       /* ---->  Generate filename (Automatically add path and extension if neccessary)  <---- */
       for(fptr = filename + strlen(filename); (fptr >= filename) && (*fptr != '.'); fptr--);
       snprintf(fname,TEXT_SIZE,"%s%s%s",strchr(filename,'/') ? "":"lib/",filename,((fptr >= filename) && (*fptr == '.')) ? "":".tcz");

       /* ---->  Open configuration file  <---- */
       if((config = fopen(fname,"r"))) {
	  writelog(OPTIONS_LOG,1,"OPTIONS","Processing options from %sconfiguration file '%s'...",(defaultconfig) ? "default ":"",fname);
	  while(fgets(buffer,BUFFER_LEN,config)) {
		option = NULL; value = NULL;

		/* ---->  Strip non-printable characters from buffer  <---- */
		for(ptr = ptr2 = buffer; *ptr; ptr++)
		    if(isprint(*ptr)) *ptr2++ = *ptr;
		*ptr2 = '\0';

		/* ---->  Get option and value  <---- */
		for(ptr = buffer; *ptr && (*ptr == ' '); ptr++);
		if(*ptr && (*ptr != '#')) {

		   /* ---->  Get option (Before ':')  <---- */
		   for(option = ptr; *ptr && (*ptr != ':') && (*ptr != '#'); ptr++);
                   comment = (*ptr == '#');
		   for(ptr2 = ptr++, *ptr2-- = '\0'; (ptr2 >= option) && (*ptr2 == ' '); *ptr2-- = '\0');

		   /* ---->  Get value (After ':')  <---- */
                   if(!comment) {
   		      for(; *ptr && (*ptr == ' '); ptr++);
		      if(*ptr != '#') {
		         for(value = ptr; *ptr && (*ptr != '#') && (*ptr != '\n'); ptr++);
		         for(ptr2 = ptr, *ptr2-- = '\0'; (ptr2 >= value) && (*ptr2 == ' '); *ptr2-- = '\0');
		      }
		   }

		   /* ---->  Match option and execute handler code  <---- */
		   if(!Blank(option))
		      option_match(NOTHING,"CONFIG",option,value,pname,1,OPTION_CONFIGFILE);
		}
	  }
	  fclose(config);
          return(1);
       } else writelog(OPTIONS_LOG,1,"OPTIONS","Unable to open the %sconfiguration file '%s' for reading (%s.)",(defaultconfig) ? "default ":"",fname,strerror(errno));
    } else writelog(OPTIONS_LOG,1,"OPTIONS","Sorry, no configuration filename specified.");
    return(0);
}

/* ---->  {J.P.Boggis 19/03/2000}  Check command-line arguments for config file  <---- */
int option_configfile(const char *pname)
{
    struct option_list_data *ptr;
    const  char *pnameptr;

    /* ---->  Process arguments (Look for -c/--configfile)  <---- */
    opterr = 0;
    if(!Blank(pname) && (pnameptr = (char *) strrchr(pname,'/')))
       pname = pnameptr + 1;

    for(ptr = option_list; ptr; ptr = ptr->next)
        if(ptr->opt == 'c') {

 	   /* ---->  Config file  <---- */
	   if(!Blank(ptr->optarg)) {
	      if(strlen(ptr->optarg) <= 256) {
	         return(option_config(ptr->optarg,pname,0));
	      } else writelog(OPTIONS_LOG,1,"OPTIONS","Sorry, configuration filename is too long (Limit is 256 characters.)");
	   } else writelog(OPTIONS_LOG,1,"OPTIONS","Please specify the name of a configuration file.");
	}

    /* ---->  Check for existence of 'lib/default.tcz' config file  <---- */
    return(option_config(CONFIG_FILE,pname,1));
}

/* ---->  {J.P.Boggis 19/03/2000}  Check command-line arguments for alternative working directory path  <---- */
int option_changepath(void)
{
    struct option_list_data *ptr;
    int  critical = 0;  /*  Critical errors  */
    int  error    = 0;  /*  Total errors     */

    /* ---->  Process arguments  <---- */
    opterr = 0;
    for(ptr = option_list; ptr; ptr = ptr->next)
        if(ptr->opt == 'p') {

           /* ---->  Working directory path  <---- */
           option_path(NOTHING,"OPTIONS",ptr->optarg,0,&error,&critical);
           if(!error && !critical) return(1);
	}

    return(0);
}

/* ---->  {J.P.Boggis 07/05/2000}  Display help options/version information  <---- */
int option_helpoptions(const char *pname)
{
    struct option_list_data *ptr;

    /* ---->  Process arguments  <---- */
    opterr = 0;
    for(ptr = option_list; ptr; ptr = ptr->next)
        switch(ptr->opt) {
               case 'a':

		    /* ---->  Display advanced options  <---- */
		    fprintf(stderr,"\nUSAGE:  %s [<OPTIONS>]\n\n",pname);
		    fputs("-a  --advanced           Display advanced options again.\n",stderr);
#ifdef DATABASE_DUMP
		    fputs("-A  --dump          (B)  Automatically dump database at regular intervals.\n",stderr);
#endif
		    fputs("-b  --backdoor      (T)  Backdoor password (USE WITH CARE.)\n",stderr);
		    fputs("-C  --check         (B)  Force full database consistency check.\n",stderr);
#ifdef HTML_INTERFACE
		    fputs("-D  --dataurl       (T)  HTML Interface data URL.\n",stderr);
#endif
#ifdef EMAIL_FORWARDING
		    fputs("-e  --email       (T/B)  E-mail forwarding domain name ('Off' to disable.)\n",stderr);
#endif
		    fputs("-E  --admin         (T)  Admin. E-mail address.\n",stderr);
#ifdef DB_FORK
		    fputs("-F  --fork          (B)  Forked database dump.\n",stderr);
#endif
		    fputs("-G  --debug         (B)  Run in debugging/developer mode.\n",stderr);
		    fputs("-H  --location      (T)  Location of server.\n",stderr);
#ifdef HTML_INTERFACE
		    fputs("-i  --images        (B)  Serve HTML Interface images internally.\n",stderr);
#endif
#ifdef SERVERINFO
		    fputs("-I  --serverinfo    (B)  Automatically lookup server information at start-up.\n",stderr);
#endif
		    fputs("-K  --core          (B)  Produce core dump on crash.\n",stderr);
		    fputs("-L  --loglevel      (N)  Logging level ('-Llist' to list.)\n",stderr);
		    fputs("-n  --DNS           (B)  DNS lookups.\n",stderr);
		    fputs("-N  --nice          (N)  Run at nice level.\n",stderr);
		    fputs("-o  --options            Display standard options.\n",stderr);
		    fputs("-O  --current            Display current settings for all options.\n",stderr);
		    fputs("-P  --panic         (B)  Emergency database dump on crash.\n",stderr);
		    fputs("-Q  --quit          (B)  Exit after processing options.\n",stderr);
#ifdef SSL_SOCKETS
		    fputs("-S  --SSL           (B)  SSL (Secure Sockets Layer) connections.\n",stderr);
#endif
#ifdef USER_LOG_FILES
		    fputs("-U  --userlogs      (B)  User log files.\n",stderr);
#endif
		    fputs("-v  --version            Display TCZ version number and exit.\n",stderr);
		    fputs("-V  --console       (B)  Display log file entries on console (stderr.)\n",stderr);
		    fputs("-W  --website       (T)  Web site URL\n",stderr);
		    fputs("-X  --shutdown      (B)  Immediate shutdown after start-up.\n",stderr);
		    fputs("-y  --fullname      (T)  Full name of server.\n",stderr);
		    fputs("-Y  --shortname     (T)  Abbreviated short name of server.\n",stderr);
#ifdef GUARDIAN_ALARM
		    fputs("-x  --guardian      (B)  Guardian alarm.\n",stderr);
#endif
#ifdef EXTRA_COMPRESSION
		    fputs("-z  --compressmem   (B)  Database compression in memory.\n",stderr);
#endif
#ifdef DB_COMPRESSION
		    fputs("-Z  --compressdisk  (B)  Database compression on disk.\n",stderr);
#endif
		    fputs("\n",stderr);
		    return(1);
                    break;
	       case 'h':

		    /* ---->  Display help  <---- */
		    fprintf(stderr,"\nUSAGE:  %s [<OPTIONS>]\n\n",pname);
		    fputs("-o  --options    Display start-up options.\n",stderr);
		    fputs("-a  --advanced   Display advanced start-up options.\n",stderr);
		    fputs("-k  --signals    Display signal handling behaviour.\n",stderr);
		    fputs("\n(T)  Text parameter required, e.g:  -o\"Option Text\" --option \"Option Text\"\n     (\"\" can be omitted if text does not contain spaces.)\n",stderr);
		    fputs("(N)  Numeric parameter required, e.g:  -o1234 --option 1234\n",stderr);
		    fputs("(B)  Boolean parameter (Yes/No), e.g:  -oYes --option=yes\n",stderr);
		    fputs("\n",stderr);
		    return(1);
                    break;
	       case 'k':

		    /* ---->  Signal handling behaviour  <---- */
		    fputs("\nSignal:   Behaviour:\n~~~~~~~   ~~~~~~~~~~\n",stderr);
		    fputs("SIGTERM   Shutdown and dump database to disk.\n",stderr);
		    fputs("SIGKILL   Immediate shutdown and exit with no database dump.\n\n",stderr);
		    fputs("SIGCONT   Leave idle state.\n",stderr);
		    fputs("SIGUSR1   Start/restart database dump.\n",stderr);
		    fputs("SIGUSR2   Enter idle state.\n",stderr);
		    fputs("SIGPWR    Power failure/resumed.\n\n",stderr);
		    fprintf(stderr,"USAGE:  kill -<SIGNAL> <%s PID>     E.g:  kill -SIGUSR2 1234\n\n",tcz_short_name);
                    return(1);
                    break;
	       case 'o':

		    /* ---->  Display start-up options  <---- */
		    fprintf(stderr,"\nUSAGE:  %s [<OPTIONS>]\n\n",pname);
		    fputs("-a  --advanced           Display advanced options.\n",stderr);
		    fputs("-c  --config        (T)  Load options from configuration file.\n",stderr);
		    fputs("-d  --database      (T)  Database name.\n",stderr);
		    fputs("-g  --generate      (T)  Generate new empty database.\n",stderr);
		    fputs("-h  --help               Display command-line options help.\n",stderr);
		    fputs("-k  --signals            Display signal handling behaviour.\n",stderr);
		    fputs("-l  --local         (B)  Run locally (127.0.0.1, localhost)\n",stderr);
		    fputs("-m  --motd          (T)  Message to display at login (MOTD.)\n",stderr);
		    fputs("-o  --options            Display standard options again.\n",stderr);
		    fputs("-p  --path          (T)  Full path to server working directory.\n",stderr);
		    fputs("-t  --telnet        (N)  Telnet port.\n",stderr);
		    fputs("-s  --server        (T)  Server address.\n",stderr);
		    fputs("-u  --logins        (B)  User logins.\n",stderr);
		    fputs("-v  --version            Display TCZ version number and exit.\n",stderr);
#ifdef HTML_INTERFACE
		    fputs("-w  --html          (N)  HTML (Web Interface) port.\n",stderr);
#endif
		    fprintf(stderr,"\nDEFAULT OPTIONS:  -t%d -w%d -d%s\n",TELNETPORT,HTMLPORT,DATABASE);
		    fputs("\n",stderr);
                    return(1);
                    break;
	       case 'v':

                    /* ---->  Version Information  <---- */
                    tcz_version(NULL,1);
                    return(1);
                    break;
               default:
                    break;
	}

    return(0);
}

/* ---->  {J.P.Boggis 24/12/1999}  Parse command-line arguments  <---- */
int option_main(const char *pname)
{
    struct option_list_data *ptr;
    int   critical = 0;  /*  Critical errors  */
    int   error    = 0;  /*  Total errors     */
    const char *pnameptr;

    opterr = 0;
    if(!Blank(pname) && (pnameptr = (char *) strrchr(pname,'/')))
       pname = pnameptr + 1;

    /* ---->  Process arguments  <---- */
    for(ptr = option_list; ptr; ptr = ptr->next)
        option_option(NOTHING,ptr,"OPTIONS",pname,&error,&critical,OPTION_COMMANDLINE);

    /* ---->  Invalid argument(s) specified  <---- */
    if(error && critical) {
       option_free_list();
       logfile_close();
       exit(1);
    }

    /* ---->  Exit ('Q' option)  <---- */
    if(option_quit(OPTSTATUS)) {
       writelog(OPTIONS_LOG,1,"OPTIONS","Exiting...  ('Q' option.)");
       option_free_list();
       logfile_close();
       exit(0);
    }
    return(0);
}
