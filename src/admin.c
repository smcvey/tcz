/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| ADMIN.C  -  Implements administrative commands (I.e:  Those only available  |
|             to Apprentice Wizards/Druids and above.)                        |
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
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: admin.c,v 1.4 2005/06/29 20:36:15 tcz_monster Exp $

*/


#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "descriptor_flags.h"
#include "object_types.h"
#include "objectlists.h"
#include "flagset.h"
#include "search.h"
#include "fields.h"
#include "match.h"
#include "quota.h"

#ifndef CYGWIN32
   #include <crypt.h>
#endif


char  *shutdown_reason = NULL;
int   shutdown_counter = -1;
long  shutdown_timing  = 0;
dbref shutdown_who     = NOTHING;


/* ---->  Determine whether Experienced Builders/Assistants may give  <---- */
/*        assistance/welcome users (If total number of administrators able  */
/*        to assist/welcome is less than 6, Experienced Builders/Assistants */
/*        may give assistance and welcome users.)                           */

/*        EXCLUDES:  *  QUIET flag.                                         */
/*                   *  Puppets.                                            */
/*                   *  Lost connections.                                   */
/*                   *  Idle users (> IDLE_TIME.)                           */

unsigned char admin_can_assist(void) {
	 struct descriptor_data *d;
	 int    count = 0;
	 time_t now;

	 gettime(now);
	 for(d = descriptor_list; d; d = d->next)
	     if((d->flags & CONNECTED) && Level4(d->player))
		if(!Quiet(d->player) && !Puppet(d->player) && !(d->flags & DISCONNECTED) && ((now - d->last_time) < (IDLE_TIME * MINUTE)))
		   count++;
	 return(count < ASSIST_MINIMUM);
}

/* ---->  Notify appropriate administrators/Experienced Builders/Assistants  <---- */
/*        with reminder message to give assistance/welcome new users.              */

/* METHOD:  (1)  Notify non-QUIET and non-puppet Admin, starting at lowest  */
/*               rank, until at least 2 different ranks have been notified  */
/*               and a minimum of 6 individual Admin who are not idle have  */
/*               been notified.                                             */

/*          (2)  Notify Assistants and Experienced Builders with HELP flag  */
/*               set, if allowed to give assistance/welcome                 */
/*               (admin_can_assist()) or if total non-idle Admin notified   */
/*               is less than 6.                                            */

/*          (3)  If still less than 6 users in total have been notified,    */
/*               notify Assistants and Experienced Builders with HELP flag  */
/*               set (Except QUIET and puppets) and notify all connected    */
/*               Admin (Regardless of QUIET and puppets.)                   */

void admin_notify_assist(const char *message,const char *altmsg,dbref exclude) {
     struct   descriptor_data *d;
     int      levels = 0;  /*  Number of different Admin levels notified  */
     int      total  = 0;  /*  Total number of users notified             */
     unsigned char rank;
     int      loop;
     time_t   now; 

     /* ---->  Initialisation  <---- */
     gettime(now);
     levels = 0, total = 0, rank = 0;
     for(d = descriptor_list; d; d = d->next)
         d->flags2 &= ~NOTIFIED;

     /* ---->  Notify connected Admin (Ignore QUIET, Puppets and lost connections)  <---- */
     for(loop = 4, rank = 0; (loop >= 0) && !((levels >= 2) && (total >= ASSIST_MINIMUM)); loop--) {
         for(d = descriptor_list; d; d = d->next)
             if((d->flags & CONNECTED) && !(d->flags & DISCONNECTED) && !(d->flags2 & NOTIFIED) && (d->player != exclude) && !Quiet(d->player) && !Puppet(d->player))
                if(privilege(d->player,4) == loop) {
                   output(d,d->player,0,1,0,(char *) message);
                   d->flags2 |= NOTIFIED;
                   if((now - d->last_time) < (IDLE_TIME * MINUTE))
                      total++, rank = 1;
                }
         if(rank) levels++, rank = 0;
     }

     /* ---->  Notify Experienced Builders/Assistants if non-idle Admin count is below required level, or too few Admin notified  <---- */
     if(admin_can_assist() || (total < ASSIST_MINIMUM))
        for(d = descriptor_list; d; d = d->next)
            if((d->flags & CONNECTED) && !(d->flags & DISCONNECTED) && !(d->flags2 & NOTIFIED) && (d->player != exclude) && !Quiet(d->player) && !Puppet(d->player))
               if(!Level4(d->player) && (Assistant(d->player) || (Experienced(d->player) && Help(d->player)))) {
                  output(d,d->player,0,1,0,(altmsg) ? (char *) altmsg : (char *) message);
                  d->flags2 |= NOTIFIED;
                  total++;
	       }

     /* ---->  Notify all connected Admin, Experienced Builders and Assistants if total notified is still too few  <---- */
     if(total < ASSIST_MINIMUM)
        for(d = descriptor_list; d; d = d->next)
            if((d->flags & CONNECTED) && !(d->flags & DISCONNECTED) && !(d->flags2 & NOTIFIED) && (d->player != exclude) && ((!Quiet(d->player) && !Puppet(d->player)) || Level4(d->player)))
               if(Level4(d->player) || Assistant(d->player) || (Experienced(d->player) && Help(d->player))) {
                  output(d,d->player,0,1,0,(altmsg && !Level4(d->player)) ? (char *) altmsg : (char *) message);
                  d->flags2 |= NOTIFIED;
                  total++;
               }
   
     /* ---->  Reset NOTIFIED flag on descriptors  <---- */
     for(d = descriptor_list; d; d = d->next)
         d->flags2 &= ~NOTIFIED;
}

/* ---->  Send welcome blurb to selected users  <---- */
#ifdef NOTIFY_WELCOME
void admin_welcome_message(struct descriptor_data *d,unsigned char guest)
{
     char buffer1[TEXT_SIZE],buffer2[TEXT_SIZE];

     if(Validchar(d->player)) {
        wrap_leading = 11;
        sprintf(buffer1,ANSI_LGREEN"[WELCOME] \016&nbsp;\016 "ANSI_LWHITE"%s"ANSI_LYELLOW"%s"ANSI_LWHITE" (A new%s user) has just been created from "ANSI_LCYAN"%s"ANSI_LWHITE".  Please type '"ANSI_LGREEN"welcome %s"ANSI_LWHITE"' to welcome them to %s and let them know that you're available to give them help and assistance, should they need it.",Article(d->player,UPPER,INDEFINITE),getcname(NOTHING,d->player,0,0),(guest) ? " guest":"",String(d->hostname),getname(d->player),tcz_full_name);
        sprintf(buffer2,ANSI_LGREEN"[WELCOME] \016&nbsp;\016 "ANSI_LWHITE"%s"ANSI_LYELLOW"%s"ANSI_LWHITE" (A new%s user) has just been created.  Please type '"ANSI_LGREEN"welcome %s"ANSI_LWHITE"' to welcome them to %s and let them know that you're available to give them help and assistance, should they need it.",Article(d->player,UPPER,INDEFINITE),getcname(NOTHING,d->player,0,0),(guest) ? " guest":"",getname(d->player),tcz_full_name);
        admin_notify_assist(buffer1,buffer2,NOTHING);
        wrap_leading = 0;
     }
}
#endif

/* ---->  {J.P.Boggis 13/08/2000}  Make time adjustment, updating time sensitive variables to compensate  <---- */
void admin_time_adjust(int adjustment)
{
     struct descriptor_data *d;

     /* ---->  Adjust server time and other time sensitive global variables  <---- */
     shutdown_timing += adjustment;
     dumptiming      += adjustment;
     timeadjust      += adjustment;
     nextcycle       += adjustment;
     activity        += adjustment;
     uptime          += adjustment;
     tcz_time_sync(1);

     /* ---->  Adjust descriptor list time sensitive variables  <---- */
     for(d = descriptor_list; d; d = d->next) {
	 d->emergency_time += adjustment;
	 d->assist_time    += adjustment;
	 d->start_time     += adjustment;
	 d->last_time      += adjustment;
	 d->name_time      += adjustment;
	 d->next_time      += adjustment;
	 d->afk_time       += adjustment;
	 if((d->flags & CONNECTED) && Validchar(d->player))
	    db[d->player].data->player.lasttime += adjustment;
     }
}

/* ---->  Display/set server administrative options  <---- */
void admin_options(CONTEXT)
{
     unsigned char errorlevel = 0, mtype = 0, mlimit = 7;
     const    char *adminoption = "*UNKNOWN*";
     long     numb = 0;

     setreturn(ERROR,COMMAND_FAIL);
     if(!in_command || (Valid(current_command) && Elder(current_command))) {
        if(Level4(Owner(player))) {
           if(Blank(arg1)) {
              struct   descriptor_data *p = getdsc(player);
              unsigned char twidth = output_terminal_width(player);
              unsigned char cached_dumpstatus = dumpstatus;

              /* ---->  Display current option settings  <---- */
              html_anti_reverse(p,1);
              if(IsHtml(p)) output(p,player,1,2,0,"<BR><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">");
              output(p,player,2,1,1,"%s%s Server Administrative Options...%s",IsHtml(p) ? "\016<TR><TH ALIGN=CENTER COLSPAN=2 BGCOLOR="HTML_TABLE_CYAN"><FONT SIZE=4><I>\016"ANSI_LCYAN:"\n ",tcz_short_name,IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
              if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));

              /* ---->  Database dumping  <---- */
              gettime(numb);
              if(dumpstatus > 0) {
                 dumpstatus = 250;
                 db_write(p);
                 if(!BlankContent(cmpbuf)) output(p,player,2,1,30,"%s",cmpbuf);
	      }
              dumpstatus = 251;
              db_write(p);
              if(!BlankContent(cmpbuf)) output(p,player,2,1,30,"%s",cmpbuf);
              dumpstatus = cached_dumpstatus;
              if(dumpstatus <= 0) {
                 if(option_dumping(OPTSTATUS))
                    output(p,player,2,1,30,"%sTime until next dump:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=35% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LGREEN:ANSI_LGREEN"       ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",interval((dumptiming + dumpinterval) - numb,(dumptiming + dumpinterval) - numb,ENTITIES,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
                       else output(p,player,2,1,30,"%sTime until next dump:%s"ANSI_LRED"Disabled.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=35% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LGREEN:ANSI_LGREEN"       ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",IsHtml(p) ? "\016</TD></TR>\016":"\n");
	      }

              /* ---->  Time left until server shutdown  <---- */
              if(shutdown_counter >= 0) {
                 if((shutdown_timing > 0) && (shutdown_timing <= numb)) numb -= shutdown_timing;
                    else numb = 0;
                 output(p,player,2,1,30,"%sServer shutdown:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=35% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LRED:ANSI_LRED"            ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",interval((shutdown_counter * MINUTE) - numb,(shutdown_counter * MINUTE) - numb,ENTITIES,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
	      }

              if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','-'));

              /* ---->  Database dumping options  <---- */
              output(p,player,2,1,30,"%sDatabase "ANSI_LYELLOW""ANSI_UNDERLINE"dumping"ANSI_LGREEN" interval:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=35% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LGREEN:ANSI_LGREEN"  ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",interval(dumpinterval,dumpinterval,ENTITIES,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
              output(p,player,2,1,30,"%s"ANSI_UNDERLINE"Data"ANSI_LGREEN" dumped per cycle:%s"ANSI_LWHITE"%dkb.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=35% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"      ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",dumpdatasize / KB,IsHtml(p) ? "\016</TD></TR>\016":"\n");
              output(p,player,2,1,30,"%sDump "ANSI_LYELLOW""ANSI_UNDERLINE"cycle"ANSI_LGREEN" interval:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=35% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LGREEN:ANSI_LGREEN"        ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",interval(dumpcycle,dumpcycle,ENTITIES,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");

              if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','-'));

              /* ---->  Maximum connections restrictions  <---- */
              output(p,player,2,1,30,"%s"ANSI_UNDERLINE"Limit"ANSI_LGREEN" maximum connections:%s"ANSI_LWHITE"%s"ANSI_LGREEN", \016&nbsp;\016"ANSI_LGREEN"\016<B><I>\016Connections\016</I></B>\016 "ANSI_LYELLOW""ANSI_UNDERLINE"\016<B><I>\016allowed\016</I></B>\016"ANSI_LGREEN"\016<B><I>\016:\016</I></B> &nbsp;\016 "ANSI_LWHITE"%d"ANSI_LGREEN".%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=35% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"  ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",(limit_connections) ? "Yes":"No",allowed,IsHtml(p) ? "\016</TD></TR>\016":"\n");

#ifdef HOME_ROOMS

              /* ---->  {J.P.Boggis 23/07/2000}  Home rooms container  <---- */
              if(Valid(homerooms)) sprintf(scratch_return_string,"#%d",homerooms);
                 else strcpy(scratch_return_string,"Not set");
              output(p,player,2,1,30,"%s"ANSI_UNDERLINE"Home"ANSI_LGREEN" rooms container:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=35% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"       ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",scratch_return_string,IsHtml(p) ? "\016</TD></TR>\016":"\n");
#endif

              /* ---->  Allow user creations/connections  <---- */
              output(p,player,2,1,30,"%s\016<B><I>\016Allow user\016</I></B>\016 "ANSI_LYELLOW""ANSI_UNDERLINE"creation"ANSI_LGREEN"\016<B><I>\016:\016</I></B>\016%s"ANSI_LWHITE"%s"ANSI_LGREEN", \016&nbsp;\016"ANSI_LGREEN"\016<B><I>\016Allow user\016</I></B>\016 "ANSI_LYELLOW""ANSI_UNDERLINE"\016<B><I>\016connections\016</I></B>\016"ANSI_LGREEN"\016<B><I>\016:\016</I></B> &nbsp;\016 "ANSI_LWHITE"%s"ANSI_LGREEN".%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=35% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LGREEN:ANSI_LGREEN"        ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",(creations) ? (creations == 2) ? "Guests Only":"Yes":"No",(connections) ? "Yes":"No",IsHtml(p) ? "\016</TD></TR>\016":"\n");

              /* ---->  Bank access room  <---- */
              if(Valid(bankroom)) sprintf(scratch_return_string,"#%d",bankroom);
                 else strcpy(scratch_return_string,"Not set");
              output(p,player,2,1,30,"%s"ANSI_UNDERLINE"Bank"ANSI_LGREEN" access room:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=35% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"           ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",scratch_return_string,IsHtml(p) ? "\016</TD></TR>\016":"\n");

              /* ---->  Mail access room  <---- */
              if(Valid(mailroom)) sprintf(scratch_return_string,"#%d",mailroom);
                 else strcpy(scratch_return_string,"Not set");
              output(p,player,2,1,30,"%s"ANSI_UNDERLINE"Mail"ANSI_LGREEN" access room:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=35% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"           ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",scratch_return_string,IsHtml(p) ? "\016</TD></TR>\016":"\n");

              /* ---->  BBS access room  <---- */
              if(Valid(bbsroom)) sprintf(scratch_return_string,"#%d",bbsroom);
                 else strcpy(scratch_return_string,"Not set");
              output(p,player,2,1,30,"%s"ANSI_UNDERLINE"BBS"ANSI_LGREEN" access room:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=35% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"            ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",scratch_return_string,IsHtml(p) ? "\016</TD></TR>\016":"\n");

              /* ---->  Current server time adjustment setting  <---- */
              output(p,player,2,1,30,"%s"ANSI_UNDERLINE"Time"ANSI_LGREEN" adjustment:%s"ANSI_LWHITE"(%c) %s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=35% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"            ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",(timeadjust < 0) ? '-':'+',interval(abs(timeadjust),0,ENTITIES,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");

              /* ---->  Global aliases owner  <---- */
              if(Validchar(aliases)) sprintf(scratch_return_string,"%s(#%d)",getname(aliases),aliases);
                 else strcpy(scratch_return_string,"Not set");
              output(p,player,2,1,30,"%s"ANSI_UNDERLINE"Global"ANSI_LGREEN" aliases:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=35% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"             ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",scratch_return_string,IsHtml(p) ? "\016</TD></TR>\016":"\n");

              /* ---->  Character/object maintenance  <---- */
              if(maint_morons) sprintf(scratch_return_string,"%d",maint_morons);
                 else strcpy(scratch_return_string,"Off");
              if(maint_newbies) sprintf(scratch_return_string + 300,"%d",maint_newbies);
                 else strcpy(scratch_return_string + 300,"Off");
              if(maint_mortals) sprintf(scratch_return_string + 100,"%d",maint_mortals);
                 else strcpy(scratch_return_string + 100,"Off");
              if(maint_builders) sprintf(scratch_return_string + 200,"%d",maint_builders);
                 else strcpy(scratch_return_string + 200,"Off");
              if(maint_objects) sprintf(scratch_return_string + 400,"%d",maint_objects);
                 else strcpy(scratch_return_string + 400,"Off");
              if(maint_junk) sprintf(scratch_return_string + 500,"%d",maint_junk);
                 else strcpy(scratch_return_string + 500,"Off");
              output(p,player,2,1,30,"%s"ANSI_UNDERLINE"Maintenance"ANSI_LGREEN":%s"ANSI_LWHITE"%s"ANSI_LGREEN", \016&nbsp;\016"ANSI_LYELLOW""ANSI_UNDERLINE"\016<B><I>\016Owner\016</I></B>\016"ANSI_LGREEN"\016<B><I>\016:\016</I></B> &nbsp;\016 "ANSI_LWHITE"%s(#%d)"ANSI_LGREEN".\n"
                    ANSI_LYELLOW""ANSI_UNDERLINE"\016<B><I>\016Morons\016</I></B>\016"ANSI_LGREEN"\016<B><I>\016:\016</I></B> &nbsp;\016 "ANSI_LWHITE"%s"ANSI_LGREEN", \016&nbsp;\016"ANSI_LYELLOW""ANSI_UNDERLINE"\016<B><I>\016Newbies\016</I></B>\016"ANSI_LGREEN"\016<B><I>\016:\016</I></B> &nbsp;\016 "ANSI_LWHITE"%s"ANSI_LGREEN", \016&nbsp;\016"ANSI_LYELLOW""ANSI_UNDERLINE"\016<B><I>\016Mortals\016</I></B>\016"ANSI_LGREEN"\016<B><I>\016:\016</I></B> &nbsp;\016 "ANSI_LWHITE"%s"ANSI_LGREEN", \016&nbsp;\016"ANSI_LYELLOW""ANSI_UNDERLINE"\016<B><I>\016Builders\016</I></B>\016"ANSI_LGREEN"\016<B><I>\016:\016</I></B> &nbsp;\016 "ANSI_LWHITE"%s"ANSI_LGREEN", \016&nbsp;\016"ANSI_LYELLOW""ANSI_UNDERLINE"\016<B><I>\016Objects\016</I></B>\016"ANSI_LGREEN"\016<B><I>\016:\016</I></B> &nbsp;\016 "ANSI_LWHITE"%s"ANSI_LGREEN", \016&nbsp;\016"ANSI_LYELLOW""ANSI_UNDERLINE"\016<B><I>\016Junk\016</I></B>\016"ANSI_LGREEN"\016<B><I>\016:\016</I></B> &nbsp;\016 "ANSI_LWHITE"%s"ANSI_LGREEN".%s",
                    IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT VALIGN=TOP WIDTH=35% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"                ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",(maintenance) ? "On":"Off",getname(maint_owner),maint_owner,scratch_return_string,scratch_return_string + 300,scratch_return_string + 100,scratch_return_string + 200,scratch_return_string + 400,scratch_return_string + 500,IsHtml(p) ? "\016</TD></TR>\016":"\n");
              if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,1,'-','='));
                 else output(p,player,1,2,0,"</TABLE><BR>");
              html_anti_reverse(p,0);
              setreturn(OK,COMMAND_SUCC);
	   } else if(string_prefix("options",gettextfield(1,' ',arg1,0,scratch_return_string))) {

              /* ---->  Display/change server options  <---- */
              for(; *arg1 && (*arg1 != ' '); arg1++);
              for(; *arg1 && (*arg1 == ' '); arg1++);

              if(Blank(arg1)) {
                 if(Blank(arg2)) {

                    /* ---->  Display current server options  <---- */
                    option_options(getdsc(player),1);
                    setreturn(OK,COMMAND_SUCC);
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the name of which server option you would like to change (Type '"ANSI_LYELLOW"@admin options"ANSI_LGREEN"' for a list.)");
	      } else if(Level1(Owner(player))) {

                 /* ---->  Change server option  <---- */
                 if(Blank(arg2)) {
                    char *ptr;

                    for(; *arg1 && (*arg1 == ' '); arg1++);
                    for(ptr = (char *) arg1; *ptr && (*ptr != ' '); ptr++);
                    if(*ptr) for(*ptr = '\0', ptr++; *ptr && (*ptr == ' '); ptr++);
                    arg2 = ptr;
		 }

                 /* ---->  Strip non-printable codes from option and value  <---- */
                 ansi_code_filter(arg1,arg1,1);
                 ansi_code_filter(arg2,arg2,1);

                 if(option_match(player,"OPTIONS",arg1,arg2,tcz_short_name,1,OPTION_ADMINOPTIONS))
                    setreturn(OK,COMMAND_SUCC);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Deities and the Supreme Being can change %s server options.",tcz_short_name);
	   } else {

              /* ---->  Set/change Admin. option  <---- */
              if(Blank(arg2)) {
                 char *ptr;

                 for(; *arg1 && (*arg1 == ' '); arg1++);
                 for(ptr = (char *) arg1; *ptr && (*ptr != ' '); ptr++);
                 if(*ptr) for(*ptr = '\0', ptr++; *ptr && (*ptr == ' '); ptr++);
                 arg2 = ptr;
	      }

	      if(string_prefix("per cycle",arg1) || string_prefix("dumped per cycle",arg1) || string_prefix("data dumped per cycle",arg1)) {

                 /* ---->  Set data dumped per cycle  <---- */
                 if(Level1(Owner(player))) {
                    if(!Blank(arg2)) {
                       if((numb = (atol(arg2) * KB)) >= 16384) {
                          dumpdatasize = numb;
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Data dumped per database dump cycle set to "ANSI_LWHITE"%dkb"ANSI_LGREEN".",numb / KB);
                          setreturn(OK,COMMAND_SUCC);
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, data dumped per database dump cycle must be "ANSI_LWHITE"16kb"ANSI_LGREEN" or greater.");
		    } else errorlevel = 4;
		 } else errorlevel = 1;
                 if(errorlevel) adminoption = "data dumped per database dump cycle";
	      } else if(string_prefix("dumping interval",arg1) || string_prefix("database dumping interval",arg1) || string_prefix("interval",arg1)) {

                 /* ---->  Set database dumping interval (In minutes)  <---- */
                 if(Level2(Owner(player))) {
                    if(!Blank(arg2)) {
                       if(((numb = atol(arg2)) >= 5) && (numb <= 120)) {
                          numb *= MINUTE, dumpinterval = numb;
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Database dumping interval set to "ANSI_LWHITE"%s"ANSI_LGREEN".",interval(numb,numb,ENTITIES,0));
                          setreturn(OK,COMMAND_SUCC);
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the database dumping interval can only be set to a value between "ANSI_LWHITE"5"ANSI_LGREEN" and "ANSI_LWHITE"120"ANSI_LGREEN" minutes.");
		    } else errorlevel = 4;
		 } else errorlevel = 2;
                 if(errorlevel) adminoption = "database dumping interval";
	      } else if(string_prefix("cycle interval",arg1) || string_prefix("dump cycle interval",arg1)) {

                 /* ---->  Set dump cycle interval  <---- */
                 if(Level1(Owner(player))) {
                    if(!Blank(arg2)) {
                       if((numb = atol(arg2)) >= 0) {
                          if(numb > 255) numb = 255;
                          dumpcycle = numb;
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Database dump cycle interval set to "ANSI_LWHITE"%d"ANSI_LGREEN" second%s.",numb,Plural(numb));
                          setreturn(OK,COMMAND_SUCC);
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the database dump cycle interval must be a positive value.");
		    } else errorlevel = 4;
		 } else errorlevel = 1;
                 if(errorlevel) adminoption = "database dump cycle interval";
	      } else if(string_prefix("limit maximum connections",arg1) || string_prefix("maximum connections",arg1)) {

                 /* ---->  Restrict maximum number of connections (Simultaneously)  <---- */
                 if(Level2(Owner(player))) {
                    if(!Blank(arg2)) {
                       if((numb = string_prefix("yes",arg2) || string_prefix("on",arg2)) || string_prefix("no",arg2) || string_prefix("off",arg2)) {
                          limit_connections = numb;
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"The maximum number of connections (Simultaneously) are %s restricted.",(numb) ? "now":"no-longer");
                          setreturn(OK,COMMAND_SUCC);
	   	       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LWHITE"yes"ANSI_LGREEN"' or '"ANSI_LWHITE"no"ANSI_LGREEN"'.");
		    } else errorlevel = 4;
		 } else errorlevel = 2;
                 if(errorlevel) adminoption = "limit maximum number of connections allowed (Simultaneously)";
	      } else if(string_prefix("allowed",arg1)) {

                 /* ---->  Connections allowed  <---- */
                 if(Level2(Owner(player))) {
                    if(!Blank(arg2)) {
                       numb = atol(arg2);
                       if((Level1(Owner(player)) && (numb >= 0)) || (Level2(Owner(player)) && (numb > 0))) {
                          if(numb > 32767) numb = 32767;
                          allowed = numb;
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Maximum number of connections allowed (Simultaneously) set to "ANSI_LWHITE"%d"ANSI_LGREEN".",allowed);
                          setreturn(OK,COMMAND_SUCC);
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only set the maximum number of connections allowed (Simultaneously) to a positive value.");
		    } else errorlevel = 4;
		 } else errorlevel = 2;
                 if(errorlevel) adminoption = "maximum number of connections allowed (Simultaneously)";
	      } else if(string_prefix("create",arg1) || string_prefix("creations",arg1) || string_prefix("usercreations",arg1) || string_prefix("allowusercreations",arg1) || string_prefix("user creations",arg1) || string_prefix("allow user creations",arg1)) {

                 /* ---->  Allow user creations  <---- */
                 if(Level2(Owner(player))) {
                    if(!Blank(arg2)) {
                       if(string_prefix("yes",arg2) || string_prefix("on",arg2)) numb = 1;
                          else if(string_prefix("guests",arg2) || string_prefix("guestsonly",arg2) || string_prefix("guests only",arg2)) numb = 2;
                             else if(string_prefix("no",arg2) || string_prefix("off",arg2)) numb = 0;
                                else numb = -1;

                       if(numb >= 0) {
                          creations = numb;
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"User creations are %s allowed%s.",(numb) ? "now":"no-longer",(numb == 2) ? ", but are restricted to guest characters only":"");
                          setreturn(OK,COMMAND_SUCC);
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LWHITE"yes"ANSI_LGREEN"', '"ANSI_LWHITE"guests"ANSI_LGREEN"' or '"ANSI_LWHITE"no"ANSI_LGREEN"'.");
		    } else errorlevel = 4;
		 } else errorlevel = 2;
                 if(errorlevel) adminoption = "allow user creations";
	      } else if(string_prefix("connections",arg1) || string_prefix("userconnections",arg1) || string_prefix("allowuserconnections",arg1) || string_prefix("user connections",arg1) || string_prefix("allow user connections",arg1)) {

                 /* ---->  Allow user connections  <---- */
                 if(Level1(Owner(player))) {
                    if(!Blank(arg2)) {
                       if(string_prefix("yes",arg2) || string_prefix("on",arg2)) numb = 1;
                          else if(string_prefix("no",arg2) || string_prefix("off",arg2)) numb = 0;
                             else numb = -1;

                       if(numb >= 0) {
                          connections = numb;
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"User connections by non-administrators are %s allowed.",(numb) ? "now":"no-longer");
                          setreturn(OK,COMMAND_SUCC);
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LWHITE"yes"ANSI_LGREEN"' or '"ANSI_LWHITE"no"ANSI_LGREEN"'.");
		    } else errorlevel = 4;
		 } else errorlevel = 1;
                 if(errorlevel) adminoption = "allow user connections";
	      } else if(((numb = 1) && string_prefix("bbs access room",arg1)) || ((numb = 1) && string_prefix("bbs",arg1)) || ((numb = 1) && string_prefix("bbs room",arg1)) || ((numb = 1) && string_prefix("bbsroom",arg1)) || ((numb = 1) && string_prefix("bbs access",arg1)) || ((numb = 1) && string_prefix("room",arg1)) || ((numb = 1) && string_prefix("access",arg1)) || ((numb = 1) && string_prefix("access room",arg1)) ||
                        ((numb = 2) && string_prefix("mail access room",arg1)) || ((numb = 2) && string_prefix("mail",arg1)) || ((numb = 2) && string_prefix("mail room",arg1)) || ((numb = 2) && string_prefix("mailroom",arg1)) || ((numb = 2) && string_prefix("mail access",arg1)) ||
                        ((numb = 3) && string_prefix("bank access room",arg1)) || ((numb = 3) && string_prefix("bank",arg1)) || ((numb = 3) && string_prefix("bank room",arg1)) || ((numb = 3) && string_prefix("bankroom",arg1)) || ((numb = 3) && string_prefix("bank access",arg1)) ||
                        ((numb = 4) && string_prefix("home rooms container",arg1)) || ((numb = 4) && string_prefix("homes",arg1)) || ((numb = 4) && string_prefix("home rooms",arg1)) || ((numb = 4) && string_prefix("homerooms",arg1)) || ((numb = 4) && string_prefix("home rooms container",arg1)) || ((numb = 4) && string_prefix("homeroomscontainer",arg1))) {

                 dbref room = NOTHING;

                 /* ---->  Set BBS and bank access rooms  <---- */
                 switch(numb) {
                        case 1:
			     sprintf(scratch_return_string,"%s BBS access room",tcz_full_name);
                             adminoption = scratch_return_string;
                             break;
                        case 2:
			     sprintf(scratch_return_string,"%s mail access room",tcz_short_name);
                             adminoption = scratch_return_string;
                             break;
                        case 3:
			     sprintf(scratch_return_string,"Bank of %s access room",tcz_short_name);
                             adminoption = scratch_return_string;
                             break;
                        case 4:
			     strcpy(scratch_return_string,"Home rooms container");
                             adminoption = scratch_return_string;
                             break;
		 }

                 if(Level1(Owner(player))) {
                    if(!Blank(arg2)) {
                       room = match_object(player,player,NOTHING,arg2,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,MATCH_OPTION_DEFAULT,SEARCH_ALL,NULL,0);
                       if(!Valid(room)) return;

                       if((Typeof(room) == TYPE_ROOM) || (Typeof(room) == TYPE_THING)) {
                          output(getdsc(player),player,0,1,0,ANSI_LWHITE"%s"ANSI_LGREEN" set to "ANSI_LWHITE"%s"ANSI_LGREEN".",adminoption,unparse_object(player,room,0));
                          setreturn(OK,COMMAND_SUCC);
		       } else {
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, "ANSI_LWHITE"%s"ANSI_LGREEN" must either be a room or thing.",adminoption);
                          return;
		       }
		    } else {
                       output(getdsc(player),player,0,1,0,ANSI_LWHITE"%s"ANSI_LGREEN" reset.",adminoption);
                       setreturn(OK,COMMAND_SUCC);
		    }
		 } else errorlevel = 1;

                 if(!errorlevel) {
                    switch(numb) {
                           case 1:
                                bbsroom = room; 
                                break;
                           case 2:
                                mailroom = room;
                                break;
                           case 3:
                                bankroom = room; 
                                break;
                           case 4:
                                homerooms = room; 
                                break;
		    }
		 }
	      } else if(string_prefix("timeadjustment",arg1) || string_prefix("adjustment",arg1) || string_prefix("time adjustment",arg1)) {

                 /* ---->  Server time adjustment  <---- */
                 if(!Blank(arg2)) {
                    unsigned char negative = 0;
                    int           adjustment;

                    for(; *arg2 && (*arg2 == ' '); arg2++);
                    if(*arg2 && (*arg2 == '-')) negative = 1;
                    for(; *arg2 && ((*arg2 == '-') || (*arg2 == '+')); arg2++);
                    for(; *arg2 && (*arg2 == ' '); arg2++);

                    adjustment = parse_time((char *) arg2);
                    if(adjustment) {
                       if(abs(adjustment) <= WEEK) {
                          if(negative) adjustment = 0 - adjustment;
                          admin_time_adjust(adjustment);
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"\nServer time adjusted by "ANSI_LWHITE"(%c) %s"ANSI_LGREEN".",(adjustment < 0) ? '-':'+',interval(abs(adjustment),0,ENTITIES,0));
                          look_date(player,NULL,NULL,NULL,NULL,0,0);
                          setreturn(OK,COMMAND_SUCC);
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that server time adjustment is too large.");
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the amount you'd like to adjust the server time by (E.g:  '"ANSI_LWHITE"@admin time = -10 minutes"ANSI_LGREEN"'.)");
		 } else errorlevel = 4;
                 if(errorlevel) adminoption = "server time adjustment";
	      } else if(string_prefix("global",arg1) || string_prefix("aliases",arg1) || string_prefix("global aliases",arg1) || string_prefix("globalaliases",arg1)) {

                 /* ---->  Set global aliases owner  <---- */
                 if(Root(player)) {
                    if(!Blank(arg2)) {
                       dbref character;
  
                       if((character = lookup_character(player,arg2,1)) != NOTHING) {
                          aliases = character;
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN"'s aliases are now global.",Article(character,UPPER,DEFINITE),getcname(NOTHING,character,0,0));
                          setreturn(OK,COMMAND_SUCC);
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg2);
		    } else {
                       aliases = NOTHING;
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Global aliases owner reset.");
                       setreturn(OK,COMMAND_SUCC);
		    }
		 } else errorlevel = 5;
                 if(errorlevel) adminoption = "global aliases owner";
	      } else if(string_prefix("maintenance",arg1) || string_prefix("character maintenance",arg1) || (string_prefix("object maintenance",arg1) && (strlen(arg1) > 7)) || string_prefix("database maintenance",arg1)) {

                 /* ---->  Database maintenance  <---- */
                 if(Level1(Owner(player))) {
                    if(!Blank(arg2)) {
                       maintenance = (string_prefix("yes",arg2) || string_prefix("on",arg2)) ? 1:0;
                       if(maintenance) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Automatic database maintenance turned "ANSI_LWHITE"on"ANSI_LGREEN".");
                          else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Automatic database maintenance turned "ANSI_LWHITE"off"ANSI_LGREEN".");
                       setreturn(OK,COMMAND_SUCC);
		    } else errorlevel = 4;
		 } else errorlevel = 1;
                 if(errorlevel) adminoption = "automatic database maintenance";
	      } else if(string_prefix("owner",arg1) || string_prefix("chown",arg1) || string_prefix("maintenance owner",arg1) || string_prefix("maintenance chown",arg1)) {

                 /* ---->  Set character maintenance owner  <---- */
                 if(Level1(Owner(player))) {
                    if(!Blank(arg2)) {
                       dbref character;
  
                       if((character = lookup_character(player,arg2,1)) != NOTHING) {
                          if(can_write_to(player,character,0)) {
                             maint_owner = character;
		             output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" will now take over the ownership of objects owned by characters destroyed by character maintenance and the '"ANSI_LYELLOW"@destroy"ANSI_LGREEN"' command.",Article(character,UPPER,DEFINITE),getcname(NOTHING,character,0,0));
                             setreturn(OK,COMMAND_SUCC);
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only set who will take over the ownership of objects owned by characters destroyed by character maintenance and the '"ANSI_LYELLOW"@destroy"ANSI_LGREEN"' command to someone who's of a lower level than yourself.");
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg2);
		    } else errorlevel = 4;
		 } else errorlevel = 1;
                 if(errorlevel) adminoption = "character maintenance owner";
	      } else if((string_prefix("morons",arg1)) ? ((mtype = 1) && (mlimit = 7) && (arg1 = "Moron character")):0 || (string_prefix("Newbie character",arg1)) ? ((mtype = 2) && (mlimit = 7) && (arg1 = "newbie")):0 || (string_prefix("mortals",arg1)) ? ((mtype = 3) && (mlimit = 14) && (arg1 = "Mortal character")):0 || (string_prefix("builders",arg1)) ? ((mtype = 4) && (mlimit = 28) && (arg1 = "Builder character")):0 || (string_prefix("objects",arg1)) ? ((mtype = 5) && (mlimit = 28) && (arg1 = "Object")):0 || (string_prefix("junkobjects",arg1) || string_prefix("junkedobjects",arg1)) ? ((mtype = 6) && (mlimit = 7) && (arg1 = "Junk object")):0) {

                 /* ---->  Set maintenance times for Morons, Newbies, Mortals, Builders, Objects and Junk objects  <---- */
                 if(Level1(Owner(player))) {
                    if(!Blank(arg2)) {
                       if(!(((numb = atol(arg2)) != 0) && (numb < mlimit))) {
                          if(numb > 999) numb = 999;
                          switch(mtype) {
                                 case 1:
                                      maint_morons = numb;
                                      break;
                                 case 2:
                                      maint_newbies = numb;
                                      break;
                                 case 3:
                                      maint_mortals = numb;
                                      break;
                                 case 4:
                                      maint_builders = numb;
                                      break;
                                 case 5:
                                      maint_objects = numb;
                                      break;
                                 case 6:
                                      maint_junk = numb;
                                      break;
			  }
                          if(numb) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s maintenance set to "ANSI_LWHITE"%d"ANSI_LGREEN" day%s.",arg1,numb,Plural(numb));
                             else output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s maintenance turned "ANSI_LWHITE"off"ANSI_LGREEN".",arg1);
                          setreturn(OK,COMMAND_SUCC);
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s maintenance can't be set to less than "ANSI_LWHITE"%d"ANSI_LGREEN" day%s.",arg1,mlimit,Plural(mlimit));
		    } else errorlevel = 4;
		 } else errorlevel = 1;

                 if(errorlevel) switch(mtype) {
                    case 1:
                         adminoption = "Moron character maintenance";
                         break;
                    case 2:
                         adminoption = "Newbie character maintenance";
                         break;
                    case 3:
                         adminoption = "Mortal character maintenance";
                         break;
                    case 4:
                         adminoption = "Builder character maintenance";
                         break;
                    case 5:
                         adminoption = "Object maintenance";
                         break;
                    case 6:
                         adminoption = "Junk object maintenance";
                         break;
		 }
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"%s"ANSI_LGREEN"' is an unknown administrative option.",arg1);

              switch(errorlevel) {
                     case 1:
                     case 2:
                     case 3:
		          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only %s may change "ANSI_LWHITE"%s"ANSI_LGREEN".",(errorlevel == 3) ? "Wizards/Druids and above":(errorlevel == 2) ? "Elder Wizards/Druids and above":"Deities and the Supreme Being",adminoption);
                          break;
                     case 4:
		          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the new setting for "ANSI_LWHITE"%s"ANSI_LGREEN".",adminoption);
                          break;
                     case 5:
		          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only the Supreme Being may change "ANSI_LWHITE"%s"ANSI_LGREEN".",adminoption);
                          break;
	      }
	   }
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may view/change %s's server administrative options.",tcz_short_name);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s's server administrative options can't be viewed/changed from within a compound command.",tcz_short_name);
}

/* ---->  Display list of all current administrators  <---- */
void admin_list_all(struct descriptor_data *p,dbref player)
{
     int   twidth = output_terminal_width(player),admin = 0,retired = 0,experienced = 0,assistants = 0;
     char  buffer[BUFFER_LEN],buffer2[BUFFER_LEN];
     int   count,header = 0;
     dbref loop;

     /* ---->  List all Admin (In short list)  <---- */
     html_anti_reverse(p,1);
     if(Validchar(player) && !in_command && p && !IsHtml(p) && !p->pager && More(player)) pager_init(p);
     if(IsHtml(p)) {
	output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_GREY">",(in_command || !Validchar(player)) ? "":"<BR>");
	output(p,player,2,1,0,"\016<TR ALIGN=CENTER><TH BGCOLOR="HTML_TABLE_CYAN"><FONT COLOR="HTML_LCYAN" SIZE=5><I>Administrators, Experienced Builders and Assistants of %s...</I></FONT></TH></TR>\016",tcz_full_name);
     } else {
	output(p,player,0,1,1,ANSI_LCYAN"\n Administrators, Experienced Builders and Assistants of %s...",tcz_full_name);
	output(p,player,0,1,0,separator(twidth,0,'-','='));
     }

     /* ---->  Deities  <---- */
     for(loop = 0, count = 0; loop < db_top; loop++)
	 if((Typeof(loop) == TYPE_CHARACTER) && !Being(loop) && Level1(loop) && (Controller(loop) == loop))
	    count++;

     if(count) {
	admin += count;
	if(header++ && !IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'=','=')), header = 1;
	output(p,player,2,1,1,"%s"DEITY_COLOUR"Deities ("ANSI_LWHITE"%d"DEITY_COLOUR")...%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH BGCOLOR="HTML_TABLE_MAGENTA"><FONT SIZE=4><I>\016":" ",count,IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
	if(header++ && !IsHtml(p)) output(p,player,0,1,0,ANSI_DMAGENTA"%s",separator(twidth,0,'-','-') + strlen(ANSI_DCYAN)), header = 1;
	output_columns(p,player,NULL,NULL,twidth,1,20,2,0,1,FIRST,0,NULL,scratch_return_string);

	for(loop = 0, count = 0; loop < db_top; loop++)
	    if((Typeof(loop) == TYPE_CHARACTER) && !Being(loop) && Level1(loop) && (Controller(loop) == loop)) {
	       if(IsHtml(p)) sprintf(scratch_buffer,"\016<A HREF=\"%sNAME=*%s&\" TARGET=_blank TITLE=\"Click to scan user...\">\016"DEITY_COLOUR"%s\016</A>\016",html_server_url(p,0,1,"scan"),html_encode(getname(loop),buffer,NULL,sizeof(buffer)),getname_prefix(loop,20,buffer2));
		  else strcpy(scratch_buffer,getname_prefix(loop,20,buffer));
	       output_columns(p,player,scratch_buffer,IsHtml(p) ? NULL:DEITY_COLOUR,0,1,20,2,0,1,DEFAULT,0,NULL,scratch_return_string);
	    }

	output_columns(p,player,NULL,NULL,0,1,20,2,0,1,LAST,0,NULL,scratch_return_string);
     }

     /* ---->  Elder Wizards  <---- */
     for(loop = 0, count = 0; loop < db_top; loop++)
	 if((Typeof(loop) == TYPE_CHARACTER) && !Being(loop) && Level2(loop) && !Level1(loop) && !Druid(loop) && (Controller(loop) == loop))
	    count++;

     if(count) {
	admin += count;
	if(header++ && !IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'=','='));
	output(p,player,2,1,1,"%s"ELDER_COLOUR"Elder Wizards ("ANSI_LWHITE"%d"ELDER_COLOUR")...%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH BGCOLOR="HTML_TABLE_GREEN"><FONT SIZE=4><I>\016":" ",count,IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
	if(header++ && !IsHtml(p)) output(p,player,0,1,0,ANSI_DGREEN"%s",separator(twidth,0,'-','-') + strlen(ANSI_DCYAN)), header = 1;
	output_columns(p,player,NULL,NULL,twidth,1,20,2,0,1,FIRST,0,NULL,scratch_return_string);

	for(loop = 0, count = 0; loop < db_top; loop++)
	    if((Typeof(loop) == TYPE_CHARACTER) && !Being(loop) && Level2(loop) && !Level1(loop) && !Druid(loop) && (Controller(loop) == loop)) {
	       if(IsHtml(p)) sprintf(scratch_buffer,"\016<A HREF=\"%sNAME=*%s&\" TARGET=_blank TITLE=\"Click to scan user...\">\016"ELDER_COLOUR"%s\016</A>\016",html_server_url(p,0,1,"scan"),html_encode(getname(loop),buffer,NULL,sizeof(buffer)),getname_prefix(loop,20,buffer2));
		  else strcpy(scratch_buffer,getname_prefix(loop,20,buffer));
	       output_columns(p,player,scratch_buffer,IsHtml(p) ? NULL:ELDER_COLOUR,0,1,20,2,0,1,DEFAULT,0,NULL,scratch_return_string);
	    }

	output_columns(p,player,NULL,NULL,0,1,20,2,0,1,LAST,0,NULL,scratch_return_string);
     }

     /* ---->  Elder Druids  <---- */
     for(loop = 0, count = 0; loop < db_top; loop++)
	 if((Typeof(loop) == TYPE_CHARACTER) && !Being(loop) && Level2(loop) && !Level1(loop) && Druid(loop) && (Controller(loop) == loop))
	    count++;

     if(count) {
	admin += count;
	if(header++ && !IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'=','='));
	output(p,player,2,1,1,"%s"ELDER_DRUID_COLOUR"Elder Druids ("ANSI_LWHITE"%d"ELDER_DRUID_COLOUR")...%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH BGCOLOR="HTML_TABLE_GREEN"><FONT SIZE=4><I>\016":" ",count,IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
	if(header++ && !IsHtml(p)) output(p,player,0,1,0,ANSI_LGREEN"%s",separator(twidth,0,'-','-') + strlen(ANSI_DCYAN)), header = 1;
	output_columns(p,player,NULL,NULL,twidth,1,20,2,0,1,FIRST,0,NULL,scratch_return_string);

	for(loop = 0, count = 0; loop < db_top; loop++)
	    if((Typeof(loop) == TYPE_CHARACTER) && !Being(loop) && Level2(loop) && !Level1(loop) && Druid(loop) && (Controller(loop) == loop)) {
	       if(IsHtml(p)) sprintf(scratch_buffer,"\016<A HREF=\"%sNAME=*%s&\" TARGET=_blank TITLE=\"Click to scan user...\">\016"ELDER_DRUID_COLOUR"%s\016</A>\016",html_server_url(p,0,1,"scan"),html_encode(getname(loop),buffer,NULL,sizeof(buffer)),getname_prefix(loop,20,buffer2));
		  else strcpy(scratch_buffer,getname_prefix(loop,20,buffer));
	       output_columns(p,player,scratch_buffer,IsHtml(p) ? NULL:ELDER_DRUID_COLOUR,0,1,20,2,0,1,DEFAULT,0,NULL,scratch_return_string);
	    }

	output_columns(p,player,NULL,NULL,0,1,20,2,0,1,LAST,0,NULL,scratch_return_string);
     }

     /* ---->  Wizards  <---- */
     for(loop = 0, count = 0; loop < db_top; loop++)
	 if((Typeof(loop) == TYPE_CHARACTER) && !Being(loop) && Level3(loop) && !Level2(loop) && !Druid(loop) && (Controller(loop) == loop))
	    count++;

     if(count) {
	admin += count;
	if(header++ && !IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'=','='));
	output(p,player,2,1,1,"%s"WIZARD_COLOUR"Wizards ("ANSI_LWHITE"%d"WIZARD_COLOUR")...%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH BGCOLOR="HTML_TABLE_CYAN"><FONT SIZE=4><I>\016":" ",count,IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
	if(header++ && !IsHtml(p)) output(p,player,0,1,0,ANSI_DCYAN"%s",separator(twidth,0,'-','-') + strlen(ANSI_DCYAN)), header = 1;
	output_columns(p,player,NULL,NULL,twidth,1,20,2,0,1,FIRST,0,NULL,scratch_return_string);

	for(loop = 0, count = 0; loop < db_top; loop++)
	    if((Typeof(loop) == TYPE_CHARACTER) && !Being(loop) && Level3(loop) && !Level2(loop) && !Druid(loop) && (Controller(loop) == loop)) {
	       if(IsHtml(p)) sprintf(scratch_buffer,"\016<A HREF=\"%sNAME=*%s&\" TARGET=_blank TITLE=\"Click to scan user...\">\016"WIZARD_COLOUR"%s\016</A>\016",html_server_url(p,0,1,"scan"),html_encode(getname(loop),buffer,NULL,sizeof(buffer)),getname_prefix(loop,20,buffer2));
		  else strcpy(scratch_buffer,getname_prefix(loop,20,buffer));
	       output_columns(p,player,scratch_buffer,IsHtml(p) ? NULL:WIZARD_COLOUR,0,1,20,2,0,1,DEFAULT,0,NULL,scratch_return_string);
	    }

	output_columns(p,player,NULL,NULL,0,1,20,2,0,1,LAST,0,NULL,scratch_return_string);
     }

     /* ---->  Druids  <---- */
     for(loop = 0, count = 0; loop < db_top; loop++)
	 if((Typeof(loop) == TYPE_CHARACTER) && !Being(loop) && Level3(loop) && !Level2(loop) && Druid(loop) && (Controller(loop) == loop))
	    count++;

     if(count) {
	admin += count;
	if(header++ && !IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'=','='));
	output(p,player,2,1,1,"%s"DRUID_COLOUR"Druids ("ANSI_LWHITE"%d"DRUID_COLOUR")...%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH BGCOLOR="HTML_TABLE_CYAN"><FONT SIZE=4><I>\016":" ",count,IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
	if(header++ && !IsHtml(p)) output(p,player,0,1,0,ANSI_LCYAN"%s",separator(twidth,0,'-','-') + strlen(ANSI_DCYAN)), header = 1;
	output_columns(p,player,NULL,NULL,twidth,1,20,2,0,1,FIRST,0,NULL,scratch_return_string);

	for(loop = 0, count = 0; loop < db_top; loop++)
	    if((Typeof(loop) == TYPE_CHARACTER) && !Being(loop) && Level3(loop) && !Level2(loop) && Druid(loop) && (Controller(loop) == loop)) {
	       if(IsHtml(p)) sprintf(scratch_buffer,"\016<A HREF=\"%sNAME=*%s&\" TARGET=_blank TITLE=\"Click to scan user...\">\016"DRUID_COLOUR"%s\016</A>\016",html_server_url(p,0,1,"scan"),html_encode(getname(loop),buffer,NULL,sizeof(buffer)),getname_prefix(loop,20,buffer2));
		  else strcpy(scratch_buffer,getname_prefix(loop,20,buffer));
	       output_columns(p,player,scratch_buffer,IsHtml(p) ? NULL:DRUID_COLOUR,0,1,20,2,0,1,DEFAULT,0,NULL,scratch_return_string);
	    }

	output_columns(p,player,NULL,NULL,0,1,20,2,0,1,LAST,0,NULL,scratch_return_string);
     }

     /* ---->  Apprentice Wizards  <---- */
     for(loop = 0, count = 0; loop < db_top; loop++)
	 if((Typeof(loop) == TYPE_CHARACTER) && !Being(loop) && Level4(loop) && !Level3(loop) && !Druid(loop) && (Controller(loop) == loop))
	    count++;

     if(count) {
	admin += count;
	if(header++ && !IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'=','='));
	output(p,player,2,1,1,"%s"APPRENTICE_COLOUR"Apprentice Wizards ("ANSI_LWHITE"%d"APPRENTICE_COLOUR")...%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH BGCOLOR="HTML_TABLE_YELLOW"><FONT SIZE=4><I>\016":" ",count,IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
	if(header++ && !IsHtml(p)) output(p,player,0,1,0,ANSI_DYELLOW"%s",separator(twidth,0,'-','-') + strlen(ANSI_DCYAN)), header = 1;
	output_columns(p,player,NULL,NULL,twidth,1,20,2,0,1,FIRST,0,NULL,scratch_return_string);

	for(loop = 0, count = 0; loop < db_top; loop++)
	    if((Typeof(loop) == TYPE_CHARACTER) && !Being(loop) && Level4(loop) && !Level3(loop) && !Druid(loop) && (Controller(loop) == loop)) {
	       if(IsHtml(p)) sprintf(scratch_buffer,"\016<A HREF=\"%sNAME=*%s&\" TARGET=_blank TITLE=\"Click to scan user...\">\016"APPRENTICE_COLOUR"%s\016</A>\016",html_server_url(p,0,1,"scan"),html_encode(getname(loop),buffer,NULL,sizeof(buffer)),getname_prefix(loop,20,buffer2));
		  else strcpy(scratch_buffer,getname_prefix(loop,20,buffer));
	       output_columns(p,player,scratch_buffer,IsHtml(p) ? NULL:APPRENTICE_COLOUR,0,1,20,2,0,1,DEFAULT,0,NULL,scratch_return_string);
	    }

	output_columns(p,player,NULL,NULL,0,1,20,2,0,1,LAST,0,NULL,scratch_return_string);
     }

     /* ---->  Apprentice Druids  <---- */
     for(loop = 0, count = 0; loop < db_top; loop++)
	 if((Typeof(loop) == TYPE_CHARACTER) && !Being(loop) && Level4(loop) && !Level3(loop) && Druid(loop) && (Controller(loop) == loop))
	    count++;

     if(count) {
	admin += count;
	if(header++ && !IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'=','='));
	output(p,player,2,1,1,"%s"APPRENTICE_DRUID_COLOUR"Apprentice Druids ("ANSI_LWHITE"%d"APPRENTICE_DRUID_COLOUR")...%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH BGCOLOR="HTML_TABLE_YELLOW"><FONT SIZE=4><I>\016":" ",count,IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
	if(header++ && !IsHtml(p)) output(p,player,0,1,0,ANSI_LYELLOW"%s",separator(twidth,0,'-','-') + strlen(ANSI_DCYAN)), header = 1;
	output_columns(p,player,NULL,NULL,twidth,1,20,2,0,1,FIRST,0,NULL,scratch_return_string);

	for(loop = 0, count = 0; loop < db_top; loop++)
	    if((Typeof(loop) == TYPE_CHARACTER) && !Being(loop) && Level4(loop) && !Level3(loop) && Druid(loop) && (Controller(loop) == loop)) {
	       if(IsHtml(p)) sprintf(scratch_buffer,"\016<A HREF=\"%sNAME=*%s&\" TARGET=_blank TITLE=\"Click to scan user...\">\016"APPRENTICE_DRUID_COLOUR"%s\016</A>\016",html_server_url(p,0,1,"scan"),html_encode(getname(loop),buffer,NULL,sizeof(buffer)),getname_prefix(loop,20,buffer2));
		  else strcpy(scratch_buffer,getname_prefix(loop,20,buffer));
	       output_columns(p,player,scratch_buffer,IsHtml(p) ? NULL:APPRENTICE_DRUID_COLOUR,0,1,20,2,0,1,DEFAULT,0,NULL,scratch_return_string);
	    }

	output_columns(p,player,NULL,NULL,0,1,20,2,0,1,LAST,0,NULL,scratch_return_string);
     }

     /* ---->  Retired Wizards  <---- */
     for(loop = 0, count = 0; loop < db_top; loop++)
	 if((Typeof(loop) == TYPE_CHARACTER) && !Being(loop) && RetiredWizard(loop) && (Controller(loop) == loop))
	    count++;

     if(count) {
	retired += count;
	if(header++ && !IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'=','='));
	output(p,player,2,1,1,"%s"RETIRED_COLOUR"Retired Wizards ("ANSI_LWHITE"%d"RETIRED_COLOUR")...%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH BGCOLOR="HTML_TABLE_RED"><FONT SIZE=4><I>\016":" ",count,IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
	if(header++ && !IsHtml(p)) output(p,player,0,1,0,ANSI_DRED"%s",separator(twidth,0,'-','-') + strlen(ANSI_DCYAN)), header = 1;
	output_columns(p,player,NULL,NULL,twidth,1,20,2,0,1,FIRST,0,NULL,scratch_return_string);

	for(loop = 0, count = 0; loop < db_top; loop++)
	    if((Typeof(loop) == TYPE_CHARACTER) && !Being(loop) && RetiredWizard(loop) && (Controller(loop) == loop)) {
	       if(IsHtml(p)) sprintf(scratch_buffer,"\016<A HREF=\"%sNAME=*%s&\" TARGET=_blank TITLE=\"Click to scan user...\">\016"RETIRED_COLOUR"%s\016</A>\016",html_server_url(p,0,1,"scan"),html_encode(getname(loop),buffer,NULL,sizeof(buffer)),getname_prefix(loop,20,buffer2));
		  else strcpy(scratch_buffer,getname_prefix(loop,20,buffer));
	       output_columns(p,player,scratch_buffer,IsHtml(p) ? NULL:RETIRED_COLOUR,0,1,20,2,0,1,DEFAULT,0,NULL,scratch_return_string);
	    }

	output_columns(p,player,NULL,NULL,0,1,20,2,0,1,LAST,0,NULL,scratch_return_string);
     }

     /* ---->  Retired Druids  <---- */
     for(loop = 0, count = 0; loop < db_top; loop++)
	 if((Typeof(loop) == TYPE_CHARACTER) && !Being(loop) && RetiredDruid(loop) && (Controller(loop) == loop))
	    count++;

     if(count) {
	retired += count;
	if(header++ && !IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'=','='));
	output(p,player,2,1,1,"%s"RETIRED_DRUID_COLOUR"Retired Druids ("ANSI_LWHITE"%d"RETIRED_DRUID_COLOUR")...%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH BGCOLOR="HTML_TABLE_RED"><FONT SIZE=4><I>\016":" ",count,IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
	if(header++ && !IsHtml(p)) output(p,player,0,1,0,ANSI_LRED"%s",separator(twidth,0,'-','-') + strlen(ANSI_DCYAN)), header = 1;
	output_columns(p,player,NULL,NULL,twidth,1,20,2,0,1,FIRST,0,NULL,scratch_return_string);

	for(loop = 0, count = 0; loop < db_top; loop++)
	    if((Typeof(loop) == TYPE_CHARACTER) && !Being(loop) && RetiredDruid(loop) && (Controller(loop) == loop)) {
	       if(IsHtml(p)) sprintf(scratch_buffer,"\016<A HREF=\"%sNAME=*%s&\" TARGET=_blank TITLE=\"Click to scan user...\">\016"RETIRED_DRUID_COLOUR"%s\016</A>\016",html_server_url(p,0,1,"scan"),html_encode(getname(loop),buffer,NULL,sizeof(buffer)),getname_prefix(loop,20,buffer2));
		  else strcpy(scratch_buffer,getname_prefix(loop,20,buffer));
	       output_columns(p,player,scratch_buffer,IsHtml(p) ? NULL:RETIRED_DRUID_COLOUR,0,1,20,2,0,1,DEFAULT,0,NULL,scratch_return_string);
	    }

	output_columns(p,player,NULL,NULL,0,1,20,2,0,1,LAST,0,NULL,scratch_return_string);
     }

     /* ---->  Experienced Builder  <---- */
     for(loop = 0, count = 0; loop < db_top; loop++)
	 if((Typeof(loop) == TYPE_CHARACTER) && !Being(loop) && Experienced(loop) && !Level4(loop) && !Retired(loop) && (Controller(loop) == loop))
	    count++;

     if(count) {
	experienced += count;
	if(header++ && !IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'=','='));
	output(p,player,2,1,1,"%s"EXPERIENCED_COLOUR"Experienced Builders ("ANSI_LWHITE"%d"EXPERIENCED_COLOUR")...%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH BGCOLOR="HTML_TABLE_RED"><FONT SIZE=4><I>\016":" ",count,IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
	if(header++ && !IsHtml(p)) output(p,player,0,1,0,ANSI_DRED"%s",separator(twidth,0,'-','-') + strlen(ANSI_DCYAN)), header = 1;
	output_columns(p,player,NULL,NULL,twidth,1,20,2,0,1,FIRST,0,NULL,scratch_return_string);

	for(loop = 0, count = 0; loop < db_top; loop++)
	    if((Typeof(loop) == TYPE_CHARACTER) && !Being(loop) && Experienced(loop) && !Level4(loop) && !Retired(loop) && (Controller(loop) == loop)) {
	       if(IsHtml(p)) sprintf(scratch_buffer,"\016<A HREF=\"%sNAME=*%s&\" TARGET=_blank TITLE=\"Click to scan user...\">\016"EXPERIENCED_COLOUR"%s\016</A>\016",html_server_url(p,0,1,"scan"),html_encode(getname(loop),buffer,NULL,sizeof(buffer)),getname_prefix(loop,20,buffer2));
		  else strcpy(scratch_buffer,getname_prefix(loop,20,buffer));
	       output_columns(p,player,scratch_buffer,IsHtml(p) ? NULL:EXPERIENCED_COLOUR,0,1,20,2,0,1,DEFAULT,0,NULL,scratch_return_string);
	    }

	output_columns(p,player,NULL,NULL,0,1,20,2,0,1,LAST,0,NULL,scratch_return_string);
     }

     /* ---->  Assistants  <---- */
     for(loop = 0, count = 0; loop < db_top; loop++)
	 if((Typeof(loop) == TYPE_CHARACTER) && !Being(loop) && Assistant(loop) && !Level4(loop) && (Controller(loop) == loop))
	    count++;

     if(count) {
	assistants += count;
	if(header++ && !IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'=','='));
	output(p,player,2,1,1,"%s"ASSISTANT_COLOUR"Assistants ("ANSI_LWHITE"%d"ASSISTANT_COLOUR")...%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH BGCOLOR="HTML_TABLE_RED"><FONT SIZE=4><I>\016":" ",count,IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
	if(header++ && !IsHtml(p)) output(p,player,0,1,0,ANSI_LRED"%s",separator(twidth,0,'-','-') + strlen(ANSI_DCYAN)), header = 1;
	output_columns(p,player,NULL,NULL,twidth,1,20,2,0,1,FIRST,0,NULL,scratch_return_string);

	for(loop = 0, count = 0; loop < db_top; loop++)
	    if((Typeof(loop) == TYPE_CHARACTER) && !Being(loop) && Assistant(loop) && !Level4(loop) && (Controller(loop) == loop)) {
	       if(IsHtml(p)) sprintf(scratch_buffer,"\016<A HREF=\"%sNAME=*%s&\" TARGET=_blank TITLE=\"Click to scan user...\">\016"ASSISTANT_COLOUR"%s\016</A>\016",html_server_url(p,0,1,"scan"),html_encode(getname(loop),buffer,NULL,sizeof(buffer)),getname_prefix(loop,20,buffer2));
		  else strcpy(scratch_buffer,getname_prefix(loop,20,buffer));
	       output_columns(p,player,scratch_buffer,IsHtml(p) ? NULL:ASSISTANT_COLOUR,0,1,20,2,0,1,DEFAULT,0,NULL,scratch_return_string);
	    }

	output_columns(p,player,NULL,NULL,0,1,20,2,0,1,LAST,0,NULL,scratch_return_string);
     }

     if(!IsHtml(p))
	output(p,player,0,1,0,separator(twidth,0,'-','='));
     output(p,player,2,1,1,"%sTotal Admin: \016&nbsp;\016 "ANSI_DWHITE"%d. \016&nbsp;\016 "EXPERIENCED_COLOUR"Retired Admin: \016&nbsp;\016 "ANSI_DWHITE"%d. \016&nbsp;\016 "EXPERIENCED_COLOUR"Exp. Builders:"ANSI_DWHITE" \016&nbsp;\016 %d. \016&nbsp;\016 "ASSISTANT_COLOUR"Assistants:"ANSI_DWHITE" \016&nbsp;\016 %d.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH BGCOLOR="HTML_TABLE_RED"><B>\016"ANSI_LWHITE:ANSI_LWHITE" ",admin,retired,experienced,assistants,IsHtml(p) ? "\016</B></TH></TR>\016":"\n\n");
     if(IsHtml(p)) output(p,player,1,2,0,"</TABLE>%s",(in_command || !Validchar(player)) ? "":"<BR>");
     html_anti_reverse(p,0);
}

/* ---->  List current Admin/connected Admin (Available to Mortals)  <---- */
void admin_list(CONTEXT)
{
     const    char *admintype = NULL, *page = NULL, *scan;
     struct   descriptor_data *p = getdsc(player);
     unsigned char all = 0;

     setreturn(ERROR,COMMAND_FAIL);
     if(!p) {
        output(p,player,0,1,0,ANSI_LGREEN"Sorry, you must be connected to list administrators of %s.",tcz_full_name);
        return;
     }

     scan = params;
     while(*scan && !isdigit(*scan) && !string_prefix(scan,"page") && !string_prefix(scan,"first") && !string_prefix(scan,"last") && !(all && string_prefix(scan,"all"))) {
           for(; *params && (*params == ' '); params++);
           if(!admintype) admintype = params;
           for(; *params && (*params != ' '); params++);
           for(scan = params; *scan && (*scan == ' '); scan++);
           all = 1;
     }
     if(*params) {
        for(*params++ = '\0'; *params && (*params == ' '); params++);
        page = params;
     }

     if(!Blank(page) && (!strcasecmp("page",page) || !strncasecmp(page,"page ",5)))
        for(page += 4; *page && (*page == ' '); page++);
     page = (char *) parse_grouprange(player,page,FIRST,1);

     if(Blank(admintype) || string_prefix("connected",admintype) || string_prefix("superusers",admintype) || string_prefix("su",admintype)) {

        /* ---->  List currently connected Admin  <---- */
        set_conditions(player,0,0,NOTHING,NOTHING,NULL,211);
        userlist_admin(p,1);
        setreturn(OK,COMMAND_SUCC);
        return;
     } else if(string_prefix("listing",admintype) || string_prefix("all",admintype)) {
        admin_list_all(p,player);
        setreturn(OK,COMMAND_SUCC);
        return;
     } else if(string_prefix("assistants",admintype)) set_conditions_ps(player,ASSISTANT,0,0,0,0,SEARCH_CHARACTER,NOTHING,NULL,305);
        else if(string_prefix("experienced",admintype) || string_prefix("experiencedbuilders",admintype) || string_prefix("experienced builders",admintype)) set_conditions_ps(player,EXPERIENCED,0,0,0,0,SEARCH_CHARACTER,NOTHING,NULL,305);
           else if(string_prefix("retired",admintype)) set_conditions_ps(player,0,0,RETIRED,0,0,SEARCH_CHARACTER,NOTHING,NULL,305);
              else if(string_prefix("retireddruids",admintype) || string_prefix("retired druids",admintype)) set_conditions_ps(player,DRUID,0,RETIRED,0,0,SEARCH_CHARACTER,NOTHING,NULL,305);
                 else if(string_prefix("retiredwizards",admintype) || string_prefix("retired wizards",admintype)) set_conditions_ps(player,0,0,RETIRED,0,DRUID,SEARCH_CHARACTER,NOTHING,NULL,305);
                    else if(string_prefix("apprentices",admintype)) set_conditions_ps(player,APPRENTICE,0,0,0,0,SEARCH_CHARACTER,NOTHING,NULL,305);
                       else if(string_prefix("apprenticedruids",admintype) || string_prefix("apprentice druids",admintype)) set_conditions_ps(player,APPRENTICE|DRUID,0,0,0,0,SEARCH_CHARACTER,NOTHING,NULL,305);
                          else if(string_prefix("apprenticewizards",admintype) || string_prefix("apprentice wizards",admintype)) set_conditions_ps(player,APPRENTICE,0,0,0,DRUID,SEARCH_CHARACTER,NOTHING,NULL,305);
                             else if(string_prefix("wizards",admintype)) set_conditions_ps(player,WIZARD,0,0,0,DRUID,SEARCH_CHARACTER,NOTHING,NULL,305);
                                else if(string_prefix("druids",admintype)) set_conditions_ps(player,WIZARD|DRUID,0,0,0,0,SEARCH_CHARACTER,NOTHING,NULL,305);
                                   else if(string_prefix("elders",admintype)) set_conditions_ps(player,ELDER,0,0,0,0,SEARCH_CHARACTER,NOTHING,NULL,305);
                                      else if(string_prefix("elderdruids",admintype) || string_prefix("elder druids",admintype)) set_conditions_ps(player,ELDER|DRUID,0,0,0,0,SEARCH_CHARACTER,NOTHING,NULL,305);
                                         else if(string_prefix("elderwizards",admintype) || string_prefix("elder wizards",admintype)) set_conditions_ps(player,ELDER,0,0,0,DRUID,SEARCH_CHARACTER,NOTHING,NULL,305);
                                            else if(string_prefix("deities",admintype) || string_prefix("deity",admintype) || string_prefix("supremebeings",admintype)) set_conditions_ps(player,DEITY,0,0,0,0,SEARCH_CHARACTER,NOTHING,NULL,305);
                                               else {
                                                  output(p,player,0,1,0,ANSI_LGREEN"Please specify either the Admin rank to list, '"ANSI_LWHITE"list"ANSI_LGREEN"' to list all ranks or '"ANSI_LWHITE"connected"ANSI_LGREEN"' to list currently connected Admin.");
                                                  return;
					       }

     /* ---->  List all Admin of given rank  <---- */
     userlist_admin(p,0);
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Ask for assistance (Mortal) or give user assistance (Admin)  <---- */
void admin_assist(CONTEXT)
{
     setreturn(ERROR,COMMAND_FAIL);
#ifdef EXPERIENCED_HELP
     if(Level4(player) || Experienced(player) || Assistant(player)) {
#else
     if(Level4(player)) {
#endif
        if(!in_command) {
           if(!Experienced(player) || Help(player)) {
              if(!strcasecmp(arg1,"list") || !strncasecmp(arg1,"list ",5)) {

                 /* ---->  List users who need assistance  <---- */
                 for(; *arg1 && (*arg1 != ' '); arg1++);
                 for(; *arg1 && (*arg1 == ' '); arg1++);
                 userlist_view(player,arg1,NULL,NULL,NULL,15,0);
	      } else {
                 struct   descriptor_data *d;
                 unsigned char found = 0;
                 dbref    who = NOTHING;

                 /* ---->  Give assistance to a user  <---- */
                 if(!Blank(arg1) && ((who = lookup_character(player,arg1,1)) == NOTHING)) {
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
                       return;
		    }
                    if(!Validchar(who) || Connected(who)) {
                       time_t now;

                       gettime(now);
                       for(d = descriptor_list; d && !found; d = (found) ? d:d->next)
                           if((d->flags & ASSIST) && Validchar(d->player) && ((d->player == who) || (who == NOTHING)))
                              found = 1;

                       if(d && Validchar(d->player)) {
                          if(!(Experienced(player) || Assistant(player)) || (admin_can_assist() || ((now - (d->assist_time - (ASSIST_TIME * MINUTE))) > (ASSIST_RESPONSE_TIME * MINUTE)))) {
                             int    ic_cache = in_command;
                             dbref  cached_loc;

                             if(!Blank(arg2)) {
                                if(!Haven(player)) {
                                   char *ptr;

                                   /* ---->  Send response to user  <---- */
                                   strcpy(scratch_return_string,getname(player));
                                   for(ptr = scratch_return_string; *ptr; ptr++)
                                       if(islower(*ptr)) *ptr = toupper(*ptr);

                                   output(d,d->player,0,1,0,"");
                                   output(d,d->player,0,1,10,ANSI_LRED"[ASSIST] \016&nbsp;\016 "ANSI_LYELLOW"%s"ANSI_LWHITE"%s"ANSI_LYELLOW" offers you %s help (Type "ANSI_LGREEN"TELL %s = YOUR MESSAGE"ANSI_LYELLOW" to reply)...\n",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0),Possessive(player,LOWER),scratch_return_string);
                                   sprintf(scratch_return_string,"*%s",getname(d->player));                                
                                   pagetell_send(player,NULL,NULL,scratch_return_string,arg2,1,0);

                                   if(command_boolean != COMMAND_FAIL) {

                                      /* ---->  Notify user giving assistance  <---- */
                                      output(getdsc(player),player,0,1,0,ANSI_LGREEN"\nResponse sent to %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s request for assistance sent.\n",Article(d->player,LOWER,(who == NOTHING) ? INDEFINITE:DEFINITE),getcname(NOTHING,d->player,0,0));

                                      /* ---->  Notify appropriate Admin that user has been given assistance  <---- */
                                      wrap_leading = 10;
                                      sprintf(scratch_buffer,ANSI_LGREEN"[ASSIST] \016&nbsp;\016 "ANSI_LWHITE"%s"ANSI_LYELLOW"%s"ANSI_LWHITE" responds to ",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0));
                                      sprintf(scratch_buffer + strlen(scratch_buffer),"%s"ANSI_LYELLOW"%s"ANSI_LWHITE"'s request for assistance.",Article(d->player,LOWER,INDEFINITE),getcname(NOTHING,d->player,0,0));
                                      admin_notify_assist(scratch_buffer,NULL,player);
                                      wrap_leading = 0;

                                      FREENULL(d->assist);
                                      d->flags      &= ~ASSIST;
                                      d->assist_time = now + ((ASSIST_TIME * MINUTE) / 2);
                                      writelog(ASSIST_LOG,1,"ASSIST","%s(#%d) responded to %s(#%d).",getname(player),player,getname(d->player),d->player);
                                      setreturn(OK,COMMAND_SUCC);
				   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, unable to page your response to %s"ANSI_LWHITE"%s"ANSI_LGREEN".\n",Article(d->player,LOWER,(who == NOTHING) ? INDEFINITE:DEFINITE),getcname(NOTHING,d->player,0,0));
				} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please reset your "ANSI_LYELLOW"HAVEN"ANSI_LGREEN" flag before sending a response to this user.");
			     } else  {

                                /* ---->  Teleport to user's location to give in-depth assistance  <---- */
                                in_command = 1;
                                cached_loc = db[player].location, db[player].owner = ROOT;
                                sprintf(scratch_return_string,"*%s",getname(d->player));
                                move_teleport(player,NULL,NULL,"me",scratch_return_string,0,0);
                                in_command = ic_cache, db[player].owner = player;

                                if(command_boolean != COMMAND_FAIL) {

                                   /* ---->  Notify user giving assistance  <---- */
                                   if(db[player].location != cached_loc) {
                                      sprintf(scratch_buffer,ANSI_LGREEN"You head over to %s"ANSI_LYELLOW"%s"ANSI_LGREEN" to give ",Article(db[player].location,LOWER,DEFINITE),unparse_object(player,db[player].location,0));
                                      output(getdsc(player),player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN" some assistance.",scratch_buffer,Article(d->player,LOWER,(who == NOTHING) ? INDEFINITE:DEFINITE),getcname(NOTHING,d->player,0,0));
				   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"You offer %s"ANSI_LWHITE"%s"ANSI_LGREEN" your assistance.",Article(d->player,LOWER,(who == NOTHING) ? INDEFINITE:DEFINITE),getcname(NOTHING,d->player,0,0));

                                   /* ---->  Notify user being given assistance  <---- */
                                   output(d,d->player,0,1,0,"");
                                   output(d,d->player,0,1,10,ANSI_LRED"[ASSIST] \016&nbsp;\016 "ANSI_LYELLOW"%s"ANSI_LWHITE"%s"ANSI_LYELLOW" %sand offers you %s help.\n",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0),(db[player].location != cached_loc) ? "enters the room ":"",Possessive(player,LOWER));

                                   if(db[player].location != cached_loc) {

                                      /* ---->  Notify users in room user who is giving assistance was in  <---- */
                                      if(!Invisible(cached_loc) && !Secret(db[player].location) && !Secret(player)) {
                                         sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" heads over to ",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0));
                                         sprintf(scratch_buffer + strlen(scratch_buffer),"%s"ANSI_LYELLOW"%s"ANSI_LGREEN" to give ",Article(db[player].location,LOWER,INDEFINITE),getname(db[player].location));
                                         output_except(cached_loc,player,NOTHING,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN" some assistance.",scratch_buffer,Article(d->player,LOWER,INDEFINITE),getcname(NOTHING,d->player,0,0));
				      }

                                      /* ---->  Notify users in room of user being given assistance  <---- */
                                      if(!Invisible(db[player].location)) {
                                         sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" heads over to ",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0));
                                         sprintf(scratch_buffer + strlen(scratch_buffer),"%s"ANSI_LYELLOW"%s"ANSI_LGREEN" to give ",Article(db[player].location,LOWER,DEFINITE),getname(db[player].location));
                                         output_except(db[player].location,player,d->player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN" some assistance.",scratch_buffer,Article(d->player,LOWER,DEFINITE),getcname(NOTHING,d->player,0,0));
				      }
				   }

                                   /* ---->  Notify appropriate Admin that user has been given assistance  <---- */
                                   wrap_leading = 10;
                                   sprintf(scratch_buffer,ANSI_LGREEN"[ASSIST] \016&nbsp;\016 "ANSI_LWHITE"%s"ANSI_LYELLOW"%s"ANSI_LWHITE" heads off to give ",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0));
                                   sprintf(scratch_buffer + strlen(scratch_buffer),"%s"ANSI_LYELLOW"%s"ANSI_LWHITE" some assistance.",Article(d->player,LOWER,INDEFINITE),getcname(NOTHING,d->player,0,0));
                                   admin_notify_assist(scratch_buffer,NULL,player);
                                   wrap_leading = 0;

                                   FREENULL(d->assist);
                                   d->flags      &= ~ASSIST;
                                   d->assist_time = now + ((ASSIST_TIME * MINUTE) / 2);
                                   writelog(ASSIST_LOG,1,"ASSIST","%s(#%d) helped %s(#%d).",getname(player),player,getname(d->player),d->player);
                                   setreturn(OK,COMMAND_SUCC);
				} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, unable to teleport you to %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s current location.",Article(d->player,LOWER,(who == NOTHING) ? INDEFINITE:DEFINITE),getcname(NOTHING,d->player,0,0));
			     }
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Assistants, Experienced Builders and retired administrators may only give assistance to users who have asked for assistance and have not received a response for over "ANSI_LWHITE"%d minute%s"ANSI_LGREEN", or when there are insufficient Apprentice Wizards/Druids and above connected.",ASSIST_RESPONSE_TIME,Plural(ASSIST_RESPONSE_TIME));
		       } else output(getdsc(player),player,0,1,0,(who == NOTHING) ? ANSI_LGREEN"Sorry, there are no users connected at the moment who require assistance.":ANSI_LGREEN"Sorry, either that character has already been given assistance, or they haven't asked for it.");
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that character isn't connected.");
		 }
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Experienced Builders and retired administrators who have their "ANSI_LYELLOW"HELP"ANSI_LGREEN" flag set may give assistance to other users.");
#ifdef EXPERIENCED_HELP
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Assistants, Experienced Builders, retired administrators and Apprentice Wizards/Druids and above may give assistance to users.");
#else
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may give assistance to users.");
#endif
     } else if(!in_command) {
        struct descriptor_data *p = getdsc(player);
        time_t now,total;
        char   *ptr;

        if(Connected(player) && p) {
           gettime(now);
           total = db[player].data->player.totaltime + (now - db[player].data->player.lasttime);
           if((total <= HOUR) || !Blank(params)) {
              if(now >= p->assist_time) {
                 if(Blank(params) || ((strlen(ptr = punctuate(params,2,'.')) <= 256) && !strchr(ptr,'\n'))) {
                    if(Blank(params) || !instring("%{",ptr)) {
                       struct descriptor_data *d;
                       short  count = 0;

                       for(d = descriptor_list; d; d = d->next)
#ifdef EXPERIENCED_HELP
                           if(Validchar(d->player) && (Experienced(d->player) || Level4(d->player))) count++;
#else
                           if(Validchar(d->player) && Level4(d->player)) count++;
#endif
                       if(count) {
                          if(!Blank(params)) {
                             FREENULL(p->assist);
                             p->assist = (char *) alloc_string(compress(ptr,1));
		  	  } else FREENULL(p->assist);
                          p->assist_time = now + (ASSIST_TIME * MINUTE);
                          output(p,player,0,1,0,ANSI_LGREEN"Your request for assistance has been sent to %s's administrators  -  Someone will be along shortly to help you.",tcz_full_name);

                          wrap_leading = 10;
                          if(!Blank(p->assist)) sprintf(scratch_return_string," ('"ANSI_LCYAN"%s"ANSI_LWHITE"')",substitute(player,scratch_buffer,ptr,0,ANSI_LCYAN,NULL,0));
                             else *scratch_return_string = '\0';
                          sprintf(scratch_buffer,ANSI_LRED"[ASSIST] \016&nbsp;\016 "ANSI_LWHITE"%s"ANSI_LYELLOW"%s"ANSI_LWHITE" is stuck and needs some assistance from you%s  -  ",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0),scratch_return_string);
                          if(!Blank(p->assist)) sprintf(scratch_return_string," ('%s')",ptr);
                             else *scratch_return_string = '\0';
                          if(p->flags & ASSIST) {
                             writelog(ASSIST_LOG,1,"ASSIST","%s(#%d) asked for assistance%s  -  Their previous request was not dealt with.",getname(player),player,scratch_return_string);
                             strcat(scratch_buffer,ANSI_LMAGENTA"\007THIS CHARACTER HAS ALREADY ASKED FOR ASSISTANCE PREVIOUSLY AND WAS IGNORED"ANSI_LWHITE"  -  "ANSI_LCYAN""ANSI_BLINK"*PLEASE*"ANSI_LWHITE" type '"ANSI_LGREEN"assist"ANSI_LWHITE"' to teleport to this user and tell them that you're available to help.");
			  } else {
                             writelog(ASSIST_LOG,1,"ASSIST","%s(#%d) asked for assistance%s",getname(player),player,BlankContent(scratch_return_string) ? ".":scratch_return_string);
                             strcat(scratch_buffer,"Type '"ANSI_LGREEN"assist"ANSI_LWHITE"' to teleport to this user and tell them that you're available to help.");
                             p->flags |= ASSIST;
			  }

                          admin_notify_assist(scratch_buffer,NULL,player);
                          wrap_leading = 0;
                          setreturn(OK,COMMAND_SUCC);
		       } else output(p,player,0,1,0,ANSI_LGREEN"\nSorry, there are no users connected at the moment who can give you assistance.  Please type "ANSI_LWHITE"TUTORIAL NEWBIE"ANSI_LGREEN" to read our tutorial for new users and "ANSI_LWHITE"HELP CHATTING"ANSI_LGREEN" for details of the commands you can use to talk to other users.  If you're still stuck, try typing "ANSI_LYELLOW"ASSIST"ANSI_LGREEN" again in a few minute's time, or (If you're really stuck), try sending E-mail to "ANSI_LWHITE"%s"ANSI_LGREEN".",tcz_admin_email);
		    } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, your reason for asking for assistance mustn't contain query substitutions ('"ANSI_LWHITE"%{<QUERY COMMAND>}"ANSI_LGREEN"'.)");
		 } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of your reason for asking for assistance is 256 characters.  It also must not contain embedded NEWLINE's (Please keep your reason short and brief, if possible.)");
	      } else if(p->flags & ASSIST) {
                 if(!Blank(p->assist)) sprintf(scratch_return_string," ('"ANSI_LWHITE"%s"ANSI_LGREEN"')",substitute(player,scratch_buffer,decompress(p->assist),0,ANSI_LWHITE,NULL,0));
                    else *scratch_return_string = '\0';
                 output(p,player,0,1,0,ANSI_LGREEN"Sorry, you have already asked for assistance in the past "ANSI_LWHITE"%d minute%s"ANSI_LGREEN"%s  -  Please wait until someone is available to help you.  If you don't receive any help, please ask for assistance again in "ANSI_LYELLOW"%s"ANSI_LGREEN" time.",ASSIST_TIME,Plural(ASSIST_TIME),scratch_return_string,interval(p->assist_time - now,p->assist_time - now,ENTITIES,0));
	      } else output(p,player,0,1,0,ANSI_LGREEN"Please wait for "ANSI_LWHITE"%s"ANSI_LGREEN" before asking for further assistance.",interval(p->assist_time - now,p->assist_time - now,ENTITIES,0));
	   } else output(p,player,0,1,0,ANSI_LGREEN"Please specify what you would like assistance on, e.g:  '"ANSI_LWHITE"assist Can someone show me how to use the BBS?"ANSI_LGREEN"', '"ANSI_LWHITE"assist Can someone help me with one of my compound commands?"ANSI_LGREEN"', etc.");
	} else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you must be connected to ask for assistance.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't ask for assistance from within a compound command.");
}

/* ---->  Ban a character from connecting for a given amount of time  <---- */
void admin_ban(CONTEXT)
{
     char   *bantime,*reason;
     time_t btime,now;
     dbref  character;

     setreturn(ERROR,COMMAND_FAIL);
     if(!in_command) {
        if(Level3(player)) {
           split_params((char *) arg2,&bantime,&reason);
           if(!Blank(reason)) reason = punctuate(reason,2,'.');
           if(!Blank(arg1)) {
              if((character = lookup_character(player,arg1,1)) != NOTHING) {
                 if(player != character) {
                    if(!Level4(character)) {
                       if(*bantime && (string_prefix("liftban",bantime) || string_prefix("no",bantime))) {

                          /* ---->  Lift character's ban  <---- */
                          if(!Blank(reason)) {
                             output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN"'s ban has been lifted.",Article(character,UPPER,DEFINITE),getcname(NOTHING,character,0,0));
                             output(getdsc(character),character,0,1,0,"\n"ANSI_LRED"[%s"ANSI_LYELLOW"%s"ANSI_LRED" has lifted your ban  -  REASON:  "ANSI_LWHITE"%s"ANSI_LRED"]\n",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0),reason);
                             writelog(BAN_LOG,1,"BAN","%s(#%d)'s ban has been lifted by %s(#%d)  -  REASON:  %s",getname(character),character,getname(player),player,reason);
                             db[character].data->player.bantime = 0;
                             setreturn(OK,COMMAND_SUCC);
		          } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the reason(s) for lifting %s"ANSI_LWHITE"%s"ANSI_LGREEN" ban.",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0));
		       } else if(*bantime && (string_prefix("permanently",bantime) || string_prefix("forever",bantime))) {

                          /* ---->  Permanently ban character  <---- */
                          if(!Blank(reason)) {
                             output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has been banned permanently  -  Type '"ANSI_LWHITE"@ban %s = lift"ANSI_LGREEN"' to lift this ban.",Article(character,UPPER,DEFINITE),getcname(NOTHING,character,0,0),arg1);
                             output(getdsc(character),character,0,1,0,"\n"ANSI_LRED"[%s"ANSI_LYELLOW"%s"ANSI_LRED" has banned you from using %s permanently  -  REASON:  "ANSI_LWHITE"%s"ANSI_LRED"]\n",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0),tcz_full_name,reason);
                             writelog(BAN_LOG,1,"BAN","%s(#%d) has been banned permanently by %s(#%d)  -  REASON:  %s",getname(character),character,getname(player),player,reason);
                             db[character].data->player.bantime = -1;
                             setreturn(OK,COMMAND_SUCC);
		          } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the reason(s) for banning %s"ANSI_LWHITE"%s"ANSI_LGREEN" permanently.",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0));
  		       } else {

                          /* ---->  Ban character for given amount of time  <---- */
                          if((btime = parse_time((char *) bantime)) >= 0) {
 	                     if(btime) {
                                if(btime <= YEAR) {
                                   if(!Blank(reason)) {
                                      gettime(now);
                                      strcpy(scratch_return_string,interval(btime,btime,ENTITIES,0));
                                      output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has been banned for "ANSI_LYELLOW"%s"ANSI_LGREEN"  -  Type '"ANSI_LWHITE"@ban %s = lift"ANSI_LGREEN"' to lift this ban.",Article(character,UPPER,DEFINITE),getcname(NOTHING,character,0,0),interval(btime,btime,ENTITIES,0),arg1);
                                      output(getdsc(character),character,0,1,0,"\n"ANSI_LRED"[%s"ANSI_LYELLOW"%s"ANSI_LRED" has banned you from using %s for "ANSI_LWHITE"%s"ANSI_LRED"  -  REASON:  "ANSI_LWHITE"%s"ANSI_LRED"]\n",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0),tcz_full_name,scratch_return_string,reason);
                                      db[character].data->player.bantime = now + btime;
                                      writelog(BAN_LOG,1,"BAN","%s(#%d) has been banned for %s by %s(#%d)  -  REASON:  %s",getname(character),character,scratch_return_string,getname(player),player,reason);
                                      setreturn(OK,COMMAND_SUCC);
				   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the reason(s) for banning %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0));
			        } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a character can't be banned for a period greater than 1 year.");
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify how long you'd like to ban %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0));
		          } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, invalid amount of time (To ban %s"ANSI_LWHITE"%s"ANSI_LGREEN" for) specified.",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0));
		       }
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Apprentice Wizards/Druids and above can't be banned.");
	         } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't ban yourself from %s.",tcz_full_name);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify who you would like to ban.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Wizards/Druids and above may ban characters from using %s.",tcz_full_name);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, bans can't be imposed from within a compound command.");
}

/* ---->  Forcefully disconnect character (Or disconnect 'dead' connections)  <---- */
unsigned char admin_boot_character(dbref player,unsigned char bootdead)
{
	 struct descriptor_data *d,*next;
	 int    count = 0;

	 if(bootdead) {
	    struct descriptor_data *active = NULL;
	    time_t now;

	    gettime(now);
	    for(d = descriptor_list, count = 0; d; d = d->next)
		if((d->flags & CONNECTED) && (d->player == player)) {
		   if(!active || (d->last_time > active->last_time))
		      active = d;
		   count++;
		}
	    if(count <= 1) return(0);

	    for(d = descriptor_list, count = 0; d; d = next) {
		next = d->next;
		if((d->flags & CONNECTED) && (d->player == player) && (d != active) && (d->last_time < (now - MINUTE))) {
		   *bootmessage = '\n';
		   server_shutdown_sock(d,1,0);
		   count++;
		}
	    }
	 } else for(d = descriptor_list; d; d = next) {
	    next = d->next;
	    if((d->flags & CONNECTED) && (d->player == player)) {
	       *bootmessage = '\n';
	       server_shutdown_sock(d,1,0);
	       count++;
	    }
	 }
	 return(count > 0);
}

/* ---->  Boot character (Forcefully disconnect ALL of their connections)  <---- */
void admin_boot(CONTEXT)
{
     char   *reason,*bantime,*tmp;
     struct descriptor_data *d;
     dbref  character;
     int    ban = 0;
     time_t now;

     setreturn(ERROR,COMMAND_FAIL);
     if(!in_command || Wizard(current_command)) {
        if(Level4(db[player].owner)) {
           if(!(flow_control & (FLOW_FOR|FLOW_WITH|FLOW_WHILE|FLOW_FOREACH))) {
              if(Boot(db[player].owner)) {
                 if(!Blank(arg1)) {
                    if((character = lookup_character(player,arg1,1)) != NOTHING) {
	               if(can_write_to(player,character,1)) {
	                  if(character != player) {
                             if(Connected(character)) {
                                /* ---->  Boot reason  <---- */
                                split_params((char *) arg2,&reason,&bantime);
                                if(!Blank(bantime)) tmp = reason, reason = bantime, bantime = tmp;

                                if(!Blank(reason)) {
                                   if(!Censor(player) && !Censor(db[player].location)) bad_language_filter(scratch_return_string,reason);
                                      else strcpy(scratch_return_string,reason);
                                   strcpy(scratch_buffer,punctuate(scratch_return_string,2,'.'));
                                   substitute(player,scratch_return_string,scratch_buffer,0,ANSI_LWHITE,NULL,0);
			        } else {
                                   output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the reason(s) for booting %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0));
                                   return;
   			        }

                                /* ---->  Optional (Short) ban  <---- */
                                if(!Blank(bantime)) {
                                   if(!Level4(character)) {
                                      if(((ban = atol(bantime)) <= 0) || (ban > 60)) {
                                         output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify a value between "ANSI_LWHITE"1"ANSI_LGREEN" and "ANSI_LWHITE"60"ANSI_LGREEN" minutes.");
                                         return;
				      }
				   } else {
                                      output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't boot and ban an Apprentice Wizard/Druid or above (You can only boot them.)");
                                      return; 
				   }
				}

                                /* ---->  Notify character doing boot  <---- */
                                gettime(now);
                                if(!db[character].data->player.bantime && ban)
                                   db[character].data->player.bantime = now + (ban * MINUTE);
                                if(ban) {
                                   writelog(BOOT_LOG,1,"BOOT","%s(#%d) booted and banned %s(#%d) for %d minute%s  -  REASON:  %s",getname(player),player,getname(character),character,ban,Plural(ban),scratch_buffer);
                                   sprintf(scratch_buffer," and banned for "ANSI_LWHITE"%d"ANSI_LYELLOW" minute%s",ban,Plural(ban));
			        } else {
                                   writelog(BOOT_LOG,1,"BOOT","%s(#%d) booted %s(#%d)  -  REASON:  %s",getname(player),player,getname(character),character,scratch_buffer);
                                   *scratch_buffer = '\0';
		   	        }
                                output(getdsc(player),player,0,1,0,ANSI_LYELLOW"[%s"ANSI_LWHITE"%s"ANSI_LYELLOW" has been booted%s!  -  REASON:  %s"ANSI_LYELLOW"]",Article(character,UPPER,DEFINITE),getcname(NOTHING,character,0,0),scratch_buffer,scratch_return_string);
                          
                                /* ---->  Notify every other non-QUIET connected user  <---- */
                                sprintf(bootmessage,ANSI_LYELLOW"[%s"ANSI_LWHITE"%s"ANSI_LYELLOW" has been booted by ",Article(character,UPPER,INDEFINITE),getcname(NOTHING,character,0,0));
                                sprintf(bootmessage + strlen(bootmessage),"%s"ANSI_LWHITE"%s"ANSI_LYELLOW"!  -  REASON:  %s"ANSI_LYELLOW"]",Article(player,LOWER,INDEFINITE),getcname(NOTHING,player,0,0),scratch_return_string);
                                for(d = descriptor_list; d; d = d->next)
                                    if((d->flags & CONNECTED) && !Quiet(d->player) && (d->player != player) && (d->player != character))
                                       output(d,d->player,0,1,0,"%s",bootmessage);

                                /* ---->  Notify and boot character  <---- */
                                if(ban) sprintf(scratch_buffer,"and banned you for "ANSI_LWHITE"%d"ANSI_LYELLOW" minute%s",ban,Plural(ban));
                                   else strcpy(scratch_buffer,"you");
                                sprintf(bootmessage,"\n"ANSI_LYELLOW"[%s"ANSI_LWHITE"%s"ANSI_LYELLOW" has booted %s!  -  REASON:  %s"ANSI_LYELLOW"]\n",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0),scratch_buffer,scratch_return_string);
                                admin_boot_character(character,0);
                                setreturn(OK,COMMAND_SUCC);
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't boot a character who isn't currently connected.");
		          } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't boot yourself.");
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only boot someone who's of a lower level than yourself.");
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify who you would like to boot.");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you've been banned from booting characters off %s.",tcz_full_name);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@boot"ANSI_LGREEN"' can't be used from within an '"ANSI_LWHITE"@for"ANSI_LGREEN"', '"ANSI_LWHITE"@with"ANSI_LGREEN"', '"ANSI_LWHITE"@while"ANSI_LGREEN"' or '"ANSI_LWHITE"@foreach"ANSI_LGREEN"' statement.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above can boot characters off %s.",tcz_full_name);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a character can not be booted from within a compound command.");
}

/* ---->  Boot character's 'dead' connections  <---- */
void admin_bootdead(CONTEXT)
{
     dbref character;

     setreturn(ERROR,COMMAND_FAIL);
     if(!in_command || Wizard(current_command)) {
        if(!Blank(params)) {
           if((character = lookup_character(player,params,1)) == NOTHING) {
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",params);
              return;
	   }
	} else character = db[player].owner;

        if(can_write_to(player,character,1)) {
           if(Connected(character)) {
              sprintf(bootmessage,"\n"ANSI_LRED"[You're currently connected to %s more than once:  "ANSI_LWHITE"This connection is either no-longer responding or idle  -  It has been booted using the '"ANSI_LGREEN"@bootdead"ANSI_LWHITE"' command."ANSI_LRED"]\n",tcz_full_name);
              if(player != character) {
                 if(admin_boot_character(character,1)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN"'s 'dead'/'locked-up' connections have been booted.",Article(character,UPPER,DEFINITE),getcname(NOTHING,character,0,0));
                    else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" has no 'dead'/'locked-up' connections at the moment ('dead'/'locked-up' connections must be idle for at least 1 minute before you can boot them using '"ANSI_LYELLOW"@bootdead"ANSI_LGREEN"'.)",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0));
	      } else output(getdsc(player),player,0,1,0,admin_boot_character(character,1) ? ANSI_LGREEN"Your 'dead'/'locked-up' connections have been booted.":ANSI_LGREEN"Sorry, you have no 'dead'/'locked-up' connections at the moment ('dead'/'locked-up' connections must be idle for at least 1 minute before you can boot them using '"ANSI_LYELLOW"@bootdead"ANSI_LGREEN"'.)");
              setreturn(OK,COMMAND_SUCC);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't boot the 'dead'/'locked-up' connections of a character who isn't currently connected.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only boot the 'dead'/'locked-up' connections of someone who's of a lower level than yourself.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't boot your 'dead'/'locked-up' connections from within a compound command.");
}

/* ---->  Change controller of a character (Turn them into a "puppet")  <---- */
void admin_controller(CONTEXT)
{
     dbref character,controller;

     setreturn(ERROR,COMMAND_FAIL);
     if(Level4(Owner(player))) {
	if(!in_command || Wizard(current_command)) {
           if(!Blank(arg1)) {
              if((character = lookup_character(player,arg1,1)) == NOTHING) {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
                 return;
	      }
	   } else character = db[player].owner;

           if(!Engaged(character)) {
              if(!Married(character)) {

                 /* ---->  Look up new controller  <---- */
                 if(!Blank(arg2)) {
                    if((controller = lookup_character(player,arg2,1)) == NOTHING) {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg2);
                       return;
		    }
		 } else controller = character;

                 if(!Readonly(character)) {
                       if(can_write_to(player,character,1)) {
                          if(!Puppet(character) || can_write_to(player,Controller(character),1)) {
   	                     if(can_write_to(player,controller,1)) {
                                if(player != character) {
                                   if(character == controller) {
                                      if(Controller(character) != character) {
                                         if(Level4(player)) writelog(ADMIN_LOG,1,"PUPPET","%s(#%d) reset %s(#%d)'s controller (%s(#%d).)",getname(player),player,getname(character),character,getname(Controller(character)),Controller(character));
                                            else writelog(ADMIN_LOG,1,"HACK","Mortal %s(#%d) reset %s(#%d)'s controller (%s(#%d)) from within compound command %s(#%d) owned by %s(#%d).",getname(player),player,getname(character),character,getname(Controller(character)),Controller(character),getname(current_command),current_command,getname(db[current_command].owner),db[current_command].owner);
				      }

                                      output(getdsc(character),character,0,1,0,"\n"ANSI_LYELLOW"[You're no-longer a puppet of %s"ANSI_LWHITE"%s"ANSI_LYELLOW".]\n",Article(Controller(character),LOWER,INDEFINITE),getcname(NOTHING,Controller(character),0,0));
                                      if(player != Controller(character)) output(getdsc(Controller(character)),Controller(character),0,1,0,"\n"ANSI_LYELLOW"[%s"ANSI_LWHITE"%s"ANSI_LYELLOW" is no-longer one of your puppets.]\n",Article(character,UPPER,INDEFINITE),getcname(NOTHING,character,0,0));
                                      output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" is no-longer a puppet.",Article(character,UPPER,DEFINITE),getcname(NOTHING,character,0,0));
                                      db[character].data->player.controller = controller;
                                      setreturn(OK,COMMAND_SUCC);
				   } else if(!Level4(character)) {
  				      if(!(Builder(character) || Assistant(character) || Experienced(character))) {
                                         if(controller != player) {
                                            output(getdsc(controller),controller,0,1,0,"\n"ANSI_LYELLOW"[%s"ANSI_LWHITE"%s"ANSI_LYELLOW" is now one of your puppets.]\n",Article(character,UPPER,INDEFINITE),getcname(NOTHING,character,0,0));
                                            sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" is now a puppet of ",Article(character,UPPER,DEFINITE),getcname(NOTHING,character,0,0));
                                            output(getdsc(player),player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(controller,LOWER,DEFINITE),getcname(NOTHING,controller,0,0));
					 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" is now one of your puppets.",Article(character,UPPER,DEFINITE),getcname(NOTHING,character,0,0));

                                      if(Level4(player)) writelog(ADMIN_LOG,1,"PUPPET","%s(#%d) made %s(#%d) a puppet of %s(#%d).",getname(player),player,getname(character),character,getname(controller),controller);
                                         else writelog(ADMIN_LOG,1,"HACK","Mortal %s(#%d) made %s(#%d) a puppet of %s(#%d) from within compound command %s(#%d) owned by %s(#%d).",getname(player),player,getname(character),character,getname(controller),controller,getname(current_command),current_command,getname(db[current_command].owner),db[current_command].owner);
                                      output(getdsc(character),character,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"You're now one of %s"ANSI_LYELLOW"%s"ANSI_LWHITE"'s puppets.",Article(controller,LOWER,INDEFINITE),getcname(NOTHING,controller,0,0));
                                      db[character].data->player.controller = controller;
                                      db[character].data->player.quotalimit = 0;
                                      setreturn(OK,COMMAND_SUCC);
                                   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the controller of a Builder, Assistant or Experienced Builder can not be changed.");
                                } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the controller of an Apprentice Wizard/Druid or above can not be changed.");
                             } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't change your own controller.");
                          } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the controller of %s"ANSI_LWHITE"%s"ANSI_LGREEN" to yourself or someone who's of a lower level than you.",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0));
                       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the controller of a character who's currently controlled by someone who's of a lower level than yourself.");
                    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the controller of a character who's of a lower level than yourself.");
                 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  %s controller can not be changed.",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0),Possessive(character,1));
              } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the controller of a character who is married can not be changed.");
           } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the controller of a character who is engaged can not be changed.");
        } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the controller of a character can not be changed from within a compound command.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above can change the controller of a character.");
}

/* ---->  Dump database to disk  <---- */
void admin_dump(CONTEXT)
{
     setreturn(ERROR,COMMAND_FAIL);
#ifdef DATABASE_DUMP
     if(!in_command || Wizard(current_command)) {
        if(Level2(Owner(player))) {
           if(!Blank(params)) {
              if(!strcasecmp("restart",params) || !strcasecmp("reset",params)) {

                 /* ---->  Restart database dump  <---- */
                 if(dumpstatus) {
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Restarting database dump...");
                    writelog(DUMP_LOG,1,"DUMP","Database dump restarted by %s(#%d).",getname(player),player);
                    dumpstatus = 255;
                    db_write(NULL);
                    if(!Level4(player) && in_command) writelog(HACK_LOG,1,"HACK","Mortal %s(#%d) restarted database dump from within compound command %s(#%d) owned by %s(#%d).",getname(player),player,getname(current_command),current_command,getname(db[current_command].owner),db[current_command].owner);
		 }
	      } else if(!strcasecmp("abort",arg1) || !strcasecmp("stop",arg1)) {

                 /* ---->  Abort database dump  <---- */
                 if(dumpstatus) {
                    if(!Blank(arg2)) {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Aborting database dump...");
                       writelog(DUMP_LOG,1,"DUMP","Database dump aborted by %s(#%d)  -  REASON:  %s.",getname(player),player,arg2);
                       dumpstatus = 255;
                       db_write(NULL);
                       if(!Level4(player) && in_command) writelog(HACK_LOG,1,"HACK","Mortal %s(#%d) restarted database dump from within compound command %s(#%d) owned by %s(#%d).",getname(player),player,getname(current_command),current_command,getname(db[current_command].owner),db[current_command].owner);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify a reason for aborting the current database dump.");
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a database dump is not currently in progress - Unable to abort database dump.");
                 return;
	      } else if(!strcasecmp("email",params) || !strcasecmp("emails",params) || !strcasecmp("emailaddresses",params) || !strcasecmp("email addresses",params)) {
                 if(Level1(db[player].owner) && !Druid(db[player].owner)) {
                    dbref i,count = 0;
                    const char *ptr;
                    FILE  *f;

                    /* ---->  Write list of user private E-mail addresses to file for external processing  <---- */
                    if((f = fopen(EMAIL_FILE,"w")) != NULL) {
                       writelog(ADMIN_LOG,1,"E-MAIL","%s(#%d) wrote list of user private E-mail addresses to the file '"EMAIL_FILE"'.",getname(player),player);
                       for(i = 0; i < db_top; i++)
                           if((Typeof(i) == TYPE_CHARACTER) && (db[i].data->player.bantime != -1) && (ptr = gettextfield(2,'\n',getfield(i,EMAIL),0,scratch_return_string)) && *ptr)
                              fprintf(f,"%s\n",ptr), count++;
                       fflush(f);
                       fclose(f);
		       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LYELLOW"%d"ANSI_LGREEN" private E-mail address%s written to the file '"ANSI_LWHITE""EMAIL_FILE""ANSI_LGREEN"'.",count,(count == 1) ? "":"es");
    	               writelog(EMAIL_LOG,1,"E-MAIL","%d private E-mail address%s written to the file '"EMAIL_FILE"'.",count,(count == 1) ? "":"es");
	               writelog(DUMP_LOG,1,"E-MAIL","%d private E-mail address%s written to the file '"EMAIL_FILE"'.",count,(count == 1) ? "":"es");
                       if(!Level4(player) && in_command) writelog(HACK_LOG,1,"HACK","Mortal %s(#%d) wrote list of user private E-mail addresses to the file '"EMAIL_FILE"' (This file is not accessible by %s) from within compound command %s(#%d) owned by %s(#%d).",getname(player),player,Objective(player,0),getname(current_command),current_command,getname(db[current_command].owner),db[current_command].owner);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, unable to open the file '"ANSI_LWHITE""EMAIL_FILE""ANSI_LGREEN"' for writing  -  Unable to write the E-mail addresses of all characters in the database to this file.");
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Deities and the Supreme Being may write the E-mail addresses of all characters in the database to the file '"ANSI_LWHITE""EMAIL_FILE""ANSI_LGREEN"'.");
                 return;
	      } else if(!strcasecmp("forward",params) || !strcasecmp("emailforward",params) || !strcasecmp("email forward",params) || !strcasecmp("emailforwards",params) || !strcasecmp("emailforwarding",params) || !strcasecmp("forwarding",params) || !strcasecmp("forwardingaddresses",params) || !strcasecmp("forwarding addresses",params)) {
                 if(Level2(db[player].owner)) {
                    writelog(ADMIN_LOG,1,"E-MAIL","%s(#%d) wrote user E-mail forwarding addresses to the file '"FORWARD_FILE"' ",getname(player),player);
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Writing E-mail forwarding addresses of characters to the file '"ANSI_LWHITE""FORWARD_FILE""ANSI_LGREEN"' (Once complete, an entry will be made in the '"ANSI_LYELLOW"Dump"ANSI_LGREEN"' and '"ANSI_LYELLOW"Email"ANSI_LGREEN"' log files)...");
                    if(!Level4(player) && in_command) writelog(HACK_LOG,1,"HACK","Mortal %s(#%d) wrote user E-mail forwarding addresses to the file '"FORWARD_FILE"' (This file is not accessible by %s) from within compound command %s(#%d) owned by %s(#%d).",getname(player),player,Objective(player,0),getname(current_command),current_command,getname(db[current_command].owner),db[current_command].owner);
                    tcz_time_sync(4);
                    setreturn(OK,COMMAND_SUCC);
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Elder Wizards/Druids and above may force the E-mail forwarding addresses of characters to be written to the file '"ANSI_LWHITE""FORWARD_FILE""ANSI_LGREEN"'.");
                 return;
	      }
	   }

           /* ---->  Start database dump  <---- */
           if(dumpstatus == 0) {
              dumpstatus = 1;
              dumptype   = DUMP_SANITISE;
#ifdef DB_FORK
              if(option_forkdump(OPTSTATUS)) {
                 dumpstatus = 254;
                 dumptype   = DUMP_NORMAL;
	      }
#endif
              writelog(DUMP_LOG,1,"DUMP","Database dump manually started by %s(#%d).",getname(player),player);
#ifndef DB_FORK
              if(option_forkdump(OPTSTATUS))
                 db_write(NULL);
#endif
              if(dumpstatus > 0) {
	         if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Dumping database to disk...");
                 if(!Level4(player) && in_command) writelog(HACK_LOG,1,"HACK","Mortal %s(#%d) started database dump from within compound command %s(#%d) owned by %s(#%d).",getname(player),player,getname(current_command),current_command,getname(db[current_command].owner),db[current_command].owner);
                 setreturn(OK,COMMAND_SUCC);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Unable to open/write to database dump file  -  Unable to dump database to disk.");
	   } else if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the database is already in the process of dumping to disk.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the database can only be dumped to disk by Elder Wizards/Druids and above (PLEASE NOTE:  Database dumping to disk takes place automatically at regular intervals.)");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the database can't be dumped to disk from within a compound command.");
#else
     output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, database dumping has been disabled.  Unable to dump database.");
#endif
}

/* ---->  'Escape' from prison-style room without triggering fuses  <---- */
void admin_escape(CONTEXT)
{
     setreturn(ERROR,COMMAND_FAIL);
     if(Level4(db[player].owner)) {
        if(db[player].location != db[player].destination) {
           if(!Quiet(db[player].location)) output_except(db[player].location,player,NOTHING,0,1,2,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" cunningly escapes and heads back to %s home.",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0),Possessive(player,0));
           move_to(player,(Valid(db[player].destination)) ? db[player].destination:ROOMZERO);
           if(!Quiet(db[player].location)) output_except(db[player].location,player,NOTHING,0,1,2,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" arrives, having cunningly escaped from a prison.",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0));
           look_room(player,db[player].location);
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"You cunningly escape and return to your home.\n");
           setreturn(OK,COMMAND_SUCC);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you're already in your home  -  You can't escape from it.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may use the '"ANSI_LWHITE"escape"ANSI_LGREEN"' command.");
}

/* ---->  Force character to execute a command (As though they typed it at their prompt)  <---- */
void admin_force(CONTEXT)
{
     dbref         character;
     unsigned char abort = 0;

     setreturn(ERROR,COMMAND_FAIL);
     if((character = lookup_character(player,arg1,1)) != NOTHING) {
	if(can_write_to(player,character,0)) {
           if(!((player == character) || (Controller(character) == player) || (in_command && (Controller(character) == db[current_command].owner)))) {
              if(!in_command) writelog(FORCE_LOG,1,"FORCE","%s(#%d) forced %s(#%d) to execute '%s'.",getname(player),player,getname(character),character,arg2);
                 else if(!Wizard(current_command) && Level4(db[current_command].owner)) writelog(HACK_LOG,1,"HACK","%s(#%d) forced %s(#%d) to execute '%s' within compound command %s(#%d).",getname(player),player,getname(character),character,arg2,getname(current_command),current_command);
	   }

           if(!in_command) {
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"You force %s"ANSI_LWHITE"%s"ANSI_LGREEN" to execute the command '"ANSI_LWHITE"%s"ANSI_LGREEN"'.\n\n"ANSI_DCYAN"[Start of forced character's output...]",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0),arg2);
              if((player != character) && (Controller(character) != player))
                 output(getdsc(character),character,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"%s"ANSI_LYELLOW"%s"ANSI_LWHITE" forced you to execute the command '"ANSI_LYELLOW"%s"ANSI_LWHITE"'.",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0),arg2);
	   }

           /* ---->  Force character to execute command  <---- */
           if(!in_command && (character != player)) redirect_src = character, redirect_dest = player;
           abort |= event_trigger_fuses(character,character,arg2,FUSE_ARGS);
           abort |= event_trigger_fuses(character,Location(character),arg2,FUSE_ARGS);
           if(!abort) command_sub_execute(character,arg2,0,1);
           redirect_src = NOTHING, redirect_dest = NOTHING;
           if(!in_command) output(getdsc(player),player,0,1,0,ANSI_DCYAN"[End of forced character's output.]\n");
           setreturn(OK,COMMAND_SUCC);
	} else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only force someone who's of a lower level than yourself to execute a command.");
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only force your puppets or yourself to execute a command.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
}

/* ---->  Automatically destroy unused characters  <---- */
unsigned char admin_character_maintenance(void)
{
	 static   int    destmorons,destnewbies,destmortals,destbuilders,destroyed,ashcanned;
	 static   time_t now,moron_time,newbie_time,mortal_time,builder_time;
	 int             cached_ic = in_command;
	 static   dbref  character = NOTHING;
	 unsigned char   chardest = 0;

	 if(!maintenance) return(0);
	 if(character == NOTHING) {
	    if(!Validchar(maint_owner)) {
	       writelog(MAINTENANCE_LOG,1,"MAINTENANCE","Maintenance owner %s(#%d) is invalid  -  Unable to start character maintenance.",getname(maint_owner),maint_owner);
	       character = NOTHING;
	       return(0);
	    }

	    gettime(now);
	    if((maint_morons)   && (maint_morons   < 7))  maint_morons   = 7;
	    if((maint_newbies)  && (maint_newbies  < 7))  maint_newbies  = 7;
	    if((maint_mortals)  && (maint_mortals  < 14)) maint_mortals  = 14;
	    if((maint_builders) && (maint_builders < 28)) maint_builders = 28;
	    moron_time   = now - (maint_morons   * DAY);
	    newbie_time  = now - (maint_newbies  * DAY);
	    mortal_time  = now - (maint_mortals  * DAY);
	    builder_time = now - (maint_builders * DAY);
	    destmorons   = 0, destnewbies  = 0;
	    destmortals  = 0, destbuilders = 0;
	    destroyed    = 0, ashcanned    = 0;
	    character    = 1;
	 }

	 in_command = 1;
	 for(; (character < db_top) && !chardest; character++) 
	     if(!Connected(character) && (Typeof(character) == TYPE_CHARACTER) && (maint_owner != character) &&
		!Readonly(character) && !Permanent(character) && !Being(character) && !db[character].data->player.bantime && !Level4(character) && !Retired(character) && (Controller(character) == character)) {
		   if(maint_newbies && (db[character].data->player.lasttime < newbie_time) && (db[character].data->player.totaltime < DAY)) destnewbies++, chardest = 1;
		      else if(maint_morons && Moron(character) && (db[character].data->player.lasttime < moron_time)) destmorons++, chardest = 1;
			 else if(maint_mortals && !Builder(character) && !Moron(character) && (db[character].data->player.lasttime < mortal_time)) destmortals++, chardest = 1;
			    else if(maint_builders && Builder(character) && !Experienced(character) && (db[character].data->player.lasttime < builder_time)) destbuilders++, chardest = 1;

		      /* ---->  Attempt to destroy character  <---- */
		      if(chardest) {
			 db[character].flags |= ASHCAN;
			 if(destroy_object(maint_owner,character,1,0,0,0)) destroyed++;
			    else ashcanned++;
		      }
	     }
	 in_command = cached_ic;

	 if(character >= db_top) {
	    long new;

	    /* ---->  Log result of maintenance  <---- */
	    gettime(new);
	    writelog(MAINTENANCE_LOG,1,"MAINTENANCE","(Owner:  %s(#%d))  %d/%d unused character%s destroyed, %d set ASHCAN (%d Moron%s, %d Newbie%s, %d Mortal%s and %d Builder%s  -  Background character maintenance took %s.)",getname(maint_owner),maint_owner,destroyed,destroyed + ashcanned,Plural(destroyed + ashcanned),ashcanned,destmorons,Plural(destmorons),destnewbies,Plural(destnewbies),destmortals,Plural(destmortals),destbuilders,Plural(destbuilders),interval(new - now,new - now,ENTITIES,0));
	    character = NOTHING;
	    return(0);
	 } else return(1);
}

/* ---->  Automatically destroy unused/expired/junk objects  <---- */
unsigned char admin_object_maintenance(void)
{
	 static   int    destarrays,destproperties,destvariables,destcommands,destalarms,destthings,destexits,destfuses,destrooms,destroyed,ashcanned;
	 int             cached_ic = in_command,objtype;
	 static   time_t now,object_time,junk_time;
	 unsigned char   objdest = 0,ashcan;
	 static   dbref  object = NOTHING;

	 if(!maintenance || !(maint_objects || maint_junk)) return(0);
	 if(object == NOTHING) {
	    gettime(now);
	    if(maint_junk    && (maint_junk    < 7))  maint_junk    = 7;
	    if(maint_objects && (maint_objects < 28)) maint_objects = 28;
	    object_time  = now - (maint_objects * DAY);
	    junk_time    = now - (maint_junk    * DAY);
	    destarrays   = 0, destproperties = 0, destvariables = 0;
	    destcommands = 0, destalarms     = 0, destthings    = 0;
	    destexits    = 0, destfuses      = 0, destrooms     = 0;
	    destroyed    = 0, ashcanned      = 0, object        = 0;
	 }

	 in_command = 1, command_type |= NO_USAGE_UPDATE;
	 for(; (object < db_top) && !objdest; object++) {
	     if(ValidType(object) && (Typeof(object) != TYPE_CHARACTER) && !Readonly(object) && !Permanent(object) && ((db[object].expiry && ((Expiry(object) ? db[object].created:db[object].lastused) < (now - (db[object].expiry * DAY)))) || (maint_objects && (db[object].lastused < object_time)) || (maint_junk && (Ashcan(object) || RoomZero(Location(object))) && (db[object].lastused < junk_time)))) {
		if(!RoomZero(object) && !Root(object) && !Start(object) && Global(object) && !(Level4(Owner(object)) || Experienced(Owner(object)) || Retired(Owner(object)))) {

		   /* ---->  Attempt to destroy object  <---- */
		   ashcan  = Ashcan(object), db[object].flags |= ASHCAN;
		   objtype = Typeof(object), objdest = 1;
		   if(destroy_object(maint_owner,object,0,0,0,0)) {
		      switch(objtype) {
			     case TYPE_PROPERTY:
				  destproperties++;
				  break;
			     case TYPE_VARIABLE:
				  destvariables++;
				  break;
			     case TYPE_COMMAND:
				  destcommands++;
				  break;
			     case TYPE_ALARM:
				  destalarms++;
				  break;
			     case TYPE_ARRAY:
				  destarrays++;
				  break;
			     case TYPE_THING:
				  destthings++;
				  break;
			     case TYPE_EXIT:
				  destexits++;
				  break;
			     case TYPE_FUSE:
				  destfuses++;
				  break;
			     case TYPE_ROOM:
				  destrooms++;
				  break;
		      }
		      destroyed++;
		   } else if(!ashcan) ashcanned++;
		}
	     }
	 }
	 in_command = cached_ic, command_type &= ~NO_USAGE_UPDATE;

	 if(object >= db_top) {
	    long new;

	    /* ---->  Log result of maintenance  <---- */
	    gettime(new);
	    writelog(MAINTENANCE_LOG,1,"MAINTENANCE","%d object%s destroyed (%d alarm%s, %d compound command%s, %d dynamic array%s, %d exit%s, %d fuse%s, %d propert%s, %d room%s, %d thing%s, %d variable%s), %d set ASHCAN  -  Background object maintenance took %s.",destroyed,Plural(destroyed),destalarms,Plural(destalarms),destcommands,Plural(destcommands),destarrays,Plural(destarrays),destexits,Plural(destexits),destfuses,Plural(destfuses),destproperties,(destproperties == 1) ? "y":"ies",destrooms,Plural(destrooms),destthings,Plural(destthings),destvariables,Plural(destvariables),ashcanned,interval(new - now,new - now,ENTITIES,0));
	    object = NOTHING;
	    return(0);
	 } else return(1);
}

/* ---->  Manually start maintenance (Characters, objects, mail or BBS)  <---- */
void admin_maintenance(CONTEXT)
{
#ifndef DEMO
     setreturn(ERROR,COMMAND_FAIL);
     if(!in_command || Wizard(current_command)) {
        if(Level1(player)) {
           if(!Blank(params) && (string_prefix("characters",params) || string_prefix("players",params))) {
              if(maintenance) {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Starting background character maintenance...");
                 if(!Level4(player) && in_command) writelog(ADMIN_LOG,1,"HACK","Mortal %s(#%d) manually started character maintenance within compound command %s(#%d) owned by %s(#%d)...",getname(player),player,getname(current_command),current_command,getname(db[current_command].owner),db[current_command].owner);
                 tcz_time_sync(2);
                 admin_character_maintenance();
                 setreturn(OK,COMMAND_SUCC);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please turn '"ANSI_LYELLOW"Maintenance"ANSI_LGREEN"' in '"ANSI_LWHITE"@admin"ANSI_LGREEN"' "ANSI_LYELLOW"ON"ANSI_LGREEN" first (You can turn it off again after background character maintenance has completed if you don't want it to take place automatically on a weekly basis.)");
	   } else if(!Blank(params) && (string_prefix("objects",params) || string_prefix("databaseobjects",params))) {
              if(maintenance) {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Starting background object maintenance...");
                 if(!Level4(player) && in_command) writelog(ADMIN_LOG,1,"HACK","Mortal %s(#%d) manually started object maintenance within compound command %s(#%d) owned by %s(#%d)...",getname(player),player,getname(current_command),current_command,getname(db[current_command].owner),db[current_command].owner);
                 tcz_time_sync(3);
                 admin_object_maintenance();
                 setreturn(OK,COMMAND_SUCC);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please turn '"ANSI_LYELLOW"Maintenance"ANSI_LGREEN"' in '"ANSI_LWHITE"@admin"ANSI_LGREEN"' "ANSI_LYELLOW"ON"ANSI_LGREEN" first (You can turn it off again after background object maintenance has completed if you don't want it to take place automatically on a monthly basis.)");
	   } else if(!Blank(params) && (string_prefix("mailboxes",params) || string_prefix("mailitems",params))) {
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, mailbox maintenance hasn't been implemented yet.");
              if(!Level4(player) && in_command) writelog(ADMIN_LOG,1,"HACK","Mortal %s(#%d) manually started mailbox maintenance within compound command %s(#%d) owned by %s(#%d)...",getname(player),player,getname(current_command),current_command,getname(db[current_command].owner),db[current_command].owner);
/*              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Starting background mailbox maintenance..."); */
              setreturn(OK,COMMAND_SUCC);
	   } else if(!Blank(params) && (string_prefix("bbsmessages",params) || string_prefix("bbstopics",params))) {
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Starting %s BBS maintenance...",tcz_full_name);
              if(!Level4(player) && in_command) writelog(ADMIN_LOG,1,"HACK","Mortal %s(#%d) manually started %s BBS maintenance within compound command %s(#%d) owned by %s(#%d)...",getname(player),player,tcz_full_name,getname(current_command),current_command,getname(db[current_command].owner),db[current_command].owner);
              bbs_delete_outofdate();
              setreturn(OK,COMMAND_SUCC);
	   } else if(!Blank(params) && string_prefix("requests",params)) {
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Starting new character requests queue maintenance...");
              if(!Level4(player) && in_command) writelog(ADMIN_LOG,1,"HACK","Mortal %s(#%d) manually started new character request queue maintenance within compound command %s(#%d) owned by %s(#%d)...",getname(player),player,getname(current_command),current_command,getname(db[current_command].owner),db[current_command].owner);
              request_expired();
              setreturn(OK,COMMAND_SUCC);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LWHITE"characters"ANSI_LGREEN"', '"ANSI_LWHITE"objects"ANSI_LGREEN"', '"ANSI_LWHITE"mail"ANSI_LGREEN"' or '"ANSI_LWHITE"BBS"ANSI_LGREEN"'.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Deities and the Supreme Being may manually start database maintenance.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, database maintenance can't be manually started from within a compound command.");
#else
     output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, maintenance is not available in the demonstration version of TCZ.");
#endif
}

/* ---->  Monitor abusive/problem user  <---- */
void admin_monitor(CONTEXT)
{
#ifndef DEMO
     dbref  character,monitor = NOTHING;
     struct descriptor_data *c,*d;
     char   *options,*reason;
     int    flags = 0;

     setreturn(ERROR,COMMAND_FAIL);
     if(!in_command) {
        if(Level4(db[player].owner)) {
           split_params((char *) arg2,&options,&reason);
           if(!Blank(arg1)) {
              if((character = lookup_character(player,arg1,1)) == NOTHING) {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
                 return;
	      }
           } else {
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify which abusive/problem user you'd like to monitor.");
              return;
	   }

           if(Connected(character)) {
              if(player != character) {
                 if(can_write_to(player,character,1)) {
                    if(!Blank(options) && string_prefix("commands",options)) flags = MONITOR_CMDS;
	               else if(!Blank(options) && (string_prefix("output",options) || string_prefix("text",options))) flags = MONITOR_OUTPUT;
                          else if(!Blank(options) && (string_prefix("both",options) || string_prefix("all",options))) flags = MONITOR_OUTPUT|MONITOR_CMDS;
                             else if(Blank(options) || !(string_prefix("off",options) || string_prefix("stop",options) || string_prefix("no",options))) {
                                output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LWHITE"commands"ANSI_LGREEN"', '"ANSI_LWHITE"output"ANSI_LGREEN"', '"ANSI_LWHITE"both"ANSI_LGREEN"' or '"ANSI_LWHITE"off"ANSI_LGREEN"'.");
                                return;
			     }

                    if(!flags || !Blank(reason)) {

                       /* ---->  Character starting monitor connected?  <---- */
                       for(c = descriptor_list; c && (c->player != player); c = c->next);
                       if(!c || !(c->flags & CONNECTED)) {
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only a connected user can monitor another character.");
                          return;
		       }

                       /* ---->  Someone else is already monitoring user  <---- */
                       for(d = descriptor_list; d; d = d->next)
                           if((d->flags & CONNECTED) && d->monitor && (d->player == character) && (d->monitor->player != player))
                              monitor = d->monitor->player;

                       if(Validchar(monitor)) {
                          if(flags) {
                             sprintf(scratch_buffer,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is being monitored by ",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0));
                             output(getdsc(player),player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN" (You can stop this monitor by typing '"ANSI_LYELLOW"@monitor %s = off"ANSI_LGREEN"', providing you're of a higher level than the person who's monitoring them.)",scratch_buffer,Article(monitor,LOWER,INDEFINITE),getcname(NOTHING,monitor,0,0),getname(character));
                             return;
		          } else if(!can_write_to(player,monitor,0)) {
                             sprintf(scratch_buffer,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is being monitored by ",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0));
                             output(getdsc(player),player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN", who's of the same or higher level than yourself.",scratch_buffer,Article(monitor,LOWER,INDEFINITE),getcname(NOTHING,monitor,0,0));
                             return;
			  }
		       }

                       /* ---->  Start/stop monitor  <---- */
                       for(d = descriptor_list; d; d = d->next)
                           if((d->flags & CONNECTED) && (d->player == character)) {
                              d->monitor =  (flags) ? c:NULL;
                              d->flags  &= ~(MONITOR_OUTPUT|MONITOR_CMDS);
                              d->flags  |=  flags;
			   }

                       /* ---->  Log monitor start/stop  <---- */
                       if(flags) {
                          writelog(ADMIN_LOG,1,"MONITOR","Monitor started on %s(#%d) by %s(#%d)%s  -  REASON:  %s",getname(character),character,getname(player),player,(flags & MONITOR_OUTPUT) ? (flags & MONITOR_CMDS) ? " (Commands and output monitored)":" (Output monitored only)":(flags & MONITOR_CMDS) ? " (Commands monitored only)":"",reason);
                          writelog(MONITOR_LOG,1,"MONITOR","Monitor started on %s(#%d) by %s(#%d)%s  -  REASON:  %s",getname(character),character,getname(player),player,(flags & MONITOR_OUTPUT) ? (flags & MONITOR_CMDS) ? " (Commands and output monitored)":" (Output monitored only)":(flags & MONITOR_CMDS) ? " (Commands monitored only)":"",reason);
		       } else {
                          writelog(ADMIN_LOG,1,"MONITOR","%s(#%d)'s monitor on %s(#%d) stopped by %s(#%d)%s%s",getname(Validchar(monitor) ? monitor:player),Validchar(monitor) ? monitor:player,getname(character),character,getname(player),player,Blank(reason) ? ".":"  -  REASON:  ",String(reason));
                          writelog(MONITOR_LOG,1,"MONITOR","%s(#%d)'s monitor on %s(#%d) stopped by %s(#%d)%s%s",getname(Validchar(monitor) ? monitor:player),Validchar(monitor) ? monitor:player,getname(character),character,getname(player),player,Blank(reason) ? ".":"  -  REASON:  ",String(reason));
		       }

                       /* ---->  Inform connected Admin (If non-Mortal is monitored)  <---- */
                       if(!Level4(character)) {
                          if(flags) {
                             sprintf(scratch_buffer,ANSI_LBLUE"[MONITOR] \016&nbsp;\016 "ANSI_LWHITE"Monitor started on %s"ANSI_LYELLOW"%s"ANSI_LWHITE"%s by ",Article(character,LOWER,INDEFINITE),getcname(NOTHING,character,0,0),(flags & MONITOR_OUTPUT) ? (flags & MONITOR_CMDS) ? " (Commands and output monitored)":" (Output monitored only)":(flags & MONITOR_CMDS) ? " (Commands monitored only)":"");
                             output_admin(1,0,1,11,"%s%s"ANSI_LYELLOW"%s"ANSI_LWHITE"%s%s",scratch_buffer,Article(player,LOWER,INDEFINITE),getcname(NOTHING,player,0,0),!Blank(reason) ? " \016&nbsp;\016 "ANSI_DBLUE"- \016&nbsp;\016 "ANSI_LBLUE"REASON:  "ANSI_LCYAN:".",!Blank(reason) ? punctuate(reason,1,'.'):"");
			  } else {
                             sprintf(scratch_buffer,ANSI_LBLUE"[MONITOR] \016&nbsp;\016 "ANSI_LWHITE"Monitor on %s"ANSI_LYELLOW"%s"ANSI_LWHITE" stopped by ",Article(character,LOWER,INDEFINITE),getcname(NOTHING,character,0,0));
                             output_admin(1,0,1,11,"%s%s"ANSI_LYELLOW"%s"ANSI_LWHITE"%s%s",scratch_buffer,Article(player,LOWER,INDEFINITE),getcname(NOTHING,player,0,0),!Blank(reason) ? " \016&nbsp;\016 "ANSI_DBLUE"- \016&nbsp;\016 "ANSI_LBLUE"REASON:  "ANSI_LCYAN:".",!Blank(reason) ? punctuate(reason,1,'.'):"");
			  }
		       }

                       if(!in_command) {
                          if(flags) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%sMonitor started on %s"ANSI_LWHITE"%s"ANSI_LGREEN"%s  -  Type '"ANSI_LYELLOW"@monitor %s = off"ANSI_LGREEN"' to stop monitoring this user.%s",!Level4(character) ? "\n":"",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0),(flags & MONITOR_OUTPUT) ? (flags & MONITOR_CMDS) ? " (Commands and output monitored)":" (Output monitored only)":(flags & MONITOR_CMDS) ? " (Commands monitored only)":"",getname(character),!Level4(character) ? "\n":"");
                             else output(getdsc(player),player,0,1,0,ANSI_LGREEN"%sMonitor on %s"ANSI_LWHITE"%s"ANSI_LGREEN" stopped.%s",!Level4(character) ? "\n":"",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0),!Level4(character) ? "\n":"");
		       }
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the reason(s) for monitoring %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0));
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only monitor someone of a lower level than yourself.");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't monitor yourself.");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only monitor/stop the monitor on a character who's currently connected.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may monitor abusive/problem users.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, characters can't monitored from within a compound command.");
#else
     output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, monitoring is not available in the demonstration version of TCZ.");
#endif
}

/* ---->  Admin. '@nat' chat channel  <---- */
void admin_chat(CONTEXT)
{
     struct descriptor_data *d;
     const  char *title;
#ifdef QMW_RESEARCH
     int qmwpose = 0; /* place to store pose for qmwlogsocket */
     char *qmwposestr[3] = { "STD", "EMOTE", "THINK" };
     int qmwparams = 0; /* shift in params to loose any pose char */ 
#endif /* #ifdef QMW_RESEARCH */

     setreturn(ERROR,COMMAND_FAIL);
     if(Level4(db[player].owner)) {
        if(!Blank(params)) {
#ifdef QMW_RESEARCH
	  /* ----> Save pose and params shift for qmwlogsocket() later */
	  if(*params && ((*params == *POSE_TOKEN) || (*params == *ALT_POSE_TOKEN)))
	    qmwpose = 1;
	  else if(*params && ((*params == *THINK_TOKEN) || (*params == *ALT_THINK_TOKEN)))
	    qmwpose = 2;
	  if (qmwpose > 0) 
	    qmwparams = 1;
#endif /* #ifdef QMW_RESEARCH */
           if(!Quiet(player)) {
              if(!Level1(player)) {
                 if(Level2(player)) {
                    if(Druid(player)) title = ELDER_DRUID_COLOUR"[ELDER DRUID] \016&nbsp;\016 ", wrap_leading = 15;
                       else title = ELDER_COLOUR"[ELDER WIZARD] \016&nbsp;\016 ", wrap_leading = 16;
		 } else if(Level3(player)) {
                    if(Druid(player)) title = DRUID_COLOUR"[DRUID] \016&nbsp;\016 ", wrap_leading = 9;
                       else title = WIZARD_COLOUR"[WIZARD] \016&nbsp;\016 ", wrap_leading = 10;
		 } else if(Level4(player)) {
                    if(Druid(player)) title = APPRENTICE_DRUID_COLOUR"[APP. DRUID] \016&nbsp;\016 ", wrap_leading = 14;
                       else title = APPRENTICE_COLOUR"[APP. WIZARD] \016&nbsp;\016 ", wrap_leading = 15;
		 } else if(!Moron(player)) {
                    if(!Retired(player)) {
                       if(!Experienced(player)) {
                          if(!Assistant(player)) {
         	             if(Builder(player)) title = BUILDER_COLOUR"[BUILDER] \016&nbsp;\016 ", wrap_leading = 11;
		                else title = MORTAL_COLOUR"[MORTAL] \016&nbsp;\016 ", wrap_leading = 10;
			  } else title = ASSISTANT_COLOUR"[ASSISTANT] \016&nbsp;\016 ", wrap_leading = 13;
		       } else title = EXPERIENCED_COLOUR"[EXP. BUILDER] \016&nbsp;\016 ", wrap_leading = 16;
		    } else {
                       if(RetiredDruid(player)) title = RETIRED_DRUID_COLOUR"[RET. DRUID] \016&nbsp;\016 ", wrap_leading = 14;
                          else title = RETIRED_COLOUR"[RET. WIZARD] \016&nbsp;\016 ", wrap_leading = 15;
		    }
		 } else title = MORON_COLOUR"[MORON] \016&nbsp;\016 ", wrap_leading = 9;
	      } else title = DEITY_COLOUR"[DEITY] \016&nbsp;\016 ", wrap_leading = 9;

              strcpy(scratch_return_string,construct_message(player,ANSI_LWHITE,ANSI_DWHITE,"say",'.',-1,PLAYER,params,0,DEFINITE));
              output(getdsc(player),player,0,1,0,"%s%s",title,scratch_return_string);

              strcpy(scratch_return_string,construct_message(player,ANSI_LWHITE,ANSI_DWHITE,"says",'.',-1,OTHERS,params,0,INDEFINITE));
              sprintf(scratch_buffer,"%s%s",title,scratch_return_string);
              for(d = descriptor_list; d; d = d->next)
		if((d->flags & CONNECTED) && Validchar(d->player) && (d->player != player) && Level4(d->player) && !Quiet(d->player)) {
                     output(d,d->player,0,1,0,"%s",scratch_buffer);
#ifdef QMW_RESEARCH
		     qmwlogsocket("NAT:%s:%d:%d:%d:%d:%d:%d:%s",
				  
				  qmwposestr[qmwpose],
				  player, privilege(player,255),
				  db[player].location,
				  d->player,  privilege(d->player,255),
				  db[d->player].location,
				  (char *)(params+qmwparams));
#endif /* #ifdef QMW_RESEARCH */
		}
              wrap_leading = 0;
              setreturn(OK,COMMAND_SUCC);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't chat over the Admin. channel while you're set "ANSI_LYELLOW"QUIET"ANSI_LGREEN".");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"What would you like to say over the Admin. channel?");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may chat over the Admin. channel  -  Please use a chatting channel (See '"ANSI_LWHITE"help chat"ANSI_LGREEN"') or the friends chatting channel (See '"ANSI_LWHITE"help friends"ANSI_LGREEN"') instead.");
}

/* ---->  Change character's password  <---- */
void admin_newpassword(CONTEXT)
{
     dbref character;

     setreturn(ERROR,COMMAND_FAIL);
     filter_spaces(scratch_buffer,arg2,0);
     comms_spoken(player,1);

     if(!in_command) {
        if(Level4(player)) {
           if((character = lookup_character(player,arg1,1)) != NOTHING) {
	      if(player != character) {
                 if(can_write_to(player,character,1)) {
	            if(!Blank(arg2)) {
                       switch(ok_password(scratch_buffer)) {
                              case 1:
                              case 2:
                              case 4:
                                   output(getdsc(player),player,0,1,0,ANSI_LRED"Sorry, that new password is invalid.");
                                   return;
                              case 3:
                                   if(!Level1(db[player].owner)) {
                                      output(getdsc(player),player,0,1,0,ANSI_LRED"Sorry, the minimum length for a password is 6 characters.");
                                      return;
				   }
                                   break;
                              case 5:
                                   output(getdsc(player),player,0,1,0,ANSI_LRED"Sorry, the new password can't be blank.");
                                   return;
		       }

                       gettime(db[character].data->player.pwexpiry);
                       if(Controller(character) != player) writelog(ADMIN_LOG,1,"PASSWORD","%s(#%d) changed %s(#%d)'s password.",getname(player),player,getname(character),character);
                       FREENULL(/* (char *) */ db[character].data->player.password);
#ifdef CYGWIN32
                       db[character].data->player.password = (char *) alloc_string(scratch_buffer);
#else
                       db[character].data->player.password = (char *) alloc_string((char *) crypt(scratch_buffer,scratch_buffer) + 2);
#endif
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN"'s password changed.",Article(character,UPPER,DEFINITE),getcname(NOTHING,character,0,0));
                       output(getdsc(character),character,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"%s"ANSI_LYELLOW"%s"ANSI_LWHITE" has changed your password.",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0));
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify a new password for %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0));
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the password of someone who's of a lower level than yourself.");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please use '"ANSI_LWHITE"@password"ANSI_LGREEN"' if you'd like to change your own password.");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above can change the password of another character (Please type '"ANSI_LWHITE"@password"ANSI_LGREEN"' if you'd like to change your own password.)");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the password of a character can't be changed from within a compound command.");
}

/* ---->  Set or remove marriage prefixes (Mr/Mrs)  <---- */
void admin_marriage_prefix(dbref player,unsigned char set)
{
     const char *ptr;

     if(!Validchar(player)) return;
     if(set) {
        ptr = getfield(player,PREFIX);
        switch(Genderof(player)) {
               case GENDER_MALE:
                    if(Blank(ptr) || (strncasecmp(ptr,"Mr ",3) && strncasecmp(ptr,"Mrs ",4) && strcasecmp(ptr,"Mr") && strcasecmp(ptr,"Mrs"))) {
                       sprintf(scratch_buffer,"Mr%s%s",!Blank(ptr) ? " ":"",String(ptr));
                       if(strlen(scratch_buffer) <= 40) setfield(player,PREFIX,scratch_buffer,0);
		    }
                    break;
               case GENDER_FEMALE:
                    if(Blank(ptr) || (strncasecmp(ptr,"Mr ",3) && strncasecmp(ptr,"Mrs ",4) && strcasecmp(ptr,"Mr") && strcasecmp(ptr,"Mrs"))) {
                       sprintf(scratch_buffer,"Mrs%s%s",!Blank(ptr) ? " ":"",String(ptr));
                       if(strlen(scratch_buffer) <= 40) setfield(player,PREFIX,scratch_buffer,0);
		    }
	}
     } else if((ptr = getfield(player,PREFIX)) && *ptr) {
        if(!strncasecmp(ptr,"Mr ",3) || !strcasecmp(ptr,"Mr")) {
           for(; *ptr && (*ptr != ' '); ptr++);
           for(; *ptr && (*ptr == ' '); ptr++);
           strcpy(scratch_buffer,ptr);
           setfield(player,PREFIX,scratch_buffer,0);
	} else if(!strncasecmp(ptr,"Mrs ",4) || !strcasecmp(ptr,"Mrs")) {
           for(; *ptr && (*ptr != ' '); ptr++);
           for(; *ptr && (*ptr == ' '); ptr++);
           strcpy(scratch_buffer,ptr);
           setfield(player,PREFIX,scratch_buffer,0);
	}
     }
}

/* ---->  Set who character is engaged/married to  <---- */
void admin_partner(CONTEXT)
{
     dbref    character,partner,cache;
     unsigned char marry = 1;
     char     *ptr,*action;

     setreturn(ERROR,COMMAND_FAIL);
     if(!in_command || Wizard(current_command)) {
        if(Level4(db[player].owner)) {
           if(!Blank(arg1)) {
              for(ptr = (char *) arg1; *ptr && (*ptr == ' '); ptr++);
              for(action = ptr; *ptr && (*ptr != ' '); ptr++);
              if(*ptr) *ptr++ = '\0';
                 else *ptr = '\0';
              for(; *ptr && (*ptr == ' '); ptr++);
              arg1 = ptr;

              if(!Blank(action) && (string_prefix("marry",action) || string_prefix("marriage",action) || (string_prefix("engaged",action) && !(marry = 0)))) {
		 if((character = lookup_character(player,ptr,1)) != NOTHING) {
                    if(!Blank(arg2)) {
                       if((partner = lookup_character(player,arg2,1)) != NOTHING) {
                          if(character != partner) {
                             if(!Readonly(character)) {
                                if(!Readonly(partner)) {
                                   if(!Puppet(character) && !Moron(character)) {
                                      if(!Puppet(partner) && !Moron(partner)) {
                                         if(can_write_to(player,character,1)) {
                                            if(can_write_to(player,character,1)) {
                                               if(!((Partner(character) == partner) && ((marry && Married(character)) || (!marry && Engaged(character)))) || ((Partner(partner) == partner) && ((marry && Married(character)) || (!marry && Engaged(character))))) {

                                                  /* ---->  Engage/marry two characters  <---- */
                                                  admin_marriage_prefix(character,0);
                                                  admin_marriage_prefix(partner,0);
                                                  admin_marriage_prefix(Partner(character),0);
                                                  admin_marriage_prefix(Partner(partner),0);
                                                  if(marry) {
                                                     admin_marriage_prefix(character,1);
                                                     admin_marriage_prefix(partner,1);
					          }

                                                  if(!in_command) {
                                                     if(Validchar(Partner(character)) && (Partner(character) != partner) && (Partner(character) != player))
                                                        output(getdsc(Partner(character)),Partner(character),0,1,0,ANSI_LGREEN"You are no-longer %s to %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Married(Partner(character)) ? "married":"engaged",Article(Partner(Partner(character)),LOWER,INDEFINITE),getcname(NOTHING,Partner(Partner(character)),0,0));
                                                     if(Validchar(Partner(partner)) && (Partner(partner) != character) && (Partner(partner) != player))
                                                        output(getdsc(Partner(partner)),Partner(partner),0,1,0,ANSI_LGREEN"You are no-longer %s to %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Married(Partner(partner)) ? "married":"engaged",Article(Partner(Partner(partner)),LOWER,INDEFINITE),getcname(NOTHING,Partner(Partner(partner)),0,0));
                                                     if(character != player) output(getdsc(character),character,0,1,0,ANSI_LGREEN"You are now %s to %s"ANSI_LWHITE"%s"ANSI_LGREEN".",(marry) ? "married":"engaged",Article(partner,LOWER,INDEFINITE),getcname(NOTHING,partner,0,0));
                                                     if(partner   != player) output(getdsc(partner),partner,0,1,0,ANSI_LGREEN"You are now %s to %s"ANSI_LWHITE"%s"ANSI_LGREEN".",(marry) ? "married":"engaged",Article(character,LOWER,INDEFINITE),getcname(NOTHING,character,0,0));
                                                     sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" is now %s to ",Article(character,UPPER,DEFINITE),getcname(NOTHING,character,0,0),(marry) ? "married":"engaged");
                                                     output(getdsc(player),player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(partner,LOWER,DEFINITE),getcname(NOTHING,partner,0,0));
						  }

                                                  if(Valid(cache = Partner(character))) {
                                                     db[cache].flags &= ~(MARRIED|ENGAGED);
                                                     db[cache].data->player.controller = cache;
						  }

                                                  if(Valid(cache = Partner(partner))) {
                                                     db[cache].flags &= ~(MARRIED|ENGAGED);
                                                     db[cache].data->player.controller = cache;
						  }

                                                  db[character].data->player.controller = partner;
                                                  db[partner].data->player.controller = character;
                                                  if(marry) {
                                                     db[character].flags &= ~ENGAGED;
                                                     db[partner].flags   &= ~ENGAGED;
                                                     db[character].flags |=  MARRIED;
                                                     db[partner].flags   |=  MARRIED;
						  } else {
                                                     db[character].flags &= ~MARRIED;
                                                     db[partner].flags   &= ~MARRIED;
                                                     db[character].flags |=  ENGAGED;
                                                     db[partner].flags   |=  ENGAGED;
						  }
                                                  writelog(ADMIN_LOG,1,"PARTNER","%s(#%d) %s %s(#%d) to %s(#%d).",getname(player),player,(marry) ? "married":"engaged",getname(character),character,getname(partner),partner);
                                                  setreturn(OK,COMMAND_SUCC);
					       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, those two characters are already %s.",(marry) ? "married":"engaged");
					    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only %s %s"ANSI_LWHITE"%s"ANSI_LGREEN" to someone who's of a lower level than yourself.",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0),(marry) ? "marry":"engage");
					 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't set who %s"ANSI_LWHITE"%s"ANSI_LGREEN" is %s to (They're of the same or higher level than you.)",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0),(marry) ? "married":"engaged");
				      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" can't be %s to a Puppet/Moron.",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0),(marry) ? "married":"engaged");
				   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Puppets/Morons can't be %s to another character.",(marry) ? "married":"engaged");
				} else {
                                   sprintf(scratch_return_string,"%s"ANSI_LYELLOW"%s"ANSI_LGREEN,Article(partner,LOWER,DEFINITE),getcname(NOTHING,partner,0,0));
                                   output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s is Read-Only  -  You can't %s %s"ANSI_LWHITE"%s"ANSI_LGREEN" to %s.",scratch_return_string,(marry) ? "marry":"engage",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0),Objective(partner,0));
				}
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't set who they are %s to.",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0),(marry) ? "married":"engaged");
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" can't be %s to %s.",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0),(marry) ? "married":"engaged",Reflexive(character,0));
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg2);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify who %s"ANSI_LWHITE"%s"ANSI_LGREEN" would like to %s.",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0),(marry) ? "marry":"be engaged to");
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",ptr);
	      } else if(!Blank(action) && (string_prefix("divorced",action) || string_prefix("unengage",action) || string_prefix("unmarry",action))) {
		 if((character = lookup_character(player,ptr,1)) != NOTHING) {
                    if(Married(character) || Engaged(character)) {
                       if(!Readonly(character)) {
                          if(can_write_to(player,character,1)) {

                             /* ---->  Divorce character  <---- */
                             admin_marriage_prefix(character,0);
                             admin_marriage_prefix(Partner(character),0);
                             if(!in_command) {
                                if(Validchar(Partner(character)) && (Partner(character) != player))
                                   output(getdsc(Partner(character)),Partner(character),0,1,0,ANSI_LGREEN"You are no-longer %s to %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Married(Partner(character)) ? "married":"engaged",Article(Partner(Partner(character)),LOWER,INDEFINITE),getcname(NOTHING,Partner(Partner(character)),0,0));
                                if(character != player) output(getdsc(character),character,0,1,0,ANSI_LGREEN"You are no-longer %s to %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Married(character) ? "married":"engaged",Article(Partner(character),LOWER,INDEFINITE),getcname(NOTHING,Partner(character),0,0));
                                sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" is no-longer %s to ",Article(character,UPPER,DEFINITE),getcname(NOTHING,character,0,0),Married(character) ? "married":"engaged");
                                output(getdsc(player),player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(Partner(character),LOWER,DEFINITE),getcname(NOTHING,Partner(character),0,0));
			     }

                             writelog(ADMIN_LOG,1,"PARTNER","%s(#%d) %s %s(#%d) and %s(#%d).",getname(player),player,Married(character) ? "divorced":"reset the partners of",getname(character),character,getname(Partner(character)),Partner(character));
                             if(Valid(cache = Partner(character))) {
                                db[cache].flags &= ~(MARRIED|ENGAGED);
                                db[cache].data->player.controller = cache;
			     }
                             db[character].flags &= ~(MARRIED|ENGAGED);
                             db[character].data->player.controller = character;
                             setreturn(OK,COMMAND_SUCC);
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only reset the partner of a character who's of a lower level than yourself.");
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't reset %s partner.",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0),Possessive(character,0));
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only reset the partner of a character who is engaged or married to another character.");
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",ptr);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LWHITE"engage"ANSI_LGREEN"', '"ANSI_LWHITE"marry"ANSI_LGREEN"' or '"ANSI_LWHITE"divorce"ANSI_LGREEN"'.");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify who's partner you'd like to set/reset.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above can set/reset the partner of a character.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the partner of a character can't be set/reset from within a compound command.");
}

/* ---->  Change Building Quota limit of a Mortal character  <---- */
void admin_quotalimit(CONTEXT)
{
     int   quotalimit,quotalimitmax;
     char  *value,*reason;
     dbref character;

     setreturn(ERROR,COMMAND_FAIL);
     split_params((char *) arg2,&value,&reason);
     if(!in_command) {
        if(Level4(player)) {
           if(!Blank(arg1)) {
              if((character = lookup_character(player,arg1,1)) == NOTHING) {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
                 return;
	      }
           } else {
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify who's Building Quota limit you'd like to change.");
              return;
	   }

           if(player != character) {
              if((quotalimit = atoi(value)) >= 0) {
 	         if(quotalimit <= (quotalimitmax = MaxQuotaLimit(player))) {
                    if(!(Puppet(character) && (quotalimit > 0))) {
	               if(!Blank(reason)) {
                          if(!Blank(reason)) reason = (char *) punctuate(reason,2,'\0');
                          writelog(ADMIN_LOG,1,"QUOTA","%s(#%d) changed %s(#%d)'s Building Quota limit to %d%s%s.",getname(player),player,getname(character),character,quotalimit,(Blank(reason)) ? "":"  -  REASON:  ",(Blank(reason)) ? "":reason);
  
                          db[character].data->player.quotalimit = quotalimit;
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN"'s Building Quota limit is now "ANSI_LWHITE"%d"ANSI_LGREEN".",Article(character,UPPER,DEFINITE),getcname(NOTHING,character,0,0),quotalimit);
                          output(getdsc(character),character,0,1,0,"\n"ANSI_LRED"[%s"ANSI_LWHITE"%s"ANSI_LRED" has changed your Building Quota limit to "ANSI_LWHITE"%d"ANSI_LRED".]\n",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0),quotalimit);
                          setreturn(OK,COMMAND_SUCC);
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the reason(s) for changing this character's Building Quota limit.");
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't give Building Quota to a puppet.");
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum Building Quota limit you can set is "ANSI_LWHITE"%d"ANSI_LGREEN".",quotalimitmax);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the new Building Quota limit must be a positive number.");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't set your own Building Quota limit.");
	} else if(!Builder(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you aren't a Builder  -  If you'd like to be one, please ask an Apprentice Wizard/Druid or above.");
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't adjust your Building Quota limit  -  Please ask an Apprentice Wizard/Druid or above.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the Building Quota limit of a character can't be changed from within a compound command.");
}

/* ---->  Shout message to every connected character  <---- */
void admin_shout(CONTEXT)
{
     struct descriptor_data *d,*p = getdsc(player);
     const  char *ptr;

     setreturn(ERROR,COMMAND_FAIL);
#ifdef MORTAL_SHOUT

     /* ---->  Mortal shouting allowed (-DMORTAL_SHOUT)  <---- */
     if(!Moron(player)) {
        time_t now,total = db[player].data->player.totaltime;

        gettime(now);
        if(Connected(player)) total += (now - db[player].data->player.lasttime);
        if(total < (MORTAL_SHOUT_CONSTRAINT * HOUR)) {
           sprintf(scratch_buffer,ANSI_LGREEN"Sorry, only Mortals with a total connected time of more than "ANSI_LWHITE"%s"ANSI_LGREEN" may shout (Your total time connected is ",interval(MORTAL_SHOUT_CONSTRAINT * HOUR,MORTAL_SHOUT_CONSTRAINT * HOUR,ENTITIES,0));
           output(p,player,0,1,0,"%s"ANSI_LYELLOW"%s"ANSI_LGREEN".)",scratch_buffer,interval(total,total,ENTITIES,0));
           return;
	}
     } else {
        output(p,player,0,1,0,ANSI_LGREEN"Sorry, Morons aren't allowed to shout.");
        return;
     }
#else

     /* ---->  Mortal shouting not allowed (Default)  <---- */
     if(!Level4(db[player].owner) || (in_command && !Wizard(current_command))) {
        output(p,player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may shout messages to every user currently connected to %s.",tcz_full_name);
        return;
     }
#endif

     /* ---->  Blank message?  <---- */
     if(*params) ptr = params + (((*params == *POSE_TOKEN) || (*params == *THINK_TOKEN) || (*params == *ALT_THINK_TOKEN) || (*params == *BROADCAST_TOKEN)) ? 1:0);
        else ptr = params;

     if(Shout(db[player].owner)) {
        if(!Blank(ptr)) {
           if(!Censor(player) && !Censor(db[player].location)) bad_language_filter((char *) params,params);
           if((*params == *POSE_TOKEN) || (*params == *THINK_TOKEN) || (*params == *ALT_THINK_TOKEN)) {

              /* ---->  [ANNOUNCEMENT:  <NAME> <MESSAGE>]   <---- */
              if(!in_command) writelog(SHOUT_LOG,1,"SHOUT","%s(#%d) announced '%s'.",getname(player),player,params + 1);
                 else writelog(SHOUT_LOG,1,"SHOUT","%s(#%d) announced '%s' from within compound command %s(#%d) owned %s(#%d).",getname(player),player,params + 1,getname(current_command),current_command,getname(db[current_command].owner),db[current_command].owner);
              strcpy(scratch_return_string,construct_message(player,ANSI_LWHITE,ANSI_LWHITE,"",'.',-1,OTHERS,params,0,INDEFINITE));
              sprintf(scratch_buffer,ANSI_LYELLOW"[ANNOUNCEMENT: \016&nbsp;\016 %s"ANSI_LYELLOW"]",scratch_return_string);
              for(d = descriptor_list; d; d = d->next)
                  if((d->flags & CONNECTED) && ((d == p) || (!Quiet(d->player) && !Quiet(db[d->player].location))))
                     output(d,d->player,0,1,2,"%s",scratch_buffer);
	   } else if(*params == *BROADCAST_TOKEN) {

              /* ---->  [BROADCAST MESSAGE from <NAME>:  <MESSAGE>]  <---- */
#ifdef MORTAL_SHOUT
              if(!Level4(db[player].owner)) {
                 output(p,player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may broadcast messages.");
#else
              if(!Level3(db[player].owner)) {
                 output(p,player,0,1,0,ANSI_LGREEN"Sorry, only Wizards/Druids and above may broadcast messages.");
#endif
                 return;
	      }

              if(!in_command) writelog(SHOUT_LOG,1,"SHOUT","%s(#%d) broadcasted '%s'.",getname(player),player,params + 1);
                 else writelog(SHOUT_LOG,1,"SHOUT","%s(#%d) broadcasted '%s' from within compound command %s(#%d) owned by %s(#%d).",getname(player),player,params + 1,getname(current_command),current_command,getname(db[current_command].owner),db[current_command].owner);
              sprintf(scratch_buffer,ANSI_LRED"\007[BROADCAST MESSAGE from %s"ANSI_LYELLOW"%s"ANSI_LRED": \016&nbsp;\016 %s"ANSI_LRED"]",Article(player,LOWER,INDEFINITE),getcname(NOTHING,player,0,0),substitute(player,scratch_return_string,punctuate((char *) params + 1,2,'.'),0,ANSI_LWHITE,NULL,0));
              for(d = descriptor_list; d; d = d->next)
                  if(d->flags & CONNECTED)
                     output(d,d->player,0,0,2,"%s",scratch_buffer);
	   } else {

              /* ---->  <NAME> shouts "<MESSAGE>"  <---- */
              if(!in_command) writelog(SHOUT_LOG,1,"SHOUT","%s(#%d) shouted '%s'.",getname(player),player,params);
                 else writelog(SHOUT_LOG,1,"SHOUT","%s(#%d) shouted '%s' from within compound command %s(#%d) owned by %s(#%d).",getname(player),player,params,getname(current_command),current_command,getname(db[current_command].owner),db[current_command].owner);
              output(p,player,0,1,2,"%s",construct_message(player,ANSI_LWHITE,ANSI_LYELLOW,"shout",'.',0,PLAYER - 2,params,0,DEFINITE));
              strcpy(scratch_buffer,construct_message(player,ANSI_LWHITE,ANSI_LYELLOW,"shouts",'.',0,OTHERS - 2,params,0,INDEFINITE));
              for(d = descriptor_list; d; d = d->next)
                  if((d->flags & CONNECTED) && (d->player != player) && !Quiet(d->player) && !Quiet(db[d->player].location))
                     output(d,d->player,0,1,2,"%s",scratch_buffer);
	   }
           setreturn(OK,COMMAND_SUCC);
	} else output(p,player,0,1,0,ANSI_LGREEN"What message would you like to shout?");
     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you're currently banned from shouting.");
}

/* ---->  Shutdown TCZ MUD server  <---- */
void admin_shutdown(CONTEXT)
{
     const char *ptr;
     int   minutes;

     setreturn(ERROR,COMMAND_FAIL);
     if(!in_command) {
        if(Level1(player)) {
           if(!Druid(player)) {
              if(Blank(arg2) && !Blank(arg1) && (!strcasecmp("cancel",arg1) || !strcasecmp("abort",arg1))) {

                 /* ---->  Cancel shutdown  <---- */
                 if(shutdown_counter >= 0) {
                    writelog(SERVER_LOG,1,"SHUTDOWN","Shutdown cancelled by %s(#%d).",getname(player),player);
                    output_all(1,1,0,0,"\n\x05\x09\x05\x03"ANSI_LRED"["ANSI_BLINK""ANSI_UNDERLINE"SHUTDOWN"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"Shutdown cancelled by %s"ANSI_LYELLOW"%s"ANSI_LWHITE".\n",Article(player,LOWER,INDEFINITE),getcname(NOTHING,player,0,0));
                    FREENULL(shutdown_reason);
                    shutdown_counter = -1;
                    shutdown_who     = NOTHING;
                    setreturn(OK,COMMAND_SUCC);
		 } else {
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the server isn't in the process of shutting down at the moment.");
                    shutdown_counter = -1;
                    setreturn(OK,COMMAND_SUCC);
		 }
	      } else if(shutdown_counter <= 0) {

                 /* ---->  Number of minutes warning before shutdown  <---- */
                 for(ptr = arg2; !Blank(ptr); ptr++)
                     if(!isdigit(*ptr)) {
                        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the time (In minutes) before shutdown.");
                        return;
		     }
                 if(!Blank(arg1)) {
  	            if(!Blank(arg2)) {
	               if((minutes = atol(arg2)) >= 0) {
	                  if(!((minutes < 5) && !Root(player))) {

                             /* ---->  Reason for shutdown  <---- */
                             while(*arg1 && (*arg1 == ' ')) arg1++;
                             if(strlen(arg1) <= KB) {

                                /* ---->  Log who and reason, then begin shutdown  <---- */
                                ptr = (char *) punctuate((char *) arg1,0,'.');
                                writelog(SERVER_LOG,1,"SHUTDOWN","Shutdown started by %s(#%d) (REASON:  %s)  -   Shutdown in %d minute%s.",getname(player),player,String(ptr),minutes, Plural(minutes));
                                shutdown_reason  = (char *) alloc_string(substitute(player,scratch_return_string,(char *) String(ptr),0,ANSI_LWHITE,NULL,0));
                                shutdown_counter = minutes + 1;
                                shutdown_timing  = 0;
                                shutdown_who     = player;
                                setreturn(OK,COMMAND_SUCC);
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the reason for shutting down %s is too long.",tcz_full_name);
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only the Supreme Being may specify a shutdown time of less than 5 minutes.");
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify a positive shutdown time.");
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the time (In minutes) before shutdown.");
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the reason for shutting down %s.",tcz_full_name);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s is already in the process of shutting down (Shutdown started by %s"ANSI_LWHITE"%s"ANSI_LGREEN")  -  Shutdown may be cancelled by typing '"ANSI_LYELLOW"@shutdown cancel"ANSI_LGREEN"'.",tcz_full_name,Article(shutdown_who,LOWER,INDEFINITE),getcname(NOTHING,shutdown_who,0,0));
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Druids may not shutdown %s.",tcz_full_name);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Deities and the Supreme Being may shutdown %s.",tcz_full_name);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s can't be shutdown from within a compound command.",tcz_full_name);
}

/* ---->  Teleport user PLAYER to location of user SUMMON  <---- */
void summon_user(dbref player,dbref summon,unsigned char unconditional,const char *reason)
{
     int    ic_cache = in_command;
     struct descriptor_data *d;
     dbref  cached_loc,owner;
     time_t now;

     gettime(now);
     in_command = 1, owner = db[player].owner;
     cached_loc = db[player].location, db[player].owner = summon;
     if(!unconditional) for(d = descriptor_list; d; d = d->next)
        if(d->player == player) d->summon = NOTHING;

     if(unconditional || Connected(summon)) {
        if(unconditional || Level4(summon) || Abode(db[summon].location) || (can_write_to(summon,db[summon].location,0) == 1)) {
           if(unconditional || (db[player].location != db[summon].location)) {
              sprintf(scratch_return_string,"#%d",db[summon].location);
              move_teleport(player,NULL,NULL,"me",scratch_return_string,0,0);
              if(command_boolean != COMMAND_FAIL) {

                 /* ---->  Notify user being summoned  <---- */
                 sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has summoned you to ",Article(summon,UPPER,INDEFINITE),getcname(NOTHING,summon,0,0));
                 if(!Blank(reason)) output(getdsc(player),player,0,1,0,"%s%s"ANSI_LYELLOW"%s"ANSI_LGREEN" (REASON:  "ANSI_LWHITE"%s"ANSI_LGREEN")",scratch_buffer,Article(db[player].location,LOWER,DEFINITE),getcname(NOTHING,db[player].location,0,0),punctuate((char *) reason,2,'.'));
                    else output(getdsc(player),player,0,1,0,"%s%s"ANSI_LYELLOW"%s"ANSI_LGREEN".",scratch_buffer,Article(db[player].location,LOWER,DEFINITE),getcname(NOTHING,db[player].location,0,0));

                 /* ---->  Notify user who requested summon  <---- */
                 if(reason && unconditional && (Controller(player) != Controller(summon)))
                    writelog(SUMMON_LOG,1,"SUMMON","%s(#%d) summoned %s(#%d) to %s(#%d)  -  REASON:  %s",getname(summon),summon,getname(player),player,getname(db[player].location),db[player].location,punctuate((char *) reason,2,'.'));
                 output(getdsc(summon),summon,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has been summoned to your current location.",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0));

                 /* ---->  Notify users in room user who was summoned was in  <---- */
                 if(!Invisible(cached_loc) && !Secret(db[player].location) && !Secret(player)) {
                    sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has been summoned%s to ",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0),(reason) ? "":" (With permission)");
                    sprintf(scratch_buffer + strlen(scratch_buffer),"%s"ANSI_LYELLOW"%s"ANSI_LGREEN" by ",Article(db[player].location,LOWER,INDEFINITE),getname(db[player].location));
                    output_except(cached_loc,player,NOTHING,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(summon,LOWER,INDEFINITE),getcname(NOTHING,summon,0,0));
		 }

                 /* ---->  Notify users in room where user has been summoned to  <---- */
                 if(!Invisible(db[player].location)) {
                    sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has been summoned%s to ",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0),(reason) ? "":" (With permission)");
                    sprintf(scratch_buffer + strlen(scratch_buffer),"%s"ANSI_LYELLOW"%s"ANSI_LGREEN" by ",Article(db[player].location,LOWER,DEFINITE),getname(db[player].location));
                    output_except(db[player].location,player,summon,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(summon,LOWER,DEFINITE),getcname(NOTHING,summon,0,0));
		 }
                 setreturn(OK,COMMAND_SUCC);
	      } else {
                 if(unconditional) output(getdsc(summon),summon,0,1,0,ANSI_LGREEN"Sorry, unable to teleport %s"ANSI_LWHITE"%s"ANSI_LGREEN" to your current location.",Article(player,LOWER,DEFINITE),getcname(NOTHING,player,0,0));
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, unable to teleport you to %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s current location.",Article(summon,LOWER,INDEFINITE),getcname(NOTHING,summon,0,0));
	      }
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you are already in the same location as %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(summon,LOWER,INDEFINITE),getcname(NOTHING,summon,0,0));
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is no-longer on property that %s owns  -  Unable to teleport you to %s current location.",Article(summon,LOWER,INDEFINITE),getcname(NOTHING,summon,0,0),Subjective(summon,LOWER),Possessive(summon,LOWER));
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is no-longer connected.",Article(summon,LOWER,INDEFINITE),getcname(NOTHING,summon,0,0));
     db[player].owner = owner, in_command = ic_cache;
}

/* ---->  Summon a user to your current location  <---- */
void admin_summon(CONTEXT)
{
     struct descriptor_data *d;
     dbref  who = NOTHING;

     setreturn(ERROR,COMMAND_FAIL);
     if(!in_command || Wizard(current_command)) {
        if(!Blank(arg1)) {
           if(string_prefix("accept",params) || string_prefix("yes",params)) {

              /* ---->  Accept summon  <---- */
              for(d = descriptor_list; d; d = d->next)
                  if(Validchar(d->player) && (d->player == player) && Validchar(d->summon))
                     who = d->summon;

              if(Validchar(who)) summon_user(player,who,0,NULL);
	         else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, no one has asked you for permission to be summoned to their present location.");
	   } else if(string_prefix("refuse",params) || string_prefix("reject",params) || string_prefix("no",params)) {

              /* ---->  Refuse summon  <---- */
              for(d = descriptor_list; d; d = d->next)
                  if(Validchar(d->player) && (d->player == player) && Validchar(d->summon))
                     who = d->summon;

              if(Validchar(who)) {
                 for(d = descriptor_list; d; d = d->next)
                     if(Validchar(d->player) && (d->player == player))
                        d->summon = NOTHING;

                 output(getdsc(who),who,0,1,10,ANSI_LGREEN"[SUMMON] \016&nbsp;\016 "ANSI_LWHITE"Sorry, %s"ANSI_LYELLOW"%s"ANSI_LWHITE" does not wish to be summoned to your present location.",Article(player,LOWER,DEFINITE),getcname(NOTHING,player,0,0));
                 setreturn(OK,COMMAND_SUCC);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, no one has asked you for permission to be summoned to their present location.");
	   } else if(!Blank(arg2)) {

              /* ---->  Forcefully summon a user (Admin only)  <---- */
              if(Level4(player)) {
                 if((who = lookup_character(player,arg1,1)) != NOTHING) {
		    if(player != who) {
                       if(can_write_to(player,who,1)) {
                          if(Connected(who)) {
                             if(db[player].location != db[who].location) summon_user(who,player,1,arg2);
	       	                else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that character is already in the same location as you.");
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that character isn't connected.");
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only summon someone who's of a lower level than yourself.");
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't summon yourself.");
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards and above may forcefully summon another user to their present location.");
	   } else {

              /* ---->  Request to summon a user  <---- */
              if((who = lookup_character(player,arg1,1)) != NOTHING) {
                 if(player != who) {
                    if(Controller(who) != player) {
                       if(Connected(who)) {
                          if(Level4(player) || Abode(db[player].location) || (can_write_to(player,db[player].location,0) == 1)) {
                             if(db[player].location != db[who].location) {
                                for(d = descriptor_list; d; d = d->next)
                                    if(Validchar(d->player) && (d->player == who) && (d->summon == player))
                                       who = NOTHING;

                                if(Validchar(who)) {

                                   /* ---->  Request to summon user  <---- */
                                   sprintf(scratch_buffer,ANSI_LGREEN"[SUMMON] \016&nbsp;\016 "ANSI_LWHITE"%s"ANSI_LYELLOW"%s"ANSI_LWHITE" requests your permission to be summoned to %s current location ",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0),Possessive(player,LOWER));
                                   output(getdsc(who),who,0,1,10,"%s(%s"ANSI_LCYAN"%s"ANSI_LWHITE")  -  Please type '"ANSI_LGREEN"summon accept"ANSI_LWHITE"' to accept, or '"ANSI_LGREEN"summon refuse"ANSI_LWHITE"' to refuse.",scratch_buffer,Article(db[player].location,UPPER,INDEFINITE),unparse_object(who,db[player].location,0));
                                   output(getdsc(player),player,0,1,0,ANSI_LGREEN"You ask %s"ANSI_LWHITE"%s"ANSI_LGREEN" for permission to be summoned to your current location.",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0));

                                   for(d = descriptor_list; d; d = d->next)
                                       if(d->player == who) d->summon = player;
                                   setreturn(OK,COMMAND_SUCC);
				} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have already asked that character for permission to be summoned  -  Please wait for them to accept or refuse.");
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that character is already in the same location as you.");
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only ask a character for permission to be summoned to a location that you own (Or a location that has its "ANSI_LYELLOW"ABODE"ANSI_LGREEN" flag set.)");
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that character isn't connected.");

                       /* ---->  Summon your puppet to your current location  <---- */
		    } else if(db[player].location != db[who].location) summon_user(who,player,1,NULL);
                       else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, your puppet is already in the same location as you.");
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't summon yourself.");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
	   }
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify who you would like to summon.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, users can't be summoned from within a compound command.");
}

/* ---->  Give user an official warning  <---- */
void admin_warn(CONTEXT)
{
     dbref character;
     char  *ptr;

     setreturn(ERROR,COMMAND_FAIL);
     if(!in_command || (Valid(current_command) && Wizard(current_command))) {
        if(Level4(Owner(player))) {
           if(!Blank(arg1)) {
              if((character = lookup_character(player,arg1,1)) != NOTHING) {
                 if(can_write_to(player,character,1)) {
	            if(character != player) {
                       if(!Blank(arg2)) {
                          output(getdsc(player),player,0,1,2,ANSI_LGREEN"You issue an official warning to %s"ANSI_LYELLOW"%s"ANSI_LGREEN":  "ANSI_LWHITE"%s%s",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0),substitute(player,scratch_return_string,ptr = punctuate(arg2,2,'.'),0,ANSI_LWHITE,NULL,0),Connected(character) ? "":"\n");
                          if(!Connected(character)) output(getdsc(player),player,0,1,14,ANSI_LCYAN"PLEASE NOTE:  "ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" isn't currently connected  -  Please also mail details of %s official warning to %s.\n",Article(character,UPPER,DEFINITE),getcname(NOTHING,character,0,0),Possessive(character,0),Objective(character,0));
                          output(getdsc(character),character,0,1,0,"\n\x05\x09\x05\x02"ANSI_LRED"\007["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LMAGENTA"This is an "ANSI_LYELLOW""ANSI_UNDERLINE""ANSI_BLINK"OFFICIAL WARNING"ANSI_LMAGENTA" from %s"ANSI_LCYAN"%s"ANSI_LMAGENTA": \016&nbsp;\016 "ANSI_LWHITE"%s\n",Article(player,LOWER,INDEFINITE),getcname(NOTHING,player,0,0),scratch_return_string);
                          writelog(WARN_LOG,1,"OFFICIAL WARNING","%s(#%d) issued an official warning to %s(#%d):  %s",getname(player),player,getname(character),character,ptr);
                          setreturn(OK,COMMAND_SUCC);
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"What official warning would you like to issue to %s"ANSI_LWHITE"%s"ANSI_LGREEN"?",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0));
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't issue an official warning to yourself.");
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only issue an official warning to someone who's of a lower level than yourself.");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify who you'd like to issue an official warning to.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may issue an official warning to a user.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, official warnings can't be issued from within a compound command.");
}

/* ---->  Welcome new user  <---- */
void admin_welcome(CONTEXT)
{
     setreturn(ERROR,COMMAND_FAIL);
     if(!in_command) {

#ifdef EXPERIENCED_HELP
        if(!(Level4(player) || Experienced(player) || Assistant(player))) {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Assistants, Experienced Builders (Who have their "ANSI_LYELLOW"HELP"ANSI_LGREEN" flag set) and Apprentice Wizards/Druids and above may welcome new users to %s.",tcz_full_name);
           return;
	}
#else
        if(!Level4(player)) {
	   output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may welcome new users to %s.",tcz_full_name);
           return;
	}
#endif

        if(!Experienced(player) || Help(player)) {
           if(!strcasecmp(params,"list") || !strncasecmp(params,"list ",5)) {

              /* ---->  List users who need welcoming  <---- */
              for(; *params && (*params != ' '); params++);
              for(; *params && (*params == ' '); params++);
              userlist_view(player,params,NULL,NULL,NULL,14,0);
	   } else if(!Haven(player)) {
              struct   descriptor_data *d;
              unsigned char found = 0;
              dbref    who = NOTHING;
              char     *ptr;
 
              /* ---->  Welcome user  <---- */
              if(!Blank(params) && ((who = lookup_character(player,params,1)) == NOTHING)) {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",params);
                 return;
	      }

              if(!Validchar(who) || Connected(who)) {
                 time_t now;

                 gettime(now);
                 for(d = descriptor_list; d && !found; d = (found) ? d:d->next)
                     if((d->flags & WELCOME) && Validchar(d->player) && ((d->player == who) || (who == NOTHING)))
                        found = 1;

                 if(d && Validchar(d->player)) {
                    if(!(Experienced(player) || Assistant(player)) || (admin_can_assist() || ((now - d->start_time) > (WELCOME_RESPONSE_TIME * MINUTE)))) {
                       strcpy(scratch_return_string,getname(player));
                       for(ptr = scratch_return_string; *ptr; ptr++)
                           if(islower(*ptr)) *ptr = toupper(*ptr);
  
                       sprintf(scratch_buffer,"\n%s"ANSI_LYELLOW"\x05\x01 Hello and welcome to "ANSI_LWHITE"%s"ANSI_LYELLOW", %s"ANSI_LMAGENTA"%s"ANSI_LYELLOW"!  I'm ",separator(d->terminal_width,1,'-','='),tcz_full_name,Article(d->player,LOWER,DEFINITE),getcname(NOTHING,d->player,0,0));
                       output(d,d->player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LYELLOW" and I'm here to help you.  If you need any help or assistance, please don't hesitate to ask me by typing "ANSI_LGREEN"TELL %s = YOUR MESSAGE"ANSI_LYELLOW".  Enjoy your visit to "ANSI_LWHITE"%s"ANSI_LYELLOW" and have fun!\n\x05\x0B%s",scratch_buffer,Article(player,LOWER,INDEFINITE),getcname(NOTHING,player,0,0),scratch_return_string,tcz_full_name,separator(d->terminal_width,1,'-','='));
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"You welcome %s"ANSI_LWHITE"%s"ANSI_LGREEN" to %s.",Article(d->player,LOWER,(who == NOTHING) ? INDEFINITE:DEFINITE),getcname(NOTHING,d->player,0,0),tcz_full_name);

                       wrap_leading = 11;
                       sprintf(scratch_buffer,ANSI_LGREEN"[WELCOME] \016&nbsp;\016 "ANSI_LWHITE"%s"ANSI_LYELLOW"%s"ANSI_LWHITE" welcomes ",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0));
                       sprintf(scratch_buffer + strlen(scratch_buffer),"%s"ANSI_LYELLOW"%s"ANSI_LWHITE" to %s.",Article(d->player,LOWER,INDEFINITE),getcname(NOTHING,d->player,0,0),tcz_full_name);
                       admin_notify_assist(scratch_buffer,NULL,player);
                       wrap_leading = 0;

                       d->flags &= ~WELCOME;
                       writelog(WELCOME_LOG,1,"WELCOME","%s(#%d) welcomed %s(#%d) to %s.",getname(player),player,getname(d->player),d->player,tcz_full_name);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Assistants, retired administrators and Experienced Builders may only welcome users to %s when they have been connected for over "ANSI_LWHITE"%d minute%s"ANSI_LGREEN", or when there are insufficient Apprentice Wizards/Druids and above connected.",tcz_full_name,WELCOME_RESPONSE_TIME,Plural(WELCOME_RESPONSE_TIME));
		 } else output(getdsc(player),player,0,1,0,(who == NOTHING) ? ANSI_LGREEN"Sorry, there are no new users connected who need welcoming to %s.":ANSI_LGREEN"Sorry, either that character is still answering the login questions, has already been welcomed to %s, or they aren't a new user.",tcz_full_name);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that character isn't connected.");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please reset your "ANSI_LYELLOW"HAVEN"ANSI_LGREEN" flag before welcoming users to %s (Otherwise they will not be able to ask you for help via '"ANSI_LWHITE"page"ANSI_LGREEN"' or '"ANSI_LWHITE"tell"ANSI_LGREEN"'.)",tcz_full_name);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Experienced Builders and retired administrators who have their "ANSI_LYELLOW"HELP"ANSI_LGREEN" flag set may welcome new users to %s.",tcz_full_name);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, new users can't be welcomed to %s from within a compound command.",tcz_full_name);
}
