/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| DESTROY.C  -  Implements destruction and recovery of objects.               |
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
| Module originally designed and written by:  J.P.Boggis 24/03/1995.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                        https://github.com/smcvey/tcz                        |
`-----------------------------------------------------------------------------'

*/


#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "structures.h"
#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "object_types.h"
#include "friend_flags.h"
#include "objectlists.h"
#include "flagset.h"
#include "search.h"
#include "fields.h"
#include "match.h"
#include "quota.h"


/* ---->  Is specified object part of given lock?  <---- */
int part_of_boolexp(dbref object,struct boolexp *a_boolexp)
{
    if(a_boolexp == NULL) return(0);
    switch(a_boolexp->type) {
           case BOOLEXP_AND:
           case BOOLEXP_OR:
                if(part_of_boolexp(object,a_boolexp->sub1) || part_of_boolexp(object,a_boolexp->sub2)) return(1);
           case BOOLEXP_NOT:
                return(part_of_boolexp(object,a_boolexp->sub1));
           case BOOLEXP_CONST:
                if(a_boolexp->object == object) return(1);
    }
    return(0);
}

/* ---->  Destroy given object (Removing/destroying other objects within it)  <---- */
unsigned char destroy_object(dbref player,dbref object,unsigned char char_ok,unsigned char log,unsigned char queue,unsigned char nested)
{
	 struct descriptor_data *p = getdsc(player);
	 int    cached_commandtype = command_type;
	 int    result,ok_to_destroy = 1;
	 dbref  ptr,cache;

	 /* ---->  Object set READONLY?  <---- */
	 command_type |= NO_USAGE_UPDATE;
	 if(Readonly(object)) {
	     output(p,player,0,1,0,ANSI_LRED"Sorry, %s"ANSI_LWHITE"%s"ANSI_LRED" is Read-Only  -  You can't %s %s.",Article(object,LOWER,(nested) ? INDEFINITE:DEFINITE),unparse_object(player,object,0),(queue) ? "destroy":/*"purge"*/"destroy",(Typeof(object) == TYPE_CHARACTER) ? "them":"it");
	    command_type = cached_commandtype;
	    return(0);
	 }

	 /* ---->  Object set PERMANENT?  <---- */
	 if(Permanent(object)) {
	     output(p,player,0,1,0,ANSI_LRED"Sorry, %s"ANSI_LWHITE"%s"ANSI_LRED" is permanent  -  You can't %s %s.",Article(object,LOWER,(nested) ? INDEFINITE:DEFINITE),unparse_object(player,object,0),(queue) ? "destroy":/*"purge"*/"destroy",(Typeof(object) == TYPE_CHARACTER) ? "them":"it");
	    command_type = cached_commandtype;
	    return(0);
	 }

	 /* ---->  Check that objects #0, #1, #3 & #4 aren't being destroyed (Required internally)  <---- */
	 if(RoomZero(object)) {
	     output(p,player,0,1,0,ANSI_LRED"Sorry, %s"ANSI_LWHITE"%s"ANSI_LRED" ("ANSI_LYELLOW"Room Zero"ANSI_LRED") can't be %s  -  It's required internally.",Article(object,LOWER,(nested) ? INDEFINITE:DEFINITE),unparse_object(player,object,0),(queue) ? "destroyed":/*"purged"*/"destroyed");
	    command_type = cached_commandtype;
	    return(0);
	 } else if(Root(object)) {
	     output(p,player,0,1,0,ANSI_LRED"Sorry, %s"ANSI_LWHITE"%s"ANSI_LRED" ("ANSI_LYELLOW"The Supreme Being"ANSI_LRED") can't be %s  -  They're required internally.",Article(object,LOWER,(nested) ? INDEFINITE:DEFINITE),unparse_object(player,object,0),(queue) ? "destroyed":/*"purged"*/"destroyed");
	    command_type = cached_commandtype;
	    return(0);
	 } else if(Start(object)) {
	     output(p,player,0,1,0,ANSI_LRED"Sorry, %s"ANSI_LWHITE"%s"ANSI_LRED" ("ANSI_LYELLOW"New Character Starting Room"ANSI_LRED") can't be %s  -  It's required internally.",Article(object,LOWER,(nested) ? INDEFINITE:DEFINITE),unparse_object(player,object,0),(queue) ? "destroyed":/*"purged"*/"destroyed");
	    command_type = cached_commandtype;
	    return(0);
	 } else if(Global(object)) {
	     output(p,player,0,1,0,ANSI_LRED"Sorry, %s"ANSI_LWHITE"%s"ANSI_LRED" ("ANSI_LYELLOW"Command Last Resort"ANSI_LRED") can't be %s  -  It's required internally.",Article(object,LOWER,(nested) ? INDEFINITE:DEFINITE),unparse_object(player,object,0),(queue) ? "destroyed":/*"purged"*/"destroyed");
	    command_type = cached_commandtype;
	    return(0);
	 } else if(object == maint_owner) {
	     output(p,player,0,1,0,ANSI_LRED"Sorry, %s"ANSI_LWHITE"%s"ANSI_LRED" ("ANSI_LYELLOW"Maintenance Owner"ANSI_LRED") can't be %s.",Article(object,LOWER,(nested) ? INDEFINITE:DEFINITE),getcname(player,object,1,0),(queue) ? "destroyed":/*"purged"*/"destroyed");
	    command_type = cached_commandtype;
	    return(0);
	 } else if(object == bbsroom) {
	     output(p,player,0,1,0,ANSI_LRED"Sorry, %s"ANSI_LWHITE"%s"ANSI_LRED" ("ANSI_LYELLOW"%s BBS"ANSI_LRED") can't be %s.",Article(object,LOWER,(nested) ? INDEFINITE:DEFINITE),unparse_object(player,object,0),tcz_full_name,(queue) ? "destroyed":/*"purged"*/"destroyed");
	    command_type = cached_commandtype;
	    return(0);
	 } else if(object == bankroom) {
	     output(p,player,0,1,0,ANSI_LRED"Sorry, %s"ANSI_LWHITE"%s"ANSI_LRED" ("ANSI_LYELLOW"The Bank of %s"ANSI_LRED") can't be %s.",Article(object,LOWER,(nested) ? INDEFINITE:DEFINITE),unparse_object(player,object,0),tcz_short_name,(queue) ? "destroyed":/*"purged"*/"destroyed");
	    command_type = cached_commandtype;
	    return(0);
	 }

	 if(Typeof(object) == TYPE_CHARACTER) {
	    if(!char_ok && !Level4(player)) {

	       /* ---->  Character being destroyed by non-Admin?  <---- */
		output(p,player,0,1,0,ANSI_LRED"Sorry, only Apprentice Wizards/Druids and above may %s characters.",(queue) ? "destroy":/*"purge"*/"destroy");
	       command_type = cached_commandtype;
	       return(0);
	    } else if(!char_ok && in_command) {

	       /* ---->  Character being destroyed in compound command?  <---- */
		output(p,player,0,1,0,ANSI_LRED"Sorry, a character can't be %s from within a compound command.",(queue) ? "destroyed":/*"purged"*/"destroyed");
	       command_type = cached_commandtype;
	       return(0);
	    } else if(Level4(object) || Experienced(object)) {

	       /* ---->  Experienced Builder or Apprentice Wizard/Druid or above being destroyed?  <---- */
		output(p,player,0,1,0,ANSI_LRED"Sorry, an Experienced Builder or an Apprentice Wizard/Druid or above can't be %s.",(queue) ? "destroyed":/*"purged"*/"destroyed");
	       command_type = cached_commandtype;
	       return(0);
	    } else if(!char_ok && Being(object)) {

	       /* ---->  'Being' character being destroyed?  <---- */
		output(p,player,0,1,0,ANSI_LRED"Sorry, a Being can't be %s.",(queue) ? "destroyed":/*"purged"*/"destroyed");
	       command_type = cached_commandtype;
	       return(0);
	    } else if(!char_ok && !Ashcan(object)) {

	       /* ---->  Non-ASHCAN'ed character being destroyed?  <---- */
		output(p,player,0,1,0,ANSI_LRED"Sorry, only characters with their "ANSI_LYELLOW"ASHCAN"ANSI_LRED" flag set can be %s.",(queue) ? "destroyed":/*"purged"*/"destroyed");
	       command_type = cached_commandtype;
	       return(0);
	    }
	 }

	 /* ---->  Deal with objects linked/CSUCC'ed, etc. to object being destroyed  <---- */
	 if(!((Typeof(object) == TYPE_VARIABLE) || (Typeof(object) == TYPE_PROPERTY) || (Typeof(object) == TYPE_ARRAY))) {
	    for(ptr = 0; ok_to_destroy && (ptr < db_top); ptr++)
		if((Typeof(ptr) != TYPE_FREE) && (ptr != object)) {
		   switch(Typeof(ptr)) {
			  case TYPE_CHARACTER:
			       if(db[ptr].destination == object) {
				  if(!in_command) output(p,player,0,1,0,ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED"'s home reset to %s"ANSI_LYELLOW"%s(#%d)"ANSI_LRED".",Article(ptr,UPPER,INDEFINITE),getcname(player,ptr,1,0),Article(ROOMZERO,LOWER,INDEFINITE),getname(ROOMZERO),ROOMZERO);
				  db[ptr].destination = ROOMZERO;
			       }
			       if((Typeof(object) == TYPE_CHARACTER) && ((Controller(ptr) == object) || (Partner(ptr) == object))) {
				  if(!in_command) {
				     sprintf(scratch_return_string,ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED" is no-longer %s of ",Article(ptr,UPPER,INDEFINITE),getcname(player,ptr,1,0),(Controller(ptr) == object) ? "a puppet":"the partner");
				     output(p,player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LRED".",scratch_return_string,Article(object,LOWER,(nested) ? INDEFINITE:DEFINITE),getcname(player,object,1,0));
				  }
				  db[ptr].data->player.controller = ptr;
			       }
			       break;
			  case TYPE_COMMAND:
			  case TYPE_FUSE:
			       if(Typeof(object) == TYPE_COMMAND) {
				  if(db[ptr].contents == object) {
				     sprintf(scratch_return_string,ANSI_LRED"Sorry, %s"ANSI_LWHITE"%s"ANSI_LRED"'s csuccess is linked to ",Article(ptr,LOWER,INDEFINITE),unparse_object(player,ptr,0));
				     output(p,player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LRED".",scratch_return_string,Article(object,LOWER,(nested) ? INDEFINITE:DEFINITE),unparse_object(player,object,0));
				     ok_to_destroy = 0;
				  }
				  if(db[ptr].exits == object) {
				     sprintf(scratch_return_string,ANSI_LRED"Sorry, %s"ANSI_LWHITE"%s"ANSI_LRED"'s cfailure is linked to ",Article(ptr,LOWER,INDEFINITE),unparse_object(player,ptr,0));
				     output(p,player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LRED".",scratch_return_string,Article(object,LOWER,(nested) ? INDEFINITE:DEFINITE),unparse_object(player,object,0));
				     ok_to_destroy = 0;
				  }
			       }
			       if(part_of_boolexp(object,getlock(ptr,0))) {
				  sprintf(scratch_return_string,ANSI_LRED"Sorry, %s"ANSI_LWHITE"%s"ANSI_LRED" uses ",Article(ptr,LOWER,INDEFINITE),unparse_object(player,ptr,0));
				  output(p,player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LRED" in its lock.",scratch_return_string,Article(object,LOWER,(nested) ? INDEFINITE:DEFINITE),unparse_object(player,object,0));
				  ok_to_destroy = 0;
			       }
			       break;
			  case TYPE_ALARM:
			       if((Typeof(object) == TYPE_COMMAND) && (db[ptr].destination == object)) {
				  sprintf(scratch_return_string,ANSI_LRED"Sorry, %s"ANSI_LWHITE"%s"ANSI_LRED"'s csuccess is linked to ",Article(ptr,LOWER,INDEFINITE),unparse_object(player,ptr,0));
				  output(p,player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LRED".",scratch_return_string,Article(object,LOWER,(nested) ? INDEFINITE:DEFINITE),unparse_object(player,object,0));
				  ok_to_destroy = 0;
			       }
			       break;
			  case TYPE_THING:
			       if(db[ptr].destination == object) {
				  if(!in_command) output(p,player,0,1,0,ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED"'s home reset to %s"ANSI_LYELLOW"%s(#%d)"ANSI_LRED".",Article(ptr,UPPER,INDEFINITE),unparse_object(player,ptr,0),Article(ROOMZERO,LOWER,INDEFINITE),getname(ROOMZERO),ROOMZERO);
				  db[ptr].destination = ROOMZERO;
			       }
			       if(part_of_boolexp(object,getlock(ptr,0))) {
				  sprintf(scratch_return_string,ANSI_LRED"Sorry, %s"ANSI_LWHITE"%s"ANSI_LRED" uses ",Article(ptr,LOWER,INDEFINITE),unparse_object(player,ptr,0));
				  output(p,player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LRED" in its lock.",scratch_return_string,Article(object,LOWER,(nested) ? INDEFINITE:DEFINITE),unparse_object(player,object,0));
				  ok_to_destroy = 0;
			       }
			       if(part_of_boolexp(object,getlock(ptr,1))) {
				  sprintf(scratch_return_string,ANSI_LRED"Sorry, %s"ANSI_LWHITE"%s"ANSI_LRED" uses ",Article(ptr,LOWER,INDEFINITE),unparse_object(player,ptr,0));
				  output(p,player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LRED" in its lock key.",scratch_return_string,Article(object,LOWER,(nested) ? INDEFINITE:DEFINITE),unparse_object(player,object,0));
				  ok_to_destroy = 0;
			       }
			       break;
			  case TYPE_ROOM:
			       if(db[ptr].destination == object) {
				  if(!in_command) output(p,player,0,1,0,ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED"'s drop-to reset.",Article(ptr,UPPER,INDEFINITE),unparse_object(player,ptr,0));
				  db[ptr].destination = NOTHING;
			       }
			       if(part_of_boolexp(object,getlock(ptr,0))) {
				  sprintf(scratch_return_string,ANSI_LRED"Sorry, %s"ANSI_LWHITE"%s"ANSI_LRED" uses ",Article(ptr,LOWER,INDEFINITE),unparse_object(player,ptr,0));
				  output(p,player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LRED" in its lock.",scratch_return_string,Article(object,LOWER,(nested) ? INDEFINITE:DEFINITE),unparse_object(player,object,0));
				  ok_to_destroy = 0;
			       }
			       break;
			  case TYPE_EXIT:
			       if(db[ptr].destination == object)
				  if(!(result = destroy_object(player,ptr,0,log,queue,1)))
				     db[ptr].destination = NOTHING;
			       if(part_of_boolexp(object,getlock(ptr,0))) {
				  sprintf(scratch_return_string,ANSI_LRED"Sorry, %s"ANSI_LWHITE"%s"ANSI_LRED" uses ",Article(ptr,LOWER,INDEFINITE),unparse_object(player,ptr,0));
				  output(p,player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LRED" in its lock.",scratch_return_string,Article(object,LOWER,(nested) ? INDEFINITE:DEFINITE),unparse_object(player,object,0));
				  ok_to_destroy = 0;
			       }
			       break;
			  default:
			       break;
		   }

		   /* ---->  Object parent of another object?  <---- */
		   if(db[ptr].parent == object) {
		      sprintf(scratch_return_string,ANSI_LRED"Sorry, %s"ANSI_LWHITE"%s"ANSI_LRED" is the parent of ",Article(object,LOWER,(nested) ? INDEFINITE:DEFINITE),unparse_object(player,object,0));
		      output(p,player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LRED".",scratch_return_string,Article(ptr,LOWER,INDEFINITE),unparse_object(player,ptr,0));
		      ok_to_destroy = 0;
		   }
		}

	    /* ---->  Continue searching for objects that use object being destroyed in their lock  <---- */
	    for(; ptr < db_top; ptr++) {

		/* ---->  Object part of another object's lock?  <---- */
		if((ptr != object) && HasField(ptr,LOCK) && part_of_boolexp(object,getlock(ptr,0))) {
		   sprintf(scratch_return_string,ANSI_LRED"Sorry, %s"ANSI_LWHITE"%s"ANSI_LRED" uses ",Article(ptr,LOWER,INDEFINITE),unparse_object(player,ptr,0));
		   output(p,player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LRED" in its lock.",scratch_return_string,Article(object,LOWER,(nested) ? INDEFINITE:DEFINITE),unparse_object(player,object,0));
		   ok_to_destroy = 0;
		}

		/* ---->  Object parent of another object?  <---- */
		if(db[ptr].parent == object) {
		   sprintf(scratch_return_string,ANSI_LRED"Sorry, %s"ANSI_LWHITE"%s"ANSI_LRED" is the parent of ",Article(object,LOWER,(nested) ? INDEFINITE:DEFINITE),unparse_object(player,object,0));
		   output(p,player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LRED".",scratch_return_string,Article(ptr,LOWER,INDEFINITE),unparse_object(player,ptr,0));
		   ok_to_destroy = 0;
		}
	    }

	    if(ok_to_destroy && (Typeof(object) == TYPE_CHARACTER) && Connected(object)) {
		output(p,player,0,1,0,ANSI_LRED"Sorry, %s"ANSI_LWHITE"%s"ANSI_LRED" can't be %s because they are still connected.",Article(object,LOWER,(nested) ? INDEFINITE:DEFINITE),getcname(player,object,1,0),(queue) ? "destroyed":/*"purged"*/"destroyed");
	       command_type = cached_commandtype;
	       return(0);
	    }
	 }

	 if(!ok_to_destroy) {
	     output(p,player,0,1,0,ANSI_LRED"Sorry, %s"ANSI_LWHITE"%s"ANSI_LRED" can't be %s for the above reason(s).",Article(object,LOWER,(nested) ? INDEFINITE:DEFINITE),getcname(player,object,1,0),(queue) ? "destroyed":/*"purged"*/"destroyed");
	    command_type = cached_commandtype;
	    return(0);
	 }

	 /* ---->  Remove everything from object's contents list  <---- */
	 if(HasList(object,CONTENTS) && Valid(db[object].contents))
	    for(ptr = db[object].contents; Valid(ptr); ptr = cache) {
		cache = Next(ptr);
		if(((Typeof(ptr) == TYPE_CHARACTER) || (Typeof(ptr) == TYPE_THING)) && will_fit(ptr,db[ptr].destination)) {
		   if(!in_command) output(p,player,0,1,0,ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED" sent to %s home.",Article(ptr,UPPER,INDEFINITE),getcname(player,ptr,1,0),(Typeof(ptr) == TYPE_CHARACTER) ? "their":"its");
		   move_to(ptr,db[ptr].destination);
		} else if(Typeof(ptr) != TYPE_ROOM) {
		   if(!in_command) output(p,player,0,1,0,ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED" sent to %s"ANSI_LYELLOW"%s(#%d)"ANSI_LRED".",Article(ptr,UPPER,INDEFINITE),unparse_object(player,ptr,0),Article(ROOMZERO,LOWER,INDEFINITE),getname(ROOMZERO),ROOMZERO);
		   move_to(ptr,ROOMZERO);
		} else {
		   if(!in_command) output(p,player,0,1,0,ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED" is now a floating room.",Article(ptr,UPPER,INDEFINITE),unparse_object(player,ptr,0));
		   move_to(ptr,ptr);
		}
	    }

	 /* ---->  Remove everything from object's compound commands list  <---- */
	 if(HasList(object,COMMANDS) && Valid(db[object].commands))
	    for(ptr = db[object].commands; Valid(ptr); ptr = cache) {
		cache = Next(ptr);
		if(!in_command) output(p,player,0,1,0,ANSI_LRED"Compound command "ANSI_LWHITE"%s"ANSI_LRED" set "ANSI_LYELLOW"INVISIBLE"ANSI_LRED" and sent to %s"ANSI_LYELLOW"%s(#%d)"ANSI_LRED".",unparse_object(player,ptr,0),Article(ROOMZERO,LOWER,INDEFINITE),getname(ROOMZERO),ROOMZERO);
		db[ptr].flags |= INVISIBLE;
		move_to(ptr,ROOMZERO);
	    }

	 /* ---->  Remove alarms and fuses from object's fuses list (Destroy fuses if possible)  <---- */
	 if(HasList(object,FUSES) && Valid(db[object].fuses))
	    for(ptr = db[object].fuses; Valid(ptr); ptr = cache) {
		cache = Next(ptr);
		if(Typeof(ptr) != TYPE_FUSE) {
		   if(!in_command) output(p,player,0,1,0,ANSI_LRED"Alarm "ANSI_LWHITE"%s"ANSI_LRED" sent to %s"ANSI_LYELLOW"%s(#%d)"ANSI_LRED".",unparse_object(player,ptr,0),Article(ROOMZERO,LOWER,INDEFINITE),getname(ROOMZERO),ROOMZERO);
		   move_to(ptr,ROOMZERO);
		   result = 0;
		} else if(!(result = destroy_object(player,ptr,0,log,queue,1))) {
		   if(!in_command) output(p,player,0,1,0,ANSI_LRED"Fuse "ANSI_LWHITE"%s"ANSI_LRED" sent to %s"ANSI_LYELLOW"%s(#%d)"ANSI_LRED".",unparse_object(player,ptr,0),Article(ROOMZERO,LOWER,INDEFINITE),getname(ROOMZERO),ROOMZERO);
		   move_to(ptr,ROOMZERO);
		}
	    }

	 /* ---->  Destroy exits attached to object  <---- */
	 if(HasList(object,EXITS) && Valid(db[object].exits))
	    for(ptr = db[object].exits; Valid(ptr); ptr = cache) {
		cache = Next(ptr);
		if(!(result = destroy_object(player,ptr,0,log,queue,1))) {
		   if(!in_command) output(p,player,0,1,0,ANSI_LRED"Exit %s"ANSI_LWHITE"%s"ANSI_LRED" set "ANSI_LYELLOW"INVISIBLE"ANSI_LRED" and sent to %s"ANSI_LYELLOW"%s(#%d)"ANSI_LRED".",Article(ptr,LOWER,INDEFINITE),unparse_object(player,ptr,0),Article(ROOMZERO,LOWER,INDEFINITE),getname(ROOMZERO),ROOMZERO);
		   db[ptr].flags |= INVISIBLE;
		   move_to(ptr,ROOMZERO);
		}
	    }

	 /* ---->  Destroy variables, properties and dynamic arrays attached to object  <---- */
	 if(HasList(object,VARIABLES) && Valid(db[object].variables))
	    for(ptr = db[object].variables; Valid(ptr); ptr = cache) {
		cache = Next(ptr);
		if(!(result = destroy_object(player,ptr,0,log,queue,1))) {
		   if(!in_command) {
		      if(Typeof(ptr) == TYPE_ARRAY)
			 output(p,player,0,1,0,ANSI_LRED"Dynamic array "ANSI_LWHITE"%s"ANSI_LRED" sent to %s"ANSI_LYELLOW"%s(#%d)"ANSI_LRED".",unparse_object(player,ptr,0),Article(ROOMZERO,LOWER,INDEFINITE),getname(ROOMZERO),ROOMZERO);
			    else output(p,player,0,1,0,ANSI_LRED"%s "ANSI_LWHITE"%s"ANSI_LRED" sent to %s"ANSI_LYELLOW"%s(#%d)"ANSI_LRED".",(Typeof(ptr) == TYPE_VARIABLE) ? "Variable":"Property",unparse_object(player,ptr,0),Article(ROOMZERO,LOWER,INDEFINITE),getname(ROOMZERO),ROOMZERO);
		   }
		   move_to(ptr,ROOMZERO);
		}
	    }

	 /* ---->  Remove object from whatever list it's in  <---- */
	 switch(WhichList(object)) {
		case VARIABLES:
		     if(Valid(db[object].location) && HasList(db[object].location,VARIABLES) && member(object,db[db[object].location].variables))
			db[db[object].location].variables = remove_first(db[db[object].location].variables,object);
		     break;
		case CONTENTS:
		     if(Valid(db[object].location) && HasList(db[object].location,CONTENTS) && member(object,db[db[object].location].contents))
			db[db[object].location].contents = remove_first(db[db[object].location].contents,object);
		     break;
		case COMMANDS:
		     if(Valid(db[object].location) && HasList(db[object].location,COMMANDS) && member(object,db[db[object].location].commands))
			db[db[object].location].commands = remove_first(db[db[object].location].commands,object);
		     break;
		case EXITS:
		     if(Valid(db[object].location) && HasList(db[object].location,EXITS) && member(object,db[db[object].location].exits))
			db[db[object].location].exits = remove_first(db[db[object].location].exits,object);
		     break;
		case FUSES:
		     if(Valid(db[object].location) && HasList(db[object].location,FUSES) && member(object,db[db[object].location].fuses))
			db[db[object].location].fuses = remove_first(db[db[object].location].fuses,object);
		     break;
		default:
		     break;
	 }

	 /* ---->  Remove alarm/fuse from event queue  <---- */
	 if((Typeof(object) == TYPE_ALARM) || ((Typeof(object) == TYPE_FUSE) && Sticky(object)))
	    event_remove(object);

	 /* ---->  Reimburse Building Quota used by object  <---- */
	 adjustquota(player,db[object].owner,0 - ((Typeof(object) != TYPE_ARRAY) ? ObjectQuota(object):(ObjectQuota(object) + (array_element_count(db[object].data->array.start) * ELEMENT_QUOTA))));

	 /* ---->  If object being destroyed is a character, change ownership of everything they own to MAINT_OWNER (Maintenance owner, as set in '@admin')  <---- */
	 if(Typeof(object) == TYPE_CHARACTER) {
	    unsigned char             objects = 0, topics = 0, messages = 0;
	    struct   bbs_topic_data   *subtopic,*topic;
	    char                      lfbuffer[KB];
	    FILE                      *lf = NULL;
	    struct   bbs_message_data *message;
	    dbref                     newowner;
	    struct   mail_data        *mail;
	    int                       quota;

	    /* ---->  Transfer character's possesions (Set ASHCAN) to MAINT_OWNER (Or destroyer, if MAINT_OWNER is invalid)  <---- */
	    newowner = (Validchar(maint_owner)) ? maint_owner:db[player].owner;
	    for(ptr = 0; ptr < db_top; ptr++)
		if(Typeof(ptr) != TYPE_CHARACTER) {
		   if(db[ptr].owner == object) {
		      quota = ObjectQuota(ptr);
		      if(Typeof(ptr) == TYPE_ARRAY)
			 quota += array_element_count(db[ptr].data->array.start) * ELEMENT_QUOTA;
		      db[newowner].data->player.quota += quota;
		      db[ptr].owner  = newowner;
		      db[ptr].flags |= ASHCAN;
		      objects        = 1;
		   }

		   /* ---->  Set WHO of mail from character being destroyed to NOTHING  <---- */
		} else for(mail = db[ptr].data->player.mail; mail; mail = mail->next)
		   if(mail->who == object) mail->who = NOTHING;

	    if(objects && !in_command) {
	       sprintf(scratch_return_string,ANSI_LRED"\nOwner of all %s"ANSI_LWHITE"%s"ANSI_LRED"'s objects changed to ",Article(object,LOWER,(nested) ? INDEFINITE:DEFINITE),getcname(player,object,1,0));
	       output(p,player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LRED".  These objects have all been set "ANSI_LYELLOW"ASHCAN"ANSI_LRED"  -  You can use '"ANSI_LWHITE"@list *%s = ashcan"ANSI_LRED"' to list them (You should check out all "ANSI_LYELLOW"ASHCAN"ANSI_LRED"'ed compound commands and fuses.)\n",scratch_return_string,Article(newowner,LOWER,INDEFINITE),getcname(player,newowner,1,0),getname(newowner));
	    }

	    /* ---->  Transfer character's BBS messages and topics  <---- */
	    for(topic = bbs; topic; topic = topic->next) {
		for(message = topic->messages; message; message = message->next)
		    if(message->owner == object) message->owner = newowner, messages = 1;
		if(topic->owner == object) topic->owner = newowner, topics = 1;

		if(topic->subtopics)
		   for(subtopic = topic->subtopics; subtopic; subtopic = subtopic->next) {
		       for(message = subtopic->messages; message; message = message->next)
			   if(message->owner == object) message->owner = newowner, messages = 1;
		       if(subtopic->owner == object) subtopic->owner = newowner, topics = 1;
		   }
	    }
	    if(!in_command && (messages || topics)) {
	       sprintf(scratch_return_string,ANSI_LRED"%sOwner of all %s"ANSI_LWHITE"%s"ANSI_LRED"'s BBS messages%s changed to ",(objects) ? "":"\n",Article(object,LOWER,(nested) ? INDEFINITE:DEFINITE),getcname(player,object,1,0),(topics) ? " and topics/sub-topics":"");
	       output(p,player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LRED".\n",scratch_return_string,Article(newowner,LOWER,INDEFINITE),getcname(player,newowner,1,0));
	    }


	    /* ---->  Delete user log file  <---- */
	    sprintf(lfbuffer,"log/users/user_%d.log",object);
	    if((lf = fopen(lfbuffer,"r"))) {
	       fclose(lf);
	       if(unlink(lfbuffer))
		  writelog(DESTROY_LOG,1,"DESTROY","Unable to delete %s(#%d)'s log file '%s' (%s.)",getname(object),object,lfbuffer,strerror(errno));
	    }
	 }

	 /* ---->  'Hard' destroy object (Adding to queue of destroyed objects, if QUEUE == 1)  <---- */
	 if(log) {
	    if(db[player].owner != db[object].owner) {
	       if(in_command && Valid(current_command))
		   writelog(DESTROY_LOG,1,(queue) ? "DESTROY":/*"PURGE"*/"DESTROY","%s(#%d) %s %s (%s, owned by %s(#%d)) within compound command %s(#%d).",getname(player),player,(queue) ? "destroyed":/*"purged"*/"destroyed",unparse_object(ROOT,object,0),names[Typeof(object)],getname(db[object].owner),db[object].owner,getname(current_command),current_command);
	       else writelog(DESTROY_LOG,1,(queue) ? "DESTROY":/*"PURGE"*/"DESTROY","%s(#%d) %s %s (%s, owned by %s(#%d).)",getname(player),player,(queue) ? "destroyed":/*"purged"*/"destroyed",unparse_object(ROOT,object,0),names[Typeof(object)],getname(db[object].owner),db[object].owner);
	    }
	 }

	 if(!in_command) output(p,player,0,1,0,ANSI_LRED"%s "ANSI_LWHITE"%s"ANSI_LRED" %s.",names[Typeof(object)],unparse_object(player,object,0),(queue) ? "destroyed":/*"purged"*/"destroyed");
	 if(Valid(object)) delete_object(object,1,queue);
	 command_type = cached_commandtype;
	 return(1);
}

/* ---->  Destroy specified object  <---- */
/*        (VAL1:  0 = '@destroy', 1 = '@purge'.)  */
void destroy_destroy(CONTEXT)
{
     unsigned char queue = (val1) ? 0:1,log = 0,result;
     char     *verify = NULL,*reason = NULL;
     dbref    object;
     int      count;

     /* ---->  Nothing specified to destroy?  <---- */
     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(arg1)) {
        object = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
        if(!Valid(object)) return;

        if(object != player) {
           if(object != db[player].owner) {
              if((result = can_write_to(player,object,0)) && ((result != 2) || (!in_command && friendflags_set(db[object].owner,player,object,FRIEND_DESTROY)))) {

                 /* ---->  Must type '@destroy <OBJECT> = yes [= <REASON>]' to destroy a character/objects owned by another character  <---- */
                 if(Typeof(object) == TYPE_CHARACTER) {
                    split_params((char *) arg2,&verify,&reason);
                    if(Blank(verify) || Blank(reason) || strcasecmp("yes",verify)) {
			output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you must type '"ANSI_LYELLOW"@%s <NAME> = yes = <REASON>"ANSI_LGREEN"' to %s %s"ANSI_LWHITE"%s"ANSI_LGREEN".",(val1) ? /*"purge"*/"destroy":"destroy",(queue) ? "destroy":/*"purge"*/"destroy",Article(object,LOWER,DEFINITE),getcname(player,object,1,0));
                       return;
		    }
                    strcpy(scratch_buffer,getname(object)), log = 1;
		 } else if(!Blank(arg2) && (queue = !string_prefix("permanent",arg2))) {
                    if(strcasecmp("yes",arg2)) {
			output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@%s"ANSI_LGREEN"' only accepts a single parameter (The object which you'd like to %s.)  If you're trying to %s an object which doesn't belong to you, you must type '"ANSI_LWHITE"@%s <OBJECT> = yes"ANSI_LGREEN"' to %s it.\n",(val1) ? /*"purge"*/"destroy":"destroy",(queue) ? "destroy":/*"purge"*/"destroy",(queue) ? "destroy":/*"purge"*/"destroy",(val1) ? /*"purge"*/"destroy":"destroy",(queue) ? "destroy":/*"purge"*/"destroy");
                       return;
		    }
		 } else if(!in_command && (db[object].owner != db[player].owner)) {
		     output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" doesn't belong to you  -  You must type '"ANSI_LYELLOW"@%s <OBJECT> = yes"ANSI_LGREEN"' to %s an object which doesn't belong to you.\n",Article(object,LOWER,DEFINITE),unparse_object(player,object,0),(val1) ? /*"purge"*/"destroy":"destroy",(queue) ? "destroy":/*"purge"*/"destroy");
                    return;
		 }
                 if(!queue && !((in_command && (Valid(current_command) && (Owner(current_command) == Owner(object)))) || (!in_command && (Owner(object) == player)))) queue = 1;

                 /* ---->  Destroy object or given elements of dynamic array  <---- */
                 if((Typeof(object) == TYPE_ARRAY) && (elementfrom != NOTHING)) {
                    if(!Readonly(object)) {
                       if(elementfrom != INVALID) {
                          array_destroy_elements(player,object,elementfrom,elementto,&count);
                          if(!in_command) {
                             sprintf(scratch_buffer,ANSI_LGREEN"Element%s %s of dynamic array ",((elementto == UNSET) || ((elementfrom == elementto) && !((elementto == INDEXED) && !BlankContent(indexto)))) ? "":"s",array_unparse_element_range(elementfrom,elementto,ANSI_LGREEN));
                             output(getdsc(player),player,0,1,0,"%s"ANSI_LWHITE"%s"ANSI_LGREEN" %s.",scratch_buffer,unparse_object(player,object,0),(queue) ? "destroyed":/*"purged"*/"destroyed");
			  }
                          if(count > 0) setreturn(OK,COMMAND_SUCC);
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that element (Or range of elements) is invalid.");
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that dynamic array is Read-Only  -  You can't %s any of its elements.",(queue) ? "destroy":/*"purge"*/"destroy");
		 } else if(destroy_object(player,object,0,1,queue,0)) {
                    if(log) {
                       verify = punctuate(reason,2,'.');
                       writelog(ADMIN_LOG,1,(queue) ? "DESTROY":/*"PURGE"*/"DESTROY","%s(#%d) %s character %s(#%d)  -  REASON:  %s",getname(player),player,(queue) ? "destroyed":/*"purged"*/"destroyed",scratch_buffer,object,verify);
		    }
                    setreturn(OK,COMMAND_SUCC);
		 }
	      } else if((Typeof(object) == TYPE_ARRAY) && (elementfrom != NOTHING)) {
		  if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only %s elements of a dynamic array you own or one that's owned by someone of a lower level than yourself.",(queue) ? "destroy":/*"purge"*/"destroy");
		  else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only %s elements of a dynamic array you own.",(queue) ? "destroy":/*"purge"*/"destroy");
	      } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only %s something you own or something owned by someone of a lower level than yourself.",(queue) ? "destroy":/*"purge"*/"destroy");
	      else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only %s something you own.",(queue) ? "destroy":/*"purge"*/"destroy");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't %s the owner of the compound command currently being executed.",(queue) ? "destroy":/*"purge"*/"destroy");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"If you no-longer need your character, please ask a Wizard/Druid or above to %s them for you ("ANSI_LWHITE"PLEASE NOTE"ANSI_LGREEN":  Once your character has been %s, there will be no way of retrieving them, or their possessions.)",(queue) ? "destroy":/*"purge"*/"destroy",(queue) ? "destroyed":/*"purged"*/"destroyed");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify what you'd like to %s.",(queue) ? "destroy":/*"purge"*/"destroy");
}

/* ---->  Does given boolean expression use objects owned by SUBJECT  <---- */
int check_boolexp(dbref subject,struct boolexp *a_boolexp)
{
    if(a_boolexp == NULL) return(0);
    switch(a_boolexp->type) {
           case BOOLEXP_AND:
           case BOOLEXP_OR:
                if(check_boolexp(subject,a_boolexp->sub1) || part_of_boolexp(subject,a_boolexp->sub2)) return(1);
           case BOOLEXP_NOT:
                return(part_of_boolexp(subject,a_boolexp->sub1));
           case BOOLEXP_CONST:
                if(Valid(a_boolexp->object) && (Typeof(a_boolexp->object) != TYPE_CHARACTER) && (Owner(a_boolexp->object) == subject)) {
                   db[a_boolexp->object].flags &= ~OBJECT;
                   return(1);
		}
    }
    return(0);
}

/* ---->  Attempt to destroy ALL objects owned by the specified character  <---- */
void destroy_destroyall(CONTEXT)
{
     int    alarms = 0,arrays = 0,characters = 0,commands = 0,exits = 0;
     int    fuses = 0,properties = 0,rooms = 0,things = 0,variables = 0;
     char   *verify = NULL,*reason = NULL;
     dbref  subject,cache,ptr,tmp;
     int    count = 0,after = 0;
     const  char *nptr = arg1;

     command_type |= NO_USAGE_UPDATE;
     if(!in_command) {
        if(Level3(player)) {
           setreturn(ERROR,COMMAND_FAIL);
           if(!Blank(nptr)) for(; *nptr && (*nptr == '*'); nptr++);
           if(!Blank(nptr)) {
              sprintf(scratch_return_string,"*%s",nptr);
              if((subject = lookup_character(player,scratch_return_string,1)) == NOTHING) {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist (Please specify the "ANSI_LYELLOW"FULL NAME"ANSI_LGREEN" of the character who's objects you'd like to destroy.)\n",arg1);
                 command_type &= ~NO_USAGE_UPDATE;
                 return;
	      }
	   } else {
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the "ANSI_LYELLOW"FULL NAME"ANSI_LGREEN" of the character who's objects you'd like to destroy.\n");
              command_type &= ~NO_USAGE_UPDATE;
              return;
	   }

           if(player != subject) {
              if(Ashcan(subject)) {
                 if(!Level4(subject)) {
                    if(!(Experienced(subject) || Assistant(subject))) {
                       if(can_write_to(player,subject,0)) {

                          /* ---->  Reason given?  <---- */
                          split_params((char *) arg2,&verify,&reason);
                          if(Blank(verify) || Blank(reason) || strcasecmp("yes",verify)) {
                             output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you must type '"ANSI_LWHITE"@destroyall <NAME> = yes = <REASON>"ANSI_LGREEN"' to destroy all of the objects owned by a character.");
                             command_type &= ~NO_USAGE_UPDATE;
                             return;
		          }

                          /* ---->  Set OBJECT flag of objects owned by SUBJECT which aren't set PERMANENT/READONLY  <---- */
                          for(ptr = 0; ptr < db_top; ptr++)
                              if((Typeof(ptr) != TYPE_FREE) && (Typeof(ptr) != TYPE_CHARACTER)) {
                                 if(Owner(ptr) == subject) {
                                    if(!Permanent(ptr) && !Readonly(ptr))
                                       db[ptr].flags |= OBJECT;
                                    count++;
				 } else db[ptr].flags &= ~OBJECT;
       			      } else db[ptr].flags &= ~OBJECT;

                          /* ---->  Check objects in database not owned by SUBJECT, ensuring they don't use objects owned by SUBJECT  <---- */
                          for(ptr = 0; ptr < db_top; ptr++)
                              if((Typeof(ptr) != TYPE_FREE) && (Owner(ptr) != subject)) {
                                 switch(Typeof(ptr)) {
                                        case TYPE_COMMAND:
                                        case TYPE_FUSE:
                                             if(Valid(db[ptr].contents) && (db[db[ptr].contents].owner == subject)) db[db[ptr].contents].flags &= ~OBJECT;
                                             if(Valid(db[ptr].exits)    && (db[db[ptr].exits].owner == subject)) db[db[ptr].exits].flags &= ~OBJECT;
                                             check_boolexp(subject,getlock(ptr,0));
                                             break;
                                        case TYPE_THING:
                                             check_boolexp(subject,getlock(ptr,0));
                                             check_boolexp(subject,getlock(ptr,1));
                                             break;
                                        case TYPE_ROOM:
                                             check_boolexp(subject,getlock(ptr,0));
                                             break;
                                        case TYPE_EXIT:
                                             check_boolexp(subject,getlock(ptr,0));
                                             break;
				 }
                                 if(Valid(db[ptr].destination) && (db[db[ptr].destination].owner == subject)) db[db[ptr].destination].flags &= ~OBJECT;
                                 if(Valid(db[ptr].parent)      && (db[db[ptr].parent].owner == subject)) db[db[ptr].parent].flags &= ~OBJECT;
			      }

                          /* ---->  'Hard' destroy all objects which have their OBJECT flag set  <---- */
                          for(ptr = 0; ptr < db_top; ptr++)
                              if(Object(ptr) && (Typeof(ptr) != TYPE_FREE) && (Typeof(ptr) != TYPE_CHARACTER) && (db[ptr].owner == subject)) {

                                 /* ---->  Move objects in object's contents list to #0 (ROOMZERO)  <---- */
                                 if(HasList(ptr,CONTENTS) && Valid(db[ptr].contents))
                                    for(tmp = db[ptr].contents; Valid(tmp); tmp = cache) {
                                        cache = Next(tmp);
                                        move_to(tmp,ROOMZERO);
				    }

                                 /* ---->  Move compound commands attached to object to #0 (ROOMZERO)  <---- */
                                 if(HasList(ptr,COMMANDS) && Valid(db[ptr].commands))
                                    for(tmp = db[ptr].commands; Valid(tmp); tmp = cache) {
                                        cache = Next(tmp);
                                        db[ptr].flags |= INVISIBLE;
                                        move_to(tmp,ROOMZERO);
				    }

                                 /* ---->  Move alarms and fuses attached to object to #0 (ROOMZERO)  <---- */
                                 if(HasList(ptr,FUSES) && Valid(db[ptr].fuses))
                                    for(tmp = db[ptr].fuses; Valid(tmp); tmp = cache) {
                                        cache = Next(tmp);
                                        move_to(tmp,ROOMZERO);
				    }

                                 /* ---->  Move exits in object to #0 (ROOMZERO)  <---- */
                                 if(HasList(ptr,EXITS) && Valid(db[ptr].exits))
                                    for(tmp = db[ptr].exits; Valid(tmp); tmp = cache) {
                                        cache = Next(tmp);
                                        db[ptr].flags |= INVISIBLE;
                                        move_to(tmp,ROOMZERO);
				    }

                                 /* ---->  Move variables, properties and dynamic arrays attached to object to #0 (ROOMZERO)  <---- */
                                 if(HasList(ptr,VARIABLES) && Valid(db[ptr].variables))
                                    for(tmp = db[ptr].variables; Valid(tmp); tmp = cache) {
                                        cache = Next(tmp);
                                        move_to(tmp,ROOMZERO);
				    }

                                 /* ---->  Remove object from whatever list it's in  <---- */
                                 switch(WhichList(ptr)) {
                                        case VARIABLES:
                                             if(Valid(db[ptr].location) && HasList(db[ptr].location,VARIABLES) && member(ptr,db[db[ptr].location].variables))
                                                db[db[ptr].location].variables = remove_first(db[db[ptr].location].variables,ptr);
                                             break;
                                        case CONTENTS:
                                             if(Valid(db[ptr].location) && HasList(db[ptr].location,CONTENTS) && member(ptr,db[db[ptr].location].contents))
                                                db[db[ptr].location].contents = remove_first(db[db[ptr].location].contents,ptr);
                                             break;
                                        case COMMANDS:
                                             if(Valid(db[ptr].location) && HasList(db[ptr].location,COMMANDS) && member(ptr,db[db[ptr].location].commands))
                                                db[db[ptr].location].commands = remove_first(db[db[ptr].location].commands,ptr);
                                             break;
                                        case EXITS:
                                             if(Valid(db[ptr].location) && HasList(db[ptr].location,EXITS) && member(ptr,db[db[ptr].location].exits))
                                                db[db[ptr].location].exits = remove_first(db[db[ptr].location].exits,ptr);
                                             break;
                                        case FUSES:
                                             if(Valid(db[ptr].location) && HasList(db[ptr].location,FUSES) && member(ptr,db[db[ptr].location].fuses))
                                                db[db[ptr].location].fuses = remove_first(db[db[ptr].location].fuses,ptr);
                                             break;
				 }

                                 /* ---->  Update object destruction statistics  <---- */
                                 switch(Typeof(ptr)) {
                                        case TYPE_PROPERTY:
                                             properties++;
                                             break;
                                        case TYPE_VARIABLE:
                                             variables++;
                                             break;
                                        case TYPE_COMMAND:
                                             commands++;
                                             break;
                                        case TYPE_CHARACTER:
                                             characters++;
                                             break;
                                        case TYPE_ALARM:
                                             alarms++;
                                             break;
                                        case TYPE_ARRAY:
                                             arrays++;
                                             break;
                                        case TYPE_THING:
                                             things++;
                                             break;
                                        case TYPE_EXIT:
                                             exits++;
                                             break;
                                        case TYPE_FUSE:
                                             fuses++;
                                             break;
                                        case TYPE_ROOM:
                                             rooms++;
                                             break;
				 }

                                 /* ---->  Reimburse Building Quota used by object  <---- */
                                 adjustquota(player,db[ptr].owner,0 - ((Typeof(ptr) != TYPE_ARRAY) ? ObjectQuota(ptr):(ObjectQuota(ptr) + (array_element_count(db[ptr].data->array.start) * ELEMENT_QUOTA))));

                                 /* ---->  'Hard' destroy object  <---- */
                                 delete_object(ptr,1,0);
			      }

                          /* ---->  Sanitise remaining objects owned by SUBJECT  <---- */
                          for(ptr = 0; ptr < db_top; ptr++)
                              if((Typeof(ptr) != TYPE_FREE) && (Owner(ptr) == subject)) {
                                 switch(Typeof(ptr)) {
                                        case TYPE_COMMAND:
                                        case TYPE_FUSE:
                                             if(Valid(db[ptr].contents) && (Typeof(db[ptr].contents) == TYPE_FREE)) db[ptr].contents = NOTHING;
                                             if(Valid(db[ptr].exits)    && (Typeof(db[ptr].exits) == TYPE_FREE)) db[ptr].exits = NOTHING;
                                             if(Typeof(ptr) == TYPE_COMMAND) db[ptr].data->command.lock = sanitise_boolexp(db[ptr].data->command.lock);
                                                else db[ptr].data->fuse.lock = sanitise_boolexp(db[ptr].data->fuse.lock);
                                             break;
                                        case TYPE_THING:
                                             db[ptr].data->thing.lock_key = sanitise_boolexp(db[ptr].data->thing.lock_key);
                                             db[ptr].data->thing.lock     = sanitise_boolexp(db[ptr].data->thing.lock);
                                             break;
                                        case TYPE_ROOM:
                                             db[ptr].data->room.lock = sanitise_boolexp(db[ptr].data->room.lock);
                                             break;
                                        case TYPE_EXIT:
                                             db[ptr].data->exit.lock = sanitise_boolexp(db[ptr].data->exit.lock);
                                             break;
				 }
                                 if(Valid(db[ptr].destination) && (Typeof(db[ptr].destination) == TYPE_FREE)) db[ptr].destination = NOTHING;
                                 if(Valid(db[ptr].parent)      && (Typeof(db[ptr].parent) == TYPE_FREE)) db[ptr].parent = NOTHING;
                                 if(Valid(db[ptr].location)    && (Typeof(db[ptr].location) == TYPE_FREE)) {
                                    if((Typeof(ptr) == TYPE_COMMAND) || (Typeof(ptr) == TYPE_EXIT)) db[ptr].flags |= INVISIBLE;
                                    move_to(ptr,ROOMZERO);
				 }
			      }

                          /* ---->  Set OBJECT flag of all objects in DB again  <---- */
                          for(ptr = 0; ptr < db_top; ptr++)
                              if(Typeof(ptr) != TYPE_FREE) {
                                 if((Typeof(ptr) != TYPE_CHARACTER) && (db[ptr].owner == subject)) after++;
                                 db[ptr].flags |= OBJECT;
		  	      }

                          /* ---->  Result(s) of '@destroyall'  <---- */
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"\nAll of %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s objects have been destroyed ("ANSI_LYELLOW"%d"ANSI_LGREEN" object%s destroyed ("ANSI_LCYAN"%d"ANSI_LGREEN" alarm%s, "ANSI_LCYAN"%d"ANSI_LGREEN" character%s, "ANSI_LCYAN"%d"ANSI_LGREEN" compound command%s, "ANSI_LCYAN"%d"ANSI_LGREEN" dynamic array%s, "ANSI_LCYAN"%d"ANSI_LGREEN" exit%s, "ANSI_LCYAN"%d"ANSI_LGREEN" fuse%s, "ANSI_LCYAN"%d"ANSI_LGREEN" propert%s, "ANSI_LCYAN"%d"ANSI_LGREEN" room%s, "ANSI_LCYAN"%d"ANSI_LGREEN" thing%s, "ANSI_LCYAN"%d"ANSI_LGREEN" variable%s), "ANSI_LYELLOW"%d"ANSI_LGREEN" couldn't be destroyed.)\n",Article(subject,LOWER,DEFINITE),getcname(NOTHING,subject,0,0),count - after,Plural(count - after),alarms,Plural(alarms),characters,Plural(characters),commands,Plural(commands),arrays,Plural(arrays),exits,Plural(exits),fuses,Plural(fuses),properties,(properties == 1) ? "y":"ies",rooms,Plural(rooms),things,Plural(things),variables,Plural(variables),after);
                          verify = punctuate(reason,2,'.');
                          writelog(ADMIN_LOG,1,"DESTROY ALL","%s(#%d) destroyed all objects owned by %s(#%d)  -  %d object%s destroyed (%d alarm%s, %d character%s, %d compound command%s, %d dynamic array%s, %d exit%s, %d fuse%s, %d propert%s, %d room%s, %d thing%s, %d variable%s), %d couldn't be destroyed  -  REASON:  %s",getname(player),player,getname(subject),subject,count - after,Plural(count - after),alarms,Plural(alarms),characters,Plural(characters),commands,Plural(commands),arrays,Plural(arrays),exits,Plural(exits),fuses,Plural(fuses),properties,(properties == 1) ? "y":"ies",rooms,Plural(rooms),things,Plural(things),variables,Plural(variables),after,verify);
                          writelog(DESTROY_LOG,1,"DESTROY ALL","%s(#%d) destroyed all objects owned by %s(#%d)  -  %d object%s destroyed (%d alarm%s, %d character%s, %d compound command%s, %d dynamic array%s, %d exit%s, %d fuse%s, %d propert%s, %d room%s, %d thing%s, %d variable%s), %d couldn't be destroyed  -  REASON:  %s",getname(player),player,getname(subject),subject,count - after,Plural(count - after),alarms,Plural(alarms),characters,Plural(characters),commands,Plural(commands),arrays,Plural(arrays),exits,Plural(exits),fuses,Plural(fuses),properties,(properties == 1) ? "y":"ies",rooms,Plural(rooms),things,Plural(things),variables,Plural(variables),after,verify);
                          setreturn(OK,COMMAND_SUCC);
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only destroy all the objects owned by someone of a lower level than yourself.");
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't destroy everything owned by an Experienced Builder, Assistant or a Retired Admin.");
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't destroy everything owned by an Apprentice Wizard/Druid or above.");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only destroy all the objects of a character who's set "ANSI_LYELLOW"ASHCAN"ANSI_LGREEN".");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't destroy everything YOU own.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Wizards/Druids and above may destroy all the objects of a character.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, all of the objects owned by a character can't be destroyed from within a compound command.");
     command_type &= ~NO_USAGE_UPDATE;
}

/* ---->  Given object may be recovered?  <---- */
unsigned char can_recover(dbref player,dbref object,dbref original_owner,unsigned long checksum,unsigned char destroyed,unsigned char sub,unsigned char link)
{
	 unsigned char result;

	 if(!destroyed) {
	    if(Valid(object) && (Typeof(object) != TYPE_FREE)) {
	       if(checksum) return((db[object].checksum == checksum) ? object:NOTHING);
		  else return(((link && (((link == 1) && can_link_or_home_to(player,object)) || ((link == 2) && Inheritable(object)))) || can_write_to(player,object,0)) ? object:NOTHING);
	    } else {
	       struct destroy_data *ptr;

	       for(ptr = destroy_queue; ptr && (ptr->id != object); ptr = ptr->next);
	       if(ptr && !ptr->recovered) {
		  int frflags = ((ptr->obj.type) == TYPE_COMMAND) ? (FRIEND_WRITE|FRIEND_CREATE|FRIEND_COMMANDS):(FRIEND_WRITE|FRIEND_CREATE);

		  if(checksum) return((ptr->obj.checksum == checksum) && ((!Validchar(ptr->obj.owner) && Level4(player)) || (((result = can_write_to(player,ptr->obj.owner,0)) == 1) || (((result == 2) || ((ptr->obj.type) != TYPE_CHARACTER)) && (friendflags_set(ptr->obj.owner,player,NOTHING,frflags) == frflags)))));
		     else return((!Validchar(ptr->obj.owner) && Level4(player)) || (((result = can_write_to(player,ptr->obj.owner,0)) == 1) || (((result == 2) || ((ptr->obj.type) != TYPE_CHARACTER)) && (friendflags_set(ptr->obj.owner,player,NOTHING,frflags) == frflags))));
	       } else return(sub && ((ptr && ptr->recovered) || (!ptr && Valid(object) && (Typeof(object) != TYPE_FREE))));
	    }
	 } else return((!Validchar(original_owner) && Level4(player)) || (((result = can_write_to(player,original_owner,0)) == 1) || ((result == 2) && (friendflags_set(original_owner,player,NOTHING,FRIEND_WRITE|FRIEND_CREATE) == (FRIEND_WRITE|FRIEND_CREATE)))));
	 return(0);
}

/* ---->  Recover object with specified original #ID (If possible), returning #ID of recovered object  <---- */
dbref recover_object(dbref player,dbref object,dbref owner,unsigned char sub,unsigned char *cr)
{
      struct destroy_data *ptr;
      dbref  new;

      for(ptr = destroy_queue; ptr && (ptr->id != object); ptr = ptr->next);
      if(ptr && !ptr->recovered) {
         if(adjustquota(player,owner,ptr->quota)) {

            /* ---->  Recreate object, preferably with original #ID  <---- */
            if(Typeof(object) == TYPE_FREE) {
               if(Valid(db_free_chain)) {
                  dbref cobj,last = NOTHING;

                  for(cobj = db_free_chain; Valid(cobj) && (cobj != object); last = cobj, cobj = Next(cobj));
                  if(Valid(cobj)) {
                     if(Valid(last)) {
                        db[last].next = Next(cobj);
                        if(db_free_chain_end == cobj) db_free_chain_end = last;
		     } else {
                        db_free_chain = Next(cobj);
                        if(db_free_chain_end == cobj) db_free_chain_end = NOTHING;
		     }
                     initialise_object(db + (new = cobj));
		  } else new = new_object();
	       } else new = new_object();
	    } else new = new_object();

            /* ---->  Restore original fields of object  <---- */
            ptr->recovered  = 1;
            db[new].created = ptr->obj.created;
            db[new].flags2  = ptr->obj.flags2;
            db[new].flags   = ptr->obj.flags;
            db[new].data    = ptr->obj.data;
            db[new].name    = ptr->obj.name;
	    db[new].type    = ptr->obj.type;
            gettime(db[new].lastused);
            db[new].flags  &= ~ASHCAN;

            /* ---->  Zero Building Quota currently in use (Recovered character shouldn't have any objects yet.)  <---- */
            if(Typeof(new) == TYPE_CHARACTER) db[new].data->player.quota = 0;

            /* ---->  Original location of object  <---- */
            if(can_recover(player,ptr->obj.location,0,ptr->checksum.location,0,1,0)) {
               db[new].location = recover_object(player,ptr->obj.location,owner,1,cr);
               if(!Valid(db[new].location) && (Typeof(new) != TYPE_ROOM)) db[new].location = ROOMZERO;
	    } else if(Typeof(new) != TYPE_ROOM) db[new].location = ROOMZERO;
            if(Valid(db[new].location))
               switch(Typeof(new)) {
                      case TYPE_CHARACTER:
                      case TYPE_THING:
                      case TYPE_ROOM:
                           PUSH(new,db[db[new].location].contents);
                           break;
                      case TYPE_EXIT:
                           PUSH(new,db[db[new].location].exits);
                           break;
                      case TYPE_COMMAND:
                           PUSH(new,db[db[new].location].commands);
                           if(Global(effective_location(new))) global_add(new);
                           break;
                      case TYPE_ALARM:
                      case TYPE_FUSE:
                           PUSH(new,db[db[new].location].fuses);
                           break;
                      case TYPE_PROPERTY:
                      case TYPE_VARIABLE:
                      case TYPE_ARRAY:
                           PUSH(new,db[db[new].location].variables);
                           break;
	       }

            /* ---->  Original owner of object  <---- */
            if(Typeof(new) != TYPE_CHARACTER) {
               if(owner == player) {
                  if(can_recover(player,ptr->obj.owner,0,ptr->checksum.owner,0,1,0))
                     db[new].owner = recover_object(player,ptr->obj.owner,owner,1,cr);
	       } else if(!((ptr->obj.type) == TYPE_COMMAND) || friendflags_set(owner,player,NOTHING,FRIEND_COMMANDS)) db[new].owner = owner;
                  else db[new].owner = player;
	    } else db[new].owner = new;

            /* ---->  Home/destination of object  <---- */
            if(can_recover(player,ptr->obj.destination,0,ptr->checksum.destination,0,1,1)) {
               db[new].destination = recover_object(player,ptr->obj.destination,owner,1,cr);
               if(!Valid(db[new].destination) && ((Typeof(new) == TYPE_ROOM) || (Typeof(new) == TYPE_THING) || (Typeof(new) == TYPE_CHARACTER))) db[new].destination = ROOMZERO;
	    } else if(((Typeof(new) == TYPE_ROOM) || (Typeof(new) == TYPE_THING) || (Typeof(new) == TYPE_CHARACTER))) db[new].destination = ROOMZERO;

            /* ---->  Parent of object  <---- */
            if(can_recover(player,ptr->obj.parent,0,ptr->checksum.parent,0,1,2))
               db[new].parent = recover_object(player,ptr->obj.parent,owner,1,cr);

            /* ---->  CSUCC/CFAIL (Compound command/fuse)  <---- */
            if((Typeof(new) == TYPE_COMMAND) || (Typeof(new) == TYPE_FUSE)) {
               if(can_recover(player,ptr->obj.contents,0,ptr->checksum.contents,0,1,0))
                  db[new].contents = recover_object(player,ptr->obj.contents,owner,1,cr);
               if(can_recover(player,ptr->obj.exits,0,ptr->checksum.exits,0,1,0))
                  db[new].exits = recover_object(player,ptr->obj.exits,owner,1,cr);
	    } else if(Typeof(new) == TYPE_CHARACTER) {

               /* ---->  Controller  <---- */
               if(can_recover(player,ptr->obj.data->player.controller,0,ptr->checksum.controller,0,1,0)) {
                  db[new].data->player.controller = recover_object(player,ptr->obj.data->player.controller,owner,1,cr);
                  if(!Valid(db[new].data->player.controller)) db[new].data->player.controller = new;
	       } else db[new].data->player.controller = new;

               /* ---->  Mail redirect  <---- */
               if(can_recover(player,ptr->obj.data->player.redirect,0,ptr->checksum.redirect,0,1,2))
                  db[new].data->player.redirect = recover_object(player,ptr->obj.data->player.redirect,owner,1,cr);

               /* ---->  UID (Who character is currently building as)  <---- */
               if(can_recover(player,ptr->obj.data->player.uid,0,ptr->checksum.uid,0,1,0)) {
                  db[new].data->player.uid = recover_object(player,ptr->obj.data->player.uid,owner,1,cr);
                  if(!Valid(db[new].data->player.uid)) db[new].data->player.uid = new;
	       } else db[new].data->player.uid = new;
	    }

            /* ---->  Check consistency of lock  <---- */
            switch(Typeof(new)) {
                   case TYPE_EXIT:
                        db[new].data->exit.lock = sanitise_boolexp(db[new].data->exit.lock);
                        break;
                   case TYPE_FUSE:
                        db[new].data->fuse.lock = sanitise_boolexp(db[new].data->fuse.lock);
                        break;
                   case TYPE_ROOM:
                        db[new].data->room.lock = sanitise_boolexp(db[new].data->room.lock);
                        break;
                   case TYPE_THING:
                        db[new].data->thing.lock     = sanitise_boolexp(db[new].data->thing.lock);
                        db[new].data->thing.lock_key = sanitise_boolexp(db[new].data->thing.lock_key);
                        break;
                   case TYPE_COMMAND:
                        db[new].data->command.lock = sanitise_boolexp(db[new].data->command.lock);
                        break;
	    }

            /* ---->  Attempt to recover destroyed objects originally located in object  <---- */
            for(ptr = destroy_queue; ptr; ptr = ptr->next)
                if(!ptr->recovered && (ptr->obj.location == object))
                   if(can_recover(player,ptr->id,ptr->obj.owner,0,1,1,0))
                      recover_object(player,ptr->id,owner,1,cr);

            if(new == object) output(getdsc(player),player,0,1,0,"%s%s%s "ANSI_LWHITE"%s%s recovered.",(*cr) ? "\n":"",(sub) ? ANSI_LYELLOW:ANSI_LGREEN,names[Typeof(new)],getcname(player,new,1,UPPER|INDEFINITE),(sub) ? ANSI_LYELLOW:ANSI_LGREEN), *cr = 0;
	       else output(getdsc(player),player,0,1,0,"%s%s%s "ANSI_LWHITE"%s%s recovered (An object with the ID "ANSI_LWHITE"#%d%s already exists  -  Object has been allocated the new ID "ANSI_LWHITE"#%d%s.)",(*cr) ? "\n":"",(sub) ? ANSI_LYELLOW:ANSI_LGREEN,names[Typeof(new)],getcname(player,new,1,UPPER|INDEFINITE),object,(sub) ? ANSI_LYELLOW:ANSI_LGREEN,new,(sub) ? ANSI_LYELLOW:ANSI_LGREEN), *cr = 0;
            return(new);
	 } else output(getdsc(player),player,0,1,0,"%s%sUnable to recover object "ANSI_LWHITE"#%d%s (Insufficient Building Quota.)",(*cr) ? "\n":"",(sub) ? ANSI_LYELLOW:ANSI_LGREEN,object,(sub) ? ANSI_LYELLOW:ANSI_LGREEN), *cr = 0;
      } else if(sub && ((ptr && ptr->recovered) || (!ptr && (Typeof(object) != TYPE_FREE)))) return(object);
      return(NOTHING);
}

/* ---->  Attempt to undestroy given object (Remove it from destroy queue), or list objects on destroy queue  <---- */
void destroy_undestroy(CONTEXT)
{
     struct descriptor_data *p = getdsc(player);

     setreturn(ERROR,COMMAND_FAIL);
     command_type |= NO_USAGE_UPDATE;
     if(!in_command) {
        if(!Blank(arg1) && (!strcasecmp("list",arg1) || !strcasecmp("view",arg1) || !strncasecmp("list ",arg1,5) || !strncasecmp("view ",arg1,5))) {
           unsigned char cr = 1,cached_scrheight,twidth = output_terminal_width(player);
           char     *objecttypes,*ownername,*ptr,*tmp;
           int      object_flags = 0,result;
           dbref    owner;

           /* ---->  List objects on destroy queue  <---- */
           if(!Blank(arg1)) for(arg1 += 4; *arg1 && (*arg1 == ' '); arg1++);
           arg1 = (char *) parse_grouprange(player,arg1,FIRST,1);
           split_params((char *) arg2,&objecttypes,&ownername);

           /* ---->  List destroyed objects owned by specified character  <---- */
           if(!Blank(ownername)) {
              if((owner = lookup_character(player,ownername,1)) == NOTHING) {
                 output(p,player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",ownername);
                 command_type &= ~NO_USAGE_UPDATE;
                 return;
	      }

              if(!Level4(player) && !Experienced(player) && !can_write_to(player,owner,0) && !friendflags_set(owner,player,NOTHING,FRIEND_READ)) {
                 output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only list objects you own that have been destroyed recently.");
                 command_type &= ~NO_USAGE_UPDATE;
                 return;
	      }
	   } else owner = NOTHING;

           /* ---->  List destroyed objects of given type(s) only?  <---- */
           if(!Blank(objecttypes)) {
              ptr = objecttypes;
              while(*ptr) {
                    while(*ptr && (*ptr == ' ')) ptr++;
                    if(*ptr) {
                       for(tmp = scratch_buffer; *ptr && (*ptr != ' '); *tmp++ = *ptr++);
                       *tmp = '\0';

                       if(!(result = parse_objecttype(scratch_buffer))) {
                          if(strcasecmp("all",scratch_buffer)) output(getdsc(player),player,0,1,0,"%s"ANSI_LGREEN"Sorry, '"ANSI_LWHITE"%s"ANSI_LGREEN"' is an unknown object type.",(cr) ? "\n":"",scratch_buffer), cr = 0;
                             else object_flags = SEARCH_ALL_TYPES;
		       } else object_flags |= result;
		    }
	      }
	   }

           if(!(object_flags & SEARCH_ALL_OBJECTS)) object_flags |= SEARCH_ALL_OBJECTS;
           if(p && !p->pager && Validchar(p->player) && More(p->player)) pager_init(p);
           set_conditions(player,0,0,object_flags,owner,arg1,506);
           cached_scrheight                   = db[player].data->player.scrheight;
           db[player].data->player.scrheight -= 6;
           union_initgrouprange((union group_data *) destroy_queue);

           if(!in_command) {
              output(p,player,0,1,0,"\n #ID:         Name, object type and original owner:");
              output(p,player,0,1,0,separator(twidth,0,'-','='));
	   }

           if(grp->distance > 0) {
              while(union_grouprange())
                    output(p, player, 2, 1, 14, ANSI_LYELLOW " #%-12d" ANSI_LWHITE "%s  " ANSI_DCYAN "(" ANSI_LMAGENTA "%s" ANSI_DCYAN ", " ANSI_LGREEN "%s" ANSI_DCYAN ")\n", grp->cunion->destroy.id, grp->cunion->destroy.obj.name, names[((grp->cunion->destroy.obj.type))], (Validchar(grp->cunion->destroy.obj.owner)) ? getcname(player, grp->cunion->destroy.obj.owner, 1, UPPER|INDEFINITE) : "*NO OWNER*");

              if(!in_command) {
                 output(p,player,0,1,0,separator(twidth,0,'-','='));
	         output(p, player, 2, 1, 0, ANSI_LWHITE " Destroyed objects listed:  " ANSI_DWHITE "%s   " ANSI_LWHITE "Queue size:  " ANSI_DWHITE "%d/%d.\n\n", listed_items(scratch_return_string, 1), destroy_queue_size, DESTROY_QUEUE_SIZE);
	      }
	   } else {
              output(p, player, 2, 1, 0, ANSI_LCYAN " ***  NO DESTROYED OBJECTS FOUND  ***\n");
              if(!in_command) {
                 output(p,player,0,1,0,separator(twidth,0,'-','='));
                 output(p, player, 2, 1, 0, ANSI_LWHITE " Destroyed objects listed:  " ANSI_DWHITE "None.   " ANSI_LWHITE "Queue size:  " ANSI_DWHITE "%d/%d.\n\n", destroy_queue_size, DESTROY_QUEUE_SIZE);
	      }
	   }
           db[player].data->player.scrheight = cached_scrheight;
           setreturn(OK,COMMAND_SUCC);
	} else if(!Blank(arg1)) {
           struct   destroy_data *dest,*next,*last = NULL;
           dbref                 object, owner, recovered;
           unsigned short        count = 0;
           const    char         *ptr;
           char                  *tmp;
           unsigned char         cr;

           /* ----> Attempt to undestroy given object (Remove it from destroy queue)  <---- */
           if(!Blank(arg2)) {
              if((owner = lookup_character(player,arg2,1)) == NOTHING) {
                 output(p,player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg2);
                 command_type &= ~NO_USAGE_UPDATE;
                 return;
	      }

              if(!Level4(player) && !can_write_to(player,owner,1) && (friendflags_set(owner,player,NOTHING,FRIEND_WRITE|FRIEND_CREATE) != (FRIEND_WRITE|FRIEND_CREATE))) {
                 if(Level3(player)) output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only recover objects and change their owner to yourself or a lower level character.");
                    else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only recover objects and change their owner to yourself.");
                 command_type &= ~NO_USAGE_UPDATE;
                 return;
	      }
	   } else owner = player;

           /* ---->  Set all objects in destroy queue to !recovered  <---- */
           for(dest = destroy_queue; dest; dest->recovered = 0, dest = dest->next);

           /* ---->  Process list of #<ID>'s specified by user, attempting to recover each object  <---- */
           ptr = arg1;
           while(*ptr) {

                 /* ---->  Get next #<ID>  <---- */
                 *scratch_buffer = '\0';
                 for(; *ptr && (*ptr != '#'); ptr++);
                 for(; *ptr && (*ptr == '#'); ptr++);
                 if(*ptr) {
                    for(tmp = scratch_buffer; *ptr && isdigit(*ptr); *tmp++ = *ptr++);
                    *tmp = '\0';
		 }
                 cr = 1;

                 /* ---->  Attempt to recover given object  <---- */
                 if(!BlankContent(scratch_buffer)) {
                    if((object = atol(scratch_buffer)) != 0) {
                       if(can_recover(player,object,NOTHING,0,0,0,0)) {
                          recovered = recover_object(player,object,owner,0,&cr);
                          if(!Valid(recovered)) output(p,player,0,1,0,ANSI_LGREEN"\nSorry, the object "ANSI_LWHITE"#%d"ANSI_LGREEN" is not on the queue of destroyed objects and cannot be recovered.",object);
		       } else if(Level3(player)) output(p,player,0,1,0,ANSI_LGREEN"\nSorry, you can only recover objects which were originally owned by yourself or someone of a lower level.");
                          else output(p,player,0,1,0,ANSI_LGREEN"\nSorry, you can only recover objects which were originally owned by yourself.");
		    } else output(p,player,0,1,0,ANSI_LGREEN"\nSorry, the object ID '"ANSI_LWHITE"%s"ANSI_LGREEN"' is invalid.  Please specify the original #ID number(s) of the object(s) you'd like to recover (E.g:  '"ANSI_LYELLOW"@undestroy #12345"ANSI_LGREEN"'.)",scratch_buffer);
		 }
	   }

           /* ---->  Remove objects from queue of destroyed objects which have been recovered  <---- */
           for(dest = destroy_queue; dest; dest = next) {
               next = dest->next;
               if(dest->recovered) {
                  if(last) {
                     last->next = dest->next;
                     if(dest->next) dest->next->prev = last;
                        else destroy_queue_tail = last;
		  } else destroy_queue = dest->next;
                  destroy_queue_size--;
                  FREENULL(dest);
                  count++;
	       } else last = dest;
	   }

           if(count) {
              output(p,player,0,1,0,ANSI_LWHITE"\n%d"ANSI_LGREEN" object%s recovered.\n",count,Plural(count));
              setreturn(OK,COMMAND_SUCC);
	   } else output(p,player,0,1,0,ANSI_LGREEN"\nSorry, unable to recover the specified object(s).\n");
	} else output(p,player,0,1,0,ANSI_LGREEN"Please specify the original #ID (Or a list of #ID's) of the object(s) you would like to recover (Type '"ANSI_LYELLOW"@undestroy list [<NAME>]"ANSI_LGREEN"' if you are unsure.)");
     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@undestroy"ANSI_LGREEN"' can't be used from within a compound command.");
     command_type &= ~NO_USAGE_UPDATE;
}
