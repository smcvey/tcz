/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| USERLIST.C  -  Implements listings in various formats of connected users.   |
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
| Module originally designed and written by:  J.P.Boggis 10/03/1994.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: userlist.c,v 1.2 2005/06/29 20:31:33 tcz_monster Exp $

*/


#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "descriptor_flags.h"
#include "object_types.h"
#include "friend_flags.h"
#include "objectlists.h"
#include "flagset.h"
#include "fields.h"


/* ---->  Display summary of number of users  <---- */
const char *userlist_users(dbref player,const char *listed,short deities,short elders,short delders,short wizards,short druids,short apprentices,short dapprentices,short retired,short dretired,short experienced,short assistants,short builders,short mortals,short beings,short puppets,short morons,short idle,unsigned char space,unsigned char html)
{
      int   total = deities + elders + delders + wizards + druids + apprentices + dapprentices + retired + dretired + assistants + builders + mortals + beings + puppets + morons;
      int   admin = deities + elders + delders + wizards + druids + apprentices + dapprentices;

      listed_items(scratch_return_string,1);
      wrap_leading = (grp->nogroups > 0) ? (11 + ((space) ? 1:0)):(10 + ((space) ? 1:0)) + digit_wrap(0,grp->totalitems) + (!Blank(listed) ? 1:2);
      
      if(html) sprintf(scratch_buffer,"\016<TR><TH ALIGN=CENTER WIDTH=15%% BGCOLOR="HTML_TABLE_CYAN">"ANSI_LWHITE"<I><B>Users:</B></I></TH><TD ALIGN=LEFT>\016"ANSI_LYELLOW""ANSI_UNDERLINE"%s"ANSI_LCYAN"%s",scratch_return_string,listed);
         else sprintf(scratch_buffer," %s"ANSI_LWHITE"Users:  "ANSI_LYELLOW""ANSI_UNDERLINE"%s"ANSI_LCYAN"%s",(space) ? " ":"",scratch_return_string,listed);
      *scratch_return_string = '\0';

      if(deities      > 0) sprintf(scratch_return_string + strlen(scratch_return_string),ANSI_LWHITE"%d "DEITY_COLOUR"Deit%s"ANSI_DCYAN,deities,(deities == 1) ? "y":"ies");
      if(elders       > 0) sprintf(scratch_return_string + strlen(scratch_return_string),"%s"ANSI_LWHITE"%d "ELDER_COLOUR"Elder Wizard%s"ANSI_DCYAN,(*scratch_return_string) ? ", ":"",elders,Plural(elders));
      if(delders      > 0) sprintf(scratch_return_string + strlen(scratch_return_string),"%s"ANSI_LWHITE"%d "ELDER_DRUID_COLOUR"Elder Druid%s"ANSI_DCYAN,(*scratch_return_string) ? ", ":"",delders,Plural(delders));
      if(wizards      > 0) sprintf(scratch_return_string + strlen(scratch_return_string),"%s"ANSI_LWHITE"%d "WIZARD_COLOUR"Wizard%s"ANSI_DCYAN,(*scratch_return_string) ? ", ":"",wizards,Plural(wizards));
      if(druids       > 0) sprintf(scratch_return_string + strlen(scratch_return_string),"%s"ANSI_LWHITE"%d "DRUID_COLOUR"Druid%s"ANSI_DCYAN,(*scratch_return_string) ? ", ":"",druids,Plural(druids));
      if(apprentices  > 0) sprintf(scratch_return_string + strlen(scratch_return_string),"%s"ANSI_LWHITE"%d "APPRENTICE_COLOUR"Apprentice Wizard%s"ANSI_DCYAN,(*scratch_return_string) ? ", ":"",apprentices,Plural(apprentices));
      if(dapprentices > 0) sprintf(scratch_return_string + strlen(scratch_return_string),"%s"ANSI_LWHITE"%d "APPRENTICE_DRUID_COLOUR"Apprentice Druid%s"ANSI_DCYAN,(*scratch_return_string) ? ", ":"",dapprentices,Plural(dapprentices));
      if(retired      > 0) sprintf(scratch_return_string + strlen(scratch_return_string),"%s"ANSI_LWHITE"%d "RETIRED_COLOUR"Retired Wizard%s"ANSI_DCYAN,(*scratch_return_string) ? ", ":"",retired,Plural(retired));
      if(dretired     > 0) sprintf(scratch_return_string + strlen(scratch_return_string),"%s"ANSI_LWHITE"%d "RETIRED_DRUID_COLOUR"Retired Druid%s"ANSI_DCYAN,(*scratch_return_string) ? ", ":"",dretired,Plural(dretired));
      if(builders > experienced) {
         sprintf(scratch_return_string + strlen(scratch_return_string),"%s"ANSI_LWHITE"%d "BUILDER_COLOUR"Builder%s"ANSI_DCYAN,(*scratch_return_string) ? ", ":"",builders,Plural(builders));
         if(experienced > 0) sprintf(scratch_return_string + strlen(scratch_return_string),ANSI_DCYAN" ("ANSI_LWHITE"%d "EXPERIENCED_COLOUR"Experienced"ANSI_DCYAN")",experienced);
      } else if(experienced > 0) sprintf(scratch_return_string + strlen(scratch_return_string),"%s"ANSI_LWHITE"%d "EXPERIENCED_COLOUR"Experienced Builder%s"ANSI_DCYAN,(*scratch_return_string) ? ", ":"",experienced,Plural(experienced));
      if(assistants   > 0) sprintf(scratch_return_string + strlen(scratch_return_string),"%s"ANSI_LWHITE"%d "ASSISTANT_COLOUR"Assistant%s"ANSI_DCYAN,(*scratch_return_string) ? ", ":"",assistants,Plural(assistants));
      if(mortals      > 0) sprintf(scratch_return_string + strlen(scratch_return_string),"%s"ANSI_LWHITE"%d "MORTAL_COLOUR"Mortal%s"ANSI_DCYAN,(*scratch_return_string) ? ", ":"",mortals,Plural(mortals));
      if(beings       > 0) sprintf(scratch_return_string + strlen(scratch_return_string),"%s"ANSI_LWHITE"%d "MORTAL_COLOUR"Being%s"ANSI_DCYAN,(*scratch_return_string) ? ", ":"",beings,Plural(beings));
      if(puppets      > 0) sprintf(scratch_return_string + strlen(scratch_return_string),"%s"ANSI_LWHITE"%d "MORTAL_COLOUR"Puppet%s"ANSI_DCYAN,(*scratch_return_string) ? ", ":"",puppets,Plural(puppets));
      if(morons       > 0) sprintf(scratch_return_string + strlen(scratch_return_string),"%s"ANSI_LWHITE"%d "MORON_COLOUR"Moron%s"ANSI_DCYAN,(*scratch_return_string) ? ", ":"",morons,Plural(morons));

      /* ---->  Percentage of users idle/admin (Shown to Admin only)  <---- */
      if(Validchar(player) && Level4(player) && (total > 0)) {
         sprintf(scratch_return_string + strlen(scratch_return_string),"%s"ANSI_LWHITE"%.0f%% "ANSI_LCYAN"idle"ANSI_DCYAN,(*scratch_return_string) ? ", ":"",((double) idle / total) * 100);
         sprintf(scratch_return_string + strlen(scratch_return_string),"%s"ANSI_LWHITE"%.0f%% "ANSI_LCYAN"Admin"ANSI_DCYAN,(*scratch_return_string) ? ", ":"",((double) admin / total) * 100);
      }

      if(*scratch_return_string) sprintf(scratch_buffer + strlen(scratch_buffer),"%s %s "ANSI_DCYAN"(%s.)%s",Blank(listed) ? "":".",(html) ? "\016&nbsp;\016":"",scratch_return_string,(html) ? "\016</TD></TR>\016":"\n");
         else strcat(scratch_buffer,(html) ? ".\016</TD></TR>\016":".\n");
      return(scratch_buffer);
}

/* ---->  Display today's peak and average daily peak  <---- */
const char *userlist_peak(unsigned char space,unsigned char html)
{
      int apptr,apdays = 0,avgpeak = 0;

      for(apptr = stat_ptr; (apptr >= 0) && (apdays < 7); avgpeak += stats[apptr].peak, apptr--, apdays++);
      avgpeak /= apdays;

      if(html) sprintf(scratch_return_string,"\016<TR><TH ALIGN=CENTER WIDTH=15%% BGCOLOR="HTML_TABLE_CYAN">"ANSI_LWHITE"<B><I>Peak:</B></I></TH><TD ALIGN=LEFT>\016"ANSI_LCYAN"Today's peak is "ANSI_LWHITE"%d"ANSI_LCYAN" user%s. \016&nbsp;\016 The average daily peak is "ANSI_LWHITE"%d"ANSI_LCYAN" user%s.\016</TD></TR>\016",(int) stats[stat_ptr].peak,Plural(stats[stat_ptr].peak),avgpeak,Plural(avgpeak));
         else sprintf(scratch_return_string,"  %s"ANSI_LWHITE"Peak:  "ANSI_LCYAN"Today's peak is "ANSI_LWHITE"%d"ANSI_LCYAN" user%s.  The average daily peak is "ANSI_LWHITE"%d"ANSI_LCYAN" user%s.\n",(space) ? " ":"",(int) stats[stat_ptr].peak,Plural(stats[stat_ptr].peak),avgpeak,Plural(avgpeak));
      return(scratch_return_string);
}

/* ---->  Display amount of time TCZ has been running (Up time)  <--- */
const char *userlist_uptime(long total,unsigned char space,unsigned char html)
{
      if(html) sprintf(scratch_return_string,"\016<TR><TH ALIGN=CENTER WIDTH=15%% BGCOLOR="HTML_TABLE_CYAN">"ANSI_LWHITE"<B><I>Uptime:</I></B></TH><TD ALIGN=LEFT>\016"ANSI_LYELLOW"%s.\016</TD></TR></TABLE></TD></TR>\016",interval(total - uptime,total - uptime,ENTITIES,0));
         else sprintf(scratch_return_string,"    %s"ANSI_LWHITE"Up:  "ANSI_LYELLOW"%s.\n\n",(space) ? " ":"",interval(total - uptime,total - uptime,ENTITIES,0));
      return(scratch_return_string);
}

/* ---->  Return given time as interval in short form  <---- */
const char *userlist_shorttime(time_t time,time_t now,char *buffer,unsigned char spod)
{
      if(time >= 0) {
         now -= time;
         if(now >= 0) {
            if(now < MINUTE) sprintf(buffer,"    %02ds",(int) now);
               else if(now < HOUR) sprintf(buffer,"%2dm %02ds",((int) now) / MINUTE,((int) now) % MINUTE);
                  else if(now < DAY) {
                     if(!(spod && ((now / HOUR) >= 8) && ((now / HOUR) < 15)))
                        sprintf(buffer,"%2dh %02dm",((int) now) / HOUR,(((int) now) % HOUR) / MINUTE);
                           else return(" *SPOD*");
		  } else sprintf(buffer,"%2dd %2dh",((int) now) / DAY,(((int) now) % DAY) / HOUR);
	    return(buffer);
         } else return("Invalid");
      } else return("Invalid");
}

/* ---->  Standard user list (Single column):  Login time, title, flags & idle time  <---- */
void userlist_who(struct descriptor_data *d)
{
     short         deities = 0,elders = 0,delders = 0,wizards = 0,druids = 0,apprentices = 0,dapprentices = 0,retired = 0,dretired = 0,experienced = 0,assistants = 0,builders = 0,mortals = 0,beings = 0,puppets = 0,morons = 0,idle = 0;
     unsigned char cached_scrheight,twidth = (d->terminal_width < 50) ? 50:d->terminal_width;
     int           length,flags;
     unsigned long longdate;
     const    char *colour;
     char          *ptr;
     time_t        now;

     gettime(now);
     html_anti_reverse(d,1);
     command_type |= COMM_CMD;
     longdate = epoch_to_longdate(now);
     if(!d->pager && !IsHtml(d) && Validchar(d->player) && More(d->player)) pager_init(d);
     if(IsHtml(d)) output(d,d->player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");

     if(!in_command) {
        if(!IsHtml(d)) {
           for(length = 0; length < (twidth - 31); scratch_return_string[length] = ' ', length++);
           scratch_return_string[twidth - 31] = '\0';
           output(d,d->player,0,1,0,"\n Time:    Name:%sFlags:  Idle:",scratch_return_string);
           output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','='));
	} else output(d,d->player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Time:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Name:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Flags:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Idle:</I></FONT></TH></TR>\016");
     }

     if(Validchar(d->player)) {
        cached_scrheight                     = db[d->player].data->player.scrheight;
        db[d->player].data->player.scrheight = (db[d->player].data->player.scrheight - 9) * 2;
     }
     union_initgrouprange((union group_data *) descriptor_list);
     while(union_grouprange()) {

           /* ---->  Character's login time and name  <---- */
           strcpy(scratch_buffer,colour = privilege_countcolour(grp->cunion->descriptor.player,&deities,&elders,&delders,&wizards,&druids,&apprentices,&dapprentices,&retired,&dretired,&experienced,&assistants,&builders,&mortals,&beings,&puppets,&morons));
           if((now - grp->cunion->descriptor.last_time) >= MINUTE) idle++;

           sprintf(scratch_buffer + strlen(scratch_buffer),IsHtml(d) ? "\016<TR ALIGN=CENTER><TD>\016%s%s":" %s%s  ",IsHtml(d) ? colour:"",(char *) userlist_shorttime(grp->cunion->descriptor.start_time,now,scratch_return_string,1));
           ptr = (char *) getfield(grp->cunion->descriptor.player,PREFIX);
           if(!Blank(ptr) && ((strlen(ptr) + 1 + strlen(getname(grp->cunion->descriptor.player))) <= 20))
              sprintf(scratch_return_string,"%s%s %s",IsHtml(d) ? colour:"",ptr,getname(grp->cunion->descriptor.player));
                 else sprintf(scratch_return_string,"%s%s",IsHtml(d) ? colour:"",getname(grp->cunion->descriptor.player));

           /* ---->  Character's title  <---- */
           if(!(hasprofile(db[grp->cunion->descriptor.player].data->player.profile) && ((db[grp->cunion->descriptor.player].data->player.profile->dob & 0xFFFF) == (longdate & 0xFFFF)))) {
              if(!(grp->cunion->descriptor.flags & DISCONNECTED)) {
                 if(!grp->cunion->descriptor.afk_message) {
                    if(!Moron(grp->cunion->descriptor.player)) {
		       if(!db[grp->cunion->descriptor.player].data->player.bantime) {
                          ptr = (char *) punctuate((char *) getfield(grp->cunion->descriptor.player,TITLE),0,'.');
                          if(!Blank(ptr)) {
                             strcat(scratch_return_string,pose_string(&ptr,"*"));
                             if(*ptr) {
                                length = strlen(scratch_return_string);
                                substitute(grp->cunion->descriptor.player,scratch_return_string + length,ptr,0,colour,NULL,0);
                                bad_language_filter(scratch_return_string + length,scratch_return_string + length);
			     }
			  }
		       } else sprintf(scratch_return_string + strlen(scratch_return_string),BANNED_TITLE,tcz_short_name);
		    } else strcat(scratch_return_string," the Moron!");
		 } else strcat(scratch_return_string,(grp->cunion->descriptor.flags2 & SENT_AUTO_AFK) ? AUTO_AFK_TITLE:AFK_TITLE);
	      } else sprintf(scratch_return_string + strlen(scratch_return_string),LOST_TITLE,Possessive(grp->cunion->descriptor.player,0));
	   } else sprintf(scratch_return_string + strlen(scratch_return_string),BIRTHDAY_TITLE,longdate_difference(db[grp->cunion->descriptor.player].data->player.profile->dob,longdate) / 12);

           /* ---->  Truncate to correct length  <---- */
           if(!IsHtml(d)) {
              length = 0, ptr = scratch_return_string;
              while(*ptr && (length < (twidth - 28)))
                    if(*ptr == '\x1B') {
                       for(; *ptr && (*ptr != 'm'); ptr++);
                       if(*ptr && (*ptr == 'm')) ptr++;
		    } else {
                       if(*ptr >= 32) length++;
                       ptr++;
		    }
              *ptr = '\0';
              sprintf(scratch_buffer + strlen(scratch_buffer),"%s%s",scratch_return_string,colour);
              for(ptr = (scratch_buffer + strlen(scratch_buffer)); length < (twidth - 28); *ptr++ = ' ', length++);
              *ptr = '\0';
	   } else sprintf(scratch_buffer + strlen(scratch_buffer),"\016</TD><TD ALIGN=LEFT>\016%s",scratch_return_string);

           /* ---->  Character's flags  <---- */
           if(!friendflags_set(d->player,grp->cunion->descriptor.player,NOTHING,FRIEND_EXCLUDE))
              flags = friend_flags(d->player,grp->cunion->descriptor.player)|friend_flags(grp->cunion->descriptor.player,d->player);
                 else flags = 0;
           sprintf(scratch_buffer + strlen(scratch_buffer),IsHtml(d) ? "\016</TD><TD><TT>\016%s[%c%c%c%c]\016</TT><TD>\016":"  %s[%c%c%c%c]  ",IsHtml(d) ? colour:"",Moron(grp->cunion->descriptor.player) ? 'M':Level1(grp->cunion->descriptor.player) ? Druid(grp->cunion->descriptor.player) ? 'd':'D':Level2(grp->cunion->descriptor.player) ? Druid(grp->cunion->descriptor.player) ? 'e':'E':Level3(grp->cunion->descriptor.player) ? Druid(grp->cunion->descriptor.player) ? 'w':'W':Level4(grp->cunion->descriptor.player) ? Druid(grp->cunion->descriptor.player) ? 'a':'A':Retired(grp->cunion->descriptor.player) ? RetiredDruid(grp->cunion->descriptor.player) ? 'r':'R':
                   Experienced(grp->cunion->descriptor.player) ? 'X':Assistant(grp->cunion->descriptor.player) ? 'x':Builder(grp->cunion->descriptor.player) ? 'B':Being(grp->cunion->descriptor.player) ? 'b':Puppet(grp->cunion->descriptor.player) ? 'p':'-',Quiet(grp->cunion->descriptor.player) ? 'Q':'-',Haven(grp->cunion->descriptor.player) ? 'H':'-',(!flags) ? '-':(flags & FRIEND_ENEMY) ? '!':'F');

           /* ---->  Idle time  <---- */
           sprintf(scratch_buffer + strlen(scratch_buffer),"%s%s",IsHtml(d) ? colour:"",(char *) userlist_shorttime(grp->cunion->descriptor.last_time,now,scratch_return_string,0));
           strcat(scratch_buffer,IsHtml(d) ? "\016</TD></TR>\016":"\n");
           output(d,d->player,2,1,0,"%s",scratch_buffer);
     }
     if(Validchar(d->player)) db[d->player].data->player.scrheight = cached_scrheight;

     if((grp->condition == 210) && (grp->totalitems == 0)) output(d,d->player,2,1,1,"%s",IsHtml(d) ? "\016<TR ALIGN=CENTER><TD COLSPAN=4>"ANSI_LCYAN"<I>*** &nbsp; NO ONE IS CONNECTED AT THE MOMENT &nbsp; ***</I></TD></TR>\016":" ***  NO ONE IS CONNECTED AT THE MOMENT  ***\n");
        else if(grp->rangeitems == 0) output(d,d->player,2,1,1,"%s",IsHtml(d) ? "\016<TR ALIGN=CENTER><TD COLSPAN=4>"ANSI_LCYAN"<I>*** &nbsp; NO ONE LISTED &nbsp; ***</I></TD></TR>\016":" ***  NO ONE LISTED  ***\n");
     if(!in_command) {
        if(IsHtml(d)) output(d,d->player,1,2,0,"%s","<TR><TD COLSPAN=4 CELLPADDING=0 BGCOLOR="HTML_TABLE_GREY"><TABLE BORDER WIDTH=100% CELLPADDING=4 BGCOLOR="HTML_TABLE_MGREY">");
	   else output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','='));
        output(d,d->player,2,1,0,"%s",userlist_users(d->player,"",deities,elders,delders,wizards,druids,apprentices,dapprentices,retired,dretired,experienced,assistants,builders,mortals,beings,puppets,morons,idle,1,IsHtml(d)));
        output(d,d->player,2,1,10,"%s",userlist_peak(1,IsHtml(d)));
        output(d,d->player,2,1,10,"%s",userlist_uptime(now,1,IsHtml(d)));
        wrap_leading = 0;
     }

     if(IsHtml(d)) output(d,d->player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
     command_type &= ~COMM_CMD;
     html_anti_reverse(d,0);
}

/* ---->  Short user list (Multiple columns):  Name only  <---- */
void userlist_swho(struct descriptor_data *d)
{
     short         deities = 0,elders = 0,delders = 0,wizards = 0,druids = 0,apprentices = 0,dapprentices = 0,retired = 0,dretired = 0,experienced = 0,assistants = 0,builders = 0,mortals = 0,beings = 0,puppets = 0,morons = 0,idle = 0;
     int           flags,flags2,width,fother,counter = 0;
     unsigned char cached_scrheight,scrheight_adjust = 9;
     char          wchar = ' ',buffer[32];
     const    char *p1;
     time_t        now;

     gettime(now);
     html_anti_reverse(d,1);
     if(!d->pager && !IsHtml(d) && Validchar(d->player) && More(d->player)) pager_init(d);
     if(grp->condition != 206) {
        if(grp->condition != 204) {
           if(grp->condition != 208) {
              if(grp->object_type != NOTHING) {
                 if(!in_command) {
                    dbref operator = comms_chat_operator(grp->object_type);
                    *scratch_return_string = '\0';
                    if((operator != NOTHING) && (grp->object_type > 0)) {
                       if(db[operator].flags2 & CHAT_PRIVATE) strcpy(scratch_return_string," (PRIVATE)");
                       sprintf(scratch_return_string + strlen(scratch_return_string)," (Operator:  %s"ANSI_LWHITE"%s"ANSI_LCYAN")",Article(operator,UPPER,INDEFINITE),getcname(NOTHING,operator,0,0));
		    }
                    sprintf(scratch_buffer,"%sPeople using chatting channel "ANSI_LWHITE"%s"ANSI_LCYAN"%s...%s",IsHtml(d) ? "\016<TR ALIGN=CENTER><TH COLSPAN=4 BGCOLOR="HTML_TABLE_CYAN"><FONT COLOR="HTML_LCYAN" SIZE=4><I>\016":"\n ",comms_chat_channelname(grp->object_type),scratch_return_string,IsHtml(d) ? "\016</I></FONT></TH></TR>\016":"\n");
		 }
                 scrheight_adjust = 10;
	      } else if(!in_command) sprintf(scratch_buffer,"%sThe following people are connected to %s...%s",IsHtml(d) ? "\016<TR ALIGN=CENTER><TH COLSPAN=4 BGCOLOR="HTML_TABLE_CYAN"><FONT COLOR="HTML_LCYAN" SIZE=4><I>\016":"\n ",tcz_full_name,IsHtml(d) ? "\016</I></FONT></TH></TR>\016":"\n");
	   } else if(!in_command) sprintf(scratch_buffer,"%sThe following people need welcoming to %s...%s",IsHtml(d) ? "\016<TR ALIGN=CENTER><TH COLSPAN=4 BGCOLOR="HTML_TABLE_CYAN"><FONT COLOR="HTML_LCYAN" SIZE=4><I>\016":"\n ",tcz_full_name,IsHtml(d) ? "\016</I></FONT></TH></TR>\016":"\n");
        } else if(!in_command) sprintf(scratch_buffer,"%sThe following friends of yours are currently connected...%s",IsHtml(d) ? "\016<TR ALIGN=CENTER><TH COLSPAN=4 BGCOLOR="HTML_TABLE_CYAN"><FONT COLOR="HTML_LCYAN" SIZE=4><I>\016":"\n ",IsHtml(d) ? "\016</I></FONT></TH></TR>\016":"\n");
     }

     if(IsHtml(d)) {
        width = 4;
     } else if(d->terminal_width > 1) {
        width = (d->terminal_width - 1) / ((grp->condition == 204) ? 25:23);
     } else width = 3;

     if(Validchar(d->player)) {
        cached_scrheight                     = db[d->player].data->player.scrheight;
        db[d->player].data->player.scrheight = ((db[d->player].data->player.scrheight - scrheight_adjust) * width) * 2;
     }
     union_initgrouprange((union group_data *) descriptor_list);

     /* ---->  'where *<NAME>' on connected user  <---- */
     if(grp->condition == 206) {
        if(!in_command || (grp->rangeitems == 0)) {
           if(!((Secret(grp->object_who) || Secret(db[grp->object_who].location)) && !can_write_to(d->player,db[grp->object_who].location,1) && !can_write_to(d->player,grp->object_who,1))) {
              dbref area = get_areaname_loc(db[grp->object_who].location);
              if(Valid(area) && !Blank(getfield(area,AREANAME))) sprintf(scratch_return_string," in "ANSI_LYELLOW"%s%s",getfield(area,AREANAME),(grp->rangeitems == 0) ? ANSI_LGREEN:ANSI_LCYAN);
   	         else *scratch_return_string = '\0';
              sprintf(scratch_buffer,ANSI_LWHITE"%s%s%s%s is ",(grp->rangeitems == 0) ? "":"\n ",Article(grp->object_who,UPPER,DEFINITE),getcname(NOTHING,grp->object_who,0,0),(grp->rangeitems == 0) ? ANSI_LGREEN:ANSI_LCYAN);
              sprintf(scratch_buffer + strlen(scratch_buffer),"in %s"ANSI_LYELLOW"%s%s%s%s\n",Article(db[grp->object_who].location,LOWER,INDEFINITE),unparse_object(d->player,db[grp->object_who].location,0),(grp->rangeitems == 0) ? ANSI_LGREEN:ANSI_LCYAN,scratch_return_string,(grp->rangeitems == 0) ? ".":" with the following people...");
	   } else {
              sprintf(scratch_buffer,ANSI_LWHITE"%s%s"ANSI_LGREEN" is hiding in a secret location.\n",Article(grp->object_who,UPPER,DEFINITE),getcname(NOTHING,grp->object_who,0,0));
              if(Validchar(d->player)) db[d->player].data->player.scrheight = cached_scrheight;
              output(d,d->player,2,1,0,"%s",scratch_buffer);
              return;
	   }
           if(grp->rangeitems == 0) {
              if(Validchar(d->player)) db[d->player].data->player.scrheight = cached_scrheight;
              output(d,d->player,2,1,0,"%s",scratch_buffer);
              return;
	   }
	}
        scrheight_adjust = 10;
     }

     if(IsHtml(d))
        output(d,d->player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");

     if(!in_command) {
        output(d,d->player,2,1,0,"%s",scratch_buffer);
        if(!IsHtml(d)) output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','='));
     }

     if(IsHtml(d)) strcpy(scratch_buffer,"\016<TR ALIGN=LEFT>\016");
        else *scratch_buffer = '\0';

     while(union_grouprange()) {
           if(++counter > width) {
              strcat(scratch_buffer,IsHtml(d) ? "\016</TR>\016":"\n");
              output(d,d->player,2,1,0,"%s",scratch_buffer);
              if(IsHtml(d)) strcpy(scratch_buffer,"\016<TR ALIGN=LEFT>\016");
                 else *scratch_buffer = '\0';
              counter = 1;
	   }

           if(grp->condition != 208) {
              if(Moron(grp->cunion->descriptor.player)) wchar = '!';
                 else if(Level2(grp->cunion->descriptor.player)) wchar = '@';
  	            else if(Level3(grp->cunion->descriptor.player)) wchar = '*';
                       else if(Level4(grp->cunion->descriptor.player)) wchar = '~';
                          else if(Experienced(grp->cunion->descriptor.player)) wchar = '=';
                             else if(Assistant(grp->cunion->descriptor.player)) wchar = '+';
                                else if(Puppet(grp->cunion->descriptor.player)) wchar = '`';
         	                   else wchar = ' ';
	   }

           sprintf(scratch_buffer + strlen(scratch_buffer),"%s%s",IsHtml(d) ? "\016<TD WIDTH=25%>\016":"",(char *) privilege_countcolour(grp->cunion->descriptor.player,&deities,&elders,&delders,&wizards,&druids,&apprentices,&dapprentices,&retired,&dretired,&experienced,&assistants,&builders,&mortals,&beings,&puppets,&morons));
           if((now - grp->cunion->descriptor.last_time) >= MINUTE) idle++;
           if(grp->condition == 204) {
              if(!(flags = friend_flags(grp->player,grp->cunion->descriptor.player))) {
                 flags  = FRIEND_STANDARD;
                 fother = 1;
	      } else fother = 0;
              if(!(flags2 = friend_flags(grp->cunion->descriptor.player,grp->player))) flags2 = FRIEND_STANDARD;
              p1 = getfield(grp->cunion->descriptor.player,PREFIX);
              if(!Blank(p1) && ((strlen(p1) + 1 + strlen(getname(grp->cunion->descriptor.player))) <= 20))
                 sprintf(scratch_return_string,"%s %s",p1,getname(grp->cunion->descriptor.player));
                    else strcpy(scratch_return_string,getname(grp->cunion->descriptor.player));

              sprintf(buffer,"%s%c%s%s%s",IsHtml(d) ? "":" ",wchar,((flags|flags2) & FRIEND_ENEMY) ? "{":(!(flags & FRIEND_FCHAT) || !(flags2 & FRIEND_FCHAT) || !(db[grp->cunion->descriptor.player].flags2 & FRIENDS_CHAT)) ? "<":(fother) ? "[":"",scratch_return_string,((flags|flags2) & FRIEND_ENEMY) ? "}":(!(flags & FRIEND_FCHAT) || !(flags2 & FRIEND_FCHAT) || !(db[grp->cunion->descriptor.player].flags2 & FRIENDS_CHAT)) ? ">":(fother) ? "]":"");
              sprintf(scratch_return_string,IsHtml(d) ? "%s\016</TD>\016":"%-25s",buffer);
	   } else {
              p1 = getfield(grp->cunion->descriptor.player,PREFIX);
              if(!Blank(p1) && ((strlen(p1) + 1 + strlen(getname(grp->cunion->descriptor.player))) <= 20))
                 sprintf(buffer,"%s %s",p1,getname(grp->cunion->descriptor.player));
                    else strcpy(buffer,getname(grp->cunion->descriptor.player));
              sprintf(scratch_return_string,IsHtml(d) ? "%s%c%s\016</TD>\016":"%s%c%-21s",(!IsHtml(d) && (grp->condition == 208)) ? "":" ",wchar,buffer);
	   }
           strcat(scratch_buffer,scratch_return_string);
     }
     if(Validchar(d->player)) db[d->player].data->player.scrheight = cached_scrheight;

     if(counter != 0) {
        if(IsHtml(d)) while(++counter <= width) strcat(scratch_buffer,"\016<TD WIDTH=25%>&nbsp;</TD>\016");
        strcat(scratch_buffer,IsHtml(d) ? "\016</TR>\016":"\n");
        output(d,d->player,2,1,0,"%s",scratch_buffer);
     }

     if(grp->rangeitems == 0)
        output(d,d->player,2,1,1,"%s",IsHtml(d) ? "\016<TR ALIGN=CENTER><TD COLSPAN=4>"ANSI_LCYAN"<I>*** &nbsp; NO ONE LISTED &nbsp; ***</I></TD></TR>\016":" ***  NO ONE LISTED  ***\n");

     if(!in_command) {
        if(grp->condition == 208) {
           if(!IsHtml(d)) output(d,d->player,2,1,0,separator(d->terminal_width,1,'-','-'));
           output(d,d->player,2,1,1,"%sTo welcome one of the above users, simply type '"ANSI_LGREEN"welcome <NAME>"ANSI_LWHITE"'.%s",IsHtml(d) ? "\016<TR ALIGN=CENTER><TD COLSPAN=4 BGCOLOR="HTML_TABLE_GREY">"ANSI_LWHITE"<I>\016":ANSI_LWHITE" ",IsHtml(d) ? "\016</I></TD></TR>\016":"\n");
           if(!IsHtml(d)) output(d,d->player,2,1,0,separator(d->terminal_width,1,'-','='));
           output(d,d->player,2,1,1,"%sUsers who need to be welcomed: %s "ANSI_DWHITE"%s%s",IsHtml(d) ? "\016<TR ALIGN=CENTER><TD COLSPAN=4 BGCOLOR="HTML_TABLE_MGREY">"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",IsHtml(d) ? "\016&nbsp;\016":"",listed_items(scratch_return_string,1),IsHtml(d) ? "\016</B></TD></TR>\016":"\n\n");
	} else {
           if(IsHtml(d)) output(d,d->player,1,2,0,"<TR><TD COLSPAN=4 CELLPADDING=0 BGCOLOR="HTML_TABLE_GREY"><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_MGREY">");
	      else output(d,d->player,2,1,0,separator(d->terminal_width,1,'-','='));
           output(d,d->player,2,1,0,"%s",(char *) userlist_users(d->player,((grp->condition == 204) || (grp->condition == 206)) ? " listed":"",deities,elders,delders,wizards,druids,apprentices,dapprentices,retired,dretired,experienced,assistants,builders,mortals,beings,puppets,morons,idle,0,IsHtml(d)));
           output(d,d->player,2,1,9,"%s",(char *) userlist_peak(0,IsHtml(d)));
           output(d,d->player,2,1,9,"%s",(char *) userlist_uptime(now,0,IsHtml(d)));
	}
     }

     if(IsHtml(d)) output(d,d->player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
     html_anti_reverse(d,0);
}

/* ---->  Host list (Single column):  Login time, name & host logged in from  <---- */
void userlist_hosts(struct descriptor_data *d)
{
     int           local = 0,remote = 0,telnet = 0,html = 0,length;
     unsigned char cached_scrheight;
     const    char *colour;
     char          *p1,*p2;
     time_t        now;

     gettime(now);
     html_anti_reverse(d,1);
     if(!d->pager && !IsHtml(d) && Validchar(d->player) && More(d->player)) pager_init(d);
     if(IsHtml(d)) output(d,d->player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");

     if(!in_command){
        if(!IsHtml(d)) {
           output(d,d->player,0,1,0,"\n Time:    Name:                 Host:");
           output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','='));
	} else output(d,d->player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH WIDTH=15%%><FONT COLOR="HTML_LCYAN" SIZE=4><I>Time:</I></FONT></TH><TH WIDTH=25%%><FONT COLOR="HTML_LCYAN" SIZE=4><I>Name:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Host:</I></FONT></TH></TR>\016");
     }

     if(Validchar(d->player)) {
        cached_scrheight                      = db[d->player].data->player.scrheight;
        db[d->player].data->player.scrheight -= 8;
     }

     union_initgrouprange((union group_data *) descriptor_list);
     while(union_grouprange()) {

           /* ---->  Connect time and character's name  <---- */
           sprintf(scratch_buffer,"%s%s",IsHtml(d) ? "\016<TR ALIGN=LEFT><TD ALIGN=CENTER WIDTH=15%>\016":"",colour = privilege_colour(grp->cunion->descriptor.player));
           sprintf(p2 = (scratch_buffer + strlen(scratch_buffer)),IsHtml(d) ? "%s\016</TD><TD WIDTH=25%%>\016%s":" %s%s  ",(char *) userlist_shorttime(grp->cunion->descriptor.start_time,now,scratch_return_string,1),IsHtml(d) ? colour:"");
           p1 = (char *) getfield(grp->cunion->descriptor.player,PREFIX);
           if(!Blank(p1) && ((strlen(p1) + 1 + strlen(getname(grp->cunion->descriptor.player))) <= 20))
              sprintf(scratch_buffer + strlen(scratch_buffer),"%s %s",p1,getname(grp->cunion->descriptor.player));
                 else strcat(scratch_buffer,getname(grp->cunion->descriptor.player));

           if(!IsHtml(d)) {
              if((length = strlen(p2)) <= 30) {
                 for(p1 = p2 + length; length < 30; *p1++ = ' ', length++);
                 *p1 = '\0';
	      } else p2[30] = '\0';
	   }
	    
           /* ---->  Host character's currently connected from  <---- */
           sprintf(scratch_buffer + strlen(scratch_buffer),IsHtml(d) ? "\016</TD><TD>\016%s%s%s\016</TD></TR>\016":"  %s%s%s\n",colour,(grp->cunion->descriptor.html) ? (IsHtml(d) ? "\016<I>(HTML)</I> &nbsp; \016":"(HTML)  "):(IsHtml(d) ? "\016<I>(TELNET)</I> &nbsp; \016":"(TELNET)  "),(!Blank(grp->cunion->descriptor.hostname)) ? grp->cunion->descriptor.hostname:"<Unknown>");
           if(((grp->cunion->descriptor.address & tcz_server_netmask) == tcz_server_network) || (grp->cunion->descriptor.address == 0x7F000001)) local++;
              else remote++;
           if(grp->cunion->descriptor.html) html++;
              else telnet++;
           output(d,d->player,2,1,32,"%s",scratch_buffer);
     }
     if(Validchar(d->player)) db[d->player].data->player.scrheight = cached_scrheight;

     if(grp->rangeitems == 0) output(d,d->player,2,1,1,"%s",IsHtml(d) ? "\016<TR ALIGN=CENTER><TD COLSPAN=3>"ANSI_LCYAN"<I>*** &nbsp; NO ONE LISTED &nbsp; ***</I></TD></TR>\016":" ***  NO ONE LISTED  ***\n");
     if(!in_command) {
        if(IsHtml(d)) output(d,d->player,1,2,0,"<TR><TD COLSPAN=3 CELLPADDING=0 BGCOLOR="HTML_TABLE_GREY"><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_MGREY">");
	   else output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','='));
        listed_items(scratch_return_string,1);
        output(d,d->player,2,1,10,"%s"ANSI_LYELLOW""ANSI_UNDERLINE"%s"ANSI_DCYAN" %s ("ANSI_LWHITE"%d "ANSI_LCYAN"local"ANSI_DCYAN", "ANSI_LWHITE"%d "ANSI_LCYAN"remote"ANSI_DCYAN", "ANSI_LWHITE"%d "ANSI_LCYAN"Telnet, "ANSI_LWHITE"%d "ANSI_LCYAN"HTML.)%s",IsHtml(d) ? "\016<TR><TH ALIGN=CENTER WIDTH=15% BGCOLOR="HTML_TABLE_CYAN">"ANSI_LWHITE"<B><I>Users:</I></B></TH><TD ALIGN=LEFT>\016":ANSI_LWHITE"  Users:  ",scratch_return_string,IsHtml(d) ? "\016&nbsp;\016":"",local,remote,telnet,html,IsHtml(d) ? "\016</TD></TR>\016":"\n");
        output(d,d->player,2,1,10,"%s",(char *) userlist_peak(1,IsHtml(d)));
        output(d,d->player,2,1,10,"%s",(char *) userlist_uptime(now,1,IsHtml(d)));
     }

     if(IsHtml(d)) output(d,d->player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
     html_anti_reverse(d,0);
}

/* ---->  Where list (Single column):  Name & current location.  <---- */
void userlist_where(struct descriptor_data *d,int lwho)
{
     short         deities = 0,elders = 0,delders = 0,wizards = 0,druids = 0,apprentices = 0,dapprentices = 0,retired = 0,dretired = 0,experienced = 0,assistants = 0,builders = 0,mortals = 0,beings = 0,puppets = 0,morons = 0,idle = 0;
     unsigned char cached_scrheight,scrheight_adjust = 9;
     int           length,flags,flags2,fother,aname = 0;
     char          *ptr,*p2;
     const    char *colour;
     dbref         area;
     time_t        now;

     gettime(now);
     html_anti_reverse(d,1);
     if(!d->pager && !IsHtml(d) && Validchar(d->player) && More(d->player)) pager_init(d);
     if(IsHtml(d)) output(d,d->player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");

     if(lwho) {
        if(!in_command) {
           area = get_areaname_loc(db[d->player].location);
           if(Valid(area) && !Blank(getfield(area,AREANAME))) {
              sprintf(scratch_return_string," in "ANSI_LWHITE"%s"ANSI_LGREEN,getfield(area,AREANAME));
              aname = 1;
	   } else *scratch_return_string = '\0';
           output(d,d->player,2,1,0,"%sPeople in locations%s owned by %s"ANSI_LWHITE"%s"ANSI_LGREEN"...%s",IsHtml(d) ? "\016<TR ALIGN=CENTER><TH COLSPAN=2 BGCOLOR="HTML_TABLE_GREEN"><FONT COLOR="HTML_LGREEN" SIZE=4>\016":"\n"ANSI_LGREEN,scratch_return_string,Article(db[db[d->player].location].owner,LOWER,DEFINITE),getcname(d->player,db[db[d->player].location].owner,1,0),IsHtml(d) ? "\016</FONT></TH></TR>\016":"\n");
	}
        scrheight_adjust = 12;
     } else if(grp->condition == 204) {
        if(!in_command) output(d,d->player,2,1,0,ANSI_LGREEN"%sYour friends are in the following locations...%s",IsHtml(d) ? "\016<TR ALIGN=CENTER><TH COLSPAN=2 BGCOLOR="HTML_TABLE_GREEN"><FONT COLOR="HTML_LGREEN" SIZE=4>\016":"\n",IsHtml(d) ? "\016</FONT></TH></TR>\016":"\n");
        scrheight_adjust = 11;
     }

     if(!in_command) {
        if(!IsHtml(d)) {
           output(d,d->player,0,1,0,"\n Name:                 %sLocation:",(grp->condition == 204) ? "  ":"");
           output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','='));
	} else output(d,d->player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH WIDTH=25%%><FONT COLOR="HTML_LCYAN" SIZE=4><I>Name:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Location:</I></FONT></TH></TR>\016");
     }

     if(Validchar(d->player)) {
        cached_scrheight                     = db[d->player].data->player.scrheight;
        db[d->player].data->player.scrheight = (db[d->player].data->player.scrheight - scrheight_adjust) * 2;
     }

     union_initgrouprange((union group_data *) descriptor_list);
     while(union_grouprange()) {

           /* ---->  Character's name  <---- */
           sprintf(scratch_buffer,"%s%s",IsHtml(d) ? "\016<TR ALIGN=LEFT><TD WIDTH=25%>\016":"",colour = privilege_countcolour(grp->cunion->descriptor.player,&deities,&elders,&delders,&wizards,&druids,&apprentices,&dapprentices,&retired,&dretired,&experienced,&assistants,&builders,&mortals,&beings,&puppets,&morons));
           if((now - grp->cunion->descriptor.last_time) >= MINUTE) idle++;
           if(grp->condition == 204) {
              if(!(flags = friend_flags(grp->player,grp->cunion->descriptor.player))) {
                 flags  = FRIEND_STANDARD;
                 fother = 1;
	      } else fother = 0;
              if(!(flags2 = friend_flags(grp->cunion->descriptor.player,grp->player))) flags2 = FRIEND_STANDARD;
              ptr = (char *) getfield(grp->cunion->descriptor.player,PREFIX);
              if(!Blank(ptr) && ((strlen(ptr) + 1 + strlen(getname(grp->cunion->descriptor.player))) <= 20))
                 sprintf(scratch_return_string,"%s %s",ptr,getname(grp->cunion->descriptor.player));
                    else strcpy(scratch_return_string,getname(grp->cunion->descriptor.player));
              sprintf(p2 = (scratch_buffer + strlen(scratch_buffer))," %s%s%s",((flags|flags2) & FRIEND_ENEMY) ? "{":(!(flags & FRIEND_FCHAT) || !(flags2 & FRIEND_FCHAT) || !(db[grp->cunion->descriptor.player].flags2 & FRIENDS_CHAT)) ? "<":(fother) ? "[":"",scratch_return_string,((flags|flags2) & FRIEND_ENEMY) ? "}":(!(flags & FRIEND_FCHAT) || !(flags2 & FRIEND_FCHAT) || !(db[grp->cunion->descriptor.player].flags2 & FRIENDS_CHAT)) ? ">":(fother) ? "]":"");
	   } else {
              ptr = (char *) getfield(grp->cunion->descriptor.player,PREFIX);
              if(!Blank(ptr) && ((strlen(ptr) + 1 + strlen(getname(grp->cunion->descriptor.player))) <= 20))
                 sprintf(p2 = (scratch_buffer + strlen(scratch_buffer))," %s %s",ptr,getname(grp->cunion->descriptor.player));
                    else sprintf(p2 = (scratch_buffer + strlen(scratch_buffer))," %s",getname(grp->cunion->descriptor.player));
	   }

           if(!IsHtml(d)) {
              if((length = strlen(p2)) <= ((grp->condition == 204) ? 23:21)) {
                 for(ptr = p2 + length; length < ((grp->condition == 204) ? 23:21); *ptr++ = ' ', length++);
                 *ptr = '\0';
	      } else p2[(grp->condition == 204) ? 23:21] = '\0';
              strcat(scratch_buffer,"  ");
	   }

           /* ---->  Location  <---- */
           if(!((Secret(grp->cunion->descriptor.player) || Secret(db[grp->cunion->descriptor.player].location)) && !can_write_to(d->player,db[grp->cunion->descriptor.player].location,1) && !can_write_to(d->player,grp->cunion->descriptor.player,1))) {
              strcpy(scratch_return_string,unparse_object(d->player,db[grp->cunion->descriptor.player].location,UPPER|INDEFINITE));
              if(Valid(db[grp->cunion->descriptor.player].location)) {
                 if(!(lwho && aname)) {

                    /* ---->  Area name of location (If it has one)  <---- */
                    area = get_areaname_loc(db[grp->cunion->descriptor.player].location);
                    if(Valid(area) && !Blank(getfield(area,AREANAME)))
                       sprintf(scratch_return_string + strlen(scratch_return_string)," in %s",getfield(area,AREANAME));
		 }
	      } else strcpy(scratch_return_string,"Unknown");
              ptr = punctuate(scratch_return_string,1,'.');
              sprintf(scratch_buffer + strlen(scratch_buffer),"%s%s%s%s",IsHtml(d) ? "\016</TD><TD>\016":"",colour,ptr,IsHtml(d) ? "\016</TD></TR>\016":"\n");
	   } else sprintf(scratch_buffer + strlen(scratch_buffer),"%s%sHiding in a secret location.%s",IsHtml(d) ? "\016</TD><TD>\016":"",colour,IsHtml(d) ? "\016</TD></TR>\016":"\n");
           output(d,d->player,2,1,(grp->condition == 204) ? 25:23,"%s",scratch_buffer);
     }
     if(Validchar(d->player)) db[d->player].data->player.scrheight = cached_scrheight;

     if(grp->rangeitems == 0) output(d,d->player,2,1,1,"%s",IsHtml(d) ? "\016<TR ALIGN=CENTER><TD COLSPAN=2>"ANSI_LCYAN"<I>*** &nbsp; NO ONE LISTED &nbsp; ***</I></TD></TR>\016":" ***  NO ONE LISTED  ***\n");
     if(!in_command) {
        if(IsHtml(d)) output(d,d->player,1,2,0,"<TR><TD COLSPAN=2 CELLPADDING=0 BGCOLOR="HTML_TABLE_GREY"><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_MGREY">");
	   else output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','='));

        if(grp->condition == 204) output(d,d->player,2,1,0,"%s",(char *) userlist_users(d->player," listed",deities,elders,delders,wizards,druids,apprentices,dapprentices,retired,dretired,experienced,assistants,builders,mortals,beings,puppets,morons,idle,0,IsHtml(d)));
           else output(d,d->player,2,1,0,"%s",(char *) userlist_users(d->player,(lwho) ? " found":"",deities,elders,delders,wizards,druids,apprentices,dapprentices,retired,dretired,experienced,assistants,builders,mortals,beings,puppets,morons,idle,0,IsHtml(d)));
        output(d,d->player,2,1,9,"%s",(char *) userlist_peak(0,IsHtml(d)));
        output(d,d->player,2,1,9,"%s",(char *) userlist_uptime(now,0,IsHtml(d)));
     }

     if(IsHtml(d)) output(d,d->player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
     html_anti_reverse(d,0);
}

/* ---->  E-mail addresses list (Single column):  Name & E-mail address (Admin only)  <---- */
void userlist_email(struct descriptor_data *d,int number)
{
     short         deities = 0,elders = 0,delders = 0,wizards = 0,druids = 0,apprentices = 0,dapprentices = 0,retired = 0,dretired = 0,experienced = 0,assistants = 0,builders = 0,mortals = 0,beings = 0,puppets = 0,morons = 0,idle = 0;
     unsigned char cached_scrheight;
     const    char *colour,*ptr;
     char          *p1,*p2;
     int           length;
     time_t        now;

     gettime(now);
     html_anti_reverse(d,1);
     if(!d->pager && !IsHtml(d) && Validchar(d->player) && More(d->player)) pager_init(d);
     if(IsHtml(d)) output(d,d->player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");

     if(!in_command) {
        if(!IsHtml(d)) {
           output(d,d->player,0,1,0,"\n Name:                 %s (%s) E-mail address:",rank(number),(number == 2) ? "Private":"Public");
           output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','='));
	} else output(d,d->player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH WIDTH=25%%><FONT COLOR="HTML_LCYAN" SIZE=4><I>Name:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>E-mail address:</I></FONT></TH></TR>\016");
     }

     if(Validchar(d->player)) {
        cached_scrheight                     = db[d->player].data->player.scrheight;
        db[d->player].data->player.scrheight = (db[d->player].data->player.scrheight - 9) * 2;
     }
     union_initgrouprange((union group_data *) descriptor_list);
     while(union_grouprange()) {

           /* ---->  Character's name  <---- */
           sprintf(scratch_buffer,"%s%s",IsHtml(d) ? "\016<TR ALIGN=LEFT><TD WIDTH=25%%>\016":"",colour = privilege_countcolour(grp->cunion->descriptor.player,&deities,&elders,&delders,&wizards,&druids,&apprentices,&dapprentices,&retired,&dretired,&experienced,&assistants,&builders,&mortals,&beings,&puppets,&morons));
           if((now - grp->cunion->descriptor.last_time) >= MINUTE) idle++;
           p1 = (char *) getfield(grp->cunion->descriptor.player,PREFIX);
           if(!Blank(p1) && ((strlen(p1) + 1 + strlen(getname(grp->cunion->descriptor.player))) <= 20))
              sprintf(p2 = (scratch_buffer + strlen(scratch_buffer))," %s %s",p1,getname(grp->cunion->descriptor.player));
                 else sprintf(p2 = (scratch_buffer + strlen(scratch_buffer))," %s",getname(grp->cunion->descriptor.player));

           if(!IsHtml(d)) {
              if((length = strlen(p2)) <= 21) {
                 for(p1 = p2 + length; length < 21; *p1++ = ' ', length++);
                 *p1 = '\0';
	      } else p2[21] = '\0';
	   }

           /* ---->  Character's E-mail address  <---- */
           output(d,d->player,2,1,23,"%s%s%s%s%s",scratch_buffer,IsHtml(d) ? "\016</TD><TD>\016":"  ",IsHtml(d) ? colour:"",(ptr = gettextfield(number,'\n',getfield(grp->cunion->descriptor.player,EMAIL),0,scratch_return_string)) ? ptr:"E-mail address not set.",IsHtml(d) ? "\016</TD></TR>\016":"\n");
     }
     if(Validchar(d->player)) db[d->player].data->player.scrheight = cached_scrheight;

     if(grp->rangeitems == 0) output(d,d->player,2,1,1,"%s",IsHtml(d) ? "\016<TR ALIGN=CENTER><TD COLSPAN=2>"ANSI_LCYAN"<I>*** &nbsp; NO ONE LISTED &nbsp; ***</I></TD></TR>\016":" ***  NO ONE LISTED  ***\n");
     if(!in_command) {
        if(IsHtml(d)) output(d,d->player,1,2,0,"<TR><TD COLSPAN=2 CELLPADDING=0 BGCOLOR="HTML_TABLE_GREY"><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_MGREY">");
	   else output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','='));

        output(d,d->player,2,1,9,"%s",(char *) userlist_users(d->player," listed",deities,elders,delders,wizards,druids,apprentices,dapprentices,retired,dretired,experienced,assistants,builders,mortals,beings,puppets,morons,idle,0,IsHtml(d)));
        output(d,d->player,2,1,9,"%s",(char *) userlist_peak(0,IsHtml(d)));
        output(d,d->player,2,1,9,"%s",(char *) userlist_uptime(now,0,IsHtml(d)));
     }

     if(IsHtml(d)) output(d,d->player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
     html_anti_reverse(d,0);
}

/* ---->  Last commands list (Single column):  Name & last command typed (Admin only)  <---- */
void userlist_last(struct descriptor_data *d)
{
     short         deities = 0,elders = 0,delders = 0,wizards = 0,druids = 0,apprentices = 0,dapprentices = 0,retired = 0,dretired = 0,experienced = 0,assistants = 0,builders = 0,mortals = 0,beings = 0,puppets = 0,morons = 0,idle = 0;
     unsigned char cached_scrheight;
     const    char *colour;
     char          *p1,*p2;
     int           length;
     time_t        now;

     gettime(now);
     html_anti_reverse(d,1);
     if(!d->pager && !IsHtml(d) && Validchar(d->player) && More(d->player)) pager_init(d);
     if(IsHtml(d)) output(d,d->player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");

     if(!in_command) {
        if(!IsHtml(d)) {
           output(d,d->player,0,1,0,"\n Name:                 Last command:");
           output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','='));
	} else output(d,d->player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH WIDTH=25%%><FONT COLOR="HTML_LCYAN" SIZE=4><I>Name:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Last command:</I></FONT></TH></TR>\016");
     }

     if(Validchar(d->player)) {
        cached_scrheight                     = db[d->player].data->player.scrheight;
        db[d->player].data->player.scrheight = (db[d->player].data->player.scrheight - 9) * 2;
     }
     union_initgrouprange((union group_data *) descriptor_list);
     while(union_grouprange()) {

           /* ---->  Character's name  <---- */
           sprintf(scratch_buffer,"%s%s",IsHtml(d) ? "\016<TR><TD ALIGN=LEFT WIDTH=25%>\016":"",colour = privilege_countcolour(grp->cunion->descriptor.player,&deities,&elders,&delders,&wizards,&druids,&apprentices,&dapprentices,&retired,&dretired,&experienced,&assistants,&builders,&mortals,&beings,&puppets,&morons));
           if((now - grp->cunion->descriptor.last_time) >= MINUTE) idle++;
           p1 = (char *) getfield(grp->cunion->descriptor.player,PREFIX);
           if(!Blank(p1) && ((strlen(p1) + 1 + strlen(getname(grp->cunion->descriptor.player))) <= 20))
              sprintf(p2 = (scratch_buffer + strlen(scratch_buffer))," %s %s",p1,getname(grp->cunion->descriptor.player));
                 else sprintf(p2 = (scratch_buffer + strlen(scratch_buffer))," %s",getname(grp->cunion->descriptor.player));

           if(!IsHtml(d)) {
              if((length = strlen(p2)) <= 21) {
                 for(p1 = p2 + length; length < 21; *p1++ = ' ', length++);
                 *p1 = '\0';
	      } else p2[21] = '\0';
	   }

           /* ---->  Last command typed by character  <---- */
           sprintf(scratch_buffer + strlen(scratch_buffer),"%s%s",IsHtml(d) ? "\016</TD><TD>\016":"  ",IsHtml(d) ? colour:"");
           if(grp->cunion->descriptor.prompt) strcat(scratch_buffer,IsHtml(d) ? "\016<I>(@prompt)</I> &nbsp;\016":"(@prompt) ");
              else if(grp->cunion->descriptor.edit) strcat(scratch_buffer,IsHtml(d) ? "\016<I>(Editor)</I> &nbsp; \016":"(Editor) ");

           if((grp->player == grp->cunion->descriptor.player) || grp->cunion->descriptor.monitor || (Validchar(grp->cunion->descriptor.player) && (grp->player == Controller(grp->cunion->descriptor.player))) || !((grp->cunion->descriptor.flags & SPOKEN_TEXT) || ((grp->cunion->descriptor.flags & ABSOLUTE) && !(grp->cunion->descriptor.flags2 & ABSOLUTE_OVERRIDE)))) {
              if(strlen(decompress(grp->cunion->descriptor.last_command)) > 512) {
                 strncat(scratch_buffer,decompress(grp->cunion->descriptor.last_command),512);
                 strcat(scratch_buffer,ANSI_DCYAN"...");
	      } else strcat(scratch_buffer,decompress(grp->cunion->descriptor.last_command));
	   } else strcat(scratch_buffer,ANSI_DRED"("ANSI_LRED"Privacy Upheld"ANSI_DRED")");
           output(d,d->player,2,1,23,"%s%s%s",scratch_buffer,(grp->cunion->descriptor.monitor && Validchar(grp->cunion->descriptor.monitor->player) && Level4(d->player) && (d->player != grp->cunion->descriptor.player)) ? " \016&nbsp;\016 "ANSI_DBLUE"("ANSI_LBLUE"Monitored"ANSI_DBLUE")":"",IsHtml(d) ? "\016</TD></TR>\016":"\n");
     }

     if(Validchar(d->player))
        db[d->player].data->player.scrheight = cached_scrheight;

     if(grp->rangeitems == 0) output(d,d->player,2,1,1,"%s",IsHtml(d) ? "\016<TR ALIGN=CENTER><TD COLSPAN=2>"ANSI_LCYAN"<I>*** &nbsp; NO ONE OF A LOWER LEVEL THAN YOU LISTED &nbsp; ***</I></TD></TR>\016":" ***  NO ONE OF A LOWER LEVEL THAN YOU LISTED  ***\n");
     if(!in_command) {
        if(IsHtml(d)) output(d,d->player,1,2,0,"<TR><TD COLSPAN=2 CELLPADDING=0 BGCOLOR="HTML_TABLE_GREY"><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_MGREY">");
	   else output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','='));

        output(d,d->player,2,1,0,"%s",(char *) userlist_users(d->player," listed",deities,elders,delders,wizards,druids,apprentices,dapprentices,retired,dretired,experienced,assistants,builders,mortals,beings,puppets,morons,idle,0,IsHtml(d)));
        output(d,d->player,2,1,9,"%s",(char *) userlist_peak(0,IsHtml(d)));
        output(d,d->player,2,1,9,"%s",(char *) userlist_uptime(now,0,IsHtml(d)));
     }

     if(IsHtml(d)) output(d,d->player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
     html_anti_reverse(d,0);
}

/* ---->  Chatting channel list (Single column):  Channel, operator, users & public/private/subject  <---- */
void userlist_channels(struct descriptor_data *d)
{
     int                      channels = 0,users = 0,count;
     unsigned char            cached_scrheight;
     const    char            *colour,*ptr;
     char                     buffer[32];
     struct   descriptor_data *c;

     html_anti_reverse(d,1);
     if(!d->pager && !IsHtml(d) && Validchar(d->player) && More(d->player)) pager_init(d);
     if(IsHtml(d)) output(d,d->player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");

     if(!in_command) {
        if(!IsHtml(d)) {
           output(d,d->player,0,1,0,"\n Channel:     Operator:             Users:  Public/Private/Subject:");
           output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','='));
	} else output(d,d->player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Channel:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Operator:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Users:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Public/Private/Subject:</I></FONT></TH></TR>\016");
     }

     if(Validchar(d->player)) {
        cached_scrheight                     = db[d->player].data->player.scrheight;
        db[d->player].data->player.scrheight = (db[d->player].data->player.scrheight - 6) * 2;
     }

     /* ---->  Check for channels with no operator (But users)  <---- */
     for(c = descriptor_list; c; c = c->next)
         if((c->flags & CONNECTED) && (c->channel != NOTHING))
            comms_chat_check(c,comms_chat_channelname(c->channel));

     union_initgrouprange((union group_data *) descriptor_list);
     while(union_grouprange()) {

           /* ---->  Count users on channel  <---- */
           for(count = 0, c = descriptor_list; c; c = c->next)
               if((c->flags & CONNECTED) && (grp->cunion->descriptor.channel == c->channel))
                  count++;

           /* ---->  Construct list entry and display it  <---- */
           if(grp->cunion->descriptor.channel == 0) strcpy(scratch_return_string,"The default channel.");
              else if(!Blank(grp->cunion->descriptor.subject) && (!(db[grp->cunion->descriptor.player].flags2 & CHAT_PRIVATE) || ((db[grp->cunion->descriptor.player].flags2 & CHAT_PRIVATE) && (grp->cunion->descriptor.channel == d->channel)))) strcpy(scratch_return_string,grp->cunion->descriptor.subject);
                 else strcpy(scratch_return_string,(db[grp->cunion->descriptor.player].flags2 & CHAT_PRIVATE) ? "PRIVATE":"PUBLIC");

           ptr = getfield(grp->cunion->descriptor.player,PREFIX);
           if(!Blank(ptr) && ((strlen(ptr) + 1 + strlen(getname(grp->cunion->descriptor.player))) <= 20))
              sprintf(buffer,"%s %s",ptr,getname(grp->cunion->descriptor.player));
                 else strcpy(buffer,getname(grp->cunion->descriptor.player));

           colour = privilege_colour(grp->cunion->descriptor.player);
           output(d,d->player,2,1,0,IsHtml(d) ? "%s%s%d%s%s%s%s%s%d%s%s%s%s":"%s%s%-13d%s%s%-22s%s%s%-8d%s%s%s%s",IsHtml(d) ? "\016<TR><TD ALIGN=CENTER>\016":" ",colour,grp->cunion->descriptor.channel,IsHtml(d) ? "\016</TD><TD ALIGN=LEFT>\016":"",IsHtml(d) ? colour:"",buffer,IsHtml(d) ? "\016</TD><TD ALIGN=CENTER>\016":"",IsHtml(d) ? colour:"",count,IsHtml(d) ? "\016</TD><TD ALIGN=LEFT>\016":"",IsHtml(d) ? colour:"",scratch_return_string,IsHtml(d) ? "\016</TD></TR>\016":"\n");
           users += count, channels++;
     }
     if(Validchar(d->player)) db[d->player].data->player.scrheight = cached_scrheight;

     if(grp->rangeitems == 0) output(d,d->player,2,1,1,"%s",IsHtml(d) ? "\016<TR ALIGN=CENTER><TD COLSPAN=4>"ANSI_LCYAN"<I>*** &nbsp; NO CHATTING CHANNELS IN USE &nbsp; ***</I></TD></TR>\016":" ***  NO CHATTING CHANNELS IN USE  ***\n");
     if(!in_command) {
        if(!IsHtml(d)) output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','='));
        listed_items(scratch_return_string,1);
        if(!IsHtml(d)) strcat(scratch_return_string,"  ");
        output(d,d->player,2,1,1,"%sChannels: %s "ANSI_DWHITE"%-10s%sTotal users: %s "ANSI_DWHITE"%d%s",IsHtml(d) ? "\016<TR ALIGN=CENTER><TD COLSPAN=2 BGCOLOR="HTML_TABLE_GREY">"ANSI_LWHITE"<B>\016":" "ANSI_LWHITE,IsHtml(d) ? "\016&nbsp;\016":"",scratch_return_string,IsHtml(d) ? "\016</B></TD><TD COLSPAN=2 BGCOLOR="HTML_TABLE_MGREY">"ANSI_LWHITE"<B>\016":ANSI_LWHITE,IsHtml(d) ? "\016&nbsp;\016":"",users,IsHtml(d) ? "\016</B></TD></TR>\016":"\n\n");
     }

     if(IsHtml(d)) output(d,d->player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
     html_anti_reverse(d,0);
     setreturn(OK,COMMAND_SUCC);
}


extern char  *session_title;
extern dbref session_who;


/* ---->  Session list (Single column):  Name & session comment  <---- */
void userlist_session(struct descriptor_data *d)
{
     unsigned char cached_scrheight;
     const    char *colour;
     char          *p1,*p2;
     int           length;

     /* ---->  Session title  <---- */
     html_anti_reverse(d,1);
     command_type |= COMM_CMD;
     if(!d->pager && !IsHtml(d) && Validchar(d->player) && More(d->player)) pager_init(d);
     if(IsHtml(d)) output(d,d->player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");

     if(!in_command && !IsHtml(d)) output(d,d->player,0,1,0,"\n%s",separator(d->terminal_width,0,'-','='));
     output(d,d->player,2,1,1,"%s%s%s",IsHtml(d) ? "\016<TR ALIGN=CENTER><TH COLSPAN=2 BGCOLOR="HTML_TABLE_CYAN"><FONT COLOR="HTML_LCYAN" SIZE=4><I>":" ",substitute(session_who,scratch_return_string,decompress(session_title),0,ANSI_LCYAN,NULL,0),IsHtml(d) ? "</I></FONT></TH></TR>\016":"\n");
     if(!IsHtml(d)) output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','-'));

     /* ---->  Session comments  <---- */
     if(Validchar(d->player)) {
        cached_scrheight                     = db[d->player].data->player.scrheight;
        db[d->player].data->player.scrheight = (db[d->player].data->player.scrheight - 10) * 2;
     }
     union_initgrouprange((union group_data *) descriptor_list);
     while(union_grouprange()) {

           /* ---->  Character's name  <---- */
           sprintf(scratch_buffer,"%s%s",IsHtml(d) ? "\016<TR ALIGN=LEFT><TD WIDTH=25%>\016":"",colour = privilege_colour(grp->cunion->descriptor.player));
           p1 = (char *) getfield(grp->cunion->descriptor.player,PREFIX);
           if(!Blank(p1) && ((strlen(p1) + 1 + strlen(getname(grp->cunion->descriptor.player))) <= 20))
              sprintf(p2 = (scratch_buffer + strlen(scratch_buffer))," %s %s",p1,getname(grp->cunion->descriptor.player));
                 else sprintf(p2 = (scratch_buffer + strlen(scratch_buffer))," %s",getname(grp->cunion->descriptor.player));

           if(!IsHtml(d)) {
              if((length = strlen(p2)) <= 21) {
                 for(p1 = p2 + length; length < 21; *p1++ = ' ', length++);
                 *p1 = '\0';
	      } else p2[21] = '\0';
	   }

           /* ---->  Character's session comment  <---- */
           output(d,d->player,2,1,23,"%s%s%s%s%s",scratch_buffer,IsHtml(d) ? "\016</TD><TD>\016":"  ",IsHtml(d) ? colour:"",(grp->cunion->descriptor.comment) ? substitute(grp->cunion->descriptor.player,scratch_return_string,decompress(grp->cunion->descriptor.comment),0,colour,NULL,0):"No comment.",IsHtml(d) ? "\016</TD></TR>\016":"\n");
     }
     if(Validchar(d->player)) db[d->player].data->player.scrheight = cached_scrheight;

     if(grp->rangeitems == 0) output(d,d->player,2,1,1,"%s",IsHtml(d) ? "\016<TR ALIGN=CENTER><TD COLSPAN=2>"ANSI_LCYAN"<I>*** &nbsp; NO ONE HAS ANY OPINION ON THIS SESSION YET &nbsp; ***</I></TD></TR>\016":" ***  NO ONE HAS ANY OPINION ON THIS SESSION YET  ***\n");
     if(!in_command) {
        if(!IsHtml(d)) output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','-'));
        listed_items(scratch_return_string,1);
        output(d,d->player,2,1,((grp->nogroups > 0) ? 17:21) + digit_wrap(0,grp->totalitems),"%sUsers listed:%s"ANSI_LYELLOW"%s %s "ANSI_DCYAN"- %s "ANSI_LWHITE"Type '"ANSI_LGREEN"session comment <COMMENT>"ANSI_LWHITE"' to set your comment, or '"ANSI_LGREEN"session title <TITLE>"ANSI_LWHITE"' to change the session title.%s",IsHtml(d) ? "\016<TR><TH ALIGN=CENTER WIDTH=25% BGCOLOR="HTML_TABLE_CYAN">"ANSI_LCYAN"<B><I>\016":" "ANSI_LCYAN,IsHtml(d) ? "\016</I></B></TH><TD ALIGN=LEFT BGCOLOR="HTML_TABLE_GREY">\016":"  ",scratch_return_string,IsHtml(d) ? "\016&nbsp;\016":"",IsHtml(d) ? "\016&nbsp;\016":"",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        if(!IsHtml(d)) output(d,d->player,0,1,0,separator(d->terminal_width,1,'-','='));
     }
     if(IsHtml(d)) output(d,d->player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
     command_type &= ~COMM_CMD;
     html_anti_reverse(d,0);
}

/* ---->  Assist list (Single column):  Assist time, name and reason  <---- */
void userlist_assist(struct descriptor_data *d)
{
     unsigned char cached_scrheight;
     const    char *colour;
     char          *p1,*p2;
     int           length;
     time_t        now;

     gettime(now);
     html_anti_reverse(d,1);
     command_type |= COMM_CMD;
     if(!d->pager && !IsHtml(d) && Validchar(d->player) && More(d->player)) pager_init(d);
     if(IsHtml(d)) output(d,d->player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");

     if(!in_command) {
        if(!IsHtml(d)) {
           output(d,d->player,0,1,0,"\n Time:    Name:                 Reason:");
           output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','='));
	} else output(d,d->player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH WIDTH=10%%><FONT COLOR="HTML_LCYAN" SIZE=4><I>Time:</I></FONT></TH><TH WIDTH=25%%><FONT COLOR="HTML_LCYAN" SIZE=4><I>Name:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Reason:</I></FONT></TH></TR>\016");
     }

     if(Validchar(d->player)) {
        cached_scrheight                     = db[d->player].data->player.scrheight;
        db[d->player].data->player.scrheight = (db[d->player].data->player.scrheight - 9) * 2;
     }
     union_initgrouprange((union group_data *) descriptor_list);
     while(union_grouprange()) {

           /* ---->  Character's assist time and name  <---- */
           sprintf(scratch_buffer,"%s%s",IsHtml(d) ? "\016<TR ALIGN=LEFT><TD ALIGN=CENTER WIDTH=10%>\016":"",colour = privilege_colour(grp->cunion->descriptor.player));
           sprintf(p2 = (scratch_buffer + strlen(scratch_buffer)),IsHtml(d) ? "%s%s":" %s%s  ",userlist_shorttime(grp->cunion->descriptor.assist_time - (ASSIST_TIME * MINUTE),now,scratch_return_string,1),IsHtml(d) ? "\016</TD><TD WIDTH=25%>\016":"");
           p1 = (char *) getfield(grp->cunion->descriptor.player,PREFIX);
           if(!Blank(p1) && ((strlen(p1) + 1 + strlen(getname(grp->cunion->descriptor.player))) <= 20))
              sprintf(scratch_buffer + strlen(scratch_buffer),"%s %s",p1,getname(grp->cunion->descriptor.player));
                 else strcat(scratch_buffer,getname(grp->cunion->descriptor.player));

           if(!IsHtml(d)) {
              if((length = strlen(p2)) <= 30) {
                 for(p1 = p2 + length; length < 30; *p1++ = ' ', length++);
                 *p1 = '\0';
	      } else p2[30] = '\0';
	   }

           /* ---->  Assist reason  <---- */
           if(grp->cunion->descriptor.assist) substitute(grp->cunion->descriptor.player,scratch_return_string,decompress(grp->cunion->descriptor.assist),0,colour,NULL,0);
              else strcpy(scratch_return_string,"A new user who needs assistance.");
           output(d,d->player,2,1,32,IsHtml(d) ? "%s%s%s%s%s":"%s  %s%s%s%s",scratch_buffer,IsHtml(d) ? "\016</TD><TD>\016":"",colour,scratch_return_string,IsHtml(d) ? "\016</TD></TR>\016":"\n");
     }
     if(Validchar(d->player)) db[d->player].data->player.scrheight = cached_scrheight;
     if(grp->rangeitems == 0) output(d,d->player,2,1,0,"%s",IsHtml(d) ? "\016<TR ALIGN=CENTER><TD COLSPAN=3>"ANSI_LCYAN"<I>*** &nbsp; NO ONE LISTED &nbsp; ***</I></TD></TR>\016":" ***  NO ONE LISTED  ***\n");
     if(!in_command) {
        if(!IsHtml(d)) output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','-'));
        output(d,d->player,2,1,1,"%sTo assist one of the above users, simply type '"ANSI_LGREEN"assist <NAME>"ANSI_LWHITE"' to teleport to their present location and let them know that you're available to help them.%s",IsHtml(d) ? "\016<TR><TD ALIGN=CENTER COLSPAN=3 BGCOLOR="HTML_TABLE_GREY">"ANSI_LWHITE"<I>\016":ANSI_LWHITE" ",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        if(!IsHtml(d)) output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','='));
        output(d,d->player,2,1,1,"%sUsers who need assistance: %s "ANSI_DWHITE"%s%s",IsHtml(d) ? "\016<TR ALIGN=CENTER><TD COLSPAN=3 BGCOLOR="HTML_TABLE_MGREY">"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",IsHtml(d) ? "\016&nbsp;\016":"",listed_items(scratch_return_string,1),IsHtml(d) ? "\016</B></TD></TR>\016":"\n\n");
     }

     if(IsHtml(d)) output(d,d->player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
     command_type &= ~COMM_CMD;
     html_anti_reverse(d,0);
}

/* ---->  Admin list (Single column):  Login time, idle, name, rank, Haven & Quiet  <---- */
void userlist_admin(struct descriptor_data *d,unsigned char dsc)
{
     short                    deities = 0,elders = 0,delders = 0,wizards = 0,druids = 0,apprentices = 0,dapprentices = 0,retired = 0,dretired = 0,experienced = 0,assistants = 0,builders = 0,mortals = 0,beings = 0,puppets = 0,morons = 0,idle = 0;
     unsigned char            cached_scrheight;
     struct   descriptor_data *descriptor;
     unsigned long            longdate;
     const    char            *colour;
     dbref                    object;
     char                     *ptr;
     time_t                   now;

     gettime(now);
     html_anti_reverse(d,1);
     longdate = epoch_to_longdate(now);
     if(!d->pager && !IsHtml(d) && Validchar(d->player) && More(d->player)) pager_init(d);
     if(IsHtml(d)) output(d,d->player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");

     if(!in_command) {
        if(!IsHtml(d)) {
           if(dsc) output(d,d->player,0,1,0,"\n Time:    Idle:    Name:                 Rank:                Haven:  Quiet:",scratch_return_string);
              else output(d,d->player,0,1,0,"\n Time:    Idle:    Name:                 Connected:  Idling:  Haven:  Quiet:",scratch_return_string);
           output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','='));
	} else if(dsc) output(d,d->player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Time:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Idle:</I></FONT></TH><TH WIDTH=25%%><FONT COLOR="HTML_LCYAN" SIZE=4><I>Name:</I></FONT></TH><TH WIDTH=20%%><FONT COLOR="HTML_LCYAN" SIZE=4><I>Rank:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Haven:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Quiet:</I></FONT></TH></TR>\016");
           else output(d,d->player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Time:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Idle:</I></FONT></TH><TH WIDTH=25%%><FONT COLOR="HTML_LCYAN" SIZE=4><I>Name:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Connected:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Idling:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Haven:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Quiet:</I></FONT></TH></TR>\016");
     }

     if(Validchar(d->player)) {
        cached_scrheight                     = db[d->player].data->player.scrheight;
        db[d->player].data->player.scrheight = (db[d->player].data->player.scrheight - 9) * 2;
     }

     if(dsc) union_initgrouprange((union group_data *) descriptor_list);
        else entiredb_initgrouprange();
     while((dsc) ? union_grouprange():entiredb_grouprange()) {
           if(dsc) {
              object     = grp->cunion->descriptor.player;
              descriptor = &(grp->cunion->descriptor);
	   } else {
              object     = grp->cobject;
              descriptor = getdsc(object);
	   }

           /* ---->  Character's login time and idle time  <---- */
           strcpy(scratch_buffer,colour = privilege_countcolour(object,&deities,&elders,&delders,&wizards,&druids,&apprentices,&dapprentices,&retired,&dretired,&experienced,&assistants,&builders,&mortals,&beings,&puppets,&morons));
           if(descriptor && ((now - descriptor->last_time) >= MINUTE)) idle++;
           sprintf(scratch_buffer + strlen(scratch_buffer),IsHtml(d) ? "\016<TR ALIGN=CENTER><TD>\016%s%s":" %s%s  ",IsHtml(d) ? colour:"",(descriptor) ? (char *) userlist_shorttime(descriptor->start_time,now,scratch_return_string,1):IsHtml(d) ? "\016&nbsp;\016":"       ");
           sprintf(scratch_buffer + strlen(scratch_buffer),IsHtml(d) ? "\016</TD><TD>\016%s%s":"%s%-9s",IsHtml(d) ? colour:"",(descriptor) ? (char *) userlist_shorttime(descriptor->last_time,now,scratch_return_string,0):IsHtml(d) ? "\016&nbsp;\016":"       ");

           /* ---->  Character's name  <---- */
           ptr = (char *) getfield(object,PREFIX);
           if(!Blank(ptr) && ((strlen(ptr) + 1 + strlen(getname(object))) <= 20))
              sprintf(scratch_return_string,"%s%s %s",IsHtml(d) ? colour:"",ptr,getname(object));
                 else sprintf(scratch_return_string,"%s%s",IsHtml(d) ? colour:"",getname(object));
           sprintf(scratch_buffer + strlen(scratch_buffer),IsHtml(d) ? "\016</TD><TD ALIGN=LEFT WIDTH=25%%>\016%s%s":"%s%-22s",IsHtml(d) ? colour:"",scratch_return_string);

           /* ---->  Character's rank/connected & idling, Haven & Quiet  <---- */
           if(dsc) {
              if(Puppet(object)) {
                 ptr = "Puppet";
	      } else if(Moron(object)) {
                 ptr = "Moron";
	      } else if(Level1(object)) {
                 ptr = "Deity";
	      } else if(Level2(object)) {
                 if(Druid(object)) ptr = "Elder Druid";
                    else ptr = "Elder Wizard";
	      } else if(Level3(object)) {
                 if(Druid(object)) ptr = "Druid";
                    else ptr = "Wizard";
	      } else if(Level4(object)) {
                 if(Druid(object)) ptr = "Apprentice Druid";
                    else ptr = "Apprentice Wizard";
	      } else if(Retired(object)) {
                 if(RetiredDruid(object)) ptr = "Retired Druid";
                    else ptr = "Retired Wizard";
	      } else if(Experienced(object)) {
                 ptr = "Experienced Builder";
	      } else if(Assistant(object)) {
                 ptr = "Assistant";
	      } else if(Builder(object)) {
                 ptr = "Builder";
	      } else if(Being(object)) {
                 ptr = "Being";
	      } else if(Puppet(object)) {
                ptr = "Puppet";
	      } else ptr = "Mortal";

              output(d,d->player,2,1,0,IsHtml(d) ? "%s\016</TD><TD ALIGN=LEFT WIDTH=20%%>\016%s%s\016</TD><TD>\016%s%s\016</TD><TD>\016%s%s\016</TD></TR>\016":"%s%s%-21s%s%-8s%s%s\n",scratch_buffer,IsHtml(d) ? colour:"",ptr,IsHtml(d) ? colour:"",Haven(object) ? "Yes":"No",IsHtml(d) ? colour:"",Quiet(object) ? "Yes":"No");
	   } else output(d,d->player,2,1,0,IsHtml(d) ? "%s\016</TD><TD>\016%s%s\016</TD><TD>\016%s%s\016</TD><TD>\016%s%s\016</TD><TD>\016%s%s\016</TD></TR>\016":"%s%s%-12s%s%-9s%s%-8s%s%s\n",scratch_buffer,IsHtml(d) ? colour:"",Connected(object) ? "Yes":"No",IsHtml(d) ? colour:"",(descriptor && ((now - descriptor->last_time) >= MINUTE)) ? "Yes":"No",IsHtml(d) ? colour:"",Haven(object) ? "Yes":"No",IsHtml(d) ? colour:"",Quiet(object) ? "Yes":"No");
     }
     if(Validchar(d->player)) db[d->player].data->player.scrheight = cached_scrheight;

     if(grp->rangeitems == 0) output(d,d->player,2,1,0,"%s",IsHtml(d) ? "\016<TR ALIGN=CENTER><TD COLSPAN=4>"ANSI_LCYAN"<I>*** &nbsp; NO ONE LISTED &nbsp; ***</I></TD></TR>\016":" ***  NO ONE LISTED  ***\n");
     if(!in_command) {
        if(IsHtml(d)) output(d,d->player,1,2,0,"<TR><TD COLSPAN=%d CELLPADDING=0 BGCOLOR="HTML_TABLE_GREY"><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_MGREY">",(dsc) ? 6:7);
	   else output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','='));

        output(d,d->player,2,1,0,"%s",userlist_users(d->player,"",deities,elders,delders,wizards,druids,apprentices,dapprentices,retired,dretired,experienced,assistants,builders,mortals,beings,puppets,morons,idle,1,IsHtml(d)));
        output(d,d->player,2,1,10,"%s",userlist_peak(1,IsHtml(d)));
        output(d,d->player,2,1,10,"%s",userlist_uptime(now,1,IsHtml(d)));
     }

     if(IsHtml(d)) output(d,d->player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
     html_anti_reverse(d,0);
}

/* ---->  Return character's title  <---- */
void userlist_query_title(CONTEXT)
{
     dbref         character = query_find_character(player,params,0);
     unsigned long longdate;
     time_t        now;

     gettime(now);
     longdate = epoch_to_longdate(now);
     if(!Validchar(character) || !HasField(character,TITLE)) return;
     if(hasprofile(db[character].data->player.profile) && ((db[character].data->player.profile->dob & 0xFFFF) == (longdate & 0xFFFF))) {
        sprintf(querybuf,"is %ld today  -  HAPPY BIRTHDAY!",longdate_difference(db[character].data->player.profile->dob,longdate) / 12);
        setreturn(querybuf,COMMAND_SUCC);
     } else setreturn(String(getfield(character,TITLE)),COMMAND_SUCC);
}

/* ---->  Display user list of specified type...        <---- */
/*        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~              */
/*        val1:                                               */
/*        (1)    Standard user listing ('who'.)               */
/*        (2)    Short user listing ('swho'.)                 */
/*        (3)    Host listing ('hosts'/'hostlist'.)           */
/*        (4)    Location listing ('where'.)                  */
/*        (5)    Local 'who' listing ('lwho'.)                */
/*        (6)    E-Mail address listing ('emaillist'.)        */
/*        (7)    Last commands listing ('last'.)              */
/*        (8)    Users on current chatting channel.           */
/*        (9)    List of currently active chatting channels.  */
/*        (10)   Friends 'who' listing.                       */
/*        (11)   Friends 'where' listing.                     */
/*        (12)   'where *<NAME>' list                         */
/*        (13)   Session listing                              */
/*        (14)   Welcome list                                 */
/*        (15)   Assist list                                  */

void userlist_view(CONTEXT)
{
     int                      flagspec = 0,flagmask = 0,flagspec2 = 0,flagmask2 = 0,flagexc = 0;
     struct   descriptor_data *p = getdsc(player);
     unsigned char            permission = 1,friends = 0;
     const    char            *p1,*email = NULL;
     dbref                    character = NOTHING;
     int                      number = 2;
     char                     *p2;

     setreturn(ERROR,COMMAND_FAIL);
     if(!p) {
        output(p,player,0,1,0,ANSI_LGREEN"Sorry, you must be connected to view that user list.");
        return;
     }
     if(val1 == 6) params = arg1, email = arg2;

     switch(val1) {
            case 3:

                 /* ---->  Host listing  <---- */
                 if(!in_command || Wizard(current_command)) {
                    if(!Level4(db[player].owner)) output(p,player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above can list the hosts from which characters are currently connected."), permission = 0;
                       else if(!Level4(player)) writelog(ADMIN_LOG,1,"HACK","Mortal %s(#%d) listed the hosts from which characters are currently connected within compound command %s(#%d) owned by %s(#%d).",getname(player),player,getname(current_command),current_command,getname(db[current_command].owner),db[current_command].owner);
		 } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, the hosts from which characters are currently connected can't be listed from within a compound command."), permission = 0;
                 break;
            case 6:

                 /* ---->  E-mail address listing  <---- */
                 if(!in_command || Wizard(current_command)) {
                    if(!Level4(db[player].owner)) output(p,player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above can list the E-mail addresses of currently connected characters."), permission = 0;
                       else if(!Level4(player)) writelog(ADMIN_LOG,1,"HACK","Mortal %s(#%d) listed the E-mail addresss of currently connected characters within compound command %s(#%d) owned by %s(#%d).",getname(player),player,getname(current_command),current_command,getname(db[current_command].owner),db[current_command].owner);
		 } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, the E-mail addresses of currently connected characters can't be listed from within a compound command."), permission = 0;
                 break;
            case 7:

                 /* ---->  Last command listing  <---- */
                 if(in_command && !(Valid(current_command) && Wizard(current_command)))
		    output(p,player,0,1,0,ANSI_LGREEN"Sorry, the last command typed by currently connected characters can't be listed from within a compound command."), permission = 0;
                 break;
     }
     if(!permission) return;

     /* ---->  Display appropriate 'who' list  <---- */
     if((!strcasecmp("page",params) && (strlen(params) == 4)) || !strncasecmp(params,"page ",5))
        for(params += 4; *params && (*params == ' '); params++);
     params = (char *) parse_grouprange(player,params,ALL,1);
     if(!Blank(params)) {
        if(strcasecmp(params,"me")) {
           if(((character = lookup_character(player,params,5)) == NOTHING) || (val1 == 4)) {
              int result = 0,dummy;

              /* ---->  Parse flags  <---- */
              if(character == NOTHING) {
                 if(*(p1 = params) && (*p1 != '*')) while(*p1) {
                    while(*p1 && (*p1 == ' ')) p1++;
                    if(*p1) {
 
                       /* ---->  Grab word/character name  <---- */
                       for(p2 = scratch_buffer; *p1 && (*p1 != ' '); *p2++ = *p1, p1++);
                       *p2 = '\0';

                       /* ---->  Determine what object type/field/flag word is  <---- */
                       if(strcasecmp("friends",scratch_buffer) && strcasecmp("enemies",scratch_buffer) && strcasecmp("friend",scratch_buffer) && strcasecmp("enemy",scratch_buffer)) {
                          result = parse_flagtype(scratch_buffer,&dummy,&flagmask);
                          if(result) flagspec |= result;
                             else {
                                result = parse_flagtype2(scratch_buffer,&dummy,&flagmask2);
                                if(result) flagspec2 |= result;
                                   else if(string_prefix("mortals",scratch_buffer)) {
                                      flagexc |= BUILDER|APPRENTICE|WIZARD|ELDER|DEITY;
                                      result++;
				   }
			     }
		       } else friends = 1;
		    }
		 } else if(*params) params++;
	      }
	   }

           /* ---->  If no valid flags specified, match as character name  <---- */
	   if(!(flagspec || flagspec2) && !flagexc && (val1 != 9) && !friends) {
              if((character == NOTHING) && ((character = lookup_character(player,params,1)) == NOTHING)) {
                 if(!in_command) output(p,player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",params);
                 return;
	      } else if((character != NOTHING) && (val1 == 4)) {
                 if(!Connected(character)) {
                    sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" is sleeping ",Article(character,UPPER,DEFINITE),getcname(NOTHING,character,0,0));
                    if(!((Secret(character) || Secret(db[character].location)) && !can_write_to(player,db[character].location,1) && !can_write_to(player,character,1))) {
                       dbref area = get_areaname_loc(db[character].location);
                       if(Valid(area) && !Blank(getfield(area,AREANAME)))
                          sprintf(scratch_return_string," in "ANSI_LYELLOW"%s"ANSI_LGREEN,getfield(area,AREANAME));
                             else *scratch_return_string = '\0';
                       sprintf(scratch_buffer + strlen(scratch_buffer),"in %s"ANSI_LYELLOW"%s"ANSI_LGREEN"%s.",Article(db[character].location,LOWER,INDEFINITE),unparse_object(player,db[character].location,0),scratch_return_string);
		    } else strcat(scratch_buffer,"in a secret location.");
                    output(p,player,0,1,0,"%s",scratch_buffer);
                    return;
		 } else val1 = 12;
	      } else if(!Connected(character)) {
                 if(!in_command) output(p,player,0,1,0,ANSI_LGREEN"Sorry, that character isn't connected.");
                 return;
	      }
              flagmask2 = 0, flagmask = 0;
	   }
	} else {
           if(val1 == 4) val1 = 12;
           character = player;
	}
     }

     /* ---->  Public/private E-mail address ('emaillist')  <---- */
     if(!Blank(email)) {
        if(string_prefix("public",email)) {
           number = 1;
	} else if(string_prefix("private",email)) {
           number = 2;
	} else if(!isdigit(*email) || ((number = atol(email)) <= 0) || (number > EMAIL_ADDRESSES)) {
           output(p,player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LYELLOW"public"ANSI_LGREEN"', '"ANSI_LYELLOW"private"ANSI_LGREEN"' or a number between "ANSI_LYELLOW"1"ANSI_LGREEN" and "ANSI_LYELLOW"%d"ANSI_LGREEN".",EMAIL_ADDRESSES);
           return;
	}
     }

     /* ---->  Set grouping/range matching conditions  <---- */
     if(!((val1 == 5) || (val1 == 7) || (val1 >= 9)))
        set_conditions_ps(player,flagspec,flagmask,flagspec2,flagmask2,flagexc,(val1 == 8) ? p->channel:NOTHING,character,NULL,(friends) ? 204:200);

     switch(val1) {
            case 1:
                 userlist_who(p);
                 break;
            case 2:
            case 8:
                 userlist_swho(p);
                 break;
            case 3:
                 userlist_hosts(p);
                 break;
            case 4:
                 userlist_where(p,0);
                 break;
            case 5:
                 set_conditions_ps(player,flagspec,flagmask,flagspec2,flagmask2,flagexc,get_areaname_loc(db[player].location),character,NULL,201);
                 userlist_where(p,1);
                 break;
            case 6:
                 userlist_email(p,number);
                 break;
            case 7:
                 set_conditions_ps(player,flagspec,flagmask,flagspec2,flagmask2,flagexc,NOTHING,character,NULL,202);
                 userlist_last(p);
                 break;
            case 9:
                 set_conditions_ps(player,flagspec,flagmask,flagspec2,flagmask2,flagexc,NOTHING,NOTHING,NULL,203);
                 userlist_channels(p);
                 break;
            case 10:
                 set_conditions_ps(player,flagspec,flagmask,flagspec2,flagmask2,flagexc,NOTHING,character,NULL,204);
                 userlist_swho(p);
                 break;
            case 11:
                 set_conditions_ps(player,flagspec,flagmask,flagspec2,flagmask2,flagexc,NOTHING,character,NULL,204);
                 userlist_where(p,0);
                 break;
            case 12:
                 set_conditions_ps(player,flagspec,flagmask,flagspec2,flagmask2,flagexc,NOTHING,character,NULL,206);
                 userlist_swho(p);
                 break;
            case 13:
                 set_conditions_ps(player,flagspec,flagmask,flagspec2,flagmask2,flagexc,NOTHING,character,NULL,207);
                 userlist_session(p);
                 break;
            case 14:
                 set_conditions_ps(player,flagspec,flagmask,flagspec2,flagmask2,flagexc,NOTHING,character,NULL,208);
                 userlist_swho(p);
                 break;
            case 15:
                 set_conditions_ps(player,flagspec,flagmask,flagspec2,flagmask2,flagexc,NOTHING,character,NULL,209);
                 userlist_assist(p);
                 break;
     }
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Change user list title (Appears after your name)  <---- */
void userlist_set_title(CONTEXT)
{
     dbref character;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(arg1)) {
        if((character = lookup_character(player,arg1,1)) == NOTHING) {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
           return;
	}
     } else character = db[player].owner;

     if(!in_command || Apprentice(current_command)) {
        if(can_write_to(player,character,0)) {
           if(!Readonly(character)) {
              if(Blank(arg2) || !strchr(arg2,'\n')) {
                 if(Blank(arg2) || !instring("%{",arg2)) {
                    if(Blank(arg2) || !instring("%h",arg2)) {
                       if(Blank(arg2) || Level2(db[player].owner) || (*arg2 != '.')) {
                          if(!Blank(arg2) || (strlen(arg2) <= 100)) {
		   	     ansi_code_filter((char *) arg2,arg2,0);
                             setfield(character,TITLE,arg2,0);
                             if(!in_command) {
                                substitute(player,scratch_return_string,arg2,0,ANSI_LYELLOW,NULL,0);
                                if(character != player) {
                                   if(Controller(character) != player) {
                                      if(!in_command) writelog(ADMIN_LOG,1,"TITLE CHANGE","%s(#%d) changed %s(#%d)'s title to '%s'.",getname(player),player,getname(character),character,arg2);
                                         else if(!Wizard(current_command)) writelog(HACK_LOG,1,"HACK","%s(#%d) changed %s(#%d)'s title to '%s' within compound command %s(#%d).",getname(player),player,getname(character),character,arg2,getname(current_command),current_command);
				   }

                                   if(!in_command) output(getdsc(character),character,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"%s"ANSI_LYELLOW"%s"ANSI_LWHITE" has changed your title to '"ANSI_LYELLOW"%s"ANSI_LWHITE"'.",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0),scratch_return_string);
                                   output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN"'s title is now '"ANSI_LYELLOW"%s"ANSI_LGREEN"'.",Article(character,UPPER,DEFINITE),getcname(NOTHING,character,0,0),scratch_return_string);
				} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Your title is now '"ANSI_LYELLOW"%s"ANSI_LGREEN"'.",scratch_return_string);
			     }
                             setreturn(OK,COMMAND_SUCC);
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of a title is 100 characters.");
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a title mustn't begin with a '"ANSI_LWHITE"."ANSI_LGREEN"'.");
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a title can't contain embedded HTML tags.");
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a title can't contain query command substitutions ('"ANSI_LWHITE"%{<QUERY COMMAND>}"ANSI_LGREEN"'.)");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a title can't contain embedded NEWLINE's.");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't change their title.",Article(character,LOWER,DEFINITE),getcname(player,character,1,0));
	} else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the title of someone who's of a lower level than yourself.");
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change your own title.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the title of a character can't be changed from within a compound command.");
}

/* ---->  Set own title  <---- */
void userlist_title(CONTEXT)
{
     userlist_set_title(player,NULL,NULL,"me",params,0,0);
}

