/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| TCZ.C  -  The Chatting Zone (TCZ) Server:  Implements initialisation,       |
|           start-up/automatic restart and shutdown.                          |
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
| Module originally designed and written by:  J.P.Boggis 21/12/1993.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: tcz.c,v 1.4 2005/06/29 20:29:47 tcz_monster Exp $

*/


#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "object_types.h"
#include "prompts.h"
#include "flagset.h"
#include "search.h"
#include "match.h"


extern char scratch_return_string[];
extern char scratch_buffer[];

extern int telnet;
extern int html;


/* ---->  {J.P.Boggis}  Display version information  <---- */
void tcz_version(struct descriptor_data *d,int console)
{
     char buffer2[BUFFER_LEN];
     char buffer[BUFFER_LEN];

     if(!console && d) {

        /* ---->  HTML header  <---- */
        if(IsHtml(d)) {
           html_anti_reverse(d,1);
           output(d,d->player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
	} else if(!in_command && d->player != NOTHING && !d->pager && More(d->player)) pager_init(d);

        /* ---->  TCZ version header  <---- */
        if(!IsHtml(d)) output(d,d->player,0,1,0,"");
        sprintf(buffer,"%s"ANSI_LCYAN"%s (TCZ v"TCZ_VERSION")  -  (C) J.P.Boggis 1993 - %d.%s",IsHtml(d) ? "\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER COLSPAN=2><FONT SIZE=5><B><I>\016":"",tcz_full_name,tcz_year,IsHtml(d) ? "\016</I></B></FONT></TH></TR>\016":"");
        if(!IsHtml(d)) {
           if(!Validchar(d->player))
              output(d,d->player,0,1,0,"%s\n%s",buffer,strpad('~',strlen(buffer) - strlen(ANSI_LCYAN),buffer2));
                 else tilde_string(d->player,buffer,"",ANSI_DCYAN,0,0,5);
	} else output(d,d->player,2,1,0,"%s",buffer);

        output(d,d->player,2,1,0,"%s%s%s",IsHtml(d) ? "\016<TR><TD ALIGN=LEFT COLSPAN=2>\016":"",substitute(d->player,buffer,ANSI_LWHITE"TCZ is free software, which is distributed under %c%lversion 2%x of the %c%lGNU General Public License%x (See '%g%l%<gpl%>%x' in TCZ, or visit %b%l%u%{@?link \"\" \"http://www.gnu.org\" \"Visit the GNU web site...\"}%x)  For more information about the %y%lTCZ%x, please visit:  %b%l%u%{@?link \"\" \"http://www.sourceforge.net/projects/tcz\" \"Visit the TCZ project web site...\"}%x",0,ANSI_LWHITE,NULL,0),IsHtml(d) ? "\016</TD></TR>\016":"\n\n\n");
     }

     /* ---->  Description of TCZ  <---- */
     if(!console && d) {
        sprintf(buffer,"%s"ANSI_LGREEN"Description%s",IsHtml(d) ? "\016<TR BGCOLOR="HTML_TABLE_GREEN"><TH ALIGN=CENTER COLSPAN=2><FONT SIZE=5><B><I>\016":"",IsHtml(d) ? "\016</I></B></FONT></TH></TR>\016":"");
        if(!IsHtml(d)) {
           if(!Validchar(d->player))
              output(d,d->player,0,1,0,"%s\n%s",buffer,strpad('~',strlen(buffer) - strlen(ANSI_LGREEN),buffer2));
                 else tilde_string(d->player,buffer,"",ANSI_DGREEN,0,0,5);
	} else output(d,d->player,2,1,0,"%s",buffer);

        if(IsHtml(d)) output(d,d->player,1,1,0,"<TR><TD ALIGN=LEFT COLSPAN=2>");
        output(d,d->player,0,1,0,ANSI_LYELLOW"The Chatting Zone"ANSI_LWHITE" ("ANSI_LYELLOW"TCZ"ANSI_LWHITE") is a user-friendly, advanced multi-user environment for "ANSI_LCYAN"social"ANSI_LWHITE" ("ANSI_LCYAN"Chat"ANSI_LWHITE") or "ANSI_LCYAN"gaming"ANSI_LWHITE" ("ANSI_LCYAN"Adventure"ANSI_LWHITE") purposes either privately over any network supporting TCP/IP, or publicly over the Internet.  It supports both "ANSI_LYELLOW"Telnet"ANSI_LWHITE" ("ANSI_LCYAN"Text-only"ANSI_LWHITE") and "ANSI_LYELLOW"HTML"ANSI_LWHITE" ("ANSI_LCYAN"World Wide Web"ANSI_LWHITE") connections.\n");
	
        output(d,d->player,0,1,0,ANSI_LYELLOW"TCZ"ANSI_LMAGENTA" is based on "ANSI_LYELLOW"TinyMUD "ANSI_LMAGENTA"("ANSI_LWHITE"1989"ANSI_LMAGENTA") and "ANSI_LYELLOW"UglyMUG"ANSI_LMAGENTA" ("ANSI_LWHITE"1990"ANSI_LMAGENTA"-"ANSI_LWHITE"1991"ANSI_LMAGENTA".)  It was designed and developed by "ANSI_LYELLOW"J.P.Boggis"ANSI_LMAGENTA" from "ANSI_LWHITE"21/12/1993"ANSI_LMAGENTA" before release under the GPL license on "ANSI_LWHITE"02/12/2004"ANSI_LMAGENTA".\n");
	
        output(d,d->player,2,1,0,ANSI_LCYAN"Please read the file "ANSI_LWHITE"MODULES"ANSI_LCYAN" or type '"ANSI_LYELLOW"\016<A HREF=\"%s\" TARGET=_blank TITLE=\"Click to view list of modules...\">\016modules\016</A>\016"ANSI_LCYAN"' on TCZ for detailed author information.%s",html_server_url(d,0,0,"modules"),IsHtml(d) ? "":"\n\n");
	
#ifdef DEMO
        output(d,d->player,0,1,0,ANSI_LRED"This is a "ANSI_LYELLOW""ANSI_UNDERLINE"demonstration"ANSI_LRED" version of TCZ.\n");
#endif

        if(IsHtml(d)) output(d,d->player,1,1,0,"</TD></TR>");
           else output(d,d->player,0,1,0,"");
     } else {
        fputs("\nDescription:\n~~~~~~~~~~~~\n",stderr);
	
        fputs("The Chatting Zone (TCZ) is a user-friendly, advanced multi-user\nenvironment for social (Chat) or gaming (Adventure) purposes\neither privately over any network supporting TCP/IP, or\npublicly over the Internet.  It supports both Telnet\n(Text-only) and HTML (World Wide Web) connections.\n\n",stderr);
	
        fputs("TCZ is based on TinyMUD (1989) and UglyMUG (1990-1991.)  It was\ndesigned and developed by J.P.Boggis from 21/12/1993 before\nrelease under the GPL license on 02/12/2004.\n\n",stderr);
	
        fputs("Please see the file MODULES or type 'modules' on TCZ for\ndetailed author information.\n\n",stderr);
        fputs("For more information about TCZ, please visit:\n\n   http://www.sourceforge.net/projects/tcz\n\n\n",stderr);
     }

     /* ---->  Version Information  <---- */
     if(!console && d) {
        sprintf(buffer,"%s"ANSI_LMAGENTA"Version Information%s",IsHtml(d) ? "\016<TR BGCOLOR="HTML_TABLE_MAGENTA"><TH ALIGN=CENTER COLSPAN=2><FONT SIZE=5><B><I>\016":"",IsHtml(d) ? "\016</I></B></FONT></TH></TR>\016":"");
        if(!IsHtml(d)) {
           if(!Validchar(d->player))
              output(d,d->player,0,1,0,"%s\n%s",buffer,strpad('~',strlen(buffer) - strlen(ANSI_LMAGENTA),buffer2));
                 else tilde_string(d->player,buffer,"",ANSI_DMAGENTA,0,0,5);
	} else output(d,d->player,2,1,0,"%s",buffer);

	query_version(NOTHING,NULL,NULL,NULL,NULL,0,0);
        output(d,d->player,2,1,14,IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=15%% BGCOLOR="HTML_TABLE_YELLOW">"ANSI_LYELLOW"Version:&nbsp;</TH><TD ALIGN=LEFT>&nbsp;"ANSI_LWHITE"\016%s\016</TD></TR>\016":ANSI_LYELLOW"    Version:  "ANSI_LWHITE"%s\n",command_result);
        output(d,d->player,2,1,14,IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=15%% BGCOLOR="HTML_TABLE_YELLOW">"ANSI_LYELLOW"CVS:&nbsp;</TH><TD ALIGN=LEFT>&nbsp;"ANSI_LWHITE"\016%s\016</TD></TR>\016":ANSI_LYELLOW"        CVS:  "ANSI_LWHITE"%s\n\n",TCZ_CVS_ID);
        output(d,d->player,2,1,14,IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=15%% BGCOLOR="HTML_TABLE_RED">"ANSI_LRED"Code Base:&nbsp;</TH><TD ALIGN=LEFT>&nbsp;"ANSI_LWHITE"\016%s\016</TD></TR>\016":ANSI_LRED"  Code Base:  "ANSI_LWHITE"%s\n",punctuate(CODEBASE,3,'.'));
        output(d,d->player,2,1,14,IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=15%% BGCOLOR="HTML_TABLE_RED">"ANSI_LRED"Description:&nbsp;</TH><TD ALIGN=LEFT>&nbsp;"ANSI_LWHITE"\016%s\016</TD></TR>\016":ANSI_LRED"Description:  "ANSI_LWHITE"%s\n",punctuate(CODEDESC,3,'.'));
        output(d,d->player,2,1,14,IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=15%% BGCOLOR="HTML_TABLE_RED">"ANSI_LRED"Web Site:&nbsp;</TH><TD ALIGN=LEFT>&nbsp;<A HREF=\"%s\" TARGET=_blank TITLE=\"Click to visit the official web site of this code base...\">\016%s\016</A></TD></TR>\016":ANSI_LRED"   Web Site:  "ANSI_LBLUE""ANSI_UNDERLINE"%s%s\n",IsHtml(d) ? html_encode_basic(CODESITE,buffer,NULL,512):"",CODESITE);
        output(d,d->player,2,1,14,IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=15%% BGCOLOR="HTML_TABLE_RED">"ANSI_LRED"Code Name:&nbsp;</TH><TD ALIGN=LEFT>&nbsp;"ANSI_LWHITE"\016%s\016</TD></TR>\016":ANSI_LRED"  Code Name:  "ANSI_LWHITE"%s\n",punctuate(CODENAME,3,'.'));
        if(!IsHtml(d)) output(d,d->player,0,1,0,"\n");
     } else {
	fputs("Version Information:\n~~~~~~~~~~~~~~~~~~~~\n",stderr);
	query_version(NOTHING,NULL,NULL,NULL,NULL,0,0);
	fprintf(stderr,"    Version:  %s\n",command_result);
	fprintf(stderr,"        CVS:  %s\n\n",TCZ_CVS_ID);
	fprintf(stderr,"  Code Base:  %s\n",punctuate(CODEBASE,3,'.'));
	fprintf(stderr,"Description:  %s\n",punctuate(CODEDESC,3,'.'));
	fprintf(stderr,"   Web Site:  %s\n",CODESITE);
	fprintf(stderr,"  Code Name:  %s\n\n",punctuate(CODENAME,3,'.'));
     }

     /* ---->  Associated help page links  <---- */
     if(!console && d) {
        sprintf(buffer,"%s"ANSI_LBLUE"References%s",IsHtml(d) ? "\016<TR BGCOLOR="HTML_TABLE_BLUE"><TH ALIGN=CENTER COLSPAN=2><FONT SIZE=5><B><I>\016":"",IsHtml(d) ? "\016</I></B></FONT></TH></TR>\016":"");
        if(!IsHtml(d)) {
           if(!Validchar(d->player))
              output(d,d->player,0,1,0,"%s\n%s",buffer,strpad('~',strlen(buffer) - strlen(ANSI_LMAGENTA),buffer2));
                 else tilde_string(d->player,buffer,"",ANSI_DBLUE,0,0,5);
	} else output(d,d->player,2,1,0,"%s",buffer);

        if(IsHtml(d)) output(d,d->player,1,1,0,"<TR><TD ALIGN=LEFT COLSPAN=2>");
	strcpy(buffer,"%[%9%3%c%lAlso, see:  %g%l%(disclaimer%)             %c-  %xTerms and conditions for using %@%m.\n" \
	       "            %g%l%(help rules%)             %c-  %xThe official %@%m rules.\n" \
	       "            %g%l%(help tcz%)               %c-  %xInformation about TCZ.\n" \
	       "            %g%l%(help history%)           %c-  %xThe history of TCZ.\n" \
	       "            %g%l%(help version info%)      %c-  %xTCZ version information.\n" \
	       "            %g%l%(help acknowledgements%)  %c-  %xCredits and acknowledgements.\n" \
	       "            %g%l%(help bug fixes%)         %c-  %xLatest source code bug fixes.\n" \
	       "            %g%l%(modules%)                %c-  %xSource code module information.\n" \
	       "            %g%l%(authors%)                %c-  %xModule author information.\n" \
	       "            %g%l%(help gpl%)               %c-  %xThe GNU General Public License.");

        strcat(buffer,"%]");
        output(d,d->player,2,1,0,"%s%s",substitute(d->player,buffer2,buffer,0,ANSI_LWHITE,NULL,0),IsHtml(d) ? "":"\n\n");
        if(IsHtml(d)) output(d,d->player,1,1,0,"</TD></TR>");

        if(IsHtml(d)) {
           output(d,d->player,1,2,0,"</TABLE>%s",(in_command) ? "":"<BR>");
           html_anti_reverse(d,0);
	}
     }
}

/* ---->  Check for existance of specified database file  <---- */
unsigned char tcz_db_exists(const char *filename,unsigned char panic_db,unsigned char compressed_db)
{
	 FILE *f;

	 sprintf(scratch_buffer,"%s%s",dumpfile,filename);
	 if((f = fopen(scratch_buffer,"r"))) {
	    writelog(SERVER_LOG,0,"RESTART","Using %s%sdatabase '%s'...",(compressed_db) ? "compressed ":"",(panic_db) ? "PANIC ":"",scratch_buffer);
	    dumpfull = (char *) alloc_string(scratch_buffer);
	    fclose(f);
	 } else return(0);
	 return(1);
}

/* ---->  Make backup copy of database  <---- */
void tcz_db_copy(const char *srcfile,const char *destfile,unsigned char site)
{
     time_t        start,finish;
     FILE          *src,*dest;
     unsigned char error = 0;
     int           count;

     gettime(start);
     if((src = fopen(srcfile,"r"))) {
        writelog(SERVER_LOG,0,"RESTART","Backing up %sdatabase '%s' to '%s'...",(site) ? "registered Internet site ":"",srcfile,destfile);
        if((dest = fopen(destfile,"w"))) {
           while(!error && (count = fread(scratch_return_string,1,KB,src)))
                 if(!fwrite(scratch_return_string,1,count,dest)) {
                    writelog(SERVER_LOG,0,"RESTART","Error writing to the file '%s'  -  Backup aborted.",destfile);
                    error = 1;
		 }

           gettime(finish);
           if(!error && ferror(src)) writelog(SERVER_LOG,0,"RESTART","Error reading from the %sdatabase '%s'  -  Backup aborted.",(site) ? "registered Internet site ":"",srcfile);
              else if(!error) writelog(SERVER_LOG,0,"RESTART","%satabase backup completed in %s.",(site) ? "Registered Internet site d":"D",interval(finish - start,0,ENTITIES,0));
           fclose(dest);
           fclose(src);
	} else writelog(SERVER_LOG,0,"RESTART","Unable to open the file '%s' for writing (%s)  -  Backup aborted.",destfile,strerror(errno));
     } else writelog(SERVER_LOG,0,"RESTART","Unable to open the file '%s' for reading (%s)  -  Backup aborted.",srcfile,strerror(errno));
}

/* ---->  {J.P.Boggis 30/07/2001}  Make backup copy of executable (For core dump debug)  <---- */
void tcz_exec_copy(const char *execfile)
{
     char          destfile[BUFFER_LEN];
     char          corefile[BUFFER_LEN];
     char          buffer[BUFFER_LEN];
     time_t        start,finish;
     FILE          *src,*dest;
     unsigned char error = 0;
     int           count;

     gettime(start);
     sprintf(corefile,"%s.%d.%d.core",execfile,(int) telnetport,(int) htmlport);
     sprintf(destfile,"%s.%d.%d",execfile,(int) telnetport,(int) htmlport);

     /* ---->  If executable backup already exists, rename to 'bin/<EXEC FILE>.<TELNET PORT>.<HTML PORT>.core'  <---- */
     if(!rename(destfile,corefile)) {
        writelog(SERVER_LOG,0,"RESTART","Renaming executable backup '%s' to '%s'.",destfile,corefile);
     } else {
        writelog(SERVER_LOG,0,"RESTART","Unable to rename executable backup '%s' to '%s' (%s.)",destfile,corefile,strerror(errno));
     }

     /* ---->  Backup current executable to 'bin/<EXEC FILE>.<TELNET PORT>.<HTML PORT>'  <---- */
     if((src = fopen(execfile,"r"))) {
        writelog(SERVER_LOG,0,"RESTART","Backing up executable '%s' to '%s'.",execfile,destfile);
        if((dest = fopen(destfile,"w"))) {
           while(!error && (count = fread(buffer,1,KB,src)))
                 if(!fwrite(buffer,1,count,dest)) {
                    writelog(SERVER_LOG,0,"RESTART","Error writing to the file '%s'  -  Backup aborted.",destfile);
                    error = 1;
		 }

           gettime(finish);
           if(!error && ferror(src)) {
              writelog(SERVER_LOG,0,"RESTART","Error reading from the executable '%s'  -  Backup aborted.",execfile);
	   } else if(!error) {
              if(chmod(destfile,S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP))
                 writelog(SERVER_LOG,0,"RESTART","Error changing permissions of the executable backup '%s' (%s.)",execfile,strerror(errno));
              writelog(SERVER_LOG,0,"RESTART","Executable backup completed in %s.",interval(finish - start,0,ENTITIES,0));
	   }
           fclose(dest);
           fclose(src);
	} else writelog(SERVER_LOG,0,"RESTART","Unable to open the file '%s' for writing (%s)  -  Backup aborted.",destfile,strerror(errno));
     } else writelog(SERVER_LOG,0,"RESTART","Unable to open the file '%s' for reading (%s)  -  Backup aborted.",execfile,strerror(errno));
}

/* ---->  {J.P.Boggis 11/04/1999}  Check for existance (And create if neccessary) of standard directories  <---- */
void tcz_check_directories(void)
{
     mode_t dmode = (S_IRUSR|S_IWUSR|S_IXUSR);
     FILE   *f;

     /* ---->  'log' directory  <---- */
     if(!(f = fopen("log","r"))) {
        if(mkdir("log",dmode))
           writelog(SERVER_LOG,0,"RESTART","'log' directory does not exist  -  Unable to create directory (%s.)",strerror(errno));
	      else writelog(SERVER_LOG,0,"RESTART","'log' directory does not exist  -  Directory created.");
     } else {

        /* ---->  Log directory exists  -  Check for 'gripe.log' and rename to 'complaints.log'  <---- */
        fclose(f);
        if((f = fopen("log/gripe.log","r"))) {
           fclose(f);
           if(rename("log/gripe.log","log/complaints.log"))
              writelog(SERVER_LOG,0,"RESTART","Unable to rename 'log/gripe.log' to 'log/complaints.log' (%s.)",strerror(errno));
	         else writelog(SERVER_LOG,0,"RESTART","'log/gripe.log' renamed to 'log/complaints.log'.");
	}
     }

#ifdef USER_LOG_FILES

     /* ---->  'log/users' directory  <---- */
     if(!(f = fopen("log/users","r"))) {
        if(mkdir("log/users",dmode))
           writelog(SERVER_LOG,0,"RESTART","'log/users' directory does not exist  -  Unable to create directory (%s.)",strerror(errno));
	      else writelog(SERVER_LOG,0,"RESTART","'log/users' directory does not exist  -  Directory created.");
     } else fclose(f);
#endif

     /* ---->  'lib' directory  <---- */
     if(!(f = fopen("lib","r"))) {
        if(mkdir("lib",dmode))
           writelog(SERVER_LOG,0,"RESTART","'lib' directory does not exist  -  Unable to create directory (%s.)",strerror(errno));
	      else writelog(SERVER_LOG,0,"RESTART","'lib' directory does not exist  -  Directory created.");
     } else fclose(f);
}

/* ---->  {J.P.Boggis 17/04/2000}  Check for existence of lock file (Indicating that TCZ is probably still running)  <---- */
void tcz_check_lockfile(void)
{
     struct stat fs;
     FILE   *lf;

     /* ---->  Check for existance of 'lib' directory  <---- */
     if(!stat("./lib",&fs)) {
	if(S_ISDIR(fs.st_mode)) lib_dir = 1;
	   else writelog(RESTART_LOG,0,"RESTART","'lib' is not a directory.");
     } else writelog(RESTART_LOG,0,"RESTART","The 'lib' directory does not exist.");

     /* ---->  Check and create TCZ process lock file  <---- */
     sprintf(scratch_buffer,"%stcz.%d.%d.pid",(lib_dir) ? "lib/":"",(int) telnetport,(int) htmlport);
     lockfile = (char *) alloc_string(scratch_buffer);
     if(!(lf = fopen(lockfile,"r"))) {

	/* ---->  Lock file doesn't exist  <---- */
	if((lf = fopen(lockfile,"w"))) {
	   fprintf(lf,"%d",getpid());
	   fflush(lf);
	   fclose(lf);
	} else {
	   writelog(RESTART_LOG,0,"RESTART","Unable to create the lock file '%s'  -  Restart aborted.",lockfile);
	   logfile_close();
	   exit(1);
	}
     } else {
	int pid = NOTHING;

	/* ---->  Lock file exists  -  Check to see if process is running with PID in lock file  <---- */
	fscanf(lf,"%d",&pid);
	fclose(lf);

	if(!kill(pid,0)) {

	   /* ---->  Lock file exists and process is still running  -  Abort restart  <---- */
	   writelog(RESTART_LOG,0,"RESTART","Lock file '%s' currently exists and is associated with the process %d (PID)  -  %s is either currently running, or an improper shutdown has left this lock file behind.  If %s is not currently running, please remove the lock file before attempting another restart.",lockfile,pid,tcz_full_name,tcz_short_name);
	   logfile_close();
	   exit(1);
	} else {

	   /* ---->  Lock file exists, but process is not still running  -  Replace lock file and continue restart  <---- */
	   unlink(lockfile);
	   if((lf = fopen(lockfile,"w"))) {
	      fprintf(lf,"%d",getpid());
	      fflush(lf);
	      fclose(lf);
	   } else {
	      writelog(RESTART_LOG,0,"RESTART","Unable to create the lock file '%s'  -  Restart aborted.",lockfile);
	      logfile_close();
	      exit(1);
	   }
	}
     }
}

/* ---->  {J.P.Boggis 13/08/2000}  Get server time zone  <---- */
int tcz_get_timezone(int log,int restart)
{
    char   buffer[TEXT_SIZE];
    struct tm *tmnow;
    time_t now;

    now = server_gettime(NULL,0);
    tmnow = localtime(&now);
    FREENULL(tcz_timezone);

    if((strftime(buffer,TEXT_SIZE,"%Z",tmnow) > 0)  && !Blank(buffer)) {
       tcz_timezone = (char *) alloc_string(buffer);
       if(log) writelog(SERVER_LOG,0,(restart) ? "RESTART":"TIME ZONE","System time zone is %s.",tcz_timezone);
       return(1);
    } else tcz_timezone = (char *) alloc_string("GMT/BST");

    if(log) writelog(SERVER_LOG,0,(restart) ? "RESTART":"TIME ZONE","Unable to determine system time zone  -  %s assumed.",tcz_timezone);
    return(0);
}

/* ---->  {J.P.Boggis 25/06/2000}  Execute '.startup' or '.shutdown' compound command in #4  <---- */
void tcz_startup_shutdown(const char *name,const char *logentry)
{
     dbref cached_loc,command;

     if(Blank(name)) return;
     command_type |= STARTUP_SHUTDOWN;
     cached_loc = Location(GLOBAL_COMMANDS);
     db[GLOBAL_COMMANDS].location = NOTHING;
     command = match_object(ROOT,GLOBAL_COMMANDS,NOTHING,name,MATCH_PHASE_GLOBAL,MATCH_PHASE_GLOBAL,SEARCH_COMMAND,MATCH_OPTION_DEFAULT_COMMAND,SEARCH_ALL,NULL,0);
     db[GLOBAL_COMMANDS].location = cached_loc;

     if(Valid(command) && Root(Owner(command)) && Global(Location(command))) {
        command_timelimit = STANDARD_EXEC_TIME_STARTUP;
	db[ROOT].owner = db[ROOT].data->player.chpid = ROOT;
	writelog(SERVER_LOG,0,logentry,"Executing '%s' compound command in %s(#%d)...",name,getname(GLOBAL_COMMANDS),GLOBAL_COMMANDS);
	command_cache_execute(ROOT,command,1,1);
	db[ROOT].owner = db[ROOT].data->player.chpid = ROOT;
        command_timelimit = STANDARD_EXEC_TIME;
     } else writelog(SERVER_LOG,0,logentry,"Unable to execute '%s' compound command in %s(#%d) (It must exist, be owned by %s(#%d) and located in %s(#%d).)",name,getname(GLOBAL_COMMANDS),GLOBAL_COMMANDS,getname(ROOT),ROOT,getname(GLOBAL_COMMANDS),GLOBAL_COMMANDS);

     match_done();
     command_type &= ~STARTUP_SHUTDOWN;
}

/* ---->  {J.P.Boggis 14/07/2000}  Host operating system  <---- */
void tcz_get_operatingsystem(void)
{
     struct utsname uname_data;

     if(!uname(&uname_data) && !Blank(uname_data.sysname)) {
        if(islower(uname_data.sysname[0]))
           uname_data.sysname[0] = toupper(uname_data.sysname[0]);
        operatingsystem = (char *) alloc_string(uname_data.sysname);
     } else operatingsystem = (char *) alloc_string("Unknown");
}

/* ---->  Main program  <---- */
int main(int argc,char **argv) 
{
    unsigned char generate_db = 0,panic_db = 0,noconfig = 0,nopath = 0;
    unsigned char compressed_db = 0,compressed_sites = 0;
    int           count,error,critical;
    time_t        now,total;
    const    char *pname;
    struct   tm   *rtime;

#ifdef RESTRICT_MEMORY
#ifdef USE_PROC
    int    fd;
#endif
#endif

#ifdef RESTRICT_MEMORY
    struct rlimit limit;
#endif

    gettime(uptime);
    srand48(uptime + getpid());
    fclose(stdout);
#ifndef CONSOLE
    fclose(stdin);
#endif

    /* ---->  Default server name and URL's  <---- */
    email_forward_name = alloc_string(EMAIL_FORWARD_NAME);
    tcz_server_name    = alloc_string(TCZ_SERVER_NAME);
    tcz_admin_email    = alloc_string(TCZ_ADMIN_EMAIL);
    tcz_short_name     = alloc_string(TCZ_SHORT_NAME);
    tcz_full_name      = alloc_string(TCZ_FULL_NAME);
    html_home_url      = alloc_string(HTML_HOME_URL);
    html_data_url      = alloc_string(HTML_DATA_URL);
    tcz_location       = alloc_string(TCZ_LOCATION);
    sprintf(scratch_buffer,TELNET_TCZ_PROMPT,tcz_short_name);
    tcz_prompt         = alloc_string(scratch_buffer);
    tcz_get_operatingsystem();
    match_recursed     = 0;

    /* ---->  Set current year  <---- */
    gettime(now);
    rtime = localtime(&now);
    tcz_year = (rtime->tm_year + 1900);

    /* ---->  Set DB creation date and accumulated uptime  <---- */
    db_accumulated_uptime = 0;
    db_creation_date      = uptime;
    db_longest_date       = uptime;

    /* ---->  Set program name (Used by options)  <---- */
    pname = argv[0];
    if(!Blank(pname) && (pname = (char *) strrchr(pname,'/'))) pname++;
       else pname = argv[0];

    /* ---->  Get options  <---- */
    option_get_list(argc,argv);

    /* ---->  Display start-up header  <---- */
    sprintf(scratch_buffer,"The Chatting Zone (TCZ v"TCZ_VERSION".%d)  -  (C) J.P.Boggis 1993 - %d.",TCZ_REVISION,tcz_year);
    fprintf(stderr,"\n%s\n%s\n",scratch_buffer,strpad('~',strlen(scratch_buffer),scratch_return_string));
    fputs("TCZ is free software, which is distributed under version 2 of\nthe GNU General Public License (See 'help gpl' in TCZ, or visit\nhttp://www.gnu.org)  For more information about the TCZ, please\nvisit:  http://www.sourceforge.net/projects/tcz\n\n",stderr);

    /* ---->  Demonstration TCZ  <---- */
#ifdef DEMO
    fputs("This is a demonstration version of TCZ.\n\n",stderr);
#endif

    /* ---->  Options help hint message  <---- */
    fprintf(stderr,"Type '%s --help' for command-line options/arguments.\n\n",pname);

    /* ---->  Display help options/version information and exit?  <---- */
    if(option_helpoptions(pname)) exit(0);

    /* ---->  Use config file?  <---- */
    if(!option_changepath()) {

       /* ---->  If literal path given to executable (Starting with '/' or '~'), change to directory  <---- */
       strcpy(scratch_buffer,argv[0]);
       if((scratch_buffer[0] == '/') || (scratch_buffer[0] == '~')) {
	  char *ptr = scratch_buffer + strlen(scratch_buffer) - 1;

	  for(; (ptr > scratch_buffer) && (*ptr != '/'); ptr--);
	  if((ptr > scratch_buffer) && ((*ptr == '/') || (*ptr == '~'))) {
	     for(*ptr-- = '\0'; (ptr > scratch_buffer) && (*ptr != '/'); ptr--);
	     if((ptr > scratch_buffer) && (*ptr == '/') && !strcasecmp(ptr + 1,"bin")) *ptr = '\0';
	     if(!chdir(scratch_buffer)) {
		tcz_check_directories();
		logfile_open(1);
		writelog(RESTART_LOG,0,"RESTART","Restarting TCZ v%s.%d server on %s.",TCZ_VERSION,TCZ_REVISION,date_to_string(uptime,UNSET_DATE,NOTHING,FULLDATEFMT));
		writelog(RESTART_LOG,0,"RESTART","Working directory path changed to '%s'.",scratch_buffer);
                nopath = 0;
	     } else {
		tcz_check_directories();
		writelog(RESTART_LOG,0,"RESTART","Restarting TCZ v%s.%d server on %s.",TCZ_VERSION,TCZ_REVISION,date_to_string(uptime,UNSET_DATE,NOTHING,FULLDATEFMT));
		writelog(RESTART_LOG,0,"RESTART","Unable to change working directory path to '%s' (%s.)",scratch_buffer,strerror(errno));
                nopath = 1;
	     }
	  } else {
	     tcz_check_directories();
	     logfile_open(1);
             writelog(RESTART_LOG,0,"RESTART","Restarting TCZ v%s.%d server on %s.",TCZ_VERSION,TCZ_REVISION,date_to_string(uptime,UNSET_DATE,NOTHING,FULLDATEFMT));
   	     writelog(RESTART_LOG,0,"RESTART","Working directory path not changed (No literal path to TCZ executable used ('%s'.))",argv[0]);
             nopath = 1;
	  }
       } else {
	  tcz_check_directories();
	  logfile_open(1);
          writelog(RESTART_LOG,0,"RESTART","Restarting TCZ v%s.%d server on %s.",TCZ_VERSION,TCZ_REVISION,date_to_string(uptime,UNSET_DATE,NOTHING,FULLDATEFMT));
	  writelog(RESTART_LOG,0,"RESTART","Working directory path not changed (No literal path to TCZ executable used ('%s'.))",argv[0]);
          nopath = 1;
       }
    } else {
       tcz_check_directories();
       logfile_open(1);
       writelog(RESTART_LOG,0,"RESTART","Restarting TCZ v%s.%d server on %s.",TCZ_VERSION,TCZ_REVISION,date_to_string(uptime,UNSET_DATE,NOTHING,FULLDATEFMT));
       nopath = 1;
    }

    /* ---->  Use config file?  <---- */
    if(!option_configfile(argv[0])) noconfig = 1;

    /* ---->  Process command-line options  <---- */
    option_main(argv[0]);

    /* ---->  Free option list  <---- */
    option_free_list();

    /* ---->  Check for existence of lock file  <---- */
    tcz_check_lockfile();
    logfile_open(2);

    /* ---->  Working directory path not changed?  <---- */
    if(nopath) writelog(RESTART_LOG,1,"RESTART","Working directory path was not changed at start-up (Check 'Restart' log file for details.)");

    /* ---->  No configuration file loaded?  <---- */
    if(noconfig) writelog(RESTART_LOG,1,"RESTART","No configuration file was processed at start-up (Check 'Options' log file for details.)");

    /* ---->  Server system time zone  <---- */
    tcz_get_timezone(1,1);

    /* ---->  Set maximum core dump size  <---- */
#ifdef RESTRICT_MEMORY
    getrlimit(RLIMIT_CORE,&limit);
    if(option_coredump(OPTSTATUS))
       limit.rlim_cur = RLIM_INFINITY;
          else limit.rlim_cur = 0;
    setrlimit(RLIMIT_CORE,&limit);
#endif

    /* ---->  Automatically lookup server information  <---- */
#ifdef SERVERINFO
    if(option_serverinfo(OPTSTATUS)) {
       if(option_local(OPTSTATUS)) {
          writelog(SERVER_LOG,0,"RESTART","Server is running locally  -  Server information will not be looked up automatically.");
       } else serverinfo();
    }
#endif

    /* ---->  Set scheduling priority (Preferably maximum, if ran as root)  <---- */
#ifdef RESTRICT_MEMORY
    if(setpriority(PRIO_PROCESS,getpid(),option_nice(OPTSTATUS)))
       writelog(SERVER_LOG,0,"RESTART","Unable to set scheduling priority (Nice level) to %d (%s.)",option_nice(OPTSTATUS),strerror(errno));
          else writelog(SERVER_LOG,0,"RESTART","Scheduling priority (Nice level) set to %d.",option_nice(OPTSTATUS));
#endif

    /* ---->  Core dump status  <---- */
    writelog(SERVER_LOG,0,"RESTART","Core dump is %sabled.",option_coredump(OPTSTATUS) ? "en":"dis");

    /* ---->  Emergency (Panic) database dump status  <---- */
    writelog(SERVER_LOG,0,"RESTART","Emergency (Panic) database dump is %sabled.",option_emergency(OPTSTATUS) ? "en":"dis");

    /* ---->  Set stack limit  <---- */
#ifdef RESTRICT_MEMORY
    getrlimit(RLIMIT_STACK,&limit);
    limit.rlim_cur = LIMIT_STACK * KB;
    if(!setrlimit(RLIMIT_STACK,&limit))
       writelog(SERVER_LOG,0,"RESTART","Stack limit set to %.2fMb.",(float) limit.rlim_cur / MB);
          else writelog(SERVER_LOG,0,"RESTART","Unable to set stack limit to %.2fMb.",(float) limit.rlim_cur / MB);
#endif

    /* ---->  Pre-initialisation  <---- */
    max_descriptors = getdtablesize() - RESERVED_DESCRIPTORS;
    grproot.next = NULL;

    if(option_generate(OPTSTATUS)) {

       /* ---->  Create new empty database  <---- */
       sprintf(scratch_buffer,"%s%s",(lib_dir) ? "lib/":"",option_generate(OPTSTATUS));
       dumpfile = (char *) alloc_string(scratch_buffer);
       strcat(scratch_buffer,".db");
       dumpfull = (char *) alloc_string(scratch_buffer);
       generate_db = 1;
       db_create();
    } else {

       /* ---->  Check for existance of database (Try extensions '.PANIC', '.new' and '.db' (In that order.))  <---- */
       sprintf(scratch_buffer,"%s%s",(lib_dir) ? "lib/":"",option_database(OPTSTATUS));
       dumpfile = (char *) alloc_string(scratch_buffer);
       if(!option_compress_disk(OPTSTATUS) || !tcz_db_exists(".PANIC"DB_EXTENSION,1,1)) {
          if(!option_compress_disk(OPTSTATUS) || !tcz_db_exists(".new"DB_EXTENSION,0,1)) {
              if(!option_compress_disk(OPTSTATUS) || !tcz_db_exists(".db"DB_EXTENSION,0,1)) {
                 if(!tcz_db_exists(".PANIC",1,0)) {
                    if(!tcz_db_exists(".new",0,0)) {
                       if(!tcz_db_exists(".db",0,0)) {
                          writelog(SERVER_LOG,0,"RESTART","Unable to load the database '%s'  -  Restart aborted.",dumpfile);
                          logfile_close();
                          if(lockfile) unlink(lockfile);
                          exit(1);
		       }
		    }
		 } else panic_db = 1;
	      } else compressed_db = 1;
	  } else compressed_db = 1;
       } else panic_db = 1, compressed_db = 1;
    }

#ifdef EXTRA_COMPRESSION

    /* ---->  Initialise compression table for database compression  <---- */
    if(!generate_db && option_compress_memory(OPTSTATUS) && !initialise_compression_table(dumpfull,compressed_db)) {
       writelog(SERVER_LOG,0,"RESTART","Sorry, couldn't initialise the compression table from the database '%s' (It either doesn't exist, or is unreadable)  -  Restart aborted.",dumpfull);
       logfile_close();
       if(lockfile) unlink(lockfile);
       exit(1);
    }
#endif

    /* ---->  Load HTML Interface images  <---- */
#ifdef HTML_INTERFACE
    if(option_images(OPTSTATUS))
       html_load_images();
#endif

    /* ---->  Load registered Internet sites  <---- */
#ifndef DEMO
    compressed_sites = register_sites();
#endif

    /* ---->  Load terminal definitions  <---- */
    reload_termcap(0);

    /* ---->  Load generic & local On-line Help topics and tutorials  <---- */
    count = help_register_topics(&localhelp,LOCAL_HELP_FILE,"*",0,1,1);
    writelog(SERVER_LOG,0,"RESTART","  %d local help topic%s registered.",count,Plural(count));

    count = help_register_topics(&generichelp,GENERIC_HELP_FILE,"*",0,1,0);
    writelog(SERVER_LOG,0,"RESTART","  %d generic help topic%s registered.",count,Plural(count));

    count = help_register_topics(&localtutorials,LOCAL_TUTORIAL_FILE,"*",0,0,1);
    writelog(SERVER_LOG,0,"RESTART","  %d local tutorial%s registered.",count,Plural(count));

    count = help_register_topics(&generictutorials,GENERIC_TUTORIAL_FILE,"*",0,0,0);
    writelog(SERVER_LOG,0,"RESTART","  %d generic tutorial%s registered.",count,Plural(count));

    help_status();

    /* ---->  Load title screens  <---- */
    if(!help_reload_titles())
       writelog(SERVER_LOG,0,"RESTART","Unable to load the title screens.");

    /* ---->  Load disclaimer  <---- */
    if(!(disclaimer = (char *) help_reload_text(DISC_FILE,0,1,1)))
       writelog(SERVER_LOG,0,"RESTART","Unable to load the disclaimer.");

    /* ---->  Load TCZ map  <---- */
    map_reload(NOTHING,0);

    /* ---->  Load database  <---- */
    if(init_tcz(!generate_db,compressed_db) < 0) {
       if(panic_db) {
          sprintf(scratch_buffer,"%s.PANIC%s",dumpfile,(compressed_db) ? DB_EXTENSION:"");
          sprintf(scratch_buffer + KB,"%s.CORRUPT%s",dumpfile,(compressed_db) ? DB_EXTENSION:"");
          writelog(SERVER_LOG,0,"RESTART","Unable to load the database '%s'  -  Renaming to '%s'  -  Restart aborted.",scratch_buffer,scratch_buffer + KB);
          if(rename(scratch_buffer,scratch_buffer + KB))
             writelog(SERVER_LOG,0,"RESTART","Unable to rename '%s' to '%s'.",scratch_buffer,scratch_buffer + KB);
       } else writelog(SERVER_LOG,0,"RESTART","Unable to load the database '%s'  -  Restart aborted.",dumpfile);
       logfile_close();
       if(lockfile) unlink(lockfile);
       exit(1);
    }

    /* ---->  Backup executable (To allow later core dump debug if executable changes due to re-compile.)  <---- */
    tcz_exec_copy(argv[0]);

    /* ---->  Backup databases  <---- */
    if(!generate_db) {

       /* ---->  Copy '<DBNAME>.db' to '<DBNAME>.backup'...  <---- */
       sprintf(scratch_buffer,"%s.db%s",dumpfile,(compressed_db) ? DB_EXTENSION:"");
       sprintf(scratch_buffer + KB,"%s.backup%s",dumpfile,(compressed_db) ? DB_EXTENSION:"");
       tcz_db_copy(scratch_buffer,scratch_buffer + KB,0);

       /* ---->  Copy 'sites.db' to 'sites.backup'...  <--- */
       if(sitelist) {
          if(compressed_sites) tcz_db_copy(SITE_FILE""DB_EXTENSION,SITE_FILE".backup"DB_EXTENSION,1);
             else tcz_db_copy(SITE_FILE,SITE_FILE".backup",1);
       }

       /* ---->  Move '<DBNAME>.new' (Or '<DBNAME>.PANIC', if started with a PANIC database) to '<DBNAME>.db'... <---- */
       sprintf(scratch_buffer,"%s.%s%s",dumpfile,(panic_db) ? "PANIC":"new",(compressed_db) ? DB_EXTENSION:"");
       sprintf(scratch_buffer + KB,"%s.db%s",dumpfile,(compressed_db) ? DB_EXTENSION:"");
       if(!rename(scratch_buffer,scratch_buffer + KB))
          writelog(SERVER_LOG,0,"RESTART","Database '%s' moved to '%s'.",scratch_buffer,scratch_buffer + KB);
    }

    /* ---->  Initialise TCZ server  <---- */
    server_set_signals();
    initialise_quotas(1);
    move_sendhome();
    grp = (struct grp_data *) grouprange_initialise(&grproot);
    grproot.next = NULL;
    stats_tcz_update_record(0,0,0,0,0,1,0);
    tcz_time_sync(1);

    /* ---->  Execute '.startup' command in GLOBAL_COMMANDS location  <---- */
    tcz_startup_shutdown(".startup","RESTART");

    /* ---->  Check and open Telnet/HTML sockets  <---- */
#ifdef SOCKETS
    if(telnetport > 0) 
       if((telnet = server_open_socket(telnetport,1,0,0,0)) < 0) {
          logfile_close();
          if(lockfile) unlink(lockfile);
          exit(1);
       }

#ifdef HTML_INTERFACE
    if(htmlport > 0)
       if((html = server_open_socket(htmlport,1,0,1,0)) < 0) {
          logfile_close();
          if(lockfile) unlink(lockfile);
          exit(1);
       }
#endif
#endif

    /* ---->  Set memory restrictions  <---- */
#ifdef RESTRICT_MEMORY
#ifdef USE_PROC
    sprintf(scratch_return_string,PROC_STATM,getpid());
    if((fd = open(scratch_return_string,O_RDONLY,0)) != -1) {
       int psize = getpagesize(),rss;

       read(fd,scratch_return_string,TEXT_SIZE);
       close(fd);
       sscanf(scratch_return_string,"%*u %u %*u %*u %*u %*u %*u%*s",&rss);

       if(rss > 0) {
          long newlimit;

          getrlimit(RLIMIT_RSS,&limit);
          newlimit = (rss * psize) * LIMIT_MEMORY_PERCENTAGE;
          if(newlimit < (LIMIT_MEMORY_MINIMUM * MB)) newlimit = (LIMIT_MEMORY_MINIMUM * MB);
          limit.rlim_cur = (rss * psize) + newlimit;
          memory_restriction = limit.rlim_cur;

          if(!setrlimit(RLIMIT_DATA,&limit) && !setrlimit(RLIMIT_RSS,&limit))
             writelog(SERVER_LOG,0,"RESTART","Data and Resident Set Size (RSS) restricted to a maximum of %.2fMb.",(float) limit.rlim_cur / MB);
                else writelog(SERVER_LOG,0,"RESTART","Unable to restrict data and Resident Set Size (RSS) to a maximum of %.2fMb.",(float) limit.rlim_cur / MB);
       } else writelog(SERVER_LOG,0,"RESTART","Unable to get current memory usage  -  Cannot set memory usage restrictions.");
    } else writelog(SERVER_LOG,0,"RESTART","Unable to open/access the file '"ANSI_LWHITE"%s"ANSI_LGREEN"' -  Cannot set memory usage restrictions.",scratch_return_string);
#else
    if(getrusage(RUSAGE_SELF,&usage) != -1) {
       int  psize = getpagesize();
       long newlimit;

       getrlimit(RLIMIT_RSS,&limit);
       if(limit.rlim_cur > 0) {
          newlimit = (usage.ru_maxrss * psize) * LIMIT_MEMORY_PERCENTAGE;
          if(newlimit < (LIMIT_MEMORY_MINIMUM * MB)) newlimit = (LIMIT_MEMORY_MINIMUM * MB);
          limit.rlim_cur = (usage.ru_maxrss * psize) + newlimit;
          memory_restriction = limit.rlim_cur;

          if(!setrlimit(RLIMIT_DATA,&limit) && !setrlimit(RLIMIT_RSS,&limit))
             writelog(SERVER_LOG,0,"RESTART","Data and Resident Set Size (RSS) restricted to a maximum of %.2fMb.",(float) limit.rlim_cur / MB);
                else writelog(SERVER_LOG,0,"RESTART","Unable to restrict data and Resident Set Size (RSS) to a maximum of %.2fMb.",(float) limit.rlim_cur / MB);
       } else writelog(SERVER_LOG,0,"RESTART","Unable to get current memory usage  -  Cannot set memory usage restrictions.");
    } else writelog(SERVER_LOG,0,"RESTART","Unable to get current memory usage  -  Cannot set memory usage restrictions.");
#endif
#endif

#ifdef GUARDIAN_ALARM

    /* ---->  Guardian alarm enabled?  <---- */
    writelog(SERVER_LOG,0,"RESTART","Guardian alarm is %sabled (%s will %sbe shutdown automatically if user command/IO processing takes more than %s.)",option_guardian(OPTSTATUS) ? "en":"dis",tcz_short_name,option_guardian(OPTSTATUS) ? "":"not ",interval(GUARDIAN_ALARM_TIME,0,ENTITIES,0));
#endif

    /* ---->  Start TCZ server  <---- */
    gettime(now);
    writelog(SERVER_LOG,0,"RESTART","Total time of restart:  %s.",interval(now - uptime,0,ENTITIES,0));
    server_mainloop();

    /* ---->  Close sockets and dump database to disk  <---- */
    log_stderr = 1;
    update_all_lasttotal();
    server_close_sockets();
    gettime(now), total = now;
    writelog(SERVER_LOG,0,"SHUTDOWN","Shutdown on %s (Total uptime:  %s.)",date_to_string(now,UNSET_DATE,NOTHING,FULLDATEFMT),interval(now - uptime,0,ENTITIES,0));

    /* ---->  Execute '.shutdown' command in GLOBAL_COMMANDS location  <---- */
    tcz_startup_shutdown(".shutdown","SHUTDOWN");

    /* ---->  Generate final database dump  <---- */
    if(option_console(OPTSTATUS))
       option_console(NOTHING,"SHUTDOWN","No",0,&error,&critical);

#ifdef DATABASE_DUMP
#ifdef DB_COMPRESSION
    writelog(SERVER_LOG,0,"SHUTDOWN","Dumping database '%s.new"DB_EXTENSION"'...",dumpfile);
#else
    writelog(SERVER_LOG,0,"SHUTDOWN","Dumping database '%s.new'...",dumpfile);
#endif

    if(dumpstatus > 0) {
       dumpstatus = 255;
       db_write(NULL);
    }

    dumpstatus = 1;
    dumptype   = DUMP_NORMAL;
    for(; dumpstatus > 0; db_write(NULL));
#else 
    writelog(SERVER_LOG,0,"SHUTDOWN","Database dumping has been disabled.  Unable to dump database.");
#endif

    gettime(now);
    writelog(SERVER_LOG,1,"SHUTDOWN","%s (TCZ v%s.%d) server shutdown completed in %s.",tcz_short_name,TCZ_VERSION,TCZ_REVISION,interval(now - total,0,ENTITIES,0));
    logfile_close();

    if(lockfile) unlink(lockfile);
    exit(0);
}
