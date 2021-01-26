/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| STATISTICS.C  -  Database statistics ('@stats'), memory usage ('@size'),    |
|                  Building Quota usage ('@quota') and object ranking         |
|                  ('@rank'.)                                                 |
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
| Module originally designed and written by:  J.P.Boggis 29/12/1994.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                        https://github.com/smcvey/tcz                        |
`-----------------------------------------------------------------------------'

*/


#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "descriptor_flags.h"
#include "object_types.h"
#include "friend_flags.h"
#include "flagset.h"
#include "search.h"
#include "match.h"
#include "quota.h"

#ifdef USE_PROC
   #include <sys/types.h>
   #include <sys/stat.h>
   #include <fcntl.h>
#else
   #include <sys/resource.h>
   #ifdef LINUX
      #include <sys/time.h>
   #else
      #include <time.h>
   #endif
#endif


/* ---->  Constants for 'rank' command  <---- */
#define RANK_ACTIVE       1   /*  Rank by total time spent active             */
#define RANK_AVGACTIVE    2   /*  Rank by average time spent active           */
#define RANK_AVGIDLE      3   /*  Rank by average time spent idling           */
#define RANK_AVGLOGINS    4   /*  Rank by average time between logins         */
#define RANK_AVGTOTAL     5   /*  Rank by average total time                  */
#define RANK_BALANCE      6   /*  Rank by bank balance                        */
#define RANK_BATTLES      7   /*  Rank by total battles fought (Won + Lost)   */
#define RANK_CREATED      8   /*  Rank by creation date                       */
#define RANK_CREDIT       9   /*  Rank by credit in pocket                    */
#define RANK_CSIZE        10  /*  Rank by compressed database memory usage    */
#define RANK_EXPENDITURE  11  /*  Rank by expenditure this quarter            */
#define RANK_IDLE         12  /*  Rank by total time spent idling             */
#define RANK_INCOME       13  /*  Rank by income this quarter                 */
#define RANK_LAST         14  /*  Rank by last connect time/date              */
#define RANK_LASTUSED     15  /*  Rank by date last used                      */
#define RANK_LOGINS       16  /*  Rank by total number of logins              */
#define RANK_LONGEST      17  /*  Rank by longest connect time                */
#define RANK_LOST         18  /*  Rank by battles lost                        */
#define RANK_PERFORMANCE  19  /*  Rank by combat performance                  */
#define RANK_PROFIT       20  /*  Rank by profit this quarter                 */
#define RANK_QUOTA        21  /*  Rank by Building Quota currently in use (Admin only.)  */
#define RANK_QUOTALIMIT   22  /*  Rank by Building Quota limit (Admin only.)  */
#define RANK_QUOTAEXCESS  23  /*  Rank by exceeded Building Quota limits (Admin only.)  */
#define RANK_TOTAL        24  /*  Rank by total connect time                  */
#define RANK_SCORE        25  /*  Rank by score                               */
#define RANK_SIZE         26  /*  Rank by database memory usage               */
#define RANK_WON          27  /*  Rank by battles won                         */

#define RANK_ASCENDING    1   /*  Ascending sort order   */
#define RANK_DESCENDING   0   /*  Descending sort order  */

#define RANK_ADMIN        1   /*  Rank Apprentice Wizard/Druids and above only  */
#define RANK_FRIENDS      2   /*  Rank friends (On your list) only  */
#define RANK_ENEMIES      3   /*  Rank enemies (On your list) only  */

#define RANK_DAY	  1   /*  Rank by average per day      */
#define RANK_WEEK	  2   /*  Rank by average per week     */
#define RANK_MONTH	  3   /*  Rank by average per month    */
#define RANK_QUARTER	  4   /*  Rank by average per quarter  */


/* ---->  Return percentage  <---- */
const char *stats_percent(double numb1,double numb2)
{
      static char buffer[32];
      double divide;
    
      if(!((numb1 == 0) || (numb2 == 0))) {
         divide = (100 * (numb1 / numb2));
         sprintf(buffer,"(%.1f%%)",divide);
         return(buffer);
      } else return("(0.0%)");
}

/* ---->  Update current TCZ statistics record  <---- */
void stats_tcz_update_record(int peak,int charconnected,int charcreated,int objcreated,int objdestroyed,int shutdowns,time_t now)
{
     int    cached_stat_ptr = stat_ptr;
     struct tm *ctime;

     if(!now) gettime(now);

     /* ---->  New day?  <---- */
     if((stat_ptr < 0) || ((now / DAY) > (stats[stat_ptr].time / DAY))) {
        char buffer[256];

        /* ---->  Update global peak  <---- */
        if(stat_ptr > (STAT_HISTORY - 1)) stat_ptr = (STAT_HISTORY - 1);
        if(stat_days >= 0) stats[STAT_TOTAL].peak += stats[stat_ptr].peak;

        /* ---->  Log day's statistics for future reference  <---- */
        ctime = localtime(&(stats[stat_ptr].time));
        sprintf(buffer,"%s %02d/%02d/%02d",dayabbr[ctime->tm_wday],ctime->tm_mday,ctime->tm_mon + 1,(ctime->tm_year + 1900) % 100);
        writelog(STATS_LOG,0,buffer,"%-10d%-10d%-10d%-10d%-10d%d",stats[stat_ptr].peak,stats[stat_ptr].charconnected,stats[stat_ptr].charcreated,stats[stat_ptr].objcreated,stats[stat_ptr].objdestroyed,stats[stat_ptr].shutdowns);

        /* ---->  Shift entries to make room for new entry  <---- */       
        if(stat_ptr >= (STAT_HISTORY - 1)) {
           int count;

           for(count = 1; count < STAT_HISTORY; count++) {
               stats[count - 1].charconnected = stats[count].charconnected;
               stats[count - 1].objdestroyed  = stats[count].objdestroyed;
               stats[count - 1].charcreated   = stats[count].charcreated;
               stats[count - 1].objcreated    = stats[count].objcreated;
               stats[count - 1].shutdowns     = stats[count].shutdowns;
               stats[count - 1].time          = stats[count].time;
               stats[count - 1].peak          = stats[count].peak;
	   }
        } else stat_ptr++;

        /* ---->  Initialise new stats array entry  <---- */
        stats[stat_ptr].charconnected = 0;
        stats[stat_ptr].objdestroyed  = 0;
        stats[stat_ptr].charcreated   = 0;
        stats[stat_ptr].objcreated    = 0;
        stats[stat_ptr].shutdowns     = 0;
        stats[stat_ptr].time          = ((now / DAY) * DAY);
        stats[stat_ptr].peak          = 0;
        stat_days++;
     } else {

        /* ---->  Don't update crashes/shutdowns if new day  <---- */
        stats[stat_ptr].shutdowns   += shutdowns;
        stats[STAT_TOTAL].shutdowns += shutdowns;
     }

     /* ---->  Update daily stats  <---- */
     if(peak > stats[stat_ptr].peak) stats[stat_ptr].peak = peak;
     stats[stat_ptr].charconnected += charconnected;
     stats[stat_ptr].objdestroyed  += objdestroyed;
     stats[stat_ptr].charcreated   += charcreated;
     stats[stat_ptr].objcreated    += objcreated;

     /* ---->  Update max. stats  <---- */
     if(stats[cached_stat_ptr].charconnected > stats[STAT_MAX].charconnected) stats[STAT_MAX].charconnected = stats[cached_stat_ptr].charconnected;
     if(stats[cached_stat_ptr].objdestroyed  > stats[STAT_MAX].objdestroyed)  stats[STAT_MAX].objdestroyed = stats[cached_stat_ptr].objdestroyed;
     if(stats[cached_stat_ptr].charcreated   > stats[STAT_MAX].charcreated)   stats[STAT_MAX].charcreated = stats[cached_stat_ptr].charcreated;
     if(stats[cached_stat_ptr].objcreated    > stats[STAT_MAX].objcreated)    stats[STAT_MAX].objcreated = stats[cached_stat_ptr].objcreated;
     if(stats[cached_stat_ptr].shutdowns     > stats[STAT_MAX].shutdowns)     stats[STAT_MAX].shutdowns = stats[cached_stat_ptr].shutdowns;
     if(stats[cached_stat_ptr].peak          > stats[STAT_MAX].peak)          stats[STAT_MAX].peak = stats[cached_stat_ptr].peak;

     /* ---->  Update total stats  <---- */
     stats[STAT_TOTAL].charconnected += charconnected;
     stats[STAT_TOTAL].objdestroyed  += objdestroyed;
     stats[STAT_TOTAL].charcreated   += charcreated;
     stats[STAT_TOTAL].objcreated    += objcreated;
}

/* ---->  Update stats ('@stats tcz') to next day (If neccessary) when viewing them  <---- */
void stats_tcz_update(void)
{
     struct descriptor_data *d;
     int    count = 0;

     /* --->  Count total number of users currently connected  <--- */
     for(d = descriptor_list; d; d = d->next)
         if(d->flags & CONNECTED) count++;
     stats_tcz_update_record(count,0,0,0,0,0,0);
}

/* ---->  TCZ server statistics  <---- */
void stats_tcz(dbref player)
{
     struct descriptor_data *p = getdsc(player);
     struct tm *ctime;
     int    loop;

     if(IsHtml(p)) output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
     if(!in_command) {
        if(IsHtml(p)) {
           html_anti_reverse(p,1);
           output(p,player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TD ROWSPAN=2>"ANSI_LCYAN"<I><B>Days logged:</B></I><P>"ANSI_LYELLOW"%dy %dw %dd</TD><TH COLSPAN=3><FONT SIZE=4>"ANSI_LWHITE"<I>Characters</I></FONT></TH><TH COLSPAN=2><FONT SIZE=4>"ANSI_LWHITE"<I>Objects</I></FONT></TH><TH ROWSPAN=2>"ANSI_LCYAN"<I>Crashes/<BR>Shutdowns:</I></TH></TR>",stat_days / 364,(stat_days % 364) / 7,(stat_days % 364) % 7);
           output(p,player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH>"ANSI_LCYAN"<I>Peak:</I></TH><TH>"ANSI_LCYAN"<I>Connected:</I></TH><TH>"ANSI_LCYAN"<I>Created:</I></TH><TH>"ANSI_LCYAN"<I>Created:</I></TH><TH>"ANSI_LCYAN"<I>Destroyed:</I></TH></TR>");
	} else {
           output(p,player,0,1,0,"\n Days logged:   "ANSI_DCYAN"<-----  "ANSI_LWHITE"Characters  "ANSI_DCYAN"------>  <----  "ANSI_LWHITE"Objects  "ANSI_DCYAN"---->   "ANSI_LCYAN"Crashes/");
           output(p,player,0,1,0,ANSI_LYELLOW" %2dy %2dw %2dd    "ANSI_LCYAN"Peak:  Connected:  Created:  Created:   Destroyed:  Shutdowns:\n"ANSI_DCYAN"-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-",stat_days / 364,(stat_days % 364) / 7,(stat_days % 364) % 7);
	}
     }

     /* ---->  Display statistic entries  <---- */
     for(loop = 0; loop <= stat_ptr; loop++) {
         ctime = localtime(&(stats[loop].time));
         if(IsHtml(p)) output(p,player,2,1,0,"\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_GREEN"><I>"ANSI_LGREEN"\016%s %02d/%02d/%02d"ANSI_DGREEN":\016</I></TH><TD>\016"ANSI_LWHITE"%d\016</TD><TD>\016"ANSI_LWHITE"%d\016</TD><TD>\016"ANSI_LWHITE"%d\016</TD><TD BGCOLOR="HTML_TABLE_DGREY">\016"ANSI_LWHITE"%d\016</TD><TD BGCOLOR="HTML_TABLE_DGREY">\016"ANSI_LWHITE"%d\016</TD><TD>\016"ANSI_LWHITE"%d\016</TD></TR>",dayabbr[ctime->tm_wday],ctime->tm_mday,ctime->tm_mon + 1,(ctime->tm_year + 1900) % 100,stats[loop].peak,stats[loop].charconnected,stats[loop].charcreated,stats[loop].objcreated,stats[loop].objdestroyed,stats[loop].shutdowns);
            else output(p,player,0,1,0,ANSI_LGREEN" %s %02d/%02d/%02d"ANSI_DGREEN":  "ANSI_LWHITE"%-7d%-12d%-10d%-11d%-12d%d",dayabbr[ctime->tm_wday],ctime->tm_mday,ctime->tm_mon + 1,(ctime->tm_year + 1900) % 100,stats[loop].peak,stats[loop].charconnected,stats[loop].charcreated,stats[loop].objcreated,stats[loop].objdestroyed,stats[loop].shutdowns);
     }

     /* ---->  Display averages  <---- */
     if(!IsHtml(p)) output(p,player,0,1,0,ANSI_DCYAN"-------------------------------------------------------------------------------");
     if(stat_days > 0) {
        if(IsHtml(p)) output(p,player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_CYAN"><I>"ANSI_LCYAN"Daily average:</I></TH><TH>"ANSI_LWHITE"<I>%d</I></FONT></TH><TH>"ANSI_LWHITE"<I>%d</I></FONT></TH><TH>"ANSI_LWHITE"<I>%d</I></FONT></TH><TH>"ANSI_LWHITE"<I>%d</I></FONT></TH><TH>"ANSI_LWHITE"<I>%d</I></FONT></TH><TH>"ANSI_LWHITE"<I>%d</I></TH></TR>",(stats[STAT_TOTAL].peak + stats[stat_ptr].peak) / stat_days,(stats[STAT_TOTAL].charconnected / stat_days),(stats[STAT_TOTAL].charcreated / stat_days),(stats[STAT_TOTAL].objcreated / stat_days),(stats[STAT_TOTAL].objdestroyed / stat_days),(stats[STAT_TOTAL].shutdowns / stat_days));
           else output(p,player,0,1,0,"Daily average:  "ANSI_LWHITE"%-7d%-12d%-10d%-11d%-12d%d",(stats[STAT_TOTAL].peak + stats[stat_ptr].peak) / stat_days,(stats[STAT_TOTAL].charconnected / stat_days),(stats[STAT_TOTAL].charcreated / stat_days),(stats[STAT_TOTAL].objcreated / stat_days),(stats[STAT_TOTAL].objdestroyed / stat_days),(stats[STAT_TOTAL].shutdowns / stat_days));
     }

     /* ---->  Display maximum  <---- */
     if(IsHtml(p)) output(p,player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_CYAN"><I>"ANSI_LCYAN"Maximum:</I></TH><TH>"ANSI_LWHITE"<I>%d</I></TH><TH>"ANSI_LWHITE"<I>%d</I></TH><TH>"ANSI_LWHITE"<I>%d</I></TH><TH>"ANSI_LWHITE"<I>%d</I></TH><TH>"ANSI_LWHITE"<I>%d</I></TH><TH>"ANSI_LWHITE"<I>%d</I></TH></TR>",stats[STAT_MAX].peak,stats[STAT_MAX].charconnected,stats[STAT_MAX].charcreated,stats[STAT_MAX].objcreated,stats[STAT_MAX].objdestroyed,stats[STAT_MAX].shutdowns);
        else output(p,player,0,1,0,"      Maximum:  "ANSI_LWHITE"%-7d%-12d%-10d%-11d%-12d%d",stats[STAT_MAX].peak,stats[STAT_MAX].charconnected,stats[STAT_MAX].charcreated,stats[STAT_MAX].objcreated,stats[STAT_MAX].objdestroyed,stats[STAT_MAX].shutdowns);

     /* ---->  Display totals  <---- */
     if(IsHtml(p)) output(p,player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_BLUE"><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_CYAN"><FONT SIZE=4><I>"ANSI_LCYAN"Total:</I></FONT></TH><TH><FONT SIZE=4>"ANSI_DCYAN"<I>N/A</I></FONT></TH><TH><FONT SIZE=4>"ANSI_LWHITE"<I>%d</I></FONT></TH><TH><FONT SIZE=4>"ANSI_LWHITE"<I>%d</I></FONT></TH><TH><FONT SIZE=4>"ANSI_LWHITE"<I>%d</I></FONT></TH><TH><FONT SIZE=4>"ANSI_LWHITE"<I>%d</I></FONT></TH><TH><FONT SIZE=4>"ANSI_LWHITE"<I>%d</I></FONT></TH></TR>",stats[STAT_TOTAL].charconnected,stats[STAT_TOTAL].charcreated,stats[STAT_TOTAL].objcreated,stats[STAT_TOTAL].objdestroyed,stats[STAT_TOTAL].shutdowns);
        else output(p,player,0,1,0,"        Total:  "ANSI_DCYAN"N/A    "ANSI_LWHITE"%-12d%-10d%-11d%-12d%d",stats[STAT_TOTAL].charconnected,stats[STAT_TOTAL].charcreated,stats[STAT_TOTAL].objcreated,stats[STAT_TOTAL].objdestroyed,stats[STAT_TOTAL].shutdowns);

     if(IsHtml(p)) {
        output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
        html_anti_reverse(p,0);
     } else if(!in_command) output(p,player,0,1,0,ANSI_DCYAN"-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Character statistics  <---- */
void stats_character(dbref player)
{
     int   deities     = 0;
     int   elders      = 0, delders      = 0;
     int   wizards     = 0, druids       = 0;
     int   apprentices = 0, dapprentices = 0;
     int   retired     = 0, dretired     = 0;
     int   experienced = 0, assistants   = 0;
     int   mortals     = 0, builders     = 0;
     int   puppets     = 0, beings       = 0;
     int   morons      = 0, total        = 0;

     struct descriptor_data *p = getdsc(player);
     dbref  i;

     /* ---->  Count characters of each type  <---- */
     for(i = 0; i < db_top; i++)
         if(Typeof(i) == TYPE_CHARACTER) {
            if(Puppet(i)) puppets++;
            if(Being(i))  beings++;
            total++;

            if(!Moron(i)) {
               if(!Level1(i)) {
   	          if(Level2(i)) {
                     if(Druid(i)) delders++;
                        else elders++;
		  } else if(Level3(i)) {
                     if(Druid(i)) druids++;
                        else wizards++;
		  } else if(Level4(i)) {
                     if(Druid(i)) dapprentices++;
	                else apprentices++;
		  } else if(Retired(i)) {
                     if(RetiredDruid(i)) dretired++;
                        else retired++;
		  } else if(Experienced(i)) experienced++;
                     else if(Assistant(i)) assistants++;
		        else if(Builder(i)) builders++;
                           else mortals++;
	       } else deities++;
	    } else morons++;
	 }

     /* ---->  Display character statistics  <---- */
     if(IsHtml(p)) {
        html_anti_reverse(p,1);
        output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
     }

     if(!in_command) {
        output(p,player,2,1,0,"%sThere %s "ANSI_LWHITE"%d"ANSI_LCYAN" character%s ("ANSI_LWHITE"%d"ANSI_LCYAN" %s, "ANSI_LWHITE"%d"ANSI_LCYAN" %s)...%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER COLSPAN=3><FONT SIZE=4><I>\016"ANSI_LCYAN:"\n ",(total == 1) ? "is":"are",total,Plural(total),puppets,(puppets == 1) ? "is a Puppet":"are Puppets",beings,(beings == 1) ? "is a Being":"are Beings",IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
        if(!IsHtml(p)) output(p,player,0,1,0,separator(79,0,'-','='));
     }

     output(p,player,2,1,0,"%sDeities:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-13s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_MAGENTA"><I>\016"DEITY_COLOUR:DEITY_COLOUR"              ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",deities,IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"",stats_percent(deities,total),IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sElder Wizards:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ELDER_COLOUR:ELDER_COLOUR"        ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",elders,IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"",stats_percent(elders,total),IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sElder Druids:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ELDER_DRUID_COLOUR:ELDER_DRUID_COLOUR"         ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",delders,IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"",stats_percent(delders,total),IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sWizards:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_CYAN"><I>\016"WIZARD_COLOUR:WIZARD_COLOUR"              ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",wizards,IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"",stats_percent(wizards,total),IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sDruids:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_CYAN"><I>\016"DRUID_COLOUR:DRUID_COLOUR"               ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",druids,IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"",stats_percent(druids,total),IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sApprentice Wizards:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"APPRENTICE_COLOUR:APPRENTICE_COLOUR"   ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",apprentices,IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"",stats_percent(apprentices,total),IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sApprentice Druids:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"APPRENTICE_DRUID_COLOUR:APPRENTICE_DRUID_COLOUR"    ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",dapprentices,IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"",stats_percent(dapprentices,total),IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sRetired Wizards:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_RED"><I>\016"RETIRED_COLOUR:RETIRED_COLOUR"      ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",retired,IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"",stats_percent(retired,total),IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sRetired Druids:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_RED"><I>\016"RETIRED_DRUID_COLOUR:RETIRED_DRUID_COLOUR"       ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",dretired,IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"",stats_percent(dretired,total),IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sExperienced Builders:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_RED"><I>\016"EXPERIENCED_COLOUR:EXPERIENCED_COLOUR" ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",experienced,IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"",stats_percent(experienced,total),IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sAssistants:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_RED"><I>\016"ASSISTANT_COLOUR:ASSISTANT_COLOUR"           ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",assistants,IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"",stats_percent(assistants,total),IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sBuilders:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_LGREY"><I>\016"BUILDER_COLOUR:BUILDER_COLOUR"             ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",builders,IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"",stats_percent(builders,total),IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sMortals:%s%-10d%s"ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_LGREY"><I>\016"MORTAL_COLOUR:MORTAL_COLOUR"              ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",mortals,IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"",stats_percent(mortals,total),IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sMorons:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_BLUE"><I>\016"MORON_COLOUR:MORON_COLOUR"               ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",morons,IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"",stats_percent(morons,total),IsHtml(p) ? "\016</I></TH></TR>\016":"\n");

     if(!in_command) {
        if(!IsHtml(p)) output(p,player,0,1,0,separator(79,0,'-','='));
        output(p,player,2,1,0,"%sAn average of "ANSI_LWHITE"%d"ANSI_DWHITE" user%s connect per day and "ANSI_LWHITE"%d"ANSI_DWHITE" new user%s created.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><TD COLSPAN=3>"ANSI_DWHITE"<B>\016":ANSI_DWHITE" ",(stats[STAT_TOTAL].charconnected / stat_days),Plural(stats[STAT_TOTAL].charconnected / stat_days),(stats[STAT_TOTAL].charcreated / stat_days),((stats[STAT_TOTAL].charcreated / stat_days) == 1) ? " is":"s are",IsHtml(p) ? "\016</B></TD></TR></TABLE><BR>\016":"\n\n");
     } else if(IsHtml(p)) output(p,player,1,2,0,"</TABLE>");
     if(IsHtml(p)) html_anti_reverse(p,1);
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Display other DB statistics (Non-objects, e.g:  Array elements, friends, etc.)  <---- */
void stats_others(dbref player,const char *name)
{
     int    subtopics = 0,tsubtopics = 0;
     int    elements  = 0,telements  = 0;
     int    messages  = 0,tmessages  = 0;
     int    aliases   = 0,taliases   = 0;
     int    friends   = 0,tfriends   = 0;
     int    readers   = 0,treaders   = 0;
     int    topics    = 0,ttopics    = 0;
     int    mail      = 0,tmail      = 0;
     int    temp;

     unsigned char             twidth = output_terminal_width(player);
     struct   descriptor_data  *p = getdsc(player);
     struct   bbs_topic_data   *topic,*subtopic;
     struct   alias_data       *alias,*ptr;
     struct   mail_data        *mailitem;
     struct   bbs_message_data *message;
     struct   friend_data      *friend;
     struct   bbs_reader_data  *reader;
     dbref                     owner,i;

     if(!Blank(name)) {
        if((owner = lookup_character(player,name,1)) == NOTHING) {
           output(p,player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",name);
           return;
	}
        if(!Level4(db[player].owner) && !Experienced(db[player].owner) && !can_write_to(player,owner,0) && !(!in_command && friendflags_set(owner,player,NOTHING,FRIEND_READ))) {
           output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only view your own statistics or those of one of your puppets.");
           return;
	}
     } else owner = player;

     /* ---->  Gather BBS statistics (Messages, topics and sub-topics)  <---- */
     for(topic = bbs; topic; topic = topic->next) {
         for(message = topic->messages; message; message = message->next) {
             for(reader = message->readers; reader; reader = reader->next) {
                 if(reader->reader == owner) readers++;
                 treaders++;
	     }
             if(message->owner == owner) messages++;
             tmessages++;
	 }
         if(topic->owner == owner) topics++;
         ttopics++;

         if(topic->subtopics)
            for(subtopic = topic->subtopics; subtopic; subtopic = subtopic->next) {
                for(message = subtopic->messages; message; message = message->next) {
                    for(reader = message->readers; reader; reader = reader->next) {
                        if(reader->reader == owner) readers++;
                        treaders++;
		    }
                    if(message->owner == owner) messages++;
                    tmessages++;
		}
                if(subtopic->owner == owner) subtopics++;
                tsubtopics++;
	    }
     }

     /* ---->  Gather other statistics (Aliases, dynamic array elements, friends, mail items)  <---- */
     for(i = 0; i < db_top; i++)
         switch(Typeof(i)) {
                case TYPE_CHARACTER:

                     /* ---->  Aliases  <---- */
                     for(alias = db[i].data->player.aliases; alias; alias = (alias->next == db[i].data->player.aliases) ? NULL:alias->next) {
                         for(ptr = db[i].data->player.aliases; ptr && !((ptr->id == alias->id) || (ptr == alias)); ptr = (ptr->next == db[i].data->player.aliases) ? NULL:ptr->next);
                         if(ptr && (ptr == alias)) {
                            if(db[i].owner == owner) aliases++;
                            taliases++;
			 }
		     }

                     /* ---->  Friends  <---- */
                     for(friend = db[i].data->player.friends; friend; friend = friend->next) {
                         if(db[i].owner == owner) friends++;
                         tfriends++;
		     }

                     /* ---->  Mail items  <---- */
                     for(mailitem = db[i].data->player.mail; mailitem; mailitem = mailitem->next) {
                         if(db[i].owner == owner) mail++;
                         tmail++;
		     }
                     break;
                case TYPE_ARRAY:
                     temp = array_element_count(db[i].data->array.start);
                     if(db[i].owner == owner) elements += temp;
                     telements += temp;
	 }

     /* ---->  Display other DB statistics  <---- */
     if(IsHtml(p)) {
        html_anti_reverse(p,1);
        output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
     }

     if(!in_command) {
        if(owner != player) output(p,player,2,1,1,"%sOther (Miscellaneous) statistics for %s"ANSI_LWHITE"%s"ANSI_LCYAN"...%s",IsHtml(p) ? (Level4(db[player].owner) || Experienced(db[player].owner)) ? "\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER COLSPAN=4><FONT SIZE=4><I>\016"ANSI_LCYAN:"\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER COLSPAN=3><FONT SIZE=4><I>\016"ANSI_LCYAN:"\n ",Article(owner,LOWER,DEFINITE),getcname(NOTHING,owner,0,0),IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
           else output(p,player,2,1,1,"%sOther (Miscellaneous) statistics for yourself...%s",IsHtml(p) ? (Level4(db[player].owner) || Experienced(db[player].owner)) ? "\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER COLSPAN=4><FONT SIZE=4><I>\016"ANSI_LCYAN:"\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER COLSPAN=3><FONT SIZE=4><I>\016"ANSI_LCYAN:"\n ",IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
        if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
     }

     if(Level4(db[player].owner) || Experienced(db[player].owner)) {
        output(p,player,2,1,19,"%sFriends/enemies:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s"ANSI_LWHITE"...out of a total of %d.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW" ",IsHtml(p) ? "\016</I></TH><TD WIDTH=25%>\016":"  ",friends,IsHtml(p) ? "\016</TD><TD WIDTH=25%>\016":"",stats_percent(friends,tfriends),IsHtml(p) ? "\016</TD><TD ALIGN=LEFT WIDTH=25%>\016":"",tfriends,IsHtml(p) ? "\016</TD></TR>\016":"\n");
        output(p,player,2,1,19,"%sArray elements:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s"ANSI_LWHITE"...out of a total of %d.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"  ",IsHtml(p) ? "\016</I></TH><TD WIDTH=25%>\016":"  ",elements,IsHtml(p) ? "\016</TD><TD WIDTH=25%>\016":"",stats_percent(elements,telements),IsHtml(p) ? "\016</TD><TD ALIGN=LEFT WIDTH=25%>\016":"",telements,IsHtml(p) ? "\016</TD></TR>\016":"\n");
        output(p,player,2,1,19,"%sBBS sub-topics:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s"ANSI_LWHITE"...out of a total of %d.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"  ",IsHtml(p) ? "\016</I></TH><TD WIDTH=25%>\016":"  ",subtopics,IsHtml(p) ? "\016</TD><TD WIDTH=25%>\016":"",stats_percent(subtopics,tsubtopics),IsHtml(p) ? "\016</TD><TD ALIGN=LEFT WIDTH=25%>\016":"",tsubtopics,IsHtml(p) ? "\016</TD></TR>\016":"\n");
        output(p,player,2,1,19,"%sBBS messages:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s"ANSI_LWHITE"...out of a total of %d.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"    ",IsHtml(p) ? "\016</I></TH><TD WIDTH=25%>\016":"  ",messages,IsHtml(p) ? "\016</TD><TD WIDTH=25%>\016":"",stats_percent(messages,tmessages),IsHtml(p) ? "\016</TD><TD ALIGN=LEFT WIDTH=25%>\016":"",tmessages,IsHtml(p) ? "\016</TD></TR>\016":"\n");
        output(p,player,2,1,19,"%sBBS readers:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s"ANSI_LWHITE"...out of a total of %d.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"     ",IsHtml(p) ? "\016</I></TH><TD WIDTH=25%>\016":"  ",readers,IsHtml(p) ? "\016</TD><TD WIDTH=25%>\016":"",stats_percent(readers,treaders),IsHtml(p) ? "\016</TD><TD ALIGN=LEFT WIDTH=25%>\016":"",treaders,IsHtml(p) ? "\016</TD></TR>\016":"\n");
        output(p,player,2,1,19,"%sBBS topics:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s"ANSI_LWHITE"...out of a total of %d.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"      ",IsHtml(p) ? "\016</I></TH><TD WIDTH=25%>\016":"  ",topics,IsHtml(p) ? "\016</TD><TD WIDTH=25%>\016":"",stats_percent(topics,ttopics),IsHtml(p) ? "\016</TD><TD ALIGN=LEFT WIDTH=25%>\016":"",ttopics,IsHtml(p) ? "\016</TD></TR>\016":"\n");
        output(p,player,2,1,19,"%sMail items:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s"ANSI_LWHITE"...out of a total of %d.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"      ",IsHtml(p) ? "\016</I></TH><TD WIDTH=25%>\016":"  ",mail,IsHtml(p) ? "\016</TD><TD WIDTH=25%>\016":"",stats_percent(mail,tmail),IsHtml(p) ? "\016</TD><TD ALIGN=LEFT WIDTH=25%>\016":"",tmail,IsHtml(p) ? "\016</TD></TR>\016":"\n");
        output(p,player,2,1,19,"%sAliases:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s"ANSI_LWHITE"...out of a total of %d.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"         ",IsHtml(p) ? "\016</I></TH><TD WIDTH=25%>\016":"  ",aliases,IsHtml(p) ? "\016</TD><TD WIDTH=25%>\016":"",stats_percent(aliases,taliases),IsHtml(p) ? "\016</TD><TD ALIGN=LEFT WIDTH=25%>\016":"",taliases,IsHtml(p) ? "\016</TD></TR>\016":"\n");
     } else {
        output(p,player,2,1,19,"%sFriends/enemies:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW" ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",friends,IsHtml(p) ? "\016</TD><TD WIDTH=33%>\016":"",stats_percent(friends,tfriends),IsHtml(p) ? "\016</TD></TR>\016":"\n");
        output(p,player,2,1,19,"%sArray elements:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"  ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",elements,IsHtml(p) ? "\016</TD><TD WIDTH=33%>\016":"",stats_percent(elements,telements),IsHtml(p) ? "\016</TD></TR>\016":"\n");
        output(p,player,2,1,19,"%sBBS sub-topics:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"  ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",subtopics,IsHtml(p) ? "\016</TD><TD WIDTH=33%>\016":"",stats_percent(subtopics,tsubtopics),IsHtml(p) ? "\016</TD></TR>\016":"\n");
        output(p,player,2,1,19,"%sBBS messages:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"    ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",messages,IsHtml(p) ? "\016</TD><TD WIDTH=33%>\016":"",stats_percent(messages,tmessages),IsHtml(p) ? "\016</TD></TR>\016":"\n");
        output(p,player,2,1,19,"%sBBS readers:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"     ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",readers,IsHtml(p) ? "\016</TD><TD WIDTH=33%>\016":"",stats_percent(readers,treaders),IsHtml(p) ? "\016</TD></TR>\016":"\n");
        output(p,player,2,1,19,"%sBBS topics:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"      ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",topics,IsHtml(p) ? "\016</TD><TD WIDTH=33%>\016":"",stats_percent(topics,ttopics),IsHtml(p) ? "\016</TD></TR>\016":"\n");
        output(p,player,2,1,19,"%sMail items:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"      ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",mail,IsHtml(p) ? "\016</TD><TD WIDTH=33%>\016":"",stats_percent(mail,tmail),IsHtml(p) ? "\016</TD></TR>\016":"\n");
        output(p,player,2,1,19,"%sAliases:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"         ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",aliases,IsHtml(p) ? "\016</TD><TD WIDTH=33%>\016":"",stats_percent(aliases,taliases),IsHtml(p) ? "\016</TD></TR>\016":"\n");
     }

     if(IsHtml(p)) {
        output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
        html_anti_reverse(p,0);
     } else if(!in_command) output(p,player,0,1,0,separator(twidth,1,'-','='));
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Display bandwidth usage statistics  <---- */
void stats_bandwidth(dbref player)
{
     struct descriptor_data *p = getdsc(player);
     time_t avg;

     gettime(avg);
     avg -= uptime;
     if(IsHtml(p)) {
        html_anti_reverse(p,1);
        output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
     }

     if(!in_command) {
        output(p,player,2,1,12,"%sTotal (Kb):%sAvg. Kb/day:%sAvg. Kb/hour:%sAvg. bytes/second:%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH>&nbsp;</TH><TH><FONT SIZE=4><I>\016"ANSI_LCYAN:ANSI_LCYAN"\n            ",IsHtml(p) ? "\016</I></FONT></TH><TH><FONT SIZE=4><I>\016"ANSI_LCYAN:ANSI_LCYAN"    ",IsHtml(p) ? "\016</I></FONT></TH><TH><FONT SIZE=4><I>\016"ANSI_LCYAN:ANSI_LCYAN"   ",IsHtml(p) ? "\016</I></FONT></TH><TH><FONT SIZE=4><I>\016"ANSI_LCYAN:ANSI_LCYAN"  ",IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
        if(!IsHtml(p)) output(p,player,0,1,0,separator(79,0,'-','='));
     }

     /* ---->  Incoming (User input)  <---- */
     sprintf(scratch_return_string,"%.2f",((double) upper_bytes_in + ((double) lower_bytes_in / KB)));
     sprintf(scratch_return_string + 100,"%.2f",(avg >= DAY) ? ((double) upper_bytes_in / (avg / DAY)):((double) upper_bytes_in + ((double) lower_bytes_in / KB)));
     sprintf(scratch_return_string + 200,"%.2f",(avg >= HOUR)  ? ((double) upper_bytes_in / (avg / HOUR)):((double) upper_bytes_in + ((double) lower_bytes_in / KB)));
     sprintf(scratch_return_string + 300,"%.0f",((double) upper_bytes_in / avg) * KB);
     output(p,player,2,1,0,"%sIncoming:%s"ANSI_LWHITE"%-15s%s"ANSI_LWHITE"%-15s%s"ANSI_LWHITE"%-15s%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=20% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW" ",IsHtml(p) ? "\016</I></TH><TH WIDTH=20%><I>\016":"  ",scratch_return_string,IsHtml(p) ? "\016</I></TH><TD WIDTH=20%>\016":"",scratch_return_string + 100,IsHtml(p) ? "\016</TD><TD WIDTH=20%>\016":"",scratch_return_string + 200,IsHtml(p) ? "\016</TD><TD WIDTH=20%>\016":"",scratch_return_string + 300,IsHtml(p) ? "\016</TD></TR>\016":"\n");

     /* ---->  Outgoing (Output)  <---- */
     sprintf(scratch_return_string,"%.2f",((double) upper_bytes_out + ((double) lower_bytes_out / KB)));
     sprintf(scratch_return_string + 100,"%.2f",(avg >= DAY) ? ((double) upper_bytes_out / (avg / DAY)):((double) upper_bytes_out + ((double) lower_bytes_out / KB)));
     sprintf(scratch_return_string + 200,"%.2f",(avg >= HOUR)  ? ((double) upper_bytes_out / (avg / HOUR)):((double) upper_bytes_out + ((double) lower_bytes_out / KB)));
     sprintf(scratch_return_string + 300,"%.0f",((double) upper_bytes_out / avg) * KB);
     output(p,player,2,1,0,"%sOutgoing:%s"ANSI_LWHITE"%-15s%s"ANSI_LWHITE"%-15s%s"ANSI_LWHITE"%-15s%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=20% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW" ",IsHtml(p) ? "\016</I></TH><TH WIDTH=20%><I>\016":"  ",scratch_return_string,IsHtml(p) ? "\016</I></TH><TD WIDTH=20%>\016":"",scratch_return_string + 100,IsHtml(p) ? "\016</TD><TD WIDTH=20%>\016":"",scratch_return_string + 200,IsHtml(p) ? "\016</TD><TD WIDTH=20%>\016":"",scratch_return_string + 300,IsHtml(p) ? "\016</TD></TR>\016":"\n");

     if(!in_command) {
        if(!IsHtml(p)) output(p,player,0,1,0,separator(79,0,'-','='));
        output(p,player,2,1,18,"%sServer up-time: \016&nbsp;\016 "ANSI_DWHITE"%s.%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_GREY"><TD ALIGN=CENTER COLSPAN=5>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",interval(avg,avg,ENTITIES,0),IsHtml(p) ? "\016</B></TD></TR></TABLE><BR>\016":"\n\n");
     } else if(IsHtml(p)) output(p,player,1,2,0,"</TABLE>");
     if(IsHtml(p)) html_anti_reverse(p,0);
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Display command execution statistics  <---- */
void stats_command(dbref player)
{
     struct descriptor_data *p = getdsc(player);
     long   avg;

     gettime(avg);
     avg -= uptime;
     if(IsHtml(p)) {
        html_anti_reverse(p,1);
        output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
     }

     if(!in_command) {
        if(!IsHtml(p)) {
           output(p,player,0,1,0,ANSI_LGREEN"\n                           Average number of commands executed (Since %s\n                           server was restarted) per...\n\n"ANSI_LCYAN"              Total:       Second:      Minute:      Hour:        Day:",tcz_short_name);
           if(!IsHtml(p)) output(p,player,0,1,0,separator(79,0,'-','='));
	} else output(p,player,2,1,0,"\016<TR><TH COLSPAN=2 BGCOLOR="HTML_TABLE_DGREY">&nbsp;</TH><TH ALIGN=CENTER COLSPAN=4 BGCOLOR="HTML_TABLE_GREEN"><FONT SIZE=4><I>"ANSI_LGREEN"Average number of commands executed (Since %s server was restarted) per...</I></FONT></TH></TR><TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH>&nbsp;</TH><TH><FONT SIZE=4><I>"ANSI_LCYAN"Total:</I></FONT></TH><TH><FONT SIZE=4><I>"ANSI_LCYAN"Second:</I></FONT></TH><TH><FONT SIZE=4><I>"ANSI_LCYAN"Minute:</I></FONT></TH><TH><FONT SIZE=4><I>"ANSI_LCYAN"Hour:</I></FONT></TH><TH><FONT SIZE=4><I>"ANSI_LCYAN"Day:</I></FONT></TH></TR>",tcz_short_name);
     }

     output(p,player,2,1,1,"%sUser typed:%s"ANSI_LWHITE"%-13d%s"ANSI_LWHITE"%-13d%s"ANSI_LWHITE"%-13d%s"ANSI_LWHITE"%-13d%s"ANSI_LWHITE"%d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=20% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW" ",IsHtml(p) ? "\016</I></TH><TH WIDTH=20%><I>\016":"  ",toplevel_commands,IsHtml(p) ? "\016</I></TH><TD WIDTH=20%>\016":"",(avg > 0) ? (toplevel_commands / avg):toplevel_commands,IsHtml(p) ? "\016</TD><TD WIDTH=20%>\016":"",((avg / MINUTE) > 0) ? (toplevel_commands / (avg / MINUTE)):toplevel_commands,IsHtml(p) ? "\016</TD><TD WIDTH=20%>\016":"",((avg / HOUR) > 0) ? (toplevel_commands / (avg / HOUR)):toplevel_commands,IsHtml(p) ? "\016</TD><TD WIDTH=20%>\016":"",((avg / DAY) > 0) ? (toplevel_commands / (avg / DAY)):toplevel_commands,IsHtml(p) ? "\016</TD></TR>\016":"\n");
     if(!IsHtml(p)) output(p,player,0,1,0,separator(79,0,'-','-'));
     output(p,player,2,1,1,"%sStandard:%s"ANSI_LWHITE"%-13d%s"ANSI_LWHITE"%-13d%s"ANSI_LWHITE"%-13d%s"ANSI_LWHITE"%-13d%s"ANSI_LWHITE"%d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=20% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"   ",IsHtml(p) ? "\016</I></TH><TH WIDTH=20%><I>\016":"  ",standard_commands,IsHtml(p) ? "\016</I></TH><TD WIDTH=20%>\016":"",(avg > 0) ? (standard_commands / avg):standard_commands,IsHtml(p) ? "\016</TD><TD WIDTH=20%>\016":"",((avg / MINUTE) > 0) ? (standard_commands / (avg / MINUTE)):standard_commands,IsHtml(p) ? "\016</TD><TD WIDTH=20%>\016":"",((avg / HOUR) > 0) ? (standard_commands / (avg / HOUR)):standard_commands,IsHtml(p) ? "\016</TD><TD WIDTH=20%>\016":"",((avg / DAY) > 0) ? (standard_commands / (avg / DAY)):standard_commands,IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,1,"%sEditor:%s"ANSI_LWHITE"%-13d%s"ANSI_LWHITE"%-13d%s"ANSI_LWHITE"%-13d%s"ANSI_LWHITE"%-13d%s"ANSI_LWHITE"%d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=20% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"     ",IsHtml(p) ? "\016</I></TH><TH WIDTH=20%><I>\016":"  ",editor_commands,IsHtml(p) ? "\016</I></TH><TD WIDTH=20%>\016":"",(avg > 0) ? (editor_commands / avg):editor_commands,IsHtml(p) ? "\016</TD><TD WIDTH=20%>\016":"",((avg / MINUTE) > 0) ? (editor_commands / (avg / MINUTE)):editor_commands,IsHtml(p) ? "\016</TD><TD WIDTH=20%>\016":"",((avg / HOUR) > 0) ? (editor_commands / (avg / HOUR)):editor_commands,IsHtml(p) ? "\016</TD><TD WIDTH=20%>\016":"",((avg / DAY) > 0) ? (editor_commands / (avg / DAY)):editor_commands,IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,1,"%sCompound:%s"ANSI_LWHITE"%-13d%s"ANSI_LWHITE"%-13d%s"ANSI_LWHITE"%-13d%s"ANSI_LWHITE"%-13d%s"ANSI_LWHITE"%d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=20% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"   ",IsHtml(p) ? "\016</I></TH><TH WIDTH=20%><I>\016":"  ",compound_commands,IsHtml(p) ? "\016</I></TH><TD WIDTH=20%>\016":"",(avg > 0) ? (compound_commands / avg):compound_commands,IsHtml(p) ? "\016</TD><TD WIDTH=20%>\016":"",((avg / MINUTE) > 0) ? (compound_commands / (avg / MINUTE)):compound_commands,IsHtml(p) ? "\016</TD><TD WIDTH=20%>\016":"",((avg / HOUR) > 0) ? (compound_commands / (avg / HOUR)):compound_commands,IsHtml(p) ? "\016</TD><TD WIDTH=20%>\016":"",((avg / DAY) > 0) ? (compound_commands / (avg / DAY)):compound_commands,IsHtml(p) ? "\016</TD></TR>\016":"\n");

     if(!in_command) {
        if(!IsHtml(p)) output(p,player,0,1,0,separator(79,0,'-','='));
        output(p,player,2,1,18,"%sServer up-time: \016&nbsp;\016 "ANSI_DWHITE"%s.%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_GREY"><TD ALIGN=CENTER COLSPAN=6>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",interval(avg,avg,ENTITIES,0),IsHtml(p) ? "\016</B></TD></TR></TABLE><BR>\016":"\n\n");
     } else if(IsHtml(p)) output(p,player,1,2,0,"</TABLE>");
     if(IsHtml(p)) html_anti_reverse(p,0);
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Display current resource usages of TCZ server  <---- */
void stats_resource(dbref player)
{
     struct descriptor_data *d,*p = getdsc(player);
     int    psize,inuse = 0,pending = 0;

#ifdef USE_PROC
     int           utime  = 0,stime = 0,cutime = 0,cstime  = 0,starttime = 0;
     unsigned long mtotal = 0,mused = 0,mfree  = 0,mshared = 0,mbuffers  = 0,mcached = 0;
     unsigned long size   = 0,rss   = 0,shrd   = 0,trs     = 0,lib       = 0;
     unsigned long stotal = 0,sused = 0,sfree  = 0,drs     = 0,td        = 0;
     int           systemuptime = 0,fd;

     /* ---->  System uptime  <---- */
     if((fd = open(PROC_UPTIME,O_RDONLY,0)) == -1) {
        output(p,player,0,1,0,ANSI_LGREEN"Sorry, unable to open/access the file '"ANSI_LWHITE""PROC_UPTIME""ANSI_LGREEN"' (System uptime)  -  Unable to determine current resource usages of %s server.",tcz_full_name);
        return;
     }

     read(fd,scratch_return_string,80);
     close(fd);
     systemuptime = atoi(scratch_return_string);

     /* ---->  Memory usage  <---- */
     if((fd = open(PROC_MEMINFO,O_RDONLY,0)) == -1) {
        output(p,player,0,1,0,ANSI_LGREEN"Sorry, unable to open/access the file '"ANSI_LWHITE""PROC_MEMINFO""ANSI_LGREEN"' (Memory usage)  -  Unable to determine current resource usages of %s server.",tcz_full_name);
        return;
     }
     read(fd,scratch_return_string,TEXT_SIZE);
     close(fd);
#ifdef USE_PROC2
     sscanf(scratch_return_string," %*s %*s %*s %*s %*s %*s\n%*s %lu %lu %lu %lu %lu %lu\n%*s %lu %lu %lu%*s",&mtotal,&mused,&mfree,&mshared,&mbuffers,&mcached,&stotal,&sused,&sfree);
#else
     sscanf(scratch_return_string,"%*s %*s %*s %*s %*s %*s %lu %lu %lu %lu %lu %*s %lu %lu %lu%*s",&mtotal,&mused,&mfree,&mshared,&mbuffers,&stotal,&sused,&sfree);
#endif

     /* ---->  TCZ's process statistics  <---- */
     sprintf(scratch_return_string,PROC_STAT,getpid());
     if((fd = open(scratch_return_string,O_RDONLY,0)) == -1) {
        output(p,player,0,1,0,ANSI_LGREEN"Sorry, unable to open/access the file '"ANSI_LWHITE"%s"ANSI_LGREEN"' (%s's process statistics)  -  Unable to determine current resource usages of %s server.",scratch_return_string,tcz_short_name,tcz_full_name);
        return;
     }
     read(fd,scratch_return_string,TEXT_SIZE);
     close(fd);
     sscanf(scratch_return_string,"%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %d %d %d %d %*d %*d %*u %*u %d %*u %*u %*u %*u %*u %*u %*u %*u %*d %*d %*d %*d %*u%*s",&utime,&stime,&cutime,&cstime,&starttime);

     /* ---->  TCZ's process memory statistics  <---- */
     sprintf(scratch_return_string,PROC_STATM,getpid());
     if((fd = open(scratch_return_string,O_RDONLY,0)) == -1) {
        output(p,player,0,1,0,ANSI_LGREEN"Sorry, unable to open/access the file '"ANSI_LWHITE"%s"ANSI_LGREEN"' (%s's process memory statistics)  -  Unable to determine current resource usages of %s server.",scratch_return_string,tcz_short_name,tcz_full_name);
        return;
     }
     read(fd,scratch_return_string,TEXT_SIZE);
     close(fd);
     sscanf(scratch_return_string,"%lu %lu %lu %lu %lu %lu %lu%*s",&size,&rss,&shrd,&trs,&lib,&drs,&td);
#else
     struct rusage usage;

     if(getrusage(RUSAGE_SELF,&usage) == -1) {
        output(p,player,0,1,0,ANSI_LGREEN"Sorry, unable to determine the current resource usages of %s server.",tcz_full_name);
        return;
     }
#endif

     psize = getpagesize();
     for(d = descriptor_list; d; d = d->next) {
         if(!(d->flags & CONNECTED)) pending++;
         if(d->descriptor >= 0) inuse++;
     }

     if(IsHtml(p)) {
        html_anti_reverse(p,1);
        output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_GREY">",(in_command) ? "":"<BR>");
     }

     if(!in_command) {
        output(p,player,2,1,1,"%sCurrent Resource Usages of %s Server...%s",IsHtml(p) ? "\016<TR><TH ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><FONT SIZE=4><I>\016"ANSI_LCYAN:ANSI_LCYAN"\n ",tcz_full_name,IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
        if(!IsHtml(p)) output(p,player,0,1,0,ANSI_DCYAN"-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=("ANSI_LRED"CPU Usage"ANSI_DCYAN")=-");
     }

#ifdef USE_PROC
     output(p,player,2,1,0,"%sCPU time used:%s"ANSI_LWHITE"%dd \016&nbsp;\016%dh \016&nbsp;\016%dm \016&nbsp;\016%ds \016&nbsp;\016%dms%sSystem CPU usage:%s"ANSI_LWHITE"%.2f%%%s",IsHtml(p) ? "\016<TR><TD ALIGN=LEFT CELLPADDING=0><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK"><TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED"><I>\016"ANSI_LRED:ANSI_LRED"           ",
           IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",((((utime + stime + cutime + cstime) / 100) / 60) / 60) / 24,((((utime + stime + cutime + cstime) / 100) / 60) / 60) % 24,(((utime + stime + cutime + cstime) / 100) / 60) % 60,((utime + stime + cutime + cstime) / 100) % 60,((utime + stime + cutime + cstime) % 100) * 10,IsHtml(p) ? "\016</TD></TR><TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED"><I>\016"ANSI_LRED:"\n"ANSI_LRED"        ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",(((float) utime + stime + cutime + cstime) / ((systemuptime * 100) - starttime)) * 100,IsHtml(p) ? "\016</TD></TR></TABLE></TD></TR>\016":"\n");
#else
     output(p,player,2,1,0,"%sUser CPU time used:%s"ANSI_LWHITE"%2dd \016&nbsp;\016%2dh \016&nbsp;\016%2dm \016&nbsp;\016%02ds \016&nbsp;\016%dms%sSystem CPU time used:%s"ANSI_LWHITE"%2dd \016&nbsp;\016%2dh \016&nbsp;\016%2dm \016&nbsp;\016%02ds \016&nbsp;\016%dms%s",
           IsHtml(p) ? "\016<TR><TD ALIGN=LEFT CELLPADDING=0><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK"><TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_RED"><I>\016"ANSI_LRED:ANSI_LRED"              ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",((usage.ru_utime.tv_sec / 60) / 60) / 24,((usage.ru_utime.tv_sec / 60) / 60) % 24,(usage.ru_utime.tv_sec / 60) % 60,usage.ru_utime.tv_sec % 60,usage.ru_utime.tv_usec / 1000,IsHtml(p) ? "\016</TD></TR><TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_RED"><I>\016"ANSI_LRED:"\n"ANSI_LRED"            ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",((usage.ru_stime.tv_sec / 60) / 60) / 24,((usage.ru_stime.tv_sec / 60) / 60) % 24,(usage.ru_stime.tv_sec / 60) % 60,usage.ru_stime.tv_sec % 60,usage.ru_stime.tv_usec / 1000,IsHtml(p) ? "\016</TD></TR></TABLE></TD></TR>\016":"\n");
#endif
     if(!IsHtml(p)) output(p,player,0,1,0,ANSI_DCYAN"---------------------------------------------------------------("ANSI_LYELLOW"Memory Usage"ANSI_DCYAN")--");

     if(IsHtml(p)) output(p,player,1,2,0,"<TR BGCOLOR="HTML_TABLE_DGREY"><TD ALIGN=LEFT CELLPADDING=0><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">");
     *scratch_return_string = '\0';
#ifdef RESTRICT_MEMORY
     if(memory_restriction > 0) sprintf(scratch_return_string," \016&nbsp;\016 "ANSI_LRED"("ANSI_LWHITE"%.2fMb"ANSI_LRED" maximum.)",(float) memory_restriction / MB);
#endif

#ifdef USE_PROC
     output(p,player,2,1,0,"%sProgram data size (DRS):%s"ANSI_LWHITE"%.2fMb \016&nbsp;\016(%d byte%s)%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW" ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",((float) (drs * psize)) / MB,drs * psize,Plural(drs * psize),IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,0,"%sProgram code size (TRS):%s"ANSI_LWHITE"%.2fMb \016&nbsp;\016(%d byte%s)%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW" ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",((float) (trs * psize)) / MB,trs * psize,Plural(trs * psize),IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,0,"%sTotal program size:%s"ANSI_LWHITE"%.2fMb \016&nbsp;\016(%d byte%s)%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"      ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",((float) (size * psize)) / MB,size * psize,Plural(size * psize),scratch_return_string,IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,0,"%sSwap space in use:%s"ANSI_LWHITE"%.2fMb \016&nbsp;\016(%d byte%s)%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"       ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",((float) ((size - rss) * psize)) / MB,(size - rss) * psize,Plural((size - rss) * psize),IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,0,"%sResident size (RSS):%s"ANSI_LWHITE"%.2fMb \016&nbsp;\016(%d byte%s)%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"     ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",((float) (rss * psize)) / MB,rss * psize,Plural(rss * psize),scratch_return_string,IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,0,"%sSystem memory usage:%s"ANSI_LWHITE"%.2f%%%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"     ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",(((float) rss * psize) / mtotal) * 100,IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,0,"%sDescriptors in use:%s"ANSI_LWHITE"%d/%d \016&nbsp;\016(%d connection%s pending)%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LGREEN:ANSI_LGREEN"      ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",inuse + RESERVED_DESCRIPTORS,max_descriptors + RESERVED_DESCRIPTORS,pending,Plural(pending),IsHtml(p) ? "\016</TD></TR>\016":"\n");
#else
     output(p,player,2,1,0,"%sMaximum Resident Set Size (RSS):%s"ANSI_LWHITE"%.2f \016&nbsp;\016(%d byte%s)%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW" ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",((float) (usage.ru_maxrss * psize)) / MB,usage.ru_maxrss * psize,Plural(usage.ru_maxrss * psize),scratch_return_string,IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,0,"%sIntegral shared memory size:%s"ANSI_LWHITE"%.2f \016&nbsp;\016(%d byte%s)%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"     ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",((float) (usage.ru_ixrss * psize)) / MB,usage.ru_ixrss * psize,Plural(usage.ru_ixrss * psize),IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,0,"%sIntegral unshared data size:%s"ANSI_LWHITE"%.2f \016&nbsp;\016(%d byte%s)%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"     ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",((float) (usage.ru_idrss * psize)) / MB,usage.ru_idrss * psize,Plural(usage.ru_idrss * psize),IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,0,"%sIntegral unshared stack size:%s"ANSI_LWHITE"%.2f \016&nbsp;\016(%d byte%s)%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"    ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",((float) (usage.ru_isrss * psize)) / MB,usage.ru_isrss * psize,Plural(usage.ru_isrss * psize),IsHtml(p) ? "\016</TD></TR>\016":"\n");
#endif
     if(IsHtml(p)) output(p,player,1,2,0,"</TABLE></TD></TR>");

#ifdef USE_PROC
     if(!IsHtml(p)) {
        output(p,player,0,1,0,separator(79,0,'=','='));
        output(p,player,0,1,0,ANSI_LCYAN"                      Total:     Used:      Free:      Shared:    Cache:");
        output(p,player,0,1,0,separator(79,0,'-','-'));
     } else {
        output(p,player,1,2,0,"<TR><TD ALIGN=LEFT CELLPADDING=0><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">");
        output(p,player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH>&nbsp;</TH><TH>"ANSI_LCYAN"<I>Total:</I></TH><TH>"ANSI_LCYAN"<I>Used:</I></TH><TH>"ANSI_LCYAN"<I>Free:</I></TH><TH>"ANSI_LCYAN"<I>Shared:</I></TH><TH>"ANSI_LCYAN"<I>Cache:</I></TH></TR>");
     }

     output(p,player,2,1,0,"%sSystem memory (Kb):%s"ANSI_LYELLOW"%-11d%s"ANSI_LWHITE"%-11d%s"ANSI_LWHITE"%-11d%s"ANSI_LGREEN"%-11d%s"ANSI_LGREEN"%-11d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN"><I>\016"ANSI_LCYAN:ANSI_LCYAN" ",IsHtml(p) ? "\016</I></TH><TH WIDTH=14%><I>\016":"  ",mtotal / KB,IsHtml(p) ? "\016</I></TH><TD WIDTH=14%>\016":"",mused / KB,IsHtml(p) ? "\016</TD><TD WIDTH=14%>\016":"",mfree / KB,IsHtml(p) ? "\016</TD><TD WIDTH=14% BGCOLOR="HTML_TABLE_DGREY">\016":"",mshared / KB,IsHtml(p) ? "\016</TD><TD WIDTH=14% BGCOLOR="HTML_TABLE_DGREY">\016":"",(mbuffers + mcached) / KB,IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,0,"%s+/- Cache (Kb):%s"ANSI_DCYAN"N/A%s"ANSI_LWHITE"%-11d%s"ANSI_LWHITE"%-11d%s"ANSI_DCYAN"N/A%s"ANSI_DCYAN"N/A%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN"><I>\016"ANSI_LCYAN:ANSI_LCYAN"     ",IsHtml(p) ? "\016</I></TH><TH WIDTH=14%><I>\016":"  ",IsHtml(p) ? "\016</I></TH><TD WIDTH=14%>\016":"        ",(mused - (mbuffers + mcached)) / KB,IsHtml(p) ? "\016</TD><TD WIDTH=14%>\016":"",(mfree + mbuffers + mcached) / KB,IsHtml(p) ? "\016</TD><TD WIDTH=14% BGCOLOR="HTML_TABLE_DGREY">\016":"",IsHtml(p) ? "\016</TD><TD WIDTH=14% BGCOLOR="HTML_TABLE_DGREY">\016":"        ",IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,0,"%sSystem swap (Kb):%s"ANSI_LYELLOW"%-11d%s"ANSI_LWHITE"%-11d%s"ANSI_LWHITE"%-11d%s"ANSI_DCYAN"N/A%s"ANSI_DCYAN"N/A%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN"><I>\016"ANSI_LCYAN:ANSI_LCYAN"   ",IsHtml(p) ? "\016</I></TH><TH WIDTH=14%><I>\016":"  ",stotal / KB,IsHtml(p) ? "\016</I></TH><TD WIDTH=14%>\016":"",sused / KB,IsHtml(p) ? "\016</TD><TD WIDTH=14%>\016":"",sfree / KB,IsHtml(p) ? "\016</TD><TD WIDTH=14% BGCOLOR="HTML_TABLE_DGREY">\016":"",IsHtml(p) ? "\016</TD><TD WIDTH=14% BGCOLOR="HTML_TABLE_DGREY">\016":"        ",IsHtml(p) ? "\016</TD></TR>\016":"\n");
     if(!IsHtml(p) && !in_command) output(p,player,0,1,0,separator(79,0,'-','-'));
     output(p,player,2,1,0,"%sTotal (Kb):%s"ANSI_LYELLOW"%-11d%s"ANSI_LWHITE"%-11d%s%-11d%s"ANSI_DCYAN"N/A%s"ANSI_DCYAN"N/A%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_BLUE"><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN"><FONT SIZE=4><I>\016"ANSI_LCYAN:ANSI_LCYAN"         ",IsHtml(p) ? "\016</I></FONT></TH><TH WIDTH=14%><FONT SIZE=4><I>\016":"  ",(mtotal + stotal) / KB,IsHtml(p) ? "\016</I></FONT></TH><TD WIDTH=14%><FONT SIZE=4><I>\016":"",(mused + sused) / KB,IsHtml(p) ? "\016</I></FONT></TD><TD WIDTH=14%><FONT SIZE=4><I>\016":"",(mfree + sfree) / KB,IsHtml(p) ? "\016</I></FONT></TD><TD WIDTH=14%><FONT SIZE=4><I>\016":"",IsHtml(p) ? "\016</I></FONT></TD><TD WIDTH=14%><FONT SIZE=4><I>\016":"        ",IsHtml(p) ? "\016</I></FONT></TD></TR>\016":"\n");
     if(IsHtml(p)) output(p,player,1,2,0,"</TABLE></TD></TR>");
#else
     if(!IsHtml(p)) output(p,player,0,1,0,ANSI_DCYAN"----------------------------------------------------("ANSI_LMAGENTA"Swapping "ANSI_DCYAN"& "ANSI_LBLUE"Input/Output"ANSI_DCYAN")--");
        else output(p,player,1,2,0,"<TR><TD ALIGN=LEFT CELLPADDING=0><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">");
     output(p,player,2,1,0,"%sNumber of swaps:%s"ANSI_LWHITE"%d%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_MAGENTA"><I>\016"ANSI_LMAGENTA:ANSI_LMAGENTA"                 ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT COLSPAN=2>\016":"  ",usage.ru_nswap,IsHtml(p) ? "\016</TD></TR>\016":"\n");
     if(IsHtml(p)) {
        output(p,player,2,1,0,"\016<TR><TH ALIGN=RIGHT WIDTH=40%% BGCOLOR="HTML_TABLE_MAGENTA">"ANSI_LMAGENTA"<I>\016Page faults:\016</I></TH><TD ALIGN=LEFT>\016"ANSI_LWHITE"%d "ANSI_LMAGENTA"major\016</TD><TD ALIGN=LEFT>\016"ANSI_LWHITE"%d "ANSI_LMAGENTA"minor\016</TD></TR>\016",usage.ru_majflt,usage.ru_minflt);
        output(p,player,2,1,0,"\016<TR><TH ALIGN=RIGHT WIDTH=40%% BGCOLOR="HTML_TABLE_BLUE">"ANSI_LBLUE"<I>\016Block operations:\016</I></TH><TD ALIGN=LEFT>\016"ANSI_LWHITE"%d "ANSI_LBLUE"input\016</TD><TD ALIGN=LEFT>\016"ANSI_LWHITE"%d "ANSI_LBLUE"output\016</TD></TR>\016",usage.ru_inblock,usage.ru_oublock);
        output(p,player,2,1,0,"\016<TR><TH ALIGN=RIGHT WIDTH=40%% BGCOLOR="HTML_TABLE_BLUE">"ANSI_LBLUE"<I>\016Messages:\016</I></TH><TD ALIGN=LEFT>\016"ANSI_LWHITE"%d "ANSI_LBLUE"received\016</TD><TD ALIGN=LEFT>\016"ANSI_LWHITE"%d "ANSI_LBLUE"sent\016</TD></TR>\016",usage.ru_msgrcv,usage.ru_msgsnd);
        output(p,player,1,2,0,"</TABLE></TD></TR>");
     } else {
        sprintf(scratch_return_string,"%ld "ANSI_LMAGENTA"major",usage.ru_majflt);
        output(p,player,0,1,0,ANSI_LMAGENTA"                     Page faults:  "ANSI_LWHITE"%-34s"ANSI_LWHITE"%d "ANSI_LMAGENTA"minor",scratch_return_string,usage.ru_minflt);
        sprintf(scratch_return_string,"%ld "ANSI_LBLUE"input",usage.ru_inblock);
        output(p,player,0,1,0,ANSI_LBLUE"                Block operations:  "ANSI_LWHITE"%-34s"ANSI_LWHITE"%d "ANSI_LBLUE"output",scratch_return_string,usage.ru_oublock);
        sprintf(scratch_return_string,"%ld "ANSI_LBLUE"received",usage.ru_msgrcv);
        output(p,player,0,1,0,ANSI_LBLUE"                        Messages:  "ANSI_LWHITE"%-34s"ANSI_LWHITE"%d "ANSI_LBLUE"sent",scratch_return_string,usage.ru_msgsnd);
     }
#endif

#ifndef USE_PROC
     if(!IsHtml(p)) output(p,player,0,1,0,ANSI_DCYAN"---------------------------------------------------------------("ANSI_LGREEN"Miscellanous"ANSI_DCYAN")--");
        else output(p,player,1,2,0,"<TR><TD ALIGN=LEFT CELLPADDING=0><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">");
     output(p,player,2,1,0,"%sVoluntarily context switches:%s"ANSI_LWHITE"%d%sInvoluntarily context switches:%s"ANSI_LWHITE"%d%sSignals received:%s"ANSI_LWHITE"%d%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LGREEN:ANSI_LGREEN"    ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",usage.ru_nvcsw,IsHtml(p) ? "\016</TD></TR><TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LGREEN:"\n"ANSI_LGREEN"  ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",usage.ru_nivcsw,IsHtml(p) ? "\016</TD></TR><TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LGREEN:"\n"ANSI_LGREEN"                ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",usage.ru_nsignals,IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,0,"%sDescriptors in use:%s"ANSI_LWHITE"%d/%d \016&nbsp;\016(%d connection%s pending)%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=40% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LGREEN:ANSI_LGREEN"              ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",inuse + RESERVED_DESCRIPTORS,max_descriptors + RESERVED_DESCRIPTORS,pending,Plural(pending),IsHtml(p) ? "\016</TD></TR>\016":"\n");
     if(IsHtml(p)) output(p,player,1,2,0,"</TABLE></TD></TR>");
#endif

     if(IsHtml(p)) {
        output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
        html_anti_reverse(p,0);
     } else if(!in_command) output(p,player,0,1,0,separator(79,1,'-','='));
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Display summary of Telnet and HTML connections  <---- */
void stats_connections(dbref player)
{
     int    ssl_telnet_disconnected = 0, ssl_html_disconnected = 0;
     int    telnet_disconnected     = 0, html_disconnected     = 0;
     int    ssl_telnet_delayed      = 0, ssl_html_delayed      = 0;
     int    telnet_delayed          = 0, html_delayed          = 0;
     int    ssl_telnet_pending      = 0, ssl_html_pending      = 0;
     int    telnet_pending          = 0, html_pending          = 0;
     int    ssl_telnet_active       = 0, ssl_html_active       = 0;
     int    telnet_active           = 0, html_active           = 0;
     int    ssl_telnet_output       = 0, ssl_html_output       = 0;
     int    telnet_output           = 0, html_output           = 0;
     int    ssl_telnet_input        = 0, ssl_html_input        = 0;
     int    telnet_input            = 0, html_input            = 0;
     int    ssl_telnet_total        = 0, ssl_html_total        = 0;
     int    telnet_total            = 0, html_total            = 0;

     struct descriptor_data *d,*p = getdsc(player);
     int    ssl_ok = option_ssl(OPTSTATUS);
     time_t now;

     gettime(now);
     for(d = descriptor_list; d; d = d->next)
         if(IsHtml(d)) {
            if((d->flags & SSL) && ssl_ok) {
#ifdef SSL_SOCKETS
	       if(d->descriptor == NOTHING) ssl_html_disconnected++;
		  else if(!(d->flags & CONNECTED)) ssl_html_pending++;
		     else if((d->next_time > now) && ((d->next_time - now) > 3)) ssl_html_delayed++;
			else ssl_html_active++;
	       if(d->input.start || d->raw_input || d->negbuf) ssl_html_input++;
	       if(d->output.start) ssl_html_output++;
	       ssl_html_total++;
#endif
	    } else {
	       if(d->descriptor == NOTHING) html_disconnected++;
		  else if(!(d->flags & CONNECTED)) html_pending++;
		     else if((d->next_time > now) && ((d->next_time - now) > 3)) html_delayed++;
			else html_active++;
	       if(d->input.start || d->raw_input || d->negbuf) html_input++;
	       if(d->output.start) html_output++;
	       html_total++;
	    }
	 } else {
	    if((d->flags & SSL) && ssl_ok) {
#ifdef SSL_SOCKETS
	       if(d->descriptor == NOTHING) ssl_telnet_disconnected++;
		  else if(!(d->flags & CONNECTED)) ssl_telnet_pending++;
		     else if((d->next_time > now) && ((d->next_time - now) > 3)) ssl_telnet_delayed++;
			else ssl_telnet_active++;
	       if(d->input.start || d->raw_input || d->negbuf) ssl_telnet_input++;
	       if(d->output.start) ssl_telnet_output++;
	       ssl_telnet_total++;
#endif
	    } else {
	       if(d->descriptor == NOTHING) telnet_disconnected++;
		  else if(!(d->flags & CONNECTED)) telnet_pending++;
		     else if((d->next_time > now) && ((d->next_time - now) > 3)) telnet_delayed++;
			else telnet_active++;
	       if(d->input.start || d->raw_input || d->negbuf) telnet_input++;
	       if(d->output.start) telnet_output++;
	       telnet_total++;
	    }
	 }

     if(IsHtml(p)) {
        html_anti_reverse(p,1);
        output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
     }

     if(!in_command) {
        output(p,player,2,1,1,"%sSummary of current Telnet and HTML connections...%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_GREEN"><TH ALIGN=CENTER COLSPAN=8><FONT SIZE=5><I>\016"ANSI_LGREEN:ANSI_LGREEN"\n ",IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n\n");
        if(!IsHtml(p)) {
           output(p,player,0,1,0,separator(79,0,'-','='));
           output(p,player,0,1,0,ANSI_LYELLOW"              Total:  Pending:  Active:  Delayed:  Disconnct:  "ANSI_LCYAN"Input:  Output:");
           output(p,player,0,1,0,separator(79,0,'-','-'));
	} else output(p,player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_YELLOW"><TH>&nbsp;</TH><TH><I>"ANSI_LYELLOW"Total:</I></TH><TH><I>"ANSI_LYELLOW"Pending:</I></TH><TH><I>"ANSI_LYELLOW"Active:</I></TH><TH><I>"ANSI_LYELLOW"Delayed:</I></TH><TH><I>"ANSI_LYELLOW"Disconnected:</I></TH><TH BGCOLOR="HTML_TABLE_CYAN"><I>"ANSI_LCYAN"Input:</I></TH><TH BGCOLOR="HTML_TABLE_CYAN"><I>"ANSI_LCYAN"Output:</I></TH></TR>\016");
     }

     output(p,player,2,1,0,"%sTelnet:%s"ANSI_LWHITE"%-8d%s"ANSI_LWHITE"%-10d%s"ANSI_LWHITE"%-9d%s"ANSI_LWHITE"%-10d%s"ANSI_LWHITE"%-12d%s"ANSI_LWHITE"%-8d%s"ANSI_LWHITE"%d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"     ",IsHtml(p) ? "\016</I></TH><TD><I>\016":"  ",telnet_total,IsHtml(p) ? "\016</I></TD><TD>\016":"",telnet_pending,IsHtml(p) ? "\016</TD><TD>\016":"",telnet_active,IsHtml(p) ? "\016</TD><TD>\016":"",telnet_delayed,IsHtml(p) ? "\016</TD><TD>\016":"",telnet_disconnected,IsHtml(p) ? "\016</TD><TD><I>\016":"",telnet_input,IsHtml(p) ? "\016</I></TD><TD><I>\016":"",telnet_output,IsHtml(p) ? "\016</I></TD></TR>\016":"\n");
#ifdef SSL_SOCKETS
     if(ssl_ok) output(p,player,2,1,0,"%sSSL Telnet:%s"ANSI_LWHITE"%-8d%s"ANSI_LWHITE"%-10d%s"ANSI_LWHITE"%-9d%s"ANSI_LWHITE"%-10d%s"ANSI_LWHITE"%-12d%s"ANSI_LWHITE"%-8d%s"ANSI_LWHITE"%d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW" ",IsHtml(p) ? "\016</I></TH><TD><I>\016":"  ",ssl_telnet_total,IsHtml(p) ? "\016</I></TD><TD>\016":"",ssl_telnet_pending,IsHtml(p) ? "\016</TD><TD>\016":"",ssl_telnet_active,IsHtml(p) ? "\016</TD><TD>\016":"",ssl_telnet_delayed,IsHtml(p) ? "\016</TD><TD>\016":"",ssl_telnet_disconnected,IsHtml(p) ? "\016</TD><TD><I>\016":"",ssl_telnet_input,IsHtml(p) ? "\016</I></TD><TD><I>\016":"",ssl_telnet_output,IsHtml(p) ? "\016</I></TD></TR>\016":"\n");
#endif
     output(p,player,2,1,0,"%sHTML:%s"ANSI_LWHITE"%-8d%s"ANSI_LWHITE"%-10d%s"ANSI_LWHITE"%-9d%s"ANSI_LWHITE"%-10d%s"ANSI_LWHITE"%-12d%s"ANSI_LWHITE"%-8d%s"ANSI_LWHITE"%d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"       ",IsHtml(p) ? "\016</I></TH><TD><I>\016":"  ",html_total,IsHtml(p) ? "\016</I></TD><TD>\016":"",html_pending,IsHtml(p) ? "\016</TD><TD>\016":"",html_active,IsHtml(p) ? "\016</TD><TD>\016":"",html_delayed,IsHtml(p) ? "\016</TD><TD>\016":"",html_disconnected,IsHtml(p) ? "\016</TD><TD><I>\016":"",html_input,IsHtml(p) ? "\016</I></TD><TD><I>\016":"",html_output,IsHtml(p) ? "\016</I></TD></TR>\016":"\n");
#ifdef SSL_SOCKETS
     if(ssl_ok) output(p,player,2,1,0,"%sSSL HTML:%s"ANSI_LWHITE"%-8d%s"ANSI_LWHITE"%-10d%s"ANSI_LWHITE"%-9d%s"ANSI_LWHITE"%-10d%s"ANSI_LWHITE"%-12d%s"ANSI_LWHITE"%-8d%s"ANSI_LWHITE"%d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"   ",IsHtml(p) ? "\016</I></TH><TD><I>\016":"  ",ssl_html_total,IsHtml(p) ? "\016</I></TD><TD>\016":"",ssl_html_pending,IsHtml(p) ? "\016</TD><TD>\016":"",ssl_html_active,IsHtml(p) ? "\016</TD><TD>\016":"",ssl_html_delayed,IsHtml(p) ? "\016</TD><TD>\016":"",ssl_html_disconnected,IsHtml(p) ? "\016</TD><TD><I>\016":"",ssl_html_input,IsHtml(p) ? "\016</I></TD><TD><I>\016":"",ssl_html_output,IsHtml(p) ? "\016</I></TD></TR>\016":"\n");
#endif

     if(!IsHtml(p)) output(p,player,0,1,0,separator(79,0,'-','-'));
     output(p,player,2,1,0,"%sTotal:%s"ANSI_LWHITE"%-8d%s"ANSI_LWHITE"%-10d%s"ANSI_LWHITE"%-9d%s"ANSI_LWHITE"%-10d%s"ANSI_LWHITE"%-12d%s"ANSI_LWHITE"%-8d%s"ANSI_LWHITE"%d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_DGREY"><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><FONT SIZE=4><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"      ",IsHtml(p) ? "\016</I></FONT></TH><TH><FONT SIZE=4><I>\016":"  ",
           telnet_total + html_total + ssl_telnet_total + ssl_html_total,IsHtml(p) ? "\016</I></FONT></TH><TH><FONT SIZE=4>\016":"",
           telnet_pending + html_pending + ssl_telnet_pending + ssl_html_pending,IsHtml(p) ? "\016</FONT></TH><TH><FONT SIZE=4>\016":"",
           telnet_active + html_active + ssl_telnet_active + ssl_html_active,IsHtml(p) ? "\016</FONT></TH><TH><FONT SIZE=4>\016":"",
           telnet_delayed + html_delayed + ssl_telnet_delayed + ssl_html_delayed,IsHtml(p) ? "\016</FONT></TH><TH><FONT SIZE=4>\016":"",
           telnet_disconnected + html_disconnected + ssl_telnet_disconnected + ssl_html_disconnected,IsHtml(p) ? "\016</FONT></TH><TH><FONT SIZE=4><I>\016":"",
           telnet_input + html_input + ssl_telnet_input + ssl_html_input,IsHtml(p) ? "\016</I></FONT></TH><TH><FONT SIZE=4><I>\016":"",
           telnet_output + html_output + ssl_telnet_output + ssl_html_output,IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");

     if(IsHtml(p)) {
        output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
        html_anti_reverse(p,0);
     } else if(!in_command) output(p,player,0,1,0,separator(79,1,'-','='));
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Display DB statistics  <---- */
void stats_statistics(CONTEXT)
{
     setreturn(ERROR,COMMAND_FAIL);
     if(!strcasecmp(params,"other") || !strcasecmp(params,"others") || !strncasecmp(params,"other ",6) || !strncasecmp(params,"others ",7)) {

        /* ---->  Other DB statistics ('@stats other')  <---- */
        for(; *params && (*params != ' '); params++);
        for(; *params && (*params == ' '); params++);
        stats_others(player,params);
        return;
     } else if(!strcasecmp(params,"tcz") || !strcasecmp(params,"server")) {

        /* ---->  TCZ server statistics ('@stats tcz')  <---- */
        if(Level4(db[player].owner)) {
           stats_tcz_update();
           stats_tcz(player);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may see %s statistics.",tcz_full_name);
        return;
     } else if(!strcasecmp(params,"characters") || !strcasecmp(params,"character") || !strcasecmp(params,"chars") || !strcasecmp(params,"char")) {

        /* ---->  Character statistics ('@stats char')  <---- */
        if(Level4(db[player].owner)) stats_character(player);
	   else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may see the character type statistics.");
        return;
     } else if(!strcasecmp(params,"bandwidth") || !strcasecmp(params,"bandwidthusages")) {
        
        /* ---->  Bandwidth usage statistics ('@stats bandwidth')  <---- */
        if(Level4(db[player].owner)) stats_bandwidth(player);
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may see the bandwidth usage statistics.");
        return;
     } else if(!strcasecmp(params,"command") || !strcasecmp(params,"cmd") || !strcasecmp(params,"commands") || !strcasecmp(params,"cmds") || !strcasecmp(params,"comm") || !strcasecmp(params,"com") || !strcasecmp(params,"compoundcommand") || !strcasecmp(params,"compoundcommands")) {
        
        /* ---->  Command execution statistics ('@stats command')  <---- */
        if(Level4(db[player].owner)) stats_command(player);
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may see the command execution statistics.");
        return;
     } else if(!strcasecmp(params,"resource") || !strcasecmp(params,"resources") || !strcasecmp(params,"res") || !strcasecmp(params,"usage") || !strcasecmp(params,"usages")) {
        
        /* ---->  Current resource usages of TCZ server ('@stats resource')  <---- */
        if(Level4(db[player].owner)) stats_resource(player);
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may see the current resource usages of %s server.",tcz_full_name);
        return;
     } else if(!strcasecmp(params,"connections") || !strcasecmp(params,"connection") || !strcasecmp(params,"connect") || !strcasecmp(params,"conn") || !strcasecmp(params,"con") || !strcasecmp(params,"telnet") || !strcasecmp(params,"html") || !strcasecmp(params,"ssl")) {
        
        /* ---->  Summary of current Telnet and HTML connections ('@stats connections')  <---- */
        if(Level4(db[player].owner)) stats_connections(player);
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may see a summary of current Telnet and HTML connections.");
        return;
     } else {

        /* ---->  DB (Object) statistics ('@stats')  <---- */
        int properties = 0, tproperties = 0;
        int variables  = 0, tvariables  = 0;
        int commands   = 0, tcommands   = 0;
        int players    = 0, tplayers    = 0;
        int puppets    = 0, tpuppets    = 0;
        int things     = 0, tthings     = 0;
        int alarms     = 0, talarms     = 0;
        int arrays     = 0, tarrays     = 0;
        int dbfree     = 0, tchars      = 0;
        int rooms      = 0, trooms      = 0;
        int exits      = 0, texits      = 0;
        int fuses      = 0, tfuses      = 0;
        int total      = 0, ttotal      = 0;

        unsigned char            twidth = output_terminal_width(player);
        struct   descriptor_data *p = getdsc(player);
        dbref                    owner,i;

        if(!Blank(params)) {
           if((owner = lookup_character(player,params,1)) == NOTHING) {
              output(p,player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",params);
              return;
	   }
           if(!Level4(db[player].owner) && !Experienced(db[player].owner) && !can_write_to(player,owner,0) && !(!in_command && friendflags_set(owner,player,NOTHING,FRIEND_READ))) {
              output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only view your own statistics or those of one of your puppets.");
              return;
	   }
	} else owner = player;

        /* ---->  Gather DB object statistics  <---- */
        for(i = 0; i < db_top; i++) {
            if(Typeof(i) != TYPE_FREE) {
               if(db[i].owner == owner) total++;
               ttotal++;
	    }

            switch(Typeof(i)) {
                   case TYPE_EXIT:
                        if(db[i].owner == owner) exits++;
                        texits++;
                        break;
                   case TYPE_FREE:
                        dbfree++;
                        break;
                   case TYPE_FUSE:
                        if(db[i].owner == owner) fuses++;
                        tfuses++;
                        break;
                   case TYPE_ROOM:
                        if(db[i].owner == owner) rooms++;
                        trooms++;
                        break;
                   case TYPE_ALARM:
                        if(db[i].owner == owner) alarms++;
                        talarms++;
                        break;
                   case TYPE_ARRAY:
                        if(db[i].owner == owner) arrays++;
                        tarrays++;
                        break;
                   case TYPE_THING:
                        if(db[i].owner == owner) things++;
                        tthings++;
                        break;
                   case TYPE_CHARACTER:
                        if((i != owner) && Puppet(i)) {
                           if(Controller(i) == owner) puppets++, total++;
                           tpuppets++;
                        } else {
                           if(db[i].owner == owner) players++;
                           tplayers++;
                        }
                        tchars++;		     
		        break;
                   case TYPE_COMMAND:
                        if(db[i].owner == owner) commands++;
                        tcommands++;
                        break;
                   case TYPE_PROPERTY:
                        if(db[i].owner == owner) properties++;
                        tproperties++;
                        break;
                   case TYPE_VARIABLE:
                        if(db[i].owner == owner) variables++;
                        tvariables++;
                        break;
	    }
	}

        /* ---->  Display DB object statistics  <---- */
        if(IsHtml(p)) {
           html_anti_reverse(p,1);
           output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
	}

        if(!in_command) {
           if(owner != player) output(p,player,2,1,1,"%sThe database consists of "ANSI_LWHITE"%d"ANSI_LCYAN" objects, of which %s"ANSI_LWHITE"%s"ANSI_LCYAN" owns "ANSI_LWHITE"%d "ANSI_LYELLOW"%s"ANSI_LCYAN"...%s",IsHtml(p) ? (Level4(db[player].owner) || Experienced(db[player].owner)) ? "\016<TR><TH ALIGN=CENTER COLSPAN=4 BGCOLOR="HTML_TABLE_CYAN"><FONT SIZE=4><I>\016"ANSI_LCYAN:"\016<TR><TH ALIGN=CENTER COLSPAN=3 BGCOLOR="HTML_TABLE_CYAN"><FONT SIZE=4><I>\016"ANSI_LCYAN:"\n ",ttotal,Article(owner,LOWER,DEFINITE),getcname(NOTHING,owner,0,0),total,stats_percent(total,ttotal),IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
              else output(p,player,2,1,1,"%sThe database consists of "ANSI_LWHITE"%d"ANSI_LCYAN" objects, of which you own "ANSI_LWHITE"%d "ANSI_LYELLOW"%s"ANSI_LCYAN"...%s",IsHtml(p) ? (Level4(db[player].owner) || Experienced(db[player].owner)) ? "\016<TR><TH ALIGN=CENTER COLSPAN=4 BGCOLOR="HTML_TABLE_CYAN"><FONT SIZE=4><I>\016"ANSI_LCYAN:"\016<TR><TH ALIGN=CENTER COLSPAN=3 BGCOLOR="HTML_TABLE_CYAN"><FONT SIZE=4><I>\016"ANSI_LCYAN:"\n ",ttotal,total,stats_percent(total,ttotal),IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
           if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
	}

        if(Level4(db[player].owner) || Experienced(db[player].owner)) {
           output(p,player,2,1,19,"%sDynamic arrays:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s"ANSI_LWHITE"...out of a total of %d.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW" ",IsHtml(p) ? "\016</I></TH><TD WIDTH=25%>\016":"  ",arrays,IsHtml(p) ? "\016</TD><TD WIDTH=25%>\016":"",stats_percent(arrays,tarrays),IsHtml(p) ? "\016</TD><TD ALIGN=LEFT WIDTH=25%>\016":"",tarrays,IsHtml(p) ? "\016</TD></TR>\016":"\n");
           output(p,player,2,1,19,"%sCharacters:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s"ANSI_LWHITE"...out of a total of %d.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"     ",IsHtml(p) ? "\016</I></TH><TD WIDTH=25%>\016":"  ",players,IsHtml(p) ? "\016</TD><TD WIDTH=25%>\016":"",stats_percent(players,tplayers),IsHtml(p) ? "\016</TD><TD ALIGN=LEFT WIDTH=25%>\016":"",tplayers,IsHtml(p) ? "\016</TD></TR>\016":"\n");
           output(p,player,2,1,19,"%sProperties:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s"ANSI_LWHITE"...out of a total of %d.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"     ",IsHtml(p) ? "\016</I></TH><TD WIDTH=25%>\016":"  ",properties,IsHtml(p) ? "\016</TD><TD WIDTH=25%>\016":"",stats_percent(properties,tproperties),IsHtml(p) ? "\016</TD><TD ALIGN=LEFT WIDTH=25%>\016":"",tproperties,IsHtml(p) ? "\016</TD></TR>\016":"\n");
           output(p,player,2,1,19,"%sVariables:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s"ANSI_LWHITE"...out of a total of %d.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"      ",IsHtml(p) ? "\016</I></TH><TD WIDTH=25%>\016":"  ",variables,IsHtml(p) ? "\016</TD><TD WIDTH=25%>\016":"",stats_percent(variables,tvariables),IsHtml(p) ? "\016</TD><TD ALIGN=LEFT WIDTH=25%>\016":"",tvariables,IsHtml(p) ? "\016</TD></TR>\016":"\n");
           output(p,player,2,1,19,"%sCommands:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s"ANSI_LWHITE"...out of a total of %d.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"       ",IsHtml(p) ? "\016</I></TH><TD WIDTH=25%>\016":"  ",commands,IsHtml(p) ? "\016</TD><TD WIDTH=25%>\016":"",stats_percent(commands,tcommands),IsHtml(p) ? "\016</TD><TD ALIGN=LEFT WIDTH=25%>\016":"",tcommands,IsHtml(p) ? "\016</TD></TR>\016":"\n");
           output(p,player,2,1,19,"%sPuppets:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s"ANSI_LWHITE"...out of a total of %d.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"        ",IsHtml(p) ? "\016</I></TH><TD WIDTH=25%>\016":"  ",puppets,IsHtml(p) ? "\016</TD><TD WIDTH=25%>\016":"",stats_percent(puppets,tpuppets),IsHtml(p) ? "\016</TD><TD ALIGN=LEFT WIDTH=25%>\016":"",tpuppets,IsHtml(p) ? "\016</TD></TR>\016":"\n");
           output(p,player,2,1,19,"%sAlarms:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s"ANSI_LWHITE"...out of a total of %d.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"         ",IsHtml(p) ? "\016</I></TH><TD WIDTH=25%>\016":"  ",alarms,IsHtml(p) ? "\016</TD><TD WIDTH=25%>\016":"",stats_percent(alarms,talarms),IsHtml(p) ? "\016</TD><TD ALIGN=LEFT WIDTH=25%>\016":"",talarms,IsHtml(p) ? "\016</TD></TR>\016":"\n");
           output(p,player,2,1,19,"%sThings:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s"ANSI_LWHITE"...out of a total of %d.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"         ",IsHtml(p) ? "\016</I></TH><TD WIDTH=25%>\016":"  ",things,IsHtml(p) ? "\016</TD><TD WIDTH=25%>\016":"",stats_percent(things,tthings),IsHtml(p) ? "\016</TD><TD ALIGN=LEFT WIDTH=25%>\016":"",tthings,IsHtml(p) ? "\016</TD></TR>\016":"\n");
           output(p,player,2,1,19,"%sExits:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s"ANSI_LWHITE"...out of a total of %d.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"          ",IsHtml(p) ? "\016</I></TH><TD WIDTH=25%>\016":"  ",exits,IsHtml(p) ? "\016</TD><TD WIDTH=25%>\016":"",stats_percent(exits,texits),IsHtml(p) ? "\016</TD><TD ALIGN=LEFT WIDTH=25%>\016":"",texits,IsHtml(p) ? "\016</TD></TR>\016":"\n");
           output(p,player,2,1,19,"%sFuses:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s"ANSI_LWHITE"...out of a total of %d.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"          ",IsHtml(p) ? "\016</I></TH><TD WIDTH=25%>\016":"  ",fuses,IsHtml(p) ? "\016</TD><TD WIDTH=25%>\016":"",stats_percent(fuses,tfuses),IsHtml(p) ? "\016</TD><TD ALIGN=LEFT WIDTH=25%>\016":"",tfuses,IsHtml(p) ? "\016</TD></TR>\016":"\n");
           output(p,player,2,1,19,"%sRooms:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s"ANSI_LWHITE"...out of a total of %d.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"          ",IsHtml(p) ? "\016</I></TH><TD WIDTH=25%>\016":"  ",rooms,IsHtml(p) ? "\016</TD><TD WIDTH=25%>\016":"",stats_percent(rooms,trooms),IsHtml(p) ? "\016</TD><TD ALIGN=LEFT WIDTH=25%>\016":"",trooms,IsHtml(p) ? "\016</TD></TR>\016":"\n");
	} else {
           output(p,player,2,1,19,"%sDynamic arrays:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW" ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",arrays,IsHtml(p) ? "\016</TD><TD WIDTH=33%>\016":"",stats_percent(arrays,tarrays),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           output(p,player,2,1,19,"%sCharacters:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"     ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",players,IsHtml(p) ? "\016</TD><TD WIDTH=33%>\016":"",stats_percent(players,tplayers),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           output(p,player,2,1,19,"%sProperties:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"     ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",properties,IsHtml(p) ? "\016</TD><TD WIDTH=33%>\016":"",stats_percent(properties,tproperties),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           output(p,player,2,1,19,"%sVariables:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"      ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",variables,IsHtml(p) ? "\016</TD><TD WIDTH=33%>\016":"",stats_percent(variables,tvariables),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           output(p,player,2,1,19,"%sCommands:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"       ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",commands,IsHtml(p) ? "\016</TD><TD WIDTH=33%>\016":"",stats_percent(commands,tcommands),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           output(p,player,2,1,19,"%sPuppets:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"        ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",puppets,IsHtml(p) ? "\016</TD><TD WIDTH=33%>\016":"",stats_percent(puppets,tpuppets),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           output(p,player,2,1,19,"%sAlarms:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"         ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",alarms,IsHtml(p) ? "\016</TD><TD WIDTH=33%>\016":"",stats_percent(alarms,talarms),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           output(p,player,2,1,19,"%sThings:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"         ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",things,IsHtml(p) ? "\016</TD><TD WIDTH=33%>\016":"",stats_percent(things,tthings),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           output(p,player,2,1,19,"%sExits:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"          ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",exits,IsHtml(p) ? "\016</TD><TD WIDTH=33%>\016":"",stats_percent(exits,texits),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           output(p,player,2,1,19,"%sFuses:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"          ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",fuses,IsHtml(p) ? "\016</TD><TD WIDTH=33%>\016":"",stats_percent(fuses,tfuses),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           output(p,player,2,1,19,"%sRooms:%s"ANSI_LWHITE"%-10d%s"ANSI_DWHITE"%-11s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"          ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",rooms,IsHtml(p) ? "\016</TD><TD WIDTH=33%>\016":"",stats_percent(rooms,trooms),IsHtml(p) ? "\016</TD></TR>\016":"\n");
	}

        if(!in_command) {
           if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,(Level4(db[player].owner) || Experienced(db[player].owner)) ? 0:1,'-','='));
           if(Level4(db[player].owner) || Experienced(db[player].owner))
              output(p,player,2,1,1,"%sCurrent size of database: \016&nbsp;\016 "ANSI_DWHITE"%d (%d unused object%s.)%s",IsHtml(p) ? "\016<TR><TD ALIGN=CENTER COLSPAN=4 BGCOLOR="HTML_TABLE_GREY">"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",db_top,dbfree,Plural(dbfree),IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
	}

        if(IsHtml(p)) {
           output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
           html_anti_reverse(p,0);
	}
        setreturn(OK,COMMAND_SUCC);
     }
}
/* ---->  Display Building Quota summary  <---- */
void stats_quota(CONTEXT)
{
     int properties = 0, qproperties = 0;
     int variables  = 0, qvariables  = 0;
     int elements   = 0, qelements   = 0;
     int commands   = 0, qcommands   = 0;
     int things     = 0, qthings     = 0;
     int alarms     = 0, qalarms     = 0;
     int arrays     = 0, qarrays     = 0;
     int rooms      = 0, qrooms      = 0;
     int exits      = 0, qexits      = 0;
     int fuses      = 0, qfuses      = 0;
     int temp;

     unsigned char            twidth = output_terminal_width(player);
     struct   descriptor_data *p = getdsc(player);
     dbref                    subject,i;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(params)) {
        if(string_prefix(params,"initialise") || string_prefix(params,"reset")) {
           if(!Level4(db[player].owner)) {
              output(p,player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may re-initialise the Building Quotas of all characters in the database.");
              return;
	   }
           for(i = 0; i < db_top; i++)
               if(Typeof(i) == TYPE_CHARACTER)
                  db[i].data->player.quota = 0;
           initialise_quotas(0);
           output(p,player,0,1,0,ANSI_LGREEN"Building Quotas of all characters in the database re-initialised.");
           return;
	} else {
           if((subject = lookup_character(player,params,1)) == NOTHING) {
              output(p,player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",params);
              return;
	   }
           if(!Level4(Owner(player)) && !Experienced(Owner(player)) && !can_write_to(player,subject,0) && !(!in_command && friendflags_set(subject,player,NOTHING,FRIEND_READ))) {
              output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only view your own Building Quota summary or that of one of your puppets.");
              return;
	   }
	}
     } else subject = player;

     for(i = 0; i < db_top; i++)
         if(db[i].owner == subject)
            switch(Typeof(i)) {
                   case TYPE_EXIT:
                        qexits += EXIT_QUOTA;
                        exits++;
                        break;
                   case TYPE_FUSE:
                        qfuses += FUSE_QUOTA;
                        fuses++;
                        break;
                   case TYPE_ROOM:
                        qrooms += ROOM_QUOTA;
                        rooms++;
                        break;
                   case TYPE_ALARM:
                        qalarms += ALARM_QUOTA;
                        alarms++;
                        break;
                   case TYPE_ARRAY:
                        temp       = array_element_count(db[i].data->array.start);
                        qelements += (ELEMENT_QUOTA * temp);
                        elements  += temp;
                        qarrays   += ARRAY_QUOTA;
                        arrays++;
                        break;
                   case TYPE_THING:
                        qthings += THING_QUOTA;
                        things++;
                        break;
                   case TYPE_COMMAND:
                        qcommands += COMMAND_QUOTA;
		        commands++;
                        break;
                   case TYPE_PROPERTY:
                        qproperties += PROPERTY_QUOTA;
                        properties++;
                        break;
                   case TYPE_VARIABLE:
                        qvariables += VARIABLE_QUOTA;
                        variables++;
                        break;
	    }

     if(IsHtml(p)) {
        html_anti_reverse(p,1);
        output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
     }

     if(!in_command) {
        if(subject == player) {
           if(!Level4(subject) && (db[subject].data->player.quota > db[subject].data->player.quotalimit))
              sprintf(scratch_return_string," (You've exceeded your quota limit of "ANSI_LWHITE"%d"ANSI_LCYAN")",db[subject].data->player.quota);
                 else *scratch_return_string = '\0';
           output(p,player,2,1,1,"%sBuilding Quota summary for yourself%s...%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER COLSPAN=4><FONT SIZE=4><I>\016"ANSI_LCYAN:"\n ",scratch_return_string,IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
	} else output(p,player,2,1,1,"%sBuilding Quota summary for %s"ANSI_LWHITE"%s"ANSI_LCYAN"...%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER COLSPAN=4><FONT SIZE=4><I>\016"ANSI_LCYAN:"\n ",Article(subject,LOWER,DEFINITE),getcname(NOTHING,subject,0,0),IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
	if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
        output(p,player,2,1,1,"%sOwned:%sQuota per object:%sTotal quota used:%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_YELLOW"><TD>&nbsp;</TD><TH><FONT COLOR="HTML_LYELLOW" SIZE=4><I>\016":ANSI_LYELLOW"                  ",IsHtml(p) ? "\016</I></FONT></TH><TH><FONT COLOR="HTML_LYELLOW" SIZE=4><I>\016":"             ",IsHtml(p) ? "\016</I></FONT></TH><TH><FONT COLOR="HTML_LYELLOW" SIZE=4><I>\016":"   ",IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
	if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','-'));
     }

     output(p,player,2,1,0,"%sDynamic arrays:%s"ANSI_LWHITE"%-19d%s%-20d%s%d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW" ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",arrays,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",ARRAY_QUOTA,IsHtml(p) ? "\016</TD><TH BGCOLOR="HTML_TABLE_DGREY">"ANSI_LWHITE"<I>\016":"",qarrays,IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sArray elements:%s"ANSI_LWHITE"%-19d%s%-20d%s%d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW" ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",elements,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",ELEMENT_QUOTA,IsHtml(p) ? "\016</TD><TH BGCOLOR="HTML_TABLE_DGREY">"ANSI_LWHITE"<I>\016":"",qelements,IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sProperties:%s"ANSI_LWHITE"%-19d%s%-20d%s%d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"     ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",properties,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",PROPERTY_QUOTA,IsHtml(p) ? "\016</TD><TH BGCOLOR="HTML_TABLE_DGREY">"ANSI_LWHITE"<I>\016":"",qproperties,IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sVariables:%s"ANSI_LWHITE"%-19d%s%-20d%s%d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"      ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",variables,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",VARIABLE_QUOTA,IsHtml(p) ? "\016</TD><TH BGCOLOR="HTML_TABLE_DGREY">"ANSI_LWHITE"<I>\016":"",qvariables,IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sCommands:%s"ANSI_LWHITE"%-19d%s%-20d%s%d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"       ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",commands,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",COMMAND_QUOTA,IsHtml(p) ? "\016</TD><TH BGCOLOR="HTML_TABLE_DGREY">"ANSI_LWHITE"<I>\016":"",qcommands,IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sAlarms:%s"ANSI_LWHITE"%-19d%s%-20d%s%d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"         ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",alarms,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",ALARM_QUOTA,IsHtml(p) ? "\016</TD><TH BGCOLOR="HTML_TABLE_DGREY">"ANSI_LWHITE"<I>\016":"",qalarms,IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sThings:%s"ANSI_LWHITE"%-19d%s%-20d%s%d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"         ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",things,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",THING_QUOTA,IsHtml(p) ? "\016</TD><TH BGCOLOR="HTML_TABLE_DGREY">"ANSI_LWHITE"<I>\016":"",qthings,IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sExits:%s"ANSI_LWHITE"%-19d%s%-20d%s%d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"          ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",exits,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",EXIT_QUOTA,IsHtml(p) ? "\016</TD><TH BGCOLOR="HTML_TABLE_DGREY">"ANSI_LWHITE"<I>\016":"",qexits,IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sFuses:%s"ANSI_LWHITE"%-19d%s%-20d%s%d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"          ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",fuses,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",FUSE_QUOTA,IsHtml(p) ? "\016</TD><TH BGCOLOR="HTML_TABLE_DGREY">"ANSI_LWHITE"<I>\016":"",qfuses,IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sRooms:%s"ANSI_LWHITE"%-19d%s%-20d%s%d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"          ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",rooms,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",ROOM_QUOTA,IsHtml(p) ? "\016</TD><TH BGCOLOR="HTML_TABLE_DGREY">"ANSI_LWHITE"<I>\016":"",qrooms,IsHtml(p) ? "\016</I></TH></TR>\016":"\n");

     if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','-'));
     temp = qarrays + qelements + qproperties + qvariables + qcommands + qalarms + qthings + qexits + qfuses + qrooms;
     if(!Level4(subject)) sprintf(scratch_return_string,"%s%d/%ld",(temp > db[subject].data->player.quotalimit) ? ANSI_LRED:ANSI_LWHITE,temp,db[subject].data->player.quotalimit);
        else sprintf(scratch_return_string,ANSI_LWHITE"%d/UNLIMITED",temp);
     temp = (arrays + properties + variables + commands + alarms + things + exits + fuses + rooms);
     output(p,player,2,1,0,"%sTotal objects:%s"ANSI_LWHITE"%-19d%sN/A%s%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><FONT SIZE=4><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"  ",IsHtml(p) ? "\016</I></FONT></TH><TH><FONT SIZE=4>\016":"  ",temp,IsHtml(p) ? "\016</FONT></TH><TH><FONT SIZE=4>\016"ANSI_DCYAN:ANSI_DCYAN,IsHtml(p) ? "\016</FONT></TH><TH><FONT SIZE=4>"ANSI_LWHITE"<I>\016":"                 "ANSI_LWHITE,scratch_return_string,IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");

     if(IsHtml(p)) {
        output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
        html_anti_reverse(p,0);
     } else if(!in_command) output(p,player,0,1,0,separator(twidth,1,'-','='));
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Display memory usage summary  <---- */
void stats_size(CONTEXT)
{
     unsigned long properties = 0, sproperties = 0, cproperties = 0;
     unsigned long characters = 0, scharacters = 0, ccharacters = 0;
     unsigned long variables  = 0, svariables  = 0, cvariables  = 0;
     unsigned long commands   = 0, scommands   = 0, ccommands   = 0;
     unsigned long puppets    = 0, spuppets    = 0, cpuppets    = 0;
     unsigned long things     = 0, sthings     = 0, cthings     = 0;
     unsigned long alarms     = 0, salarms     = 0, calarms     = 0;
     unsigned long arrays     = 0, sarrays     = 0, carrays     = 0;
     unsigned long rooms      = 0, srooms      = 0, crooms      = 0;
     unsigned long exits      = 0, sexits      = 0, cexits      = 0;
     unsigned long fuses      = 0, sfuses      = 0, cfuses      = 0;
     unsigned long total      = 0, stotal      = 0, ctotal      = 0;

     unsigned char            twidth = output_terminal_width(player);
     struct   descriptor_data *p = getdsc(player);
     dbref                    subject = NOTHING,i;
     unsigned char            all = 0;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(arg1)) {
        if(strcasecmp("all",arg1)) {
           if((subject = lookup_character(player,arg1,1)) == NOTHING) {
              output(p,player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
              return;
	   } else if(!Level4(db[player].owner) && !can_write_to(player,subject,0) && !(!in_command && friendflags_set(subject,player,NOTHING,FRIEND_READ))) {
              output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only view your own database memory usage summary or that of one of your puppets.");
              return;
	   }
	} else if(!Level2(db[player].owner)) {
           output(p,player,0,1,0,ANSI_LGREEN"Sorry, only Elder Wizards/Druids and above may view the database memory usage summary for the entire database.");
           return;
	} else if(Blank(arg2) || strcasecmp("yes",arg2)) {
           output(p,player,0,1,10,ANSI_LRED"WARNING:  "ANSI_LWHITE"Producing a database memory usage summary for the entire database may cause excessive lag (As each object needs to be decompressed.)  To produce this summary, please type '"ANSI_LYELLOW"@size all = yes"ANSI_LWHITE"'.");
           return;
	} else all = 1;
     } else subject = player;

     for(i = 0; i < db_top; i++)
         if((Typeof(i) != TYPE_FREE) && (all || ((db[i].owner == subject) || ((Typeof(i) == TYPE_CHARACTER) && (i != subject) && (Controller(i) == subject)))))
            switch(Typeof(i)) {
                   case TYPE_EXIT:
                        sexits += getsize(i,1);
                        cexits += getsize(i,0);
                        exits++;
                        break;
                   case TYPE_FUSE:
                        sfuses += getsize(i,1);
                        cfuses += getsize(i,0);
                        fuses++;
                        break;
                   case TYPE_ROOM:
                        srooms += getsize(i,1);
                        crooms += getsize(i,0);
                        rooms++;
                        break;
                   case TYPE_ALARM:
                        salarms += getsize(i,1);
                        calarms += getsize(i,0);
                        alarms++;
                        break;
                   case TYPE_ARRAY:
                        sarrays += getsize(i,1);
                        carrays += getsize(i,0);
                        arrays++;
                        break;
                   case TYPE_THING:
                        sthings += getsize(i,1);
                        cthings += getsize(i,0);
                        things++;
                        break;
                   case TYPE_COMMAND:
                        scommands += getsize(i,1);
                        ccommands += getsize(i,0);
		        commands++;
                        break;
                   case TYPE_PROPERTY:
                        sproperties += getsize(i,1);
                        cproperties += getsize(i,0);
                        properties++;
                        break;
                   case TYPE_VARIABLE:
                        svariables += getsize(i,1);
                        cvariables += getsize(i,0);
                        variables++;
                        break;
                   case TYPE_CHARACTER: 
                        if(Puppet(i)) {
                           spuppets += getsize(i,1);
                           cpuppets += getsize(i,0);
                           puppets++;
			} else {
                           scharacters += getsize(i,1);
                           ccharacters += getsize(i,0);
                           characters++;
			}
                        break;
	    }

     total  = fuses  + exits  + rooms  + arrays  + alarms  + things  + puppets  + commands  + variables  + characters  + properties;
     stotal = sfuses + sexits + srooms + sarrays + salarms + sthings + spuppets + scommands + svariables + scharacters + sproperties;
     ctotal = cfuses + cexits + crooms + carrays + calarms + cthings + cpuppets + ccommands + cvariables + ccharacters + cproperties;

     if(IsHtml(p)) {
        html_anti_reverse(p,1);
        output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
     }

     if(!in_command) {
        if(!all) {
           if(subject != player) output(p,player,2,1,1,"%sDatabase memory usage summary for %s"ANSI_LWHITE"%s"ANSI_LCYAN"...%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER COLSPAN=5><FONT SIZE=4><I>\016"ANSI_LCYAN:"\n ",Article(subject,LOWER,DEFINITE),getcname(NOTHING,subject,0,0),IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
	      else output(p,player,2,1,1,"%sDatabase memory usage summary for yourself...%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER COLSPAN=5><FONT SIZE=4><I>\016"ANSI_LCYAN:"\n ",IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
	} else output(p,player,2,1,1,"%sDatabase memory usage summary for the entire database...%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER COLSPAN=5><FONT SIZE=4><I>\016"ANSI_LCYAN:"\n ",IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
        if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
        output(p,player,2,1,1,"%sOwned:%sSize:%sCompressed size:%sRatio:%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_YELLOW"><TD>&nbsp;</TD><TH><FONT COLOR="HTML_LYELLOW" SIZE=4><I>\016":ANSI_LYELLOW"                  ",IsHtml(p) ? "\016</I></FONT></TH><TH><FONT COLOR="HTML_LYELLOW" SIZE=4><I>\016":"      ",IsHtml(p) ? "\016</I></FONT></TH><TH><FONT COLOR="HTML_LYELLOW" SIZE=4><I>\016":"            ",IsHtml(p) ? "\016</I></FONT></TH><TH><FONT COLOR="HTML_LYELLOW" SIZE=4><I>\016":"  ",IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
        if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','-'));
     }

     sprintf(scratch_return_string,"%.2fKb",(double) sarrays / KB);
     sprintf(scratch_return_string + 100,"%.2fKb",(double) carrays / KB);
     output(p,player,2,1,0,"%sDynamic arrays:%s"ANSI_LWHITE"%-12d%s%-17s%s%-18s%s%.1f%%%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW" ",IsHtml(p) ? "\016</I></TH><TD>\016"ANSI_LWHITE:"  ",arrays,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",scratch_return_string,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",scratch_return_string + 100,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",(sarrays < 1) ? 0:100 - (((double) carrays / sarrays) * 100),IsHtml(p) ? "\016</TD></TR>\016":"\n");

     sprintf(scratch_return_string,"%.2fKb",(double) scharacters / KB);
     sprintf(scratch_return_string + 100,"%.2fKb",(double) ccharacters / KB);
     output(p,player,2,1,0,"%sCharacters:%s"ANSI_LWHITE"%-12d%s%-17s%s%-18s%s%.1f%%%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"     ",IsHtml(p) ? "\016</I></TH><TD>\016"ANSI_LWHITE:"  ",characters,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",scratch_return_string,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",scratch_return_string + 100,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",(scharacters < 1) ? 0:100 - (((double) ccharacters / scharacters) * 100),IsHtml(p) ? "\016</TD></TR>\016":"\n");

     sprintf(scratch_return_string,"%.2fKb",(double) sproperties / KB);
     sprintf(scratch_return_string + 100,"%.2fKb",(double) cproperties / KB);
     output(p,player,2,1,0,"%sProperties:%s"ANSI_LWHITE"%-12d%s%-17s%s%-18s%s%.1f%%%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"     ",IsHtml(p) ? "\016</I></TH><TD>\016"ANSI_LWHITE:"  ",properties,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",scratch_return_string,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",scratch_return_string + 100,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",(sproperties < 1) ? 0:100 - (((double) cproperties / sproperties) * 100),IsHtml(p) ? "\016</TD></TR>\016":"\n");

     sprintf(scratch_return_string,"%.2fKb",(double) svariables / KB);
     sprintf(scratch_return_string + 100,"%.2fKb",(double) cvariables / KB);
     output(p,player,2,1,0,"%sVariables:%s"ANSI_LWHITE"%-12d%s%-17s%s%-18s%s%.1f%%%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"      ",IsHtml(p) ? "\016</I></TH><TD>\016"ANSI_LWHITE:"  ",variables,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",scratch_return_string,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",scratch_return_string + 100,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",(svariables < 1) ? 0:100 - (((double) cvariables / svariables) * 100),IsHtml(p) ? "\016</TD></TR>\016":"\n");

     sprintf(scratch_return_string,"%.2fKb",(double) scommands / KB);
     sprintf(scratch_return_string + 100,"%.2fKb",(double) ccommands / KB);
     output(p,player,2,1,0,"%sCommands:%s"ANSI_LWHITE"%-12d%s%-17s%s%-18s%s%.1f%%%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"       ",IsHtml(p) ? "\016</I></TH><TD>\016"ANSI_LWHITE:"  ",commands,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",scratch_return_string,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",scratch_return_string + 100,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",(scommands < 1) ? 0:100 - (((double) ccommands / scommands) * 100),IsHtml(p) ? "\016</TD></TR>\016":"\n");

     sprintf(scratch_return_string,"%.2fKb",(double) spuppets / KB);
     sprintf(scratch_return_string + 100,"%.2fKb",(double) cpuppets / KB);
     output(p,player,2,1,0,"%sPuppets:%s"ANSI_LWHITE"%-12d%s%-17s%s%-18s%s%.1f%%%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"        ",IsHtml(p) ? "\016</I></TH><TD>\016"ANSI_LWHITE:"  ",puppets,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",scratch_return_string,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",scratch_return_string + 100,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",(spuppets < 1) ? 0:100 - (((double) cpuppets / spuppets) * 100),IsHtml(p) ? "\016</TD></TR>\016":"\n");

     sprintf(scratch_return_string,"%.2fKb",(double) salarms / KB);
     sprintf(scratch_return_string + 100,"%.2fKb",(double) calarms / KB);
     output(p,player,2,1,0,"%sAlarms:%s"ANSI_LWHITE"%-12d%s%-17s%s%-18s%s%.1f%%%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"         ",IsHtml(p) ? "\016</I></TH><TD>\016"ANSI_LWHITE:"  ",alarms,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",scratch_return_string,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",scratch_return_string + 100,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",(salarms < 1) ? 0:100 - (((double) calarms / salarms) * 100),IsHtml(p) ? "\016</TD></TR>\016":"\n");

     sprintf(scratch_return_string,"%.2fKb",(double) sthings / KB);
     sprintf(scratch_return_string + 100,"%.2fKb",(double) cthings / KB);
     output(p,player,2,1,0,"%sThings:%s"ANSI_LWHITE"%-12d%s%-17s%s%-18s%s%.1f%%%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"         ",IsHtml(p) ? "\016</I></TH><TD>\016"ANSI_LWHITE:"  ",things,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",scratch_return_string,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",scratch_return_string + 100,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",(sthings < 1) ? 0:100 - (((double) cthings / sthings) * 100),IsHtml(p) ? "\016</TD></TR>\016":"\n");

     sprintf(scratch_return_string,"%.2fKb",(double) sexits / KB);
     sprintf(scratch_return_string + 100,"%.2fKb",(double) cexits / KB);
     output(p,player,2,1,0,"%sExits:%s"ANSI_LWHITE"%-12d%s%-17s%s%-18s%s%.1f%%%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"          ",IsHtml(p) ? "\016</I></TH><TD>\016"ANSI_LWHITE:"  ",exits,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",scratch_return_string,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",scratch_return_string + 100,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",(sexits < 1) ? 0:100 - (((double) cexits / sexits) * 100),IsHtml(p) ? "\016</TD></TR>\016":"\n");

     sprintf(scratch_return_string,"%.2fKb",(double) sfuses / KB);
     sprintf(scratch_return_string + 100,"%.2fKb",(double) cfuses / KB);
     output(p,player,2,1,0,"%sFuses:%s"ANSI_LWHITE"%-12d%s%-17s%s%-18s%s%.1f%%%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"          ",IsHtml(p) ? "\016</I></TH><TD>\016"ANSI_LWHITE:"  ",fuses,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",scratch_return_string,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",scratch_return_string + 100,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",(sfuses < 1) ? 0:100 - (((double) cfuses / sfuses) * 100),IsHtml(p) ? "\016</TD></TR>\016":"\n");

     sprintf(scratch_return_string,"%.2fKb",(double) srooms / KB);
     sprintf(scratch_return_string + 100,"%.2fKb",(double) crooms / KB);
     output(p,player,2,1,0,"%sRooms:%s"ANSI_LWHITE"%-12d%s%-17s%s%-18s%s%.1f%%%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"          ",IsHtml(p) ? "\016</I></TH><TD>\016"ANSI_LWHITE:"  ",rooms,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",scratch_return_string,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",scratch_return_string + 100,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"",(srooms < 1) ? 0:100 - (((double) crooms / srooms) * 100),IsHtml(p) ? "\016</TD></TR>\016":"\n");

     if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','-'));
     sprintf(scratch_return_string,"%.2fKb",(double) stotal / KB);
     sprintf(scratch_return_string + 100,"%.2fKb",(double) ctotal / KB);
     output(p,player,2,1,0,"%sTotal:%s"ANSI_LWHITE"%-12d%s%-17s%s%-18s%s%.1f%%%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_DGREY"><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><FONT SIZE=4><I><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"          ",IsHtml(p) ? "\016</I></FONT></TH><TH><FONT SIZE=4>\016":"  ",total,IsHtml(p) ? "\016</FONT></TH><TH><FONT SIZE=4>\016"ANSI_LWHITE:"",scratch_return_string,IsHtml(p) ? "\016</FONT></TH><TH><FONT SIZE=4>\016"ANSI_LWHITE:"",scratch_return_string + 100,IsHtml(p) ? "\016</FONT></TH><TH><FONT SIZE=4>\016"ANSI_LWHITE:"",(stotal < 1) ? 0:100 - (((double) ctotal / stotal) * 100),IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");

     if(IsHtml(p)) {
        output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
        html_anti_reverse(p,0);
     } else if(!in_command) output(p,player,0,1,0,separator(twidth,1,'-','='));
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Display contents of an object  <---- */
void stats_contents(CONTEXT)
{
     int inactchars = 0, oinactchars = 0;
     int properties = 0, oproperties = 0;
     int connchars  = 0, oconnchars  = 0;
     int variables  = 0, ovariables  = 0;
     int commands   = 0, ocommands   = 0;
     int things     = 0, othings     = 0;
     int alarms     = 0, oalarms     = 0;
     int arrays     = 0, oarrays     = 0;
     int rooms      = 0, orooms      = 0;
     int exits      = 0, oexits      = 0;
     int fuses      = 0, ofuses      = 0;
     int total      = 0, ototal      = 0;

     int    twidth = output_terminal_width(player);
     struct descriptor_data *p = getdsc(player);
     dbref  object,owner,i;

     setreturn(ERROR,COMMAND_FAIL);
     object = match_preferred(player,player,(Blank(arg1)) ? "here":arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(object)) return;

     if(!Level4(db[player].owner) && !Experienced(db[player].owner) && !can_write_to(db[player].owner,object,0) && !(!in_command && friendflags_set(db[object].owner,player,object,FRIEND_READ))) {
        output(p,player,0,1,0,ANSI_LGREEN"Sorry, you may only use '"ANSI_LWHITE"@contents"ANSI_LGREEN"' on objects that belong to you.");
        return;
     }

     /* ---->  Optional character name (Experienced Builders and above only)  -  Works as if you were that character doing '@contents <OBJECT>'  <---- */
     if(!Blank(arg2)) {
        if((owner = lookup_character(player,arg2,1)) == NOTHING) {
           output(p,player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg2);
           return;
	}

        if(!Level4(db[player].owner) && !Experienced(db[player].owner) && !can_write_to(player,owner,0) && !(!in_command && friendflags_set(owner,player,NOTHING,FRIEND_READ))) {
           output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only do that if the character is yourself or one of your puppets.");
           return;
	}
     } else owner = player;

     /* ---->  Search for objects within given object  <---- */
     for(i = 0; i < db_top; i++)
         if(contains(i,object) && (i != object)) 
            switch(Typeof(i)) {
                   case TYPE_EXIT:
                        if(db[i].owner == owner) exits++;
			   else oexits++;
                        break;
                   case TYPE_FUSE:
                        if(db[i].owner == owner) fuses++;
                           else ofuses++;
                        break;
                   case TYPE_ROOM:
                        if(db[i].owner == owner) rooms++;
                           else orooms++;
                        break;
                   case TYPE_ALARM:
                        if(db[i].owner == owner) alarms++;
                           else oalarms++;
                        break;
                   case TYPE_ARRAY:
                        if(db[i].owner == owner) arrays++;
                           else oarrays++;
                        break;
                   case TYPE_THING:
                        if(db[i].owner == owner) things++;
                           else othings++;
                        break;
                   case TYPE_CHARACTER:
                        if((Typeof(object) == TYPE_CHARACTER) && (i = object)) break;
                        if(Connected(i)) {
                           if(db[i].owner == owner) connchars++;
                              else oconnchars++;
		        } else if(db[i].owner == owner) inactchars++;
                           else oinactchars++;
                        break;
                   case TYPE_COMMAND:
                        if(db[i].owner == owner) commands++;
                           else ocommands++;
                        break;
                   case TYPE_PROPERTY:
                        if(db[i].owner == owner) properties++;
                           else oproperties++;
                        break;
                   case TYPE_VARIABLE:
                        if(db[i].owner == owner) variables++;
                           else ovariables++;
                        break;
	    }

     total  = connchars  + inactchars  + commands  + variables  + things  + alarms  + rooms  + exits  + properties  + fuses  + arrays;
     ototal = oconnchars + oinactchars + ocommands + ovariables + othings + oalarms + orooms + oexits + oproperties + ofuses + oarrays;

     /* ---->  List contents totals of given object  <---- */     
     if(IsHtml(p)) {
        html_anti_reverse(p,1);
        output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
     }

     if(!in_command) {
        sprintf(scratch_buffer,"%s%s"ANSI_LWHITE"%s"ANSI_LCYAN" ",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER COLSPAN=4><FONT SIZE=4><I>\016":"\n ",Article(object,UPPER,DEFINITE),getcname(player,object,1,0));
        if((db[object].owner != player) && (db[object].owner != object))
           sprintf(scratch_buffer + strlen(scratch_buffer),"owned by %s"ANSI_LWHITE"%s"ANSI_LCYAN" ",Article(db[object].owner,LOWER,INDEFINITE),getcname(player,db[object].owner,0,0));
        sprintf(scratch_buffer + strlen(scratch_buffer),"contains "ANSI_LWHITE"%d"ANSI_LCYAN" object%s, of which ",total + ototal,Plural(total + ototal));
        if(owner != player) sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LCYAN"%s"ANSI_LWHITE"%s"ANSI_LCYAN" owns ",Article(owner,LOWER,DEFINITE),getcname(NOTHING,owner,0,0));
           else strcat(scratch_buffer,"you own ");
        output(p,player,2,1,1,"%s"ANSI_LWHITE"%d"ANSI_LCYAN"...%s",scratch_buffer,total,IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
        if(!IsHtml(p)) output(p,player,0,1,0,(char *) separator(twidth,0,'-','='));

        if(owner != player) {
           int count,length;
           char *ptr;

           for(ptr = scratch_return_string, count = 0, length = (22 - strlen(getname(owner))); count < length; *ptr++ = ' ', count++);
	   *ptr = '\0', sprintf(scratch_buffer,"%sOwned by "ANSI_LWHITE"%s"ANSI_LYELLOW":%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_YELLOW"><TD>&nbsp;</TD><TH><FONT COLOR="HTML_LYELLOW" SIZE=4><I>\016":ANSI_LYELLOW"                      ",getname(owner),IsHtml(p) ? "\016</I></FONT></TH><TH><FONT COLOR="HTML_LYELLOW" SIZE=4><I>\016":scratch_return_string);
	} else sprintf(scratch_buffer,"%sOwned by you:%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_YELLOW"><TD>&nbsp;</TD><TH><FONT COLOR="HTML_LYELLOW" SIZE=4><I>\016":ANSI_LYELLOW"                               ",IsHtml(p) ? "\016</I></FONT></TH><TH><FONT COLOR="HTML_LYELLOW" SIZE=4><I>\016":"          ");
        output(p,player,2,1,0,"%s%sOthers:%sTotal:%s",scratch_buffer,IsHtml(p) ? "":ANSI_LYELLOW,IsHtml(p) ? "\016</I></FONT></TH><TH><FONT COLOR="HTML_LYELLOW" SIZE=4><I>\016":"      ",IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
        if(!IsHtml(p)) output(p,player,0,1,0,(char *) separator(twidth,0,'-','-'));
     }

     output(p,player,2,1,0,"%sConnected characters:%s"ANSI_LWHITE"%-21d%s%-11d%s%-11d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LGREEN:ANSI_LGREEN"        ",IsHtml(p) ? "\016</I></TH><TD>\016"ANSI_LWHITE:"  ",connchars,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"  ",oconnchars,IsHtml(p) ? "\016</TD><TH BGCOLOR="HTML_TABLE_DGREY">"ANSI_LWHITE"<I>\016":"  ",connchars + oconnchars,IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sInactive characters:%s"ANSI_LWHITE"%-21d%s%-11d%s%-11d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_RED"><I>\016"ANSI_LRED:ANSI_LRED"         ",IsHtml(p) ? "\016</I></TH><TD>\016"ANSI_LWHITE:"  ",inactchars,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"  ",oinactchars,IsHtml(p) ? "\016</TD><TH BGCOLOR="HTML_TABLE_DGREY">"ANSI_LWHITE"<I>\016":"  ",inactchars + oinactchars,IsHtml(p) ? "\016</I></TH></TR>\016":"\n");

     if(!IsHtml(p)) output(p,player,0,1,0,(char *) separator(twidth,0,'-','-'));
     output(p,player,2,1,0,"%sCompound commands:%s"ANSI_LWHITE"%-21d%s%-11d%s%-11d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"           ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",commands,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"  ",ocommands,IsHtml(p) ? "\016</TD><TH BGCOLOR="HTML_TABLE_DGREY">"ANSI_LWHITE"<I>\016":"  ",commands + ocommands,IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sDynamic arrays:%s"ANSI_LWHITE"%-21d%s%-11d%s%-11d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"              ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",arrays,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"  ",oarrays,IsHtml(p) ? "\016</TD><TH BGCOLOR="HTML_TABLE_DGREY">"ANSI_LWHITE"<I>\016":"  ",arrays + oarrays,IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sProperties:%s"ANSI_LWHITE"%-21d%s%-11d%s%-11d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"                  ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",properties,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"  ",oproperties,IsHtml(p) ? "\016</TD><TH BGCOLOR="HTML_TABLE_DGREY">"ANSI_LWHITE"<I>\016":"  ",properties + oproperties,IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sVariables:%s"ANSI_LWHITE"%-21d%s%-11d%s%-11d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"                   ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",variables,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"  ",ovariables,IsHtml(p) ? "\016</TD><TH BGCOLOR="HTML_TABLE_DGREY">"ANSI_LWHITE"<I>\016":"  ",variables + ovariables,IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sAlarms:%s"ANSI_LWHITE"%-21d%s%-11d%s%-11d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"                      ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",alarms,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"  ",oalarms,IsHtml(p) ? "\016</TD><TH BGCOLOR="HTML_TABLE_DGREY">"ANSI_LWHITE"<I>\016":"  ",alarms + oalarms,IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sThings:%s"ANSI_LWHITE"%-21d%s%-11d%s%-11d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"                      ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",things,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"  ",othings,IsHtml(p) ? "\016</TD><TH BGCOLOR="HTML_TABLE_DGREY">"ANSI_LWHITE"<I>\016":"  ",things + othings,IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sExits:%s"ANSI_LWHITE"%-21d%s%-11d%s%-11d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"                       ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",exits,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"  ",oexits,IsHtml(p) ? "\016</TD><TH BGCOLOR="HTML_TABLE_DGREY">"ANSI_LWHITE"<I>\016":"  ",exits + oexits,IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sFuses:%s"ANSI_LWHITE"%-21d%s%-11d%s%-11d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"                       ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",fuses,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"  ",ofuses,IsHtml(p) ? "\016</TD><TH BGCOLOR="HTML_TABLE_DGREY">"ANSI_LWHITE"<I>\016":"  ",fuses + ofuses,IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
     output(p,player,2,1,0,"%sRooms:%s"ANSI_LWHITE"%-21d%s%-11d%s%-11d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"                       ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",rooms,IsHtml(p) ? "\016</TD><TD>\016"ANSI_LWHITE:"  ",orooms,IsHtml(p) ? "\016</TD><TH BGCOLOR="HTML_TABLE_DGREY">"ANSI_LWHITE"<I>\016":"  ",rooms + orooms,IsHtml(p) ? "\016</I></TH></TR>\016":"\n");

     if(!IsHtml(p)) output(p,player,2,1,0,(char *) separator(twidth,0,'-','-'));
     output(p,player,2,1,0,"%sTotal:%s"ANSI_LWHITE"%-21d%s%-11d%s%-11d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_DGREY"><TH ALIGN=RIGHT BGCOLOR="HTML_TABLE_YELLOW"><FONT SIZE=4><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"                       ",IsHtml(p) ? "\016</I></FONT></TH><TH><FONT SIZE=4>\016":"  ",total,IsHtml(p) ? "\016</FONT></TH><TH><FONT SIZE=4>\016"ANSI_LWHITE:"  ",ototal,IsHtml(p) ? "\016</FONT></TH><TH><FONT SIZE=4>"ANSI_LWHITE"<I>\016":"  ",total + ototal,IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");

     if(IsHtml(p)) {
        output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
        html_anti_reverse(p,0);
     } else if(!in_command) output(p,player,0,1,0,separator(twidth,1,'-','='));
     setreturn(OK,COMMAND_SUCC);
}


/* ---->  Root node, used by stats_rank()  <---- */
static struct list_data *rootnode = NULL,*tail = NULL;


/* ---->  Recursively traverse tertiary tree to sort ranked characters into linked list  <---- */
void stats_rank_traverse(struct rank_data *current)
{
     static struct rank_data *rankptr,*ranknext;
     static struct list_data *new;

     if(current->left) stats_rank_traverse(current->left);
     for(rankptr = current, current = current->right; rankptr; rankptr = ranknext) {
         ranknext = rankptr->centre;
         if(rootnode) {
            MALLOC(new,struct list_data);
            new->player = rankptr->object;
            new->next   = NULL;
            tail->next  = new;
            tail        = new;
	 } else {
            MALLOC(new,struct list_data);
            new->player     = rankptr->object;
            new->next       = NULL;
            tail = rootnode = new;
	 }
         FREENULL(rankptr);
     }

     if(current) stats_rank_traverse(current);
}

/* ---->     Rank users by given criteria      <---- */
/*        (val1:  0 = Rank, 1 = Spod, 2 = Lastseen, 3 = Longspod)  */
void stats_rank(CONTEXT)
{
     unsigned char            cached_scrheight,twidth = output_terminal_width(player);
     unsigned char            direction = 0,entities,order = 0,rtype = 0,found;
     unsigned char            puppets = 1, colspan = 0;
     struct   rank_data       *start = NULL,*current,*last,*new;
     int                      flags,types = 0,loop,loop2,pm;
     time_t                   now,lasttime,curtime,ptrtime;
     double                   pcent1,pcent2,cmp,rdiv = 1;
     struct   descriptor_data *p = getdsc(player),*w;
     dbref                    ptr,rwho = NOTHING;
     const    char            *stitle = NULL;
     unsigned long            *usage = NULL;
     char                     comparison;
     const    char            *rtitle;
     const    char            *colour;
     struct   tm              *rtime;
     char                     *tmp;

     setreturn(ERROR,COMMAND_FAIL);
     if(val1) {

        /* ---->  'spod', 'lastseen' and 'longspod' commands  <---- */
        params = (char *) parse_grouprange(player,params,FIRST,1);
        arg2 = params, arg1 = NULL;
        switch(val1) {
               case 1:
                    order = RANK_ACTIVE;
                    break;
               case 2:
                    order = RANK_LAST;
                    break;
               case 3:
                    order = RANK_LONGEST;
	            break;
	}
     } else arg1 = (char *) parse_grouprange(player,arg1,FIRST,1);
     rootnode = NULL, tail = NULL;

     if(!val1) {

        /* ---->  Ordering of list  <---- */
        for(tmp = scratch_return_string; *arg1 && !((*arg1 == ' ') || (*arg1 == ',') || (*arg1 == '/')); *tmp++ = *arg1++);
        *tmp = '\0';
        if(!BlankContent(scratch_return_string)) {
         if(string_prefix("totaltimeconnected",scratch_return_string)) order = RANK_TOTAL;
	  else if(string_prefix("active",scratch_return_string) || string_prefix("totalactive",scratch_return_string)) order = RANK_ACTIVE;
	   else if(string_prefix("avgactive",scratch_return_string) || string_prefix("averageactive",scratch_return_string)) order = RANK_AVGACTIVE;
	    else if(string_prefix("avgidle",scratch_return_string) || string_prefix("averageidle",scratch_return_string)) order = RANK_AVGIDLE;
	     else if(string_prefix("avglogins",scratch_return_string) || string_prefix("averagelogins",scratch_return_string)) order = RANK_AVGLOGINS, direction = 1;
	      else if(string_prefix("avgtotal",scratch_return_string) || string_prefix("averagetotal",scratch_return_string)) order = RANK_AVGTOTAL;
	       else if(string_prefix("balance",scratch_return_string) || string_prefix("account",scratch_return_string) || string_prefix("bankaccount",scratch_return_string) || string_prefix("bankbalance",scratch_return_string)) order = RANK_BALANCE;
		else if(string_prefix("battles",scratch_return_string)) order = RANK_BATTLES;
		 else if(string_prefix("created",scratch_return_string)) order = RANK_CREATED;
		  else if(string_prefix("credits",scratch_return_string) || string_prefix("money",scratch_return_string) || string_prefix("cash",scratch_return_string)) order = RANK_CREDIT;
		   else if(string_prefix("csize",scratch_return_string)) order = RANK_CSIZE;
		    else if(string_prefix("expenditure",scratch_return_string) || string_prefix("expenses",scratch_return_string) || string_prefix("spendings",scratch_return_string) || string_prefix("spent",scratch_return_string)) order = RANK_EXPENDITURE;
		     else if(string_prefix("idle",scratch_return_string) || string_prefix("totalidle",scratch_return_string)) order = RANK_IDLE;
		      else if(string_prefix("income",scratch_return_string) || string_prefix("incomings",scratch_return_string) || string_prefix("earnings",scratch_return_string) || string_prefix("earned",scratch_return_string)) order = RANK_INCOME;
		       else if(string_prefix("lastconnected",scratch_return_string) || string_prefix("lasttimeconnected",scratch_return_string)) order = RANK_LAST;
			else if(string_prefix("lastused",scratch_return_string)) order = RANK_LASTUSED;
			 else if(string_prefix("logins",scratch_return_string) || string_prefix("totallogins",scratch_return_string)) order = RANK_LOGINS;
			  else if(string_prefix("longesttimeconnected",scratch_return_string)) order = RANK_LONGEST;
			   else if(string_prefix("lost",scratch_return_string)) order = RANK_LOST;
			    else if(string_prefix("quotainuse",scratch_return_string) || string_prefix("buildingquotainuse",scratch_return_string)) order = RANK_QUOTA;
			     else if(string_prefix("quotalimits",scratch_return_string) || string_prefix("buildingquotalimits",scratch_return_string)) order = RANK_QUOTALIMIT;
			      else if(string_prefix("quotaexcessive",scratch_return_string) || string_prefix("buildingquotaexcessive",scratch_return_string) || string_prefix("quotaexceeded",scratch_return_string) || string_prefix("buildingquotaexceeded",scratch_return_string)) order = RANK_QUOTAEXCESS;
			       else if(string_prefix("performance",scratch_return_string)) order = RANK_PERFORMANCE;
				else if(string_prefix("profits",scratch_return_string)) order = RANK_PROFIT;
 			         else if(string_prefix("scores",scratch_return_string) || string_prefix("scorepoints",scratch_return_string)) order = RANK_SCORE;
				  else if(string_prefix("size",scratch_return_string)) order = RANK_SIZE;
				   else if(string_prefix("won",scratch_return_string)) order = RANK_WON;
				    else {
				       output(p,player,0,1,2,ANSI_LGREEN"Please specify one of the following:\n\n"\
					      ANSI_LWHITE"active"ANSI_DGREEN", "\
					      ANSI_LWHITE"avgactive"ANSI_DGREEN", "\
					      ANSI_LWHITE"avgidle"ANSI_DGREEN", "\
					      ANSI_LWHITE"avglogins"ANSI_DGREEN", "\
					      ANSI_LWHITE"avgtotal"ANSI_DGREEN", "\
					      ANSI_LWHITE"balance"ANSI_DGREEN", "\
					      ANSI_LWHITE"battles"ANSI_DGREEN", "\
					      ANSI_LWHITE"created"ANSI_DGREEN", "\
					      ANSI_LWHITE"credit"ANSI_DGREEN", "\
					      ANSI_LWHITE"csize"ANSI_DGREEN", "\
					      ANSI_LWHITE"expenditure"ANSI_DGREEN", "\
					      ANSI_LWHITE"idle"ANSI_DGREEN", "\
					      ANSI_LWHITE"income"ANSI_DGREEN", "\
					      ANSI_LWHITE"lasttime"ANSI_DGREEN", "\
					      ANSI_LWHITE"lastused"ANSI_DGREEN", "\
					      ANSI_LWHITE"logins"ANSI_DGREEN", "\
					      ANSI_LWHITE"longest"ANSI_DGREEN", "\
					      ANSI_LWHITE"lost"ANSI_DGREEN", "\
					      ANSI_LWHITE"performance"ANSI_DGREEN", "\
					      ANSI_LWHITE"profit"ANSI_DGREEN", "\
					      ANSI_LWHITE"quota"ANSI_DGREEN", "\
					      ANSI_LWHITE"quotaexcess"ANSI_DGREEN", "\
					      ANSI_LWHITE"quotalimits"ANSI_DGREEN", "\
					      ANSI_LWHITE"total"ANSI_DGREEN", "\
					      ANSI_LWHITE"score"ANSI_DGREEN", "\
					      ANSI_LWHITE"size"ANSI_DGREEN", "\
					      ANSI_LWHITE"won"ANSI_DGREEN".");
				       return;
				    }

           /* ---->  Constraints  <---- */
	   if((order == RANK_CSIZE) || (order == RANK_SIZE)) {
              if(in_command) {
                 output(p,player,0,1,0,ANSI_LGREEN"Sorry, the '"ANSI_LYELLOW"%ssize"ANSI_LGREEN"' option of the '"ANSI_LWHITE"@rank"ANSI_LGREEN"' command can't be used from within a compound command.",(order == RANK_CSIZE) ? "c":"");
                 return;
	      } else if(((order == RANK_CSIZE) && !Level4(player)) || ((order == RANK_SIZE) && !Level2(player))) {
                 output(p,player,0,1,0,ANSI_LGREEN"Sorry, only %s Wizards/Druids and above may rank characters by %sdatabase memory usage.",(order == RANK_CSIZE) ? "Apprentice":"Elder",(order == RANK_CSIZE) ? "compressed ":"");
                 return;
	      }
	   }

           if((Level4(Owner(player)) && ((order == RANK_CSIZE) || (order == RANK_SIZE))) || (order == RANK_CREDIT) || (order == RANK_CREATED) || (order == RANK_LASTUSED)) {

              /* ---->  Object types ('@rank size', '@rank csize', '@rank credits', '@rank created' and '@rank lastused')  <---- */
              int           valid = 0,result;
              unsigned char cr = 1;

              if(*arg1 && (*arg1 != ' ')) {
                 while(*arg1 && (*arg1 != ' ')) {
                       for(; *arg1 && ((*arg1 == ',') || (*arg1 == '/')); arg1++);
                       for(tmp = scratch_return_string; *arg1 && (*arg1 != ' ') && (*arg1 != ','); *tmp++ = *arg1++);
                       *tmp = '\0';
                       if(!BlankContent(scratch_return_string)) {
                          if((result = parse_objecttype(scratch_return_string))) valid = types |= result;
                             else if(!strcasecmp("all",scratch_return_string)) types = SEARCH_ALL_TYPES, valid = 1, result++;
                          if(!result) output(p,player,0,1,0,"%s"ANSI_LGREEN"Sorry, '"ANSI_LWHITE"%s"ANSI_LGREEN"' is an unknown object type.",(cr) ? "\n":"",scratch_return_string), cr = 0;
		       }
		 }

                 if(!valid) {
	            output(p,player,0,1,0,"%s"ANSI_LGREEN"Please specify the object type(s) to rank.\n",(cr) ? "\n":"");
                    return;
		 }
	      }

              if(types && (order == RANK_CREDIT))
                 types &= (SEARCH_CHARACTER|SEARCH_ROOM|SEARCH_THING);
	   } else if((order == RANK_EXPENDITURE) || (order == RANK_INCOME) || (order == RANK_PROFIT)) {

              /* ---->  Averages ('expenditure', 'income' and 'profit'.)  <---- */
              time_t now;

              while(*arg1 && !isalpha(*arg1) && (*arg1 != ' ')) arg1++;
              for(tmp = scratch_return_string; *arg1 && (*arg1 != ' '); *tmp++ = *arg1++);
              *tmp = '\0';

              gettime(now);
              
              if(BlankContent(scratch_return_string) || string_prefix("quarter",scratch_return_string)) {
                    stitle = "Average per quarter";
                    rdiv   = (double) (now - (quarter - QUARTER)) / QUARTER;
              } else if(string_prefix("day",scratch_return_string)) {
                    stitle = "Average per day";
                    rdiv   = (double) (now - (quarter - QUARTER)) / DAY;
              } else if(string_prefix("week",scratch_return_string)) {
                    stitle = "Average per week";
                    rdiv   = (double) (now - (quarter - QUARTER)) / WEEK;
              } else if(string_prefix("month",scratch_return_string)) {
                    stitle = "Average per month";
                    rdiv   = (double) (now - (quarter - QUARTER)) / MONTH;
              } else {
                    output(p,player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LWHITE"day"ANSI_LGREEN"', '"ANSI_LWHITE"week"ANSI_LGREEN"', '"ANSI_LWHITE"month"ANSI_LGREEN"' or '"ANSI_LWHITE"quarter"ANSI_LGREEN"'.");
                    return;
              }
              if(rdiv < 1) rdiv = 1;
	   }
           for(; *arg1 && (*arg1 == ' '); arg1++);
	} else order = RANK_TOTAL;

        /* ---->  Direction of sorting (Ascending/descending)  <---- */
        for(tmp = scratch_return_string; *arg1 && (*arg1 != ' '); *tmp++ = *arg1++);
        for(; *arg1 && (*arg1 == ' '); arg1++);
        *tmp = '\0';
        if(!BlankContent(scratch_return_string)) {
           if(string_prefix("ascending",scratch_return_string)) {
              direction = 1;
	   } else if(string_prefix("descending",scratch_return_string)) {
              direction = 0;
	   } else {
              output(p,player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LWHITE"ascending"ANSI_LGREEN"' or '"ANSI_LWHITE"descending"ANSI_LGREEN"'.");
              return;
	   }
	}
     }

     /* ---->  Character/object name  <---- */
     if(!Blank(arg2)) {
        if(!types) {
           if(string_prefix("administrators",arg2) || string_prefix("administration",arg2)) {
              rtype = RANK_ADMIN;
	   } else if(string_prefix("friendslist",arg2)) {
              rtype = RANK_FRIENDS;
	   } else if(string_prefix("enemieslist",arg2) || string_prefix("enemylist",arg2)) {
              rtype = RANK_ENEMIES;
	   } else if((rwho = lookup_character(player,arg2,1)) == NOTHING) {
              output(p,player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg2);
              return;
	   } else if((order == RANK_QUOTALIMIT) && Level4(rwho)) {
              output(p,player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is an Apprentice Wizard/Druid or above (Administrators are excluded from '"ANSI_LYELLOW"@rank quotalimit"ANSI_LGREEN"' because they have unlimited Building Quota.)",Article(rwho,LOWER,DEFINITE),getcname(NOTHING,rwho,0,0));
              return;
	   }
	} else {
           rwho = match_preferred(player,player,arg2,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
           if(!Valid(rwho)) return;
	}
     }

     /* ---->  Exclude puppets from character ranking  <---- */
     if((order == RANK_LONGEST)  || (order == RANK_SCORE) || 
        (order == RANK_LOGINS)   || (order == RANK_AVGLOGINS) ||
        (order == RANK_TOTAL)    || (order == RANK_ACTIVE)    || (order == RANK_IDLE) ||
        (order == RANK_AVGTOTAL) || (order == RANK_AVGACTIVE) || (order == RANK_AVGIDLE))
           puppets = 0;

     /* ---->  Construct array of database memory usage for each user/object ('@rank size|csize')  <---- */
     if((order == RANK_SIZE) || (order == RANK_CSIZE)) {
        dbref loop;

        NMALLOC(usage,unsigned long,db_top);
        for(loop = 0; loop < db_top; usage[loop] = 0, loop++);
        if(types) {
	   for(loop = 0; loop < db_top; loop++)
               if((Typeof(loop) != TYPE_FREE) && match_object_type(loop,0,types))
                  usage[loop] = getsize(loop,(order == RANK_SIZE));
	} else for(loop = 0; loop < db_top; loop++)
           if((Typeof(loop) != TYPE_FREE) && Validchar(db[loop].owner))
              usage[db[loop].owner] += getsize(loop,(order == RANK_SIZE));
     }

     /* ---->  Construct tertiary tree of sorted users/objects  <---- */
     gettime(now);
     for(ptr = 0; ptr < db_top; ptr++)
         if((Typeof(ptr) != TYPE_FREE) &&
           ((types && match_object_type(ptr,0,types) && (!types || can_read_from(player,ptr) || can_write_to(player,ptr,1))) || 
           (!types && ((Typeof(ptr) == TYPE_CHARACTER) && (puppets || !Puppet(ptr))))) &&
           ((!rtype && !(((order == RANK_QUOTALIMIT) || (order == RANK_QUOTAEXCESS)) && Level4(ptr))) || 
           ((rtype == RANK_ADMIN) && Level4(ptr)) ||
           ((rtype == RANK_FRIENDS) && (flags = friend_flags(player,ptr)) && !(flags & FRIEND_ENEMY) && !(flags & FRIEND_EXCLUDE)) ||
           ((rtype == RANK_ENEMIES) && (flags = friend_flags(player,ptr)) && (flags & FRIEND_ENEMY) && !(flags & FRIEND_EXCLUDE)))) {

              /* ---->  Pre-processing for current character (PTR)  <---- */
              switch(order) {
                     case RANK_ACTIVE:

                          /* ---->  Total time spent active  <---- */
                          ptrtime = db[ptr].data->player.totaltime;
                          if(Connected(ptr))
                             ptrtime += (now - db[ptr].data->player.lasttime);

                          ptrtime -= db[ptr].data->player.idletime;
                          if(Connected(ptr) && (w = getdsc(ptr)))
                             ptrtime -= (now - w->last_time);
                          break;
                     case RANK_AVGACTIVE:

                          /* ---->  Average time spent active  <---- */
                          ptrtime = db[ptr].data->player.totaltime;
                          if(Connected(ptr))
                             ptrtime += (now - db[ptr].data->player.lasttime);

                          ptrtime -= db[ptr].data->player.idletime;
                          if(Connected(ptr) && (w = getdsc(ptr)))
                             ptrtime -= (now - w->last_time);

                          ptrtime /= ((db[ptr].data->player.logins > 1) ? db[ptr].data->player.logins:1);
                          break;
                     case RANK_AVGIDLE:

                          /* ---->  Average time spent idling  <---- */
                          ptrtime = db[ptr].data->player.idletime;
                          if(Connected(ptr) && (w = getdsc(ptr)))
                             ptrtime += (now - w->last_time);
                          ptrtime /= ((db[ptr].data->player.logins > 1) ? db[ptr].data->player.logins:1);
                          break;
                     case RANK_AVGLOGINS:

                          /* ---->  Average number of logins  <---- */
                          ptrtime = ((now - db[ptr].created) / ((db[ptr].data->player.logins > 1) ? db[ptr].data->player.logins:1));
                          break;
                     case RANK_AVGTOTAL:

                          /* ---->  Average connect time  <---- */
                          ptrtime  = db[ptr].data->player.totaltime;
                          if(Connected(ptr)) ptrtime += (now - db[ptr].data->player.lasttime);
                          ptrtime /= ((db[ptr].data->player.logins > 1) ? db[ptr].data->player.logins:1);
                          break;
                     case RANK_IDLE:

                          /* ---->  Total time spent idling  <---- */
                          ptrtime = db[ptr].data->player.idletime;
                          if(Connected(ptr) && (w = getdsc(ptr)))
                             ptrtime += (now - w->last_time);
                          break;
                     case RANK_LONGEST:

                          /* ---->  Longest connect time (Update if neccessary)  <---- */
                          if(Connected(ptr)) {
                             if((curtime = (now - db[ptr].data->player.lasttime)) == now) curtime = 0;
                             if(db[ptr].data->player.longesttime < curtime)
                                db[ptr].data->player.longesttime = curtime;
			  }
                          break;
                     case RANK_TOTAL:

                          /* ---->  Total connect time  <---- */
                          ptrtime = db[ptr].data->player.totaltime;
                          if(Connected(ptr)) ptrtime += (now - db[ptr].data->player.lasttime);
                          break;
	      }

              if(!start) {

                 /* ---->  Create tertiary tree  <---- */
                 MALLOC(start,struct rank_data);
                 start->left = start->right = start->centre = NULL;
                 start->object = ptr;
	      } else {
                 current = start, last = NULL, found = 0;
                 while(current && !found) {

                       /* ---->  Comparison  <---- */
                       switch(order) {
			      case RANK_ACTIVE:

				   /* ---->  Total time spent active  <---- */
				   curtime = db[current->object].data->player.totaltime;
				   if(Connected(current->object))
                                      curtime += (now - db[current->object].data->player.lasttime);

				   curtime -= db[current->object].data->player.idletime;
				   if(Connected(current->object) && (w = getdsc(current->object)))
				      curtime -= (now - w->last_time);

                                   if(curtime == ptrtime) found = 1;
                                      else if(curtime < ptrtime) comparison = 1;
                                         else comparison = -1;
				   break;
			      case RANK_AVGACTIVE:

				   /* ---->  Average time spent active  <---- */
				   curtime = db[current->object].data->player.totaltime;
				   if(Connected(current->object))
                                      curtime += (now - db[current->object].data->player.lasttime);

				   curtime -= db[current->object].data->player.idletime;
				   if(Connected(current->object) && (w = getdsc(current->object)))
				      curtime -= (now - w->last_time);

                                   curtime /= ((db[current->object].data->player.logins > 1) ? db[current->object].data->player.logins:1);

                                   if(curtime == ptrtime) found = 1;
                                      else if(curtime < ptrtime) comparison = 1;
                                         else comparison = -1;
				   break;
                              case RANK_AVGIDLE:

                                   /* ---->  Average time spent idling  <---- */
                                   curtime = db[current->object].data->player.idletime;
                                   if(Connected(current->object) && (w = getdsc(current->object)))
                                      curtime += (now - w->last_time);

                                   curtime /= ((db[current->object].data->player.logins > 1) ? db[current->object].data->player.logins:1);

                                   if(curtime == ptrtime) found = 1;
                                      else if(curtime < ptrtime) comparison = 1;
                                         else comparison = -1;
                                   break;
                              case RANK_AVGLOGINS:

                                   /* ---->  Average number of logins  <---- */
                                   curtime = ((now - db[current->object].created) / ((db[current->object].data->player.logins > 1) ? db[current->object].data->player.logins:1));
                                   if(curtime == ptrtime) found = 1;
                                      else if(curtime < ptrtime) comparison = 1;
                                         else comparison = -1;
                                   break;
                              case RANK_AVGTOTAL:

                                   /* ---->  Average time connected  <---- */
                                   curtime  = db[current->object].data->player.totaltime;
                                   if(Connected(current->object)) curtime += (now - db[current->object].data->player.lasttime);
                                   curtime /= ((db[current->object].data->player.logins > 1) ? db[current->object].data->player.logins:1);

                                   if(curtime == ptrtime) found = 1;
                                      else if(curtime < ptrtime) comparison = 1;
                                         else comparison = -1;
                                   break;
                              case RANK_BALANCE:

                                   /* ---->  Bank balance  <---- */
                                   if(!(cmp = currency_compare(&(db[ptr].data->player.balance),&(db[current->object].data->player.balance)))) {
                                      if(!(cmp = currency_compare(&(db[ptr].data->player.credit),&(db[current->object].data->player.credit)))) {
                                         found = 1;
                                      } else comparison = ((cmp > 0) ? 1:-1);
                                   } else comparison = ((cmp > 0) ? 1:-1);
                                   break;
                              case RANK_BATTLES:

                                   /* ---->  Total number of battles fought  <---- */
                                   if((db[current->object].data->player.won + db[current->object].data->player.lost) == (db[ptr].data->player.won + db[ptr].data->player.lost)) {
                                      if(db[current->object].data->player.won == db[ptr].data->player.won) {
                                         if(db[current->object].data->player.lost == db[ptr].data->player.lost) found = 1;
                                            else if(db[current->object].data->player.lost < db[ptr].data->player.lost) comparison = 1;
                                               else comparison = -1;
                                      } else if(db[current->object].data->player.won < db[ptr].data->player.won) comparison = 1;
                                         else comparison = -1;
                                   } else if((db[current->object].data->player.won + db[current->object].data->player.lost) < (db[ptr].data->player.won + db[ptr].data->player.lost)) comparison = 1;
                                      else comparison = -1;
                                   break;
                              case RANK_CREATED:

                                   /* ---->  Creation time/date  <---- */
                                   if(db[current->object].created == db[ptr].created) found = 1;
                                      else if(db[current->object].created < db[ptr].created) comparison = 1;
                                         else comparison = -1;
                                   break;
                              case RANK_CREDIT:

                                   /* ---->  Credit in pocket  <---- */
                                   if(types) {
                                      switch(Typeof(current->object)) {
                                             case TYPE_CHARACTER:
                                                  pcent1 = currency_to_double(&(db[current->object].data->player.credit)) + currency_to_double(&(db[current->object].data->player.balance));
                                                  break;
                                             case TYPE_THING:
                                                  pcent1 = currency_to_double(&(db[current->object].data->thing.credit));
                                                  break;
                                             case TYPE_ROOM:
                                                  pcent1 = currency_to_double(&(db[current->object].data->room.credit));
                                                  break;
                                      }

                                      switch(Typeof(ptr)) {
                                             case TYPE_CHARACTER:
                                                  pcent2 = currency_to_double(&(db[ptr].data->player.credit)) + currency_to_double(&(db[ptr].data->player.balance));
                                                  break;
                                             case TYPE_THING:
                                                  pcent2 = currency_to_double(&(db[ptr].data->thing.credit));
                                                  break;
                                             case TYPE_ROOM:
                                                  pcent2 = currency_to_double(&(db[ptr].data->room.credit));
                                                  break;
                                      }
   
                                      if(pcent1 == pcent2) found = 1;
                                         else if(pcent1 < pcent2) comparison = 1;
                                            else comparison = -1;
                                   } else if(!(cmp = currency_compare(&(db[ptr].data->player.credit),&(db[current->object].data->player.credit)))) {
                                      if(!(cmp = currency_compare(&(db[ptr].data->player.balance),&(db[current->object].data->player.balance)))) {
                                         found = 1;
                                      } else comparison = ((cmp > 0) ? 1:-1);
                                   } else comparison = ((cmp > 0) ? 1:-1);
                                   break;
                              case RANK_EXPENDITURE:

                                   /* ---->  Expenditure  <---- */
                                   pcent1 = currency_to_double(&(db[current->object].data->player.expenditure)) / rdiv;
                                   pcent2 = currency_to_double(&(db[ptr].data->player.expenditure)) / rdiv;

                                   if(pcent1 == pcent2) {
                                      pcent1 = currency_to_double(&(db[current->object].data->player.income)) / rdiv;
                                      pcent2 = currency_to_double(&(db[ptr].data->player.income)) / rdiv;

                                      if(pcent1 == pcent2) found = 1;
                                         else if(pcent1 < pcent2) comparison = 1;
                                            else comparison = -1;
                                   } else if(pcent1 < pcent2) comparison = 1;
                                      else comparison = -1;
                                   break;
                              case RANK_IDLE:

                                   /* ---->  Total time spent idling  <---- */
                                   curtime = db[current->object].data->player.idletime;
                                   if(Connected(current->object) && (w = getdsc(current->object)))
                                      curtime += (now - w->last_time);

                                   if(curtime == ptrtime) found = 1;
                                      else if(curtime < ptrtime) comparison = 1;
                                         else comparison = -1;
                                   break;
                              case RANK_INCOME:

                                   /* ---->  Income  <---- */
                                   pcent1 = currency_to_double(&(db[current->object].data->player.income)) / rdiv;
                                   pcent2 = currency_to_double(&(db[ptr].data->player.income)) / rdiv;

                                   if(pcent1 == pcent2) {
                                      pcent1 = currency_to_double(&(db[current->object].data->player.expenditure)) / rdiv;
                                      pcent2 = currency_to_double(&(db[ptr].data->player.expenditure)) / rdiv;

                                      if(pcent1 == pcent2) found = 1;
                                         else if(pcent1 < pcent2) comparison = 1;
                                            else comparison = -1;
                                   } else if(pcent1 < pcent2) comparison = 1;
                                      else comparison = -1;
                                   break;
                              case RANK_LAST:

                                   /* ---->  Last time connected  <---- */
                                   if(db[current->object].data->player.lasttime == db[ptr].data->player.lasttime) found = 1;
                                      else if(db[current->object].data->player.lasttime < db[ptr].data->player.lasttime) comparison = 1;
                                         else comparison = -1;
                                   break;
                              case RANK_LASTUSED:

                                   /* ---->  Time/date last used  <---- */
                                   if(db[current->object].lastused == db[ptr].lastused) found = 1;
                                      else if(db[current->object].lastused < db[ptr].lastused) comparison = 1;
                                         else comparison = -1;
                                   break;
                              case RANK_LOGINS:

                                   /* ---->  Total number of logins  <---- */
                                   if(db[current->object].data->player.logins == db[ptr].data->player.logins) found = 1;
                                      else if(db[current->object].data->player.logins < db[ptr].data->player.logins) comparison = 1;
                                         else comparison = -1;
                                   break;
                              case RANK_LONGEST:

                                   /* ---->  Longest connect time  <---- */
                                   if(db[current->object].data->player.longesttime == db[ptr].data->player.longesttime) found = 1;
                                      else if(db[current->object].data->player.longesttime < db[ptr].data->player.longesttime) comparison = 1;
                                         else comparison = -1;
                                   break;
                              case RANK_LOST:

                                   /* ---->  Total number of battles lost  <---- */
                                   if(db[current->object].data->player.lost == db[ptr].data->player.lost) {
                                      if(db[current->object].data->player.won == db[ptr].data->player.won) found = 1;
                                         else if(db[current->object].data->player.won < db[ptr].data->player.won) comparison = 1;
                                            else comparison = -1;
                                   } else if(db[current->object].data->player.lost < db[ptr].data->player.lost) comparison = 1;
                                      else comparison = -1;
                                   break;
                              case RANK_PERFORMANCE:

                                   /* ---->  Combat performance  <---- */
                                   if((db[current->object].data->player.won + db[current->object].data->player.lost) > 0)
                                      pcent1 = (100 * (db[current->object].data->player.won / (db[current->object].data->player.won + db[current->object].data->player.lost)));
                                         else pcent1 = 0;

                                   if((db[ptr].data->player.won + db[ptr].data->player.lost) > 0)
                                      pcent2 = (100 * (db[ptr].data->player.won / (db[ptr].data->player.won + db[ptr].data->player.lost)));
                                         else pcent2 = 0;

                                   if(pcent1 == pcent2) {
                                      if(db[current->object].data->player.won == db[ptr].data->player.won) {
                                         if(db[current->object].data->player.lost == db[ptr].data->player.lost) found = 1;
                                            else if(db[current->object].data->player.lost < db[ptr].data->player.lost) comparison = 1;
                                               else comparison = -1;
                                      } else if(db[current->object].data->player.won < db[ptr].data->player.won) comparison = 1;
                                         else comparison = -1;
                                   } else if(pcent1 < pcent2) comparison = 1;
                                      else comparison = -1;
                                   break;
                              case RANK_PROFIT:

                                   /* ---->  Profit  <---- */
                                   pcent1  = currency_to_double(&(db[current->object].data->player.income));
                                   pcent1 -= currency_to_double(&(db[current->object].data->player.expenditure));
                                   pcent1 /= rdiv;

                                   pcent2  = currency_to_double(&(db[ptr].data->player.income));
                                   pcent2 -= currency_to_double(&(db[ptr].data->player.expenditure));
                                   pcent2 /= rdiv;

                                   if(pcent1 == pcent2) {
                                      pcent1 = currency_to_double(&(db[current->object].data->player.income)) / rdiv;
                                      pcent2 = currency_to_double(&(db[ptr].data->player.income)) / rdiv;

                                      if(pcent1 == pcent2) {
                                         pcent1 = currency_to_double(&(db[current->object].data->player.expenditure)) / rdiv;
                                         pcent2 = currency_to_double(&(db[ptr].data->player.expenditure)) / rdiv;

                                         if(pcent1 == pcent2) found = 1;
                                            else if(pcent1 < pcent2) comparison = 1;
                                               else comparison = -1;
                                      } else if(pcent1 < pcent2) comparison = 1;
                                         else comparison = -1;
                                   } else if(pcent1 < pcent2) comparison = 1;
                                      else comparison = -1;
                                   break;
                              case RANK_QUOTA:

                                   /* ---->  Building Quota currently in use  <---- */
                                   if(db[current->object].data->player.quota == db[ptr].data->player.quota) found = 1;
                                      else if(db[current->object].data->player.quota < db[ptr].data->player.quota) comparison = 1;
                                         else comparison = -1;
                                   break;
                              case RANK_QUOTAEXCESS:

                                   /* ---->  Building Quota excess  <---- */
                                   if((db[current->object].data->player.quota - (Level4(current->object) ? TCZ_INFINITY:db[current->object].data->player.quotalimit)) == (db[ptr].data->player.quota - (Level4(ptr) ? TCZ_INFINITY:db[ptr].data->player.quotalimit))) found = 1;
                                      else if((db[current->object].data->player.quota - (Level4(current->object) ? TCZ_INFINITY:db[current->object].data->player.quotalimit)) < (db[ptr].data->player.quota - (Level4(ptr) ? TCZ_INFINITY:db[ptr].data->player.quotalimit))) comparison = 1;
                                         else comparison = -1;
                                   break;
                              case RANK_QUOTALIMIT:

                                   /* ---->  Building Quota limit  <---- */
                                   if((Level4(current->object) ? TCZ_INFINITY:db[current->object].data->player.quotalimit) == (Level4(ptr) ? TCZ_INFINITY:db[ptr].data->player.quotalimit)) found = 1;
                                      else if((Level4(current->object) ? TCZ_INFINITY:db[current->object].data->player.quotalimit) < (Level4(ptr) ? TCZ_INFINITY:db[ptr].data->player.quotalimit)) comparison = 1;
                                         else comparison = -1;
                                   break;
                              case RANK_SCORE:

                                   /* ---->  Score  <---- */
                                   if(db[current->object].data->player.score == db[ptr].data->player.score) found = 1;
                                      else if(db[current->object].data->player.score < db[ptr].data->player.score) comparison = 1;
                                         else comparison = -1;
                                   break;
                              case RANK_CSIZE:
                              case RANK_SIZE:

                                   /* ---->  Compressed/uncompressed database memory usage  <---- */
                                   if(usage[current->object] == usage[ptr]) found = 1;
                                      else if(usage[current->object] < usage[ptr]) comparison = 1;
                                         else comparison = -1;
                                   break;
                              case RANK_WON:

                                   /* ---->  Total number of battles won  <---- */
                                   if(db[current->object].data->player.won == db[ptr].data->player.won) {
                                      if(db[current->object].data->player.lost == db[ptr].data->player.lost) found = 1;
                                         else if(db[current->object].data->player.lost < db[ptr].data->player.lost) comparison = 1;
                                            else comparison = -1;
                                   } else if(db[current->object].data->player.won < db[ptr].data->player.won) comparison = 1;
                                      else comparison = -1;
                                   break;
                              case RANK_TOTAL:
                              default:

                                   /* ---->  Total time connected  <---- */
                                   curtime = db[current->object].data->player.totaltime;
                                   if(Connected(current->object)) curtime += (now - db[current->object].data->player.lasttime);
                                   if(curtime == ptrtime) found = 1;
                                      else if(curtime < ptrtime) comparison = 1;
                                         else comparison = -1;
                                   break;
		       }

                       /* ---->  Result of comparison  <---- */
                       if(!found)
                          switch(comparison) {
                                 case -1:
                                      last = current, current = (direction) ? current->left:current->right;
                                      break;
                                 case 1:
                                      last = current, current = (direction) ? current->right:current->left;
                                      break;
			  }
		 }

                 /* ---->  Add to tertiary tree  <---- */
                 MALLOC(new,struct rank_data);
                 if(found) {
                    new->centre     = current->centre;
                    current->centre = new;
		 } else new->centre = NULL;
                 new->left   = new->right = NULL;
                 new->object = ptr;

                 if(!found && last) {
                    if(comparison < 0) {
                       if(direction) last->left = new;
                          else last->right = new;
		    } else if(direction) last->right = new;
                       else last->left = new;
		 }
	      }
	   }

     /* ---->  Produce sorted linked list  <---- */
     if(start) stats_rank_traverse(start);
     cached_scrheight = db[player].data->player.scrheight;
     db[player].data->player.scrheight -= 7;
     if(stitle) db[player].data->player.scrheight--;
     if(order != RANK_LAST) db[player].data->player.scrheight *= 2;
     set_conditions(player,0,0,0,rwho,NULL,500);
     if(rwho != NOTHING) grp->groupno = ALL;
     union_initgrouprange((union group_data *) rootnode);

     /* ---->  Display sorted linked list  <---- */
     entities = IsHtml(p) ? ENTITIES:(((twidth - 33) / 11) - ((order == RANK_LAST) ? 1:0));
     if(IsHtml(p)) {
        html_anti_reverse(p,1);
        output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
     } else if(!in_command && p && !p->pager && !IsHtml(p) && More(player)) pager_init(p);

     if(!in_command) {
        switch(order) {
               case RANK_ACTIVE:

                    /* ---->  Total time spent active  <---- */
                    rtitle = "Time spent active";
                    break;
               case RANK_AVGACTIVE:

                    /* ---->  Average time spent active  <---- */
                    rtitle = "Average time spent active";
                    break;
               case RANK_AVGIDLE:

                    /* ---->  Average time spent idling  <---- */
                    rtitle = "Average time spent idling";
                    break;
               case RANK_AVGLOGINS:

                    /* ---->  Average time between logins  <---- */
                    rtitle = "Average time between logins";
                    break;
               case RANK_AVGTOTAL:

                    /* ---->  Average time connected  <---- */
                    rtitle = "Average time connected";
                    break;
               case RANK_BALANCE:

                    /* ---->  Bank balance  <---- */
                    if(IsHtml(p)) rtitle= "</I></FONT><FONT COLOR="HTML_LYELLOW" SIZE=4><I><U>Bank balance:</U></I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Credit in pocket";
                       else rtitle = ANSI_LYELLOW""ANSI_UNDERLINE"Bank balance:"ANSI_LCYAN"  Credit in pocket";
                    colspan = 1;
                    break;
               case RANK_BATTLES:

                    /* ---->  Total number of battles fought  <---- */
                    if(IsHtml(p)) rtitle= "Won:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Lost:</I></FONT></TH><TH><FONT COLOR="HTML_LYELLOW" SIZE=4><I><U>Battles:</U></I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Performance";
                       else rtitle = "Won:       Lost:      "ANSI_LYELLOW""ANSI_UNDERLINE"Battles:"ANSI_LCYAN"   Performance";
                    colspan = 3;
                    break;
               case RANK_CREATED:

                    /* ---->  Creation time/date  <---- */
                    rtitle = "Creation time/date";
                    break;
               case RANK_CREDIT:

                    /* ---->  Credit in pocket  <---- */
                    if(!types) {
                       if(IsHtml(p)) rtitle= "Bank balance:</I></FONT></TH><TH><FONT COLOR="HTML_LYELLOW" SIZE=4><I><U>Credit in pocket</U>";
                          else rtitle = "Bank balance:  "ANSI_LYELLOW""ANSI_UNDERLINE"Credit in pocket";
                       colspan = 1;
                    } else rtitle = "Credits";
                    break;
               case RANK_CSIZE:

                    /* ---->  Compressed database memory usage  <---- */
                    rtitle = "Database memory usage (Compressed)";
                    break;
               case RANK_EXPENDITURE:

                    /* ---->  Expenditure  <---- */
                    if(IsHtml(p)) rtitle= "Income:</I></FONT></TH><TH><FONT COLOR="HTML_LYELLOW" SIZE=4><I><U>Expenditure:</U></I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Profit";
                       else  rtitle= "Income:         "ANSI_LYELLOW""ANSI_UNDERLINE"Expenditure:"ANSI_LCYAN"    Profit";
                    colspan = 2;
                    break;
               case RANK_IDLE:

                    /* ---->  Total time spent idling  <---- */
                    rtitle = "Time spent idling";
                    break;
               case RANK_INCOME:

                    /* ---->  Income  <---- */
                    if(IsHtml(p)) rtitle= "<FONT COLOR="HTML_LYELLOW" SIZE=4><I><U>Income:</U></I></FONT></I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Expenditure:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Profit";
                       else  rtitle= ANSI_LYELLOW""ANSI_UNDERLINE"Income:"ANSI_LCYAN"         Expenditure:    Profit";
                    colspan = 2;
                    break;
               case RANK_LAST:

                    /* ---->  Last time connected  <---- */
                    rtitle = "Last time connected";
                    break;
               case RANK_LASTUSED:

                    /* ---->  Date/time last used  <---- */

		    /* SAB 19 Nov 2003:
		     * this seems to be broken and (multi-line string)
		     * and makes no sense since it talks about
		     * Income/Expenditure in the RANK_LASTUSED section:
		     * I'm commenting it out.
		     */ 
		    /*
                    if(types) {
                       if(IsHtml(p)) rtitle= "Income:</I></FONT></TH><TH><FONT COLOR="HTML_LYELLOW" SIZE=4><I><U>Expenditure:</U></I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Profit";
                          else  rtitle= " Rank:     ID:          N

Income:         "ANSI_LYELLOW""ANSI_UNDERLINE"Expenditure:"ANSI_LCYAN"    Profit";
		    } else rtitle = "Time/date last used";
		    */
		    rtitle = "Time/date last used";
                    break;
               case RANK_LOGINS:

                    /* ---->  Total number of logins  <---- */
                    rtitle = "Total number of logins";
                    break;
               case RANK_LONGEST:

                    /* ---->  Longest connect time  <---- */
                    rtitle = "Longest time connected";
                    break;
               case RANK_LOST:

                    /* ---->  Total number of battles lost  <---- */
                    if(IsHtml(p)) rtitle= "Won:</I></FONT></TH><TH><FONT COLOR="HTML_LYELLOW" SIZE=4><I><U>Lost:</U></I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Battles:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Performance";
                       else rtitle = "Won:       "ANSI_LYELLOW""ANSI_UNDERLINE"Lost:"ANSI_LCYAN"      Battles:   Performance";
                    colspan = 3;
                    break;
               case RANK_PERFORMANCE:

                    /* ---->  Combat performance  <---- */
                    if(IsHtml(p)) rtitle= "Won:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Lost:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Battles:</I></FONT></TH><TH><FONT COLOR="HTML_LYELLOW" SIZE=4><I><U>Performance</U>";
                       else rtitle = "Won:       Lost:      Battles:   "ANSI_LYELLOW""ANSI_UNDERLINE"Performance";
                    colspan = 3;
                    break;
               case RANK_PROFIT:

                    /* ---->  Profit  <---- */
                    if(IsHtml(p)) rtitle= "Income:</I></FONT></I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Expenditure:</I></FONT></TH><TH><FONT COLOR="HTML_LYELLOW" SIZE=4><I><U>Profit</U>";
                       else  rtitle= "Income:         Expenditure:    "ANSI_UNDERLINE""ANSI_LYELLOW"Profit";
                    colspan = 2;
                    break;
               case RANK_QUOTA:

                    /* ---->  Building Quota currently in use  <---- */
                    if(IsHtml(p)) rtitle= "</I></FONT><FONT COLOR="HTML_LYELLOW" SIZE=4><I><U>Building Quota in use:</U></I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Building Quota limit:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Excess";
                       else rtitle = ANSI_LYELLOW""ANSI_UNDERLINE"In use:"ANSI_LCYAN"     Limit:      Excess";
                    colspan = 2;
                    break;
               case RANK_QUOTAEXCESS:

                    /* ---->  Building Quota excess  <---- */
                    if(IsHtml(p)) rtitle= "Building Quota in use:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Building Quota limit:</I></FONT></TH><TH><FONT COLOR="HTML_LYELLOW" SIZE=4><I><U>Excess</U>";
                       else rtitle = "In use:     Limit:      "ANSI_LYELLOW""ANSI_UNDERLINE"Excess";
                    colspan = 2;
                    break;
               case RANK_QUOTALIMIT:

                    /* ---->  Building Quota limit  <---- */
                    if(IsHtml(p)) rtitle= "Building Quota in use:</I></FONT></TH><TH><FONT COLOR="HTML_LYELLOW" SIZE=4><I><U>Building Quota limit:</U></I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Excess";
                       else rtitle = "In use:     "ANSI_LYELLOW""ANSI_UNDERLINE"Limit:"ANSI_LCYAN"      Excess";
                    colspan = 2;
                    break;
               case RANK_SCORE:

                    /* ---->  Score  <---- */
                    rtitle = "Score";
                    break;
               case RANK_SIZE:

                    /* ---->  Uncompressed database memory usage  <---- */
                    rtitle = "Database memory usage (Decompressed)";
                    break;
               case RANK_WON:

                    /* ---->  Total number of battles won  <---- */
                    if(IsHtml(p)) rtitle= "</I></FONT><FONT COLOR="HTML_LYELLOW" SIZE=4><I><U>Won:</U></I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Lost:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Battles:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Performance";
                       else rtitle = ANSI_LYELLOW""ANSI_UNDERLINE"Won:"ANSI_LCYAN"       Lost:      Battles:   Performance";
                    colspan = 3;
                    break;
               case RANK_TOTAL:
               default:

                    /* ---->  Total time connected  <---- */
                    rtitle = "Total time connected";
                    break;
	}

        if(!IsHtml(p)) {
           if(types) {
              int tlwidth,rwidth,width;

              if(!((order == RANK_CREATED) || (order == RANK_LASTUSED))) {
		 if(stitle) {
		    int slen = strlen(stitle);

		    for(tmp = scratch_return_string, loop = 0; loop < (twidth - slen); *tmp++ = ' ', loop++);
		    *tmp = '\0';
		    output(p,player,0,1,0,"\n%s%s",scratch_return_string,stitle);
		 }

		 rwidth = (order == RANK_CREDIT) ? 14:16;
		 width   = twidth - 29 - rwidth;
		 tlwidth = (strlen(rtitle) + 1);
		 for(; (29 + width + tlwidth) >= twidth; width--);
		 for(tmp = scratch_return_string, loop = 0; loop < width; *tmp++ = ' ', loop++);
		 *tmp = '\0';

                 output(p,player,0,1,0,"%s Rank:     ID:          Name:%s%s:",(stitle) ? "":"\n",scratch_return_string,rtitle);
	      } else output(p,player,0,1,0,"%s Rank:     ID:          Date:       Time:        Name:",(stitle) ? "":"\n");
           } else {
              if(stitle) output(p,player,0,1,0,"\n                                 %s:",stitle);
              output(p,player,0,1,0,"%s Rank:     Name:                 %s:",(stitle) ? "":"\n",rtitle);
           }
           output(p,player,0,1,0,separator(twidth,0,'-','='));
        } else {
           if(!(types && ((order == RANK_CREATED) || (order == RANK_LASTUSED)))) {
              if(stitle) output(p,player,2,1,0,"\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=RIGHT COLSPAN=%d><FONT COLOR="HTML_LCYAN" SIZE=3><I>%s</I></FONT></TH></TR>\016",((types) ? 4:3) + colspan,rtitle);
              if(types) output(p,player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH WIDTH=10%%><FONT COLOR="HTML_LCYAN" SIZE=4><I>Rank:</I></FONT></TH><TH WIDTH=10%%><FONT COLOR="HTML_LCYAN" SIZE=4><I>ID:</I></FONT></TH><TH WIDTH=65%%><FONT COLOR="HTML_LCYAN" SIZE=4><I>Name:</I></FONT></TH><TH WIDTH=15%%><FONT COLOR="HTML_LCYAN" SIZE=4><I>%s:</I></FONT></TH></TR>\016",rtitle);
                 else output(p,player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Rank:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Name:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>%s:</I></FONT></TH></TR>\016",rtitle);
	   } else output(p,player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH WIDTH=10%%><FONT COLOR="HTML_LCYAN" SIZE=4><I>Rank:</I></FONT></TH><TH WIDTH=10%%><FONT COLOR="HTML_LCYAN" SIZE=4><I>ID:</I></FONT></TH><TH WIDTH=10%%><FONT COLOR="HTML_LCYAN" SIZE=4><I>Date:</I></FONT></TH><TH WIDTH=10%%><FONT COLOR="HTML_LCYAN" SIZE=4><I>Time:</I></FONT></TH><TH WIDTH=65%%><FONT COLOR="HTML_LCYAN" SIZE=4><I>Name:</I></FONT></TH></TR>\016",rtitle);
        }
     }

     loop = 0;
     if(grp->distance > 0) {
        while(union_grouprange()) {
              loop++;
              if((rwho == NOTHING) || (grp->cunion->list.player == rwho)) {
                 if(!types) colour = privilege_colour(grp->cunion->list.player);
                 switch(order) {
			case RANK_ACTIVE:

			     /* ---->  Total time spent active  <---- */
			     curtime = db[grp->cunion->list.player].data->player.totaltime;
			     if(Connected(grp->cunion->list.player))
				curtime += (now - db[grp->cunion->list.player].data->player.lasttime);

			     curtime -= db[grp->cunion->list.player].data->player.idletime;
			     if(Connected(grp->cunion->list.player) && (w = getdsc(grp->cunion->list.player)))
				curtime -= (now - w->last_time);

			     if(IsHtml(p)) output(p,player,2,1,0,"\016<TR ALIGN=LEFT><TD BGCOLOR="HTML_TABLE_DGREY">\016%s%s\016</TD><TD ALIGN=LEFT>\016%s%s\016</TD><TD>\016%s%s.\016</TD></TR>\016",colour,rank(grp->before + loop),colour,getname(grp->cunion->list.player),colour,interval(curtime,curtime,entities,0));
                                else output(p,player,0,1,33," %s%-10s%-22s%s.",colour,rank(grp->before + loop),getname(grp->cunion->list.player),interval(curtime,curtime,entities,0));
                             break;
			case RANK_AVGACTIVE:

			     /* ---->  Average time spent active  <---- */
			     curtime = db[grp->cunion->list.player].data->player.totaltime;
			     if(Connected(grp->cunion->list.player))
				curtime += (now - db[grp->cunion->list.player].data->player.lasttime);

			     curtime -= db[grp->cunion->list.player].data->player.idletime;
			     if(Connected(grp->cunion->list.player) && (w = getdsc(grp->cunion->list.player)))
				curtime -= (now - w->last_time);

                             curtime /= ((db[grp->cunion->list.player].data->player.logins > 1) ? db[grp->cunion->list.player].data->player.logins:1);

			     if(IsHtml(p)) output(p,player,2,1,0,"\016<TR ALIGN=LEFT><TD BGCOLOR="HTML_TABLE_DGREY">\016%s%s\016</TD><TD ALIGN=LEFT>\016%s%s\016</TD><TD>\016%s%s.\016</TD></TR>\016",colour,rank(grp->before + loop),colour,getname(grp->cunion->list.player),colour,interval(curtime,curtime,entities,0));
                                else output(p,player,0,1,33," %s%-10s%-22s%s.",colour,rank(grp->before + loop),getname(grp->cunion->list.player),interval(curtime,curtime,entities,0));
                             break;
                        case RANK_AVGIDLE:

                             /* ---->  Average time spent idling  <---- */
                             curtime = db[grp->cunion->list.player].data->player.idletime;
                             if(Connected(grp->cunion->list.player) && (w = getdsc(grp->cunion->list.player)))
                                curtime += (now - w->last_time);

                             curtime /= ((db[grp->cunion->list.player].data->player.logins > 1) ? db[grp->cunion->list.player].data->player.logins:1);

			     if(IsHtml(p)) output(p,player,2,1,0,"\016<TR ALIGN=LEFT><TD BGCOLOR="HTML_TABLE_DGREY">\016%s%s\016</TD><TD ALIGN=LEFT>\016%s%s\016</TD><TD>\016%s%s.\016</TD></TR>\016",colour,rank(grp->before + loop),colour,getname(grp->cunion->list.player),colour,interval(curtime,curtime,entities,0));
                                else output(p,player,0,1,33," %s%-10s%-22s%s.",colour,rank(grp->before + loop),getname(grp->cunion->list.player),interval(curtime,curtime,entities,0));
                             break;
                        case RANK_AVGLOGINS:

                             /* ---->  Average time between logins  <---- */
                             curtime = ((now - db[grp->cunion->list.player].created) / ((db[grp->cunion->list.player].data->player.logins > 1) ? db[grp->cunion->list.player].data->player.logins:1));
			     if(IsHtml(p)) output(p,player,2,1,0,"\016<TR ALIGN=LEFT><TD BGCOLOR="HTML_TABLE_DGREY">\016%s%s\016</TD><TD ALIGN=LEFT>\016%s%s\016</TD><TD>\016%s%s.\016</TD></TR>\016",colour,rank(grp->before + loop),colour,getname(grp->cunion->list.player),colour,interval(curtime,curtime,entities,0));
                                else output(p,player,0,1,33," %s%-10s%-22s%s.",colour,rank(grp->before + loop),getname(grp->cunion->list.player),interval(curtime,curtime,entities,0));
                             break;
                        case RANK_AVGTOTAL:

                             /* ---->  Average time connected  <---- */
                             curtime  = db[grp->cunion->list.player].data->player.totaltime;
                             if(Connected(grp->cunion->list.player)) curtime += (now - db[grp->cunion->list.player].data->player.lasttime);
                             curtime /= ((db[grp->cunion->list.player].data->player.logins > 1) ? db[grp->cunion->list.player].data->player.logins:1);

                             if(IsHtml(p)) output(p,player,2,1,0,"\016<TR ALIGN=LEFT><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_DGREY">\016%s%s\016</TD><TD>\016%s%s\016</TD><TD>\016%s%s.\016</TD></TR>\016",colour,rank(grp->before + loop),colour,getname(grp->cunion->list.player),colour,interval(curtime,curtime,entities,0));
                                else output(p,player,0,1,33," %s%-10s%-22s%s.",colour,rank(grp->before + loop),getname(grp->cunion->list.player),interval(curtime,curtime,entities,0));
                             break;
                        case RANK_BALANCE:
                        case RANK_CREDIT:

                             /* ---->  Bank balance/credit in pocket  <---- */
                             if(types) {
                                switch(Typeof(grp->cunion->list.player)) {
                                       case TYPE_CHARACTER:
                                            cmp = currency_to_double(&(db[grp->cunion->list.player].data->player.credit)) + currency_to_double(&(db[grp->cunion->list.player].data->player.balance));
                                            break;
                                       case TYPE_THING:
                                            cmp = currency_to_double(&(db[grp->cunion->list.player].data->thing.credit));
                                            break;
                                       case TYPE_ROOM:
                                            cmp = currency_to_double(&(db[grp->cunion->list.player].data->room.credit));
                                            break;
                                }

			        if(!IsHtml(p)) {
                                   sprintf(scratch_return_string,"%s%s",Article(grp->cunion->list.player,UPPER,INDEFINITE),getname(grp->cunion->list.player));
                                   loop2 = strlen(scratch_return_string);
                                   if(loop2 > (twidth - 40)) loop2 = (twidth - 40);
                                   tmp = scratch_return_string + loop2;
                                   for(; loop2 < (twidth - 40); *tmp++ = ' ', loop2++);
                                   *tmp = '\0';
                                   output(p,player,0,1,33," "ANSI_LRED"%-10s"ANSI_LGREEN"#%-12d"ANSI_LWHITE"%s  "ANSI_LYELLOW"%.2f",rank(grp->before + loop),grp->cunion->list.player,scratch_return_string,cmp);
                                } else output(p,player,2,1,0,"\016<TR ALIGN=CENTER><TD WIDTH=10%% BGCOLOR="HTML_TABLE_RED">\016"ANSI_LRED"%s\016</TD><TD WIDTH=10%% BGCOLOR="HTML_TABLE_GREEN">\016"ANSI_LGREEN"#%d\016</TD><TD ALIGN=LEFT>\016"ANSI_LWHITE"%s%s\016</TD><TD WIDTH=15%% BGCOLOR="HTML_TABLE_YELLOW">\016"ANSI_LYELLOW"%.2f\016</TD></TR>\016",rank(grp->before + loop),grp->cunion->list.player,Article(grp->cunion->list.player,UPPER,INDEFINITE),getname(grp->cunion->list.player),cmp);
                             } else  {
                                sprintf(scratch_return_string,"%.2f",currency_to_double(&(db[grp->cunion->list.player].data->player.balance)));
                                if(!IsHtml(p)) output(p,player,0,1,33," %s%-10s%-22s%-15s%.2f",colour,rank(grp->before + loop),getname(grp->cunion->list.player),scratch_return_string,currency_to_double(&(db[grp->cunion->list.player].data->player.credit)));
                                   else output(p,player,2,1,0,"\016<TR ALIGN=CENTER><TD BGCOLOR="HTML_TABLE_DGREY">\016%s%s\016</TD><TD ALIGN=LEFT>\016%s%s\016</TD><TD>\016%s%s\016</TD><TD>\016%s%.2f\016</TD></TR>\016",colour,rank(grp->before + loop),colour,getname(grp->cunion->list.player),colour,scratch_return_string,colour,currency_to_double(&(db[grp->cunion->list.player].data->player.credit)));
                             }
                             break;
		        case RANK_CREATED:

                             /* ---->  Creation time/date  <---- */
                             sprintf(scratch_buffer,"(#%d) %s",grp->cunion->list.player,getname(grp->cunion->list.player));
                             scratch_buffer[20] = '\0';

                             if(types) {
                                rtime = localtime(&(db[grp->cunion->list.player].created));
                                if(rtime->tm_hour >= 12) pm = 1;
                                if(rtime->tm_hour > 12) rtime->tm_hour -= 12;
                                   else if(rtime->tm_hour == 0) rtime->tm_hour = 12;

			        if(!IsHtml(p)) {
                                   sprintf(scratch_return_string,"%s%s",Article(grp->cunion->list.player,UPPER,INDEFINITE),getname(grp->cunion->list.player));
                                   loop2 = strlen(scratch_return_string);
                                   if(loop2 > (twidth - 50)) loop2 = (twidth - 50);
                                   tmp = scratch_return_string + loop2;
                                   *tmp = '\0';

                                   strcat(scratch_return_string,".");
                                   if(strlen(scratch_return_string) > (twidth - 50))
                                      strcpy(scratch_return_string + (twidth - 50) - 3,ANSI_DCYAN"...");

                                   output(p,player,0,1,33," "ANSI_LRED"%-10s"ANSI_LGREEN"#%-12d"ANSI_LYELLOW"%02d/%02d/%04d  %2d:%02d.%02d %s  "ANSI_LWHITE"%s",
                                          rank(grp->before + loop),grp->cunion->list.player,
                                          rtime->tm_mday,rtime->tm_mon + 1,rtime->tm_year + 1900,
                                          rtime->tm_hour,rtime->tm_min,rtime->tm_sec,(pm) ? "pm":"am",
                                          scratch_return_string);
				} else {
                                   output(p,player,2,1,0,"\016<TR ALIGN=CENTER><TD BGCOLOR="HTML_TABLE_RED">\016"ANSI_LRED"%s\016</TD><TD BGCOLOR="HTML_TABLE_GREEN">\016"ANSI_LGREEN"#%d\016</TD><TD BGCOLOR="HTML_TABLE_YELLOW">\016"ANSI_LYELLOW"%02d/%02d/%04d\016</TD><TD BGCOLOR="HTML_TABLE_YELLOW">\016"ANSI_LYELLOW"%d:%02d.%02d %s\016</TD><TD ALIGN=LEFT>\016"ANSI_LWHITE"%s%s.\016</TD></TR>\016",
					  rank(grp->before + loop),grp->cunion->list.player,
					  rtime->tm_mday,rtime->tm_mon + 1,rtime->tm_year + 1900,
					  rtime->tm_hour,rtime->tm_min,rtime->tm_sec,(pm) ? "pm":"am",
					  Article(grp->cunion->list.player,UPPER,INDEFINITE),getname(grp->cunion->list.player));
                                   colspan = 1;
				}
			     } else {
   			        if(IsHtml(p)) output(p,player,2,1,0,"\016<TR ALIGN=LEFT><TD BGCOLOR="HTML_TABLE_DGREY">\016%s%s\016</TD><TD ALIGN=LEFT>\016%s%s\016</TD><TD>\016%s%s.\016</TD></TR>\016",colour,rank(grp->before + loop),colour,getname(grp->cunion->list.player),colour,date_to_string(db[grp->cunion->list.player].created,UNSET_DATE,player,FULLDATEFMT));
                                   else output(p,player,0,1,33," %s%-10s%-22s%s.",colour,rank(grp->before + loop),getname(grp->cunion->list.player),date_to_string(db[grp->cunion->list.player].created,UNSET_DATE,player,FULLDATEFMT));
			     }
                             break;
                        case RANK_EXPENDITURE:
                        case RANK_PROFIT:
                        case RANK_INCOME:

                             /* ---->  Expenditure/profit/income  <---- */
                             pcent1 = currency_to_double(&(db[grp->cunion->list.player].data->player.income)) / rdiv;
                             pcent2 = currency_to_double(&(db[grp->cunion->list.player].data->player.expenditure)) / rdiv;
                             cmp    = (pcent1 - pcent2) / rdiv;

                             if(!IsHtml(p)) {
                                sprintf(scratch_return_string,"%.2f",pcent1);
                                sprintf(scratch_return_string + 1000,"%.2f",pcent2);
                                output(p,player,0,1,33," %s%-10s%-22s%-16s%-16s%.2f",colour,rank(grp->before + loop),getname(grp->cunion->list.player),scratch_return_string,scratch_return_string + 1000,cmp);
                             } else output(p,player,2,1,0,"\016<TR ALIGN=CENTER><TD BGCOLOR="HTML_TABLE_DGREY">\016%s%s\016</TD><TD ALIGN=LEFT>\016%s%s\016</TD><TD>\016%s%.2f\016</TD><TD>\016%s%.2f\016</TD><TD>\016%s%.2f\016</TD></TR>\016",colour,rank(grp->before + loop),colour,getname(grp->cunion->list.player),colour,pcent1,colour,pcent2,colour,cmp);
                             break;
                        case RANK_IDLE:

                             /* ---->  Total time spent idling  <---- */
                             curtime = db[grp->cunion->list.player].data->player.idletime;
                             if(Connected(grp->cunion->list.player) && (w = getdsc(grp->cunion->list.player)))
                                curtime += (now - w->last_time);
			     if(IsHtml(p)) output(p,player,2,1,0,"\016<TR ALIGN=LEFT><TD BGCOLOR="HTML_TABLE_DGREY">\016%s%s\016</TD><TD ALIGN=LEFT>\016%s%s\016</TD><TD>\016%s%s.\016</TD></TR>\016",colour,rank(grp->before + loop),colour,getname(grp->cunion->list.player),colour,interval(curtime,curtime,entities,0));
                                else output(p,player,0,1,33," %s%-10s%-22s%s.",colour,rank(grp->before + loop),getname(grp->cunion->list.player),interval(curtime,curtime,entities,0));
                             break;
                        case RANK_LAST:

                             /* ---->  Last time connected  <---- */
                             lasttime = db[grp->cunion->list.player].data->player.lasttime;
                             if(db[player].data->player.timediff) lasttime += (db[player].data->player.timediff * HOUR);
                             if(Connected(grp->cunion->list.player)) {
                                if(IsHtml(p)) output(p,player,2,1,0,"\016<TR ALIGN=LEFT><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_DGREY">\016%s%s\016</TD><TD>\016%s%s\016</TD><TD>\016%s%s.\016<BR>\016(Still connected.)\016</TD></TR>\016",colour,rank(grp->before + loop),colour,getname(grp->cunion->list.player),colour,date_to_string(lasttime,UNSET_DATE,player,FULLDATEFMT));
                                   else output(p,player,0,1,33," %s%-10s%-22s%s.\n(Still connected.)",colour,rank(grp->before + loop),getname(grp->cunion->list.player),date_to_string(lasttime,UNSET_DATE,player,FULLDATEFMT));
			     } else if(IsHtml(p)) output(p,player,2,1,0,"\016<TR ALIGN=LEFT><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_DGREY">\016%s%s\016</TD><TD>\016%s%s\016</TD><TD>\016%s%s.\016<BR>\016(%s.)\016</TD></TR>\016",colour,rank(grp->before + loop),colour,getname(grp->cunion->list.player),colour,date_to_string(lasttime,UNSET_DATE,player,FULLDATEFMT),interval(0,db[grp->cunion->list.player].data->player.lasttime - now,entities,0));
                                else output(p,player,0,1,33," %s%-10s%-22s%s.\n(%s.)",colour,rank(grp->before + loop),getname(grp->cunion->list.player),date_to_string(lasttime,UNSET_DATE,player,FULLDATEFMT),interval(0,db[grp->cunion->list.player].data->player.lasttime - now,entities,0));
                             break;
		        case RANK_LASTUSED:

                             /* ---->  Time/date last used  <---- */
                             sprintf(scratch_buffer,"(#%d) %s",grp->cunion->list.player,getname(grp->cunion->list.player));
                             scratch_buffer[20] = '\0';

                             if(types) {
                                rtime = localtime(&(db[grp->cunion->list.player].lastused));
                                if(rtime->tm_hour >= 12) pm = 1;
                                if(rtime->tm_hour > 12) rtime->tm_hour -= 12;
                                   else if(rtime->tm_hour == 0) rtime->tm_hour = 12;

			        if(!IsHtml(p)) {
                                   sprintf(scratch_return_string,"%s%s",Article(grp->cunion->list.player,UPPER,INDEFINITE),getname(grp->cunion->list.player));
                                   loop2 = strlen(scratch_return_string);
                                   if(loop2 > (twidth - 50)) loop2 = (twidth - 50);
                                   tmp = scratch_return_string + loop2;
                                   *tmp = '\0';

                                   strcat(scratch_return_string,".");
                                   if(strlen(scratch_return_string) > (twidth - 50))
                                      strcpy(scratch_return_string + (twidth - 50) - 3,ANSI_DCYAN"...");

                                   output(p,player,0,1,33," "ANSI_LRED"%-10s"ANSI_LGREEN"#%-12d"ANSI_LYELLOW"%02d/%02d/%04d  %2d:%02d.%02d %s  "ANSI_LWHITE"%s",
                                          rank(grp->before + loop),grp->cunion->list.player,
                                          rtime->tm_mday,rtime->tm_mon + 1,rtime->tm_year + 1900,
                                          rtime->tm_hour,rtime->tm_min,rtime->tm_sec,(pm) ? "pm":"am",
                                          scratch_return_string);
				} else {
                                   output(p,player,2,1,0,"\016<TR ALIGN=CENTER><TD BGCOLOR="HTML_TABLE_RED">\016"ANSI_LRED"%s\016</TD><TD BGCOLOR="HTML_TABLE_GREEN">\016"ANSI_LGREEN"#%d\016</TD><TD BGCOLOR="HTML_TABLE_YELLOW">\016"ANSI_LYELLOW"%02d/%02d/%04d\016</TD><TD BGCOLOR="HTML_TABLE_YELLOW">\016"ANSI_LYELLOW"%d:%02d.%02d %s\016</TD><TD ALIGN=LEFT>\016"ANSI_LWHITE"%s%s.\016</TD></TR>\016",
					  rank(grp->before + loop),grp->cunion->list.player,
					  rtime->tm_mday,rtime->tm_mon + 1,rtime->tm_year + 1900,
					  rtime->tm_hour,rtime->tm_min,rtime->tm_sec,(pm) ? "pm":"am",
					  Article(grp->cunion->list.player,UPPER,INDEFINITE),getname(grp->cunion->list.player));
                                   colspan = 1;
				}
			     } else {
   			        if(IsHtml(p)) output(p,player,2,1,0,"\016<TR ALIGN=LEFT><TD BGCOLOR="HTML_TABLE_DGREY">\016%s%s\016</TD><TD ALIGN=LEFT>\016%s%s\016</TD><TD>\016%s%s.\016</TD></TR>\016",colour,rank(grp->before + loop),colour,getname(grp->cunion->list.player),colour,date_to_string(db[grp->cunion->list.player].lastused,UNSET_DATE,player,FULLDATEFMT));
                                   else output(p,player,0,1,33," %s%-10s%-22s%s.",colour,rank(grp->before + loop),getname(grp->cunion->list.player),date_to_string(db[grp->cunion->list.player].lastused,UNSET_DATE,player,FULLDATEFMT));
			     }
                             break;
                        case RANK_LOGINS:

                             /* ---->  Total number of logins  <---- */
			     if(IsHtml(p)) output(p,player,2,1,0,"\016<TR ALIGN=CENTER><TD BGCOLOR="HTML_TABLE_DGREY">\016%s%s\016</TD><TD ALIGN=LEFT>\016%s%s\016</TD><TD>\016%s%d\016</TD></TR>\016",colour,rank(grp->before + loop),colour,getname(grp->cunion->list.player),colour,db[grp->cunion->list.player].data->player.logins);
                                else output(p,player,0,1,33," %s%-10s%-22s%d",colour,rank(grp->before + loop),getname(grp->cunion->list.player),db[grp->cunion->list.player].data->player.logins);
                             break;
                        case RANK_LONGEST:

                             /* ---->  Longest connect time  <---- */
                             if(IsHtml(p)) output(p,player,2,1,0,"\016<TR ALIGN=LEFT><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_DGREY">\016%s%s\016</TD><TD>\016%s%s\016</TD><TD>\016%s%s.\016</TD></TR>\016",colour,rank(grp->before + loop),colour,getname(grp->cunion->list.player),colour,interval(db[grp->cunion->list.player].data->player.longesttime,0,entities,0));
                                else output(p,player,0,1,33," %s%-10s%-22s%s.",colour,rank(grp->before + loop),getname(grp->cunion->list.player),interval(db[grp->cunion->list.player].data->player.longesttime,0,entities,0));
                             break;
                        case RANK_QUOTAEXCESS:
                        case RANK_QUOTALIMIT:
                        case RANK_QUOTA:

                             /* ---->  Building quota excess/limit/currently in use  <---- */
                            if Level4(grp->cunion->list.player) {
                                if(IsHtml(p)) output(p,player,2,1,0,"\016<TR ALIGN=CENTER><TD BGCOLOR="HTML_TABLE_DGREY">\016%s%s\016</TD><TD ALIGN=LEFT>\016%s%s\016</TD><TD>\016%s%d\016</TD><TD>\016%s%s\016</TD><TD>\016%s%s\016</TD></TR>\016",colour,rank(grp->before + loop),colour,getname(grp->cunion->list.player),colour,db[grp->cunion->list.player].data->player.quota,colour,"UNLIMITED",colour,"n/a");
                                    else output(p,player,0,1,33," %s%-10s%-22s%-12d%-12s%s",colour,rank(grp->before + loop),getname(grp->cunion->list.player),db[grp->cunion->list.player].data->player.quota,"UNLIMITED","n/a");
                            } else {
                                if(IsHtml(p)) output(p,player,2,1,0,"\016<TR ALIGN=CENTER><TD BGCOLOR="HTML_TABLE_DGREY">\016%s%s\016</TD><TD ALIGN=LEFT>\016%s%s\016</TD><TD>\016%s%d\016</TD><TD>\016%s%d\016</TD><TD>\016%s%d\016</TD></TR>\016",colour,rank(grp->before + loop),colour,getname(grp->cunion->list.player),colour,db[grp->cunion->list.player].data->player.quota,colour,db[grp->cunion->list.player].data->player.quotalimit,colour,db[grp->cunion->list.player].data->player.quota - db[grp->cunion->list.player].data->player.quotalimit);
                                    else output(p,player,0,1,33," %s%-10s%-22s%-12d%-12d%d",colour,rank(grp->before + loop),getname(grp->cunion->list.player),db[grp->cunion->list.player].data->player.quota,db[grp->cunion->list.player].data->player.quotalimit,db[grp->cunion->list.player].data->player.quota - db[grp->cunion->list.player].data->player.quotalimit);
                            }
                             break;
                        case RANK_SCORE:

                             /* ---->  Score  <---- */
			     if(IsHtml(p)) output(p,player,2,1,0,"\016<TR ALIGN=CENTER><TD BGCOLOR="HTML_TABLE_DGREY">\016%s%s\016</TD><TD ALIGN=LEFT>\016%s%s\016</TD><TD>\016%s%d\016</TD></TR>\016",colour,rank(grp->before + loop),colour,getname(grp->cunion->list.player),colour,db[grp->cunion->list.player].data->player.score);
                                else output(p,player,0,1,33," %s%-10s%-22s%d",colour,rank(grp->before + loop),getname(grp->cunion->list.player),db[grp->cunion->list.player].data->player.score);
                             break;
                        case RANK_CSIZE:
                        case RANK_SIZE:

                             /* ---->  Compressed/uncompressed database memory usage  <---- */
                             if(types) {
			        if(!IsHtml(p)) {
                                   sprintf(scratch_return_string,"%s%s",Article(grp->cunion->list.player,UPPER,INDEFINITE),getname(grp->cunion->list.player));
                                   loop2 = strlen(scratch_return_string);
                                   if(loop2 > (twidth - 42)) loop2 = (twidth - 42);
                                   tmp = scratch_return_string + loop2;
                                   for(; loop2 < (twidth - 42); *tmp++ = ' ', loop2++);
                                   *tmp = '\0';
				   output(p,player,0,1,33," "ANSI_LRED"%-10s"ANSI_LGREEN"#%-12d"ANSI_LWHITE"%s  "ANSI_LYELLOW"%.2fKb",rank(grp->before + loop),grp->cunion->list.player,scratch_return_string,(double) usage[grp->cunion->list.player] / KB);
				} else output(p,player,2,1,0,"\016<TR ALIGN=CENTER><TD WIDTH=10%% BGCOLOR="HTML_TABLE_RED">\016"ANSI_LRED"%s\016</TD><TD WIDTH=10%% BGCOLOR="HTML_TABLE_GREEN">\016"ANSI_LGREEN"#%d\016</TD><TD ALIGN=LEFT>\016"ANSI_LWHITE"%s%s\016</TD><TD WIDTH=15%% BGCOLOR="HTML_TABLE_YELLOW">\016"ANSI_LYELLOW"%.2fKb\016</TD></TR>\016",rank(grp->before + loop),grp->cunion->list.player,Article(grp->cunion->list.player,UPPER,INDEFINITE),getname(grp->cunion->list.player),(double) usage[grp->cunion->list.player] / KB);
			     } else if(IsHtml(p)) output(p,player,2,1,0,"\016<TR ALIGN=CENTER><TD BGCOLOR="HTML_TABLE_DGREY">\016%s%s\016</TD><TD ALIGN=LEFT>\016%s%s\016</TD><TD>\016%s%.2fKb\016</TD></TR>\016",colour,rank(grp->before + loop),colour,getname(grp->cunion->list.player),colour,(double) usage[grp->cunion->list.player] / KB);
                                else output(p,player,0,1,33," %s%-10s%-22s%.2fKb",colour,rank(grp->before + loop),getname(grp->cunion->list.player),(double) usage[grp->cunion->list.player] / KB);
                             break;
                        case RANK_BATTLES:
                        case RANK_WON:
                        case RANK_LOST:
                        case RANK_PERFORMANCE:

                             /* ---->  Total number of battles, battles won/lost and performance  <---- */
                             if(!IsHtml(p)) {
                                output(p,player,0,1,33," %s%-10s%-22s%-11d%-11d%-11d%s",colour,rank(grp->before + loop),getname(grp->cunion->list.player),db[grp->cunion->list.player].data->player.won,db[grp->cunion->list.player].data->player.lost,db[grp->cunion->list.player].data->player.won + db[grp->cunion->list.player].data->player.lost,combat_percent(db[grp->cunion->list.player].data->player.won,db[grp->cunion->list.player].data->player.won + db[grp->cunion->list.player].data->player.lost));
                             } else output(p,player,2,1,0,"\016<TR ALIGN=CENTER><TD BGCOLOR="HTML_TABLE_DGREY">\016%s%s\016</TD><TD ALIGN=LEFT>\016%s%s\016</TD><TD>\016%s%d\016</TD><TD>\016%s%d\016</TD><TD>\016%s%d\016</TD><TD>\016%s%s\016</TD></TR>\016",colour,rank(grp->before + loop),colour,getname(grp->cunion->list.player),colour,db[grp->cunion->list.player].data->player.won,colour,db[grp->cunion->list.player].data->player.lost,colour,db[grp->cunion->list.player].data->player.won + db[grp->cunion->list.player].data->player.lost,colour,combat_percent(db[grp->cunion->list.player].data->player.won,db[grp->cunion->list.player].data->player.won + db[grp->cunion->list.player].data->player.lost));
                             break;
                        case RANK_TOTAL:
                        default:

                             /* ---->  Total time connected  <---- */
                             curtime = db[grp->cunion->list.player].data->player.totaltime;
                             if(Connected(grp->cunion->list.player)) curtime += (now - db[grp->cunion->list.player].data->player.lasttime);
                             if(IsHtml(p)) output(p,player,2,1,0,"\016<TR ALIGN=LEFT><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_DGREY">\016%s%s\016</TD><TD>\016%s%s\016</TD><TD>\016%s%s.\016</TD></TR>\016",colour,rank(grp->before + loop),colour,getname(grp->cunion->list.player),colour,interval(curtime,curtime,entities,0));
                                else output(p,player,0,1,33," %s%-10s%-22s%s.",colour,rank(grp->before + loop),getname(grp->cunion->list.player),interval(curtime,curtime,entities,0));
                             break;
		 }
	      }
	}

        if(!in_command) {
           if(rwho != NOTHING) grp->groupitems = 1;
           if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
           if(IsHtml(p)) sprintf(scratch_return_string,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><TD COLSPAN=%d>"ANSI_LWHITE"<B>\016",((types) ? 4:3) + colspan);
           output(p,player,2,1,0,"%s%s listed: \016&nbsp;\016 "ANSI_DWHITE"%s%s",IsHtml(p) ? scratch_return_string:ANSI_LWHITE" ",!((val1 == 1) || (val1 == 3)) ? (types) ? "Objects":"Users":"Spods",listed_items(scratch_buffer,1),IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
	}
     } else {
        if(IsHtml(p)) output(p,player,2,1,0,"\016<TR ALIGN=CENTER><TD COLSPAN=%d>"ANSI_LCYAN"<I>*** &nbsp; NO %s FOUND &nbsp; ***</I></TD></TR>\016",((types) ? 4:3) + colspan,!((val1 == 1) || (val1 == 3)) ? (types) ? "OBJECTS":"USERS":"SPODS");
           else output(p,player,2,1,0," ***  NO %s FOUND  ***\n",!((val1 == 1) || (val1 == 3)) ? (types) ? "OBJECTS":"USERS":"SPODS");

        if(!in_command) {
           if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
           if(IsHtml(p)) sprintf(scratch_return_string,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><TD COLSPAN=%d>"ANSI_LWHITE"<B>\016",((types) ? 4:3) + colspan);
           output(p,player,2,1,0,"%s%s listed: \016&nbsp;\016 "ANSI_DWHITE"None.%s",IsHtml(p) ? scratch_return_string:ANSI_LWHITE" ",!((val1 == 1) || (val1 == 3)) ? (types) ? "Objects":"Users":"Spods",IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
	}
     }

     if(IsHtml(p)) {
        output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
        html_anti_reverse(p,0);
     }

     /* ---->  Free linked list  <---- */
     if(usage) FREENULL(usage);
     db[player].data->player.scrheight = cached_scrheight;
     for(; rootnode; rootnode = tail) {
         tail = rootnode->next;
         FREENULL(rootnode);
     }
     setreturn(OK,COMMAND_SUCC);
}
