/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| EVENT.C  -  Implement queued/timed events (Presently alarms and fuses.)     |
|                                                                             |
|             Excludes system events handled by tcz_time_sync() in            |
|             interface.c.                                                    |
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

  $Id: event.c,v 1.1.1.1 2004/12/02 17:41:19 jpboggis Exp $

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

#include "object_types.h"
#include "objectlists.h"
#include "flagset.h"
#include "search.h"
#include "fields.h"


/* ---->  Initialise pending events (At server start-time)  <---- */
void event_initialise()
{
     dbref i;

     for(i = 0; i < db_top; i++)
         switch(Typeof(i)) {
                case TYPE_ALARM:
                     if(Valid(db[i].destination) && getfield(i,DESC))
                        event_add(NOTHING,i,NOTHING,NOTHING,event_next_cron(getfield(i,DESC)),NULL);
                     break;
	 }
}

/* ---->  Remove event from pending queue  <---- */
void event_remove(dbref object)
{
     struct event_data *newptr,*cache,*ptr,*newlist = NULL;

     for(ptr = event_queue; ptr; ptr = cache) {
         cache = ptr->next;
         if(ptr->object != object) {
            if(newlist) {
               newptr->next = ptr;
               newptr       = ptr;
               ptr->next    = NULL;
	    } else {
               newlist = newptr = ptr;
               ptr->next        = NULL;
	    }
	 } else {
            FREENULL(ptr->string);
            FREENULL(ptr);
	 }
     }
     event_queue = newlist;
}

/* ---->  Add new event to pending queue  <---- */
void event_add(dbref player,dbref object,dbref command,dbref data,long exectime,const char *str)
{
     struct event_data *current,*new,*prev = NULL;
        
     event_remove(object);
     if((Typeof(object) == TYPE_ALARM) && (!Valid(db[object].destination) || (Typeof(db[object].destination) != TYPE_COMMAND))) return;
     MALLOC(new,struct event_data);
     new->command = command;
     new->object  = object;
     new->player  = player;
     new->string  = (char *) alloc_string(str);
     new->data    = data;
     new->time    = exectime;

     for(current = event_queue; current && (current->time <= exectime); prev = current, current = current->next);
     if(prev) prev->next = new;
	else event_queue = new;
     new->next = current;
}

/* ---->  Event pending at TIME?  (If so, return details)  <---- */
unsigned char event_pending_at(long exectime,dbref *player,dbref *object,dbref *command,dbref *data,char **str)
{
	 struct event_data *current;

	 current = event_queue;
	 if(current && (current->time < exectime)) {
	    event_queue = current->next;
	    if(command) *command = current->command;
	    if(player)  *player  = current->player;
	    if(object)  *object  = current->object;
	    if(data)    *data    = current->data;
	    if(str)     *str     = current->string;
	       else FREENULL(current->string);
	    FREENULL(current);
	    return(1);
	 } else return(0);
}

/* ---->  Set $1, $2 and $3 appropriately for fuse compound command execution  <---- */
void event_set_fuse_args(const char *args,char **arg0,char **arg1,char **arg2,char **arg3,char *buffer,char *token,unsigned char flags)
{
     char *ptr,*tmp;

     if(!Blank(args)) {
        if(args != buffer) {
           for(; *args && (*args == ' '); args++);
           strcpy(buffer,(*arg3) = (char *) args);
	} else {
           for(; *args && (*args == ' '); args++);
           (*arg3) = "";
	}

        /* ---->  Check for special cases  <---- */
        ptr = buffer;
        if(TOKEN(*ptr)) {
           token[0] = *ptr, token[1] = '\0', (*arg0) = (*arg1) = token;
           for(ptr++; *ptr == ' '; ptr++);
           (*arg2) = (*ptr) ? ptr:"";
	} else if(flags & FUSE_CONVERSE) {
           token[0] = '\"', token[1] = '\0', (*arg0) = (*arg1) = token;
           for(ptr++; *ptr == ' '; ptr++);
           (*arg2) = (*ptr) ? ptr:"";
	} else if(!(tmp = (char *) strchr(ptr,' '))) {

           /* ---->  No parameters  <---- */
           (*arg0) = (*arg1) = ptr;
           (*arg2) = "";
	} else {
           *tmp++  = '\0';
           for(; *tmp && (*tmp == ' '); tmp++);
           (*arg0) = (*arg1) = ptr;
           (*arg2) = tmp;
	}
     } else (*arg0) = (*arg1) = (*arg2) = (*arg3) = "";
}

/* ---->  Trigger fuses attached to OBJECT  <---- */
unsigned char event_trigger_fuses(dbref player,dbref object,const char *args,unsigned char flags)
{
	 char     *cached_arg0 = command_arg0,*cached_arg1 = command_arg1,*cached_arg2 = command_arg2,*cached_arg3 = command_arg3;
	 dbref    fuse,next,cached_parent,cached_owner,cached_chpid,currentobj;
	 char     buf[16],buffer[TEXT_SIZE],token[2];
	 unsigned char abort = 0,aborted = 0;
	 int      cached_security,value;
	 time_t   now;

	 gettime(now);
	 if(RoomZero(object)) return(0);
	 if(!Blank(args) && Level4(player) && !strcasecmp("escape",args)) return(0);
	 event_set_fuse_args(((flags & FUSE_COMMAND) && in_command && command_lineptr) ? command_lineptr:(args) ? args:command_line,&command_arg0,&command_arg1,&command_arg2,&command_arg3,buffer,token,flags);
	 if(command_type & FUSE_ABORT) abort = 1;
	 command_type &= ~FUSE_ABORT;

	 next = getfirst(object,FUSES,&currentobj);
	 while(Valid(next)) {
	       fuse = next;
	       getnext(next,FUSES,currentobj);
	       if((Typeof(fuse) == TYPE_FUSE) && !Invisible(fuse) && Executable(fuse) && !RoomZero(Location(fuse)) && (!Tom(fuse) || (flags & FUSE_TOM)) && could_satisfy_lock(player,fuse,0) && getfield(fuse,DESC)) {
		  value = atoi(getfield(fuse,DESC)) - 1;
		  setfield(fuse,DESC,"",0);
		  if(value <= 0) {

		     /* ---->  Execute CSUCC link of fuse (Counter <= 0)  <---- */
		     if(Valid(db[fuse].contents) && (could_satisfy_lock(player,db[fuse].contents,0))) {
			if(!Sticky(fuse)) {
			   if(option_loglevel(OPTSTATUS) >= 5)
			      writelog(COMMAND_LOG,1,"FUSE","(CSUCC)  Compound command %s(#%d) owned by %s(#%d) executed by %s(#%d) via fuse %s(#%d) owned by %s(#%d).",getname(db[fuse].contents),db[fuse].contents,getname(Owner(db[fuse].contents)),Owner(db[fuse].contents),getname(player),player,getname(fuse),fuse,getname(Owner(fuse)),Owner(fuse));

			   cached_owner     = db[player].owner;
			   cached_chpid     = db[player].data->player.chpid;
			   cached_parent    = parent_object;
			   cached_security  = security;
			   parent_object    = object;
			   db[player].owner = db[player].data->player.chpid = db[fuse].owner;
			   if(!Wizard(db[fuse].contents) && !Level4(Owner(db[fuse].contents))) security = 1;
			   if(Abort(fuse)) command_type |= FUSE_ABORT;
			   db[fuse].flags2 |=  NON_EXECUTABLE;
			   command_type    |=  FUSE_CMD;
			   command_cache_execute(player,db[fuse].contents,1,1);
			   command_type    &= ~FUSE_CMD;
			   db[fuse].flags2 &= ~NON_EXECUTABLE;
			   security                      = cached_security;
			   parent_object                 = cached_parent;
			   db[player].owner              = cached_owner;
			   db[player].data->player.chpid = cached_chpid;
			} else event_add(player,fuse,db[fuse].contents,object,now,((flags & FUSE_COMMAND) && in_command && command_lineptr) ? command_lineptr:(args) ? args:command_line);
		     }

		     if(getfield(fuse,DROP))
			setfield(fuse,DESC,getfield(fuse,DROP),0);
		  } else {

		     /* ---->  Execute CFAIL link of fuse (Counter > 0)  <---- */
		     if(Valid(db[fuse].exits) && (could_satisfy_lock(player,db[fuse].exits,0))) {
			if(!Sticky(fuse)) {
			   if(option_loglevel(OPTSTATUS) >= 5)
			      writelog(COMMAND_LOG,1,"FUSE","(CFAIL)  Compound command %s(#%d) owned by %s(#%d) executed by %s(#%d) via fuse %s(#%d) owned by %s(#%d).",getname(db[fuse].exits),db[fuse].exits,getname(Owner(db[fuse].exits)),Owner(db[fuse].exits),getname(player),player,getname(fuse),fuse,getname(Owner(fuse)),Owner(fuse));

			   cached_owner     = db[player].owner;
			   cached_chpid     = db[player].data->player.chpid;
			   cached_parent    = parent_object;
			   cached_security  = security;
			   parent_object    = object;
			   db[player].owner = db[player].data->player.chpid = db[fuse].owner;
			   if(!Wizard(db[fuse].exits) && !Level4(Owner(db[fuse].exits))) security = 1;
			   if(Abort(fuse)) command_type |= FUSE_ABORT;
			   db[fuse].flags2 |=  NON_EXECUTABLE;
			   command_type    |=  FUSE_CMD;
			   command_cache_execute(player,db[fuse].exits,1,1);
			   command_type    &= ~FUSE_CMD;
			   db[fuse].flags2 &= ~NON_EXECUTABLE;
			   security                      = cached_security;
			   parent_object                 = cached_parent;
			   db[player].owner              = cached_owner;
			   db[player].data->player.chpid = cached_chpid;
			} else event_add(player,fuse,db[fuse].exits,object,now,((flags & FUSE_COMMAND) && in_command && command_lineptr) ? command_lineptr:(args) ? args:command_line);
		     }
		     sprintf(buf,"%d",value);
		     setfield(fuse,DESC,buf,0);
		  }
	       }
	 }

	 /* ---->  ABORT fuse handling  <---- */
	 aborted = ((command_type & FUSE_ABORT) != 0);
	 if(!(flags & FUSE_ABORTING)) {
	    if(abort) command_type |= FUSE_ABORT;
	       else command_type &= ~FUSE_ABORT;
	 }

	 command_arg0 = cached_arg0;
	 command_arg1 = cached_arg1;
	 command_arg2 = cached_arg2;
	 command_arg3 = cached_arg3;
	 return(aborted);
}

/* ---->  List pending events  <---- */
void event_pending(CONTEXT)
{
     unsigned char cr = 1,cached_scrheight,twidth = output_terminal_width(player);
     int      result,count = 0,event = 0,valid = 0;
     struct   descriptor_data *p = getdsc(player);
     time_t   now,ftime,ttime;
     struct   tm *rtime;
     dbref    command;
     char     *tmp;

     /* ---->  Start parsing event object types to list from second parameter  <---- */
     gettime(now);
     if(now > 0) now += (db[player].data->player.timediff * HOUR);
     if(!Blank(arg2)) {
        while(*arg2) {
              while(*arg2 && (*arg2 == ' ')) arg2++;
              if(*arg2) {

                 /* ---->  Determine what object type/field/flag word is  <---- */
                 for(tmp = scratch_buffer; *arg2 && (*arg2 != ' '); *tmp++ = *arg2++);
                 *tmp = '\0';

                 if((result = parse_objecttype(scratch_buffer))) valid = event |= result;
                    else if(string_prefix("all",scratch_buffer)) event = SEARCH_ALL;
                 if(!result) output(p,player,0,1,0,"%s"ANSI_LGREEN"Sorry, '"ANSI_LWHITE"%s"ANSI_LGREEN"' is an unknown object type.",(cr) ? "\n":"",scratch_buffer), cr = 0;
	      }
	}
     }

     if(!event) event = SEARCH_ALARM;
     arg1 = (char *) parse_grouprange(player,arg1,FIRST,1);
     set_conditions(player,0,0,event,NOTHING,arg1,509), cr = 0;

     html_anti_reverse(p,1);
     if(p && !p->pager && !IsHtml(p) && Validchar(p->player) && More(p->player)) pager_init(p);
     if(IsHtml(p)) output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(!in_command) ? "<BR>":"");
     if(!in_command) {
        output(p,player,2,1,1,"%sPending events...%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER><FONT SIZE=4><I>\016"ANSI_LCYAN:"\n ",IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
        if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
     }

     cached_scrheight                  = db[player].data->player.scrheight;
     db[player].data->player.scrheight = ((db[player].data->player.scrheight - 6) / 3) * 2;

     union_initgrouprange((union group_data *) event_queue);
     while(union_grouprange()) {
           if(Typeof(grp->cunion->event.object) != TYPE_ALARM) command = grp->cunion->event.command;
              else command = db[grp->cunion->event.object].destination;
           if((ftime = grp->cunion->event.time) > 0) ftime += (db[player].data->player.timediff * HOUR);
           ttime = ftime, ftime -= now;
           if(ftime < 0) ftime = 0;
           rtime = localtime((time_t *) &ttime);
           if(cr && !IsHtml(p)) output(p,player,0,1,0,"");
           sprintf(scratch_return_string,"%s"ANSI_DCYAN"["ANSI_LGREEN"(%s) "ANSI_LWHITE"%d:%02d.%02d"ANSI_DCYAN"] \016&nbsp;\016 ",IsHtml(p) ? "\016<TR><TD ALIGN=LEFT>\016":" ",dayabbr[rtime->tm_wday & 7],rtime->tm_hour,rtime->tm_min,rtime->tm_sec);
           if(Typeof(grp->cunion->event.object) == TYPE_ALARM) sprintf(scratch_return_string + strlen(scratch_return_string),ANSI_LYELLOW"(Executes in "ANSI_LWHITE"%s"ANSI_LYELLOW") \016&nbsp;\016 ",interval(ftime,ftime,ENTITIES,0));
           sprintf(scratch_return_string + strlen(scratch_return_string),ANSI_LWHITE"%s"ANSI_LGREEN" executing ",unparse_object(player,grp->cunion->event.object,0));
           output(p,player,2,1,3,"%s"ANSI_LWHITE"%s"ANSI_LGREEN".%s",scratch_return_string,unparse_object(player,command,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           count++, cr = 1;
     }
     db[player].data->player.scrheight = cached_scrheight;
 
     if(grp->rangeitems == 0) output(p,player,2,1,1,"%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TD>"ANSI_LCYAN"<I>*** &nbsp; NO PENDING EVENTS &nbsp; ***</I></TD></TR>\016":" ***  NO PENDING EVENTS  ***\n");
     if(!in_command) {
        if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
         output(p,player,2,1,0,"%sTotal pending events: \016&nbsp;\016 "ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><TD>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",listed_items(scratch_return_string,1),IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
     }
     if(IsHtml(p)) output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
     html_anti_reverse(p,0);
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Skip over blank space and number  <---- */
static void skip_space_and_number(const char **ptr,int *number)
{
       *number = 0;
       while((**ptr) && (**ptr == ' ')) (*ptr)++;
       if(**ptr && (**ptr == '*')) {
          (*ptr)++;
          *number = -1;
          return;
       }
       while(**ptr && (**ptr >= '0') && (**ptr <= '9')) *number = *number * 10 + *((*ptr)++) - '0';
}

/* ---->  Get time when alarm event will next execute  <---- */
long event_next_cron(const char *string)
{
     int      second,minute,hour,day;
     unsigned char forward = 0;
     struct   tm *rtime;
     const    char *ptr;
     time_t   now,then;

     gettime(now);
     if(!string) return(now + 1);
     for(ptr = string; *ptr && (*ptr == ' '); ptr++);
     if(*ptr == '+') ptr++, forward = 1;
     skip_space_and_number(&ptr,&second);
     skip_space_and_number(&ptr,&minute);
     skip_space_and_number(&ptr,&hour);
     skip_space_and_number(&ptr,&day);

     if(!forward) {
        if(second != -1) second  = (((second % MINUTE) + 9) / 10) * 10;
        if(minute != -1) minute %= MINUTE;
        if(hour   != -1) hour   %= 24;
        if(day    != -1) day    %= 7;
     }

     now++, rtime = localtime(&now), then = now;
     if(forward) {
        now += ((day * DAY) + (hour * HOUR) + (minute * MINUTE) + second);
        if(now <= then) now += 10;
     } else {
        if(second == NOTHING) second = (rtime->tm_sec + 10) % MINUTE;
        now += second - rtime->tm_sec;
        if(minute == NOTHING) minute = (rtime->tm_min + (now < then)) % MINUTE;
        now += (minute - rtime->tm_min) * MINUTE;
        if(hour == NOTHING) hour = (rtime->tm_hour + (now < then)) % MINUTE;
        now += (hour - rtime->tm_hour) * HOUR;
        if(day == -1) day = (rtime->tm_wday + (now < then)) % 7;
        now += (day - rtime->tm_wday) * DAY;
        if(now < then) now += WEEK;
     }
     return(now);
}
