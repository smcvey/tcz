/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| LOGFILES.C  -  Implements system and user log files that are fully          |
|                accessible from within TCZ.                                  |
|                                                                             |
|                '@log'       -  Browse log files.                            |
|                '@logentry'  -  Write custom entries to log files.           |
|                                                                             |
|                include/logfiles.h       -  Log file constants               |
|                include/logfile_table.h  -  Table of log files.              |
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

  $Id: logfiles.c,v 1.1.1.1 2004/12/02 17:41:44 jpboggis Exp $

*/


#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "object_types.h"
#include "flagset.h"


static int logfile_current = NOTHING;  /*  Current log file being accessed by '@log'  */


/* ---->  Log files  <---- */
struct log_data {
       const    char *filename;  /*  Log filename  */
       const    char *name;      /*  Name of log (For '@log')  */
       unsigned long maxsize;    /*  Maximum size of log file  */
       unsigned long newsize;    /*  New size of log file (After truncation)  */
       unsigned char open;       /*  Keep log file open permanently (Rather than opening as and when needed)  */
       unsigned char level;      /*  Access level (As for level_app()) required to access log file  */
       FILE          *file;      /*  Descriptor currently in use by log file  */
};

#include "logfile_table.h"


/* ---->  Open log files  <---- */
void logfile_open(int restart)
{
     int    count,restartlog;
     time_t now;

     gettime(now);
     if(option_loglevel(OPTSTATUS) <= 1) return;
     for(count = 0; count < LOG_COUNT; count++) {
         if(!logfs[count].file) logfs[count].file = fopen(logfs[count].filename,"a+");

         if(logfs[count].file) {
            restartlog = ((count == RESTART_LOG) || (count == OPTIONS_LOG));
            if(((restart == 1) && restartlog) || ((restart == 2) && !restartlog)) {
	       if(count != STATS_LOG) {
                  fprintf(logfs[count].file,"[***  %s restarted on %s.  ***]\n",tcz_short_name,date_to_string(now,UNSET_DATE,NOTHING,FULLDATEFMT));
	       }
	    }

            if(!(logfs[count].open && logfs[count].maxsize)) {
               fclose(logfs[count].file);
               logfs[count].file = NULL;
	    }
	 } else fprintf(stderr,"LOG:  Unable to open/create the log file '%s'.\n",logfs[count].filename);
     }
}

/* ---->  Close log files  <---- */
void logfile_close()
{
     int count;

     for(count = 0; count < LOG_COUNT; count++) {
         if(logfs[count].file) {
            fflush(logfs[count].file);
            fclose(logfs[count].file);
	 }
         logfs[count].file = NULL;
     }
}

/* ---->  Get current time and date (Short form)  <---- */
const char *get_timedate(void)
{
      static char buffer[32];
      struct tm *now;
      time_t stamp;

      gettime(stamp);
      now = localtime(&stamp);
 
      sprintf(buffer,"  [%02d:%02d %02d/%02d/%04d]",now->tm_hour,now->tm_min,now->tm_mday,now->tm_mon + 1,now->tm_year + 1900);
      return(buffer);
}

/* ---->  Write to given log file  <---- */
void writelog(int logfile,int logdate,const char *title,const char *fmt, ...)
{
     char          lfbuffer[KB];
     FILE          *lf = NULL;
     unsigned long length;
     va_list       ap;

     if(option_loglevel(OPTSTATUS) <= 0) return;
     if(logfile == logfile_current) return;

#ifndef USER_LOG_FILES
     if(logfile < 0) return;
#else
     if((logfile < 0) && (!option_userlogs(OPTSTATUS) || !Validchar(ABS(logfile)))) return;
#endif

     if((logfile >= 0) && !logfs[logfile].maxsize) return;

     if(logfile >= 0) {
        if(!logfs[logfile].file) logfs[logfile].file = fopen(logfs[logfile].filename,"a+");
     } else {
        sprintf(lfbuffer,"log/users/user_%d.log",ABS(logfile));
        lf = fopen(lfbuffer,"a+");
     }

     /* ---->  Log to log file  <---- */
     if(lf || ((logfile >= 0) && logfs[logfile].file)) {
        fprintf((lf) ? lf:logfs[logfile].file,"%s:  ",title);
	va_start(ap, fmt);
        vfprintf((lf) ? lf:logfs[logfile].file,fmt,ap);
	va_end(ap);
        fprintf((lf) ? lf:logfs[logfile].file,"%s\n",(logdate) ? get_timedate():"");
        fflush((lf) ? lf:logfs[logfile].file);
     }

     /* ---->  Log to console (stderr)  <---- */
     if(!(lf || ((logfile >= 0) && logfs[logfile].file)) || option_console(OPTSTATUS) || (log_stderr && ((logfile == SERVER_LOG) || (logfile == OPTIONS_LOG) || (logfile == RESTART_LOG)))) {
        char buffer[KB];

        if(option_console(OPTSTATUS)) {
	   if((logfile >= 0) && logfs[logfile].file) {
              const char *src;
              char  *dest;

              for(dest = buffer, src = logfs[logfile].name; *src; *dest++ = tolower(*src++));
              *dest = '\0';
              strcat(buffer,".log");
	   } else strcpy(buffer,"stderr.log");
           fprintf(stderr,"%s:  (%s)  ",title,buffer);
	} else fprintf(stderr,"%s:  ",title);

	va_start(ap, fmt);
        vfprintf(stderr,fmt,ap);
	va_end(ap);
        fprintf(stderr,"%s\n",(logdate) ? get_timedate():"");
        fflush(stderr);
     }

     /* ---->  Log file > MAXSIZE  -  Truncate to NEWSIZE  <---- */
     if((lf || ((logfile >= 0) && logfs[logfile].file)) && ((length = ftell((lf) ? lf:logfs[logfile].file)) > (((lf) ? USER_LOG_FILE_SIZE:logfs[logfile].maxsize) * KB))) {
        char buffer[131072];
        char fname[256];
        int  count;
        FILE *f;

        /* ---->  Copy truncated portion of log file to '<LOGFILENAME>.temp'  <---- */
        fseek((lf) ? lf:logfs[logfile].file,length - (((lf) ? USER_LOG_FILE_RESET:logfs[logfile].newsize) * KB),SEEK_SET);
        sprintf(fname,"%s.temp",(lf) ? lfbuffer:logfs[logfile].filename);
        if((f = fopen(fname,"w"))) {
           while((count = fread(buffer,1,131072,(lf) ? lf:logfs[logfile].file)))
                 fwrite(buffer,1,count,f);
           fclose(f);

           /* ---->  Truncate log file and rename to '<LOGFILE>.backup' for backup  <---- */
           rewind((lf) ? lf:logfs[logfile].file);
           ftruncate(fileno((lf) ? lf:logfs[logfile].file),length - (((lf) ? USER_LOG_FILE_RESET:logfs[logfile].newsize) * KB));
           if(!lf) {
              sprintf(fname,"%s.backup",logfs[logfile].filename);
              fclose(logfs[logfile].file);
              rename(logfs[logfile].filename,fname);
	   } else unlink(fname);

           /* ---->  Delete '<LOGFILE>.bak' (Old-style log file backup), if it exists  <---- */
           if(!lf) {
              sprintf(fname,"%s.bak",logfs[logfile].filename);
              if((f = fopen(fname,"r"))) {
                 fclose(f);
                 unlink(fname);
	      }
	   }
          
           /* ---->  Re-open new truncated copy of log file  <---- */
           sprintf(fname,"%s.temp",(lf) ? lfbuffer:logfs[logfile].filename);
           rename(fname,(lf) ? lfbuffer:logfs[logfile].filename);
           if(!lf) {
              logfs[logfile].file = fopen(logfs[logfile].filename,"a+");
              if(!logfs[logfile].file) fprintf(stderr,"LOG:  Unable to re-open new truncated log file '%s'.\n",logfs[logfile].filename);
	   } else {
              lf = fopen(lfbuffer,"a+");
              if(!lf) fprintf(stderr,"LOG:  Unable to re-open new truncated log file '%s'.\n",lfbuffer);
	   }
        } else fseek((lf) ? lf:logfs[logfile].file,length,SEEK_SET);
     }

     /* ---->  Close log file (If not kept open permanently)  <---- */
     if(lf || ((logfile >= 0) && !logfs[logfile].open && logfs[logfile].file)) {
        fclose((lf) ? lf:logfs[logfile].file);
        if(!lf) logfs[logfile].file = NULL;
     }
}

/* ---->  View/reset log file  <---- */
void logfile_log(CONTEXT)
{
     int                      logfile = NOTHING,loop,gsize,page = 0,count = 10,c;
     unsigned char            twidth = output_terminal_width(player);
     struct   descriptor_data *p = getdsc(player);
     char                     buffer2[BUFFER_LEN];
     char                     buffer[BUFFER_LEN];
     long                     oldpos,curpos;
     dbref                    userlog = 0;
     FILE                     *lf = NULL;
     char                     *ptr;
     time_t                   now;

     setreturn(ERROR,COMMAND_FAIL);
     if(!in_command || Wizard(current_command)) {

	/* ---->  User log file  <---- */
	if((*arg1 == NUMBER_TOKEN) || (*arg1 == LOOKUP_TOKEN)) {
	   if((userlog = lookup_character(player,arg1,1)) <= 0) {
	      output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
	      return;
	   }
	} else if(Blank(arg1) || !strcasecmp("me",arg1)) userlog = player;

	/* ---->  User log files disabled?  <---- */
	if((userlog > 0) && !option_userlogs(OPTSTATUS)) {
	   output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, user log files are disabled.");
	   return;
	}

	/* ---->  View log file  <---- */
	if(((userlog <= 0) && Level4(Owner(player))) || ((userlog > 0) && Builder(Owner(player)) && can_write_to(player,userlog,1))) {

	   /* ---->  Find log file  <---- */
	   if(!userlog) {
	      if(!Blank(arg1)) for(loop = 0; (loop < LOG_COUNT) && (logfile == NOTHING); loop++)
		 if(string_prefix(logfs[loop].name,arg1)) logfile = loop;
              logfile_current = logfile;

	      /* ---->  Check for valid log file name  <---- */
	      if(logfile == NOTHING) {
		 html_anti_reverse(p,1);
		 if(Blank(arg1)) {
                    output(p,player,0,1,0,ANSI_LGREEN"\nPlease specify which log file to view.");
		 } else if(!string_prefix("list",arg1)) {
                    output(p,player,0,1,0,ANSI_LGREEN"\nSorry, the log file '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
		 }

		 if(IsHtml(p)) output(p,player,1,2,0,"<BR><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_GREY">");
		 output(p,player,2,1,1,"%sThe following log files are available...%s",IsHtml(p) ? "\016<TR><TH ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><FONT SIZE=4><I>\016"ANSI_LCYAN:"\n ",IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
		 if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));

		 output_columns(p,player,NULL,NULL,twidth,1,13,2,0,1,FIRST,0,"***  NO LOG FILES ARE AVAILABLE  ***",buffer2);
		 for(loop = 0; loop < LOG_COUNT; loop++)
		     output_columns(p,player,logfs[loop].name,ANSI_LYELLOW,0,1,13,2,0,1,DEFAULT,0,NULL,buffer2);
		 output_columns(p,player,NULL,NULL,0,1,13,2,0,1,LAST,0,NULL,buffer2);

		 if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,1,'-','='));
		    else output(p,player,1,2,0,"</TABLE><BR>");
		 html_anti_reverse(p,0);
                 logfile_current = NOTHING;
		 return;
	      }
	   } else {
	      sprintf(buffer,"log/users/user_%d.log",userlog);
	      if(!(lf = fopen(buffer,"r"))) {
		 output(p,player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" doesn't currently have a log file.",Article(userlog,LOWER,DEFINITE),getcname(NOTHING,userlog,0,0));
                 logfile_current = NOTHING;
		 return;
	      } else fclose(lf);
	   }

	   if(!Blank(arg2) && !strcasecmp(arg2,"reset")) {
	      if((userlog > 0) || Level1(player)) {
		 if(!in_command) {

		    /* ---->  Reset log file  <---- */
		    if(!userlog) {
		       if(logfs[logfile].file) fclose(logfs[logfile].file);
		       sprintf(buffer,"%s.backup",logfs[logfile].filename);
		       rename(logfs[logfile].filename,buffer);
		       logfs[logfile].file = fopen(logfs[logfile].filename,"a+");
		    } else {
		       sprintf(buffer,"log/users/user_%d.log",userlog);
		       if(unlink(buffer)) {
			  if(userlog == player) output(p,player,0,1,0,ANSI_LGREEN"Unable to reset your log file ("ANSI_LYELLOW"%s"ANSI_LGREEN".)",strerror(errno));
			     else output(p,player,0,1,0,ANSI_LGREEN"Unable to reset %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s log file ("ANSI_LYELLOW"%s"ANSI_LGREEN".)",Article(userlog,LOWER,DEFINITE),getcname(NOTHING,userlog,0,0),strerror(errno));
                          logfile_current = NOTHING;
			  return;
		       }
		       lf = fopen(buffer,"a+");
		    }

		    if(lf || logfs[logfile].file) {
		       gettime(now);
		       fprintf((lf) ? lf:logfs[logfile].file,"        [***  Log file reset by %s on %s...  ***]\n",db[player].name,date_to_string(now,UNSET_DATE,NOTHING,FULLDATEFMT));
		       fflush((lf) ? lf:logfs[logfile].file);
		       if(lf) {
			  if(userlog != player) {
			     output(p,player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN"'s log file has been reset.",Article(userlog,UPPER,DEFINITE),getcname(NOTHING,userlog,0,0));
			     if(!Level4(player)) {
				if(in_command && Valid(current_command)) {
				   if(!Wizard(current_command))
				      writelog(HACK_LOG,1,"LOG","%s(#%d) reset %s(#%d)'s log file from within compound command %s(#%d.)",getname(player),player,getname(userlog),userlog,getname(current_command),current_command);
				} else writelog(HACK_LOG,1,"LOG","%s(#%d) reset %s(#%d)'s log file.",getname(player),player,getname(userlog),userlog);
			     } else writelog(ADMIN_LOG,1,"LOG","%s(#%d) reset %s(#%d)'s log file.",getname(player),player,getname(userlog),userlog);
			  } else output(p,player,0,1,0,ANSI_LGREEN"Your log file has been reset.");
		       } else {
			  output(p,player,0,1,0,ANSI_LGREEN"Log file '"ANSI_LWHITE"%s"ANSI_LGREEN"' has been reset.",logfs[logfile].name);
			  sprintf(buffer,"%s.bak",logfs[logfile].filename);
			  unlink(buffer);
		       }
		       setreturn(OK,COMMAND_SUCC);
		    } else {
		       writelog(SERVER_LOG,1,"LOG","Unable to open/create the log file '%s'.",logfs[logfile].filename);
		       output(p,player,0,1,0,ANSI_LGREEN"Sorry, unable to reset the log file '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",logfs[logfile].name);
		    }

		    if((userlog > 0) || !logfs[logfile].open) {
		       fclose((lf) ? lf:logfs[logfile].file);
		       if(!userlog) logfs[logfile].file = NULL;
		    }
		 } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, log files cannot be reset from within a compound command.");
	      } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, only Deities and the Supreme Being may reset log files.");
	   } else if(!Blank(arg2) && (!strcasecmp(arg2,"reopen") || !strcasecmp(arg2,"re-open"))) {
	      if(!userlog) {
		 if(Level1(db[player].owner)) {
		    if(logfs[logfile].open) {

		       /* ---->  Re-open log file  <---- */
		       if(!Level4(player) && in_command) writelog(ADMIN_LOG,1,"HACK","Mortal %s(#%d) re-opened the log file '%s' from within compound command %s(#%d) owned by %s(#%d).",getname(player),player,logfs[logfile].file,getname(current_command),current_command,getname(db[current_command].owner),db[current_command].owner);
		       fclose(logfs[logfile].file);
		       logfs[logfile].file = fopen(logfs[logfile].filename,"a+");
		       if(!logfs[logfile].file) {
			  fprintf(stderr,"LOG:  Unable to re-open the log file '%s'.\n",logfs[logfile].filename);
			  output(p,player,0,1,0,ANSI_LGREEN"Sorry, unable to re-open the log file '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",logfs[logfile].name);
		       } else {
			  output(p,player,0,1,0,ANSI_LGREEN"Log file '"ANSI_LWHITE"%s"ANSI_LGREEN"' re-opened.",logfs[logfile].name);
			  setreturn(OK,COMMAND_SUCC);
		       }
		    } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, that log file isn't kept permanently open (It's closed when not in use.)");
		 } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, only Deities and the Supreme Being may re-open %s log files.",tcz_full_name);
	      } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, user log files cannot be re-opened.");
	   } else if((userlog > 0) || (privilege(Owner(player),4) <= logfs[logfile].level)) {
	      if((userlog > 0) || (logfile != COMMAND_LOG) || !((option_loglevel(OPTSTATUS) >= 3) && !Level1(Owner(player)))) {
		 if(userlog > 0) sprintf(buffer,"log/users/user_%d.log",userlog);
		 if(((userlog > 0) && (lf = fopen(buffer,"a+"))) || (!(!logfs[logfile].file && !(logfs[logfile].file = fopen(logfs[logfile].filename,"a+"))))) {
		    int  cached_command_type = command_type;
		    char *title,*text,custom;

		    /* ---->  List entries from log file (Back-tracked by <N> page(s))  <---- */
		    html_anti_reverse(p,1);
		    command_type |= NO_AUTO_FORMAT;
		    if(!userlog && !Level4(player) && in_command) {
		       writelog(ADMIN_LOG,1,"HACK","Mortal %s(#%d) browsed the log file '%s' from within compound command %s(#%d) owned by %s(#%d).",getname(player),player,logfs[logfile].file,getname(current_command),current_command,getname(db[current_command].owner),db[current_command].owner);
		       writelog(HACK_LOG,1,"HACK","Mortal %s(#%d) browsed the log file '%s' from within compound command %s(#%d) owned by %s(#%d).",getname(player),player,logfs[logfile].file,getname(current_command),current_command,getname(db[current_command].owner),db[current_command].owner);
		    }

		    if(!Blank(arg2)) page = atol(arg2);
		    if(page < 0) page = 0;
		    /*
		     * SAB: seems that opening "a+" used to position at
		     * end of file, and doesn't now so that ftell is returning
		     * the _beginning_ of the file (linux 2.4.22/gcc 3.3).
		     * Lets make it explicit with an fseek().
		     */
		    fseek((lf) ? lf:logfs[logfile].file, 0, SEEK_END);
		    curpos = (oldpos =  ftell((lf) ? lf:logfs[logfile].file)) - 1;
		    gsize  = (db[player].data->player.scrheight - 6) / 2;
		    loop   = gsize + ((gsize - 1) * page);

		    if(Level1(db[player].owner) || (loop <= LOG_BACKTRACK_CONSTRAINT)) {

		       /* ---->  Skip blank lines at end of log file  <---- */
		       for(c = '\n'; (curpos > 0) && (c == '\n') && (c != EOF); curpos--) {
			   if(curpos >= 0) fseek((lf) ? lf:logfs[logfile].file,curpos,SEEK_SET);
			   c = fgetc((lf) ? lf:logfs[logfile].file);
		       }

		       /* ---->  Seek to start of line  <---- */
		       for(curpos++; (loop > 0) && (curpos > 0) && (c != EOF); loop--)
			   for(c = ' '; (curpos > 0) && (c != '\n') && (c != EOF); c = fgetc((lf) ? lf:logfs[logfile].file))
			       if(curpos > 0) fseek((lf) ? lf:logfs[logfile].file,--curpos,SEEK_SET);

		       if(IsHtml(p))
                          output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");

		       if(!in_command) {
			  if(!page) {
			     if(userlog > 0) output(p,player,2,1,1,"%sLast page of entries in %s"ANSI_LWHITE"%s"ANSI_LCYAN"'s log file...%s",IsHtml(p) ? "\016<TR><TH ALIGN=CENTER COLSPAN=2 BGCOLOR="HTML_TABLE_CYAN"><FONT SIZE=4><I>\016"ANSI_LCYAN:"\n ",Article(userlog,LOWER,DEFINITE),getcname(NOTHING,userlog,0,0),IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
				else output(p,player,2,1,1,"%sLast page of entries in log file '"ANSI_LWHITE"%s"ANSI_LCYAN"'...%s",IsHtml(p) ? "\016<TR><TH ALIGN=CENTER COLSPAN=2 BGCOLOR="HTML_TABLE_CYAN"><FONT SIZE=4><I>\016"ANSI_LCYAN:"\n ",logfs[logfile].name,IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
			  } else {
			     if(userlog > 0) output(p,player,2,1,1,"%sEntries in %s"ANSI_LWHITE"%s"ANSI_LCYAN"'s log file (Back-tracked by "ANSI_LWHITE"%d"ANSI_LCYAN" page%s)...%s",IsHtml(p) ? "\016<TR><TH ALIGN=CENTER COLSPAN=2 BGCOLOR="HTML_TABLE_CYAN"><FONT SIZE=4><I>\016"ANSI_LCYAN:"\n ",Article(userlog,LOWER,DEFINITE),getcname(NOTHING,userlog,0,0),page,Plural(page),IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
				else output(p,player,2,1,1,"%sEntries in log file '"ANSI_LWHITE"%s"ANSI_LCYAN"' (Back-tracked by "ANSI_LWHITE"%d"ANSI_LCYAN" page%s)...%s",IsHtml(p) ? "\016<TR><TH ALIGN=CENTER COLSPAN=2 BGCOLOR="HTML_TABLE_CYAN"><FONT SIZE=4><I>\016"ANSI_LCYAN:"\n ",logfs[logfile].name,page,Plural(page),IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
			  }
			  if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
		       }

		       count = 0, loop = gsize;
		       if(curpos <= 0) {
			  fseek((lf) ? lf:logfs[logfile].file,0,SEEK_SET);
			  output(p,player,2,1,3,"%s[***** \016&nbsp;\016 START OF LOG FILE \016&nbsp;\016 *****]%s",IsHtml(p) ? "\016<TR><TH ALIGN=CENTER COLSPAN=2 BGCOLOR="HTML_TABLE_BLACK"><I>\016"ANSI_LCYAN:ANSI_LCYAN" ",IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
		       }
	      
		       while((loop > 0) && fgets(buffer2,BUFFER_LEN - 128,(lf) ? lf:logfs[logfile].file)) {
			     ptr = (char *) strchr(buffer2,'\n');
			     if(ptr && (*ptr == '\n')) *ptr = '\0';
			     for(ptr = buffer2; *ptr && (*ptr == ' '); ptr++);
			     if(!(*ptr && !strncasecmp(ptr,"[***  ",6))) {
				title = ptr, ptr = (char *) strchr(ptr,':');
				custom = (*title == '[');
				if(!Blank(ptr)) {
				   for(text = (*ptr) ? ptr + 1:ptr; *text && (*text == ' '); text++);
				   *ptr = '\0';
				   output(p,player,2,1,strlen(title) + 4,"%s%s%s:%s"ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_GREY"><I>\016":" ",(custom) ? ANSI_LYELLOW:ANSI_LWHITE,title,IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",text,IsHtml(p) ? "\016</TD></TR>\016":"\n");
				} else output(p,player,2,1,3,"%s%s%s",IsHtml(p) ? "\016<TR><TH BGCOLOR="HTML_TABLE_BLACK">&nbsp;</TH><TD ALIGN=LEFT>\016"ANSI_DCYAN:ANSI_DCYAN" ",title,IsHtml(p) ? "\016</TD></TR>\016":"\n");
			     } else output(p,player,2,1,3,"%s%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=CENTER COLSPAN=2 BGCOLOR="HTML_TABLE_BLACK"><I>\016"ANSI_LGREEN:ANSI_LGREEN" ",ptr,IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
			     count++, loop--;
		       }

		       if(!in_command) {
			  if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
			  output(p,player,2,1,1,"%sLog file entries listed: \016&nbsp;\016 "ANSI_DWHITE"%d. \016&nbsp; &nbsp;\016 "ANSI_LWHITE"Log file size: \016&nbsp;\016 "ANSI_DWHITE"%.1f Kb.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TD COLSPAN=2 BGCOLOR="HTML_TABLE_BLUE">"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",count,(double) oldpos / KB,IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
		       }
		       if(IsHtml(p)) output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
		       fseek((lf) ? lf:logfs[logfile].file,oldpos,SEEK_SET);
		       if(lf || (!logfs[logfile].open && logfs[logfile].file)) {
			  fclose((lf) ? lf:logfs[logfile].file);
			  if(!userlog) logfs[logfile].file = NULL;
		       }

		       html_anti_reverse(p,0);
		       command_type = cached_command_type;
		       setreturn(OK,COMMAND_SUCC);
		    } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, only Deities and the Supreme Being can back-track a log file that far (Back-tracking log files by a large number of pages may cause excessive lag.)");
		 } else {
		    fprintf(stderr,"LOG:  Unable to open/create the log file '%s' for listing via '@log'.\n",(userlog > 0) ? buffer:logfs[logfile].filename);
		    if(userlog > 0) {
		       if(userlog == player) output(p,player,0,1,0,ANSI_LGREEN"Unable to open your log file ("ANSI_LYELLOW"%s"ANSI_LGREEN".)",strerror(errno));
			  else output(p,player,0,1,0,ANSI_LGREEN"Unable to open %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s log file ("ANSI_LYELLOW"%s"ANSI_LGREEN".)",Article(userlog,LOWER,DEFINITE),getcname(NOTHING,userlog,0,0),strerror(errno));
		    } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, unable to open the log file '"ANSI_LWHITE"%s"ANSI_LGREEN"' ("ANSI_LYELLOW"%s"ANSI_LGREEN".)",logfs[logfile].name,strerror(errno));
		 }
	      } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, only Deities may view the '"ANSI_LWHITE"%s"ANSI_LGREEN"' log file when the logging level may breech user privacy.",logfs[logfile].name);
	   } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, only %s may view the '"ANSI_LWHITE"%s"ANSI_LGREEN"' log file.",clevels[logfs[logfile].level],logfs[logfile].name);
	} else {
	   if(userlog) {
    	      if(Builder(player)) {
                 if(Level4(player)) output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only view or reset your own log file or the log file of a lower level character.");
	            else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only view or reset your own log file.");
	      } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, only Builders and above have their own log files.");
	   } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may view %s log files.",tcz_full_name);
	}
     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, log files can not be viewed from within a compound command.");
     logfile_current = NOTHING;
}
			     
/* ---->  {J.P.Boggis 14/05/1999}  Make entry in log file  <---- */
void logfile_logentry(CONTEXT)
{
     int    logfile = NOTHING,loop;
     char   buffer2[BUFFER_LEN];
     char   buffer[BUFFER_LEN];
     char   *title,*entry,*ptr;
     dbref  userlog = 0;

     setreturn(ERROR,COMMAND_FAIL);
     if(in_command && Valid(current_command)) {

        /* ---->  User log file  <---- */
	if((*arg1 == NUMBER_TOKEN) || (*arg1 == LOOKUP_TOKEN)) {
	   if((userlog = lookup_character(player,arg1,1)) <= 0) {
	      output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
	      return;
	   }
	} else if(Blank(arg1) || !strcasecmp("me",arg1))
	   userlog = (in_command) ? Owner(current_command):player;

	/* ---->  System log file (Admin owned compound command with WIZARD flag set.)  <---- */
	if(!userlog) {
	   for(loop = 0; (loop < LOG_COUNT) && (logfile == NOTHING); loop++)
	       if(string_prefix(logfs[loop].name,arg1)) logfile = loop;

           /* ---->  No access to system log file?  <---- */
           if(logfile != NOTHING) {
              if((Owner(player) != Owner(current_command)) || !Level4(Owner(current_command)) || !Wizard(current_command)) {
		 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, custom log file entries can only be made to the '"ANSI_LWHITE"%s"ANSI_LGREEN"' system log file from within a compound command with its "ANSI_LYELLOW"WIZARD"ANSI_LGREEN" flag set, which is owned by an Apprentice Wizard/Druid or above (Under their '"ANSI_LWHITE"@chpid"ANSI_LGREEN"'.)",logfs[logfile].name);
		 return;
	      }

	      /* ---->  System log files in which custom entries cannot be made?  <---- */
	      if(!Level1(player) && ((logfile == LOGENTRY_LOG) || (logfile == COMMAND_LOG) || (logfile == EMERGENCY_LOG))) {
		 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, custom entries cannot be made to the '"ANSI_LWHITE"Command"ANSI_LGREEN"', '"ANSI_LWHITE"Emergency"ANSI_LGREEN"' or '"ANSI_LWHITE"LogEntry"ANSI_LGREEN"' system log files.");
		 return;
	      }
	   }
	} else if(userlog > 0) {

           /* ---->  User log files disabled?  <---- */
           if(!option_userlogs(OPTSTATUS)) return;

           /* ---->  User log file must be owned by a Builder or above  <---- */
           if(!Builder(userlog)) {
	      output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" does not have a log file (They must be a Builder or above.)",Article(userlog,LOWER,INDEFINITE),getcname(NOTHING,userlog,0,0));
	      return;
	   }

           /* ---->  Owner of compound command doesn't have permission to write to specified user log file  <---- */
           if((Owner(current_command) != userlog) && ((Owner(player) == player) || !can_write_to(Owner(current_command),userlog,1))) {
              sprintf(buffer,"%s"ANSI_LWHITE"%s"ANSI_LGREEN,Article(Owner(current_command),LOWER,INDEFINITE),getcname(NOTHING,Owner(current_command),0,0));
              sprintf(buffer2,"%s"ANSI_LWHITE"%s"ANSI_LGREEN,Article(userlog,LOWER,INDEFINITE),getcname(NOTHING,userlog,0,0));
	      output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s (The owner of the compound command "ANSI_LYELLOW"%s"ANSI_LGREEN") doesn't have permission to write to %s's log file%s",buffer,unparse_object(player,current_command,0),buffer2,(Owner(player) == player) ? " (No '"ANSI_LWHITE"@chpid"ANSI_LGREEN"' in compound command.)":".");
              return;
	   }
	}

	/* ---->  Invalid log file specified?  <---- */
	if(!(userlog || (logfile != NOTHING))) {
	   output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the log file '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
	   return;
	}

        /* ---->  Make custom log file entry  <---- */
	split_params(arg2,&title,&entry);     

	if(!Blank(title)) {
	   ansi_code_filter(title,title,1);
	   filter_spaces(title,title,0);
	}

	if(!Blank(entry))
	   ansi_code_filter(entry,entry,1);
     
	if(!Blank(title)) {

	   /* ---->  Validate title  <---- */
	   for(ptr = title, loop = 0; !Blank(ptr); ptr++, loop++)
	       if(!(isalnum(*ptr) || (*ptr == ' '))) {
		  output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a log file entry title may only consist of alphanumeric text.");
		  return;
	       } else if(loop > 15) {
		  output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a log file entry title cannot exceed "ANSI_LWHITE"15 characters"ANSI_LGREEN" in length.");
		  return;
	       } else *ptr = toupper(*ptr);

	   if(!Blank(entry)) {
	      *entry = toupper(*entry);
	      if(userlog) {
                 if(Owner(current_command) != userlog)
                    writelog(LOGENTRY_LOG,1,"LOGENTRY","%s(#%d) made an entry to %s(#%d)'s log file with the title '[%s]:' from within compound command %s(#%d) owned by %s(#%d).",getname(player),player,getname(userlog),userlog,title,getname(current_command),current_command,getname(Owner(current_command)),Owner(current_command));
	      } else writelog(LOGENTRY_LOG,1,"LOGENTRY","%s(#%d) made an entry in the '%s' log file with the title '[%s]:' from within compound command %s(#%d) owned by %s(#%d).",getname(player),player,logfs[logfile].name,title,getname(current_command),current_command,getname(Owner(current_command)),Owner(current_command));

	      /* ---->  Remove newlines from custom log file entry (Screws up formatting of log file when using '@log')  <---- */
	      for(ptr = entry; *ptr; ptr++)
		  if(*ptr == '\n') *ptr = ' ';

	      sprintf(buffer,"[%s]",title);
              writelog((!userlog) ? logfile:UserLog(userlog),1,buffer,"%s",entry); 
	      setreturn(OK,COMMAND_SUCC);
	   } else {
              if(!userlog) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a blank entry cannot be made to the '"ANSI_LWHITE"%s"ANSI_LGREEN"' log file.",logfs[logfile].name);
                 else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a blank entry cannot be made to %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s log file.",Article(userlog,LOWER,INDEFINITE),getcname(NOTHING,userlog,0,0));
	   }
	} else {
           if(!userlog) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a title must be specified to make an entry to the '"ANSI_LWHITE"%s"ANSI_LGREEN"' log file.",logfs[logfile].name);
              else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a title must be specified to make an entry to %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s log file.",Article(userlog,LOWER,INDEFINITE),getcname(NOTHING,userlog,0,0));
	}
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, log file entries can only be made from within a compound command.");
}

