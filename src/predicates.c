/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| PREDICATES.C  -  Utility functions and conditional tests.                   |
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

  $Id: predicates.c,v 1.2 2004/12/10 11:09:59 tcz_monster Exp $

*/


#include <string.h>
#include <ctype.h>

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


/* ---->  Can character see an object in a room?  <---- */
unsigned char can_see(dbref player,dbref thing,unsigned char can_see_loc)
{
	 switch(Typeof(thing)) {
		case TYPE_CHARACTER:
		case TYPE_THING:
		     if(player == thing) return(0);
		     if((Invisible(thing) || !can_see_loc) && (Number(player) || (!Level4(db[player].owner) && !can_write_to(player,thing,0)))) return(0);
		     return(1);
		default:
		     return(0);
	 }
}

/* ---->  Warn user if their Building Quota limit is exceeded  <---- */
void warnquota(dbref player,dbref subject,const char *message)
{
     if(player == subject) output(getdsc(player),player,0,1,0,ANSI_LRED"Sorry, you have insufficient Building Quota %s.  Please destroy some objects which you no-longer need to free up some of your Building Quota (See '"ANSI_LYELLOW"help @list"ANSI_LRED"' for details on how to list your objects of various types.)  To negotiate getting your Building Quota limit raised, please see an Apprentice Wizard/Druid or above.\n",message);
        else output(getdsc(player),player,0,1,0,ANSI_LRED"Sorry, %s"ANSI_LWHITE"%s"ANSI_LRED" has insufficient Building Quota %s.",Article(subject,LOWER,DEFINITE),getcname(NOTHING,subject,0,0),message);
}

/* ---->  Adjust specified character's Building Quota in use by amount (If this is greater than their Building Quota limit, leaves Building Quota as it is and returns 0)  <---- */
unsigned char adjustquota(dbref player,dbref subject,int quota)
{
	 if(!Validchar(player) || !Validchar(subject)) return(0);
	 if(quota > 0) {
	    if(Level4(Owner(player)) || Level4(subject) || ((db[subject].data->player.quota + quota) <= db[subject].data->player.quotalimit)) {
	       db[subject].data->player.quota += quota;
	       return(1);
	    } else return(0);
	 } else {
	    db[subject].data->player.quota += quota;
	    if(db[subject].data->player.quota < 0) {
	       writelog(BUG_LOG,1,"BUG","Adjusting %s(#%d)'s Building Quota by %d gives a negative amount of Building Quota (This has been fixed.)",getname(subject),subject,quota);
	       db[subject].data->player.quota = 0;
	    }
	    return(1);
	 }
}

/* ---->  Is given name acceptable for an object?  <---- */
unsigned char ok_name(const char *name)
{
	 return(!Blank(name) &&
		(*name != LOOKUP_TOKEN) && (*name != NUMBER_TOKEN) &&
		strcasecmp(name,"me") && strcasecmp(name,"here") &&
		strcasecmp(name,"myself") && strcasecmp(name,"myleaf") &&
		strcasecmp(name,"home") && strcasecmp(name,"unknown") &&
		strcasecmp(name,OK) && strcasecmp(name,ERROR));
}


/* ---->  Matches if STRING==WORD or WORDSPACE prefixes STRING  <---- */
#define word_matched(string,word,wordspace) (!strcasecmp(string,word) || string_prefix(string,wordspace))


/* ---->  Is name OK for a character?  <---- */
unsigned char ok_character_name(dbref player,dbref who,const char *name)
{
	 char          buffer[256];
	 unsigned char alpha = 0;
	 dbref         search;
	 const    char *scan;
	 char          *ptr;

	 if(!name)                                         return(3);
	 if(!*name)                                        return(5);
	 if(!ok_name(name))                                return(1);
	 if(strlen(name) > 20)                             return(2);

	 if((player == NOTHING) || !Level2(Owner(player))) {
	    if(strlen(name) < 4)                           return(3);

	    for(scan = name, ptr = buffer; *scan; scan++)
		if(isalnum(*scan)) *ptr++ = *scan;
	    *ptr = '\0';

	    /* ---->  Admin names  <---- */
	    if(string_prefix(buffer,"superuser"))          return(1);
	    if(string_prefix(buffer,"supervisor"))         return(1);
	    if(string_prefix(buffer,"supremebeing"))       return(1);
	    if(string_prefix(buffer,"being"))              return(1);
	    if(string_prefix(buffer,"deity"))              return(1);
	    if(string_prefix(buffer,"elder"))              return(1);
	    if(string_prefix(buffer,"wizard"))             return(1);
	    if(string_prefix(buffer,"apprentice"))         return(1);
	    if(string_prefix(buffer,"druid"))              return(1);
	    if(string_prefix(buffer,"experiencedbuilder")) return(1);
	    if(string_prefix(buffer,"builder"))            return(1);
	    if(string_prefix(buffer,"moron"))              return(1);
	    if(string_prefix(buffer,"admin"))              return(1);

	    if(instring("temporary",buffer))               return(1);
	    if(instring("connect",buffer))                 return(1);
	    if(instring("telnet",buffer))                  return(1);
	    if(instring("http",buffer))                    return(1);
	    if(instring("ftp",buffer))                     return(1);

	    /* ---->  Check for names of main commands  <---- */
	    if(string_prefix(buffer,"unknown"))            return(1);
	    if(!strcasecmp(buffer,"friends"))              return(1);
	    if(!strcasecmp(buffer,"enemies"))              return(1);

            /* ---->  Check for command names used at login screen  <---- */
	    if(word_matched(name, "information","information ")) return(1);
	    if(word_matched(name, "discliamer", "disclaimer "))  return(1);
	    if(word_matched(name, "tuturial",   "tutorial "))    return(1);
	    if(word_matched(name, "authors",    "authors "))     return(1);
	    if(word_matched(name, "modules",    "modules "))     return(1);
	    if(word_matched(name, "version",    "version "))     return(1);
	    if(word_matched(name, "author",     "author "))      return(1);
	    if(word_matched(name, "module",     "module "))      return(1);
	    if(word_matched(name, "disc",       "disc "))        return(1);
	    if(word_matched(name, "help",       "help "))        return(1);
	    if(word_matched(name, "info",       "info "))        return(1);
	    if(word_matched(name, "quit",       "quit "))        return(1);
	    if(word_matched(name, "set",        "set "))         return(1);
	    if(word_matched(name, "tut",        "tut "))         return(1);
	    if(word_matched(name, "who",        "who "))         return(1);
	    if(word_matched(name, "ver",        "ver "))         return(1);

            /* ---->  Miscellaneous disallowed names (Exact match)  <---- */
	    if(strlen(name) == 4) {
	       if(!strcasecmp(buffer,"here"))              return(1);
	       if(!strcasecmp(buffer,"test"))              return(1);
	       if(!strcasecmp(buffer,"temp"))              return(1);
	       if(!strcasecmp(buffer,"mail"))              return(1);
	       if(!strcasecmp(buffer,"news"))              return(1);
	       if(!strcasecmp(buffer,"name"))              return(1);
	       if(!strcasecmp(buffer,"root"))              return(1);
	    }
	 }

	 /* ---->  Ensure name doesn't contain the word 'Guest'  <---- */
	 if(instring("guest",name)) return(1);

	 /* ---->  Ensure name doesn't contain invalid characters, doesn't just consist of '_'s and/or numbers and doesn't start with a number  <---- */
	 if(isdigit(*name) && (player == NOTHING || !Level2(player))) return(1);
	 bad_language_filter(buffer,name);
	 for(scan = buffer; *scan; scan++)
	     if(!ValidCharName(*scan)) return(1);
		else if(isalpha(*scan)) alpha = 1;
	 if(!alpha) return(1);

	 /* ---->  Lookup name to avoid conflicts  <---- */
	 search = lookup_nccharacter(player,name,1);
	 if(((search != NOTHING) || (search == INVALID)) && (search != who)) return(4);

	 /* ---->  Check name isn't banished  <---- */
	 if(((player == NOTHING) || !Level2(player)) && banish_lookup(name,NULL,0)) return(6);
	 return(0);
}

/* ---->  Is password valid?  <---- */
unsigned char ok_password(const char *password)
{
	 const char *scan;

	 if(!password)            return(3);
	 if(!*password)           return(5);
	 if(strlen(password) < 6) return(3);
      
	 for(scan = password; *scan; scan++)
	     if(!ValidCharName(*scan)) return(1);

	 return(0);
}

/* ---->  Is E-mail address valid?  <---- */
unsigned char ok_email(dbref player,char *email)
{
	 char *ptr,c;

	 if(!Blank(email)) {
	    for(ptr = email; *ptr; ptr++)
		if((*ptr == ' ') || (*ptr == '\n') || (*ptr == '*'))
		   return(0);

	    if(!(Validchar(player) && Level2(Owner(player)))) {
	       ptr = strchr(email,'@');
	       if(!Blank(ptr)) {
		  c    = *ptr;
		  *ptr = '\0';

		  /* ---->  Bit before '@' is too short?  <---- */
		  if(Blank(email) || (strlen(email) < 2)) {
		     *ptr = c;
		     return(0);
		  } else *ptr = c;

		  /* ---->  No '.' in bit after '@' or too short?  <---- */
		  if(!*(++ptr) || (strlen(ptr) < 5) || !strchr(ptr,'.') || (strlen(email) > 128)) return(0);
		  return(1);
	       } else return(0);
	    } else return(1);
	 } else return(0);
}

/* ---->  Is name prefix/suffix valid?  <---- */
unsigned char ok_presuffix(dbref player,const char *presuffix,unsigned char prefix)
{
	 const char *ptr;

	 /* ---->  NULL?  <---- */
	 if(!presuffix) return(1);

	 /* ---->  Check for '@', '#' or '*' in prefix  <---- */
	 if(prefix && ((*presuffix == COMMAND_TOKEN) || (*presuffix == NUMBER_TOKEN) || (*presuffix == LOOKUP_TOKEN))) return(5);

	 /* ---->  Disallowed names in prefix?  <---- */ 
	 if(prefix && *presuffix && !ok_name(presuffix)) return(1);

	 /* ---->  Prefix/suffix too long?  <---- */
	 if(strlen(presuffix) > 40) return(2);

	 /* ---->  Check for embedded NEWLINE's, $'s, {}'s, %'s, @'s, ='s and :'s   <---- */
	 for(ptr = presuffix; *ptr; ptr++) {
	     switch(*ptr) {
		    case '\n':
			 return(3);
			 break;
		    case '$':
		    case '{':
		    case '}':
		    case '%':
		    case '=':
			 return(4);
			 break;
		    case ':':
			 if(prefix) return(4);
			 break;
	     }
	     if(!isprint(*ptr)) return(1);
	 }

	 /* ---->  Check for Admin-style prefixes  <---- */
	 if(prefix && !Level1(player)) {
	    if(string_prefix(presuffix,"supreme"))    return(1);
	    if(string_prefix(presuffix,"being"))      return(1);
	    if(string_prefix(presuffix,"deity"))      return(1);
	    if(string_prefix(presuffix,"elder"))      return(1);
	    if(string_prefix(presuffix,"wizard"))     return(1);
	    if(string_prefix(presuffix,"apprentice")) return(1);
	    if(string_prefix(presuffix,"moron"))      return(1);
	 }

	 /* ---->  Check name isn't banished  <---- */
	 if(!Level2(player) && banish_lookup(presuffix,NULL,0)) return(6);
	 return(0);
}

/* ---->  Is specified object within reach of specified character?  <---- */
unsigned char can_reach(dbref player,dbref thing)
{
	 while(Valid(thing) && (thing != db[player].location)) {
	       if(Valid(db[thing].location) && (Typeof(db[thing].location) == TYPE_THING) && !Open(db[thing].location)) return(0);
	       thing = db[thing].location;
	 }
	 return(Valid(thing));
}

/* ---->  Return weight of OBJECT in Kilograms  <---- */
int getweight(dbref thing)
{
    long weight = 0;

    if(!Valid(thing)) return(0);
    switch(Typeof(thing)) {
           case TYPE_CHARACTER:
           case TYPE_THING:
                weight += get_mass_or_volume(thing,0);
           case TYPE_ROOM:
                weight += find_mass_of_contents(thing,0);
    }
    return(weight);
}

/* ---->  Will VICTIM fit in the location/object DESTINATION?  <---- */
unsigned char will_fit(dbref victim,dbref destination)
{
	 static int contents_of_dest_volume;
	 static int contents_of_dest_mass;
	 static int destination_volume;
	 static int destination_mass;
	 static int victim_volume;
	 static int victim_mass;

	 if(destination == HOME) destination = db[victim].destination;
	 if(destination == NOTHING) return(1);

	 victim_volume           = get_mass_or_volume(victim,1);
	 victim_mass             = get_mass_or_volume(victim,0);
	 destination_volume      = get_mass_or_volume(destination,1);
	 destination_mass        = get_mass_or_volume(destination,0);
	 contents_of_dest_volume = find_volume_of_contents(destination,0);
	 contents_of_dest_mass   = find_mass_of_contents(destination,0);

	 switch(Typeof(victim)) {
		case TYPE_THING:
		     switch(Typeof(destination)) {
			    case TYPE_THING:
				 if((!Container(destination))
				    || ((destination_volume != INFINITY)
				    && ((contents_of_dest_volume == INFINITY)
				    || ((victim_volume + contents_of_dest_volume) < 0)
				    || ((victim_volume + contents_of_dest_volume) >= destination_volume))))
				       return(0);
				 break;
			    case TYPE_ROOM:
				 if(((destination_volume !=INFINITY)
				    && ((contents_of_dest_mass == INFINITY)
				    || ((victim_volume + contents_of_dest_volume) < 0) 
				    || ((victim_volume + contents_of_dest_volume) >= destination_volume)))
				    || ((destination_mass != INFINITY)
				    && ((contents_of_dest_volume == INFINITY) 
				    || ((victim_mass + contents_of_dest_mass) < 0) 
				    || ((victim_mass + contents_of_dest_mass) >= destination_mass))))
				       return(0);
				 break;
			    case TYPE_CHARACTER:
				 if(Level3(destination) || (destination_volume == INFINITY)) return(1);
				 if((victim_volume == INFINITY)
				    || ((victim_volume + contents_of_dest_volume) >= destination_volume * 2)
				    || (victim_volume >= destination_volume)
				    || ((victim_mass + contents_of_dest_mass) > (STANDARD_CHARACTER_STRENGTH / 10)))
				       return(0);
				 break;
			    default:
				 return(0);
		     }
		     break;
		case TYPE_ROOM:
		     switch(Typeof(destination)) {
			    case TYPE_ROOM:
				 break;
			    case TYPE_THING:
			    case TYPE_CHARACTER:
			    default:
				 return(0);
		     }
		     break;
		case TYPE_CHARACTER:
		     if(Level4(victim)) return(1);
		     switch(Typeof(destination)) {
			    case TYPE_THING:
				 if((!Container(destination))
				    || ((destination_volume != INFINITY)
				    && ((victim_volume + contents_of_dest_volume) >= destination_volume)))
				       return(0);
				 break;
				 /*  Fall through  */
			    case TYPE_ROOM:
				 if(((destination_volume != INFINITY)
				    && ((victim_volume + contents_of_dest_volume) >= destination_volume))
				    || ((destination_mass != INFINITY)
				    && ((victim_mass + contents_of_dest_mass) >= destination_mass)))
				       return(0);
				 break;
			    case TYPE_CHARACTER:
			    default:
				 return(0);
		     }
		default:
		     return(1);
	 }
	 return(1);
}

/* ---->  Get volume of contents of an object  <---- */
int find_volume_of_contents(dbref thing,unsigned char level)
{
    dbref object,currentobj;
    int   total_volume = 0;

    if((level >= MATCH_RECURSION_LIMIT) || !Valid(thing)) return(0);
    object = getfirst(thing,CONTENTS,&currentobj);
    if(Valid(object)) do {
       if(HasField(object,VOLUME) && (Typeof(object) != TYPE_ROOM)) {
          total_volume += get_mass_or_volume(object,1);
          if(Typeof(object) == TYPE_CHARACTER) total_volume += find_volume_of_contents(object,level + 1);
          if(total_volume < 0) return(INFINITY);
       }
       getnext(object,CONTENTS,currentobj);
    } while(Valid(object));
    return(total_volume);
}

/* ---->  Get mass of contents of an object  <---- */
int find_mass_of_contents(dbref thing,unsigned char level)
{ 
    dbref object,currentobj;
    int   total_mass = 0;
        
    if((level >= MATCH_RECURSION_LIMIT) || !Valid(thing)) return(0);
    object = getfirst(thing,CONTENTS,&currentobj);
    if(Valid(object)) do {
       if(HasField(object,MASS) && (Typeof(object) != TYPE_ROOM)) {
          total_mass += get_mass_or_volume(object,0) + find_mass_of_contents(object,level + 1);
          if(total_mass < 0) return(INFINITY);
       }
       getnext(object,CONTENTS,currentobj);
    } while(Valid(object));
    return(total_mass); 
}

/* ---->  Can given character link something to the specified destination  <---- */
unsigned char can_link_to(dbref player,dbref destination,unsigned char override)
{
	 if(!Valid(destination)) return(0);
	 if(!((Typeof(destination) == TYPE_ROOM) || Container(destination))) return(0);
	 if(!((override && Level4(Owner(player))) || can_write_to(player,destination,1))) return(0);
	 return(1);
}

/* ---->  Returns privilege level of character  <---- */
/*        0  -  Supreme Being                         */
/*        1  -  Deity                                 */
/*        2  -  Elder Wizard/Druid                    */
/*        3  -  Wizard/Druid                          */
/*        4  -  Apprentice Wizard/Druid               */
/*        5  -  Experienced Builder/Assistant         */
/*        6  -  Builder                               */
/*        7  -  Mortal                                */
/*        8  -  Moron                                 */
unsigned char privilege(dbref player,unsigned char cutoff)
{
	 if(Root(player)) return(0);
	    else if((cutoff >= 8) && Moron(player)) return(8);
	       else if((cutoff >= 1) && Level1(player)) return(1);
		  else if((cutoff >= 2) && Level2(player)) return(2);
		     else if((cutoff >= 3) && Level3(player)) return(3);
			else if((cutoff >= 4) && Level4(player)) return(4);
			   else if((cutoff >= 5) && (Experienced(player) || Assistant(player))) return(5);
			      else if((cutoff >= 6) && Builder(player)) return(6);
				 else return(7);
}

/* ---->  Returns privilege level of a character  -  Used in security routines  <---- */
/*        4  -  Supreme Being                                         */
/*        3  -  Deity                                                 */
/*        2  -  Elder Wizard/Druid                                    */
/*        1  -  Wizard/Druid                                          */
/*        0  -  Any other type of character (No security privileges)  */
unsigned char level(dbref player)
{
	 if(Root(player)) return(4);
	    else if(Level1(player)) return(3);
	       else if(Level2(player)) return(2);
		  else if(Level3(player)) return(1);
		     else return(0);
}

/* ---->  Returns privilege level of a character (Includes Apprentice Wizards/Druids)  -  Used in security routines  <---- */
/*        5  -  Supreme Being                                         */
/*        4  -  Deity                                                 */
/*        3  -  Elder Wizard/Druid                                    */
/*        2  -  Wizard/Druid                                          */
/*        1  -  Apprentice Wizard/Druid                               */
/*        0  -  Any other type of character (No security privileges)  */

/*        WARNING:  THIS FUNCTION ISN'T COMPATIBLE WITH LEVEL()!!!    */
unsigned char level_app(dbref player)
{
	 if(Root(player)) return(5);
	    else if(Level1(player)) return(4);
	       else if(Level2(player)) return(3);
		  else if(Level3(player)) return(2);
		     else if(Level4(player)) return(1);
			else return(0);
}

/* ---->  Returns 1 if character has 'read' permission on given object  <---- */
unsigned char can_read_from(dbref player,dbref object)
{
	 if(!Valid(object) || !Validchar(player)) return(0);
	 if(!Private(object)) return((Owner(player) == Owner(object)) || Visible(object) || Level4(Owner(player)) || ((Typeof(object) == TYPE_CHARACTER) && (Owner(player) == Controller(Owner(object)))) || ((Typeof(object) != TYPE_CHARACTER) && (Experienced(Owner(player)) || (!in_command && friendflags_set(Owner(object),player,object,FRIEND_READ)))));
	    else return((Owner(player) == Owner(object)) || ((Typeof(object) == TYPE_CHARACTER) && (Owner(player) == Controller(Owner(object)))) || (level_app(Owner(player)) > level_app(Owner(object))) || (!in_command && friendflags_set(Owner(object),player,object,FRIEND_READ)));
}

/* ---->  Returns 1 if given character has 'write' permission on given object  <---- */
/*          PLAYER:  Character seeking write permission.  */
/*          OBJECT:  Object character is seeking write permission to.  */
/*        APP_PRIV:  1 = Apprentice taken into account when doing Admin. level check, 0 = Apprentice not taken into account (Wizard and above only.)  */
unsigned char can_write_to(dbref player,dbref object,unsigned char app_priv)
{
         /* ---->  Invalid object/owner?  <---- */
	 if(!Valid(object) || !Validchar(player) || !Validchar(Owner(object))) return(0);

         /* ---->  No write to self allowed within irreversible @chpid compound command  <---- */
	 if((player == object) && security && in_command && Valid(current_command) && !Wizard(current_command) && !Level4(Owner(current_command)) && (player != Owner(current_command))) return(0);

         /* ---->  No write permission to self or own objects granted within non-'@chpid' compound command owned by another character, unless that character is Admin. or the compound command is set APPRENTICE  <---- */
         if(in_command && Valid(current_command) &&  /*  Currently executing compound command?  */
           (Owner(current_command) != player) &&     /*  Current compound command is not owned by character  */
           (((Typeof(object) == TYPE_CHARACTER) ? object:Owner(object)) == player) &&  /*  Object is owned by character?  */
	   !Apprentice(current_command) &&           /*  APPRENTICE flag not set on compound command  */
           !Level4(Owner(current_command)))          /*  Compound command is not Admin. owned  */
           return(0);

	 /* ---->  Write permission granted if owner of object matches owner of character seeking write permission  <---- */
         if(Owner(object) == Owner(player)) return(1);

         /* ---->  Write permission granted if object is controlled (Puppet) by character seeking write permission  <---- */
         if(Controller(object) == Owner(player)) return(1);

         /* ---->  Write permission granted if character seeking write permission is of higher Admin. privilege level than the owner of the object  <---- */
	 if(app_priv) {
	    if(level_app(Owner(player)) > level_app(Owner(object))) return(1);
	 } else if(level(Owner(player)) > level(Owner(object))) return(1);
	 
	 /* ---->  Write permission to object granted if allowed via friend flags outside of compound command  <---- */
	 if(!in_command && (Typeof(object) != TYPE_CHARACTER) && friendflags_set(Owner(object),player,object,FRIEND_WRITE) && ((Typeof(object) != TYPE_COMMAND) || friendflags_set(Owner(object),player,object,FRIEND_COMMANDS))) return(2);

         /* ---->  No write permission granted  <---- */
	 return(0);
}

/* ---->  Ensure specified NAME is a compound command or HOME (CSUCC/CFAIL)  <---- */
dbref parse_link_command(dbref player,dbref object,const char *name,unsigned char prompt)
{
      dbref command;

      if(Blank(name)) return(NOTHING);
      if(!strcasecmp(name,"home")) return(HOME);
      command = match_preferred(player,player,name,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_COMMAND,MATCH_OPTION_DEFAULT);
      if(!Valid(command)) return(NOTHING);

      /* ---->  Check compound command  <---- */
      if(!Valid(command) || (Typeof(command) != TYPE_COMMAND)) {
         switch(prompt) {
                case 0:
                     output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" can only be linked to a compound command.",Article(object,LOWER,DEFINITE),unparse_object(player,object,0));
                     break;
                case 255:
                     output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the compound command that will be executed when you leave the editor and abandon your changes during this edit session must be a compound command.");
                     break;
                default:
                     output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, "ANSI_LYELLOW"%s"ANSI_LGREEN" must be a compound command.",(prompt == 1) ? "<PROCESS CMD>":(prompt == 2) ? "<SUCC CMD>":"<FAIL CMD>");
	 }
         return(NOTHING);
      }

      if(!can_write_to(player,command,0)) {
         switch(prompt) {
                case 0:
                     if(Level3(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" can only be linked to something you own or something owned by a lower level character than yourself.",Article(object,LOWER,DEFINITE),unparse_object(player,object,0));
                        else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" can only be linked to something you own.",Article(object,LOWER,DEFINITE),unparse_object(player,object,0));
                     break;
                case 255:
                     if(Level3(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the compound command that will be executed when you leave the editor and abandon your changes during this edit session must be owned by yourself or a lower level character.");
                        else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you must own the compound command that will be executed when you leave the editor and abandon your changes during this edit session.");
                     break;
	        default:
                     if(Level3(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, "ANSI_LYELLOW"%s"ANSI_LGREEN" must be a compound command which is owned by yourself or a lower level character.",(prompt == 1) ? "<PROCESS CMD>":(prompt == 2) ? "<SUCC CMD>":"<FAIL CMD>");
                        else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, "ANSI_LYELLOW"%s"ANSI_LGREEN" must be a compound command which you own.",(prompt == 1) ? "<PROCESS CMD>":(prompt == 2) ? "<SUCC CMD>":"<FAIL CMD>");
	 }
         return(NOTHING);
      } else return(command);
}

/* ---->  Match link destination and check that it's possible to link the specified object to it  <---- */
dbref parse_link_destination(dbref player,dbref object,const char *name,int flags)
{
      dbref destination;

      /* ---->  Check for HOME first  <---- */
      if(Blank(name)) return(NOTHING);
      if(!strcasecmp(name,"home")) return(HOME);
      destination = match_preferred(player,player,name,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
      if(!Valid(destination)) return(NOTHING);

      /* ---->  Valid link destination?  <---- */
      if(Valid(destination))
         switch(Typeof(destination)) {
                case TYPE_CHARACTER:
                     if(Typeof(object) != TYPE_EXIT) {
			if(!can_write_to(player,destination,0)) {
			   if(Level3(Owner(player))) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only link %s"ANSI_LWHITE"%s"ANSI_LGREEN" to someone of a lower level than yourself.",Article(object,LOWER,DEFINITE),unparse_object(player,object,0));
			      else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only link %s"ANSI_LWHITE"%s"ANSI_LGREEN" to yourself.",Article(object,LOWER,DEFINITE),unparse_object(player,object,0));
			   return(NOTHING);
			} else return(destination);
		     }
                     break;
                case TYPE_THING:
                     if(!Container(destination)) {
			char buffer[BUFFER_LEN];

			sprintf(buffer,"%s"ANSI_LWHITE"%s"ANSI_LGREEN,Article(destination,LOWER,DEFINITE),unparse_object(player,destination,0));
			output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't link %s"ANSI_LWHITE"%s"ANSI_LGREEN" to %s (It must be a container.)",Article(object,LOWER,DEFINITE),unparse_object(player,object,0),buffer);
                        return(NOTHING);
		     }
                case TYPE_ROOM:
                     if(can_link_to(player,destination,(Typeof(object) == TYPE_EXIT)) || ((Typeof(object) == TYPE_CHARACTER) && (db[destination].flags & flags)))
                        return(destination);

                     if(object != player) {
			char buffer[BUFFER_LEN];

			sprintf(buffer,"%s"ANSI_LWHITE"%s"ANSI_LGREEN,Article(destination,LOWER,DEFINITE),unparse_object(player,destination,0));
			output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't %s %s"ANSI_LWHITE"%s"ANSI_LGREEN"%s to %s%s",(Typeof(object) == TYPE_CHARACTER) ? "set":"link",Article(object,LOWER,DEFINITE),unparse_object(player,object,0),(Typeof(object) == TYPE_CHARACTER) ? "'s home to":"",buffer,!can_write_to(player,destination,1) ? " (Permission denied.)":".");
		     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't set your home to %s"ANSI_LWHITE"%s"ANSI_LGREEN" (It must have its "ANSI_LYELLOW"ABODE"ANSI_LGREEN" flag set.)",Article(destination,LOWER,DEFINITE),unparse_object(player,destination,0));
                     return(NOTHING);
	 }

      /* ---->  Invalid object, or incorrect types of object for link  <---- */
      if(1) {
         char buffer[BUFFER_LEN];

         sprintf(buffer,"%s"ANSI_LWHITE"%s"ANSI_LGREEN,Article(destination,LOWER,DEFINITE),unparse_object(player,destination,0));
         output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't link %s"ANSI_LWHITE"%s"ANSI_LGREEN" to %s.",Article(object,LOWER,DEFINITE),unparse_object(player,object,0),buffer);
      }
      return(NOTHING);
}
