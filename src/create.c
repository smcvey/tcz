/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| CREATE.C  -  Implements creation of objects of various types.               |
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
|            Additional major coding by:  J.P.Boggis 05/08/1994.              |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                        https://github.com/smcvey/tcz                        |
`-----------------------------------------------------------------------------'

*/


#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "structures.h"
#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "descriptor_flags.h"
#include "object_types.h"
#include "friend_flags.h"
#include "flagset.h"
#include "fields.h"
#include "search.h"
#include "match.h"
#include "quota.h"


/* ---->  Create an alarm  <---- */
dbref create_alarm(CONTEXT)
{
      dbref alarm,owner = (!in_command && (Uid(player) != player)) ? Uid(player):Owner(player);

      setreturn(ERROR,COMMAND_FAIL);
      if(Level3(Owner(player))) {
	 if(!Blank(arg1)) {
	    if(strlen(arg1) <= 128) {
	       if(ok_name(arg1)) {
		  if(adjustquota(player,owner,ALARM_QUOTA)) {

		     /* ---->  Create and initialise alarm  <---- */
		     alarm                 = new_object();
		     db[alarm].destination = NOTHING;
		     db[alarm].location    = player;
		     db[alarm].flags       = OBJECT;
		     db[alarm].owner       = owner;
		     db[alarm].type        = TYPE_ALARM;

		     if(!in_command && (Uid(player) == owner) && friendflags_set(owner,player,NOTHING,FRIEND_SHARABLE))
			db[alarm].flags |= SHARABLE;

		     ansi_code_filter((char *) arg1,arg1,1);
		     initialise_data(alarm);
		     setfield(alarm,NAME,arg1,1);
		     setfield(alarm,DESC,arg2,0);
		     stats_tcz_update_record(0,0,0,1,0,0,0);
		     PUSH(alarm,db[player].fuses);

		     if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Alarm "ANSI_LWHITE"%s"ANSI_LGREEN" created with ID "ANSI_LYELLOW"#%d"ANSI_LGREEN".",getfield(alarm,NAME),alarm);
			else writelog(HACK_LOG,1,"HACK","Alarm %s(%d) created within compound command %s(%d) owned by %s(%d).",getname(alarm),alarm,getname(current_command),current_command,getname(Owner(current_command)),Owner(current_command));
		     setreturn(getnameid(player,alarm,NULL),COMMAND_SUCC);
                     return(alarm);
		  } else warnquota(player,owner,"to build an alarm");
	       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, an alarm can't have that name.");
	    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of an alarm's name is 128 characters.");
	 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify a name for the new alarm.");
      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Wizards/Druids and above can create an alarm.");
      return(NOTHING);
}

/* ---->  Create a dynamic array  <---- */
dbref create_array(CONTEXT)
{
      dbref array,elements,count,new,owner = (!in_command && (Uid(player) != player)) ? Uid(player):Owner(player);

      setreturn(ERROR,COMMAND_FAIL);
      if(Builder(Owner(player))) {
	 if(!Blank(arg1)) {
       if(!strchr(arg1,'\n')) {
	    if(strlen(arg1) <= 128) {
	       if(!strchr(arg1,'[')) {
		  if(ok_name(arg1)) {
		     if(adjustquota(player,owner,ARRAY_QUOTA)) {
			if((elements = Blank(arg2) ? 0:atol(arg2)) >= 0) {

			   /* ---->  Create and initialise dynamic array  <---- */
			   array                 = new_object();
			   db[array].destination = NOTHING;
			   db[array].location    = player;
			   db[array].flags       = OBJECT;
			   db[array].owner       = owner;
			   db[array].type        = TYPE_ARRAY;

			   if(!in_command && (Uid(player) == owner) && friendflags_set(owner,player,NOTHING,FRIEND_SHARABLE))
			      db[array].flags |= SHARABLE;

			   ansi_code_filter((char *) arg1,arg1,1);
			   initialise_data(array);
			   setfield(array,NAME,arg1,1);
			   strcpy(scratch_return_string,(char *) getfield(array,NAME));
			   stats_tcz_update_record(0,0,0,1,0,0,0);
			   PUSH(array,db[player].variables);

			   /* ---->  Initialise dynamic array elements  <---- */
			   switch(array_set_elements(player,array,elements,elements,NULL,0,&count,&new)) {
				  case ARRAY_INSUFFICIENT_QUOTA:
				       warnquota(player,Owner(array),"to initialise a dynamic array with that number of elements");
				       break;
				  case ARRAY_TOO_MANY_ELEMENTS:
				  case ARRAY_TOO_MANY_BLANKS:
				       output(getdsc(player),player,0,1,0,ANSI_LRED"Sorry, a dynamic array can't be initialised to more than "ANSI_LWHITE"%d"ANSI_LRED" elements.",ARRAY_LIMIT);
				       break;
				  default:
				       break;
			   }

			   if(!in_command) {
			      if(new > 0) sprintf(scratch_buffer,ANSI_LWHITE" %d"ANSI_LGREEN" element%s and",new,Plural(new));
				 else *scratch_buffer = '\0';
			      output(getdsc(player),player,0,1,0,ANSI_LGREEN"Dynamic array "ANSI_LWHITE"%s"ANSI_LGREEN" created with%s ID "ANSI_LYELLOW"#%d"ANSI_LGREEN".",scratch_return_string,scratch_buffer,array);
			   }
			   setreturn(getnameid(player,array,NULL),COMMAND_SUCC);
                           return(array);
			} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a dynamic array can't be initialised to a negative number of elements.");
		     } else warnquota(player,owner,"to build a dynamic array");
		  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a dynamic array can't have that name.");
	       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the name of a dynamic array mustn't contain the character '"ANSI_LWHITE"["ANSI_LGREEN"'.");
	    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of a dynamic array's name is 128 characters.");
			   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the name of a dynamic array mustn't contain embedded NEWLINES.");
	 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify a name for the new dynamic array.");
      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Builders can create dynamic arrays.");
      return(NOTHING);
}

/* ---->  Create a compound command  <---- */
dbref create_command(CONTEXT)
{
      dbref command,owner = (!in_command && (Uid(player) != player)) ? Uid(player):(in_command && !Level4(Owner(current_command)) && !Level4(current_command)) ? Owner(current_command):Owner(player);

      setreturn(ERROR,COMMAND_FAIL);
      if(Builder(Owner(player))) {
	 if(!Blank(arg1)) {
       if(!strchr(arg1,'\n')) {
	    if(strlen(arg1) <= 256) {
	       if(ok_name(arg1)) {
		  if(adjustquota(player,owner,COMMAND_QUOTA)) {
		     if(!(in_command && (Owner(player) != Owner(current_command)) && (level_app(Owner(player)) > level_app(Owner(current_command))))) {
			if(in_command || (Uid(player) == player) || friendflags_set(owner,player,NOTHING,FRIEND_COMMANDS)) {

			   /* ---->  Create and initialise compound command  <---- */
			   command                 = new_object();
			   db[command].destination = NOTHING;
			   db[command].location    = player;
			   db[command].flags       = OBJECT|CENSOR;
			   db[command].owner       = owner;
			   db[command].type        = TYPE_COMMAND;

			   if(!in_command && (Uid(player) == owner) && friendflags_set(owner,player,NOTHING,FRIEND_SHARABLE))
			   db[command].flags |= SHARABLE;

			   ansi_code_filter((char *) arg1,arg1,1);
			   initialise_data(command);
			   setfield(command,NAME,arg1,1);
			   setfield(command,DESC,arg2,0);
			   stats_tcz_update_record(0,0,0,1,0,0,0);
			   PUSH(command,db[player].commands);

			   /* ---->  Set CSUCC/CFAIL to HOME if '.enter'/'.leave'/'.login'/'.logout' compound command  <---- */
			   if((*arg1 == '.') && (!strcasecmp(".enter",arg1) || !strcasecmp(".leave",arg1) || !strcasecmp(".login",arg1) || !strcasecmp(".logout",arg1))) {
			      db[command].contents = HOME;
                              db[command].exits    = HOME;
			   }

			   /* ---->  Warn of possible hacking  <---- */
			   if(in_command && (Owner(player) != Owner(current_command)) && (Owner(command) != Owner(current_command)) && (level_app(Owner(player)) >= level_app(Owner(current_command)))) {
			      if(!Wizard(current_command)) writelog(HACK_LOG,1,"HACK","Compound command %s(#%d) created with %s(#%d)'s ownership within compound command %s(#%d) (Owned by %s(#%d).)",getname(command),command,getname(Owner(player)),Owner(player),getname(current_command),current_command,getname(Owner(current_command)),Owner(current_command));
			      sprintf(scratch_buffer,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"]  "ANSI_LWHITE"Compound command "ANSI_LYELLOW"%s"ANSI_LWHITE" created with your ownership from within compound command ",unparse_object(player,command,0));
			      sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LYELLOW"%s"ANSI_LWHITE" (Owned by ",unparse_object(Owner(current_command),current_command,0));
			      output(getdsc(player),player,0,1,11,"%s%s"ANSI_LYELLOW"%s"ANSI_LWHITE".)",scratch_buffer,Article(Owner(current_command),LOWER,INDEFINITE),getcname(player,Owner(current_command),1,0));
			   }

			   if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Compound command "ANSI_LWHITE"%s"ANSI_LGREEN" created with ID "ANSI_LYELLOW"#%d"ANSI_LGREEN".",getfield(command,NAME),command);
			   setreturn(getnameid(player,command,NULL),COMMAND_SUCC);
                           return(command);
			} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't build compound commands under the ownership of %s"ANSI_LWHITE"%s"ANSI_LGREEN" unless %s has granted you permission via the "ANSI_LYELLOW"COMMANDS"ANSI_LGREEN" friend flag.",Article(owner,LOWER,DEFINITE),getcname(NOTHING,owner,0,0),Subjective(owner,0));
		     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a compound command can't be created under the ID of a higher level character within a compound command.");
		  } else warnquota(player,owner,"to build a compound command");
	       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a compound command can't have that name.");
	    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of a compound command's name is 256 characters.");
	  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the name of a compound command mustn't contain embedded NEWLINES.");
	 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify a name for the new compound command.");
      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Builders can create compound commands.");
      return(NOTHING);
}

/* ---->  Create a character (Admin only)  <---- */
dbref create_character(CONTEXT)
{
      dbref character;

      setreturn(ERROR,COMMAND_FAIL);
      comms_spoken(player,1);

      if(!in_command) {
	 if(Level4(Owner(player))) {
	    if(!Blank(arg1)) {
         if(!strchr(arg1,'\n')) {
	       if(!Blank(arg2)) {
		  ansi_code_filter((char *) arg1,arg1,1);
		  filter_spaces((char *) arg1,(char *) arg1,1);
		  switch(ok_character_name(player,NOTHING,arg1)) {
			 case 0:
			      switch(ok_password(arg2)) {
				     case 0:
					  character = create_new_character(arg1,arg2,0);
					  if(Valid(character)) {
					     stats_tcz_update_record(0,0,1,0,0,0,0);
					     writelog(CREATE_LOG,1,"CREATED","%s(#%d) by %s(#%d) (Using '@character'.)",getname(character),character,getname(player),player);
					     output(getdsc(player),player,0,1,0,ANSI_LGREEN"Character '"ANSI_LWHITE"%s"ANSI_LGREEN"' created with ID "ANSI_LYELLOW"#%d"ANSI_LGREEN" and password '"ANSI_LYELLOW"%s"ANSI_LGREEN"'.",getname(character),character,arg2);
#ifdef HOME_ROOMS
                                             create_homeroom(character,0,1,0);
#endif
					     setreturn(getnameid(player,character,NULL),COMMAND_SUCC);
                                             return(character);
					  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, couldn't create a new character with the name '"ANSI_LWHITE"%s"ANSI_LGREEN"' and password '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",arg1,arg2);
					  break;
				     case 3:
					  output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the password for the new character must be at least 6 characters in length.");
					  break;
				     case 5:
					  output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the password for the new character can't be blank.");
					  break;
				     default:
					  output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the password '"ANSI_LWHITE"%s"ANSI_LGREEN"' is invalid.",arg2);
			      }
			      break;
			 case 2:
			      output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of a new character's name is 20 characters.");
			      break;
			 case 3:
			      output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the name for the new character must be at least 4 characters in length.");
			      break;
			 case 4:
			      output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a character with the name '"ANSI_LWHITE"%s"ANSI_LGREEN"' already exists.",arg1);
			      break;
			 case 5:
			      output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the name for the new character can't be blank.");
			      break;
			 case 6:
			      output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the name '"ANSI_LWHITE"%s"ANSI_LGREEN"' is not allowed.",arg1);
			      break;
			 default:
			      output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the name '"ANSI_LWHITE"%s"ANSI_LGREEN"' is invalid.",arg1);
		  }
	       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify a password for the new character.");
		} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the name of a character mustn't contain embedded NEWLINES.");
	    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify a name for the new character.");
	 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may create new characters.");
      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a new character can't be created from within a compound command.");
      return(NOTHING);
}

/* ---->  Duplicate lock of object  <---- */
struct boolexp *create_duplicate_lock(struct boolexp *bool)
{
       struct boolexp *ptr;

       if(bool == TRUE_BOOLEXP) return(TRUE_BOOLEXP);
       if((ptr = (struct boolexp *) malloc(sizeof(struct boolexp))) == NULL)
          abortmemory("(create_duplicate_lock() in create.c)  Insufficient memory to create duplicate lock.");

       ptr->object = bool->object;
       ptr->type   = bool->type;
       ptr->sub1   = (bool->sub1) ? (struct boolexp *) create_duplicate_lock(bool->sub1):NULL;
       ptr->sub2   = (bool->sub2) ? (struct boolexp *) create_duplicate_lock(bool->sub2):NULL;
       return(ptr);
}

/* ---->  Duplicate specified object (Optionally giving it a different name)  <---- */
dbref create_duplicate(CONTEXT)
{
      dbref    object,new,cached_parent,newowner;
      unsigned char ok = 1,inherit = 0;
      char     *newname,*inheritable;
      int      count;

      setreturn(ERROR,COMMAND_FAIL);
      if(Builder(Owner(player))) {
	 split_params((char *) arg2,&newname,&inheritable);
	 if(!Blank(inheritable)) {
	    if(string_prefix("inheritable",inheritable)) inherit = 1;
	 } else if(!strcasecmp("inherit",newname) || !strcasecmp("inheritable",newname)) inherit = 1, newname = "";

	 object = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
	 if(!Valid(object)) return(NOTHING);

	 if(!(!Level4(Owner(player)) && !can_write_to(player,object,0))) {
	    if(!(Private(object) && !can_read_from(player,object))) {
	       if(!(!Blank(newname) && !ok_name(newname))) {
           if(!strchr(newname,'\n')) {
		  switch(Typeof(object)) {
			 case TYPE_ALARM:
			      output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, alarms can't be duplicated.");
			      return(NOTHING);
                              break;
			 case TYPE_FUSE:
			 case TYPE_ROOM:
			 case TYPE_ARRAY:
			 case TYPE_THING:
			 case TYPE_PROPERTY:
			 case TYPE_VARIABLE:
			      if(strlen(newname) > 128) {
				 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of the new (Duplicated) %s's name is 128 characters.",object_type(object,0));
				 return(NOTHING);
			      }
			      break;
			 case TYPE_EXIT:
			      if(strlen(newname) > 512) {
				 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of the new (Duplicated) exit's name is 512 characters.");
				 return(NOTHING);
			      }
			      break;
			 case TYPE_COMMAND:
			      if(strlen(newname) > 256) {
				 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of the new (Duplicated) compound command's name is 256 characters.");
				 return(NOTHING);
			      }
			      break;
			 case TYPE_CHARACTER:
			      output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, characters can't be duplicated.");
			      return(NOTHING);
                              break;
			 default:
			      writelog(BUG_LOG,1,"BUG","(create_duplicate() in create.c)  Unable to duplicate unknown object %s(%d).",getname(object),object);
			      output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that type of object is unknown  -  Unable to duplicate it.");
			      return(NOTHING);
		  }

		  if(!(in_command && (Owner(player) != Owner(current_command)) && (level_app(Owner(player)) > level_app(Owner(current_command))))) {
		     if(in_command && !Level4(Owner(current_command)) && !Wizard(current_command) && ((Typeof(object) == TYPE_COMMAND) || (Typeof(object) == TYPE_FUSE))) newowner = Owner(current_command);
			else newowner = (!in_command && (Uid(player) != player)) ? Uid(player):Owner(player);
		     if(adjustquota(player,newowner,ObjectQuota(object))) {

			/* ---->  Create and initialise duplicate  <---- */
			if(!inherit) {
			   cached_parent    = db[object].parent;
			   db[object].parent = NOTHING;
			}
			new            =  new_object();
                        db[new].type   =  db[object].type;
			db[new].flags2 =  db[object].flags2;
			db[new].flags  =  db[object].flags;
			db[new].flags &= ~(APPRENTICE|WIZARD|ELDER|DEITY);  /*  Admin flags removed for security  */
			if(!Global(effective_location(object)))
                           db[new].flags &= ~(READONLY|PERMANENT);
			if(!in_command && (Uid(player) == newowner) && friendflags_set(newowner,player,NOTHING,FRIEND_SHARABLE))
			   db[object].flags |= SHARABLE;
			initialise_data(new);

			/* ---->  Duplicate name, desc, odesc, succ, osucc, fail, ofail, drop, odrop  <---- */
			if(!Blank(newname)) {
			   ansi_code_filter((char *) newname,newname,1);
			   setfield(new,NAME,newname,0);
			} else setfield(new,NAME,getfield(object,NAME),0);

			if(!inherit) {
			   setfield(new,DESC,getfield(object,DESC),0);
			   setfield(new,SUCC,getfield(object,SUCC),0);
			   setfield(new,FAIL,getfield(object,FAIL),0);
			   setfield(new,DROP,getfield(object,DROP),0);
			   setfield(new,ODESC,getfield(object,ODESC),0);
			   setfield(new,OSUCC,getfield(object,OSUCC),0);
			   setfield(new,OFAIL,getfield(object,OFAIL),0);
			   setfield(new,ODROP,getfield(object,ODROP),0);
			}

			/* ---->  Destination, parent, expiry time and owner  <---- */
			db[new].destination = Destination(object);
			db[new].parent      = (inherit) ? object:cached_parent;
			db[new].expiry      = db[object].expiry;
			db[new].owner       = newowner;

			/* ---->  Lock  <---- */
			if(!inherit) {
                           switch(Typeof(new)) {
				  case TYPE_EXIT:
				       db[new].data->exit.lock = (struct boolexp *) create_duplicate_lock(db[object].data->exit.lock);
				       break;
				  case TYPE_FUSE:
				       db[new].data->fuse.lock = (struct boolexp *) create_duplicate_lock(db[object].data->fuse.lock);
				       break;
				  case TYPE_ROOM:
				       db[new].data->room.lock = (struct boolexp *) create_duplicate_lock(db[object].data->room.lock);
				       break;
				  case TYPE_THING:
				       db[new].data->thing.lock = (struct boolexp *) create_duplicate_lock(db[object].data->thing.lock);
				       break;
				  case TYPE_COMMAND:
				       db[new].data->command.lock = (struct boolexp *) create_duplicate_lock(db[object].data->command.lock);
				       break;
			   }
			}

			/* ---->  Location  <---- */
			if(Valid(Location(object)) && (can_teleport_object(player,new,Location(object)) || can_write_to(player,Location(object),0)))
                           db[new].location = Location(object);
			      else db[new].location = (Typeof(new) == TYPE_ROOM) ? NOTHING:player;

			/* ---->  Push it into the appropriate linked list  <---- */
			if(Valid(Location(new))) {
			   switch(Typeof(new)) {
				  case TYPE_THING:
				  case TYPE_ROOM:
				       PUSH(new,db[Location(new)].contents);
				       break;
				  case TYPE_EXIT:
				       PUSH(new,db[Location(new)].exits);
				       break;
				  case TYPE_COMMAND:
				       PUSH(new,db[Location(new)].commands);
				       if(Global(Location(new))) global_add(new);
				       break;
				  case TYPE_ALARM:
				  case TYPE_FUSE:
				       PUSH(new,db[Location(new)].fuses);
				       break;
				  case TYPE_PROPERTY:
				  case TYPE_VARIABLE:
				  case TYPE_ARRAY:
				       PUSH(new,db[Location(new)].variables);
				       break;
			   }
			}

			/* ---->  Various linked lists  <---- */
			if((Typeof(new) == TYPE_COMMAND) || (Typeof(new) == TYPE_FUSE)) db[new].contents = db[object].contents;
			   else db[new].contents = NOTHING;
			if((Typeof(new) == TYPE_COMMAND) || (Typeof(new) == TYPE_FUSE)) db[new].exits = db[object].exits;
			   else db[new].exits = NOTHING;
			db[new].variables = NOTHING;
			db[new].commands  = NOTHING;
			db[new].fuses     = NOTHING;

			/* ---->  Extra data  <---- */
			switch(Typeof(new)) {
			       case TYPE_ROOM:

				    /* ---->  Room extra data  <---- */
				    if(!inherit) setfield(new,AREANAME,getfield(object,AREANAME),0);
				    setfield(new,CSTRING,getfield(object,CSTRING),0);
				    setfield(new,ESTRING,getfield(object,ESTRING),0);
				    db[new].data->room.credit = db[object].data->room.credit;
				    db[new].data->room.volume = (inherit) ? INHERIT:db[object].data->room.volume;
				    db[new].data->room.mass   = (inherit) ? INHERIT:db[object].data->room.mass;
				    break;
			       case TYPE_ARRAY:

				    /* ---->  Dynamic array extra data (Elements)  <---- */
				    if(!adjustquota(player,newowner,(count = array_element_count(db[object].data->array.start)) * ELEMENT_QUOTA)) {
				       sprintf(scratch_buffer,"to duplicate the elements of "ANSI_LWHITE"%s"ANSI_LRED,unparse_object(player,object,0));
				       warnquota(player,newowner,scratch_buffer);
				       ok = 0;
				    }

				    /* ---->  Don't allow dynamic arrays with more than 1000 elements to be duplicated  <---- */
				    if(ok && (count > ARRAY_LIMIT)) {
				       output(getdsc(player),player,0,1,0,ANSI_LRED"Sorry, "ANSI_LWHITE"%s"ANSI_LRED" has too many elements  -  Elements not duplicated.\n",unparse_object(player,object,0));
				       ok = 0;
				    }

				    /* ---->  Duplicate elements of dynamic array  <---- */
				    if(ok) {
				       struct array_element *ptr = db[object].data->array.start;
				       struct array_element *element,*tail = NULL;

				       for(; ptr; ptr = ptr->next) {
					   MALLOC(element,struct array_element);
					   element->index = (char *) alloc_string(ptr->index);
					   element->text  = (char *) alloc_string(ptr->text);
					   element->next  = NULL;
					   if(tail) {
					      tail->next = element;
					      tail       = element;
					   } else db[new].data->array.start = tail = element;
				       }
				    }
				    break;
			       case TYPE_THING:

				    /* ---->  Thing extra data  <---- */
				    if(!inherit) {
				       setfield(new,CSTRING,getfield(object,CSTRING),0);
				       setfield(new,ESTRING,getfield(object,ESTRING),0);
				       setfield(new,AREANAME,getfield(object,AREANAME),0);
				       db[new].data->thing.lock_key = (struct boolexp *) create_duplicate_lock(db[object].data->thing.lock_key);
				    }
				    db[new].data->thing.credit = db[object].data->thing.credit;
				    db[new].data->thing.volume = (inherit) ? INHERIT:db[object].data->thing.volume;
				    db[new].data->thing.mass   = (inherit) ? INHERIT:db[object].data->thing.mass;
				    break;
			       default:
				    break; 
			}

			/* ---->  Warn of possible hacking  <---- */
			if((Typeof(object) == TYPE_COMMAND) || (Typeof(object) == TYPE_FUSE))
			   if(in_command && (Owner(player) != Owner(current_command)) && ((Typeof(object) != TYPE_COMMAND) || (Owner(object) != Owner(current_command))) && (level_app(Owner(player)) >= level_app(Owner(current_command)))) {
			      if(!Wizard(current_command)) writelog(HACK_LOG,1,"HACK","%s %s(#%d) duplicated with %s(#%d)'s ownership within compound command %s(#%d) (Owned by %s(#%d).)  -  The duplicate object is %s(%d).",(Typeof(object) == TYPE_COMMAND) ? "Compound command":"Fuse",getname(object),object,getname(Owner(player)),Owner(player),getname(current_command),current_command,getname(Owner(current_command)),Owner(current_command),getname(new),new);
			      sprintf(scratch_buffer,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"]  "ANSI_LWHITE"%s "ANSI_LYELLOW"%s"ANSI_LWHITE" created with your ownership from within compound command ",(Typeof(object) == TYPE_COMMAND) ? "Compound command":"Fuse",unparse_object(player,object,0));
			      sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LYELLOW"%s"ANSI_LWHITE" (Owned by ",unparse_object(Owner(current_command),current_command,0));
			      sprintf(scratch_buffer + strlen(scratch_buffer),"%s"ANSI_LYELLOW"%s"ANSI_LWHITE".)  -  The duplicate object is ",Article(Owner(current_command),LOWER,INDEFINITE),getcname(player,Owner(current_command),1,0));
			      output(getdsc(player),player,0,1,11,ANSI_LYELLOW"%s%s"ANSI_LWHITE".)",scratch_buffer,unparse_object(player,new,0));
			   }
			stats_tcz_update_record(0,0,0,1,0,0,0);

			if(!in_command) {
			   if(strcmp(db[object].name,db[new].name))
                              sprintf(scratch_return_string," and name %s"ANSI_LWHITE"%s"ANSI_LGREEN,Article(new,LOWER,INDEFINITE),getname(new));
			         else *scratch_return_string = '\0';
			   output(getdsc(player),player,0,1,0,ANSI_LGREEN"%suplicate of %s"ANSI_LWHITE"%s"ANSI_LGREEN" created with ID "ANSI_LYELLOW"#%d"ANSI_LGREEN"%s.",(inherit) ? "Inherited d":"D",Article(object,LOWER,DEFINITE),unparse_object(player,object,0),new,scratch_return_string);
			}

			if(!inherit) db[object].parent = cached_parent;
			setreturn(getnameid(player,new,NULL),COMMAND_SUCC);
                        return(new);
		     } else {
			sprintf(scratch_return_string,"to duplicate %s",object_type(object,1));
			warnquota(player,newowner,scratch_return_string);
			return(NOTHING);
		     }
		  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s can't be duplicated under the ID of a higher level character within a compound command.",object_type(object,1));
           } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the duplicate object can't have embedded NEWLINE characters.");
	       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the duplicate object can't have that name.");
	    } else {
	       sprintf(scratch_buffer,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is set "ANSI_LYELLOW"PRIVATE"ANSI_LGREEN"  -  Only characters above the level of its owner (",Article(object,LOWER,DEFINITE),unparse_object(player,object,0));
	       output(getdsc(player),player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN") may duplicate it.",scratch_buffer,Article(Owner(object),LOWER,INDEFINITE),getcname(player,Owner(object),1,0));
	       return(NOTHING);
	    }
	 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only duplicate an object you own.");
      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Builders can duplicate objects.");
      return(NOTHING);
}

/* ---->  Create an exit, and link it to specified room (If specified)  <---- */
dbref create_exit(CONTEXT)
{
    dbref exit,owner = (!in_command && (Uid(player) != player)) ? Uid(player):Owner(player);

    setreturn(ERROR,COMMAND_FAIL);
    if(Builder(Owner(player))) {
        if(!((Typeof(Location(player)) != TYPE_ROOM) && (!Container(Location(player))))) {
            if(!Blank(arg1)) {
                if(!strchr(arg1,'\n')) {
                    if(strlen(arg1) <= 512) {
                        if(ok_name(arg1)) {
                            if(!(!Level4(Owner(player)) && !can_write_to(player,Location(player),0))) {
                                if(adjustquota(player,owner,EXIT_QUOTA)) {

                                    /* ---->  Create and initialise exit  <---- */
                                    exit              = new_object();
                                    db[exit].location = Location(player);
                                    db[exit].flags2   = TRANSPORT;
                                    db[exit].flags    = OBJECT|OPEN;
                                    db[exit].owner    = owner;
                                    db[exit].type     = TYPE_EXIT;

                                    if(!in_command && (Uid(player) == owner) && friendflags_set(owner,player,NOTHING,FRIEND_SHARABLE))
                                        db[exit].flags |= SHARABLE;

                                    ansi_code_filter((char *) arg1,arg1,1);
                                    initialise_data(exit);
                                    setfield(exit,NAME,arg1,1);
                                    stats_tcz_update_record(0,0,0,1,0,0,0);
                                    PUSH(exit,db[Location(player)].exits);

                                    if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Exit opened with ID "ANSI_LYELLOW"#%d"ANSI_LGREEN".",exit);
                                    setreturn(getnameid(player,exit,NULL),COMMAND_SUCC);

                                    /* ---->  Check second parameter to see if exit should be linked  <---- */
                                    if(!Blank(arg2)) {
                                        db[exit].destination = parse_link_destination(player,exit,arg2,0);
                                        if(!in_command && Valid(Destination(exit)))
                                            output(getdsc(player),player,0,1,0,ANSI_LGREEN"Exit linked to %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(Destination(exit),LOWER,DEFINITE),unparse_object(player,Destination(exit),0));
                                    }
                                    return(exit);
                                } else warnquota(player,owner,"to open an exit");
                            } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only open an exit from your own property.");
                        } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, an exit can't have that name.");
                    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of an exit's name is 512 characters.");
                } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the name of a exit mustn't contain embedded NEWLINES.");
            } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the name and short name for the new exit, e.g:  '"ANSI_LWHITE"@open Heavy oak door;oak = #12345"ANSI_LGREEN"'.");
        } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only open an exit from a room or container.");
    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Builders can open exits.");
    return(NOTHING);
}

/* ---->  Create a fuse  <---- */
dbref create_fuse(CONTEXT)
{
    dbref fuse,csuccess,owner = (!in_command && (Uid(player) != player)) ? Uid(player):Owner(player);

    setreturn(ERROR,COMMAND_FAIL);
    if(Builder(Owner(player))) {
        if(!Blank(arg1)) {
            if(!strchr(arg1,'\n')) {
                if(strlen(arg1) <= 128) {
                    if(ok_name(arg1)) {
                        if(adjustquota(player,owner,FUSE_QUOTA)) {
                            if(!(in_command && (Owner(player) != Owner(current_command)) && (level_app(Owner(player)) > level_app(Owner(current_command))))) {

                                /* ---->  Create and initialise fuse  <---- */
                                fuse                 = new_object();
                                db[fuse].location    = player;
                                db[fuse].destination = NOTHING;
                                db[fuse].flags       = OBJECT;
                                db[fuse].owner       = owner;
                                db[fuse].type        = TYPE_FUSE;

                                if(!in_command && (Uid(player) == owner) && friendflags_set(owner,player,NOTHING,FRIEND_SHARABLE))
                                    db[fuse].flags |= SHARABLE;

                                ansi_code_filter((char *) arg1,arg1,1);
                                initialise_data(fuse);
                                setfield(fuse,NAME,arg1,1);
                                stats_tcz_update_record(0,0,0,1,0,0,0);
                                PUSH(fuse,db[player].fuses);

                                /* ---->  Warn of possible hacking  <---- */
                                if(in_command && (Owner(player) != Owner(current_command)) && (level_app(Owner(player)) >= level_app(Owner(current_command)))) {
                                    if(!Wizard(current_command)) writelog(HACK_LOG,1,"HACK","Fuse %s(#%d) created with %s(#%d)'s ownership within compound command %s(#%d) (Owned by %s(#%d).)",getname(fuse),fuse,getname(Owner(player)),Owner(player),getname(current_command),current_command,getname(Owner(current_command)),Owner(current_command));
                                    sprintf(scratch_buffer,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"]  "ANSI_LWHITE"Fuse "ANSI_LYELLOW"%s"ANSI_LWHITE" created with your ownership from within compound command ",unparse_object(player,fuse,0));
                                    sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LYELLOW"%s"ANSI_LWHITE" (Owned by ",unparse_object(Owner(current_command),current_command,0));
                                    output(getdsc(player),player,0,1,11,"%s%s"ANSI_LYELLOW"%s"ANSI_LWHITE".)",scratch_buffer,Article(Owner(current_command),LOWER,INDEFINITE),getcname(player,Owner(current_command),1,0));
                                }
                                if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Fuse "ANSI_LWHITE"%s"ANSI_LGREEN" created with ID "ANSI_LYELLOW"#%d"ANSI_LGREEN".",getfield(fuse,NAME),fuse);

                                /* ---->  Set csuccess of fuse?  <---- */
                                if(!Blank(arg2)) {
                                    csuccess = parse_link_command(player,fuse,arg2,0);
                                    if(Valid(csuccess)) {
                                        db[fuse].contents = csuccess;
                                        if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Csuccess of fuse linked to "ANSI_LWHITE"%s"ANSI_LGREEN".",unparse_object(player,csuccess,0));
                                    }
                                }
                                setreturn(getnameid(player,fuse,NULL),COMMAND_SUCC);
                                return(fuse);
                            } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a fuse can't be created under the ID of a higher level character within a compound command.");
                        } else warnquota(player,owner,"to build a fuse");
                    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a fuse can't have that name.");
                } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of a fuse's name is 128 characters.");
            } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the name of a fuse mustn't contain embedded NEWLINES.");
        } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify a name for the new fuse.");
    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Builders can create fuses.");
    return(NOTHING);
}

/* ---->  {J.P.Boggis 23/07/2000}  Create home room  <---- */
/*        PLAYER  = DBref of character.     */
/*        WARN    = Output error messages.  */
/*        SETHOME = Set character's home to newly created home room.  */
/*        TOHOME  = Move character to newly created home room.        */
dbref create_homeroom(dbref player,unsigned char warn,unsigned char sethome,unsigned char tohome)
{
#ifdef HOME_ROOMS
      if(Validchar(player)) {
         if(!instring("guest",getname(player))) {
	    if(Valid(homerooms) && ((Typeof(homerooms) == TYPE_ROOM) || (Typeof(homerooms) == TYPE_ROOM))) {
	       if(search_room_by_owner(player,homerooms,0) == NOTHING) {
		  char  description[BUFFER_LEN];
		  char  name[BUFFER_LEN];
		  dbref room;

		  /* ---->  Increase character's Building Quota limit, if necessary  <---- */
		  if(!Builder(player) && (db[player].data->player.quotalimit < ROOM_QUOTA))
		     db[player].data->player.quotalimit += ROOM_QUOTA;

		  if(Level4(player) || ((db[player].data->player.quota + ROOM_QUOTA) <= db[player].data->player.quotalimit)) {
		     struct descriptor_data *r = getdsc(ROOT);

		     /* ---->  Create home room  <---- */
		     sprintf(name,"%s's Home",getname(player));
		     sprintf(description,"%%c%%lWelcome to your new home room, %%y%%l%%{@?name}%%c%%l.\n\n%%y%%lBefore you do anything else, please set an appropriate description for your home room by typing:\n\n   %%3%%w%%l%s> %%w@desc here = %%g%%lType the description of your room after the '%%y%%l=%%g%%l' sign.\n\n%%y%%lAlternatively, you can use the editor to set the description (See '%%g%%l%%ueditor%%y%%l') by typing:\n\n   %%3%%w%%l%s> %%wedit here",tcz_short_name,tcz_short_name);
		     if(r) r->flags2 |= OUTPUT_SUPPRESS;
		     room = create_room(ROOT,NULL,NULL,name,description,0,0);
		     if(r) r->flags2 &= ~OUTPUT_SUPPRESS;

		     if(Valid(room)) {

			/* ---->  Set default flags and move to home rooms container  <---- */
			db[room].flags  |= HAVEN;
			db[room].flags2 &= ~(WARP|ABODE|YELL);
			move_to(room,homerooms);

			/* ---->  Change home room ownership  <---- */
			db[Owner(room)].data->player.quota -= ROOM_QUOTA;
			db[room].owner                      = player;
			db[player].data->player.quota      += ROOM_QUOTA;

			/* ---->  Set character's home to their new home room (Login)?  <---- */
			if(sethome) db[player].destination = room;

			/* ---->  Move character to new home room  <---- */
			if(!in_command && tohome)
			   move_enter(player,room,1);

			if(warn && !in_command)
			   output(getdsc(player),player,0,1,0,ANSI_LGREEN"Home room '"ANSI_LWHITE"%s"ANSI_LGREEN"' has been created for you with ID "ANSI_LYELLOW"#%d"ANSI_LGREEN".  Type '"ANSI_LYELLOW"homeroom"ANSI_LGREEN"' at any time to go to your home room.",getname(room),room);
		     } else if(warn) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, unable to create a home room for you.");
		  } else if(warn) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you do not have enough Building Quota to create your home room.  Please destroy some objects which you no-longer need and try again.");
	       } else if(warn) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you already have a home room (Type '"ANSI_LYELLOW"homeroom"ANSI_LGREEN"' to go there.)");
	    } else if(warn) {
	       if(Level4(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the home rooms container room does not exist.");
                  else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, unable to create a home room for you.  This feature has been disabled by the administrators.");
	       writelog(SERVER_LOG,1,"HOME ROOMS","Home rooms container room (#%d) either doesn't exist, or is invalid.",homerooms);
	    }
	 } else if(warn) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Guest characters cannot have a home room.");
      }
#endif
      return(NOTHING);
}

/* ---->  Create a room  <---- */
/*        (Val2:  0 = Normal, 1 = db_create())  */
dbref create_room(CONTEXT)
{
    dbref room,owner;

    setreturn(ERROR,COMMAND_FAIL);
    if(!val2) owner = (!in_command && (Uid(player) != player)) ? Uid(player):Owner(player);

    if(val2 || Builder(Owner(player))) {
        if(val2 || !Blank(arg1)) {
            if(val2 || !strchr(arg1,'\n')) {
                if(val2 || (strlen(arg1) <= 128)) {
                    if(val2 || ok_name(arg1)) {
                        if(val2 || adjustquota(player,owner,ROOM_QUOTA)) {

                            /* ---->  Create and initialise room  <---- */
                            room                 = new_object();
                            db[room].destination = NOTHING;
                            db[room].location    = NOTHING;
                            db[room].flags2      = (FINANCE|TRANSPORT|VISIT|WARP);
                            db[room].owner       = (val2) ? ROOT:owner;
                            db[room].flags       = HAVEN|OBJECT|YELL|OPENABLE;
                            db[room].type        = TYPE_ROOM;

                            if(!in_command && !val2 && (Uid(player) == owner) && friendflags_set(owner,player,NOTHING,FRIEND_SHARABLE))
                                db[room].flags |= SHARABLE;

                            if(!val2) ansi_code_filter((char *) arg1,arg1,1);
                            initialise_data(room);
                            setfield(room,NAME,arg1,1);
                            setfield(room,DESC,arg2,0);
                            stats_tcz_update_record(0,0,0,1,0,0,0);

                            if(!in_command && !val2) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Room "ANSI_LWHITE"%s"ANSI_LGREEN" created with ID "ANSI_LYELLOW"#%d"ANSI_LGREEN".",getname(room),room);
                            if(!val2) setreturn(getnameid(player,room,NULL),COMMAND_SUCC);
                            return(room);
                        } else warnquota(player,owner,"to build a room");
                    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a room can't have that name.");
                } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of a room's name is 128 characters.");
            } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the name of a room mustn't contain embedded NEWLINES.");
        } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify a name for the new room.");
    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Builders can build new rooms.");
    return(NOTHING);
}

/* ---->  Create a thing  <---- */
dbref create_thing(CONTEXT)
{
    dbref thing,owner = (!in_command && (Uid(player) != player)) ? Uid(player):Owner(player);

    setreturn(ERROR,COMMAND_FAIL);
    if(Builder(Owner(player))) {
        if(!Blank(arg1)) {
            if(!strchr(arg1,'\n')) {
                if(strlen(arg1) <= 128) {
                    if(ok_name(arg1)) {
                        if((find_volume_of_contents(player,0) + STANDARD_THING_VOLUME) <= db[player].data->player.volume) {
                            if((find_mass_of_contents(player,0) + STANDARD_THING_MASS) <= (STANDARD_CHARACTER_STRENGTH / 10)) {
                                if(adjustquota(player,owner,THING_QUOTA)) {

                                    /* ---->  Create and initialise thing  <---- */
                                    thing                 = new_object();
                                    db[thing].destination = NOTHING;
                                    db[thing].location    = player;
                                    db[thing].flags       = OBJECT|OPAQUE;
                                    db[thing].owner       = owner;
                                    db[thing].type        = TYPE_THING;

                                    if(!in_command && (Uid(player) == owner) && friendflags_set(owner,player,NOTHING,FRIEND_SHARABLE))
                                        db[thing].flags |= SHARABLE;

                                    ansi_code_filter((char *) arg1,arg1,1);
                                    initialise_data(thing);
                                    setfield(thing,NAME,arg1,1);
                                    setfield(thing,DESC,arg2,0);
                                    stats_tcz_update_record(0,0,0,1,0,0,0);

                                    /* ---->  Home is here (If can link to it) or character's home  <---- */
                                    if(Valid(Location(player)) && can_link_or_home_to(player,Location(player)))
                                        db[thing].destination = Location(player);
                                    else db[thing].destination = Destination(Owner(player));
                                    PUSH(thing,db[player].contents);

                                    if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Thing "ANSI_LWHITE"%s"ANSI_LGREEN" created with ID "ANSI_LYELLOW"#%d"ANSI_LGREEN".",getfield(thing,NAME),thing);
                                    setreturn(getnameid(player,thing,NULL),COMMAND_SUCC);
                                    return(thing);
                                } else warnquota(player,owner,"to build a thing");
                            } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"The extra weight makes you stagger  -  You drop the object and it disintegrates.");
                        } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, your hands are full  -  Please drop some objects first.");
                    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a thing can't have that name.");
                } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of a thing's name is 128 characters.");
            } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the name of a thing mustn't contain embedded NEWLINES.");
        } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify a name for the new thing.");
    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Builders can create things.");
    return(NOTHING);
}

/* ---->     Create a variable or property      <---- */
/*        (val1:  0 = Variable, 1 = Property.)        */
dbref create_variable_property(CONTEXT)
{
    dbref object,owner = (!in_command && (Uid(player) != player)) ? Uid(player):Owner(player);

    setreturn(ERROR,COMMAND_FAIL);
    if(Builder(Owner(player))) {
        if(!Blank(arg1)) {
            if(!strchr(arg1,'\n')) {
                if(strlen(arg1) <= 128) {
                    if(ok_name(arg1)) {
                        if(adjustquota(player,owner,(val1) ? PROPERTY_QUOTA:VARIABLE_QUOTA)) {

                            /* ---->  Create and initialise variable/property  <---- */
                            object                 = new_object();
                            db[object].destination = NOTHING;
                            db[object].location    = player;
                            db[object].flags       = OBJECT;
                            db[object].owner       = owner;
                            db[object].type        = (val1) ? TYPE_PROPERTY:TYPE_VARIABLE;

                            if(!in_command && (Uid(player) == owner) && friendflags_set(owner,player,NOTHING,FRIEND_SHARABLE))
                                db[object].flags |= SHARABLE;

                            ansi_code_filter((char *) arg1,arg1,1);
                            initialise_data(object);
                            setfield(object,NAME,arg1,1);
                            setfield(object,DESC,arg2,0);
                            stats_tcz_update_record(0,0,0,1,0,0,0);
                            PUSH(object,db[player].variables);

                            if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s "ANSI_LWHITE"%s"ANSI_LGREEN" created with ID "ANSI_LYELLOW"#%d"ANSI_LGREEN".",(val1) ? "Property":"Variable",getfield(object,NAME),object);
                            setreturn(getnameid(player,object,NULL),COMMAND_SUCC);
                            return(object);
                        } else warnquota(player,owner,(val1) ? "to build a property":"to build a variable");
                    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a %s can't have that name.",(val1) ? "property":"variable");
                } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of a %s's name is 128 characters.",(val1) ? "property":"variable");
            } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the name of a %s mustn't contain embedded NEWLINES.",(val1) ? "property":"variable");
        } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify a name for the new %s.",(val1) ? "property":"variable");
    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Builders can create %s.",(val1) ? "properties":"variables");
    return(NOTHING);
}
