/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| SANITY.C:  Implements automated database integrity checking.  Unless a      |
|            'clean' database has been dumped (I.e:  '@shutdown'), a full     |
|            'sanity' check is always performed on databases to ensure there  |
|            are no inconsistencies (Caused by modifications to the database  |
|            by users while the database is dumped in the background.)        |
|                                                                             |
|            'Sanity' checks are performed automatically at start-up, and     |
|            can also be ran manually via the '@sanity' command.              |
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
| Module originally designed and written by:  J.P.Boggis 23/03/1995.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: sanity.c,v 1.2 2005/06/29 20:21:50 tcz_monster Exp $

*/

#include <string.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "object_types.h"
#include "friend_flags.h"
#include "objectlists.h"
#include "flagset.h"


/* ---->  Mark all invalid objects in given linked list by removing their OBJECT flag (Also, move them out of the list at same time)  <---- */
dbref sanity_marklist(dbref first,dbref object)
{
      dbref newptr,cache,ptr,cptr,newlist = NOTHING;
      int   counter;

      ptr = first;
      while(Valid(ptr)) {
            if(Valid(ptr)) {

               /* ---->  Check next object in linked list isn't already in new linked list (Resulting in cyclic linked list)  <---- */
               for(cptr = newlist; Valid(cptr) && Valid(Next(ptr)); cptr = Next(cptr))
                   if(cptr == Next(ptr)) db[ptr].next = NOTHING;

               cache = Next(ptr);
               if(!((Typeof(ptr) == TYPE_FREE) || (db[ptr].location != object))) {
  	          if(newlist != NOTHING) {
                     db[newptr].next  = ptr;
                     newptr           = ptr;
                     db[ptr].next     = NOTHING;
                  } else {
                     newlist = newptr = ptr;
                     db[ptr].next     = NOTHING;
		  }
                  db[ptr].flags |= OBJECT;
	       } else {

                  /* ---->  Scan ahead through linked list to avoid cyclic list  <---- */
                  db[ptr].flags &= ~OBJECT;
                  for(cptr = cache, counter = 0; Valid(cptr) && (counter < db_top); cptr = Next(cptr), counter++)
                      if(db[cptr].next == ptr) db[cptr].next = NOTHING;
	       }
	    }
            ptr = cache;
      }
      return(newlist);
}

/* ---->  General sanity check  <---- */
void sanity_general(dbref player,unsigned char log)
{
     dbref                     newowner = (Validchar(maint_owner)) ? maint_owner:ROOT;
     int                       found = 0,uncorrectable = 0;
     dbref                     i,parent,original,chk;
     struct   descriptor_data  *p = getdsc(player);
     struct   bbs_topic_data   *subtopic,*topic;
     unsigned char             corrupt,cr = 0;
     time_t                    start,finish;
     struct   bbs_message_data *message;
     struct   friend_data      *friend;

     /* ---->  Sanitise database objects  <---- */
     gettime(start);
     if(!in_command && p && !p->pager && !IsHtml(p) && More(player)) pager_init(p);
     writelog(SANITY_LOG,1,"SANITY","General integrity check started on database by %s(#%d)...",getname(player),player);
     if(!log) writelog(ADMIN_LOG,1,"SANITY","General integrity check started on database by %s(#%d).",getname(player),player);
     for(i = 0; i < db_top; i++) {
         corrupt = 0;
         if(Typeof(i) != TYPE_FREE) {
            db[i].flags |= OBJECT;
            if(Typeof(i) != TYPE_CHARACTER)
               db[i].flags2 &= ~NON_EXECUTABLE;
	 }

         switch(Typeof(i)) {
                case TYPE_CHARACTER:

                     /* ---->  Character's owner invalid?  <---- */
                     if(db[i].owner != i) {
                        if(!log) {
                           if(!cr) output(p,player,0,1,0,""), cr = 1;
                           output(p,player,0,1,2,ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED"'s doesn't own %sself.  [CORRECTED]",Article(i,UPPER,INDEFINITE),getcname(player,i,1,0),Objective(i,0));
			}
                        writelog(SANITY_LOG,0,"SANITY","%s(#%d) doesn't own %sself.  [CORRECTED]",getname(i),i,Objective(i,0));
                        db[i].owner = db[i].data->player.chpid = i;
                        corrupt = 1;
		     }

                     /* ---->  Puppet's controller/character's partner invalid?  <---- */
                     if(!Validchar(db[i].data->player.controller)) {
                        if(!log) {
                           if(!cr) output(p,player,0,1,0,""), cr = 1;
                           output(p,player,0,1,2,ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED"'s %s is invalid.  [CORRECTED]",Article(i,UPPER,INDEFINITE),getcname(player,i,1,0),(Married(i) || Engaged(i)) ? "partner":"controller");
			}
                        writelog(SANITY_LOG,0,"SANITY","%s(#%d)'s %s is invalid.  [CORRECTED]",getname(i),i,(Married(i) || Engaged(i)) ? "partner":"controller");

                        if(Married(i) || Engaged(i)) for(chk = 0; chk < db_top; chk++)
                           if((Typeof(chk) == TYPE_CHARACTER) && (Married(chk) || Engaged(chk)) && (db[chk].data->player.controller == i)) {
                               db[chk].data->player.controller = chk;
                               db[chk].flags                  &= ~(MARRIED|ENGAGED);
			    }

                        db[i].data->player.controller = i;
                        db[i].flags &= ~MARRIED, corrupt = 1;
		     } else if(Married(i) || Engaged(i)) for(chk = 0; chk < db_top; chk++)
                        if((Typeof(chk) == TYPE_CHARACTER) && (chk != i) && (chk != Partner(i)) && (Married(chk) || Engaged(chk)) && (db[chk].data->player.controller == i)) {
                           db[chk].data->player.controller = chk;
                           db[chk].flags &= ~(MARRIED|ENGAGED), corrupt = 1;
			}
                     if((Married(i) || Engaged(i)) && (Partner(i) == i)) db[i].flags &= ~(MARRIED|ENGAGED);

                     /* ---->  UID invalid (Who character is currently building as?)  <---- */
                     if(!Validchar(Uid(i))) {
                        if(!log) {
                           if(!cr) output(p,player,0,1,0,""), cr = 1;
                           output(p,player,0,1,2,ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED"'s UID (Who they are building as) is invalid.  [CORRECTED]",Article(i,UPPER,INDEFINITE),getcname(player,i,1,0));
			}
                        writelog(SANITY_LOG,0,"SANITY","%s(#%d)'s UID (Who they are building as) is invalid.  [CORRECTED]",getname(i),i);
                        db[i].data->player.uid = i, corrupt = 1;
		     } else if(privilege(i,255) > privilege(Uid(i),255)) {
                        if(!log) {
                           if(!cr) output(p,player,0,1,0,""), cr = 1;
                           output(p,player,0,1,2,ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED"'s UID (Who they are building as) is set to a higher level character.  [CORRECTED]",Article(i,UPPER,INDEFINITE),getcname(player,i,1,0));
			}
                        writelog(SANITY_LOG,0,"SANITY","%s(#%d)'s UID (Who they are building as) is set to a higher level character (%s(#%d).)  [CORRECTED]",getname(i),i,getname(Uid(i)),Uid(i));
                        db[i].data->player.uid = i, corrupt = 1;
		     }

                     /* ---->  Last time connected > current time  <---- */
                     if(db[i].data->player.lasttime > start) {
                        if(!log) {
                           if(!cr) output(p,player,0,1,0,""), cr = 1;
                           output(p,player,0,1,2,ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED"'s last time connected is invalid.  [CORRECTED]",Article(i,UPPER,INDEFINITE),getcname(player,i,1,0));
			}
                        writelog(SANITY_LOG,0,"SANITY","%s(#%d)'s last time connected is invalid.  [CORRECTED]",getname(i),i);
                        db[i].data->player.lasttime = start, corrupt = 1;
		     }

                     /* ---->  Mail redirect  <---- */
                     for(original = parent = i; (original != NOTHING) && Valid(parent); parent = (Typeof(parent) == TYPE_CHARACTER) ? db[parent].data->player.redirect:NOTHING) {
                         if(!Valid(db[parent].data->player.redirect) || (Typeof(db[parent].data->player.redirect) != TYPE_CHARACTER) || (db[parent].data->player.redirect == original) || (db[parent].data->player.redirect == parent))
                            if(db[parent].data->player.redirect != NOTHING) {
                               if(!log) {
                                  if(!cr) output(p,player,0,1,0,""), cr = 1;
                                  output(p,player,0,1,2,ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED"'s mail redirect chain is invalid.  [CORRECTED]",Article(i,UPPER,INDEFINITE),getcname(player,i,1,0));
			       }
                               writelog(SANITY_LOG,0,"SANITY","%s(#%d)'s mail redirect chain is invalid.  [CORRECTED]",getname(i),i);
                               db[parent].data->player.redirect = original = NOTHING;
                               corrupt = 1;
			    }

                         for(chk = original; Valid(chk) && (chk != parent) && Valid(original); chk = db[chk].data->player.redirect)
                             if(chk == db[parent].data->player.redirect) {
                                if(!log) {
                                   if(!cr) output(p,player,0,1,0,""), cr = 1;
                                   output(p,player,0,1,2,ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED"'s mail redirect chain is invalid.  [CORRECTED]",Article(i,UPPER,INDEFINITE),getcname(player,i,1,0));
				}
                                writelog(SANITY_LOG,0,"SANITY","%s(#%d)'s mail redirect chain is invalid.  [CORRECTED]",getname(i),i);
                                db[parent].data->player.redirect = original = NOTHING;
                                corrupt = 1;
			     }
		     }

                     /* ---->  Privileges given out via friend flags  <---- */
                     for(friend = db[i].data->player.friends; friend; friend = friend->next)
                         if(Validchar(friend->friend))
                            if(privilege(friend->friend,255) > privilege(i,255)) {
                               if(friend->flags & FRIEND_WRITE) {
                                  if(!log) {
                                     if(!cr) output(p,player,0,1,0,""), cr = 1;
                                     sprintf(scratch_return_string,ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED" has 'write' privileges (Via friend flags) to ",Article(friend->friend,UPPER,INDEFINITE),getcname(player,friend->friend,1,0));
                                     output(p,player,0,1,2,"%s"ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED", but is of a lower level than %s.  [CORRECTED]",Article(i,UPPER,INDEFINITE),getcname(player,i,1,0),Objective(i,0));
				  }
                                  writelog(SANITY_LOG,0,"SANITY","%s(#%d) has 'write' permission (Via friend flags) to %s(#%d), but is of a lower level than %s.  [CORRECTED]",getname(friend->friend),friend->friend,getname(i),i,Objective(i,0));
			       }
                               friend->flags &= ~(FRIEND_COMMANDS|FRIEND_CREATE|FRIEND_DESTROY|FRIEND_WRITE);
                               if(Uid(friend->friend) == player)
                                  db[friend->friend].data->player.uid = friend->friend;
			    }

                     /* ---->  Miscellaneous checks  <---- */
                     if(Druid(i) && ((!Retired(i) && !Level4(i)) || Level1(i)))
                        db[i].flags &= ~DRUID;
                     break;
                case TYPE_PROPERTY:
                case TYPE_VARIABLE:
                case TYPE_COMMAND:
                case TYPE_ALARM:
                case TYPE_ARRAY:
                case TYPE_THING:
                case TYPE_EXIT:
                case TYPE_FUSE:
                case TYPE_ROOM:

                     /* ---->  CSUCC/CFAIL  <---- */
                     original = 0;
                     if((Typeof(i) == TYPE_COMMAND) || (Typeof(i) == TYPE_FUSE)) {
                        if(!Valid(db[i].exits) || (Typeof(db[i].exits) != TYPE_COMMAND))
                           if((db[i].exits != NOTHING) && (db[i].exits != HOME)) {
                              db[i].exits = NOTHING, corrupt = 1;
                              if(!log) {
                                 if(!cr) output(p,player,0,1,0,""), cr = 1;
                                 output(p,player,0,1,2,ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED"'s cfailure branch is invalid.  [CORRECTED]",Article(i,UPPER,INDEFINITE),getcname(player,i,1,0));
			      }
                              writelog(SANITY_LOG,0,"SANITY","%s(#%d)'s cfailure branch is invalid.  [CORRECTED]",getname(i),i);
			   }

                        if(!Valid(db[i].contents) || (Typeof(db[i].contents) != TYPE_COMMAND))
                           if((db[i].contents != NOTHING) && (db[i].contents != HOME))
                              db[i].contents = NOTHING, original = 1, corrupt = 1;
		     } else if(Typeof(i) == TYPE_ALARM)
                        if(!Valid(db[i].destination) || (Typeof(db[i].destination) != TYPE_COMMAND))
                           if((db[i].destination != NOTHING) && (db[i].destination != HOME))
                              db[i].destination = NOTHING, original = 1, corrupt = 1;

                     if(original) {
                        if(!log) {
                           if(!cr) output(p,player,0,1,0,""), cr = 1;
                           output(p,player,0,1,2,ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED"'s csuccess branch is invalid.  [CORRECTED]",Article(i,UPPER,INDEFINITE),getcname(player,i,1,0));
			}
                        writelog(SANITY_LOG,0,"SANITY","%s(#%d)'s csuccess branch is invalid.  [CORRECTED]",getname(i),i);
		     }

                     if(!Validchar(db[i].owner)) {
                        if(!log) {
                           if(!cr) output(p,player,0,1,0,""), cr = 1;
                           output(p,player,0,1,2,ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED"'s owner is invalid  -  Object set "ANSI_LYELLOW"ASHCAN"ANSI_LRED", "ANSI_LYELLOW"INVISIBLE"ANSI_LRED" and owner set to "ANSI_LWHITE"%s(#%d)"ANSI_LRED".",Article(i,UPPER,INDEFINITE),unparse_object(player,i,0),getname(newowner),newowner);
			}
                        writelog(SANITY_LOG,0,"SANITY","%s(#%d)'s owner is invalid  -  Object set ASHCAN, INVISIBLE and owner set to %s(#%d).",getname(i),i,getname(newowner),newowner);
                        db[i].flags |= ASHCAN|INVISIBLE, db[i].owner = newowner;
                        corrupt = 1;
		     }
                     break;
                case TYPE_FREE:
                     break;
                default:
                     if(!log) {
                        if(!cr) output(p,player,0,1,0,""), cr = 1;
                        output(p,player,0,1,2,ANSI_LWHITE"%s(#%d)"ANSI_LRED"'s object type ("ANSI_LYELLOW"0x%0X"ANSI_LRED") is unknown  -  Object set "ANSI_LYELLOW"ASHCAN"ANSI_LRED".",getname(i),i,Typeof(i));
		     }
                     writelog(SANITY_LOG,0,"SANITY","%s(#%d)'s object type (0x%0X) is unknown  -  Object set ASHCAN.",getname(i),i,Typeof(i));
                     db[i].flags |= ASHCAN, uncorrectable++, corrupt = 1;
	 }

         /* ---->  Home of object/Exit's destination/Drop-to of room  <---- */
         if((Typeof(i) == TYPE_CHARACTER) || (Typeof(i) == TYPE_THING) || (Typeof(i) == TYPE_EXIT) || (Typeof(i) == TYPE_ROOM))
            if(!Valid(db[i].destination) || (Typeof(db[i].destination) == TYPE_FREE) || (db[i].destination == i) || !((Typeof(db[i].destination) == TYPE_ROOM) || (Typeof(db[i].destination) == TYPE_THING)))
               if((db[i].destination != NOTHING) && !((Typeof(i) == TYPE_THING) && (Typeof(db[i].destination) == TYPE_CHARACTER))) {
                  db[i].destination = ((Typeof(i) == TYPE_EXIT) || (Typeof(i) == TYPE_ROOM)) ? NOTHING:ROOMZERO, corrupt = 1;
                  if(!log && !cr) output(p,player,0,1,0,""), cr = 1;
                  if(Typeof(i) == TYPE_EXIT) {
                     if(!log) output(p,player,0,1,2,ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED"'s destination is invalid.  [CORRECTED]",Article(i,UPPER,INDEFINITE),getcname(player,i,1,0));
                     writelog(SANITY_LOG,0,"SANITY","%s(#%d)'s destination is invalid.  [CORRECTED]",getname(i),i);
		  } else if(Typeof(i) == TYPE_ROOM) {
                     if(!log) output(p,player,0,1,2,ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED"'s drop-to is invalid.  [CORRECTED]",Article(i,UPPER,INDEFINITE),getcname(player,i,1,0));
                     writelog(SANITY_LOG,0,"SANITY","%s(#%d)'s drop-to is invalid.  [CORRECTED]",getname(i),i);
		  } else {
                     if(!log) output(p,player,0,1,2,ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED"'s home is invalid.  [CORRECTED]",Article(i,UPPER,INDEFINITE),getcname(player,i,1,0));
                     writelog(SANITY_LOG,0,"SANITY","%s(#%d)'s home is invalid.  [CORRECTED]",getname(i),i);
		  }
	       }

         /* ---->  Location of object  <---- */
         if(!Valid(db[i].location) || (Typeof(db[i].location) == TYPE_FREE) || (db[i].location == i))
            if((db[i].location != NOTHING) || (Typeof(i) == TYPE_CHARACTER)) {
               db[i].location = ROOMZERO;
               PUSH(i,db[ROOMZERO].contents);
               corrupt = 1;
               if(!log) {
                  if(!cr) output(p,player,0,1,0,""), cr = 1;
                  output(p,player,0,1,2,ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED"'s current location is invalid.  [CORRECTED]",Article(i,UPPER,INDEFINITE),getcname(player,i,1,0));
	       }
	       writelog(SANITY_LOG,0,"SANITY","%s(#%d)'s current location is invalid.  [CORRECTED]",getname(i),i);
	    }

         /* ---->  Parent chain of object  <---- */
         for(original = parent = i; (original != NOTHING) && Valid(parent); parent = db[parent].parent) {
            if(!Valid(db[parent].parent) || (Typeof(db[parent].parent) == TYPE_FREE) || (Typeof(db[parent].parent) != Typeof(original)) || (db[parent].parent == original) || (db[parent].parent == parent))
               if(db[parent].parent != NOTHING) {
                  if(!log) {
                     if(!cr) output(p,player,0,1,0,""), cr = 1;
                     output(p,player,0,1,2,ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED"'s parent chain is invalid.  [CORRECTED]",Article(i,UPPER,INDEFINITE),getcname(player,i,1,0));
		  }
	          writelog(SANITY_LOG,0,"SANITY","%s(#%d)'s parent chain is invalid.  [CORRECTED]",getname(i),i);
                  db[parent].parent = original = NOTHING, corrupt = 1;
	       }

            for(chk = original; Valid(chk) && (chk != parent) && Valid(original); chk = db[chk].parent)
               if(chk == db[parent].parent) {
                  if(!log) {
                     if(!cr) output(p,player,0,1,0,""), cr = 1;
                     output(p,player,0,1,2,ANSI_LRED"%s"ANSI_LWHITE"%s"ANSI_LRED"'s parent chain is invalid.  [CORRECTED]",Article(i,UPPER,INDEFINITE),getcname(player,i,1,0));
		  }
	          writelog(SANITY_LOG,0,"SANITY","%s(#%d)'s parent chain is invalid.  [CORRECTED]",getname(i),i);
                  db[parent].parent = original = NOTHING, corrupt = 1;
	       }
	 }
         if(corrupt) found++;
     }

     /* ---->  Sanitise BBS messages/topics/sub-topics  <---- */
     for(topic = bbs; topic; topic = topic->next) {

         /* ---->  Topic  <---- */
         if(!Validchar(topic->owner)) {
            if(!log) {
               if(!cr) output(p,player,0,1,0,""), cr = 1;
               output(p,player,0,1,2,ANSI_LRED"Owner of BBS topic '"ANSI_LYELLOW"%s"ANSI_LRED"' is invalid  -  Owner set to "ANSI_LWHITE"%s(#%d)"ANSI_LRED".",topic->name,getname(newowner),newowner);
	    }
            writelog(SANITY_LOG,0,"SANITY","Owner of BBS topic '%s' is invalid  -  Owner set to %s(#%d).",topic->name,getname(newowner),newowner);
            topic->owner = newowner, found++;
	 }

         /* ---->  Topic's messages  <---- */
         for(message = topic->messages; message; message = message->next)
             if(!Validchar(message->owner)) {
                if(!log) {
                   if(!cr) output(p,player,0,1,0,""), cr = 1;
                   output(p,player,0,1,2,ANSI_LRED"Owner of BBS message '%s"ANSI_LYELLOW"%s"ANSI_LRED"' in the topic '"ANSI_LYELLOW"%s"ANSI_LRED"' is invalid  -  Owner set to "ANSI_LWHITE"%s(#%d)"ANSI_LRED".",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",substitute(player,scratch_return_string,decompress(message->subject),0,ANSI_LYELLOW,NULL,0),topic->name,getname(newowner),newowner);
		}
                writelog(SANITY_LOG,0,"SANITY","Owner of BBS message '%s%s' in the topic '%s' is invalid  -  Owner set to %s(#%d).",(message->flags & MESSAGE_REPLY) ? "Re:  ":"",decompress(message->subject),topic->name,getname(newowner),newowner);
                message->owner = newowner, found++;
	     }

         /* ---->  Sub-topics  <---- */
         if(topic->subtopics)
            for(subtopic = topic->subtopics; subtopic; subtopic = subtopic->next) {
                if(!Validchar(subtopic->owner)) {
                   if(!log) {
                      if(!cr) output(p,player,0,1,0,""), cr = 1;
                      output(p,player,0,1,2,ANSI_LRED"Owner of BBS sub-topic '"ANSI_LYELLOW"%s/%s"ANSI_LRED"' is invalid  -  Owner set to "ANSI_LWHITE"%s(#%d)"ANSI_LRED".",topic->name,subtopic->name,getname(newowner),newowner);
		   }
                   writelog(SANITY_LOG,0,"SANITY","Owner of BBS sub-topic '%s/%s' is invalid  -  Owner set to %s(#%d).",topic->name,subtopic->name,getname(newowner),newowner);
                   subtopic->owner = newowner, found++;
		}

                /* ---->  Messages in sub-topic  <---- */
                for(message = subtopic->messages; message; message = message->next)
                    if(!Validchar(message->owner)) {
                       if(!log) {
                          if(!cr) output(p,player,0,1,0,""), cr = 1;
                          output(p,player,0,1,2,ANSI_LRED"Owner of BBS message '%s"ANSI_LYELLOW"%s"ANSI_LRED"' in the sub-topic '"ANSI_LYELLOW"%s/%s"ANSI_LRED"' is invalid  -  Owner set to "ANSI_LWHITE"%s(#%d)"ANSI_LRED".",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",substitute(player,scratch_return_string,decompress(message->subject),0,ANSI_LYELLOW,NULL,0),topic->name,subtopic->name,getname(newowner),newowner);
		       }
                       writelog(SANITY_LOG,0,"SANITY","Owner of BBS message '%s%s' in the sub-topic '%s/%s' is invalid  -  Owner set to %s(#%d).",(message->flags & MESSAGE_REPLY) ? "Re:  ":"",decompress(message->subject),topic->name,subtopic->name,getname(newowner),newowner);
                       message->owner = newowner, found++;
		    }
	    }
     }

     /* ---->  Bank access room '@admin' setting  <---- */
     if((bankroom != NOTHING) && (!Valid(bankroom) || (Typeof(bankroom) != TYPE_ROOM))) {
        if(!log) {
           if(!cr) output(p,player,0,1,0,""), cr = 1;
           output(p,player,0,1,2,ANSI_LRED"Bank access room is invalid  -  Reset to "NOTHING_STRING".  [CORRECTED]");
	}
        writelog(SANITY_LOG,0,"SANITY","Bank access room is invalid  -  Reset to "NOTHING_STRING".  [CORRECTED]");
        bankroom = NOTHING, found++;
     }

     /* ---->  BBS access room '@admin' setting  <---- */
     if((bbsroom != NOTHING) && (!Valid(bbsroom) || (Typeof(bbsroom) != TYPE_ROOM))) {
        if(!log) {
           if(!cr) output(p,player,0,1,0,""), cr = 1;
           output(p,player,0,1,2,ANSI_LRED"%s BBS access room is invalid  -  Reset to "NOTHING_STRING".  [CORRECTED]",tcz_full_name);
	}
        writelog(SANITY_LOG,0,"SANITY","%s BBS access room is invalid  -  Reset to "NOTHING_STRING".  [CORRECTED]",tcz_full_name);
        bbsroom = NOTHING, found++;
     }

     /* ---->  Maintenance owner '@admin' setting  <---- */
     if((maint_owner != NOTHING) && !Validchar(maint_owner)) {
        if(!log) {
           if(!cr) output(p,player,0,1,0,""), cr = 1;
           output(p,player,0,1,2,ANSI_LRED"Maintenance owner is invalid  -  Reset to %s(#%d).  [CORRECTED]",getname(ROOT),ROOT);
	}
        writelog(SANITY_LOG,0,"SANITY","Maintenance owner is invalid  -  Reset to %s(#%d).  [CORRECTED]",getname(ROOT),ROOT);
        maint_owner = ROOT, found++;
     }

     /* ---->  Global aliases owner '@admin' setting  <---- */
     if((aliases != NOTHING) && !Validchar(aliases)) {
        if(!log) {
           if(!cr) output(p,player,0,1,0,""), cr = 1;
           output(p,player,0,1,2,ANSI_LRED"Global aliases owner is invalid  -  Reset.  [CORRECTED]");
	}
        writelog(SANITY_LOG,0,"SANITY","Global aliases owner is invalid  -  Reset.  [CORRECTED]");
        aliases = NOTHING, found++;
     }

     gettime(finish);
     if(found > 0) {
        if(!log) {
           if(cr) output(p,player,0,1,0,"");
	   output(p,player,0,1,10,ANSI_LGREEN"[SANITY] \016&nbsp;\016 "ANSI_LYELLOW"%d"ANSI_LWHITE" corrupted object%s detected in the database ("ANSI_LYELLOW"%d"ANSI_LWHITE" corrected, "ANSI_LYELLOW"%d"ANSI_LWHITE" couldn't be fixed  -  Check took "ANSI_LCYAN"%s"ANSI_LWHITE".)%s",found,Plural(found),found - uncorrectable,uncorrectable,interval(finish - start,1,ENTITIES,0),(cr) ? "\n":"");
        } else writelog(SERVER_LOG,0,"RESTART","%d corrupted object%s detected in the database (%d corrected, %d couldn't be fixed  -  Check took %s.)",found,Plural(found),found - uncorrectable,uncorrectable,interval(finish - start,1,ENTITIES,0));
        writelog(SANITY_LOG,0,"SANITY","%d corrupted object%s detected in the database (%d corrected, %d couldn't be fixed  -  Check took %s.)",found,Plural(found),found - uncorrectable,uncorrectable,interval(finish - start,1,ENTITIES,0));
     } else {
        if(!log) {
           if(cr) output(p,player,0,1,0,"");
           output(p,player,0,1,10,ANSI_LGREEN"[SANITY] \016&nbsp;\016 "ANSI_LWHITE"No corrupted objects detected in the database (Check took "ANSI_LCYAN"%s"ANSI_LWHITE".)%s",interval(finish - start,1,ENTITIES,0),(cr) ? "\n":"");
	}
        writelog(SANITY_LOG,0,"SANITY","No corrupted objects detected in the database (Check took %s.)",interval(finish - start,1,ENTITIES,0));
     }
}

/* ---->  Check lists of objects  <---- */
int sanity_checklists(dbref player)
{
    struct   descriptor_data *p = getdsc(player);
    int                      error_count = 0;
    time_t                   start,finish;
    unsigned char            cr = 0,error;
    dbref                    ptr,ptr2;

    gettime(start);
    if(!in_command && p && !p->pager && !IsHtml(p) && More(player)) pager_init(p);
    writelog(SANITY_LOG,1,"SANITY","General list check started on database by %s(#%d)...",getname(player),player);
    writelog(ADMIN_LOG,1,"SANITY","General list check started on database by %s(#%d).",getname(player),player);

    for(ptr = 0; ptr < db_top; ptr++)
        if(Valid(db[ptr].location) && HasList(db[ptr].location,WhichList(ptr)) && (Typeof(ptr) != TYPE_FREE)) {
           error = 0;
           switch(WhichList(ptr)) {
                  case VARIABLES:
                       if(!member(ptr,db[db[ptr].location].variables)) error = 1;
                       break;
                  case COMMANDS:
                       if(!member(ptr,db[db[ptr].location].commands)) error = 1;
                       break;
                  case CONTENTS:
                       if(!member(ptr,db[db[ptr].location].contents)) error = 1;
                       break;
                  case EXITS:
                       if(!member(ptr,db[db[ptr].location].exits)) error = 1;
                       break;
                  case FUSES:
                       if(!member(ptr,db[db[ptr].location].fuses)) error = 1;
                       break;
                  default:
                       error = 1;
	   }

           if(error) {
              output(p,player,0,1,0,""), cr = 1;
              sprintf(scratch_return_string,ANSI_LRED"Object %s"ANSI_LWHITE"%s"ANSI_LRED" should be in the %s list of ",Article(ptr,LOWER,INDEFINITE),unparse_object(player,ptr,0),list_type(WhichList(ptr)));
              output(p,player,0,1,2,"%s%s"ANSI_LWHITE"%s"ANSI_LRED".  [IGNORED]",scratch_return_string,Article(db[ptr].location,LOWER,INDEFINITE),unparse_object(player,db[ptr].location,0));
              writelog(SANITY_LOG,0,"SANITY","Object %s(#%d) should be in the %s list of %s(#%d).  [IGNORED]",getname(ptr),ptr,list_type(WhichList(ptr)),getname(db[ptr].location),db[ptr].location);

              /* ---->  List any objects with db[x].next pointer pointing to corrupted entry  <---- */
              for(ptr2 = 0; ptr2 < db_top; ptr2++)
                  if((Typeof(ptr2) != TYPE_FREE) && (db[ptr2].next == ptr)) {
                     if(!cr) output(p,player,0,1,0,""), cr = 1;
                     output(p,player,0,1,4,"  "ANSI_LYELLOW"%s"ANSI_LWHITE"%s"ANSI_LYELLOW"'s next pointer points to this object.  [IGNORED]",Article(ptr2,UPPER,INDEFINITE),unparse_object(player,ptr2,0));
                     writelog(SANITY_LOG,0,"SANITY","-  %s(#%d)'s next pointer points to %s(#%d).  [IGNORED]",getname(ptr2),ptr2,getname(ptr),ptr);
		  }
              error_count++;
	   }
	} else if((Typeof(ptr) != TYPE_FREE) && Valid(db[ptr].location)) {
           output(p,player,0,1,0,""), cr = 1;
           sprintf(scratch_return_string,ANSI_LRED"Object %s"ANSI_LWHITE"%s"ANSI_LRED" should be in the %s list of ",Article(ptr,LOWER,INDEFINITE),unparse_object(player,ptr,0),list_type(WhichList(ptr)));
           output(p,player,0,1,2,"%s%s"ANSI_LWHITE"%s"ANSI_LRED", but this object doesn't have a list of that type.  [IGNORED]",scratch_return_string,Article(db[ptr].location,LOWER,INDEFINITE),unparse_object(player,db[ptr].location,0));
           writelog(SANITY_LOG,0,"SANITY","Object %s(#%d) should be in the %s list of %s(#%d), but this object doesn't have a list of that type.  [IGNORED]",getname(ptr),ptr,list_type(WhichList(ptr)),getname(db[ptr].location),db[ptr].location);
           error_count++;
	}

    gettime(finish);
    if(cr) output(p,player,0,1,0,"");
    if(error_count) {
       output(p,player,0,1,10,ANSI_LGREEN"[SANITY] \016&nbsp;\016 "ANSI_LYELLOW"%d"ANSI_LWHITE" possibly corrupted list%s detected (Check took "ANSI_LCYAN"%s"ANSI_LWHITE".)%s",error_count,Plural(error_count),interval(finish - start,1,ENTITIES,0),(cr) ? "\n":"");
       writelog(SANITY_LOG,0,"SANITY","%d possibly corrupted list%s detected (Check took %s.)",error_count,Plural(error_count),interval(finish - start,1,ENTITIES,0));
    } else {
       output(p,player,0,1,10,ANSI_LGREEN"[SANITY] \016&nbsp;\016 "ANSI_LWHITE"All lists appear to be intact (No possible corruption detected  -  Check took "ANSI_LCYAN"%s"ANSI_LWHITE".)%s",interval(finish - start,1,ENTITIES,0),(cr) ? "\n":"");
       writelog(SANITY_LOG,0,"SANITY","All lists appear to be intact (No possible corruption detected  -  Check took %s.)",interval(finish - start,1,ENTITIES,0));
    }
    return(error_count);
}

/* ---->  Sanitise linked lists of objects  <---- */
int sanity_fixlists(dbref player,unsigned char log)
{
    time_t start,finish;
    int    fixed = 0;
    dbref  ptr;

    /* ---->  First go through DB and remove OBJECT flag from all objects that are in wrong linked lists (Remove object at same time too)  <---- */
    gettime(start);
    writelog(SANITY_LOG,1,"SANITY","Extensive list check started on database by %s(#%d)...",getname(player),player);
    if(!log) writelog(ADMIN_LOG,1,"SANITY","Extensive list check started on database by %s(#%d).",getname(player),player);
    for(ptr = 0; ptr < db_top; ptr++)
        if(Typeof(ptr) != TYPE_FREE) {
           if(HasList(ptr,VARIABLES)) db[ptr].variables = sanity_marklist(db[ptr].variables,ptr);
           if(HasList(ptr,COMMANDS))  db[ptr].commands  = sanity_marklist(db[ptr].commands,ptr);
           if(HasList(ptr,CONTENTS))  db[ptr].contents  = sanity_marklist(db[ptr].contents,ptr);
           if(HasList(ptr,EXITS))     db[ptr].exits     = sanity_marklist(db[ptr].exits,ptr);
           if(HasList(ptr,FUSES))     db[ptr].fuses     = sanity_marklist(db[ptr].fuses,ptr);
	} else {

           /* ---->  Free object:  Set all pointers to NOTHING for sanity  <---- */
           db[ptr].variables = NOTHING;
           db[ptr].location  = NOTHING;
           db[ptr].commands  = NOTHING;
           db[ptr].contents  = NOTHING;
           db[ptr].parent    = NOTHING;
           db[ptr].exits     = NOTHING;
           db[ptr].fuses     = NOTHING;
           db[ptr].next      = NOTHING;
	}

    /* ---->  Next, search for any objects with OBJECT flag still set that aren't in the appropriate list of the object they're attached to  <---- */
    for(ptr = 0; ptr < db_top; ptr++)
        if((db[ptr].flags & OBJECT) && Valid(db[ptr].location) && HasList(db[ptr].location,WhichList(ptr)) && (Typeof(ptr) != TYPE_FREE)) {
           switch(WhichList(ptr)) {
                  case VARIABLES:
                       if(!member(ptr,db[db[ptr].location].variables)) db[ptr].flags &= ~OBJECT;
                       break;
                  case COMMANDS:
                       if(!member(ptr,db[db[ptr].location].commands)) db[ptr].flags &= ~OBJECT;
                       break;
                  case CONTENTS:
                       if(!member(ptr,db[db[ptr].location].contents)) db[ptr].flags &= ~OBJECT;
                       break;
                  case EXITS:
                       if(!member(ptr,db[db[ptr].location].exits)) db[ptr].flags &= ~OBJECT;
                       break;
                  case FUSES:
                       if(!member(ptr,db[db[ptr].location].fuses)) db[ptr].flags &= ~OBJECT;
                       break;
                  default:
                       db[ptr].flags &= ~OBJECT;
	   }
	} else if(db[ptr].flags & OBJECT) db[ptr].location = NOTHING;

    /* ---->  Count objects that will be fixed  <---- */
    for(ptr = 0; ptr < db_top; ptr++)
        if(!(db[ptr].flags & OBJECT))
           if(Typeof(ptr) != TYPE_FREE) 
              fixed++;

    /* ---->  Now go through DB and put all objects without OBJECT flag back in the lists they should be in  <---- */
    for(ptr = 0; ptr < db_top; ptr++)
        if(!(db[ptr].flags & OBJECT) && Valid(db[ptr].location) && (Typeof(ptr) != TYPE_FREE)) {
           if(HasList(db[ptr].location,WhichList(ptr))) {
              switch(WhichList(ptr)) {
                     case VARIABLES:
                          if(!member(ptr,db[db[ptr].location].variables))
                             PUSH(ptr,db[db[ptr].location].variables);
                          break;
                     case COMMANDS:
                          if(!member(ptr,db[db[ptr].location].commands)) {
                             PUSH(ptr,db[db[ptr].location].commands);
                             if(Global(effective_location(ptr))) {
                                global_delete(ptr);
                                global_add(ptr);
			     }
			  }
                          break;
                     case CONTENTS:
                          if(!member(ptr,db[db[ptr].location].contents))
                             PUSH(ptr,db[db[ptr].location].contents);
                          break;
                     case EXITS:
                          if(!member(ptr,db[db[ptr].location].exits))
                             PUSH(ptr,db[db[ptr].location].exits);
                          break;
                     case FUSES:
                          if(!member(ptr,db[db[ptr].location].fuses))
                             PUSH(ptr,db[db[ptr].location].fuses);
                          break;
                     default:
                          db[ptr].location = NOTHING;
	      }
              db[ptr].flags |= OBJECT;
	   } else {
              db[ptr].location = NOTHING;
              db[ptr].flags   |= OBJECT;
	   }
	} else if(!(db[ptr].flags & OBJECT)) {
           if(Typeof(ptr) != TYPE_FREE) db[ptr].flags |= OBJECT;
           db[ptr].location = NOTHING;
	}

    /* ---->  Reconstruct list of free objects  <---- */
    db_free_chain     = NOTHING;
    db_free_chain_end = NOTHING;
    for(ptr = 0; ptr < db_top; ptr++)
        if(Typeof(ptr) == TYPE_FREE) {
           if(db_free_chain == NOTHING) db_free_chain = ptr;
              else db[db_free_chain_end].next = ptr;
           db_free_chain_end = ptr;
           db[ptr].next = NOTHING;
	}

    gettime(finish);
    if(player != NOTHING) {
       if(fixed > 0) {
          if(!log) output(getdsc(player),player,0,1,10,ANSI_LGREEN"[SANITY] \016&nbsp;\016 "ANSI_LYELLOW"%d"ANSI_LWHITE" possibly corrupted list%s corrected (Check took "ANSI_LCYAN"%s"ANSI_LWHITE".)",fixed,Plural(fixed),interval(finish - start,1,ENTITIES,0));
	     else writelog(SERVER_LOG,0,"RESTART","%d possibly corrupted list%s corrected (Check took %s.)",fixed,Plural(fixed),interval(finish - start,1,ENTITIES,0));
          writelog(SANITY_LOG,0,"SANITY","%d possibly corrupted list%s corrected (Check took %s.)",fixed,Plural(fixed),interval(finish - start,1,ENTITIES,0));
       } else {
          if(!log) output(getdsc(player),player,0,1,10,ANSI_LGREEN"[SANITY] \016&nbsp;\016 "ANSI_LWHITE"No corrupted lists were detected (Check took "ANSI_LCYAN"%s"ANSI_LWHITE".)",interval(finish - start,1,ENTITIES,0));
          writelog(SANITY_LOG,0,"SANITY","No corrupted lists were detected (Check took %s.)",interval(finish - start,1,ENTITIES,0));
       }
    }
    return(fixed);
}

/* ---->  Perform santity checks on the database (Some of these are now fully automatic)  <---- */
void sanity_main(CONTEXT)
{
     setreturn(ERROR,COMMAND_FAIL);
     if(!in_command) {
        if(Level4(player)) {
           if(!Blank(arg1)) {
              if(string_prefix("fixlists",arg1) || string_prefix("fixlinkedlists",arg1) || string_prefix("extensivelistcheck",arg1)) {
                 if(Level1(player)) {
                    if(!strcasecmp("yes",arg2)) {
                       sanity_fixlists(player,0);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the extensive list check is *extremely* intensive and will cause excessive lag for a long period of time.  To perform this check (Which is done automatically by the server on start-up), please type '"ANSI_LYELLOW"@sanity fixlists = yes"ANSI_LGREEN"'.");
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Deities and the Supreme Being may perform the extensive list check.");
                 return;
	      } else if(string_prefix("checklists",arg1) || string_prefix("checklinkedlists",arg1) || string_prefix("lists",arg1) || string_prefix("linkedlists",arg1)) {
                 if(Level2(player)) {
                    if(!strcasecmp("yes",arg2)) {
                       sanity_checklists(player);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the general list check is fairly intensive and may cause excessive lag.  To perform this check, please type '"ANSI_LYELLOW"@sanity checklists = yes"ANSI_LGREEN"'.");
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Elder Wizards/Druids and above may perform the general list check.");
                 return;
	      } else if(string_prefix("general",arg1) || string_prefix("generalcheck",arg1) || string_prefix("general check",arg1)) {
                 if(!strcasecmp("yes",arg2)) {
                    sanity_general(player,0);
                    setreturn(OK,COMMAND_SUCC);
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the general sanity check is reasonably intensive and may cause lag.  To perform this check (Which is done automatically by the server on start-up), please type '"ANSI_LYELLOW"@sanity general = yes"ANSI_LGREEN"'.");
                 return;
	      } else if(!(string_prefix("checkquotas",arg1) || string_prefix("check quotas",arg1))) {
	         if(!(string_prefix("quotalimits",arg1) || string_prefix("quota limits",arg1))) {
  	            if(!(string_prefix("credits",arg1) || string_prefix("money",arg1) || string_prefix("cash",arg1))) {
	               if(!(string_prefix("score",arg1) || string_prefix("score points",arg1) || string_prefix("scorepoints",arg1))) {
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LWHITE"general"ANSI_LGREEN"', '"ANSI_LWHITE"checklists"ANSI_LGREEN"' or '"ANSI_LWHITE"fixlists"ANSI_LGREEN"'.");
                          return;
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the '"ANSI_LWHITE"@sanity score"ANSI_LGREEN"' command is obsolete.  Please use '"ANSI_LWHITE"@rank score"ANSI_LGREEN"' to rank users by their score.");
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the '"ANSI_LWHITE"@sanity credits"ANSI_LGREEN"' command is obsolete.  Please use '"ANSI_LWHITE"@rank credit"ANSI_LGREEN"' and '"ANSI_LWHITE"@rank balance"ANSI_LGREEN"' to rank users by credit in pocket or bank balance.");
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the '"ANSI_LWHITE"@sanity quotalimits"ANSI_LGREEN"' command is obsolete.  Please use '"ANSI_LWHITE"@rank quotalimits"ANSI_LGREEN"' to rank users by their Building Quota limits.");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the '"ANSI_LWHITE"@sanity checkquotas"ANSI_LGREEN"' command is obsolete.  Please use '"ANSI_LWHITE"@rank excess"ANSI_LGREEN"' to rank users by exceeded Building Quota limits.");
              setreturn(OK,COMMAND_SUCC);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify which sanity check to perform.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may perform sanity checks and corrections on the database.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, sanity checks and corrections can't be performed from within a compound command.");
}

