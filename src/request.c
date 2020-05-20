/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| REQUEST.C:  Implements a request queue for new characters.  This allows     |
|             users from Internet sites which have been banned from creating  |
|             new characters to make a request for a new character by giving  |
|             their E-mail address.                                           |
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
| Module originally designed and written by:  J.P.Boggis 18/11/1996.          |
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

#include "object_types.h"
#include "flagset.h"
#include "fields.h"


/* ---->  Remove expired requests from queue (Performed automatically on a daily basis)  <---- */
void request_expired(void)
{
     struct request_data *head = NULL,*tail = NULL,*next,*ptr;
     time_t expiry,now;
     int    count = 0;

     gettime(now);
     expiry = now - (REQUEST_EXPIRY * DAY);

     for(ptr = request; ptr; ptr = next) {
         next = ptr->next;
         if(ptr->date < expiry) {

            /* ---->  Request expired:  Remove from queue  <---- */
            strcpy(scratch_return_string,!Blank(ptr->email) ? decompress(ptr->email):"<UNKNOWN>");
            writelog(REQUEST_LOG,1,"REQUEST","Request %d has expired  -  NAME:  %s,  E-MAIL:  %s.",ptr->ref,!Blank(ptr->name) ? decompress(ptr->name):"<UNKNOWN>",scratch_return_string);
            FREENULL(ptr->name);
            FREENULL(ptr->email);
            FREENULL(ptr);
            count++;
	 } else if(head) {
            tail->next = ptr;
            tail       = ptr;
            tail->next = NULL;
	 } else {
            head = tail = ptr;
            head->next = NULL;
	 }
     }
     request = head;
     writelog(MAINTENANCE_LOG,1,"MAINTENANCE","%d expired new character request%s removed from queue.",count,Plural(count));
}

/* ---->  Add request to queue (Sorted by date, oldest first)  <---- */
time_t request_add(char *email,char *name,unsigned long address,const char *host,time_t date,dbref user,unsigned short ref)
{
       struct   request_data *ptr,*new,*last = NULL;
       char                  namebuf[TEXT_SIZE];
       unsigned char         log = 0;
       char                  *tmp;

       /* ---->  Strip leading/trailing spaces  <---- */
       if(!email) return(-1);
       for(; *email && (*email == ' '); email++);
       for(tmp = email + strlen(email) - 1; (tmp > email) && (*tmp == ' '); *tmp-- = ' ');

       /* ---->  Request with E-mail address already exists?  <---- */
       for(ptr = request; ptr; ptr = ptr->next)
           if(ptr->email && !strcasecmp(email,decompress(ptr->email)))
              return(ptr->date);

       /* ---->  Allocate reference number?  <---- */
       if(!ref) log = 1;
       for(ptr = request; ptr && (ref > 0); ptr = ptr->next)
           if(ptr->ref == ref) ref = 0;
       while(!ref) {
             ref = (lrand48() % 0xFFFF) + 1;
             for(ptr = request; ptr && (ref > 0); ptr = ptr->next)
                 if(ptr->ref == ref) ref = 0;
       }
       if(log) writelog(REQUEST_LOG,1,"REQUEST","New request %d queued for %s (%s) from %s.",ref,name,email,!Blank(host) ? host:"<UNKNOWN>");

       /* ---->  Add new request  <---- */
       filter_spaces(namebuf,name,1);
       for(ptr = request; ptr && (ptr->date <= date); last = ptr, ptr = ptr->next);
       MALLOC(new,struct request_data);
       new->address = address;
       new->email   = (char *) alloc_string(compress(email,0));
       new->name    = (char *) alloc_string(compress(namebuf,0));
       new->date    = date;
       new->user    = user;
       new->ref     = ref;

       if(last) last->next = new;
          else request = new;
       if(ptr) new->next = ptr;
          else new->next = NULL;
       return(0);
}

/* ---->  Lookup request by number, and return pointer to it  <---- */
struct request_data *request_lookup(const char *reference,unsigned short refno,struct request_data **last)
{
       struct   request_data *ptr;
       unsigned short        ref;

       (*last) = NULL;
       if(!Blank(reference) || refno)
          for(ref = ((refno) ? refno:atol(reference)), ptr = request; ptr; (*last) = ptr, ptr = ptr->next)
              if(ptr->ref == ref) return(ptr);
       return(NULL);
}

/* ---->  View details of request  <---- */
void request_view(dbref player,const char *reference,unsigned short refno)
{
     unsigned char            twidth = output_terminal_width(player);
     struct   descriptor_data *p = getdsc(player);
     const    char            *email = NULL;
     int                      copied;
     struct   request_data    *ptr;
     struct   in_addr         addr;

     /* ---->  Lookup request by reference number  <---- */
     if(!Blank(reference) || refno) {
        if(!(ptr = request_lookup(reference,refno,&ptr))) {
           if(refno) output(p,player,0,1,0,ANSI_LGREEN"Sorry, unable to show details of new character request number "ANSI_LWHITE"%d"ANSI_LGREEN".",refno);
              else output(p,player,0,1,0,ANSI_LGREEN"Sorry, a new character request with the reference number '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",reference);
           return;
	}
     } else {
        for(ptr = request; ptr && (ptr->user != player); ptr = ptr->next);
        if(!ptr) {
           output(p,player,0,1,0,ANSI_LGREEN"Sorry, you aren't currently handling any requests for new characters.  Please type '"ANSI_LWHITE"@request new"ANSI_LGREEN"' if you'd like to handle a new request.");
           return;
	}
     }

     /* ---->  Display details of request  <---- */
     html_anti_reverse(p,1);
     if(IsHtml(p)) output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
     if(!in_command) {
        if(!IsHtml(p)) {
           output(p,player,0,1,0,(refno) ? "\n The following new character request has been allocated to you...":"\n Details of request for new character...");
           output(p,player,0,1,0,separator(twidth,0,'-','='));
	} else output(p,player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH COLSPAN=2><FONT COLOR="HTML_LCYAN" SIZE=4><I>%s</I></FONT></TH></TR>\016",(refno) ? "The following new character request has been allocated to you...":"Details of request for new character...");
     }

     output(p,player,2,1,0,"%sReference number:%s"ANSI_LWHITE"%d.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"     ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",ptr->ref,IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,24,"%sTime/date of request:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW" ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",date_to_string(ptr->date,UNSET_DATE,player,FULLDATEFMT),IsHtml(p) ? "\016</TD></TR>\016":"\n");
     addr.s_addr = htonl(ptr->address), lookup_site(&addr,scratch_return_string);
     output(p,player,2,1,24,"%sRequest made from:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"    ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",scratch_return_string,IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,24,"%sE-mail address:%s"ANSI_LWHITE"\016<A HREF=\"mailto:%s\" TARGET=_blank>\016%s\016</A>\016%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"       ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",html_encode((ptr->email && IsHtml(p)) ? ptr->email:"",scratch_return_string,&copied,256),(ptr->email) ? decompress(ptr->email):"<UNKNOWN>",IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,24,"%sPreferred name:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"        ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",(ptr->name) ? decompress(ptr->name):"<UNKNOWN>",IsHtml(p) ? "\016</TD></TR>\016":"\n");
     if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','-'));

     if(!Blank(ptr->email)) email = check_duplicates(NOTHING,decompress(ptr->email),0,1);
     if(IsHtml(p)) output(p,player,1,2,0,"<TR BGCOLOR="HTML_TABLE_DGREY"><TD ALIGN=LEFT COLSPAN=2>");
     output(p,player,2,1,1,"%sAdministrator currently dealing with request:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<B><I>\016"ANSI_LGREEN:ANSI_LGREEN" ",IsHtml(p) ? "\016</I></B> &nbsp; \016":"  ",Validchar(ptr->user) ? getname(ptr->user):"<AWAITING ATTENTION>",IsHtml(p) ? "\016<P>\016":"\n\n");
     output(p,player,2,1,1,"%sExisting users with similar E-mail addresses:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<B><I>\016"ANSI_LRED:ANSI_LRED" ",IsHtml(p) ? "\016</I></B> &nbsp; \016":"  ",!Blank(email) ? email:"None",IsHtml(p) ? "":"\n");
     if(IsHtml(p)) output(p,player,1,2,0,"</TD></TR>");

     if(!IsHtml(p) && !in_command) output(p,player,0,1,0,separator(twidth,1,'-','='));
     if(IsHtml(p)) output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
     html_anti_reverse(p,0);
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  List requests  <---- */
void request_list(dbref player,const char *email,const char *name)
{
     unsigned char            cached_scrheight,twidth = output_terminal_width(player);
     struct   descriptor_data *p = getdsc(player);
     int                      length,waiting = 0;
     const    char            *colour;
     struct   tm              *rtime;
     struct   request_data    *req;
     char                     *ptr;
     dbref                    who;

     /* ---->  List requests currently being handled by <NAME>, or list 'all' requests  <---- */
     email = parse_grouprange(player,email,FIRST,1);
     if(!Blank(name)) {
        if(strcasecmp("all",name)) {
           if((who = lookup_character(player,name,1)) == NOTHING) {
              output(p,player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",name);
              return;
	   }
	} else who = NOTHING;
     } else who = player;

     /* ---->  Count waiting requests  <---- */
     for(req = request; req; req = req->next)
         if(!Validchar(req->user))
            req->user = NOTHING, waiting++;

     html_anti_reverse(p,1);
     if(p && !p->pager && !IsHtml(p) && More(player)) pager_init(p);
     if(IsHtml(p)) output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");

     if(!in_command) {
        if(!IsHtml(p)) {
           output(p,player,0,1,0,"\n Number:  Date:       Administrator:        Preferred name and E-mail address:");
           output(p,player,0,1,0,separator(twidth,0,'-','='));
	} else output(p,player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Number:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Date:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Administrator:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Preferred name and E-mail address:</I></FONT></TH></TR>\016");
     }
     cached_scrheight                  = db[player].data->player.scrheight;
     db[player].data->player.scrheight = (db[player].data->player.scrheight - 7) * 2;

     set_conditions(player,0,0,0,who,!Blank(email) ? email:NULL,505);
     union_initgrouprange((union group_data *) request);
     while(union_grouprange()) {

           /* ---->  Request reference number  <---- */
           rtime = localtime(&(grp->cunion->request.date));
           if(Validchar(grp->cunion->request.user))
              colour = privilege_colour(grp->cunion->request.user);
                 else colour = ANSI_LWHITE;
           sprintf(scratch_buffer,IsHtml(p) ? "\016<TR ALIGN=CENTER><TD BGCOLOR="HTML_TABLE_RED">\016"ANSI_LRED"%d\016</TD><TD BGCOLOR="HTML_TABLE_MAGENTA">\016"ANSI_LMAGENTA"%02d/%02d/%04d":" "ANSI_LRED"%-9d"ANSI_LMAGENTA"%02d/%02d/%04d  ",grp->cunion->request.ref,rtime->tm_mday,rtime->tm_mon + 1,rtime->tm_year + 1900);

           /* ---->  Administrator's name  <---- */
           if(Validchar(grp->cunion->request.user)) {
              ptr = (char *) getfield(grp->cunion->request.user,PREFIX);
              if(!Blank(ptr) && ((strlen(ptr) + 1 + strlen(getname(grp->cunion->request.user))) <= 20))
                 sprintf(scratch_return_string,"%s %s",ptr,getname(grp->cunion->request.user));
                    else strcpy(scratch_return_string,getname(grp->cunion->request.user));
	   } else strcpy(scratch_return_string,"<AWAITING ATTENTION>");
           sprintf(scratch_buffer + strlen(scratch_buffer),IsHtml(p) ? "\016</TD><TD ALIGN=LEFT>\016%s%s":"%s%-22s",colour,scratch_return_string);

           /* ---->  Preferred name and E-mail address  <---- */
           if((length = (twidth - 45)) < 0) length = 0;
           strcpy(scratch_return_string,(grp->cunion->request.email) ? decompress(grp->cunion->request.email):"<E-MAIL UNKNOWN>");
           scratch_return_string[length] = '\0';
           output(p,player,2,1,44,IsHtml(p) ? "%s\016</TD><TD ALIGN=LEFT>\016"ANSI_LYELLOW"%s%s"ANSI_LGREEN"%s\016</TD></TR>\016":"%s"ANSI_LYELLOW"%s%s"ANSI_LGREEN"%s\n",scratch_buffer,(grp->cunion->request.name) ? decompress(grp->cunion->request.name):"",(grp->cunion->request.name) ? "\n":"",scratch_return_string);
     }
     if(Validchar(player)) db[player].data->player.scrheight = cached_scrheight;

     if(grp->rangeitems == 0) output(p,player,2,1,0,"%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TD COLSPAN=4>"ANSI_LCYAN"<I>*** &nbsp; NO REQUESTS LISTED &nbsp; ***</I></TD></TR>\016":" ***  NO REQUESTS LISTED  ***\n");
     if(!in_command) {
        if(!IsHtml(p)) output(p,player,2,1,0,separator(twidth,1,'-','='));
        output(p,player,2,1,1,"%sRequests listed: \016&nbsp;\016 "ANSI_DWHITE"%s\016 &nbsp; &nbsp; \016"ANSI_LWHITE"Awaiting attention: \016&nbsp;\016 "ANSI_DWHITE"%d.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><TD COLSPAN=4>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",listed_items(scratch_return_string,1),waiting,IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
     }
     if(IsHtml(p)) output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
     html_anti_reverse(p,0);
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Take on new request  <---- */
void request_new(dbref player)
{
     struct request_data *ptr;

     /* ---->  Find request which is not currently being dealt with  <---- */
     for(ptr = request; ptr && Validchar(ptr->user); ptr = ptr->next);
     if(ptr) {
        ptr->user = player;
        request_view(player,NULL,ptr->ref);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, there aren't any requests for new characters on the queue at the moment  -  Please try again later.");
}

/* ---->  Accept request  <---- */
void request_accept(dbref player,const char *reference,const char *name)
{
     struct request_data *ptr,*last;
     dbref  user;

     if(!Blank(reference)) {
        if((ptr = request_lookup(reference,0,&last))) {
           if(ptr->user == player) {
              if(!Blank(name)) {
                 if((user = lookup_nccharacter(player,name,0)) != NOTHING) {
                    if(!((user == player) || Level4(user) || Experienced(user) || Assistant(user))) {
                       writelog(REQUEST_LOG,1,"REQUEST","Request %d accepted by %s(#%d)  -  E-MAIL:  %s,  NEW CHARACTER:  %s(#%d).",ptr->ref,getname(player),player,!Blank(ptr->email) ? decompress(ptr->email):"<UNKNOWN>",getname(user),user);
                       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Request number "ANSI_LWHITE"%d"ANSI_LGREEN" has now been accepted for the new character %s"ANSI_LYELLOW"%s"ANSI_LGREEN".",ptr->ref,Article(user,LOWER,DEFINITE),getcname(NOTHING,user,0,0));
                       if(last) last->next = ptr->next;
                          else request = ptr->next;
                       FREENULL(ptr->email);
                       FREENULL(ptr->name);
                       FREENULL(ptr);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't specify yourself, an Assistant, Experienced Builder or Apprentice Wizard/Druid or above as the new character created in response to the request with the reference number "ANSI_LWHITE"%d"ANSI_LGREEN".",ptr->ref);
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist (Please specify the name of the new character in FULL.)",name);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the name (In FULL) of the new character you created in response to the request with the reference number "ANSI_LWHITE"%d"ANSI_LGREEN".",ptr->ref);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you aren't currently handling the new character request with the reference number "ANSI_LWHITE"%d"ANSI_LGREEN".",ptr->ref);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a new character request with the reference number '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",reference);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the reference number of the new character request to accept (Use '"ANSI_LWHITE"@request list"ANSI_LGREEN"' if you can't remember it.)");
}

/* ---->  Refuse request  <---- */
void request_refuse(dbref player,const char *reference,const char *reason)
{
     struct request_data *ptr,*last;

     if(!Blank(reference)) {
        if((ptr = request_lookup(reference,0,&last))) {
           if(ptr->user == player) {
              if(!Blank(reason)) {
                 strcpy(scratch_return_string,!Blank(ptr->email) ? decompress(ptr->email):"<UNKNOWN>");
                 writelog(REQUEST_LOG,1,"REQUEST","Request %d refused by %s(#%d)  -  NAME:  %s,  E-MAIL:  %s,  REASON:  %s",ptr->ref,getname(player),player,!Blank(ptr->name) ? decompress(ptr->name):"<UNKNOWN>",scratch_return_string,punctuate((char *) reason,2,'.'));
                 if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Request number "ANSI_LWHITE"%d"ANSI_LGREEN" for a new character has been refused for the E-mail address '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",ptr->ref,!Blank(ptr->email) ? decompress(ptr->email):"<UNKNOWN>");
                 if(last) last->next = ptr->next;
                    else request = ptr->next;
                 FREENULL(ptr->email);
                 FREENULL(ptr->name);
                 FREENULL(ptr);
                 setreturn(OK,COMMAND_SUCC);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the reason for refusing this request for a new character.");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you aren't currently handling the new character request with the reference number "ANSI_LWHITE"%d"ANSI_LGREEN".",ptr->ref);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a new character request with the reference number '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",reference);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the reference number of the new character request to refuse (Use '"ANSI_LWHITE"@request list"ANSI_LGREEN"' if you can't remember it.)");
}

/* ---->  Cancel handling a request  <---- */
void request_cancel(dbref player,const char *reference)
{
     struct request_data *ptr;

     if(!Blank(reference)) {
        if((ptr = request_lookup(reference,0,&ptr))) {
           if(ptr->user == player) {
              ptr->user = NOTHING;
              if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"You are no-longer handling the new character request with the reference number "ANSI_LWHITE"%d"ANSI_LGREEN" (It has been placed back on the queue of requests awaiting attention.)",ptr->ref);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you aren't currently handling the new character request with the reference number "ANSI_LWHITE"%d"ANSI_LGREEN".",ptr->ref);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a new character request with the reference number '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",reference);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the reference number of the new character request to cancel (Use '"ANSI_LWHITE"@request list"ANSI_LGREEN"' if you can't remember it.)");
}

/* ---->  Process requests for new characters  <---- */
void request_process(CONTEXT)
{
     char  command[32];
     short count = 0;
     const char *p1;

     setreturn(ERROR,COMMAND_FAIL);
     if(!in_command || Wizard(current_command)) {
        if(Level4(db[player].owner)) {

           /* ---->  Site command  <---- */
           for(; *arg1 && (*arg1 == ' '); arg1++);
           for(p1 = arg1; *p1 && (*p1 != ' ') && (count < 31); command[count] = *p1, count++, p1++);
           command[count] = '\0';
           for(; *p1 && (*p1 != ' '); p1++);
           for(; *p1 && (*p1 == ' '); p1++);
           if(!BlankContent(command)) {
	      if(string_prefix("newrequest",command)) {
                 request_new(player);
	      } else if(string_prefix("refuserequest",command)) {
                 request_refuse(player,p1,arg2);
	      } else if(string_prefix("acceptrequest",command)) {
                 request_accept(player,p1,arg2);
	      } else if(string_prefix("cancelrequest",command)) {
                 request_cancel(player,!Blank(arg2) ? arg2:p1);
	      } else if(string_prefix("listrequests",command)) {
                 request_list(player,p1,arg2);
	      } else {
                 if(!string_prefix("viewrequests",command)) p1 = arg1;
                 request_view(player,!Blank(arg2) ? arg2:p1,0);
	      }
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify a request command or the reference number of the request you'd like to view the details of.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may deal with requests for new characters.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, requests for new characters can't be dealt with from within a compound command.");
}
