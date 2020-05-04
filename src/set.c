/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| SET.C  -  Implements setting/modifying fields, parameters, flags, etc. of   |
|           objects, characters, etc.                                         |
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

  $Id: set.c,v 1.4 2005/06/29 20:13:16 tcz_monster Exp $

*/


#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <math.h>

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

#include "search.h"
#include "fields.h"
#include "match.h"
#include "quota.h"

#ifndef CYGWIN32
   #include <crypt.h>
#endif


#define PROFILE_METRIC      0x80000000
#define PROFILE_MAJOR_MASK  0x7FFF0000
#define PROFILE_MINOR_MASK  0x0000FFFF
#define PROFILE_MAJOR_SHIFT 16


/* ---->  Enter AFK (Away From Keyboard) mode  <---- */
/*        (VAL1:  0 = Normal 'afk', 1 = 'set afk'.)  */
void set_afk(CONTEXT)
{
     struct   descriptor_data *d,*p = getdsc(player);
     int                      adjustment;
     unsigned char            adjust = 0;
     time_t                   now;

     /* ---->  Change auto-AFK time or disable it?  <---- */
     setreturn(ERROR,COMMAND_FAIL);
     if(val1 && Blank(arg1) && !Blank(arg2))
        params = arg2; 

     if(!Blank(params)) {
        if(!string_prefix("on",params) && !string_prefix("yes",params)) {
           if(!string_prefix("off",params) && !string_prefix("no",params)) {
              if(isdigit(*params) || (*params == '-')) {
                 const char *ptr = params;

                 if(*ptr == '-') ptr++;
                 for(; *ptr && isdigit(*ptr); ptr++);
                 for(; *ptr && (*ptr == ' '); ptr++);
                 if(!*ptr) adjust = 1, adjustment = atol(params);
	      }
	   } else adjust = 1, adjustment = 0;
	} else adjust = 1, adjustment = AFK_TIME;

        if(adjust) {
           if(adjustment >= 0) {
              if(adjustment <= MAX_IDLE_TIME) {
                 if(Validchar(p->player)) {
		    if(adjustment > 0) {
		       if(!in_command) output(p,player,0,1,0,ANSI_LGREEN"Auto-AFK facility turned "ANSI_LWHITE"on"ANSI_LGREEN".  You will now be automatically sent AFK when you idle for over "ANSI_LYELLOW"%d"ANSI_LGREEN" minute%s.",adjustment,Plural(adjustment));
		       db[p->player].data->player.afk = adjustment;
		    } else {
		       if(!in_command) output(p,player,0,1,0,ANSI_LGREEN"Auto-AFK facility turned "ANSI_LYELLOW"off"ANSI_LGREEN".  You will no-longer be automatically sent AFK when you idle.");
		       db[p->player].data->player.afk = 0;
		    }
		    setreturn(OK,COMMAND_SUCC);
		 } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you must be connected to set your auto-AFK idle time.");
	      } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, auto-AFK idle time cannot be set above "ANSI_LWHITE"%d"ANSI_LGREEN" minute%s (The maximum allowed idle time before automatic disconnection.)",MAX_IDLE_TIME,Plural(MAX_IDLE_TIME));
	   } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, auto-AFK idle time cannot be set to a negative value.");
           return;
	} else if(val1) {
           output(p,player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LWHITE"on"ANSI_LGREEN"', '"ANSI_LWHITE"off"ANSI_LGREEN"' or a value (In minutes) between "ANSI_LWHITE"1"ANSI_LGREEN" and "ANSI_LWHITE"%d"ANSI_LGREEN".",MAX_IDLE_TIME);
           return;
	}
     } else if(val1) {
        if(Validchar(p->player) && db[p->player].data->player.afk)
           output(p,player,0,1,0,ANSI_LGREEN"Auto-AFK facility is "ANSI_LWHITE"on"ANSI_LGREEN"  -  Max. idle time:  "ANSI_LWHITE"%d"ANSI_LGREEN" minute%s.",db[p->player].data->player.afk,Plural(db[p->player].data->player.afk));
              else output(p,player,0,1,0,ANSI_LGREEN"Auto-AFK facility is "ANSI_LWHITE"off"ANSI_LGREEN".");
        setreturn(OK,COMMAND_SUCC);
        return;
     }

     /* ---->  Enter AFK  <---- */
     if(!(!Blank(getname(player)) && instring("guest",getname(player)))) {
        if(!Blank(params)) {
           if(p) {
              gettime(now);
              if((p->player == player) && (p->clevel == 126)) {
                 output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can't go AFK (Away From Keyboard) during an interactive command which uses '"ANSI_LWHITE"@prompt"ANSI_LGREEN"'  -  Please type '"ANSI_LYELLOW"abort"ANSI_LGREEN"' first.");
                 return;
	      }

              for(d = descriptor_list; d; d = d->next)
                  if((d->player == player) && (d->flags & CONNECTED) && !d->clevel) {
                     output(d,player,0,1,0,ANSI_LGREEN"\nYou are now AFK (Away From Keyboard)  -  To leave AFK mode, simply enter your password.\n");
                     output(d,player,0,1,14,"PLEASE NOTE: \016&nbsp;\016 "ANSI_LWHITE"If you idle for more than "ANSI_LYELLOW"%d minute%s"ANSI_LWHITE", you'll still get disconnected.\n",MAX_IDLE_TIME,Plural(MAX_IDLE_TIME));
                     FREENULL(d->afk_message);
                     d->afk_message =  (char *) alloc_string(params);
                     d->afk_time    =  now;
                     d->flags2     &= ~(SENT_AUTO_AFK);
                     d->clevel      =  14;
                     if(d != p) prompt_display(d);
		  }

              if(!Invisible(db[player].location) && !Quiet(db[player].location))
                 output_except(db[player].location,player,NOTHING,0,1,2,"%s",construct_message(player,ANSI_LWHITE,ANSI_LGREEN,"",'.',1,OTHERS,"will be away from %p keyboard  -  Back soon...",0,DEFINITE));
              setreturn(OK,COMMAND_SUCC);
	   } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you must be connected to go AFK (Away From Keyboard.)");
	} else output(p,player,0,1,0,ANSI_LGREEN"Please specify your AFK (Away From Keyboard) message, e.g:  '"ANSI_LWHITE"afk Gone to get something to eat  -  back soon."ANSI_LGREEN"'.");
     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, Guest characters can't go AFK (Away From Keyboard)  -  Please ask an Apprentice Wizard/Druid or above to create a proper character for you.");
}

/* ---->  Set room/thing's area name  <---- */
void set_areaname(CONTEXT)
{
     dbref thing;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if(HasField(thing,AREANAME)) {
        if(can_write_to(player,thing,0)) {
           if(!Readonly(thing)) {
              filter_spaces(scratch_buffer,arg2,0);
              if(strlen(scratch_buffer) <= 100) {
	         if(!strchr(arg2,'\n')) {
                    if(!(!BlankContent(scratch_buffer) && !ok_name(scratch_buffer))) {
                       setfield(thing,AREANAME,scratch_buffer,1);
                       if(!in_command) {
                          sprintf(scratch_return_string,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN"'s area name %s",Article(thing,UPPER,DEFINITE),unparse_object(player,thing,0),BlankContent(scratch_buffer) ? "reset.":"set to '");
                          if(!BlankContent(scratch_buffer)) sprintf(scratch_return_string + strlen(scratch_return_string),ANSI_LYELLOW"%s"ANSI_LGREEN"'.",getfield(thing,AREANAME));
                          output(getdsc(player),player,0,1,0,"%s",scratch_return_string);
		       }
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that area name is invalid.");
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, an area name mustn't contain embedded NEWLINE's.");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of an area name is 100 characters.");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't change its area name.",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0));
	} else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the area name of something you own or something owned by someone of a lower level than yourself.");
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the area name of something you own.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s doesn't have an area name.",object_type(thing,1));
}

/* ---->  Set contents string of a room or thing  <---- */
void set_cstring(CONTEXT)
{
     dbref thing;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if(HasField(thing,CSTRING)) {
        if(can_write_to(player,thing,0)) {
           if(!Readonly(thing)) {
              if(!strchr(arg2,'\n')) {
                 if(strlen(arg2) <= 128) {
		    ansi_code_filter((char *) arg2,arg2,1);
                    setfield(thing,CSTRING,arg2,1);
                    if(!in_command) {
                       if(!Blank(arg2)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Contents string of %s"ANSI_LWHITE"%s"ANSI_LGREEN" set to '"ANSI_LYELLOW"%s"ANSI_LGREEN"'.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0),getfield(thing,CSTRING));
                          else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Contents string of %s"ANSI_LWHITE"%s"ANSI_LGREEN" reset.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
		    }
                    setreturn(OK,COMMAND_SUCC);
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of a contents string is 128 characters.");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a contents string mustn't contain embedded NEWLINES.");
           } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't change its contents string.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
	} else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the contents string of a room/thing which you own or a room/thing owned by someone of a lower level than yourself.");
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the contents string of a room/thing which you own.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s doesn't have a contents string.",object_type(thing,1));
}

/* ---->  {J.P.Boggis 24/09/2000}  Set preferred date/time format  <---- */
void set_datetimeformat(CONTEXT)
{
     dbref      character = NOTHING;
     char       *fmttype,*format;
     const char *charname = NULL;

     if(!Blank(arg2)) {
        split_params(arg2,&fmttype,&format);
        if(Blank(arg1))
           character = Owner(player);
              else charname = arg1;
     } else {
        character = Owner(player);
        fmttype   = arg1;
        format    = arg2;
     }

     if((character == player) || ((character = lookup_character(player,charname,1)) != NOTHING)) {
        
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg2);
     setreturn(ERROR,COMMAND_FAIL);
}

/* ---->  Set description of object  <---- */
void set_description(CONTEXT)
{
     int   value,new;
     dbref thing;

     setreturn(ERROR,COMMAND_FAIL);

#ifdef BETA

     /*  {J.P.Boggis 22/05/2001}  Beta code for prompting user for command parameters, if omitted on the command-line  */
     if(!Blank(arg1)) {
#endif

	thing = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
	if(!Valid(thing)) return;

#ifdef BETA

        /*  {J.P.Boggis 22/05/2001}  Beta code for prompting user for command parameters, if omitted on the command-line  */
        if(!Blank(arg2) || in_command || (!PrefsPrompt(player) && !PrefsEditor(player))) {
#endif
	   if((Typeof(thing) == TYPE_ARRAY) && (elementfrom != NOTHING)) {
	      if(can_write_to(player,thing,0)) {
		 if(!Readonly(thing)) {
		    if(elementfrom != NOTHING) {
		       if(elementfrom != INVALID) {
			  switch(array_set_elements(player,thing,elementfrom,elementto,arg2,0,&value,&new)) {
				 case ARRAY_INSUFFICIENT_QUOTA:
				      sprintf(scratch_return_string,"to add that many new elements to "ANSI_LWHITE"%s"ANSI_LRED,unparse_object(player,thing,0));
				      warnquota(player,db[thing].owner,scratch_return_string);
				      break;
				 case ARRAY_TOO_MANY_ELEMENTS:
				      output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, no more than "ANSI_LWHITE"%d"ANSI_LGREEN" new elements may be added to a dynamic array in one go.",ARRAY_LIMIT);
				      break;
				 case ARRAY_TOO_MANY_BLANKS:
				      output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, no more than "ANSI_LWHITE"%d"ANSI_LGREEN" new elements may be created in a dynamic array in one go.",ARRAY_LIMIT);
				      break;
				 default:
				      if(!in_command) {
					 sprintf(scratch_return_string," ("ANSI_LWHITE"%d"ANSI_LGREEN" new element%s created.)",new,Plural(new));
					 sprintf(scratch_buffer,ANSI_LGREEN"Description of element%s %s of dynamic array ",((elementto == UNSET) || ((elementfrom == elementto) && !((elementto == INDEXED) && !BlankContent(indexto)))) ? "":"s",array_unparse_element_range(elementfrom,elementto,ANSI_LGREEN));
					 output(getdsc(player),player,0,1,0,"%s"ANSI_LWHITE"%s"ANSI_LGREEN" set%s",scratch_buffer,unparse_object(player,thing,0),(new > 0) ? scratch_return_string:".");
				      }
			  }
			  setreturn(OK,COMMAND_SUCC);
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that element (Or range of elements) is invalid.");
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify which elements of %s"ANSI_LWHITE"%s"ANSI_LGREEN" you'd like to set the description of.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't change/add to its elements.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
	      } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change/add to the elements of a dynamic array you own or one that's owned by someone of a lower level than yourself.");
		 else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change/add to the elements of a dynamic array you own.");
	   } else if(HasField(thing,DESC)) {
	      if(CanSetField(thing,DESC)) {
		 if(can_write_to(player,thing,0)) {
		    if(!Readonly(thing)) {
		       if(!Readonlydesc(thing)) {
			  switch(Typeof(thing)) {
				 case TYPE_FUSE:
				      if((value = !Blank(arg2) ? atoi(arg2):0) < 1) {
					 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a fuse must tick at least once.");
					 return;
				      }
				      break;
				 case TYPE_ALARM:
				      event_remove(thing);
				      if(!Blank(arg2) && Valid(db[thing].destination))
					 event_add(NOTHING,thing,NOTHING,NOTHING,event_next_cron(arg2),NULL);
				      break;
			  }

			  if((Typeof(thing) == TYPE_COMMAND) && (!in_command || (Valid(current_command) && !Wizard(current_command))))
			     if(Apprentice(thing) && !Level4(player)) {
				if(!in_command) writelog(HACK_LOG,1,"HACK","%s(#%d) changed the description of compound command %s(#%d), which has its APPRENTICE, WIZARD or ELDER flag set.",getname(player),player,getname(thing),thing);
				   else writelog(HACK_LOG,1,"HACK","%s(#%d) changed the description of compound command %s(#%d) within compound command %s(#%d), which has its APPRENTICE, WIZARD or ELDER flag set.",getname(player),player,getname(thing),thing,getname(current_command),current_command);
			     }

			  setfield(thing,DESC,arg2,0);
			  if((Typeof(thing) == TYPE_FUSE) && Blank(getfield(thing,DROP)))
			     setfield(thing,DROP,arg2,0);

			  if(!in_command) {
			     if(Typeof(thing) == TYPE_FUSE) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Ticks of fuse "ANSI_LWHITE"%s"ANSI_LGREEN" set to "ANSI_LYELLOW"%d"ANSI_LGREEN".",unparse_object(player,thing,0),value);
				else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Description of %s"ANSI_LWHITE"%s"ANSI_LGREEN" set.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
			  }
			  setreturn(OK,COMMAND_SUCC);
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" description is Read-Only.",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0));
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't change %s description.",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0),(Typeof(thing) == TYPE_CHARACTER) ? "their":"its");
		 } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the description of something you own or something owned by someone of a lower level than yourself.");
		    else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the description of something you own.");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the description of %s can't be set.",object_type(thing,1));
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s doesn't have a description.",object_type(thing,1));

#ifdef BETA

        /*  {J.P.Boggis 22/05/2001}  Beta code for prompting user for command parameters, if omitted on the command-line  */
	} else {

           /* ---->  Edit or interactively prompt for description  <---- */
           if(!PrefsEditor(player)) {
              char buffer[BUFFER_LEN];

              snprintf(buffer,BUFFER_LEN,"|edit description %s",arg1);
              process_basic_command(player,buffer,0);
	   } else {
              if((Typeof(thing) == TYPE_ARRAY) && (elementfrom != NOTHING))
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Changing the description of element%s %s of dynamic array %s"ANSI_LYELLOW"%s"ANSI_LGREEN"...\n",((elementto == UNSET) || ((elementfrom == elementto) && !((elementto == INDEXED) && !Blank(indexto)))) ? "":"s",array_unparse_element_range(elementfrom,elementto,ANSI_LGREEN),Article(thing,LOWER,DEFINITE),getcname(NOTHING,thing,0,0));
	            else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Changing the description of %s"ANSI_LYELLOW"%s"ANSI_LGREEN"...\n",Article(thing,LOWER,DEFINITE),getcname(NOTHING,thing,0,0));
              prompt_user(player,"Enter description: ",NULL,NULL,NULL,NULL,arg0,ARG2,params,arg1,arg2,NULL);
	   }
	}
     } else prompt_user(player,"Enter object name ('"ANSI_LGREEN"me"ANSI_LWHITE"'): ",NULL,"me",NULL,"Please specify the name of the object you would like to change the description of.",arg0,ARG1,params,arg1,arg2,NULL);
#endif
}

/* ---->  Set drop message of object  <---- */
void set_drop(CONTEXT)
{
     dbref thing;
     int   value;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if(HasField(thing,DROP)) {
        if(CanSetField(thing,DROP)) {
           if(can_write_to(player,thing,0)) {
              if(!Readonly(thing)) {
                 if(!Blank(arg2) && (Typeof(thing) == TYPE_FUSE)) {
                    value = atoi(arg2);
                    if(value < 1) {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the reset of a fuse must be 1 or greater.");
                       return;
		    }
		 }

                 setfield(thing,DROP,arg2,0);
                 if((Typeof(thing) == TYPE_FUSE) && Blank(getfield(thing,DESC)))
                    setfield(thing,DESC,arg2,0);

                 if(!in_command) {
                    if(Typeof(thing) == TYPE_FUSE)
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Reset of fuse "ANSI_LWHITE"%s"ANSI_LGREEN" set to "ANSI_LYELLOW"%d"ANSI_LGREEN".",unparse_object(player,thing,0),value);
                          else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Drop message of %s"ANSI_LWHITE"%s"ANSI_LGREEN" set.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
		 }
                 setreturn(OK,COMMAND_SUCC);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't change %s drop message.",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0),(Typeof(thing) == TYPE_CHARACTER) ? "their":"its");
	   } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the drop message of something you own or something owned by someone of a lower level than yourself.");
              else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the drop message of something you own.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the drop message of %s can't be set.",object_type(thing,1));
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s doesn't have a drop message.",object_type(thing,1));
}

/* ---->  Set E-mail address of character  <---- */
void set_email(CONTEXT)
{
     unsigned char forward = 0;
     dbref         character;
     const    char *address;
     int           number;
     char          *tmp;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(arg2)) {
        if(!Blank(arg1)) {
           if((character = lookup_character(player,arg1,1)) == NOTHING) {
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
              return;
	   }
	} else character = db[player].owner;
     } else character = db[player].owner, arg2 = arg1;

     if(!in_command || Wizard(current_command)) {
        if(can_write_to(player,character,1)) {
           if(!Readonly(character)) {
              if(!Blank(arg2)) {
                 for(tmp = scratch_return_string; *arg2 && (*arg2 == ' '); arg2++);
                 for(; *arg2 && (*arg2 != ' '); *tmp++ = *arg2++);
                 for(; *arg2 && (*arg2 == ' '); arg2++);
                 *tmp = '\0';

                 if((!strcasecmp("public",scratch_return_string) && (number = 1)) || (!strcasecmp("private",scratch_return_string) && (number = 2)) || ((number = atol(scratch_return_string)) && (number > 0))) {
                    if(number <= EMAIL_ADDRESSES) {
                       if(!strcasecmp("reset",arg2)) {

                          /* ---->  Reset E-mail address  <---- */
                          if((number != 2) || Level4(db[player].owner)) {
                             if((number == 1) || ((number == 2) && gettextfield(1,'\n',getfield(character,EMAIL),0,scratch_return_string) && !strcasecmp("forward",scratch_return_string)))
                                db[character].flags2 &= ~FORWARD_EMAIL;
                             setfield(character,EMAIL,settextfield("",number,'\n',getfield(character,EMAIL),scratch_return_string),0);

                             if(!in_command) {
                                if(player != character) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN"'s %s E-mail address ("ANSI_LYELLOW"%d"ANSI_LGREEN") has been reset.",Article(character,UPPER,DEFINITE),unparse_object(player,character,0),(number == 2) ? "private":"public",number);
                                   else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Your %s E-mail address ("ANSI_LYELLOW"%d"ANSI_LGREEN") had been reset.",(number == 2) ? "private":"public",number);
			     }

                             if((player != character) && (Controller(character) != player) && !(in_command && Wizard(current_command))) {
                                if(Level4(player)) writelog(ADMIN_LOG,1,"E-MAIL","%s(#%d) reset %s(#%d)'s %s E-mail address (%d).",getname(player),player,getname(character),character,(number == 2) ? "private":"public",number);
                                   else writelog(HACK_LOG,1,"HACK","%s(#%d) reset %s(#%d)'s %s E-mail address (%d).",getname(player),player,getname(character),character,(number == 2) ? "private":"public",number);
			     }

                             if(player == character) writelog(EMAIL_LOG,1,"E-MAIL","%s(#%d) reset %s %s E-mail address (%d).",getname(player),player,Possessive(player,0),(number == 2) ? "private":"public",number);
                                else writelog(EMAIL_LOG,1,"E-MAIL","%s(#%d) reset %s(#%d)'s %s E-mail address (%d).",getname(player),player,getname(character),character,(number == 2) ? "private":"public",number);
                             setreturn(OK,COMMAND_SUCC);
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above can reset the private E-mail address of a character (If your private E-mail address is incorrect, please ask an Apprentice Wizard/Druid or above to change it for you.)");

                          /* ---->  Set/change E-mail address  <---- */
		       } else if(Level4(db[player].owner) || (number != 2) || ((number == 2) && !((address = gettextfield(number,'\n',getfield(character,EMAIL),0,scratch_return_string)) && *address))) {
                          if(!Blank(arg2)) {
                             if((((forward = !strcasecmp("forward",arg2)) && (number == 1))) || !forward) {
                                if(forward || (strlen(arg2) <= 128)) {
                                   if(forward || ok_email(player,arg2)) {
                                      if((number == 1) || ((number == 2) && gettextfield(1,'\n',getfield(character,EMAIL),0,scratch_return_string) && !strcasecmp("forward",scratch_return_string)))
                                         db[character].flags2 &= ~FORWARD_EMAIL;
                                      ansi_code_filter((char *) arg2,arg2,1);
                                      setfield(character,EMAIL,settextfield((forward) ? "Forward":arg2,number,'\n',getfield(character,EMAIL),scratch_return_string),0);

                                      if(!in_command) {
                                         if(forward) {
                                            if(player != character) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN"'s %s E-mail address ("ANSI_LYELLOW"%d"ANSI_LGREEN") has been set to forward to %s private E-mail address.",Article(character,UPPER,DEFINITE),unparse_object(player,character,0),(number == 2) ? "private":"public",number,Possessive(character,0));
                                               else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Your %s E-mail address ("ANSI_LYELLOW"%d"ANSI_LGREEN") has been set to forward to your private E-mail address.",(number == 2) ? "private":"public",number);
					 } else {
                                            if(player != character) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN"'s %s E-mail address ("ANSI_LYELLOW"%d"ANSI_LGREEN") has been set to '"ANSI_LYELLOW"%s"ANSI_LGREEN"'.",Article(character,UPPER,DEFINITE),unparse_object(player,character,0),(number == 2) ? "private":"public",number,arg2);
                                               else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Your %s E-mail address ("ANSI_LYELLOW"%d"ANSI_LGREEN") has been set to '"ANSI_LYELLOW"%s"ANSI_LGREEN"'.",(number == 2) ? "private":"public",number,arg2);
					 }
				      }

                                      if((player != character) && (Controller(character) != player) && !(in_command && Wizard(current_command))) {
                                         if(Level4(player)) writelog(ADMIN_LOG,1,"E-MAIL","%s(#%d) changed %s(#%d)'s %s E-mail address (%d) to '%s'.",getname(player),player,getname(character),character,(number == 2) ? "private":"public",number,arg2);
                                            else writelog(HACK_LOG,1,"HACK","%s(#%d) changed %s(#%d)'s %s E-mail address (%d) to '%s'.",getname(player),player,getname(character),character,(number == 2) ? "private":"public",number,arg2);
				      }

                                      if(player == character) writelog(EMAIL_LOG,1,"E-MAIL","%s(#%d) changed %s %s E-mail address (%d) to '%s'.",getname(player),player,Possessive(player,0),(number == 2) ? "private":"public",number,arg2);
                                         else writelog(EMAIL_LOG,1,"E-MAIL","%s(#%d) changed %s(#%d)'s %s E-mail address (%d) to '%s'.",getname(player),player,getname(character),character,(number == 2) ? "private":"public",number,arg2);
#ifdef WARN_DUPLICATES
                                      if(!forward && !Level4(db[character].owner))
                                         check_duplicates(character,arg2,1,0);
#endif
                                      setreturn(OK,COMMAND_SUCC);
				   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LYELLOW"%s"ANSI_LGREEN"' isn't a valid E-mail address  -  It should be in the form of '"ANSI_LWHITE"<YOUR USER NAME>"ANSI_DWHITE"@"ANSI_LWHITE"<YOUR HOST>"ANSI_LGREEN"', e.g:  '"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Example only.)",arg2,tcz_admin_email);
				} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of an E-mail address is 128 characters.");
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only your first public E-mail address ("ANSI_LYELLOW"1"ANSI_LGREEN") can be set to '"ANSI_LWHITE"forward"ANSI_LGREEN"'.");
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify your E-mail address (Or '"ANSI_LYELLOW"reset"ANSI_LGREEN"' to reset it.)");
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above can change the private E-mail address of a character (If your private E-mail address is incorrect, please ask an Apprentice Wizard/Druid or above to change it for you.)");
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you only have a maximum of "ANSI_LYELLOW"%d"ANSI_LGREEN" E-mail addresses.",EMAIL_ADDRESSES);
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify which E-mail address you would like to set (I.e:  '"ANSI_LWHITE"public"ANSI_LGREEN"' or '"ANSI_LWHITE"private"ANSI_LGREEN"'.)");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify which E-mail address you would like to set (I.e:  '"ANSI_LWHITE"public"ANSI_LGREEN"' or '"ANSI_LWHITE"private"ANSI_LGREEN"'.)");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't change %s E-mail addresses.",Article(character,LOWER,DEFINITE),getcname(player,character,1,0),Possessive(character,0));
	} else if(Level4(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change your own E-mail addresses or the E-mail addresses of someone of a lower level than yourself.");
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change your own E-mail addresses.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the E-mail addresses of a character can't be changed from within a compound command.");
}

/* ---->  Make E-mail address public/private, or list E-mail addresses of every connected character (Admin only)  <---- */
void set_email_publicprivate(CONTEXT)
{
     if(!Blank(params) && string_prefix("public",params)) {
        look_notice(player,NULL,NULL,NULL,NULL,8,0);
     } else if(!Blank(params) && string_prefix("private",params)) {
        look_notice(player,NULL,NULL,NULL,NULL,9,0);
     } else userlist_view(player,params,arg0,arg1,arg2,6,0);
}

/* ---->  Set exits string of a room or thing  <---- */
void set_estring(CONTEXT)
{
     dbref thing;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if(HasField(thing,ESTRING)) {
        if(can_write_to(player,thing,0)) {
           if(!Readonly(thing)) {
              if(!strchr(arg2,'\n')) {
                 if(strlen(arg2) <= 128) {
		    ansi_code_filter((char *) arg2,arg2,1);
                    setfield(thing,ESTRING,arg2,1);
                    if(!in_command) {
                       if(!Blank(arg2)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Exits string of %s"ANSI_LWHITE"%s"ANSI_LGREEN" set to '"ANSI_LYELLOW"%s"ANSI_LGREEN"'.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0),getfield(thing,ESTRING));
                          else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Exits string of %s"ANSI_LWHITE"%s"ANSI_LGREEN" reset.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
		    }
                    setreturn(OK,COMMAND_SUCC);
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of a exits string is 128 characters.");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a exits string mustn't contain embedded NEWLINES.");
           } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't change its exits string.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
	} else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the exits string of a room/thing which you own or a room/thing owned by someone of a lower level than yourself.");
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the exits string of a room/thing which you own.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s doesn't have a exits string.",object_type(thing,1));
}

/* ---->  Set expiry time of an object  <---- */
void set_expiry(CONTEXT)
{
     int   expiry;
     dbref thing;
 
     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if(Typeof(thing) != TYPE_CHARACTER) {
        if(can_write_to(player,thing,0)) {
           if(!Readonly(thing)) {
              expiry = Blank(arg2) ? 0:atol(arg2);
  	      if((expiry >= 0) && (expiry <= 255)) {
                 db[thing].expiry = expiry;
                 if(!in_command) {
                    if(expiry) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Expiry time of %s"ANSI_LWHITE"%s"ANSI_LGREEN" set to "ANSI_LYELLOW"%d day%s"ANSI_LGREEN".",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0),expiry,Plural(expiry));
                       else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Expiry time of %s"ANSI_LWHITE"%s"ANSI_LGREEN" reset.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
		 }
                 setreturn(OK,COMMAND_SUCC);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the expiry time must be between "ANSI_LWHITE"1"ANSI_LGREEN" and "ANSI_LWHITE"255"ANSI_LGREEN" days (Or "ANSI_LWHITE"0"ANSI_LGREEN" for no expiry.)");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't set %s expiry time.",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0),(Typeof(thing) == TYPE_CHARACTER) ? "their":"its");
	} else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only set the expiry time of something you own or something owned by someone of a lower level than yourself.");
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only set the expiry time of something you own.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the expiry time of a character can't be set (This is determined by the maintenance settings in '"ANSI_LWHITE"@admin"ANSI_LGREEN"'  -  See '"ANSI_LYELLOW""ANSI_UNDERLINE"help @admin"ANSI_LGREEN"'.)");
}

/* ---->  Set failure message of object  <---- */
void set_fail(CONTEXT)
{
     dbref thing;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if(HasField(thing,FAIL)) {
        if(CanSetField(thing,FAIL)) {
           if(can_write_to(player,thing,0)) {
              if(!Readonly(thing)) {
                 setfield(thing,FAIL,arg2,0);
                 if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Failure message of %s"ANSI_LWHITE"%s"ANSI_LGREEN" set.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
                 setreturn(OK,COMMAND_SUCC);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't change %s failure message.",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0),(Typeof(thing) == TYPE_CHARACTER) ? "their":"its");
	   } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the failure message of something you own or something owned by someone of a lower level than yourself.");
              else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the failure message of something you own.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the failure message of %s can't be set.",object_type(thing,1));
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s doesn't have a failure message.",object_type(thing,1));
}

/* ---->  Set current 'feeling' of character  <---- */
void set_feeling(CONTEXT)
{
     dbref character = player;
     int   loop;

     if(!Blank(arg2)) {
        if(!Blank(arg1)) {
           if((character = lookup_character(player,arg1,1)) == NOTHING) {
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
              return;
	   }
	} else character = db[player].owner;
        arg1 = arg2;
     }

     if(Blank(arg1) || isdigit(*arg1) || !strncasecmp(arg1,"list ",5) || !strcasecmp(arg1,"list")) {
        unsigned char            twidth = output_terminal_width(player);
        struct   descriptor_data *p = getdsc(player);

        /* ---->  List available feelings  <---- */
        if(!strncasecmp(arg1,"list ",5) || !strcasecmp(arg1,"list"))
           for(arg1 += 4; *arg1 && (*arg1 == ' '); arg1++);
        if(!strncasecmp(arg1,"page ",5) || !strcasecmp(arg1,"page"))
           for(arg1 += 4; *arg1 && (*arg1 == ' '); arg1++);
        arg1 = (char *) parse_grouprange(player,arg1,FIRST,1);

        if(IsHtml(p)) {
           html_anti_reverse(p,1);
           output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_GREY">",(in_command) ? "":"<BR>");
	} else if(!in_command && p && !p->pager && !IsHtml(p) && More(player)) pager_init(p);

        if(!in_command) {
           output(p,player,2,1,1,"%sThe following feelings are available...%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER><FONT COLOR="HTML_LCYAN" SIZE=4><I>\016":"\n ",IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
           if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
	}

        output_columns(p,player,NULL,NULL,output_terminal_width(player),1,17,2,0,1,FIRST,9,"***  NO FEELINGS ARE AVAILABLE  ***",scratch_return_string);
        union_initgrouprange((union group_data *) feelinglist);
        while(union_grouprange())
              output_columns(p,player,grp->cunion->feeling.name,ANSI_LYELLOW,0,1,17,2,0,1,DEFAULT,0,NULL,scratch_return_string);
        output_columns(p,player,NULL,NULL,0,1,17,2,0,1,LAST,0,NULL,scratch_return_string);

        if(!in_command) {
           if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','-'));
           output(p,player,2,1,1,"%sTo use one of the above feelings, simply type '"ANSI_LGREEN"@feeling <FEELING>"ANSI_LWHITE"', e.g:  '"ANSI_LGREEN"@feeling happy"ANSI_LWHITE"'.  You can reset your feeling by typing '"ANSI_LGREEN"@feeling reset"ANSI_LWHITE"'.  Type '"ANSI_LGREEN"@feeling list <PAGE>"ANSI_LWHITE"' to show other pages.%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_GREY"><TD ALIGN=CENTER>"ANSI_LWHITE"<I>\016":ANSI_LWHITE" ",IsHtml(p) ? "\016</TD></TR>\016":"\n");
           if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
           output(p,player,2,1,0,"%sFeelings listed: \016&nbsp;\016 "ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_DGREY"><TD>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",listed_items(scratch_return_string,1),IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
	}

        if(IsHtml(p)) {
           output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
           html_anti_reverse(p,0);
	}
        setreturn(OK,COMMAND_SUCC);
     } else if(!in_command || Wizard(current_command)) {
        if(can_write_to(player,character,0)) {
           if(!Readonly(character)) {
              if(string_prefix("reset",arg1) || string_prefix("none",arg1)) {

                 /* ---->  Reset feeling  <---- */
                 db[character].data->player.feeling = 0;
                 if(!in_command) {
                    if(player != character) {
                       output(getdsc(character),character,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"%s"ANSI_LYELLOW"%s"ANSI_LWHITE" has reset your feeling.",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0));
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN"'s feeling has been reset.",Article(character,UPPER,DEFINITE),getcname(player,character,0,0));
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Your feeling has been reset.");
		 }
                 setreturn(OK,COMMAND_SUCC);
	      } else {

                 /* ---->  Set feeling  <---- */
                 for(loop = 0; feelinglist[loop].name && !string_prefix(feelinglist[loop].name,arg1); loop++);
                 if(feelinglist[loop].name) {
                    db[character].data->player.feeling = feelinglist[loop].id;
                    if(!in_command) {
                       if(player != character) {
                          output(getdsc(character),character,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"%s"ANSI_LYELLOW"%s"ANSI_LWHITE" has changed your feeling to '"ANSI_LYELLOW"%s"ANSI_LWHITE"'.",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0),feelinglist[loop].name);
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN"'s feeling has been changed to '"ANSI_LYELLOW"%s"ANSI_LGREEN"'.",Article(character,UPPER,DEFINITE),getcname(player,character,0,0),feelinglist[loop].name);
		       } else {
                          strcpy(scratch_return_string,feelinglist[loop].name);
                          *scratch_return_string = tolower(*scratch_return_string);
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"You are now feeling "ANSI_LYELLOW"%s"ANSI_LGREEN".",scratch_return_string);
		       }
		    }
                    setreturn(OK,COMMAND_SUCC);
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the feeling '"ANSI_LWHITE"%s"ANSI_LGREEN"' is invalid  -  Please type '"ANSI_LYELLOW"@feeling list"ANSI_LGREEN"' and choose one of the feelings listed.",arg1);
	      }
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't change their feeling.",Article(character,LOWER,DEFINITE),getcname(player,character,1,0));
	} else if(Level3(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change your own feeling or the feeling of someone of a lower level than yourself.");
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change your own feeling.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the feeling of a character can't be changed from within a compound command.");
}

/* ---->  Set/reset single flag by name on specified object  <---- */
int set_flag_by_name(dbref player,dbref thing,const char *flagname,int reset,const char *reason,unsigned char write,int fflags)
{
    int           pos,flag = 0,mask = 0,primary = 1,error = 0,level = 0,status = 0;
    char          realname[256],*dest;
    unsigned char privs = 0,skip;
    const    char *src;

    /* ---->  Look up flag in primary flags list  <---- */
    pos = 0, skip = 0;
    while(!flag && !mask && !skip && flag_list[pos].string)
          if(string_prefix(flag_list[pos].string,flagname)) {
             if(!(flag_list[pos].flags & FLAG_SKIP)) {
                for(src = flag_list[pos].string, dest = realname; !Blank(src); *dest++ = toupper(*src++)); 
                *dest = '\0';
                flag  = flag_list[pos].flag;
                mask  = flag_list[pos].mask;
	     } else skip = 1;
	  } else pos++;

    /* ---->  Look up flag in secondary flags list (If not found in primary)  <---- */
    if(!flag) {
       pos = 0, skip = 0, primary = 0;
       while(!flag && !mask && !skip && flag_list2[pos].string)
             if(string_prefix(flag_list2[pos].string,flagname)) {
                if(!(flag_list2[pos].flags & FLAG_SKIP)) {
                   for(src = flag_list2[pos].string, dest = realname; !Blank(src); *dest++ = toupper(*src++));
                   *dest = '\0';
                   flag  = flag_list2[pos].flag;
                   mask  = flag_list2[pos].mask;
		} else skip = 1;
	     } else pos++;
    }

    if(!flag) {
       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"%s"ANSI_LGREEN"' is an unknown flag.",flagname);
       return(0);
    }

    if(Readonly(thing) && !(reset && primary && (flag == READONLY))) {
       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't %s %s "ANSI_LYELLOW"%s"ANSI_LGREEN" flag.",Article(thing,LOWER,DEFINITE),getcname(NOTHING,thing,0,0),(reset) ? "reset":"set",(Typeof(thing) == TYPE_CHARACTER) ? "their":"its",realname);
       return(0);
    }

    if(primary) {  /* ---->  Primary flag constraints  <---- */

       /* ---->  '@set me = ansi' is obsolete (Use 'set ansi on')  <---- */
       if(flag == ANSI) {
          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please type '"ANSI_LWHITE"set ansi %s"ANSI_LGREEN"' to set your ANSI colour preference.",(reset) ? "off":"on");
          return(0);
       }

       /* ---->  Internal flag?  <---- */
       if(flag_list[pos].flags & FLAG_INTERNAL) error = 1;

       /* ---->  Flag can't be (re)set on object of that type?  <---- */
       if(!error && !(flag_map[Typeof(thing)] & flag)) error = 2;

       /* ---->  Flag can't be (re)set on character?  <---- */
       if(!error && (flag_list[pos].flags & FLAG_NOT_CHARACTER) && (Typeof(thing) == TYPE_CHARACTER)) error = 2;

       /* ---->  Flag can't be (re)set on object?  <---- */
       if(!error && (flag_list[pos].flags & FLAG_NOT_OBJECT) && (Typeof(thing) != TYPE_CHARACTER)) error = 2;

       /* ---->  Flag can't be (re)set by Apprentice Wizard/Druid on someone else?  <---- */
       if(!error && !(flag_list[pos].flags & FLAG_APPRENTICE) && (Level4(db[player].owner) && !Level3(db[player].owner)) && (player != thing) && (Typeof(thing) == TYPE_CHARACTER) && (player != Controller(thing))) error = 8;

       /* ---->  Flag can't be (re)set within compound command?  <---- */
       if(!error && (flag_list[pos].flags & FLAG_NOT_INCOMMAND) && in_command) error = 3;

       /* ---->  Flag can't be (re)set on character within compound command?  <---- */
       if(!error && (flag_list[pos].flags & FLAG_NOT_CHCOMMAND) && (Typeof(thing) == TYPE_CHARACTER) && in_command) error = 4;

       /* ---->  Flag can't be (re)set on yourself  <---- */
       if(!error && (flag_list[pos].flags & FLAG_NOT_OWN) && (player == thing)) error = 5;

       /* ---->  Flag can't be (re)set on one of YOUR puppets  <---- */
       if(!error && !Level4(db[player].owner) && (flag_list[pos].flags & FLAG_OWN_PUPPET) && (Typeof(thing) == TYPE_CHARACTER) && (player == Controller(thing))) error = 9;

       /* ---->  Must be of given level?  <---- */
       if(!error && (privilege(db[player].owner,255) > (flag_list[pos].flags & LEVEL_MASK))) {
          level = (flag_list[pos].flags & LEVEL_MASK);
          error = 6;
       }
       if((flag_list[pos].flags & FLAG_EXPERIENCED) && ((Experienced(db[player].owner) && (Typeof(thing) == TYPE_CHARACTER)) || Assistant(db[player].owner))) error = 6, level = 3;

       /* ---->  Flag can't be set on puppet?  <---- */
       if(!error && !reset && (flag_list[pos].flags & FLAG_NOT_PUPPET) && (Typeof(thing) == TYPE_CHARACTER) && (Controller(thing) != thing) && !((flag_list[pos].flags & FLAG_ADMIN_PUPPET) && Level4(Controller(thing))) && !((flag_list[pos].flags & FLAG_EXP_PUPPET) && (Experienced(Controller(thing)) || Assistant(Controller(thing))))) error = 7;

       /* ---->  Reason must be given for (re)setting flag?  <---- */
       if(!error && (flag_list[pos].flags & FLAG_REASON) && (!(flag_list[pos].flags & FLAG_REASON_MORTAL) || Level4(thing)) && Blank(reason)) error = 10;

    } else {  /* ---->  Secondary flag constraints  <---- */

       /* ---->  Internal flag?  <---- */
       if(flag_list2[pos].flags & FLAG_INTERNAL) error = 1;

       /* ---->  Flag can't be (re)set on object of that type?  <---- */
       if(!error && !(flag_map2[Typeof(thing)] & flag)) error = 2;

       /* ---->  Flag can't be (re)set on character?  <---- */
       if(!error && (flag_list2[pos].flags & FLAG_NOT_CHARACTER) && (Typeof(thing) == TYPE_CHARACTER)) error = 2;

       /* ---->  Flag can't be (re)set on object?  <---- */
       if(!error && (flag_list2[pos].flags & FLAG_NOT_OBJECT) && (Typeof(thing) != TYPE_CHARACTER)) error = 2;

       /* ---->  Flag can't be (re)set by Apprentice Wizard/Druids on someone else?  <---- */
       if(!error && !(flag_list2[pos].flags & FLAG_APPRENTICE) && (Level4(db[player].owner) && !Level3(db[player].owner)) && (player != thing) && (Typeof(thing) == TYPE_CHARACTER) && (player != Controller(thing))) error = 8;

       /* ---->  Flag can't be (re)set within compound command?  <---- */
       if(!error && (flag_list2[pos].flags & FLAG_NOT_INCOMMAND) && in_command) error = 3;

       /* ---->  Flag can't be (re)set on character within compound command?  <---- */
       if(!error && (flag_list2[pos].flags & FLAG_NOT_CHCOMMAND) && (Typeof(thing) == TYPE_CHARACTER) && in_command) error = 4;

       /* ---->  Flag can't be (re)set on yourself  <---- */
       if(!error && (flag_list2[pos].flags & FLAG_NOT_OWN) && (player == thing)) error = 5;

       /* ---->  Flag can't be (re)set on one of YOUR puppets  <---- */
       if(!error && !Level4(db[player].owner) && (flag_list2[pos].flags & FLAG_OWN_PUPPET) && (Typeof(thing) == TYPE_CHARACTER) && (player == Controller(thing))) error = 9;

       /* ---->  Must be of given level?  <---- */
       if(!error && (privilege(db[player].owner,255) > (flag_list2[pos].flags & LEVEL_MASK))) {
          level = (flag_list2[pos].flags & LEVEL_MASK);
          error = 6;
       }
       if((flag_list2[pos].flags & FLAG_EXPERIENCED) && ((Experienced(db[player].owner) && (Typeof(thing) == TYPE_CHARACTER)) || Assistant(db[player].owner))) error = 11;

       /* ---->  Flag can't be set on puppet?  <---- */
       if(!error && !reset && (flag_list2[pos].flags & FLAG_NOT_PUPPET) && (Typeof(thing) == TYPE_CHARACTER) && (Controller(thing) != thing) && !((flag_list2[pos].flags & FLAG_ADMIN_PUPPET) && Level4(Controller(thing))) && !((flag_list2[pos].flags & FLAG_EXP_PUPPET) && (Experienced(Controller(thing)) || Assistant(Controller(thing))))) error = 7;

       /* ---->  Reason must be given for (re)setting flag?  <---- */
       if(!error && (flag_list2[pos].flags & FLAG_REASON) && (!(flag_list2[pos].flags & FLAG_REASON_MORTAL) || Level4(thing)) && Blank(reason)) error = 10;
    }

    /* ---->  Handle Druid flag  <---- */
    if(primary && (flag == DRUID)) {
       if(Level1(thing)) status = DEITY;
          else if(Level2(thing)) status = ELDER;
             else if(Level3(thing)) status = WIZARD;
                else if(Level4(thing)) status = APPRENTICE;
                   else status = DRUID;
    }

    /* ---->  Primary flags miscellaneous contraints/victim notification for specific flag types (E.g:  Builder, Wizard, etc.)  <---- */
    if(!error && primary) switch((flag == DRUID) ? status:flag) {
       case APPRENTICE:
            if(!reset) {
               db[thing].flags &= ~(DEITY|ELDER|WIZARD|EXPERIENCED|ASSISTANT|MORON);
               if(Typeof(thing) == TYPE_CHARACTER) {
                  if(db[thing].data->player.maillimit < MAIL_LIMIT_ADMIN) db[thing].data->player.maillimit = MAIL_LIMIT_ADMIN;
                  if(!Level4(thing)) db[thing].flags |= (BOOT|SHOUT);
                  db[thing].flags2 &= ~RETIRED;
	       }
	    } else if(Typeof(thing) == TYPE_CHARACTER) {
               db[thing].data->player.maillimit = MAIL_LIMIT_MORTAL;
               if(flag == DRUID) db[thing].flags &= ~APPRENTICE;
               db[thing].flags  &= ~DRUID;
	    }
            privs = 1;
            break;
       case ASHCAN:
            if(!reset && (RoomZero(thing) || Root(thing) || Start(thing) || Global(thing))) {
               output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"ASHCAN"ANSI_LGREEN" flag can't be set on that object (It's required internally.)");
               return(SEARCH_ALL);
	    } else if(!reset && (Level4(thing) || Experienced(thing) || Assistant(thing))) {
               output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"ASHCAN"ANSI_LGREEN" flag can't be set on an Assistant, Experienced Builder or an Apprentice Wizard/Druid or above.");
               return(SEARCH_ALL);
	    } else if(!reset && (write == 2) && !(fflags & FRIEND_DESTROY)) {
               output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you must have the "ANSI_LYELLOW"DESTROY"ANSI_LGREEN" friend flag set on you to set the "ANSI_LYELLOW"ASHCAN"ANSI_LGREEN" flag on objects owned by %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(db[thing].owner,LOWER,INDEFINITE),getcname(NOTHING,db[thing].owner,0,0));
               return(SEARCH_ALL);
	    }
            break;
       case ASSISTANT:
            if((Typeof(thing) == TYPE_CHARACTER) && !reset) {
               if(!(!Level4(thing) && !Moron(thing) && !Retired(thing))) {
                  output(getdsc(player),player,0,1,0,Moron(thing) ? ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"ASSISTANT"ANSI_LGREEN" flag can't be set on a Moron.":Retired(thing) ? ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"ASSISTANT"ANSI_LGREEN" flag can't be set on a retired administrator.":ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"ASSISTANT"ANSI_LGREEN" flag can't be set on a Apprentice Wizard/Druid or above.");
                  return(SEARCH_ALL);
	       } else db[thing].flags &= ~EXPERIENCED;
	    }
            privs = 1;
            break;
       case BUILDER:
            if(Level4(thing)) {
               if(!(Level4(db[player].owner) && Boot(db[player].owner))) {
                  output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't %s the "ANSI_LYELLOW"BOOT"ANSI_LGREEN" flag when you don't have one yourself.",(reset) ? "reset":"set");
                  return(SEARCH_ALL);
	       }
	    } else if(Moron(thing)) {
               output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"BUILDER"ANSI_LGREEN" flag can't be set on a Moron.");
               return(SEARCH_ALL);
	    } else if(Puppet(thing)) {
               db[thing].data->player.quotalimit = 0;
	    } else if(db[thing].data->player.quotalimit < STANDARD_CHARACTER_QUOTA) {
               db[thing].data->player.quotalimit = STANDARD_CHARACTER_QUOTA;
	    }
            privs = 1;
            break;
       case CENSOR:
            if(!Level3(Owner(player)) && (Typeof(thing) == TYPE_COMMAND)) {
               output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"CENSORED"ANSI_LGREEN" flag can only be %sset on compound commands by Wizards/Druids and above.",(reset) ? "re":"");
               return(SEARCH_ALL);
	    }
            break;
       case DEITY:
            if(flag == DRUID) {
               output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"DRUID"ANSI_LGREEN" flag can't be set on a Deity.");
               return(SEARCH_ALL);
	    } else if(!reset) {
               if((Typeof(thing) == TYPE_CHARACTER) && (db[thing].data->player.maillimit < MAIL_LIMIT_ADMIN)) db[thing].data->player.maillimit = MAIL_LIMIT_ADMIN;
               db[thing].flags &= ~(ELDER|WIZARD|APPRENTICE|DRUID|EXPERIENCED|ASSISTANT|MORON);
               if(!Level4(thing)) db[thing].flags |= (BOOT|SHOUT);
               db[thing].flags2 &= ~RETIRED;
	    } else {
               if(Typeof(thing) == TYPE_CHARACTER) db[thing].data->player.maillimit = MAIL_LIMIT_MORTAL;
               db[thing].flags  &= ~DRUID;
	    }
            privs = 1;
            break;
       case DRUID:
            if(Typeof(thing) == TYPE_CHARACTER)
   	       if(!reset) {
                  if(db[thing].data->player.maillimit < MAIL_LIMIT_ADMIN) db[thing].data->player.maillimit = MAIL_LIMIT_ADMIN;
                  db[thing].flags &= ~(ASSISTANT|EXPERIENCED|MORON);
                  if(!Level4(thing) && !Retired(thing)) db[thing].flags |= APPRENTICE|BOOT|SHOUT;
	       }
            break;
       case ELDER:
            if(!reset) {
               if((Typeof(thing) == TYPE_CHARACTER) && (db[thing].data->player.maillimit < MAIL_LIMIT_ADMIN)) db[thing].data->player.maillimit = MAIL_LIMIT_ADMIN;
               db[thing].flags &= ~(DEITY|WIZARD|APPRENTICE|EXPERIENCED|ASSISTANT|MORON);
               if(!Level4(thing)) db[thing].flags |= (BOOT|SHOUT);
               db[thing].flags2 &= ~RETIRED;
	    } else {
               if(Typeof(thing) == TYPE_CHARACTER) db[thing].data->player.maillimit = MAIL_LIMIT_MORTAL;
               if(flag == DRUID) db[thing].flags &= ~ELDER;
               db[thing].flags &= ~DRUID;
	    }
            privs = 1;
            break;
       case EXPERIENCED:
            if(Typeof(thing) == TYPE_CHARACTER) {
   	       if(!reset) {
	          if(!Level4(thing) && !Moron(thing) && !Retired(thing)) {
                     if(Puppet(thing)) {
                        db[thing].data->player.quotalimit = 0;
		     } else if(db[thing].data->player.quotalimit < STANDARD_CHARACTER_QUOTA) {
                        db[thing].data->player.quotalimit = STANDARD_CHARACTER_QUOTA;
		     }
                     db[thing].flags &= ~(ASSISTANT|HELP);
		  } else {
                     output(getdsc(player),player,0,1,0,Moron(thing) ? ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"EXPERIENCED"ANSI_LGREEN" flag can't be set on a Moron.":Retired(thing) ? ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"EXPERIENCED"ANSI_LGREEN" flag can't be set on a retired administrator.":ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"EXPERIENCED"ANSI_LGREEN" flag can't be set on an Apprentice Wizard/Druid or above.");
                     return(SEARCH_ALL);
		  }
	       } else db[thing].flags |= BUILDER;
	    }
            privs = 1;
            break;
       case INVISIBLE:
            if(!Level4(db[player].owner) && !Builder(player)) {
               if(!reset) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Builders and above can make objects invisible.");
                  else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Builders and above can make invisible objects visible again.");
               return(SEARCH_ALL);
	    } else if(reset && RoomZero(Location(thing)) && !Level4(Owner(player))) {
               output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may reset the "ANSI_LYELLOW"INVISIBLE"ANSI_LGREEN" flag on objects currently located in "ANSI_LWHITE"%s"ANSI_LGREEN".",unparse_object(player,ROOMZERO,0));
               return(SEARCH_ALL);
	    }
            break;
       case LISTEN:
            if(!reset) {
               struct descriptor_data *d = descriptor_list;

               for(; d; d = d->next)
                   if(d->player == thing)
                      d->flags &= ~NOLISTEN;
	    }
            break;
       case MORON:
            if(!reset) {
               db[thing].flags  &= ~(DEITY|ELDER|WIZARD|APPRENTICE|DRUID|EXPERIENCED|ASSISTANT|BUILDER|YELL);
               db[thing].flags2 &= ~RETIRED;
               db[thing].flags  |=  CENSOR;
  	    } else {
               db[thing].flags2 &= ~RETIRED;
               db[thing].flags  &= ~CENSOR;
               db[thing].flags  |=  YELL;
	    }
            privs = 1;
            break;
       case OPEN:

            /* ---->  Container with contents can't be set !OPEN/!OPENABLE  <---- */
            if(reset && Valid(db[thing].contents) && Container(thing)) {
               output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please remove "ANSI_LWHITE"%s"ANSI_LGREEN"'s contents first before resetting its "ANSI_LYELLOW"%s"ANSI_LGREEN" flag.",getname(thing),realname);
               return(SEARCH_ALL);
	    }
            break;
       case READONLY:

            /* ---->  READONLY flag can't be reset by Experienced Builder on compound command set VALIDATED  <---- */
            if(reset && !Level4(player)) {
               if(Validated(thing)) {
                  output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"READONLY"ANSI_LGREEN" flag can only be reset by an Apprentice Wizard/Druid or above on %s with its "ANSI_LYELLOW"VALIDATED"ANSI_LGREEN" flag set.",object_type(thing,1));
                  return(SEARCH_ALL);
	       } else if((Typeof(thing) == TYPE_COMMAND) && Level4(thing)) {
                  output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"READONLY"ANSI_LGREEN" flag can only be reset by an Apprentice Wizard/Druid or above on %s with its "ANSI_LYELLOW"APPRENTICE"ANSI_LGREEN" or "ANSI_LYELLOW"WIZARD"ANSI_LGREEN" flag set.",object_type(thing,1));
                  return(SEARCH_ALL);
	       }
	    }
            break;
       case SHARABLE:

            /* ---->  TRANSFERABLE flag can't be set/reset on object without DESTROY friend flag (Friend flag privileges only)  <---- */
            if((write == 2) && (fflags & FRIEND_SHARABLE)) {
               output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have the "ANSI_LYELLOW"SHARABLE"ANSI_LGREEN" friend flag set on you by %s"ANSI_LWHITE"%s"ANSI_LGREEN".  You can't %s the "ANSI_LYELLOW"SHARABLE"ANSI_LGREEN" flag on %s objects.",Article(db[thing].owner,LOWER,INDEFINITE),getcname(NOTHING,db[thing].owner,0,0),(reset) ? "reset":"set",Possessive(db[thing].owner,0));
               return(SEARCH_ALL);
	    }
            break;
       case TRANSFERABLE:

            /* ---->  TRANSFERABLE flag can't be set/reset on object without DESTROY friend flag (Friend flag privileges only)  <---- */
            if(!reset && (write == 2) && !(fflags & FRIEND_DESTROY)) {
               output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you must have the "ANSI_LYELLOW"DESTROY"ANSI_LGREEN" friend flag set on you to set the "ANSI_LYELLOW"TRANSFERABLE"ANSI_LGREEN" flag on objects owned by %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(db[thing].owner,LOWER,INDEFINITE),getcname(NOTHING,db[thing].owner,0,0));
               return(SEARCH_ALL);
	    }
            break;
       case WIZARD:
            if(!reset) {
               db[thing].flags &= ~(DEITY|ELDER|APPRENTICE|EXPERIENCED|ASSISTANT|MORON);
               if(Typeof(thing) == TYPE_CHARACTER) {
                  if(db[thing].data->player.maillimit < MAIL_LIMIT_ADMIN) db[thing].data->player.maillimit = MAIL_LIMIT_ADMIN;
                  if(!Level4(thing)) db[thing].flags |= (BOOT|SHOUT);
                  db[thing].flags2 &= ~RETIRED;
	       }
	    } else if(Typeof(thing) == TYPE_CHARACTER) {
               db[thing].data->player.maillimit = MAIL_LIMIT_MORTAL;
               if(flag == DRUID) db[thing].flags &= ~WIZARD;
               db[thing].flags  &= ~DRUID;
	    }
            privs = 1;
            break;
       case YELL:
            if(Typeof(thing) == TYPE_CHARACTER)
               if(Level4(db[player].owner) && !Shout(db[player].owner)) {
                  output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't %s the "ANSI_LYELLOW"SHOUT"ANSI_LGREEN" flag when you don't have one yourself.",(reset) ? "reset":"set");
                  return(SEARCH_ALL);
	       }
            break;
    } else if(!error) {
       switch(flag) {
	      case RETIRED:
		   if(Typeof(thing) == TYPE_CHARACTER) {
		      if(!reset) {
			 db[thing].data->player.maillimit = MAIL_LIMIT_MORTAL;
			 db[thing].flags &= ~(MORON|ASSISTANT|EXPERIENCED|APPRENTICE|WIZARD|ELDER|DEITY|HELP);
		      } else if(Retired(thing)) db[thing].flags &= ~DRUID;
			 else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"RETIRED"ANSI_LGREEN" flag may only be reset on retired administrators.");
		   }
		   privs = 1;
		   break;
	      case TRANSPORT:
		   if(!reset && in_area(thing,player)) {
		      output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"TRANSPORT"ANSI_LGREEN" flag can't be set on %s which you're currently carrying.",object_type(thing,1));
		      return(SEARCH_ALL);
		   }
		   break;
       }
    }

    /* ---->  Set/reset flag or display description of error (If there was one)  <---- */
    switch(error) {
           case SEARCH_ALL:
                break;
           case 1:
                output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"%s"ANSI_LGREEN" flag is used internally and can't be %s by you.",realname,(reset) ? "reset":"set");
                break;
           case 2:
                output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"%s"ANSI_LGREEN" flag can't be %s on %s.",realname,(reset) ? "reset":"set",object_type(thing,1));
                break;
           case 3:
                output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"%s"ANSI_LGREEN" flag can't be %s from within a compound command.",realname,(reset) ? "reset":"set");
                break;
           case 4:
                output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"%s"ANSI_LGREEN" flag can't be %s on a character from within a compound command.",realname,(reset) ? "reset":"set");
                break;
           case 5:
                output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't %s your "ANSI_LYELLOW"%s"ANSI_LGREEN" flag.",(reset) ? "reset":"set",realname);
                break;
           case 6:
           case 11:
                output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"%s"ANSI_LGREEN" flag can only be %s%s by %s.",realname,(reset) ? "reset":"set",(error == 11) ? " on characters":"",clevels[(error == 11) ? 4:level]);
                break;
           case 7:
                output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"%s"ANSI_LGREEN" flag can't be %s on a puppet.",realname,(reset) ? "reset":"set");
                break;
           case 8:
                output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Wizards/Druids and above can %s the "ANSI_LYELLOW"%s"ANSI_LGREEN" flag on another character.",(reset) ? "reset":"set",realname);
                break;
           case 9:
                output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't %s the "ANSI_LYELLOW"%s"ANSI_LGREEN" flag on one of your puppets.",(reset) ? "reset":"set",realname);
                break;
           case 10:
                output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the reason(s) for %sting the "ANSI_LYELLOW"%s"ANSI_LGREEN" flag on %s"ANSI_LWHITE"%s"ANSI_LGREEN".",(reset) ? "reset":"set",realname,Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0));
                break;
           default:

                /* ---->  Set/reset flag  <---- */
                if(reset) {
                   if(primary) db[thing].flags &= ~mask;
		      else db[thing].flags2    &= ~mask;
		} else if(primary) {
                   db[thing].flags  &= ~mask;
                   db[thing].flags  |=  flag;
		} else {
                   db[thing].flags2 &= ~mask;
                   db[thing].flags2 |=  flag;
		}

                /* ---->  Log flag set/reset?  <---- */
                if((primary) ? (flag_list[pos].flags & FLAG_LOG):(flag_list2[pos].flags & FLAG_LOG)) {
                   const char *rptr = NULL;
                   if(reason) rptr = (char *) punctuate((char *) reason,2,'.');
                   if(((Typeof(thing) == TYPE_CHARACTER) && ((primary) ? (flag_list[pos].flags & FLAG_LOG_CHARACTER):(flag_list2[pos].flags & FLAG_LOG_CHARACTER))) || ((Typeof(thing) != TYPE_CHARACTER) && ((primary) ? (flag_list[pos].flags & FLAG_LOG_OBJECT):(flag_list2[pos].flags & FLAG_LOG_OBJECT))))
                      writelog(FLAGS_LOG,1,"SET","%s(#%d) %s %s flag on %s(#%d)%s%s",getname(player),player,(reset) ? "reset":"set",realname,getname(thing),thing,(!Blank(rptr)) ? "  -  REASON:  ":".",(!Blank(rptr)) ? rptr:"");
		}

                /* ---->  Check privileges obtained through friend flags?  <---- */
                if(privs) friendflags_privs(thing);

	        /* ---->  Flag that affects global compound command?  <---- */
                if((Typeof(thing) == TYPE_COMMAND) && Global(effective_location(thing))) {
                   if((primary && ((flag == READONLY) || (flag == WIZARD))) || (!primary && (flag == VALIDATED))) {
                      global_delete(thing);
                      global_add(thing);
		   }
		}

                /* ---->  Inform user setting/resetting flag  <---- */
                if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LYELLOW"%s"ANSI_LGREEN" flag %s on %s"ANSI_LWHITE"%s"ANSI_LGREEN".",realname,(reset) ? "reset":"set",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0));

                /* ---->  Inform user who flag was set/reset on  <---- */
                if((Typeof(thing) == TYPE_CHARACTER) && (thing != player)) {
                   const char *colour = (reset) ? ANSI_LRED:ANSI_LGREEN;
                   const char *rptr = NULL;

                   if(reason)
                      rptr = substitute(thing,scratch_return_string,punctuate((char *) reason,2,'.'),0,ANSI_LWHITE,NULL,0);
		         else *scratch_return_string = '\0';
                   output(getdsc(thing),thing,0,1,0,"%s[%s"ANSI_LWHITE"%s%s has %sset your "ANSI_LYELLOW"%s%s flag%s%s%s]",colour,Article(player,UPPER,(Location(player) == Location(thing)) ? DEFINITE:INDEFINITE),getcname(NOTHING,player,0,0),colour,(reset) ? "re":"",realname,colour,(!Blank(rptr)) ? "  -  REASON:  "ANSI_LWHITE:".",(!Blank(rptr)) ? rptr:"",(!Blank(rptr)) ? colour:"");
		}
    }
    return(error);
}

/* ---->  Set/reset flag(s) on object  <---- */
void set_flag(CONTEXT)
{
     int           failed = 0,fflags = 0,reset;
     char          *flaglist,*reason;
     unsigned char write;
     dbref         thing;
     char          *p1;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     split_params((char *) arg2,&flaglist,&reason);
     if(!Blank(flaglist)) {
        if((write = can_write_to(player,thing,(Typeof(thing) == TYPE_CHARACTER)))) {

           /* ---->  Parse flag names and set/reset them on object  <---- */
           if(write) fflags = friend_flags(db[thing].owner,player);
           while(*flaglist) {
                 while(*flaglist && (*flaglist == ' ')) flaglist++;
                 if(*flaglist && (*flaglist == '!')) flaglist++, reset = 1;
	            else reset = 0;

                 /* ---->  Flag name  <---- */
                 for(p1 = scratch_buffer; *flaglist && (*flaglist != ' '); *p1++ = *flaglist, flaglist++);
                 *p1 = '\0';

                 if(!BlankContent(scratch_buffer)) failed |= set_flag_by_name(player,thing,scratch_buffer,reset,reason,write,fflags);
                    else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify which flag you'd like to reset.");
	   }
           if(!failed) setreturn(OK,COMMAND_SUCC);
	} else if(Typeof(thing) == TYPE_CHARACTER) {
           if(Level4(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only set/reset your own flags or those of someone who's of a lower level than yourself.");
              else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only set/reset your own flags.");
	} else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only set/reset flags of something you own or something owned by someone of a lower level than yourself.");
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only set/reset flags of something you own.");
     } else if(thing == player) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify which flags you'd like to set/reset on yourself.");
        else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify which flags you'd like to set/reset on this object.");
}

/* ---->  Set lock key of object  <---- */
void set_key(CONTEXT)
{
     struct boolexp *key;
     dbref  thing;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if(Container(thing)) {
        if(can_write_to(player,thing,0)) {
           if(!Readonly(thing)) {
              if(!Blank(arg2)) {
                 if((key = parse_boolexp(player,(char *) arg2)) == TRUE_BOOLEXP) {
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, I don't understand that key.");
                    return;
		 }
	      } else key = TRUE_BOOLEXP;

              free_boolexp(&(db[thing].data->thing.lock_key));
              db[thing].data->thing.lock_key = key;
              if(!in_command) {
                 if(Blank(arg2)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" no-longer requires a key to be locked/unlocked.",Article(thing,UPPER,DEFINITE),unparse_object(player,thing,0));
                    else output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" now requires a key to be locked/unlocked.",Article(thing,UPPER,DEFINITE),unparse_object(player,thing,0));
	      }
              setreturn(OK,COMMAND_SUCC);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't change %s key.",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0),(Typeof(thing) == TYPE_CHARACTER) ? "their":"its");
	} else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the key of something you own or something owned by someone of a lower level than yourself.");
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the key of something you own.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only containers can have a key.");
}

/* ---->  Set home of object, destination of exit or drop-to location of room  <---- */
void set_link(CONTEXT)
{
     dbref object,destination;

     setreturn(ERROR,COMMAND_FAIL);
     object = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(object)) return;

     if(!((Typeof(object) != TYPE_EXIT) && !can_write_to(player,object,0))) {
        if(!Blank(arg2)) {
           destination = parse_link_destination(player,object,arg2,ABODE);
           if(!Valid(destination) && (destination != HOME)) return;
	}

        switch(Typeof(object)) {
               case TYPE_EXIT:
                    if(!Blank(arg2)) {
                       if(destination != HOME) {
                          if(!(Valid(Destination(object)) && !can_write_to(player,object,0))) {
  	    	             if(!((Typeof(destination) != TYPE_ROOM) && (Typeof(destination) != TYPE_THING))) {
		                if(Builder(db[player].owner)) {
                                   if(Owner(object) != Owner(player)) {

                                      /* ---->  Transfer Building Quota used by exit to new owner  <---- */
                                      if(!adjustquota(player,db[player].owner,EXIT_QUOTA)) {
                                         warnquota(player,db[player].owner,"to link that exit and take over its ownership");
                                         return;
				      }
                                      adjustquota(player,Owner(object),0 - EXIT_QUOTA);
	  			   }
                                   db[object].destination = destination;
                                   db[object].owner       = Owner(player);
                                   if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Exit linked to %s"ANSI_LYELLOW"%s"ANSI_LGREEN".",Article(destination,LOWER,DEFINITE),unparse_object(player,destination,0));
                                   setreturn(OK,COMMAND_SUCC);
				} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Builders may link exits.");
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only link %s"ANSI_LWHITE"%s"ANSI_LGREEN" to a room or container.",Article(object,LOWER,DEFINITE),unparse_object(player,object,0));
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is already linked.",Article(object,LOWER,DEFINITE),unparse_object(player,object,0));
                       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't link %s"ANSI_LWHITE"%s"ANSI_LGREEN" to "ANSI_LYELLOW""HOME_STRING""ANSI_LGREEN".",Article(object,LOWER,DEFINITE),unparse_object(player,object,0));
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s destination (Use '"ANSI_LWHITE"@unlink"ANSI_LGREEN"' to reset it.)",Article(object,LOWER,DEFINITE),unparse_object(player,object,0));
                    break;
               case TYPE_CHARACTER:
                    if(!Blank(arg2)) {
                       if(destination != HOME) {
                          if((Typeof(destination) != TYPE_ROOM) && (Typeof(destination) != TYPE_THING)) {
                             output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only set %s home to a room or container.",(object == player) ? "your":"a character's");
                             return;
			  }
                          if((Owner(object) != Owner(destination)) && !friendflags_check(player,object,Owner(destination),FRIEND_LINK,"Link")) return;
		       }
		    } else {
                       if(object == player) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify your new home location.");
                          else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s new home location.",Article(object,LOWER,DEFINITE),getcname(NOTHING,object,0,0));
                       return;
		    }
               case TYPE_THING:
                    if(!Blank(arg2)) {
                       if(destination != HOME) {
                          db[object].destination = destination;
                          if(!in_command) {
    		             if(object != player) {
                                sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN"'s home is now ",Article(object,UPPER,DEFINITE),getcname(player,object,1,0));
                                output(getdsc(player),player,0,1,0,"%s%s"ANSI_LYELLOW"%s"ANSI_LGREEN".",scratch_buffer,Article(destination,LOWER,DEFINITE),unparse_object(player,destination,0));
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Your home is now %s"ANSI_LYELLOW"%s"ANSI_LGREEN".",Article(destination,LOWER,DEFINITE),unparse_object(player,destination,0));
			  }
                          setreturn(OK,COMMAND_SUCC);
                       } else if(Typeof(object) == TYPE_CHARACTER) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't set %s home to "ANSI_LYELLOW""HOME_STRING""ANSI_LGREEN".",(object == player) ? "your":"a character's");
                          else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't set %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s home to "ANSI_LYELLOW""HOME_STRING""ANSI_LGREEN".",Article(object,LOWER,DEFINITE),unparse_object(player,object,0));
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s new home location.",Article(object,LOWER,DEFINITE),unparse_object(player,object,0));
                    break;
               case TYPE_ROOM:
                    if(!Blank(arg2)) {
                       if(destination != HOME) {
        	          if((Typeof(destination) == TYPE_ROOM) || (Typeof(destination) == TYPE_THING)) {
                             db[object].destination = destination;
                             if(!in_command) {
                                sprintf(scratch_buffer,ANSI_LGREEN"Drop-to of %s"ANSI_LWHITE"%s"ANSI_LGREEN" set to ",Article(object,LOWER,DEFINITE),getcname(player,object,1,0));
                                output(getdsc(player),player,0,1,0,"%s%s"ANSI_LYELLOW"%s"ANSI_LGREEN".",scratch_buffer,Article(destination,LOWER,DEFINITE),unparse_object(player,destination,0));
			     }
                             setreturn(OK,COMMAND_SUCC);
		          } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the drop-to of a room can't be set to %s.",object_type(destination,1));
                       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't set the drop-to of a room to "ANSI_LYELLOW""HOME_STRING""ANSI_LGREEN".");
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s drop-to location (Use '"ANSI_LWHITE"@unlink"ANSI_LGREEN"' to reset it.)",Article(object,LOWER,DEFINITE),unparse_object(player,object,0));
                    break;
               default:
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s can't be linked.",object_type(object,1));
	}
     } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only link something you own or something owned by someone of a lower level than yourself.");
        else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only link something you own.");
}

/* ---->  Set lock of object  <---- */
void set_lock(CONTEXT)
{
     struct boolexp *key;
     dbref  thing;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if(can_write_to(player,thing,0)) {
        if(!Readonly(thing)) {
           if(Blank(arg2)) {
              if(HasField(thing,LOCK)) {

                 /* ---->  No lock specified  -  Unlock object  <---- */
                 switch(Typeof(thing)) {
                        case TYPE_EXIT:
                             free_boolexp(&(db[thing].data->exit.lock));
                             db[thing].data->exit.lock = TRUE_BOOLEXP;
                             break;
                        case TYPE_FUSE:
                             free_boolexp(&(db[thing].data->fuse.lock));
                             db[thing].data->fuse.lock = TRUE_BOOLEXP;
                             break;
                        case TYPE_ROOM:
                             free_boolexp(&(db[thing].data->room.lock));
                             db[thing].data->room.lock = TRUE_BOOLEXP;
                             break;
                        case TYPE_THING:
                             free_boolexp(&(db[thing].data->thing.lock));
                             db[thing].data->thing.lock = TRUE_BOOLEXP;
                             break;
                        case TYPE_COMMAND:
                             free_boolexp(&(db[thing].data->command.lock));
                             db[thing].data->command.lock = TRUE_BOOLEXP;
                             break;
		 }
                 if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" unlocked.",Article(thing,UPPER,INDEFINITE),unparse_object(player,thing,0));
                 setreturn(OK,COMMAND_SUCC);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s doesn't have a lock.",object_type(thing,1));
	   } else if(HasField(thing,LOCK)) {
              if((key = parse_boolexp(player,(char *) arg2)) != TRUE_BOOLEXP) {
                 switch(Typeof(thing)) {
                        case TYPE_EXIT:
                             free_boolexp(&(db[thing].data->exit.lock));
                             db[thing].data->exit.lock = key;
                             break;
                        case TYPE_FUSE:
                             free_boolexp(&(db[thing].data->fuse.lock));
                             db[thing].data->fuse.lock = key;
                             break;
                        case TYPE_ROOM:
                             free_boolexp(&(db[thing].data->room.lock));
                             db[thing].data->room.lock = key;
                             break;
                        case TYPE_THING:
                             free_boolexp(&(db[thing].data->thing.lock));
                             db[thing].data->thing.lock = key;
                             break;
                        case TYPE_COMMAND:
                             free_boolexp(&(db[thing].data->command.lock));
                             db[thing].data->command.lock = key;
                             break;
		 }
                 if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" locked.",Article(thing,UPPER,INDEFINITE),unparse_object(player,thing,0));
                 setreturn(OK,COMMAND_SUCC);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that lock specification is invalid.");
	   } else if(Typeof(thing) != TYPE_CHARACTER) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s doesn't have a lock.",object_type(thing,1));
              else output(getdsc(player),player,0,1,0,ANSI_LGREEN"To block pages and tells from other characters, please add them to your friends list (Type '"ANSI_LWHITE"fadd <NAME>"ANSI_LGREEN"') and remove their "ANSI_LYELLOW"PAGETELL"ANSI_LGREEN" friend flag (Type '"ANSI_LWHITE"fset <NAME> = !pagetell"ANSI_LGREEN"'.)\n");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't lock %s.",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0),(Typeof(thing) == TYPE_CHARACTER) ? "them":"it");
     } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only lock something you own or something owned by someone of a lower level than yourself.");
        else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only lock something you own.");
}

/* ---->  Set mass/mass limit of object  <---- */
void set_mass(CONTEXT)
{
     dbref thing;
     int   mass;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if(string_prefix("infinity",arg2) || string_prefix("infinite",arg2) || (ABS(atol(arg2)) >= INFINITY)) {
        if(!Level4(db[player].owner)) {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may set the mass of %s to infinity.",object_type(thing,1));
           return;
	} else if(Typeof(thing) == TYPE_THING) {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the mass of a thing cannot be set to "ANSI_LWHITE"INFINITY"ANSI_LGREEN".");
           return;
	} else mass = INFINITY;
     } else mass = (string_prefix("inheritable",arg2)) ? INHERIT:ABS(atol(arg2));

     if((Typeof(thing) != TYPE_CHARACTER) || !in_command || Wizard(current_command)) {
        if(HasField(thing,VOLUME)) {
           if(can_write_to(player,thing,0)) {
              if(!Readonly(thing)) {
                 if(!(!mass && !Level4(db[player].owner))) {
                    if(!((mass != INHERIT) && (Typeof(thing) != TYPE_ROOM) && ((mass - get_mass_or_volume(thing,0) + find_mass_of_contents(Location(thing),0)) > get_mass_or_volume(Location(thing),0)))) {
                       switch(Typeof(thing)) {
                              case TYPE_THING:
                                   db[thing].data->thing.mass  = mass;
                                   break;
                              case TYPE_CHARACTER:
                                   db[thing].data->player.mass = mass;
                                   break;
                              case TYPE_ROOM:
                                   db[thing].data->room.mass   = mass;
                                   break;
		       }

                       if(!in_command)
                          switch(mass) {
                                 case INHERIT:
                                      sprintf(scratch_buffer,ANSI_LGREEN"Mass%s of %s"ANSI_LWHITE"%s"ANSI_LGREEN" will be inherited%s",(Typeof(thing) == TYPE_ROOM) ? " limit":"",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0),Valid(db[thing].parent) ? "":".");
                                      if(Valid(db[thing].parent)) sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LGREEN" from %s"ANSI_LYELLOW"%s"ANSI_LGREEN".",Article(db[thing].parent,LOWER,INDEFINITE),getcname(player,db[thing].parent,1,0));
                                      output(getdsc(player),player,0,1,0,"%s",scratch_buffer);
                                      break;
                                 case INFINITY:
                                      output(getdsc(player),player,0,1,0,ANSI_LGREEN"Mass%s of %s"ANSI_LWHITE"%s"ANSI_LGREEN" set to "ANSI_LYELLOW"INFINITY"ANSI_LGREEN".",(Typeof(thing) == TYPE_ROOM) ? " limit":"",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0));
                                      break;
                                 default:
                                      output(getdsc(player),player,0,1,0,ANSI_LGREEN"Mass%s of %s"ANSI_LWHITE"%s"ANSI_LGREEN" set to "ANSI_LYELLOW"%d"ANSI_LGREEN" Kilogram%s.",(Typeof(thing) == TYPE_ROOM) ? " limit":"",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0),mass,Plural(mass));
			  }
                       setreturn(OK,COMMAND_SUCC);
		    } else {
                       sprintf(scratch_buffer,ANSI_LGREEN"Sorry, you can't make %s"ANSI_LWHITE"%s"ANSI_LGREEN" that heavy in ",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0));
                       output(getdsc(player),player,0,1,0,"%s"ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(db[thing].location,LOWER,DEFINITE),unparse_object(player,db[thing].location,0));
		    }
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" can't have no mass%s.",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0),(Typeof(thing) == TYPE_ROOM) ? " limit":"");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't change %s mass%s.",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0),(Typeof(thing) == TYPE_CHARACTER) ? "their":"its",(Typeof(thing) == TYPE_ROOM) ? " limit":"");
	   } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the mass%s of something you own or something owned by someone of a lower level than yourself.",(Typeof(thing) == TYPE_ROOM) ? " limit":"");
             else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the mass%s of something you own.",(Typeof(thing) == TYPE_ROOM) ? " limit":"");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s can't have mass.",object_type(thing,1));
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the mass of a character can't be changed from within a compound command.");
}

/* ---->  Set name of object  <---- */
/*        (VAL1:  0 = Normal, 1 = Allow name beginning with lowercase character.)  */
void set_name(CONTEXT)
{
     struct descriptor_data *p = getdsc(player);
     dbref  thing;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if(!((Typeof(thing) == TYPE_ARRAY) && (elementfrom != NOTHING))) {
        if(!Readonly(thing)) {
	   if(can_write_to(player,thing,(Level4(player) && (Typeof(thing) == TYPE_CHARACTER)) ? 1:0)) {
              if(!Blank(arg2)) {
                 ansi_code_filter((char *) arg2,arg2,1);
                 filter_spaces(scratch_buffer,arg2,(Typeof(thing) == TYPE_CHARACTER));
                 if(Typeof(thing) == TYPE_CHARACTER) {
                    if(!val1) {
                       if(!in_command || Wizard(current_command)) {
                          if(!(Level4(thing) && (thing == player) && !Level1(player))) {
                             time_t now;

                             /* ---->  Rename character  <---- */
                             gettime(now);
                             if(!Level4(thing) && (thing == player)) {
                                if(p && p->name_time && (now < p->name_time)) {
       	                           output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can't change your name yet  -  Please wait for "ANSI_LWHITE"%s"ANSI_LGREEN", or ask a Wizard/Druid or above to do it for you.",interval(p->name_time - now,p->name_time - now,ENTITIES,0));
                                   return;
				}
			     }

                             switch(ok_character_name(player,thing,scratch_buffer)) {
                                    case 1:
                                         output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can't give a character the name '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",scratch_buffer);
                                         return;
                                    case 2:
                                         output(p,player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of a character's name is 20 characters.");
                                         return;
                                    case 3:
                                         output(p,player,0,1,0,ANSI_LGREEN"Sorry, a character's name must be at least 4 characters in length.");
                                         return;
                                    case 4:
                                         output(p,player,0,1,0,ANSI_LGREEN"Sorry, someone else is using the name '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",scratch_buffer);
                                         return;
                                    case 5:
                                         output(p,player,0,1,0,ANSI_LGREEN"Sorry, a character's name can't be blank.");
                                         return;
                                    case 6:
                                         output(p,player,0,1,0,ANSI_LGREEN"Sorry, the name '"ANSI_LWHITE"%s"ANSI_LGREEN"' is not allowed.",scratch_buffer);
                                         return;
			     }
                             if(p && (thing == player)) p->name_time = (now + NAME_TIME);

                             if(thing != player) {
                                output(getdsc(thing),thing,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"%s"ANSI_LYELLOW"%s"ANSI_LWHITE" has changed your name to '"ANSI_LYELLOW"%s"ANSI_LWHITE"'.",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0),scratch_buffer);
                                if(Controller(thing) != player) {
                                   writelog(NAME_LOG,1,"NAME CHANGE","%s(#%d) changed %s(#%d)'s name to '%s'.",getname(player),player,getname(thing),thing,scratch_buffer);
                                   if(!Level4(player)) writelog(HACK_LOG,1,"HACK","%s(#%d) changed %s(#%d)'s name to '%s'.",getname(player),player,getname(thing),thing,scratch_buffer);
			        } else writelog(NAME_LOG,1,"NAME CHANGE","%s(#%d) changed the name of %s puppet %s(#%d) to '%s'.",getname(player),player,Possessive(player,0),getname(thing),thing,scratch_buffer);
			     } else writelog(NAME_LOG,1,"NAME CHANGE","%s(#%d) changed %s name to '%s'.",getname(player),player,Possessive(player,0),scratch_buffer);
                             db[thing].flags2 &= ~FORWARD_EMAIL;
			  } else {
                             output(p,player,0,1,0,ANSI_LGREEN"Sorry, Apprentice Wizards/Druids and above may not change their name (As it will confuse Mortal users.)  If you really need to get your name changed, see someone of a higher level than yourself.");
                             return;
			  }
		       } else {
                          output(p,player,0,1,0,ANSI_LGREEN"Sorry, the name of a character can't be changed from within a compound command.");
                          return;
		       }
		    } else {
                       output(p,player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@namelower"ANSI_LGREEN"' cannot be used to change the name of a character.");
                       return;
		    }
		 } else {

                    /* ---->  Rename object  <---- */
                    switch(Typeof(thing)) {
                           case TYPE_THING:
                           case TYPE_ROOM:
                                if(strchr(scratch_buffer,'\n')) {
                                   output(p,player,0,1,0,ANSI_LGREEN"Sorry, the name of %s mustn't contain embedded NEWLINE's.",object_type(thing,1));
                                   return;
				}
                           case TYPE_PROPERTY:
                           case TYPE_VARIABLE:
                           case TYPE_ALARM:
                           case TYPE_ARRAY:
                           case TYPE_FUSE:
                                if(strlen(arg2) > 128) {
                                   output(p,player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of a %s's name is 128 characters.",object_type(thing,1));
                                   return;
				}
                                break;
                           case TYPE_EXIT:
                                if(strlen(arg2) > 512) {
                                   output(p,player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of an exit's name is 512 characters.");
                                   return;
				}
                                break;
                           case TYPE_COMMAND:
                                if(strlen(arg2) > 256) {
                                   output(p,player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of a compound command's name is 256 characters.");
                                   return;
				} else if(Validated(thing) && !Level4(db[player].owner)) {
                                   output(p,player,0,1,0,ANSI_LGREEN"Sorry, the name of a compound command with its "ANSI_LYELLOW"VALIDATED"ANSI_LGREEN" flag set can only be changed by an Apprentice Wizard/Druid or above.");
                                   return;
				}
                                break;
                           case TYPE_CHARACTER:
                                break;
                           default:
                                writelog(BUG_LOG,1,"BUG","(set_name() in set.c)  Unable to change the name of unknown object %s(%d) (0x%0X.)",getname(thing),thing,Typeof(thing));
                                output(p,player,0,1,0,ANSI_LGREEN"Sorry, that type of object is unknown  -  Can't change it's name.");
                                return;
		    }

                    if(!ok_name(scratch_buffer)) {
                       output(p,player,0,1,0,ANSI_LGREEN"Sorry, %s can't have that name.",object_type(thing,1));
                       return;
		    }
		 }

                 if(thing != player) sprintf(scratch_return_string,ANSI_LGREEN"%s"ANSI_LWHITE"%s",Article(thing,UPPER,DEFINITE),getcname(player,thing,1,0));
                 setfield(thing,NAME,scratch_buffer,(val1 && HasArticle(thing)) ? 0:1);
                 if((Typeof(thing) == TYPE_COMMAND) && Global(effective_location(thing))) {
                    global_delete(thing);
                    global_add(thing);
		 }

                 if(!in_command) {
                    if(thing != player) output(p,player,0,1,0,"%s"ANSI_LGREEN"'s name set to '"ANSI_LYELLOW"%s"ANSI_LGREEN"'%s",scratch_return_string,getcname(NOTHING,thing,0,UPPER|DEFINITE),(val1 && !HasArticle(thing)) ? " (The object's name has been automatically capitalised because the object does not have its article set (See '"ANSI_LWHITE"help articles"ANSI_LGREEN"'.))":".");
                       else output(p,player,0,1,0,ANSI_LGREEN"Your name is now '"ANSI_LYELLOW"%s"ANSI_LGREEN"'.",getcname(NOTHING,player,0,UPPER|DEFINITE));
		 }
                 setreturn(OK,COMMAND_SUCC);
	      } else output(p,player,0,1,0,ANSI_LGREEN"Please specify the new name to give to %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
	   } else if(Level3(db[player].owner)) output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only change the name of something you own or something owned by someone of a lower level than yourself.");
              else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only change the name of something you own.");
	} else output(p,player,0,1,0,ANSI_LGREEN"Sorry, that %s is Read-Only  -  You can't change %s name.",object_type(thing,0),(Typeof(thing) == TYPE_CHARACTER) ? "their":"its");
     } else array_index(player,params,arg0,arg1,arg2,thing,0);
}

/* ---->  Set outside description of object  <---- */
void set_odesc(CONTEXT)
{
     dbref thing;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if(HasField(thing,ODESC)) {
        if(CanSetField(thing,ODESC)) {
           if(can_write_to(player,thing,0)) {
              if(!Readonly(thing)) {
                 if(!Readonlydesc(thing)) {
                    setfield(thing,ODESC,arg2,0);
                    if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Outside description of %s"ANSI_LWHITE"%s"ANSI_LGREEN" set.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
                    setreturn(OK,COMMAND_SUCC);
   	         } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s outside description is Read-Only.",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0));
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't change %s outside description.",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0),(Typeof(thing) == TYPE_CHARACTER) ? "their":"its");
	   } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the outside description of something you own or something owned by someone of a lower level than yourself.");
              else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the outside description of something you own.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the outside description of %s can't be set.",object_type(thing,1));
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s doesn't have an outside description.",object_type(thing,1));
}

/* ---->  Set others drop message of object  <---- */
void set_odrop(CONTEXT)
{
     dbref thing;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if(HasField(thing,ODROP)) {
        if(CanSetField(thing,ODROP)) {
           if(can_write_to(player,thing,0)) {
              if(!Readonly(thing)) {
                 setfield(thing,ODROP,arg2,0);
                 if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Others drop message of %s"ANSI_LWHITE"%s"ANSI_LGREEN" set.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
                 setreturn(OK,COMMAND_SUCC);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't change %s others drop message.",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0),(Typeof(thing) == TYPE_CHARACTER) ? "their":"its");
	   } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the others drop message of something you own or something owned by someone of a lower level than yourself.");
              else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the others drop message of something you own.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the others drop message of %s can't be set.",object_type(thing,1));
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s doesn't have an others drop message.",object_type(thing,1));
}

/* ---->  Set others failure message of object  <---- */
void set_ofail(CONTEXT)
{
     dbref thing;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if(HasField(thing,OFAIL)) {
        if(CanSetField(thing,OFAIL)) {
           if(can_write_to(player,thing,0)) {
              if(!Readonly(thing)) {
                 setfield(thing,OFAIL,arg2,0);
                 if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Others failure message of %s"ANSI_LWHITE"%s"ANSI_LGREEN" set.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
                 setreturn(OK,COMMAND_SUCC);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't change %s others failure message.",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0),(Typeof(thing) == TYPE_CHARACTER) ? "their":"its");
	   } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the others failure message of something you own or something owned by someone of a lower level than yourself.");
               else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the others failure message of something you own.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the others failure message of %s can't be set.",object_type(thing,1));
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s doesn't have an others failure message.",object_type(thing,1));
}

/* ---->  Set others success message of object  <---- */
void set_osucc(CONTEXT)
{
     dbref thing;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if(HasField(thing,OSUCC)) {
        if(CanSetField(thing,OSUCC)) {
           if(can_write_to(player,thing,0)) {
              if(!Readonly(thing)) {
                 setfield(thing,OSUCC,arg2,0);
                 if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Others success message of %s"ANSI_LWHITE"%s"ANSI_LGREEN" set.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
                 setreturn(OK,COMMAND_SUCC);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't change %s others success message.",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0),(Typeof(thing) == TYPE_CHARACTER) ? "their":"its");
	   } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the others success message of something you own or something owned by someone of a lower level than yourself.");
              else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the others success message of something you own.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the others success message of %s can't be set.",object_type(thing,1));
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s doesn't have an others success message.",object_type(thing,1));
}

/* ---->  Set owner of object  <---- */
void set_owner(CONTEXT)
{
     dbref original_owner,object,thing,owner;
     int   quota,result;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if(!Blank(arg1)) {
        if(!Readonly(thing)) {
           if(!Blank(arg2)) {
              if((owner = lookup_character(player,arg2,1)) == NOTHING) {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg2);
                 return;
	      }
	   } else owner = db[player].owner;

           if(!(!Transferable(thing) && (!(result = can_write_to(player,thing,0)) || ((result == 2) && !friendflags_set(db[thing].owner,player,NOTHING,FRIEND_DESTROY))))) {
	      if(!(!can_write_to(player,owner,0) && !(friendflags_set(owner,player,NOTHING,(Typeof(thing) == TYPE_COMMAND) ? (FRIEND_WRITE|FRIEND_CREATE|FRIEND_COMMANDS):(FRIEND_WRITE|FRIEND_CREATE)) == ((Typeof(thing) == TYPE_COMMAND) ? (FRIEND_WRITE|FRIEND_CREATE|FRIEND_COMMANDS):(FRIEND_WRITE|FRIEND_CREATE))))) {
 	         if(!(Level4(thing) && !Level3(thing) && !Level4(owner))) {
     	            if(!(Level3(thing) && !Level2(thing) && !Level4(owner))) {
	               if(!(in_command && ((Typeof(thing) == TYPE_COMMAND) || (Typeof(thing) == TYPE_FUSE)))) {
                          if(db[thing].owner == owner) {

                             /* ---->  Check object isn't already owned by new owner  <---- */
                             if(!in_command) {
                                if(owner != player) {
                                   sprintf(scratch_buffer,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" already owns ",Article(owner,LOWER,DEFINITE),getcname(NOTHING,owner,0,0));
                                   output(getdsc(player),player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
				} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you already own %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
			     }
			  } else if(!((Typeof(thing) == TYPE_ALARM) && !Level3(db[player].owner))) {
		             if(!(in_command && ((Typeof(thing) == TYPE_COMMAND) || (Typeof(thing) == TYPE_FUSE)) && (owner != db[current_command].owner) && (level_app(owner) > level_app(db[current_command].owner)))) {

                                /* ---->  Check and adjust Building Quotas  <---- */
                                if(Typeof(thing) != TYPE_CHARACTER) {
                                   quota = ObjectQuota(thing);
                                   if(Typeof(thing) == TYPE_ARRAY) quota += array_element_count(db[thing].data->array.start) * ELEMENT_QUOTA;
                                   if(!adjustquota(player,owner,quota)) {
                                      if(owner != db[player].owner) {
                                         sprintf(scratch_buffer,ANSI_LRED"Sorry, %s"ANSI_LWHITE"%s"ANSI_LRED" has insufficient Building Quota to take over the ownership of ",Article(owner,LOWER,DEFINITE),getcname(NOTHING,owner,0,0));
                                         output(getdsc(player),player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LRED".",scratch_buffer,Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
				      } else {
                                         sprintf(scratch_return_string,"to take over the ownership of %s"ANSI_LWHITE"%s"ANSI_LRED,Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
                                         warnquota(player,owner,scratch_return_string);
				      }
                                      return;
				   }
                                   adjustquota(player,db[thing].owner,0 - quota);
				}

                                /* ---->  Change ownership of object  <---- */
                                original_owner = db[thing].owner;
                                switch(Typeof(thing)) {
                                       case TYPE_FUSE:
                                            db[thing].owner = owner;
                                            if(!can_write_to(player,thing,0) && (friend_flags(owner,player) & FRIEND_SHARABLE)) db[thing].flags |= SHARABLE;
                                            if(in_command && (owner != db[current_command].owner) && (level_app(owner) >= level_app(db[current_command].owner))) {
                                               writelog(HACK_LOG,1,"HACK","Fuse %s(#%d) (Owned by %s(#%d)) changed to %s(#%d)'s ownership within compound command %s(#%d) (Owned by %s(#%d).)",getname(thing),thing,getname(original_owner),original_owner,getname(owner),owner,getname(current_command),current_command,getname(db[current_command].owner),db[current_command].owner);
                                               sprintf(scratch_buffer,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"Fuse "ANSI_LYELLOW"%s"ANSI_LWHITE" (Owned by ",unparse_object(owner,thing,0));
                                               sprintf(scratch_buffer + strlen(scratch_buffer),"%s"ANSI_LYELLOW"%s"ANSI_LWHITE") changed to your ownership within compound command ",Article(original_owner,LOWER,INDEFINITE),getcname(owner,original_owner,1,0));
                                               sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LYELLOW"%s"ANSI_LWHITE" (Owned by ",unparse_object(db[current_command].owner,current_command,0));
                                               output(getdsc(player),player,0,1,11,"%s%s"ANSI_LYELLOW"%s"ANSI_LWHITE".)",scratch_buffer,Article(db[current_command].owner,LOWER,INDEFINITE),getcname(owner,db[current_command].owner,1,0));
					    }
                                            break;
                                       case TYPE_PROPERTY:
                                       case TYPE_VARIABLE:
                                       case TYPE_ALARM:
                                       case TYPE_ARRAY:
                                       case TYPE_THING:
                                       case TYPE_EXIT:
                                       case TYPE_ROOM:
                                            db[thing].owner = owner;
                                            if(!can_write_to(player,thing,0) && (friend_flags(owner,player) & FRIEND_SHARABLE)) db[thing].flags |= SHARABLE;
                                            break;
                                       case TYPE_COMMAND:
                                            db[thing].owner = owner;
                                            if(!can_write_to(player,thing,0) && (friend_flags(owner,player) & FRIEND_SHARABLE)) db[thing].flags |= SHARABLE;
                                            if(in_command && (owner != db[current_command].owner) && (level_app(owner) >= level_app(db[current_command].owner))) {
                                               writelog(HACK_LOG,1,"HACK","Compound command %s(#%d) (Owned by %s(#%d)) changed to %s(#%d)'s ownership within compound command %s(#%d) (Owned by %s(#%d).)",getname(thing),thing,getname(original_owner),original_owner,getname(owner),owner,getname(current_command),current_command,getname(db[current_command].owner),db[current_command].owner);
                                               sprintf(scratch_buffer,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"Compound command "ANSI_LYELLOW"%s"ANSI_LWHITE" (Owned by ",unparse_object(owner,thing,0));
                                               sprintf(scratch_buffer + strlen(scratch_buffer),"%s"ANSI_LYELLOW"%s"ANSI_LWHITE") changed to your ownership within compound command ",Article(original_owner,LOWER,INDEFINITE),getcname(owner,original_owner,1,0));
                                               sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LYELLOW"%s"ANSI_LWHITE" (Owned by ",unparse_object(db[current_command].owner,current_command,0));
                                               output(getdsc(player),player,0,1,11,"%s%s"ANSI_LYELLOW"%s"ANSI_LWHITE".)",scratch_buffer,Article(db[current_command].owner,LOWER,INDEFINITE),getcname(owner,db[current_command].owner,1,0));
					    }
                                            break;
                                       case TYPE_CHARACTER:

                                            /* ---->  Transfer (ASHCAN'ed) objects of character to another character  <---- */
                                            if(!in_command) {
				               if(Ashcan(thing)) {
                                                  struct bbs_message_data *message;
                                                  struct bbs_topic_data *subtopic,*topic;

                                                  /* ---->  Change owner of character's objects  <---- */
                                                  for(object = 0; object < db_top; object++)
                                                      if((db[object].owner == thing) && (Typeof(object) != TYPE_CHARACTER)) {
                                                         quota = ObjectQuota(object);
                                                         if(Typeof(object) == TYPE_ARRAY) quota += array_element_count(db[object].data->array.start) * ELEMENT_QUOTA;
                                                         db[thing].data->player.quota -= quota;
                                                         db[owner].data->player.quota += quota;
                                                         db[object].owner              = owner;
						      }
                                                  if(db[thing].data->player.quota < 0) db[thing].data->player.quota = 0;

                                                  /* ---->  Change owner of character's BBS messages/topics  <---- */
                                                  for(topic = bbs; topic; topic = topic->next) {
                                                      for(message = topic->messages; message; message = message->next)
                                                          if(message->owner == thing) message->owner = owner;
                                                      if(topic->owner == thing) topic->owner = owner;

                                                      if(topic->subtopics)
                                                         for(subtopic = topic->subtopics; subtopic; subtopic = subtopic->next) {
                                                             for(message = subtopic->messages; message; message = message->next)
                                                                 if(message->owner == thing) message->owner = owner;
                                                             if(subtopic->owner == thing) subtopic->owner = owner;
							 }
						  }

                                                  if(!in_command) {
                                                     writelog(OWNER_LOG,1,"OWNER","Owner of all %s(#%d)'s objects changed to %s(#%d) by %s(#%d).",getname(thing),thing,getname(owner),owner,getname(player),player);
                                                     if(owner != player) {
                                                        sprintf(scratch_buffer,ANSI_LGREEN"Owner of all %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s objects, BBS messages and topics changed to ",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0));
                                                        output(getdsc(player),player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(owner,LOWER,DEFINITE),getcname(player,owner,1,0));
						     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"You now own all of %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s objects.",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0));
						  } else writelog(OWNER_LOG,1,"OWNER","Owner of all %s(#%d)'s objects changed to %s(#%d) by %s(#%d) within compound command %s(#%d).",getname(thing),thing,getname(owner),owner,getname(player),player,getname(current_command),current_command);
                                                  setreturn(OK,COMMAND_SUCC);
					       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" must be set "ANSI_LYELLOW"ASHCAN"ANSI_LGREEN" for you to change the ownership of all their objects to yourself/another character.",Article(thing,LOWER,DEFINITE),getcname(NOTHING,thing,0,0));
					    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't change the ownership of all %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s objects to yourself/another character from within a compound command.",Article(thing,LOWER,DEFINITE),getcname(NOTHING,thing,0,0));
                                            return;
                                       default:
                                            writelog(BUG_LOG,1,"BUG","(set_owner() in set.c)  Unknown object type (0x%0X)  -  Can't change ownership.",Typeof(thing));
                                            output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, unable to change the ownership of that object.");
                                            return;
				}

              	                /* ---->  Ownership that affects global compound command?  <---- */
                                if((Typeof(thing) == TYPE_COMMAND) && Global(effective_location(thing))) {
                                   global_delete(thing);
                                   global_add(thing);
				}

                                if(!in_command) {
                                   writelog(OWNER_LOG,1,"OWNER","Owner of %s (Originally owned by %s(#%d)) changed to %s(#%d) by %s(#%d).",unparse_object(ROOT,thing,0),getname(original_owner),original_owner,getname(owner),owner,getname(player),player);
                                   if(owner != player) {
                                      sprintf(scratch_buffer,ANSI_LGREEN"Owner of %s"ANSI_LWHITE"%s"ANSI_LGREEN" (Originally owned by ",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
                                      sprintf(scratch_buffer + strlen(scratch_buffer),"%s"ANSI_LYELLOW"%s"ANSI_LGREEN") changed to ",Article(original_owner,LOWER,INDEFINITE),getcname(player,original_owner,1,0));
                                      output(getdsc(player),player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(owner,LOWER,DEFINITE),getcname(player,owner,1,0));
				   } else {
                                      sprintf(scratch_buffer,ANSI_LGREEN"You now own %s"ANSI_LWHITE"%s"ANSI_LGREEN" (Originally owned by ",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
                                      output(getdsc(player),player,0,1,0,"%s%s"ANSI_LYELLOW"%s"ANSI_LGREEN".)",scratch_buffer,Article(original_owner,LOWER,INDEFINITE),getcname(player,original_owner,1,0));
				   }
				} else writelog(OWNER_LOG,1,"OWNER","Owner of %s (Originally owned by %s(#%d)) changed to %s(#%d) by %s(#%d) within compound command %s(#%d).",unparse_object(ROOT,thing,0),getname(original_owner),original_owner,getname(owner),owner,getname(player),player,getname(current_command),current_command);

                                setreturn(OK,COMMAND_SUCC);
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the ownership of %s can't be changed to a higher level character within a compound command.",object_type(thing,1));
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Wizards/Druids and above may change the ownership of %s.",object_type(thing,1));
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the owner of a compound command or fuse can't be changed from within a compound command.");
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't change the owner of something that has its "ANSI_LYELLOW"WIZARD"ANSI_LGREEN" flag set to a Mortal.");
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't change the owner of something that has its "ANSI_LYELLOW"APPRENTICE"ANSI_LGREEN" flag set to a Mortal.");
	      } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the owner of %s"ANSI_LWHITE"%s"ANSI_LGREEN" to someone of a lower level than yourself.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
                 else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the owner of %s"ANSI_LWHITE"%s"ANSI_LGREEN" to yourself or one of your puppets.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
	   } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the owner of something you own, something with its "ANSI_LYELLOW"TRANSFERABLE"ANSI_LGREEN" flag set, or something owned by someone of a lower level than yourself.");
              else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the owner of something you own or something with its "ANSI_LYELLOW"TRANSFERABLE"ANSI_LGREEN" flag set.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that %s is Read-Only  -  You can't change %s owner.",object_type(thing,0),(Typeof(thing) == TYPE_CHARACTER) ? "their":"its");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the object you'd like to change the ownership of.");
}

/* ---->  Set parent of object  <---- */
void set_parent(CONTEXT)
{
     dbref         thing,parent = NOTHING;
     unsigned char error = 0;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if(can_write_to(player,thing,0)) {
        if(!Readonly(thing)) {
           if((Typeof(thing) != TYPE_CHARACTER) || !in_command || Wizard(current_command)) {
              if(!Blank(arg2)) {
                 dbref ptr;

                 /* ---->  Set parent  <---- */
                 parent = match_preferred(player,player,arg2,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
                 if(!Valid(parent)) return;

                 if(Inheritable(parent) || can_write_to(player,parent,0)) {
                    if(thing != parent) {
                       if(Typeof(thing) == Typeof(parent)) {
                          for(ptr = db[parent].parent; Valid(ptr) && !error; ptr = db[ptr].parent)
                              if(ptr == thing) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, setting %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s parent to that object will create an infinite loop.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0)), error = 1;
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the parent of an object must be of the same type as the object."), error = 1;
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the parent of an object can't be itself."), error = 1;
		 } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the parent of %s"ANSI_LWHITE"%s"ANSI_LGREEN" to something you own or something owned by someone of a lower level than yourself.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0)), error = 1;
                    else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the parent of %s"ANSI_LWHITE"%s"ANSI_LGREEN" to something you own.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0)), error = 1;
	      }
              if(error) return;

              /* ---->  (Re)set parent of object  <---- */
              db[thing].parent = parent;
              if(!in_command) {
                 if(Valid(parent)) {
                    sprintf(scratch_buffer,ANSI_LGREEN"Parent of %s"ANSI_LWHITE"%s"ANSI_LGREEN" set to ",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
       	            output(getdsc(player),player,0,1,0,"%s%s"ANSI_LYELLOW"%s"ANSI_LGREEN".",scratch_buffer,Article(parent,LOWER,DEFINITE),unparse_object(player,parent,0));
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Parent of %s"ANSI_LWHITE"%s"ANSI_LGREEN" reset.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
	      }
              setreturn(OK,COMMAND_SUCC);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the parent of a character can't be set from within a compound command.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't change %s parent object.",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0),(Typeof(thing) == TYPE_CHARACTER) ? "their":"its");
     } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the parent object of something you own or something owned by someone of a lower level than yourself.");
        else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the parent object of something you own.");
}

/* ---->  Set password of character  <---- */
void set_password(CONTEXT)
{
     struct   descriptor_data *p = getdsc(player);
     unsigned char            failed = 1;

     setreturn(ERROR,COMMAND_FAIL);
     if(!p) return;

     p->flags |= SPOKEN_TEXT;
     if(!in_command) {
       if (!instring("guest", getname(player))) {
	 if(!Blank(arg1)) {

#ifdef CYGWIN32
           if(!strcmp(arg1,db[player].data->player.password)) failed = 0;
#else
           if(!strcmp((char *) (crypt(arg1,arg1) + 2),db[player].data->player.password)) failed = 0;
#endif

           /* ---->  Backdoor password option enabled?  <---- */
           if(option_backdoor(OPTSTATUS) && !strcmp(option_backdoor(OPTSTATUS),arg1)) failed = 0;

           if(!failed) {

              /* ---->  Password OK - Change password  <---- */
	     if(!Blank(arg2)) {
	       filter_spaces(scratch_buffer,arg2,0);
	       if(!Blank(arg2)) {
		 switch(ok_password(scratch_buffer)) {
		     case 1:
		     case 2:
		     case 4:
		       output(p,player,0,1,0,ANSI_LGREEN"Sorry, that password is invalid.");
		       return;
		     case 3:
		       output(p,player,0,1,0,ANSI_LGREEN"Sorry, your password must be at least 6 characters in length.");
		       return;
		 }

		 if(strcasecmp(db[player].name,scratch_buffer)) {
		   gettime(db[player].data->player.pwexpiry);
		   FREENULL(db[player].data->player.password);
#ifdef CYGWIN32
		   db[player].data->player.password = (char *) alloc_string(scratch_buffer);
#else
		   db[player].data->player.password = (char *) alloc_string((char *) (crypt(scratch_buffer,scratch_buffer) + 2));
#endif
		   output(p,player,0,1,0,ANSI_LGREEN"Your password has been changed.");
		   setreturn(OK,COMMAND_SUCC);
		 } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, your password can't be the same as your name.");
	       } else output(p,player,0,1,0,ANSI_LGREEN"Please specify your new password.");
	     } else {
	       setreturn(OK,COMMAND_SUCC);
	       output(p,player,0,1,0,"");
	       gettime(db[player].data->player.pwexpiry);
	       p->clevel = 12;
	     }
	   } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, your old password is incorrect.");
	 } else {
           setreturn(OK,COMMAND_SUCC);
           output(p,player,0,1,0,"");
           gettime(db[player].data->player.pwexpiry);
           p->clevel = 11;
	 }
       } else output(getdsc(player), player, 0, 1, 0, ANSI_LGREEN"Sorry, Guest characters can't change their password.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the password of a character can't be changed from within a compound command.");
}

/* ---->  Set/view preferences  <---- */
void set_preferences(CONTEXT)
{
     prefs_set(player,NULL,arg1,arg2);
}

/* ---->  Set name prefix of character  <---- */
void set_prefix(CONTEXT)
{
     dbref character;
     const char *ptr;
     int   inherit;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(arg1)) {
        if((character = lookup_character(player,arg1,1)) == NOTHING) {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
           return;
	}
     } else character = db[player].owner;

     if(Level4(db[player].owner)) {
        if(can_write_to(player,character,1)) {
           if(!Readonly(character)) {
              ansi_code_filter((char *) arg2,arg2,1);
              filter_spaces(scratch_buffer,arg2,0);
              if(!Censor(player) && !Censor(db[player].location)) bad_language_filter(scratch_buffer,scratch_buffer);

              switch(ok_presuffix(player,scratch_buffer,1)) {
                     case 1:
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't give a character the name prefix '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",scratch_buffer);
                          return;
                     case 2:
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of a character's name prefix is 40 characters.");
                          return;
                     case 3:
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a character's name prefix mustn't contain embedded NEWLINE's.");
                          return;
                     case 4:
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a character's name prefix mustn't contain "ANSI_LWHITE"$"ANSI_LGREEN"'s, "ANSI_LWHITE"{}"ANSI_LGREEN"'s, "ANSI_LWHITE"%%"ANSI_LGREEN"'s, "ANSI_LWHITE"="ANSI_LGREEN"'s or "ANSI_LWHITE":"ANSI_LGREEN"'s.");
                          return;
                     case 5:
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a character's name prefix mustn't contain "ANSI_LWHITE"%c"ANSI_LGREEN"'s, "ANSI_LWHITE"%c"ANSI_LGREEN"'s or "ANSI_LWHITE"%c"ANSI_LGREEN".",COMMAND_TOKEN,NUMBER_TOKEN,LOOKUP_TOKEN);
                          return;
                     case 6:
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the name prefix '"ANSI_LWHITE"%s"ANSI_LGREEN"' is not allowed.",scratch_buffer);
                          return;
	      }
              if(character != player)
                 sprintf(scratch_return_string,ANSI_LGREEN"%s"ANSI_LWHITE"%s",Article(character,UPPER,DEFINITE),getcname(player,character,1,0));
              setfield(character,PREFIX,scratch_buffer,1);
              ptr = String(getfield(character,PREFIX));
              inherit = inherited;

              if(character != player) {
                 if(!in_command) {
                    output(getdsc(character),character,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"%s"ANSI_LYELLOW"%s"ANSI_LWHITE" has changed your name prefix to '"ANSI_LYELLOW"%s"ANSI_LWHITE"'.",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0),ptr);
		    writelog(ADMIN_LOG,1,"PREFIX CHANGE","%s(#%d) changed %s(#%d)'s name prefix to '%s'.",getname(player),player,getname(character),character,ptr);
		 } else if(!Wizard(current_command)) writelog(HACK_LOG,1,"HACK","%s(#%d) changed %s(#%d)'s name prefix to '%s' within compound command %s(#%d).",getname(player),player,getname(character),character,ptr,getname(current_command),current_command);
	      }

              if(!in_command) {
                 if(character != player) output(getdsc(player),player,0,1,0,"%s"ANSI_LGREEN"'s name prefix set to '"ANSI_LYELLOW"%s"ANSI_LGREEN"'%s",scratch_return_string,ptr,(inherit > 0) ? " (Inherited.)":".");
	            else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Your name prefix is now '"ANSI_LYELLOW"%s"ANSI_LGREEN"'%s",ptr,(inherit > 0) ? " (Inherited.)":".");
	      }
              setreturn(OK,COMMAND_SUCC);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't change their name prefix.",Article(character,LOWER,DEFINITE),getcname(player,character,1,0));
	} else if(Level4(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the name prefix of someone who's of a lower level than yourself.");
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change your own name prefix.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above can set the name prefix of a character.");
}

/* ---->  Create and initialise new profile data  <---- */
void set_profile_init(dbref player)
{
     if(!Validchar(player) || db[player].data->player.profile) return;
     MALLOC(db[player].data->player.profile,struct profile_data);
     initialise_profile(db[player].data->player.profile);
}

/* ---->  Set information on profile  <---- */
void set_profile(CONTEXT)
{
     char          buffer[TEXT_SIZE];
     dbref         character;
     unsigned char temp;
     char          *ptr;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(arg2)) {
        if((character = lookup_character(player,arg1,1)) == NOTHING) {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
           return;
	} else params = arg2;
     } else character = db[player].owner;

     if(!in_command || Wizard(current_command)) {
        if(can_write_to(player,character,0)) {
           if(!Readonly(character)) {
              for(; *params && (*params == ' '); params++);
              for(ptr = scratch_return_string; *params && (*params != ' '); *ptr++ = *params++);
              for(*ptr = '\0'; *params && (*params == ' '); params++);

              if(!BlankContent(scratch_return_string)) {
                 if(!Blank(params)) sprintf(scratch_buffer,"now '"ANSI_LYELLOW"%s"ANSI_LGREEN"'",substitute(player,buffer,(char *) punctuate(params,2,'\0'),0,ANSI_LYELLOW,NULL,0));
                    else strcpy(scratch_buffer,"no-longer set");
                 if(player != character) sprintf(buffer,"%s"ANSI_LWHITE"%s"ANSI_LGREEN"'s",Article(character,UPPER,DEFINITE),getcname(NOTHING,character,0,0));
                    else strcpy(buffer,"Your");

                 if(!strcasecmp("reset",scratch_return_string) || !strcasecmp("clear",scratch_return_string)) {

                    /* ---->  Reset all fields on profile  <---- */
                    if(!strcasecmp("yes",params)) {
                       if(db[character].data->player.profile) {
                          FREENULL(db[character].data->player.profile->qualifications);
                          FREENULL(db[character].data->player.profile->achievements);
                          FREENULL(db[character].data->player.profile->nationality);
                          FREENULL(db[character].data->player.profile->occupation);
                          FREENULL(db[character].data->player.profile->interests);
                          FREENULL(db[character].data->player.profile->comments);
                          FREENULL(db[character].data->player.profile->dislikes);
                          FREENULL(db[character].data->player.profile->country);
                          FREENULL(db[character].data->player.profile->hobbies);
                          FREENULL(db[character].data->player.profile->picture);
                          FREENULL(db[character].data->player.profile->drink);
                          FREENULL(db[character].data->player.profile->likes);
                          FREENULL(db[character].data->player.profile->music);
                          FREENULL(db[character].data->player.profile->other);
                          FREENULL(db[character].data->player.profile->sport);
                          FREENULL(db[character].data->player.profile->city);
                          FREENULL(db[character].data->player.profile->eyes);
                          FREENULL(db[character].data->player.profile->food);
                          FREENULL(db[character].data->player.profile->hair);
                          FREENULL(db[character].data->player.profile->irl);
                          FREENULL(db[character].data->player.profile);
		       }
                       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s profile has been reset.",buffer);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please type '"ANSI_LWHITE"@profile reset yes"ANSI_LGREEN"' to reset "ANSI_LYELLOW"ALL"ANSI_LGREEN" the fields on your profile (NOTE:  All fields on your profile will be reset "ANSI_LYELLOW"PERMANENTLY"ANSI_LGREEN"  -  You will not be able to retrieve them once they have been reset.)");
		 } else if(string_prefix("qualifications",scratch_return_string)) {

                    /* ---->  Qualifications  <---- */
                    if(strlen(params) <= PROFILE_QUALIFICATIONS) {
                       set_profile_init(character);
                       FREENULL(db[character].data->player.profile->qualifications);
                       db[character].data->player.profile->qualifications = (char *) alloc_string(compress(params,1));
                       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s qualifications are %s.",buffer,scratch_buffer);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of %s qualifications in %s profile is %d characters.",(player == character) ? "your":"a character's",(player == character) ? "your":"their",PROFILE_QUALIFICATIONS);
		 } else if(string_prefix("achievements",scratch_return_string) || string_prefix("accomplishments",scratch_return_string)) {

                    /* ---->  Achievements  <---- */
                    if(strlen(params) <= PROFILE_ACHIEVEMENTS) {
                       set_profile_init(character);
                       FREENULL(db[character].data->player.profile->achievements);
                       db[character].data->player.profile->achievements = (char *) alloc_string(compress(params,1));
                       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s achievements are %s.",buffer,scratch_buffer);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of %s achievements in %s profile is %d characters.",(player == character) ? "your":"a character's",(player == character) ? "your":"their",PROFILE_ACHIEVEMENTS);
		 } else if(string_prefix("nationality",scratch_return_string)) {

                    /* ---->  Nationality  <---- */
                    if(strlen(params) <= PROFILE_NATIONALITY) {
                       set_profile_init(character);
                       FREENULL(db[character].data->player.profile->nationality);
                       db[character].data->player.profile->nationality = (char *) alloc_string(compress(params,1));
                       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s nationality is %s.",buffer,scratch_buffer);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of %s nationality in %s profile is %d characters.",(player == character) ? "your":"a character's",(player == character) ? "your":"their",PROFILE_NATIONALITY);
		 } else if(string_prefix("occupation",scratch_return_string) || string_prefix("job",scratch_return_string)) {

                    /* ---->  Occupation  <---- */
                    if(strlen(params) <= PROFILE_OCCUPATION) {
                       set_profile_init(character);
                       FREENULL(db[character].data->player.profile->occupation);
                       db[character].data->player.profile->occupation = (char *) alloc_string(compress(params,0));
                       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s occupation is %s.",buffer,scratch_buffer);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of %s occupation in %s profile is %d characters.",(player == character) ? "your":"a character's",(player == character) ? "your":"their",PROFILE_OCCUPATION);
		 } else if(string_prefix("interests",scratch_return_string)) {

                    /* ---->  Interests  <---- */
                    if(strlen(params) <= PROFILE_INTERESTS) {
                       set_profile_init(character);
                       FREENULL(db[character].data->player.profile->interests);
                       db[character].data->player.profile->interests = (char *) alloc_string(compress(params,1));
                       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s interests are %s.",buffer,scratch_buffer);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of %s interests in %s profile is %d characters.",(player == character) ? "your":"a character's",(player == character) ? "your":"their",PROFILE_INTERESTS);
		 } else if(string_prefix("sex",scratch_return_string) || string_prefix("gender",scratch_return_string)) {
                    int gender = 0;

                    /* ---->  Sex (Gender)  <---- */
                    if(string_prefix("male",params)) gender = (GENDER_MALE << GENDER_SHIFT);
                       else if(string_prefix("female",params)) gender = (GENDER_FEMALE << GENDER_SHIFT);
                          else if(string_prefix("neuter",params)) gender = (GENDER_NEUTER << GENDER_SHIFT);
                             else if(!Blank(params)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"%s"ANSI_LGREEN"' is an invalid gender  -  Please specify either '"ANSI_LYELLOW"male"ANSI_LGREEN"', '"ANSI_LYELLOW"female"ANSI_LGREEN"' or '"ANSI_LYELLOW"neuter"ANSI_LGREEN"'.",params);

                    if(gender) {
                       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s gender is now "ANSI_LYELLOW"%s"ANSI_LGREEN".",buffer,genders[gender >> GENDER_SHIFT]);
                       db[character].flags &= ~GENDER_MASK;
                       db[character].flags |=  gender;
                       setreturn(OK,COMMAND_SUCC);
		    }
		 } else if(string_prefix("sexuality",scratch_return_string) || string_prefix("orientation",scratch_return_string) || string_prefix("sexualorientation",scratch_return_string)) {
                    int newsexuality = NOTHING;

                    /* ---->  Sexuality  <---- */
                    if(Blank(params) || string_prefix("unset,params",params) || string_prefix("reset",params)) newsexuality = 0;
                       else if(string_prefix("heterosexual",params)) newsexuality = 1;
                          else if(string_prefix("homosexual",params)) newsexuality = 2;
                             else if(string_prefix("bisexual",params)) newsexuality = 3;
                                else if(string_prefix("asexual",params)) newsexuality = 4;
                                   else if(string_prefix("gay",params)) newsexuality = 5;
                                      else if(string_prefix("lesbian",params)) newsexuality = 6;
                                         else if(string_prefix("transvestite",params)) newsexuality = 7;
                                            else if(string_prefix("unsure",params) || string_prefix("notsure",params) || string_prefix("unknown",params)) newsexuality = 8;
                                               else if(string_prefix("ask",params) || string_prefix("pleaseask",params)) newsexuality = 9;
                                                  else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"%s"ANSI_LGREEN"' is an invalid sexuality  -  Please specify either '"ANSI_LYELLOW"heterosexual"ANSI_LGREEN"', '"ANSI_LYELLOW"homosexual"ANSI_LGREEN"', '"ANSI_LYELLOW"bisexual"ANSI_LGREEN"', '"ANSI_LYELLOW"asexual"ANSI_LGREEN"', '"ANSI_LYELLOW"gay"ANSI_LGREEN"', '"ANSI_LYELLOW"lesbian"ANSI_LGREEN"', '"ANSI_LYELLOW"transvestite"ANSI_LGREEN"', '"ANSI_LYELLOW"unsure"ANSI_LGREEN"' or '"ANSI_LYELLOW"ask"ANSI_LGREEN"'.",params);

                    if(newsexuality != NOTHING) {
                       set_profile_init(character);
                       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s sexuality is now "ANSI_LYELLOW"%s"ANSI_LGREEN".",buffer,sexuality[newsexuality]);
                       db[character].data->player.profile->sexuality = newsexuality;
                       setreturn(OK,COMMAND_SUCC);
		    }
		 } else if(((temp = 1) && (string_prefix("statusirl",scratch_return_string) || string_prefix("irlstatus",scratch_return_string))) ||
                          (!(temp = 0) && (string_prefix("statusivl",scratch_return_string) || string_prefix("ivlstatus",scratch_return_string)))) {

                    int status = NOTHING;

                    /* ---->  Status (IRL/IVL)  <---- */
                    if(Blank(params) || string_prefix("unset",params) || string_prefix("reset",params)) status = 0;
                       else if(string_prefix("available",params)) status = 1;
                          else if(string_prefix("unavailable",params)) status = 2;
                             else if(string_prefix("single",params)) status = 3;
                                else if(string_prefix("engaged",params)) status = 4;
                                   else if(string_prefix("married",params)) status = 5;
                                      else if(string_prefix("divorced",params)) status = 6;
                                         else if(string_prefix("widowed",params)) status = 7;
                                            else if(string_prefix("dating",params) || string_prefix("date",params)) status = 8;
                                               else if(string_prefix("committed",params) || string_prefix("date",params)) status = 9;
                                                  else if(string_prefix("separated",params) || string_prefix("date",params)) status = 10;
                                                     else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"%s"ANSI_LGREEN"' is an invalid status (%s)  -  Please specify either '"ANSI_LYELLOW"available"ANSI_LGREEN"', '"ANSI_LYELLOW"committed"ANSI_LGREEN"', '"ANSI_LYELLOW"dating"ANSI_LGREEN"', '"ANSI_LYELLOW"divorced"ANSI_LGREEN"', '"ANSI_LYELLOW"engaged"ANSI_LGREEN"', '"ANSI_LYELLOW"married"ANSI_LGREEN"', '"ANSI_LYELLOW"separated"ANSI_LGREEN"', '"ANSI_LYELLOW"single"ANSI_LGREEN"', '"ANSI_LYELLOW"widowed"ANSI_LGREEN"' or '"ANSI_LYELLOW"unavailable"ANSI_LGREEN"'.",params,(temp) ? "IRL":"IVL");

                    if(status != NOTHING) {
                       set_profile_init(character);
                       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s marital status (%s) is now "ANSI_LYELLOW"%s"ANSI_LGREEN".",buffer,(temp) ? "IRL":"IVL",statuses[status]);
                       if(temp) db[character].data->player.profile->statusirl = status;
                          else db[character].data->player.profile->statusivl = status;
                       setreturn(OK,COMMAND_SUCC);
		    }
		 } else if(string_prefix("comments",scratch_return_string)) {

                    /* ---->  Comments  <---- */
                    if(strlen(params) <= PROFILE_COMMENTS) {
                       set_profile_init(character);
                       FREENULL(db[character].data->player.profile->comments);
                       db[character].data->player.profile->comments = (char *) alloc_string(compress(params,1));
                       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s comments are %s.",buffer,scratch_buffer);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of %s comments in %s profile is %d characters.",(player == character) ? "your":"a character's",(player == character) ? "your":"their",PROFILE_COMMENTS);
		 } else if(string_prefix("country",scratch_return_string) || string_prefix("homecountry",scratch_return_string)) {

                    /* ---->  Home country  <---- */
                    if(strlen(params) <= PROFILE_COUNTRY) {
                       set_profile_init(character);
                       FREENULL(db[character].data->player.profile->country);
                       db[character].data->player.profile->country = (char *) alloc_string(compress(params,1));
                       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s country is %s.",buffer,scratch_buffer);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of %s country in %s profile is %d characters.",(player == character) ? "your":"a character's",(player == character) ? "your":"their",PROFILE_COUNTRY);
		 } else if(string_prefix("hobbies",scratch_return_string) || string_prefix("activities",scratch_return_string) || string_prefix("hobby",scratch_return_string) || string_prefix("activity",scratch_return_string)) {

                    /* ---->  Hobbies  <---- */
                    if(strlen(params) <= PROFILE_HOBBIES) {
                       set_profile_init(character);
                       FREENULL(db[character].data->player.profile->hobbies);
                       db[character].data->player.profile->hobbies = (char *) alloc_string(compress(params,1));
                       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s hobbies are %s.",buffer,scratch_buffer);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of %s hobbies in %s profile is %d characters.",(player == character) ? "your":"a character's",(player == character) ? "your":"their",PROFILE_HOBBIES);
		 } else if(string_prefix("likes",scratch_return_string)) {

                    /* ---->  Likes  <---- */
                    if(strlen(params) <= PROFILE_LIKES) {
                       set_profile_init(character);
                       FREENULL(db[character].data->player.profile->likes);
                       db[character].data->player.profile->likes = (char *) alloc_string(compress(params,1));
                       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s likes are %s.",buffer,scratch_buffer);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of %s likes in %s profile is %d characters.",(player == character) ? "your":"a character's",(player == character) ? "your":"their",PROFILE_LIKES);
		 } else if(string_prefix("dislikes",scratch_return_string) || string_prefix("hates",scratch_return_string)) {

                    /* ---->  Dislikes  <---- */
                    if(strlen(params) <= PROFILE_DISLIKES) {
                       set_profile_init(character);
                       FREENULL(db[character].data->player.profile->dislikes);
                       db[character].data->player.profile->dislikes = (char *) alloc_string(compress(params,1));
                       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s dislikes are %s.",buffer,scratch_buffer);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of %s likes in %s profile is %d characters.",(player == character) ? "your":"a character's",(player == character) ? "your":"their",PROFILE_DISLIKES);
		 } else if(string_prefix("height",scratch_return_string)) {
                    const    char *majtype = NULL, *mintype = NULL;
                    const    char *majptr  = NULL, *minptr  = NULL;
                    unsigned char metric   = 1,    reset    = 0;
                    int           major    = 0,    minor    = 0;
                    char          *ptr     = params;

                    /* ---->  Height  <---- */
                    if(!Blank(params)) {
                       for(; *ptr && !isdigit(*ptr); ptr++);
                       for(majptr = ptr; *ptr && isdigit(*ptr); ptr++);
                       for(; *ptr && (*ptr == ' '); ptr++);
                       for(majtype = ptr; *ptr && !isdigit(*ptr); ptr++);
                       for(; *ptr && (*ptr == ' '); ptr++);
                       for(minptr = ptr; *ptr && isdigit(*ptr); ptr++);
                       for(; *ptr && (*ptr == ' '); ptr++);
                       for(mintype = ptr; *ptr && !isdigit(*ptr); ptr++);
                       for(; *ptr && (*ptr == ' '); ptr++);

                       if(majptr) major = atol(majptr);
                       if(minptr) minor = atol(minptr);
                       if(majtype && (string_prefix(majtype,"'") || string_prefix(majtype,"ft") || string_prefix(majtype,"feet") || string_prefix(majtype,"foot"))) metric = 0;
                       if(mintype && (string_prefix(mintype,"\"") || string_prefix(mintype,"inches"))) metric = 0;
		    } else reset = 1;

                    if(reset || major || minor) {
                       if(reset || ((major <= 255) && ((metric && (minor <= 99)) || (!metric && (minor <= 11))))) {
                          if(!reset) {
                             if(major > 0) sprintf(scratch_return_string,"%d%s",major,(metric) ? "m":"'");
                                else *scratch_return_string = '\0';
                             if(minor > 0) sprintf(scratch_return_string + strlen(scratch_return_string),"%s%d%s",(*scratch_return_string) ? " ":"",minor,(metric) ? "cm":"\"");
                             sprintf(scratch_buffer,"now "ANSI_LYELLOW"%s"ANSI_LGREEN,scratch_return_string);
		          } else strcpy(scratch_buffer,"no-longer set");
                          set_profile_init(character);
                          db[character].data->player.profile->height = (reset) ? 0:((metric << 15) + (major << 7) + minor);
                          if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s height is %s.",buffer,scratch_buffer);
                          setreturn(OK,COMMAND_SUCC);
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that height is invalid.");
		    } else if(player == character) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify your height either in "ANSI_LYELLOW"metres"ANSI_LGREEN"/"ANSI_LYELLOW"centimetres"ANSI_LGREEN" (E.g:  '"ANSI_LWHITE"1m 96cm"ANSI_LGREEN"') or "ANSI_LYELLOW"feet"ANSI_LGREEN"/"ANSI_LYELLOW"inches"ANSI_LGREEN" (E.g:  '"ANSI_LWHITE"5' 8\""ANSI_LGREEN"'.)");
                       else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s height either in "ANSI_LYELLOW"metres"ANSI_LGREEN"/"ANSI_LYELLOW"centimetres"ANSI_LGREEN" (E.g:  '"ANSI_LWHITE"1m 96cm"ANSI_LGREEN"') or "ANSI_LYELLOW"feet"ANSI_LGREEN"/"ANSI_LYELLOW"inches"ANSI_LGREEN" (E.g:  '"ANSI_LWHITE"5' 8\""ANSI_LGREEN"'.)",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0));
		 } else if(string_prefix("weight",scratch_return_string)) {
                    const    char *majtype = NULL, *mintype = NULL;
                    const    char *majptr  = NULL, *minptr  = NULL;
                    unsigned char metric   = 1,    reset    = 0;
                    int           major    = 0,    minor    = 0;
                    char          *ptr     = params;

                    /* ---->  Weight  <---- */
                    if(!Blank(params)) {
                       for(; *ptr && !isdigit(*ptr); ptr++);
                       for(majptr = ptr; *ptr && isdigit(*ptr); ptr++);
                       for(; *ptr && (*ptr == ' '); ptr++);
                       for(majtype = ptr; *ptr && !isdigit(*ptr); ptr++);
                       for(; *ptr && (*ptr == ' '); ptr++);
                       for(minptr = ptr; *ptr && isdigit(*ptr); ptr++);
                       for(; *ptr && (*ptr == ' '); ptr++);
                       for(mintype = ptr; *ptr && !isdigit(*ptr); ptr++);
                       for(; *ptr && (*ptr == ' '); ptr++);

                       if(majptr) major = atol(majptr);
                       if(minptr) minor = atol(minptr);
                       if(majtype && (string_prefix(majtype,"lbs") || string_prefix(majtype,"pounds"))) metric = 0;
                       if(mintype && (string_prefix(mintype,"oz") || string_prefix(mintype,"ounces"))) metric = 0;
		    } else reset = 1;

                    if(reset || major || minor) {
                       if(reset || ((major <= 999) && ((metric && (minor <= 999)) || (!metric && (minor <= 11))))) {
                          if(!reset) {
                             if(major > 0) sprintf(scratch_return_string,"%d%s",major,(metric) ? "Kg":"lbs");
                                else *scratch_return_string = '\0';
                             if(minor > 0) sprintf(scratch_return_string + strlen(scratch_return_string),"%s%d%s",(*scratch_return_string) ? " ":"",minor,(metric) ? "g":"oz");
                             sprintf(scratch_buffer,"now "ANSI_LYELLOW"%s"ANSI_LGREEN,scratch_return_string);
		          } else strcpy(scratch_buffer,"no-longer set");
                          set_profile_init(character);
                          db[character].data->player.profile->weight = (reset) ? 0:((metric << 31) + (major << 16) + minor);
                          if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s weight is %s.",buffer,scratch_buffer);
                          setreturn(OK,COMMAND_SUCC);
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that weight is invalid.");
		    } else if(player == character) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify your weight either in "ANSI_LYELLOW"kilograms"ANSI_LGREEN"/"ANSI_LYELLOW"grams"ANSI_LGREEN" (E.g:  '"ANSI_LWHITE"96Kg 200g"ANSI_LGREEN"') or "ANSI_LYELLOW"pounds"ANSI_LGREEN"/"ANSI_LYELLOW"ounces"ANSI_LGREEN" (E.g:  '"ANSI_LWHITE"168lbs 3oz"ANSI_LGREEN"'.)");
                       else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s weight either in "ANSI_LYELLOW"kilograms"ANSI_LGREEN"/"ANSI_LYELLOW"grams"ANSI_LGREEN" (E.g:  '"ANSI_LWHITE"96Kg 200g"ANSI_LGREEN"') or "ANSI_LYELLOW"pounds"ANSI_LGREEN"/"ANSI_LYELLOW"ounces"ANSI_LGREEN" (E.g:  '"ANSI_LWHITE"168lbs 3oz"ANSI_LGREEN"'.)",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0));
		 } else if(string_prefix("drinks",scratch_return_string) || string_prefix("favouritedrinks",scratch_return_string)) {

                    /* ---->  Favourite drinks  <---- */
                    if(strlen(params) <= PROFILE_DRINK) {
                       set_profile_init(character);
                       FREENULL(db[character].data->player.profile->drink);
                       db[character].data->player.profile->drink = (char *) alloc_string(compress(params,1));
                       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s favourite drinks are %s.",buffer,scratch_buffer);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of %s favourite drinks in %s profile is %d characters.",(player == character) ? "your":"a character's",(player == character) ? "your":"their",PROFILE_DRINK);
		 } else if(string_prefix("music",scratch_return_string) || string_prefix("favouritemusic",scratch_return_string)) {

                    /* ---->  Favourite music  <---- */
                    if(strlen(params) <= PROFILE_MUSIC) {
                       set_profile_init(character);
                       FREENULL(db[character].data->player.profile->music);
                       db[character].data->player.profile->music = (char *) alloc_string(compress(params,1));
                       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s favourite music is %s.",buffer,scratch_buffer);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of %s favourite music in %s profile is %d characters.",(player == character) ? "your":"a character's",(player == character) ? "your":"their",PROFILE_MUSIC);
		 } else if(string_prefix("other",scratch_return_string) || string_prefix("miscellaneous",scratch_return_string)) {

                    /* ---->  Other (Miscellaneous) information  <---- */
                    if(strlen(params) <= PROFILE_OTHER) {
                       set_profile_init(character);
                       FREENULL(db[character].data->player.profile->other);
                       db[character].data->player.profile->other = (char *) alloc_string(compress(params,1));
                       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s other (Miscellaneous) information is %s.",buffer,scratch_buffer);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of %s other (Miscellaneous) information in %s profile is %d characters.",(player == character) ? "your":"a character's",(player == character) ? "your":"their",PROFILE_OTHER);
		 } else if(string_prefix("sports",scratch_return_string) || string_prefix("favouritesports",scratch_return_string)) {

                    /* ---->  Favourite sports  <---- */
                    if(strlen(params) <= PROFILE_SPORT) {
                       set_profile_init(character);
                       FREENULL(db[character].data->player.profile->sport);
                       db[character].data->player.profile->sport = (char *) alloc_string(compress(params,1));
                       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s favourite sports are %s.",buffer,scratch_buffer);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of %s favourite sports in %s profile is %d characters.",(player == character) ? "your":"a character's",(player == character) ? "your":"their",PROFILE_SPORT);
		 } else if(string_prefix("city",scratch_return_string) || string_prefix("town",scratch_return_string) || string_prefix("homecity",scratch_return_string) || string_prefix("hometown",scratch_return_string)) {

                    /* ---->  Town/city  <---- */
                    if(strlen(params) <= PROFILE_CITY) {
                       set_profile_init(character);
                       FREENULL(db[character].data->player.profile->city);
                       db[character].data->player.profile->city = (char *) alloc_string(compress(params,1));
                       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s town/city is %s.",buffer,scratch_buffer);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of %s town/city in %s profile is %d characters.",(player == character) ? "your":"a character's",(player == character) ? "your":"their",PROFILE_CITY);
		 } else if(string_prefix("eyes",scratch_return_string) || string_prefix("eyecolour",scratch_return_string) || string_prefix("eyecolor",scratch_return_string)) {

                    /* ---->  Eye colour  <---- */
                    if(strlen(params) <= PROFILE_EYES) {
                       set_profile_init(character);
                       FREENULL(db[character].data->player.profile->eyes);
                       db[character].data->player.profile->eyes = (char *) alloc_string(compress(params,0));
                       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s eye colour is %s.",buffer,scratch_buffer);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of %s eye colour in %s profile is %d characters.",(player == character) ? "your":"a character's",(player == character) ? "your":"their",PROFILE_EYES);
		 } else if(string_prefix("foods",scratch_return_string) || string_prefix("favouritefoods",scratch_return_string)) {

                    /* ---->  Favourite foods  <---- */
                    if(strlen(params) <= PROFILE_FOOD) {
                       set_profile_init(character);
                       FREENULL(db[character].data->player.profile->food);
                       db[character].data->player.profile->food = (char *) alloc_string(compress(params,1));
                       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s favourite foods are %s.",buffer,scratch_buffer);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of %s favourite foods in %s profile is %d characters.",(player == character) ? "your":"a character's",(player == character) ? "your":"their",PROFILE_FOOD);
		 } else if(string_prefix("hair",scratch_return_string) || string_prefix("haircolour",scratch_return_string) || string_prefix("haircolor",scratch_return_string)) {

                    /* ---->  Hair colour  <---- */
                    if(strlen(params) <= PROFILE_HAIR) {
                       set_profile_init(character);
                       FREENULL(db[character].data->player.profile->hair);
                       db[character].data->player.profile->hair = (char *) alloc_string(compress(params,0));
                       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s hair colour is %s.",buffer,scratch_buffer);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of %s hair colour in %s profile is %d characters.",(player == character) ? "your":"a character's",(player == character) ? "your":"their",PROFILE_HAIR);
		 } else if(string_prefix("name",scratch_return_string) || string_prefix("irl",scratch_return_string) || string_prefix("realname",scratch_return_string) || string_prefix("irlname",scratch_return_string)) {

                    /* ---->  Real name  <---- */
                    if(strlen(params) <= PROFILE_IRL) {
                       set_profile_init(character);
                       filter_spaces(params,params,1);
                       if(!Blank(params)) sprintf(scratch_buffer,"now '"ANSI_LYELLOW"%s"ANSI_LGREEN"'",substitute(player,scratch_return_string,(char *) punctuate(params,2,'\0'),0,ANSI_LYELLOW,NULL,0));
                       FREENULL(db[character].data->player.profile->irl);
                       db[character].data->player.profile->irl = (char *) alloc_string(compress(params,1));
                       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s real name is %s.",buffer,scratch_buffer);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of %s real name in %s profile is %d characters.",(player == character) ? "your":"a character's",(player == character) ? "your":"their",PROFILE_IRL);
		 } else if(string_prefix("birthday",scratch_return_string) || string_prefix("dob",scratch_return_string) || string_prefix("dateofbirth",scratch_return_string) || string_prefix("birthdate",scratch_return_string)) {
                    unsigned long longdate = UNSET_DATE,current;
                    unsigned char invalid;
      	            time_t        now;

                    gettime(now);
                    set_profile_init(character);
                    current = epoch_to_longdate(now);
                    if(!Blank(params)) {
                       current  = epoch_to_longdate(now);
                       longdate = string_to_date(player,params,0,1,&invalid);
                       if(invalid) {
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that date is invalid  -  Please specify your date of birth in the following format:  "ANSI_LYELLOW"<DAY> <MONTH> <YEAR>"ANSI_LGREEN", e.g:  '"ANSI_LWHITE"15/4/1975"ANSI_LGREEN"' or '"ANSI_LWHITE"15 April 1975"ANSI_LGREEN"' for "ANSI_LYELLOW"15th April 1975"ANSI_LGREEN".");
                          return;
		       } else if(current < longdate) {
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, your birthday can't be in the future.");
                          return;
		       } else if(!Level4(db[player].owner) && ((longdate_difference(longdate,current) / 12) <= 5)) {
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that date of birth makes you 5 or less years old!  -  Surely you can't be that young?");
                          return;
		       }
                       db[character].data->player.profile->dob = longdate;
		    } else {
                       db[character].data->player.profile->dob = UNSET_DATE;
                       longdate = UNSET_DATE;
		    }

                    if(!in_command) {
                       birthday_notify(now,player);
                       if(player != character) {
                          if(longdate != UNSET_DATE) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN"'s birthday is now "ANSI_LYELLOW"%s"ANSI_LGREEN".",Article(character,UPPER,DEFINITE),getcname(player,character,0,0),date_to_string(UNSET_DATE,longdate,player,SHORTDATEFMT));
                             else output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN"'s birthday is no-longer set.",Article(character,UPPER,DEFINITE),getcname(player,character,0,0));
		       } else if(longdate != UNSET_DATE) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Your birthday is now "ANSI_LYELLOW"%s"ANSI_LGREEN".",date_to_string(UNSET_DATE,longdate,player,SHORTDATEFMT));
   	                  else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Your birthday is no-longer set.");
		    }
                    setreturn(OK,COMMAND_SUCC);
		 } else if(string_prefix("url",scratch_return_string) || string_prefix("pictureurl",scratch_return_string) || string_prefix("imageurl",scratch_return_string) || string_prefix("portraiturl",scratch_return_string) || string_prefix("galleryurl",scratch_return_string)) {
                    char *extptr;

                    /* ---->  Picture (URL)  <---- */
                    if(strlen(params) <= PROFILE_PICTURE) {
                       if(!strchr(params,'\n')) {
                          if(Level2(player) || Blank(params) || !strncasecmp(params,"http:",5) || !strncasecmp(params,"ftp:",4)) {
                             if(Level2(player) || Blank(params) || instring("/?",params) || ((extptr = (char *) strrchr(params,'.')) && (!strcasecmp(extptr + 1,"gif") || !strcasecmp(extptr + 1,"jpg") || !strcasecmp(extptr + 1,"jpeg") || !strcasecmp(extptr + 1,"html") || !strcasecmp(extptr + 1,"htm")))) {
                                set_profile_init(character);
                                ansi_code_filter(params,params,1);
                                if(!Blank(params)) sprintf(scratch_buffer,"now '"ANSI_LYELLOW"%s"ANSI_LGREEN"'",params);
                                FREENULL(db[character].data->player.profile->picture);
                                db[character].data->player.profile->picture = (char *) alloc_string(compress(params,0));
                                if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s picture (URL) is %s.",buffer,scratch_buffer);
                                setreturn(OK,COMMAND_SUCC);
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s picture (URL) in %s profile must end with either '"ANSI_LWHITE".gif"ANSI_LGREEN"', '"ANSI_LWHITE".jpg"ANSI_LGREEN"' or '"ANSI_LWHITE".jpeg"ANSI_LGREEN"'.",(player == character) ? "your":"a character's",(player == character) ? "your":"their");
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s picture (URL) in %s profile must begin with either '"ANSI_LWHITE"http:"ANSI_LGREEN"' or '"ANSI_LWHITE"ftp:"ANSI_LGREEN"'.",(player == character) ? "your":"a character's",(player == character) ? "your":"their");
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s picture (URL) in %s profile can't contain embedded NEWLINE's.",(player == character) ? "your":"a character's",(player == character) ? "your":"their");
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of %s picture (URL) in %s profile is %d characters.",(player == character) ? "your":"a character's",(player == character) ? "your":"their",PROFILE_PICTURE);
		 } else if(string_prefix("email",scratch_return_string) || string_prefix("emailaddress",scratch_return_string)) {
                    sprintf(buffer,"*%s",getname(character));
                    set_email(player,NULL,NULL,buffer,params,0,0);
		 } else if(string_prefix("wwwpages",scratch_return_string) || string_prefix("homepages",scratch_return_string) || string_prefix("webpages",scratch_return_string) || string_prefix("worldwidewebpages",scratch_return_string)) {
                    sprintf(buffer,"*%s",getname(character));
                    set_www(player,NULL,NULL,buffer,params,0,0);
		 } else if(string_prefix("age",scratch_return_string)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't set %s age in %s profile  -  This is calculated automatically from %s date of birth.",(player == character) ? "your":"a character's",(player == character) ? "your":"their",(player == character) ? "your":"their");
		    else if(string_prefix("statusivl",scratch_return_string) || string_prefix("ivlstatus",scratch_return_string)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't set %s status (IVL) in %s profile  -  This is set using the '"ANSI_LYELLOW"@partner"ANSI_LGREEN"' command (By an Apprentice Wizard/Druid or above.)",(player == character) ? "your":"a character's",(player == character) ? "your":"their");
		       else if(player == character) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, your profile doesn't have the field '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",scratch_return_string);
                          else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s profile doesn't have the field '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0),scratch_return_string);
                 if(!hasprofile(db[character].data->player.profile)) FREENULL(db[character].data->player.profile);
                 setreturn(OK,COMMAND_SUCC);
	      } else if(player == character) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify which information in your profile you'd like to change.");
                 else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify which information in %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s profile you'd like to change.",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0));
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't change information on their profile.",Article(character,LOWER,DEFINITE),getcname(player,character,1,0));
	} else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change information on the profile of someone who's of a lower level than yourself.");
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change information on your own profile or the profile of one of your puppets.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't change information on a character's profile from within a compound command.");
}

/* ---->  Set race of character  <---- */
void set_race(CONTEXT)
{
     dbref character;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(arg2)) {
        if(!Blank(arg1)) {
           if((character = lookup_character(player,arg1,1)) == NOTHING) {
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
              return;
	   }
	} else character = db[player].owner;
     } else character = db[player].owner, arg2 = arg1;

     ansi_code_filter((char *) arg2,arg2,1);
     if(!in_command || Wizard(current_command)) {
        if(!Readonly(character)) {
           if(can_write_to(player,character,0)) {
	      if(strlen(arg2) <= 50) {
                 if(Level2(db[player].owner) || (strlen(arg2) > 2)) {
                    if(!(!Blank(arg2) && strchr(arg2,'\n'))) {
                       if(!(!Blank(arg2) && instring("%{",arg2))) {
                          if(!(!Blank(arg2) && instring("%h",arg2))) {
   	                     if(ok_name(arg2)) {
                                setfield(character,RACE,arg2,1);
                                if(!in_command) {
                                   strcpy(scratch_buffer,punctuate((char *) substitute(player,scratch_return_string,arg2,0,ANSI_LWHITE,NULL,0),2,'\0'));
                                   if(player != character) {
                                      if(!in_command) output(getdsc(character),character,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"%s"ANSI_LYELLOW"%s"ANSI_LWHITE" has changed your race to '"ANSI_LYELLOW"%s"ANSI_LWHITE"'.",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0),arg2);
                                      *arg2 = toupper(*arg2);
                                      if(Controller(character) != player) {
                                         if(!in_command) writelog(ADMIN_LOG,1,"RACE CHANGE","%s(#%d) changed %s(#%d)'s race to '%s'.",getname(player),player,getname(character),character,arg2);
                                            else if(!Wizard(current_command)) writelog(HACK_LOG,1,"HACK","%s(#%d) changed %s(#%d)'s race to '%s' within compound command %s(#%d).",getname(player),player,getname(character),character,arg2,getname(current_command),current_command);
				      }
                                      output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN"'s race set to '"ANSI_LYELLOW"%s"ANSI_LGREEN"'.",Article(character,UPPER,DEFINITE),getcname(NOTHING,character,0,0),scratch_buffer);
				   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Your race is now '"ANSI_LYELLOW"%s"ANSI_LGREEN"'.",scratch_buffer);
				}
                                setreturn(OK,COMMAND_SUCC);
		  	     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that race is invalid.");
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s race can't contain embedded HTML tags.",(character == player) ? "your":"a character's");
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s race can't contain query command substitutions ('"ANSI_LWHITE"%%{<QUERY COMMAND>}"ANSI_LGREEN"'.)",(character == player) ? "your":"a character's");
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s race can't contain embedded NEWLINE's.",(character == player) ? "your":"a character's");
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that race is too short  -  It must be at least 3 characters in length.");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of %s race is 50 characters.",(character == player) ? "your":"a character's");
	   } else if(Level3(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change your own race or the race of someone of a lower level than yourself.");
              else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change your own race.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that character is Read-Only  -  You can't change their race.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the race of a character can't be changed from within a compound command.");
}

/* ---->  Set/adjust character's score  <---- */
void set_score(CONTEXT)
{
     int   score,oldscore;
     dbref character;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(arg1)) {
        if((character = lookup_character(player,arg1,1)) == NOTHING) {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
           return;
	}
     } else character = db[player].owner;

     if(Level3(db[player].owner)) {
        if(can_write_to(player,character,0)) {
           if((score = atoi(arg2))) {
              oldscore = db[character].data->player.score;
              db[character].data->player.score += score;
              if(!in_command) {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"You %s %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s score by "ANSI_LWHITE"%d"ANSI_LGREEN" point%s to "ANSI_LYELLOW"%d"ANSI_LGREEN".",(db[character].data->player.score > oldscore) ? "increase":"decrease",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0),ABS(db[character].data->player.score - oldscore),Plural(ABS(db[character].data->player.score - oldscore)),db[character].data->player.score);
                 if(player != character) output(getdsc(character),character,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" %s your score by "ANSI_LWHITE"%d"ANSI_LGREEN" point%s to "ANSI_LYELLOW"%d"ANSI_LGREEN".",Article(player,UPPER,(db[player].location == db[character].location) ? DEFINITE:INDEFINITE),getcname(NOTHING,player,0,0),(db[character].data->player.score > oldscore) ? "increases":"decreases",ABS(db[character].data->player.score - oldscore),Plural(ABS(db[character].data->player.score - oldscore)),db[character].data->player.score);
	      }
              setreturn(OK,COMMAND_SUCC);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify how much you'd like to adjust %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s score by.",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0));
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only adjust your own score, or the score of someone who's of a lower level than yourself.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Wizards/Druids and above can adjust the score of a character.");
}

/* ---->  Run through screen configuration questions again  <---- */
void set_screenconfig(CONTEXT)
{
     struct descriptor_data *p = getdsc(player);

     setreturn(ERROR,COMMAND_FAIL);
     if(p) {
        if(!IsHtml(p)) {
           p->clevel = 21;
           setreturn(OK,COMMAND_SUCC);
	} else output(p,player,0,1,0,ANSI_LGREEN"Sorry, the '"ANSI_LWHITE"screenconfig"ANSI_LGREEN"' command can't be used with World Wide Web Interface connections (It isn't neccessary.)");
     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you must be connected to use the '"ANSI_LWHITE"screenconfig"ANSI_LGREEN"' command.");
}

/* ---->  Set success message of object  <---- */
void set_succ(CONTEXT)
{
     dbref thing;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if(HasField(thing,SUCC)) {
        if(CanSetField(thing,SUCC)) {
           if(can_write_to(player,thing,0)) {
              if(!Readonly(thing)) {
                 setfield(thing,SUCC,arg2,0);
                 if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Success message of %s"ANSI_LWHITE"%s"ANSI_LGREEN" set.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
                 setreturn(OK,COMMAND_SUCC);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't change %s success message.",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0),(Typeof(thing) == TYPE_CHARACTER) ? "their":"its");
	   } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the success message of something you own or something owned by someone of a lower level than yourself.");
              else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the success message of something you own.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the success message of %s can't be set.",object_type(thing,1));
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s doesn't have a success message.",object_type(thing,1));
}

/* ---->  Set name suffix of character  <---- */
void set_suffix(CONTEXT)
{
     dbref character;
     const char *ptr;
     int   inherit;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(arg1)) {
        if((character = lookup_character(player,arg1,1)) == NOTHING) {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
           return;
	}
     } else character = db[player].owner;

     if(Level4(db[player].owner)) {
        if(can_write_to(player,character,1)) {
           if(!Readonly(character)) {
              ansi_code_filter((char *) arg2,arg2,1);
              filter_spaces(scratch_buffer,arg2,0);
              if(!Censor(player) && !Censor(db[player].location)) bad_language_filter(scratch_buffer,scratch_buffer);

              switch(ok_presuffix(player,scratch_buffer,0)) {
                     case 1:
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't give a character the name suffix '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",scratch_buffer);
                          return;
                     case 2:
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of a character's name suffix is 40 characters.");
                          return;
                     case 3:
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a character's name suffix mustn't contain embedded NEWLINE's.");
                          return;
                     case 4:
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a character's name suffix mustn't contain "ANSI_LWHITE"$"ANSI_LGREEN"'s, "ANSI_LWHITE"{}"ANSI_LGREEN"'s, "ANSI_LWHITE"%%"ANSI_LGREEN"'s or "ANSI_LWHITE"="ANSI_LGREEN"'s.");
                          return;
                     case 5:
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a character's name suffix mustn't contain "ANSI_LWHITE"%c"ANSI_LGREEN"'s, "ANSI_LWHITE"%c"ANSI_LGREEN"'s or "ANSI_LWHITE"%c"ANSI_LGREEN".",COMMAND_TOKEN,NUMBER_TOKEN,LOOKUP_TOKEN);
                          return;
                     case 6:
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the name suffix '"ANSI_LWHITE"%s"ANSI_LGREEN"' is not allowed.",scratch_buffer);
                          return;
	      }

              if(character != player) {
                 if(!in_command) output(getdsc(character),character,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"%s"ANSI_LYELLOW"%s"ANSI_LWHITE" has changed your name suffix to '"ANSI_LYELLOW"%s"ANSI_LWHITE"'.",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0),scratch_buffer);
                 sprintf(scratch_return_string,ANSI_LGREEN"%s"ANSI_LWHITE"%s",Article(player,UPPER,DEFINITE),getcname(player,character,1,0));
                 if(!in_command) writelog(ADMIN_LOG,1,"SUFFIX CHANGE","%s(#%d) changed %s(#%d)'s name suffix to '%s'.",getname(player),player,getname(character),character,scratch_buffer);
                    else if(!Wizard(current_command)) writelog(HACK_LOG,1,"HACK","%s(#%d) changed %s(#%d)'s name suffix to '%s' within compound command %s(#%d).",getname(player),player,getname(character),character,scratch_buffer,getname(current_command),current_command);
	      }
              setfield(character,SUFFIX,scratch_buffer,0);
              ptr = String(getfield(character,SUFFIX));
              inherit = inherited;

              if(!in_command) {
                 if(character != player) output(getdsc(player),player,0,1,0,"%s"ANSI_LGREEN"'s name suffix set to '"ANSI_LYELLOW"%s"ANSI_LGREEN"'%s",scratch_return_string,ptr,(inherit > 0) ? " (Inherited.)":".");
                    else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Your name suffix is now '"ANSI_LYELLOW"%s"ANSI_LGREEN"'%s",ptr,(inherit > 0) ? " (Inherited.)":".");
	      }
              setreturn(OK,COMMAND_SUCC);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't change their name suffix.",Article(character,LOWER,DEFINITE),getcname(player,character,1,0));
	} else if(Level4(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the name suffix of someone who's of a lower level than yourself.");
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change your own name suffix.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above can set the name suffix of a character.");
}

/* ---->  Unlink object  <---- */
void set_unlink(CONTEXT)
{
     dbref thing;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,params,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_ROOM|SEARCH_EXIT,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if(can_write_to(player,thing,0)) {
        if(!Readonly(thing)) {
           switch(Typeof(thing)) {
                  case TYPE_EXIT:
                       db[thing].destination = NOTHING;
                       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Exit %s"ANSI_LWHITE"%s"ANSI_LGREEN" unlinked.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
                       setreturn(OK,COMMAND_SUCC);
                       break;
                  case TYPE_ROOM:
                       db[thing].destination = NOTHING;
                       if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Drop-to of %s"ANSI_LWHITE"%s"ANSI_LGREEN" reset.",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
                       setreturn(OK,COMMAND_SUCC);
                       break;
                  default:
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't unlink %s.",object_type(thing,1));
	   }
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that %s is Read-Only  -  You can't unlink %s.",object_type(thing,0),(Typeof(thing) == TYPE_CHARACTER) ? "them":"it");
     } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only unlink something you own or something owned by someone of a lower level than yourself.");
        else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only unlink something you own.");
}

/* ---->  Remove lock of object  <---- */
void set_unlock(CONTEXT)
{
     dbref thing;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,params,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if(HasField(thing,LOCK)) {
        if(can_write_to(player,thing,0)) {
           if(!Readonly(thing)) {
              switch(Typeof(thing)) {
                     case TYPE_EXIT:
                          free_boolexp(&(db[thing].data->exit.lock));
                          db[thing].data->exit.lock = TRUE_BOOLEXP;
                          break;
                     case TYPE_FUSE:
                          free_boolexp(&(db[thing].data->fuse.lock));
                          db[thing].data->fuse.lock = TRUE_BOOLEXP;
                          break;
                     case TYPE_ROOM:
                          free_boolexp(&(db[thing].data->room.lock));
                          db[thing].data->room.lock = TRUE_BOOLEXP;
                          break;
                     case TYPE_THING:
                          free_boolexp(&(db[thing].data->thing.lock));
                          db[thing].data->thing.lock = TRUE_BOOLEXP;
                          break;
                     case TYPE_COMMAND:
                          free_boolexp(&(db[thing].data->command.lock));
                          db[thing].data->command.lock = TRUE_BOOLEXP;
                          break;
	      }
              if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" unlocked.",Article(thing,UPPER,INDEFINITE),unparse_object(player,thing,0));
              setreturn(OK,COMMAND_SUCC);
           } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't unlock %s.",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0),(Typeof(thing) == TYPE_CHARACTER) ? "them":"it");
	} else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only unlock something you own or something owned by someone of a lower level than yourself.");
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only unlock something you own.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s can't be unlocked.",object_type(thing,1));
}

/* ---->  Set volume/volume limit of object  <---- */
void set_volume(CONTEXT)
{
     int   volume;
     dbref thing;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if(string_prefix("infinity",arg2) || string_prefix("infinite",arg2) || (ABS(atol(arg2)) >= INFINITY)) {
        if(!Level4(db[player].owner)) {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may set the volume of %s to infinity.",object_type(thing,1));
           return;
	} else volume = INFINITY;
     } else volume = (string_prefix("inheritable",arg2)) ? INHERIT:ABS(atol(arg2));

     if((Typeof(thing) != TYPE_CHARACTER) || !in_command || Wizard(current_command)) {
        if(HasField(thing,VOLUME)) {
           if(can_write_to(player,thing,0)) {
              if(!Readonly(thing)) {
                 if(!((volume != INHERIT) && (volume < find_volume_of_contents(thing,0)))) {
                    if(!(!volume && !Level4(db[player].owner))) {
                       if(!((volume != INHERIT) && (Typeof(thing) != TYPE_ROOM) && ((volume - get_mass_or_volume(thing,1) + find_volume_of_contents(Location(thing),0)) > get_mass_or_volume(Location(thing),1)))) {
                          switch(Typeof(thing)) {
                                 case TYPE_THING:
                                      db[thing].data->thing.volume  = volume;
                                      break;
                                 case TYPE_CHARACTER:
                                      db[thing].data->player.volume = volume;
                                      break;
                                 case TYPE_ROOM:
                                      db[thing].data->room.volume   = volume;
                                      break;
                                 default:
                                      break;
			  }

                          if(!in_command)
                             switch(volume) {
                                    case INHERIT:
                                         sprintf(scratch_buffer,ANSI_LGREEN"Volume%s of %s"ANSI_LWHITE"%s"ANSI_LGREEN" will be inherited%s",(Typeof(thing) == TYPE_ROOM) ? " limit":"",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0),Valid(db[thing].parent) ? "":".");
                                         if(Valid(db[thing].parent)) sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LGREEN" from %s"ANSI_LYELLOW"%s"ANSI_LGREEN".",Article(db[thing].parent,LOWER,INDEFINITE),unparse_object(player,db[thing].parent,0)); 
                                         output(getdsc(player),player,0,1,0,"%s",scratch_buffer);
                                         break;
                                    case INFINITY:
                                         output(getdsc(player),player,0,1,0,ANSI_LGREEN"Volume%s of %s"ANSI_LWHITE"%s"ANSI_LGREEN" set to "ANSI_LYELLOW"INFINITY"ANSI_LGREEN".",(Typeof(thing) == TYPE_ROOM) ? " limit":"",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0));
                                         break;
                                    default:
                                         output(getdsc(player),player,0,1,0,ANSI_LGREEN"Volume%s of %s"ANSI_LWHITE"%s"ANSI_LGREEN" set to "ANSI_LYELLOW"%d"ANSI_LGREEN" Litre%s.",(Typeof(thing) == TYPE_ROOM) ? " limit":"",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0),volume,Plural(volume));
			     }
                          setreturn(OK,COMMAND_SUCC);
		       } else {
                          sprintf(scratch_buffer,ANSI_LGREEN"Sorry, you can't make %s"ANSI_LWHITE"%s"ANSI_LGREEN" that large in ",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0));
                          output(getdsc(player),player,0,1,0,"%s"ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(db[thing].location,LOWER,DEFINITE),unparse_object(player,db[thing].location,0));
		       }
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" can't have no volume%s.",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0),(Typeof(thing) == TYPE_ROOM) ? " limit":"");
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't set %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s volume%s to less than its contents.",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0),(Typeof(thing) == TYPE_ROOM) ? " limit":"");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't change %s volume%s.",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0),(Typeof(thing) == TYPE_CHARACTER) ? "their":"its",(Typeof(thing) == TYPE_ROOM) ? " limit":"");
	   } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the volume%s of something you own or something owned by someone of a lower level than yourself.",(Typeof(thing) == TYPE_ROOM) ? " limit":"");
              else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the volume%s of something you own.",(Typeof(thing) == TYPE_ROOM) ? " limit":"");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s doesn't have volume.",object_type(thing,1));
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the volume of a character can't be changed from within a compound command.");
}

/* ---->  Set web site address of character  <---- */
void set_www(CONTEXT)
{
     dbref character;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(arg1)) {
        if((character = lookup_character(player,arg1,1)) == NOTHING) {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
           return;
	}
     } else character = db[player].owner;

     if(!in_command || Wizard(current_command)) {
        if(can_write_to(player,character,0)) {
           if(!Readonly(character)) {
	      if(strlen(arg2) <= 128) {
                 if(Level2(db[player].owner) || !strchr(arg2,'\n')) {
                    if(!Level2(player) && !Blank(arg2) && strncasecmp(arg2,"http://",7))
                       sprintf(scratch_return_string,"http://%s",arg2);
                          else strcpy(scratch_return_string,arg2);

		    setfield(character,WWW,scratch_return_string,0);
		    if(!in_command) {
		       if(player != character) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN"'s web site address set to '"ANSI_LYELLOW"%s"ANSI_LGREEN"'.",Article(character,UPPER,DEFINITE),getcname(NOTHING,character,0,0),scratch_return_string);
			  else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Your web site address is now '"ANSI_LYELLOW"%s"ANSI_LGREEN"'.",scratch_return_string);
		    }

		    if((player != character) && (Controller(character) != player)) {
		       writelog(ADMIN_LOG,1,"WEB SITE","%s(#%d) changed %s(#%d)'s web site address to '%s'.",getname(player),player,getname(character),character,scratch_return_string);
		       if(!Level4(player)) writelog(HACK_LOG,1,"HACK","%s(#%d) changed %s(#%d)'s web site address to '%s'.",getname(player),player,getname(character),character,scratch_return_string);
		    }
		    setreturn(OK,COMMAND_SUCC);
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a web site address mustn't contain embedded NEWLINE's.");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of a web site address is 128 characters.");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't change their web site address.",Article(character,LOWER,DEFINITE),getcname(player,character,1,0));
	} else if(Level3(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change your own web site address or the web site address of someone of a lower level than yourself.");
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change your own web site address.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the web site address of a character can't be changed from within a compound command.");
}
