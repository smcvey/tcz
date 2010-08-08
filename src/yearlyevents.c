/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| YEARLYEVENTS.C:  Implements a hard-coded list of yearly events, of which    |
|                  users are informed at the appropriate time(s).  Such       |
|                  events include:  Christmas, New Year, TCZ's birthday,      |
|                  etc.                                                       |
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
| Module originally designed and written by:  J.P.Boggis 24/11/1997.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: yearlyevents.c,v 1.2 2005/06/29 20:09:39 tcz_monster Exp $

*/


#include <time.h>

#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "object_types.h"
#include "yearlyevents.h"
#include "flagset.h"


struct yearly_event_data *yearly_event_start = NULL;  /*  Pointer to start of yearly events linked list  */


/* ---->  {J.P.Boggis 30/12/1999}  Sort yearly events into ascending date order  <---- */
int yearly_event_sort(dbref player)
{
    struct yearly_event_data *thisyear = NULL,*nextyear = NULL;
    struct yearly_event_data *ptr,*current,*last;
    int    loop,nxtyear;
    struct tm *rtime;
    time_t now;

    gettime(now);
    if(Validchar(player)) now += (db[player].data->player.timediff * HOUR);
    rtime = localtime(&now);

    /* ---->  Sort yearly events into date/time order  <---- */
    for(loop = 0; yearly_events[loop].name; loop++) {
        last = NULL, current = &(yearly_events[loop]);

        /* ---->  Next year's event (I.e:  Already occurred this year)?  <---- */
        if((current->month < (rtime->tm_mon + 1)) || ((current->month == (rtime->tm_mon + 1)) && (current->day < rtime->tm_mday)))
           ptr = nextyear, nxtyear = 1;
              else ptr = thisyear, nxtyear = 0;

        /* ---->  Find correct date position for event  <---- */
        for(; ptr && (ptr->month < current->month); last = ptr, ptr = ptr->next);
        for(; ptr && (ptr->month == current->month) && (ptr->day < current->day); last = ptr, ptr = ptr->next);

        /* ---->  Find correct time position for event  <---- */
        for(; ptr && (ptr->month == current->month) && (ptr->day == current->day) && (ptr->hour < current->hour); last = ptr, ptr = ptr->next);
        for(; ptr && (ptr->month == current->month) && (ptr->day == current->day) && (ptr->hour == current->hour) && (ptr->minute < current->minute); last = ptr, ptr = ptr->next);

        /* ---->  Insert event into correct position  <---- */
        if(last) {
           current->next = ptr;
           last->next = current;
	} else if(nxtyear) {
           current->next = nextyear;
           nextyear      = current;
	} else {
           current->next = thisyear;
           thisyear      = current;
	}
    }

    /* ---->  Add next year events onto end of this year events  <---- */
    if(thisyear) {
       for(ptr = thisyear, last = NULL; ptr; last = ptr, ptr = ptr->next);
       if(last) last->next = nextyear;
    } else thisyear = nextyear;

    /* ---->  Set yearly event pointers  <---- */
    yearly_event_start = thisyear;
    return(loop);
}

/* ---->  {J.P.Boggis 29/12/1999}  Show current yearly event(s)  -  Called once per minute  <---- */
void yearly_event_show(dbref player,unsigned char timed)
{
     struct yearly_event_data *ptr = yearly_event_start;
     struct descriptor_data *d;
     char   buffer[BUFFER_LEN];
     struct tm *rtime;
     time_t now,tdnow;

     gettime(now);
     for(; ptr; ptr = ptr->next)
         for(d = descriptor_list; d; d = d->next)
             if(timed || (d->player == player))
                if((d->flags & CONNECTED) && Validchar(d->player)) {
                   tdnow = now + ((ptr->timediff) ? (db[d->player].data->player.timediff * HOUR):0);
                   rtime = localtime(&tdnow);

                   if((ptr->month == (rtime->tm_mon + 1)) && (ptr->day == rtime->tm_mday)) {
                      if(!timed || (ptr->hour > rtime->tm_hour) || ((ptr->hour == rtime->tm_hour) && (ptr->minute >= rtime->tm_min))) {
                         if(timed) {
                            if(ptr->notify && (ptr->minute == rtime->tm_min) && !Quiet(d->player) && !Quiet(Location(d->player)))
                               output(d,d->player,0,1,0,ANSI_LRED"\n\x05\x02[%s"ANSI_LRED"]\n",substitute(player,buffer,(char *) ptr->banner,0,ANSI_LRED,NULL,0));
			 } else if(Validchar(player)) {
                            output(d,d->player,0,1,0,"%s\n",substitute(player,buffer,(char *) ptr->banner,0,ANSI_LRED,NULL,0));
			 }
		      }
		   }
		}
}

/* ---->  {J.P.Boggis 29/12/1999}  List yearly events  <---- */
void yearly_event_list(CONTEXT)
{
     char buffer[BUFFER_LEN];

     setreturn(ERROR,COMMAND_FAIL);
     params = (char *) parse_grouprange(player,params,FIRST,1);

     if(!Blank(params)) {
        struct yearly_event_data *ptr = yearly_events;

	/* ---->  Show information about yearly event  <---- */
	for(ptr = yearly_event_start; ptr && !instring(params,ptr->name); ptr = ptr->next);
	if(ptr) {
           struct   descriptor_data *p = getdsc(player);
           time_t                   now,adjust,date;
           unsigned char            invalid;
           struct   tm              *rtime;
           int                      year;

           if(IsHtml(p)) {
              html_anti_reverse(p,1);
              output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
	      output(p,player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>\016%s\016</I></FONT></TH></TR>\016",ptr->name);
	   } else tilde_string(player,ptr->name,ANSI_LCYAN,ANSI_DCYAN,0,1,6);

           gettime(now);
           rtime = localtime(&now);
           date  = 0;
           year  = rtime->tm_year + 1900;

           while(date < now) {
                 sprintf(buffer,"%02d/%02d/%04d %02d:%02d",ptr->day,ptr->month,year,ptr->hour,ptr->minute);
                 date = string_to_date(player,buffer,1,0,&invalid);
                 year++;
	   }

           if(IsHtml(p)) output(p,player,1,2,0,"<TR><TD>");
           output(p,player,0,1,0,ANSI_LGREEN"Date: \016&nbsp;\016 "ANSI_LYELLOW"%s\n",date_to_string(date + ((ptr->timediff) ? 0:(db[player].data->player.timediff * HOUR)),UNSET_DATE,player,ptr->datetime));

           adjust = timeadjust;
           timeadjust += (date - now);
           output(p,player,0,1,0,"%s%s",substitute(player,buffer,(char *) ptr->banner,0,ANSI_LRED,NULL,0),IsHtml(p) ? "":"\n");
           timeadjust = adjust;

           if(IsHtml(p)) {
              output(p,player,1,2,0,"</TD></TR></TABLE>%s",(!in_command) ? "<BR>":"");
              html_anti_reverse(p,0);
	   }
           setreturn(OK,COMMAND_SUCC);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the yearly event '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",params);
     } else {
        unsigned char            invalid,cached_scrheight,twidth = output_terminal_width(player);
        struct   descriptor_data *p = getdsc(player);
        time_t                   now,date;
        struct   tm              *rtime;
        int                      year;

        /* ---->  List yearly events  <---- */
        yearly_event_sort(player);
        if(p && !p->pager && !IsHtml(p) && More(player)) pager_init(p);
        if(IsHtml(p)) {
           html_anti_reverse(p,1);
           output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
	}

        if(!in_command) {
           if(!IsHtml(p)) {
              output(p,player,0,1,0,"\n Yearly Event:                   Date/Time:");
              output(p,player,0,1,0,separator(twidth,0,'-','='));
	   } else output(p,player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH WIDTH=30%%><FONT COLOR="HTML_LCYAN" SIZE=4><I>Yearly Event:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Date/Time:</I></FONT></TH></TR>\016");
	}

        cached_scrheight                  = db[player].data->player.scrheight;
        db[player].data->player.scrheight = (db[player].data->player.scrheight - 9) * 2;

        gettime(now);
        union_initgrouprange((union group_data *) yearly_event_start);
        while(union_grouprange()) {
              invalid = 0;
              rtime   = localtime(&now);
              date    = 0;
              year    = (rtime->tm_year + 1900);

              while((date < now) && !invalid) {
                    sprintf(buffer,"%02d/%02d/%04d %02d:%02d",grp->cunion->yearly_event.day,grp->cunion->yearly_event.month,year,grp->cunion->yearly_event.hour,grp->cunion->yearly_event.minute);
                    date = string_to_date(player,buffer,1,0,&invalid);
                    year++;
	      }

              output(p,player,2,1,1,"%s%-30s%s%s%s",IsHtml(p) ? "\016<TR><TD ALIGN=CENTER WIDTH=30%% BGCOLOR="HTML_TABLE_GREEN">\016"ANSI_LGREEN:ANSI_LGREEN" ",grp->cunion->yearly_event.name,IsHtml(p) ? "\016</TD><TD ALIGN=LEFT BGCOLOR="HTML_TABLE_YELLOW">\016"ANSI_LYELLOW:"  "ANSI_LYELLOW,date_to_string(date + ((grp->cunion->yearly_event.timediff) ? 0:(db[player].data->player.timediff * HOUR)),UNSET_DATE,player,grp->cunion->yearly_event.datetime),IsHtml(p) ? "\016</TD></TR>\016":"\n");
	}

        db[player].data->player.scrheight = cached_scrheight;
        if(grp->rangeitems == 0) output(p,player,2,1,0,"%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TD COLSPAN=2>"ANSI_LCYAN"<I>*** &nbsp; NO YEARLY EVENTS LISTED &nbsp; ***</I></TD></TR>\016":" ***  NO YEARLY EVENTS LISTED  ***\n");
        if(!in_command) {
           if(!IsHtml(p)) output(p,player,2,1,0,separator(twidth,1,'-','-'));
           output(p,player,2,1,1,"%sFor more information on one of the above yearly events, type '"ANSI_LGREEN"event <NAME>"ANSI_LWHITE"', where "ANSI_LYELLOW"<NAME>"ANSI_LWHITE" is the name of the event.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_DGREY"><TD COLSPAN=2>"ANSI_LWHITE"<I>\016":ANSI_LWHITE" ",IsHtml(p) ? "\016</I></TD></TR>\016":"\n");
           if(!IsHtml(p)) output(p,player,2,1,0,separator(twidth,1,'-','='));
           output(p,player,2,1,1,"%sYearly events listed: \016&nbsp;\016 "ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><TD COLSPAN=2>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",listed_items(buffer,1),IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
	}

        if(IsHtml(p)) {
           output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
           html_anti_reverse(p,0);
	}
        setreturn(OK,COMMAND_SUCC);
     }
}
