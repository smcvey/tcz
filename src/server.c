/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| SERVER.C  -  Implements the bulk of TCZ server, including handling of       |
|              Telnet connections and initial handling of HTML connections.   |
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
|            Additional major coding by:  J.P.Boggis 28/12/1994.              |
|           Improved Telnet handling by:  J.P.Boggis 28/10/1995.              |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                        https://github.com/smcvey/tcz                        |
`-----------------------------------------------------------------------------'

*/


#include <sys/resource.h>
#include <arpa/telnet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "descriptor_flags.h"
#include "object_types.h"
#include "signallist.h"
#include "flagset.h"
#include "fields.h"
#include "html.h"

#ifndef CYGWIN32
   #include <crypt.h>
#endif


/* ---->  Telnet commands  <---- */
#define _QIS    "\x00"  /*  TELQUAL_IS    (Option is...)           */
#define _QSEND  "\x01"  /*  TELQUAL_SEND  (Send option)            */
#define _ECHO   "\x01"  /*  Echo control                           */
#define _MODE   "\x01"  /*  LINEMODE:  Change mode                 */
#define _SGA    "\x03"  /*  Suppress Go Ahead                      */
#define _TTYPE  "\x18"  /*  Terminal type                          */
#define _NEGEOR "\x19"  /*  End Of Record negotiation              */
#define _NAWS   "\x1F"  /*  Window size                            */
#define _LFLOW  "\x21"  /*  Remote flow control                    */
#define _LINE   "\x22"  /*  Line-by-line mode                      */
#define _EOR    "\xEF"  /*  End Of Record                          */
#define _SE     "\xF0"  /*  Sub-negotiation End                    */
#define _EC     "\xF7"  /*  Erase Character                        */
#define _SB     "\xFA"  /*  Sub-negotiation Begin                  */
#define _WILL   "\xFB"  /*  Remote end will use option             */
#define _WONT   "\xFC"  /*  Remote end will not use option         */
#define _DO     "\xFD"  /*  Remote end should use option           */
#define _DONT   "\xFE"  /*  Remote end should not use option       */
#define _IAC    "\xFF"  /*  Interpret As Command                   */


/* ---->  Descriptor list handling control  <---- */
static  unsigned char   reset_list        = 0;

struct  descriptor_data *descriptor_list  = NULL;
static  int             ndescriptors      = 0;
dbref                   current_character = NOTHING;
const   char            *current_cmdptr   = NULL;
char                    **output_fmt;  /*  fmt pointer used for passing external fmt pointer to output()  */
va_list                 output_ap;     /*  va_list variable used for passing external va_list to output()  */

int                     telnet            = -1;   /*  Socket for incoming Telnet connections  */
int                     html              = -1;   /*  Socket for incoming HTML connections  */


/* ---->  UPS support  <---- */
int     powerstate = 1;
int     powercount = 0;
time_t  powertime  = 0;


/* ---->  Allow time adjustment (Disabled during forked DB dump)  <---- */
unsigned char auto_time_adjust = 1;


/* ---->  Get current system time, checking for large variation (I.e:  Daylight saving, time changed on server, etc.)  <---- */
time_t server_gettime(time_t *tm,int check_tz)
{
       time_t               current,adjustment;
       static unsigned char initialised = 0;
       static time_t        lasttime    = 0;
       static time_t        tz_check    = 0;
       unsigned char        adjusted    = 0;
       static int           called      = 0;
       int                  dst;

       current = time(NULL);

#ifdef AUTO_TIME_ADJUST
       if(auto_time_adjust) {
          if(initialised && !called) {

             /* ---->  Check for server system time adjustment or Daylight Saving Time  <---- */
             adjustment = (current - lasttime);
             if((adjustment < 0) || (adjustment > (TIME_VARIATION * MINUTE))) {
                called++;
                timeadjust -= adjustment;

                /* ---->  Daylight Saving Time?  <---- */
                if((abs(adjustment) >= (HOUR - DST_THRESHOLD)) && (abs(adjustment) <= (HOUR + DST_THRESHOLD))) {
                   admin_time_adjust((adjustment >= 0) ? HOUR:(0 - HOUR));
                   dst = 1;
	        } else dst = 0;
                tz_check = 0;

                /* ---->  Log and warn administrators  <---- */
                writelog(SERVER_LOG,1,"TIME","The system time has changed by (%c) %dd %dh %dm %ds (%s.)  Time on %s adjusted to compensate  -  Please check that the time is correct and adjust if neccessary using '@admin time'.",(adjustment < 0) ? '-':'+',ABS(adjustment) / DAY,(ABS(adjustment) % DAY) / HOUR,(ABS(adjustment) % HOUR) / MINUTE,ABS(adjustment) % MINUTE,(dst) ? "Due to Daylight Saving Time":"Due to manual adjustment from the server",tcz_short_name);
	        writelog(ADMIN_LOG,1,"TIME","The system time has changed by (%c) %dd %dh %dm %ds (%s.)  Time on %s adjusted to compensate  -  Please check that the time is correct and adjust if neccessary using '@admin time'.",(adjustment < 0) ? '-':'+',ABS(adjustment) / DAY,(ABS(adjustment) % DAY) / HOUR,(ABS(adjustment) % HOUR) / MINUTE,ABS(adjustment) % MINUTE,(dst) ? "Due to Daylight Saving Time":"Due to manual adjustment from the server",tcz_short_name);
	        output_admin(0,0,0,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"The system time has changed by "ANSI_LMAGENTA"(%c) "ANSI_LYELLOW"%dd %dh %dm %ds"ANSI_LWHITE" (%s.) \016&nbsp;\016 Time on %s adjusted to compensate \016&nbsp;\016 - \016&nbsp;\016 Please check that the time is correct and adjust if neccessary using '"ANSI_LGREEN"@admin time"ANSI_LWHITE"'.\n",(adjustment < 0) ? '-':'+',ABS(adjustment) / DAY,(ABS(adjustment) % DAY) / HOUR,(ABS(adjustment) % HOUR) / MINUTE,ABS(adjustment) % MINUTE,(dst) ? "Due to Daylight Saving Time":"Due to manual adjustment from the server",tcz_short_name);
                if(called > 0) called--;
                adjusted = 1;
	     } else adjusted = 0;

             /* ---->  Check time zone  <---- */
             if(check_tz && (current > (tz_check + HOUR))) {
                tcz_get_timezone(adjusted,0);
                tz_check = current + HOUR;
	     }
          } else initialised = 1;
          lasttime = current;
       }
#endif

       /* ---->  Return current time with adjustment taken into account  <---- */
       if(tm) *tm = (current + timeadjust);
       return(current + timeadjust);
}

/* ---->  Log last commands typed by current connected characters to 'Command' log file  <---- */
void server_log_commands(void)
{
     struct descriptor_data *d;
     int    logtime = 1;
    
     for(d = descriptor_list; d; d = d->next) {
         if(d->flags & CONNECTED) {
            writelog(COMMAND_LOG,logtime,"SHUTDOWN","Last command typed by %s(#%d) was '%s'.",getname(d->player),d->player,decompress(d->last_command));
            logtime = 0;
	 }
     }
}

/* ---->  When a character connects, increase total no. users and sort out peak no. users  <---- */
void server_connect_peaktotal(void)
{
     struct descriptor_data *d;
     int    count = 0;

     /* ---->  Count total number of users currently connected  <---- */
     for(d = descriptor_list; d; d = d->next)
         if(d->flags & CONNECTED) count++;

     /* ---->  Now see if we've beaten the peak number of users...  <---- */
     stats_tcz_update_record(count,0,0,0,0,0,0);
}

/* ---->  Count number of connections currently in use by given character  <---- */
int server_count_connections(dbref player,unsigned char html)
{
    struct descriptor_data *d;
    int    count = 0;

    for(d = descriptor_list; d; d = d->next)
        if((d->flags & CONNECTED) && (d->player == player) && (html || !IsHtml(d)))
            count++;
    return(count);
}

/* ---->  Remove descriptor from list and re-insert in the correct order (By login time)  <---- */
void server_sort_descriptor(struct descriptor_data *d)
{
     struct descriptor_data *ptr,*last = NULL;

     /* ---->  Remove descriptor from list  <---- */
     if(d->prev) {
        if(d->next) d->prev->next = d->next;
           else d->prev->next = NULL;
     }

     if(d->next) {
        if(d->prev) d->next->prev = d->prev;
           else d->next->prev = NULL;
     }
     if(d == descriptor_list) descriptor_list = d->next;

     /* ---->  Find correct position and place back into list  <---- */
     for(ptr = descriptor_list; ptr && (d->start_time < ptr->start_time); last = ptr, ptr = ptr->next);
     if(last) last->next = d, d->prev = last;
        else descriptor_list = d, d->prev = NULL;
     if(ptr) d->next = ptr, ptr->prev = d;
        else d->next = NULL;
}

#ifdef GUARDIAN_ALARM

/* ---->  Alarm signal handler  <---- */
void server_SIGALRM_handler(int sig)
{
     char buffer[TEXT_SIZE];

     writelog(SERVER_LOG,1,"GUARDIAN ALARM","User command/IO processing loop has taken more than %s to complete.  This is most likely caused by a bug in the source code that has created an infinite loop.  Please check the core dump to determine the cause.  %s will be terminated in 10 seconds time (SIGSEGV.)",interval(GUARDIAN_ALARM_TIME,0,ENTITIES,0),tcz_short_name);
     writelog(BUG_LOG,1,"GUARDIAN ALARM","User command/IO processing loop has taken more than %s to complete.  This is most likely caused by a bug in the source code that has created an infinite loop.  Please check the core dump to determine the cause.  %s will be terminated in 10 seconds time (SIGSEGV.)",interval(GUARDIAN_ALARM_TIME,0,ENTITIES,0),tcz_short_name);

     sprintf(buffer,"sleep 10; kill -SIGSEGV %d",getpid());
     popen(buffer,"r");
}

#endif

/* ---->  Arithmetic exception handler  <---- */
void server_SIGFPE_handler(int sig)
{
     command_type |= ARITHMETIC_EXCEPTION;
     signal(SIGFPE,server_SIGFPE_handler);
}

/* ---->  Handler for SIGCONT (Leave idle state)  <---- */
void server_SIGCONT_handler(int sig)
{
     writelog(SERVER_LOG,1,"SERVER","Caught signal SIGCONT  -  Leaving %sidle state.",(idle_state == 2) ? "":"semi-");
     gettime(activity);
     idle_state = 0;
     signal(SIGCONT,server_SIGCONT_handler);
}

/* ---->  Handler for SIGUSR1 (External database dump restart)  <---- */
void server_SIGUSR1_handler(int sig)
{
     dumpstatus = 255;
     db_write(NULL);
     writelog(SERVER_LOG,1,"SERVER","Caught signal SIGUSR1 (External database dump restart)  -  Restarting database dump.");
     dumpstatus = 1;
     dumptype   = DUMP_SANITISE;
     db_write(NULL);
     if(dumpstatus <= 0) writelog(SERVER_LOG,1,"SERVER","Unable to open/write to database dump file  -  Database dump aborted.");
     signal(SIGUSR1,server_SIGUSR1_handler);
}

/* ---->  Handler for SIGUSR2 (Enter idle state)  <---- */
void server_SIGUSR2_handler(int sig)
{
     idle_state = 2;
     writelog(SERVER_LOG,1,"SERVER","Caught signal SIGUSR2  -  Entering idle state.");
     while(idle_state == 2) sleep(IDLE_STATE_DELAY);
     signal(SIGUSR2,server_SIGUSR2_handler);
}

#ifdef UPS_SUPPORT

/* ---->  {J.P.Boggis 26/08/2000}  Handler for SIGPWR (Power fail/restart)  <---- */
void server_SIGPWR_handler(int sig)
{
     struct descriptor_data *d;

     powerstate = !powerstate;
     if(!powerstate) {

        /* ---->  Power lost  <---- */
        powercount++;
        for(d = descriptor_list; d; d = d->next) {
            if((d->flags & CONNECTED) && Validchar(d->player) && Connected(d->player)) {
               if(Level4(d->player))
                  output(d,d->player,0,0,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LYELLOW"Power to the server on which %s runs has been lost for the "ANSI_LWHITE"%s time"ANSI_LYELLOW".  The server is now running on "ANSI_UNDERLINE"battery backup"ANSI_LYELLOW", which may run out if mains power is not resumed again soon.\n\nA database dump %s.  If the power fails, any changes made to the database since "ANSI_LWHITE"%s"ANSI_LYELLOW" (The time of the last successful database dump) may be lost.\n\nWhen mains power is resumed, you will receive notification of this.  If you suddenly get disconnected, the battery backup has probably run out of power (Try connecting again later when mains power has been resumed.)\n",tcz_full_name,rank(powercount),(!dumpstatus) ? "has been started":"is already in progress",date_to_string((dumptiming) ? dumptiming:uptime,UNSET_DATE,d->player,FULLDATEFMT));
	             else output(d,d->player,0,0,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LYELLOW"Power to the server on which %s runs has been lost.  The server is now running on "ANSI_UNDERLINE"battery backup"ANSI_LYELLOW", which may run out if mains power is not resumed again soon.\n\nA database dump %s.  If the power fails, any building work or changes made to your character, objects, areas, etc. since "ANSI_LWHITE"%s"ANSI_LYELLOW" (The time of the last successful database dump) may be lost.\n\nWhen mains power is resumed, you will receive notification of this.  If you suddenly get disconnected, the battery backup has probably run out of power (Try connecting again later when mains power has been resumed.)\n",tcz_full_name,(!dumpstatus) ? "has been started":"is already in progress",date_to_string((dumptiming) ? dumptiming:uptime,UNSET_DATE,d->player,FULLDATEFMT));
	    }
	}

        writelog(SERVER_LOG,1,"POWER","Power to host server has been lost for the %s time.  Running off battery backup (Database dump %s.)",rank(powercount),(!dumpstatus) ? "started":"already in progress");
        writelog(ADMIN_LOG,1,"POWER","Power to host server has been lost for the %s time.  Running off battery backup (Database dump %s.)",rank(powercount),(!dumpstatus) ? "started":"already in progress");
        gettime(powertime);

        if(dumpstatus == 0) {

           /* ---->  Start database dump, if not already in progress  <---- */
	   dumpstatus = 1;
	   dumptype   = DUMP_SANITISE;
#ifdef DB_FORK
	   if(option_forkdump(OPTSTATUS)) {
	      dumpstatus = 254;
	      dumptype   = DUMP_NORMAL;
	   }
#endif
	}
     } else {
        time_t now;

        /* ---->  Power resumed  <---- */
        gettime(now);
        output_all(1,0,0,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LYELLOW"Mains power has now been resumed.  It is now safe to make changes to your character, objects, areas, etc. again.  However, please wait for atleast "ANSI_LWHITE"5-10 minutes"ANSI_LYELLOW" to be sure that the power will not be lost again.\n");
        output_all(0,1,0,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LYELLOW"Mains power has now been resumed (Total time without power:  "ANSI_LWHITE"%s"ANSI_LYELLOW", Total number of power failures:  "ANSI_LWHITE"%d"ANSI_LYELLOW".)  It is now safe to make changes to the database again.  However, please wait for atleast "ANSI_LWHITE"5-10 minutes"ANSI_LYELLOW" to be sure that the power will not be lost again.\n",interval(now - powertime,0,ENTITIES,0),powercount);
        writelog(SERVER_LOG,1,"POWER","Power to host server has been resumed (Total time without power:  %s, Total number of power failures:  %d.)",interval(now - powertime,0,ENTITIES,0),powercount);
        writelog(ADMIN_LOG,1,"POWER","Power to host server has been resumed (Total time without power:  %s, Total number of power failures:  %d.)",interval(now - powertime,0,ENTITIES,0),powercount);
     }

     signal(SIGPWR,server_SIGPWR_handler);
}

#endif  /*  UPS_SUPPORT  */

#ifdef DB_FORK

/* ---->  Handler for SIGCHLD (Child process terminated)  <---- */
void server_SIGCHLD_handler(int sig)
{
     int status;

     while(waitpid(-1,&status,WNOHANG) > 0);
     if((dumpchild > 0) && (!WIFEXITED(status) || (WEXITSTATUS(status) != 0))) {

	/* ---->  Child dumping process did not complete - Attempt to dump database internally  <---- */
	writelog(DUMP_LOG,1,"DUMP","Database dumping process has crashed/terminated  -  Attempting to dump database internally...");
	writelog(SERVER_LOG,1,"DUMP","Database dumping process has crashed/terminated  -  Attempting to dump database internally...");
	output_admin(0,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 \007"ANSI_LYELLOW"Database dumping process has crashed/terminated  -  Attempting to dump database internally in the background...");

	dumpchild  = NOTHING;
	dumpstatus = 1;
	dumptype   = DUMP_SANITISE;
	signal(SIGCHLD,server_SIGCHLD_handler);
	return;
     }

     if(dumpchild != NOTHING) {

	/* ---->  Update database dump statistics and schedule next dump  <---- */
	dumpstatus = 100;
	db_write(NULL);
	dumpchild = NOTHING;
     }
     signal(SIGCHLD,server_SIGCHLD_handler);
}

#endif  /*  DB_FORK  */

/* ---->  Process queued output (Send to descriptor)  <---- */
int server_process_output(struct descriptor_data *d)
{
    int cnt;

#define HTML_JAVA_SCROLLBY "<SCRIPT LANGUAGE=\"JavaScript\">\n<!--\nscrl2()\n//-->\n</SCRIPT>"
#define HTML_JAVA_SCROLL   "<SCRIPT LANGUAGE=\"JavaScript\">\n<!--\nscrl1()\n//-->\n</SCRIPT>"

    if(d->descriptor == NOTHING) return(1);
    if((d->flags2 & OUTPUT_PENDING) && IsHtml(d) && (d->html->flags & HTML_OUTPUT) && (d->html->flags & HTML_JAVA)) {

       /* ---->  Scroll HTML output window  <---- */
       if(d->html->flags & HTML_SCROLL)
          server_queue_output(d,HTML_JAVA_SCROLL,sizeof(HTML_JAVA_SCROLL));
       
       if(d->html->flags & HTML_SCROLLBY)
          server_queue_output(d,HTML_JAVA_SCROLLBY,sizeof(HTML_JAVA_SCROLLBY));

       d->flags2 &= ~OUTPUT_PENDING;
    }

    while(d->output.start) {

          /* ---->  Deliver output  <---- */
          if((cnt = write(d->descriptor,d->output.start->text,d->output.start->len)) < 0) {
             if(errno == EWOULDBLOCK) return(1);
             return(0);
	  }
	
          /* ---->  Update outgoing bandwidth usage  <---- */
          lower_bytes_out += cnt;
          while(lower_bytes_out > KB) lower_bytes_out -= KB, upper_bytes_out++;

          /* ---->  Free text queue entry  <---- */
          if(cnt < d->output.start->len) {
             char *buffer;

             NMALLOC(buffer,char,d->output.start->len - cnt);
             memcpy(buffer,d->output.start->text + cnt,d->output.start->len - cnt);
             FREENULL(d->output.start->text);
             d->output.start->text = buffer;
             d->output.start->len -= cnt;
             d->output.size       -= cnt;
	  } else {
             struct text_data *next;

             d->output.size -= d->output.start->len;
             next            = d->output.start->next;
             FREENULL(d->output.start->text);
             FREENULL(d->output.start);
             if(!(d->output.start = next)) d->output.end = NULL;
	  }
    }

    if(!(d->output.start) && (d->flags & DELIVERED)) {
       shutdown(d->descriptor,2);
       close(d->descriptor);
       d->descriptor = NOTHING;
       d->flags     |= DISCONNECTED;
    }
    return(1);
}

/* ---->  Process input from Telnet/HTML connections  <---- */
int server_process_input(struct descriptor_data *d,unsigned char html)
{
    static unsigned char  *t,*p,*pend,*q,*qend;
    static char           tbuffer[BUFFER_LEN];
    static unsigned char  buffer[BUFFER_LEN];
    static char           tbuf[3];
    static unsigned short adjust;
    static char           *ptr;
    static int            got;

    /* ---->  Read input  <---- */
    adjust = 0, got = read(d->descriptor,buffer,BUFFER_LEN);
    if(got >= BUFFER_LEN) got = BUFFER_LEN - 1;
    buffer[got] = 0;
    if(got <= 0) return(0);

    /* ---->  Update incoming bandwidth usage  <---- */
    lower_bytes_in += got;
    while(lower_bytes_in > KB) {
          lower_bytes_in -= KB;
          upper_bytes_in++;
    }

#ifdef HTML_INTERFACE
    if(html) {
       html_process_input(d,buffer,got);
       if(IsHtml(d) && !(d->html->flags & HTML_INPUT_PENDING))
          if(!html_process_data(d)) return(0);
       return(1);
    }
#endif

    if(!(d->flags & CONNECTED) && (option_loglevel(OPTSTATUS) >= 6))
       writelog(COMMAND_LOG,1,"TELNET","[%d]  %s",d->descriptor,binary_to_ascii(buffer,(got > KB) ? KB:got,tbuffer));

    /* ---->  'Split' Telnet sub-negotation in progress?  <---- */
    if(d->flags & TELNET_SUBNEG) {
       unsigned short count = 0,loop = 0,loop2 = 0;

       for(t = buffer; (count < got) && (*t != SE); t++, count++);
       if((count < got) && (*t == SE)) {

          /* ---->  Sub-negotiation complete  <---- */
          t++, count++, q = d->negbuf;
          NMALLOC(d->negbuf,unsigned char,d->neglen + count);
          for(loop = 0, t = q; loop < d->neglen; d->negbuf[loop] = *t, loop++, t++);
          FREENULL(q);
          for(t = buffer; loop2 < count; d->negbuf[loop] = *t, loop++, loop2++, t++);
          adjust = count;

          qend = d->negbuf + loop;
          if((q = d->negbuf + 1) < qend) switch(*q) {
             case TELOPT_NAWS:

                  /* ---->  Window size  <---- */
                  if(++q < qend) {
                     d->terminal_width = MIN(MAX((*q * 256) + *(q + 1) - 1,79),255);
                     if((q += 2) < qend) d->terminal_height = MIN(MAX((*q * 256) + *(q + 1),10),255);
                     for(; (q < qend) && (*q != SE); q++);
                     if((q < qend) && (*q == SE)) q++;
		  }
                  break;
             case TELOPT_TTYPE:

                  /* ---->  Terminal type  <---- */
                  if((q += 2) < qend) {
                     unsigned char buflen = 0;
                     char          buffer[32];

                     for(; (q < qend) && *q && (*q != IAC) && (*q != SE) && (buflen < 31); buffer[buflen] = *q, buflen++, q++);
                     buffer[buflen] = '\0';
                     set_terminal_type(d,buffer,1);
                     for(; (q < qend) && (*q != SE); q++);
                     if((q < qend) && (*q == SE)) q++;
		  }
                  break;
             default:

                  /* ---->  Seek terminating SE  <---- */
                  for(; (q < qend) && (*q != SE); q++);
                  if((q < qend) && (*q == SE)) q++;
	  }
          d->flags &= ~TELNET_SUBNEG;
          FREENULL(d->negbuf);
          d->neglen = 0;
       } else {

          /* ---->  Sub-negotiation split into further parts  <---- */
          q = d->negbuf;
          NMALLOC(d->negbuf,unsigned char,d->neglen + count);
          for(loop = 0, t = q; loop < d->neglen; d->negbuf[loop] = *t, loop++, t++);
          FREENULL(q);
          for(t = buffer; loop2 < count; d->negbuf[loop] = *t, loop++, loop2++, t++);
          d->neglen = loop;
          return(1);
       }
    }

    /* ---->  Allocate unprocessed input buffer  <---- */
    if(!d->raw_input) {
       NMALLOC(d->raw_input,unsigned char,TEXT_SIZE);
       d->raw_input_at = d->raw_input;
    }
    p    = d->raw_input_at;
    pend = d->raw_input + TEXT_SIZE - 1;

    /* ---->  Process input  <---- */
    for(q = buffer + adjust, qend = buffer + got; q < qend; q++)
        switch(*q) {

               /* ---->  Backspace/delete  <---- */
               case '\x08':
               case '\x7F':
                    if((d->flags & LOCAL_ECHO) && !(d->flags & SUPPRESS_ECHO)) write(d->descriptor,"\x08 \x08",3);
                    if(p > d->raw_input) {
                       p--;
                       *p = '\0';
		    }
                    break;

               /* ---->  Backslash?  <---- */
               case '\\':
                    if(!(d->flags & EVALUATE)) {
                       if(p < pend) *p++ = *q;
		    } else if(d->flags & BACKSLASH) {
                       if((p + 1) < pend) {
                          *p++ = '\\';
                          *p++ = *q;
		       }
                       d->flags &= ~BACKSLASH;
		    } else d->flags |= BACKSLASH;
                    break;

               /* ---->  Newline/Carriage return?  (TCZ supports the end-of-line termination sequences:  LF, CR, LF+CR, CR+LF)  <---- */
               case '\r':
               case '\n':
                    if(d->flags & BACKSLASH) {
                       if(p < pend) *p++ = '\n';
                       d->flags &= ~BACKSLASH;
	  	    } else {
                       *p = '\0';
                       if(p >= d->raw_input)
			   server_queue_input(d,d->raw_input,strlen(d->raw_input) + 1);
                       p = d->raw_input;
		    }
                
                    /* ---->  Handle following LF or CR correctly (Auto-setting end-of-line termination sequence if neccessary)  <---- */
                    if((*q == '\r') && ((q + 1) < qend) && (*(q + 1) == '\n')) {
                       if(!(d->flags & TERMINATOR_MASK)) d->flags |= LFTOCR_CRLF;
                       q++;
  		    } else if((*q == '\n') && ((q + 1) < qend) && (*(q + 1) == '\r')) {
                       if(!(d->flags & TERMINATOR_MASK)) d->flags |= LFTOCR_LFCR;
                       q++;
		    } else if((*q == '\r') && !(d->flags & TERMINATOR_MASK)) d->flags |= LFTOCR_CR;
                    if((d->flags & LOCAL_ECHO) && !(d->flags & SUPPRESS_ECHO)) {
                       ptr = tbuf;
                       output_terminate(&ptr,d->flags & TERMINATOR_MASK,NULL,NULL);
                       *ptr = '\0';
                       write(d->descriptor,tbuf,strlen(tbuf));
		    }
                    break;
               case IAC:

                    /* ---->  TELNET command  <---- */
                    if(++q < qend) switch(*q) {
                       case DO:
                            if(++q < qend) switch(*q) {
                               case TELOPT_SGA:
                                    write(d->descriptor,_IAC""_WILL""_SGA,3);
                                    break;
                               case TELOPT_EOR:
                                    server_queue_output(d,_IAC""_EOR,2);
                                    d->flags |= TELNET_EOR;
                                    break;
                               default:
                                    break;
			    }
                            break;
                       case DONT:
                            q++;
                            break;
                       case WILL:
                            if(++q < qend) switch(*q) {
                               case TELOPT_LINEMODE:
                                    write(d->descriptor,_IAC""_SB""_LINE""_MODE"\x03"_IAC""_SE,7);
                                    break;
                               case TELOPT_TTYPE:
                                    write(d->descriptor,_IAC""_SB""_TTYPE""_QSEND""_IAC""_SE,6);
                                    break;
                               default:
                                    break;
			    }
                            break;
                       case WONT:
                            q++;
                            break;
                       case SB:

                            /* ---->  Begin sub-negotiation  <---- */
                            for(d->neglen = 0, t = q; (t < qend) && (*t != SE); t++, d->neglen++);
                            if(t >= qend) {
                               unsigned short loop = 0;

                               /* ---->  'Split' negotiation  <---- */
                               FREENULL(d->negbuf);
                               NMALLOC(d->negbuf,unsigned char,d->neglen);
                               for(; (q < qend) && (loop < d->neglen); d->negbuf[loop] = *q, loop++, q++);
                               d->flags |= TELNET_SUBNEG;
                               FREENULL(d->raw_input);
                               d->raw_input_at = NULL;
                               return(1);
			    } else if(++q < qend) switch(*q) {

                               /* ---->  Normal negotiation  <---- */
                               case TELOPT_NAWS:

                                    /* ---->  Window size  <---- */
                                    if(++q < qend) {
                                       d->terminal_width = MIN(MAX((*q * 256) + *(q + 1) - 1,79),255);
                                       if((q += 2) < qend) d->terminal_height = MIN(MAX((*q * 256) + *(q + 1),10),255);
                                       for(; (q < qend) && (*q != SE); q++);
				    }
                                    break;
                               case TELOPT_TTYPE:

                                    /* ---->  Terminal type  <---- */
                                    if((q += 2) < qend) {
                                       unsigned char buflen = 0;
                                       char          buffer[32];

                                       for(; (q < qend) && *q && (*q != IAC) && (*q != SE) && (buflen < 31); buffer[buflen] = *q, buflen++, q++);
                                       buffer[buflen] = '\0';
                                       set_terminal_type(d,buffer,1);
                                       for(; (q < qend) && (*q != SE); q++);
				    }
                                    break;
                               default:

                                    /* ---->  Seek terminating SE  <---- */
                                    for(; (q < qend) && (*q != SE); q++);
			    }
                            break;
                       case AYT:

                            /* ---->  Are You There?  <---- */
                            if(1) {
                               char buffer[128];
                               char *ptr;

                               sprintf(buffer,"%s[%s is still here! :-)]%s",(d->flags & ANSI) ? (d->flags & ANSI8) ? ANSI_DGREEN:ANSI_LGREEN:"",tcz_full_name,(d->flags & (ANSI|ANSI8)) ? ANSI_DWHITE:"");
                               ptr = buffer + strlen(buffer);
                               output_terminate(&ptr,d->flags & TERMINATOR_MASK,NULL,NULL);
                               *ptr = '\0';
                               write(d->descriptor,buffer,strlen(buffer) + 1);
			    }
                            break;
		    }
                    break;
               default:

                    /* ---->  Ordinary ASCII text (No control codes/extended codes allowed)  <---- */
                    if((p < pend) && isascii(*q) && isprint(*q)) {
                       if((d->flags & LOCAL_ECHO) && !(d->flags & SUPPRESS_ECHO)) write(d->descriptor,q,1);
                       if(d->flags & BACKSLASH) {
                          if(p + 1 < pend) *p++ = '\\';
                          d->flags &= ~BACKSLASH;
		       }
                       *p++ = *q;
	 	    }
	}

    /* ---->  If there's still some input pending, remember what it is;  otherwise, free the pending buffer  <---- */
    if(!(p > d->raw_input)) {
       FREENULL(d->raw_input);
       d->raw_input_at = NULL;
    } else d->raw_input_at = p;
    return(1);
}

/* ---->  Shutdown all sockets (User connections and Telnet/HTML sockets)  <---- */
void server_close_sockets()
{
     struct descriptor_data *d,*dnext;

     wrap_leading = 0;
     for(d = descriptor_list; d; d = dnext) {
         dnext = d->next;
         if(!(IsHtml(d) && !(d->html->flags & HTML_OUTPUT))) {
            output(d,d->player,2,0,0,"%s",bootmessage);
            server_process_output(d);
	 }
         if(d->descriptor) shutdown(d->descriptor,2);
         close(d->descriptor);
     }

     if(html   >= 0) close(html);
     if(telnet >= 0) close(telnet);
}

/* ---->  Signal exception handler for emergency database dump (Indicates corrupt emergency database dump, which should not be used)  <---- */
void server_emergency_exception(int sig)
{
     char *panicfrom,*panicto;
     char panicbuffer[1024];

     writelog(SERVER_LOG,1,"SHUTDOWN","Caught signal %d (%s:  %s) during emergency database dump  -  Unable to dump database.",sig,SignalName(sig),SignalDesc(sig));
     logfile_close();

     /* ---->  Move 'DBname.PANIC' to 'DBname.CORRUPT'  <---- */
     sprintf(panicfrom = panicbuffer,"%s.PANIC",dumpfile);
     sprintf(panicto   = (panicbuffer + 512),"%s.CORRUPT",dumpfile);
     rename(panicfrom,panicto);

     /* ---->  Remove lock file  <---- */
     if(lockfile) unlink(lockfile);

     /* ---->  Generate core dump  <---- */
     if(option_coredump(OPTSTATUS)) {
	signal(SIGABRT,SIG_DFL);
	abort();
     }
     _exit(1);
}

/* ---->  General signal exception handler (Called if TCZ server crashes, database intergrity is breached, or some other exception is raised, performing an emergency database dump and then shutting down TCZ server.)  <---- */
void server_emergency_dump(const char *message,int sig)
{
     struct descriptor_data *p = getdsc(current_character);
     int    i,exitcode,error,critical;
     static int called = 0;
     time_t now;

#ifdef RESTRICT_MEMORY
     struct rlimit limit;
#endif

     gettime(now);
     if(!called) {

#ifdef RESTRICT_MEMORY

        /* ---->  Remove data memory restriction  <---- */
        getrlimit(RLIMIT_DATA,&limit);
        limit.rlim_cur = RLIM_INFINITY;
        setrlimit(RLIMIT_DATA,&limit);

        /* ---->  Remove Resident Set Size (RSS) memory restriction  <---- */
        getrlimit(RLIMIT_RSS,&limit);
        limit.rlim_cur = RLIM_INFINITY;
        setrlimit(RLIMIT_RSS,&limit);
#endif

        /* ---->  Log emergency shutdown details  <---- */
        called = 1, log_stderr = 1;
        writelog(SERVER_LOG,0,"SHUTDOWN","Emergency shutdown on %s (Total uptime:  %s.)",date_to_string(now,UNSET_DATE,NOTHING,FULLDATEFMT),interval(now - uptime,0,ENTITIES,0));
        writelog(SERVER_LOG,1,"SHUTDOWN","%s",message);

        /* ---->  Log current command being executed at time of emergency shutdown  <---- */
        if(Validchar(current_character)) writelog(SERVER_LOG,0,"SHUTDOWN","Current command being executed was '%s' by %s(#%d)%s in %s(#%d).",!Blank(current_cmdptr) ? current_cmdptr:"<UNKNOWN>",getname(current_character),current_character,(p && p->edit) ? " (Editing)":"",getname(db[current_character].location),db[current_character].location);
           else writelog(SERVER_LOG,0,"SHUTDOWN","Current command being executed was '%s'.",!Blank(current_cmdptr) ? current_cmdptr:"<UNKNOWN>");
     } else {
        writelog(SERVER_LOG,1,"SHUTDOWN","%s",message);
        server_emergency_exception(SIGABRT);
     }

     /* ---->  Log current command command being executed (If executing compound command)  <---- */
     if(in_command && Valid(current_command)) writelog(SERVER_LOG,0,"SHUTDOWN","Current compound command being executed was %s(#%d).",getfield(current_command,NAME),current_command);

     /* ---->  Log last commands typed by characters who were connected  <---- */
     /*        at time of panic (As they may have caused the crash.)            */
     server_log_commands();

     /* ---->  Trap exceptions raised during PANIC database dump  <---- */
     for(i = 0; i < NSIG; i++) signal(i,SIG_IGN);
     signal(SIGSTOP,server_emergency_exception);
     signal(SIGCONT,server_emergency_exception);
     signal(SIGTSTP,server_emergency_exception);
     signal(SIGTERM,server_emergency_exception);
     signal(SIGQUIT,SIG_IGN);
     signal(SIGKILL,server_emergency_exception);
     signal(SIGSEGV,server_emergency_exception);
     signal(SIGBUS, server_emergency_exception);

     /* ---->  Construct shutdown message  <---- */
     if((sig == SIGTERM) || (sig == SIGKILL) || (sig == SIGHUP) || (sig == SIGINT) || (sig == SIGQUIT))
        sprintf(bootmessage,SYSTEM_SHUTDOWN""ANSI_LWHITE"%s has been shutdown externally by the administrator(s) of its server machine  -  Keep trying, we'll be back up again soon.\n\n\n",tcz_full_name,tcz_full_name);
           else sprintf(bootmessage,EMERGENCY_SHUTDOWN""ANSI_LMAGENTA"%s has crashed!  :-(\n"ANSI_LGREEN"Keep trying, we'll be back up again soon...\n\n\n",tcz_full_name,tcz_full_name);

     /* ---->  Shutdown user connections  <---- */
     server_close_sockets();

     /* ---->  Dump panic database  <---- */
     if(dumpstatus > 0) {
        dumpstatus = 255;
        db_write(NULL);
     }
     dumpstatus = 1;
     dumptype   = DUMP_PANIC;

     if(option_console(OPTSTATUS))
        option_console(NOTHING,"SHUTDOWN","No",0,&error,&critical);

     writelog(SERVER_LOG,0,"SHUTDOWN","Dumping database '%s.PANIC'...",dumpfile);
     db_write(NULL);
     if(dumpstatus > 0) {
        for(; dumpstatus > 0; db_write(NULL));
        writelog(SERVER_LOG,0,"SHUTDOWN","Emergency shutdown complete.");
        exitcode = 136;
     } else exitcode = 135;

     /* ---->  Close log files and remove lock file  <---- */
     logfile_close();
     if(lockfile) unlink(lockfile);

     /* ---->  Generate core dump  <---- */
     if(option_coredump(OPTSTATUS)) {
        signal(SIGABRT,SIG_DFL);
        abort();
     }
     _exit(exitcode);
}

/* ---->  Signal/exception handler  <---- */
void server_signal_handler(int sig)
{
     char message[KB];

     sprintf(message,"(BAILOUT)  Caught signal %d (%s:  %s.)",sig,SignalName(sig),SignalDesc(sig));
     server_emergency_dump(message,sig);
     _exit(1);
}

/* ---->  Set up handling of signals  <---- */
void server_set_signals()
{
     /* ---->  Signals handled by TCZ  <---- */
     signal(SIGFPE,server_SIGFPE_handler);
     signal(SIGCONT,server_SIGCONT_handler);
     signal(SIGUSR1,server_SIGUSR1_handler);
     signal(SIGUSR2,server_SIGUSR2_handler);

     /* ---->  Power failure/resumption signals (UPS support)  <---- */
#ifdef UPS_SUPPORT
     signal(SIGPWR,server_SIGPWR_handler);
#else
  #ifdef SIGPWR
     signal(SIGPWR,SIG_IGN);
  #endif
#endif

     /* ---->  Signals ignored by TCZ  <---- */
     signal(SIGALRM,SIG_IGN);
     signal(SIGHUP,SIG_IGN);
     signal(SIGPIPE,SIG_IGN);

     /* ---->  Other signals which will involke the emergency shutdown and database dump  <---- */
     signal(SIGILL,server_signal_handler);
     signal(SIGINT,server_signal_handler);
     signal(SIGIOT,server_signal_handler);
     signal(SIGTERM,server_signal_handler);
     signal(SIGTRAP,server_signal_handler);
     signal(SIGVTALRM,server_signal_handler);
     signal(SIGXCPU,server_signal_handler);
     signal(SIGXFSZ,server_signal_handler);

     /* ---->  Forking database dump  <---- */
     signal(SIGCHLD,SIG_DFL);
#ifdef DB_FORK
     if(option_forkdump(OPTSTATUS))
        signal(SIGCHLD,server_SIGCHLD_handler);
#endif

     /* ---->  Linux specific signals  <---- */
#ifndef LINUX
     signal(SIGEMT,server_signal_handler);
     signal(SIGSYS,server_signal_handler);
#endif

     /* ---->  Emergency shutdown and database dump on crash (SIGSEGV/SIGBUS)  <---- */
     if(option_emergency(OPTSTATUS)) {
        signal(SIGBUS,server_signal_handler);
        signal(SIGSEGV,server_signal_handler);
     }
}

/* ---->  Initialise remote Telnet software  <---- */
void server_initialise_telnet(int descriptor)
{
     write(descriptor,_IAC""_WILL""_NEGEOR,3);  /*  Use EOR to mark end of prompts  */
     write(descriptor,_IAC""_DO""_LINE,3);      /*  Line-by-line mode preferred     */
     write(descriptor,_IAC""_WONT""_ECHO,3);    /*  Local echo                      */
     write(descriptor,_IAC""_DO""_TTYPE,3);     /*  Terminal type                   */
     write(descriptor,_IAC""_DO""_NAWS,3);      /*  Window size                     */
}

/* ---->  Open socket for incoming connections  <---- */
int server_open_socket(int port,unsigned char restart,unsigned char refresh,unsigned char html,unsigned char logtime)
{
    struct sockaddr_in server;
    int    opt,new;

    /* ---->  Attempt to open socket  <---- */
    if((new = socket(AF_INET,SOCK_STREAM,0)) < 0) {
       if(restart) writelog(SERVER_LOG,logtime,"RESTART","Unable to open socket for incoming %s connections (%s)  -  Either %s is already running or the port is in use by another process  -  Restart aborted.",(html) ? "HTML":"Telnet",strerror(errno),tcz_short_name);
          else writelog(SERVER_LOG,logtime,(html) ? "HTML":"TELNET","Error  -  Unable to %sopen socket for incoming %s connections (%s)  -  Re-trying in %d minute%s.",(refresh) ? "re-":"",(html) ? "HTML":"Telnet",strerror(errno),(refresh) ? 1:REFRESH_SOCKET_INTERVAL,Plural((refresh) ? 1:REFRESH_SOCKET_INTERVAL));
       return(-1);
    }

    /* ---->  Set socket options  <---- */
    opt = 1;
    if(setsockopt(new,SOL_SOCKET,SO_REUSEADDR,(char *) &opt,sizeof(opt)) < 0) {
       if(restart) writelog(SERVER_LOG,logtime,"RESTART","Unable to set options of socket for incoming %s connections (%s)  -  Restart aborted.",(html) ? "HTML":"Telnet",strerror(errno));
          else writelog(SERVER_LOG,logtime,(html) ? "HTML":"TELNET","Error  -  Unable to set options of %ssocket for incoming %s connections (%s)  -  Re-trying in %d minute%s.",(refresh) ? "new ":"",(html) ? "HTML":"Telnet",strerror(errno),(refresh) ? 1:REFRESH_SOCKET_INTERVAL,Plural((refresh) ? 1:REFRESH_SOCKET_INTERVAL));
       close(new);
       return(-1);
    }

    /* ---->  Bind socket to port PORT and listen for incoming connections  <---- */
    if(option_local(OPTSTATUS))
       server.sin_addr.s_addr = IPADDR(1,0,0,127);  /*  Run locally  */
          else server.sin_addr.s_addr = INADDR_ANY;
   
    server.sin_family      = AF_INET;
    server.sin_port        = htons(port);
    if(bind(new,(struct sockaddr *) &server,sizeof(server))) {
       if(restart) writelog(SERVER_LOG,logtime,"RESTART","Unable to bind socket for incoming %s connections (%s)  -  Restart aborted.",(html) ? "HTML":"Telnet",strerror(errno));
          else writelog(SERVER_LOG,logtime,(html) ? "HTML":"TELNET","Error  -  Unable to bind %ssocket for incoming %s connections (%s)  -  Re-trying in %d minute%s.",(refresh) ? "new ":"",(html) ? "HTML":"Telnet",strerror(errno),(refresh) ? 1:REFRESH_SOCKET_INTERVAL,Plural((refresh) ? 1:REFRESH_SOCKET_INTERVAL));
       close(new);
       return(-1);
    }
    listen(new,MAX_PENDING_NEW_CONNECTIONS);
    return(new);
}

/* ---->  Process queued TCZ commands (Typed/executed by users)  <---- */
void server_process_commands(void)
{
     struct   descriptor_data *d,*dnext;
     unsigned char            processed;
     char                     *command;
     struct   text_data       *next;
     time_t                   now;
     int                      np;

     for(d = descriptor_list, np = 1; d; d->flags &= ~PROCESSED, d = d->next);
     while(np > 0) {
           for(d = descriptor_list, np = 0, reset_list = 0; d && !reset_list; d = (!reset_list) ? dnext:NULL) {
               gettime(now);
               dnext = d->next;
               if(!(d->flags & PROCESSED)) {
                  d->flags |= PROCESSED, np++;
                  if((now >= d->next_time) && d->input.start) {

                     /* ---->  Get user command from input queue and remove entry  <---- */
                     command = d->input.start->text;
                     if(d->input.end == d->input.start) d->input.end = NULL;
                     next = d->input.start->next;
                     FREENULL(d->input.start);
                     d->input.start = next;
                     d->input.size--;

                     /* ---->  Process user command  <---- */
                     processed = server_command(d,command);
                     FREENULL(command);

                     if(reset_list) {
                        struct descriptor_data *ptr;

                        for(ptr = descriptor_list; ptr && (ptr != d); ptr = ptr->next);
                        if(!ptr) processed = 1;
		     }

                     if(!processed) server_shutdown_sock(d,0,0);
                     if(dumpstatus > 0) db_write(NULL);
                     tcz_time_sync(0);
		  }
	       }
	   }
     }
}

/* ---->  Server main loop:  *  Process user input/output                <---- */
/*                           *	External dumping process error messages        */
/*                           *  Process pending events                         */
/*                           *  Accept new connections                         */
/*                           *  Disconnect idle connections                    */
void server_mainloop(void)
{
     time_t                   diff,now;
     fd_set                   input_set,output_set;
     int                      select_value,maxd;
     struct   descriptor_data *d,*dnext,*newd;
     struct   timeval         timeout;
     short                    np;

     gettime(activity);
     
#ifdef SOCKETS
     if((telnetport > 0) && (telnet >= 0))
        writelog(SERVER_LOG,0,"RESTART","Accepting Telnet connections on port %d.",telnetport);

#ifdef HTML_INTERFACE
     if((htmlport > 0) && (html >= 0))
        writelog(SERVER_LOG,0,"RESTART","Accepting HTML (World Wide Web) connections on port %d.",htmlport);
#endif
#endif

     log_stderr = 0;
     if(option_shutdown(OPTSTATUS))
        shutdown_counter = 0;

     while(shutdown_counter) {

           /* ---->  Enter semi-idle state?  <---- */
           gettime(now);
           if(!idle_state && ((now - activity) > (IDLE_STATE_INTERVAL * MINUTE))) {
              writelog(SERVER_LOG,1,"SERVER","No input/output or new connections for over %d minute%s  -  Entering semi-idle state.",IDLE_STATE_INTERVAL,Plural(IDLE_STATE_INTERVAL));
              idle_state = 1;
	   }

           /* ---->  Dump database and process pending user commands  <---- */
           if(dumpstatus > 0) {
              db_write(NULL);
              gettime(now);
	   }

#ifdef GUARDIAN_ALARM
	   alarm(0);
	   signal(SIGALRM,SIG_IGN);
#endif

           if(idle_state) {
              sleep(IDLE_STATE_DELAY);
              gettime(now);
	   }

#ifdef GUARDIAN_ALARM
	   if(option_guardian(OPTSTATUS)) {
	      signal(SIGALRM,server_SIGALRM_handler);
	      alarm(GUARDIAN_ALARM_TIME);
	   }
#endif

           server_process_commands();

           if(!shutdown_counter) break;
           timeout.tv_sec  = 1;
           timeout.tv_usec = 0;
           FD_ZERO(&input_set);
           FD_ZERO(&output_set);
           gettime(now);

#ifdef REFRESH_SOCKETS
#ifdef SOCKETS

           /* ---->  Refresh socket for new Telnet connections if last new connection was more than REFRESH_SOCKET_INTERVAL minutes ago  <---- */
           if(((now - lastconn) >= (REFRESH_SOCKET_INTERVAL * MINUTE)) && (telnetport > 0)) {
              if(telnet >= 0) {
                 if(newconnections) writelog(SERVER_LOG,1,"TELNET","No activity on socket for incoming Telnet connections for %d minute%s  -  Refreshing socket.",(now - lastconn) / MINUTE,Plural((now - lastconn) / MINUTE));
                 newconnections = 0;
                 if(close(telnet)) {
                    writelog(SERVER_LOG,1,"TELNET","Unable to close socket for incoming Telnet connections (%s)  -  Retrying in 1 minutes time.",strerror(errno));
                    lastconn -= ((REFRESH_SOCKET_INTERVAL * MINUTE) - MINUTE);
		 } else telnet = -1;
	      }
              if(telnet < 0) {
                 gettime(lastconn);
                 if((telnet = server_open_socket(telnetport,0,1,0,1)) < 0) {
                    lastconn -= ((REFRESH_SOCKET_INTERVAL * MINUTE) - MINUTE);
                    telnet = -1;
		 }
	      }
	   }

#ifdef HTML_INTERFACE

           /* ---->  Refresh socket for new HTML connections if last new connection was more than REFRESH_SOCKET_INTERVAL minutes ago  <---- */
           if(((now - lasthtml) >= (REFRESH_SOCKET_INTERVAL * MINUTE)) && (htmlport > 0)) {
              if(html >= 0) {
                 if(newconnections) writelog(SERVER_LOG,1,"HTML","No activity on socket for incoming HTML connections for %d minute%s  -  Refreshing socket.",(now - lasthtml) / MINUTE,Plural((now - lasthtml) / MINUTE));
                 newconnections = 0;
                 if(close(html)) {
                    writelog(SERVER_LOG,1,"HTML","Unable to close socket for incoming HTML connections (%s)  -  Retrying in 1 minutes time.",strerror(errno));
                    lasthtml -= ((REFRESH_SOCKET_INTERVAL * MINUTE) - MINUTE);
		 } else html = -1;
	      }
              if(html < 0) {
                 gettime(lasthtml);
                 if((html = server_open_socket(htmlport,0,1,1,1)) < 0) {
                    lasthtml -= ((REFRESH_SOCKET_INTERVAL * MINUTE) - MINUTE);
                    html = -1;
		 }
	      }
	   }
#endif
#endif
#endif

           /* ---->  Determine highest active descriptor (For select())  <---- */
           maxd = 0;
           if((telnet >= 0) && (ndescriptors < (max_descriptors + RESERVED_DESCRIPTORS))) {
              if(telnet >= maxd) maxd = telnet + 1;
              FD_SET(telnet,&input_set);
	   }
#ifdef HTML_INTERFACE
           if((html >= 0) && (ndescriptors < (max_descriptors + RESERVED_DESCRIPTORS))) {
              if(html >= maxd) maxd = html + 1;
              FD_SET(html,&input_set);
	   }
#endif

           /* ---->  Set descriptors to listen for input and process output  <---- */
           for(d = descriptor_list; d; d = d->next)
               if(d->descriptor != NOTHING) {
                  if(d->descriptor >= maxd) maxd = d->descriptor + 1;
                  FD_SET(d->descriptor,&input_set);
                  if(d->output.start || (IsHtml(d) && (d->html->flags & HTML_INPUT_PENDING))) {
                     if(d->descriptor >= maxd) maxd = d->descriptor + 1;
                     FD_SET(d->descriptor,&output_set);
		  }
	       }

           /* ---->  Wait for activity on sockets  <---- */
           if((select_value = select(maxd,(fd_set *) &input_set,(fd_set *) &output_set,(fd_set *) NULL,&timeout)) < 0) {
             
              /* ---->  Unexpected error  <---- */
              if(errno != EINTR) {
                 sprintf(scratch_return_string,"(select() call in server_mainloop() in server.c)  %s.",strerror(errno));
                 server_emergency_dump(scratch_return_string,0);
	      }
           } else if(select_value == 0) {

              /* ---->  No activity  -  Process timed events  <---- */
              tcz_time_sync(0);
              gettime(now);
           } else {

#ifdef SOCKETS

              /* ---->  New Telnet connections  <---- */
              gettime(now);
              if((telnet >= 0) && FD_ISSET(telnet,&input_set)) {
                 if((newd = server_new_connection(telnet,0))) {
                    gettime(activity);
                    if(idle_state == 1) {
                       writelog(SERVER_LOG,1,"SERVER","New Telnet connection  -  Leaving semi-idle state.");
                       idle_state = 0;
		    }
                    server_initialise_telnet(newd->descriptor);
		 } else if(errno) writelog(CONNECT_LOG,1,"CONNECT","Unable to establish new Telnet connection (Errno = %d:  %s.)",errno,strerror(errno));
	      }

#ifdef HTML_INTERFACE

              /* ---->  New HTML connections  <---- */
              if((html >= 0) && FD_ISSET(html,&input_set)) {
                 if((newd = server_new_connection(html,1))) {
                    gettime(activity);
                    if(idle_state == 1) {
                       writelog(SERVER_LOG,1,"SERVER","New HTML connection  -  Leaving semi-idle state.");
                       idle_state = 0;
		    }
		 } else if(errno) writelog(CONNECT_LOG,1,"CONNECT","Unable to establish new HTML connection (Errno = %d:  %s.)",errno,strerror(errno));
	      }
#endif
#endif

              /* ---->  Input/Output  <---- */
              for(d = descriptor_list, np = 1; d; d->flags &= ~PROCESSED, d = d->next);
              while(np > 0) {
                    for(d = descriptor_list, np = 0, reset_list = 0; d && !reset_list; d = (!reset_list) ? dnext:NULL) {
                        gettime(now);
                        dnext = d->next;
                        if(!(d->flags & PROCESSED)) {
                           d->flags |= PROCESSED, np++;
                           if((d->descriptor != NOTHING) && FD_ISSET(d->descriptor,&input_set)) {
                              gettime(activity);
                              if(idle_state == 1) {
                                 writelog(SERVER_LOG,1,"SERVER","Input on descriptor %d (%s)  -  Leaving semi-idle state.",d->descriptor,IsHtml(d) ? "HTML":"Telnet");
                                 idle_state = 0;
			      }

                              if(!server_process_input(d,IsHtml(d) ? 1:0)) server_shutdown_sock(d,0,1);
			   }

                           if(!reset_list && (d->descriptor != NOTHING) && FD_ISSET(d->descriptor,&output_set)) {
                              gettime(activity);
                              if(idle_state == 1) {
                                 writelog(SERVER_LOG,1,"SERVER","Output on descriptor %d (%s)  -  Leaving semi-idle state.",d->descriptor,IsHtml(d) ? "HTML":"Telnet");
                                 idle_state = 0;
			      }

                              if(!server_process_output(d))
                                 server_shutdown_sock(d,0,1);
			   }
			}
		    }
	      }
	   }

           /* ---->  Handle idle user connections  <---- */
           for(d = descriptor_list; d; d = dnext) {
               gettime(now);
               dnext = d->next;
               diff  = now - d->last_time;

               if(((d->flags & CLOSING) && !d->output.start) || ((d->descriptor == NOTHING) && (diff > ((d->flags & DELIVERED) ? ((KEEPALIVE * MINUTE) / 2):(KEEPALIVE * MINUTE))))) {
                  server_shutdown_sock(d,0,0);
	       } else {

                     /* ---->  Automatically send user AFK if idle for more than set time (Default is 10 minutes)  <---- */
                     if(!d->afk_message && Validchar(d->player) && db[d->player].data->player.afk && (diff > (db[d->player].data->player.afk * MINUTE)))
			if(!(!Blank(db[d->player].name) && instring("guest",getname(d->player)))) {
                           sprintf(scratch_return_string,AUTO_AFK,db[d->player].data->player.afk,Plural(db[d->player].data->player.afk),tcz_short_name);
                           d->afk_message = (char *) alloc_string(scratch_return_string);
	    	   	   d->flags2     |= SENT_AUTO_AFK;
                           d->clevel      = 14;
                           gettime(d->afk_time);
                           output(d,d->player,0,1,0,ANSI_LMAGENTA"\nYou have been automatically sent AFK for idling for "ANSI_LWHITE"%d minute%s"ANSI_LMAGENTA".  Please enter your password to resume your %s session...\n",db[d->player].data->player.afk,Plural(db[d->player].data->player.afk),tcz_short_name);
                           prompt_display(d);

                           /* ---->  Use JAVA Script (1.1+) to reload the 'TCZ Command:' input window to show the 'Please enter password:' prompt  <---- */
                           if(IsHtml(d) && (d->html->flags & HTML_JAVA))
                              output(d,d->player,1,0,0,"<SCRIPT LANGUAGE=\"JavaScript1.1\">\n<!--\n parent.TCZINPUT.document.location.replace(\"%sREFRESH=%d&\");\n//-->\n</SCRIPT>",html_server_url(d,1,1,"input"),d->afk_time);
			}

                     if(!(d->flags & CONNECTED)) {

                        /* ---->  Maximum allowed idle time at Telnet login screen exceeded  <---- */
                        if(diff > (MAX_LOGIN_IDLE_TIME * MINUTE)) {
                           output(d,d->player,2,0,0,ANSI_WRED"\n[You have been idle for "ANSI_WWHITE"%d minute%s"ANSI_WRED"  -  Connection timed out...]\n\n",MAX_LOGIN_IDLE_TIME,Plural(MAX_LOGIN_IDLE_TIME));
                           server_shutdown_sock(d,0,0);
			}
		     } else if(diff > (MAX_IDLE_TIME * MINUTE)) {

                        /* ---->  Maximum allowed idle time of connected user exceeded (MAX_IDLE_TIME minutes)  <---- */
                        sprintf(bootmessage,"\n"ANSI_WRED"\x07[You have been idle for "ANSI_WWHITE"%d minute%s"ANSI_WRED"  -  You have been automatically disconnected from %s...]\n",MAX_IDLE_TIME,Plural(MAX_IDLE_TIME),tcz_full_name);
                        server_shutdown_sock(d,0,0);                        
		     } else if(diff > ((WARN_IDLE_TIME + (d->warning_level * 5)) * MINUTE)) {

                        /* ---->  Warning messages for every 5 minutes of idle time over WARN_IDLE_TIME minutes  <---- */
                        output(d,d->player,2,0,0,ANSI_WYELLOW"\n[You have been idle for "ANSI_WWHITE"%d minute%s"ANSI_WYELLOW"  -  %d minute%s until automatic disconnection...]\n\n",(WARN_IDLE_TIME + (d->warning_level * 5)),Plural(WARN_IDLE_TIME + (d->warning_level * 5)),MAX_IDLE_TIME - (WARN_IDLE_TIME + (d->warning_level * 5)),Plural(MAX_IDLE_TIME - (WARN_IDLE_TIME + (d->warning_level * 5))));
                        d->warning_level++;
		     }
		  }
	   }
     }
}

/* ---->  Set telnet local echo on/off  <---- */
void server_set_echo(struct descriptor_data *d,int echo)
{
     if(echo) {
        if(d->flags & SUPPRESS_ECHO) {
           d->flags &= ~SUPPRESS_ECHO;
           if(!IsHtml(d)) {
              write(d->descriptor,_IAC""_DONT""_LFLOW,3);
              write(d->descriptor,_IAC""_DO""_LINE,3);
              write(d->descriptor,_IAC""_WONT""_ECHO,3);
	   }
	}
     } else if(!(d->flags & SUPPRESS_ECHO)) {
        d->flags |= SUPPRESS_ECHO;
        if(!IsHtml(d)) {
           write(d->descriptor,_IAC""_DO""_LFLOW,3);
           write(d->descriptor,_IAC""_DONT""_LINE,3);
           write(d->descriptor,_IAC""_WILL""_ECHO,3);
	}
     }
}

/* ---->  Check site isn't unconditionally banned/connection restricted  <---- */
int server_unconditionally_banned(struct site_data *site,int sock,char *buffer,unsigned char ctype)
{
    const char *error;

    if(!site || !(site->flags & SITE_UNCONDITIONAL)) return(0);

    /* ---->  Check site isn't banned  <---- */
    if(site->flags & SITE_BANNED) {
       if(ctype == 1) {
          char buffer[BUFFER_LEN],buffer2[TEXT_SIZE];

          sprintf(buffer,"Sorry, connections from your site are no-longer accepted due to excessive abuse.<P>Please send E-mail to <A HREF=\"mailto:%s\">%s</A> to negotiate lifting this ban.",tcz_admin_email,tcz_admin_email);
	  sprintf(buffer2,"Back to %s web site...",tcz_full_name);
          error = html_error(NULL,0,buffer,"CONNECTION REFUSED",buffer2,html_home_url,HTML_CODE_ERROR|HTML_CODE_HEADER);
          write(sock,error,strlen(error));
       } else {
          sprintf(buffer,"\n\n-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n Sorry, connections from your site are no-longer accepted due to excessive\n abuse.\n\n Please send E-mail to %s to negotiate lifting this ban.\n-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n\n",tcz_admin_email);
          write(sock,buffer,strlen(buffer));
       }
       return(1);
    }

    /* ---->  If connections to site are limited, check limit hasn't been reached  <---- */
    if(site->max_connections != NOTHING) {
       struct descriptor_data *d;
       int    count = 0;

       for(d = descriptor_list; d; d = d->next)
           if(d->site && (d->site->addr == site->addr)) count++;

       if(count >= site->max_connections) {
          if(ctype == 1) {
             char buffer2[TEXT_SIZE],buffer3[TEXT_SIZE];

             sprintf(buffer2,"Sorry, there are too many users connected from your site at the moment (A maximum of %d are allowed simultaneously.)<P>Please try again later.",site->max_connections);
  	     sprintf(buffer3,"Back to %s web site...",tcz_full_name);
             error = html_error(NULL,0,buffer2,"CONNECTION REFUSED",buffer3,html_home_url,HTML_CODE_ERROR|HTML_CODE_HEADER);
             write(sock,error,strlen(error));
	  } else {
             sprintf(buffer,"\n\n-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n Sorry, there are too many users connected from your site at the moment (A\n maximum of %d are allowed simultaneously.)  Please try again later.\n-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n\n",site->max_connections);
             write(sock,buffer,strlen(buffer));
	  }
          return(1);
       }
    }
    return(0);
}

/* ---->  Initialise DESCRIPTOR_DATA of new connection  <---- */
/*        (CTYPE:  0 = Normal, 1 = HTML.)                     */
struct descriptor_data *server_initialise_sock(int new,struct sockaddr_in *a,struct site_data *site,unsigned char ctype)
{
       char                   buffer[TEXT_SIZE];
       static unsigned long   identifier = 0;
       time_t                 now;
       struct descriptor_data *d;

       /* ---->  Make socket non-blocking  <---- */
       if(fcntl(new,F_SETFL,FNDELAY) == -1)
          server_emergency_dump("(server_initialise_sock() in server.c)  FNDELAY fcntl failed.",0);

       /* ---->  Misc. stuff  <---- */
       gettime(now);
       ndescriptors++;
       MALLOC(d,struct descriptor_data);

       d->emergency_time = now;
       d->warning_level  = 0;
       d->output.start   = NULL;
       d->messagecount   = 0;
       d->raw_input_at   = NULL;
       d->last_command   = (char *) alloc_string(compress("look",0));
       d->output.size    = 0;
       d->input.start    = NULL;
       sprintf(buffer,ANSI_LWHITE"%s "ANSI_DWHITE,tcz_prompt);
       d->user_prompt    = (char *) alloc_string(buffer);
       d->currentpage    = 0;
       d->afk_message    = NULL;
       d->lastmessage    = NULL;
       d->assist_time    = 0;
       d->page_clevel    = 0;
       d->page_title     = 0;
       d->currentmsg     = 0;
       d->start_time     = now;
       d->descriptor     = new;
       d->output.end     = NULL;
       d->input.size     = 0;
       d->input.end      = 0;
       d->cmdprompt      = NULL;
       d->helptopic      = NULL;
       d->last_time      = now;
       d->next_time      = now;
       d->name_time      = now + NAME_TIME;
       d->raw_input      = NULL;
       d->helpname       = NULL;
       d->password       = NULL;
       d->afk_time       = now;
       d->comment        = NULL;
       d->subject        = NULL;
       d->channel        = 1;
       d->monitor        = NULL;
       d->assist         = NULL;
       d->chname         = NULL;
       d->clevel         = 0;
       d->negbuf         = NULL;
       d->neglen         = 0;
       d->player         = 0;
       d->prompt         = NULL;
       d->summon         = NOTHING;
       d->flags2         = 0;
       d->flags          = EVALUATE|LFTOCR_LFCR|INITIALISE;
       d->pager          = NULL;
       d->edit           = NULL;
       d->name           = NULL;
       d->page           = 0;

       /* ---->  HTML data  <---- */
       if(ctype == 1) {
          MALLOC(d->html,struct html_data);
          d->html->background = NULL;
          d->html->identifier = ++identifier;
          d->html->txtflags   = 0;
          d->html->cmdwidth   = HTML_CMDWIDTH;
          d->html->protocol   = 0;
          d->html->flags      = HTML_INPUT_PENDING;
          d->html->tag        = NULL;
          d->html->id1        = 0;
          d->html->id2        = 0;
       } else d->html = NULL;

       /* ---->  Site data  <---- */
       d->site              = site;
       d->address           = ntohl(a->sin_addr.s_addr);
       d->hostname          = (char *) malloc_string(scratch_return_string);

       /* ---->  Terminal data  <---- */
       d->terminal_height   = STANDARD_CHARACTER_SCRHEIGHT;
       d->terminal_width    = STANDARD_CHARACTER_SCRWIDTH - 1;
       d->terminal_type     = NULL;
       d->terminal_xpos     = 0;
       d->termcap           = NULL;

       /* ---->  Insert new entry into descriptor list  <---- */
       if(descriptor_list) descriptor_list->prev = d;
       d->next              = descriptor_list;
       d->prev              = NULL;
       descriptor_list      = d;

       if(!ctype) {
#ifdef PAGE_TITLESCREENS
          d->page_title  = ((lrand48() % titlescreens) + 1);
          d->page_clevel = 1;
          d->clevel = 30;
          d->page = 1;
#else
          const char *ptr = help_get_titlescreen(0);

          /* ---->  Display title screen and instructions  <---- */
          if(ptr) output(d,d->player,2,0,0,"%s",decompress(ptr));
             else output(d,d->player,2,0,0,WELCOME_MESSAGE,tcz_full_name,tcz_year);
          d->clevel = 1;
#endif
          prompt_display(d);
       }
       return(d);
}

/* ---->  Accept new Telnet/HTML connections  <---- */
/*         (CTYPE:  0 = Normal, 1 = HTML.)          */
struct descriptor_data *server_new_connection(int sock,unsigned char ctype)
{
       struct descriptor_data *d;
       struct sockaddr_in addr;
       struct site_data *site;
       int    addr_len,new;
       int    count = 0;

       for(d = descriptor_list; d; d = d->next, count++);
       addr_len = sizeof(addr);

#ifdef DEBUG_ACCEPT

       /* ---->  Log new connection (Debugging only)  <---- */
       writelog(CONNECT_LOG,1,"ESTABLISHING CONNECTION","Socket %d%s.",sock,(ctype == 1) ? ", HTML protocol":", Telnet protocol");
#endif

       /* ---->  Try to establish connection  <---- */
       new = accept(sock,(struct sockaddr *) &addr,&addr_len);
       if(new < 0) return(0);

       /* ---->  Look up site details and log connection accepted  <---- */
       site = lookup_site(&(addr.sin_addr),scratch_return_string);
       if(count >= max_descriptors) {

          /* ---->  Too many simultanous user connections  <---- */
          writelog(CONNECT_LOG,1,"CONNECTION REFUSED","%s connection from %s (%d) on descriptor %d (Too many simultaneous connections.)",(ctype == 1) ? "HTML":"Telnet",scratch_return_string,ntohs(addr.sin_port),new);
          if(ctype == 1) {
             char buffer[TEXT_SIZE],buffer2[TEXT_SIZE];

	     sprintf(buffer2,"Back to %s web site...",tcz_full_name);
	     sprintf(buffer,"Sorry, there are too many users connected to %s at the moment  -  Please try again in a few minutes time.",tcz_full_name);
             strcpy(scratch_buffer,html_error(NULL,0,buffer,"CONNECTION REFUSED",buffer2,html_home_url,HTML_CODE_ERROR|HTML_CODE_HEADER));
	  } else strcpy(scratch_buffer,"\n\r\n\r-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n\r           Sorry, there are too many users connected at the moment.\n\r\n\r                           Please try again later.\n\r-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n\r\n\r");
          write(new,scratch_buffer,strlen(scratch_buffer));
          shutdown(new,2);
          close(new);
          return(0);
       } else if(server_unconditionally_banned(site,new,scratch_buffer,ctype)) {

          /* ---->  Site unconditionally banned/restricted?  <----- */
          writelog(CONNECT_LOG,1,"CONNECTION REFUSED","%s connection from %s (%d) on descriptor %d (Site unconditionally banned/restricted.)",(ctype == 1) ? "HTML":"Telnet",scratch_return_string,ntohs(addr.sin_port),new);
          shutdown(new,2);
          close(new);
          return(0);
#ifdef DEMO
       } else if((count >= 7) || (ntohl(addr.sin_addr.s_addr) != 0x7F000001)) {

          /* ---->  DEMO TCZ:  Connections (Maximum of 7 simultaneously) only allowed from 127.0.0.1  <---- */
          if(ctype == 1) {
             char buffer[TEXT_SIZE],buffer2[TEXT_SIZE];

             sprintf(buffer2,"Back to %s web site...",tcz_full_name);
	     sprintf(buffer,"Sorry, this is a <I>demonstration version</I> of %s.<P>Only a maximum of <B>7 connections</B> are allowed simultaneously via local loop-back (<B>127.0.0.1</B>) of the machine on which %s is currently running.",tcz_full_name,tcz_short_name);
             strcpy(scratch_buffer,html_error(NULL,0,buffer,"CONNECTION REFUSED",buffer2,html_home_url,HTML_CODE_ERROR|HTML_CODE_HEADER));
	  } else sprintf(scratch_buffer,"\n\r\n\r-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n\r Sorry, this is a demonstration version of %s.\n\r\n\r Only a maximum of 7 connections are allowed simultaneously from local\n\r loop-back (<I>127.0.0.1</I>) of the machine on which %s is currently\n\r running.\n\r-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n\r\n\r",tcz_full_name,tcz_short_name);

          write(new,scratch_buffer,strlen(scratch_buffer));
          writelog(CONNECT_LOG,1,"CONNECTION REFUSED","%s connection from %s (%d) on descriptor %d (Connections (Maximum of 7 simultaneously) are only allowed from 127.0.0.1 (Local loop-back.))",(ctype == 1) ? "HTML":"Telnet",scratch_return_string,ntohs(addr.sin_port),new);
          shutdown(new,2);
          close(new);
          return(0);
#endif
       } else if(ctype != 1) writelog(CONNECT_LOG,1,"CONNECTION ACCEPTED","%s connection from %s (%d) on descriptor %d.",(ctype == 1) ? "HTML":"Telnet",scratch_return_string,ntohs(addr.sin_port),new);

       if(!ctype && (option_loglevel(OPTSTATUS) >= 6))
          writelog(COMMAND_LOG,1,"TELNET","[%d]  Connection on descriptor %d.",new,new);

       return(server_initialise_sock(new,&addr,site,ctype));
}

/* ---->  Clear text queue structure (Free memory used)  <---- */
void server_clear_textqueue(struct text_queue_data *queue)
{
     struct text_data *start,*next;

     if(!queue) return;
     for(start = queue->start; start; start = next) {
         next = start->next;
         FREENULL(start->text);
         FREENULL(start);
         start = next;
     }
     queue->start = NULL;
     queue->end   = NULL;
     queue->size  = 0;
}

/* ---->  Clear descriptor strings (Free memory used)  <---- */
void server_clear_strings(struct descriptor_data *d)
{
     /* ---->  General  <---- */
     FREENULL(d->terminal_type);
     FREENULL(d->last_command);
     FREENULL(d->user_prompt);
     FREENULL(d->afk_message);
     FREENULL(d->lastmessage);
     FREENULL(d->raw_input);
     FREENULL(d->helpname);
     FREENULL(d->hostname);
     FREENULL(d->password);
     FREENULL(d->comment);
     FREENULL(d->subject);
     FREENULL(d->assist);
     FREENULL(d->negbuf);
     FREENULL(d->chname);
     FREENULL(d->name);

     /* ---->  Editor (If user was in editor at time of disconnecting)  <---- */
     if(d->edit) {
        FREENULL(d->edit->prompt);
        FREENULL(d->edit->index);
        FREENULL(d->edit->data1);
        FREENULL(d->edit->data2);
        FREENULL(d->edit->text);
        FREENULL(d->edit);
     }

     /* ---->  Interactive '@prompt' session  <---- */
     if(d->prompt) {
        if(d->prompt->temp) temp_clear(&(d->prompt->temp),NULL);
        FREENULL(d->prompt->prompt);
        FREENULL(d->prompt);
     }

     /* ---->  Command arguments prompt session  <---- */
     if(d->cmdprompt)
        prompt_user_free(d);

     /* ---->  Stored messages  <---- */
     for(; d->messagecount > 0; d->messagecount--)
         FREENULL(d->messages[d->messagecount - 1].message);

     /* ---->  'more' pager  <---- */
     if(d->pager) pager_free(d);

     /* ---->  HTML data  <---- */
     if(IsHtml(d)) {
        struct html_tag_data *current;

        while(d->html->tag) {
              current = d->html->tag;
              d->html->tag = d->html->tag->next;
              FREENULL(current);
	}
        FREENULL(d->html->background);
        FREENULL(d->html);
     }
}

/* ---->  Queue user input  <---- */
unsigned char server_queue_input(struct descriptor_data *d,const char *str,int len)
{
	 struct text_data *new;

	 /* ---->  Truncate string if too large  <---- */
	 if(!str || (len < 1)) return(0);
	 if(len > (TEXT_SIZE * 2)) len = (TEXT_SIZE * 2);

	 /* ---->  If too many commands are queued (16 or more), remove earlier commands from queue  <---- */
	 while(d->input.size >= (d->edit ? INPUT_MAX_EDITOR:INPUT_MAX)) {
	       if(d->input.end == d->input.start) d->input.end = NULL;
	       new = d->input.start->next;
	       FREENULL(d->input.start->text);
	       FREENULL(d->input.start);
	       d->input.start = new;
	       d->input.size--;
	 }

	 /* ---->  Add string to end of queue  <---- */
	 MALLOC(new,struct text_data);
	 NMALLOC(new->text,char,len);
	 memcpy(new->text,str,len);
	 new->next = NULL;
	 new->len  = len;
	 if(d->input.end) {
	    d->input.end->next = new;
	    d->input.end       = new;
	 } else d->input.start = d->input.end = new;
	 d->input.size++;
	 return(1);
}

/* ---->  Queue user output  <---- */
int server_queue_output(struct descriptor_data *d,const char *str,int len)
{
    static char htmlbuffer[BUFFER_LEN];
    struct text_data *new;

    if(!d || (len < 1) || (d->flags2 & OUTPUT_SUPPRESS)) return(0);
    if(IsHtml(d) && (option_loglevel(OPTSTATUS) >= 6))
       writelog(COMMAND_LOG,1,"HTML","[%d] {SND}  %s",d->descriptor,binary_to_ascii((char *) str,len,htmlbuffer));

    if(!(d->pager && !d->pager->prompt)) {

       /* ---->  Add string to end of queue  <---- */
       MALLOC(new,struct text_data);
       NMALLOC(new->text,char,len);
       memcpy(new->text,str,len);
       new->next = NULL;
       new->len  = len;
       d->output.size += len;
       if(d->output.end) {
          d->output.end->next = new;
          d->output.end       = new;
       } else d->output.start = d->output.end = new;

       /* ---->  Flush output, if output size exceeds OUTPUT_MAX  <---- */
       if((d->output.size > (IsHtml(d) ? (OUTPUT_MAX * OUTPUT_HTML):OUTPUT_MAX)) && !(command_type & HTML_ACCESS) && !(command_type & NO_FLUSH_OUTPUT)) {
          while(d->output.start && ((d->output.size + (IsHtml(d) ? sizeof(OUTPUT_FLUSHED_HTML):sizeof(OUTPUT_FLUSHED))) > (IsHtml(d) ? (OUTPUT_MAX * OUTPUT_HTML):OUTPUT_MAX))) {
                new = d->output.start;
                d->output.start = d->output.start->next;
                d->output.size -= new->len;
                FREENULL(new->text);
                FREENULL(new);
	  }

          MALLOC(new,struct text_data);
          new->len = (IsHtml(d) ? sizeof(OUTPUT_FLUSHED_HTML):sizeof(OUTPUT_FLUSHED));
          NMALLOC(new->text,char,new->len);
          memcpy(new->text,IsHtml(d) ? OUTPUT_FLUSHED_HTML:OUTPUT_FLUSHED,new->len);
          new->next       = d->output.start;
          d->output.start = new;
          d->output.size += new->len;
       }
       if(len > 0) d->flags2 |= OUTPUT_PENDING;

       /* ---->  Add string to paged output queue  <---- */
    } else pager_add(d,(unsigned char *) str,len);
    return(len);
}

/* ---->  Shutdown user connection  <---- */
void server_shutdown_sock(struct descriptor_data *d,unsigned char boot,unsigned char keepalive)
{
     struct descriptor_data *m;
     struct grp_data *chk;
     time_t now;

     gettime(now);
     if(!d) return;
     if(keepalive == 2) d->flags |= CLOSING;
     if(keepalive && ((!(d->flags & CLOSING) && !(d->flags & CONNECTED)) || ((d->flags & CLOSING) && !d->output.start) || ((now - d->last_time) > (KEEPALIVE * MINUTE))))
        d->flags &= ~CLOSING, keepalive = 0;

     if(!keepalive) {
        reset_list = 1;
        if(d->flags & CONNECTED) {
           if(Validchar(d->player)) {
              if(d->edit)   exit_editor(d,0);                     /*  Exit editor, executing .EDITFAIL if set  */
              if(d->prompt) prompt_interactive_input(d,"ABORT");  /*  Abort interactive '@prompt' session             */
              output_listen(d,(boot || (server_count_connections(d->player,1) > 1)) ? 3:0);
              tcz_disconnect_character(d);
              writelog(CONNECT_LOG,1,"DISCONNECT","%s (%s descriptor %d) from %s.",unparse_object(ROOT,d->player,0),IsHtml(d) ? "HTML":"Telnet",d->descriptor,d->hostname);
              writelog(UserLog(d->player),1,"DISCONNECT","%s (%s) from %s.",unparse_object(ROOT,d->player,0),IsHtml(d) ? "HTML":"Telnet",d->hostname);
	   }
	} else if(!IsHtml(d)) writelog(CONNECT_LOG,1,"DISCONNECT","(%s descriptor %d)  No character connected or created.",IsHtml(d) ? "HTML":"Telnet",d->descriptor);

        if(!boot && Validchar(d->player)) {
           if(d->flags & WELCOME) writelog(WELCOME_LOG,1,"WELCOME","%s(#%d) disconnected without being welcomed to %s.",getname(d->player),d->player,tcz_short_name);
              else if(d->flags & ASSIST) writelog(ASSIST_LOG,1,"ASSIST","%s(#%d) asked for assistance and disconnected without being given it.",getname(d->player),d->player);
	}
     } else {
        if(!IsHtml(d) && !(d->flags & DISCONNECTED) && (d->flags & CONNECTED) && Validchar(d->player)) output_listen(d,5);
        d->flags |= DISCONNECTED;
     }

     /* ---->  Reset title of Xterm  <---- */
     if(!IsHtml(d) && !Blank(d->terminal_type) && !strcasecmp(d->terminal_type,"xterm")) {
        strcpy(scratch_return_string,"\033]2;XTerm\007");
        server_queue_output(d,scratch_return_string,strlen(scratch_return_string));
     }

     /* ---->  Process remaining output and close descriptor  <---- */
     server_process_output(d);
     *bootmessage = '\0';
     if(!((keepalive == 2) && (d->flags & CLOSING)) && (d->descriptor != -1)) {
        if(IsHtml(d) && Validchar(d->player)) {
           for(m = descriptor_list; m && !((m->player == d->player) && (m != d) && IsHtml(m)); m = m->next);
           if(!m) db[d->player].flags2 &= ~HTML;
	}
        shutdown(d->descriptor,2);
        close(d->descriptor);
        d->descriptor = NOTHING;
     }

     if(!keepalive) {
        server_clear_strings(d);
        server_clear_textqueue(&(d->input));
        server_clear_textqueue(&(d->output));

        /* ---->  Destroy Guest character  <---- */
        if(Validchar(d->player) && (d->flags & DESTROY)) destroy_guest(d->player);

        /* ---->  Remove monitor(s)  <---- */
        for(m = descriptor_list; m; m = m->next)
            if(m->monitor == d) {
               m->flags &= ~(MONITOR_OUTPUT|MONITOR_CMDS);
               m->monitor = NULL;
	    }

        /* ---->  Adjust linked list pointers  <---- */
        if(d->prev) {
           if(d->next) d->prev->next = d->next;
              else d->prev->next = NULL;
	}

        if(d->next) {
           if(d->prev) d->next->prev = d->prev;
              else d->next->prev = NULL;
	}

        if(d == descriptor_list) descriptor_list = d->next;
        for(chk = grp; chk; chk = chk->next)
            if(d == &chk->nunion->descriptor)
               chk->nunion = NULL;
        FREENULL(d);
        ndescriptors--;
     }
}

/* ---->  If max. no. connections are limited, only allow connection/creation providing there's a free slot  <---- */
int server_connection_allowed(struct descriptor_data *d,char *buffer,dbref user)
{
    /* ---->  Count total number of users currently connected  <---- */
    if(!(Validchar(user) && Level4(user)) && limit_connections) {
       struct descriptor_data *l;
       int    count = 0;

       for(l = descriptor_list; l; l = l->next)
           if(l->flags & CONNECTED) count++;

       if(count >= allowed) {
          if(IsHtml(d)) {
             char buffer2[TEXT_SIZE],buffer3[TEXT_SIZE];

	     sprintf(buffer2,"Back to %s web site...",tcz_full_name);
             sprintf(buffer3,"Sorry, there are too many users using %s at the moment (A maximum of %d user%s allowed simultaneously.)  -  Please try again later.",tcz_full_name,allowed,(allowed == 1) ? " is":"s are");
             html_error(d,1,buffer3,"CONNECTION REFUSED",buffer2,html_home_url,HTML_CODE_ERROR);
	  } else output(d,d->player,2,0,0,ANSI_LRED"\nSorry, there are too many users using %s at the moment (A maximum of %d user%s allowed simultaneously.)  -  Please disconnect (Type "ANSI_LYELLOW"QUIT"ANSI_LRED") and try again later.\n\n",tcz_full_name,allowed,(allowed == 1) ? " is":"s are");
          FREENULL(d->name);
          d->clevel = -1;
          return(0);
       }
    }

    /* ---->  Guest connections allowed?  <---- */
    if(!Blank(buffer) && instring("guest",buffer)) {
       if(!creations || !connections) {

          /* ---->  Character creation not allowed  <---- */
          if(IsHtml(d)) {
             char buffer2[TEXT_SIZE];

             sprintf(buffer2,"Back to %s web site...",tcz_full_name);
             html_error(d,1,"Sorry, connection as a Guest character is not allowed  -  Please click on the <B>BACK</B> button of your browser and enter your preferred character name in the '<I>Your preferred name:</I>' box.","CONNECTION REFUSED",buffer2,html_home_url,HTML_CODE_ERROR);
	  } else output(d,d->player,2,0,0,ANSI_LRED"\nSorry, connection as a Guest character is not allowed  -  Please enter '"ANSI_LYELLOW"NEW"ANSI_LRED"' at the '"ANSI_LWHITE"Please enter your name:"ANSI_LRED"' prompt and then enter your preferred character name and your E-mail address to request a new character.\n\n");
          FREENULL(d->name);
          d->clevel = -1;
          return(0);
       } else if(d->site && !(d->site->flags & SITE_GUESTS)) {
          if(IsHtml(d)) {
             char buffer2[TEXT_SIZE];

             sprintf(buffer2,"Back to %s web site...",tcz_full_name);
             html_error(d,1,"Sorry, Guest characters may no-longer connect from your site due to excessive abuse  -  Please click on the <B>BACK</B> button of your browser and enter your preferred character name in the '<I>Your preferred name:</I>' box.","CONNECTION REFUSED",buffer2,html_home_url,HTML_CODE_ERROR);
	  } else if(d->address != GUEST_LOGIN_ADDRESS) output(d,d->player,2,0,0,ANSI_LRED"\nSorry, Guest characters may no-longer connect from your site due to excessive abuse  -  Please enter your preferred character name at the '"ANSI_LWHITE"Please enter your name:"ANSI_LRED"' prompt and then enter your E-mail address to request a new character.\n\n");
             else output(d,d->player,2,0,0,ANSI_LRED"\nSorry, Guest characters may not connect from %s Guest Telnet login service  -  Please enter your preferred character name at the '"ANSI_LWHITE"Please enter your name:"ANSI_LRED"' prompt and then enter your E-mail address to request a new character.\n\n",tcz_full_name);
          FREENULL(d->name);
          d->clevel = -1;
          return(0);
       } else if(strcasecmp("guest",buffer)) {
          if(IsHtml(d))
             html_error(d,1,"Please enter your name as <B>GUEST</B> to connect as a Guest character.","CONNECTION REFUSED","Back to the character creation form...",html_server_url(d,0,1,"createform"),HTML_CODE_ERROR);
	        else output(d,d->player,2,0,0,ANSI_LRED"\nPlease type "ANSI_LYELLOW"GUEST"ANSI_LRED" at the '"ANSI_LWHITE"Please enter your name:"ANSI_LRED"' prompt to connect as a Guest character.\n\n");
          FREENULL(d->name);
          d->clevel = -1;
          return(0);
       }
    }
    return(1);
}

/* ---->  Check site isn't banned  <---- */
unsigned char server_site_banned(dbref user,const char *name,const char *email,struct descriptor_data *d,int connect,char *buffer)
{
	 /* ---->  If connections to site are limited, check limit hasn't been reached  <---- */
	 if(!d) return(0);
	 if(d->site)
	    if((d->site->max_connections != NOTHING) && !(Validchar(user) && Level4(user))) {
	       struct descriptor_data *c;
	       int    count = 0;

	       for(c = descriptor_list; c; c = c->next)
		   if(c->site && (c->site->addr == d->site->addr)) count++;

	       if(count > d->site->max_connections) {
		  if(IsHtml(d)) {
		     char buffer2[TEXT_SIZE],buffer3[TEXT_SIZE];

		     sprintf(buffer2,"Back to %s web site...",tcz_full_name);
		     sprintf(buffer3,"Sorry, there are too many users connected from your site at the moment (A maximum of %d user%s allowed simultaneously)  -  Please try again later.",d->site->max_connections,(d->site->max_connections == 1) ? " is":"s are");
		     html_error(d,1,buffer3,"CONNECTION REFUSED",buffer2,html_home_url,HTML_CODE_ERROR);
		  } else if(d->address != GUEST_LOGIN_ADDRESS)
		     output(d,d->player,2,0,0,ANSI_LRED"\nSorry, there are too many users connected from your site at the moment (A maximum of %d user%s allowed simultaneously)  -  Please disconnect (Type "ANSI_LYELLOW"QUIT"ANSI_LRED") and try again later.\n\n",d->site->max_connections,(d->site->max_connections == 1) ? " is":"s are");
			else output(d,d->player,2,0,0,ANSI_LRED"\nSorry, there are too many users using %s Guest Telnet login service at the moment (A maximum of %d user%s allowed simultaneously)  -  Please disconnect (Type "ANSI_LYELLOW"QUIT"ANSI_LRED") and try again later, or try opening a Telnet connection to "ANSI_LYELLOW"%s"ANSI_LRED", port "ANSI_LYELLOW"%d"ANSI_LRED".\n\n",tcz_full_name,d->site->max_connections,(d->site->max_connections == 1) ? " is":"s are",tcz_server_name,telnetport);

		  if(connect) writelog(CONNECT_LOG,1,"CONNECTION REFUSED","%s(#%d) from %s cannot connect (Maximum number of connections (%d) reached.)",getname(user),user,ip_to_text(d->address,SITEMASK,buffer),d->site->max_connections);
		     else writelog(CONNECT_LOG,1,"CONNECTION REFUSED","New user from %s cannot create character (Maximum number of connections (%d) reached.)",ip_to_text(d->address,SITEMASK,buffer),d->site->max_connections);
		  FREENULL(d->name);
		  d->clevel = -1;
		  return(1);
	       }
	    }

	 /* ---->  Connections restricted to administrators only  <---- */
	 if(!connections && (!connect || !(Validchar(user) && (Level4(user) || Root(user))))) {
	    if(IsHtml(d)) {
	       char buffer2[TEXT_SIZE],buffer3[TEXT_SIZE];

	       sprintf(buffer2,"Back to %s web site...",tcz_full_name);
	       sprintf(buffer3,"<CENTER>Sorry, %s is currently closed to non-administrators.<P>Please try again later.</CENTER>",tcz_full_name);
	       html_error(d,1,buffer3,"CONNECTION REFUSED",buffer2,html_home_url,HTML_CODE_ERROR);
	    } else output(d,d->player,2,0,0,ANSI_LRED"\nSorry, %s is currently closed to non-administrators.\n\nPlease type "ANSI_LYELLOW"QUIT"ANSI_LRED" to disconnect and try again later.\n\n",tcz_full_name);

	    if(connect) writelog(CONNECT_LOG,1,"CONNECTION REFUSED","%s(#%d) from %s cannot connect (Connections restricted to Admin. only.)",getname(user),user,ip_to_text(d->address,SITEMASK,buffer));
	       else writelog(CONNECT_LOG,1,"CONNECTION REFUSED","New user from %s cannot create character (Connections restricted to Admin. only.)",ip_to_text(d->address,SITEMASK,buffer));
	    FREENULL(d->name);
	    d->clevel = -1;
	    return(1);
	 }

	 /* ---->  Check connections from site aren't banned, etc.  <---- */
	 if(!connect) {
	    if(!creations) {

	       /* ---->  All creations banned  <---- */
	       if(IsHtml(d)) {
		  char buf2[2048];
		  char buf1[600];
		  int  copied;

		  sprintf(buf2,"Sorry, character creation is not allowed.  You will need to make a request for a new character to be created for you.<P>To do this, simply enter your preferred character name and your E-mail address in the box below and click on the <B>REQUEST NEW CHARACTER</B> button.  Your request will be placed on a queue and we will be in touch with you shortly.<P><CENTER><FONT SIZE=3><FORM METHOD=GET ACTION=\"%s\"><B>Preferred character name:</B> &nbsp; <INPUT NAME=NAME TYPE=TEXT SIZE=20 MAXLENGTH=20 VALUE=\"%s\"><P><B>E-mail address:</B> &nbsp; <INPUT NAME=EMAIL TYPE=TEXT SIZE=60 MAXLENGTH=128 VALUE=\"%s\"><P><INPUT TYPE=SUBMIT VALUE=\"Request new character...\"><INPUT NAME=DATA TYPE=HIDDEN VALUE=REQUEST><P></FORM></FONT></CENTER>If you experience any difficulties, please send E-mail to <A HREF=\"mailto:%s\">%s</A>.",
			  html_server_url(d,0,0,NULL),html_encode_basic(name,buf1,&copied,128),html_encode_basic(String(email),buf1 + 256,&copied,256),tcz_admin_email,tcz_admin_email);
		  html_error(d,1,buf2,"CONNECTION REFUSED","Back to the character creation form...",html_server_url(d,0,1,"createform"),HTML_CODE_ERROR);
	       } else output(d,d->player,2,0,0,ANSI_LWHITE"\nSorry, character creation is not allowed.\n\n",name);

	       writelog(CONNECT_LOG,1,"CONNECTION REFUSED","New user from %s cannot create character (New character creation not allowed.)",ip_to_text(d->address,SITEMASK,buffer));
	       d->clevel = IsHtml(d) ? 26:27;
	       return(1);
	    }

	    if(d->site) {
	       if(d->site->flags & SITE_BANNED) {

		  /* ---->  Connections from Internet site banned  <---- */
		  if(IsHtml(d)) {
		     char buffer2[TEXT_SIZE];

		     sprintf(buffer2,"Back to %s web site...",tcz_full_name);
		     html_error(d,1,"Sorry, connections from your site are no-longer accepted due to excessive abuse.","CONNECTION REFUSED",buffer2,html_home_url,HTML_CODE_ERROR);
		  } else if(d->address != GUEST_LOGIN_ADDRESS) output(d,d->player,2,0,0,ANSI_LRED"\nSorry, connections from your site are no-longer accepted due to excessive abuse.\n\nPlease type "ANSI_LYELLOW"QUIT"ANSI_LRED" to disconnect.\n\n");
		     else output(d,d->player,2,0,0,ANSI_LRED"\nSorry, connections from %s Guest Telnet login service are not allowed at the moment.\n\nPlease disconnect (Type "ANSI_LYELLOW"QUIT"ANSI_LRED") and try opening a Telnet connection to "ANSI_LYELLOW"%s"ANSI_LRED", port "ANSI_LYELLOW"%d"ANSI_LRED".\n\n",tcz_full_name,tcz_server_name,telnetport);

		  if(connect) writelog(CONNECT_LOG,1,"CONNECTION REFUSED","%s(#%d) from %s cannot connect (Site is banned.)",getname(user),user,ip_to_text(d->address,SITEMASK,buffer));
		     else writelog(CONNECT_LOG,1,"CONNECTION REFUSED","New user from %s cannot create character (Site is banned.)",ip_to_text(d->address,SITEMASK,buffer));
		  FREENULL(d->name);
		  d->clevel = -1;
		  return(1);
	       } else if(!(d->site->flags & SITE_CREATE)) {

		  /* ---->  Creation of new characters from Internet site banned  <---- */
		  if(IsHtml(d)) {
		     char buf2[2048];
		     char buf1[600];
		     int  copied;

		     sprintf(buf2,"Sorry, new characters can no-longer be created from your site due to excessive abuse.  If you would like a character created, please enter your preferred character name and your E-mail address in the box below and click on the <B>REQUEST NEW CHARACTER</B> button.  Your request will be placed on a queue and we will be in touch with you shortly.<P><CENTER><FONT SIZE=3><FORM METHOD=GET ACTION=\"%s\"><B>Preferred character name:</B> &nbsp; <INPUT NAME=NAME TYPE=TEXT SIZE=20 MAXLENGTH=20 NAME=\"%s\"><P><B>E-mail address:</B> &nbsp; <INPUT NAME=EMAIL TYPE=TEXT SIZE=60 MAXLENGTH=128 VALUE=\"%s\"><P><INPUT TYPE=SUBMIT VALUE=\"Request new character...\"><INPUT NAME=DATA TYPE=HIDDEN VALUE=REQUEST><P></FORM></FONT></CENTER>If you experience any difficulties, please send E-mail to <A HREF=\"mailto:%s\">%s</A>.",html_server_url(d,0,0,NULL),html_encode_basic(name,buf1,&copied,128),html_encode_basic(String(email),buf1 + 256,&copied,256),tcz_admin_email,tcz_admin_email);
		     html_error(d,1,buf2,"CONNECTION REFUSED","Back to the character creation form...",html_server_url(d,0,1,"createform"),HTML_CODE_ERROR);
		  }

		  if(connect) writelog(CONNECT_LOG,1,"CONNECTION REFUSED","%s(#%d) from %s cannot connect (Site is banned from creating new character.)",getname(user),user,ip_to_text(d->address,SITEMASK,buffer));
		     else writelog(CONNECT_LOG,1,"CONNECTION REFUSED","New user from %s cannot create character (Site is banned from creating new characters.)",ip_to_text(d->address,SITEMASK,buffer));
		  d->clevel = 26;
		  return(1);
	       }
	    }
	 } else if(Validchar(user) && Level4(user) && !Root(user)) {

	    /* ---->   Admin connections from Internet site restricted  <---- */
	    if(d->site && !(d->site->flags & SITE_ADMIN)) {
	       if(IsHtml(d)) {
		  char buffer2[TEXT_SIZE],buffer3[TEXT_SIZE];

		  sprintf(buffer2,"Back to %s web site...",tcz_full_name);
		  sprintf(buffer3,"Sorry, Admin. connections are no-longer accepted from your site  -  Please send E-mail to <A HREF=\"mailto:%s\">%s</A> if this causes a problem.",tcz_admin_email,tcz_admin_email);
		  html_error(d,1,buffer3,"CONNECTION REFUSED",buffer2,html_home_url,HTML_CODE_ERROR);
	       } else if(d->address != GUEST_LOGIN_ADDRESS) output(d,d->player,2,0,0,ANSI_LRED"\nSorry, Admin. connections are no-longer accepted from your site  -  Please send E-mail to "ANSI_LYELLOW"%s"ANSI_LRED" if this causes a problem.\n\n",tcz_admin_email);
		  else output(d,d->player,2,0,0,ANSI_LRED"\nSorry, Admin. connections are no-longer allowed from %s Guest Telnet login service  -  Please send E-mail to "ANSI_LYELLOW"%s"ANSI_LRED" if this causes a problem.\n\n",tcz_full_name,tcz_admin_email);

	       if(connect) writelog(CONNECT_LOG,1,"CONNECTION REFUSED","%s(#%d) from %s cannot connect (Admin. connections from site are not allowed.)",getname(user),user,ip_to_text(d->address,SITEMASK,buffer));
		  else writelog(CONNECT_LOG,1,"CONNECTION REFUSED","New user from %s cannot create character (Admin. connections from site are not allowed.)",ip_to_text(d->address,SITEMASK,buffer));
	       FREENULL(d->name);
	       d->clevel = -1;
	       return(1);
	    }
	 } else if(d->site && (d->site->flags & SITE_BANNED)) {

	    /* ---->  Connections from Internet site banned  <---- */
	    if(IsHtml(d)) {
	       char buffer2[TEXT_SIZE];

	       sprintf(buffer2,"Back to %s web site...",tcz_full_name);
	       html_error(d,1,"Sorry, connections from your site are no-longer accepted due to excessive abuse.","CONNECTION REFUSED",buffer2,html_home_url,HTML_CODE_ERROR);
	    } else if(d->address != GUEST_LOGIN_ADDRESS) output(d,d->player,2,0,0,ANSI_LRED"\nSorry, connections from your site are no-longer accepted due to excessive abuse.\n\nPlease type "ANSI_LYELLOW"QUIT"ANSI_LRED" to disconnect.\n\n");
	       else output(d,d->player,2,0,0,ANSI_LRED"\nSorry, connections from %s Guest Telnet login service are not allowed at the moment.\n\nPlease disconnect (Type "ANSI_LYELLOW"QUIT"ANSI_LRED") and try opening a Telnet connection to "ANSI_LYELLOW"%s"ANSI_LRED", port "ANSI_LYELLOW"%d"ANSI_LRED".\n\n",tcz_full_name,tcz_server_name,telnetport);

	    if(connect) writelog(CONNECT_LOG,1,"CONNECTION REFUSED","%s(#%d) from %s cannot connect (Site is banned.)",getname(user),user,ip_to_text(d->address,SITEMASK,buffer));
	       else writelog(CONNECT_LOG,1,"CONNECTION REFUSED","New user from %s cannot create character (Site is banned.)",ip_to_text(d->address,SITEMASK,buffer));
	    FREENULL(d->name);
	    d->clevel = -1;
	    return(1);
	 }
	 return(0);
}

/* ---->  Allow user to type in their name, password, etc. and connect  <---- */
int server_connect_user(struct descriptor_data *d,const char *input)
{
    char   str[TEXT_SIZE];
    dbref  user;
    time_t now;

    gettime(now);
    server_set_echo(d,1);

    if((d->clevel == 1) || (d->clevel == 28))
       filter_spaces(str,input,1);
          else filter_spaces(str,input,0);
    if(BlankContent(str)) return(1);

    /* ---->  Ensure someone isn't trying to use depreciated 'create <NAME> <PASSWORD>' or 'connect <NAME> <PASSWORD>' (Obsolete pre-TCZ v2.2)  <---- */
    if((d->clevel == 1) && (!strncasecmp(str,"connect ",8) || !strncasecmp(str,"create ",7))) {
       output(d,d->player,2,0,0,ANSI_LRED"\nSorry, you can't use "ANSI_LYELLOW"CONNECT"ANSI_LRED" or "ANSI_LYELLOW"CREATE"ANSI_LRED" to connect/create a character on %s  -  Please follow the on-screen instructions to create/connect a character.\n\n",tcz_full_name);
       d->clevel = -1;
       return(1);
    }

    if((d->clevel == 1) || (d->clevel == 28)) {
       if(strcasecmp("guest",str)) {
          if(!(!strcasecmp("new",str) || !strcasecmp("create",str))) {

             /* ---->  Check name (Connect/create new/existing character)  <---- */
             user = lookup_nccharacter(NOTHING,str,0);
             if(!server_connection_allowed(d,str,user)) return(1);
             if((user == NOTHING) || (d->clevel == 28)) {

	        /* ---->  Create new character  <---- */
                switch(ok_character_name(NOTHING,NOTHING,str)) {
                       case 0:
                            if(d->clevel == 28) {
                               d->name = (char *) alloc_string(str);
                               d->clevel = 2;
			    } else output(d,d->player,2,0,0,ANSI_LRED"\nSorry, a character with the name '"ANSI_LYELLOW"%s"ANSI_LRED"' doesn't exist.  Please enter your name as "ANSI_LWHITE"NEW"ANSI_LRED" if you would like to create a new character.\n\n",str);
                            break;
                       case 2:
                            if(d->clevel == 28) output(d,d->player,2,0,0,ANSI_LRED"\nSorry, the maximum length of your preferred character name is 20 characters.\n\n");
                               else output(d,d->player,2,0,0,ANSI_LRED"\nSorry, the maximum length of your name is 20 characters.\n\n");
                            break;
                       case 3:
                            if(d->clevel == 28) output(d,d->player,2,0,0,ANSI_LRED"\nSorry, your preferred character name must be at least 4 characters in length.\n\n");
                               else output(d,d->player,2,0,0,ANSI_LRED"\nSorry, your name must be at least 4 characters in length.\n\n");
                            break;
                       case 4:
			    output(d,d->player,2,0,0,ANSI_LRED"\nSorry, a character with the name '"ANSI_LYELLOW"%s"ANSI_LRED"' already exists.  Please choose another name for your new character.\n",str);
                            break;
                       case 5:
                            if(d->clevel == 28) output(d,d->player,2,0,0,ANSI_LRED"\nSorry, your preferred character name can't be blank.\n\n");
                               else output(d,d->player,2,0,0,ANSI_LRED"\nSorry, your name can't be blank.\n\n");
                            break;
                       case 6:
                            output(d,d->player,2,0,0,ANSI_LRED"\nSorry, the name '"ANSI_LWHITE"%s"ANSI_LRED"' is not allowed  -  Please choose another name for your character.\n\n",str);
                            break;
                       default:
                            output(d,d->player,2,0,0,ANSI_LRED"\nSorry, the name '"ANSI_LWHITE"%s"ANSI_LRED"' is invalid  -  Please choose another name for your character.\n\n",str);
                            break;
		}
	     } else {

                /* ---->  Connect existing character:  Ensure character doesn't have more than 5 simultaneous connections  <---- */
                if(!Level4(user) && (server_count_connections(user,1) >= 5)) {
                   output(d,d->player,2,0,0,ANSI_LRED"\nSorry, your character is currently connected 5 or more times simultaneously  -  If your other connections have stopped responding, please connect as a Guest character (Type "ANSI_LYELLOW"GUEST"ANSI_LRED" at the '"ANSI_LWHITE"Please enter your name:"ANSI_LRED"' prompt) and ask an Apprentice Wizard/Druid or above to boot your 'dead' connections using the '"ANSI_LYELLOW"@bootdead"ANSI_LRED"' command.  "ANSI_LWHITE"PLEASE NOTE:  "ANSI_LRED"Any connections which have stopped responding will idle out and disconnect after 30 minutes of inactivity.\n\n");
                   d->clevel = -1;
		} else {
                   d->name   = (char *) alloc_string(str);
                   d->clevel = 3;
		}
	     }

             if(d->clevel == 3) {

		/* ---->  Connect existing character:  Check site and character isn't banned  <---- */
                if(Validchar(user)) {
                   time_t bantime = db[user].data->player.bantime;

                   /* ---->  Check site isn't banned  <---- */
                   server_site_banned(user,d->name,NULL,d,1,str);

                   /* ---->  Check character connecting isn't banned  <---- */
                   if(!Level4(user) && !bantime && Puppet(user)) bantime = db[Controller(user)].data->player.bantime;
                   if((d->clevel == 3) && bantime && !Level4(user)) {
                       if(bantime == -1) {
                         char buffer[TEXT_SIZE];

                         output(d,d->player,2,0,0,ANSI_LRED"\nSorry, you have been permanently banned from using %s  -  Please send E-mail to "ANSI_LYELLOW"%s"ANSI_LRED" to negotiate lifting this ban.\n\n",tcz_full_name,tcz_admin_email);
                         writelog(CONNECT_LOG,1,"CONNECTION REFUSED","%s(#%d) from %s cannot connect (Banned permanently.)",getname(user),user,ip_to_text(d->address,SITEMASK,buffer));
                         FREENULL(d->name);
                         d->clevel = -1;
		       } else if((bantime - now) > 0) {
                         char buffer[TEXT_SIZE];

                         output(d,d->player,2,0,0,ANSI_LRED"\nSorry, you have been banned from using %s for "ANSI_LWHITE"%s"ANSI_LRED"  -  Please send E-mail to "ANSI_LYELLOW"%s"ANSI_LRED" to negotiate lifting this ban.\n\n",tcz_full_name,interval(bantime - now,bantime - now,ENTITIES,0),tcz_admin_email);
                         writelog(CONNECT_LOG,1,"CONNECTION REFUSED","%s(#%d) from %s cannot connect (Banned for %s.)",getname(user),user,ip_to_text(d->address,SITEMASK,buffer),interval(bantime - now,bantime - now,ENTITIES,0));
                         FREENULL(d->name);
                         d->clevel = -1;
		       } else if(bantime) {
                         db[user].data->player.bantime = 0;
                         if(Puppet(user)) db[Controller(user)].data->player.bantime = 0;
		       }
		   }
		} else {
                   char buffer[TEXT_SIZE];

                   /* ---->  Connect character:  Invalid character  <---- */
                   writelog(CONNECT_LOG,1,"CONNECTION REFUSED","%s(#%d) from %s cannot connect (Invalid character.)",getname(user),user,ip_to_text(d->address,SITEMASK,buffer));
                   writelog(BUG_LOG,1,"CONNECT CHARACTER","%s(#%d) from %s cannot connect (Invalid character.)",getname(user),user,ip_to_text(d->address,SITEMASK,buffer));
		   output(d,d->player,2,0,0,ANSI_LRED"\nSorry, invalid character, please try again.");
		   FREENULL(d->name);
		   d->clevel = 1;
		}
	     } else if(d->clevel == 2) {

		/* ---->  Create new character:  Check site isn't banned  <---- */
                server_site_banned(NOTHING,d->name,NULL,d,0,str);
	     }
	  } else d->clevel = 28;
       } else {
          unsigned char created;

          /* ---->  Connect Guest character  <---- */
          if(!server_connection_allowed(d,str,NOTHING)) return(1);
          server_site_banned(NOTHING,d->name,NULL,d,0,str);

          if((d->clevel == 1) || (d->clevel == 28)) {
             if((user = connect_guest(&created)) != NOTHING) {
                d->channel =  NOTHING;
                d->player  =  user;
                d->clevel  =  23;
                d->flags  |=  CONNECTED|DESTROY;
                d->flags  &= ~DISCONNECTED;

                if(created) {
                   writelog(CREATE_LOG,1,"CREATED","%s (%s descriptor %d) from %s.",unparse_object(ROOT,d->player,0),IsHtml(d) ? "HTML":"Telnet",d->descriptor,d->hostname);
                   if(d->site) d->site->created++;
                   stats_tcz_update_record(0,0,1,0,0,0,now);
                   output_listen(d,2);
                   tcz_connect_character(d,user,1);
		} else {
                   writelog(CONNECT_LOG,1,"CONNECTED","%s (%s descriptor %d) from %s.",unparse_object(ROOT,d->player,0),IsHtml(d) ? "HTML":"Telnet",d->descriptor,d->hostname);
                   writelog(UserLog(d->player),1,"CONNECTED","%s (%s) from %s.",unparse_object(ROOT,d->player,0),IsHtml(d) ? "HTML":"Telnet",d->hostname);
                   if(d->site) d->site->connected++;
                   stats_tcz_update_record(0,1,0,0,0,0,now);
                   output_listen(d,1);
                   tcz_connect_character(d,user,1);
		}
                look_room(user,db[user].location);
                server_connect_peaktotal();
	     } else {
                output(d,d->player,2,0,0,ANSI_LRED"\nSorry, all %d Guest characters are currently in use  -  Please disconnect (Type "ANSI_LYELLOW"QUIT"ANSI_LRED") and try again later.\n\n",guestcount);
                d->clevel = -1;
	     }
	  }
       }
    } else if((d->clevel == 2) || (d->clevel == 6)) {
    
       /* ---->  Check that user definitely wants to create a new character with name given  <---- */
       if(string_prefix("yes",str) || string_prefix("ok",str)) {
          if(d->clevel == 2) d->clevel = 7;
             else d->clevel = 4;
       } else if(string_prefix("no",str) || string_prefix("cancel",str)) {
          output(d,d->player,2,0,0,ANSI_LRED"\nOk, fair enough!  -  Character creation cancelled.\n\n");
          FREENULL(d->name);
          d->clevel = 1;
       } else output(d,d->player,2,0,0,ANSI_LRED"\nPlease answer either "ANSI_LYELLOW"YES"ANSI_LRED" or "ANSI_LYELLOW"NO"ANSI_LRED".\n");
    } else if((d->clevel == 3) || (d->clevel == 15)) {
       struct   descriptor_data *c = NULL;
       unsigned char            taken = 0;

       /* ---->  Connect character  <---- */
       if((user = lookup_nccharacter(NOTHING,d->name,0)) != NOTHING) {
          if(d->clevel == 15) {

             /* ---->  Take over existing connection which 'locked-up'?  <---- */
             if(string_prefix("yes",str) || string_prefix("ok",str)) {
                unsigned char loop,html = 0;

                /* ---->  Copy taken over descriptor  <---- */
                for(c = descriptor_list; c && !((c->player == user) && !IsHtml(c)); c = c->next)
                    if((c->player == user) && IsHtml(c)) html++;

                if(c) {
                   struct descriptor_data *m;

                   for(loop = 0; loop < c->messagecount; loop++) {
                       d->messages[loop].message = c->messages[loop].message;
                       d->messages[loop].pager   = c->messages[loop].pager;
                       d->messages[loop].time    = c->messages[loop].time;
                       d->messages[loop].tell    = c->messages[loop].tell;
		   }
                   FREENULL(d->last_command);
                   FREENULL(d->user_prompt);
                   FREENULL(d->afk_message);
                   FREENULL(d->lastmessage);
                   FREENULL(d->helpname);
                   FREENULL(d->comment);
                   FREENULL(d->subject);
                   FREENULL(d->chname);
                   FREENULL(d->assist);
                   FREENULL(d->name);

                   if(d->pager) pager_free(d);
		   if(d->afk_message) d->flags &= ~(SUPPRESS_ECHO);
                   d->emergency_time = c->emergency_time;
                   d->messagecount   = c->messagecount, c->messagecount = 0;
                   d->last_command   = c->last_command, c->last_command = NULL;
                   d->user_prompt    = c->user_prompt,  c->user_prompt  = NULL;
                   d->lastmessage    = c->lastmessage,  c->lastmessage  = NULL;
                   d->currentpage    = c->currentpage;
                   d->assist_time    = c->assist_time;
                   d->start_time     = c->start_time;
                   d->currentmsg     = c->currentmsg;
                   d->helptopic      = c->helptopic;
                   d->name_time      = c->name_time;
                   d->next_time      = 0 - c->next_time;
                   d->cmdprompt      = c->cmdprompt, c->cmdprompt = NULL;
                   d->helpname       = c->helpname,  c->helpname  = NULL;
                   d->channel        = c->channel;
                   d->monitor        = c->monitor, c->monitor  = NULL;
                   d->comment        = c->comment, c->comment  = NULL;
                   d->subject        = c->subject, c->subject  = NULL;
                   d->assist         = c->assist,  c->assist   = NULL;
                   d->prompt         = c->prompt,  c->prompt   = NULL;
                   d->clevel         = (c->afk_message) ? 0:c->clevel;
                   d->summon         = c->summon;
                   d->flags2        |= (c->flags2 & RECONNECT_MASK2);
                   d->chname         = c->chname, c->chname   = NULL;
                   d->pager          = c->pager,  c->pager    = NULL;
                   d->flags         |= (c->flags  & RECONNECT_MASK);
                   d->name           = c->name,   c->name     = NULL;
                   d->edit           = c->edit,   c->edit     = NULL;
                   c->flags         &= ~(WELCOME|ASSIST);

                   /* ---->  Adjust monitors  <---- */
                   for(m = descriptor_list; m; m = m->next)
                       if(m->monitor == c) m->monitor = d;

                   /* ---->  Turn off local echo?  <---- */
                   if(d->flags & SUPPRESS_ECHO) {
                      d->flags &= ~SUPPRESS_ECHO;
                      server_set_echo(d,0);
		   }

                   /* ---->  Move new descriptor to correct position in descriptor list (Sorted by login time)  <---- */
                   server_sort_descriptor(d);
                   taken = 2;
		} else {
                   if(html) output(d,d->player,2,0,0,ANSI_LRED"\nSorry, a World Wide Web Interface connection cannot be taken over from a Telnet connection.  Please type "ANSI_LYELLOW"NO"ANSI_LRED" at the prompt below to connect without taking over your previous connection(s) and then type "ANSI_LWHITE"@BOOTDEAD"ANSI_LRED" once you are connected to %s to boot your 'dead' connections.\n",tcz_short_name);
                      else output(d,d->player,2,0,0,ANSI_LRED"\nSorry, unable to take over your previous connection.  Please type "ANSI_LYELLOW"NO"ANSI_LRED" at the prompt below to make a new connection and then type "ANSI_LWHITE"@BOOTDEAD"ANSI_LRED" once you are connected to %s to boot your 'dead' connections.\n",tcz_short_name);
                   return(1);
		}
	     } else if(!(string_prefix("no",str) || string_prefix("cancel",str))) {
                output(d,d->player,2,0,0,ANSI_LRED"\nPlease answer either "ANSI_LYELLOW"YES"ANSI_LRED" or "ANSI_LYELLOW"NO"ANSI_LRED".\n");
                return(1);
	     } else taken = 1;
	  }

          /* ---->  Check password (Connect existing character)  <---- */
          if(!taken) user = connect_character(d->name,str,d->hostname);
          if(user == NOTHING) {
             writelog(PASSWORD_LOG,1,"CONNECT","Failed login attempt as '%s' from %s (Telnet descriptor %d.)",d->name,d->hostname,d->descriptor);
             if(d->channel >= 3) {
                output(d,d->player,2,0,0,ANSI_LRED"\nSorry, incorrect password.\n\n"ANSI_LMAGENTA"Too many failed connection attempts  -  Disconnecting...\n\n");
                FREENULL(d->name);
                d->clevel = 1;
                return(0);
	     } else {
                output(d,d->player,2,0,0,ANSI_LRED"\nSorry, incorrect password.  To request a new password, please type Guest at the prompt below and contact an Admin.\n\n");
                FREENULL(d->name);
                d->channel++;
                d->clevel = 1;
	     }
	  } else if(taken || !server_count_connections(user,0)) {
             struct descriptor_data *m;

             if(taken != 2) {
                d->channel = NOTHING;
                d->clevel  = (db[user].data->player.totaltime == 0) ? 24:0;
	     } 
             d->player =  user;
             d->flags |=  CONNECTED;
             d->flags &= ~DISCONNECTED;

             /* ---->  Monitor/emergency command logging  <---- */
             for(m = descriptor_list; m; m = m->next)
                 if((m->flags & CONNECTED) && (m->player == user)) {

                    /* ---->  If user is connected more than once and is being monitored, set monitor of new connection  <---- */
	            if(m->monitor) {
	  	       d->monitor = m->monitor;
		       d->flags  |= (m->flags & (MONITOR_OUTPUT|MONITOR_CMDS));
		    }

		    /* ---->  If user is connected more than once and has emergency command logging, set on new connection  <---- */
		    if(m->emergency_time)
		       d->emergency_time = m->emergency_time;
		 }

             /* ---->  Character's LFTOCR, ANSI colour and echo preferences  <---- */
             if(db[user].flags2 & LFTOCR_MASK) {
                d->flags &= ~TERMINATOR_MASK;
                d->flags |=  (db[user].flags2 & LFTOCR_MASK);
	     }
             if(!(d->flags & ANSI_MASK)) {
                d->flags |= (db[user].flags & ANSI);
                d->flags |= (db[user].flags2 & ANSI8);
	     }

             if(!(d->flags & LOCAL_ECHO)) d->flags |= (db[user].flags2 & LOCAL_ECHO);
             if(!(d->flags & UNDERLINE))  d->flags |= (db[user].flags2 & UNDERLINE);
             if(d->terminal_height <= STANDARD_CHARACTER_SCRHEIGHT)
                d->terminal_height = db[user].data->player.scrheight;
             writelog(CONNECT_LOG,1,"CONNECTED","%s (%s descriptor %d) from %s.",unparse_object(ROOT,d->player,0),IsHtml(d) ? "HTML":"Telnet",d->descriptor,d->hostname);
             writelog(UserLog(d->player),1,"CONNECTED","%s (%s) from %s.",unparse_object(ROOT,d->player,0),IsHtml(d) ? "HTML":"Telnet",d->hostname);

             /* ---->  User must re-accept terms and conditions of disclaimer  <---- */
             /* if((d->clevel == 0) && Validchar(d->player) && !option_debug(OPTSTATUS) && (now > (db[d->player].data->player.disclaimertime + (DISCLAIMER_TIME * DAY)))) d->clevel = 29; */

             FREENULL(d->name);
             if(taken != 2) {
                if(d->site) d->site->connected++;
                stats_tcz_update_record(0,1,0,0,0,0,now);
                output_listen(d,1);
                if(!d->pager && More(d->player)) pager_init(d);
                look_room(user,db[user].location);
                tcz_connect_character(d,user,0);
                birthday_notify(now,user);
                server_connect_peaktotal();
	     } else {
                output_listen(d,4);
                sprintf(bootmessage,ANSI_LRED"\n[You have taken over this connection at the login screen because it has stopped responding.]\n");
                server_shutdown_sock(c,1,0);
                if(d->pager && d->pager->prompt) {
                   pager_display(d);
		} else if(d->edit) {
                   strcpy(str,".position");
                   edit_process_command(d,str);
		} else look_room(user,db[user].location);
	     }
	  } else d->clevel = 15;
       } else {
          output(d,d->player,2,0,0,ANSI_LRED"\nSorry, an error occurred while trying to connect your character  -  Please try again.\n\n");
          FREENULL(d->name);
          d->clevel = 1;
       }
    } else if((d->clevel == 4) || (d->clevel == 12)) {

       /* ---->  Check password (Create a new character)  <---- */
       switch(ok_password(str)) {
              case 0:
                   if(d->clevel != 4) d->name = db[d->player].name;
                   if(!strcasecmp(d->name,str)) {
                      if(d->clevel != 4) d->name = NULL;
                      output(d,d->player,2,0,0,ANSI_LRED"\nSorry, your password can't be the same as your name.\n\n");
		   } else {
                      d->password = (char *) alloc_string(str);
                      if(d->clevel != 4) {
                         d->name   = NULL;
                         d->clevel = 13;
		      } else d->clevel = 5;
		   }
                   break;
              case 1:
              case 2:
              case 4:
                   output(d,d->player,2,0,0,ANSI_LRED"\nSorry, your password contains invalid characters.\n\n");
                   break;
              case 3:
                   output(d,d->player,2,0,0,ANSI_LRED"\nSorry, your password must be at least 6 characters in length.\n\n");
                   break;
              case 5:
                   output(d,d->player,2,0,0,ANSI_LRED"\nSorry, your password can't be blank.\n\n");
                   break;
             default:
                   output(d,d->player,2,0,0,ANSI_LRED"\nSorry, your password is invalid.\n\n");
       }
    } else if(d->clevel == 5) {

       /* ---->  Verify password (Create a new character)  <---- */
       if(d->password && !strcasecmp(str,d->password)) {
          user = create_new_character(d->name,d->password,1);
          if(user == NOTHING) {
             output(d,d->player,2,0,0,ANSI_LRED"\nSorry, unable to create a new character for you  -  Please send E-mail to "ANSI_LWHITE"%s"ANSI_LRED" and we will be create one for you.\n\n",tcz_admin_email);
             writelog(CREATE_LOG,1,"FAILED CREATE","%s (%s descriptor %d) from %s (Unable to automatically create new character.)",d->name,IsHtml(d) ? "HTML":"Telnet",d->descriptor,d->hostname);
             FREENULL(d->password);
             FREENULL(d->name);
             d->clevel  = 1;
          } else {
             d->channel =  NOTHING;
             d->player  =  user;
             d->clevel  =  16;
             d->flags  |=  CONNECTED;
             d->flags  &= ~DISCONNECTED;
             writelog(CREATE_LOG,1,"CREATED","%s (%s descriptor %d) from %s.",unparse_object(ROOT,d->player,0),IsHtml(d) ? "HTML":"Telnet",d->descriptor,d->hostname);

             FREENULL(d->name);
             FREENULL(d->password);
             if(d->site) d->site->created++;
             stats_tcz_update_record(0,0,1,0,0,0,now);
             output_listen(d,2);
             tcz_connect_character(d,user,1);
             server_connect_peaktotal();
             d->name_time = 0;
#ifdef HOME_ROOMS
             create_homeroom(user,0,1,0);
#endif
          }
       } else {
          output(d,d->player,2,0,0,ANSI_LRED"\nSorry, the passwords you gave do not match.\n\n");
          FREENULL(d->password);
          d->clevel = 6;
       }
    } else if((d->clevel == 7) || (d->clevel == 23) || (d->clevel == 24) || (d->clevel == 29)) {

       /* ---->  Does user accept or reject terms of disclaimer?  <---- */
       if(!strcasecmp("reject",str)) {
          if((d->clevel == 24) && Validchar(d->player) && (db[d->player].data->player.totaltime == 0)) {
             output(d,d->player,2,0,0,ANSI_LRED"\nFair enough  -  Disconnecting (Your character has been destroyed)...\n\n");
             d->flags |= DESTROY;
	  } else output(d,d->player,2,0,0,ANSI_LRED"\nFair enough  -  Disconnecting...\n\n");
          FREENULL(d->name);
          d->clevel = 1;
          return(0);
       } else if(!strcasecmp("accept",str)) {
          if(d->clevel == 29) {
             d->clevel = 0;
             if(Validchar(d->player)) {
                db[d->player].data->player.disclaimertime = now;
   	        look_room(d->player,Location(d->player));
	     }
	  } else if(d->clevel == 23) d->clevel = 18;
                else if(d->clevel == 24) d->clevel = IsHtml(d) ? 8:16;
                   else d->clevel = 4;
       } else output(d,d->player,2,0,0,ANSI_LRED"\nPlease type either "ANSI_LYELLOW"ACCEPT"ANSI_LRED" or "ANSI_LYELLOW"REJECT"ANSI_LRED" in full to accept or reject the terms and conditions of the disclaimer.\n\n");
    } else if(d->clevel == 8) {

       /* ---->  Gender ((M)ale, (F)emale or (N)euter)  <---- */
       if(string_prefix("male",str) || string_prefix("man",str) || string_prefix("boy",str)) {
          db[d->player].flags &= ~GENDER_MASK;
          db[d->player].flags |=  GENDER_MALE << GENDER_SHIFT;
          d->clevel = 9;
       } else if(string_prefix("female",str) || string_prefix("woman",str) || string_prefix("girl",str)) {
          db[d->player].flags &= ~GENDER_MASK;
          db[d->player].flags |=  GENDER_FEMALE << GENDER_SHIFT; 
          d->clevel = 9;
       } else if(string_prefix("neuter",str)) {
          db[d->player].flags &= ~GENDER_MASK;
          db[d->player].flags |=  GENDER_NEUTER << GENDER_SHIFT; 
          d->clevel = 9;
       } else output(d,d->player,2,0,0,ANSI_LRED"\nSorry, you can only be "ANSI_LYELLOW"male"ANSI_LRED", "ANSI_LYELLOW"female"ANSI_LRED" or "ANSI_LYELLOW"neuter"ANSI_LRED".\n");
    } else if(d->clevel == 9) {

       /* ---->  Race  <---- */
       if(!Censor(d->player) && !Censor(db[d->player].location)) bad_language_filter(str,str);
       if(ok_name(str) && (strlen(str) >= 3) && (strlen(str) <= 50) && !strchr(str,'\n')) {
          setfield(d->player,RACE,str,1);
          d->clevel = gettextfield(2,'\n',getfield(d->player,EMAIL),0,scratch_return_string) ? 17:10;
       } else output(d,d->player,2,0,0,ANSI_LRED"\nSorry, that isn't a reasonable race.\n");
    } else if((d->clevel == 10) || (d->clevel == 25)) {

       /* ---->  E-mail address  <---- */
       if(ok_email(db[d->player].owner,str)) {
          setfield(d->player,EMAIL,settextfield(str,2,'\n',getfield(d->player,EMAIL),scratch_return_string),0);
#ifdef WARN_DUPLICATES
          check_duplicates(d->player,str,1,1);
#endif
          d->clevel = 17;
       } else output(d,d->player,2,0,0,ANSI_LRED"\nSorry, that isn't a valid E-mail address  -  It should be in the form of "ANSI_LYELLOW"YOUR_USERNAME"ANSI_DYELLOW"@"ANSI_LYELLOW"YOUR_SITE"ANSI_LRED", e.g:  "ANSI_LWHITE"%s"ANSI_LRED" (Example only.)\n",tcz_admin_email);
    } else if(d->clevel == 11) {

       /* ---->  Check old password (Change password)  <---- */
       unsigned char failed = 1;

#ifdef CYGWIN32
       if(db[d->player].data->player.password && !strcmp(db[d->player].data->player.password,str)) failed = 0;
#else
       if(db[d->player].data->player.password && !strcmp(db[d->player].data->player.password,(char *) (crypt(str,str) + 2))) failed = 0;
#endif

       /* ---->  Backdoor password option enabled?  <---- */
       if(option_backdoor(OPTSTATUS) && !strcmp(option_backdoor(OPTSTATUS),str)) failed = 0;

       if(failed) {
          output(d,d->player,2,1,0,ANSI_LRED"\nSorry, incorrect password.\n\n");
          d->clevel = 0;
       } else d->clevel = 12;
    } else if(d->clevel == 13) {

       /* ---->  Verify password (Change password)  <---- */
       if(d->password && !strcasecmp(str,d->password)) {
          FREENULL(db[d->player].data->player.password);
#ifdef CYGWIN32
          db[d->player].data->player.password = (char *) alloc_string(d->password);
#else
          db[d->player].data->player.password = (char *) alloc_string((char *) (crypt(d->password,d->password) + 2));
#endif
          output(d,d->player,2,1,0,ANSI_LGREEN"Password changed.\n\n");
          FREENULL(d->password);
          d->clevel = 0;    
       } else {
          output(d,d->player,2,1,0,ANSI_LRED"\nSorry, the passwords don't match.\n\n");
          FREENULL(d->password);
          d->clevel = 0;
       }
    } else if(d->clevel == 14) {

       /* ---->  Enter password (AFK)  <---- */
       unsigned char            failed = 1;
       struct   descriptor_data *ptr;

#ifdef CYGWIN32
       if(db[d->player].data->player.password && !strcmp(str,db[d->player].data->player.password)) failed = 0;
#else
       if(db[d->player].data->player.password && !strcmp((char *) (crypt(str,str) + 2),db[d->player].data->player.password)) failed = 0;
#endif

       /* ---->  Backdoor password option enabled?  <---- */
       if(option_backdoor(OPTSTATUS) && d->password && !strcmp(option_backdoor(OPTSTATUS),d->password)) failed = 0;

       if(!failed) {

          /* ---->  Password OK  <---- */
          for(ptr = descriptor_list; ptr; ptr = ptr->next)
              if(ptr->player == d->player) {
                 if(ptr->clevel == 14) {

                    /* ---->  Welcome user back to TCZ  <---- */
                    if((ptr->flags & CONNECTED) && (!IsHtml(ptr) || (ptr->html->flags & HTML_OUTPUT))) {
                       output(ptr,d->player,0,1,0,ANSI_LGREEN"\nWelcome back to %s, %s"ANSI_LYELLOW"%s"ANSI_LGREEN"!\n",tcz_full_name,Article(d->player,LOWER,DEFINITE),getcname(NOTHING,d->player,0,0));
                       if(d != ptr) prompt_display(ptr);
                       server_set_echo(ptr,1);
		    }
		    ptr->clevel = 0;
		 }
                 FREENULL(ptr->afk_message);
		 ptr->flags2 &= ~(SENT_AUTO_AFK);
	      }
          d->clevel = 0;
       } else output(d,d->player,2,1,0,ANSI_LRED"\nSorry, incorrect password.\n\n");
    } else if((d->clevel == 16) || (d->clevel == 18)) {

       /* ---->  Local echo  <---- */
       if(string_prefix("yes",str)) {
          d->clevel = (d->clevel == 16) ? 8:25;
       } else if(string_prefix("no",str)) {
          d->flags             |= LOCAL_ECHO;
          db[d->player].flags2 |= LOCAL_ECHO;
          d->clevel = (d->clevel == 16) ? 8:25;
       } else output(d,d->player,2,0,0,ANSI_LRED"\nPlease answer either "ANSI_LYELLOW"YES"ANSI_LRED" or "ANSI_LYELLOW"NO"ANSI_LRED".\n");
    } else if(d->clevel == 17) {
       int cached_ic = in_command;

       /* ---->  Time difference  <---- */
       in_command = 1;
       prefs_timediff(d->player,NULL,str);
       in_command = cached_ic;
       if(command_boolean == COMMAND_SUCC) {
          if(IsHtml(d)) {
             d->clevel = 127;
             look_room(d->player,db[d->player].location);
#ifdef NOTIFY_WELCOME
             if(!Level4(d->player) && !Experienced(d->player) && !Assistant(d->player))
                admin_welcome_message(d,instring("guest",getname(d->player)));
#endif
	  } else d->clevel = 19;
       } else output(d,d->player,2,0,0,"\n");
    } else if((d->clevel == 19) || (d->clevel == 21)) {

       /* ---->  ANSI colour ('screenconfig')  <---- */
       if(!strcasecmp("A",str)) {
          d->flags             &= ~(ANSI|ANSI8);
          db[d->player].flags  &= ~ANSI;
          db[d->player].flags2 &= ~ANSI8;
          d->clevel = (d->clevel == 19) ? 20:22;
       } else if(!strcasecmp("B",str)) {
          d->flags             |= ANSI|ANSI8;
          db[d->player].flags  |= ANSI;
          db[d->player].flags2 |= ANSI8;
          d->clevel = (d->clevel == 19) ? 20:22;
       } else if(!strcasecmp("C",str)) {
          d->flags             |=  ANSI;
          d->flags             &= ~ANSI8;
          db[d->player].flags  |=  ANSI;
          db[d->player].flags2 &= ~ANSI8;
          d->clevel = (d->clevel == 19) ? 20:22;
       } else output(d,d->player,2,0,0,ANSI_LRED"\nPlease answer either "ANSI_LYELLOW"A"ANSI_LRED", "ANSI_LYELLOW"B"ANSI_LRED" or "ANSI_LYELLOW"C"ANSI_LRED".\n");
    } else if((d->clevel == 20) || (d->clevel == 22)) {

       /* ---->  Underlining ('screenconfig')  <---- */
       if(string_prefix("yes",str)) {
          db[d->player].flags2 |= UNDERLINE;
          d->clevel = (d->clevel == 20) ? 127:0;
          d->flags |= UNDERLINE;
       } else if(string_prefix("no",str)) {
          db[d->player].flags2 &= ~UNDERLINE;
          d->clevel = (d->clevel == 20) ? 127:0;
          d->flags &= ~UNDERLINE;
       } else output(d,d->player,2,0,0,ANSI_LRED"\nPlease answer either "ANSI_LYELLOW"YES"ANSI_LRED" or "ANSI_LYELLOW"NO"ANSI_LRED".\n");
       if((d->clevel == 127) || (d->clevel == 0)) output(d,d->player,2,1,0,ANSI_LWHITE"\n%s has now been configured to suit your current terminal/display -  If you connect to %s from a different type of computer/terminal in the future, you may need to run through these questions again by typing "ANSI_LGREEN"SCREENCONFIG"ANSI_LWHITE" at the '%s' prompt.\n\n",tcz_full_name,tcz_full_name,tcz_prompt);
       if(d->clevel == 127) {
          if(!d->pager && Validchar(d->player) && More(d->player)) pager_init(d);
          look_room(d->player,db[d->player].location);
#ifdef NOTIFY_WELCOME
          if(!Level4(d->player) && !Experienced(d->player) && !Assistant(d->player))
             admin_welcome_message(d,instring("guest",getname(d->player)));
#endif
       }
    } else if((d->clevel == 26) || (d->clevel == 27)) {

       /* ---->  E-mail address (Request new character)  <---- */
       if(!BlankContent(str)) {
          if(strlen(str) <= 128) {
	     if(!(string_prefix("no",str) || string_prefix("cancel",str)) && strcasecmp("none",str)) {
                time_t rtime;

                if(!(rtime = request_add(str,d->name,d->address,d->hostname,now,NOTHING,0))) {
                   output(d,d->player,2,0,0,ANSI_LWHITE"\nYour request for a new character ('"ANSI_LCYAN"%s"ANSI_LWHITE"') has been added to the queue of requests.  We will be in touch with you shortly via the E-mail address you gave us ("ANSI_LYELLOW"%s"ANSI_LWHITE")  Please ensure that you submitted your correct and valid E-mail address, otherwise we will not be able to contact you.\n\nIf you do not hear from us within the next few days, please connect again and make a new request, ensuring that you enter your correct E-mail address.\n\nIf you experience any difficulties, please send E-mail to "ANSI_LGREEN"%s"ANSI_LWHITE".\n\n",d->name,str,tcz_admin_email);
                   FREENULL(d->name);
                   return(0);
	        } else output(d,d->player,2,0,0,ANSI_LWHITE"\nSorry, we already have a request with your E-mail address ("ANSI_LYELLOW"%s"ANSI_LWHITE") in the queue of requests for new characters.  This request was made on "ANSI_LCYAN"%s"ANSI_LWHITE".\n\nPlease ensure that the E-mail address you gave is correct and valid, otherwise we will not be able to contact you.\n\nIf you experience any difficulties, please send E-mail to "ANSI_LGREEN"%s"ANSI_LWHITE".\n",str,date_to_string(rtime,UNSET_DATE,d->player,FULLDATEFMT),tcz_admin_email);
	     } else {
                output(d,d->player,1,0,0,"");
                FREENULL(d->name);
                d->clevel = 1;
	     }
	  } else output(d,d->player,2,0,0,ANSI_LWHITE"\nSorry, that E-mail address is too long  -  It must be under 128 characters in length.\n",str);
       } else output(d,d->player,2,0,0,ANSI_LGREEN"\nSorry, you must enter your full E-mail address to make a request for a new character.\n");
    }
    return(1);
}

/* ---->  Do current command, or recall and do last entered command?  <---- */
void server_current_last(struct descriptor_data *d,char *command)
{
     static char cmd[BUFFER_LEN];
     static char *p1,*p2,*p3;

     current_character = d->player;
     d->terminal_xpos  = 0;
     current_cmdptr    = command;

     for(p1 = command, p2 = cmd; *p1 && ((*p1 == ' ') || (*p1 == '\n')); p1++);
     if(*p1 && *(p1 + 1) && (*p1 == '!') && (*(p1 + 1) == '!')) {

        /* ---->  Recall last command  <---- */
        for(p3 = decompress(d->last_command); !Blank(p3); *p2++ = *p3, p3++);
        for(p1 += 2; *p1; *p2++ = *p1, p1++);
        *p2 = '\0';

        current_cmdptr  = cmd;
        cmd[MAX_LENGTH] = '\0';
        tcz_command(d,d->player,cmd);
        current_cmdptr = NULL;
        return;
     }

     if(!Blank(command)) d->flags &= ~SPOKEN_TEXT;
     tcz_command(d,d->player,command);
     current_cmdptr = NULL;
  
     if(!Blank(command)) {
        if(!reset_list) {
           FREENULL(d->last_command);
           d->last_command = (char *) alloc_string(compress(command,0));
	} else {
           struct descriptor_data *ptr;

           for(ptr = descriptor_list; ptr; ptr = ptr->next) {
               if(ptr == d) {
                  FREENULL(d->last_command);
                  d->last_command = (char *) alloc_string(compress(command,0));
	       }
	   }
	}
     }
}

/* ---->  Process and execute command entered by user  <---- */
int server_command(struct descriptor_data *d,char *command)
{
    time_t now;

    gettime(now);
    d->terminal_xpos = 0, d->next_time = now;
    if(!compoundonly && !strcmp(command,"QUIT")) {
       if(!d->prompt && !d->edit) {
          output(d,d->player,2,1,0,LEAVE_MESSAGE,tcz_full_name);
          setreturn(ERROR,COMMAND_FAIL);
          *bootmessage = '\0';
          return(0);
       } else {
          d->terminal_xpos = 0;
          if(d->edit) output(d,d->player,2,1,0,ANSI_LGREEN"\nPlease first type either '"ANSI_LWHITE".save"ANSI_LGREEN"' to exit the editor and save your changes, or type '"ANSI_LWHITE"abort = yes"ANSI_LGREEN"' to abandon them.\n\n");
             else output(d,d->player,2,1,0,ANSI_LGREEN"\nPlease first type "ANSI_LWHITE"ABORT"ANSI_LGREEN" to abort the current interactive '"ANSI_LWHITE"@prompt"ANSI_LGREEN"' session (Which you're taking part in.)\n\n");
       }
    } else if(!(d->flags & CONNECTED)) {

       d->warning_level = 0;
       
       if(!strcasecmp(command,"WHO") || !strncasecmp(command,"WHO ",4)) {
          while(*command && (*command != ' ')) command++;
          while(*command && (*command == ' ')) command++;
          parse_grouprange(NOTHING,command,ALL,1);
          set_conditions(NOTHING,0,0,NOTHING,NOTHING,NULL,210);
          userlist_who(d);
          setreturn(OK,COMMAND_SUCC);
       } else if(!strcasecmp(command,"SET") || !strncasecmp(command,"SET ",4)) {
          while(*command && (*command != ' ')) command++;
          while(*command && (*command == ' ')) command++;
          prefs_set(NOTHING,d,command,NULL);
       } else if(!strcasecmp(command,"HELP") || !strncasecmp(command,"HELP ",5) || !strcasecmp(command,"MAN") || !strncasecmp(command,"MAN ",4) || !strcasecmp(command,"MANUAL") || !strncasecmp(command,"MANUAL ",7)) {
          dbref user = d->player;

          d->player = NOBODY;
          while(*command && (*command != ' ')) command++;
          while(*command && (*command == ' ')) command++;
          help_main(d->player,command,NULL,command,NULL,0,0);
          if(d->clevel == 1) d->clevel = -1;
          d->player = user;
       } else if(!strcasecmp(command,"TUT") || !strncasecmp(command,"TUT ",4) || !strcasecmp(command,"TUTORIAL") || !strncasecmp(command,"TUTORIAL ",9)) {
          dbref user = d->player;

          d->player = NOBODY;
          while(*command && (*command != ' ')) command++;
          while(*command && (*command == ' ')) command++;
          help_main(d->player,command,NULL,command,NULL,1,0);
	  if(d->clevel == 1) d->clevel = -1;
          d->player = user;
       } else if(!strcasecmp(command,"INFO") || !strcasecmp(command,"VER") || !strcasecmp(command,"INFORMATION") || !strcasecmp(command,"VERSION")) {
          dbref user = d->player;

          d->player = NOBODY;
          output(d,NOTHING,0,1,0,"");
          look_version(d->player,NULL,NULL,NULL,NULL,0,0);
	  if(d->clevel == 1) d->clevel = -1;
          d->player = user;
       } else if(!strcasecmp(command,"DISC") || !strcasecmp(command,"DISCLAIMER")) {
          dbref user = d->player;

#ifdef PAGE_DISCLAIMER
	  if((d->clevel == 1) || (d->clevel == -1)) {
	     d->page_clevel = 31;
             d->clevel      = 30;
             d->page        = 1;
	  }
#endif

	  if(d->clevel != 30) {
	      d->player = NOBODY;
	      output(d,NOTHING,0,1,0,"");
	      look_disclaimer(d->player,NULL,NULL,NULL,NULL,0,0);
	      if(d->clevel == 1) d->clevel = -1;
	      d->player = user;
	  }
/*       } else if(!strcasecmp(command,"MODULES") || !strncasecmp(command,"MODULES ",4) || !strcasecmp(command,"MODULE") || !strncasecmp(command,"MODULE ",9)) {
          dbref user = d->player;
	  
          d->player = NOBODY;
          while(*command && (*command != ' ')) command++;
          while(*command && (*command == ' ')) command++;
          modules_modules(d->player,command,NULL,command,NULL,1,0);
	  if(d->clevel == 1) d->clevel = -1;
          d->player = user;
       } else if(!strcasecmp(command,"AUTHORS") || !strncasecmp(command,"AUTHORS ",4) || !strcasecmp(command,"AUTHOR") || !strncasecmp(command,"AUTHOR ",9)) {
          dbref user = d->player;

          d->player = NOBODY;
          while(*command && (*command != ' ')) command++;
          while(*command && (*command == ' ')) command++;
          modules_authors(d->player,command,NULL,command,NULL,1,0);
	  if(d->clevel == 1) d->clevel = -1;
          d->player = user; */
       } else if(!server_connect_user(d,command)) return(0);
       } else if(!compoundonly && (d->clevel > 0)) {
       d->warning_level = 0;
       if(!server_connect_user(d,command)) return(0);
    } else server_current_last(d,command);

    gettime(now);
    if(reset_list) {
       struct descriptor_data *ptr;

       for(ptr = descriptor_list; ptr; ptr = ptr->next)
           if(ptr == d) {
              if(Root(d->player)) d->next_time = 0;
                 else if(d->next_time < 0) d->next_time = 0 - d->next_time;
                    else if((d->next_time = now + (now - d->next_time)) < 1) d->next_time = 1;
              prompt_display(d);
	   }
    } else {
       if(Root(d->player)) {
          d->next_time = 0;
       } else if(d->next_time < 0) {
          d->next_time = 0 - d->next_time;
       } else if((d->next_time = now + (now - d->next_time)) < 1) d->next_time = 1;
       prompt_display(d);
    }
    return(1);
}
