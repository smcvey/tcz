/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| LIST.C  -  Implements parsing and display of user lists.  These are used by |
|            a variety of commands, such as 'page', 'tell', etc. to allow     |
|            messages to be sent to groups of users.                          |
|                                                                             |
| NOTE:  Unfinished BETA code, not currently in use.                          |
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
| Modules originally designed and written by:  J.P.Boggis 31/08/1998.         |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: lists.c,v 1.1.1.1 2004/12/02 17:41:42 jpboggis Exp $

NEEDS TO SUPPORT:

  page NAME|GROUP message
  page CHARACTER NAME message
  page NAME,NAME,NAME = message

  USERLIST_OTHERS

*/

#include <stdlib.h>
#include <string.h>

#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "object_types.h"
#include "friend_flags.h"
#include "flagset.h"
#include "search.h"


/* ---->  Add entry to user list  <---- */
/*  SOURCE      = User parsing list                                       */
/*  USER        = User being added to list                                */
/*  USERFLAGS   = User flags to set on user                               */
/*  CUSTOMFLAGS = Custom flags to set on user  (USERLIST_CUSTOM_INCLUDE)  */
/*  INTERNAL    = Internal entry                                     (0)  */
/*  EXCLUDE     = Exclude user from list?                            (0)  */
/*  SELF        = Exclude self                                       (1)  */
/*  GROUP       = Friend group ID                                    (0)  */
/*  NAME        = Name of group                                   (NULL)  */
/*  START       = Pointer to user list structure                          */
/*      Returns:  Successful or not                                       */
unsigned char lists_add(dbref source,dbref user,int userflags,int customflags,unsigned char internal,unsigned char exclude,unsigned char self,unsigned char group,const char *name,struct userlist_data **start)
{
	 struct userlist_data *last,*ptr,*new;
	 int    adduserflags = 0;

	    /* ---->  Internal entry (Groups)  <---- */
	 if(internal) {

	    /* ---->  Identical internal entry exists?  <---- */
	    for(ptr = *start, last = NULL; ptr && (!(ptr->userflags & USERLIST_INTERNAL) || !(((ptr->userflags & USERLIST_INTERNAL_MASK) == (userflags & USERLIST_INTERNAL_MASK)) && (ptr->user == user) && (ptr->group == group))); last = ptr, ptr = ptr->next);
	    if(ptr) {
	       if(exclude) ptr->userflags |= USERLIST_EXCLUDE;
	       ptr->userflags |= userflags;
	       return(0);
	    }
	    adduserflags = USERLIST_INTERNAL;

	    /* ---->  User entry  <---- */
	 } else {

	    /* ---->  Additional user flags  <---- */
	    if(!Connected(user)) adduserflags |= USERLIST_DISCONNECTED;
	    if(source == user) {
	       if(!self) return(0);
	       adduserflags |= USERLIST_SELF;
	    }
	    if(exclude) adduserflags |= USERLIST_EXCLUDE;

	    /* ---->  User already in list?  <---- */
	    for(ptr = *start, last = NULL; ptr && ((ptr->userflags & USERLIST_INTERNAL) || (ptr->user != user)); last = ptr, ptr = ptr->next);
	    if(ptr) {
	       ptr->userflags   |= adduserflags;
	       ptr->customflags |= customflags;
	       return(0);
	    }
	 }

	 /* ---->  Create new entry  <---- */
	 MALLOC(new,struct userlist_data);
	 new->next          = NULL;
	 new->user          = user;
	 new->userflags     = (userflags|adduserflags);
	 new->customflags   = (USERLIST_CUSTOM_INCLUDE|customflags);
	 new->group         = group;
	 if(name) new->name = (char *) alloc_string(name);
	    else  new->name = NULL;

	 /* ---->  Add entry to list  <---- */
	 if(last) last->next = new;
	    else *start = new;
	 return(1);
}

/* ---->  Free list of users (Optionally selectively preserving entries)  <--- */
/*  USERLIST  = Pointer to user list structure pointer  */
/*  INCUSER   = User flags to include   (SEARCH_ALL)    */
/*  EXCUSER   = User flags to exclude            (0)    */
/*  INCCUSTOM = Custom flags to include (SEARCH_ALL)    */
/*  EXCCUSTOM = Custom flags to exclude          (0)    */
/*  ALL       = Unconditional free all entries   (1)    */
/*    Returns:  Number of entries remaining in list     */
int lists_free(struct userlist_data **userlist,int incuser,int excuser,int inccustom,int exccustom,unsigned char all)
{
    struct userlist_data *start = NULL,*tail = NULL,*next,*ptr;
    int    remaining = 0;

    for(ptr = *userlist; ptr; ptr = next) {
        next = ptr->next;

        if(all || ((ptr->userflags & incuser) && !(ptr->userflags & excuser) && (ptr->customflags & inccustom) && !(ptr->customflags & exccustom))) {

           /* ---->  Delete entry  <---- */
           FREENULL(ptr->name);
           FREENULL(ptr);
        } else {

           /* ---->  Preserve entry  <---- */
           if(start) {
              tail->next = ptr;
              tail       = ptr;
           } else start = tail = ptr;
           ptr->next = NULL;
           remaining++;
        }
    }
    *userlist = start;
    return(remaining);
}

/* ---->  Parse given list of users  <---- */
/*  PLAYER    = User parsing list                                */
/*  USERLIST  = String containing list of users                  */
/*  ACTION    = Action of user list (E.g:  'page a message to')  */
/*  USERFLAGS = Allowed keywords             (USERLIST_DEFAULT)  */
/*  ERRORS    = Display appropriate error messages          (1)  */
/*    Returns:  Resulting user list structure                    */
struct userlist_data *lists_parse(dbref player,const char *userlist,const char *action,int userflags,unsigned char errors)
{
       struct   descriptor_data *p = getdsc(player);
       unsigned char found,exclude,self,enemy;
       struct   userlist_data *start = NULL;
       int      keywordlen,count,err = 0;
       char     keyword[TEXT_SIZE];
       int      processed = 0;
       char     *ptr;

       self = (userflags & USERLIST_SELF);
       do {
          found = 0, count = 0;

          /* ---->  Get keyword/username  <---- */
          if(userlist) {
             for(; *userlist && (*userlist == ' '); userlist++);
             if(*userlist && (*userlist == '!')) {
                for(userlist++; *userlist && (*userlist == ' '); userlist++);
                exclude = 1;
             } else exclude = 0;
             for(ptr = keyword; *userlist && !((*userlist == ',') || (*userlist == ';')); *ptr++ = *userlist++);
             for(*ptr-- = '\0'; (ptr >= keyword) && (*ptr == ' '); *ptr-- = '\0');
             for(; *userlist && ((*userlist == ',') || (*userlist == ';') || (*userlist == ' ')); userlist++);
	  }

          if(!Blank(keyword)) {
             keywordlen = strlen(keyword);

             /* ---->  Friends or enemies of user  <---- */
             if(!found && (keywordlen >= 5) && ((!(enemy = 0) && (string_prefix("friends",keyword))) || ((enemy = 1) && (string_prefix("enemies",keyword) || string_prefix("enemy",keyword)))))  {
                found = 1, processed++;
                if(exclude || enemy || (userflags & USERLIST_FRIEND)) {
                   if(exclude || !enemy || (userflags & USERLIST_ENEMY)) {
                      int ftype  = (enemy) ? USERLIST_ENEMY:USERLIST_FRIEND;

                      if(lists_add(player,NOTHING,ftype|(userflags & USERLIST_DISCONNECTED),0,1,exclude,self,0,NULL,&start)) {
                         int fflags;

                         if(userflags & !USERLIST_DISCONNECTED) {

                            /* ---->  Connected friends/enemies of user  <---- */
                            struct descriptor_data *ptr;

                            for(ptr = descriptor_list, count = 0; ptr; ptr = ptr->next)
                                if((ptr->flags & CONNECTED) && Validchar(ptr->player)) {
                                   fflags = (friend_flags(player,ptr->player)|(friend_flags(ptr->player,player) & !(FRIEND_EXCLUDE)));

                                   if(!(fflags & FRIEND_EXCLUDE) && ((enemy && (fflags & FRIEND_ENEMY)) || (!enemy && !(fflags & FRIEND_ENEMY))))
                                      lists_add(player,ptr->player,ftype|USERLIST_DISCONNECTED,0,0,exclude,self,0,NULL,&start), count++;
				}
                                                
                            if(!count) {
                               if(errors) output(p,player,0,1,0,ANSI_LGREEN"%s\x05\x02Sorry, no %s of yours are currently connected.",(err) ? "":"\n",(enemy) ? "enemies":"friends"), err++;
                               lists_add(player,NOTHING,ftype|USERLIST_EXCLUDE,0,1,exclude,self,0,NULL,&start);
			    }
			 } else {

                            /* ---->  All users in user's friends list  <---- */
                            struct friend_data *friend;

                            for(friend = db[player].data->player.friends, count = 0; friend; friend = friend->next)
                                if(Validchar(friend->friend) && (friend->friend != player) && !(friend->flags & FRIEND_EXCLUDE) && ((enemy && (friend->flags & FRIEND_ENEMY)) || (!enemy && !(friend->flags & FRIEND_ENEMY))))
                                   lists_add(player,friend->friend,ftype|USERLIST_DISCONNECTED,0,0,exclude,self,0,NULL,&start), count++;

                            if(!count) {
                               if(errors) output(p,player,0,1,0,ANSI_LGREEN"%s\x05\x02Sorry, you have no %s in your friends list.",(err) ? "":"\n",(enemy) ? "enemies":"friends"), err++;
                               lists_add(player,NOTHING,ftype|USERLIST_EXCLUDE,0,1,exclude,self,0,NULL,&start);
                            }
                         }
                      }
                   } else if(errors) output(p,player,0,1,0,ANSI_LGREEN"%s\x05\x02Sorry, you can't %s your%s enemies.",(err) ? "":"\n",action,(userflags & USERLIST_DISCONNECTED) ? "":" connected"), err++;
                } else if(errors) output(p,player,0,1,0,ANSI_LGREEN"%s\x05\x02Sorry, you can't %s your%s friends.",(err) ? "":"\n",action,(userflags & USERLIST_DISCONNECTED) ? "":" connected"), err++;
             }

             /* ---->  Assistants  <---- */
             if(!found && (keywordlen >= 6) && string_prefix("assistants",keyword)) {
                found = 1, processed++;
                if(exclude || (userflags & USERLIST_ASSISTANT)) {
                   if(lists_add(player,NOTHING,USERLIST_ASSISTANT|(userflags & USERLIST_DISCONNECTED),0,1,exclude,self,0,NULL,&start)) {
                      if(userflags & USERLIST_DISCONNECTED) {
                         dbref ptr;

                         for(ptr = 0; ptr < db_top; ptr++)
                             if(Validchar(ptr) && Assistant(ptr))
                                lists_add(player,ptr,USERLIST_ASSISTANT,0,0,exclude,self,0,NULL,&start), count++;
                      } else {
                         struct descriptor_data *ptr;

                         for(ptr = descriptor_list; ptr; ptr = ptr->next)
                             if((ptr->flags & CONNECTED) && Validchar(ptr->player) && Assistant(ptr->player))
                                lists_add(player,ptr->player,USERLIST_ASSISTANT,0,0,exclude,self,0,NULL,&start), count++;
                      }

                      if(!count) {
                         if(errors) output(p,player,0,1,0,ANSI_LGREEN"%s\x05\x02Sorry, there are no Assistants %s.",(err) ? "":"\n",(userflags & USERLIST_DISCONNECTED) ? "in the database":"currently connected"), err++;
                         lists_add(player,NOTHING,USERLIST_ASSISTANT|USERLIST_EXCLUDE,0,1,exclude,self,0,NULL,&start);
                      }
                   }
                } else if(errors) output(p,player,0,1,0,ANSI_LGREEN"%s\x05\x02Sorry, you can't %s all%s Assistants.",(err) ? "":"\n",action,(userflags & USERLIST_DISCONNECTED) ? "":" connected"), err++;
	     }

             /* ---->  Experienced Builders  <---- */
             if(!found && (keywordlen >= 5) && (string_prefix("experiencedbuilders",keyword) || string_prefix("experienced builders",keyword))) {
                found = 1, processed++;
                if(exclude || (userflags & USERLIST_EXPERIENCED)) {
                   if(lists_add(player,NOTHING,USERLIST_EXPERIENCED|(userflags & USERLIST_DISCONNECTED),0,1,exclude,self,0,NULL,&start)) {
                      if(userflags & USERLIST_DISCONNECTED) {
                         dbref ptr;

                         for(ptr = 0; ptr < db_top; ptr++)
                             if(Validchar(ptr) && (Experienced(ptr) && !Retired(ptr)))
                                lists_add(player,ptr,USERLIST_EXPERIENCED,0,0,exclude,self,0,NULL,&start), count++;
                      } else {
                         struct descriptor_data *ptr = descriptor_list;

                         for(; ptr; ptr = ptr->next)
                             if((ptr->flags & CONNECTED) && Validchar(ptr->player) && (Experienced(ptr->player) && !Retired(ptr->player)))
                                lists_add(player,ptr->player,USERLIST_EXPERIENCED,0,0,exclude,self,0,NULL,&start), count++;
                      }

                      if(!count) {
                         if(errors) output(p,player,0,1,0,ANSI_LGREEN"%s\x05\x02Sorry, there are no Experienced Builders %s.",(err) ? "":"\n",(userflags & USERLIST_DISCONNECTED) ? "in the database":"currently connected"), err++;
                         lists_add(player,NOTHING,USERLIST_EXPERIENCED|USERLIST_EXCLUDE,0,1,exclude,self,0,NULL,&start);
                      }
                   }
                } else if(errors) output(p,player,0,1,0,ANSI_LGREEN"%s\x05\x02Sorry, you can't %s all%s Experienced Builders.",(err) ? "":"\n",action,(userflags & USERLIST_DISCONNECTED) ? "":" connected"), err++;
	     }

             /* ---->  Retired Administrators  <---- */
             if(!found && (keywordlen >= 6) && (string_prefix("retiredadministrators",keyword) || string_prefix("retiredadministration",keyword) || string_prefix("retired administrators",keyword) || string_prefix("retired administration",keyword))) {
                found = 1, processed++;
                if(exclude || (userflags & USERLIST_RETIRED)) {
                   if(lists_add(player,NOTHING,USERLIST_RETIRED|(userflags & USERLIST_DISCONNECTED),0,1,exclude,self,0,NULL,&start)) {
                      if(userflags & USERLIST_DISCONNECTED) {
                         dbref ptr;

                         for(ptr = 0; ptr < db_top; ptr++)
                             if(Validchar(ptr) && Retired(ptr))
                                lists_add(player,ptr,USERLIST_RETIRED,0,0,exclude,self,0,NULL,&start), count++;
                      } else {
                         struct descriptor_data *ptr = descriptor_list;

                         for(; ptr; ptr = ptr->next)
                             if((ptr->flags & CONNECTED) && Validchar(ptr->player) && Retired(ptr->player))
                                lists_add(player,ptr->player,USERLIST_RETIRED,0,0,exclude,self,0,NULL,&start), count++;
                      }

                      if(!count) {
                         if(errors) output(p,player,0,1,0,ANSI_LGREEN"%s\x05\x02Sorry, there are no Retired Administrators %s.",(err) ? "":"\n",(userflags & USERLIST_DISCONNECTED) ? "in the database":"currently connected"), err++;
                         lists_add(player,NOTHING,USERLIST_RETIRED|USERLIST_EXCLUDE,0,1,exclude,self,0,NULL,&start);
                      }
                   }
                } else if(errors) output(p,player,0,1,0,ANSI_LGREEN"%s\x05\x02Sorry, you can't %s all%s Retired Administrators.",(err) ? "":"\n",action,(userflags & USERLIST_DISCONNECTED) ? "":" connected"), err++;
	     }

             /* ---->  Administrators  <---- */
             if(!found && (keywordlen >= 5) && (string_prefix("administrators",keyword) || string_prefix("administration",keyword))) {
                found = 1, processed++;
                if(exclude || (userflags & USERLIST_ADMIN)) {
                   if(lists_add(player,NOTHING,USERLIST_ADMIN|(userflags & USERLIST_DISCONNECTED),0,1,exclude,self,0,NULL,&start)) {
                      if(userflags & USERLIST_DISCONNECTED) {
                         dbref ptr;

                         for(ptr = 0; ptr < db_top; ptr++)
                             if(Validchar(ptr) && Level4(ptr))
                                lists_add(player,ptr,USERLIST_ADMIN,0,0,exclude,self,0,NULL,&start), count++;
                      } else {
                         struct descriptor_data *ptr = descriptor_list;

                         for(; ptr; ptr = ptr->next)
                             if((ptr->flags & CONNECTED) && Validchar(ptr->player) && Level4(ptr->player))
                                lists_add(player,ptr->player,USERLIST_ADMIN,0,0,exclude,self,0,NULL,&start), count++;
                      }

                      if(!count) {
                         if(errors) output(p,player,0,1,0,ANSI_LGREEN"%s\x05\x02Sorry, there are no %s Administrators %s.",(err) ? "":"\n",tcz_short_name,(userflags & USERLIST_DISCONNECTED) ? "in the database":"currently connected"), err++;
                         lists_add(player,NOTHING,USERLIST_ADMIN|USERLIST_EXCLUDE,0,1,exclude,self,0,NULL,&start);
                      }
                   }
                } else if(errors) output(p,player,0,1,0,ANSI_LGREEN"%s\x05\x02Sorry, you can't %s all%s %s Administrators.",(err) ? "":"\n",action,(userflags & USERLIST_DISCONNECTED) ? "":" connected",tcz_short_name), err++;
	     }

             /* ---->  All users  <---- */
             if((!found && !strcasecmp("all",keyword))) {
                found = 1, processed++;
                if(userflags & USERLIST_ALL) {
                   if(lists_add(player,NOTHING,USERLIST_ALL|(userflags & USERLIST_DISCONNECTED),0,1,exclude,self,0,NULL,&start)) {
                      if(userflags & USERLIST_DISCONNECTED) {
                         dbref i;
                   
                         for(i = 0; i <= db_top; i++)
                             if(Validchar(i))
                                lists_add(player,i,USERLIST_ALL,0,0,exclude,self,0,NULL,&start), count++;
                      } else {
                         struct descriptor_data *ptr = descriptor_list;

                         for(; ptr; ptr = ptr->next)
                             if((ptr->flags & CONNECTED) && Validchar(ptr->player))
                                lists_add(player,ptr->player,USERLIST_ALL,0,0,exclude,self,0,NULL,&start), count++;
                      }

                      if(!count) {
                         if(errors) output(p,player,0,1,0,ANSI_LGREEN"%s\x05\x02Sorry, no users %s",(err) ? "":"\n",(userflags & USERLIST_DISCONNECTED) ? "exist in the database":"are currently connected"), err++;
                         lists_add(player,NOTHING,USERLIST_ALL|USERLIST_EXCLUDE,0,1,exclude,self,0,NULL,&start);
                      }
                   }
                } else if(errors) output(p,player,0,1,0,ANSI_LGREEN"%s\x05\x02Sorry, you can't %s all %susers.",(err) ? "":"\n",action,(userflags & USERLIST_DISCONNECTED) ? "":" connected"), err++;
	     }

             /* ---->  Friend group  <---- */
/*             if(!found && (group = friends_group_lookup(player,keyword,NOTHING,0,NULL))) {
                found = 1, processed++;
                if(userflags & USERLIST_GROUP) {
                   if(lists_add(player,NOTHING,USERLIST_GROUP|(userflags & USERLIST_DISCONNECTED),0,1,exclude,self,(1 << (group->id - 1)),NULL,&start) {
                      int    gcount = 0,count = 0;
                      struct friends_data *ptr;

                      for(ptr = db[player].data->player.friends; ptr; ptr = ptr->next)
                          if(Validchar(ptr->player) && (ptr->group & (1 << (group->id - 1)))) {
                             if(Connected(ptr->player) || (userflgs & USERLIST_DISCONNECTED))
                                lists_add(player,ptr->player,USERLIST_GROUP,0,0,exclude,self,(1 << (group->id - 1)),group->name,&start), count++;
                             gcount++;
                          }
                             
                      if(!gcount) {
                         if(errors) output(p,player,0,1,0,ANSI_LGREEN"%s\x05\x02Sorry, there are no users remaining in the friend group '"ANSI_LWHITE"%s"ANSI_LGREEN"'.  This group has been removed.",(err) ? "":"\n",group->name), err++;
                         friends_group_delete(player,group->id);
                      } else if(!count)
                         if(errors) output(p,player,0,1,0,ANSI_LGREEN"%s\x05\x02Sorry, no users in the friend group '"ANSI_LWHITE"%s"ANSI_LGREEN"' are connected at the moment.",(err) ? "":"\n",group->name), err++;
                   }
                } else if(errors) output(p,player,0,1,0,ANSI_LGREEN"%s\x05\x02Sorry, you can't %s the friend group '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",(err) ? "":"\n",action,group->name), err++;
             }  */

             /* ---->  User name  <---- */
             if(!found) {
                dbref user;

                processed++;
                if((user = lookup_character(player,keyword,1)) != NOTHING) {
		   if((user != player) || self) {
                      if(Connected(user) || (userflags & USERLIST_DISCONNECTED)) {
                         lists_add(player,user,USERLIST_INDIVIDUAL,0,0,exclude,self,0,NULL,&start);
                      } else if(errors) output(p,player,0,1,0,ANSI_LGREEN"%s\x05\x02Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" isn't connected.",(err) ? "":"\n",Article(user,LOWER,DEFINITE),getcname(NOTHING,user,0,0)), err++;
                   } else if(errors) output(p,player,0,1,0,ANSI_LGREEN"%s\x05\x02Sorry, you can't %s yourself.",(err) ? "":"\n",action), err++;
                } else if(errors) output(p,player,0,1,0,ANSI_LGREEN"%s\x05\x02Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",(err) ? "":"\n",keyword), err++;
	     }
	  }
       } while(!Blank(userlist));

       /* ---->  No user names specified?  <---- */
       if(!processed && errors)
          output(p,player,0,1,0,ANSI_LGREEN"%s\x05\x02Please specify who you would like to %s.",(err) ? "":"\n",action), err++;

       /* ---->  Remove all excluded entries  <---- */
       lists_free(&start,USERLIST_EXCLUDE,0,SEARCH_ALL,0,0);
       if(errors && err) output(p,player,0,1,0,"");
       return(start);
}

/* ---->  Count users in list (Optionally selective)  <---- */
/*  USERLIST  = Pointer to user list structure        */
/*  INCUSER   = User flags to include   (SEARCH_ALL)  */
/*  EXCUSER   = User flags to exclude            (0)  */
/*  INCCUSTOM = Custom flags to include (SEARCH_ALL)  */
/*  EXCCUSTOM = Custom flags to exclude          (0)  */
/*  ALL       = Unconditional free all entries   (1)  */
/*    Returns:  Number of counted entries             */
int lists_count(struct userlist_data *userlist,int incuser,int excuser,int inccustom,int exccustom,unsigned char all)
{
    struct userlist_data *ptr;
    int    counted = 0;

    for(ptr = userlist; ptr; ptr = ptr->next)
        if(all || ((ptr->userflags & incuser) && !(ptr->userflags & excuser) && (ptr->customflags & inccustom) && !(ptr->customflags & exccustom)))
           counted++;
    return(counted);
}

/* ---->  Set/reset user flags on user list (Optionally excluding entries)  <---- */
/*  USERLIST  = Pointer to user list structure        */
/*  SET       = User flags to set                     */
/*  RESET     = User flags to reset                   */
/*  INCUSER   = User flags to include   (SEARCH_ALL)  */
/*  EXCUSER   = User flags to exclude            (0)  */
/*  INCCUSTOM = Custom flags to include (SEARCH_ALL)  */
/*  EXCCUSTOM = Custom flags to exclude          (0)  */
/*  ALL       = Unconditional free all entries   (1)  */
/*    Returns:  Number of modified entries            */
int lists_userflags(struct userlist_data *userlist,int set,int reset,int incuser,int excuser,int inccustom,int exccustom,unsigned char all)
{
    struct userlist_data *ptr;
    int    actioned = 0;

    for(ptr = userlist; ptr; ptr = ptr->next)
        if(all || ((ptr->userflags & incuser) && !(ptr->userflags & excuser) && (ptr->customflags & inccustom) && !(ptr->customflags & exccustom))) {

           /* ---->  Set/reset user flags on entry  <---- */
           ptr->userflags |= set;
           ptr->userflags &= ~(reset);
           actioned++;
        }
    return(actioned);
}

/* ---->  Set/reset custom flags on user list (Optionally excluding entries)  <---- */
/*  USERLIST  = Pointer to user list structure        */
/*  SET       = Custom flags to set                   */
/*  RESET     = Custom flags to reset                 */
/*  INCUSER   = User flags to include   (SEARCH_ALL)  */
/*  EXCUSER   = User flags to exclude            (0)  */
/*  INCCUSTOM = Custom flags to include (SEARCH_ALL)  */
/*  EXCCUSTOM = Custom flags to exclude          (0)  */
/*  ALL       = Unconditional free all entries   (1)  */
/*    Returns:  Number of modified entries            */
int lists_customflags(struct userlist_data *userlist,int set,int reset,int incuser,int excuser,int inccustom,int exccustom,unsigned char all)
{
    struct userlist_data *ptr;
    int    actioned = 0;

    for(ptr = userlist; ptr; ptr = ptr->next)
        if(all || ((ptr->userflags & incuser) && !(ptr->userflags & excuser) && (ptr->customflags & inccustom) && !(ptr->customflags & exccustom))) {

           /* ---->  Set/reset user flags on entry  <---- */
           ptr->customflags |= set;
           ptr->customflags &= ~(reset);
           actioned++;
        }
    return(actioned);
}

/* ---->  Duplicate user list (I.e:  For 'recall' command.)  (Optionally selective)  <---- */
/*  USERLIST = Pointer to user list structure            */
/*  INCUSER   = User flags to include      (SEARCH_ALL)  */
/*  EXCUSER   = User flags to exclude               (0)  */
/*  INCCUSTOM = Custom flags to include    (SEARCH_ALL)  */
/*  EXCCUSTOM = Custom flags to exclude             (0)  */
/*  ALL       = Unconditional free all entries      (1)  */
/*    Returns:  Resulting duplicate user list structure  */
struct userlist_data *lists_duplicate(struct userlist_data *userlist,int incuser,int excuser,int inccustom,int exccustom,unsigned char all)
{
       return(NULL);
}

/* ---->  Construct visual list of users from parsed user list  <---- */
/*  SOURCE    = Source user of list            (E.g:  User sending message)  */
/*  TARGET    = Target user of list          (E.g:  User receiving message)  */
/*  USERLIST  = Parsed list of users                                         */
/*  ANSI1     = Colour used for character names               (ANSI_LWHITE)  */
/*  ANSI2     = Colour used for everything else                (ANSI_LCYAN)  */
/*  BUFFER    = Text buffer for constructed user list (TEXT_SIZE length or greater)  */
/*  INCUSER   = User flags to include                          (SEARCH_ALL)  */
/*  EXCUSER   = User flags to exclude                                   (0)  */
/*  INCCUSTOM = Custom flags to include                        (SEARCH_ALL)  */
/*  EXCCUSTOM = Custom flags to exclude                                 (0)  */
/*  ALL       = Unconditionally include all entries                     (1)  */
/*  SINGLE    = Single user name/group (I.e:  'page NAME message')      (0)  */
/*   Returns:  Constructed and formatted list of users.                      */
const char *lists_construct(dbref source,dbref target,struct userlist_data *userlist,const char *ansi1,const char *ansi2,char *buffer,int incuser,int excuser,int inccustom,int exccustom,unsigned char all,unsigned char single)
{
      unsigned char friends      = 0, enemies     = 0;
      unsigned char assistants   = 0, experienced = 0;
      unsigned char retired      = 0, admin       = 0;
      unsigned char everyone     = 0, groups      = 0;
      unsigned char disconnected = 0, count       = 0;

      unsigned char _continue,finished = 0,first = 1,addand = 0;
      struct   userlist_data *ptr,*ptr2;
      struct   str_ops str_data;
      char     next[TEXT_SIZE];
      int      names;

      *next           = '\0';
      names           = USERLIST_NAMES;
      str_data.dest   = buffer;
      str_data.src    = NULL;
      str_data.length = 0;

      /* ---->  Source/target users and keywords  <---- */
      for(ptr = userlist; ptr; ptr = ptr->next)
          if(!(ptr->userflags & USERLIST_EXCLUDE) && (all || ((ptr->userflags & incuser) && !(ptr->userflags & excuser) && (ptr->customflags & inccustom) && !(ptr->customflags & exccustom)))) {
             if(!(ptr->userflags & USERLIST_INTERNAL) && Validchar(ptr->user)) {

                /* ---->  Source/target users  <---- */
                if((ptr->user == source) || (ptr->user == target)) {
                   if(!Blank(next)) {
                      if(!first) strcat_limits(&str_data,", ");
                      strcat_limits(&str_data,next), first = 0;
                   }
     
                   if(ptr->user == source) {
                      if(source == target) strcpy(next,"yourself");
                         else sprintf(next,"%sself",Objective(ptr->user,0));
                   } else strcpy(next,"you");
                   names--;
                }
             } else {

                /* ---->  Determine usage of standard keywords  <---- */
                if(ptr->userflags & USERLIST_FRIEND)      friends      = 1;
                if(ptr->userflags & USERLIST_ENEMY)       enemies      = 1;
                if(ptr->userflags & USERLIST_ASSISTANT)   assistants   = 1;
                if(ptr->userflags & USERLIST_EXPERIENCED) experienced  = 1;
                if(ptr->userflags & USERLIST_RETIRED)     retired      = 1;
                if(ptr->userflags & USERLIST_ADMIN)       admin        = 1;
                if(ptr->userflags & USERLIST_ALL)         everyone     = 1;
                if(ptr->userflags & USERLIST_GROUP)       groups       = 1;
             }
             if(ptr->userflags & USERLIST_DISCONNECTED)   disconnected = 1;
      }

      /* ---->  Individual users  <---- */
      addand = 0, _continue = 0;
      for(ptr = userlist; ptr; ptr = ((!ptr) ? NULL:ptr->next))
          if((ptr->userflags & USERLIST_INDIVIDUAL) && Validchar(ptr->user) && !(ptr->userflags & (USERLIST_EXCLUDED|USERLIST_INTERNAL)) && (ptr->user != source) && (ptr->user != target))
             if(all || ((ptr->userflags & incuser) && !(ptr->userflags & excuser) && (ptr->customflags & inccustom) && !(ptr->customflags & exccustom))) {
                if(!Blank(next)) {
                   if(!first) strcat_limits(&str_data,", ");
                   strcat_limits(&str_data,next), first = 0;
                }

                if((names > 1) || _continue) {

                   /* ---->  Name of individual user  <---- */
                   sprintf(next,"%s%s%s%s",Article(ptr->user,LOWER,(Location(ptr->user) == Location(target)) ? DEFINITE:INDEFINITE),ansi1,getcname(NOTHING,ptr->user,0,0),ansi2);
                   addand = 1, names--;
                   if(!_continue) {

                      /* ---->  If space for one more name remaining, check remaining number of names  <---- */
                      if(names == 1) {
                         for(ptr2 = ptr->next, count = 0; ptr2; ptr2 = ptr2->next)
                             if((ptr2->userflags & USERLIST_INDIVIDUAL) && Validchar(ptr2->user) && !(ptr2->userflags & (USERLIST_EXCLUDED|USERLIST_INTERNAL)) && (ptr2->user != source) && (ptr2->user != target))
                                if(all || ((ptr2->userflags & incuser) && !(ptr2->userflags & excuser) && (ptr2->customflags & inccustom) && !(ptr2->customflags & exccustom)))
                                   count++;
                         if(count <= 1) _continue = 1;
                      }
                   }
                } else {
                   int count;

                   /* ---->  Too many names:  Summarise remaining individual users  <---- */
                   for(count = 0; ptr; ptr = ptr->next)
                       if((ptr->userflags & USERLIST_INDIVIDUAL) && Validchar(ptr->user) && !(ptr->userflags & (USERLIST_EXCLUDED|USERLIST_INTERNAL)) && (ptr->user != source) && (ptr->user != target))
                          if(all || ((ptr->userflags & incuser) && !(ptr->userflags & excuser) && (ptr->customflags & inccustom) && !(ptr->customflags & exccustom)))
                             count++;

                   if(count > 0) {
                      sprintf(next,"%s%d%s other user%s",ansi1,count,ansi2,Plural(count));
                      addand = 1;
                   }
                }
             }

      /* ---->  Standard keywords  <---- */
      addand = 0;
      while(!finished) {
            finished = ((friends || enemies || assistants || experienced || retired || admin || everyone) == 0);

            if(!finished) {
               if(!Blank(next)) {
                  if(!first) strcat_limits(&str_data,", ");
                  strcat_limits(&str_data,next), first = 0;
               }

               if(friends) {
                  if(source == target) strcpy(next,"your friends");
                     else sprintf(next,"%s friends",Possessive(source,0));
                  friends = 0;
               } else if(enemies) {
                  if(source == target) strcpy(next,"your enemies");
                     else sprintf(next,"%s enemies",Possessive(source,0));
                  enemies = 0;
               } else if(assistants) {
                  sprintf(next,"%sAssistants",(!addand) ? ((disconnected) ? "all ":"all connected"):"");
                  assistants = 0, addand = 1;
               } else if(experienced) {
                  sprintf(next,"%sExperienced Builders",(!addand) ? ((disconnected) ? "all ":"all connected"):"");
                  experienced = 0, addand = 1;
               } else if(retired) {
                  sprintf(next,"%sRetired Administrators",(!addand) ? ((disconnected) ? "all ":"all connected"):"");
                  retired = 0, addand = 1;
               } else if(admin) {
                  sprintf(next,"%s%s Administrators",(!addand) ? ((disconnected) ? "all ":"all connected"):"",tcz_short_name);
                  admin = 0, addand = 1;
               } else if(everyone) {
                  sprintf(next,"%susers",(!addand) ? ((disconnected) ? "all ":"all connected"):"");
                  everyone = 0, addand = 1;
               }
            }
      }

      /* ---->  Friend groups  <---- */

/*  [NEEDS TO EXCLUDE DUPLICATE GROUPS]
    [NEEDS TO EXCLUDE GROUPS WHICH NO-LONGER EXIST]
    [NEEDS TO TAKE GROUP NAME FROM ->NAME POINTER, IF NOT NULL]

      names     = USERLIST_GROUPS;
      _continue = 0;
      for(grp = userlist, count = 0; grp; grp = ptr->next)
          if((ptr->userflags & USERLIST_GROUP) && (ptr->userflags & USERLIST_INTERNAL)

          (ptr->userflags  USERLIST_GROUP) count++;
      manygroups = (count != 1);             

      for(ptr = userlist; ptr; ptr = ptr->next)
          if((ptr->userflags == USERLIST_GROUP) && !(ptr->userflags & USERLIST_EXCLUDE)) {
             if(!Blank(next)) {
                if(!first) strcat_limits(&str_data,", ");
                strcat_limits(&str_data,next), first = 0;
             }

             if((names > 1) || _continue) {
*/
                /* ---->  Name of individual group  <---- */
/*                if(firstgroup) {
                   sprintf(next,"friend group%s ",Plural(manygroups));
                   firstgroup = 0;
                } else *next = '\0';

                sprintf(next,"%s%s%s",ansi1,friends_group_lookup(source,NULL,ptr->group),ansi2);

                if(!_continue) {
*/
                   /* ---->  If space for one more group remaining, check remaining number of groups  <---- */
/*                   if(--names == 1) {
                      for(ptr2 = ptr->next, count = 0; ptr2; ptr2 = ptr2->next)
                          if((ptr2->userflags == USERLIST_GROUP) && !(ptr2->userflags & USERLIST_EXCLUDE))
                             count++;
                      if(count <= 1) _continue = 1;
		   }
		} else _continue = 0;
	     } else {
                int count;
*/
                /* ---->  Too many groups:  Summarise remaining individual groups  <---- */
/*                for(count = 0; ptr; ptr = ptr->next)
                    if((ptr->userflags == USERLIST_GROUP) && !(ptr->userflags & USERLIST_EXCLUDE))
                       count++;
                if(count > 0) sprinf(next,"%s%d%s other group%s",ansi2,count,ansi1,Plural(count));
             }
          }
*/

      /* ---->  Remaining entity (NEXT)  <---- */
      if(!Blank(next)) {
         if(!first) strcat_limits(&str_data," and ");
         strcat_limits(&str_data,next), first = 0;
      }
      *str_data.dest = '\0';
      return(buffer);
}

/* ---->  Code to allow testing of list parsing - REMOVE from this source and general_cmdtable.h  <---- */
void lists_test(CONTEXT)
{
     struct descriptor_data *p = getdsc(player);
     struct userlist_data *userlist,*ptr;

     setreturn(OK,COMMAND_SUCC);
#ifndef BETA
     output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the new user lists system is under development and has been disabled.");
     return;
#endif

     userlist = lists_parse(player,params,"construct a list of",USERLIST_DEFAULT|USERLIST_DISCONNECTED|USERLIST_ALL,1);
     if(userlist) {
        output(p,player,0,1,0,ANSI_LCYAN"\nIndividual Users Matched:\n"ANSI_DCYAN"~~~~~~~~~~~~~~~~~~~~~~~~~");
        for(ptr = userlist; ptr; ptr = ptr->next)
            if(!(ptr->userflags & USERLIST_INTERNAL))
               output(p,player,0,1,0,"%s"ANSI_LWHITE"%s",(ptr->userflags & USERLIST_EXCLUDE) ? ANSI_LRED"!":"",getcname(player,ptr->user,1,0));
        output(p,player,0,1,0,"");
        output(getdsc(player),player,0,1,7,ANSI_LYELLOW"LIST:  "ANSI_LGREEN"%s\n",lists_construct(player,player,userlist,ANSI_LWHITE,ANSI_LGREEN,scratch_return_string,SEARCH_ALL,0,SEARCH_ALL,0,1,0));
        lists_free(&userlist,SEARCH_ALL,0,SEARCH_ALL,0,1);
     }
}
