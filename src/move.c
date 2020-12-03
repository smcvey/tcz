/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| MOVE.C  -  Implements moving objects/characters from one location to        |
|            another.                                                         |
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


#include <stdlib.h>
#include <string.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "friend_flags.h"
#include "object_types.h"
#include "objectlists.h"
#include "flagset.h"
#include "fields.h"
#include "search.h"
#include "match.h"


/* ---->  Move an object from one contents list to another  <---- */
unsigned char move_to(dbref source,dbref destination)
{
	 dbref oldloc;

	 if(!Valid(source) || (!Valid(destination) && (destination != HOME))) return(0);
	 oldloc = Location(source);

	 /* ---->  If moving room into itself, set to NOTHING (-1)  <---- */
	 if((Typeof(source) == TYPE_ROOM) && (source == destination)) {
	    if(Valid(oldloc)) db[oldloc].contents = remove_first(db[oldloc].contents,source);
	    db[source].location = NOTHING;
	    db[source].next     = NOTHING;
	    return(1);
	 }

	 /* ---->  Check restrictions  <---- */
	 if(contains(destination,source) || (source == destination)) {
	    if(destination == HOME) {

	       /* ---->  Move object to Room Zero(#0)  <---- */
	       if(!RoomZero(oldloc)) gettime(db[source].lastused);
	       if(Valid(oldloc)) db[oldloc].contents = remove_first(db[oldloc].contents,source);
	       db[source].location = ROOMZERO;
	       PUSH(source,db[ROOMZERO].contents);
	       return(1);
	    } else return(0);
	 }

	 switch(Typeof(source)) {
		case TYPE_CHARACTER:
		case TYPE_THING:
		case TYPE_ROOM:
		     if(!Valid(destination)) return(0);
		     if(Valid(oldloc)) db[oldloc].contents = remove_first(db[oldloc].contents,source);
		     db[source].location = destination;
		     PUSH(source,db[destination].contents);
		     break;
		case TYPE_COMMAND:
		     if(destination == HOME) return(0);
		     if(Global(effective_location(source))) global_delete(source);
		     if(Valid(oldloc)) db[oldloc].commands = remove_first(db[oldloc].commands,source);
		     db[source].location = destination;
		     PUSH(source,db[destination].commands);
		     if(Global(effective_location(source))) global_add(source);
		     break;
		case TYPE_PROPERTY:
		case TYPE_VARIABLE:
		case TYPE_ARRAY:
		     if(destination == HOME) return(0);
		     if(Valid(oldloc)) db[oldloc].variables = remove_first(db[oldloc].variables,source);
		     db[source].location = destination;
		     PUSH(source,db[destination].variables);
		     break;
		case TYPE_FUSE:
		case TYPE_ALARM:
		     if(destination == HOME) return(0);
		     if(Valid(oldloc)) db[oldloc].fuses = remove_first(db[oldloc].fuses,source);
		     db[source].location = destination;
		     PUSH(source,db[destination].fuses);
		     break;
		case TYPE_EXIT:
		     if(destination == HOME) return(0);
		     if(Valid(oldloc)) db[oldloc].exits = remove_first(db[oldloc].exits,source);
		     db[source].location = destination;
		     PUSH(source,db[destination].exits);
		     break;
		default:
		     writelog(BUG_LOG,1,"BUG","(move_to() in move.c)  Type of object %s(#%d) is unknown (0x%0X.)",getname(source),source,Typeof(source));
		     return(0);
	 }

	 /* ---->  Set expiry if object is now located in #0 (ROOMZERO)  <---- */
	 if(RoomZero(Location(source)) && (oldloc != Location(source)))
	    gettime(db[source].lastused);
	 return(1);
}

/* ---->  Send an object home  <---- */
void move_home(dbref object,int homeroom)
{
     int destination;

     switch(Typeof(object)) {
            case TYPE_CHARACTER:

                 /* ---->  Check owner of character's home hasn't reset character's 'Link' friend flag  <---- */
                 destination = (!homeroom) ? Destination(object):search_room_by_owner(object,homerooms,0);
                 if(!Valid(destination) || ((Owner(object) != Owner(destination)) && !friendflags_check(object,object,Owner(destination),FRIEND_LINK,"Home"))) return;

                 /* ---->  If character can't fit in their home, send them to Room Zero (#0)  <---- */
                 if(will_fit(object,destination)) {
                    dbref oldloc = Location(object);

                    /* ---->  Attempt move  <---- */
                    move_enter(object,destination,1);

                    if((oldloc != Location(object)) && !Invisible(Location(object))) {
                       char *cmd_arg0,*cmd_arg1,*cmd_arg2,*cmd_arg3;
                       char buffer[TEXT_SIZE],token[2];

                       /* ---->  Send character's possessions to their respective home locations  <---- */
#ifndef KEEP_POSSESSIONS
                       move_contents(object,HOME,NOTHING);
#endif

                       /* ---->  Arrival message and .entercmd  <---- */
                       output_except(Location(object),object,NOTHING,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" %s arrived.",Article(object,UPPER,INDEFINITE),getcname(NOTHING,object,0,0),(Articleof(object) == ARTICLE_PLURAL) ? "have":"has");
                       event_set_fuse_args((in_command && command_lineptr) ? command_lineptr:command_line,&cmd_arg0,&cmd_arg1,&cmd_arg2,&cmd_arg3,buffer,token,0);
                       command_execute_action(object,NOTHING,".entercmd",NULL,cmd_arg1,cmd_arg2,cmd_arg3,0);
		    }
		 } else {
                    output(getdsc(object),object,0,1,0,ANSI_LGREEN"A space-time distortion (Caused by trying to fit too many things into too little space) causes you to land in %s"ANSI_LWHITE"%s"ANSI_LGREEN".\n",Article(object,LOWER,DEFINITE),unparse_object(object,ROOMZERO,0));
                    move_enter(object,ROOMZERO,1);
		 }
                 break;
            case TYPE_THING:
                 if(Valid(Destination(object)) && will_fit(object,Destination(object)))
                     move_to(object,Destination(object));
                        else move_to(object,ROOMZERO); 
                 break;
            default:
                 break;
     }
}

/* ---->  Send contents of object LOC to DEST  <---- */
void move_contents(dbref loc,dbref dest,dbref override)
{
     dbref ptr,next;

     ptr = db[loc].contents;
     while(Valid(ptr)) {
	   next = Next(ptr);
	   if(((Typeof(ptr) == TYPE_THING) || (Typeof(ptr) == TYPE_CHARACTER)) && (ptr != dest) && (db[ptr].location == loc) && !((db[ptr].destination == override) && Sticky(ptr))) {
	      if(Sticky(ptr)) move_home(ptr,0);
		 else move_to(ptr,dest);
	   }
	   ptr = next;
     }
}

/* ---->  Delayed drop-to (When last connected character leaves the room)  <---- */
void move_dropto_delay(dbref object,dbref dropto)
{
     dbref ptr;

     /* ---->  Check for connected characters  <---- */
     if(object == dropto) return;

     for(ptr = db[object].contents; Valid(ptr); ptr = Next(ptr))
         if((Typeof(ptr) == TYPE_CHARACTER) && Connected(ptr)) return;

     /* ---->  No connected characters  -  Send everything to the drop-to  <---- */
     move_contents(object,dropto,object);
}

/* ---->  Enter specified location  <---- */
unsigned char move_enter(dbref player,dbref destination,unsigned char autolook)
{
	 dbref                    cached_owner,cached_chpid,old,dropto,command,overlap_room;
	 unsigned char            overlapped,number,abort = 0;
	 struct   descriptor_data *p;

	 /* ---->  Home room?  <---- */
	 if(destination == HOME)
	    destination = Destination(player);
	 old = Location(player);

	 /* ---->  Trying to enter object currently being carried?  <---- */
	 if(contains(destination,player)) {
	    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't enter a room/container that you're currently carrying  -  Try teleporting it elsewhere first.");
	    return(0);
	 }

	 /* ---->  Lock of OPENABLE room not satisfied (Display @fail/@ofail messages)?  <---- */
	 if((Typeof(destination) != TYPE_THING) && Openable(destination) && !could_satisfy_lock(player,destination,0)) {
	    sprintf(scratch_return_string,"Sorry, you cannot enter %s"ANSI_LYELLOW"%s"ANSI_LGREEN" (You do not satisfy the lock specification of the room.)",Article(destination,LOWER,INDEFINITE),unparse_object(player,destination,0));
	    can_satisfy_lock(player,destination,scratch_return_string,0);
	    return(0);
	 }

	 if(!in_command && (p = getdsc(player)) && !p->pager && !IsHtml(p) && More(player)) pager_init(p);
	 if(destination != old) {

	    /* ---->  Destination room is different from current room  <---- */
	    abort |= event_trigger_fuses(player,player,NULL,FUSE_TOM|FUSE_ARGS|FUSE_COMMAND);
	    abort |= event_trigger_fuses(player,Location(player),NULL,FUSE_TOM|FUSE_ARGS|FUSE_COMMAND);
	    if(abort) return(0);

	    if(Valid(old)) {
   
	       /* ---->  Notify others unless room is set INVISIBLE  <---- */
	       if(!Invisible(old)) {
		  char *cmd_arg0,*cmd_arg1,*cmd_arg2,*cmd_arg3;
		  char buffer[TEXT_SIZE],token[2];

		  event_set_fuse_args((in_command && command_lineptr) ? command_lineptr:command_line,&cmd_arg0,&cmd_arg1,&cmd_arg2,&cmd_arg3,buffer,token,0);
		  command_execute_action(player,NOTHING,".leavecmd",NULL,cmd_arg1,cmd_arg2,cmd_arg3,0);
		  output_except(old,player,NOTHING,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" %s left.",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0),(Articleof(player) == ARTICLE_PLURAL) ? "have":"has");
	       }
	    }

	    if(in_area(destination,old)) {

	       /* ---->  Move down area  <---- */
	       move_to(player,destination);
	       command = match_object(player,destination,old,".enter",MATCH_PHASE_AREA,MATCH_PHASE_AREA,SEARCH_COMMAND,MATCH_OPTION_DEFAULT_AREA_COMMAND,SEARCH_ALL,NULL,0);
	       if(Valid(command) && Executable(command)) {
		  number              = Number(player);
		  cached_owner        = db[player].owner;
		  cached_chpid        = db[player].data->player.chpid;
		  command_type       |= AREA_CMD;
		  if((player != db[command].owner) && !Level4(player)) db[player].flags |= NUMBER;
		  if(!(Wizard(command) || Level4(db[command].owner))) db[player].owner = db[player].data->player.chpid = db[command].owner;
		  command_arg0 = ".enter";
		  command_cache_execute(player,command,1,1);
		  command_type                 &= ~AREA_CMD;
		  if(!number) db[player].flags &= ~NUMBER;
		  db[player].owner              =  cached_owner;
		  db[player].data->player.chpid =  cached_chpid;
	       }
	       match_done();
	    } else {

	       /* ---->  Move up area  <---- */
	       overlapped   = 0;
	       overlap_room = destination;
	       while(Valid(overlap_room) && !overlapped) {
		     if(in_area(old,overlap_room)) overlapped = 1;
			else overlap_room = Location(overlap_room);
	       }

	       command = match_object(player,old,overlap_room,".leave",MATCH_PHASE_AREA,MATCH_PHASE_AREA,SEARCH_COMMAND,MATCH_OPTION_DEFAULT_AREA_COMMAND,SEARCH_ALL,NULL,0);
	       if(Valid(command) && Executable(command)) {
		  number              = Number(player);
		  cached_owner        = db[player].owner;
		  cached_chpid        = db[player].data->player.chpid;
		  command_type       |= AREA_CMD;
		  if((player != db[command].owner) && !Level4(player)) db[player].flags |= NUMBER;
		  if(!(Wizard(command) || Level4(db[command].owner))) db[player].owner = db[player].data->player.chpid = db[command].owner;
		  command_arg0 = ".leave";
		  command_cache_execute(player,command,1,1);
		  command_type                 &= ~AREA_CMD;
		  if(!number) db[player].flags &= ~NUMBER;
		  db[player].owner              =  cached_owner;
		  db[player].data->player.chpid =  cached_chpid;
	       }
	       match_done();

	       move_to(player,destination);
	       command = match_object(player,destination,overlap_room,".enter",MATCH_PHASE_AREA,MATCH_PHASE_AREA,SEARCH_COMMAND,MATCH_OPTION_DEFAULT_AREA_COMMAND,SEARCH_ALL,NULL,0);
	       if(Valid(command) && Executable(command)) {
		  number              = Number(player);
		  cached_owner        = db[player].owner;
		  cached_chpid        = db[player].data->player.chpid;
		  command_type       |= AREA_CMD;
		  if((player != db[command].owner) && !Level4(player)) db[player].flags |= NUMBER;
		  if(!(Wizard(command) || Level4(db[command].owner))) db[player].owner = db[player].data->player.chpid = db[command].owner;
		  command_arg0 = ".enter";
		  command_cache_execute(player,command,1,1);
		  command_type                 &= ~AREA_CMD;
		  if(!number) db[player].flags &= ~NUMBER;
		  db[player].owner              =  cached_owner;
		  db[player].data->player.chpid =  cached_chpid;
	       }
	       match_done();
	    }

	    /* ---->  If old location has STICKY drop-to, send objects to it  <---- */
	    if(Valid(old) && (Typeof(old) == TYPE_ROOM) && Valid(dropto = Destination(old)) && Sticky(old))
	       move_dropto_delay(old,dropto);
	 }
       
	 /* ---->  Auto-look  <---- */
	 if(Valid(Location(player)) && autolook) {
	    cached_owner     = Owner(player);
	    db[player].owner = player;
	    look_room(player,Location(player));
	    db[player].owner = cached_owner;    
	 }

	 /* ---->  Lock of OPENABLE room satisfied (Display @succ/@osucc messages?  <---- */
	 if((Typeof(destination) != TYPE_THING) && Openable(destination))
	    can_satisfy_lock(player,destination,"Sorry, you cannot enter that location.",1);
	 return(1);
}

/* ---->  Send objects set SENDHOME back to their home locations (At server start-up)  <---- */
void move_sendhome()
{
     int   count = 0;
     dbref i;

     for(i = 0; i < db_top; i++)
         if((Typeof(i) != TYPE_CHARACTER) && Sendhome(i) && Valid(Destination(i)))
            if(move_to(i,Destination(i))) count++;
     if(count > 0) writelog(SERVER_LOG,0,"RESTART","%d object%s with their SENDHOME flag set sent to their home location.",count,Plural(count));
}

/* ---->        Move character through exit or to their home room         <---- */
/*        (val1:  0 = Normal, 1 = No error messages.)                           */
/*        (val2:  0 = Normal, 1 = Send user home, 2 = Send user to home room.)  */
unsigned char move_character(CONTEXT)
{
	 dbref exit,room;

	 setreturn(ERROR,COMMAND_FAIL);
	 if(val2 || !strcasecmp(params,"home")) {
	    if(!val2 || Blank(params) || (!strcasecmp(params,"room") && (val2 = 2))) {
	       if((val2 == 2) || (Valid(homerooms) && (Location(player) == Destination(player)))) {

		  /* ---->  Go to home room  <---- */
#ifdef HOME_ROOMS
		  if(Valid(homerooms) && ((room = search_room_by_owner(player,homerooms,0)) != NOTHING) && (Typeof(room) == TYPE_ROOM)) {
		     if(Location(player) != room) {
			if(!Quiet(Location(player)) && !Invisible(Location(player)))
			   output_except(Location(player),player,NOTHING,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" returns to %s home room.",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0),Possessive(player,0));
			output(getdsc(player),player,0,1,0,HOME_MESSAGE);
			move_home(player,1);
			setreturn(OK,COMMAND_SUCC);
		     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you are already in your home room.");
		  } else if(val2 == 2) {
		     if(Valid(homerooms)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you do not currently have a home room.  Please create one by typing '"ANSI_LYELLOW"@homeroom"ANSI_LGREEN"'.");
			else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you cannot go to your home room.  This feature has been disabled by the administrators.");
		  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you are already in your home location.");
#else
		  if(val2 == 2) {
		     output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you do not have a home room (Feature disabled.)");
		  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you are already in your home location.");
#endif
	       } else if(Location(player) != Destination(player)) {

		  /* ---->  Go to home location  <---- */
		  if(!Quiet(Location(player)) && !Invisible(Location(player)))
		     output_except(Location(player),player,NOTHING,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" returns to %s home location.",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0),Possessive(player,0));
		  output(getdsc(player),player,0,1,0,HOME_MESSAGE);
		  move_home(player,0);
		  setreturn(OK,COMMAND_SUCC);
	       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you are already in your home location.");
	    } else if(!strcasecmp("home",params) || !strcasecmp("homeroom",params)) {

	       /* ---->  Set home to home room  <---- */
#ifdef HOME_ROOMS
	       if(Valid(homerooms) && ((room = search_room_by_owner(player,homerooms,0)) != NOTHING) && (Typeof(room) == TYPE_ROOM)) {
		  db[player].destination = room;
		  if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Your home is now %s"ANSI_LYELLOW"%s"ANSI_LGREEN".",Article(room,LOWER,DEFINITE),unparse_object(player,room,0));
	       } else {
		  if(Valid(homerooms)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you do not currently have a home room.  Please create one by typing '"ANSI_LYELLOW"@homeroom"ANSI_LGREEN"'.");
		     else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you cannot set your home to your home room.  This feature has been disabled by the administrators.");
	       }
#else
	       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you do not have a home room (Feature disabled.)");
#endif
	    } else set_link(player,NULL,NULL,"me",params,0,0);
	    return(1);
	 } else {

	    /* ---->  Find the exit  <---- */
	    exit = match_simple(Location(player),params,EXITS,0,1);
	    if(val1 && !Valid(exit)) return(0);

	    if(exit != NOTHING) {
	       if(Valid(db[exit].destination)) {
		  if(!((db[player].owner != db[db[exit].destination].owner) && (db[db[player].location].owner != db[db[exit].destination].owner) && !friendflags_check(player,player,db[db[exit].destination].owner,FRIEND_LINK,"Entry"))) {
		     if(!contains(db[exit].destination,player)) {
			if(will_fit(player,db[exit].destination)) {
			   if(can_satisfy_lock(player,exit,(Openable(exit) && !Open(exit)) ? (Locked(exit)) ? "Sorry, that exit is locked (Try unlocking it first.)":"Sorry, that exit is closed (Try opening it first.)":"Sorry, you can't go that way.",0)) {
			      char buffer[32];

			      if(event_trigger_fuses(player,exit,NULL,FUSE_ARGS|FUSE_COMMAND)) return(1);
			      command_execute_action(player,exit,".exit",NULL,getname(player),getnameid(player,exit,NULL),getnameid(player,db[exit].destination,buffer),1);
			      move_enter(player,db[exit].destination,1);
			      if(Openable(exit) && Sticky(exit)) {
				 db[exit].flags &= ~OPEN;
				 if(getlock(exit,0)) db[exit].flags |= LOCKED;
				 if(!Invisible(db[exit].location)) {
				    sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" closes%s ",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0),(Locked(exit)) ? " and locks":"");
				    sprintf(scratch_buffer + strlen(scratch_buffer),"%s"ANSI_LWHITE"%s"ANSI_LGREEN" behind %%o.",Article(exit,LOWER,DEFINITE),getexit_firstname(player,exit,0));
				    output_except(db[exit].location,player,NOTHING,0,1,0,"%s",substitute(player,scratch_return_string,scratch_buffer,0,ANSI_LGREEN,NULL,0));
				 }
			      }
			      if(getfield(exit,DROP)) {
				 substitute(player,scratch_buffer,(char *) getfield(exit,DROP),0,ANSI_LCYAN,NULL,0);
				 output(getdsc(player),player,0,1,0,"%s",punctuate(scratch_buffer,2,'.'));
			      }

			      if(!Invisible(db[player].location)) {
				 char *cmd_arg0,*cmd_arg1,*cmd_arg2,*cmd_arg3;
				 char buffer[TEXT_SIZE],token[2];

				 if(getfield(exit,ODROP)) {
				    substitute(player,scratch_buffer,(char *) getfield(exit,ODROP),DEFINITE,ANSI_LCYAN,NULL,0);
				    output_except(db[player].location,player,NOTHING,0,1,2,"%s",punctuate(scratch_buffer,0,'.'));
				 }
				 output_except(db[player].location,player,NOTHING,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" %s arrived.",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0),(Articleof(player) == ARTICLE_PLURAL) ? "have":"has");
				 event_set_fuse_args((in_command && command_lineptr) ? command_lineptr:command_line,&cmd_arg0,&cmd_arg1,&cmd_arg2,&cmd_arg3,buffer,token,0);
				 command_execute_action(player,NOTHING,".entercmd",NULL,cmd_arg1,cmd_arg2,cmd_arg3,0);
				 command_execute_action(player,exit,".exited",NULL,getname(player),getnameid(player,exit,NULL),getnameid(player,db[exit].destination,buffer),1);
			      }
			      setreturn(OK,COMMAND_SUCC);
			   }
			} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't go that way  -  There isn't enough room for you in there.");
		     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you're already in the room where that exit leads to.");
		  }
	       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't go that way  -  That exit doesn't lead anywhere.");
	    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't go that way.");
	 }
	 return(1);
}

/* ---->  Drive/ride vehicle to destination  <---- */
void move_vehicle(dbref player,dbref location,dbref vehicle,char *dest,unsigned char drive)
{
     dbref destination,passenger,cache,exit = NOTHING,oldloc = db[player].location;
     char  buffer[32];

     if(!Blank(dest)) {
        if(!Immovable(vehicle)) {
           if(could_satisfy_lock(player,vehicle,0)) {

              /* ---->  Destination  <---- */
              setreturn(ERROR,COMMAND_FAIL);
              if(strcasecmp(dest,"home")) {
                 move_to(player,location);
                 destination = match_preferred(player,player,dest,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT & ~MATCH_OPTION_NOTIFY);
                 if(!Valid(destination) && ((destination = lookup_character(player,dest,1)) == NOTHING))
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"%s"ANSI_LGREEN"' cannot be found.  Please specify the name of an exit, connected character, or the #ID of a room.",dest);

                 move_to(player,oldloc);
                 if(Valid(destination)) {
                    if(Typeof(destination) == TYPE_CHARACTER) {
                       if(!Connected(destination)) {
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" isn't currently connected.",Article(destination,LOWER,DEFINITE),getcname(NOTHING,destination,0,0));
                          return;
		       } else destination = db[destination].location;
		    }
		 } else return;

                 if(!Abode(destination) && !can_write_to(player,destination,1) && !Level4(db[player].owner)) {
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only %s a vehicle to a location which has its "ANSI_LYELLOW"ABODE"ANSI_LGREEN" flag set, or a location which you own.",(drive) ? "drive":"ride");
                    return;
		 }
	      } else destination = db[player].destination;

              /* ---->  Drive/ride to destination  <---- */
              if((Typeof(destination) == TYPE_ROOM) || (Typeof(destination) == TYPE_EXIT)) {
                 if(Typeof(destination) == TYPE_EXIT) {
                    exit = destination, destination = db[exit].destination;
                    if(!Valid(destination) || (destination == location) || (Typeof(destination) != TYPE_ROOM)) {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only %s %s"ANSI_LWHITE"%s"ANSI_LGREEN" through an exit which leads to another room.",(drive) ? "drive":"ride",Article(vehicle,LOWER,DEFINITE),unparse_object(player,vehicle,0));
                       return;
		    } else if(!Transport(exit)) {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't %s %s"ANSI_LWHITE"%s"ANSI_LGREEN" through that exit.",(drive) ? "drive":"ride",Article(vehicle,LOWER,DEFINITE),unparse_object(player,vehicle,0));
                       return;
		    }
		 } else if(destination == location) {
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only %s %s"ANSI_LWHITE"%s"ANSI_LGREEN" to a different room.",(drive) ? "drive":"ride",Article(vehicle,LOWER,DEFINITE),unparse_object(player,vehicle,0));
                    return;
		 }

                 if(!in_area(destination,vehicle)) {
                    if(!in_area(destination,player)) {
                       if(Transport(destination)) {
                          if(!((db[player].owner != db[destination].owner) && (db[location].owner != db[destination].owner) && !friendflags_check(player,player,db[destination].owner,FRIEND_LINK,"Entry"))) {
                             if(will_fit(player,destination)) {
                                move_to(player,location);
                                if(!Valid(exit) || could_satisfy_lock(player,exit,0)) {

                                   /* ---->  Trigger fuses on vehicle  <---- */
                                   if(event_trigger_fuses(player,vehicle,NULL,FUSE_ARGS|FUSE_COMMAND)) {
                                      move_to(player,oldloc);
                                      return;
				   }
                                   command_execute_action(player,vehicle,".drive",NULL,getname(player),getnameid(player,vehicle,NULL),getnameid(player,destination,buffer),1);

                                   /* ---->  Success messages of vehicle  <---- */
                                   if(getfield(vehicle,SUCC)) {
                                      substitute(player,scratch_buffer,(char *) getfield(vehicle,SUCC),0,ANSI_LCYAN,NULL,0);
                                      output(getdsc(player),player,0,1,0,"%s",punctuate(scratch_buffer,2,'.'));
				   }

                                   if(!Invisible(location)) {
                                      if(getfield(vehicle,OSUCC)) {
                                         substitute(player,scratch_buffer,(char *) getfield(vehicle,OSUCC),(drive) ? INDEFINITE:DEFINITE,ANSI_LCYAN,NULL,0);
                                         output_except(location,player,NOTHING,0,1,2,"%s",punctuate(scratch_buffer,0,'.'));
				      } else {
                                         sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN,Article(player,UPPER,(drive) ? INDEFINITE:DEFINITE),getcname(NOTHING,player,0,0));
                                         output_except(location,player,NOTHING,0,1,2,"%s %s %s"ANSI_LYELLOW"%s"ANSI_LGREEN".",scratch_buffer,(drive) ? "drives away in":"rides off on",Article(vehicle,LOWER,(drive) ? INDEFINITE:DEFINITE),getname(vehicle));
				      }
				   }

                                   if(!Invisible(vehicle) && getfield(vehicle,OSUCC)) {
                                      substitute(player,scratch_buffer,(char *) getfield(vehicle,OSUCC),DEFINITE,ANSI_LCYAN,NULL,0);
                                      output_except(vehicle,player,NOTHING,0,1,2,"%s",punctuate(scratch_buffer,0,'.'));
				   }

                                   if(!Valid(exit) || can_satisfy_lock(player,exit,(Openable(exit) && !Open(exit)) ? (Locked(exit)) ? "Sorry, that exit is locked (Try unlocking it first.)":"Sorry, that exit is closed (Try opening it first.)":"Sorry, you can't go that way.",0)) {
    
                                      /* ---->  Trigger fuses on exit  <----*/
                                      if(Valid(exit) && event_trigger_fuses(player,exit,NULL,FUSE_ARGS|FUSE_COMMAND)) {
                                         move_to(player,oldloc);
                                         return;
				      }

                                      /* ---->  Enter destination room  <---- */
                                      if(!drive) move_to(vehicle,destination);
                                      cache = db[player].commands, db[player].commands = NOTHING;
                                      if(!Valid(exit)) {
                                         int  ic_cache = in_command;
                                         char buffer[16];

                                         in_command = 1;
                                         sprintf(buffer,"#%d",destination);
                                         move_teleport(player,NULL,NULL,"me",buffer,0,0);
                                         in_command = ic_cache;
				      } else move_enter(player,destination,1);
                                      db[player].commands = cache;
                                      if(Location(player) == location)
                                         move_to(player,oldloc);

                                      for(passenger = db[vehicle].contents; Valid(passenger); passenger = Next(passenger))
                                          if(Validchar(passenger) && Connected(passenger) && (passenger != player))
                                             look_room(passenger,destination);

                                      /* ---->  Close auto-closing exit  <---- */
                                      if(Valid(exit) && Openable(exit) && Sticky(exit)) {
                                         db[exit].flags &= ~OPEN;
                                         if(getlock(exit,0)) db[exit].flags |= LOCKED;
                                         if(!Invisible(db[exit].location)) {
                                            sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" closes%s ",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0),(Locked(exit)) ? " and locks":"");
                                            sprintf(scratch_buffer + strlen(scratch_buffer),"%s"ANSI_LWHITE"%s"ANSI_LGREEN" behind %%o.",Article(exit,LOWER,DEFINITE),getexit_firstname(player,exit,0));
                                            output_except(db[exit].location,player,NOTHING,0,1,0,"%s",substitute(player,scratch_return_string,scratch_buffer,0,ANSI_LGREEN,NULL,0));
					 }
				      }

                                      /* ---->  Drop messages of vehicle  <---- */
                                      if(getfield(vehicle,DROP)) {
                                         substitute(player,scratch_buffer,(char *) getfield(vehicle,DROP),0,ANSI_LCYAN,NULL,0);
                                         output(getdsc(player),player,0,1,0,"%s",punctuate(scratch_buffer,2,'.'));
				      }

                                      if(!Invisible(db[player].location)) {
                                         if(getfield(vehicle,ODROP)) {
                                            substitute(player,scratch_buffer,(char *) getfield(vehicle,ODROP),INDEFINITE,ANSI_LCYAN,NULL,0);
                                            output_except(db[player].location,player,NOTHING,0,1,2,"%s",punctuate(scratch_buffer,0,'.'));
					 } else {
                                            sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN,Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0));
                                            output_except(db[player].location,player,NOTHING,0,1,2,"%s arrives %s %s"ANSI_LYELLOW"%s"ANSI_LGREEN".",scratch_buffer,(drive) ? "in":"on",Article(vehicle,LOWER,INDEFINITE),getname(vehicle));
					 }
				      }

                                      if(!Invisible(vehicle) && getfield(vehicle,ODROP)) {
                                         substitute(player,scratch_buffer,(char *) getfield(vehicle,ODROP),DEFINITE,ANSI_LCYAN,NULL,0);
                                         output_except(vehicle,player,NOTHING,0,1,2,"%s",punctuate(scratch_buffer,0,'.'));
				      }
                                      move_to(vehicle,db[player].location);
                                      command_execute_action(player,vehicle,".driven",NULL,getname(player),getnameid(player,vehicle,NULL),getnameid(player,destination,buffer),1);
                                      if(drive) move_to(player,vehicle);
                                      setreturn(OK,COMMAND_SUCC);
				   } else move_to(player,oldloc);
				} else move_to(player,oldloc);
			     } else {
                                sprintf(scratch_return_string,ANSI_LGREEN"Sorry, there isn't enough room in %s"ANSI_LWHITE"%s"ANSI_LGREEN" to ",Article(destination,LOWER,Valid(exit) ? INDEFINITE:DEFINITE),unparse_object(player,destination,0));
                                output(getdsc(player),player,0,1,0,"%s%s %s"ANSI_LWHITE"%s"ANSI_LGREEN" into it.",scratch_return_string,(drive) ? "drive":"ride",Article(vehicle,LOWER,DEFINITE),unparse_object(player,vehicle,0));
			     }
			  }
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, vehicles aren't permitted in %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(destination,LOWER,Valid(exit) ? INDEFINITE:DEFINITE),unparse_object(player,destination,0));
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't drive a vehicle into %s which you're currently carrying.",object_type(destination,1));
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't drive a vehicle into %s which is within itself.",object_type(destination,1));
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only %s %s"ANSI_LWHITE"%s"ANSI_LGREEN" through an exit or into a room.",(drive) ? "drive":"ride",Article(vehicle,LOWER,DEFINITE),unparse_object(player,vehicle,0));
	   } else {

              /* ---->  Failure messages of vehicle  <---- */
              if(getfield(vehicle,FAIL)) {
                 substitute(player,scratch_buffer,(char *) getfield(vehicle,FAIL),0,ANSI_LCYAN,NULL,0);
                 output(getdsc(player),player,0,1,0,"%s",punctuate(scratch_buffer,2,'.'));
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" can't be %s by you.",Article(vehicle,LOWER,DEFINITE),unparse_object(player,vehicle,0),(drive) ? "driven":"riden");

              if(!Invisible(location) && getfield(vehicle,OFAIL)) {
                 substitute(player,scratch_buffer,(char *) getfield(vehicle,OFAIL),(drive) ? INDEFINITE:DEFINITE,ANSI_LCYAN,NULL,0);
                 output_except(location,player,NOTHING,0,1,2,"%s",punctuate(scratch_buffer,0,'.'));
	      } 

              if(!Invisible(vehicle) && getfield(vehicle,OFAIL)) {
                 substitute(player,scratch_buffer,(char *) getfield(vehicle,OFAIL),DEFINITE,ANSI_LCYAN,NULL,0);
                 output_except(vehicle,player,NOTHING,0,1,2,"%s",punctuate(scratch_buffer,0,'.'));
	      }
	   }
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" can't be %s%s.",Article(vehicle,LOWER,DEFINITE),unparse_object(player,vehicle,0),(drive) ? "driven":"riden",(db[db[player].location].location != destination) ? " to another location":"");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify where you would like to %s %s"ANSI_LWHITE"%s"ANSI_LGREEN" to.",(drive) ? "drive":"ride",Article(vehicle,LOWER,DEFINITE),unparse_object(player,vehicle,0));
}

/* ---->  Drive vehicle  <---- */
void move_drive(CONTEXT)
{
     setreturn(ERROR,COMMAND_FAIL);
     if(Vehicle(db[player].location)) {
        if(Valid(db[db[player].location].location)) {
           move_vehicle(player,db[db[player].location].location,db[player].location,params,1);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the vehicle you're currently inside isn't within a location (It's currently floating around in the void.)");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you aren't inside a vehicle.");
}

/* ---->      Take or drop object       <---- */
/*        (val1:  0 = Take, 1 = Drop.)        */
void move_getdrop(CONTEXT)
{
     dbref         loc,thing,container;
     unsigned char notified = 0;

     setreturn(ERROR,COMMAND_FAIL);
     loc = Location(player);
     if(!Valid(loc)) return;

     thing = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     /* ---->  Work out where to put it  <---- */
     if(!Blank(arg2)) {
        container = match_preferred(player,player,arg2,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
        if(!Valid(container)) return;
     } else container = (val1) ? Location(player):player;

     /* ---->  Check character isn't trying to pick up/drop themselves!  <---- */
     if(thing != player) {
        if(Typeof(thing) != TYPE_CHARACTER) {
           if(can_write_to(player,thing,0) || CanTakeOrDrop(player,thing)) {
              if(!(!can_write_to(player,thing,0) && CanTakeOrDrop(player,thing) && !can_reach(player,thing))) {
                 if(in_area(thing,db[player].location)) {
                    if(HasList(container,WhichList(thing))) {
                       if(can_teleport_object(player,thing,container)) {
                          if(!Immovable(thing)) {
                             if(!Vehicle(thing)) {
                                if(thing != container) {
                                   if(db[thing].location != container) {
                                      if((val1 == 1) || ((val1 == 0) && Typeof(db[thing].location) != TYPE_CHARACTER)) {
                                         if(!contains(container,thing)) {
                                            if(!((container == player) && contains(player,thing))) {

                                               /* ---->  Take object/drop it  <---- */
                                               if(Typeof(thing) == TYPE_THING) {
                                                  if(!((Container(container) || (container == player)) && !will_fit(thing,container))) {
                                                     if(!(Valid(db[thing].location) && (Typeof(db[thing].location) == TYPE_THING) && (!Container(db[thing].location) || !Open(db[thing].location)))) {
                                                        if(Typeof(container) == TYPE_THING) {
                                                           if(!(!Container(container) || !Open(container))) {
                                                              if(event_trigger_fuses(player,thing,NULL,FUSE_ARGS|FUSE_COMMAND)) return;
                                                              command_execute_action(player,thing,(!val1) ? ".take":".drop",NULL,getname(player),getnameid(player,thing,NULL),"",1);
                                                              if(db[thing].location != player) {
                                                                 if(!can_satisfy_lock(player,thing,(val1) ? "Sorry, you can't pick up that object to drop it elsewhere.":"Sorry, you can't pick up that object.",0)) return;
                                                                 if(getfield(thing,SUCC)) notified = 1;
                                                              }
                                                              move_to(thing,container);
                                                           } else {
                                                              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only %s %s"ANSI_LWHITE"%s"ANSI_LGREEN" %sinto a container that's open.",(db[thing].location == player) ? "drop":"pick up",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0),(db[thing].location == player) ? "":"and drop it ");
                                                              return;
                                                           }
                                                        } else if(!(val1 && Sticky(thing))) {
                                                           if(val1 && Valid(db[container].destination) && (Typeof(container) == TYPE_ROOM) && !Sticky(container) && (db[container].destination != thing) && (db[container].destination != container)) {

                                                              /* ---->  Location has an immediate drop-to  -  Test to see if it will fit first  <---- */
                                                              if(event_trigger_fuses(player,thing,NULL,FUSE_ARGS|FUSE_COMMAND)) return;
                                                              command_execute_action(player,thing,(!val1) ? ".take":".drop",NULL,getname(player),getnameid(player,thing,NULL),"",1);
                                                              if(db[thing].location != player) {
                                                                 if(!can_satisfy_lock(player,thing,(val1) ? "Sorry, you can't pick up that object to drop it elsewhere.":"Sorry, you can't pick up that object.",0)) return;
                                                                 if(getfield(thing,SUCC)) notified = 1;
                                                              }
                                                              output(getdsc(player),player,0,1,0,ANSI_LGREEN"As the object touches the ground, it vanishes in a "ANSI_BLINK"flash"ANSI_LGREEN" of "ANSI_LCYAN"blinding blue light"ANSI_LGREEN".");
                                                              if(will_fit(thing,db[container].destination))
                                                                 move_to(thing,db[container].destination);
                                                              else move_home(thing,0);
                                                           } else {
                                                              if(event_trigger_fuses(player,thing,NULL,FUSE_ARGS|FUSE_COMMAND)) return;
                                                              command_execute_action(player,thing,(!val1) ? ".take":".drop",NULL,getname(player),getnameid(player,thing,NULL),"",1);
                                                              if(db[thing].location != player) {
                                                                 if(!can_satisfy_lock(player,thing,(val1) ? "Sorry, you can't pick up that object to drop it elsewhere.":"Sorry, you can't pick up that object.",0)) return;
                                                                 if((container == player) && getfield(thing,SUCC)) notified = 1;
                                                              }
                                                              move_to(thing,container);
                                                           }
                                                        } else {
                                                           command_execute_action(player,thing,(!val1) ? ".take":".drop",NULL,getname(player),getnameid(player,thing,NULL),"",1);
                                                           if(db[thing].location != player) {
                                                              if(!can_satisfy_lock(player,thing,(val1) ? "Sorry, you can't pick up that object to drop it elsewhere.":"Sorry, you can't pick up that object.",0)) return;
                                                              if((container == player) && getfield(thing,SUCC)) notified++;
                                                           }
                                                           move_home(thing,0);
                                                        }

                                                        /* ---->  Appropriate messages for get/drop  <---- */
                                                        if(container == player) {

                                                           /* ---->  Object is being picked up  <---- */
                                                           if(!getfield(thing,OSUCC) && !Invisible(db[player].location) && !Invisible(thing))
                                                              output_except(db[player].location,player,NOTHING,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" picks up %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0),Article(thing,LOWER,DEFINITE),getname(thing));
                                                        } else {

                                                           /* ---->  Object is being dropped  <---- */
                                                           if(getfield(thing,DROP)) {
                                                              substitute(player,scratch_buffer,(char *) getfield(thing,DROP),0,ANSI_LCYAN,NULL,0);
                                                              output(getdsc(player),player,0,1,0,"%s",punctuate(scratch_buffer,2,'.'));
                                                              notified = 1;
                                                           }
                                                           if(!Vehicle(container) && getfield(container,DROP)) {
                                                              substitute(player,scratch_buffer,(char *) getfield(container,DROP),0,ANSI_LCYAN,NULL,0);
                                                              output(getdsc(player),player,0,1,0,"%s",punctuate(scratch_buffer,2,'.'));
                                                           }

                                                           /* ---->  Others drop message of thing and/or new location  <---- */
                                                           if((db[thing].location == db[player].location) && !Invisible(db[player].location) && !Invisible(thing)) {
                                                              if(getfield(thing,ODROP)) substitute(player,scratch_buffer,(char *) getfield(thing,ODROP),DEFINITE,ANSI_LCYAN,NULL,0);
                                                              else sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" drops %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0),Article(thing,LOWER,INDEFINITE),getname(thing));
                                                              output_except(db[thing].location,player,NOTHING,0,1,2,"%s",punctuate(scratch_buffer,0,'.'));
                                                              if(getfield(db[thing].location,ODROP)) {
                                                                 substitute(player,scratch_buffer,(char *) getfield(db[thing].location,ODROP),DEFINITE,ANSI_LCYAN,NULL,0);
                                                                 output_except(db[thing].location,player,NOTHING,0,1,2,"%s",punctuate(scratch_buffer,0,'.'));
                                                              }
                                                           }
                                                        }
                                                     } else {
                                                        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" %s  -  %s.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0),Container(db[thing].location) ? "is in a closed container":"is in an object that isn't a container",(val1) ? "You can't pick it up and drop it elsewhere":"You can't pick it up");
                                                        return;
                                                     }
                                                  } else {
                                                     if(container != player) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't %s %s"ANSI_LWHITE"%s"ANSI_LGREEN" %sin there  -  There isn't enough room.",(db[thing].location == player) ? "drop":"pick up",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0),(db[thing].location == player) ? "":"and put it ");
                                                     else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is too heavy for you to pick up  -  If you're carrying a lot of objects, try dropping some of them first.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
                                                     return;
                                                  }
                                               } else {
                                                  command_execute_action(player,thing,(!val1) ? ".take":".drop",NULL,getname(player),getnameid(player,thing,NULL),"",1);
                                                  move_to(thing,container);
                                               }

                                               if(!in_command && !notified) {
                                                  sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" %s",Article(thing,UPPER,INDEFINITE),unparse_object(player,thing,0),(container == player) ? "taken.":"dropped in ");
                                                  if(container != player) sprintf(scratch_buffer + strlen(scratch_buffer),"%s"ANSI_LWHITE"%s"ANSI_LGREEN"%s",Article(db[thing].location,LOWER,DEFINITE),unparse_object(player,db[thing].location,0),(val1) ? (db[thing].location != container) ? Sticky(thing) ? " (Object's home.)":" (Location's drop-to.)":".":".");
                                                  output(getdsc(player),player,0,1,0,"%s",scratch_buffer);
                                               }
                                               command_execute_action(player,thing,(container == player) ? ".taken":".dropped",NULL,getname(player),getnameid(player,thing,NULL),"",1);
                                               setreturn(OK,COMMAND_SUCC);
                                            } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't pick up the container you're currently inside.");
                                         } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" can't be %sdropped into %s within itself.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0),(db[thing].location == player) ? "":"picked up and ",object_type(container,1));
                                      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't take an object from a player.");
                                   } else if(container != player) {
                                      sprintf(scratch_buffer,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is already in ",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
                                      output(getdsc(player),player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(container,LOWER,DEFINITE),unparse_object(player,container,0));
                                   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you're already carrying %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
                                } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't %s %s"ANSI_LWHITE"%s"ANSI_LGREEN" %s itself.",(db[thing].location == player) ? "drop":"pick up",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0),(db[thing].location == player) ? "into":"and drop it into");
                             } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a vehicle can't be %s.",(val1) ? "dropped":"picked up");
                          } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" can't be %s.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0),(val1) ? "dropped":"picked up");
                       } else if(Typeof(container) == TYPE_CHARACTER) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can%s %s %s"ANSI_LWHITE"%s"ANSI_LGREEN" and give it to %s.",Level4(db[player].owner) ? " only":"'t",(db[thing].location == player) ? "drop":"pick up",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0),Level4(db[player].owner) ? "someone of a lower level than yourself":"another character");
                          else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can%s %s %s"ANSI_LWHITE"%s"ANSI_LGREEN" into %s owned by %s.",Level4(db[player].owner) ? " only":"'t",(db[thing].location != player) ? "drop":"pick up and drop",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0),object_type(container,1),Level4(db[player].owner) ? "someone of a lower level than yourself":"another character");
                    } else if(container == player) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't pick up %s.",object_type(thing,1));
                       else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s can't be dropped into %s.",object_type(thing,1),object_type(container,1));
                 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only %s an object which is within your current location.",(container == player) ? "drop an object which you're holding or":"pick up");
              } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't reach %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
           } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only %s %s which belongs to someone who's of a lower level than yourself.",(val1) ? "drop":"pick up",object_type(thing,1));
              else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only %s %s which you own.",(val1) ? "drop":"pick up",object_type(thing,1));
        } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't %s another character.",(container != player) ? "drop":"pick up");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't %s yourself.",(container != player) ? "drop":"pick up");
}

/* ---->  Give object to another character  <---- */
void move_give(CONTEXT)
{
     dbref currentobj,recipient,object;
     char  buffer[32];

     setreturn(ERROR,COMMAND_FAIL);
     if(Blank(arg2)) {
        keyword("to",arg1,&arg2);
        if(!Blank(arg2)) {
           char *swap = arg2;
           arg2 = arg1, arg1 = swap;
	}
     }

     if(!Blank(arg1)) {
        if((recipient = lookup_character(player,arg1,1)) != NOTHING) {
           object = match_preferred(player,player,arg2,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
           if(!Valid(object)) return;

           if(Typeof(object) == TYPE_THING) {
              if(!Immovable(object)) {
                 if(db[object].location == player) {
                    if(db[player].location == db[recipient].location) {
                       if(!(!Connected(recipient) && !Being(recipient) && !Level4(db[player].owner))) {
	                  if(will_fit(object,recipient)) {
	                     if(Level4(db[player].owner) || Level4(db[object].owner) || (db[db[recipient].location].owner == db[object].owner) || can_write_to(player,db[recipient].location,0) || (getfirst(object,COMMANDS,&currentobj) == NOTHING)) {
                                command_execute_action(player,object,".give",NULL,getname(player),getnameid(player,object,NULL),getnameid(player,recipient,buffer),1);
                                sprintf(scratch_buffer,ANSI_LGREEN"You give %s"ANSI_LWHITE"%s"ANSI_LGREEN" to ",Article(object,LOWER,DEFINITE),unparse_object(player,object,0));
                                output(getdsc(player),player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(recipient,LOWER,DEFINITE),getcname(NOTHING,recipient,0,0));

                                sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" gives you ",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0));
                                output(getdsc(recipient),player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(object,LOWER,(db[object].location == db[recipient].location) ? DEFINITE:INDEFINITE),unparse_object(recipient,object,0));

                                if(!Invisible(db[player].location) && !Invisible(object)) {
                                   sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" gives ",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0));
                                   output_except(db[player].location,player,recipient,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN" to %s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(object,LOWER,(db[object].location == db[player].location) ? DEFINITE:INDEFINITE),getname(object),Article(recipient,LOWER,DEFINITE),getcname(NOTHING,recipient,0,0));
				}
                                move_to(object,recipient);
                                command_execute_action(player,object,".given",NULL,getname(player),getnameid(player,object,NULL),getnameid(player,recipient,buffer),1);
                                setreturn(OK,COMMAND_SUCC);
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't give an object with attached compound commands to another character (Unless their current location is owned by yourself or the owner of the object being given.)");
			  } else {
                             sprintf(scratch_buffer,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is carrying far too much to be able to carry ",Article(recipient,LOWER,DEFINITE),getcname(NOTHING,recipient,0,0));
                             output(getdsc(player),player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(object,LOWER,DEFINITE),unparse_object(player,object,0));
			  }
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"You notice that %s"ANSI_LWHITE"%s"ANSI_LGREEN" is currently asleep and decide not to disturb %s.",Article(recipient,LOWER,DEFINITE),getcname(NOTHING,recipient,0,0),Objective(recipient,0));
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only give %s"ANSI_LWHITE"%s"ANSI_LGREEN" to someone who's in the same room as you.",Article(object,LOWER,DEFINITE),unparse_object(player,object,0));
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you must be carrying %s"ANSI_LWHITE"%s"ANSI_LGREEN" to give it to somebody else.",Article(object,LOWER,DEFINITE),unparse_object(player,object,0));
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" can't be given to another character.",Article(object,LOWER,DEFINITE),unparse_object(player,object,0));
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only give a thing to another character.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify who you'd like to give the object to.");
}

/* ---->  {J.P.Boggis 23/07/2000}  Create user home room  <---- */
/*        (VAL1:  0 = Normal, 1 = Set character's home to newly created home room)  */
void move_homeroom(CONTEXT)
{
     setreturn(ERROR,COMMAND_FAIL);
     if(!in_command || (Valid(current_command) && Wizard(current_command))) {
#ifdef HOME_ROOMS
        create_homeroom(player,1,0,1);
#else
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't have a home room on %s (Feature disabled.)",tcz_full_name);
#endif
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@homeroom"ANSI_LGREEN"' can not be used from within a compound command.");
}

/* ---->  Kick character/junk object  <---- */
void move_kick(CONTEXT)
{
     dbref thing;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(params)) {
        thing = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
        if(!Valid(thing)) return;

        if(!RoomZero(Location(thing))) {
           if(Typeof(thing) == TYPE_CHARACTER) {
              if((db[player].owner == db[db[thing].location].owner) || can_write_to(player,thing,1)) {
                 if(thing != player) {
                    char  *p1,*store;
                    dbref oldloc;

                    /* ---->  Kick character  <---- */
                    oldloc = db[thing].location;
                    if(db[db[thing].destination].owner == db[player].owner) db[thing].destination = ROOMZERO;
                    if(!Blank(arg2)) {
                       if(!Censor(player) && !Censor(db[player].location)) bad_language_filter(scratch_return_string,arg2);
                          else strcpy(scratch_return_string,arg2);
                       substitute(player,scratch_buffer,punctuate(scratch_return_string,0,'!'),0,ANSI_LGREEN,NULL,0);
		    } else strcpy(scratch_buffer,"!");
                    p1 = scratch_buffer;
                    strcpy(scratch_return_string,pose_string(&p1,"*"));
                    if(*p1) strcat(scratch_return_string,p1);

                    command_execute_action(player,thing,".junk",NULL,getname(player),getnameid(player,thing,NULL),"",1);
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"You chuck %s"ANSI_LWHITE"%s"ANSI_LGREEN" out%s",Article(thing,LOWER,DEFINITE),getcname(NOTHING,thing,0,0),scratch_return_string);
   	            if(Connected(thing)) {
                       sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" chucks ",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0));
                       output_except(oldloc,player,thing,0,1,2,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN" out%s",scratch_buffer,Article(thing,LOWER,DEFINITE),getcname(NOTHING,thing,0,0),scratch_return_string);
		    }

                    store = (char *) alloc_string(scratch_return_string);
                    move_enter(thing,ROOMZERO,1);
                    if(Connected(thing)) {
                       sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has chucked you out of ",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0));
                       output(getdsc(thing),thing,0,1,2,"%s%s"ANSI_LYELLOW"%s"ANSI_LGREEN"%s\n",scratch_buffer,Article(oldloc,LOWER,DEFINITE),unparse_object(thing,oldloc,0),String(store));
		    }
                    command_execute_action(player,thing,".junked",NULL,getname(player),getnameid(player,thing,NULL),"",1);
                    FREENULL(store);
                    setreturn(OK,COMMAND_SUCC);
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't kick yourself out of your current location.");
	      } else if(Level4(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only kick lower level characters out of locations which you don't own.");
                 else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only kick other characters out of locations which you own.");
	   } else if((Valid(db[thing].location) && (db[player].owner == db[db[thing].location].owner)) || can_write_to(player,thing,1)) {

              /* ---->  Junk object  <---- */
              command_execute_action(player,thing,".junk",NULL,getname(player),getnameid(player,thing,NULL),"",1);
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has been junked.",Article(thing,UPPER,DEFINITE),unparse_object(player,thing,0));
              move_to(thing,ROOMZERO);
              if((Typeof(thing) == TYPE_COMMAND) || (Typeof(thing) == TYPE_FUSE) || (Typeof(thing) == TYPE_EXIT))
                 db[thing].flags |= INVISIBLE;
              command_execute_action(player,thing,".junked",NULL,getname(player),getnameid(player,thing,NULL),"",1);
              setreturn(OK,COMMAND_SUCC);
	   } else if(Level4(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only junk objects owned by lower level characters from locations which you don't own.");
              else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only junk objects you own or objects which are in locations you own.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't %s %s"ANSI_LWHITE"%s"ANSI_LGREEN".",(Typeof(thing) == TYPE_CHARACTER) ? "kick a character out of":"junk an object in",Article(ROOMZERO,LOWER,DEFINITE),unparse_object(player,ROOMZERO,0));
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify who you would like to kick or what you would like to junk.");
}

/* ---->  Execute a command at remote location  <---- */
void move_remote(CONTEXT)
{
     dbref         loc,cached_loc;
     unsigned char abort = 0;

     setreturn(ERROR,COMMAND_FAIL);
     loc  = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(loc)) return;

     if(Level3(db[player].owner) || can_write_to(player,loc,0)) {
        if(!((Typeof(loc) != TYPE_ROOM) && !Container(loc))) {
           cached_loc = db[player].location;
           move_to(player,loc);

           abort |= event_trigger_fuses(player,player,arg2,FUSE_ARGS);
           abort |= event_trigger_fuses(player,Location(player),arg2,FUSE_ARGS);
           if(!abort) command_sub_execute(player,arg2,0,1);

           if(!Valid(cached_loc) || (Typeof(cached_loc) == TYPE_FREE)) {
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the location you remoted from no-longer exists  -  You have been teleported to %s"ANSI_LWHITE"%s"ANSI_LGREEN" ("ANSI_LYELLOW"ROOM ZERO"ANSI_LGREEN".)",Article(ROOMZERO,LOWER,DEFINITE),unparse_object(player,ROOMZERO,0));
              move_enter(player,ROOMZERO,1);
	   } else move_to(player,cached_loc);
 	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" isn't a room or container.",Article(loc,LOWER,DEFINITE),getcname(player,loc,0,0));
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only remote to a room or container that you own.");
}

/* ---->  Ride vehicle  <---- */
void move_ride(CONTEXT)
{
     dbref vehicle;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(arg1)) {
        vehicle = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_THING,MATCH_OPTION_DEFAULT);
        if(!Valid(vehicle)) return;

        if(Vehicle(vehicle)) {
           if(db[vehicle].location == db[player].location) {
              if(!Container(vehicle)) {
                 move_vehicle(player,db[player].location,vehicle,arg2,0);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" can't be riden (It must be entered and then driven.)",Article(vehicle,LOWER,INDEFINITE),unparse_object(player,vehicle,0));
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only ride a vehicle which is in your current location.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" isn't a vehicle.",Article(vehicle,LOWER,INDEFINITE),unparse_object(player,vehicle,0));
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify which vehicle you would like to ride.");
}

/* ---->  Teleport something to somewhere  <---- */
void move_teleport(CONTEXT)
{
     dbref object,destination,character = NOTHING;

     setreturn(ERROR,COMMAND_FAIL);
     object = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(object)) return;

     if(!Blank(arg2)) {
        destination = match_preferred(player,player,arg2,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
        if(!Valid(destination)) return;
     } else {
        destination = object;
        object      = player;
     }

     if((Typeof(object) == TYPE_CHARACTER) && (Typeof(destination) == TYPE_CHARACTER)) {
        character   = destination;
        destination = Location(destination);
     }

     if(!((object == destination) && (Typeof(object) != TYPE_ROOM))) {
        if(!((Owner(object) != Owner(destination)) && (Typeof(object) == TYPE_CHARACTER) && !friendflags_check(player,object,Owner(destination),FRIEND_LINK,"Teleport"))) {
           if(object == player) {

              /* ---->  Character is trying to teleport themselves elsewhere  <---- */
              if(Validchar(character)) {
                 if(!Level4(Owner(player)) && !can_write_to(player,destination,0) && !Abode(destination)) {
                    if(!friendflags_set(character,player,NOTHING,FRIEND_VISIT)) {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only teleport to a character who's on your own property, on property with its "ANSI_LYELLOW"ABODE"ANSI_LGREEN" flag set or has set the "ANSI_LYELLOW"VISIT"ANSI_LGREEN" friend flag on you in their friends list to allow you to visit them whenever you like.");
                       return;
		    } else if(db[character].flags & PRIVATE) {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is either holding a private conversation and doesn't wish to be interupted or simply doesn't want to be disturbed.",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0));
                       return;
		    } else if(!Visit(Location(character))) {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, visitors aren't allowed in %s"ANSI_LWHITE"%s"ANSI_LGREEN" (The owner of this %s has reset its "ANSI_LYELLOW"VISIT"ANSI_LGREEN" flag.)",Article(Location(character),LOWER,INDEFINITE),unparse_object(player,Location(character),0),object_type(Location(character),0));
                       return;
		    }
		 }
	      } else if(!Level4(Owner(player)) && !can_write_to(player,destination,0) && !Abode(destination)) {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only teleport around/to your own property or property with its "ANSI_LYELLOW"ABODE"ANSI_LGREEN" flag set.");
                 return;
	      }
	   } else if(Typeof(object) == TYPE_CHARACTER) {

              /* ---->  Character is trying to teleport someone else elsewhere  <---- */
              if(!(can_write_to(player,object,0) || (can_write_to(player,destination,0) && can_write_to(player,Location(object),0)))) {
                 if(Level3(Owner(player))) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't teleport that character  -  They're either the same or a higher level than you or they are not on property you own.");
                    else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only teleport someone around your own property.");
                 return;
	      }
	   } else if(!(can_write_to(player,object,0) && ((Level4(Owner(player)) && (Typeof(destination) != TYPE_CHARACTER)) || can_write_to(player,destination,0)))) {

              /* ---->  Character is trying to teleport an object elsewhere  <---- */
              if(Level3(Owner(player))) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only teleport something owned by yourself or a lower level character  -  Also, the destination mustn't be a character who's of a higher level than yourself.");
                 else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only teleport something around your own property.");
              return;
	   }

           if(HasList(destination,WhichList(object))) {
              if(!(!can_teleport_object(player,object,destination) && !((object == player) && ((Typeof(destination) == TYPE_ROOM) || (Typeof(destination) == TYPE_THING))))) {
                 if(!contains(destination,object) || ((Typeof(object) == TYPE_ROOM) && (destination == object))) {
                    if(!Immovable(object)) {
                       if(!((!will_fit(object,destination) && (destination != Location(object))) && (Typeof(object) != TYPE_ROOM))) {

                          /* ---->  Teleport OBJECT to DESTINATION  <---- */
                          if(Typeof(object) == TYPE_CHARACTER) {
                             if(will_fit(object,destination)) {
                                dbref oldloc = Location(object);
                                if(!move_enter(object,destination,1)) return;
                                if((oldloc != destination) && !Invisible(Location(object))) {
                                   char *cmd_arg0,*cmd_arg1,*cmd_arg2,*cmd_arg3;
                                   char buffer[TEXT_SIZE],token[2];

                                   output_except(Location(object),object,NOTHING,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" %s arrived.",Article(object,UPPER,INDEFINITE),getcname(NOTHING,object,0,0),(Articleof(object) == ARTICLE_PLURAL) ? "have":"has");
                                   event_set_fuse_args((in_command && command_lineptr) ? command_lineptr:command_line,&cmd_arg0,&cmd_arg1,&cmd_arg2,&cmd_arg3,buffer,token,0);
                                   command_execute_action(object,NOTHING,".entercmd",NULL,cmd_arg1,cmd_arg2,cmd_arg3,0);
				}
			     } else {
                                if(player != object) {
                                   sprintf(scratch_buffer,ANSI_LGREEN"Sorry, there isn't enough room for %s"ANSI_LWHITE"%s"ANSI_LGREEN" ",Article(object,LOWER,DEFINITE),getcname(player,object,1,0));
                                   output(getdsc(player),player,0,1,0,"%sin %s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(destination,LOWER,DEFINITE),getcname(player,destination,1,0));
				} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, to teleport you need enough free space to arrive in.  There isn't enough room in %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(destination,LOWER,DEFINITE),unparse_object(player,destination,0));
                                return;
			     }
			  } else if(!will_fit(object,destination)) {
                             sprintf(scratch_buffer,ANSI_LGREEN"Sorry, there isn't enough room for %s"ANSI_LWHITE"%s"ANSI_LGREEN" ",Article(object,LOWER,DEFINITE),getcname(player,object,1,0));
                             output(getdsc(player),player,0,1,0,"%sin %s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(destination,LOWER,DEFINITE),getcname(player,destination,1,0));
                             return;
			  } else move_to(object,destination);

                          if(!in_command) {
                             if(object != player) {
                                if(destination == object) destination = NOTHING;
                                sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" teleported to ",Article(object,UPPER,INDEFINITE),getcname(player,object,1,0));
                                output(getdsc(player),player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(destination,LOWER,DEFINITE),getcname(player,destination,1,0));
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"You teleport to %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(Location(player),LOWER,DEFINITE),unparse_object(player,Location(player),0)); 
			  }
                          setreturn(OK,COMMAND_SUCC);
		       } else if(!((Typeof(destination) == TYPE_THING) && !Container(destination))) {
   	  	          if((get_mass_or_volume(object,1) + find_volume_of_contents(destination,0)) > get_mass_or_volume(destination,1)) {
                             sprintf(scratch_buffer,ANSI_LGREEN"Sorry, there isn't enough room for %s"ANSI_LWHITE"%s"ANSI_LGREEN" ",Article(object,LOWER,DEFINITE),getcname(player,object,1,0));
                             output(getdsc(player),player,0,1,0,"%sin %s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(destination,LOWER,DEFINITE),getcname(player,destination,1,0));
			  } else if((get_mass_or_volume(object,0) + find_mass_of_contents(destination,0)) > get_mass_or_volume(destination,0)) {
                             sprintf(scratch_buffer,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is too heavy to be teleported into ",Article(object,LOWER,DEFINITE),getcname(player,object,1,0));
                             output(getdsc(player),player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(destination,LOWER,DEFINITE),getcname(player,destination,1,0));
			  } else {
                             sprintf(scratch_buffer,ANSI_LGREEN"Sorry, either there isn't enough room for %s"ANSI_LWHITE"%s"ANSI_LGREEN" in ",Article(object,LOWER,DEFINITE),getcname(player,object,1,0));
                             output(getdsc(player),player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN" or it's too heavy to be teleported there.",scratch_buffer,Article(destination,LOWER,DEFINITE),getcname(player,destination,1,0));
			  }
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" isn't a container.",Article(destination,LOWER,DEFINITE),getcname(player,destination,1,0));
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" can't be teleported%s.",Article(object,LOWER,DEFINITE),unparse_object(player,object,0),(Location(object) != destination) ? " to another location":"");
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s can't be teleported into %s %s.",object_type(object,1),object_type(destination,1),(Typeof(object) == TYPE_CHARACTER) ? "they're carrying":"within itself");
	      } else if(Typeof(destination) == TYPE_CHARACTER) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only teleport %s to %s.",object_type(object,1),(Level4(Owner(player))) ? "someone of a lower level than yourself":"yourself");
                 else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only teleport %s to %s owned by %s.",object_type(object,1),object_type(destination,1),(Level4(Owner(player))) ? "someone of a lower level than yourself":"yourself");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s can't be teleported to %s.",object_type(object,1),object_type(destination,1));
	}
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't teleport %s"ANSI_LWHITE"%s"ANSI_LGREEN" into itself.",Article(object,LOWER,DEFINITE),getcname(player,object,1,0));
}

/* ---->  Go to given location (If set ABODE)  <---- */
void move_location(CONTEXT)
{
     for(; *arg1 && (*arg1 == ' '); arg1++);
     for(; *arg1 && (*arg1 == '#'); arg1++);
     if(!Blank(arg1)) {
        sprintf(scratch_return_string,"#%s",arg1);
        move_teleport(player,NULL,NULL,"me",scratch_return_string,0,0);
     } else setreturn(ERROR,COMMAND_FAIL);
     if(!in_command && (command_boolean == COMMAND_FAIL))
        output(getdsc(player),player,0,1,0,ANSI_LCYAN"\nUSAGE:  "ANSI_LGREEN"to #<ROOM ID>\n");
}

/* ---->  Visit another character  <---- */
void move_visit(CONTEXT)
{
     dbref who,oldloc,cmd_cache = current_command;
     int   ic_cache = in_command;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(params)) {
        if((who = lookup_character(player,params,1)) != NOTHING) {
	   if(who != player) {
   	      if(Connected(who)) {
 	         if(db[who].location != db[player].location) {
                    in_command = 1, current_command = 0, oldloc = db[player].location;
                    sprintf(scratch_return_string,"*%s",getname(who));
                    move_teleport(player,NULL,NULL,"me",scratch_return_string,0,0);
                    in_command = ic_cache, current_command = cmd_cache;
                    if(!in_command) {
                       if(command_boolean != COMMAND_FAIL) {
                          sprintf(scratch_buffer,ANSI_LGREEN"You head over to %s"ANSI_LYELLOW"%s"ANSI_LGREEN" to visit ",Article(db[player].location,LOWER,DEFINITE),unparse_object(player,db[player].location,0));
                          output(getdsc(player),player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0));
                          output(getdsc(who),who,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" heads over to %s"ANSI_LYELLOW"%s"ANSI_LGREEN" to visit you.",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0),Article(db[player].location,LOWER,DEFINITE),getname(db[player].location));

                          if(!Invisible(oldloc)) {
		             if(!Secret(db[player].location) && !Secret(player)) {
                                sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" heads over to ",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0));
                                sprintf(scratch_buffer + strlen(scratch_buffer),"%s"ANSI_LYELLOW"%s"ANSI_LGREEN" to visit ",Article(db[player].location,LOWER,INDEFINITE),getname(db[player].location));
			     } else sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" heads over to a secret location to visit ",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0));
                             output_except(oldloc,player,NOTHING,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(who,LOWER,INDEFINITE),getcname(NOTHING,who,0,0));
			  }

                          if(!Invisible(db[player].location)) {
                             if(!Secret(oldloc) && !Secret(player)) {
                                sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" arrives from ",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0));
                                sprintf(scratch_buffer + strlen(scratch_buffer),"%s"ANSI_LYELLOW"%s"ANSI_LGREEN" to visit ",Article(oldloc,LOWER,DEFINITE),getname(oldloc));
			     } else sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" arrives from a secret location to visit ",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0));
                             output_except(db[player].location,player,who,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0));
			  }
		       } else output(getdsc(player),player,0,1,0,ANSI_LCYAN"\nUSAGE:  "ANSI_LGREEN"visit <NAME>\n        go <NAME>\n");
		    }
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you're already visiting %s"ANSI_LWHITE"%s"ANSI_LGREEN" (They're in the same room/container as you.)",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0));
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" isn't connected.",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0));
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't visit yourself.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",params);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the name of who you'd like to visit.");
}

/* ---->  Warp to random ABODE location  <---- */
void move_warp(CONTEXT)
{
     dbref                i,owner,warp = NOTHING,lower = NOTHING,upper = NOTHING;
     int                  counter = 0,result;
     static unsigned char direction = 0;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(params)) {
        owner = lookup_character(player,params,1);
        if(owner == NOTHING) {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",params);
           return;
	}
     } else owner = NOTHING;

     /* ---->  Find upper and lower range limits of ABODE+WARP locations  <---- */
     for(i = 0; i < db_top; i++)
         if(((Typeof(i) == TYPE_ROOM) || (Typeof(i) == TYPE_THING)) && Abode(i) && Warp(i)) {
            if(lower == NOTHING) lower = i;
            upper = i;
	 }

     /* ---->  Find random ABODE+WARP location  <---- */
     if(Valid(lower) && Valid(upper))
        for(i = lower + (lrand48() % ((upper - lower) + 1)), counter = (upper - lower) + 1, direction++; !Valid(warp) && (counter > 0); counter--) {
            if(direction % 2) {
               if(++i > upper) i = lower;
	    } else if(--i < lower) i = upper;
            if((i != db[player].location) && ((Typeof(i) == TYPE_ROOM) || (Typeof(i) == TYPE_THING)) && (!Valid(owner) || (db[i].owner == owner)) && Abode(i) && Warp(i) && (!(result = friend_flags(player,db[i].owner)) || (result & FRIEND_LINK))) warp = i;
	}

     /* ---->  Teleport to location  <---- */
     if(Valid(warp)) {
        dbref oldloc = db[player].location,cmd_cache = current_command;
        int   ic_cache = in_command;

        in_command = 1, current_command = 0;
        sprintf(scratch_return_string,"#%d",warp);
        move_teleport(player,NULL,NULL,"me",scratch_return_string,0,0);
        in_command = ic_cache, current_command = cmd_cache;
        if(!in_command) {
           if((command_boolean != COMMAND_FAIL) && (Location(player) == warp)) {
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"You click your fingers and disappear in a "ANSI_BLINK"flash"ANSI_LGREEN" of "ANSI_LWHITE"blinding light"ANSI_LGREEN".  You materialise in %s"ANSI_LYELLOW"%s"ANSI_LGREEN".",Article(warp,LOWER,INDEFINITE),unparse_object(player,warp,0));
              if(!Invisible(oldloc)) output_except(oldloc,player,NOTHING,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" clicks %s fingers and disappears in a "ANSI_BLINK"flash"ANSI_LGREEN" of "ANSI_LWHITE"blinding light"ANSI_LGREEN".",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0),Possessive(player,0));
              if(!Invisible(warp)) output_except(warp,player,NOTHING,0,1,0,ANSI_LGREEN"With a "ANSI_BLINK"flash"ANSI_LGREEN" of "ANSI_LWHITE"blinding light"ANSI_LGREEN", %s"ANSI_LYELLOW"%s"ANSI_LGREEN" materialises right in front of you, having warped to this location.",Article(player,LOWER,DEFINITE),getcname(NOTHING,player,0,0));
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"\nSorry, unable to warp to %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(warp,LOWER,INDEFINITE),unparse_object(player,warp,0));
	}
     } else if(Validchar(owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" has no%s locations that you can warp to.",Article(owner,LOWER,DEFINITE),getcname(NOTHING,owner,0,0),(Valid(Location(player)) && (Owner(Location(player)) == owner) && Abode(Location(player))) ? " other":"");
        else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, there are no locations that you can warp to.");
}
