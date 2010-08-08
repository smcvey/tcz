/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| FRIENDS.C  -  Implements list of friends, friend flags and the friends      |
|               chatting channel.                                             |
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
| Module originally designed and written by:  J.P.Boggis 20/03/1994.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: friends.c,v 1.2 2005/06/29 21:08:10 tcz_monster Exp $

*/

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

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


/* ---->  Returns 1 if FRIEND is in PLAYER's friends list (Also sanitises friends list at same time)  <---- */
unsigned char friend(dbref player,dbref friend)
{
	 struct   friend_data *newptr,*cache,*ptr,*newlist = NULL;
	 unsigned char found = 0;
	 struct   grp_data *chk;

	 if(!Validchar(player) || !Validchar(friend)) return(0);
	 ptr = db[player].data->player.friends;

	 for(; ptr; ptr = cache) {
	     cache = ptr->next;
	     if(Validchar(ptr->friend)) {
		if(ptr->friend == friend) found = 1;
		if(newlist) {
		    /* if(ptr->flags & (FRIEND_READ|FRIEND_WRITE)) {
		      ptr->next = newlist;
		      newlist   = ptr;
		      } else {
		    (Removed because the reordering interferes with @with friends do fset $6 = <FLAG>) */
		      newptr->next = ptr;
		      newptr       = ptr;
		      ptr->next    = NULL;
		      /* } */
		} else {
		   newlist = newptr = ptr;
		   ptr->next        = NULL;
		}
	     } else {
		for(chk = grp; chk; chk = chk->next)
		    if(ptr == &chk->nunion->friend)
		       chk->nunion = NULL;
		FREENULL(ptr);
	     }
	 }
	 db[player].data->player.friends = newlist;
	 return(found);
}

/* ---->  Return friend flags of character in specified character's friends list  <---- */
int friend_flags(dbref player,dbref friend)
{
    struct friend_data *newptr,*cache,*ptr,*newlist = NULL;
    struct grp_data *chk;
    int    flags = 0;

    if(!Validchar(player) || !Validchar(friend)) return(0);
    ptr = db[player].data->player.friends;

    for(; ptr; ptr = cache) {
        cache = ptr->next;
        if(Validchar(ptr->friend)) {
           if(ptr->friend == friend) flags = ptr->flags|0x80000000;
           if(newlist) {
	       /* if(ptr->flags & (FRIEND_READ|FRIEND_WRITE)) {
                 ptr->next = newlist;
                 newlist   = ptr;
		 } else { */
                 newptr->next = ptr;
                 newptr       = ptr;
                 ptr->next    = NULL;
		 /* } */
	   } else {
              newlist = newptr = ptr;
              ptr->next        = NULL;
	   }
	} else {
           for(chk = grp; chk; chk = chk->next)
               if(ptr == &chk->nunion->friend)
                  chk->nunion = NULL;
           FREENULL(ptr);
	}
    }
    db[player].data->player.friends = newlist;
    return(flags);
}

/* ---->  Returns FLAGS if set on specified FRIEND, otherwise 0  <---- */
int friendflags_set(dbref player,dbref friend,dbref object,int flags)
{
    struct friend_data *current;

    if(!Validchar(player) || !Validchar(friend)) return(0);
    /* for(current = db[player].data->player.friends; current && !((flags & (FRIEND_READ|FRIEND_WRITE)) && !(current->flags & (FRIEND_READ|FRIEND_WRITE))); current = current->next) */
    for (current = db[player].data->player.friends; current; current = current->next)
        if(current->friend == friend) {
           if((flags & (FRIEND_READ|FRIEND_WRITE)) && (((flags & FRIEND_WRITE) && !(Builder(friend) || (Controller(friend) == player))) || (Valid(object) && (current->flags & FRIEND_SHARABLE) && !Sharable(object)))) return(0);
           if(flags == FRIEND_READ) flags |= FRIEND_WRITE;
           return(current->flags & flags);
	}
    return(0);
}

/* ---->  Remove privileges from users obtained via FRIEND_READ/FRIEND_WRITE friend flags which would effectively allow user to obtain the privileges of a higher level character (Used on promotions to Admin for security)  <---- */
void friendflags_privs(dbref player)
{
     struct friend_data *current;
     dbref  i;
 
     /* ---->  Remove privileges given to users in PLAYER's list  <---- */
     if(!Validchar(player)) return;
     for(current = db[player].data->player.friends; current; current = current->next)
         if(Validchar(current->friend)) {
            if(privilege(current->friend,255) > privilege(player,255))
               current->flags &= ~(FRIEND_COMMANDS|FRIEND_CREATE|FRIEND_DESTROY|FRIEND_WRITE);
            if(Uid(current->friend) == player)
               db[current->friend].data->player.uid = current->friend;
	 }

     /* ---->  Remove privileges given to PLAYER by other users  <---- */
     for(i = 0; i < db_top; i++)
         if(Typeof(i) == TYPE_CHARACTER) {
            if(privilege(player,255) > privilege(i,255)) {
               for(current = db[i].data->player.friends; current && (current->friend != player); current = current->next);
               if(current) current->flags &= ~(FRIEND_COMMANDS|FRIEND_CREATE|FRIEND_DESTROY|FRIEND_WRITE);
	    }
            if((Uid(i) == player) && !can_write_to(i,player,0)) db[i].data->player.uid = i;
	 }

     /* ---->  Reset PLAYER's CHUID if they no-longer have 'write' permission  <---- */
     if((Uid(player) != player) && !can_write_to(player,Uid(player),0))
        db[player].data->player.uid = player;
}

/* ---->  Returns full names of friend flags set on friend  <---- */
const char *friendflags_description(int flags)
{
      unsigned char i;

      *scratch_return_string = '\0';
      for(i = 0; friendflags[i].name != NULL; i++)
          if((flags & friendflags[i].flag) == friendflags[i].flag)
             sprintf(scratch_return_string + strlen(scratch_return_string),"%s%s%s",(*scratch_return_string) ? ANSI_DCYAN", ":"",(friendflags[i].flag == FRIEND_ENEMY) ? ANSI_LRED:(friendflags[i].flag == FRIEND_EXCLUDE) ? ANSI_LMAGENTA:(friendflags[i].flag & (FRIEND_READ|FRIEND_WRITE|FRIEND_COMMANDS|FRIEND_CREATE|FRIEND_DESTROY|FRIEND_COMBAT|FRIEND_SHARABLE)) ? ANSI_LYELLOW:ANSI_LWHITE,friendflags[i].name);

      if(Blank(scratch_return_string)) strcpy(scratch_return_string,"No privileges");
      return(scratch_return_string);
}

/* ---->  Friend flag set against SUBJECT by USER?  <---- */
unsigned char friendflags_check(dbref player,dbref subject,dbref user,int flag,const char *msg)
{
	 dbref automatic;
	 int   flags;

	 if(!Validchar(player) || !Validchar(subject) || !Validchar(user) || ((flag == FRIEND_LINK) && (Level4(db[player].owner) || Level4(db[subject].owner)))) return(1);
	 flags = friend_flags(user,subject);
	 if((flags && !(flags & flag))) {
	    automatic = match_simple(user,".message",VARIABLES,0,0);
	    switch(flag) {
		   case FRIEND_LINK:
			if(!in_command) {
			   if(Valid(automatic) && !Blank(getfield(automatic,DESC))) {
			      sprintf(scratch_buffer,ANSI_LGREEN"\n%s refusal message from %s"ANSI_LWHITE"%s"ANSI_LGREEN":  ",msg,Article(user,LOWER,INDEFINITE),getcname(NOTHING,user,0,0));
			      sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LWHITE"%s\n",punctuate((char *) getfield(automatic,DESC),2,'.'));
			   } else if(player != subject) {
			      sprintf(scratch_buffer,ANSI_LGREEN"\nSorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" doesn't want ",Article(user,LOWER,INDEFINITE),getcname(NOTHING,user,0,0));
			      sprintf(scratch_buffer + strlen(scratch_buffer),"%s"ANSI_LWHITE"%s"ANSI_LGREEN" in %%p rooms/locations.\n",Article(subject,LOWER,DEFINITE),getcname(NOTHING,subject,0,0));
			   } else sprintf(scratch_buffer,ANSI_LGREEN"\nSorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" doesn't want you in %%p rooms/locations.\n",Article(user,LOWER,INDEFINITE),getcname(NOTHING,user,0,0));
			   output(getdsc(player),player,0,1,0,"%s",substitute(user,scratch_return_string,scratch_buffer,0,ANSI_LWHITE,NULL,0));
			}
			break;
		   case FRIEND_MAIL:
			if(Valid(automatic) && !Blank(getfield(automatic,ODROP))) {
			   sprintf(scratch_buffer,ANSI_LGREEN"\nMail refusal message from %s"ANSI_LWHITE"%s"ANSI_LGREEN":  ",Article(user,LOWER,DEFINITE),getcname(NOTHING,user,0,0));
			   sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LWHITE"%s\n",punctuate((char *) getfield(automatic,ODROP),2,'.'));
			   substitute(user,scratch_return_string,scratch_buffer,0,ANSI_LWHITE,NULL,0);
			} else if(player != subject) {
			   sprintf(scratch_return_string,ANSI_LGREEN"\nSorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" doesn't want to receive mail from ",Article(user,LOWER,DEFINITE),getcname(NOTHING,user,0,0));
			   sprintf(scratch_return_string + strlen(scratch_return_string),"%s"ANSI_LWHITE"%s"ANSI_LGREEN".\n",Article(subject,LOWER,DEFINITE),getcname(NOTHING,subject,0,0));
			} else sprintf(scratch_return_string,ANSI_LGREEN"\nSorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" doesn't want to receive mail from you.\n",Article(user,LOWER,DEFINITE),getcname(NOTHING,user,0,0));
			output(getdsc(player),player,0,1,0,"%s",substitute(user,scratch_return_string,scratch_buffer,0,ANSI_LWHITE,NULL,0));
			break;
		   default:
			break;
	    }
	    return(0);
	 } else return(1);
}

/* ---->  Add friend to your friends list  <---- */
void friends_add(CONTEXT)
{
     dbref user;

     setreturn(ERROR,COMMAND_FAIL);
     if(!in_command || Wizard(current_command) || (db[current_command].owner == player)) {
        if(!Blank(arg1)) {
           if((user = lookup_character(player,arg1,1)) == NOTHING) {
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
              return;
	   }
	} else {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify who you'd like to add to your friends list.");
           return;
	}

        if(!friend(player,user)) {
           struct friend_data *new,*last = NULL,*ptr = db[player].data->player.friends;
           int    count;

           if(user != player) {

              /* ---->  Count friends  <---- */
              for(count = 0; ptr; last = ptr, ptr = ptr->next, count++);
              if((!Level4(db[player].owner) && (count >= MAX_FRIENDS_MORTAL)) || (Level4(db[player].owner) && (count >= MAX_FRIENDS_ADMIN))) {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have too many friends in your friends list (You're allowed a maximum of "ANSI_LWHITE"%d"ANSI_LGREEN")  -  Please see '"ANSI_LYELLOW"help fremove"ANSI_LGREEN"' for details on how to remove friends (To make room for new ones.)",(Level4(db[player].owner)) ? MAX_FRIENDS_ADMIN:MAX_FRIENDS_MORTAL);
                 return;
	      }

              /* ---->  Add new friend to friends list  <---- */
              MALLOC(new,struct friend_data);
              new->friend = user;
              new->flags  = FRIEND_STANDARD;
              new->next   = NULL;
              if(last) last->next = new;
                 else db[player].data->player.friends = new;
              if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" is now one of your friends.",Article(user,UPPER,DEFINITE),getcname(NOTHING,user,0,0));
              if(FriendsInform(user) && !friendflags_set(user,player,NOTHING,FRIEND_EXCLUDE) && !Quiet(user))
                 output(getdsc(user),user,0,1,2,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has made you one of %s friends.",Article(player,LOWER,(Location(player) == Location(user)) ? DEFINITE:INDEFINITE),getcname(NOTHING,player,0,0),Possessive(player,0));

              /* ---->  Optionally set/reset friend flags on new friend  <---- */
              if(!Blank(arg2)) friends_set(player,NULL,NULL,arg1,arg2,0,0);
              setreturn(OK,COMMAND_SUCC);
           } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't add yourself to your friends list.");
	} else if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is already one of your %s.",Article(user,LOWER,DEFINITE),getcname(NOTHING,user,0,0),(friend_flags(player,user) & FRIEND_ENEMY) ? "enemies":"friends");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't add a new friend to your friends list from within a compound command (Unless the compound command is owned by yourself.)");
}

/* ---->  Remove given friend from given character's friends list  <---- */
void remove_friend(dbref player,dbref friend)
{
     struct friend_data *newptr,*cache,*ptr,*newlist = NULL;
     struct grp_data *chk;

     ptr = db[player].data->player.friends;
     while(ptr != NULL) {
           cache = ptr->next;
           if(ptr->friend != friend) {
              if(newlist) {
                 newptr->next = ptr;
                 newptr       = ptr;
                 ptr->next    = NULL;
	      } else {
                 newlist = newptr = ptr;
                 ptr->next        = NULL;
	      }
	   } else {
             for(chk = grp; chk; chk = chk->next)
                 if(ptr == &chk->nunion->friend)
                    chk->nunion = NULL;
              FREENULL(ptr);
	   }
           ptr = cache;
     }
     db[player].data->player.friends = newlist;
}

/* ---->  Remove friend from your friends list (Or remove yourself from someone else's friends list)  <---- */
void friends_remove(CONTEXT)
{
     unsigned char removed = 0;
     int      flags;
     dbref    user;

     setreturn(ERROR,COMMAND_FAIL);
     if(!in_command || Wizard(current_command) || (db[current_command].owner == player)) {
        if(!Blank(params)) {
           if((user = lookup_character(player,params,1)) == NOTHING) {
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",params);
              return;
	   }
	} else {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify who you'd like to remove from your friends list.");
           return;
	}

        if(user != player) {

           /* ---->  Remove user from your friends list  <---- */
           if(friend(player,user)) {
              remove_friend(player,user);
              if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" is no-longer one of your %s.",Article(user,UPPER,DEFINITE),getcname(NOTHING,user,0,0),(friend_flags(player,user) & FRIEND_ENEMY) ? "enemies":"friends");
              removed = 1;
	   }

           /* ---->  Remove yourself from someone else's friends list  <---- */
           if((flags = friend_flags(user,player))) {
              if(flags & FRIEND_ENEMY) {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" has set the "ANSI_LYELLOW"ENEMY"ANSI_LGREEN" friend flag on you in their friends list  -  You can't remove yourself from their friends list.",Article(user,LOWER,DEFINITE),getcname(NOTHING,user,0,0));
                 return;
	      } else if(!(flags & FRIEND_PAGETELL)) {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" has reset the "ANSI_LYELLOW"PAGETELL"ANSI_LGREEN" friend flag on you in their friends list  -  You can't remove yourself from their friends list.",Article(user,LOWER,DEFINITE),getcname(NOTHING,user,0,0));
                 return;
	      } else if(!(flags & FRIEND_LINK)) {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" has reset the "ANSI_LYELLOW"LINK"ANSI_LGREEN" friend flag on you in their friends list  -  You can't remove yourself from their friends list.",Article(user,LOWER,DEFINITE),getcname(NOTHING,user,0,0));
                 return;
	      } else if(!(flags & FRIEND_MAIL)) {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" has reset the "ANSI_LYELLOW"MAIL"ANSI_LGREEN" friend flag on you in their friends list  -  You can't remove yourself from their friends list.",Article(user,LOWER,DEFINITE),getcname(NOTHING,user,0,0));
                 return;
	      } else {
                 remove_friend(user,player);
                 if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"You have been removed from %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s friends list.",Article(user,LOWER,DEFINITE),getcname(NOTHING,user,0,0));
                 removed = 2;
	      }
	   }

           if(!removed) {
              if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" isn't one of your friends or enemies.",Article(user,LOWER,DEFINITE),getcname(NOTHING,user,0,0));
              return;
	   } else if(FriendsInform(user) && !friendflags_set(user,player,NOTHING,FRIEND_EXCLUDE) && !Quiet(user))
              output(getdsc(user),user,0,1,2,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has removed %s from %s friends list.",Article(player,LOWER,(Location(player) == Location(user)) ? DEFINITE:INDEFINITE),getcname(NOTHING,player,0,0),(removed == 2) ? Reflexive(player,0):"you",(removed == 2) ? "your":Possessive(player,0));
           setreturn(OK,COMMAND_SUCC);
        } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't remove yourself from your friends list.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't remove one of your friends from within a compound command (Unless the compound command is owned by yourself.)");
}

/* ---->  Process given friend flag name and then set/reset it on given friend  <---- */
unsigned char process_friendflag(dbref player,struct friend_data *fptr,const char *flagname,unsigned char reset)
{
	 int      flag = 0;
	 unsigned char pos;

	 /* ---->  Look up flag in friend flags list  <---- */
	 pos = 0;
	 while(!flag && friendflags[pos].name)
	       if(string_prefix(friendflags[pos].name,flagname) || string_prefix(friendflags[pos].alt,flagname)) flag = friendflags[pos].flag;
		  else pos++;

	 if(!flag) {
	    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"%s"ANSI_LGREEN"' is an unknown friend flag.",flagname);
	    return(0);
	 }

	 switch(flag) {
		case FRIEND_CREATE:
		     if(!in_command) {
			if(reset) {
			   if(can_write_to(fptr->friend,player,0) != 1)
			      db[fptr->friend].data->player.uid = fptr->friend;
			} else {
			   output(getdsc(player),player,0,1,0,"");
			   output(getdsc(player),player,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"%s"ANSI_LYELLOW"%s"ANSI_LWHITE" will now be able to create objects under your ownership (%s compound commands) and change the owner of any of their objects to you.  If you do not want them to be able to do this, please reset their "ANSI_LYELLOW"CREATE"ANSI_LWHITE" friend flag ('"ANSI_LGREEN"fset %s = !create"ANSI_LWHITE"'.)\n",Article(fptr->friend,UPPER,DEFINITE),getcname(NOTHING,fptr->friend,0,0),(fptr->flags & FRIEND_COMMANDS) ? "Including":"Excluding",getname(fptr->friend));
			}
		     } else {
			output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"CREATE"ANSI_LGREEN" friend flag can't be set/reset from within a compound command.");
			return(0);
		     }
		     friend(player,fptr->friend);
		     break;
		case FRIEND_DESTROY:
		     if(in_command) {
			output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"DESTROY"ANSI_LGREEN" friend flag can't be set/reset from within a compound command.");
			return(0);
		     } else if(!reset) {
			output(getdsc(player),player,0,1,0,"");
			output(getdsc(player),player,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"%s"ANSI_LYELLOW"%s"ANSI_LWHITE" will now be able to destroy %s of your objects%s, %s compound commands.  If you do not want them to be able to do this, please reset their "ANSI_LYELLOW"DESTROY"ANSI_LWHITE" friend flag ('"ANSI_LGREEN"fset %s = !destroy"ANSI_LWHITE"'.)\n",Article(fptr->friend,UPPER,DEFINITE),getcname(NOTHING,fptr->friend,0,0),(fptr->flags & FRIEND_SHARABLE) ? "any":ANSI_LMAGENTA""ANSI_BLINK"ANY"ANSI_LWHITE,(fptr->flags & FRIEND_SHARABLE) ? " which have their "ANSI_LYELLOW"SHARABLE"ANSI_LWHITE" flag set":"",(fptr->flags & FRIEND_COMMANDS) ? "including":"excluding",getname(fptr->friend));
		     }
		     break;
		case FRIEND_PAGETELL:
		     if(reset) fptr->flags &= ~FRIEND_PAGETELLFRIENDS;
		     break;
		case FRIEND_READ:
		     if(in_command) {
			output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"READ"ANSI_LGREEN" friend flag can't be set/reset from within a compound command.");
			return(0);
		     }
		     friend(player,fptr->friend);
		     break;
		case FRIEND_SHARABLE:
		     if(in_command) {
			output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"SHARABLE"ANSI_LGREEN" friend flag can't be set/reset from within a compound command.");
			return(0);
		     } else if(reset && (fptr->flags & FRIEND_WRITE)) {
			fptr->flags &= ~FRIEND_COMMANDS;
			output(getdsc(player),player,0,1,0,"");
			output(getdsc(player),player,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"%s"ANSI_LYELLOW"%s"ANSI_LWHITE" will now be able to modify and destroy "ANSI_LMAGENTA""ANSI_BLINK"ANY"ANSI_LWHITE" of your objects (Excluding compound commands.)  If you do not want them to be able to do this, please set their "ANSI_LYELLOW"SHARABLE"ANSI_LWHITE" friend flag ('"ANSI_LGREEN"fset %s = sharable"ANSI_LWHITE"'.)\n",Article(fptr->friend,UPPER,DEFINITE),getcname(NOTHING,fptr->friend,0,0),getname(fptr->friend));
		     }
		     friend(player,fptr->friend);
		     break;
		case FRIEND_WRITE:
		     if(!in_command) {
			if(reset) {
			   if(can_write_to(fptr->friend,player,0) != 1)
			      db[fptr->friend].data->player.uid = fptr->friend;
			} else if(privilege(player,255) >= privilege(fptr->friend,255)) {
			   if(!(fptr->flags & FRIEND_WRITE)) {
			      fptr->flags |=  FRIEND_SHARABLE;
			      fptr->flags &= ~FRIEND_COMMANDS;
			      output(getdsc(player),player,0,1,0,"");
			      output(getdsc(player),player,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"%s"ANSI_LYELLOW"%s"ANSI_LWHITE" will now be able to modify and destroy "ANSI_LMAGENTA""ANSI_BLINK"ANY"ANSI_LWHITE" of your objects (Excluding compound commands) which have their "ANSI_LYELLOW"SHARABLE"ANSI_LWHITE" flag set (If you do not want them to be able to do this, please reset their "ANSI_LYELLOW"WRITE"ANSI_LWHITE" friend flag ('"ANSI_LGREEN"fset %s = !write"ANSI_LWHITE"'.))\n",Article(fptr->friend,UPPER,DEFINITE),getcname(NOTHING,fptr->friend,0,0),getname(fptr->friend));
			   }
			} else {
			   output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"WRITE"ANSI_LGREEN" friend flag can't be set on a lower level character (As this would allow them to obtain privileges to which they are not entitled.)");
			   return(0);
			}
		     } else {
			output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"WRITE"ANSI_LGREEN" friend flag can't be set/reset from within a compound command.");
			return(0);
		     }
		     break;
	 }

	 /* ---->  Set/reset friend flag  <---- */
	 if(friend && Validchar(fptr->friend)) {
	    if(reset) fptr->flags &= ~flag;
	       else fptr->flags   |=  flag;
	    if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LYELLOW"%s"ANSI_LGREEN" friend flag %s on %s"ANSI_LWHITE"%s"ANSI_LGREEN"  "ANSI_LCYAN"--->  "ANSI_LWHITE"%s"ANSI_DCYAN".",friendflags[pos].name,(reset) ? "reset":"set",Article(fptr->friend,LOWER,DEFINITE),getcname(player,fptr->friend,1,0),friendflags_description(fptr->flags));
	    return(1);
	 } else {
	    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that friend is invalid.");
	    return(0);
	 }
}

/* ---->  Set/reset friend flags on friend in your friends list  <---- */
void friends_set(CONTEXT)
{
     unsigned char result = 0,reset;
     struct   friend_data *friends;
     dbref    user;
     char     *p1;

     setreturn(ERROR,COMMAND_FAIL);
     if(!in_command || Wizard(current_command) || (db[current_command].owner == player)) {
        if(Blank(arg2)) {
  
           /* ---->  Grab first word as name(s)  <---- */
           for(; *arg1 && (*arg1 == ' '); arg1++);
           for(p1 = arg1; *p1 && (*p1 != ' '); p1++);
           if(*p1) for(*p1 = '\0', p1++; *p1 && (*p1 == ' '); p1++);
           arg2 = (char *) p1;
	}

        if(!Blank(arg1)) {
           if((user = lookup_character(player,arg1,1)) == NOTHING) {
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
              return;
	   }
	} else {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify who you'd like to set/reset friend flags on in your friends list.");
           return;
	}

        if(friend(player,user)) {
           if(Blank(arg2)) {
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify which friend flag(s) you'd like to set/reset on %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(user,LOWER,DEFINITE),getcname(NOTHING,user,0,0));
              return;
	   }
           for(friends = db[player].data->player.friends; friends && (friends->friend != user); friends = friends->next);

           /* ---->  Grab flag names and set/reset them on specified friend  <---- */
           while(*arg2) {
                 while(*arg2 && (*arg2 == ' ')) arg2++;
                 if(*arg2 && (*arg2 == '!')) {
                    arg2++;
                    reset = 1;
		 } else reset = 0;

                 /* ---->  Grab flag name from FLAGLIST  <---- */
                 p1 = scratch_buffer;
                 while(*arg2 && (*arg2 != ' ')) {
                       *p1++ = *arg2;
                       arg2++;
		 }
                 *p1 = '\0';

                 if(!Blank(scratch_buffer)) result |= process_friendflag(player,friends,scratch_buffer,reset);
                    else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify which friend flag you'd like to reset.");
	   }

           if(result) {
              if(FriendsInform(user) && !friendflags_set(user,player,NOTHING,FRIEND_EXCLUDE) && !Quiet(user))
                 output(getdsc(user),user,0,1,2,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has changed the friend flags %s has set on you in %s friends list to the following:  "ANSI_LWHITE"%s"ANSI_DCYAN".",Article(player,LOWER,(Location(player) == Location(user)) ? DEFINITE:INDEFINITE),getcname(NOTHING,player,0,0),Subjective(player,0),Possessive(player,0),friendflags_description(friend_flags(player,user)));
              setreturn(OK,COMMAND_SUCC);
	   }
	} else {
           if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" isn't one of your friends.",Article(user,LOWER,DEFINITE),getcname(NOTHING,user,0,0));
           return;
	}
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't set/reset friend flags on your friends from within a compound command (Unless the compound command is owned by yourself.)");
}

/* ---->  Returns the friend flag that STR matches  <----- */
unsigned char parse_friendflagtype(const char *str,int *flags_inc,int *flags_exc)
{
	 unsigned char not = 0,matched = 0;
	 int      count = 0;

	 if(*str && (*str == '!')) {
	    for(; *str && (*str == '!'); str++);
	    not = 1;
	 }

	 for(count = 0; friendflags[count].name && !matched; count++) {
	     if(string_prefix(friendflags[count].name,str) || string_prefix(friendflags[count].alt,str)) {
		if(!not) *flags_inc |= friendflags[count].flag, matched = 1;
		   else *flags_exc |= friendflags[count].flag, matched = 1;
	     }
	 }
	 return(matched);
}

/* ---->  List friends in your friends list, or list users who have you in their friends list  <---- */
/*        (val1:  0 = fothers, 1 = flist)                                                                            */
void friends_list(CONTEXT)
{
     unsigned char cr = 1,cached_scrheight,twidth = output_terminal_width(player);
     int      dummy, flags_inc = 0,flags_mask = 0,flags_inc2 = 0,flags_mask2 = 0;
     int      result,flags_exc = 0,fflags_inc = 0,fflags_exc = 0,valid       = 0;
     struct   descriptor_data *p = getdsc(player);
     const    char *p1,*colour,*namespec = NULL;
     dbref    who = player;
     char     *p2,*ptr;

     setreturn(ERROR,COMMAND_FAIL);
     if(!(in_command && !Level4(db[player].owner))) {
        if((!strcasecmp("page",arg1) && (strlen(arg1) == 4)) || !strncasecmp(arg1,"page ",5))
           for(arg1 += 4; *arg1 && (*arg1 == ' '); arg1++);
        arg1 = (char *) parse_grouprange(player,arg1,FIRST,1);
        if(!Blank(arg1)) {
           if((*arg1 == LOOKUP_TOKEN) || (*arg1 == NUMBER_TOKEN)) {

              /* ---->  Friends list/others list of another character  <---- */
              if((who = lookup_character(player,arg1,1)) == NOTHING) {
                 output(p,player,0,1,0,ANSI_LGREEN"Sorry, that character doesn't exist.");
                 return;
	      }

              if(!can_read_from(player,who)) {
                 output(p,player,0,1,0,(val1) ? ANSI_LGREEN"Sorry, you can only list your own friends.":ANSI_LGREEN"Sorry, you can only list people who have you in their friends list.");
                 return;
	      }

              /* ---->  Users matching given <NAME>  <---- */
	   } else namespec = arg1;
	}

        /* ---->  Start parsing flag types from first parameter  <---- */
        p1 = arg2;
        while(*p1) {
              while(*p1 && (*p1 == ' ')) p1++;
              if(*p1) {
                 for(p2 = scratch_buffer; *p1 && (*p1 != ' '); *p2++ = *p1++);
                 *p2 = '\0';

                 if((result = parse_friendflagtype(scratch_buffer,&fflags_inc,&fflags_exc))) valid = 1;
                    else if((result = parse_flagtype(scratch_buffer,&dummy,&flags_mask))) valid = flags_inc |= result;
                       else if((result = parse_flagtype2(scratch_buffer,&dummy,&flags_mask2))) valid = flags_inc2 |= result;
                          else if(string_prefix("mortals",scratch_buffer)) {
                             flags_exc    |= BUILDER|APPRENTICE|WIZARD|ELDER|DEITY;
                             valid         = 1;
                             result++;
		          } else if(string_prefix("all",scratch_buffer)) {
                             flags_mask2 = 0, flags_mask = 0;
                             flags_inc2  = 0, flags_inc  = 0;
                             flags_exc   = 0, valid      = 1;
                             result++;
			  }

   	         if(!result) output(p,player,0,1,0,ANSI_LGREEN"%sSorry, '"ANSI_LWHITE"%s"ANSI_LGREEN"' is an unknown friend flag/flag.",(cr) ? "\n":"",scratch_buffer), cr = 0;
	      }
	}
        if(!fflags_inc) fflags_inc = SEARCH_ALL;

        if(!(!valid && !Blank(arg1) && !Blank(arg2))) {
           setreturn(OK,COMMAND_SUCC);
           html_anti_reverse(p,1);
           if(p && !p->pager && !IsHtml(p) && Validchar(p->player) && More(p->player)) pager_init(p);
           if(IsHtml(p)) output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
           set_conditions_ps(who,flags_inc,flags_mask,flags_inc2,flags_mask2,flags_exc,fflags_exc,fflags_inc,namespec,(val1) ? 303:304);
           cached_scrheight               = db[who].data->player.scrheight;
           db[who].data->player.scrheight = (db[player].data->player.scrheight) - 8;
           entiredb_initgrouprange();

           /* ---->  List friends  <---- */
           if(!in_command) {
              if(val1) {
                 if(who != player) output(p,player,2,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN" has the following people in %s friends list...%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREEN"><TH COLSPAN=2><FONT COLOR="HTML_LGREEN" SIZE=4>\016":"\n "ANSI_LGREEN,Article(who,UPPER,DEFINITE),getcname(player,who,1,0),Possessive(who,0),IsHtml(p) ? "\016</FONT></TH></TR>\016":"\n\n");
                    else output(p,player,2,1,0,"%sYou have the following people in your friends list...%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREEN"><TH COLSPAN=2><FONT COLOR="HTML_LGREEN" SIZE=4>\016":"\n "ANSI_LGREEN,IsHtml(p) ? "\016</FONT></TH></TR>\016":"\n\n");
                 if(IsHtml(p)) output(p,player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH WIDTH=25%%><FONT COLOR="HTML_LCYAN" SIZE=4><I>Name:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Friend flags that you've set on them:</I></FONT></TH></TR>\016");
                    else output(p,player,0,1,0," Name:                 Friend flags that you've set on them:");
	      } else {
                 if(who != player) output(p,player,2,1,0,"%sThe following people have %s"ANSI_LWHITE"%s"ANSI_LGREEN" in their friends list...%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREEN"><TH COLSPAN=2><FONT COLOR="HTML_LGREEN" SIZE=4>\016":"\n "ANSI_LGREEN,Article(who,LOWER,DEFINITE),getcname(player,who,1,0),IsHtml(p) ? "\016</FONT></TH></TR>\016":"\n\n");
                    else output(p,player,2,1,0,"%sThe following people have you in their friends list...%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREEN"><TH COLSPAN=2><FONT COLOR="HTML_LGREEN" SIZE=4>\016":"\n "ANSI_LGREEN,IsHtml(p) ? "\016</FONT></TH></TR>\016":"\n\n");
                 if(IsHtml(p)) output(p,player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH WIDTH=25%%><FONT COLOR="HTML_LCYAN" SIZE=4><I>Name:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Friend flags that they've set on you:</I></FONT></TH></TR>\016");
                    else output(p,player,0,1,0," Name:                 Friend flags that they've set on you:");
	      }
              if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
	   }

           if(grp->distance > 0) {
              while(entiredb_grouprange()) {
                    sprintf(scratch_buffer,"%s%s",IsHtml(p) ? "\016<TR ALIGN=LEFT><TD WIDTH=25%>\016":"",colour = privilege_colour(grp->cobject));
                    p1 = (char *) getfield(grp->cobject,PREFIX);
                    if(!Blank(p1) && ((strlen(p1) + 1 + strlen(getname(grp->cobject))) <= 20))
                       sprintf(p2 = (scratch_buffer + strlen(scratch_buffer))," %s %s",p1,getname(grp->cobject));
                          else sprintf(p2 = (scratch_buffer + strlen(scratch_buffer))," %s",getname(grp->cobject));

                    if(!IsHtml(p)) {
                       if((result = strlen(p2)) <= 21) {
                          for(ptr = p2 + result; result < 21; *ptr++ = ' ', result++);
                              *ptr = '\0';
		       } else p2[21] = '\0';
                       strcat(scratch_buffer,"  ");
		    }
                    output(p,player,2,1,23,"%s%s"ANSI_LWHITE"%s"ANSI_DCYAN".%s",scratch_buffer,IsHtml(p) ? "\016</TD><TD>\016":"",friendflags_description((val1) ? friend_flags(who,grp->cobject):friend_flags(grp->cobject,who)),IsHtml(p) ? "\016</TD></TR>\016":"\n");
	      }

              if(!in_command) {
                 if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
                 output(p,player,2,1,1,"%sTotal friends found: \016&nbsp;\016 "ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><TD COLSPAN=2>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",listed_items(scratch_return_string,1),IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
	      }
	   } else {
              output(p,player,2,1,0,"%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TD COLSPAN=2>"ANSI_LCYAN"<I>*** &nbsp; NO FRIENDS FOUND &nbsp; ***</I></TD></TR>\016":ANSI_LCYAN" ***  NO FRIENDS FOUND  ***\n");
              if(!in_command) {
                 if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
                 output(p,player,2,1,0,"%sTotal friends found: \016&nbsp;\016 "ANSI_DWHITE"None.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><TD COLSPAN=2>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
	      }
	   }
           db[who].data->player.scrheight = cached_scrheight;
           if(IsHtml(p)) output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
           html_anti_reverse(p,1);
	} else output(p,player,0,1,0,ANSI_LGREEN"%sPlease specify the flag(s) characters must have to be listed.\n",(cr) ? "\n":"");
     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may use '"ANSI_LWHITE"%s"ANSI_LGREEN"' from within a compound command.",(val1) ? "flist":"fothers");
}

/* ---->  Output MSG to friends of PLAYER over friends chatting channel  <---- */
void output_fchat(dbref player,dbref exception,const char *msg)
{
     int    flags,flags2,friend;
     struct descriptor_data *d;

     for(d = descriptor_list; d; d = d->next) {
         friend = 0;
         if(!(flags  = friend_flags(player,d->player))) flags  = FRIEND_STANDARD;
            else friend |= flags;
	 if(!(flags2 = friend_flags(d->player,player))) flags2 = FRIEND_STANDARD;
            else friend |= flags;
         if((d->flags & CONNECTED) && (d->player != exception) && Validchar(d->player) && (db[d->player].flags2 & FRIENDS_CHAT) && ((player == d->player) || (friend && !(!(flags & FRIEND_FCHAT) || !(flags2 & FRIEND_FCHAT)))))
            output(d,d->player,0,1,11,"%s  %s",((flags|flags2) & FRIEND_ENEMY) ? ANSI_LRED"[ENEMIES]":ANSI_LYELLOW"[FRIENDS]",msg);
     }
}

/* ---->  Chat over friends chatting channel  <---- */
void friends_chat(CONTEXT)
{
     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(params) && !strcasecmp(params,"on")) {
        if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"You'll now receive messages from your friends over the friends chatting channel.");
        if(!(db[player].flags2 & FRIENDS_CHAT)) {
           db[player].flags2 |= FRIENDS_CHAT;
           sprintf(scratch_return_string,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has joined the friends chatting channel.",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0));
           output_fchat(player,player,scratch_return_string);
	}
     } else if(!Blank(params) && !strcasecmp(params,"off")) {
        if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"You'll no-longer receive messages from your friends over the friends chatting channel.");
        if(db[player].flags2 & FRIENDS_CHAT) {
           db[player].flags2 &= ~FRIENDS_CHAT;
           sprintf(scratch_return_string,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has left the friends chatting channel.",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0));
           output_fchat(player,player,scratch_return_string);
	}
     } else {
        comms_spoken(player,1);
        if(!Censor(player) && !Censor(db[player].location)) bad_language_filter((char *) params,params);
        if(!Moron(player)) {
	   if(db[player].flags2 & FRIENDS_CHAT) {
	      if(db[player].data->player.friends) {
                 if(!Blank(params)) {
                    output(getdsc(player),player,0,1,11,ANSI_LYELLOW"[FRIENDS]  %s",construct_message(player,ANSI_LWHITE,ANSI_DWHITE,"say",'.',-1,PLAYER,params,0,DEFINITE));
                    output_fchat(player,player,construct_message(player,ANSI_LWHITE,ANSI_DWHITE,"says",'.',-1,OTHERS,params,0,INDEFINITE));
                    setreturn(OK,COMMAND_SUCC);
                 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"What would you like to say to your friends?");
              } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, your friends list is empty  -  Please see '"ANSI_LWHITE"help friends"ANSI_LGREEN"' for details on how to add to and use your friends list.");
           } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you've turned the friends chatting channel facility off  -  Please type '"ANSI_LWHITE"fchat on"ANSI_LGREEN"' if you'd like to turn it back on (So you can talk to your friends again.)");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Morons can't chat over the friends chatting channel.");
     }
}

/* ---->  Add friend to friends list (Or list connected friends, if parameters blank)  <---- */
void friends_cmd(CONTEXT)
{
     if(!Blank(params)) friends_add(player,params,NULL,NULL,NULL,0,0);
        else userlist_view(player,params,NULL,NULL,NULL,10,0);
}
