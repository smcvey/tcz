/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| GROUP.C  -  Implements grouping and range operators (Allows lists to be     |
|             viewed a page at a time, etc.)                                  |
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
| Module originally designed and written by:  J.P.Boggis 17/11/1994.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: group.c,v 1.1.1.1 2004/12/02 17:41:25 jpboggis Exp $

*/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

#define  PRIV_FLAGS (MORON|BUILDER|APPRENTICE|WIZARD|ELDER|DEITY)


/* ---->  Initialise GROUP_DATA  <---- */
struct grp_data *grouprange_initialise(struct grp_data *grpdata)
{
       grpdata->groupitems = 0;
       grpdata->rangeitems = 0;
       grpdata->totalitems = 0;
       grpdata->condition  = 0;
       grpdata->groupsize  = DEFAULT;
       grpdata->distance   = 0;
       grpdata->nogroups   = 0;
       grpdata->cobject    = NOTHING;
       grpdata->nobject    = NOTHING;
       grpdata->groupno    = UNSET;
       grpdata->before     = 0;
       grpdata->cunion     = NULL;
       grpdata->nunion     = NULL;
       grpdata->clower     = 0;
       grpdata->cupper     = 0xFFFFFFFF;
       grpdata->llower     = 0;
       grpdata->lupper     = 0xFFFFFFFF;
       grpdata->player     = NOTHING;
       grpdata->rfrom      = UNSET;
       grpdata->cobj       = NOTHING;
       grpdata->list       = 0;
       grpdata->next       = grp;
       grpdata->rto        = UNSET;
       gettime(grpdata->time);
       return(grpdata);
}

/* ---->  Return value of group/range operator parameter  <---- */
const char *grouprange_value(char *buffer,int value)
{
      switch(value) {
             case ALL:
                  return("ALL");
                  break;
             case LAST:
                  return("LAST");
                  break;
             case FIRST:
                  return("FIRST");
                  break;
             case UNSET:
                  return("UNSET");
                  break;
             case DEFAULT:
                  return("DEFAULT");
                  break;
             default:
                  sprintf(buffer,"%d",value);
                  return(buffer);
      }
}

/* ---->  Returns items listed in form:  N/N [(Page N of N)]  <---- */
/*        BUFFER:  Buffer to write returned string to.                 */
/*        FULLSTOP:  1 = Add full stop ('.'), 0 = Don't add fullstop.  */
const char *listed_items(char *buffer,u_char fullstop)
{
      /* ---->  Display no. items displayed/total items  <---- */
      if(grp->groupitems == grp->totalitems) sprintf(buffer,"%d",grp->rangeitems);
         else if(grp->groupno > 0) sprintf(buffer,"%d/%d",grp->groupitems,grp->rangeitems);
            else sprintf(buffer,"%d/%d",grp->groupitems,grp->totalitems);

      /* ---->  If grouping ops used, display which page is being viewed  <---- */
      if((grp->groupno > 0) && (grp->nogroups > 0)) {
         if(grp->groupitems != grp->totalitems) {
            sprintf(buffer + strlen(buffer)," (Page %d of %d%s)",grp->groupno,grp->nogroups,fullstop ? ".":"");
            fullstop = 0;
	 } else {
            sprintf(buffer + strlen(buffer)," (Page 1 of 1%s)",fullstop ? ".":"");
            fullstop = 0;
	 }
      }

      if(fullstop) strcat(buffer,".");
      return(buffer);
}

/* ---->  Fix-up GRP not pointing to GRPROOT (Grouping/range ops)  <---- */
void sanitise_grproot()
{
     if(grp != &grproot) {
        writelog(BUG_LOG,1,"BUG","(sanitise_grouprange() in group.c)  Grouping/range ops error:  GRP not pointing to GRPROOT at start of tcz_command() in interface.c");
        grp = &grproot;
     }
}

/* ---->  Grab grouping(0)/range(1) operator from given string  <---- */  
const char *grab_operator(int deftype,const char *str,unsigned char defaults)
{
      int   value1 = UNSET,value2 = UNSET,loop;
      const char *scan;
      char  buffer[16];
      char  *p2;

      scan = str;
      *(buffer + 15) = '\x02';
      for(loop = 0; loop < 15; loop++) buffer[loop] = '\0';
      while(*scan && (*scan == ' ')) scan++;
      p2 = buffer;
      while(*scan && isalnum(*scan)) {
            if(*p2 != '\02') *p2++ = *scan;
            scan++;
      }
      *p2 = '\0';

      /* ---->  Grab first value  <---- */
      if(!strcasecmp("default",buffer)) {
         value1 = DEFAULT;
      } else if(!strcasecmp("first",buffer)) {
         value1 = FIRST;
      } else if(!strcasecmp("last",buffer)) {
         value1 = LAST;
      } else if(!strcasecmp("all",buffer)) {
         value1 = ALL;
      } else if(*buffer && isdigit(*buffer)) {
         value1 = atol(buffer);
      } else if(*buffer) {
         return(str);
      } else value1 = INVALID;

      /* ---->  Skip over separator (If present)  <---- */
      if(*scan) switch(*scan) {
         case ':':
              scan++;
              deftype = 0;
              break;
         case '.':
              scan++;
              if(!(*scan && (*scan == '.'))) return(str); 
         case '-':
              scan++;
              deftype = 1;
              break;
          default:
              if((deftype == 1) || ((value1 < 0) && (*scan != ' '))) return(str);
      } else if(deftype == 1) return(str);

      if(value1 == INVALID) {
         if(deftype) value1 = (defaults) ? FIRST:INVALID;
            else value1 = grp->groupno;
      }

      /* ---->  Grab second value  <---- */
      p2 = buffer;
      *(buffer + 15) = '\x02';
      for(loop = 0; loop < 15; loop++) buffer[loop] = '\0';
      while(*scan && isalnum(*scan)) {
            if(*p2 != '\x02') *p2++ = *scan;
            scan++;
      }
      *p2 = '\0';

      if(!strcasecmp("default",buffer)) {
         value2 = DEFAULT;
      } else if(!strcasecmp("first",buffer)) {
         value2 = FIRST;
      } else if(!strcasecmp("last",buffer)) {
         value2 = LAST;
      } else if(!strcasecmp("all",buffer)) {
         value2 = ALL;
      } else if(*buffer && isdigit(*buffer)) {
         value2 = atol(buffer);
      } else if(*buffer) {
         return(str);
      } else value2 = (defaults) ? DEFAULT:INVALID;

      if((value2 < 0) && !(!*scan || (*scan == ' '))) return(str);

      if(!deftype) {
         grp->groupno   = value1;
         grp->groupsize = value2;
      } else if(deftype == 1) {
         grp->rfrom     = value1;
         grp->rto       = value2;
      }
      while(*scan && (*scan == ' ')) scan++;
      return(scan);
}

/* ---->  Parse grouping/range operators from given string  <---- */
const char *parse_grouprange(dbref player,const char *str,char autopage,unsigned char defaults)
{
      const char *scan = str;

      /* ---->  Initialise GRP  <---- */
      grp->totalitems    = 0;
      grp->rangeitems    = 0;
      grp->groupitems    = 0;
      grp->groupsize     = DEFAULT;
      grp->distance      = 0;
      grp->nogroups      = 0;
      grp->groupno       = (autopage == UNSET) ? UNSET:autopage;
      grp->before        = 0;
      grp->rfrom         = (autopage == UNSET) ? UNSET:FIRST;
      grp->rto           = (autopage == UNSET) ? UNSET:LAST;

      grp->cobject       = NOTHING;
      grp->nobject       = NOTHING;
      grp->cunion        = NULL;
      grp->nunion        = NULL;
      grp->cobj          = NOTHING;
      grp->list          = 0;

      grp->object_flags2 = 0;
      grp->object_mask2  = 0;
      grp->object_flags  = 0;
      grp->object_mask   = 0;
      grp->object_type   = NOTHING;
      grp->object_name   = NULL;
      grp->object_who    = NOTHING;
      grp->object_exc    = 0;
      grp->condition     = 0;
      grp->player        = player;

      grp->clower        = 0;
      grp->cupper        = 0xFFFFFFFF;
      grp->llower        = 0;
      grp->lupper        = 0xFFFFFFFF;
      gettime(grp->time);

      /* ---->  Now grab grouping/range operator params (If any)  <---- */
      if(!scan) return(NULL);
      scan = grab_operator(0,scan,defaults);
      scan = grab_operator(1,scan,defaults);

      /* ---->  Return resulting pointer  <---- */
      if(str != scan) {
         while(*scan && (*scan == ' ')) scan++;
         return(scan);
      } else return(str);
}

/* ---->  Returns (1) if OBJECT is one of given type(s)  <---- */
int match_object_type(dbref object,int type,int object_type)
{
    if((object_type & SEARCH_NOT_CHARACTER) && (((object != NOTHING) ? Typeof(object):type) == TYPE_CHARACTER)) return(0);
    if((object_type & SEARCH_NOT_OBJECT)    && (((object != NOTHING) ? Typeof(object):type) != TYPE_CHARACTER)) return(0);

    switch((object != NOTHING) ? Typeof(object):type) {
           case TYPE_PROPERTY:
                if(object_type & SEARCH_PROPERTY) return(1);
                break;
           case TYPE_VARIABLE:
                if(object_type & SEARCH_VARIABLE) return(1);
                break;
           case TYPE_COMMAND:
                if(object_type & SEARCH_COMMAND) return(1);
                break;
           case TYPE_CHARACTER:
                if((object_type & SEARCH_CHARACTER) || ((object != NOTHING) && (object_type & SEARCH_PUPPET) && (Controller(object) != object))) return(1);
                break;
           case TYPE_ALARM:
                if(object_type & SEARCH_ALARM) return(1);
                break;
           case TYPE_ARRAY:
                if(object_type & SEARCH_ARRAY) return(1);
                break;
           case TYPE_THING:
                if(object_type & SEARCH_THING) return(1);
                break;
           case TYPE_EXIT:
                if(object_type & SEARCH_EXIT) return(1);
                break;
           case TYPE_FUSE:
                if(object_type & SEARCH_FUSE) return(1);
                break;
           case TYPE_ROOM:
                if(object_type & SEARCH_ROOM) return(1);
                break;
           default:
                break;
    }
    return(0);
}

/* ---->  Returns (1) if any of given field(s) of OBJECT match given wildcard spec.  <---- */
int match_fields(dbref object,int fields,const char *name)
{
    static int cached_commandtype;
    static unsigned char matched;

    if(!name || !Valid(object)) return(1);
    matched = 0;

    /* ---->  Standard object fields  <---- */
    cached_commandtype = command_type;
    command_type      |= NO_USAGE_UPDATE;
    if(!matched && (fields & SEARCH_NAME)  && match_wildcard(name,getfield(object,NAME)))  matched = 1;
    if(!matched && (fields & SEARCH_DESC)  && match_wildcard(name,getfield(object,DESC)))  matched = 1;
    if(!matched && (fields & SEARCH_ODESC) && match_wildcard(name,getfield(object,ODESC))) matched = 1;
    if(!matched && (fields & SEARCH_SUCC)  && match_wildcard(name,getfield(object,SUCC)))  matched = 1;
    if(!matched && (fields & SEARCH_OSUCC) && match_wildcard(name,getfield(object,OSUCC))) matched = 1;
    if(!matched && (fields & SEARCH_FAIL)  && match_wildcard(name,getfield(object,FAIL)))  matched = 1;
    if(!matched && (fields & SEARCH_OFAIL) && match_wildcard(name,getfield(object,OFAIL))) matched = 1;
    if(!matched && (fields & SEARCH_DROP)) {
       if(Typeof(object) == TYPE_CHARACTER) {
          char buffer[TEXT_SIZE];
          if(match_wildcard_list(name,'\n',getfield(object,EMAIL),buffer)) matched = 1;
       } else if(match_wildcard(name,getfield(object,DROP))) matched = 1;
    }
    if(!matched && (fields & SEARCH_ODROP) && match_wildcard(name,getfield(object,ODROP))) matched = 1;

    /* ---->  Object specific fields  <---- */
    if(!matched) {
       switch(Typeof(object)) {
              case TYPE_THING:
                   if(!matched && (fields & SEARCH_AREANAME) && match_wildcard(name,getfield(object,AREANAME))) matched = 1;
                   if(!matched && (fields & SEARCH_CSTRING)  && match_wildcard(name,getfield(object,CSTRING))) matched = 1;
                   if(!matched && (fields & SEARCH_ESTRING)  && match_wildcard(name,getfield(object,ESTRING))) matched = 1;
                   break;
              case TYPE_ROOM:
                   if(!matched && (fields & SEARCH_AREANAME) && match_wildcard(name,getfield(object,AREANAME))) matched = 1;
                   if(!matched && (fields & SEARCH_CSTRING)  && match_wildcard(name,getfield(object,CSTRING))) matched = 1;
                   if(!matched && (fields & SEARCH_ESTRING)  && match_wildcard(name,getfield(object,ESTRING))) matched = 1;
                   break;
       }
    }
    command_type = cached_commandtype;
    return(matched);
}

/* ---->  Optional condition(s) for each item processed (0 = none)  <---- */
int condition_met(dbref object,union group_data *objunion)
{
    static int flags;

    switch(grp->condition / 100) {
           case 1:

                /* ---->  (DB object)  Standard flags based test  <---- */
                flags = db[object].flags & grp->object_flags;
                if(flags && !(flags & grp->object_mask)) {
                   if(!match_object_type(object,0,grp->object_type)) return(0);
                   switch(grp->condition % 100) {
                          case 1:
                               if(!can_read_from(grp->player,object) && !can_write_to(grp->player,db[object].location,1)) return(0);
                               break;
                          case 2:
                               if((Typeof(object) != TYPE_CHARACTER) || (Controller(object) != object)) return(0);
                               break;
		   }
                   return(match_fields(object,grp->object_who,grp->object_name));
		} else return(0);
                break;
           case 2:

                /* ---->  (Descriptor data)  Standard flags based test  <---- */
                if(!Validchar(objunion->descriptor.player) || !Connected(objunion->descriptor.player) || !(objunion->descriptor.flags & CONNECTED)) return(0);
                if(!grp->object_mask || ((db[objunion->descriptor.player].flags & grp->object_mask) == (grp->object_flags & grp->object_mask)))
                   if(!((grp->object_flags & ~(grp->object_mask)) & PRIV_FLAGS) || (db[objunion->descriptor.player].flags & ((grp->object_flags & ~(grp->object_mask)) & PRIV_FLAGS)))
                      if((!((grp->object_flags & ~(grp->object_mask)) & ~PRIV_FLAGS) || ((db[objunion->descriptor.player].flags & ((grp->object_flags & ~(grp->object_mask)) & ~PRIV_FLAGS)) == ((grp->object_flags & ~(grp->object_mask)) & ~PRIV_FLAGS))) && !(db[objunion->descriptor.player].flags & grp->object_exc))
                         if(!grp->object_mask2 || ((db[objunion->descriptor.player].flags2 & grp->object_mask2) == (grp->object_flags2 & grp->object_mask2)))
                            if(!(grp->object_flags2 & ~(grp->object_mask2)) || ((db[objunion->descriptor.player].flags2 & (grp->object_flags2 & ~(grp->object_mask2))) == (grp->object_flags2 & ~(grp->object_mask2)))) 
                               if((grp->condition == 205) || (grp->condition == 206) || (((grp->object_who == NOTHING) || (objunion->descriptor.player == grp->object_who)) && ((grp->condition != 200) || (grp->object_type == NOTHING) || (objunion->descriptor.channel == grp->object_type))))
                                  switch(grp->condition % 100) {
                                         case 1:

                                              /* ---->  Extra conditions for 'lwho' list  <---- */
                                              if((db[db[objunion->descriptor.player].location].owner == db[db[grp->player].location].owner) && (get_areaname_loc(db[objunion->descriptor.player].location) == grp->object_type)) return(1);
                                              break;
                                         case 2:

                                              /* ---->  Extra conditions for 'last commands' list  <---- */
                                              if((grp->player == objunion->descriptor.player) || (Validchar(objunion->descriptor.player) && (grp->player == Controller(objunion->descriptor.player))) || (Root(Owner(grp->player)) || (level_app(db[objunion->descriptor.player].owner) < level_app(db[grp->player].owner)))) return(1);
                                              break;
                                         case 3:
 
                                              /* ---->  Extra conditions for channel list ('chat list')  <---- */
                                              if((objunion->descriptor.flags & CONNECTED) && (objunion->descriptor.channel >= 0) && (db[objunion->descriptor.player].flags2 & CHAT_OPERATOR)) return(1);
                                              break;
                                         case 4:

                                              /* ---->  Extra conditions for 'fwho'/'fwhere'  <---- */
                                              if((friend(grp->player,objunion->descriptor.player) || friend(objunion->descriptor.player,grp->player)) && !friendflags_set(grp->player,objunion->descriptor.player,NOTHING,FRIEND_EXCLUDE)) return(1);
                                              break;
                                         case 5:

                                              /* ---->  Extra conditions for '@with'  <---- */
                                              return(match_fields(objunion->descriptor.player,grp->object_who,grp->object_name));
                                         case 6:

                                              /* ---->  Extra conditions for 'where *<NAME>' list  <---- */
                                              if((db[objunion->descriptor.player].location == db[grp->object_who].location) && (objunion->descriptor.player != grp->object_who) && (!Secret(objunion->descriptor.player) || can_write_to(grp->player,objunion->descriptor.player,1) || can_write_to(grp->player,db[objunion->descriptor.player].location,1))) return(1);
                                              break;
                                         case 7:

                                              /* ---->  Extra conditions for 'session' list  <---- */
                                              if(objunion->descriptor.comment) return(1);
                                              break;
                                         case 8:

                                              /* ---->  Extra conditions for 'welcome' list  <---- */
                                              return((objunion->descriptor.flags & WELCOME) != 0);
                                              break;
                                         case 9:

                                              /* ---->  Extra conditions for 'assist' list  <---- */
                                              return((objunion->descriptor.flags & ASSIST) != 0);
                                              break;
                                     /*  case 10:  ---->  Used by 'who', when not connected  <----
                                              break;  */
                                         case 11:

                                              /* ---->  Extra conditions for 'admin' list (List of currently connected Admin.)  <---- */
                                              if(!Validchar(objunion->descriptor.player)) return(0);
                                              if(Being(objunion->descriptor.player)) return(0);
                                              return(Level4(objunion->descriptor.player) || Experienced(objunion->descriptor.player) || Assistant(objunion->descriptor.player));
                                         default:
                                              return(1);
				  }
                break;
           case 3:

                /* ---->  (Entire DB)  '@list' (1)/'@find' (2) conditions  <---- */
                if(!grp->object_mask || ((db[object].flags & grp->object_mask) == (grp->object_flags & grp->object_mask)))
                   if(!((grp->object_flags & ~(grp->object_mask)) & PRIV_FLAGS) || (db[object].flags & ((grp->object_flags & ~(grp->object_mask)) & PRIV_FLAGS)))
                      if((!((grp->object_flags & ~(grp->object_mask)) & ~PRIV_FLAGS) || ((db[object].flags & ((grp->object_flags & ~(grp->object_mask)) & ~PRIV_FLAGS)) == ((grp->object_flags & ~(grp->object_mask)) & ~PRIV_FLAGS))) && !(db[object].flags & grp->object_exc))
                         if(!grp->object_mask2 || ((db[object].flags2 & grp->object_mask2) == (grp->object_flags2 & grp->object_mask2)))
                            if(!(grp->object_flags2 & ~(grp->object_mask2)) || ((db[object].flags2 & (grp->object_flags2 & ~(grp->object_mask2))) == (grp->object_flags2 & ~(grp->object_mask2))))
                               if((db[object].created >= grp->clower) && (db[object].created <= grp->cupper))
                                  if((db[object].lastused >= grp->llower) && (db[object].lastused <= grp->lupper)) {

                                     /* ---->  First check character can_read_from() object, etc.  <---- */
                                     switch(grp->condition % 100) {
                                            case 1:

                                                 /* ---->  '@list'  <---- */
                                                 if(grp->object_who != NOTHING) {
                                                    if((object == grp->object_who) || !contains(object,grp->object_who)) return(0);
	   	                                    if(!can_read_from(grp->player,object)) return(0);
				                 } else if((grp->player != NOTHING) && !((db[grp->player].owner == db[object].owner) || ((Typeof(object) == TYPE_CHARACTER) && (Controller(object) == db[grp->player].owner)))) return(0);
                                                 if(!match_object_type(object,0,grp->object_type) || ((grp->object_type & SEARCH_FLOATING) && !(grp->object_type & SEARCH_ANYTHING) && (db[object].location != NOTHING))) return(0);
                                                 if((grp->object_type & SEARCH_JUNKED) && !(grp->object_type & SEARCH_ANYTHING) && !RoomZero(Location(object))) return(0);
                                                 if((grp->object_type & SEARCH_BANNED) && !(grp->object_type & SEARCH_ANYTHING) && ((Typeof(object) != TYPE_CHARACTER) || !db[object].data->player.bantime)) return(0);
                                                 if((grp->object_type & SEARCH_GLOBAL) && !(grp->object_type & SEARCH_ANYTHING) && !((Typeof(object) == TYPE_COMMAND) && (effective_location(object) == GLOBAL_COMMANDS))) return(0);
                                                 break;
                                            case 2:

                                                 /* ---->  '@find'  <---- */
                                                 if(!match_object_type(object,0,grp->object_type) || ((grp->object_type & SEARCH_FLOATING) && !(grp->object_type & SEARCH_ANYTHING) && (db[object].location != NOTHING))) return(0);
                                                 if((grp->object_type & SEARCH_JUNKED) && !(grp->object_type & SEARCH_ANYTHING) && !RoomZero(Location(object))) return(0);
                                                 if((grp->object_type & SEARCH_BANNED) && !(grp->object_type & SEARCH_ANYTHING) && ((Typeof(object) != TYPE_CHARACTER) || !db[object].data->player.bantime)) return(0);
                                                 if((grp->object_type & SEARCH_GLOBAL) && !(grp->object_type & SEARCH_ANYTHING) && !((Typeof(object) == TYPE_COMMAND) && (effective_location(object) == GLOBAL_COMMANDS))) return(0);
                                                 break;
                                            case 3:
                                            case 4:
                                                 if(grp->object_name && !instring(grp->object_name,db[object].name)) return(0);
                                                 break;
                                            case 5:

                                                 /* ---->  'adminlist' (Including retired administrators)  <---- */
                                                 return(((Typeof(object) == TYPE_CHARACTER) && !Being(object)));
                                            case 6:

                                                 /* ---->  'profile list'  <---- */
                                                 if((Typeof(object) != TYPE_CHARACTER) || ((grp->object_who != NOTHING) && (object != grp->object_who))) return(0);
                                                 return(hasprofile(db[object].data->player.profile));
                                                 break;
                                            default:
		                                 if(!can_read_from(grp->player,object)) return(0);
                                                 if(!match_object_type(object,0,grp->object_type) || ((grp->object_type & SEARCH_FLOATING) && !(grp->object_type & SEARCH_ANYTHING) && (db[object].location != NOTHING))) return(0);
                                                 break;
				     }

                                     /* ---->  Now check given fields match wildcard spec. (@find)  <---- */
                                     switch(grp->condition % 100) {
                                            case 2:
                                            case 5:
                                                 return(match_fields(object,grp->object_who,grp->object_name));
                                                 break;
                                            case 3:

                                                 /* ---->  Users on character's friends list ('flist')  <---- */
                                                 if((flags = friend_flags(grp->player,object)))
                                                    if(((flags|0x80000000) & grp->object_who) && !(flags & grp->object_type))
                                                       return(1);
                                                 break;
                                            case 4:

                                                 /* ---->  Users who have character on their friends list ('fothers')  <---- */
                                                 if((flags = friend_flags(object,grp->player)))
                                                    if(((flags|0x80000000) & grp->object_who) && !(flags & grp->object_type))
                                                       return(1);
                                                 break;
                                            default:
	   		                         return(1);
				     }
				  }
                break;
           case 4:

                /* ---->  Exits that lead to object GRP->OBJECT_WHO (Used by '@from'/'@entrances')  <---- */
                if((Typeof(object) == TYPE_EXIT) && (db[object].destination == grp->object_who)) return(1);
                break;
           case 5:

                /* ---->  Union grouping/range conditions  <---- */
                switch(grp->condition % 100) {
                       case 1:

                            /* ---->  (Array element)  Element contents matching  <---- */
                            if(!grp->object_name) return(1);
                            if((grp->object_who & (SEARCH_NAME|SEARCH_DESC)) && match_wildcard(grp->object_name,decompress(objunion->element.text))) return(1);
                            if((grp->object_who & SEARCH_INDEX) && match_wildcard(grp->object_name,objunion->element.index)) return(1);
                            break;
                       case 2:

                            /* ---->  (Array element)  Element index matching  <---- */
                            if(!grp->object_name) return(1);
                            if((grp->object_who & (SEARCH_NAME|SEARCH_INDEX)) && match_wildcard(grp->object_name,objunion->element.index)) return(1);
                            if((grp->object_who & SEARCH_DESC) && match_wildcard(grp->object_name,decompress(objunion->element.text))) return(1);
                            break;
                       case 3:

                            /* ---->  (Internet site)  Registered Internet sites matching specified address/mask/Internet site flag  <---- */
                            if(((objunion->site.flags|0x80000000) & grp->object_flags2) && !(objunion->site.flags & grp->object_mask2)) {
                               if(grp->object_name) return(match_wildcard(grp->object_name,decompress(objunion->site.description)));
                                  else return((objunion->site.addr & grp->object_mask) == grp->object_flags);
			    }
                            break;
                       case 4:

                            /* ---->  (Friends/enemies list)  Entries in friends/enemies list (Without or with the ENEMY friend flag)  <---- */
                            if(!((grp->object_flags) ? !(objunion->friend.flags & FRIEND_ENEMY):(objunion->friend.flags & FRIEND_ENEMY))) return(0);
                            return(match_fields(objunion->friend.friend,grp->object_who,grp->object_name));
                            break;
                       case 5:

                            /* ---->  ('@request list')  List requests  <---- */
                            if(Validchar(grp->object_who) && (objunion->request.user != grp->object_who)) return(0);
                            return(Blank(grp->object_name) || (objunion->request.email && instring(grp->object_name,decompress(objunion->request.email))));
                            break;
                       case 6:

                            /* ---->  ('@undestroy' list)  Objects owned by OBJECT_WHO with name matching OBJECT_NAME.  <---- */
                            if(!Validchar(grp->object_who)) {
                               if(!Level4(grp->player) && (objunion->destroy.obj.owner != grp->player) && !can_write_to(grp->player,objunion->destroy.obj.owner,0)) return(0);
			    } else if(objunion->destroy.obj.owner != grp->object_who) return(0);
                            if(!match_object_type(NOTHING,(objunion->destroy.obj.type),grp->object_type)) return(0);
                            return(Blank(grp->object_name) || instring(grp->object_name,objunion->destroy.obj.name));
                            break;
                       case 7:

                            /* ---->  ('bbs messages')  List messages only (No replies to messages)  <---- */
                            return(!(objunion->message.flags & MESSAGE_REPLY));
                            break;
                       case 8:

                            /* ---->  ('bbs replies')  List replies to message only (I.e:  All messages in topic with same ID number)  <---- */
                            return(objunion->message.id == grp->object_who);
                            break;
                       case 9:

                            /* ---->  ('@pending')  List pending events  <---- */
                            if(!Valid(objunion->event.object)) return(0);
                            if(!can_read_from(grp->player,objunion->event.object) && !Level4(Owner(grp->player))) return(0);
                            if(!match_object_type(objunion->event.object,0,grp->object_type) || ((grp->object_type & SEARCH_FLOATING) && !(grp->object_type & SEARCH_ANYTHING) && (db[objunion->event.object].location != NOTHING))) return(0);
                            return(Blank(grp->object_name) || instring(grp->object_name,db[objunion->event.object].name));
                            break;
                       case 10:

                            /* ---->  ('@banish list')  List banished names  <----- */
                            if(Blank(objunion->banish.name)) return(0);
                            return(Blank(grp->object_name) || instring(grp->object_name,objunion->banish.name));
                            break;
                       case 11:

                            /* ---->  ('-list')  List available channels  <---- */
                            return(1);
                            break;
                       case 12:

                            /* ---->  ('aliases')  List character's aliases  <---- */
                            if(1) {
                               struct alias_data *ptr;
                               for(ptr = db[grp->player].data->player.aliases; ptr && !((ptr->id == objunion->alias.id) || (ptr == &(objunion->alias))); ptr = (ptr->next == db[grp->player].data->player.aliases) ? NULL:ptr->next);
                               return(ptr && (ptr == &(objunion->alias)));
			    }
                            break;
                       case 13:

                            /* ---->  ('[bbs] readers')  List readers of message  <---- */
                            if(!Validchar(objunion->reader.reader)) return(0);
                            if(!(objunion->reader.flags & READER_READ)) return(0);
                            if(objunion->reader.flags & READER_IGNORE)  return(0);
                            if(objunion->reader.reader == grp->object_who) return(0);
                            return(1);
                            break;
                       case 14:

                            /* ---->  ('author NAME')  List of modules worked on by author  <---- */
                            return(objunion->module.module != 0);
                            break;
                       case 15:

                            /* ---->  ('module NAME')  List of module authors  <---- */
                            return(objunion->author.author != 0);
                            break;
                       default:
                            return(1);
		}
                break;
           default:

                /* ---->  No condition(s)  <---- */
                return(1);
    }
    return(0);
}

/* ---->  Set conditions for each item processed (Primary and secondary flags based)  <---- */
void set_conditions_ps(dbref player,int flags,int mask,int flags2,int mask2,int exc,int object_type,int object_who,const char *wildcard,int condition)
{
     grp->object_flags2 = flags2;
     grp->object_mask2  = mask2;
     grp->object_flags  = flags;
     grp->object_mask   = mask;
     grp->object_type   = object_type;
     grp->object_name   = wildcard;
     grp->object_who    = object_who;
     grp->object_exc    = exc;
     grp->condition     = condition;
     grp->player        = player;
}

/* ---->  Set conditions for each item processed  <---- */
void set_conditions(dbref player,int object_flags,int object_mask,int object_type,int object_who,const char *object_name,int condition)
{
     grp->object_flags2 = 0;
     grp->object_mask2  = 0;
     grp->object_flags  = object_flags;
     grp->object_mask   = object_mask;
     grp->object_type   = object_type;
     grp->object_name   = object_name;
     grp->object_who    = object_who;
     grp->object_exc    = 0;
     grp->condition     = condition;
     grp->player        = player;
}

/* ---->  Sanitise range from/to and group  <---- */
void sanitise_grouprange(void)
{
     /* ---->  Sort out group number  <---- */
     if(grp->groupno == FIRST) grp->groupno = 1;
        else if(grp->groupno == UNSET) grp->groupno = ALL;

     /* ---->  Sort out group size  <---- */
     switch(grp->groupsize) {
            case FIRST:
            case 1:
                 grp->groupsize = 2;
                 break;
            case ALL:
                 grp->groupno = ALL;
                 break;
            case LAST:
            case DEFAULT:
                 if(Validchar(grp->player)) grp->groupsize = (db[grp->player].data->player.scrheight / 2);
                    else grp->groupsize = (STANDARD_CHARACTER_SCRHEIGHT / 2);
     }

     /* ---->  Sort out range from  <---- */
     if((grp->rfrom == ALL) || (grp->rfrom == FIRST) || (grp->rfrom == DEFAULT)) grp->rfrom = 1;

     /* ---->  Sort out range to  <---- */
     switch(grp->rto) {
            case FIRST:
                 grp->rto = 1;
                 break;
            case ALL:
            case DEFAULT:
                 grp->rto = LAST;
     }
}

/* ---->  Initialise before grouping/range operation on linked list of DB objects  <---- */
void db_initgrouprange(dbref object,dbref currentobj)
{
    dbref startptr = NOTHING;
    dbref startobj = NOTHING;
    int   position = 1;
    int   distance = 0;
    int   defdist  = 1;
    int   groupno  = 1;
    dbref defptr   = NOTHING;
    dbref defobj   = NOTHING;
    dbref grpptr   = NOTHING;
    dbref grpobj   = NOTHING;
    int   defbef   = 0;
    int   grpbef   = 0;
    dbref cobj     = NOTHING;

    /* ---->  Init. misc. stuff  <---- */
    grp->totalitems = 0;
    grp->groupitems = 0;
    grp->rangeitems = 0;
    grp->nogroups   = 0;
    grp->distance   = 0;
    grp->nobject    = NOTHING;
    grp->before     = 0;
    grp->cobj       = cobj = currentobj;
    grp->list       = WhichList(object);
    sanitise_grouprange();

    /* ---->  Objects before range  <---- */
    while(Valid(object) && !defbef) {
          if(condition_met(object,NULL)) {
	     if((grp->rfrom == LAST) || (position <= grp->rfrom)) {
                if((grp->rfrom != LAST) && (position == grp->rfrom)) {
                   startptr = object;
                   startobj = cobj;
                   defbef   = 1;
	        } else if(grp->rfrom == LAST) {
                   startptr = object;
                   startobj = cobj;
		}
                grp->totalitems++;
                grp->before++;
                position++;
                getnext(object,grp->list,cobj);
	     } else defbef = 1;
	  } else getnext(object,grp->list,cobj);
    }
    if(startptr != NOTHING) {
       grp->totalitems--;
       grp->before--;
       position--;
    }
    object = startptr;
    cobj   = startobj;
    defbef = 0;

    /* ---->  Objects within range  <---- */
    if(Valid(object)) {
       while(Valid(object) && ((grp->rto == LAST) || (position <= grp->rto))) {
             grp->totalitems++;
             if(grp->groupno == ALL) {

                /* ---->  No group op.  -  all objects within range  <---- */
                distance++;

	     } else {

                /* ---->  Group op.  -  all objects within range and specified group  <---- */
                /* ---->  Update DEFPTR  <---- */
	        if(Valid(defptr)) {
                   if(defdist >= grp->groupsize) {
                      do {
                         getnext(defptr,grp->list,defobj);
	              } while(Valid(defptr) && !condition_met(defptr,NULL));
                      defbef++;
		   } else defdist++;
		} else {
                   defptr = object;
                   defobj = cobj;
		}

                /* ---->  Update GRPPTR  <---- */
                if(grpptr == NOTHING) {
                   grpptr = object;
                   grpobj = cobj;
		}
                if(distance == (grp->groupsize - 1)) {
                   if(groupno == grp->groupno) {
                      grp->groupno    = groupno;
                      grp->distance   = distance + 1;
                      grp->groupitems = distance + 1;
                      grp->nobject    = grpptr;
                      grp->cobj       = grpobj;
                      grpbef          = grp->rangeitems - distance;
		   }
                   grpptr   = object;
                   grpobj   = cobj;
                   distance = 1;
                   groupno++;
		} else distance++;
                grp->rangeitems++;
	     }

             /* ---->  Find next object  <---- */
             do {
                getnext(object,grp->list,cobj);
	     } while(Valid(object) && !condition_met(object,NULL));
             position++;
       }

       /* ---->  Objects after range  <---- */
       while(Valid(object)) {
             if(condition_met(object,NULL)) grp->totalitems++;
             getnext(object,grp->list,cobj);
       }

       if(grp->groupno == ALL) {
          grp->rangeitems = grp->groupitems = grp->distance = distance;
          grp->nobject    = startptr;
          grp->cobj       = startobj;
       } else {
          grp->nogroups = groupno;
          if(grp->groupno == DEFAULT) {
             grp->groupno     = groupno;
             grp->distance    = defdist;
             grp->groupitems  = defdist;
             grp->before     += defbef;
             grp->nobject     = defptr;
             grp->cobj        = defobj;
	  } else if(grp->nobject == NOTHING) {
             grp->before     += grp->rangeitems - distance;
             grp->distance    = distance;
             grp->groupitems  = distance;
             grp->groupno     = groupno;
             grp->nobject     = grpptr;
             grp->cobj        = grpobj;
	  } else grp->before += grpbef;
       }
    }
}

/* ---->  Initialise before grouping/range operation on entire DB  <---- */
void entiredb_initgrouprange()
{
    dbref startptr = NOTHING;
    dbref object   = 0;
    dbref defptr   = NOTHING;
    dbref grpptr   = NOTHING;
    int   position = 1;
    int   distance = 0;
    int   defdist  = 1;
    int   groupno  = 1;
    int   defbef   = 0;
    int   grpbef   = 0;

    /* ---->  Init. misc. stuff  <---- */
    grp->totalitems = 0;
    grp->groupitems = 0;
    grp->rangeitems = 0;
    grp->nogroups   = 0;
    grp->distance   = 0;
    grp->nobject    = NOTHING;
    grp->before     = 0;
    sanitise_grouprange();

    /* ---->  Objects before range  <---- */
    while(Valid(object) && !defbef) {
          if(condition_met(object,NULL)) {
	     if((grp->rfrom == LAST) || (position <= grp->rfrom)) {
                if((grp->rfrom != LAST) && (position == grp->rfrom)) {
                   startptr = object;
                   defbef   = 1;
		} else if(grp->rfrom == LAST) startptr = object;
                grp->totalitems++;
                grp->before++;
                position++;
                object++;
	     } else defbef = 1;
	  } else object++;
    }
    if(startptr != NOTHING) {
       grp->totalitems--;
       grp->before--;
       position--;
    }
    object = startptr;
    defbef = 0;

    /* ---->  Objects within range  <---- */
    if(Valid(object)) {

       /* ---->  No group op.  -  all objects within range  <---- */
       while(Valid(object) && ((grp->rto == LAST) || (position <= grp->rto))) {

             grp->totalitems++;
             if(grp->groupno == ALL) {

                /* ---->  No group op.  -  all objects within range  <---- */
                distance++;

	     } else {

                /* ---->  Group op.  -  all objects within range and specified group  <---- */
                /* ---->  Update DEFPTR  <---- */
	        if(Valid(defptr)) {
                   if(defdist >= grp->groupsize) {
                      do {
                         defptr++;
	              } while(Valid(defptr) && !condition_met(defptr,NULL));
                      defbef++;
		   } else defdist++;
		} else defptr = object;

                /* ---->  Update GRPPTR  <---- */
                if(grpptr == NOTHING) grpptr = object;
                if(distance == (grp->groupsize - 1)) {
                   if(groupno == grp->groupno) {
                      grp->groupno    = groupno;
                      grp->distance   = distance + 1;
                      grp->groupitems = distance + 1;
                      grp->nobject    = grpptr;
                      grpbef          = grp->rangeitems - distance;
		   }
                   grpptr   = object;
                   distance = 1;
                   groupno++;
		} else distance++;
                grp->rangeitems++;
	     }

             /* ---->  Find next object  <---- */
             do {
                object++;
	     } while(Valid(object) && !condition_met(object,NULL));
             position++;
       }

       /* ---->  Objects after range  <---- */
       while(Valid(object)) {
             if(condition_met(object,NULL)) grp->totalitems++;
             object++;
       }

       if(grp->groupno == ALL) {
          grp->rangeitems = grp->groupitems = grp->distance = distance;
          grp->nobject    = startptr;
       } else {
          grp->nogroups = groupno;
          if(grp->groupno == DEFAULT) {
             grp->groupno     = groupno;
             grp->distance    = defdist;
             grp->groupitems  = defdist;
             grp->before     += defbef;
             grp->nobject     = defptr;
	  } else if(grp->nobject == NOTHING) {
             grp->before     += grp->rangeitems - distance;
             grp->distance    = distance;
             grp->groupitems  = distance;
             grp->groupno     = groupno;
             grp->nobject     = grpptr;
	  } else grp->before += grpbef;
       }
    }
}

/* ---->  Initialise before grouping/range operation on union linked list  <---- */
void union_initgrouprange(union group_data *object)
{
    union group_data *startptr = NULL;
    union group_data *defptr   = NULL;
    union group_data *grpptr   = NULL;
    int   position             = 1;   
    int   distance             = 0;
    int   defdist              = 1;
    int   groupno              = 1;
    int   defbef               = 0;
    int   grpbef               = 0; 

    /* ---->  Init. misc. stuff  <---- */
    grp->totalitems = 0;
    grp->groupitems = 0;
    grp->rangeitems = 0;
    grp->nogroups   = 0;
    grp->distance   = 0;
    grp->nunion     = NULL;
    grp->before     = 0;
    sanitise_grouprange();

    /* ---->  Objects before range  <---- */
    while(object && !defbef) {
          if(condition_met(NOTHING,object)) {
  	     if((grp->rfrom == LAST) || (position <= grp->rfrom)) {
                if((grp->rfrom != LAST) && (position == grp->rfrom)) {
                   startptr = object;
                   defbef   = 1;
		} else if(grp->rfrom == LAST) startptr = object;
                grp->totalitems++;
                grp->before++;
                position++;
                object = object->data.next;
	     } else defbef = 1;
	  } else object = object->data.next;
    }
    if(startptr) {
       grp->totalitems--;
       grp->before--;
       position--;
    }
    object = startptr;
    defbef = 0;

    /* ---->  Objects within range  <---- */
    if(object) {
       while(object && ((grp->rto == LAST) || (position <= grp->rto))) {
             grp->totalitems++;
             if(grp->groupno == ALL) {

                /* ---->  No group op.  -  all objects within range  <---- */
                distance++;
	     } else {

                /* ---->  Group op.  -  all objects within range and specified group  <---- */
                /* ---->  Update DEFPTR  <---- */
	        if(defptr) {
                   if(defdist >= grp->groupsize) {
                      do {
                         defptr = defptr->data.next;
		      } while(defptr && !condition_met(NOTHING,defptr));
                      defbef++;
		   } else defdist++;
		} else defptr = object;

                /* ---->  Update GRPPTR  <---- */
                if(!grpptr) grpptr = object;
                if(distance == (grp->groupsize - 1)) {
                   if(groupno == grp->groupno) {
                      grp->groupno    = groupno;
                      grp->distance   = distance + 1;
                      grp->groupitems = distance + 1;
                      grp->nunion     = grpptr;
                      grpbef          = grp->rangeitems - distance;
		   }
                   grpptr   = object;
                   distance = 1;
                   groupno++;
		} else distance++;
                grp->rangeitems++;
	     }
             do {
                object = object->data.next;
	     } while(object && !condition_met(NOTHING,object));
             position++;
       }

       /* ---->  Objects after range  <---- */
       while(object) {
             if(condition_met(NOTHING,object)) grp->totalitems++;
             object = object->data.next;
       }

       if(grp->groupno == ALL) {
          grp->rangeitems = grp->groupitems = grp->distance = distance;
          grp->nunion     = startptr;
       } else {
          grp->nogroups       = groupno;
          if(grp->groupno == DEFAULT) {
             grp->groupno     = groupno;
             grp->distance    = defdist;
             grp->groupitems  = defdist;
             grp->before     += defbef;
             grp->nunion      = defptr;
	  } else if(!grp->nunion) {
             grp->before     += grp->rangeitems - distance;
             grp->distance    = distance;
             grp->groupitems  = distance;
             grp->groupno     = groupno;
             grp->nunion      = grpptr;
	  } else grp->before += grpbef;
       }
    }
}

/* ---->  Returns 1 while linked list DB object is within group/range  <---- */
int db_grouprange()
{
    if(Valid(grp->nobject) && (grp->distance > 0)) {
       if(Typeof(grp->nobject) == TYPE_FREE) return(0);
       grp->cobject = grp->nobject;
       getnext(grp->nobject,grp->list,grp->cobj);
       grp->distance--; 
       if(grp->distance > 0) while(Valid(grp->nobject) && !condition_met(grp->nobject,NULL))
          getnext(grp->nobject,grp->list,grp->cobj);
    } else return(0);
    return(1);
}

/* ---->  Returns 1 while DB object (In entire DB) is within group/range  <---- */
int entiredb_grouprange()
{
    if(Valid(grp->nobject) && (grp->distance > 0)) {
       if(Typeof(grp->nobject) == TYPE_FREE) return(0);
       grp->cobject = grp->nobject;
       grp->nobject++;
       grp->distance--; 
       if(grp->distance > 0) while(Valid(grp->nobject) && !condition_met(grp->nobject,NULL))
          grp->nobject++;
    } else return(0);
    return(1);
}

/* ---->  Returns 1 while union linked list item is within group/range  <---- */
int union_grouprange()
{
    if(grp->nunion && (grp->distance > 0)) {
       grp->cunion = grp->nunion;
       grp->nunion = grp->nunion->data.next;
       grp->distance--;
       if(grp->distance > 0) while(grp->nunion && !condition_met(NOTHING,grp->nunion))
          grp->nunion = grp->nunion->data.next;
    } else return(0);
    return(1);
}

/* ---->  Display total items within group/range after '@with' with grouping/range operators  <---- */
void group_query_display(CONTEXT)
{
     setreturn(listed_items(querybuf,1),COMMAND_SUCC);
}

/* ---->  Return group number parameter from given grouping/range operator  <---- */
void group_query_groupno(CONTEXT)
{
     const char *ptr = params;
     params = (char *) parse_grouprange(player,params,ALL,1);
     setreturn((params != ptr) ? grouprange_value(querybuf,grp->groupno):"ALL",COMMAND_SUCC);
}

/* ---->  Returns 1 if given string is grouping/range op.  -  Otherwise, 0  <---- */
void group_query_grouprange(CONTEXT)
{
     const char *ptr = params;
     params = (char *) parse_grouprange(player,params,ALL,1);
     setreturn(((params != ptr) && Blank(params)) ? OK:ERROR,COMMAND_SUCC);
}

/* ---->  Return group size parameter from given grouping/range operator/character  <---- */
void group_query_groupsize(CONTEXT)
{
     const char *ptr = params;
     params = (char *) parse_grouprange(player,params,ALL,1);
     if(params == ptr) {
        if(!Blank(params)) {
           dbref character = query_find_character(player,params,1);
           sprintf(querybuf,"%d",(!Validchar(character)) ? ((STANDARD_CHARACTER_SCRHEIGHT - 4) / 2):((db[character].data->player.scrheight - 4) / 2));
	} else sprintf(querybuf,"%d",((db[player].data->player.scrheight - 4) / 2));
        setreturn(querybuf,COMMAND_SUCC);
     } else setreturn(grouprange_value(querybuf,grp->groupsize),COMMAND_SUCC);
}

/* ---->  Return range from parameter from given grouping/range operator  <---- */
void group_query_rangefrom(CONTEXT)
{
     const char *ptr = params;
     params = (char *) parse_grouprange(player,params,ALL,1);
     setreturn((params != ptr) ? grouprange_value(querybuf,grp->rfrom):"FIRST",COMMAND_SUCC);
}

/* ---->  Return range to parameter from given grouping/range operator  <---- */
void group_query_rangeto(CONTEXT)
{
     const char *ptr = params;
     params = (char *) parse_grouprange(player,params,ALL,1);
     setreturn((params != ptr) ? grouprange_value(querybuf,grp->rto):"LAST",COMMAND_SUCC);
}

