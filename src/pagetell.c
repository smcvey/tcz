/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| PAGETELL.C  -  Implements paging/telling messages to other characters and   |
|                groups of characters, regardless of location.  Also, allows  |
|                recalling, replying to and repeating pages/tells.            |
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
| Module originally designed and written by:  J.P.Boggis 17/04/1995.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: pagetell.c,v 1.2 2005/06/29 20:18:21 tcz_monster Exp $

*/


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
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
#include "fields.h"


/* ---->  TARGETGROUP #define's  <---- */
#define  STANDARD  0
#define  FRIENDS   1
#define  ENEMIES   2
#define  ADMIN     3
#define  ADMIN_NC  4


/* ---->  Store page/tell message for later recall  <---- */
void pagetell_store_message(dbref player,dbref pager,long time,char *message,unsigned char tell)
{
     struct descriptor_data *p = getdsc(player);
     int    loop;

     if(!Connected(player) || !p) return;
     if(p->messagecount != MAX_STORED_MESSAGES) p->messagecount++;
        else FREENULL(p->messages[p->messagecount - 1].message);

     if(p->messagecount > 1)
        for(loop = p->messagecount - 2; loop >= 0; loop--) {
            p->messages[loop + 1].message = p->messages[loop].message;
            p->messages[loop + 1].pager   = p->messages[loop].pager;
            p->messages[loop + 1].time    = p->messages[loop].time;
            p->messages[loop + 1].tell    = p->messages[loop].tell;
	}

     if(!tell && (*message && (*message == '\007'))) message++;  /*  Don't want to store beep from page :)  */
     p->messages[0].message = (char *) alloc_string(compress(message,0));
     p->messages[0].pager   = pager;
     p->messages[0].time    = time;
     p->messages[0].tell    = tell;
}

/* ---->  Get DBref of <N>th person who sent a message via page/tell to given character  <---- */
dbref pagetell_recall_pager(dbref player,int number,const char *ptr,unsigned char *cr)
{
     struct descriptor_data *d;

     if(!Connected(player)) return(NOTHING);
     if(!(d = getdsc(player))) return(NOTHING);

     if(!d->messagecount) {
        output(d,player,0,1,0,"%s"ANSI_LGREEN"Sorry, no-one has sent any messages to you using either '"ANSI_LWHITE"page"ANSI_LGREEN"' or '"ANSI_LWHITE"tell"ANSI_LGREEN"' yet.\n",(*cr) ? "":"\n");
        *cr = 1;
        return(NOTHING);
     }

     if(number <= 0) {
        output(d,player,0,1,0,"%s"ANSI_LGREEN"Sorry, the message number '"ANSI_LWHITE"%s"ANSI_LGREEN"' is invalid  -  Please specify a positive number.\n",(*cr) ? "":"\n",ptr);
        *cr = 1;
        return(NOTHING);
     }

     if(number > d->messagecount) {
        output(d,player,0,1,0,"%s"ANSI_LGREEN"Sorry, you only have "ANSI_LWHITE"%d"ANSI_LGREEN" stored message%s.\n",(*cr) ? "":"\n",d->messagecount,Plural(d->messagecount));
        *cr = 1;
        return(NOTHING);
     }

     number--;
     if(!Validchar(d->messages[number].pager)) {
        output(d,player,0,1,0,"%s"ANSI_LGREEN"Sorry, the character who sent you that message no-longer exists.",(*cr) ? "":"\n");
        *cr = 1;
        return(NOTHING);
     }
     return(d->messages[number].pager);
}

/* ---->  Construct list of characters with respect to given character (For display)  <---- */
const char *pagetell_construct_list(dbref pager,dbref player,union group_data *list,int listsize,char *buffer,const char *ansi1,const char *ansi2,unsigned char sameroomonly,unsigned char targetgroup,unsigned char article_setting)
{
      static int pos,count,exclusions;
      static struct list_data *ptr;
      static int names[9];

      count      = 0;
      *buffer    = '\0';
      exclusions = 0;
      if(list) {
         for(ptr = &(list->list); ptr && (ptr->player != player); ptr = ptr->next);
         if(ptr) {
            names[0] = player;
            count++;
	 } else names[0] = NOTHING;
         if(targetgroup == FRIENDS) {
            if(player != pager) {
               if(names[0] == player) substitute(pager,buffer,"you and %p friends",0,ansi2,NULL,0);
                  else substitute(pager,buffer,"%p friends",0,ansi2,NULL,0);
               return(buffer);
	    } else return("your friends");
	 } else if(targetgroup == ENEMIES) {
            if(player != pager) {
               if(names[0] == player) substitute(pager,buffer,"you and %p enemies",0,ansi2,NULL,0);
                  else substitute(pager,buffer,"%p enemies",0,ansi2,NULL,0);
               return(buffer);
	    } else return("your enemies");
	 } else if(targetgroup == ADMIN) {
            sprintf(buffer,"all connected %s Admin",tcz_short_name);
            return(buffer);
	 } else if(targetgroup == ADMIN_NC) {
            sprintf(buffer,"all %s Admin",tcz_short_name);
            return(buffer);
	 } else {
            for(ptr = &(list->list); ptr && (count < 9); ptr = ptr->next)
                if(Validchar(ptr->player)) {
                   if((ptr->player != player) && (db[ptr->player].flags & OBJECT) && (!sameroomonly || (sameroomonly && (Location(ptr->player) == Location(pager))))) {
                      names[count] = ptr->player;
                      count++;
	           } else if(sameroomonly) exclusions++;
		} else listsize--;
            for(pos = 0; pos < count; pos++) {
                if(*buffer) sprintf(buffer + strlen(buffer),"%s%s",ansi2,((pos == (count - 1)) && ((listsize - count - exclusions) <= 0)) ? " and ":", ");
                if(names[pos] == player) strcat(buffer,(names[pos] == pager) ? "yourself":"you");
	           else sprintf(buffer + strlen(buffer),"%s%s%s",Article(names[pos],LOWER,article_setting),ansi1,getcname(NOTHING,names[pos],0,0));
	    }
            if((listsize - count - exclusions) >= 1)
               sprintf(buffer + strlen(buffer),"%s%s%d %sother%s",(*buffer) ? ansi2:ansi1,(*buffer) ? " and ":"",(listsize - count - exclusions),ansi2,Plural(listsize - count - exclusions));
            strcat(buffer,ansi2);
	 }
      }
      return(buffer);
}

/* ---->  Page or tell message to given character  <---- */
int pagetell_cansend(dbref player,dbref target,unsigned char tell,unsigned char targetgroup,time_t now,unsigned char *cr,unsigned char friends)
{
    struct descriptor_data *d = getdsc(target);
    dbref  automatic = NOTHING;
    int    flags;
    long   temp;

    /* ---->  Not connected?  <---- */
    if(!Connected(target)) {
       if((targetgroup != FRIENDS) && (targetgroup != ENEMIES)) {
          automatic = match_simple(target,".message",VARIABLES,0,0);
          if(Valid(automatic) && !Blank(getfield(automatic,OFAIL))) {
             if((temp = db[target].data->player.lasttime) == 0) temp = now;
             now -= temp;
             if(db[player].data->player.timediff) temp += (db[player].data->player.timediff * HOUR);
             sprintf(scratch_buffer,"%s"ANSI_LGREEN"Sleeping message from %s"ANSI_LWHITE"%s"ANSI_LGREEN", who hasn't connected since "ANSI_LYELLOW"%s"ANSI_LGREEN" ("ANSI_LWHITE"%s"ANSI_LGREEN" ago):  "ANSI_LWHITE"%s\n",(*cr) ? "":"\n",Article(target,LOWER,DEFINITE),getcname(NOTHING,target,0,0),date_to_string(temp,UNSET_DATE,player,FULLDATEFMT),interval(now,now,ENTITIES,0),punctuate((char *) getfield(automatic,OFAIL),2,'.'));
             output(getdsc(player),player,0,1,0,"%s",substitute(player,scratch_return_string,scratch_buffer,0,ANSI_LWHITE,NULL,0));
             *cr = 1;
          } else {
             if((temp = db[target].data->player.lasttime) == 0) temp = now;
             now -= temp;
             if(db[player].data->player.timediff) temp += (db[player].data->player.timediff * HOUR);
             output(getdsc(player),player,0,1,0,"%s"ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" hasn't connected since "ANSI_LYELLOW"%s"ANSI_LGREEN" ("ANSI_LWHITE"%s"ANSI_LGREEN" ago.)\n",(*cr) ? "":"\n",Article(target,LOWER,DEFINITE),getcname(NOTHING,target,0,0),date_to_string(temp,UNSET_DATE,player,FULLDATEFMT),interval(now,now,ENTITIES,0));
             *cr = 1;
	  }
       }
       return(0);
    }

    /* ---->  Moron (Can only page Apprentice Wizards/Druids and above)?  <---- */
    if(Moron(player) && (!tell || (Location(target) != Location(player))) && !Level4(db[target].owner)) {
       output(getdsc(player),player,0,1,0,"%s"ANSI_LGREEN"Sorry, Morons can only %s Apprentice Wizards/Druids (Type '"ANSI_LYELLOW"admin"ANSI_LGREEN"' for a list) and above long-distance messages (%s"ANSI_LWHITE"%s"ANSI_LGREEN" isn't an Apprentice Wizard/Druid or above.)\n",(*cr) ? "":"\n",(tell) ? "tell":"page",Article(target,UPPER,DEFINITE),getcname(NOTHING,target,0,0)), *cr = 1;
       return(0);
    }

    if(!Bbs(player) && !Mail(player) && instring("guest",db[player].name) && (!tell || (Location(target) != Location(player))) && !(Level4(Owner(target)) || Experienced(Owner(target)) || Assistant(Owner(target)))) {
       output(getdsc(player),player,0,1,0,"%s"ANSI_LGREEN"Sorry, Guest Characters can only %s Assistants, Experienced Builders or Apprentice Wizards/Druids and above long-distance messages (Type "ANSI_LYELLOW"ADMIN"ANSI_LGREEN" for a list.)\n",(*cr) ? "":"\n",(tell) ? "tell":"page"), *cr = 1;
       return(0);
    }

    /* ---->  AFK (Away From Keyboard)?  <---- */
    if((targetgroup != ADMIN) && !friends && d && d->afk_message) {
       sprintf(scratch_buffer,"%s"ANSI_LGREEN"AFK message from %s"ANSI_LWHITE"%s"ANSI_LGREEN", who's been away from %s keyboard for "ANSI_LYELLOW"%s"ANSI_LGREEN":  ",(*cr) ? "":"\n",Article(target,LOWER,DEFINITE),getcname(NOTHING,target,0,0),Possessive(target,0),interval(now - d->afk_time,now - d->afk_time,ENTITIES,0));
       sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LWHITE"%s\n",punctuate(d->afk_message,2,'.'));
       output(getdsc(player),player,0,1,0,"%s",substitute(player,scratch_return_string,scratch_buffer,0,ANSI_LWHITE,NULL,0)), *cr = 1;
    }

    /* ---->  Recipient reset PAGETELL friend flag on character sending message?  <---- */
    flags = friend_flags(target,player);
    if((!tell || !((Location(target) == Location(player)) && !Quiet(Location(player)))) && (flags && !(flags & FRIEND_PAGETELL))) {
       if((targetgroup != ADMIN) && !friends) {
          if(!Valid(automatic)) automatic = match_simple(target,".message",VARIABLES,0,0);
          if(Valid(automatic) && !Blank(getfield(automatic,DROP))) {
             sprintf(scratch_buffer,"%s"ANSI_LGREEN"%s-block message from %s"ANSI_LWHITE"%s"ANSI_LGREEN":  ",(*cr) ? "":"\n",(tell) ? "Tell":"Page",Article(target,LOWER,DEFINITE),getcname(NOTHING,target,0,0));
             sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LWHITE"%s\n",punctuate((char *) getfield(automatic,DROP),2,'.'));
             output(getdsc(player),player,0,1,0,"%s",substitute(player,scratch_return_string,scratch_buffer,0,ANSI_LWHITE,NULL,0)), *cr = 1;
	  } else output(getdsc(player),player,0,1,0,"%s"ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" doesn't want to receive messages from you.\n",(*cr) ? "":"\n",Article(target,LOWER,DEFINITE),getcname(NOTHING,target,0,0)), *cr = 1;
       }
       return(0);
    }

    /* ---->  Character sending message has reset PAGETELL friend flag on recipient?  <---- */
    flags = friend_flags(player,target);
    if((!tell || !((Location(target) == Location(player)) && !Quiet(Location(player)))) && (flags && !(flags & FRIEND_PAGETELL))) {
       output(getdsc(player),player,0,1,0,"%s"ANSI_LGREEN"Sorry, you have reset the "ANSI_LYELLOW"PAGETELL"ANSI_LGREEN" friend flag on %s"ANSI_LWHITE"%s"ANSI_LGREEN"  -  You can't %s a message to %s.\n",(*cr) ? "":"\n",Article(target,LOWER,DEFINITE),getcname(NOTHING,target,0,0),(tell) ? "tell":"page",Objective(target,0)), *cr = 1;
       return(0);
    }

    /* ---->  Haven?  <---- */
    if((!tell || !((Location(target) == Location(player)) && !Quiet(Location(player)))) && Haven(target)) {
       if((targetgroup != ADMIN) && !friends) {
          if(!Valid(automatic)) automatic = match_simple(target,".message",VARIABLES,0,0);
          if(Valid(automatic) && !Blank(getfield(automatic,FAIL))) {
             sprintf(scratch_buffer,"%s"ANSI_LGREEN"Haven message from %s"ANSI_LWHITE"%s"ANSI_LGREEN":  ",(*cr) ? "":"\n",Article(target,LOWER,DEFINITE),getcname(NOTHING,target,0,0));
             sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LWHITE"%s\n",punctuate((char *) getfield(automatic,FAIL),2,'.'));
             output(getdsc(player),player,0,1,0,"%s",substitute(player,scratch_return_string,scratch_buffer,0,ANSI_LWHITE,NULL,0)), *cr = 1;
	  } else output(getdsc(player),player,0,1,0,"%s"ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" doesn't want to be disturbed.\n",(*cr) ? "":"\n",Article(target,LOWER,DEFINITE),getcname(NOTHING,target,0,0)), *cr = 1;
       }
       return(0);
    }

    /* ---->  Idling/disconnected?  <---- */
    if(!(!friends && d && (d->flags & DISCONNECTED))) {
       if((targetgroup != ADMIN) && !friends && d && !d->afk_message) {
          if(!Valid(automatic)) automatic = match_simple(target,".message",VARIABLES,0,0);
          temp = (d) ? (now - d->last_time):0;
          if(Valid(automatic)) {
             if(Blank(getfield(automatic,SUCC)) || (!Blank(getfield(automatic,SUCC)) && ((now = atoi(getfield(automatic,SUCC))) < 120))) now = 120;
             if((now <= temp) && !Blank(getfield(automatic,OSUCC))) {
                sprintf(scratch_buffer,"%s"ANSI_LGREEN"Idle message from %s"ANSI_LWHITE"%s"ANSI_LGREEN", who's been idle for "ANSI_LYELLOW"%s"ANSI_LGREEN":  ",(*cr) ? "":"\n",Article(target,LOWER,DEFINITE),getcname(NOTHING,target,0,0),interval(temp,temp,ENTITIES,0));
                sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LWHITE"%s\n",punctuate((char *) getfield(automatic,OSUCC),2,'.'));
                output(getdsc(player),player,0,1,0,"%s",substitute(player,scratch_return_string,scratch_buffer,0,ANSI_LWHITE,NULL,0)), *cr = 1;
	     }
	  } else if(temp > 120) output(getdsc(player),player,0,1,0,"%s"ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has been idle for "ANSI_LYELLOW"%s"ANSI_LGREEN".\n",(*cr) ? "":"\n",Article(target,UPPER,DEFINITE),getcname(NOTHING,target,0,0),interval(temp,temp,ENTITIES,0)), *cr = 1;
       }
    } else output(getdsc(player),player,0,1,0,"%s"ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" has lost %s connection  -  Please wait for %s to reconnect before %s %s %s.\n",(*cr) ? "":"\n",Article(target,LOWER,DEFINITE),getcname(NOTHING,target,0,0),Possessive(target,0),Objective(target,0),(tell) ? "telling":"paging",Objective(target,0),(tell) ? "another message":"again"), *cr = 1;
    return(1);
}

/* ---->  Page/tell message to another character or list of characters  <---- */
/*        (val1:  0 = Page,   1 = Tell, 2 = Repeat.)                          */
/*        (val2:  0 = Normal, 1 = Auto-emote, 2 = Reply.)                     */
void pagetell_send(CONTEXT)
{
     unsigned char            targetgroup = STANDARD,store = 1,usage = 0,reply = 0,cr = 0,pose = 0;
     int                      listsize = 0,blocked = 0,sameroom = 0;
     struct   list_data       *ptr,*tail,*head = NULL;
     static   dbref           page_victim = NOTHING;
     struct   descriptor_data *p = getdsc(player);
     static   dbref           page_who = NOTHING;
     static   char            buffer[BUFFER_LEN];
     static   time_t          page_time = 0;
     struct   str_ops         str_data;
     dbref                    who;
     time_t                   now;
     char                     *p2;
#ifdef QMW_RESEARCH
     int qmwpose = 0; /* place to store pose for qmwlogsocket */
     char *qmwposestr[3] = { "STD", "EMOTE", "THINK" };
     int qmwarg2 = 0; /* shift in arg2 to loose any pose char */ 
#endif /* #ifdef QMW_RESEARCH */

     setreturn(ERROR,COMMAND_FAIL);
     comms_spoken(player,1);
     gettime(now);

     /* ---->  Repeat previously sent message  <---- */
     if(val2 == 2) val2 = 0, reply = 1;
     if(val1 == 2) {
        if(p) {
           arg1  = params;
           arg2  = (p->lastmessage) ? p->lastmessage:"";
           val1  = (p->flags & LASTMSG_TELL) ? 1:0; 
           store = 0;
	} else {
           output(p,player,0,1,0,ANSI_LGREEN"Sorry, you haven't sent any messages using '"ANSI_LWHITE"page"ANSI_LGREEN"' or '"ANSI_LWHITE"tell"ANSI_LGREEN"' yet.");
           return;
	}
     }

     /* ---->  Page/tell within '@with connected do...'  <---- */
     if((flow_control & FLOW_WITH_CONNECTED) && !(in_command && Wizard(current_command))) {
        if(Level4(db[player].owner) && !Shout(db[player].owner)) {
           if(!(flow_control & FLOW_WITH_BANISHED)) {
              output(p,player,0,1,0,ANSI_LGREEN"Sorry, you've been banned from shouting  -  You can't %s everyone connected using '"ANSI_LWHITE"@with connected..."ANSI_LGREEN"'.",(val1) ? "tell a message to":"page");
              flow_control |= FLOW_WITH_BANISHED;
	   }
           return;
	} else if(!(flow_control & FLOW_WITH_BANISHED)) {
           writelog(SHOUT_LOG,1,"SHOUT","%s(#%d) using '@with connected':  %s",getname(player),player,String(current_cmdptr));
           flow_control |= FLOW_WITH_BANISHED;
	}
     }

     /* ---->  Page/tell within '@with friends do...'  <---- */
     if((flow_control & FLOW_WITH_FRIENDS) && !(in_command && Wizard(current_command))) {
        if(!(flow_control & FLOW_WITH_BANISHED)) {
           output(p,player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@with friends/enemies do..."ANSI_LGREEN"' can't be used to %s all of your friends/enemies  -  Please use '"ANSI_LYELLOW"%s friends|enemies [=] <MESSAGE>"ANSI_LGREEN"' instead.",(val1) ? "tell a message to":"page",(val1) ? "tell":"page");
           flow_control |= FLOW_WITH_BANISHED;
	}
        return;
     }

     if(Blank(arg2)) {
        char          *start1,*start2,*end1,*end2,*src;
        unsigned char matched = 0;
        char          c;

        /* ---->  No '=' to separate name and message  <---- */
        for(src = arg1; *src && (*src == ' '); src++);
        for(start1 = src; *src && (*src != ' '); src++);
        for(end1 = src; *src && (*src == ' '); src++);
        for(start2 = src; *src && (*src != ' '); src++);
        for(end2 = src; *src && (*src == ' '); src++);

        /* ---->  Match against 'friends', 'enemies' and 'admin'  <---- */
        c = *end1, *end1 = '\0';
        if(!strcasecmp(start1,"friend") || !strcasecmp(start1,"friends") || !strcasecmp(start1,"enemy")  || !strcasecmp(start1,"enemies") || !strcasecmp(start1,"admin")  || !strcasecmp(start1,"administrators") || !strcasecmp(start1,"administration"))
           arg1 = start1, arg2 = start2, matched = 1;
              else *end1 = c;

        /* ---->  Match first two words  <---- */
        if(!matched) {
           c = *end2, *end2 = '\0';
           if(lookup_character(player,start1,5) != NOTHING)
              arg1 = start1, arg2 = src, matched = 1;
                 else *end2 = c;
	}

        /* ---->  Match first word  <---- */
        if(!matched)
           *end1 = '\0', arg1 = start1, arg2 = start2;
     }

     /* ---->  Store message for later repeat  <---- */
     if(store && p) {
        FREENULL(p->lastmessage);
        p->lastmessage = (char *) alloc_string(arg2);
        if(val1) p->flags |= LASTMSG_TELL;
           else p->flags &= ~LASTMSG_TELL;
     }

     /* ---->  Recipient(s) not specified  <---- */
     if(!usage && Blank(arg1)) {
        sprintf(scratch_buffer,ANSI_LGREEN"Please specify who (Or a list of who) you'd like to %s.",(!store) ? "re-send your message to":(val1) ? "speak to":"page a message to");
        usage = 1, cr = 1;
     }

     /* ---->  No message?  <---- */
     if(!usage && val1 && Blank(arg2)) {
        strcpy(scratch_buffer,ANSI_LGREEN"Please specify what you'd like to say to them.");
        usage = 1, cr = 1;
     }

     /* ---->  Construct list of people to page/tell message to  <---- */
     if(!usage) {
        unsigned char fpage = 0;

        if(((!strcasecmp(arg1,"friend") || !strcasecmp(arg1,"friends")) && (fpage = 1)) || !strcasecmp(arg1,"enemy") || !strcasecmp(arg1,"enemies")) {
           int                      flags,flags2;
           unsigned char            friend;
           struct   descriptor_data *d;

           /* ---->  Page/tell message to friends or enemies  <---- */
           for(d = descriptor_list; d; d = d->next)
               if((d->player != player) && Validchar(d->player) && (d->flags & CONNECTED)) {
                  friend = 0;
                  if((flags = friend_flags(player,d->player))) {
                     if((flags & FRIEND_EXCLUDE) || !(flags & FRIEND_PAGETELLFRIENDS)) continue;
                     friend = 1;
	          } else flags = FRIEND_STANDARD;
                  if((flags2 = friend_flags(d->player,player))) {
                     if((flags2 & FRIEND_EXCLUDE) || !(flags2 & FRIEND_PAGETELLFRIENDS)) continue;
                     friend = 1;
	          } else flags = FRIEND_STANDARD;

                  if(friend && ((fpage && !((flags|flags2) & FRIEND_ENEMY)) || (!fpage && ((flags|flags2) & FRIEND_ENEMY)))) {
                     listsize++;
                     if(!Connected(d->player) || Haven(d->player)) blocked++;
                     if(Location(d->player) == Location(player)) sameroom++;
                     MALLOC(ptr,struct list_data);
                     ptr->player = d->player;
                     ptr->next   = NULL;
                     if(head) {
                        tail->next = ptr;
                        tail       = ptr;
		     } else head = tail = ptr;
		  }
	       }
           targetgroup = (fpage) ? FRIENDS:ENEMIES;
	} else if(!strcasecmp(arg1,"admin") || !strcasecmp(arg1,"administrators") || !strcasecmp(arg1,"administration")) {

           /* ---->  Page/tell message to Admin (Apprentice Wizards/Druids and above)  <---- */
           struct descriptor_data *d;

           for(d = descriptor_list; d; d = d->next) {
               if((d->flags & CONNECTED) && Validchar(d->player) && Level4(d->player)) {
                  if(!val1 || (d->player != player)) {
                     for(ptr = head; ptr && (ptr->player != d->player); ptr = ptr->next);
                     if(!ptr) {

                        /* ---->  Admin character isn't already in list;  Add them to it  <---- */
                        listsize++;
                        if(!Connected(d->player) || Haven(d->player)) blocked++;
                        if(Location(d->player) == Location(player)) sameroom++;
                        MALLOC(ptr,struct list_data);
                        ptr->player = d->player;
                        ptr->next   = NULL;
                        if(head) {
                           tail->next = ptr;
                           tail       = ptr;
		        } else head = tail = ptr;
		     }
		  } else blocked++;
	       }
	   }
           targetgroup = ADMIN;
	} else while(*arg1) {

           /* ---->  Page/tell message to a single character or list of characters  <---- */
           for(; *arg1 && ((*arg1 == ' ') || (*arg1 == ',') || (*arg1 == ';') || (*arg1 == '&')); arg1++);
           for(p2 = scratch_buffer; *arg1 && !((*arg1 == ',') || (*arg1 == ';') || (*arg1 == '&')); *p2++ = *arg1, arg1++);
           for(; ((p2 - 1) >= scratch_buffer) && (*(p2 - 1) == ' '); p2--);  /*  Strip trailing blanks  */
           *p2 = '\0';

           if(!Blank(scratch_buffer)) {
              if(!string_prefix("last",scratch_buffer)) {
                 char *scan = scratch_buffer;
                 int  count = 0;

                 for(; *scan && (isdigit(*scan) || (*scan == ' ')); scan++);
                 if(!*scan) count = atol(scratch_buffer);
                 if(!count) {

                    /* ---->  Look up character name  <---- */
                    if((who = lookup_character(player,scratch_buffer,1)) == NOTHING)
                       output(p,player,0,1,0,"%s"ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",(cr) ? "":"\n",scratch_buffer), cr = 1;
		 } else who = pagetell_recall_pager(player,count,scratch_buffer,&cr);
	      } else who = pagetell_recall_pager(player,1,scratch_buffer,&cr);

              if(!((who == player) && val1)) {
	         if(Validchar(who)) {

                    /* ---->  Character found  -  Check they aren't already in list  <---- */
                    for(ptr = head; ptr && (ptr->player != who); ptr = ptr->next);
                    if(!ptr) {

                       /* ---->  Character isn't in list;  Add them to it  <---- */
                       listsize++;
                       if(!Connected(who) || Haven(who)) blocked++;
                       if(Location(who) == Location(player)) sameroom++;
                       MALLOC(ptr,struct list_data);
                       ptr->player = who;
                       ptr->next   = NULL;
                       if(head) {
                          tail->next = ptr;
                          tail       = ptr;
		       } else head = tail = ptr;
		    }
		 }
	      } else output(p,player,0,1,0,"%s"ANSI_LGREEN"Talking to yourself is the first sign of madness!",(cr) ? "":"\n"), cr = 1;
	   }
	}
     }
     if(cr) output(p,player,0,1,0,"");

     /* ---->  List of people to page/tell message to blank/no friends?  <---- */
     if(!usage && !head && (targetgroup == STANDARD)) {
        sprintf(scratch_buffer,"%s"ANSI_LGREEN"Please specify who (Or a list of who) you'd like to %s.",(cr) ? "":"\n",(!store) ? "re-send your message to":(val1) ? "speak to":"page a message to");
        usage = 1;
     }

     /* ---->  Everyone in list either not connected or blocking pages/tells?  <---- */
     if(!usage && (!head || ((blocked >= listsize) && ((targetgroup != STANDARD) || (listsize > 1))))) {
        switch(targetgroup) {
               case FRIENDS:
                    sprintf(scratch_buffer,ANSI_LGREEN"Sorry, none of your friends are connected at the moment, or they're all blocking %s from you at the moment.",(val1) ? "tells":"pages");
                    break;
               case ENEMIES:
                    sprintf(scratch_buffer,ANSI_LGREEN"Sorry, none of your enemies are connected at the moment, or they're all blocking %s from you at the moment.",(val1) ? "tells":"pages");
                    break;
               case ADMIN:
                    sprintf(scratch_buffer,ANSI_LGREEN"Sorry, no %s Admin are connected at the moment%s, or they're all HAVEN or blocking %s from you at the moment.",tcz_short_name,(Level4(player)) ? " (Except you)":"",(val1) ? "tells":"pages");
                    break;
               case STANDARD:
               default:
                    sprintf(scratch_buffer,ANSI_LGREEN"Sorry, none of those characters are connected at the moment, or they're all blocking %s from you at the moment.",(val1) ? "tells":"pages");
	}
        usage = 3, cr = 1;
     }

     /* ---->  Page/tell 'bombing' protection  <---- */
     if(!usage)
       if(!Level4(player) && (page_who == player) && (now < (Moron(player) ? (page_time + PAGE_TIME_MORON - PAGE_TIME):page_time)))
          for(ptr = head; ptr; ptr = (ptr) ? ptr->next:NULL)
              if(ptr->player == page_victim) {
                 sprintf(scratch_buffer,ANSI_LGREEN"Please wait "ANSI_LWHITE"%s"ANSI_LGREEN" before %s another message to that user/list of users.",interval((Moron(player) ? (page_time + PAGE_TIME_MORON - PAGE_TIME):page_time) - now,(Moron(player) ? (page_time + PAGE_TIME_MORON - PAGE_TIME):page_time) - now,ENTITIES,0),(val1) ? "telling":"paging");
                 usage = 3, ptr = NULL;
	      }
     if(!usage) {
        page_victim = head->player;
        page_time   = now + PAGE_TIME;
        page_who    = player;
     }

     if(val2 && !Blank(arg2)) *(--arg2) = ':';
     if(!usage) {

        /* ---->  Work out who user can tell/page messages to in list  <---- */
        for(ptr = head, blocked = 0; ptr; ptr = ptr->next)
            if(!pagetell_cansend(player,ptr->player,val1,targetgroup,now,&cr,((targetgroup == FRIENDS) || (targetgroup == ENEMIES)))) {
               db[ptr->player].flags &= ~OBJECT;
               blocked++;
	    } else db[ptr->player].flags |= OBJECT;

        /* ---->  Everyone in list blocking pages/tells via page-lock/tell-lock, etc.?  <---- */
        if(!usage && (blocked >= listsize)) {
           if((targetgroup == FRIENDS) || (targetgroup == ENEMIES))
              sprintf(scratch_buffer,ANSI_LGREEN"Message not sent (All of your %s are blocking %s from you/group %s to your %s.)\n",(targetgroup == FRIENDS) ? "friends":"enemies",(val1) ? "tells":"pages",(targetgroup == FRIENDS) ? "friends":"enemies",(val1) ? "tells":"pages");
                 else strcpy(scratch_buffer,ANSI_LGREEN"Message not sent.\n");
           usage = 3, cr = 1;
	}
     }

     if(!usage) {

        /* ---->  Character is set HAVEN?  <---- */
        if((!val1 || (sameroom < listsize)) && Haven(player)) {
           sprintf(scratch_buffer,ANSI_LGREEN"Please remove your "ANSI_LYELLOW"HAVEN"ANSI_LGREEN" flag (Type '"ANSI_LWHITE"@set me = !haven"ANSI_LGREEN"') before trying to %s messages to other characters.\n\nMessage not sent.",(val1) ? "tell long-distance":"page");
           usage = 3, cr = 1;
	} else {

           /* ---->  Tell/page message  <---- */

#ifdef QMW_RESEARCH
	  /* ----> Save pose and arg2 shift for qmwlogsocket() later */
	  if(*arg2 && ((*arg2 == *POSE_TOKEN) || (*arg2 == *ALT_POSE_TOKEN)))
	    qmwpose = 1;
	  else if(*arg2 && ((*arg2 == *THINK_TOKEN) || (*arg2 == *ALT_THINK_TOKEN)))
	    qmwpose = 2;
	  if (qmwpose > 0) 
	    qmwarg2 = 1;
#endif /* #ifdef QMW_RESEARCH */

           if(val1) {

              /* ---->  'Emoted'/'thought' tell?  <---- */
              if(*arg2 && ((*arg2 == *POSE_TOKEN) || (*arg2 == *ALT_POSE_TOKEN))) pose = 1;
                 else if(*arg2 && ((*arg2 == *THINK_TOKEN) || (*arg2 == *ALT_THINK_TOKEN))) pose = 2;

              /* ---->  Notify sender  <---- */
              command_type |= COMM_CMD;
              init_strops(&str_data,NULL,buffer);
              if(!pose) {
                 strcat_limits(&str_data,construct_message(player,ANSI_LWHITE,(in_command || (sameroom < listsize)) ? ANSI_LCYAN:ANSI_DWHITE,"",'\0',NONE,PLAYER,arg2,1,DEFINITE));
                 sprintf(scratch_return_string,"%s.",pagetell_construct_list(player,player,(union group_data *) head,listsize - blocked,scratch_buffer,ANSI_LWHITE,(in_command || (sameroom < listsize)) ? ANSI_LCYAN:ANSI_DWHITE,0,targetgroup,DEFINITE));
                 strcat_limits(&str_data,scratch_return_string);
                 *(str_data.dest) = '\0';
              } else {
                 sprintf(scratch_return_string,"%s[To %s]  ",(in_command || (sameroom < listsize)) ? ANSI_LYELLOW:ANSI_LCYAN,pagetell_construct_list(player,player,(union group_data *) head,listsize - blocked,scratch_buffer,ANSI_LWHITE,(in_command || (sameroom < listsize)) ? ANSI_LYELLOW:ANSI_LCYAN,0,targetgroup,DEFINITE));
                 strcat_limits(&str_data,scratch_return_string);
                 strcat_limits(&str_data,construct_message(player,ANSI_LWHITE,(in_command || (sameroom < listsize)) ? ANSI_LCYAN:ANSI_DWHITE,"",'.',(pose == 2) ? 2:-1,(pose == 2) ? PLAYER:OTHERS,(pose == 2) ? arg2 + 1:arg2,0,DEFINITE));
                 *(str_data.dest) = '\0';
	      }
              output(p,player,0,1,2,"%s",buffer);
	
              /* ---->  Notify everyone else in same room, but not in list (Providing everyone in list is in same room as person doing tell/page)  <---- */
              if(!Quiet(Location(player)) && (sameroom >= listsize)) {
                 for(who = db[Location(player)].contents; Valid(who); who = Next(who)) {
		     if((Typeof(who) == TYPE_CHARACTER) && Connected(who) && (who != player)) {
			for(ptr = head; ptr && (ptr->player != who); ptr = ptr->next);
			if(!ptr) {
			   init_strops(&str_data,NULL,buffer);
			   if(!pose) {
			      strcat_limits(&str_data,construct_message(player,ANSI_LWHITE,(in_command) ? ANSI_LCYAN:ANSI_DWHITE,"",'\0',NONE,OTHERS,arg2,1,DEFINITE));
			      sprintf(scratch_return_string,"%s.",pagetell_construct_list(player,who,(union group_data *) head,listsize - blocked,scratch_buffer,ANSI_LWHITE,(in_command) ? ANSI_LCYAN:ANSI_DWHITE,1,targetgroup,INDEFINITE));
			      strcat_limits(&str_data,scratch_return_string);
			      *(str_data.dest) = '\0';
			   } else {
			      sprintf(scratch_return_string,"%s[To %s]  ",(in_command) ? ANSI_LYELLOW:ANSI_LCYAN,pagetell_construct_list(player,who,(union group_data *) head,listsize - blocked,scratch_buffer,ANSI_LWHITE,(in_command) ? ANSI_LYELLOW:ANSI_LCYAN,1,targetgroup,INDEFINITE));
			      strcat_limits(&str_data,scratch_return_string);
			      strcat_limits(&str_data,construct_message(player,ANSI_LWHITE,(in_command) ? ANSI_LCYAN:ANSI_DWHITE,"",'.',(pose == 2) ? 2:-1,OTHERS,(pose == 2) ? arg2 + 1:arg2,0,DEFINITE));
			      *(str_data.dest) = '\0';
			   }
			   output(getdsc(who),who,0,1,2,"%s",buffer);
#ifdef QMW_RESEARCH
			   qmwlogsocket("PAGETELL:SAMEROOM:%s:%d:%d:%d:%d:%d:%d:%s",
					qmwposestr[qmwpose],
					player, privilege(player,255),
					db[player].location,
					who, privilege(who,255),
					db[who].location,
					(char *)(arg2+qmwarg2));
#endif /* #ifdef QMW_RESEARCH */
			}
		     }
		 }
	      }
	
              /* ---->  Notify everyone in list  <---- */
              for(ptr = head; ptr; ptr = ptr->next)
                  if(db[ptr->player].flags & OBJECT) {
                     init_strops(&str_data,NULL,buffer);
                     if(!pose) {
                        strcat_limits(&str_data,construct_message(player,ANSI_LWHITE,(in_command || (Location(ptr->player) != Location(player))) ? ANSI_LCYAN:ANSI_DWHITE,"",'\0',NONE,OTHERS,arg2,1,(Location(ptr->player) == Location(player)) ? DEFINITE:INDEFINITE));
                        sprintf(scratch_return_string,"%s.",pagetell_construct_list(player,ptr->player,(union group_data *) head,listsize - blocked,scratch_buffer,ANSI_LWHITE,(in_command || (Location(ptr->player) != Location(player))) ? ANSI_LCYAN:ANSI_DWHITE,0,targetgroup,INDEFINITE));
                        strcat_limits(&str_data,scratch_return_string);
                        *(str_data.dest) = '\0';
		     } else {
                        sprintf(scratch_return_string,"%s[To %s]  ",(in_command || (Location(ptr->player) != Location(player))) ? ANSI_LYELLOW:ANSI_LCYAN,pagetell_construct_list(player,ptr->player,(union group_data *) head,listsize - blocked,scratch_buffer,ANSI_LWHITE,(in_command || (Location(ptr->player) != Location(player))) ? ANSI_LYELLOW:ANSI_LCYAN,0,targetgroup,INDEFINITE));
                        strcat_limits(&str_data,scratch_return_string);
                        strcat_limits(&str_data,construct_message(player,ANSI_LWHITE,(in_command || (Location(ptr->player) != Location(player))) ? ANSI_LCYAN:ANSI_DWHITE,"",'.',(pose == 2) ? 2:-1,OTHERS,(pose == 2) ? arg2 + 1:arg2,0,(Location(ptr->player) == Location(player)) ? DEFINITE:INDEFINITE));
                        *(str_data.dest) = '\0';
		     }
                     pagetell_store_message(ptr->player,player,now,buffer,1);
                     output(getdsc(ptr->player),ptr->player,0,1,2,"%s",buffer);
                     command_execute_action(ptr->player,NOTHING,".tell",NULL,getname(player),arg2,arg2,0);
#ifdef QMW_RESEARCH
		     qmwlogsocket("PAGETELL:TELL:%s:%d:%d:%d:%d:%d:%d:%s",
				  qmwposestr[qmwpose],
				  player, privilege(player,255),
				  db[player].location,
				  ptr->player,  privilege(ptr->player,255),
				  db[ptr->player].location,
				  (char *)(arg2+qmwarg2));
#endif /* #ifdef QMW_RESEARCH */
	          }
              command_type &= ~COMM_CMD;
	   } else {

              /* ---->  Page everyone in list  <---- */
              command_type |= COMM_CMD;
              for(ptr = head; ptr; ptr = ptr->next)
                  if(db[ptr->player].flags & OBJECT) {
                     init_strops(&str_data,NULL,buffer);
                     if(!((Secret(player) || Secret(Location(player))) && !can_write_to(ptr->player,Location(player),1) && !can_write_to(ptr->player,player,1))) {
                        who = get_areaname_loc(Location(player));
                        sprintf(scratch_buffer,"%s"ANSI_LGREEN"PAGING from %s"ANSI_LYELLOW"%s"ANSI_LGREEN,Pagebell(ptr->player) ? "\007":"",Article(ptr->player,LOWER,INDEFINITE),unparse_object(ptr->player,Location(player),0));
                        if(Valid(who) && !Blank(getfield(who,AREANAME)))
                           sprintf(scratch_buffer + strlen(scratch_buffer)," in "ANSI_LYELLOW"%s"ANSI_LGREEN,getfield(who,AREANAME));
		     } else sprintf(scratch_buffer,"%s"ANSI_LGREEN"PAGING from a secret location",Pagebell(ptr->player) ? "\007":"");
                     strcat_limits(&str_data,scratch_buffer);
                     if((targetgroup != STANDARD) || (listsize > 1)) {
                        sprintf(scratch_buffer," (Group page to %s): \016&nbsp;\016 ",pagetell_construct_list(player,ptr->player,(union group_data *) head,listsize - blocked,scratch_return_string,ANSI_LWHITE,ANSI_LGREEN,0,targetgroup,INDEFINITE));
                        strcat_limits(&str_data,scratch_buffer);
	             } else strcat_limits(&str_data,": \016&nbsp;\016 ");
                     if(Blank(arg2)) sprintf(scratch_return_string,ANSI_DWHITE"%s"ANSI_LWHITE"%s"ANSI_DWHITE" tries to contact you.",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0));
                        else if((*arg2 == *POSE_TOKEN) || (*arg2 == *THINK_TOKEN) || (*arg2 == *ALT_POSE_TOKEN) || (*arg2 == *ALT_THINK_TOKEN)) strcpy(scratch_return_string,construct_message(player,ANSI_LWHITE,ANSI_DWHITE,"",'.',-1,OTHERS,arg2,0,(Location(ptr->player) == Location(player)) ? DEFINITE:INDEFINITE));
                           else strcpy(scratch_return_string,construct_message(player,ANSI_LWHITE,ANSI_DWHITE,"",'.',0,OTHERS,arg2,0,(Location(ptr->player) == Location(player)) ? DEFINITE:INDEFINITE));
                     strcat(scratch_return_string,"\n");
                     strcat_limits(&str_data,scratch_return_string);
                     *(str_data.dest) = '\0';

                     pagetell_store_message(ptr->player,player,now,buffer,0);
                     output(getdsc(ptr->player),ptr->player,0,1,2,"");
                     output(getdsc(ptr->player),ptr->player,0,1,2,"%s",buffer);
                     command_execute_action(ptr->player,NOTHING,".page",NULL,getname(player),arg2,arg2,0);
#ifdef QMW_RESEARCH
		     qmwlogsocket("PAGETELL:PAGE:%s:%d:%d:%d:%d:%d:%d:%s",
				  qmwposestr[qmwpose],
				  player, privilege(player,255),
				  db[player].location,
				  ptr->player,  privilege(ptr->player,255),
				  db[ptr->player].location,
				  (char *)(arg2+qmwarg2));
#endif /* #ifdef QMW_RESEARCH */
		  }
              command_type &= ~COMM_CMD;
              if(!in_command) output(p,player,0,1,0,ANSI_LGREEN"Message sent to %s.",pagetell_construct_list(player,player,(union group_data *) head,listsize - blocked,scratch_return_string,ANSI_LWHITE,ANSI_LGREEN,0,targetgroup,DEFINITE));
	   }
	}
     }

     /* ---->  Wipe list  <---- */
     for(ptr = head; ptr; ptr = tail) {
         db[ptr->player].flags |= OBJECT;
         tail = ptr->next;
         FREENULL(ptr);
     }

     /* ---->  Display USAGE?  <---- */
     if(usage) {
        if(usage != 3) {
           if(store) {
              if(reply) sprintf(scratch_buffer + strlen(scratch_buffer),"%s"ANSI_LCYAN"USAGE:  "ANSI_LGREEN"reply <MESSAGE>\n",(usage == 1) ? "\n\n":"\n");
                 else sprintf(scratch_buffer + strlen(scratch_buffer),"%s"ANSI_LCYAN"USAGE:  "ANSI_LGREEN"%s <NAME>[,<NAME>][,<NAME>][...] [=] <MESSAGE>\n",(usage == 1) ? "\n\n":"\n",(val1) ? "tell":"page");
	   } else sprintf(scratch_buffer + strlen(scratch_buffer),"%s"ANSI_LCYAN"USAGE:  "ANSI_LGREEN"repeat <NAME>[,<NAME>][,<NAME>][...]\n",(usage == 1) ? "\n\n":"\n");
	}
        output(p,player,0,1,0,"%s",scratch_buffer);
        return;
     } else setreturn(OK,COMMAND_SUCC);
}

/* ---->  Page/tell message to your friends  <---- */
void pagetell_friends(CONTEXT)
{
     char buffer[16];

     strcpy(buffer,"friends");
     pagetell_send(player,NULL,NULL,buffer,Blank(arg2) ? arg1:arg2,val1,0);
}

/* ---->  Display list of stored pages/tells (8 most recent)  <---- */
void pagetell_recall(CONTEXT)
{
     unsigned char            twidth = output_terminal_width(player);
     struct   descriptor_data *p = getdsc(player);
     time_t                   timediff,now;
     int                      count;

     setreturn(ERROR,COMMAND_FAIL);
     if(Connected(player) && p) {
        if(p->messagecount) {
           timediff = db[player].data->player.timediff * HOUR;
           if(Blank(params) || (!Blank(params) && !(string_prefix("list",params) || string_prefix("view",params) || string_prefix("read",params)))) {
              int last = 0;
 
              /* ---->  Recall (And display) previously page'd/tell'd message  <---- */
              if(Blank(params) || (!Blank(params) && string_prefix("last",params))) count = 1, last = 1;
                 else count = atol(params);

              if(count > 0) {
                 if(count <= p->messagecount) {
                    html_anti_reverse(p,1);
                    count--, now = p->messages[count].time + timediff;
                    if(IsHtml(p)) output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
                    if(!in_command) {
                       if(p->messages[count].pager != player) output(p,player,2,1,1,"%s%s from %s"ANSI_LWHITE"%s"ANSI_LCYAN" on "ANSI_LYELLOW"%s"ANSI_LCYAN".%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER><FONT COLOR="HTML_LCYAN" SIZE=4><I>\016":"\n ",(p->messages[count].tell) ? "Tell":"Page",Article(p->messages[count].pager,LOWER,INDEFINITE),getcname(NOTHING,p->messages[count].pager,0,0),date_to_string(now,UNSET_DATE,player,FULLDATEFMT),IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
                          else output(p,player,2,1,1,"%s%s from yourself on "ANSI_LYELLOW"%s"ANSI_LCYAN".%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER><FONT COLOR="HTML_LCYAN" SIZE=4><I>\016":"\n ",(p->messages[count].tell) ? "Tell":"Page",date_to_string(now,UNSET_DATE,player,FULLDATEFMT),IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
                       if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,1,'-','='));
		    } else if(!IsHtml(p) && !p->messages[count].tell) output(p,player,0,1,0,"");

                    output(p,player,2,1,3,"%s%s%s%s",IsHtml(p) ? "\016<TR><TD ALIGN=LEFT>\016":" ",Blank(p->messages[count].message) ? "Unknown":decompress(p->messages[count].message),(p->messages[count].tell && !(IsHtml(p) || in_command)) ? "\n":"",IsHtml(p) ? "\016</TD></TR>\016":"\n");
                    if(!in_command) {
                       if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','-'));
                       if(!last && (count > 0)) output(p,player,2,1,1,"%sTo reply to this message, simply type '"ANSI_LGREEN"page %d <MESSAGE>"ANSI_LWHITE"' or '"ANSI_LGREEN"tell %d <MESSAGE>"ANSI_LWHITE"'.%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_GREY"><TD ALIGN=CENTER>"ANSI_LWHITE"<I>\016":ANSI_LWHITE" ",count + 1,count + 1,IsHtml(p) ? "\016</TD></TR>\016":"\n");
                          else output(p,player,2,1,1,"%sTo reply to this message, simply type '"ANSI_LGREEN"reply <MESSAGE>"ANSI_LWHITE"'.%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_GREY"><TD ALIGN=CENTER>"ANSI_LWHITE"<I>\016":ANSI_LWHITE" ",IsHtml(p) ? "\016</TD></TR>\016":"\n");
                       if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,1,'-','='));
		    }
                    if(IsHtml(p)) output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
                    html_anti_reverse(p,0);
                    setreturn(OK,COMMAND_SUCC);
		 } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you only have "ANSI_LWHITE"%d"ANSI_LGREEN" stored message%s.",p->messagecount,Plural(p->messagecount));
	      } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, that message number is invalid  -  Please specify a positive number.");
	   } else {

              /* ---->  List stored messages  <---- */
              html_anti_reverse(p,1);
              if(!p->pager && !IsHtml(p) && Validchar(p->player) && More(p->player)) pager_init(p);
              if(IsHtml(p)) output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
              if(!in_command) {
                 output(p,player,2,1,1,"%sMost recent messages sent to you using either '"ANSI_LWHITE"page"ANSI_LCYAN"' or '"ANSI_LWHITE"tell"ANSI_LCYAN"'...%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER COLSPAN=2><FONT COLOR="HTML_LCYAN" SIZE=4><I>\016":"\n ",IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
                 if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
	      }

              for(count = 0; count < p->messagecount; count++) {
                  sprintf(scratch_return_string,"(%d)",count + 1);
                  if(p->messages[count].pager != player) sprintf(scratch_buffer,"%s%-6s%s"ANSI_LWHITE"%s from %s"ANSI_LYELLOW"%s"ANSI_LWHITE" on ",IsHtml(p) ? "\016<TR><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_GREEN">\016"ANSI_LGREEN:ANSI_LGREEN" ",scratch_return_string,IsHtml(p) ? "\016</TD><TD>\016":"",(p->messages[count].tell) ? "Tell":"Page",Article(p->messages[count].pager,LOWER,INDEFINITE),getcname(NOTHING,p->messages[count].pager,0,0));
                     else sprintf(scratch_buffer,"%s%-6s%s"ANSI_LWHITE"%s from yourself on ",IsHtml(p) ? "\016<TR><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_GREEN">\016"ANSI_LGREEN:ANSI_LGREEN" ",scratch_return_string,IsHtml(p) ? "\016</TD><TD>\016":"",(p->messages[count].tell) ? "Tell":"Page");
                  now = p->messages[count].time + timediff;
                  output(p,player,2,1,7,"%s"ANSI_LCYAN"%s"ANSI_LWHITE".%s",scratch_buffer,date_to_string(now,UNSET_DATE,player,FULLDATEFMT),IsHtml(p) ? "\016</TD></TR>\016":"\n");
	      }

              if(!in_command) {
                 if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','-'));
                 output(p,player,2,1,1,"%sTo recall one of the above messages, type '"ANSI_LGREEN"recall <NUMBER>"ANSI_LWHITE"'.  To reply, simply type '"ANSI_LGREEN"page <NUMBER> <MESSAGE>"ANSI_LWHITE"' or '"ANSI_LGREEN"tell <NUMBER> <MESSAGE>"ANSI_LWHITE"'.%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_GREY"><TD ALIGN=CENTER COLSPAN=2>"ANSI_LWHITE"<I>\016":ANSI_LWHITE" ",IsHtml(p) ? "\016</TD></TR>\016":"\n");
                 if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,1,'-','='));
	      }
              if(IsHtml(p)) output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
              html_anti_reverse(p,0);
              setreturn(OK,COMMAND_SUCC);
	   }
	} else output(p,player,0,1,0,ANSI_LGREEN"Sorry, no-one has sent any messages to you using '"ANSI_LWHITE"page"ANSI_LGREEN"' or '"ANSI_LWHITE"tell"ANSI_LGREEN"' yet.");
     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you must be connected to use the '"ANSI_LWHITE"recall"ANSI_LGREEN"' command.");
}

/* ---->  Reply to last 'page'/'tell' message  <---- */
void pagetell_reply(CONTEXT)
{
     struct descriptor_data *p = getdsc(player);
     char   buffer[5];

     setreturn(OK,COMMAND_FAIL);
     if(p && (p->messagecount > 0)) {
        strcpy(buffer,"last");
        pagetell_send(player,NULL,NULL,buffer,params,p->messages[1].tell,2);
     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, no-one has sent any messages to you using '"ANSI_LWHITE"page"ANSI_LGREEN"' or '"ANSI_LWHITE"tell"ANSI_LGREEN"' yet.");
}
