/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| LOOK.C  -  Implements looking at/examining rooms, objects and characters.   |
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
|                        https://github.com/smcvey/tcz                        |
`-----------------------------------------------------------------------------'

*/


#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "structures.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "descriptor_flags.h"
#include "friend_flags.h"
#include "object_types.h"
#include "objectlists.h"
#include "flagset.h"
#include "fields.h"
#include "search.h"
#include "match.h"
#include "quota.h"


/* ---->  Display name of object, complete with credentials (If a connected character)  <---- */
const char *look_name_status(dbref player,dbref object,char *buffer,unsigned char spaces)
{
      unsigned char            afk = 0,converse = 0,editing = 0,monitored = 0,disconnected = 1;
      struct   descriptor_data *d,*p = getdsc(player);
      time_t                   total,now;
      unsigned long            longdate;
      int                      flags;

      /* ---->  Valid object?  <---- */
      strcpy(buffer,(spaces) ? "  ":"");
      if(!Valid(object)) {
         strcat(buffer,NOTHING_STRING);
         return(buffer);
      }

      /* ---->  Correct rank colour (If object is a character)  <---- */
      if((Typeof(object) == TYPE_CHARACTER) && Connected(object)) {
	 termflags = TXT_BOLD;
	 sprintf(buffer + strlen(buffer),"%s*",privilege_colour(object));
      } else strcat(buffer,ANSI_DWHITE);

      /* ---->  Name of object  <---- */
      strcat(buffer,(Typeof(object) == TYPE_CHARACTER) ? getcname(player,object,1,UPPER|INDEFINITE):unparse_object(player,object,UPPER|INDEFINITE));
      if(Moron(object)) strcat(buffer,"!");

      if(Typeof(object) == TYPE_CHARACTER) {

         /* ---->  Character specific credentials  <---- */
         if(Connected(object)) strcat(buffer,ANSI_LWHITE);
         if(IsHtml(p)) strcat(buffer,"\016&nbsp;<I>\016");

	 /* ---->  Character's current 'feeling'  <---- */
	 if(db[object].data->player.feeling) {
	    int loop;

	    for(loop = 0; feelinglist[loop].name && (feelinglist[loop].id != db[object].data->player.feeling); loop++);
	    if(feelinglist[loop].name) sprintf(buffer + strlen(buffer)," \016&nbsp;\016(%s)",feelinglist[loop].name);
	       else db[object].data->player.feeling = 0;
	 }

	 /* ---->  Rank of character  <---- */
	 if(Level1(object)) {
            strcat(buffer," \016&nbsp;\016(Deity)");
	 } else if(Level2(object)) {
            strcat(buffer,(Druid(object)) ? " \016&nbsp;\016(Elder Druid)":" \016&nbsp;\016(Elder Wizard)");
	 } else if(Level3(object)) {
            strcat(buffer,(Druid(object)) ? " \016&nbsp;\016(Druid)":" \016&nbsp;\016(Wizard)");
	 } else if(Level4(object)) {
            strcat(buffer,(Druid(object)) ? " \016&nbsp;\016(Apprentice Druid)":" \016&nbsp;\016(Apprentice Wizard)");
	 } else if(Retired(object)) {
            strcat(buffer,(RetiredDruid(object)) ? " \016&nbsp;\016(Retired Druid)":" \016&nbsp;\016(Retired Wizard)");
	 } else if(Experienced(object)) {
            strcat(buffer," \016&nbsp;\016(Experienced Builder)");
	 } else if(Assistant(object)) {
            strcat(buffer," \016&nbsp;\016(Assistant)");
	 }

	 /* ---->  HTML '(Scan)' clickable link  <---- */
	 if(IsHtml(p)) {
	    char htmlbuffer[TEXT_SIZE];

	    sprintf(buffer + strlen(buffer)," \016&nbsp;(<A HREF=\"%sNAME=*%s&SECTION=%c&\" TARGET=_blank TITLE=\"Click to scan %s%s...\">\016Scan\016</A>\016)",html_server_url(p,0,2,"scan"),html_encode(getname(object),htmlbuffer,NULL,256),isalpha(*(db[object].name)) ? *(db[object].name):'*',Article(player,object,LOWER|DEFINITE),getcname(player,object,0,0));
	 }
      } else if(IsHtml(p)) strcat(buffer,"\016&nbsp;<I>\016"ANSI_LWHITE);

      if(IsHtml(p)) {
         char  htmlbuffer[TEXT_SIZE];
         dbref command;

         /* ---->  HTML '(View)' clickable link  <---- */         
         if((Typeof(object) != TYPE_CHARACTER) && !Blank(getfield(object,DESC)))
  	    sprintf(buffer + strlen(buffer)," \016&nbsp;(<A HREF=\"%sSUBST=OK&COMMAND=%%7Clook+!%s&\" TARGET=TCZINPUT TITLE=\"Click to look at %s%s...\">\016View\016</A>\016)",html_server_url(p,1,2,"input"),html_encode(getname(object),htmlbuffer,NULL,TEXT_SIZE),Article(player,object,LOWER|DEFINITE),getname(object));

         /* ---->  HTML '(Use)' clickable link  <---- */         
         if((command = match_simple(object,".use",COMMANDS,0,1)) != NOTHING)
  	    sprintf(buffer + strlen(buffer)," \016&nbsp;(<A HREF=\"%sSUBST=OK&COMMAND=%%7Cuse+!%s&\" TARGET=TCZINPUT TITLE=\"Click to use %s%s...\">\016Use\016</A>\016)",html_server_url(p,1,2,"input"),html_encode(getname(object),htmlbuffer,NULL,TEXT_SIZE),Article(player,object,LOWER|DEFINITE),getname(object));

         /* ---->  HTML '(Take)' clickable link  <---- */         
         if((Location(object) != player) && (Typeof(object) != TYPE_CHARACTER) && CanTakeOrDrop(player,object) && !Immovable(object) && could_satisfy_lock(player,object,0) && !Transport(object))
  	    sprintf(buffer + strlen(buffer)," \016&nbsp;(<A HREF=\"%sSUBST=OK&COMMAND=%%7Ctake+!%s&\" TARGET=TCZINPUT TITLE=\"Click to pick up %s%s...\">\016Take\016</A>\016)",html_server_url(p,1,2,"input"),html_encode(getname(object),htmlbuffer,NULL,TEXT_SIZE),Article(player,object,LOWER|DEFINITE),getname(object));

         /* ---->  HTML '(Drop)' clickable link  <---- */         
         if((Location(object) == player) && (Typeof(object) != TYPE_CHARACTER) && CanTakeOrDrop(player,object) && !Immovable(object))
  	    sprintf(buffer + strlen(buffer)," \016&nbsp;(<A HREF=\"%sSUBST=OK&COMMAND=%%7Cdrop+!%s&\" TARGET=TCZINPUT TITLE=\"Click to drop %s%s...\">\016Drop\016</A>\016)",html_server_url(p,1,2,"input"),html_encode(getname(object),htmlbuffer,NULL,TEXT_SIZE),Article(player,object,LOWER|DEFINITE),getname(object));

         /* ---->  HTML '(Open)' clickable link  <---- */         
         if(Container(object) && Openable(object) && !Locked(object) && !Open(object))
  	    sprintf(buffer + strlen(buffer)," \016&nbsp;(<A HREF=\"%sSUBST=OK&COMMAND=%%7Copen+!%s&\" TARGET=TCZINPUT TITLE=\"Click to open %s%s...\">\016Open\016</A>\016)",html_server_url(p,1,2,"input"),html_encode(getname(object),htmlbuffer,NULL,TEXT_SIZE),Article(player,object,LOWER|DEFINITE),getname(object));

         /* ---->  HTML '(Close)' clickable link  <---- */         
         if(Container(object) && Openable(object) && !Locked(object) && Open(object))
  	    sprintf(buffer + strlen(buffer)," \016&nbsp;(<A HREF=\"%sSUBST=OK&COMMAND=%%7Cclose+!%s&\" TARGET=TCZINPUT TITLE=\"Click to close %s%s...\">\016Close\016</A>\016)",html_server_url(p,1,2,"input"),html_encode(getname(object),htmlbuffer,NULL,TEXT_SIZE),Article(player,object,LOWER|DEFINITE),getname(object));

         /* ---->  HTML '(Lock)' clickable link  <---- */         
         if(Container(object) && Openable(object) && !Locked(object))
  	    sprintf(buffer + strlen(buffer)," \016&nbsp;(<A HREF=\"%sSUBST=OK&COMMAND=%%7Clock+!%s&\" TARGET=TCZINPUT TITLE=\"Click to lock %s%s...\">\016Lock\016</A>\016)",html_server_url(p,1,2,"input"),html_encode(getname(object),htmlbuffer,NULL,TEXT_SIZE),Article(player,object,LOWER|DEFINITE),getname(object));

         /* ---->  HTML '(Unlock)' clickable link  <---- */         
         if(Container(object) && Openable(object) && Locked(object))
  	    sprintf(buffer + strlen(buffer)," \016&nbsp;(<A HREF=\"%sSUBST=OK&COMMAND=%%7Cunlock+!%s&\" TARGET=TCZINPUT TITLE=\"Click to lock %s%s...\">\016Unlock\016</A>\016)",html_server_url(p,1,2,"input"),html_encode(getname(object),htmlbuffer,NULL,TEXT_SIZE),Article(player,object,LOWER|DEFINITE),getname(object));
      }

      if(Typeof(object) == TYPE_CHARACTER) {

	 /* ---->  Set 'now'  <---- */
	 gettime(now);

	 /* ---->  Newbie  <---- */
	 total = db[object].data->player.totaltime + (now - db[object].data->player.lasttime);
	 if((total <= NEWBIE_TIME) && !Level4(object) && !Experienced(object) && !Assistant(object) && !Retired(object))
	    strcat(buffer," \016&nbsp;\016(Newbie)");

	 /* ---->  Friend/Enemy?  <---- */
	 if((flags = friend_flags(player,object)|friend_flags(object,player)) && !friendflags_set(player,object,NOTHING,FRIEND_EXCLUDE))
	    strcat(buffer,(flags & FRIEND_ENEMY) ? " \016&nbsp;\016(Enemy)":" \016&nbsp;\016(Friend)");

	 /* ---->  Birthday?  <---- */
	 longdate = epoch_to_longdate(now);
	 if(hasprofile(db[object].data->player.profile) && ((db[object].data->player.profile->dob & 0xFFFF) == (longdate & 0xFFFF)))
	    sprintf(buffer + strlen(buffer)," \016&nbsp;\016(%ld today!)",longdate_difference(db[object].data->player.profile->dob,longdate) / 12);

	 /* ---->  AFK/conversing/editing/lost connection?  <---- */
	 if(Connected(object)) {
	    for(d = descriptor_list; d; d = d->next) {
		if(d->player == object) {
		   if(d->monitor && Validchar(d->monitor->player) && Level4(player) && (player != d->player)) monitored = 1;
		   if(disconnected && !(d->flags & DISCONNECTED)) disconnected = 0;
		   if(!converse && (d->flags & CONVERSE)) converse = 1;
		   if(!editing && (d->edit)) editing = 1;
		   if(!afk && d->afk_message) afk = 1;
		}
	    }
	 } else disconnected = 0;

	 /* ---->  AFK/BEING/Connection Lost/Conversing/EDITING/Engaged/HAVEN/Married/Monitored/Puppet/QUIET?  <---- */
	 sprintf(buffer + strlen(buffer),"%s%s%s%s%s%s%s%s%s%s%s%s",(afk) ? " \016&nbsp;\016(AFK)":"",Being(object) ? " \016&nbsp;\016(Being)":"",(disconnected) ? " \016&nbsp;\016(Connection Lost)":"",(converse) ? " \016&nbsp;\016(Conversing)":"",(editing) ? " \016&nbsp;\016(Editing)":"",(Engaged(object) && Validchar(Partner(object))) ? " \016&nbsp;\016(Engaged)":"",Haven(object) ? " \016&nbsp;\016(Haven)":"",(Married(object) && Validchar(Partner(object))) ? " \016&nbsp;\016(Married)":"",(monitored) ? " \016&nbsp;\016(Monitored)":"",Puppet(object) ? " \016&nbsp;\016(Puppet)":"",Quiet(object) ? " \016&nbsp;\016(Quiet)":"",IsHtml(p) ? "\016</I>\016":"");
      }
      return(buffer);
}

/* ---->  Display description of an object  <---- */
unsigned char look_description(dbref player,const char *desc,unsigned char subst,unsigned char censor,unsigned char addcr)
{
	 const    char *ptr;
	 unsigned char cr;

	 if(!Blank(desc)) {
	    for(ptr = desc + strlen(desc) - 1; (ptr >= desc) && (*ptr == ' '); ptr--);
	    cr = ((ptr >= desc) && (*ptr == '\n'));
	    if(subst) {
	       sprintf(scratch_buffer,"%s%s%s%s",Html(player) ? "\016<FONT SIZE=4>\016":"",desc,Html(player) ? "\016</FONT>\016":"",(!addcr || cr) ? "\n":"\n\n");
	       ptr = punctuate(scratch_buffer,3,'.');
	       substitute_large(player,player,ptr,ANSI_LWHITE,scratch_buffer,censor);
	    } else output(getdsc(player),player,0,1,0,ANSI_LWHITE"%s%s%s%s",Html(player) ? "\016<FONT SIZE=4>\016":"",desc,Html(player) ? "\016</FONT>\016":"",(!addcr || cr) ? "":"\n");
	 } else output(getdsc(player),player,0,1,0,"");
	 return(!cr);
}

/* ---->  Look at/inside or examine container  <---- */
void look_container(dbref player,dbref object,const char *title,int level,unsigned char examine)
{
     struct   descriptor_data *p = getdsc(player);
     dbref                    thing,root_object,currentobj;
     unsigned char            contents = 0;
     char                     buffer[256];
     int                      count;

     if(level > 50) return;
     if(!Open(object) && Opaque(object)) return;
     root_object = object, termflags = TXT_NORMAL, thing = getfirst(object,CONTENTS,&currentobj);
     if(!IsHtml(p)) {
	for(count = 0; count < level; count++) buffer[count] = ' ';
	buffer[level] = '\0';
     }

     if(Valid(thing)) do {
	if((Typeof(thing) != TYPE_CHARACTER) || ((Typeof(thing) == TYPE_CHARACTER) && (Connected(thing) || (Controller(thing) != thing)))) {
	   if(!((Typeof(thing) == TYPE_CHARACTER) && (root_object != currentobj)) && (!Invisible(thing) || can_write_to(player,thing,0))) {
	      if(!contents) {
		 if(level) output(p,player,2,1,0,IsHtml(p) ? "\016<P>\016":"\n");
		    else if(IsHtml(p) && examine) output(p,player,1,2,0,"<TR><TD COLSPAN=2>");
		 if(!examine && HasField(object,CSTRING) && !Blank(getfield(object,CSTRING))) tilde_string(player,getfield(object,CSTRING),ANSI_LYELLOW,ANSI_DYELLOW,level,level,5);
		    else tilde_string(player,title,ANSI_LYELLOW,ANSI_DYELLOW,level,level,5);
		 if(IsHtml(p)) output(p,player,1,2,0,"<FONT COLOR="HTML_DYELLOW"><UL>");
		 contents = 1;
	      }

	      output(p,player,2,1,level + 2,"%s%s%s",IsHtml(p) ? "\016<FONT COLOR="HTML_DYELLOW"><LI>\016":buffer,look_name_status(player,thing,scratch_buffer,0),IsHtml(p) ? "\016</LI></FONT>\016":"\n");
	      termflags = TXT_NORMAL;
	      if(Container(thing)) look_container(player,thing,"Contents:",level + 2,examine);
	   }
	}
	getnext(thing,CONTENTS,currentobj);
     } while(Valid(thing));

     if(contents && (!level || IsHtml(p)))
	output(p,player,2,1,0,IsHtml(p) ? "\016</UL></FONT>%s\016":"\n%s",(examine && IsHtml(p)) ? "<FONT SIZE=1>&nbsp;</FONT></TD></TR>":"");
}

/* ---->  Display contents of location  <---- */
void look_contents(dbref player,dbref loc,const char *title)
{
     dbref                    thing,root_object,currentobj;
     unsigned char            visible,contents = 0;
     struct   descriptor_data *p = getdsc(player);

     /* ---->  Check to see if character can see location  <---- */
     visible = !(Invisible(loc) && (Number(player) || (!Level4(db[player].owner) && !can_write_to(player,loc,0))));
     root_object = loc, termflags = TXT_NORMAL, thing = getfirst(loc,CONTENTS,&currentobj);
     if(Valid(thing)) do {
	if(((Typeof(thing) != TYPE_CHARACTER) || ((Typeof(thing) == TYPE_CHARACTER) && (Connected(thing) || (Controller(thing) != thing)))) && can_see(player,thing,visible) && !((Typeof(thing) == TYPE_CHARACTER) && (root_object != currentobj))) {
	   if(!contents) {
	      contents = 1;
	      if(HasField(loc,CSTRING) && !Blank(getfield(loc,CSTRING))) tilde_string(player,getfield(loc,CSTRING),ANSI_LYELLOW,ANSI_DYELLOW,0,0,5);
		 else tilde_string(player,title,ANSI_LYELLOW,ANSI_DYELLOW,0,0,5);
	      if(IsHtml(p)) output(p,player,1,2,0,"<FONT COLOR="HTML_DYELLOW"><UL>");
	   }
	   output(p,player,2,1,4,"%s%s%s",IsHtml(p) ? "\016<FONT COLOR="HTML_DYELLOW"><LI>\016":"",look_name_status(player,thing,scratch_buffer,1),IsHtml(p) ? "\016</LI></FONT>\016":"\n");
	   termflags = TXT_NORMAL;
	}
	getnext(thing,CONTENTS,currentobj);
     } while(Valid(thing));
     if(contents) output(p,player,2,1,0,IsHtml(p) ? "\016</UL></FONT>\016":"\n");
}

/* ---->  Looks at an object, displaying its name and description  <---- */
void look_simple(dbref player,dbref object)
{
     struct descriptor_data *p = getdsc(player);
     char   *desc;

     if(!((desc = (char *) getfield(object,ODESC)) && (Location(player) != object) && ((Typeof(object) == TYPE_ROOM) || ((Typeof(object) == TYPE_THING) && !(Container(object) && Openable(object) && Open(object))))))
        desc = (char *) getfield(object,DESC);

     if(desc) {
        if(Typeof(object) == TYPE_COMMAND) {
           unsigned char twidth = output_terminal_width(player);
           short         loop,width;
           char          *ptr;

           strcpy(scratch_buffer,ANSI_LYELLOW);
           if(!IsHtml(p)) {
              width = (twidth - strlen("Commands executed by compound command") - 4) / 2;
              for(ptr = scratch_buffer + strlen(scratch_buffer), loop = 0; loop < width; *ptr++ = '-', loop++);
              *ptr = '\0', sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LWHITE"  %s  "ANSI_LYELLOW,"Commands executed by compound command");
              for(loop += strlen("Commands executed by compound command") + 4, ptr = scratch_buffer + strlen(scratch_buffer); loop < twidth; *ptr++ = '-', loop++);
              *ptr = '\0', output(p,player,0,1,0,scratch_buffer);
	   } else {
              html_anti_reverse(p,1);
              output(p,player,1,2,0,"<TABLE WIDTH=100% BORDER CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">");
              output(p,player,2,1,0,"\016<TR BGCOLOR="HTML_TABLE_YELLOW"><TH COLSPAN=2><FONT SIZE=4>"ANSI_LYELLOW"--- &nbsp; "ANSI_LWHITE"Commands executed by compound command &nbsp; "ANSI_LYELLOW"---</FONT></TH></TR>");
	   }

           if(termflags) termflags = TXT_NORMAL;
           if(!Private(object) || can_read_from(player,object)) {
              int    counter = 1,spaces;
              const  char *ptr,*ptr2;
              char   *tmp;

              for(*scratch_buffer = '\0', ptr = desc; *ptr; counter++) {
                  if(*ptr == '\n') ptr++;
                  for(tmp = scratch_return_string; *ptr && (*ptr != '\n'); *tmp++ = *ptr++);
                  *tmp = '\0';
                  if(!IsHtml(p)) {
                     for(ptr2 = scratch_return_string, spaces = 0; *ptr2 && (*ptr2 == ' '); ptr2++, spaces++);
                     if((spaces + 7) > 50) spaces = (50 - 7);
                     output(p,player,0,1,7 + spaces,ANSI_DGREEN"[%03d]  "ANSI_DWHITE"%s",counter,scratch_return_string);
		  } else output(p,player,2,1,0,"\016<TR><TH WIDTH=10%% ALIGN=CENTER BGCOLOR="HTML_TABLE_GREEN"><TT>\016"ANSI_DGREEN"[%03d]\016</TT></TH><TD ALIGN=LEFT>\016"ANSI_DWHITE"%s\016</TD></TR>\016",counter,scratch_return_string);
	      }
	   } else output(p,player,2,1,0,"%s"ANSI_LRED"This compound command is "ANSI_LYELLOW"PRIVATE"ANSI_LRED"  -  Only characters above the level of its owner (%s"ANSI_LWHITE"%s"ANSI_LRED") may see its description.%s",IsHtml(p) ? "\016<TR><TD>\016":"",Article(Owner(object),UPPER,INDEFINITE),getcname(NOTHING,Owner(object),0,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");

           if(termflags) termflags = TXT_BOLD;
           if(!IsHtml(p)) {
              strcpy(scratch_buffer,ANSI_LYELLOW), ptr = scratch_buffer + strlen(scratch_buffer);
              for(loop = 0; loop < twidth; *ptr++ = '-', loop++);
              *ptr++ = '\n', *ptr = '\0', output(p,player,0,1,0,scratch_buffer);
	   } else {
              output(p,player,1,2,0,"</TABLE><BR>");
              html_anti_reverse(p,1);
	   }
        } else if(!Private(object) || can_read_from(player,object)) {
           unsigned char cr;

           cr = look_description(player,desc,1,((Typeof(object) == TYPE_CHARACTER) || Censor(object)),0);
           if(Typeof(object) == TYPE_CHARACTER)
              command_execute_action(object,NOTHING,".lookdesc",NULL,getname(player),"",getname(player),0);
                 else command_execute_action(player,object,".lookdesc",NULL,getname(player),getnameid(player,object,NULL),"",1);
           if(cr) output(p,player,0,1,0,"");

           /* ---->  Credits  <---- */
           switch(Typeof(object)) {
                  case TYPE_THING:
                       if(currency_to_double(&(db[object].data->thing.credit)) != 0)
                          output(p,player,2,1,2,"%sCredits:%s%.2f%s",IsHtml(p) ? ANSI_DMAGENTA"\016<UL><LI><FONT COLOR="HTML_LMAGENTA"><B><I>\016":ANSI_LMAGENTA"  ",IsHtml(p) ? "\016</I></B></FONT> &nbsp; <FONT COLOR="HTML_LWHITE"><B><I>\016":"  "ANSI_LWHITE,currency_to_double(&(db[object].data->thing.credit)),IsHtml(p) ? "\016</I></B></FONT></LI></UL>\016":"\n\n");
                       break;
                  case TYPE_ROOM:
                       if(currency_to_double(&(db[object].data->room.credit)) != 0)
                          output(p,player,2,1,2,"%sCredits:%s%.2f%s",IsHtml(p) ? ANSI_DMAGENTA"\016<UL><LI><FONT COLOR="HTML_LMAGENTA"><B><I>\016":ANSI_LMAGENTA"  ",IsHtml(p) ? "\016</I></B></FONT> &nbsp; <FONT COLOR="HTML_LWHITE"><B><I>\016":"  "ANSI_LWHITE,currency_to_double(&(db[object].data->room.credit)),IsHtml(p) ? "\016</I></B></FONT></LI></UL>\016":"\n\n");
                       break;
           }
           if(Container(object)) look_container(player,object,"Contents:",0,0);
	} else output(p,player,0,1,0,ANSI_LRED"This %s is "ANSI_LYELLOW"PRIVATE"ANSI_LRED"  -  Only characters above the level of %s owner (%s"ANSI_LWHITE"%s"ANSI_LRED") may see %s description.\n",object_type(object,0),(Typeof(object) == TYPE_CHARACTER) ? "their":"its",Article(Owner(object),UPPER,INDEFINITE),getcname(NOTHING,Owner(object),0,0),(Typeof(object) == TYPE_CHARACTER) ? "their":"its");
     } else {
        if(Typeof(object) == TYPE_CHARACTER) output(p,player,0,1,0,ANSI_LGREEN"You see nothing special about them.\n");
	   else if(Typeof(object) == TYPE_COMMAND) output(p,player,0,1,0,ANSI_LGREEN"No commands are executed by this compound command.\n");
	      else output(p,player,0,1,0,ANSI_LGREEN"You see nothing special about it.\n");
        if(Container(object) && (!Private(object) || can_read_from(player,object)))
           look_container(player,object,"Contents:",0,0);
     }
}

/* ---->  Display obvious exits in given location  <---- */
void look_exits(dbref player,dbref location)
{
     char                     buffer[BUFFER_LEN],buffer2[BUFFER_LEN];
     struct   descriptor_data *p = getdsc(player);
     dbref                    exit,currentobj;
     unsigned char            exits = 0;

     termflags = TXT_NORMAL;
     exit = getfirst(location,EXITS,&currentobj);

     if(Valid(exit)) do {
        if(!Invisible(exit) && (Level4(Owner(player)) || (Valid(Destination(exit)) && ((Typeof(Destination(exit)) == TYPE_ROOM) || Typeof(Destination(exit)) == TYPE_THING)) || can_write_to(player,exit,0))) {
           if(!exits) {
              exits = 1;
              if(HasField(location,ESTRING) && !Blank(getfield(location,ESTRING))) tilde_string(player,getfield(location,ESTRING),ANSI_LCYAN,ANSI_DCYAN,0,0,5);
                 else tilde_string(player,"Obvious exits:",ANSI_LCYAN,ANSI_DCYAN,0,0,5);
              if(IsHtml(p)) output(p,player,1,2,0,"<FONT COLOR="HTML_DCYAN"><UL>");
	   }

           /* ---->  Exit name and clickable link to go through it  <---- */
           strcpy(buffer2,getexitname(Owner(player),exit));
           if(Valid(Destination(exit))) {
              if(!Opaque(exit)) {
                 sprintf(buffer,"%s%s lead%s to %s",IsHtml(p) ? "\016<FONT COLOR="HTML_DCYAN"><LI>\016":"  ",punctuate(buffer2,0,'\0'),(Articleof(exit) == ARTICLE_PLURAL) ? "":"s",Article(Destination(exit),LOWER,INDEFINITE));
                 sprintf(buffer + strlen(buffer),"%s%s",punctuate((char *) unparse_object(player,Destination(exit),0),0,'.'),IsHtml(p) ? "\016</LI></FONT>\016":"\n");
              } else sprintf(buffer,"%s%s%s",IsHtml(p) ? "\016<FONT COLOR="HTML_DCYAN"><LI>\016":"  ",punctuate(buffer2,0,'.'),IsHtml(p) ? "\016</LI></FONT>\016":"\n");
           } else sprintf(buffer,"%s%s lead%s nowhere.%s",IsHtml(p) ? "\016<FONT COLOR="HTML_DCYAN"><LI>\016":"  ",punctuate(buffer2,0,'\0'),(Articleof(exit) == ARTICLE_PLURAL) ? "":"s",IsHtml(p) ? "\016</LI></FONT>\016":"\n");

           /* ---->  HTML clickable links  <---- */
           if(IsHtml(p)) {
              strcat(buffer,"\016&nbsp;<I><FONT COLOR="HTML_LWHITE">\016");

              /* ---->  HTML '(View)' clickable link  <---- */
              if(!Blank(getfield(exit,DESC)))
                 sprintf(buffer + strlen(buffer)," \016&nbsp;(<A HREF=\"%sSUBST=OK&COMMAND=%%7Clook+!%s&\" TARGET=TCZINPUT TITLE=\"Click to look at %s%s...\">\016View\016</A>\016)",html_server_url(p,1,2,"input"),html_encode(getexit_firstname(player,exit,0),buffer2,NULL,TEXT_SIZE),Article(player,exit,LOWER|DEFINITE),getexit_firstname(player,exit,0));

	      /* ---->  HTML '(Open)' clickable link  <---- */         
	      if(Openable(exit) && !Locked(exit) && !Open(exit))
		 sprintf(buffer + strlen(buffer)," \016&nbsp;(<A HREF=\"%sSUBST=OK&COMMAND=%%7Copen+!%s&\" TARGET=TCZINPUT TITLE=\"Click to open %s%s...\">\016Open\016</A>\016)",html_server_url(p,1,2,"input"),html_encode(getexit_firstname(player,exit,0),buffer2,NULL,TEXT_SIZE),Article(player,exit,LOWER|DEFINITE),getexit_firstname(player,exit,0));

	      /* ---->  HTML '(Close)' clickable link  <---- */         
	      if(Openable(exit) && !Locked(exit) && Open(exit))
		 sprintf(buffer + strlen(buffer)," \016&nbsp;(<A HREF=\"%sSUBST=OK&COMMAND=%%7Cclose+!%s&\" TARGET=TCZINPUT TITLE=\"Click to close %s%s...\">\016Close\016</A>\016)",html_server_url(p,1,2,"input"),html_encode(getexit_firstname(player,exit,0),buffer2,NULL,TEXT_SIZE),Article(player,exit,LOWER|DEFINITE),getexit_firstname(player,exit,0));

	      /* ---->  HTML '(Lock)' clickable link  <---- */         
	      if(Openable(exit) && !Locked(exit))
		 sprintf(buffer + strlen(buffer)," \016&nbsp;(<A HREF=\"%sSUBST=OK&COMMAND=%%7Clock+!%s&\" TARGET=TCZINPUT TITLE=\"Click to lock %s%s...\">\016Lock\016</A>\016)",html_server_url(p,1,2,"input"),html_encode(getexit_firstname(player,exit,0),buffer2,NULL,TEXT_SIZE),Article(player,exit,LOWER|DEFINITE),getexit_firstname(player,exit,0));

	      /* ---->  HTML '(Unlock)' clickable link  <---- */         
	      if(Openable(exit) && Locked(exit))
		 sprintf(buffer + strlen(buffer)," \016&nbsp;(<A HREF=\"%sSUBST=OK&COMMAND=%%7Cunlock+!%s&\" TARGET=TCZINPUT TITLE=\"Click to lock %s%s...\">\016Unlock\016</A>\016)",html_server_url(p,1,2,"input"),html_encode(getexit_firstname(player,exit,0),buffer2,NULL,TEXT_SIZE),Article(player,exit,LOWER|DEFINITE),getexit_firstname(player,exit,0));

              strcat(buffer,"\016</I></FONT>\016");
	   }

           output(p,player,2,1,4,"%s",buffer);
	}
        getnext(exit,EXITS,currentobj);
     } while(Valid(exit));
     if(exits) output(p,player,2,1,0,IsHtml(p) ? "\016</UL></FONT>\016":"\n");
}

/* ---->  Look at contents of location (Room/Thing)  <---- */
void look_room(dbref player,dbref location)
{
     struct   descriptor_data *p = getdsc(player);
     unsigned char            cr;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Valid(location)) return;
     command_execute_action(player,location,".look",NULL,getname(player),getnameid(player,location,NULL),"",1);

     /* ---->  Name  <---- */
     tilde_string(player,unparse_object(player,location,UPPER|INDEFINITE),ANSI_LGREEN,ANSI_DGREEN,0,1,6);

     /* ---->  Description  <---- */
     termflags = TXT_NORMAL;
     if(getfield(location,ODESC) && (Location(player) != location) && ((Typeof(location) == TYPE_ROOM) || (Typeof(location) == TYPE_THING)))
        cr = look_description(player,getfield(location,ODESC),1,Censor(location),0);
           else cr = look_description(player,getfield(location,DESC),1,Censor(location),0);
     command_execute_action(player,location,".lookdesc",NULL,getname(player),getnameid(player,location,NULL),"",1);
     if(cr) output(p,player,0,1,0,"");

     /* ---->  Credits  <---- */
     switch(Typeof(location)) {
            case TYPE_THING:
                 if(currency_to_double(&(db[location].data->thing.credit)) != 0)
                    output(p,player,2,1,2,"%sCredits:%s%.2f%s",IsHtml(p) ? ANSI_DMAGENTA"\016<UL><LI><FONT COLOR="HTML_LMAGENTA"><B><I>\016":ANSI_LMAGENTA"  ",IsHtml(p) ? "\016</I></B></FONT> &nbsp; <FONT COLOR="HTML_LWHITE"><B><I>\016":"  "ANSI_LWHITE,currency_to_double(&(db[location].data->thing.credit)),IsHtml(p) ? "\016</I></B></FONT></LI></UL>\016":"\n\n");
                 break;
            case TYPE_ROOM:
                 if(currency_to_double(&(db[location].data->room.credit)) != 0)
                    output(p,player,2,1,2,"%sCredits:%s%.2f%s",IsHtml(p) ? ANSI_DMAGENTA"\016<UL><LI><FONT COLOR="HTML_LMAGENTA"><B><I>\016":ANSI_LMAGENTA"  ",IsHtml(p) ? "\016</I></B></FONT> &nbsp; <FONT COLOR="HTML_LWHITE"><B><I>\016":"  "ANSI_LWHITE,currency_to_double(&(db[location].data->room.credit)),IsHtml(p) ? "\016</I></B></FONT></LI></UL>\016":"\n\n");
                 break;
     }

     /* ---->  Success/failure messages (Based on room's lock)  <---- */
     termflags = TXT_BOLD;
     if((Typeof(location) != TYPE_THING) && (Location(player) == location) && !Openable(location))
        can_satisfy_lock(player,location,NULL,1);

     /* ---->  Obvious exits  <---- */
     if((Typeof(location) == TYPE_ROOM) || ((Typeof(location) == TYPE_THING) && (Container(location))))
        look_exits(player,location);

     /* ---->  Contents  <---- */
     look_contents(player,location,"Contents:");
     termflags = 0;

     command_execute_action(player,location,".looked",NULL,getname(player),getnameid(player,location,NULL),"",1);
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  List objects of given type attached to given object (Also, takes inheritance into account and lists object(s) inherited objects are inherited from)  <---- */
void look_examine_list(dbref player,dbref object,dbref objecttype,const char *title,const char *lansi,const char *dansi)
{
     dbref                    thing,currentobj,cur_object = NOTHING;
     struct   descriptor_data *p = getdsc(player);
     unsigned char            found = 0,list;

     if(objecttype != NOTHING) list = inlist[objecttype];
        else list = CONTENTS;
     if(!HasList(object,list)) return;

     termflags = TXT_NORMAL, cur_object = object, thing = getfirst(object,list,&currentobj);
     if(Valid(thing)) do {
        while(Valid(thing) && !((objecttype == NOTHING) || (Typeof(thing) == objecttype))) getnext(thing,list,currentobj);
        if(Valid(thing)) {
           if(!found) {
              if(IsHtml(p)) output(p,player,1,2,0,"<TR><TD COLSPAN=2>");
              sprintf(scratch_buffer,"%s:",title), found = 2;
              tilde_string(player,scratch_buffer,lansi,dansi,0,0,5);
              if(IsHtml(p)) output(p,player,1,2,0,"<FONT COLOR=%s><UL>",dansi);
	   }
           if(currentobj != cur_object) output(p,player,2,1,0,"%s"ANSI_DCYAN"[%s inherited from %s"ANSI_LCYAN"%s"ANSI_DCYAN"...]%s",(found == 2) ? "":IsHtml(p) ? "\016<P>\016":"\n",(objecttype == NOTHING) ? "Objects":title,Article(currentobj,LOWER,INDEFINITE),unparse_object(player,currentobj,0),IsHtml(p) ? "":"\n");
           if(found > 1) found = 1;
           output(p,player,2,1,2,"%s%s%s%s",dansi,IsHtml(p) ? "\016<LI>\016":"",look_name_status(player,thing,scratch_buffer,0),IsHtml(p) ? "":"\n");
           termflags = TXT_NORMAL;
	}
        cur_object = currentobj;
        getnext(thing,list,currentobj);
     } while(Valid(thing));
     if(found) output(p,player,2,1,0,IsHtml(p) ? "\016</UL></FONT><FONT SIZE=1>&nbsp;</FONT></TD></TR>\016":"\n");
}

/* ---->  Clear screen  <---- */
void look_cls(CONTEXT)
{
     int  height = ((Validchar(player) ? db[player].data->player.scrheight:STANDARD_CHARACTER_SCRHEIGHT) + 5);
     int  loop = 0;
     char *ptr;

     setreturn(OK,COMMAND_SUCC);
     for(ptr = scratch_return_string; (loop < height) && (loop < TEXT_SIZE); *ptr++ = '\n', loop++);
     *ptr = '\0';
     output(getdsc(player),player,0,1,0,ANSI_DWHITE""ANSI_IBLACK"%s",scratch_return_string);
}

/* ---->  Display current time and date  <---- */
void look_date(CONTEXT)
{
     struct descriptor_data *p = getdsc(player);
     time_t local,server;

     gettime(server);
     local = server + (db[player].data->player.timediff * HOUR);

     if(local != server) {
        output(p,player,0,1,0,"\n"ANSI_LGREEN"Your local time is "ANSI_LWHITE"%s"ANSI_LGREEN".",date_to_string(local,UNSET_DATE,player,FULLDATEFMT));
        output(p,player,0,1,0,"\n"ANSI_LGREEN"%s server time is "ANSI_LWHITE"%s"ANSI_LGREEN" ("ANSI_LYELLOW"%s"ANSI_LGREEN".)",tcz_short_name,date_to_string(server,UNSET_DATE,player,FULLDATEFMT),tcz_timezone);
     } else output(p,player,0,1,0,"\n"ANSI_LGREEN"The time is "ANSI_LWHITE"%s"ANSI_LGREEN" ("ANSI_LYELLOW"%s"ANSI_LGREEN".)",date_to_string(server,UNSET_DATE,player,FULLDATEFMT),tcz_timezone);
     output(p,player,0,1,0,ANSI_LGREEN"Your time difference is "ANSI_LWHITE"%+d"ANSI_LGREEN" hour%s (See '"ANSI_LYELLOW"help set timediff"ANSI_LGREEN"'.)\n",db[player].data->player.timediff,Plural(db[player].data->player.timediff));
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Display disclaimer  <---- */
void look_disclaimer(CONTEXT)
{
     struct descriptor_data *p = getdsc(player);
     const  char *ptr,*start;
     int    nopagedisc;

     if(player == NOBODY) {
        for(p = descriptor_list; p && (p->player != NOBODY); p = p->next);
        player = NOTHING;
        if(!p) return;
        if(!IsHtml(p)) p->player = NOTHING;
        nopagedisc = 1;

#ifdef PAGE_DISCLAIMER
     } else nopagedisc = 0;
#else
     } else nopagedisc = 1;
#endif

     if(!Blank(disclaimer)) {
	if(nopagedisc) {
           for(ptr = decompress(disclaimer), start = ptr; *ptr && ((*ptr == ' ') || (*ptr == '\n')); ptr++);
           for(; *ptr && (ptr > start) && (*(ptr - 1) == ' '); ptr--);
           if(*ptr && (*ptr == '\n')) ptr++;

           if(!IsHtml(p)) {
              if(!in_command && p && !p->pager && More(player)) pager_init(p);
              output(p,player,0,0,0,"\n"ANSI_LRED"%s",ptr);
	   } else output(p,player,0,1,0,"\n\016<TABLE BORDER=5 CELLPADDING=4 BGCOLOR=#FFFFDD><TR><TD><TT><B>\016"ANSI_DRED"%s\016</B></TT></TD></TR></TABLE>\016",ptr);
	} else {
           p->page_clevel = 0;
           p->clevel = 30;
           p->page = 1;
	}
     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, the disclaimer isn't available at the moment.");
}

/* ---->                  Examine object                 <---- */
/*        (val1:  0 = Full examine, 1 = Brief examine.)        */
void look_examine(CONTEXT)
{
     struct   descriptor_data *d = NULL,*p = getdsc(player);
     unsigned char            cr,bg = 0,subtable;
     unsigned char            experienced = 0;
     time_t                   last,total,now;
     dbref                    looper,thing;
     unsigned long            size,csize;
     int                      copied;
     long                     temp;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(params)) {
        thing = match_preferred(player,player,params,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
        if(!Valid(thing)) return;
        if(Typeof(thing) == TYPE_CHARACTER) d = getdsc(thing);
     } else {
        if(!Valid(db[player].location)) return;
        thing = db[player].location;
     }

     /* ---->  ('examine')  No permission to examine object, 'look' at it instead  <---- */
     if(!val1 && ((!Private(thing) && !(can_read_from(player,thing) || (experienced = ((Typeof(thing) == TYPE_CHARACTER) && Experienced(db[player].owner))))) || (Private(thing) && !can_read_from(player,thing)))) {
        look_at(player,params,arg0,arg1,arg2,1,0);
        return;
     }

     /* ---->  ('flags')  No permission to examine object, display owner  <---- */
     gettime(now);
     command_type |= (NO_USAGE_UPDATE|NO_AUTO_FORMAT);
     if((!Private(thing) && !(can_read_from(player,thing) || (experienced = ((Typeof(thing) == TYPE_CHARACTER) && Experienced(db[player].owner))))) || (Private(thing) && !can_read_from(player,thing))) {
        output(p,player,0,1,0,"");
        output(p,player,0,1,8,ANSI_LYELLOW"Owner: \016&nbsp;\016 "ANSI_LWHITE"%s.\n",getcname(player,db[thing].owner,1,UPPER|INDEFINITE));
        if(Level4(db[player].owner) && Private(thing))
           output(p,player,0,1,0,ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED" is "ANSI_LYELLOW"PRIVATE"ANSI_LRED" and can only be examined by characters of a higher level than %s owner.\n",Article(thing,UPPER,DEFINITE),getcname(player,thing,1,0),(Typeof(thing) == TYPE_CHARACTER) ? "their":"its");
	      else if((Typeof(thing) == TYPE_CHARACTER) || (Typeof(thing) == TYPE_THING) || (Typeof(thing) == TYPE_ROOM) || (Typeof(thing) == TYPE_EXIT))
                 output(p,player,0,1,0,ANSI_LGREEN"Type '"ANSI_LYELLOW"\016<A HREF=\"%sSUBST=OK&COMMAND=%%7C%s%s%s&\" TARGET=TCZINPUT><B>\016%s%s%s\016</B></A>\016"ANSI_LGREEN"' to see a more detailed description of %s"ANSI_LWHITE"%s"ANSI_LGREEN".\n",html_server_url(p,1,2,"input"),(Typeof(thing) == TYPE_CHARACTER) ? "scan":"look",Blank(arg1) ? "":"+",html_encode(params,scratch_return_string,&copied,256),(Typeof(thing) == TYPE_CHARACTER) ? "scan":"look",Blank(params) ? "":" ",params,Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0));
        command_type &= ~(NO_USAGE_UPDATE|NO_AUTO_FORMAT);
        return;
     }

     /* ---->  Object name  <---- */
     html_anti_reverse(p,1);
     if(!in_command && p && !p->pager && !IsHtml(p) && More(player)) pager_init(p);
     if(experienced && ((thing == player) || (thing == db[player].owner))) experienced = 0;
     if(IsHtml(p)) {
        output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
        output(p,player,2,1,0,"\016<TR BGCOLOR="HTML_TABLE_GREEN"><TH ALIGN=CENTER COLSPAN=2><FONT SIZE=5><B><I>\016"ANSI_LGREEN"%s\016</I></B></FONT></TH></TR>\016",getcname(player,thing,1,UPPER|INDEFINITE));
     } else tilde_string(player,getcname(player,thing,1,UPPER|INDEFINITE),ANSI_LGREEN,ANSI_DGREEN,0,1,6);
     termflags = 0;

     /* ---->  Object header (Type, Owner, Flags, Size, etc.)  <---- */
     if(IsHtml(p)) output(p,player,1,2,0,"<TR><TD ALIGN=LEFT CELLPADDING=0 BGCOLOR=%s COLSPAN=2><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(bg) ? HTML_TABLE_GREY:HTML_TABLE_DGREY), bg = !bg;

     /* ---->  Type and Building Quota Used/Limit  <---- */
     if(IsHtml(p)) output(p,player,2,1,0,"\016<TR><TH ALIGN=RIGHT WIDTH=30%% BGCOLOR="HTML_TABLE_CYAN"><I>\016"ANSI_LCYAN"Type:\016</I></TH><TD>\016"ANSI_LWHITE"%s\016</TD></TR>\016",ObjectType(thing));
        else sprintf(scratch_buffer," Type:  "ANSI_LWHITE"%s"ANSI_LCYAN";  Building Quota",ObjectType(thing));
     if(Typeof(thing) == TYPE_CHARACTER) {
        if(!Level4(thing)) output(p,player,2,1,8,"%s:%s%s%d/%d.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN"><I>\016"ANSI_LCYAN"Building Quota":scratch_buffer,IsHtml(p) ? "\016</I></TH><TD>\016":"  ",(db[thing].data->player.quota > db[thing].data->player.quotalimit) ? ANSI_LRED:ANSI_LWHITE,db[thing].data->player.quota,db[thing].data->player.quotalimit,IsHtml(p) ? "\016</TD></TR>\016":"\n");
           else output(p,player,2,1,8,"%s:%s"ANSI_LWHITE"%d/UNLIMITED.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN"><I>\016"ANSI_LCYAN"Building Quota":scratch_buffer,IsHtml(p) ? "\016</I></TH><TD>\016":"  ",db[thing].data->player.quota,IsHtml(p) ? "\016</TD></TR>\016":"\n");
     } else if(Typeof(thing) != TYPE_ARRAY) output(p,player,2,1,8,"%s used:%s"ANSI_LWHITE"%d.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN"><I>\016"ANSI_LCYAN"Building Quota":scratch_buffer,IsHtml(p) ? "\016</I></TH><TD>\016":"  ",ObjectQuota(thing),IsHtml(p) ? "\016</TD></TR>\016":"\n");
        else output(p,player,2,1,8,"%s used:%s"ANSI_LWHITE"%d.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN"><I>\016"ANSI_LCYAN"Building Quota":scratch_buffer,IsHtml(p) ? "\016</I></TH><TD>\016":"  ",ObjectQuota(thing) + (array_element_count(db[thing].data->array.start) * ELEMENT_QUOTA),IsHtml(p) ? "\016</TD></TR>\016":"\n");

     /* ---->  Owner  <---- */
     output(p,player,2,1,((Typeof(thing) == TYPE_CHARACTER) && (Owner(thing) != thing)) ? 18:8,"%s"ANSI_LCYAN"%swner:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN"><I>\016":"",((Typeof(thing) == TYPE_CHARACTER) && (Owner(thing) != thing)) ? "Effective o":"O",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",getcname(player,Owner(thing),1,UPPER|INDEFINITE),IsHtml(p) ? "\016</TD></TR>\016":"\n");

     /* ---->  Flags  <---- */
     output(p,player,2,1,8,"%s"ANSI_LCYAN"Flags:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",unparse_flaglist(thing,1,scratch_return_string),IsHtml(p) ? "\016</TD></TR>\016":"\n");

     /* ---->  Size (Memory usage)  <---- */
     size = getsize(thing,1), csize = getsize(thing,0);
     if(csize < size) output(p,player,2,1,8,"%s"ANSI_LCYAN"Size:%s"ANSI_LWHITE"%d byte%s (%d byte%s compressed (%.1f%%))%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN"><I>\016":" ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",size,Plural(size),csize,Plural(csize),100 - (((double) csize / size) * 100),IsHtml(p) ? "\016</TD></TR>\016":"\n");
        else output(p,player,2,1,8,"%s"ANSI_LCYAN"Size:%s"ANSI_LWHITE"%d byte%s (Not compressed.)%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN"><I>\016":" ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",size,Plural(size),IsHtml(p) ? "\016</TD></TR>\016":"\n");

     /* ---->  Lock  <---- */
     if(HasField(thing,LOCK)) output(p,player,2,1,(inherited > 0) ? 16:8,"%s"ANSI_LCYAN"%sey:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN"><I>\016":(inherited > 0) ? "":"  ",(inherited > 0) ? "Inherited k":"K",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",unparse_boolexp(player,getlock(thing,0),0),IsHtml(p) ? "\016</TD></TR>\016":"\n");

     /* ---->  Standard object fields (Description, Success, Failure, etc.)  <---- */
     if(!val1) {

        /* ---->  Object's description  <---- */
        if(IsHtml(p)) output(p,player,1,2,0,"</TABLE></TD></TR><TR><TD ALIGN=LEFT CELLPADDING=0 BGCOLOR=%s COLSPAN=2><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(bg) ? HTML_TABLE_GREY:HTML_TABLE_DGREY), bg = !bg;

        if(getfield(thing,DESC)) {
           if(Typeof(thing) == TYPE_COMMAND) {
              unsigned char twidth = output_terminal_width(player);
              int           counter = 1;
              short         loop,width;
              char          *tmp;
              const    char *title;
              const    char *ptr;

              title = (inherited > 0) ? "Commands (Inherited) executed by compound command":"Commands executed by compound command";
              if(!IsHtml(p)) {
                 strcpy(scratch_buffer,ANSI_LYELLOW"\n");
                 width = (twidth - strlen(title) - 4) / 2;
                 for(tmp = scratch_buffer + strlen(scratch_buffer), loop = 0; loop < width; *tmp++ = '-', loop++);
                 *tmp = '\0', sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LWHITE"  %s  "ANSI_LYELLOW,title);
                 for(loop += strlen(title) + 4, tmp = scratch_buffer + strlen(scratch_buffer); loop < twidth; *tmp++ = '-', loop++);
                 *tmp = '\0', output(p,player,0,1,0,scratch_buffer);
	      } else {
                 output(p,player,2,1,0,"\016<TR BGCOLOR="HTML_TABLE_YELLOW"><TH COLSPAN=2><FONT SIZE=4>"ANSI_LYELLOW"--- &nbsp; "ANSI_LWHITE"%s &nbsp; "ANSI_LYELLOW"---</FONT></TH></TR>",title);
                 output(p,player,1,2,0,"<TR BGCOLOR="HTML_TABLE_GREY"><TD COLSPAN=2 CELLPADDING=0><TABLE WIDTH=100% BORDER CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">");
	      }

              if(termflags) termflags = TXT_NORMAL;
              for(*scratch_buffer = '\0', ptr = getfield(thing,DESC); *ptr; counter++) {
                  int  spaces;
                  char *ptr2;

                  if(*ptr == '\n') ptr++;
                  for(tmp = scratch_return_string; *ptr && (*ptr != '\n'); *tmp++ = *ptr++);
                  *tmp = '\0';
                  if(!IsHtml(p)) {
                     for(ptr2 = scratch_return_string, spaces = 0; *ptr2 && (*ptr2 == ' '); ptr2++, spaces++);
                     if((spaces + 7) > 50) spaces = (50 - 7);
                     output(p,player,0,1,7 + spaces,ANSI_DGREEN"[%03d]  "ANSI_DWHITE"%s",counter,scratch_return_string);
		  } else output(p,player,2,1,0,"\016<TR><TH WIDTH=10%% ALIGN=CENTER BGCOLOR="HTML_TABLE_GREEN"><TT>\016"ANSI_DGREEN"[%03d]\016</TT></TH><TD ALIGN=LEFT>\016"ANSI_DWHITE"%s\016</TD></TR>\016",counter,scratch_return_string);
	      }

              if(termflags) termflags = 0;
              if(!IsHtml(p)) {
                 strcpy(scratch_buffer,ANSI_LYELLOW), tmp = scratch_buffer + strlen(scratch_buffer);
                 for(loop = 0; loop < twidth; *tmp++ = '-', loop++);
                 *tmp++ = '\n', *tmp = '\0', output(p,player,0,1,0,scratch_buffer);
	      } else output(p,player,1,2,0,"</TABLE></TD></TR>");
	   } else if(Typeof(thing) != TYPE_FUSE) {
              if(getfield(thing,DESC)) {
                 if(IsHtml(p)) {
                    if(inherited > 0) output(p,player,2,1,0,(getfield(thing,ODESC) && (Typeof(thing) != TYPE_CHARACTER)) ? "%s"ANSI_LGREEN"Inherited inside description ("ANSI_LWHITE"@desc"ANSI_LGREEN")...%s":"%s"ANSI_LGREEN"Inherited description...%s","\016<TR BGCOLOR="HTML_TABLE_GREEN"><TH ALIGN=CENTER COLSPAN=2><FONT SIZE=4><B><I>\016","\016</I></B></FONT></TH></TR>\016");
	               else output(p,player,2,1,0,(getfield(thing,ODESC) && (Typeof(thing) != TYPE_CHARACTER)) ? "%s"ANSI_LGREEN"Inside description ("ANSI_LWHITE"@desc"ANSI_LGREEN")...%s":"%s"ANSI_LGREEN"Description...%s","\016<TR><TH ALIGN=CENTER COLSPAN=2 BGCOLOR="HTML_TABLE_GREEN"><FONT SIZE=4><B><I>\016","\016</I></B></FONT></TH></TR>\016");
                    output(p,player,2,1,0,"\016<TR BGCOLOR="HTML_TABLE_GREY"><TD COLSPAN=2 CELLPADDING=0><TABLE WIDTH=100%% BORDER CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK"><TR><TD>\016"ANSI_LWHITE"%s\016</TD></TR></TABLE></TD></TR>\016",getfield(thing,DESC));
	         } else {
                    if(inherited > 0) output(p,player,0,1,0,(getfield(thing,ODESC) && (Typeof(thing) != TYPE_CHARACTER)) ? ANSI_DCYAN"\n[Inherited inside description ("ANSI_LWHITE"@desc"ANSI_DCYAN")...]":ANSI_DCYAN"\n[Inherited description...]");
	               else output(p,player,0,1,0,(getfield(thing,ODESC) && (Typeof(thing) != TYPE_CHARACTER)) ? ANSI_DCYAN"\n[Inside description ("ANSI_LWHITE"@desc"ANSI_DCYAN")...]":"");
	            output(p,player,0,1,0,ANSI_LWHITE"%s\n",getfield(thing,DESC));
		 }
	      }
	   } else {
              if(!IsHtml(p)) output(p,player,0,1,0,"");
              output(p,player,2,1,(inherited > 0) ? 25:15,"%s"ANSI_LGREEN"%sicks (Desc):%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN"><I>\016":"",(inherited > 0) ? "Inherited t":"T",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",getfield(thing,DESC),IsHtml(p) ? "\016</TD></TR>\016":"\n");
	   }
	} else if(Typeof(thing) == TYPE_FUSE) {
           if(!IsHtml(p)) output(p,player,0,1,0,"");
           output(p,player,2,1,(inherited > 0) ? 25:15,"%s"ANSI_LGREEN"%sicks (Desc):%s"ANSI_LWHITE"Unset.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN"><I>\016":"",(inherited > 0) ? "Inherited t":"T",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",IsHtml(p) ? "\016</TD></TR>\016":"\n");
	} else if(IsHtml(p)) {
           output(p,player,2,1,0,"\016<TR><TH ALIGN=CENTER COLSPAN=2 BGCOLOR="HTML_TABLE_GREEN"><FONT SIZE=4><B><I>"ANSI_LGREEN"Description...</I></B></FONT></TH></TR>\016");
           output(p,player,2,1,0,"\016<TR BGCOLOR="HTML_TABLE_GREY"><TD COLSPAN=2 CELLPADDING=0><TABLE WIDTH=100%% BORDER CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK"><TR><TD ALIGN=CENTER>"ANSI_DCYAN"<I>Not Set.</I></TD></TR></TABLE></TD></TR>\016");
	} else output(p,player,0,1,0,"");

        /* ---->  Reset of fuse (@drop)  <---- */
        if(Typeof(thing) == TYPE_FUSE) {
           if(getfield(thing,DROP)) output(p,player,2,1,(inherited > 0) ? 25:15,"%s"ANSI_LGREEN"%seset (Drop):%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN"><I>\016":"",(inherited > 0) ? "Inherited r":"R",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",getfield(thing,DROP),IsHtml(p) ? "\016</TD></TR>\016":"\n\n");
	      else output(p,player,2,1,(inherited > 0) ? 25:15,"%s"ANSI_LGREEN"%seset (Drop):%s"ANSI_LWHITE"Unset.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN"><I>\016":"",(inherited > 0) ? "Inherited r":"R",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",IsHtml(p) ? "\016</TD></TR>\016":"\n\n");
	}

        /* ---->  Outside description/web site address  <---- */
        if(getfield(thing,ODESC)) {
           if(Typeof(thing) != TYPE_CHARACTER) {
	      if(IsHtml(p)) {
                 output(p,player,2,1,0,"\016<TR><TH ALIGN=CENTER COLSPAN=2 BGCOLOR="HTML_TABLE_GREEN"><FONT SIZE=4><B><I>\016"ANSI_LGREEN"%sutside description ("ANSI_LWHITE"@odesc"ANSI_LGREEN")...\016</I></B></FONT></TH></TR>\016",(inherited > 0) ? "Inherited o":"O");
                 output(p,player,2,1,0,"\016<TR BGCOLOR="HTML_TABLE_GREY"><TD COLSPAN=2 CELLPADDING=0><TABLE WIDTH=100%% BORDER CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK"><TR><TD>\016"ANSI_LWHITE"%s\016</TD></TR></TABLE></TD></TR>\016",getfield(thing,ODESC));
	      } else output(p,player,0,1,0,ANSI_DCYAN"[%sutside description ("ANSI_LWHITE"@odesc"ANSI_DCYAN")...]\n"ANSI_LWHITE"%s\n",(inherited > 0) ? "Inherited o":"O",getfield(thing,ODESC));
	   } else output(p,player,2,1,(inherited > 0) ? 29:19,"%s"ANSI_LGREEN"%seb site address:%s"ANSI_LWHITE"\016<A HREF=\"%s\" TARGET=_blank>\016%s\016</A>\016%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN"><I>\016":"",(inherited > 0) ? "Inherited w":"W",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",html_encode_basic(IsHtml(p) ? getfield(thing,WWW):"",scratch_return_string,&copied,512),getfield(thing,WWW),IsHtml(p) ? "\016</TD></TR>\016":"\n\n");
	}

        /* ---->  Character's race  <---- */
        if((Typeof(thing) == TYPE_CHARACTER) && getfield(thing,RACE))
           output(p,player,2,1,(inherited > 0) ? 16:6,"%s"ANSI_LRED"%sace:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED"><I>\016":"",(inherited > 0) ? "Inherited r":"R",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",punctuate((char *) getfield(thing,RACE),0,'.'),IsHtml(p) ? "\016</TD></TR>\016":"\n\n");

        /* ---->  Success message/name prefix  <---- */
        if(getfield(thing,SUCC)) {
           if(Typeof(thing) == TYPE_CHARACTER)
              output(p,player,2,1,(inherited > 0) ? 24:14,"%s"ANSI_LGREEN"%same prefix:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN"><I>\016":"",(inherited > 0) ? "Inherited n":"N",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",getfield(thing,SUCC),IsHtml(p) ? "\016</TD></TR>\016":"\n"), cr = 1;
	         else output(p,player,2,1,0,"%s"ANSI_LGREEN"%success:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN"><I>\016":"",(inherited > 0) ? "Inherited s":"S",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",getfield(thing,SUCC),IsHtml(p) ? "\016</TD></TR>\016":"\n"), cr = 1;
	} else cr = 0;

        /* ---->  Others success message/name suffix  <---- */
        if(getfield(thing,OSUCC)) {
           if(Typeof(thing) == TYPE_CHARACTER) output(p,player,2,1,(inherited > 0) ? 24:14,"%s"ANSI_LGREEN"%same suffix:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN"><I>\016":"",(inherited > 0) ? "Inherited n":"N",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",getfield(thing,OSUCC),IsHtml(p) ? "\016</TD></TR>\016":"\n"), cr = 1;
	      else output(p,player,2,1,0,"%s"ANSI_LGREEN"%ssuccess:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN"><I>\016":"",(inherited > 0) ? "Inherited o":"O",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",getfield(thing,OSUCC),IsHtml(p) ? "\016</TD></TR>\016":"\n"), cr = 1;
	}
        if(cr && !IsHtml(p)) output(p,player,0,1,0,"");

	if(Typeof(thing) != TYPE_CHARACTER) {
           cr = 0;

           /* ---->  Failure message  <---- */
   	   if(getfield(thing,FAIL))
              output(p,player,2,1,0,"%s"ANSI_LGREEN"%sailure:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN"><I>\016":"",(inherited > 0) ? "Inherited f":"F",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",getfield(thing,FAIL),IsHtml(p) ? "\016</TD></TR>\016":"\n"), cr = 1;

           /* ---->  Others failure message  <---- */
    	   if(getfield(thing,OFAIL))
              output(p,player,2,1,0,"%s"ANSI_LGREEN"%sfailure:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN"><I>\016":"",(inherited > 0) ? "Inherited o":"O",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",getfield(thing,OFAIL),IsHtml(p) ? "\016</TD></TR>\016":"\n"), cr = 1;
           if(cr && !IsHtml(p)) output(p,player,0,1,0,"");
	}

        /* ---->  Drop message/E-mail address  <---- */
        cr = 0;
        if((Typeof(thing) != TYPE_FUSE) && getfield(thing,DROP)) {
           if(Typeof(thing) == TYPE_CHARACTER) {
              const char *email = getfield(thing,EMAIL);
              const char *emailaddr;
              int   counter = 1;

              if((emailaddr = forwarding_address(thing,1,scratch_return_string)))
                 output(p,player,2,1,(inherited > 0) ? 41:31,"%s"ANSI_LGREEN"%sE-mail forwarding address:%s"ANSI_LWHITE"\016<A HREF=\"mailto:%s\">\016%s\016</A>\016%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN"><I>\016":"   ",(inherited > 0) ? "Inherited ":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",html_encode(IsHtml(p) ? emailaddr:"",scratch_buffer,&copied,256),emailaddr,IsHtml(p) ? "\016</TD></TR>\016":"\n"), cr = 1;

              for(; counter <= EMAIL_ADDRESSES; counter++) {
                  if(((counter != 2) || Level4(db[player].owner) || can_write_to(player,thing,1)) && (emailaddr = gettextfield(counter,'\n',email,0,scratch_return_string)) && *emailaddr) {
                     if(!strcasecmp("forward",emailaddr)) output(p,player,2,1,(inherited > 0) ? 41:31,"%s"ANSI_LGREEN"%s%s (%s) E-mail address:%s"ANSI_DCYAN"<FORWARDED TO PRIVATE ADDRESS>%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN"><I>\016":(counter != 2) ? " ":"",(inherited > 0) ? "Inherited ":"",rank(counter),(counter == 2) ? "Private":"Public",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",IsHtml(p) ? "\016</TD></TR>\016":"\n"), cr = 1;
                        else output(p,player,2,1,(inherited > 0) ? 41:31,"%s"ANSI_LGREEN"%s%s (%s) E-mail address:%s"ANSI_LWHITE"\016<A HREF=\"mailto:%s\">\016%s\016</A>\016%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN"><I>\016":(counter != 2) ? " ":"",(inherited > 0) ? "Inherited ":"",rank(counter),(counter == 2) ? "Private":"Public",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",html_encode(IsHtml(p) ? emailaddr:"",scratch_buffer,&copied,256),emailaddr,IsHtml(p) ? "\016</TD></TR>\016":"\n"), cr = 1;
		  }
	      }
              if(cr && !IsHtml(p)) output(p,player,0,1,0,""), cr = 0;
	   } else output(p,player,2,1,0,"%s"ANSI_LGREEN"%srop:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN"><I>\016":"",(inherited > 0) ? "Inherited d":"D",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",getfield(thing,DROP),IsHtml(p) ? "\016</TD></TR>\016":"\n"), cr = 1;
	}

        /* ---->  Others drop message/title  <---- */
        if(getfield(thing,ODROP)) {
           if(Typeof(thing) == TYPE_CHARACTER) {
              if(cr && !IsHtml(p)) output(p,player,0,1,0,"");
              output(p,player,2,1,(inherited > 0) ? 18:8,"%s"ANSI_LGREEN"%sitle:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN"><I>\016":"",(inherited > 0) ? "Inherited t":"T",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",getfield(thing,TITLE),IsHtml(p) ? "\016</TD></TR>\016":"\n"), cr = 1;
	   } else output(p,player,2,1,0,"%s"ANSI_LGREEN"%sdrop:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN"><I>\016":"",(inherited > 0) ? "Inherited o":"O",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",getfield(thing,ODROP),IsHtml(p) ? "\016</TD></TR>\016":"\n"), cr = 1;
	}
        if(cr && !IsHtml(p)) output(p,player,0,1,0,"");

        /* ---->  Character login time fields  <---- */
        cr = 0;
        if(Typeof(thing) == TYPE_CHARACTER) {
           struct descriptor_data *w = getdsc(thing);
           time_t idle,active;

           if(IsHtml(p)) output(p,player,1,2,0,"</TABLE></TD></TR><TR><TD ALIGN=LEFT CELLPADDING=0 BGCOLOR=%s COLSPAN=2><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(bg) ? HTML_TABLE_GREY:HTML_TABLE_DGREY), bg = !bg;

           /* ---->  Last time connected  <---- */
           last = db[thing].data->player.lasttime;
           if(last > 0) {
              total = now - last;
              if(last  > 0) last += (db[player].data->player.timediff * HOUR);
              if(total > 0) output(p,player,2,1,23,"%s"ANSI_LCYAN"Last time connected:%s"ANSI_LWHITE"%s \016&nbsp;\016"ANSI_DCYAN"("ANSI_LYELLOW"%s "ANSI_LCYAN"ago"ANSI_DCYAN".)%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN"><I>\016":" ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",(last == 0) ? "Unknown":date_to_string(last,UNSET_DATE,player,FULLDATEFMT),interval(total,total,ENTITIES,0),IsHtml(p) ? "\016</TD></TR>\016":"\n"), cr = 1;
                 else output(p,player,2,1,23,"%s"ANSI_LCYAN"Last time connected:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN"><I>\016":" ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",(last == 0) ? "Unknown":date_to_string(last,UNSET_DATE,player,FULLDATEFMT),IsHtml(p) ? "\016</TD></TR>\016":"\n"), cr = 1;
	   }

           /* ---->  Longest connect time  <---- */
           if(Connected(thing)) {
              if((total = (now - db[thing].data->player.lasttime)) == now) total = 0;
              if(db[thing].data->player.longesttime < total) db[thing].data->player.longesttime = total;
	   }
           if(db[thing].data->player.longesttime > 0)
              output(p,player,2,1,23,"%s"ANSI_LCYAN"Longest connect time:%s"ANSI_LWHITE"%s \016&nbsp;\016"ANSI_DCYAN"("ANSI_LYELLOW"%s"ANSI_DCYAN".)%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",interval(db[thing].data->player.longesttime,db[thing].data->player.longesttime,ENTITIES,0),date_to_string(TimeDiff(db[thing].data->player.longestdate,player),UNSET_DATE,player,FULLDATEFMT),IsHtml(p) ? "\016</TD></TR>\016":"\n"), cr = 1;

           /* ---->  Total time connected  <---- */
           total = db[thing].data->player.totaltime;
           if(Connected(thing)) total += (now - db[thing].data->player.lasttime);
           strcpy(scratch_return_string,interval(total / ((db[thing].data->player.logins > 1) ? db[thing].data->player.logins:1),0,ENTITIES,0));
           if(total > 0) output(p,player,2,1,23,"%s"ANSI_LCYAN"Total time connected:%s"ANSI_LWHITE"%s \016&nbsp;\016"ANSI_DCYAN"("ANSI_LCYAN"Average "ANSI_LYELLOW"%s"ANSI_DCYAN".)%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",interval(total,total,ENTITIES,0),scratch_return_string,IsHtml(p) ? "\016</TD></TR>\016":"\n"), cr = 1;

	   /* ---->  Time spent idle  <---- */
	   idle = db[thing].data->player.idletime;
	   if(Connected(thing) && w) idle += (now - w->last_time);
	   strcpy(scratch_return_string,interval(idle / ((db[thing].data->player.logins > 1) ? db[thing].data->player.logins:1),0,ENTITIES,0));
	   if(idle > 0) output(p,player,2,1,23,"%sTime spent idling:%s"ANSI_LWHITE"%s \016&nbsp;\016"ANSI_DCYAN"("ANSI_LCYAN"Average "ANSI_LYELLOW"%s"ANSI_DCYAN".)%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED">"ANSI_LRED"<B><I>\016":ANSI_LRED"   ",IsHtml(d) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",interval(idle,idle,ENTITIES,0),scratch_return_string,IsHtml(d) ? "\016</TD></TR>\016":"\n");

	   /* ---->  Time spent active  <---- */
	   active = total - idle;
	   strcpy(scratch_return_string,interval(active / ((db[thing].data->player.logins > 1) ? db[thing].data->player.logins:1),0,ENTITIES,0));
	   if(active > 0) output(p,player,2,1,23,"%sTime spent active:%s"ANSI_LWHITE"%s \016&nbsp;\016"ANSI_DCYAN"("ANSI_LCYAN"Average "ANSI_LYELLOW"%s"ANSI_DCYAN".)%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN">"ANSI_LGREEN"<B><I>\016":ANSI_LGREEN"   ",IsHtml(d) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",interval(active,active,ENTITIES,0),scratch_return_string,IsHtml(d) ? "\016</TD></TR>\016":"\n");

           /* ---->  Site character last connected from  <---- */
           if(!experienced && !Blank(getfield(thing,LASTSITE)))
              output(p,player,2,1,(inherited < 1) ? 23:32,"%s"ANSI_LCYAN"%sast connected from:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN"><I>\016":"",(inherited > 0) ? "Inherited l":IsHtml(p) ? "L":" L",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",getfield(thing,LASTSITE),IsHtml(p) ? "\016</TD></TR>\016":"\n"), cr = 1;

           /* ---->  Total logins  <---- */
           total = ((now - db[thing].created) / ((db[thing].data->player.logins > 1) ? db[thing].data->player.logins:1));
           output(p,player,2,1,23,"%sTotal logins:%s"ANSI_LWHITE"%d \016&nbsp;\016"ANSI_DCYAN"("ANSI_LCYAN"Average "ANSI_LYELLOW"%s"ANSI_DCYAN".)%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN">"ANSI_LCYAN"<B><I>\016":ANSI_LCYAN"        ",IsHtml(d) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",db[thing].data->player.logins,interval(ABS((total < MINUTE) ? MINUTE:total),0,ENTITIES,0),IsHtml(d) ? "\016</TD></TR>\016":"\n");
	}
        if(cr && !IsHtml(p)) output(p,player,0,1,0,"");

        /* ---->  Lock key, 'Contents:' string and 'Obvious Exits:' string  <---- */
        cr = 0;
        if(IsHtml(p)) output(p,player,1,2,0,"</TABLE></TD></TR>");

        if(Typeof(thing) == TYPE_THING)
           output(p,player,2,1,(inherited > 0) ? 21:11,"%s"ANSI_LGREEN"%sock key:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN"><I>\016":"",(inherited > 0) ? "Inherited l":"L",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",unparse_boolexp(player,getlock(thing,1),0),IsHtml(p) ? "\016</TD></TR>\016":"\n\n");
        if(!Blank(getfield(thing,CSTRING)))
           output(p,player,2,1,(inherited > 0) ? 28:18,"%s"ANSI_LGREEN"%sontents string:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN"><I>\016":"",(inherited > 0) ? "Inherited c":"C",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",getfield(thing,CSTRING),IsHtml(p) ? "\016</TD></TR>\016":"\n"), cr = 1;
        if(!Blank(getfield(thing,ESTRING)))
           output(p,player,2,1,(inherited > 0) ? 25:15,"%s"ANSI_LGREEN"%sxits string:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN"><I>\016":"",(inherited > 0) ? "Inherited e":"E",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",getfield(thing,ESTRING),IsHtml(p) ? "\016</TD></TR>\016":"\n"), cr = 1;
        if(cr && !IsHtml(p)) output(p,player,0,1,0,"");

        if(Container(thing)) look_container(player,thing,"Contents:",0,1);
           else look_examine_list(player,thing,NOTHING,"Contents",ANSI_LYELLOW,ANSI_DYELLOW);
        look_examine_list(player,thing,TYPE_EXIT,"Exits",ANSI_LCYAN,ANSI_DCYAN);
        look_examine_list(player,thing,TYPE_COMMAND,"Compound commands/modules",ANSI_LGREEN,ANSI_DGREEN);
        look_examine_list(player,thing,TYPE_VARIABLE,"Variables",ANSI_LGREEN,ANSI_DGREEN);
        look_examine_list(player,thing,TYPE_PROPERTY,"Properties",ANSI_LGREEN,ANSI_DGREEN);
        look_examine_list(player,thing,TYPE_ARRAY,"Dynamic arrays",ANSI_LGREEN,ANSI_DGREEN);
        look_examine_list(player,thing,TYPE_FUSE,"Fuses",ANSI_LCYAN,ANSI_DCYAN);
        look_examine_list(player,thing,TYPE_ALARM,"Alarms",ANSI_LRED,ANSI_DRED);

        /* ---->  Display elements of dynamic array  <---- */
        termflags = 0;
        if(Typeof(thing) == TYPE_ARRAY) {
           if(IsHtml(p)) output(p,player,1,2,0,"<TR BGCOLOR="HTML_TABLE_GREY"><TD COLSPAN=2 ALIGN=CENTER>");
           if(array_display_elements(player,elementfrom,elementto,thing,IsHtml(p) ? 2:0) < 1)
              output(p,player,2,1,0,ANSI_LRED"Sorry, that element (Or range of elements) is invalid.%s",IsHtml(p) ? "":"\n\n");
           if(IsHtml(p)) output(p,player,1,2,0,"</TD></TR>");
	}

        subtable = 0;
        if(d) {
           if(IsHtml(p)) output(p,player,1,2,0,"<TR><TD ALIGN=LEFT CELLPADDING=0 BGCOLOR=%s COLSPAN=2><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(bg) ? HTML_TABLE_GREY:HTML_TABLE_DGREY), bg = !bg, subtable = 1;

           /* ---->  Terminal dimensions and type  <---- */
           if((d->terminal_width > 0) && (d->terminal_height > 0)) sprintf(scratch_return_string," ("ANSI_LWHITE"%dx%d"ANSI_LCYAN")",d->terminal_width + 1,d->terminal_height);
              else if(d->terminal_width > 0) sprintf(scratch_return_string," ("ANSI_LWHITE"%d columns"ANSI_LCYAN")",d->terminal_width + 1);
                 else if(d->terminal_height > 0) sprintf(scratch_return_string," ("ANSI_LWHITE"%d lines"ANSI_LCYAN")",d->terminal_height);
                    else strcpy(scratch_return_string,ANSI_LWHITE".");

           if(d->terminal_type) output(p,player,2,1,16,"%s"ANSI_LCYAN"Terminal Type:%s"ANSI_LWHITE"%s"ANSI_LCYAN"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",d->terminal_type,scratch_return_string,IsHtml(p) ? "\016</TD></TR>\016":"\n\n");
              else output(p,player,2,1,16,"%s"ANSI_LCYAN"Terminal Type:%s"ANSI_LWHITE"Unknown"ANSI_LCYAN"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",scratch_return_string,IsHtml(p) ? "\016</TD></TR>\016":"\n\n");

           /* ---->  Chatting channel  <---- */
           if(d && ((player == thing) || Level4(Owner(player))))
              if(d->channel != NOTHING) output(p,player,2,1,0,"%s"ANSI_LCYAN"Chatting channel:%s"ANSI_LWHITE"%d.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",d->channel,IsHtml(p) ? "\016</TD></TR>\016":"\n\n");

           /* ---->  Monitor (Visible to Admin only)  <---- */
           if(d && Level4(player) && d->monitor && Validchar(d->monitor->player) && (player != thing))
              output(p,player,2,1,15,"%s"ANSI_LBLUE"Monitored by:%s"ANSI_LWHITE"%s%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_BLUE"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",getcname(player,d->monitor->player,1,UPPER|INDEFINITE),(d->flags & MONITOR_OUTPUT) ? (d->flags & MONITOR_CMDS) ? ANSI_DCYAN" (Commands and output monitored.)":ANSI_DCYAN" (Output monitored only.)":(d->flags & MONITOR_CMDS) ? ANSI_DCYAN" (Commands monitored only.)":".",IsHtml(p) ? "\016</TD></TR>\016":"\n\n");
	}

        /* ---->  Banned character?  <---- */
        if((Typeof(thing) ==  TYPE_CHARACTER) && db[thing].data->player.bantime) {
           if(!subtable && IsHtml(p)) output(p,player,1,2,0,"<TR><TD ALIGN=LEFT CELLPADDING=0 BGCOLOR=%s COLSPAN=2><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(bg) ? HTML_TABLE_GREY:HTML_TABLE_DGREY), bg = !bg, subtable = 1;

           if(db[thing].data->player.bantime != -1) {
	      if((db[thing].data->player.bantime - now) > 0)
                 output(p,player,2,1,0,"%s"ANSI_LRED"This character is banned for "ANSI_LWHITE"%s"ANSI_LRED".%s",IsHtml(p) ? "\016<TR><TH ALIGN=CENTER COLSPAN=2 BGCOLOR="HTML_TABLE_RED"><I>\016":"",interval(db[thing].data->player.bantime - now,db[thing].data->player.bantime - now,ENTITIES,0),IsHtml(p) ? "\016</I></TH></TR>\016":"\n\n");
	            else db[thing].data->player.bantime = 0;
	   } else output(p,player,2,1,0,"%s"ANSI_LRED"This character is banned permanently.%s",IsHtml(p) ? "\016<TR><TH ALIGN=CENTER COLSPAN=2 BGCOLOR="HTML_TABLE_RED"><I>\016":"",IsHtml(p) ? "\016</I></TH></TR>\016":"\n\n");
	}
     } else if(!IsHtml(p)) output(p,player,0,1,0,"");

     /* ---->  Date of last use, creation date and expiry  <---- */
     if(IsHtml(p)) output(p,player,1,2,0,"%s<TR><TD ALIGN=LEFT CELLPADDING=0 BGCOLOR=%s COLSPAN=2><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(subtable) ? "</TABLE></TD></TR>":"",(bg) ? HTML_TABLE_GREY:HTML_TABLE_DGREY), bg = !bg, subtable = 0;
     output(p,player,2,1,16,"%s"ANSI_LMAGENTA"Creation date:%s"ANSI_LWHITE"%s ("ANSI_LYELLOW"%s"ANSI_LWHITE" ago.)%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_MAGENTA"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",date_to_string(db[thing].created,UNSET_DATE,player,FULLDATEFMT),interval(now - db[thing].created,now - db[thing].created,ENTITIES,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,16,"%s"ANSI_LMAGENTA"Last used:%s"ANSI_LWHITE"%s ("ANSI_LYELLOW"%s"ANSI_LWHITE" ago.)%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_MAGENTA"><I>\016":"    ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",date_to_string(db[thing].lastused,UNSET_DATE,player,FULLDATEFMT),interval(now - db[thing].lastused,now - db[thing].lastused,ENTITIES,0),IsHtml(p) ? "\016</TD></TR>\016":"\n",((db[thing].expiry > 0) || IsHtml(p)) ? "":"\n");

     if(db[thing].expiry > 0) {
        if((temp = (db[thing].expiry * DAY)) > (now - (Expiry(thing) ? db[thing].created:db[thing].lastused)))
           temp = (db[thing].expiry * DAY) - (now - (Expiry(thing)? db[thing].created:db[thing].lastused));
	      else temp = 0;
        if(temp > 0) output(p,player,2,1,16,"%s"ANSI_LMAGENTA"Expiry time:%s"ANSI_LWHITE"%d day%s ("ANSI_LYELLOW"%s"ANSI_LWHITE" left.)%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_MAGENTA"><I>\016":"  ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",db[thing].expiry,Plural(db[thing].expiry),interval(temp,temp,ENTITIES,0),IsHtml(p) ? "\016</TD></TR>\016":"\n\n");
           else output(p,player,2,1,16,"%s"ANSI_LMAGENTA"Expiry time:%s"ANSI_LWHITE"%d day%s ("ANSI_LRED""ANSI_BLINK"No time remaining"ANSI_LWHITE".)%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_MAGENTA"><I>\016":"  ",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",db[thing].expiry,Plural(db[thing].expiry),IsHtml(p) ? "\016</TD></TR>\016":"\n\n");
     }

     /* ---->  Location  <---- */
     cr = 0, subtable = 0;
     if(IsHtml(p)) output(p,player,1,2,0,"</TABLE></TD></TR><TR><TD ALIGN=LEFT CELLPADDING=0 BGCOLOR=%s COLSPAN=2><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(bg) ? HTML_TABLE_GREY:HTML_TABLE_DGREY), bg = !bg;
     if(Valid(db[thing].location)) {
        if(!((Secret(thing) || Secret(db[thing].location)) && !can_write_to(player,db[thing].location,1) && !can_write_to(player,thing,1))) {
           dbref area = get_areaname_loc(db[thing].location);
           if(Valid(area) && !Blank(getfield(area,AREANAME)))
              sprintf(scratch_return_string,ANSI_LYELLOW" in "ANSI_LWHITE"%s"ANSI_LYELLOW,getfield(area,AREANAME));
                 else *scratch_return_string = '\0';
           output(p,player,2,1,11,"%s"ANSI_LYELLOW"Location:%s"ANSI_LWHITE"%s%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",unparse_object(player,db[thing].location,UPPER|INDEFINITE),scratch_return_string,IsHtml(p) ? "\016</TD></TR>\016":"\n"), cr = 1;
	} else output(p,player,2,1,11,"%s"ANSI_LYELLOW"Location:%s"ANSI_LWHITE"Secret.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",IsHtml(p) ? "\016</TD></TR>\016":"\n"), cr = 1;
        subtable = 1;
     }

     /* ---->  Object's parent object  <---- */
     if(Valid(db[thing].parent)) {
        output(p,player,2,1,9,"%s"ANSI_LYELLOW"Parent:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",unparse_object(player,db[thing].parent,UPPER|INDEFINITE),IsHtml(p) ? "\016</TD></TR>\016":"\n"), cr = 1;
        subtable = 1;
     }

     switch(Typeof(thing)) {
            case TYPE_ROOM:
                
                 /* ---->  Drop-to location  <---- */
                 if(Valid(db[thing].destination)) {
                    output(p,player,2,1,24,"%s"ANSI_LYELLOW"Dropped objects go to:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",unparse_object(player,db[thing].destination,UPPER|INDEFINITE),IsHtml(p) ? "\016</TD></TR>\016":"\n"), cr = 1;
                    subtable = 1;
		 }

                 /* ---->  Credits dropped in room  <---- */
                 if(currency_to_double(&(db[thing].data->room.credit)) != 0) {
                    output(p,player,2,1,0,"%s"ANSI_LYELLOW"Credit:%s"ANSI_LWHITE"%.2f credits.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",currency_to_double(&(db[thing].data->room.credit)),IsHtml(p) ? "\016</TD></TR>\016":"\n"), cr = 1;
                    subtable = 1;
		 }

                 if(!val1) {

                    /* ---->  Area name  <---- */
                    if(!Blank(getfield(thing,AREANAME))) {
                       output(p,player,2,1,(inherited > 0) ? 22:12,"%s"ANSI_LYELLOW"%srea name:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",(inherited > 0) ? "Inherited a":"A",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",getfield(thing,AREANAME),IsHtml(p) ? "\016</TD></TR>\016":"\n"), cr = 1;
                       subtable = 1;
		    }

                    /* ---->  Weight of contents  <---- */
                    if(IsHtml(p) && subtable) output(p,player,1,2,0,"</TABLE></TD></TR><TR><TD ALIGN=LEFT CELLPADDING=0 BGCOLOR=%s COLSPAN=2><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(bg) ? HTML_TABLE_GREY:HTML_TABLE_DGREY), bg = !bg;
                    if(cr && !IsHtml(p)) output(p,player,0,1,0,"");
                    temp = getweight(thing);
                    output(p,player,2,1,0,"%s"ANSI_LRED"Weight of contents:%s"ANSI_LWHITE"%d Kilogram%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",temp,Plural(temp),IsHtml(p) ? "\016</TD></TR>\016":"\n");

                    /* ---->  Volume limit  <---- */
                    if((temp = getvolumeroom(thing)) != TCZ_INFINITY)
                       output(p,player,2,1,0,"%s"ANSI_LRED"%solume limit:%s"ANSI_LWHITE"%d Litre%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED"><I>\016":"",(inherited > 0) ? "Inherited v":"V",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",temp,Plural(temp),IsHtml(p) ? "\016</TD></TR>\016":"\n");
                          else output(p,player,2,1,0,"%s"ANSI_LRED"%solume limit:%s"ANSI_LWHITE"Infinity.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED"><I>\016":"",(inherited > 0) ? "Inherited v":"V",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",IsHtml(p) ? "\016</TD></TR>\016":"\n");

                    /* ---->  Mass limit  <---- */
                    if((temp = getmassroom(thing)) != TCZ_INFINITY)
                       output(p,player,2,1,0,"%s"ANSI_LRED"%sass limit:%s"ANSI_LWHITE"%d Kilogram%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED"><I>\016":"",(inherited > 0) ? "Inherited m":"M",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",temp,Plural(temp),IsHtml(p) ? "\016</TD></TR>\016":"\n");
                          else output(p,player,2,1,0,"%s"ANSI_LRED"%sass limit:%s"ANSI_LWHITE"Infinity.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED"><I>\016":"",(inherited > 0) ? "Inherited m":"M",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",IsHtml(p) ? "\016</TD></TR>\016":"\n");
		 }
                 break;
            case TYPE_CHARACTER:

                 /* ---->  Who character is currently building as  <---- */
                 if((Uid(thing) != thing) && Validchar(Uid(thing))) {
                    output(p,player,2,1,24,"%s"ANSI_LYELLOW"Currently building as:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",getcname(player,Uid(thing),1,UPPER|INDEFINITE),IsHtml(p) ? "\016</TD></TR>\016":"\n");
                    subtable = 1;
		 }

                 /* ---->  Mail redirect  <---- */
                 if(Validchar(db[thing].data->player.redirect)) {
                    output(p,player,2,1,21,"%s"ANSI_LYELLOW"Mail redirected to:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",getcname(player,db[thing].data->player.redirect,1,UPPER|INDEFINITE),IsHtml(p) ? "\016</TD></TR>\016":"\n");
                    subtable = 1;
		 }

                 if(!val1) {

                    /* ---->  Time difference  <---- */
                    if(db[thing].data->player.timediff) {
                       output(p,player,2,1,0,"%s"ANSI_LYELLOW"Time difference:%s"ANSI_LWHITE"%d hour%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",db[thing].data->player.timediff,Plural(db[thing].data->player.timediff),IsHtml(p) ? "\016</TD></TR>\016":"\n");
                       subtable = 1;
		    }

                    /* ---->  Screen height  <---- */
                    if(db[thing].data->player.scrheight) {
                       output(p,player,2,1,0,"%s"ANSI_LYELLOW"Screen height:%s"ANSI_LWHITE"%d line%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",db[thing].data->player.scrheight,Plural(db[thing].data->player.scrheight),IsHtml(p) ? "\016</TD></TR>\016":"\n");
                       subtable = 1;
		    }
 		 }

                 /* ---->  Controller  <---- */
                 if((Controller(thing) != thing) && Validchar(Controller(thing))) {
                    output(p,player,2,1,13,"%s"ANSI_LYELLOW"Controller:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",getcname(player,Controller(thing),1,UPPER|INDEFINITE),IsHtml(p) ? "\016</TD></TR>\016":"\n");
                    subtable = 1;
		 }

                 if(!val1) {

                    /* ---->  Partner (If married/engaged)  <---- */
                    if(Valid(Partner(thing))) {
                       output(p,player,2,1,13,"%s"ANSI_LYELLOW"%s to:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",Married(thing) ? "Married":"Engaged",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",getcname(player,Partner(thing),1,UPPER|INDEFINITE),IsHtml(p) ? "\016</TD></TR>\016":"\n");
                       subtable = 1;
		    }

                    /* ---->  Mail count  <---- */
                    if(db[thing].data->player.mail) {
                       short  count = 0,unread = 0;
                       struct mail_data *ptr;

                       for(ptr = db[thing].data->player.mail; ptr; ptr = ptr->next, count++)
                           if(ptr->flags & MAIL_UNREAD) unread++;
                       output(p,player,2,1,0,"%s"ANSI_LYELLOW"Mail items:%s"ANSI_LWHITE"%d (%d unread.)%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",count,unread,IsHtml(p) ? "\016</TD></TR>\016":"\n");
                       subtable = 1;
		    }
		 }

		 /* ---->  Mail limit  <---- */
                 if(!experienced) {
                    output(p,player,2,1,0,"%s"ANSI_LYELLOW"Mail limit:%s"ANSI_LWHITE"%d.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",db[thing].data->player.maillimit,IsHtml(p) ? "\016</TD></TR>\016":"\n");
                    subtable = 1;
		 }

                 if(!val1) {

                    /* ---->  Aliases  <---- */
                    if(!experienced) {
                       int    inherited_aliases = 0;
                       struct alias_data *ptr,*ptr2;
                       dbref  object;

                       for(looper = 0, ptr = db[thing].data->player.aliases; ptr; ptr = (ptr->next == db[thing].data->player.aliases) ? NULL:ptr->next) {
                           for(ptr2 = db[thing].data->player.aliases; ptr2 && !((ptr2->id == ptr->id) || (ptr2 == ptr)); ptr2 = (ptr2->next == db[thing].data->player.aliases) ? NULL:ptr2->next);
                           if(ptr2 && (ptr2 == ptr)) looper++;
		       }

                       for(object = db[thing].parent; Valid(object); object = db[object].parent)
                           for(ptr = db[object].data->player.aliases; ptr; ptr = (ptr->next == db[object].data->player.aliases) ? NULL:ptr->next) {
                               for(ptr2 = db[object].data->player.aliases; ptr2 && !((ptr2->id == ptr->id) || (ptr2 == ptr)); ptr2 = (ptr2->next == db[object].data->player.aliases) ? NULL:ptr2->next);
                               if(ptr2 && (ptr2 == ptr)) inherited_aliases++;
			   }

                       if((looper > 0) || (inherited_aliases > 0)) {
                          if(looper > 0) {
                             if(inherited_aliases > 0) output(p,player,2,1,10,"%s"ANSI_LYELLOW"Aliases:%s"ANSI_LWHITE"%d (%d inherited.)%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",looper + inherited_aliases,inherited_aliases,IsHtml(p) ? "\016</TD></TR>\016":"\n");
                                else output(p,player,2,1,10,"%s"ANSI_LYELLOW"Aliases:%s"ANSI_LWHITE"%d.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",looper,IsHtml(p) ? "\016</TD></TR>\016":"\n");
		          } else output(p,player,2,1,20,"%s"ANSI_LYELLOW"Inherited aliases:%s"ANSI_LWHITE"%d.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",inherited_aliases,IsHtml(p) ? "\016</TD></TR>\016":"\n");
                          subtable = 1;
		       }
		    }

                    /* ---->  Feeling  <---- */
                    if(db[thing].data->player.feeling) {
                       int loop;

                       for(loop = 0; feelinglist[loop].name && (feelinglist[loop].id != db[thing].data->player.feeling); loop++);
                       if(feelinglist[loop].name) {
                          output(p,player,2,1,10,"%s"ANSI_LYELLOW"Feeling:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",feelinglist[loop].name,IsHtml(p) ? "\016</TD></TR>\016":"\n");
                          subtable = 1;
		       }
		    }

                    /* ---->  Friends/enemies  <---- */
                    if(!experienced && db[thing].data->player.friends) {
                       int    friends = 0,enemies = 0,exclude = 0;
                       struct friend_data *ptr;

                       for(looper = 0, ptr = db[thing].data->player.friends; ptr; ptr = ptr->next)
                           if(ptr->flags & FRIEND_EXCLUDE) exclude++;
                              else if(ptr->flags & FRIEND_ENEMY) enemies++;
                                 else friends++;

                       if(friends > 0) {
                          output(p,player,2,1,0,"%s"ANSI_LYELLOW"Friends:%s"ANSI_LWHITE"%d.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",friends,IsHtml(p) ? "\016</TD></TR>\016":"\n");
                          subtable = 1;
		       }

                       if(enemies > 0) {
                          output(p,player,2,1,0,"%s"ANSI_LYELLOW"Enemies:%s"ANSI_LWHITE"%d.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",enemies,IsHtml(p) ? "\016</TD></TR>\016":"\n");
                          subtable = 1;
		       }

                       if(exclude > 0) {
                          output(p,player,2,1,0,"%s"ANSI_LYELLOW"Excluded:%s"ANSI_LWHITE"%d.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",exclude,IsHtml(p) ? "\016</TD></TR>\016":"\n");
                          subtable = 1;
		       }
		    }

                    /* ---->  Credit (Pocket)  <---- */
                    if(currency_to_double(&(db[thing].data->player.credit)) != 0)
                       output(p,player,2,1,0,"%s"ANSI_LYELLOW"Credit:%s"ANSI_LWHITE"%.2f credits.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",currency_to_double(&(db[thing].data->player.credit)),IsHtml(p) ? "\016</TD></TR>\016":"\n");
		          else output(p,player,2,1,0,"%s"ANSI_LYELLOW"Credit:%s"ANSI_LWHITE"None.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",IsHtml(p) ? "\016</TD></TR>\016":"\n");
                    subtable = 1;

                    /* ---->  Balance (Bank)  <---- */
                    if(currency_to_double(&(db[thing].data->player.balance)) != 0)
                       output(p,player,2,1,0,"%s"ANSI_LYELLOW"Balance:%s"ANSI_LWHITE"%.2f credits.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",currency_to_double(&(db[thing].data->player.balance)),IsHtml(p) ? "\016</TD></TR>\016":"\n");
		          else output(p,player,2,1,0,"%s"ANSI_LYELLOW"Balance:%s"ANSI_LWHITE"None.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",IsHtml(p) ? "\016</TD></TR>\016":"\n");
                    subtable = 1;

                    /* ---->  Score  <---- */
                    output(p,player,2,1,0,"%s"ANSI_LYELLOW"Score:%s"ANSI_LWHITE"%d point%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",db[thing].data->player.score,Plural(db[thing].data->player.score),IsHtml(p) ? "\016</TD></TR>\016":"\n");
                    subtable = 1;
		 }

                 /* ---->  Home room  <---- */
                 output(p,player,2,1,7,"%s"ANSI_LYELLOW"Home:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",unparse_object(player,db[thing].destination,UPPER|INDEFINITE),IsHtml(p) ? "\016</TD></TR>\016":"\n");
                 subtable = 1;

                 if(!val1) {

                    /* ---->  Weight  <---- */
                    if(IsHtml(p) && subtable) output(p,player,1,2,0,"</TABLE></TD></TR><TR><TD ALIGN=LEFT CELLPADDING=0 BGCOLOR=%s COLSPAN=2><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(bg) ? HTML_TABLE_GREY:HTML_TABLE_DGREY), bg = !bg;
                    temp = getweight(thing);
                    output(p,player,2,1,0,"%s"ANSI_LRED"Weight:%s"ANSI_LWHITE"%d Kilogram%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED"><I>\016":"\n",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",temp,Plural(temp),IsHtml(p) ? "\016</TD></TR>\016":"\n");

                    /* ---->  Volume  <---- */
                    if((temp = getvolumeplayer(thing)) != TCZ_INFINITY)
                       output(p,player,2,1,0,"%s"ANSI_LRED"%solume:%s"ANSI_LWHITE"%d Litre%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED"><I>\016":"",(inherited > 0) ? "Inherited v":"V",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",temp,Plural(temp),IsHtml(p) ? "\016</TD></TR>\016":"\n");
                          else output(p,player,2,1,0,"%s"ANSI_LRED"%solume:%s"ANSI_LWHITE"Infinity.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED"><I>\016":"",(inherited > 0) ? "Inherited v":"V",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",IsHtml(p) ? "\016</TD></TR>\016":"\n");

                    /* ---->  Mass  <---- */
                    if((temp = getmassplayer(thing)) != TCZ_INFINITY)
                       output(p,player,2,1,0,"%s"ANSI_LRED"%sass:%s"ANSI_LWHITE"%d Kilogram%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED"><I>\016":"",(inherited > 0) ? "Inherited m":"M",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",temp,Plural(temp),IsHtml(p) ? "\016</TD></TR>\016":"\n");
                          else output(p,player,2,1,0,"%s"ANSI_LRED"%sass:%s"ANSI_LWHITE"Infinity.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED"><I>\016":"",(inherited > 0) ? "Inherited m":"M",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",IsHtml(p) ? "\016</TD></TR>\016":"\n");
		 }
                 break;
            case TYPE_THING:

                 /* ---->  Area name  <---- */
                 if(!val1 && !Blank(getfield(thing,AREANAME))) {
                    output(p,player,2,1,(inherited > 0) ? 22:12,"%s"ANSI_LYELLOW"%srea name:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",(inherited > 0) ? "Inherited a":"A",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",getfield(thing,AREANAME),IsHtml(p) ? "\016</TD></TR>\016":"\n");
                    subtable = 1;
		 }

                 /* ---->  Credit dropped in thing  <---- */
                 if(currency_to_double(&(db[thing].data->thing.credit)) != 0) {
                    output(p,player,2,1,0,"%s"ANSI_LYELLOW"Credit:%s"ANSI_LWHITE"%.2f credits.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",currency_to_double(&(db[thing].data->thing.credit)),IsHtml(p) ? "\016</TD></TR>\016":"\n");
                    subtable = 1;
		 }

                 /* ---->  Home location  <---- */
                 output(p,player,2,1,7,"%s"ANSI_LYELLOW"Home:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",unparse_object(player,db[thing].destination,UPPER|INDEFINITE),IsHtml(p) ? "\016</TD></TR>\016":"\n\n");
                 subtable = 1;

                 if(!val1) {

                    /* ---->  Weight  <---- */
                    if(IsHtml(p) && subtable) output(p,player,1,2,0,"</TABLE></TD></TR><TR><TD ALIGN=LEFT CELLPADDING=0 BGCOLOR=%s COLSPAN=2><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(bg) ? HTML_TABLE_GREY:HTML_TABLE_DGREY), bg = !bg;
                    temp = getweight(thing);
                    output(p,player,2,1,0,"%s"ANSI_LRED"Weight:%s"ANSI_LWHITE"%d Kilogram%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",temp,Plural(temp),IsHtml(p) ? "\016</TD></TR>\016":"\n");

                    /* ---->  Volume  <---- */
                    if((temp = getvolumething(thing)) != TCZ_INFINITY)
                       output(p,player,2,1,0,"%s"ANSI_LRED"%solume:%s"ANSI_LWHITE"%d Litre%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED"><I>\016":"",(inherited > 0) ? "Inherited v":"V",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",temp,Plural(temp),IsHtml(p) ? "\016</TD></TR>\016":"\n");
                          else output(p,player,2,1,0,"%s"ANSI_LRED"%solume:%s"ANSI_LWHITE"Infinity.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED"><I>\016":"",(inherited > 0) ? "Inherited v":"V",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",IsHtml(p) ? "\016</TD></TR>\016":"\n");

                    /* ---->  Mass  <---- */
                    if((temp = getmassthing(thing)) != TCZ_INFINITY)
                       output(p,player,2,1,0,"%s"ANSI_LRED"%sass:%s"ANSI_LWHITE"%d Kilogram%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED"><I>\016":"",(inherited > 0) ? "Inherited m":"M",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",temp,Plural(temp),IsHtml(p) ? "\016</TD></TR>\016":"\n");
                          else output(p,player,2,1,0,"%s"ANSI_LRED"%sass:%s"ANSI_LWHITE"Infinity.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED"><I>\016":"",(inherited > 0) ? "Inherited m":"M",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",IsHtml(p) ? "\016</TD></TR>\016":"\n");
		 }
                 break;
            case TYPE_EXIT:

                 /* ---->  Destination location  <---- */
                 if(Valid(db[thing].destination)) {
                    output(p,player,2,1,14,"%s"ANSI_LYELLOW"Destination:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",unparse_object(player,db[thing].destination,UPPER|INDEFINITE),IsHtml(p) ? "\016</TD></TR>\016":"\n");
                    subtable = 1;
		 }
                 break;
            case TYPE_COMMAND:
            case TYPE_FUSE:

                 /* ---->  Success/failure links  <---- */
                 if(IsHtml(p) && subtable) {
                    output(p,player,1,2,0,"</TABLE></TD></TR><TR><TD ALIGN=LEFT CELLPADDING=0 BGCOLOR=%s COLSPAN=2><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(bg) ? HTML_TABLE_GREY:HTML_TABLE_DGREY), bg = !bg;
		 } else output(p,player,0,1,0,"");
                 output(p,player,2,1,30,"%s"ANSI_LGREEN"On success, execute (CSUCC):%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",unparse_object(player,db[thing].contents,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
                 output(p,player,2,1,30,"%s"ANSI_LRED"On failure, execute (CFAIL):%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",unparse_object(player,db[thing].exits,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
                 break;
            case TYPE_ALARM:

                 /* ---->  Compound command executed by alarm  <---- */
                 if(IsHtml(p) && subtable) {
                    output(p,player,1,2,0,"</TABLE></TD></TR><TR><TD ALIGN=LEFT CELLPADDING=0 BGCOLOR=%s COLSPAN=2><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(bg) ? HTML_TABLE_GREY:HTML_TABLE_DGREY), bg = !bg;
		 } else output(p,player,0,1,0,"");
                 output(p,player,2,1,36,"%s"ANSI_LGREEN"Compound command executed (CSUCC):%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD>\016":"  ",unparse_object(player,db[thing].destination,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
                 break;
            default:
                 break;
     }

     if(!IsHtml(p)) output(p,player,0,1,0,"");
        else output(p,player,1,2,0,"</TABLE></TD></TR></TABLE>%s",(!in_command) ? "<BR>":"");
     termflags = 0, command_type &= ~NO_USAGE_UPDATE;
     html_anti_reverse(p,0);
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Display idle time of a character  <---- */
void look_idle(CONTEXT)
{
     struct descriptor_data *p = getdsc(player);
     struct descriptor_data *ptr,*d = NULL;
     dbref  character;
     time_t now,last;

     setreturn(ERROR,COMMAND_FAIL);
     if((Blank(params) && (character = player)) || ((character = lookup_character(player,params,1)) != NOTHING)) {
        for(ptr = descriptor_list; ptr; ptr = ptr->next)
            if((ptr->player == character) && (ptr->flags & CONNECTED) && (!d || (ptr->last_time > d->last_time))) d = ptr;

        if(Connected(character) && d) {
           gettime(now);
           if((now -= (last = d->last_time)) < 0) now = 0;
           if(db[player].data->player.timediff) last += (db[player].data->player.timediff * HOUR);
           if(now >= MINUTE) {
              if(character != player) output(p,player,0,1,0,ANSI_LGREEN"\n%s"ANSI_LWHITE"%s"ANSI_LGREEN" has been idle since "ANSI_LWHITE"%s"ANSI_LGREEN" ("ANSI_LYELLOW"%s"ANSI_LGREEN".)",Article(character,UPPER,DEFINITE),getcname(NOTHING,character,0,0),date_to_string(last,UNSET_DATE,player,TIMEFMT),interval(now,now,ENTITIES,0));
                 else output(p,player,0,1,0,ANSI_LGREEN"\nYou have been idle since "ANSI_LWHITE"%s"ANSI_LGREEN" ("ANSI_LYELLOW"%s"ANSI_LGREEN".)",date_to_string(last,UNSET_DATE,player,TIMEFMT),interval(now,now,ENTITIES,0));
	   } else if(character != player) output(p,player,0,1,0,ANSI_LGREEN"\n%s"ANSI_LWHITE"%s"ANSI_LGREEN" is not currently idle.",Article(character,UPPER,DEFINITE),getcname(NOTHING,character,0,0));
              else output(p,player,0,1,0,ANSI_LGREEN"\nYou are not currently idle.");

           if(character != player) {
              if(d->afk_message) output(p,player,0,1,0,ANSI_LGREEN"%s is currently away from %s keyboard (AFK%s",Subjective(character,1),Possessive(character,0),!(d->flags2 & SENT_AUTO_AFK) ? ".)":") due to idling.");
              if(d->flags & DISCONNECTED) output(p,player,0,1,0,ANSI_LGREEN"%s has lost %s connection.",Subjective(character,1),Possessive(character,0));
	   }
           output(p,player,0,1,0,"");
           setreturn(OK,COMMAND_SUCC);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" isn't connected.",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0));
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",params);
}

/* ---->  Display your inventory  <---- */
void look_inventory(CONTEXT)
{
     struct descriptor_data *p = getdsc(player);

     if(!in_command && p && !p->pager && !IsHtml(p) && More(player)) pager_init(p);
     if(Valid(db[player].contents)) look_contents(player,player,"\nYou are carrying:");
        else output(p,player,0,1,0,ANSI_LGREEN"\nYou aren't carrying anything at the moment.\n");
     look_score(player,NULL,NULL,NULL,NULL,0,0);
     setreturn(OK,COMMAND_SUCC);
}

/* ---->            Look at room/object             <---- */
/*        (val1:  0 = No 'at', 1 = 'at' accepted.)        */
void look_at(CONTEXT)
{
     struct descriptor_data *p = getdsc(player);
     dbref  thing = NOTHING;

     setreturn(ERROR,COMMAND_FAIL);
     if(!in_command && p && !p->pager && !IsHtml(p) && More(player)) pager_init(p);
     if(!Blank(params)) {
        for(; *params && (*params == ' '); params++);
        if(val1 && (((strlen(params) == 2) && !strcasecmp("at",params)) || !strncasecmp(params,"at ",3))) {

           /* ---->  Look 'at' an object  <---- */
           for(params += 2; *params && (*params == ' '); params++);
           if(Blank(params)) {
              output(p,player,0,1,0,ANSI_LGREEN"Please specify what you'd like to look at.");
              command_type &= ~(NO_USAGE_UPDATE|NO_AUTO_FORMAT);
              return;
	   } else thing = match_preferred(player,player,params,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
 	} else if(val1 && !strcasecmp("outside",params)) {

           /* ---->  Look out of container/vehicle  <---- */
           if(Container(db[player].location) || (!Opaque(db[player].location) && (Typeof(db[player].location) != TYPE_ROOM))) {
              if(Vehicle(db[player].location) || Open(db[player].location) || !Opaque(db[player].location)) {
                 if(Valid(db[db[player].location].location)) thing = db[db[player].location].location;
    	            else output(p,player,0,1,0,ANSI_LGREEN"Sorry, your current location isn't within another object/location.  You can't look out of it.");
	      } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only look out of a container if its open.");
	   } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can't see anything outside this %s.",(Typeof(db[player].location) == TYPE_ROOM) ? "room":Vehicle(db[player].location) ? "vehicle":Container(db[player].location) ? "container":"object");
           if(!Valid(thing)) {
              command_type &= ~(NO_USAGE_UPDATE|NO_AUTO_FORMAT);
              return;
	   }

           /* ---->  Look at an object/current location  <---- */
	} else thing = match_preferred(player,player,params,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);

        if(Valid(thing)) {
            switch(Typeof(thing)) {
                   case TYPE_ROOM:
                        look_room(player,thing);
                        break;
                   case TYPE_EXIT:
                        if(getfield(thing,DESC) || !Valid((db[thing].destination == HOME) ? db[player].destination:db[thing].destination)) {
                           command_execute_action(player,thing,".look",NULL,getname(player),getnameid(player,thing,NULL),"",1);
                           termflags = TXT_BOLD;
                           look_simple(player,thing);
                           command_execute_action(player,thing,".looked",NULL,getname(player),getnameid(player,thing,NULL),"",1);
			} else look_room(player,(db[thing].destination == HOME) ? db[player].destination:db[thing].destination);
                        break;
                   case TYPE_CHARACTER:
                        command_execute_action(thing,NOTHING,".look",NULL,getname(player),"",getname(player),0);
                        if(!in_command) tilde_string(player,getcname(player,thing,1,UPPER|INDEFINITE),ANSI_LGREEN,ANSI_DGREEN,0,1,6);
                        look_simple(player,thing);
                        output(getdsc(player),player,0,1,0,ANSI_LGREEN"For more detailed information about %s"ANSI_LWHITE"%s"ANSI_LGREEN", type '"ANSI_LYELLOW"scan %s"ANSI_LGREEN"'.\n",Article(thing,LOWER,DEFINITE),getcname(NOTHING,thing,0,0),params);
                        look_contents(player,thing,"Carrying:");
                        command_execute_action(thing,NOTHING,".looked",NULL,getname(player),"",getname(player),0);
                        break;
                   case TYPE_ARRAY:
                        if(can_read_from(player,thing)) {
                           command_execute_action(player,thing,".look",NULL,getname(player),getnameid(player,thing,NULL),"",1);
                           if(!in_command) tilde_string(player,unparse_object(player,thing,0),ANSI_LGREEN,ANSI_DGREEN,0,1,6);
                           if(array_display_elements(player,elementfrom,elementto,thing,(in_command) ? 2:1) < 1)
                              output(getdsc(player),player,0,1,0,ANSI_LRED"Sorry, that element (Or range of elements) is invalid.\n");
                           command_execute_action(player,thing,".looked",NULL,getname(player),getnameid(player,thing,NULL),"",1);
			} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't look at that object.");
                        break;
                   case TYPE_PROPERTY:
                   case TYPE_VARIABLE:
                   case TYPE_COMMAND:
                   case TYPE_ALARM:
                   case TYPE_FUSE:
                        if(can_read_from(player,thing)) {
                           command_execute_action(player,thing,".look",NULL,getname(player),getnameid(player,thing,NULL),"",1);
                           if(!in_command) tilde_string(player,unparse_object(player,thing,0),ANSI_LGREEN,ANSI_DGREEN,0,1,6);
                           look_simple(player,thing);
                           command_execute_action(player,thing,".looked",NULL,getname(player),getnameid(player,thing,NULL),"",1);
			} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't look at that object.");
                        break;
                   default:
                        command_execute_action(player,thing,".look",NULL,getname(player),getnameid(player,thing,NULL),"",1);
                        if(!in_command) tilde_string(player,unparse_object(player,thing,UPPER|DEFINITE),ANSI_LGREEN,ANSI_DGREEN,0,1,6);
                        look_simple(player,thing);
                        command_execute_action(player,thing,".looked",NULL,getname(player),getnameid(player,thing,NULL),"",1);
                        break;
            }
            setreturn(OK,COMMAND_SUCC);
        }
     } else if(Valid(db[player].location)) look_room(player,db[player].location);
     termflags = 0;
}

/* ---->  Show built-in (Hard-coded) Message(s) Of The Day (MOTD)   <---- */
/*        (VAL1:  0 = Connect, 1 = Create, 2 = Display ('motd' command.)  */
void look_motd(CONTEXT)
{
     struct descriptor_data *p = getdsc(player);
     time_t now;

     /* ---->  Welcome user to TCZ  <---- */
     gettime(now);
     setreturn(OK,COMMAND_SUCC);

     sprintf(scratch_buffer,"\n%s (TCZ v"TCZ_VERSION")  -  (C) J.P.Boggis 1993 - %d.",tcz_full_name,tcz_year);
     tilde_string(player,scratch_buffer,"",ANSI_DCYAN,0,0,5);
     output(p,player,0,1,0,"%s",substitute(player,scratch_return_string,ANSI_LWHITE"TCZ is free software, which is distributed under %c%lversion 2%x of the %c%lGNU General Public License%x (See '%g%l%<gpl%>%x' in TCZ, or visit %b%l%u%{@?link \"\" \"http://www.gnu.org\" \"Visit the GNU web site...\"}%x)  For more information about the %y%lTCZ%x, please visit:  %b%l%u%{@?link \"\" \"https://github.com/smcvey/tcz\" \"Visit the TCZ project web site...\"}%x\n",0,ANSI_LWHITE,NULL,0));

#ifdef DEMO
     output(p,player,0,1,0,ANSI_LGREEN"\nWelcome to the demonstration version of "ANSI_LWHITE"%s"ANSI_LGREEN" (TCZ v"TCZ_VERSION"), %s"ANSI_LYELLOW"%s"ANSI_LGREEN"!\n",tcz_full_name,Article(player,LOWER,DEFINITE),getcname(NOTHING,player,0,0));
#else
     output(p,player,0,1,0,ANSI_LGREEN"\nWelcome to "ANSI_LWHITE"%s"ANSI_LGREEN" (TCZ v"TCZ_VERSION"), %s"ANSI_LYELLOW"%s"ANSI_LGREEN"!\n",tcz_full_name,Article(player,LOWER,DEFINITE),getcname(NOTHING,player,0,0));
#endif

     output(p,player,0,1,0,ANSI_LMAGENTA"Admin E-mail:  "ANSI_LYELLOW"\016<A HREF=\"mailto:%s\">\016%s\016</A>\016",tcz_admin_email,tcz_admin_email);
     output(p,player,0,1,0,ANSI_LMAGENTA"    Web Site:  "ANSI_LYELLOW"\016<A HREF=\"%s\" TARGET=_blank>\016%s\016</A>\016\n",html_home_url,html_home_url);

     if(val1 == 1) return;

     /* ---->  Display MOTD (Message Of The Day)  <---- */
     if(!Blank(option_motd(OPTSTATUS)))
        output(p,player,0,1,0,"%s",substitute(player,scratch_return_string,(char *) option_motd(OPTSTATUS),0,ANSI_LWHITE,NULL,0));

     /* ---->  Display summary of user's mail  <---- */
#ifdef BETA
     mail_update(player,"","");
#else
     output(p,player,0,1,0,"");
#endif

     /* ---->  Warn user to change password  <---- */
     /* if((db[player].data->player.pwexpiry + (PASSWORD_EXPIRY * DAY)) < now)
        output(p,player,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"You last changed your password "ANSI_LCYAN"%s"ANSI_LWHITE" ago on "ANSI_LYELLOW"%s"ANSI_LWHITE".\n\nYou should change your password regularly (At least once every "ANSI_LCYAN"2-3 months"ANSI_LWHITE"), otherwise you run the risk of somebody else gaining unauthorised access to your character, possibly causing malicious damage to yourself or your objects.  To change your password, simply type "ANSI_LGREEN""ANSI_UNDERLINE"@PASSWORD"ANSI_LWHITE" and follow the on-screen messages.  After you have done this, you will no-longer see this message.\n",
	interval(now - db[player].data->player.pwexpiry,0,3,0),date_to_string(db[player].data->player.pwexpiry,UNSET_DATE,player,FULLDATEFMT)); */

     /* ---->  Warn user of effective '@chuid'  <---- */
     if(Uid(player) != player)
        output(p,player,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"You are currently building under the ID of %s"ANSI_LYELLOW"%s"ANSI_LWHITE".  To build as yourself again, please type '"ANSI_LGREEN"@chuid"ANSI_LWHITE"'.\n",Article(Uid(player),LOWER,DEFINITE),getcname(player,Uid(player),1,0));

     /* ---->  Warn user of failed logins  <---- */
     if(db[player].data->player.failedlogins) {
        output(p,player,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"There %s been%s "ANSI_LYELLOW"%d"ANSI_LWHITE" failed login attempt%s as your character since you last successfully connected on "ANSI_LCYAN"%s"ANSI_LWHITE". \016&nbsp;\016 If you get this message on a regular basis, please change your password by typing '"ANSI_LGREEN"@password"ANSI_LWHITE"'.\n",
              (db[player].data->player.failedlogins == 1) ? "has":"have",(db[player].data->player.failedlogins < 255) ? "":" over",db[player].data->player.failedlogins,Plural(db[player].data->player.failedlogins),date_to_string(db[player].data->player.lasttime + (db[player].data->player.timediff * HOUR),UNSET_DATE,player,FULLDATEFMT));
        if(val1 != 2) db[player].data->player.failedlogins = 0;
     }

     /* ---->  Warn user of logging level privacy invasion  <---- */
     if(option_loglevel(OPTSTATUS) >= 3)
        output(p,player,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"The current server logging level may breach your privacy by logging the commands you type in full (Allowing administrators to see your private conversations.)  The reason for this is possible server debugging (Please ask a member of Admin.)\n");

     /* ---->  Warn user of enabled BETA features  <---- */
#ifdef BETA
     output(p,player,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LMAGENTA"This is a BETA release of %s.  "ANSI_LYELLOW"Unfinished BETA features are currently enabled, which may result in instability/crashes.\n",tcz_full_name);
#endif

     /* ---->  Warn user if host server is running on backup battery power  <---- */
#ifdef UPS_SUPPORT
     if(!powerstate) {
	if(Level4(player)) {
	   output(p,player,0,0,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LYELLOW"Power to the server on which %s runs was lost at "ANSI_LWHITE"%s"ANSI_LYELLOW" ("ANSI_LWHITE"%s"ANSI_LYELLOW" ago.)  This is the "ANSI_LWHITE"%s"ANSI_LYELLOW" power failure.  The server is currently running on "ANSI_UNDERLINE"battery backup"ANSI_LYELLOW", which may run out if mains power is not resumed again soon.\n\nIf the power fails, any changes made to the database may be lost.\n\nWhen mains power is resumed, you will receive notification of this.  If you suddenly get disconnected, the battery backup has probably run out of power (Try connecting again later when mains power has been resumed.)\n",tcz_full_name,date_to_string(powertime,UNSET_DATE,p->player,FULLDATEFMT),interval(now - powertime,0,ENTITIES,0),rank(powercount));
	} else {
	   output(p,player,0,0,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LYELLOW"Power to the server on which %s runs was lost at "ANSI_LWHITE"%s"ANSI_LYELLOW" ("ANSI_LWHITE"%s"ANSI_LYELLOW" ago.)  The server is currently running on "ANSI_UNDERLINE"battery backup"ANSI_LYELLOW", which may run out if mains power is not resumed again soon.\n\nIf the power fails, any building work or changes made to your character, objects, areas, etc. may be lost.\n\nWhen mains power is resumed, you will receive notification of this.  If you suddenly get disconnected, the battery backup has probably run out of power (Try connecting again later when mains power has been resumed.)\n",tcz_full_name,date_to_string(powertime,UNSET_DATE,p->player,FULLDATEFMT),interval(now - powertime,0,ENTITIES,0));
	}
     }
#endif
}

/* ---->  Error messages for non-supported/no-longer supported features  <---- */
void look_notice(CONTEXT)
{
     switch(val1 + (val2 * 100)) {
            case 1:
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"\nTo block pages and tells from ALL other characters, type '"ANSI_LYELLOW"@set me = haven"ANSI_LGREEN"'.  You can block individual characters by adding them to your friends list and resetting their PAGETELL friend flag (See '"ANSI_LWHITE"help friends"ANSI_LGREEN"' and '"ANSI_LWHITE"help fset"ANSI_LGREEN"' for details on how to do this.)  Typing '"ANSI_LYELLOW"@set me = !haven"ANSI_LGREEN"' will remove your page/tell block.\n");
                 break;
            case 2:
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"\nTo set your terminal type, please type '"ANSI_LWHITE"set term <TERMINAL TYPE>"ANSI_LGREEN"', e.g:  '"ANSI_LYELLOW"set term vt100"ANSI_LGREEN"'.\n");
                 break;
            case 3:
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"\nIf you'd like to be informed when characters connect/disconnect, type '"ANSI_LYELLOW"@set me = listen"ANSI_LGREEN"', otherwise type '"ANSI_LYELLOW"@set me = !listen"ANSI_LGREEN"'.\n");
                 break;
            case 4:
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, to leave "ANSI_LWHITE"%s"ANSI_LGREEN", please type the word "ANSI_LYELLOW"QUIT"ANSI_LGREEN" in capital letters.",tcz_full_name);
                 break;
            case 6:
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't shout a message using '"ANSI_LWHITE""SHOUT_TOKEN""ANSI_LGREEN"'  -  Please type the word '"ANSI_LWHITE"shout"ANSI_LGREEN"' in full.");
                 break;
            case 8:
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please type '"ANSI_LWHITE"@email public <EMAIL ADDRESS>"ANSI_LGREEN"' to set your public E-mail address.");
                 break;
            case 9:
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please type '"ANSI_LWHITE"@email private <EMAIL ADDRESS>"ANSI_LGREEN"' to set your private E-mail address.");
                 break;
            case 10:
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Which '"ANSI_LWHITE"@"ANSI_LGREEN"' command would you like to execute (Type '"ANSI_LYELLOW""ANSI_UNDERLINE"help commands"ANSI_LGREEN"' for a list.)");
		 break;
            case 11:
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Which '"ANSI_LWHITE"@?"ANSI_LGREEN"' query command would you like to execute (Type '"ANSI_LYELLOW""ANSI_UNDERLINE"help query commands"ANSI_LGREEN"' for a list.)");
		 break;
	    case 900:
                 help_main(player,"general",NULL,NULL,NULL,0,0);
                 break;
            case 901:
                 help_main(player,"feelings",NULL,NULL,NULL,0,0);
                 break;
            case 902:
                 help_main(player,"rules",NULL,NULL,NULL,0,0);
                 break;
            default:
                 break;
     }
     setreturn(ERROR,COMMAND_FAIL);
}

/* ---->  List privileges user has in current room  <---- */
void look_privileges(CONTEXT)
{
     struct descriptor_data *p = getdsc(player);
     dbref  who,area;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(params)) {
        if((who = lookup_character(player,params,1)) == NOTHING) {
           output(p,player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",params);
           return;
	}

        if(!Level4(db[player].owner) && !can_write_to(player,who,0)) {
           output(p,player,0,1,0,ANSI_LGREEN"You can only see your own privileges or those of one of your puppets.");
           return;
	}
     } else who = player;

     area = get_areaname_loc(db[who].location);
     sprintf(scratch_return_string,"%s"ANSI_LWHITE"%s"ANSI_LCYAN,Article(db[who].location,LOWER,DEFINITE),unparse_object(player,db[who].location,0));
     if(Valid(area) && !Blank(getfield(area,AREANAME)))
        sprintf(scratch_return_string + strlen(scratch_return_string)," in "ANSI_LWHITE"%s"ANSI_LCYAN,getfield(area,AREANAME));

     if(player == who) sprintf(scratch_buffer,"Your privileges in %s are...",scratch_return_string);
        else sprintf(scratch_buffer,"%s"ANSI_LWHITE"%s"ANSI_LCYAN"'s privileges in %s are...",Article(who,UPPER,DEFINITE),getcname(NOTHING,who,0,0),scratch_return_string);
     tilde_string(player,scratch_buffer,ANSI_LCYAN,ANSI_DCYAN,0,1,5);
     if(player == who) strcpy(scratch_return_string,ANSI_LGREEN"You");
        else sprintf(scratch_return_string,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN,Article(who,UPPER,DEFINITE),getcname(NOTHING,who,0,0));

     output(p,player,0,1,0,"%s may%s talk.",scratch_return_string,(Quiet(db[who].location)) ? " not":"");
     output(p,player,0,1,0,"%s may%s yell.",scratch_return_string,(!Quiet(db[who].location) && Yell(who) && (Yell(db[who].location) || (db[who].owner == db[db[who].location].owner) || (db[who].owner == Controller(db[db[who].location].owner)) || Level4(db[who].owner))) ? "":" not");

#ifndef MORTAL_SHOUT
     if(Level4(db[who].owner))
#endif
        output(p,player,0,1,0,"%s may%s shout.",scratch_return_string,Shout(who) ? "":" not");

     if(Level4(db[who].owner))
        output(p,player,0,1,0,"%s may%s boot users.",scratch_return_string,Boot(who) ? "":" not");

     output(p,player,0,1,0,"%s may%s fight in this %s.",scratch_return_string,(!Haven(db[who].location)) ? "":" not",(Typeof(db[who].location) == TYPE_ROOM) ? "room":"container");
     output(p,player,0,1,0,"%s may%s use bad language.\n",scratch_return_string,(Censor(who) || Censor(db[who].location)) ? " not":"");
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  View character's profile  <---- */
void look_profile(CONTEXT)
{
     unsigned char            twidth = output_terminal_width(player),secret = 0,email = 0;
     struct   descriptor_data *p = getdsc(player);
     const    char            *colour,*ptr;
     unsigned long            longdate;
     int                      copied;
     char                     *tmp;
     dbref                    who;
     time_t                   now;

     setreturn(ERROR,COMMAND_FAIL);
     if(player == NOBODY) {
        for(p = descriptor_list; p && !(IsHtml(p) && (p->player == NOBODY)); p = p->next);
        player = NOTHING;
        if(!p) return;
     }

     if(!Blank(arg1)) {
        if((who = lookup_character(player,arg1,1)) == NOTHING) {
           output(p,player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
           return;
	}
     } else if(player == NOTHING) output(p,player,0,1,0,ANSI_LGREEN"Sorry, that character doesn't exist.");
        else who = player;

     if(!Blank(arg2)) {
        if(!(!strcasecmp("secret",arg2) || !strcasecmp("noprofile",arg2) || !strcasecmp("noprofiles",arg2))) {
           output(p,player,0,1,0,ANSI_LGREEN"Please use '"ANSI_LWHITE"profile <NAME> = secret"ANSI_LGREEN"' to view a user's profile without executing their '"ANSI_LYELLOW".profile"ANSI_LGREEN"' compound command.");
           return;
	} else secret = 1;
     }

     if(hasprofile(db[who].data->player.profile)) {
        gettime(now);
        html_anti_reverse(p,1);
        command_type |= NO_USAGE_UPDATE;
        longdate = epoch_to_longdate(now);
        if(!in_command && p && !p->pager && !IsHtml(p) && More(player)) pager_init(p);
        if(IsHtml(p)) output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_GREY">",(in_command) ? "":"<BR>");

        /* ---->  Character name and title  <---- */
        if(!IsHtml(p)) output(p,player,0,1,0,"\n%s",(char *) separator(twidth,0,'-','='));
        sprintf(scratch_buffer,"%s%s%s%s%s%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_BLUE"><TD ALIGN=CENTER><TABLE BORDER=4 CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK"><TR ALIGN=CENTER><TH>\016":" ",Being(who) ? ANSI_DCYAN"(Being)":(Controller(who) != who) ? ANSI_DCYAN"(Puppet)":"",(Being(who) || (Controller(who) != who)) ? IsHtml(p) ? " \016&nbsp;\016 ":"  ":"",colour = privilege_colour(who),IsHtml(p) ? "\016<FONT SIZE=5><B><I>\016":"",getcname(NOTHING,who,0,UPPER|INDEFINITE));
        if((db[who].data->player.profile->dob & 0xFFFF) != (longdate & 0xFFFF)) {
           struct descriptor_data *t = getdsc(who);
           if(!(t && (t->flags & DISCONNECTED))) {
              if(!(t && t->afk_message)) {
                 if(!Moron(who)) {
                    if(!db[who].data->player.bantime) {
                       tmp = (char *) getfield(who,TITLE);
                       if(!Blank(tmp)) {
                          bad_language_filter(scratch_return_string,tmp);
                          tmp = scratch_return_string;
                          strcat(scratch_buffer,pose_string(&tmp,"*"));
                          substitute(who,scratch_buffer + strlen(scratch_buffer),tmp,0,colour,NULL,0);
		       }
		    } else sprintf(scratch_buffer + strlen(scratch_buffer),BANNED_TITLE,tcz_short_name);
		 } else strcat(scratch_buffer,"!");
	      } else strcat(scratch_buffer,(t && (t->flags2 & SENT_AUTO_AFK)) ? AUTO_AFK_TITLE:AFK_TITLE);
	   } else sprintf(scratch_buffer + strlen(scratch_buffer),LOST_TITLE,Possessive(who,0));
	} else sprintf(scratch_buffer + strlen(scratch_buffer),BIRTHDAY_TITLE,longdate_difference(db[who].data->player.profile->dob,longdate) / 12);
        output(p,player,2,1,1,IsHtml(p) ? "%s%s":" %s%s",punctuate(scratch_buffer,0,'.'),IsHtml(p) ? "\016</I></B></FONT></TH></TR></TABLE></TD></TR>\016":"\n");
        if(!IsHtml(p)) output(p,player,0,1,0,(char *) separator(twidth,0,'-','-'));

        /* ---->  Picture (If URL set)  <---- */
        if(IsHtml(p) && db[who].data->player.profile->picture)
           output(p,player,1,2,0,"<TR BGCOLOR="HTML_TABLE_DGREY"><TD ALIGN=CENTER VALIGN=CENTER><BR><TABLE BORDER=7 BGCOLOR="HTML_TABLE_BLACK"><TR><TD ALIGN=CENTER VALIGN=CENTER><IMG SRC=\"%s\" ALT=\"%s User Picture\"></TD></TR></TABLE></BR></TD></TR>",html_encode_basic(decompress(db[who].data->player.profile->picture),scratch_return_string,&copied,512),tcz_short_name);

        /* ---->  Real name, town/city, country, nationality and occupation  <---- */
        if(db[who].data->player.profile->irl || db[who].data->player.profile->city || db[who].data->player.profile->country || db[who].data->player.profile->nationality || db[who].data->player.profile->occupation) {
           if(IsHtml(p)) output(p,player,1,2,0,"<TR><TD ALIGN=LEFT CELLPADDING=0><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">");
           if(db[who].data->player.profile->irl)         output(p,player,2,1,20,"%sReal name:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN">"ANSI_LCYAN"<B><I>\016":ANSI_LCYAN"        ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",substitute(who,scratch_return_string,(char *) punctuate((char *) bad_language_filter(scratch_return_string,decompress(db[who].data->player.profile->irl)),2,'.'),0,ANSI_LWHITE,NULL,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           if(db[who].data->player.profile->city)        output(p,player,2,1,20,"%sTown/city:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN">"ANSI_LCYAN"<B><I>\016":ANSI_LCYAN"        ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",substitute(who,scratch_return_string,(char *) punctuate((char *) bad_language_filter(scratch_return_string,decompress(db[who].data->player.profile->city)),2,'.'),0,ANSI_LWHITE,NULL,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           if(db[who].data->player.profile->country)     output(p,player,2,1,20,"%sCountry:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN">"ANSI_LCYAN"<B><I>\016":ANSI_LCYAN"          ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",substitute(who,scratch_return_string,(char *) punctuate((char *) bad_language_filter(scratch_return_string,decompress(db[who].data->player.profile->country)),2,'.'),0,ANSI_LWHITE,NULL,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           if(db[who].data->player.profile->nationality) output(p,player,2,1,20,"%sNationality:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN">"ANSI_LCYAN"<B><I>\016":ANSI_LCYAN"      ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",substitute(who,scratch_return_string,(char *) punctuate((char *) bad_language_filter(scratch_return_string,decompress(db[who].data->player.profile->nationality)),2,'.'),0,ANSI_LWHITE,NULL,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           if(db[who].data->player.profile->occupation)  output(p,player,2,1,20,"%sOccupation:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN">"ANSI_LCYAN"<B><I>\016":ANSI_LCYAN"       ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",substitute(who,scratch_return_string,(char *) punctuate((char *) bad_language_filter(scratch_return_string,decompress(db[who].data->player.profile->occupation)),2,'.'),0,ANSI_LWHITE,NULL,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           if(IsHtml(p)) output(p,player,1,2,0,"</TABLE></TD></TR>");
              else output(p,player,0,1,0,(char *) separator(twidth,0,'-','-'));
	}

        /* ---->  Birthday, age, sex, sexuality and status (IVL/IRL)  <---- */
        if(IsHtml(p)) output(p,player,1,2,0,"<TR BGCOLOR="HTML_TABLE_DGREY"><TD ALIGN=LEFT CELLPADDING=0><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">");
        if(db[who].data->player.profile->dob != UNSET_DATE) {
           output(p,player,2,1,20,"%sBirthday:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED">"ANSI_LRED"<B><I>\016":ANSI_LRED"         ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",date_to_string(UNSET_DATE,db[who].data->player.profile->dob,player,SHORTDATEFMT),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           output(p,player,2,1,20,"%sAge:%s"ANSI_LWHITE"%d.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED">"ANSI_LRED"<B><I>\016":ANSI_LRED"              ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",longdate_difference(db[who].data->player.profile->dob,epoch_to_longdate(now)) / 12,IsHtml(p) ? "\016</TD></TR>\016":"\n");
	}

        output(p,player,2,1,20,"%sGender:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED">"ANSI_LRED"<B><I>\016":ANSI_LRED"           ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",genders[Genderof(who)],IsHtml(p) ? "\016</TD></TR>\016":"\n");

        if(db[who].data->player.profile->sexuality)
           output(p,player,2,1,20,"%sSexuality:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED">"ANSI_LRED"<B><I>\016":ANSI_LRED"        ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",sexuality[db[who].data->player.profile->sexuality],IsHtml(p) ? "\016</TD></TR>\016":"\n");

        if(!((Engaged(who) || Married(who)) && Validchar(Partner(who)))) {
           if(db[who].data->player.profile->statusivl)
              output(p,player,2,1,20,"%sStatus (IVL):%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED">"ANSI_LRED"<B><I>\016":ANSI_LRED"     ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",statuses[db[who].data->player.profile->statusivl],IsHtml(p) ? "\016</TD></TR>\016":"\n");
	} else output(p,player,2,1,20,"%sStatus (IVL):%s"ANSI_LWHITE"%s to %s"ANSI_LYELLOW"%s"ANSI_LWHITE".%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED">"ANSI_LRED"<B><I>\016":ANSI_LRED"     ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",Engaged(who) ? "Engaged":"Married",Article(Partner(who),LOWER,INDEFINITE),getcname(NOTHING,Partner(who),0,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");

        if(db[who].data->player.profile->statusirl)
           output(p,player,2,1,20,"%sStatus (IRL):%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED">"ANSI_LRED"<B><I>\016":ANSI_LRED"     ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",statuses[db[who].data->player.profile->statusirl],IsHtml(p) ? "\016</TD></TR>\016":"\n");

        if(IsHtml(p)) output(p,player,1,2,0,"</TABLE></TD></TR>");
           else output(p,player,0,1,0,(char *) separator(twidth,0,'-','-'));

        /* ---->  Height, weight, hair colour, eye colour, other (Miscellanous) information  <---- */
        if(db[who].data->player.profile->height || db[who].data->player.profile->weight || db[who].data->player.profile->hair || db[who].data->player.profile->eyes || db[who].data->player.profile->other) {
           int           major,minor;
           unsigned char metric;

           if(IsHtml(p)) output(p,player,1,2,0,"<TR><TD ALIGN=LEFT CELLPADDING=0><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">");

           if(db[who].data->player.profile->height) {
              metric = ((db[who].data->player.profile->height & 0x8000) != 0);
              major  = ((db[who].data->player.profile->height & 0x7FFF) >> 7);
              minor  = (db[who].data->player.profile->height & 0x007F);
              if(major > 0) sprintf(scratch_return_string,"%d%s",major,(metric) ? "m":"'");
                 else *scratch_return_string = '\0';
              if(minor > 0) sprintf(scratch_return_string + strlen(scratch_return_string),"%s%d%s",(*scratch_return_string) ? " ":"",minor,(metric) ? "cm":"\"");
              output(p,player,2,1,20,"%sHeight:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW">"ANSI_LYELLOW"<B><I>\016":ANSI_LYELLOW"           ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",scratch_return_string,IsHtml(p) ? "\016</TD></TR>\016":"\n");
	   }

           if(db[who].data->player.profile->weight) {
              metric = ((db[who].data->player.profile->weight & 0x80000000) != 0);
              major  = ((db[who].data->player.profile->weight & 0x7FFF0000) >> 16);
              minor  = (db[who].data->player.profile->weight  & 0x0000FFFF);
              if(major > 0) sprintf(scratch_return_string,"%d%s",major,(metric) ? "Kg":"lbs");
                 else *scratch_return_string = '\0';
              if(minor > 0) sprintf(scratch_return_string + strlen(scratch_return_string),"%s%d%s",(*scratch_return_string) ? " ":"",minor,(metric) ? "g":"oz");
              output(p,player,2,1,20,"%sWeight:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW">"ANSI_LYELLOW"<B><I>\016":ANSI_LYELLOW"           ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",scratch_return_string,IsHtml(p) ? "\016</TD></TR>\016":"\n");
	   }

           if(db[who].data->player.profile->hair)   output(p,player,2,1,20,"%sHair colour:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW">"ANSI_LYELLOW"<B><I>\016":ANSI_LYELLOW"      ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",substitute(who,scratch_return_string,(char *) punctuate((char *) bad_language_filter(scratch_return_string,decompress(db[who].data->player.profile->hair)),2,'.'),0,ANSI_LWHITE,NULL,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           if(db[who].data->player.profile->eyes)   output(p,player,2,1,20,"%sEye colour:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW">"ANSI_LYELLOW"<B><I>\016":ANSI_LYELLOW"       ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",substitute(who,scratch_return_string,(char *) punctuate((char *) bad_language_filter(scratch_return_string,decompress(db[who].data->player.profile->eyes)),2,'.'),0,ANSI_LWHITE,NULL,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           if(db[who].data->player.profile->other)  output(p,player,2,1,20,"%sOther:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_YELLOW">"ANSI_LYELLOW"<B><I>\016":ANSI_LYELLOW"            ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",substitute(who,scratch_return_string,(char *) punctuate((char *) bad_language_filter(scratch_return_string,decompress(db[who].data->player.profile->other)),2,'.'),0,ANSI_LWHITE,NULL,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           if(IsHtml(p)) output(p,player,1,2,0,"</TABLE></TD></TR>");
              else output(p,player,0,1,0,(char *) separator(twidth,0,'-','-'));
	}

        /* ---->  Hobbies, interests, achievements, qualifications, comments, likes, dislikes  <---- */
        if(db[who].data->player.profile->hobbies || db[who].data->player.profile->interests || db[who].data->player.profile->achievements || db[who].data->player.profile->qualifications || db[who].data->player.profile->comments) {
           if(IsHtml(p)) output(p,player,1,2,0,"<TR BGCOLOR="HTML_TABLE_DGREY"><TD ALIGN=LEFT CELLPADDING=0><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">");
           if(db[who].data->player.profile->hobbies)        output(p,player,2,1,20,"%sHobbies:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_MAGENTA">"ANSI_LMAGENTA"<B><I>\016":ANSI_LMAGENTA"          ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",substitute(who,scratch_return_string,(char *) punctuate((char *) bad_language_filter(scratch_return_string,decompress(db[who].data->player.profile->hobbies)),2,'.'),0,ANSI_LWHITE,NULL,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           if(db[who].data->player.profile->interests)      output(p,player,2,1,20,"%sInterests:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_MAGENTA">"ANSI_LMAGENTA"<B><I>\016":ANSI_LMAGENTA"        ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",substitute(who,scratch_return_string,(char *) punctuate((char *) bad_language_filter(scratch_return_string,decompress(db[who].data->player.profile->interests)),2,'.'),0,ANSI_LWHITE,NULL,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           if(db[who].data->player.profile->achievements)   output(p,player,2,1,20,"%sAchievements:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_MAGENTA">"ANSI_LMAGENTA"<B><I>\016":ANSI_LMAGENTA"     ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",substitute(who,scratch_return_string,(char *) punctuate((char *) bad_language_filter(scratch_return_string,decompress(db[who].data->player.profile->achievements)),2,'.'),0,ANSI_LWHITE,NULL,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           if(db[who].data->player.profile->qualifications) output(p,player,2,1,20,"%sQualifications:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_MAGENTA">"ANSI_LMAGENTA"<B><I>\016":ANSI_LMAGENTA"   ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",substitute(who,scratch_return_string,(char *) punctuate((char *) bad_language_filter(scratch_return_string,decompress(db[who].data->player.profile->qualifications)),2,'.'),0,ANSI_LWHITE,NULL,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           if(db[who].data->player.profile->comments)       output(p,player,2,1,20,"%sComments:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_MAGENTA">"ANSI_LMAGENTA"<B><I>\016":ANSI_LMAGENTA"         ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",substitute(who,scratch_return_string,(char *) punctuate((char *) bad_language_filter(scratch_return_string,decompress(db[who].data->player.profile->comments)),2,'.'),0,ANSI_LWHITE,NULL,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           if(IsHtml(p)) output(p,player,1,2,0,"</TABLE></TD></TR>");
              else output(p,player,0,1,0,(char *) separator(twidth,0,'-','-'));
	}

        /* ---->  Favourite foods, drinks, music and sports  <---- */
        if(db[who].data->player.profile->food || db[who].data->player.profile->drink || db[who].data->player.profile->music || db[who].data->player.profile->sport  || db[who].data->player.profile->likes || db[who].data->player.profile->dislikes) {
           if(IsHtml(p)) output(p,player,1,2,0,"<TR><TD ALIGN=LEFT CELLPADDING=0><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">");
           if(db[who].data->player.profile->food)     output(p,player,2,1,20,"%sFavourite foods:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_BLUE">"ANSI_LBLUE"<B><I>\016":ANSI_LBLUE"  ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",substitute(who,scratch_return_string,(char *) punctuate((char *) bad_language_filter(scratch_return_string,decompress(db[who].data->player.profile->food)),2,'.'),0,ANSI_LWHITE,NULL,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           if(db[who].data->player.profile->drink)    output(p,player,2,1,20,"%sFavourite drinks:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_BLUE">"ANSI_LBLUE"<B><I>\016":ANSI_LBLUE" ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",substitute(who,scratch_return_string,(char *) punctuate((char *) bad_language_filter(scratch_return_string,decompress(db[who].data->player.profile->drink)),2,'.'),0,ANSI_LWHITE,NULL,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           if(db[who].data->player.profile->music)    output(p,player,2,1,20,"%sFavourite music:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_BLUE">"ANSI_LBLUE"<B><I>\016":ANSI_LBLUE"  ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",substitute(who,scratch_return_string,(char *) punctuate((char *) bad_language_filter(scratch_return_string,decompress(db[who].data->player.profile->music)),2,'.'),0,ANSI_LWHITE,NULL,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           if(db[who].data->player.profile->sport)    output(p,player,2,1,20,"%sFavourite sports:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_BLUE">"ANSI_LBLUE"<B><I>\016":ANSI_LBLUE" ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",substitute(who,scratch_return_string,(char *) punctuate((char *) bad_language_filter(scratch_return_string,decompress(db[who].data->player.profile->sport)),2,'.'),0,ANSI_LWHITE,NULL,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           if(db[who].data->player.profile->likes)    output(p,player,2,1,20,"%sLikes:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_BLUE">"ANSI_LBLUE"<B><I>\016":ANSI_LBLUE"            ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",substitute(who,scratch_return_string,(char *) punctuate((char *) bad_language_filter(scratch_return_string,decompress(db[who].data->player.profile->likes)),2,'.'),0,ANSI_LWHITE,NULL,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           if(db[who].data->player.profile->dislikes) output(p,player,2,1,20,"%sDislikes:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_BLUE">"ANSI_LBLUE"<B><I>\016":ANSI_LBLUE"         ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",substitute(who,scratch_return_string,(char *) punctuate((char *) bad_language_filter(scratch_return_string,decompress(db[who].data->player.profile->dislikes)),2,'.'),0,ANSI_LWHITE,NULL,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           if(IsHtml(p)) output(p,player,1,2,0,"</TABLE></TD></TR>");
              else output(p,player,0,1,0,(char *) separator(twidth,0,'-','-'));
	}

        /* ---->  Web site, picture (URL) and E-mail address (If public)  <---- */
        if((ptr = getfield(who,EMAIL))) {
           const char *emailaddr;
           int   counter = 1;

           for(; counter <= EMAIL_ADDRESSES; counter++)
               if(((counter != 2) || (Validchar(player) && (Level4(db[player].owner) || can_write_to(player,who,1)))) && (emailaddr = gettextfield(counter,'\n',ptr,0,scratch_return_string)) && *emailaddr && strcasecmp("forward",emailaddr))
                  email = 1;
           if(!email && (emailaddr = forwarding_address(who,1,scratch_return_string))) email = 1;
	}

        if(db[who].data->player.profile->picture || getfield(who,WWW) || email) {
           if(IsHtml(p)) output(p,player,1,2,0,"<TR BGCOLOR="HTML_TABLE_DGREY"><TD ALIGN=LEFT CELLPADDING=0><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">");
           if((ptr = getfield(who,WWW)))
              output(p,player,2,1,22,"%sWeb site:%s"ANSI_LWHITE"\016<A HREF=\"%s%s\" TARGET=_blank>\016%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN">"ANSI_LGREEN"<B><I>\016":ANSI_LGREEN"           ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",!strncasecmp(ptr,"http://",7) ? "":"http://",html_encode_basic(IsHtml(p) ? ptr:"",scratch_return_string,&copied,512),ptr,IsHtml(p) ? "\016</A></TD></TR>\016":"\n");

           if(db[who].data->player.profile->picture) {
              ptr = decompress(db[who].data->player.profile->picture);
              output(p,player,2,1,22,"%sPicture (URL):%s"ANSI_LWHITE"\016<A HREF=\"%s\" TARGET=_blank>\016%s\016</A>\016%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN">"ANSI_LGREEN"<B><I>\016":ANSI_LGREEN"      ",IsHtml(p) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",html_encode_basic(IsHtml(p) ? ptr:"",scratch_return_string,&copied,512),ptr,IsHtml(p) ? "\016</TD></TR>\016":"\n");
	   }

           if(getfield(who,EMAIL)) {
              const    char *email = getfield(who,EMAIL);
              int           counter = 1;
              const    char *emailaddr;
              unsigned char header = 0;

              sprintf(scratch_buffer,"%sE-mail address(es):%s",IsHtml(p) ? "<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN"><FONT COLOR="HTML_LGREEN"><B><I>":ANSI_LGREEN" ",IsHtml(p) ? "</I></B></FONT></TH><TD ALIGN=LEFT>":"  ");
              if((emailaddr = forwarding_address(who,1,scratch_return_string))) {
                 if(!header && IsHtml(p)) output(p,player,1,2,0,scratch_buffer);
                 output(p,player,2,1,22,"%s"ANSI_LWHITE"\016<A HREF=\"mailto:%s\">\016%s\016</A> &nbsp; &nbsp;<I>"ANSI_LGREEN"(Public)"ANSI_LWHITE"</I>\016%s",(!header && !IsHtml(p)) ? scratch_buffer:"",html_encode(IsHtml(p) ? emailaddr:"",scratch_buffer + 2048,&copied,256),emailaddr,IsHtml(p) ? "":"\n");
                 header = 1;
	      }
              for(; counter <= EMAIL_ADDRESSES; counter++)
                  if(((counter != 2) || (Validchar(player) && (Level4(db[player].owner) || can_write_to(player,who,1)))) && (emailaddr = gettextfield(counter,'\n',email,0,scratch_return_string)) && *emailaddr && strcasecmp("forward",emailaddr)) {
                     if(!header && IsHtml(p)) output(p,player,1,2,0,scratch_buffer);
                     output(p,player,2,1,22,"%s%s<A HREF=\"mailto:%s\">\016%s\016</A> &nbsp; &nbsp;<I>%s"ANSI_LWHITE"</I>\016%s",!IsHtml(p) ? (header) ? "                      ":scratch_buffer:"",(header && IsHtml(p)) ? "\016<BR>":ANSI_LWHITE"\016",html_encode(IsHtml(p) ? emailaddr:"",scratch_buffer + 2048,&copied,256),emailaddr,(counter == 2) ? ANSI_DRED"("ANSI_LRED"Private"ANSI_DRED")":ANSI_DGREEN"("ANSI_LGREEN"Public"ANSI_DGREEN")",IsHtml(p) ? "":"\n");
                     header = 1;
		  }
              if(header && IsHtml(p)) output(p,player,1,2,0,"</TD></TR>");
	   }
           if(IsHtml(p)) output(p,player,1,2,0,"</TABLE></TD></TR>");
              else output(p,player,0,1,0,(char *) separator(twidth,0,'-','-'));
	}

        if(Validchar(player)) output(p,player,2,1,1,"%sFor other information about %s"ANSI_LYELLOW"%s"ANSI_LWHITE", type '"ANSI_LGREEN"\016<A HREF=\"%sSUBST=OK&COMMAND=%%7Cscan%s%s&\" TARGET=TCZINPUT><B>\016scan%s%s\016</B></A>\016"ANSI_LWHITE"'.%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_GREY"><TD ALIGN=CENTER>"ANSI_LWHITE"<I>\016":ANSI_LWHITE" ",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0),html_server_url(p,1,2,"input"),Blank(arg1) ? "":"+",html_encode(arg1,scratch_return_string,&copied,128),Blank(arg1) ? "":" ",arg1,IsHtml(p) ? "\016</TD></TR>\016":"\n");
        if(IsHtml(p)) output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
           else output(p,player,0,1,0,(char *) separator(twidth,1,'-','='));
        command_type &= ~NO_USAGE_UPDATE;
        html_anti_reverse(p,0);

        if(!secret && Validchar(player))
           command_execute_action(who,NOTHING,".profile",NULL,getname(player),"",getname(player),0);
        setreturn(OK,COMMAND_SUCC);
     } else if(who == player) output(p,player,0,1,0,ANSI_LGREEN"Sorry, you haven't set any information on your profile yet (See '"ANSI_LWHITE"help @profile"ANSI_LGREEN"' for details on setting your profile.)");
        else output(p,player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" doesn't have a profile.",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0));
}

/* ---->  Display detailed information about specified character  <---- */
/*        (VAL1:  0 = Brief (Normal) scan, 1 = Full scan.)              */
/*        (       2 = Brief on Telnet, Full on HTML.)                   */
void look_scan(CONTEXT)
{
     unsigned char            twidth = output_terminal_width(player),font = 1,secret = 0;
     time_t                   now,last,total,idle,active;
     struct   descriptor_data *w,*d = getdsc(player);
     double                   credit,balance;
     dbref                    area,who;
     unsigned long            longdate;
     const    char            *colour;
     int                      copied;
     int                      items;
     char                     *ptr;

     setreturn(ERROR,COMMAND_FAIL);
     if(player == NOBODY) {
        for(d = descriptor_list; d && !(IsHtml(d) && (d->player == NOBODY)); d = d->next);
        player = NOTHING;
        if(!d) return;
     }

     /* ---->  Brief scan on Telnet, full scan on HTML?  <---- */
     if(val1 == 2) {
        if(d && d->html) {
           val1 = 1;
	} else val1 = 0;
     }

     if(!Blank(arg1)) {
        if((who = lookup_character(player,arg1,1)) == NOTHING) {
           output(d,player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
           return;
	}
     } else if(player == NOTHING) {
        output(d,player,0,1,0,ANSI_LGREEN"Sorry, that character doesn't exist.");
     } else who = player;

     if(!Blank(arg2)) {
        if(!(!strcasecmp("secret",arg2) || !strcasecmp("noscan",arg2) || !strcasecmp("noscans",arg2))) {
           output(d,player,0,1,0,ANSI_LGREEN"Please use '"ANSI_LWHITE"scan <NAME> = secret"ANSI_LGREEN"' to scan a user without executing their '"ANSI_LYELLOW".scan"ANSI_LGREEN"' compound command.");
           return;
	} else secret = 1;
     }

     gettime(now);
     html_anti_reverse(d,1);
     command_type |= NO_USAGE_UPDATE;
     longdate = epoch_to_longdate(now);
     if(!in_command && d && !d->pager && !IsHtml(d) && More(player)) pager_init(d);
     if(IsHtml(d)) output(d,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_GREY">",(in_command) ? "":"<BR>");
     w = getdsc(who);

     /* ---->  Character name and title  <---- */
     if(!IsHtml(d)) output(d,player,0,1,0,"\n%s",(char *) separator(twidth,0,'-','='));
     sprintf(scratch_buffer,"%s%s%s%s",IsHtml(d) ? "\016<TR BGCOLOR="HTML_TABLE_BLUE"><TD ALIGN=CENTER><TABLE BORDER=4 CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK"><TR ALIGN=CENTER><TH>\016":" ",colour = privilege_colour(who),IsHtml(d) ? "\016<FONT SIZE=5><B><I>\016":"",getcname(NOTHING,who,0,UPPER|INDEFINITE));
     if(!(hasprofile(db[who].data->player.profile) && ((db[who].data->player.profile->dob & 0xFFFF) == (longdate & 0xFFFF)))) {
        if(!(w && (w->flags & DISCONNECTED))) {
           if(!(w && w->afk_message)) {
              if(!Moron(who)) {
                 if(!db[who].data->player.bantime) {
                    ptr = (char *) getfield(who,TITLE);
                    if(!Blank(ptr)) {
                       bad_language_filter(scratch_return_string,ptr);
                       ptr = scratch_return_string;
                       strcat(scratch_buffer,pose_string(&ptr,"*"));
                       substitute(who,scratch_buffer + strlen(scratch_buffer),ptr,0,colour,NULL,0);
		    }
		 } else sprintf(scratch_buffer + strlen(scratch_buffer),BANNED_TITLE,tcz_short_name);
	      } else strcat(scratch_buffer,"!");
	   } else strcat(scratch_buffer,(w && (w->flags2 & SENT_AUTO_AFK)) ? AUTO_AFK_TITLE:AFK_TITLE);
	} else sprintf(scratch_buffer + strlen(scratch_buffer),LOST_TITLE,Possessive(who,0));
     } else sprintf(scratch_buffer + strlen(scratch_buffer),BIRTHDAY_TITLE,longdate_difference(db[who].data->player.profile->dob,longdate) / 12);
     output(d,player,2,1,1,IsHtml(d) ? "%s%s":" %s%s",punctuate(scratch_buffer,0,'.'),IsHtml(d) ? "\016</I></B></FONT></TH></TR></TABLE></TD></TR>\016":"\n");
     if(!IsHtml(d)) output(d,player,0,1,0,(char *) separator(twidth,0,'-','-'));

     /* ---->  Description  <---- */
     if(IsHtml(d)) output(d,player,1,2,0,"<TR BGCOLOR="HTML_TABLE_DGREY"><TD ALIGN=LEFT CELLPADDING=0><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK"><TR><TD ALIGN=LEFT><FONT SIZE=4>");
     if(!Blank(getfield(who,DESC))) sprintf(scratch_buffer,ANSI_LWHITE"%s",(char *) punctuate((char *) getfield(who,DESC),3,'.'));
        else strcpy(scratch_buffer,(player == who) ? ANSI_LGREEN" You haven't set your description yet.":ANSI_LGREEN" This character hasn't set their description yet.");
     substitute_large(player,Validchar(player) ? player:NOBODY,scratch_buffer,ANSI_LWHITE,scratch_return_string,1);
     if(!secret && Validchar(player))
        command_execute_action(who,NOTHING,".lookdesc",NULL,getname(player),"",getname(player),0);

     /* ---->  Current 'feeling'  <---- */
     if(db[who].data->player.feeling) {
        int loop;

        for(loop = 0; feelinglist[loop].name && (feelinglist[loop].id != db[who].data->player.feeling); loop++);
        if(feelinglist[loop].name) {
           strcpy(scratch_return_string,feelinglist[loop].name);
           *scratch_return_string = tolower(*scratch_return_string);
           output(d,player,2,1,1,"%s%s"ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" is feeling %s.%s",(IsHtml(d) && font) ? "\016</FONT>\016":"",IsHtml(d) ? "\016</TD></TR><TR BGCOLOR="HTML_TABLE_GREEN"><TD ALIGN=CENTER>\016":"\n ",Article(who,UPPER,DEFINITE),getcname(NOTHING,who,0,0),scratch_return_string,IsHtml(d) ? "":"\n"), font = 0;
	} else db[who].data->player.feeling = 0;
     }

     /* ---->  Puppet/Being?  <---- */
     if(Puppet(who)) {
        sprintf(scratch_buffer,"%s%s"ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" is a %spuppet of ",(IsHtml(d) && font) ? "\016</FONT>\016":"",IsHtml(d) ? "\016</TD></TR><TR BGCOLOR="HTML_TABLE_GREEN"><TD ALIGN=CENTER>\016":"\n ",Article(who,UPPER,DEFINITE),getcname(NOTHING,who,0,0),Being(who) ? "Being ":"");
        output(d,player,2,1,1,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN".%s",scratch_buffer,Article(Controller(who),LOWER,INDEFINITE),getcname(NOTHING,Controller(who),0,0),IsHtml(d) ? "":"\n"), font = 0;
     } else if(Being(who)) output(d,player,2,1,1,"%s%s"ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" is a Being.%s",(IsHtml(d) && font) ? "\016</FONT>\016":"",IsHtml(d) ? "\016</TD></TR><TR BGCOLOR="HTML_TABLE_GREEN"><TD ALIGN=CENTER>\016":"\n ",Article(who,UPPER,DEFINITE),getcname(NOTHING,who,0,0),IsHtml(d) ? "":"\n"), font = 0;

     if((Engaged(who) || Married(who)) && Validchar(Partner(who))) {
        sprintf(scratch_buffer,"%s%s"ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" is %s to ",(IsHtml(d) && font) ? "\016</FONT>\016":"",IsHtml(d) ? "\016</TD></TR><TR BGCOLOR="HTML_TABLE_GREEN"><TD ALIGN=CENTER>\016":"\n ",Article(who,UPPER,DEFINITE),getcname(NOTHING,who,0,0),Married(who) ? "married":"engaged");
        sprintf(scratch_buffer + strlen(scratch_buffer),"%s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(Partner(who),UPPER,DEFINITE),getcname(NOTHING,Partner(who),0,0));
        output(d,player,0,1,1,"%s",scratch_buffer), font = 0;
     }

     if(IsHtml(d)) output(d,player,1,2,0,"%s</TD></TR></TABLE></TD></TR>",(IsHtml(d) && font) ? "</FONT>":"");
        else output(d,player,0,1,0,(char *) separator(twidth,0,'-','-'));
     font = 0;

     if(IsHtml(d)) output(d,player,1,2,0,"<TR><TD ALIGN=LEFT CELLPADDING=0><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">");

     /* ---->  Longest connect time  <---- */
     items = 0;
     if(Connected(who)) {
        if((total = (now - db[who].data->player.lasttime)) == now) total = 0;
        if(db[who].data->player.longesttime < total)
           db[who].data->player.longesttime = total;
     }
     if(db[who].data->player.longesttime > 0) {
        output(d,player,2,1,26,"%sLongest connect time:%s"ANSI_LWHITE"%s.\n"ANSI_DCYAN"("ANSI_LYELLOW"%s"ANSI_DCYAN".)%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN">"ANSI_LCYAN"<B><I>\016":ANSI_LCYAN" ",IsHtml(d) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",interval(db[who].data->player.longesttime,db[who].data->player.longesttime,ENTITIES,0),date_to_string(TimeDiff(db[who].data->player.longestdate,player),UNSET_DATE,player,FULLDATEFMT),IsHtml(d) ? "\016</TD></TR>\016":"\n");
        items++;
     }

     /* ---->  Total time  <---- */
     total = db[who].data->player.totaltime;
     if(Connected(who)) total += (now - db[who].data->player.lasttime);
     if(total > 0) {
        strcpy(scratch_return_string,interval(total / ((db[who].data->player.logins > 1) ? db[who].data->player.logins:1),0,ENTITIES,0));
        output(d,player,2,1,26,"%sTotal time connected:%s"ANSI_LWHITE"%s.\n"ANSI_DCYAN"("ANSI_LCYAN"Average "ANSI_LYELLOW"%s"ANSI_DCYAN".)%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN">"ANSI_LCYAN"<B><I>\016":ANSI_LCYAN" ",IsHtml(d) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",interval(total,total,ENTITIES,0),scratch_return_string,IsHtml(d) ? "\016</TD></TR>\016":"\n");
        items++;
     }

     /* ---->  Time spent idle (Full scan only)  <---- */
     if(val1) {
        idle = db[who].data->player.idletime;
        if(Connected(who) && w) idle += (now - w->last_time);
	if(idle > 0) {
	   strcpy(scratch_return_string,interval(idle / ((db[who].data->player.logins > 1) ? db[who].data->player.logins:1),0,ENTITIES,0));
	   output(d,player,2,1,26,"%sTime spent idling:%s"ANSI_LWHITE"%s.\n"ANSI_DCYAN"("ANSI_LCYAN"Average "ANSI_LYELLOW"%s"ANSI_DCYAN".)%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_RED">"ANSI_LRED"<B><I>\016":ANSI_LRED"    ",IsHtml(d) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",interval(idle,idle,ENTITIES,0),scratch_return_string,IsHtml(d) ? "\016</TD></TR>\016":"\n");
	   items++;
	}
     }

     /* ---->  Time spent active (Full scan only)  <---- */
     if(val1) {
        active = total - idle;
	if(active > 0) {
	   strcpy(scratch_return_string,interval(active  / ((db[who].data->player.logins > 1) ? db[who].data->player.logins:1),0,ENTITIES,0));
	   output(d,player,2,1,26,"%sTime spent active:%s"ANSI_LWHITE"%s.\n"ANSI_DCYAN"("ANSI_LCYAN"Average "ANSI_LYELLOW"%s"ANSI_DCYAN".)%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_GREEN">"ANSI_LGREEN"<B><I>\016":ANSI_LGREEN"    ",IsHtml(d) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",interval(active,active,ENTITIES,0),scratch_return_string,IsHtml(d) ? "\016</TD></TR>\016":"\n");
	   items++;
	}
     }

     /* ---->  Section separator (Full scan only)  <---- */
     if(val1 && (items > 0)) {
        if(IsHtml(d)) output(d,player,1,2,0,"</TABLE></TD></TR><TR><TD ALIGN=LEFT CELLPADDING=0 BGCOLOR="HTML_TABLE_DGREY"><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">");
           else output(d,player,0,1,0,(char *) separator(twidth,0,'-','-'));
     }

     /* ---->  Last time connected  <---- */
     last = db[who].data->player.lasttime;
     if(last > 0) {
        total = now - last;
        if((last > 0) && Validchar(player)) last += (db[player].data->player.timediff * HOUR);
        sprintf(scratch_buffer,"%sLast time connected:%s"ANSI_LWHITE"%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN">"ANSI_LCYAN"<B><I>\016":ANSI_LCYAN"  ",IsHtml(d) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",(last == 0) ? "Unknown":date_to_string(last,UNSET_DATE,player,FULLDATEFMT));
        if(total > 0) sprintf(scratch_buffer + strlen(scratch_buffer),"%s\n"ANSI_DCYAN"("ANSI_LYELLOW"%s "ANSI_LCYAN"ago"ANSI_DCYAN".)%s",Connected(who) ? " \016&nbsp;\016"ANSI_DCYAN"("ANSI_LCYAN"Still connected."ANSI_DCYAN")":".",interval(total,total,ENTITIES,0),IsHtml(d) ? "\016</TD></TR>\016":"");
           else sprintf(scratch_buffer + strlen(scratch_buffer),"%s%s",Connected(who) ? " \016&nbsp;\016"ANSI_DCYAN"("ANSI_LCYAN"Still connected."ANSI_DCYAN")":".",IsHtml(d) ? "\016</TD></TR>\016":"");
        output(d,player,IsHtml(d) ? 2:0,1,26,"%s",scratch_buffer);
     }

     /* ---->  Site character last connected from  <---- */
     if(Validchar(player) && ((db[player].owner == who) || Level4(db[player].owner)) && !Blank(getfield(who,LASTSITE)))
        output(d,player,2,1,24,"%sLast connected from:%s"ANSI_LWHITE"%s%s%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN">"ANSI_LCYAN"<B><I>\016":ANSI_LCYAN"  ",IsHtml(d) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",getfield(who,LASTSITE),(player == who) ? " \016&nbsp; <I>"ANSI_DRED"("ANSI_LRED"Private"ANSI_DRED")</I>\016":"",IsHtml(d) ? "\016</TD></TR>\016":"\n");

     /* ---->  Total logins (Full scan only)  <---- */
     if(val1) {
        total = ((now - db[who].created) / ((db[who].data->player.logins > 1) ? db[who].data->player.logins:1));
        output(d,player,2,1,26,"%sTotal logins:%s"ANSI_LWHITE"%d \016&nbsp;\016"ANSI_DCYAN"("ANSI_LCYAN"Average "ANSI_LYELLOW"%s"ANSI_DCYAN".)%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_CYAN">"ANSI_LCYAN"<B><I>\016":ANSI_LCYAN"         ",IsHtml(d) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",db[who].data->player.logins,interval(ABS((total < MINUTE) ? MINUTE:total),0,ENTITIES,0),IsHtml(d) ? "\016</TD></TR>\016":"\n");
     }

     /* ---->  Date created (Full scan only)  <---- */
     if(val1) output(d,player,2,1,26,"%sCreation date:%s"ANSI_LWHITE"%s.\n"ANSI_DCYAN"("ANSI_LYELLOW"%s "ANSI_LCYAN"ago"ANSI_DCYAN".)%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=30% BGCOLOR="HTML_TABLE_MAGENTA">"ANSI_LMAGENTA"<B><I>\016":ANSI_LMAGENTA"        ",IsHtml(d) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",date_to_string(TimeDiff(db[who].created,player),UNSET_DATE,player,FULLDATEFMT),interval(ABS(now - db[who].created),0,ENTITIES,0),IsHtml(d) ? "\016</TD></TR>\016":"\n");

     /* ---->  Section separator  <---- */
     if(IsHtml(d)) output(d,player,1,2,0,"</TABLE></TD></TR><TR><TD ALIGN=LEFT CELLPADDING=0><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">");
        else output(d,player,0,1,0,(char *) separator(twidth,0,'-','-'));

     /* ---->  Race  <---- */
     if(!Blank(getfield(who,RACE))) {
        bad_language_filter(scratch_buffer,scratch_buffer);
        output(d,player,2,1,10,"%sRace:%s"ANSI_LWHITE"%s%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_RED">"ANSI_LRED"<B><I>\016":ANSI_LRED"   ",IsHtml(d) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",punctuate((char *) substitute(who,scratch_return_string,(char *) getfield(who,RACE),0,ANSI_LWHITE,NULL,0),2,'.'),IsHtml(d) ? "\016</TD></TR>\016":"\n");
     }

     /* ---->  Flags  <---- */
     output(d,player,2,1,10,"%sFlags:%s"ANSI_LWHITE"%s%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_GREEN">"ANSI_LGREEN"<B><I>\016":ANSI_LGREEN"  ",IsHtml(d) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",unparse_flaglist(who,((player > 0) && (Level4(db[player].owner) || can_read_from(player,who))),scratch_return_string),IsHtml(d) ? "\016</TD></TR>\016":"\n");

     /* ---->  Gender, health, score, credit, bank balance, Building Quota  <---- */
     update_health(who);
     switch(Genderof(who)) {
            case GENDER_MALE:
                 strcpy(scratch_return_string,"Male");
                 break;
            case GENDER_FEMALE:
                 strcpy(scratch_return_string,"Female");
                 break;
            case GENDER_NEUTER:
                 strcpy(scratch_return_string,"Neuter");
                 break;
            default:
                 strcpy(scratch_return_string,"Not set");
                 break;
     }

     credit  = currency_to_double(&(db[who].data->player.credit));
     balance = currency_to_double(&(db[who].data->player.balance));
     sprintf(scratch_buffer,"%sGender:%s"ANSI_LWHITE"%s"ANSI_LYELLOW", %sHealth: %s "ANSI_LWHITE"%s"ANSI_LYELLOW", %sScore: %s "ANSI_LWHITE"%d"ANSI_LYELLOW", %sCredit: %s "ANSI_LWHITE"%.2f"ANSI_LYELLOW", %sBank balance: %s %s%.2f"ANSI_LYELLOW", %sBuilding Quota: %s ",
            IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW">"ANSI_LYELLOW"<B><I>\016":ANSI_LYELLOW" ",IsHtml(d) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",scratch_return_string,
            IsHtml(d) ? "\016&nbsp;\016":"",IsHtml(d) ? "\016&nbsp;\016":"",combat_percent(currency_to_double(&(db[who].data->player.health)),100),IsHtml(d) ? "\016&nbsp;\016":"",
            IsHtml(d) ? "\016&nbsp;\016":"",db[who].data->player.score,
            IsHtml(d) ? "\016&nbsp;\016":"",IsHtml(d) ? "\016&nbsp;\016":"",credit,
            IsHtml(d) ? "\016&nbsp;\016":"",IsHtml(d) ? "\016&nbsp;\016":"",(balance < 0) ? ANSI_LRED:ANSI_LWHITE,balance,
            IsHtml(d) ? "\016&nbsp;\016":"",IsHtml(d) ? "\016&nbsp;\016":"");
     if(!Level4(who)) sprintf(scratch_buffer + strlen(scratch_buffer),"%s%d/%ld"ANSI_LYELLOW".",(db[who].data->player.quota > db[who].data->player.quotalimit) ? ANSI_LRED:ANSI_LWHITE,db[who].data->player.quota,db[who].data->player.quotalimit);
        else sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LWHITE"%d/UNLIMITED"ANSI_LYELLOW".",db[who].data->player.quota);
     output(d,player,2,1,10,"%s%s",scratch_buffer,IsHtml(d) ? "\016</TD></TR>\016":"\n");

     if(IsHtml(d)) output(d,player,1,2,0,"</TABLE></TD></TR><TR><TD ALIGN=LEFT CELLPADDING=0 BGCOLOR="HTML_TABLE_DGREY"><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">");
        else output(d,player,0,1,0,(char *) separator(twidth,0,'-','-'));

     /* ---->  Web Site  <---- */
     if((ptr = (char *) getfield(who,WWW)))
        output(d,player,2,1,22,"%sWeb site:%s"ANSI_LWHITE"\016<A HREF=\"%s%s\" TARGET=_blank>\016%s\016</A>\016%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_GREEN">"ANSI_LGREEN"<B><I>\016":ANSI_LGREEN"           ",IsHtml(d) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",!strncasecmp(ptr,"http://",7) ? "":"http://",html_encode_basic(IsHtml(d) ? ptr:"",scratch_return_string,&copied,512),ptr,IsHtml(d) ? "\016</TD></TR>\016":"\n");

     /* ---->  E-mail address  <---- */
     if(getfield(who,EMAIL)) {
        const    char *email = getfield(who,EMAIL);
        int           counter = 1;
        const    char *emailaddr;
        unsigned char header = 0;

        sprintf(scratch_buffer,"%sE-mail address(es):%s",IsHtml(d) ? "<TR><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_GREEN"><FONT COLOR="HTML_LGREEN"><B><I>":ANSI_LGREEN" ",IsHtml(d) ? "</I></B></FONT></TH><TD ALIGN=LEFT>":"  ");
        if((emailaddr = forwarding_address(who,1,scratch_return_string))) {
           if(!header && IsHtml(d)) output(d,player,1,2,0,scratch_buffer);
           output(d,player,2,1,22,"%s"ANSI_LWHITE"\016<A HREF=\"mailto:%s\">\016%s\016</A> &nbsp; &nbsp;<I>"ANSI_LGREEN"(Public)"ANSI_LWHITE"</I>\016%s",(!header && !IsHtml(d)) ? scratch_buffer:"",html_encode(IsHtml(d) ? emailaddr:"",scratch_buffer + 2048,&copied,256),emailaddr,IsHtml(d) ? "":"\n");
           header = 1;
	}

        for(; counter <= EMAIL_ADDRESSES; counter++)
            if(((counter != 2) || (Validchar(player) && (Level4(db[player].owner) || can_write_to(player,who,1)))) && (emailaddr = gettextfield(counter,'\n',email,0,scratch_return_string)) && *emailaddr && strcasecmp("forward",emailaddr)) {
               if(!header && IsHtml(d)) output(d,player,1,2,0,scratch_buffer);
               output(d,player,2,1,20,"%s%s<A HREF=\"mailto:%s\">\016%s\016</A> &nbsp; &nbsp;<I>%s"ANSI_LWHITE"</I>\016%s",!IsHtml(d) ? (header) ? "                      ":scratch_buffer:"",(header && IsHtml(d)) ? "\016<BR>":ANSI_LWHITE"\016",html_encode(IsHtml(d) ? emailaddr:"",scratch_buffer + 2048,&copied,256),emailaddr,(counter == 2) ? ANSI_DRED"("ANSI_LRED"Private"ANSI_DRED")":ANSI_DGREEN"("ANSI_LGREEN"Public"ANSI_DGREEN")",IsHtml(d) ? "":"\n");
               header = 1;
	    }
        if(header && IsHtml(d)) output(d,player,1,2,0,"</TD></TR>");
     }

     /* ---->  Current location  <---- */
     if(!((Secret(who) || Secret(db[who].location)) && !can_write_to(player,db[who].location,1) && !can_write_to(player,who,1))) {
        area = get_areaname_loc(db[who].location);
        if(Valid(area) && !Blank(getfield(area,AREANAME))) {
           strcpy(scratch_return_string,unparse_object(player,db[who].location,UPPER|INDEFINITE));
           sprintf(scratch_return_string + strlen(scratch_return_string),ANSI_LCYAN" in "ANSI_LWHITE"%s",getfield(area,AREANAME));
	} else strcpy(scratch_return_string,unparse_object(player,db[who].location,UPPER|INDEFINITE));
        bad_language_filter(scratch_return_string,scratch_return_string);
     } else strcpy(scratch_return_string,"Secret");
     output(d,player,2,1,22,"%sCurrent location:%s"ANSI_LWHITE"%s.%s",IsHtml(d) ? "\016<TR><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_CYAN">"ANSI_LCYAN"<B><I>\016":ANSI_LCYAN"   ",IsHtml(d) ? "\016</I></B></TH><TD ALIGN=LEFT>\016":"  ",scratch_return_string,IsHtml(d) ? "\016</TD></TR>\016":"\n");

     if(Validchar(player) && hasprofile(db[who].data->player.profile)) {
        if(IsHtml(d)) output(d,player,1,2,0,"</TABLE></TD>");
           else output(d,player,0,1,0,(char *) separator(twidth,0,'-','-'));
        output(d,player,2,1,1,"%s%s%s"ANSI_LWHITE" also has a profile about %s  -  Type '"ANSI_LGREEN"\016<A HREF=\"%sSUBST=OK&COMMAND=%%7Cprofile%s%s&\" TARGET=TCZINPUT><B>\016profile%s%s\016</B></A>\016"ANSI_LWHITE"' to view it.%s",IsHtml(d) ? "\016<TR BGCOLOR="HTML_TABLE_GREY"><TD ALIGN=CENTER>"ANSI_LYELLOW"<I>\016":ANSI_LYELLOW" ",Article(who,UPPER,DEFINITE),getcname(NOTHING,who,0,0),Reflexive(who,0),html_server_url(d,1,2,"input"),Blank(arg1) ? "":"+",html_encode(arg1,scratch_return_string,&copied,128),Blank(arg1) ? "":" ",arg1,IsHtml(d) ? "\016</TD></TR>\016":"\n");
     } else if(IsHtml(d)) output(d,player,1,2,0,"</TABLE></TD>");

     if(IsHtml(d)) output(d,player,1,2,0,"</TR></TABLE>%s",(!in_command) ? "<BR>":"");
        else output(d,player,0,1,0,(char *) separator(twidth,1,'-','='));
     command_type &= ~NO_USAGE_UPDATE;
     html_anti_reverse(d,0);

     if(!secret && Validchar(player))
        command_execute_action(who,NOTHING,".scan",NULL,getname(player),"",getname(player),0);
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Display your building points and score   <---- */
/*        (val1:  0 = No CR, 1 = Add leading CR.)        */
void look_score(CONTEXT)
{
     struct descriptor_data *p = getdsc(player);

     /* ---->  Building Quota  <---- */
     if(!Level4(player)) output(p,player,0,1,0,"%s"ANSI_LGREEN"You're currently using "ANSI_LWHITE"%d"ANSI_LGREEN" of your Building Quota of "ANSI_LWHITE"%d"ANSI_LGREEN" (Type '"ANSI_LYELLOW"@quota"ANSI_LGREEN"' for a summary on what you've used your Building Quota on.)\n",(val1) ? "\n":"",db[player].data->player.quota,db[player].data->player.quotalimit);
        else output(p,player,0,1,0,"%s"ANSI_LGREEN"Your Building Quota is unlimited.",(val1) ? "\n":"");

     /* ---->  Score  <---- */
     output(p,player,0,1,0,ANSI_LGREEN"Your score is "ANSI_LWHITE"%d"ANSI_LGREEN" point%s.",db[player].data->player.score,Plural(db[player].data->player.score));

     /* ---->  Credit  <---- */
     if(currency_to_double(&(db[player].data->player.credit)) == 0) output(p,player,0,1,0,ANSI_LGREEN"You have no credit in your pocket.");
        else output(p,player,0,1,0,ANSI_LGREEN"You have "ANSI_LWHITE"%.2f"ANSI_LGREEN" credits in your pocket.",currency_to_double(&(db[player].data->player.credit)));

     /* ---->  Bank balance  <---- */
     if(currency_to_double(&(db[player].data->player.balance)) == 0) output(p,player,0,1,0,ANSI_LGREEN"You have no credit in your bank account.\n");
        else output(p,player,0,1,0,ANSI_LGREEN"You have "ANSI_LWHITE"%.2f"ANSI_LGREEN" credits in your bank account.\n",currency_to_double(&(db[player].data->player.balance)));
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Display random title screen  <---- */
void look_titles(CONTEXT)
{
     struct descriptor_data *p = getdsc(player);
     const  char *ptr,*title,*start,*blanks;

     if((title = help_get_titlescreen(0))) {
        if(IsHtml(p)) {
           blanks = "                                                                              ";
           for(ptr = decompress(title), start = ptr; *ptr && ((*ptr == ' ') || (*ptr == '\n')); ptr++);
           for(; *ptr && (ptr > start) && (*(ptr - 1) == ' '); ptr--);
           if(*ptr && (*ptr == '\n')) ptr++;
           html_anti_reverse(p,1);
           output(getdsc(player),player,0,1,0,"\n\016<TABLE BORDER=5 CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK"><TR><TD><TT>\016"ANSI_LWHITE"%s\n%s\016</TT></TD></TR></TABLE>\016",blanks,ptr);
           html_anti_reverse(p,0);
	} else {
#ifdef PAGE_TITLESCREENS
           if(!in_command && p && !p->pager && More(player)) pager_init(p);
#endif
           output(getdsc(player),player,0,1,0,"\n"ANSI_DWHITE"%s",decompress(title));
	}
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, there are no title screens available at the moment.");
}

/* ----> Display current running time and database creation date/accumulated up-time  <---- */
void look_uptime(CONTEXT)
{
     struct descriptor_data *p = getdsc(player);
     char   buffer[TEXT_SIZE];
     time_t now;

     gettime(now);
     output(p,player,0,1,0,"");
     output(p,player,0,1,2,ANSI_LYELLOW"%s"ANSI_LGREEN" ("ANSI_LYELLOW"%s"ANSI_LGREEN") was restarted on "ANSI_LYELLOW"%s"ANSI_LGREEN" and has been running for "ANSI_LWHITE"%s"ANSI_LGREEN".\n",tcz_full_name,tcz_short_name,date_to_string(uptime + (db[player].data->player.timediff * HOUR),UNSET_DATE,player,FULLDATEFMT),interval(now - uptime,0,ENTITIES,0));
     strcpy(buffer,interval((db_accumulated_uptime + (now - uptime)) / ((db_accumulated_restarts > 1) ? db_accumulated_restarts:1),0,ENTITIES,0));
     output(p,player,0,1,2,ANSI_LGREEN"The database was created on "ANSI_LYELLOW"%s"ANSI_LGREEN" and has been running for an accumulated time of "ANSI_LWHITE"%s"ANSI_LGREEN" (Average is "ANSI_LYELLOW"%s"ANSI_LGREEN".)\n",date_to_string(db_creation_date + (db[player].data->player.timediff * HOUR),UNSET_DATE,player,FULLDATEFMT),interval(db_accumulated_uptime + (now - uptime),0,ENTITIES,0),buffer);

     if((now - uptime) > db_longest_uptime) {
        db_longest_uptime = (now - uptime);
        db_longest_date   = now;
     }

     output(p,player,0,1,2,ANSI_LGREEN"The longest recorded uptime of "ANSI_LWHITE"%s"ANSI_LGREEN" occurred on "ANSI_LYELLOW"%s"ANSI_LGREEN".\n",interval(db_longest_uptime,0,ENTITIES,0),date_to_string(db_longest_date + (db[player].data->player.timediff * HOUR),UNSET_DATE,player,FULLDATEFMT));
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Display version information  <---- */
void look_version(CONTEXT)
{
     struct descriptor_data *p = getdsc(player);

     if(player == NOBODY) {
        for(p = descriptor_list; p && (p->player != NOBODY); p = p->next);
        player = NOTHING;
        if(!p) return;
        if(!IsHtml(p)) p->player = NOTHING;
     }

     tcz_version(p,0);
     setreturn(OK,COMMAND_SUCC);
}
