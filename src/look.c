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
      struct   descriptor_data *d;
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

	 /* ---->  Character's current 'feeling'  <---- */
	 if(db[object].data->player.feeling) {
	    int loop;

	    for(loop = 0; feelinglist[loop].name && (feelinglist[loop].id != db[object].data->player.feeling); loop++);
	    if(feelinglist[loop].name) sprintf(buffer + strlen(buffer)," (%s)",feelinglist[loop].name);
	       else db[object].data->player.feeling = 0;
	 }

	 /* ---->  Rank of character  <---- */
	 if(Level1(object)) {
            strcat(buffer," (Deity)");
	 } else if(Level2(object)) {
            strcat(buffer,(Druid(object)) ? " (Elder Druid)":" (Elder Wizard)");
	 } else if(Level3(object)) {
            strcat(buffer,(Druid(object)) ? " (Druid)":" (Wizard)");
	 } else if(Level4(object)) {
            strcat(buffer,(Druid(object)) ? " (Apprentice Druid)":" (Apprentice Wizard)");
	 } else if(Retired(object)) {
            strcat(buffer,(RetiredDruid(object)) ? " (Retired Druid)":" (Retired Wizard)");
	 } else if(Experienced(object)) {
            strcat(buffer," (Experienced Builder)");
	 } else if(Assistant(object)) {
            strcat(buffer," (Assistant)");
	 }
      }

      if(Typeof(object) == TYPE_CHARACTER) {

	 /* ---->  Set 'now'  <---- */
	 gettime(now);

	 /* ---->  Newbie  <---- */
	 total = db[object].data->player.totaltime + (now - db[object].data->player.lasttime);
	 if((total <= NEWBIE_TIME) && !Level4(object) && !Experienced(object) && !Assistant(object) && !Retired(object))
	    strcat(buffer," (Newbie)");

	 /* ---->  Friend/Enemy?  <---- */
	 if((flags = friend_flags(player,object)|friend_flags(object,player)) && !friendflags_set(player,object,NOTHING,FRIEND_EXCLUDE))
	    strcat(buffer,(flags & FRIEND_ENEMY) ? " (Enemy)":" (Friend)");

	 /* ---->  Birthday?  <---- */
	 longdate = epoch_to_longdate(now);
	 if(hasprofile(db[object].data->player.profile) && ((db[object].data->player.profile->dob & 0xFFFF) == (longdate & 0xFFFF)))
	    sprintf(buffer + strlen(buffer)," (%ld today!)",longdate_difference(db[object].data->player.profile->dob,longdate) / 12);

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
	 sprintf(buffer + strlen(buffer),"%s%s%s%s%s%s%s%s%s%s%s", (afk) ? " (AFK)" : "", Being(object) ? " (Being)" : "", (disconnected) ? " (Connection Lost)" : "", (converse) ? " (Conversing)" : "", (editing) ? " (Editing)" : "", (Engaged(object) && Validchar(Partner(object))) ? " (Engaged)" : "", Haven(object) ? " (Haven)" : "", (Married(object) && Validchar(Partner(object))) ? " (Married)" : "", (monitored) ? " (Monitored)" : "", Puppet(object) ? " (Puppet)" : "", Quiet(object) ? " (Quiet)" : "");
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
	       sprintf(scratch_buffer,"%s%s",desc,(!addcr || cr) ? "\n":"\n\n");
	       ptr = punctuate(scratch_buffer,3,'.');
	       substitute_large(player,player,ptr,ANSI_LWHITE,scratch_buffer,censor);
	    } else output(getdsc(player),player,0,1,0,ANSI_LWHITE"%s%s",desc,(!addcr || cr) ? "":"\n");
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
     for(count = 0; count < level; count++) buffer[count] = ' ';
     buffer[level] = '\0';

     if(Valid(thing)) do {
	if((Typeof(thing) != TYPE_CHARACTER) || ((Typeof(thing) == TYPE_CHARACTER) && (Connected(thing) || (Controller(thing) != thing)))) {
	   if(!((Typeof(thing) == TYPE_CHARACTER) && (root_object != currentobj)) && (!Invisible(thing) || can_write_to(player,thing,0))) {
	      if(!contents) {
		 if(level) output(p,player,2,1,0,"\n");
		 if(!examine && HasField(object,CSTRING) && !Blank(getfield(object,CSTRING))) tilde_string(player,getfield(object,CSTRING),ANSI_LYELLOW,ANSI_DYELLOW,level,level,5);
		    else tilde_string(player,title,ANSI_LYELLOW,ANSI_DYELLOW,level,level,5);
		 contents = 1;
	      }

	      output(p, player, 2, 1, level + 2, "%s%s\n", buffer, look_name_status(player, thing, scratch_buffer, 0));
	      termflags = TXT_NORMAL;
	      if(Container(thing)) look_container(player,thing,"Contents:",level + 2,examine);
	   }
	}
	getnext(thing,CONTENTS,currentobj);
     } while(Valid(thing));

     if(contents && !level)
	output(p,player,2,1,0,"\n");
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
	   }
	   output(p, player, 2, 1, 4, "%s\n", look_name_status(player, thing, scratch_buffer, 1));
	   termflags = TXT_NORMAL;
	}
	getnext(thing,CONTENTS,currentobj);
     } while(Valid(thing));
     if(contents) output(p,player,2,1,0,"\n");
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
           width = (twidth - strlen("Commands executed by compound command") - 4) / 2;
           for(ptr = scratch_buffer + strlen(scratch_buffer), loop = 0; loop < width; *ptr++ = '-', loop++);
           *ptr = '\0', sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LWHITE"  %s  "ANSI_LYELLOW,"Commands executed by compound command");
           for(loop += strlen("Commands executed by compound command") + 4, ptr = scratch_buffer + strlen(scratch_buffer); loop < twidth; *ptr++ = '-', loop++);
           *ptr = '\0', output(p,player,0,1,0,scratch_buffer);

           if(termflags) termflags = TXT_NORMAL;
           if(!Private(object) || can_read_from(player,object)) {
              int    counter = 1,spaces;
              const  char *ptr,*ptr2;
              char   *tmp;

              for(*scratch_buffer = '\0', ptr = desc; *ptr; counter++) {
                  if(*ptr == '\n') ptr++;
                  for(tmp = scratch_return_string; *ptr && (*ptr != '\n'); *tmp++ = *ptr++);
                  *tmp = '\0';
                  for(ptr2 = scratch_return_string, spaces = 0; *ptr2 && (*ptr2 == ' '); ptr2++, spaces++);
                  if((spaces + 7) > 50) spaces = (50 - 7);
                  output(p,player,0,1,7 + spaces,ANSI_DGREEN"[%03d]  "ANSI_DWHITE"%s",counter,scratch_return_string);
	      }
	   } else output(p, player, 2, 1, 0, ANSI_LRED "This compound command is " ANSI_LYELLOW "PRIVATE" ANSI_LRED "  -  Only characters above the level of its owner (%s" ANSI_LWHITE "%s" ANSI_LRED ") may see its description.\n", Article(Owner(object), UPPER, INDEFINITE), getcname(NOTHING, Owner(object), 0, 0));

           if(termflags) termflags = TXT_BOLD;
           strcpy(scratch_buffer,ANSI_LYELLOW), ptr = scratch_buffer + strlen(scratch_buffer);
           for(loop = 0; loop < twidth; *ptr++ = '-', loop++);
           *ptr++ = '\n', *ptr = '\0', output(p,player,0,1,0,scratch_buffer);
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
                          output(p, player, 2, 1, 2, ANSI_LMAGENTA "  Credits:  %.2f\n\n", currency_to_double(&(db[object].data->thing.credit)));
                       break;
                  case TYPE_ROOM:
                       if(currency_to_double(&(db[object].data->room.credit)) != 0)
                          output(p, player, 2, 1, 2, ANSI_LMAGENTA "  Credits:  %.2f\n\n", currency_to_double(&(db[object].data->room.credit)));
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
	   }

           /* ---->  Exit name and clickable link to go through it  <---- */
           strcpy(buffer2,getexitname(Owner(player),exit));
           if(Valid(Destination(exit))) {
              if(!Opaque(exit)) {
                 sprintf(buffer, "  %s lead%s to %s", punctuate(buffer2, 0, '\0'), (Articleof(exit) == ARTICLE_PLURAL) ? "" : "s", Article(Destination(exit), LOWER, INDEFINITE));
                 sprintf(buffer + strlen(buffer), "%s\n", punctuate((char *) unparse_object(player, Destination(exit), 0), 0, '.'));
              } else sprintf(buffer,"  %s\n", punctuate(buffer2, 0, '.'));
           } else sprintf(buffer, "  %s lead%s nowhere.\n", punctuate(buffer2, 0, '\0'), (Articleof(exit) == ARTICLE_PLURAL) ? "" : "s");

           output(p,player,2,1,4,"%s",buffer);
	}
        getnext(exit,EXITS,currentobj);
     } while(Valid(exit));
     if(exits) output(p,player,2,1,0,"\n");
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
                    output(p, player, 2, 1, 2, ANSI_LMAGENTA "  Credits:  " ANSI_LWHITE "%.2f\n\n", currency_to_double(&(db[location].data->thing.credit)));
                 break;
            case TYPE_ROOM:
                 if(currency_to_double(&(db[location].data->room.credit)) != 0)
                    output(p, player, 2, 1, 2, ANSI_LMAGENTA "  Credits:  " ANSI_LWHITE "%.2f%\n\n", currency_to_double(&(db[location].data->room.credit)));
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
              sprintf(scratch_buffer,"%s:",title), found = 2;
              tilde_string(player,scratch_buffer,lansi,dansi,0,0,5);
	   }
           if(currentobj != cur_object)
		output(p, player, 2, 1, 0, "%s" ANSI_DCYAN "[%s inherited from %s" ANSI_LCYAN "%s" ANSI_DCYAN "...]\n", (found == 2) ? "" : "\n", (objecttype == NOTHING) ? "Objects" : title, Article(currentobj, LOWER, INDEFINITE), unparse_object(player, currentobj, 0));
           if(found > 1) found = 1;
           output(p, player, 2, 1, 2, "%s%s\n", dansi, look_name_status(player, thing, scratch_buffer, 0));
           termflags = TXT_NORMAL;
	}
        cur_object = currentobj;
        getnext(thing,list,currentobj);
     } while(Valid(thing));
     if(found) output(p, player, 2, 1, 0, "\n");
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
        p->player = NOTHING;
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

           if(!in_command && p && !p->pager && More(player)) pager_init(p);
           output(p,player,0,0,0,"\n"ANSI_LRED"%s",ptr);
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
     unsigned char            cr;
     unsigned char            experienced = 0;
     time_t                   last,total,now;
     dbref                    looper,thing;
     unsigned long            size,csize;
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
     command_type |= NO_USAGE_UPDATE;
     if((!Private(thing) && !(can_read_from(player,thing) || (experienced = ((Typeof(thing) == TYPE_CHARACTER) && Experienced(db[player].owner))))) || (Private(thing) && !can_read_from(player,thing))) {
        output(p,player,0,1,0,"");
        output(p,player,0,1,8,ANSI_LYELLOW"Owner:  "ANSI_LWHITE"%s.\n",getcname(player,db[thing].owner,1,UPPER|INDEFINITE));
        if(Level4(db[player].owner) && Private(thing))
           output(p,player,0,1,0,ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED" is "ANSI_LYELLOW"PRIVATE"ANSI_LRED" and can only be examined by characters of a higher level than %s owner.\n",Article(thing,UPPER,DEFINITE),getcname(player,thing,1,0),(Typeof(thing) == TYPE_CHARACTER) ? "their":"its");
	      else if((Typeof(thing) == TYPE_CHARACTER) || (Typeof(thing) == TYPE_THING) || (Typeof(thing) == TYPE_ROOM) || (Typeof(thing) == TYPE_EXIT))
                 output(p,player,0,1,0,ANSI_LGREEN"Type '" ANSI_LYELLOW "%s%s%s" ANSI_LGREEN "' to see a more detailed description of %s" ANSI_LWHITE "%s" ANSI_LGREEN ".\n", (Typeof(thing) == TYPE_CHARACTER) ? "scan":"look",Blank(params) ? "":" ",params,Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0));
        command_type &= ~NO_USAGE_UPDATE;
        return;
     }

     /* ---->  Object name  <---- */
     if(!in_command && p && !p->pager && More(player)) pager_init(p);
     if(experienced && ((thing == player) || (thing == db[player].owner))) experienced = 0;
     tilde_string(player,getcname(player,thing,1,UPPER|INDEFINITE),ANSI_LGREEN,ANSI_DGREEN,0,1,6);
     termflags = 0;

     /* ---->  Type and Building Quota Used/Limit  <---- */
     sprintf(scratch_buffer," Type:  "ANSI_LWHITE"%s"ANSI_LCYAN";  Building Quota",ObjectType(thing));
     if(Typeof(thing) == TYPE_CHARACTER) {
        if(!Level4(thing)) output(p, player, 2, 1, 8, "%s:  %s%d/%d.\n", scratch_buffer, (db[thing].data->player.quota > db[thing].data->player.quotalimit) ? ANSI_LRED : ANSI_LWHITE, db[thing].data->player.quota,db[thing].data->player.quotalimit);
           else output(p, player, 2, 1, 8, "%s:  " ANSI_LWHITE "%d/UNLIMITED.\n", scratch_buffer, db[thing].data->player.quota);
     } else if(Typeof(thing) != TYPE_ARRAY) output(p, player, 2, 1, 8, "%s used:  " ANSI_LWHITE "%d.\n", scratch_buffer, ObjectQuota(thing));
        else output(p, player, 2, 1, 8, "%s used:  " ANSI_LWHITE "%d.\n", scratch_buffer, ObjectQuota(thing) + (array_element_count(db[thing].data->array.start) * ELEMENT_QUOTA));

     /* ---->  Owner  <---- */
     output(p, player, 2, 1, ((Typeof(thing) == TYPE_CHARACTER) && (Owner(thing) != thing)) ? 18 : 8, ANSI_LCYAN "%swner:  " ANSI_LWHITE "%s.\n", ((Typeof(thing) == TYPE_CHARACTER) && (Owner(thing) != thing)) ? "Effective o" : "O", getcname(player, Owner(thing), 1, UPPER|INDEFINITE));

     /* ---->  Flags  <---- */
     output(p, player, 2, 1, 8, ANSI_LCYAN "Flags:  " ANSI_LWHITE "%s\n", unparse_flaglist(thing,1,scratch_return_string));

     /* ---->  Size (Memory usage)  <---- */
     size = getsize(thing,1), csize = getsize(thing,0);
     if(csize < size) output(p, player, 2, 1, 8, ANSI_LCYAN " Size:  " ANSI_LWHITE "%d byte%s (%d byte%s compressed (%.1f%%))\n", size, Plural(size), csize, Plural(csize), 100 - (((double) csize / size) * 100));
        else output(p, player, 2, 1, 8, ANSI_LCYAN " Size:  " ANSI_LWHITE "%d byte%s (Not compressed.)\n", size, Plural(size));

     /* ---->  Lock  <---- */
     if(HasField(thing, LOCK)) output(p, player, 2, 1, (inherited > 0) ? 16 : 8, "%s" ANSI_LCYAN "%sey:  " ANSI_LWHITE "%s.\n", (inherited > 0) ? "" :"  ", (inherited > 0) ? "Inherited k" : "K", unparse_boolexp(player,getlock(thing,0),0));

     /* ---->  Standard object fields (Description, Success, Failure, etc.)  <---- */
     if(!val1) {

        /* ---->  Object's description  <---- */
        if(getfield(thing,DESC)) {
           if(Typeof(thing) == TYPE_COMMAND) {
              unsigned char twidth = output_terminal_width(player);
              int           counter = 1;
              short         loop,width;
              char          *tmp;
              const    char *title;
              const    char *ptr;

              title = (inherited > 0) ? "Commands (Inherited) executed by compound command":"Commands executed by compound command";
              strcpy(scratch_buffer,ANSI_LYELLOW"\n");
              width = (twidth - strlen(title) - 4) / 2;
              for(tmp = scratch_buffer + strlen(scratch_buffer), loop = 0; loop < width; *tmp++ = '-', loop++);
              *tmp = '\0', sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LWHITE"  %s  "ANSI_LYELLOW,title);
              for(loop += strlen(title) + 4, tmp = scratch_buffer + strlen(scratch_buffer); loop < twidth; *tmp++ = '-', loop++);
              *tmp = '\0', output(p,player,0,1,0,scratch_buffer);

              if(termflags) termflags = TXT_NORMAL;
              for(*scratch_buffer = '\0', ptr = getfield(thing,DESC); *ptr; counter++) {
                  int  spaces;
                  char *ptr2;

                  if(*ptr == '\n') ptr++;
                  for(tmp = scratch_return_string; *ptr && (*ptr != '\n'); *tmp++ = *ptr++);
                  *tmp = '\0';
                  for(ptr2 = scratch_return_string, spaces = 0; *ptr2 && (*ptr2 == ' '); ptr2++, spaces++);
                  if((spaces + 7) > 50) spaces = (50 - 7);
                  output(p,player,0,1,7 + spaces,ANSI_DGREEN"[%03d]  "ANSI_DWHITE"%s",counter,scratch_return_string);
	      }

              if(termflags) termflags = 0;
              strcpy(scratch_buffer,ANSI_LYELLOW), tmp = scratch_buffer + strlen(scratch_buffer);
              for(loop = 0; loop < twidth; *tmp++ = '-', loop++);
              *tmp++ = '\n', *tmp = '\0', output(p,player,0,1,0,scratch_buffer);
	   } else if(Typeof(thing) != TYPE_FUSE) {
              if(getfield(thing,DESC)) {
                 if(inherited > 0) output(p,player,0,1,0,(getfield(thing,ODESC) && (Typeof(thing) != TYPE_CHARACTER)) ? ANSI_DCYAN"\n[Inherited inside description ("ANSI_LWHITE"@desc"ANSI_DCYAN")...]":ANSI_DCYAN"\n[Inherited description...]");
	            else output(p,player,0,1,0,(getfield(thing,ODESC) && (Typeof(thing) != TYPE_CHARACTER)) ? ANSI_DCYAN"\n[Inside description ("ANSI_LWHITE"@desc"ANSI_DCYAN")...]":"");
	         output(p,player,0,1,0,ANSI_LWHITE"%s\n",getfield(thing,DESC));
	      }
	   } else {
              output(p,player,0,1,0,"");
              output(p, player, 2, 1, (inherited > 0) ? 25 : 15, ANSI_LGREEN "%sicks (Desc):  " ANSI_LWHITE "%s.\n", (inherited > 0) ? "Inherited t" : "T", getfield(thing, DESC));
	   }
	} else if(Typeof(thing) == TYPE_FUSE) {
           output(p,player,0,1,0,"");
           output(p, player, 2, 1, (inherited > 0) ? 25 : 15, ANSI_LGREEN "%sicks (Desc):  " ANSI_LWHITE "Unset.\n", (inherited > 0) ? "Inherited t" : "T");
	} else output(p,player,0,1,0,"");

        /* ---->  Reset of fuse (@drop)  <---- */
        if(Typeof(thing) == TYPE_FUSE) {
           if(getfield(thing, DROP)) output(p, player, 2, 1, (inherited > 0) ? 25 : 15, ANSI_LGREEN "%seset (Drop):  " ANSI_LWHITE "%s.\n\n", (inherited > 0) ? "Inherited r" : "R", getfield(thing, DROP));
	      else output(p, player, 2, 1, (inherited > 0) ? 25 : 15, ANSI_LGREEN "%seset (Drop):  " ANSI_LWHITE "Unset.\n\n", (inherited > 0) ? "Inherited r" : "R");
	}

        /* ---->  Outside description/web site address  <---- */
        if(getfield(thing,ODESC)) {
           if(Typeof(thing) != TYPE_CHARACTER) {
	      output(p,player,0,1,0,ANSI_DCYAN"[%sutside description ("ANSI_LWHITE"@odesc"ANSI_DCYAN")...]\n"ANSI_LWHITE"%s\n", (inherited > 0) ? "Inherited o":"O", getfield(thing, ODESC));
	   } else output(p, player, 2, 1, (inherited > 0) ? 29 : 19, ANSI_LGREEN "%seb site address:  " ANSI_LWHITE "%s\n\n", (inherited > 0) ? "Inherited w" : "W", getfield(thing, WWW));
	}

        /* ---->  Character's race  <---- */
        if((Typeof(thing) == TYPE_CHARACTER) && getfield(thing,RACE))
           output(p, player, 2, 1, (inherited > 0) ? 16 : 6, ANSI_LRED "%sace:  " ANSI_LWHITE "%s\n\n", (inherited > 0) ? "Inherited r" : "R", punctuate((char *) getfield(thing, RACE), 0, '.'));

        /* ---->  Success message/name prefix  <---- */
        if(getfield(thing,SUCC)) {
           if(Typeof(thing) == TYPE_CHARACTER)
              output(p, player, 2, 1, (inherited > 0) ? 24 : 14, ANSI_LGREEN "%same prefix:  " ANSI_LWHITE "%s\n", (inherited > 0) ? "Inherited n" : "N", getfield(thing, SUCC)), cr = 1;
	         else output(p, player, 2, 1, 0, ANSI_LGREEN "%success:  " ANSI_LWHITE "%s\n", (inherited > 0) ? "Inherited s" : "S", getfield(thing, SUCC)), cr = 1;
	} else cr = 0;

        /* ---->  Others success message/name suffix  <---- */
        if(getfield(thing,OSUCC)) {
           if(Typeof(thing) == TYPE_CHARACTER) output(p, player, 2, 1, (inherited > 0) ? 24 : 14, ANSI_LGREEN "%same suffix:  " ANSI_LWHITE "%s\n", (inherited > 0) ? "Inherited n" : "N", getfield(thing, OSUCC)), cr = 1;
	      else output(p, player, 2, 1, 0, ANSI_LGREEN "%ssuccess:  " ANSI_LWHITE "%s\n", (inherited > 0) ? "Inherited o" : "O", getfield(thing, OSUCC)), cr = 1;
	}
        if(cr) output(p,player,0,1,0,"");

	if(Typeof(thing) != TYPE_CHARACTER) {
           cr = 0;

           /* ---->  Failure message  <---- */
   	   if(getfield(thing,FAIL))
              output(p, player, 2, 1, 0, ANSI_LGREEN "%sailure:  " ANSI_LWHITE "%s\n", (inherited > 0) ? "Inherited f" : "F", getfield(thing, FAIL)), cr = 1;

           /* ---->  Others failure message  <---- */
    	   if(getfield(thing,OFAIL))
              output(p, player, 2, 1, 0, ANSI_LGREEN "%sfailure:  " ANSI_LWHITE "%s%\n", (inherited > 0) ? "Inherited o" : "O", getfield(thing, OFAIL)), cr = 1;
           if(cr) output(p,player,0,1,0,"");
	}

        /* ---->  Drop message/E-mail address  <---- */
        cr = 0;
        if((Typeof(thing) != TYPE_FUSE) && getfield(thing,DROP)) {
           if(Typeof(thing) == TYPE_CHARACTER) {
              const char *email = getfield(thing,EMAIL);
              const char *emailaddr;
              int   counter = 1;

              if((emailaddr = forwarding_address(thing,1,scratch_return_string)))
                 output(p, player, 2, 1, (inherited > 0) ? 41 : 31, ANSI_LGREEN "   %sE-mail forwarding address:  " ANSI_LWHITE "%s\n", (inherited > 0) ? "Inherited " : "", emailaddr), cr = 1;

              for(; counter <= EMAIL_ADDRESSES; counter++) {
                  if(((counter != 2) || Level4(db[player].owner) || can_write_to(player,thing,1)) && (emailaddr = gettextfield(counter,'\n',email,0,scratch_return_string)) && *emailaddr) {
                     if(!strcasecmp("forward", emailaddr))
			output(p, player, 2, 1, (inherited > 0) ? 41 : 31, "%s" ANSI_LGREEN "%s%s (%s) E-mail address:  " ANSI_DCYAN "<FORWARDED TO PRIVATE ADDRESS>\n", (counter != 2) ? " ":"", (inherited > 0) ? "Inherited " : "", rank(counter), (counter == 2) ? "Private" : "Public"), cr = 1;
                        else output(p, player, 2, 1, (inherited > 0) ? 41 : 31, "%s" ANSI_LGREEN "%s%s (%s) E-mail address:  " ANSI_LWHITE "%s\n", (counter != 2) ? " " : "", (inherited > 0) ? "Inherited " : "", rank(counter), (counter == 2) ? "Private" : "Public", emailaddr), cr = 1;
		  }
	      }
              if(cr) output(p,player,0,1,0,""), cr = 0;
	   } else output(p, player, 2, 1, 0, ANSI_LGREEN "%srop:  " ANSI_LWHITE "%s\n", (inherited > 0) ? "Inherited d" : "D", getfield(thing, DROP)), cr = 1;
	}

        /* ---->  Others drop message/title  <---- */
        if(getfield(thing,ODROP)) {
           if(Typeof(thing) == TYPE_CHARACTER) {
              if(cr) output(p,player,0,1,0,"");
              output(p, player, 2, 1, (inherited > 0) ? 18 : 8, ANSI_LGREEN "%sitle:  " ANSI_LWHITE "%s\n", (inherited > 0) ? "Inherited t" : "T", getfield(thing, TITLE)), cr = 1;
	   } else output(p, player, 2, 1, 0, ANSI_LGREEN "%sdrop:  " ANSI_LWHITE "%s\n", (inherited > 0) ? "Inherited o" : "O", getfield(thing, ODROP)), cr = 1;
	}
        if(cr) output(p,player,0,1,0,"");

        /* ---->  Character login time fields  <---- */
        cr = 0;
        if(Typeof(thing) == TYPE_CHARACTER) {
           struct descriptor_data *w = getdsc(thing);
           time_t idle,active;

           /* ---->  Last time connected  <---- */
           last = db[thing].data->player.lasttime;
           if(last > 0) {
              total = now - last;
              if(last  > 0) last += (db[player].data->player.timediff * HOUR);
              if(total > 0) output(p, player, 2, 1, 23, ANSI_LCYAN " Last time connected:  " ANSI_LWHITE "%s " ANSI_DCYAN "(" ANSI_LYELLOW "%s " ANSI_LCYAN "ago" ANSI_DCYAN ".)\n", (last == 0) ? "Unknown" : date_to_string(last, UNSET_DATE, player, FULLDATEFMT), interval(total, total, ENTITIES, 0)), cr = 1;
                 else output(p, player, 2, 1, 23, ANSI_LCYAN " Last time connected:  " ANSI_LWHITE "%s.\n", (last == 0) ? "Unknown" : date_to_string(last, UNSET_DATE, player, FULLDATEFMT)), cr = 1;
	   }

           /* ---->  Longest connect time  <---- */
           if(Connected(thing)) {
              if((total = (now - db[thing].data->player.lasttime)) == now) total = 0;
              if(db[thing].data->player.longesttime < total) db[thing].data->player.longesttime = total;
	   }
           if(db[thing].data->player.longesttime > 0)
              output(p, player, 2, 1, 23, ANSI_LCYAN "Longest connect time:  " ANSI_LWHITE "%s " ANSI_DCYAN "(" ANSI_LYELLOW "%s" ANSI_DCYAN ".)\n", interval(db[thing].data->player.longesttime, db[thing].data->player.longesttime, ENTITIES, 0), date_to_string(TimeDiff(db[thing].data->player.longestdate, player), UNSET_DATE, player, FULLDATEFMT)), cr = 1;

           /* ---->  Total time connected  <---- */
           total = db[thing].data->player.totaltime;
           if(Connected(thing)) total += (now - db[thing].data->player.lasttime);
           strcpy(scratch_return_string,interval(total / ((db[thing].data->player.logins > 1) ? db[thing].data->player.logins:1),0,ENTITIES,0));
           if(total > 0) output(p, player, 2, 1, 23, ANSI_LCYAN "Total time connected:  " ANSI_LWHITE "%s " ANSI_DCYAN "(" ANSI_LCYAN "Average " ANSI_LYELLOW "%s" ANSI_DCYAN ".)\n", interval(total, total, ENTITIES, 0), scratch_return_string), cr = 1;

	   /* ---->  Time spent idle  <---- */
	   idle = db[thing].data->player.idletime;
	   if(Connected(thing) && w) idle += (now - w->last_time);
	   strcpy(scratch_return_string,interval(idle / ((db[thing].data->player.logins > 1) ? db[thing].data->player.logins:1),0,ENTITIES,0));
	   if(idle > 0) output(p, player, 2, 1, 23, ANSI_LRED "   Time spent idling:  " ANSI_LWHITE "%s " ANSI_DCYAN "(" ANSI_LCYAN "Average " ANSI_LYELLOW "%s" ANSI_DCYAN ".)\n", interval(idle, idle, ENTITIES, 0), scratch_return_string);

	   /* ---->  Time spent active  <---- */
	   active = total - idle;
	   strcpy(scratch_return_string,interval(active / ((db[thing].data->player.logins > 1) ? db[thing].data->player.logins:1),0,ENTITIES,0));
	   if(active > 0) output(p, player, 2, 1, 23, ANSI_LGREEN "   Time spent active:  " ANSI_LWHITE "%s " ANSI_DCYAN "(" ANSI_LCYAN "Average " ANSI_LYELLOW "%s" ANSI_DCYAN ".)\n", interval(active, active, ENTITIES, 0), scratch_return_string);

           /* ---->  Site character last connected from  <---- */
           if(!experienced && !Blank(getfield(thing,LASTSITE)))
              output(p, player, 2, 1, (inherited < 1) ? 23 : 32, ANSI_LCYAN "%sast connected from:  " ANSI_LWHITE "%s.\n", (inherited > 0) ? "Inherited l" : " L", getfield(thing, LASTSITE)), cr = 1;

           /* ---->  Total logins  <---- */
           total = ((now - db[thing].created) / ((db[thing].data->player.logins > 1) ? db[thing].data->player.logins:1));
           output(p, player, 2, 1, 23, ANSI_LCYAN "        Total logins:  " ANSI_LWHITE "%d " ANSI_DCYAN "(" ANSI_LCYAN "Average " ANSI_LYELLOW "%s" ANSI_DCYAN ".)\n", db[thing].data->player.logins, interval(ABS((total < MINUTE) ? MINUTE : total), 0, ENTITIES, 0));
	}
        if(cr) output(p,player,0,1,0,"");

        /* ---->  Lock key, 'Contents:' string and 'Obvious Exits:' string  <---- */
        cr = 0;

        if(Typeof(thing) == TYPE_THING)
           output(p, player, 2, 1, (inherited > 0) ? 21 : 11, ANSI_LGREEN "%sock key:  " ANSI_LWHITE "%s\n\n", (inherited > 0) ? "Inherited l" : "L", unparse_boolexp(player, getlock(thing, 1), 0));
        if(!Blank(getfield(thing,CSTRING)))
           output(p, player, 2, 1, (inherited > 0) ? 28 : 18, ANSI_LGREEN "%sontents string:  " ANSI_LWHITE "%s\n", (inherited > 0) ? "Inherited c" : "C", getfield(thing, CSTRING)), cr = 1;
        if(!Blank(getfield(thing,ESTRING)))
           output(p, player, 2, 1, (inherited > 0) ? 25 : 15, ANSI_LGREEN "%sxits string:  " ANSI_LWHITE "%s\n", (inherited > 0) ? "Inherited e" : "E", getfield(thing, ESTRING)), cr = 1;
        if(cr) output(p,player,0,1,0,"");

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
           if(array_display_elements(player,elementfrom,elementto,thing,0) < 1)
              output(p,player,2,1,0,ANSI_LRED"Sorry, that element (Or range of elements) is invalid.\n\n");
	}

        if(d) {
           /* ---->  Terminal dimensions and type  <---- */
           if((d->terminal_width > 0) && (d->terminal_height > 0)) sprintf(scratch_return_string," ("ANSI_LWHITE"%dx%d"ANSI_LCYAN")",d->terminal_width + 1,d->terminal_height);
              else if(d->terminal_width > 0) sprintf(scratch_return_string," ("ANSI_LWHITE"%d columns"ANSI_LCYAN")",d->terminal_width + 1);
                 else if(d->terminal_height > 0) sprintf(scratch_return_string," ("ANSI_LWHITE"%d lines"ANSI_LCYAN")",d->terminal_height);
                    else strcpy(scratch_return_string,ANSI_LWHITE".");

           if(d->terminal_type) output(p, player, 2, 1, 16, ANSI_LCYAN "Terminal Type:  " ANSI_LWHITE "%s" ANSI_LCYAN "%s\n\n", d->terminal_type, scratch_return_string);
              else output(p, player, 2, 1, 16, ANSI_LCYAN "Terminal Type:  " ANSI_LWHITE "Unknown" ANSI_LCYAN "%s\n\n", scratch_return_string);

           /* ---->  Chatting channel  <---- */
           if(d && ((player == thing) || Level4(Owner(player))))
              if(d->channel != NOTHING) output(p, player, 2, 1, 0, ANSI_LCYAN "Chatting channel:  " ANSI_LWHITE "%d.\n\n", d->channel);

           /* ---->  Monitor (Visible to Admin only)  <---- */
           if(d && Level4(player) && d->monitor && Validchar(d->monitor->player) && (player != thing))
              output(p, player, 2, 1, 15, ANSI_LBLUE "Monitored by:  " ANSI_LWHITE "%s%s\n\n", getcname(player, d->monitor->player, 1, UPPER|INDEFINITE), (d->flags & MONITOR_OUTPUT) ? (d->flags & MONITOR_CMDS) ? ANSI_DCYAN " (Commands and output monitored.)" : ANSI_DCYAN " (Output monitored only.)" : (d->flags & MONITOR_CMDS) ? ANSI_DCYAN " (Commands monitored only.)" : ".");
	}

        /* ---->  Banned character?  <---- */
        if((Typeof(thing) ==  TYPE_CHARACTER) && db[thing].data->player.bantime) {
           if(db[thing].data->player.bantime != -1) {
	      if((db[thing].data->player.bantime - now) > 0)
                 output(p, player, 2, 1, 0, ANSI_LRED "This character is banned for " ANSI_LWHITE "%s" ANSI_LRED ".\n\n", interval(db[thing].data->player.bantime - now, db[thing].data->player.bantime - now, ENTITIES, 0));
	            else db[thing].data->player.bantime = 0;
	   } else output(p, player, 2, 1, 0, ANSI_LRED "This character is banned permanently.\n\n");
	}
     } else output(p,player,0,1,0,"");

     /* ---->  Date of last use, creation date and expiry  <---- */
     output(p, player, 2, 1, 16, ANSI_LMAGENTA "Creation date:  " ANSI_LWHITE "%s (" ANSI_LYELLOW "%s" ANSI_LWHITE " ago.)\n", date_to_string(db[thing].created, UNSET_DATE, player, FULLDATEFMT), interval(now - db[thing].created, now - db[thing].created, ENTITIES, 0));
     output(p, player, 2, 1, 16, ANSI_LMAGENTA "    Last used:  " ANSI_LWHITE "%s (" ANSI_LYELLOW "%s" ANSI_LWHITE " ago.)\n%s", date_to_string(db[thing].lastused, UNSET_DATE, player, FULLDATEFMT), interval(now - db[thing].lastused, now - db[thing].lastused, ENTITIES, 0), (db[thing].expiry > 0) ? "" : "\n");

     if(db[thing].expiry > 0) {
        if((temp = (db[thing].expiry * DAY)) > (now - (Expiry(thing) ? db[thing].created:db[thing].lastused)))
           temp = (db[thing].expiry * DAY) - (now - (Expiry(thing)? db[thing].created:db[thing].lastused));
	      else temp = 0;
        if(temp > 0) output(p, player, 2, 1, 16, ANSI_LMAGENTA "  Expiry time:  " ANSI_LWHITE "%d day%s (" ANSI_LYELLOW "%s" ANSI_LWHITE " left.)\n\n", db[thing].expiry, Plural(db[thing].expiry), interval(temp, temp, ENTITIES, 0));
           else output(p, player, 2, 1, 16, ANSI_LMAGENTA "  Expiry time:  " ANSI_LWHITE "%d day%s (" ANSI_LRED ANSI_BLINK "No time remaining" ANSI_LWHITE ".)\n\n", db[thing].expiry, Plural(db[thing].expiry));
     }

     /* ---->  Location  <---- */
     cr = 0;
     if(Valid(db[thing].location)) {
        if(!((Secret(thing) || Secret(db[thing].location)) && !can_write_to(player,db[thing].location,1) && !can_write_to(player,thing,1))) {
           dbref area = get_areaname_loc(db[thing].location);
           if(Valid(area) && !Blank(getfield(area,AREANAME)))
              sprintf(scratch_return_string,ANSI_LYELLOW" in "ANSI_LWHITE"%s"ANSI_LYELLOW,getfield(area,AREANAME));
                 else *scratch_return_string = '\0';
           output(p, player, 2, 1, 11, ANSI_LYELLOW "Location:  " ANSI_LWHITE "%s%s.\n", unparse_object(player, db[thing].location, UPPER|INDEFINITE), scratch_return_string), cr = 1;
	} else output(p, player, 2, 1, 11, ANSI_LYELLOW "Location:  " ANSI_LWHITE "Secret.\n"), cr = 1;
     }

     /* ---->  Object's parent object  <---- */
     if(Valid(db[thing].parent))
        output(p, player, 2, 1, 9, ANSI_LYELLOW "Parent:  " ANSI_LWHITE "%s.\n", unparse_object(player, db[thing].parent, UPPER|INDEFINITE)), cr = 1;

     switch(Typeof(thing)) {
            case TYPE_ROOM:
                
                 /* ---->  Drop-to location  <---- */
                 if(Valid(db[thing].destination))
                    output(p, player, 2, 1, 24, ANSI_LYELLOW "Dropped objects go to:  " ANSI_LWHITE "%s.\n", unparse_object(player, db[thing].destination, UPPER|INDEFINITE)), cr = 1;

                 /* ---->  Credits dropped in room  <---- */
                 if(currency_to_double(&(db[thing].data->room.credit)) != 0)
                    output(p, player, 2, 1, 0, ANSI_LYELLOW "Credit:  " ANSI_LWHITE "%.2f credits.\n", currency_to_double(&(db[thing].data->room.credit))), cr = 1;

                 if(!val1) {

                    /* ---->  Area name  <---- */
                    if(!Blank(getfield(thing,AREANAME)))
                       output(p, player, 2, 1, (inherited > 0) ? 22 : 12, ANSI_LYELLOW "%srea name:  " ANSI_LWHITE "%s.\n", (inherited > 0) ? "Inherited a" : "A", getfield(thing, AREANAME)), cr = 1;

                    /* ---->  Weight of contents  <---- */
                    if(cr) output(p,player,0,1,0,"");
                    temp = getweight(thing);
                    output(p, player, 2, 1, 0, ANSI_LRED "Weight of contents:  " ANSI_LWHITE "%d Kilogram%s.\n", temp, Plural(temp));

                    /* ---->  Volume limit  <---- */
                    if((temp = getvolumeroom(thing)) != TCZ_INFINITY)
                       output(p, player, 2, 1, 0, ANSI_LRED "%solume limit:  " ANSI_LWHITE "%d Litre%s.\n", (inherited > 0) ? "Inherited v" : "V", temp, Plural(temp));
                          else output(p, player, 2, 1, 0, ANSI_LRED "%solume limit:  " ANSI_LWHITE "Infinity.\n", (inherited > 0) ? "Inherited v" : "V");

                    /* ---->  Mass limit  <---- */
                    if((temp = getmassroom(thing)) != TCZ_INFINITY)
                       output(p, player, 2, 1, 0, ANSI_LRED "%sass limit:  " ANSI_LWHITE "%d Kilogram%s.\n", (inherited > 0) ? "Inherited m" : "M", temp, Plural(temp));
                          else output(p, player, 2, 1, 0, ANSI_LRED "%sass limit:  " ANSI_LWHITE "Infinity.\n", (inherited > 0) ? "Inherited m" : "M");
		 }
                 break;
            case TYPE_CHARACTER:

                 /* ---->  Who character is currently building as  <---- */
                 if((Uid(thing) != thing) && Validchar(Uid(thing)))
                    output(p, player, 2, 1, 24, ANSI_LYELLOW "Currently building as:  " ANSI_LWHITE "%s.\n", getcname(player, Uid(thing), 1, UPPER|INDEFINITE));

                 /* ---->  Mail redirect  <---- */
                 if(Validchar(db[thing].data->player.redirect))
                    output(p, player, 2, 1, 21, ANSI_LYELLOW "Mail redirected to:  " ANSI_LWHITE "%s.\n", getcname(player, db[thing].data->player.redirect, 1, UPPER|INDEFINITE));

                 if(!val1) {

                    /* ---->  Time difference  <---- */
                    if(db[thing].data->player.timediff)
                       output(p, player, 2, 1, 0, ANSI_LYELLOW "Time difference:  " ANSI_LWHITE "%d hour%s.\n", db[thing].data->player.timediff, Plural(db[thing].data->player.timediff));

                    /* ---->  Screen height  <---- */
                    if(db[thing].data->player.scrheight)
                       output(p, player, 2, 1, 0, ANSI_LYELLOW "Screen height:  " ANSI_LWHITE "%d line%s.\n", db[thing].data->player.scrheight, Plural(db[thing].data->player.scrheight));
 		 }

                 /* ---->  Controller  <---- */
                 if((Controller(thing) != thing) && Validchar(Controller(thing)))
                    output(p, player, 2, 1, 13, ANSI_LYELLOW "Controller:  " ANSI_LWHITE "%s.\n", getcname(player, Controller(thing), 1, UPPER|INDEFINITE));

                 if(!val1) {

                    /* ---->  Partner (If married/engaged)  <---- */
                    if(Valid(Partner(thing)))
                       output(p, player, 2, 1, 13, ANSI_LYELLOW "%s to:  " ANSI_LWHITE "%s.\n", Married(thing) ? "Married" : "Engaged", getcname(player, Partner(thing), 1, UPPER|INDEFINITE));

                    /* ---->  Mail count  <---- */
                    if(db[thing].data->player.mail) {
                       short  count = 0,unread = 0;
                       struct mail_data *ptr;

                       for(ptr = db[thing].data->player.mail; ptr; ptr = ptr->next, count++)
                           if(ptr->flags & MAIL_UNREAD) unread++;
                       output(p, player, 2, 1, 0, ANSI_LYELLOW "Mail items:  " ANSI_LWHITE "%d (%d unread.)\n", count, unread);
		    }
		 }

		 /* ---->  Mail limit  <---- */
                 if(!experienced)
                    output(p, player, 2, 1, 0, ANSI_LYELLOW "Mail limit:  " ANSI_LWHITE "%d.\n", db[thing].data->player.maillimit);

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
                             if(inherited_aliases > 0) output(p, player, 2, 1, 10, ANSI_LYELLOW "Aliases:  " ANSI_LWHITE "%d (%d inherited.)\n", looper + inherited_aliases, inherited_aliases);
                                else output(p, player, 2, 1, 10, ANSI_LYELLOW "Aliases:  " ANSI_LWHITE "%d.\n", looper);
		          } else output(p, player, 2, 1, 20, ANSI_LYELLOW "Inherited aliases:  " ANSI_LWHITE "%d.\n", inherited_aliases);
		       }
		    }

                    /* ---->  Feeling  <---- */
                    if(db[thing].data->player.feeling) {
                       int loop;

                       for(loop = 0; feelinglist[loop].name && (feelinglist[loop].id != db[thing].data->player.feeling); loop++);
                       if(feelinglist[loop].name)
                          output(p, player, 2, 1, 10, ANSI_LYELLOW "Feeling:  " ANSI_LWHITE "%s.\n", feelinglist[loop].name);
		    }

                    /* ---->  Friends/enemies  <---- */
                    if(!experienced && db[thing].data->player.friends) {
                       int    friends = 0,enemies = 0,exclude = 0;
                       struct friend_data *ptr;

                       for(looper = 0, ptr = db[thing].data->player.friends; ptr; ptr = ptr->next)
                           if(ptr->flags & FRIEND_EXCLUDE) exclude++;
                              else if(ptr->flags & FRIEND_ENEMY) enemies++;
                                 else friends++;

                       if(friends > 0)
                          output(p, player, 2, 1, 0, ANSI_LYELLOW "Friends:  " ANSI_LWHITE "%d.\n", friends);

                       if(enemies > 0)
                          output(p, player, 2, 1, 0, ANSI_LYELLOW "Enemies:  " ANSI_LWHITE "%d.\n", enemies);

                       if(exclude > 0)
                          output(p, player, 2, 1, 0, ANSI_LYELLOW "Excluded:  " ANSI_LWHITE "%d.\n", exclude);
		    }

                    /* ---->  Credit (Pocket)  <---- */
                    if(currency_to_double(&(db[thing].data->player.credit)) != 0)
                       output(p, player, 2, 1, 0, ANSI_LYELLOW "Credit:  " ANSI_LWHITE "%.2f credits.\n", currency_to_double(&(db[thing].data->player.credit)));
		          else output(p, player, 2, 1, 0, ANSI_LYELLOW "Credit:  " ANSI_LWHITE "None.\n");

                    /* ---->  Balance (Bank)  <---- */
                    if(currency_to_double(&(db[thing].data->player.balance)) != 0)
                       output(p, player, 2, 1, 0, ANSI_LYELLOW "Balance:  " ANSI_LWHITE "%.2f credits.\n", currency_to_double(&(db[thing].data->player.balance)));
		          else output(p, player, 2, 1, 0, ANSI_LYELLOW "Balance:  " ANSI_LWHITE "None.\n");

                    /* ---->  Score  <---- */
                    output(p, player, 2, 1, 0, ANSI_LYELLOW "Score:  " ANSI_LWHITE "%d point%s.\n", db[thing].data->player.score, Plural(db[thing].data->player.score));
		 }

                 /* ---->  Home room  <---- */
                 output(p, player, 2, 1, 7, ANSI_LYELLOW "Home:  " ANSI_LWHITE "%s.\n", unparse_object(player, db[thing].destination,UPPER|INDEFINITE));

                 if(!val1) {

                    /* ---->  Weight  <---- */
                    temp = getweight(thing);
                    output(p, player, 2, 1, 0, ANSI_LRED "\nWeight:  " ANSI_LWHITE "%d Kilogram%s.\n", temp, Plural(temp));

                    /* ---->  Volume  <---- */
                    if((temp = getvolumeplayer(thing)) != TCZ_INFINITY)
                       output(p, player, 2, 1, 0, ANSI_LRED "%solume:  " ANSI_LWHITE "%d Litre%s.\n", (inherited > 0) ? "Inherited v" : "V", temp, Plural(temp));
                          else output(p, player, 2, 1, 0, ANSI_LRED "%solume:  " ANSI_LWHITE "Infinity.\n", (inherited > 0) ? "Inherited v" : "V");

                    /* ---->  Mass  <---- */
                    if((temp = getmassplayer(thing)) != TCZ_INFINITY)
                       output(p, player, 2, 1, 0, ANSI_LRED "%sass:  " ANSI_LWHITE "%d Kilogram%s.\n", (inherited > 0) ? "Inherited m" : "M", temp, Plural(temp));
                          else output(p, player, 2, 1, 0, ANSI_LRED "%sass:  " ANSI_LWHITE "Infinity.\n", (inherited > 0) ? "Inherited m" : "M");
		 }
                 break;
            case TYPE_THING:

                 /* ---->  Area name  <---- */
                 if(!val1 && !Blank(getfield(thing,AREANAME)))
                    output(p, player, 2, 1, (inherited > 0) ? 22 : 12, ANSI_LYELLOW "%srea name:  " ANSI_LWHITE "%s.\n", (inherited > 0) ? "Inherited a" : "A", getfield(thing, AREANAME));

                 /* ---->  Credit dropped in thing  <---- */
                 if(currency_to_double(&(db[thing].data->thing.credit)) != 0)
                    output(p, player, 2, 1, 0, ANSI_LYELLOW "Credit:  " ANSI_LWHITE "%.2f credits.\n", currency_to_double(&(db[thing].data->thing.credit)));

                 /* ---->  Home location  <---- */
                 output(p, player, 2, 1, 7, ANSI_LYELLOW "Home:  " ANSI_LWHITE "%s.\n\n", unparse_object(player, db[thing].destination, UPPER|INDEFINITE));

                 if(!val1) {

                    /* ---->  Weight  <---- */
                    temp = getweight(thing);
                    output(p, player, 2, 1, 0, ANSI_LRED "Weight:  " ANSI_LWHITE "%d Kilogram%s.\n", temp, Plural(temp));

                    /* ---->  Volume  <---- */
                    if((temp = getvolumething(thing)) != TCZ_INFINITY)
                       output(p, player, 2, 1, 0, ANSI_LRED "%solume:  " ANSI_LWHITE "%d Litre%s.\n", (inherited > 0) ? "Inherited v" : "V", temp, Plural(temp));
                          else output(p, player, 2, 1, 0, ANSI_LRED "%solume:  " ANSI_LWHITE "Infinity.\n", (inherited > 0) ? "Inherited v" : "V");

                    /* ---->  Mass  <---- */
                    if((temp = getmassthing(thing)) != TCZ_INFINITY)
                       output(p, player, 2, 1, 0, ANSI_LRED "%sass:  " ANSI_LWHITE "%d Kilogram%s.\n", (inherited > 0) ? "Inherited m" : "M", temp, Plural(temp));
                          else output(p, player, 2, 1, 0, ANSI_LRED "%sass:  " ANSI_LWHITE "Infinity.\n", (inherited > 0) ? "Inherited m" : "M");
		 }
                 break;
            case TYPE_EXIT:

                 /* ---->  Destination location  <---- */
                 if(Valid(db[thing].destination))
                    output(p, player, 2, 1, 14, ANSI_LYELLOW "Destination:  " ANSI_LWHITE "%s.\n", unparse_object(player, db[thing].destination, UPPER|INDEFINITE));
                 break;
            case TYPE_COMMAND:
            case TYPE_FUSE:

                 /* ---->  Success/failure links  <---- */
		 output(p,player,0,1,0,"");
                 output(p, player, 2, 1, 30, ANSI_LGREEN "On success, execute (CSUCC):  " ANSI_LWHITE "%s.\n", unparse_object(player, db[thing].contents, 0));
                 output(p, player, 2, 1, 30, ANSI_LRED "On failure, execute (CFAIL):  " ANSI_LWHITE "%s.\n", unparse_object(player, db[thing].exits, 0));
                 break;
            case TYPE_ALARM:

                 /* ---->  Compound command executed by alarm  <---- */
		 output(p,player,0,1,0,"");
                 output(p, player, 2, 1, 36, ANSI_LGREEN "Compound command executed (CSUCC):  " ANSI_LWHITE "%s.\n", unparse_object(player, db[thing].destination, 0));
                 break;
            default:
                 break;
     }

     output(p,player,0,1,0,"");
     termflags = 0, command_type &= ~NO_USAGE_UPDATE;
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

     if(!in_command && p && !p->pager && More(player)) pager_init(p);
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
     if(!in_command && p && !p->pager && More(player)) pager_init(p);
     if(!Blank(params)) {
        for(; *params && (*params == ' '); params++);
        if(val1 && (((strlen(params) == 2) && !strcasecmp("at",params)) || !strncasecmp(params,"at ",3))) {

           /* ---->  Look 'at' an object  <---- */
           for(params += 2; *params && (*params == ' '); params++);
           if(Blank(params)) {
              output(p,player,0,1,0,ANSI_LGREEN"Please specify what you'd like to look at.");
              command_type &= ~NO_USAGE_UPDATE;
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
              command_type &= ~NO_USAGE_UPDATE;
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
     output(p,player,0,1,0,"%s",substitute(player,scratch_return_string,ANSI_LWHITE"TCZ is free software, which is distributed under %c%lversion 2%x of the %c%lGNU General Public License%x (See '%g%l%ugpl%x' in TCZ, or visit %b%l%uhttp://www.gnu.org%x)  For more information about the %y%lTCZ%x, please visit:  %b%l%uhttps://github.com/smcvey/tcz%x\n",0,ANSI_LWHITE,NULL,0));

#ifdef DEMO
     output(p,player,0,1,0,ANSI_LGREEN"\nWelcome to the demonstration version of "ANSI_LWHITE"%s"ANSI_LGREEN" (TCZ v"TCZ_VERSION"), %s"ANSI_LYELLOW"%s"ANSI_LGREEN"!\n",tcz_full_name,Article(player,LOWER,DEFINITE),getcname(NOTHING,player,0,0));
#else
     output(p,player,0,1,0,ANSI_LGREEN"\nWelcome to "ANSI_LWHITE"%s"ANSI_LGREEN" (TCZ v"TCZ_VERSION"), %s"ANSI_LYELLOW"%s"ANSI_LGREEN"!\n",tcz_full_name,Article(player,LOWER,DEFINITE),getcname(NOTHING,player,0,0));
#endif

     output(p,player,0,1,0,ANSI_LMAGENTA"Admin E-mail:  "ANSI_LYELLOW"%s",tcz_admin_email);
     output(p,player,0,1,0,ANSI_LMAGENTA"    Web Site:  "ANSI_LYELLOW"%s\n",html_home_url);

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
        output(p,player,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"]  "ANSI_LWHITE"You last changed your password "ANSI_LCYAN"%s"ANSI_LWHITE" ago on "ANSI_LYELLOW"%s"ANSI_LWHITE".\n\nYou should change your password regularly (At least once every "ANSI_LCYAN"2-3 months"ANSI_LWHITE"), otherwise you run the risk of somebody else gaining unauthorised access to your character, possibly causing malicious damage to yourself or your objects.  To change your password, simply type "ANSI_LGREEN""ANSI_UNDERLINE"@PASSWORD"ANSI_LWHITE" and follow the on-screen messages.  After you have done this, you will no-longer see this message.\n",
	interval(now - db[player].data->player.pwexpiry,0,3,0),date_to_string(db[player].data->player.pwexpiry,UNSET_DATE,player,FULLDATEFMT)); */

     /* ---->  Warn user of effective '@chuid'  <---- */
     if(Uid(player) != player)
        output(p,player,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"]  "ANSI_LWHITE"You are currently building under the ID of %s"ANSI_LYELLOW"%s"ANSI_LWHITE".  To build as yourself again, please type '"ANSI_LGREEN"@chuid"ANSI_LWHITE"'.\n",Article(Uid(player),LOWER,DEFINITE),getcname(player,Uid(player),1,0));

     /* ---->  Warn user of failed logins  <---- */
     if(db[player].data->player.failedlogins) {
        output(p,player,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"]  "ANSI_LWHITE"There %s been%s "ANSI_LYELLOW"%d"ANSI_LWHITE" failed login attempt%s as your character since you last successfully connected on "ANSI_LCYAN"%s"ANSI_LWHITE".  If you get this message on a regular basis, please change your password by typing '"ANSI_LGREEN"@password"ANSI_LWHITE"'.\n",
              (db[player].data->player.failedlogins == 1) ? "has":"have",(db[player].data->player.failedlogins < 255) ? "":" over",db[player].data->player.failedlogins,Plural(db[player].data->player.failedlogins),date_to_string(db[player].data->player.lasttime + (db[player].data->player.timediff * HOUR),UNSET_DATE,player,FULLDATEFMT));
        if(val1 != 2) db[player].data->player.failedlogins = 0;
     }

     /* ---->  Warn user of logging level privacy invasion  <---- */
     if(option_loglevel(OPTSTATUS) >= 3)
        output(p,player,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"]  "ANSI_LWHITE"The current server logging level may breach your privacy by logging the commands you type in full (Allowing administrators to see your private conversations.)  The reason for this is possible server debugging (Please ask a member of Admin.)\n");

     /* ---->  Warn user of enabled BETA features  <---- */
#ifdef BETA
     output(p,player,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"]  "ANSI_LMAGENTA"This is a BETA release of %s.  "ANSI_LYELLOW"Unfinished BETA features are currently enabled, which may result in instability/crashes.\n",tcz_full_name);
#endif

     /* ---->  Warn user if host server is running on backup battery power  <---- */
#ifdef UPS_SUPPORT
     if(!powerstate) {
	if(Level4(player)) {
	   output(p,player,0,0,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"]  "ANSI_LYELLOW"Power to the server on which %s runs was lost at "ANSI_LWHITE"%s"ANSI_LYELLOW" ("ANSI_LWHITE"%s"ANSI_LYELLOW" ago.)  This is the "ANSI_LWHITE"%s"ANSI_LYELLOW" power failure.  The server is currently running on "ANSI_UNDERLINE"battery backup"ANSI_LYELLOW", which may run out if mains power is not resumed again soon.\n\nIf the power fails, any changes made to the database may be lost.\n\nWhen mains power is resumed, you will receive notification of this.  If you suddenly get disconnected, the battery backup has probably run out of power (Try connecting again later when mains power has been resumed.)\n",tcz_full_name,date_to_string(powertime,UNSET_DATE,p->player,FULLDATEFMT),interval(now - powertime,0,ENTITIES,0),rank(powercount));
	} else {
	   output(p,player,0,0,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"]  "ANSI_LYELLOW"Power to the server on which %s runs was lost at "ANSI_LWHITE"%s"ANSI_LYELLOW" ("ANSI_LWHITE"%s"ANSI_LYELLOW" ago.)  The server is currently running on "ANSI_UNDERLINE"battery backup"ANSI_LYELLOW", which may run out if mains power is not resumed again soon.\n\nIf the power fails, any building work or changes made to your character, objects, areas, etc. may be lost.\n\nWhen mains power is resumed, you will receive notification of this.  If you suddenly get disconnected, the battery backup has probably run out of power (Try connecting again later when mains power has been resumed.)\n",tcz_full_name,date_to_string(powertime,UNSET_DATE,p->player,FULLDATEFMT),interval(now - powertime,0,ENTITIES,0));
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
     char                     *tmp;
     dbref                    who;
     time_t                   now;

     setreturn(ERROR,COMMAND_FAIL);
     if(player == NOBODY) {
        for(p = descriptor_list; p; p = p->next);
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
        command_type |= NO_USAGE_UPDATE;
        longdate = epoch_to_longdate(now);
        if(!in_command && p && !p->pager && More(player)) pager_init(p);

        /* ---->  Character name and title  <---- */
        output(p,player,0,1,0,"\n%s",(char *) separator(twidth,0,'-','='));
        sprintf(scratch_buffer, " %s%s%s%s", Being(who) ? ANSI_DCYAN "(Being)" : (Controller(who) != who) ? ANSI_DCYAN "(Puppet)" : "", (Being(who) || (Controller(who) != who)) ? "  ":"", colour = privilege_colour(who), getcname(NOTHING, who, 0, UPPER|INDEFINITE));
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
        output(p, player, 2, 1, 1, " %s\n", punctuate(scratch_buffer, 0, '.'));
        output(p,player,0,1,0,(char *) separator(twidth,0,'-','-'));

        /* ---->  Real name, town/city, country, nationality and occupation  <---- */
        if(db[who].data->player.profile->irl || db[who].data->player.profile->city || db[who].data->player.profile->country || db[who].data->player.profile->nationality || db[who].data->player.profile->occupation) {
           if(db[who].data->player.profile->irl)
		output(p, player, 2, 1, 20, ANSI_LCYAN "        Real name:  " ANSI_LWHITE "%s\n", substitute(who, scratch_return_string, (char *) punctuate((char *) bad_language_filter(scratch_return_string, decompress(db[who].data->player.profile->irl)), 2, '.'), 0, ANSI_LWHITE, NULL, 0));
           if(db[who].data->player.profile->city)
		output(p, player, 2, 1, 20, ANSI_LCYAN "        Town/city:  " ANSI_LWHITE "%s\n", substitute(who, scratch_return_string, (char *) punctuate((char *) bad_language_filter(scratch_return_string, decompress(db[who].data->player.profile->city)), 2, '.'), 0, ANSI_LWHITE, NULL, 0));
           if(db[who].data->player.profile->country)
		output(p, player, 2, 1, 20, ANSI_LCYAN "          Country:  " ANSI_LWHITE "%s\n", substitute(who, scratch_return_string, (char *) punctuate((char *) bad_language_filter(scratch_return_string, decompress(db[who].data->player.profile->country)), 2, '.'), 0, ANSI_LWHITE, NULL, 0));
           if(db[who].data->player.profile->nationality)
		output(p, player, 2, 1, 20, ANSI_LCYAN "      Nationality:  " ANSI_LWHITE "%s\n", substitute(who, scratch_return_string, (char *) punctuate((char *) bad_language_filter(scratch_return_string, decompress(db[who].data->player.profile->nationality)), 2, '.'), 0, ANSI_LWHITE, NULL, 0));
           if(db[who].data->player.profile->occupation)
		output(p, player, 2, 1, 20, ANSI_LCYAN "       Occupation:  " ANSI_LWHITE "%s\n", substitute(who, scratch_return_string, (char *) punctuate((char *) bad_language_filter(scratch_return_string, decompress(db[who].data->player.profile->occupation)), 2, '.'), 0, ANSI_LWHITE, NULL, 0));
           output(p,player,0,1,0,(char *) separator(twidth,0,'-','-'));
	}

        /* ---->  Birthday, age, sex, sexuality and status (IVL/IRL)  <---- */
        if(db[who].data->player.profile->dob != UNSET_DATE) {
           output(p, player, 2, 1, 20, ANSI_LRED "         Birthday:  " ANSI_LWHITE "%s.\n", date_to_string(UNSET_DATE, db[who].data->player.profile->dob, player, SHORTDATEFMT));
           output(p, player, 2, 1, 20, ANSI_LRED "              Age:  " ANSI_LWHITE "%d.\n", longdate_difference(db[who].data->player.profile->dob, epoch_to_longdate(now)) / 12);
	}

        output(p, player, 2, 1, 20, ANSI_LRED "           Gender:  " ANSI_LWHITE "%s.\n", genders[Genderof(who)]);

        if(db[who].data->player.profile->sexuality)
           output(p, player, 2, 1, 20, ANSI_LRED "        Sexuality:  " ANSI_LWHITE "%s.\n", sexuality[db[who].data->player.profile->sexuality]);

        if(!((Engaged(who) || Married(who)) && Validchar(Partner(who)))) {
           if(db[who].data->player.profile->statusivl)
              output(p, player, 2, 1, 20, ANSI_LRED "     Status (IVL):  " ANSI_LWHITE "%s.\n", statuses[db[who].data->player.profile->statusivl]);
	} else output(p, player, 2, 1, 20, ANSI_LRED "     Status (IVL):  " ANSI_LWHITE "%s to %s" ANSI_LYELLOW "%s" ANSI_LWHITE ".\n", Engaged(who) ? "Engaged" : "Married", Article(Partner(who), LOWER, INDEFINITE), getcname(NOTHING, Partner(who), 0, 0));

        if(db[who].data->player.profile->statusirl)
           output(p, player, 2, 1, 20, ANSI_LRED "     Status (IRL):  " ANSI_LWHITE "%s.\n", statuses[db[who].data->player.profile->statusirl]);

        output(p,player,0,1,0,(char *) separator(twidth,0,'-','-'));

        /* ---->  Height, weight, hair colour, eye colour, other (Miscellanous) information  <---- */
        if(db[who].data->player.profile->height || db[who].data->player.profile->weight || db[who].data->player.profile->hair || db[who].data->player.profile->eyes || db[who].data->player.profile->other) {
           int           major,minor;
           unsigned char metric;

           if(db[who].data->player.profile->height) {
              metric = ((db[who].data->player.profile->height & 0x8000) != 0);
              major  = ((db[who].data->player.profile->height & 0x7FFF) >> 7);
              minor  = (db[who].data->player.profile->height & 0x007F);
              if(major > 0) sprintf(scratch_return_string,"%d%s",major,(metric) ? "m":"'");
                 else *scratch_return_string = '\0';
              if(minor > 0) sprintf(scratch_return_string + strlen(scratch_return_string),"%s%d%s",(*scratch_return_string) ? " ":"",minor,(metric) ? "cm":"\"");
              output(p, player, 2, 1, 20, ANSI_LYELLOW "           Height:  " ANSI_LWHITE "%s.\n", scratch_return_string);
	   }

           if(db[who].data->player.profile->weight) {
              metric = ((db[who].data->player.profile->weight & 0x80000000) != 0);
              major  = ((db[who].data->player.profile->weight & 0x7FFF0000) >> 16);
              minor  = (db[who].data->player.profile->weight  & 0x0000FFFF);
              if(major > 0) sprintf(scratch_return_string,"%d%s",major,(metric) ? "Kg":"lbs");
                 else *scratch_return_string = '\0';
              if(minor > 0) sprintf(scratch_return_string + strlen(scratch_return_string),"%s%d%s",(*scratch_return_string) ? " ":"",minor,(metric) ? "g":"oz");
              output(p, player, 2, 1, 20, ANSI_LYELLOW "           Weight:  " ANSI_LWHITE "%s.\n", scratch_return_string);
	   }

           if(db[who].data->player.profile->hair)
		output(p, player, 2, 1, 20, ANSI_LYELLOW "      Hair colour:  " ANSI_LWHITE "%s\n", substitute(who, scratch_return_string, (char *) punctuate((char *) bad_language_filter(scratch_return_string, decompress(db[who].data->player.profile->hair)), 2, '.'), 0, ANSI_LWHITE, NULL, 0));
           if(db[who].data->player.profile->eyes)
		output(p, player, 2, 1, 20, ANSI_LYELLOW "       Eye colour:  " ANSI_LWHITE "%s\n", substitute(who, scratch_return_string, (char *) punctuate((char *) bad_language_filter(scratch_return_string, decompress(db[who].data->player.profile->eyes)), 2, '.'), 0, ANSI_LWHITE, NULL, 0));
           if(db[who].data->player.profile->other)
		output(p, player, 2, 1, 20, ANSI_LYELLOW "            Other:  " ANSI_LWHITE "%s\n", substitute(who, scratch_return_string, (char *) punctuate((char *) bad_language_filter(scratch_return_string, decompress(db[who].data->player.profile->other)), 2, '.'), 0, ANSI_LWHITE, NULL, 0));
           output(p,player,0,1,0,(char *) separator(twidth,0,'-','-'));
	}

        /* ---->  Hobbies, interests, achievements, qualifications, comments, likes, dislikes  <---- */
        if(db[who].data->player.profile->hobbies || db[who].data->player.profile->interests || db[who].data->player.profile->achievements || db[who].data->player.profile->qualifications || db[who].data->player.profile->comments) {
           if(db[who].data->player.profile->hobbies)
		output(p, player, 2, 1, 20, ANSI_LMAGENTA "          Hobbies:  " ANSI_LWHITE "%s\n", substitute(who, scratch_return_string, (char *) punctuate((char *) bad_language_filter(scratch_return_string, decompress(db[who].data->player.profile->hobbies)), 2, '.'), 0, ANSI_LWHITE, NULL, 0));
           if(db[who].data->player.profile->interests)
		output(p, player, 2, 1, 20, ANSI_LMAGENTA "        Interests:  " ANSI_LWHITE "%s\n", substitute(who, scratch_return_string, (char *) punctuate((char *) bad_language_filter(scratch_return_string, decompress(db[who].data->player.profile->interests)), 2, '.'), 0, ANSI_LWHITE, NULL, 0));
           if(db[who].data->player.profile->achievements)
		output(p, player, 2, 1, 20, ANSI_LMAGENTA "     Achievements:  " ANSI_LWHITE "%s\n", substitute(who, scratch_return_string, (char *) punctuate((char *) bad_language_filter(scratch_return_string, decompress(db[who].data->player.profile->achievements)), 2, '.'), 0, ANSI_LWHITE, NULL, 0));
           if(db[who].data->player.profile->qualifications)
		output(p, player, 2, 1, 20, ANSI_LMAGENTA "   Qualifications:  " ANSI_LWHITE "%s\n", substitute(who, scratch_return_string, (char *) punctuate((char *) bad_language_filter(scratch_return_string, decompress(db[who].data->player.profile->qualifications)), 2, '.'), 0, ANSI_LWHITE, NULL, 0));
           if(db[who].data->player.profile->comments)
		output(p, player, 2, 1, 20, ANSI_LMAGENTA "         Comments:  " ANSI_LWHITE "%s\n", substitute(who, scratch_return_string, (char *) punctuate((char *) bad_language_filter(scratch_return_string, decompress(db[who].data->player.profile->comments)), 2, '.'), 0, ANSI_LWHITE, NULL, 0));
           output(p,player,0,1,0,(char *) separator(twidth,0,'-','-'));
	}

        /* ---->  Favourite foods, drinks, music and sports  <---- */
        if(db[who].data->player.profile->food || db[who].data->player.profile->drink || db[who].data->player.profile->music || db[who].data->player.profile->sport  || db[who].data->player.profile->likes || db[who].data->player.profile->dislikes) {
           if(db[who].data->player.profile->food)
		output(p, player, 2, 1, 20, ANSI_LBLUE "  Favourite foods:  " ANSI_LWHITE "%s\n", substitute(who, scratch_return_string, (char *) punctuate((char *) bad_language_filter(scratch_return_string, decompress(db[who].data->player.profile->food)), 2, '.'), 0, ANSI_LWHITE, NULL, 0));
           if(db[who].data->player.profile->drink)
		output(p, player, 2, 1, 20, ANSI_LBLUE " Favourite drinks:  " ANSI_LWHITE "%s\n", substitute(who, scratch_return_string, (char *) punctuate((char *) bad_language_filter(scratch_return_string, decompress(db[who].data->player.profile->drink)), 2, '.'), 0, ANSI_LWHITE, NULL, 0));
           if(db[who].data->player.profile->music)
		output(p, player, 2, 1, 20, ANSI_LBLUE "  Favourite music:  " ANSI_LWHITE "%s\n", substitute(who, scratch_return_string, (char *) punctuate((char *) bad_language_filter(scratch_return_string, decompress(db[who].data->player.profile->music)), 2, '.'), 0, ANSI_LWHITE, NULL, 0));
           if(db[who].data->player.profile->sport)
		output(p, player, 2, 1, 20, ANSI_LBLUE " Favourite sports:  " ANSI_LWHITE "%s\n", substitute(who, scratch_return_string, (char *) punctuate((char *) bad_language_filter(scratch_return_string, decompress(db[who].data->player.profile->sport)), 2, '.'), 0, ANSI_LWHITE, NULL, 0));
           if(db[who].data->player.profile->likes)
		output(p, player, 2, 1, 20, ANSI_LBLUE "            Likes:  " ANSI_LWHITE "%s\n", substitute(who, scratch_return_string, (char *) punctuate((char *) bad_language_filter(scratch_return_string, decompress(db[who].data->player.profile->likes)), 2, '.'), 0, ANSI_LWHITE, NULL, 0));
           if(db[who].data->player.profile->dislikes)
		output(p, player, 2, 1, 20, ANSI_LBLUE "         Dislikes:  " ANSI_LWHITE "%s\n", substitute(who, scratch_return_string, (char *) punctuate((char *) bad_language_filter(scratch_return_string, decompress(db[who].data->player.profile->dislikes)), 2, '.'), 0, ANSI_LWHITE, NULL, 0));
           output(p,player,0,1,0,(char *) separator(twidth,0,'-','-'));
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
           if((ptr = getfield(who,WWW)))
              output(p, player, 2, 1, 22, ANSI_LGREEN "           Web site:  " ANSI_LWHITE "%s%s\n", !strncasecmp(ptr, "http://", 7) ? "" : "http://", ptr);

           if(db[who].data->player.profile->picture) {
              ptr = decompress(db[who].data->player.profile->picture);
              output(p, player, 2, 1, 22, ANSI_LGREEN "      Picture (URL):  " ANSI_LWHITE "%s\n", ptr);
	   }

           if(getfield(who,EMAIL)) {
              const    char *email = getfield(who,EMAIL);
              int           counter = 1;
              const    char *emailaddr;
              unsigned char header = 0;

              sprintf(scratch_buffer, ANSI_LGREEN " E-mail address(es):  ");
              if((emailaddr = forwarding_address(who,1,scratch_return_string))) {
		 if(!header) output(p,player,1,2,0,scratch_buffer);
                 output(p, player, 2, 1, 22, "%s" ANSI_LWHITE "%s  " ANSI_LGREEN "(Public)" ANSI_LWHITE "\n", !header ? scratch_buffer : "", emailaddr);
                 header = 1;
	      }

              for(; counter <= EMAIL_ADDRESSES; counter++)
                  if(((counter != 2) || (Validchar(player) && (Level4(db[player].owner) || can_write_to(player,who,1)))) && (emailaddr = gettextfield(counter,'\n',email,0,scratch_return_string)) && *emailaddr && strcasecmp("forward",emailaddr)) {
                     if(!header) output(p,player,1,2,0,scratch_buffer);
                     output(p, player, 2, 1, 22, "%s" ANSI_LWHITE "%s  %s" ANSI_LWHITE "\n", (header) ? "                      " : "", emailaddr, (counter == 2) ? ANSI_DRED "(" ANSI_LRED "Private" ANSI_DRED ")" : ANSI_DGREEN "(" ANSI_LGREEN "Public" ANSI_DGREEN ")");
                     header = 1;
		  }
	   }
           output(p,player,0,1,0,(char *) separator(twidth,0,'-','-'));
	}

        if(Validchar(player))
		output(p, player, 2, 1, 1, ANSI_LWHITE " For other information about %s" ANSI_LYELLOW "%s" ANSI_LWHITE ", type '" ANSI_LGREEN "scan%s%s" ANSI_LWHITE "'.\n", Article(who, LOWER, DEFINITE), getcname(NOTHING, who, 0, 0), Blank(arg1) ? "" : " ", arg1);
        output(p,player,0,1,0,(char *) separator(twidth,1,'-','='));
        command_type &= ~NO_USAGE_UPDATE;

        if(!secret && Validchar(player))
           command_execute_action(who,NOTHING,".profile",NULL,getname(player),"",getname(player),0);
        setreturn(OK,COMMAND_SUCC);
     } else if(who == player) output(p,player,0,1,0,ANSI_LGREEN"Sorry, you haven't set any information on your profile yet (See '"ANSI_LWHITE"help @profile"ANSI_LGREEN"' for details on setting your profile.)");
        else output(p,player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" doesn't have a profile.",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0));
}

/* ---->  Display detailed information about specified character  <---- */
/*        (VAL1:  0 = Brief (Normal) scan, 1 = Full scan.)              */
void look_scan(CONTEXT)
{
     unsigned char            twidth = output_terminal_width(player),secret = 0;
     time_t                   now,last,total,idle,active;
     struct   descriptor_data *w,*d = getdsc(player);
     double                   credit,balance;
     dbref                    area,who;
     unsigned long            longdate;
     const    char            *colour;
     int                      items;
     char                     *ptr;

     setreturn(ERROR,COMMAND_FAIL);
     if(player == NOBODY) {
        for(d = descriptor_list; d; d = d->next);
        player = NOTHING;
        if(!d) return;
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
     command_type |= NO_USAGE_UPDATE;
     longdate = epoch_to_longdate(now);
     if(!in_command && d && !d->pager && More(player)) pager_init(d);
     w = getdsc(who);

     /* ---->  Character name and title  <---- */
     output(d,player,0,1,0,"\n%s",(char *) separator(twidth,0,'-','='));
     sprintf(scratch_buffer, " %s%s", colour = privilege_colour(who), getcname(NOTHING, who, 0, UPPER|INDEFINITE));
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
     output(d, player, 2, 1, 1, " %s\n", punctuate(scratch_buffer, 0, '.'));
     output(d,player,0,1,0,(char *) separator(twidth,0,'-','-'));

     /* ---->  Description  <---- */
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
           output(d, player, 2, 1, 1, ANSI_LGREEN "\n %s" ANSI_LWHITE "%s" ANSI_LGREEN " is feeling %s.\n", Article(who, UPPER, DEFINITE), getcname(NOTHING, who, 0, 0), scratch_return_string);
	} else db[who].data->player.feeling = 0;
     }

     /* ---->  Puppet/Being?  <---- */
     if(Puppet(who)) {
        sprintf(scratch_buffer, ANSI_LGREEN "\n %s" ANSI_LWHITE "%s" ANSI_LGREEN " is a %spuppet of ", Article(who, UPPER, DEFINITE), getcname(NOTHING, who, 0, 0), Being(who) ? "Being " : "");
        output(d, player, 2, 1, 1, "%s%s" ANSI_LWHITE "%s" ANSI_LGREEN ".\n", scratch_buffer, Article(Controller(who), LOWER, INDEFINITE), getcname(NOTHING, Controller(who), 0, 0));
     } else if(Being(who)) output(d, player, 2, 1, 1, ANSI_LGREEN "\n %s" ANSI_LWHITE "%s" ANSI_LGREEN " is a Being.\n", Article(who, UPPER, DEFINITE), getcname(NOTHING, who, 0, 0));

     if((Engaged(who) || Married(who)) && Validchar(Partner(who))) {
        sprintf(scratch_buffer, ANSI_LGREEN "\n %s" ANSI_LWHITE "%s" ANSI_LGREEN " is %s to ", Article(who, UPPER, DEFINITE), getcname(NOTHING, who, 0, 0), Married(who) ? "married" : "engaged");
        sprintf(scratch_buffer + strlen(scratch_buffer),"%s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(Partner(who),UPPER,DEFINITE),getcname(NOTHING,Partner(who),0,0));
        output(d,player,0,1,1,"%s",scratch_buffer);
     }

     output(d,player,0,1,0,(char *) separator(twidth,0,'-','-'));

     /* ---->  Longest connect time  <---- */
     items = 0;
     if(Connected(who)) {
        if((total = (now - db[who].data->player.lasttime)) == now) total = 0;
        if(db[who].data->player.longesttime < total)
           db[who].data->player.longesttime = total;
     }
     if(db[who].data->player.longesttime > 0) {
        output(d, player, 2, 1, 26, ANSI_LCYAN " Longest connect time:  " ANSI_LWHITE "%s.\n" ANSI_DCYAN "(" ANSI_LYELLOW "%s" ANSI_DCYAN ".)\n", interval(db[who].data->player.longesttime, db[who].data->player.longesttime, ENTITIES, 0), date_to_string(TimeDiff(db[who].data->player.longestdate, player), UNSET_DATE, player, FULLDATEFMT));
        items++;
     }

     /* ---->  Total time  <---- */
     total = db[who].data->player.totaltime;
     if(Connected(who)) total += (now - db[who].data->player.lasttime);
     if(total > 0) {
        strcpy(scratch_return_string,interval(total / ((db[who].data->player.logins > 1) ? db[who].data->player.logins:1),0,ENTITIES,0));
        output(d, player, 2, 1, 26, ANSI_LCYAN " Total time connected:  " ANSI_LWHITE "%s.\n" ANSI_DCYAN "(" ANSI_LCYAN "Average " ANSI_LYELLOW "%s" ANSI_DCYAN ".)\n", interval(total, total, ENTITIES, 0), scratch_return_string);
        items++;
     }

     /* ---->  Time spent idle (Full scan only)  <---- */
     if(val1) {
        idle = db[who].data->player.idletime;
        if(Connected(who) && w) idle += (now - w->last_time);
	if(idle > 0) {
	   strcpy(scratch_return_string,interval(idle / ((db[who].data->player.logins > 1) ? db[who].data->player.logins:1),0,ENTITIES,0));
	   output(d, player, 2, 1, 26, ANSI_LRED "    Time spent idling:  " ANSI_LWHITE "%s.\n" ANSI_DCYAN "(" ANSI_LCYAN "Average " ANSI_LYELLOW "%s" ANSI_DCYAN ".)\n", interval(idle, idle, ENTITIES, 0), scratch_return_string);
	   items++;
	}
     }

     /* ---->  Time spent active (Full scan only)  <---- */
     if(val1) {
        active = total - idle;
	if(active > 0) {
	   strcpy(scratch_return_string,interval(active  / ((db[who].data->player.logins > 1) ? db[who].data->player.logins:1),0,ENTITIES,0));
	   output(d, player, 2, 1, 26, ANSI_LGREEN "    Time spent active:  " ANSI_LWHITE "%s.\n" ANSI_DCYAN "(" ANSI_LCYAN "Average " ANSI_LYELLOW "%s" ANSI_DCYAN ".)\n", interval(active, active, ENTITIES, 0),scratch_return_string);
	   items++;
	}
     }

     /* ---->  Section separator (Full scan only)  <---- */
     if(val1 && (items > 0))
        output(d,player,0,1,0,(char *) separator(twidth,0,'-','-'));

     /* ---->  Last time connected  <---- */
     last = db[who].data->player.lasttime;
     if(last > 0) {
        total = now - last;
        if((last > 0) && Validchar(player)) last += (db[player].data->player.timediff * HOUR);
        sprintf(scratch_buffer, ANSI_LCYAN "  Last time connected:  " ANSI_LWHITE "%s", (last == 0) ? "Unknown" : date_to_string(last, UNSET_DATE, player, FULLDATEFMT));
        if(total > 0) sprintf(scratch_buffer + strlen(scratch_buffer), "%s\n" ANSI_DCYAN "(" ANSI_LYELLOW "%s " ANSI_LCYAN "ago" ANSI_DCYAN ".)", Connected(who) ? " " ANSI_DCYAN "(" ANSI_LCYAN "Still connected." ANSI_DCYAN ")" : ".", interval(total, total, ENTITIES, 0));
           else sprintf(scratch_buffer + strlen(scratch_buffer), "%s", Connected(who) ? " " ANSI_DCYAN "(" ANSI_LCYAN "Still connected." ANSI_DCYAN ")" : ".");
        output(d,player,0,1,26,"%s",scratch_buffer);
     }

     /* ---->  Site character last connected from  <---- */
     if(Validchar(player) && ((db[player].owner == who) || Level4(db[player].owner)) && !Blank(getfield(who,LASTSITE)))
        output(d, player, 2, 1, 24, ANSI_LCYAN "  Last connected from:  " ANSI_LWHITE "%s%s\n", getfield(who, LASTSITE), (player == who) ? "  " ANSI_DRED "(" ANSI_LRED "Private" ANSI_DRED ")" : "");

     /* ---->  Total logins (Full scan only)  <---- */
     if(val1) {
        total = ((now - db[who].created) / ((db[who].data->player.logins > 1) ? db[who].data->player.logins:1));
        output(d, player, 2, 1, 26, ANSI_LCYAN "         Total logins:  " ANSI_LWHITE "%d " ANSI_DCYAN "(" ANSI_LCYAN "Average " ANSI_LYELLOW "%s" ANSI_DCYAN ".)\n", db[who].data->player.logins, interval(ABS((total < MINUTE) ? MINUTE : total), 0, ENTITIES, 0));
     }

     /* ---->  Date created (Full scan only)  <---- */
     if(val1) output(d, player, 2, 1, 26, ANSI_LMAGENTA "        Creation date:  " ANSI_LWHITE "%s.\n" ANSI_DCYAN "(" ANSI_LYELLOW "%s " ANSI_LCYAN "ago" ANSI_DCYAN ".)\n", date_to_string(TimeDiff(db[who].created, player), UNSET_DATE, player, FULLDATEFMT), interval(ABS(now - db[who].created), 0, ENTITIES, 0));

     /* ---->  Section separator  <---- */
     output(d,player,0,1,0,(char *) separator(twidth,0,'-','-'));

     /* ---->  Race  <---- */
     if(!Blank(getfield(who,RACE))) {
        bad_language_filter(scratch_buffer,scratch_buffer);
        output(d, player, 2, 1, 10, ANSI_LRED "   Race:  " ANSI_LWHITE "%s\n", punctuate((char *) substitute(who, scratch_return_string, (char *) getfield(who, RACE), 0, ANSI_LWHITE, NULL, 0), 2, '.'));
     }

     /* ---->  Flags  <---- */
     output(d, player, 2, 1, 10, ANSI_LGREEN "  Flags:  " ANSI_LWHITE "%s\n", unparse_flaglist(who, ((player > 0) && (Level4(db[player].owner) || can_read_from(player, who))), scratch_return_string));

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
     sprintf(scratch_buffer, ANSI_LYELLOW " Gender:  " ANSI_LWHITE "%s" ANSI_LYELLOW ", Health:  " ANSI_LWHITE "%s" ANSI_LYELLOW ", Score:  " ANSI_LWHITE "%d" ANSI_LYELLOW ", Credit:  " ANSI_LWHITE "%.2f" ANSI_LYELLOW ", Bank balance:  %s%.2f" ANSI_LYELLOW ", Building Quota:  ", scratch_return_string, combat_percent(currency_to_double(&(db[who].data->player.health)), 100), db[who].data->player.score, credit, (balance < 0) ? ANSI_LRED : ANSI_LWHITE, balance);
     if(!Level4(who)) sprintf(scratch_buffer + strlen(scratch_buffer),"%s%d/%ld"ANSI_LYELLOW".",(db[who].data->player.quota > db[who].data->player.quotalimit) ? ANSI_LRED:ANSI_LWHITE,db[who].data->player.quota,db[who].data->player.quotalimit);
        else sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LWHITE"%d/UNLIMITED"ANSI_LYELLOW".",db[who].data->player.quota);
     output(d,player,2,1,10,"%s\n",scratch_buffer);

     output(d,player,0,1,0,(char *) separator(twidth,0,'-','-'));

     /* ---->  Web Site  <---- */
     if((ptr = (char *) getfield(who,WWW)))
        output(d, player, 2, 1, 22, ANSI_LGREEN "           Web site:  " ANSI_LWHITE "%s%s\n", !strncasecmp(ptr, "http://",7) ? "" : "http://", ptr);

     /* ---->  E-mail address  <---- */
     if(getfield(who,EMAIL)) {
        const    char *email = getfield(who,EMAIL);
        int           counter = 1;
        const    char *emailaddr;
        unsigned char header = 0;

        sprintf(scratch_buffer, ANSI_LGREEN " E-mail address(es):  ");
        if((emailaddr = forwarding_address(who,1,scratch_return_string))) {
           output(d, player, 2, 1, 22, "%s" ANSI_LWHITE "%s  " ANSI_LGREEN "(Public)" ANSI_LWHITE "\n", !header ? scratch_buffer : "", emailaddr);
           header = 1;
	}

        for(; counter <= EMAIL_ADDRESSES; counter++)
            if(((counter != 2) || (Validchar(player) && (Level4(db[player].owner) || can_write_to(player,who,1)))) && (emailaddr = gettextfield(counter,'\n',email,0,scratch_return_string)) && *emailaddr && strcasecmp("forward",emailaddr)) {
               output(d, player, 2, 1, 20, "%s" ANSI_LWHITE "%s  %s" ANSI_LWHITE "\n", header ? "                      " : scratch_buffer, emailaddr, (counter == 2) ? ANSI_DRED "(" ANSI_LRED "Private" ANSI_DRED ")" : ANSI_DGREEN "(" ANSI_LGREEN "Public" ANSI_DGREEN ")");
               header = 1;
	    }
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
     output(d, player, 2, 1, 22, ANSI_LCYAN "   Current location:  " ANSI_LWHITE "%s.\n", scratch_return_string);

     if(Validchar(player) && hasprofile(db[who].data->player.profile)) {
        output(d,player,0,1,0,(char *) separator(twidth,0,'-','-'));
        output(d, player, 2, 1, 1, ANSI_LYELLOW " %s%s" ANSI_LWHITE " also has a profile about %s  -  Type '" ANSI_LGREEN "profile%s%s" ANSI_LWHITE "' to view it.\n", Article(who, UPPER, DEFINITE), getcname(NOTHING, who, 0, 0), Reflexive(who, 0), Blank(arg1) ? "" : " ", arg1);
     }

     output(d,player,0,1,0,(char *) separator(twidth,1,'-','='));
     command_type &= ~NO_USAGE_UPDATE;

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
     const  char *title;

     if((title = help_get_titlescreen(0))) {
#ifdef PAGE_TITLESCREENS
        if(!in_command && p && !p->pager && More(player)) pager_init(p);
#endif
        output(getdsc(player),player,0,1,0,"\n"ANSI_DWHITE"%s",decompress(title));
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
        p->player = NOTHING;
     }

     tcz_version(p,0);
     setreturn(OK,COMMAND_SUCC);
}
