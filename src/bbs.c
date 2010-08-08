/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| BBS.C  -  Implements an easy to use and flexible topic-based public         |
|           Bulletin Board System (BBS).                                      |
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
| Module originally designed and written by:  J.P.Boggis 16/07/1995.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: bbs.c,v 1.2 2005/06/29 20:38:37 tcz_monster Exp $

*/

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "object_types.h"
#include "flagset.h"


/* ---->  Filter leading '*'s and '!'s from given topic name  <---- */
#define bbs_topicname_filter(topicname) if(topicname) \
            for(; *topicname && ((*topicname == '*') || (*topicname == '!')); topicname++);

#define SEARCH_MESSAGE 0x1  /*  Search in message text  */
#define SEARCH_SUBJECT 0x2  /*  Search in subject       */

void bbs_view(CONTEXT);

static char logbuffer[TEXT_SIZE];

/* ---->  Standard format for logging action to BBS message  <---- */
const char *bbs_logmsg(struct bbs_message_data *message,struct bbs_topic_data *topic,struct bbs_topic_data *subtopic,int msgno,unsigned char anon)
{
      sprintf(logbuffer,"(%d) '%s%s' left by ",msgno,(message->flags & MESSAGE_REPLY) ? "Re:  ":"",decompress(message->subject)); 
      if(anon || (message->flags & MESSAGE_ANON)) strcat(logbuffer + strlen(logbuffer),"<ANONYMOUS>");
         else sprintf(logbuffer + strlen(logbuffer),"%s (%s(#%d))",decompress(message->name),getname(message->owner),message->owner);
      sprintf(logbuffer + strlen(logbuffer)," in the %stopic '%s%s%s':  ",(subtopic) ? "sub-":"",(subtopic) ? subtopic->name:"",(subtopic) ? "/":"",topic->name);
      return(logbuffer);
}

/* ---->  Standard format for logging action to BBS topic/sub-topic  <---- */
const char *bbs_logtopic(struct bbs_topic_data *topic,struct bbs_topic_data *subtopic)
{
      sprintf(logbuffer,"'%s%s%s', owned by %s(#%d):  ",(subtopic) ? subtopic->name:"",(subtopic) ? "/":"",topic->name,getname(topic->owner),topic->owner);
      return(logbuffer);
}

/* ---->  Return value of vote given by specified character  <---- */
short vote_value(dbref player,unsigned char majority)
{
      if(majority && Validchar(player)) {
         if(Moron(player)) return(0);
            else if(Level1(player)) return(6);
               else if(Level2(player)) return(5);
                  else if(Level3(player)) return(4);
                     else if(Level4(player)) return(3);
                        else if(Experienced(player) || Assistant(player)) return(2);
                           else return(1);
      } else return(1);
}

/* ---->  Update (1)/sanitise (0) readers list of given message  <---- */
void bbs_update_readers(struct bbs_message_data *message,dbref who,unsigned char hupdate,unsigned char supdate)
{
     struct bbs_reader_data *new,*last,*reader = message->readers;
     short  count = 0;
     time_t now;

     /* ---->  Sanitise readers list (Remove users who no-longer exist (Providing they haven't voted))  <---- */
     gettime(now);
     for(reader = message->readers, last = NULL; reader; reader = new) {
         new = reader->next;
         if(!Validchar(reader->reader)) {
            if(reader->flags & READER_VOTE_MASK) {
               reader->reader = NOTHING;
               last           = reader;
	    } else {
               if(last) last->next = reader->next;
                  else message->readers = reader->next;
               message->readercount++;
               FREENULL(reader);
	    }
	 } else last = reader;
     }

     /* ---->  Check maximum number of readers per message hasn't been exceeded  <---- */
     for(reader = message->readers, count = 0; reader; reader = reader->next, count++);
     if(!(message->flags & MESSAGE_EVERYONE) && (((count + (hupdate == 1)) > BBS_MAX_READERS) || (message->date < (now - (BBS_READERS_EXPIRY * DAY))))) {
        for(reader = message->readers, last = NULL; reader; reader = new) {
            new = reader->next;
            if(!(reader->flags & READER_VOTE_MASK)) {
               if(last) last->next = reader->next;
                  else message->readers = reader->next;
               FREENULL(reader);
	    } else {
               reader->flags &= ~(READER_READ|READER_IGNORE);
               last           =  reader;
	    }
	}
        message->flags       |= MESSAGE_EVERYONE;
        message->readercount += count;
     }

     /* ---->  Update readers  <---- */
     if(hupdate && Validchar(who)) {
        if(!(message->flags & MESSAGE_EVERYONE)) {
           for(reader = message->readers, last = NULL; reader; last = reader, reader = reader->next)
               if(reader->reader == who) {
                  reader->flags |= ((hupdate != 2) ? READER_READ:(READER_READ|READER_IGNORE));
                  if(hupdate != 2) reader->flags &= ~READER_IGNORE;
                  return;
	       }

           MALLOC(new,struct bbs_reader_data);
           new->reader = who;
           new->flags  = 0;
           new->flags |= ((hupdate != 2) ? READER_READ:(READER_READ|READER_IGNORE));
           new->next   = NULL;
           if(last) {
              last->next = new;
	   } else message->readers = new;
	} else if(supdate) message->readercount++;
     }
}

/* ---->  Update expiry time of votes on BBS message  <---- */
void bbs_update_vote_expiry()
{
     struct bbs_topic_data *topic,*subtopic;
     struct bbs_message_data *message;

     for(topic = bbs; topic; topic = topic->next)
         for(subtopic = topic; subtopic; subtopic = (subtopic == topic) ? topic->subtopics:subtopic->next)
             for(message = subtopic->messages; message; message = message->next)
                 if(message->expiry)
                    if(!(--(message->expiry)))
                       message->flags &= ~(MESSAGE_VOTING|MESSAGE_PRIVATE);
}

/* ---->  Unread message?  <---- */
unsigned char bbs_unread_message(struct bbs_message_data *message,dbref who,unsigned char *ignored)
{
	 struct bbs_reader_data *reader = message->readers;

	 *ignored = 0;
	 bbs_update_readers(message,NOTHING,0,0);
	 if((message->owner == who) && !(message->flags & MESSAGE_APPEND)) return(0);
	 for(reader = message->readers; reader; reader = reader->next)
	     if((reader->reader == who) && ((reader->flags & READER_READ) || (reader->flags & READER_IGNORE))) {
		if(reader->flags & READER_IGNORE) *ignored = 1;
		return(0);
	     }

	 if(!(message->flags & MESSAGE_EVERYONE)) return(1);
	 return(0);
}

/* ---->  Count readers/voters of given message  <---- */
short bbs_readercount(struct bbs_message_data *message)
{
      struct bbs_reader_data *reader = message->readers;
      short  count = 0;

      bbs_update_readers(message,NOTHING,0,0);
      for(; reader; reader = reader->next)
          if((reader->flags & READER_READ) && !(reader->flags & READER_IGNORE))
             if(reader->reader != message->owner) count++;
      return(count + message->readercount);
}

/* ---->  Count replies to message (Messages in topic which have same ID number)  <---- */
short bbs_replycount(struct bbs_topic_data *topic,struct bbs_message_data *message,unsigned short id,unsigned char trailing)
{
      struct   bbs_message_data *ptr;
      unsigned char process = 0;
      short    count = 0;

      if(!topic) return(0);
      for(ptr = topic->messages; ptr; ptr = ptr->next)
          if(trailing) {
             if(ptr != message) {
                if(process && (ptr->id == id)) count++;
	     } else process = 1;
	  } else if(ptr->id == id) count++;
      return(count);
}

/* ---->  Summarise voters of given message  <---- */
short bbs_votecount(struct bbs_message_data *message,short *vfor,short *vagainst,short *vabstain,short *forscore,short *againstscore,short *abstainscore)
{
      struct bbs_reader_data *reader = message->readers;
      short  count = 0;

      bbs_update_readers(message,NOTHING,0,0);
      *vfor = *vagainst = *vabstain = *forscore = *againstscore = *abstainscore = 0;
      for(; reader; reader = reader->next)
          if(reader->flags & READER_VOTE_MASK) {
             switch(reader->flags & READER_VOTE_MASK) {
                    case READER_VOTE_FOR:
                         (*vfor)++;
                         (*forscore) += vote_value(reader->reader,((message->flags & MESSAGE_MAJORITY) != 0));
                         break;
                    case READER_VOTE_AGAINST:
                         (*vagainst)++;
                         (*againstscore) += vote_value(reader->reader,((message->flags & MESSAGE_MAJORITY) != 0));
                         break;
                    case READER_VOTE_ABSTAIN:
                         (*vabstain)++;
                         (*abstainscore) += vote_value(reader->reader,((message->flags & MESSAGE_MAJORITY) != 0));
                         break;
	     }
             count++;
	  }
      return(count);
}

/* ---->  Count available topics on BBS  <---- */
short bbs_topiccount(struct bbs_topic_data *start)
{
      short count = 0;

      for(; start; start = start->next, count++);
      return(count);
}

/* ---->  Count messages in given topic  <---- */
short bbs_messagecount(struct bbs_message_data *messages,dbref who,short *new)
{
      short         count = 0;
      unsigned char ignored;

      *new = 0;
      for(; messages; messages = messages->next, count++)
          if(bbs_unread_message(messages,who,&ignored)) (*new)++;
      return(count);
}

/* ---->  Look up topic/sub-topic by name on BBS, returning pointer to it if found  <---- */
struct bbs_topic_data *lookup_topic(dbref player,char *name,struct bbs_topic_data **last,struct bbs_topic_data **subtopic)
{
       char   *subtopicname = NULL,*topicname,*tmp = NULL;
       struct bbs_topic_data *ptr = bbs;
       char   c;

       (*last) = NULL, (*subtopic) = NULL;
       if(Blank(name)) {

          /* ---->  Find character's current selected topic/sub-topic  <---- */
          if(Validchar(player)) {
             for(; ptr; (*last) = ptr, ptr = ptr->next)
                 if(ptr->topic_id == db[player].data->player.topic_id) {
                    if(db[player].data->player.subtopic_id)
                       for((*subtopic) = ptr, ptr = ptr->subtopics, (*last) = NULL; ptr; (*last) = ptr, ptr = ptr->next)
                           if(ptr->topic_id == db[player].data->player.subtopic_id)
                              return(ptr);
                    return(ptr);
		 }
             return(NULL);
	  } else return(NULL);
       } else {
          for(; *name && (*name == ' '); name++);
          for(topicname = name; *name && (*name != '/') && (*name != '\\') && (*name != ';'); name++);
          if(*name && ((*name == '/') || (*name == '\\') || (*name == ';')))
             c = *name, tmp = name, *name++ = '\0', subtopicname = name;
          bbs_topicname_filter(topicname);
          if(subtopicname) bbs_topicname_filter(subtopicname);

          if(Blank(topicname)) {

             /* ---->  Sub-topic within character's current selected topic  <---- */
             if(Validchar(player)) {
                for(; ptr; (*last) = ptr, ptr = ptr->next)
                    if(ptr->topic_id == db[player].data->player.topic_id) {
                       if(!Blank(subtopicname)) {
	                  for((*subtopic) = ptr, ptr = ptr->subtopics, (*last) = NULL; ptr; (*last) = ptr, ptr = ptr->next)
                              if(ptr->name && string_matched_prefix(subtopicname,ptr->name))
                                 return(ptr);
                          if(tmp) *tmp = c;
                          return(NULL);
		       } else return(ptr);
		    }
                return(NULL);
	     } else return(NULL);
	  } else for(; ptr; (*last) = ptr, ptr = ptr->next) {
             if(ptr->name && string_matched_prefix(topicname,ptr->name)) {
                if(!Blank(subtopicname)) {
	           for((*subtopic) = ptr, ptr = ptr->subtopics, (*last) = NULL; ptr; (*last) = ptr, ptr = ptr->next)
                       if(ptr->name && string_matched_prefix(subtopicname,ptr->name))
                          return(ptr);
                   if(tmp) *tmp = c;
                   return(NULL);
		} else return(ptr);
	     }
	  }
       }
       return(NULL);
}

/* ---->  Search for next/previous occurence of given text in message text/subjects of messages  <---- */
struct bbs_message_data *bbs_search(struct descriptor_data *p,dbref player,const char *text,unsigned char searchmode,unsigned char direction,struct bbs_topic_data **searchtopic,struct bbs_topic_data **searchsubtopic,short *searchmsgno)
{
       struct   bbs_topic_data *starttopic,*startsubtopic,*topic,*subtopic,*search;
       struct   bbs_message_data *message,*startmessage;
       unsigned char found = 0,wrapped = 0;
       struct   descriptor_data *d;
       short    startmsgno,msgno;

       /* ---->  Find start point of search on BBS (User's current selected topic/sub-topic and last read message)  <---- */
       if(Blank(text) || !bbs) return(NULL);
       if(!p) p = getdsc(player);
       if(!(starttopic = lookup_topic(player,NULL,&starttopic,&startsubtopic))) starttopic = bbs, startsubtopic = NULL;
       for(startmessage = starttopic->messages, startmsgno = 1; startmessage && !(p && (startmsgno == p->currentmsg)); startmessage = startmessage->next, startmsgno++);
       if(!startmessage) startmsgno = 0;

       if(startsubtopic) search = starttopic, starttopic = startsubtopic, startsubtopic = search;
       topic = starttopic, subtopic = startsubtopic, message = startmessage, msgno = startmsgno;
       search = (startsubtopic) ? startsubtopic:starttopic;
       if(direction) {

          /* ---->  Search for next occurence  <---- */
          if(message) message = message->next, msgno++;
             else message = search->messages, msgno = 1;

          do {

             /* ---->  Search messages  <---- */
             if(message && can_access_topic(player,search,NULL,0)) {
                while(message && !found && (!(wrapped && (topic == starttopic) && (subtopic == startsubtopic)) || (msgno < startmsgno)))
                      if(((searchmode & SEARCH_SUBJECT) && instring(text,decompress(message->subject))) || ((searchmode & SEARCH_MESSAGE) && instring(text,decompress(message->message)))) found = 1;
                         else message = message->next, msgno++;
	     }

             /* ---->  Move to next topic/sub-topic  <---- */
             if(!found) {
                if(!wrapped) {
                   if(subtopic && subtopic->next) subtopic = search = subtopic->next;
                      else if(!subtopic && topic->subtopics) subtopic = search = topic->subtopics;
                         else if(!(topic = search = topic->next)) topic = search = bbs, subtopic = NULL;
                            else subtopic = NULL;

                   message = search->messages, msgno = 1;
                   if((topic == starttopic) && (subtopic == startsubtopic))
                      wrapped = 1;
		} else wrapped = 2;
	     }
	  } while(!found && (wrapped != 2));

          if(found && topic) {

             /* ---->  Message found?  <---- */
             db[player].data->player.subtopic_id = (subtopic) ? subtopic->topic_id:0;
   	     db[player].data->player.topic_id    = topic->topic_id;
             *searchtopic = (subtopic) ? subtopic:topic, *searchsubtopic = (subtopic) ? topic:subtopic, *searchmsgno = msgno;
             for(d = descriptor_list; d; d = d->next)
                 if(d->player == player) d->currentmsg = msgno;
             return(message);
	  } else return(NULL);
       } else {
          struct bbs_message_data *last = NULL;
          short  lastmsgno = 0;

          /* ---->  Search for previous occurence  <---- */
          if(message) message = search->messages, msgno = 1;
          do {

             /* ---->  Search messages  <---- */
             if(message && can_access_topic(player,search,NULL,0)) {
                for(last = NULL, lastmsgno = 0; message && (wrapped || !((topic == starttopic) && (subtopic == startsubtopic)) || (msgno < startmsgno)); message = message->next, msgno++)
                    if(((searchmode & SEARCH_SUBJECT) && instring(text,decompress(message->subject))) || ((searchmode & SEARCH_MESSAGE) && instring(text,decompress(message->message)))) last = message, lastmsgno = msgno;
	     }

             /* ---->  Move to previous topic/sub-topic  <---- */
             if(!last) {
                if(!wrapped) {
                   if(subtopic) {
                      if(subtopic != topic->subtopics) {
                         for(search = topic->subtopics; search && (search->next != subtopic); search = search->next);
                         subtopic = search;
		      } else search = topic, subtopic = NULL;
		   } else {
                      for(search = bbs; search && search->next && (search->next != topic); search = search->next);
                      if((topic = search) && topic->subtopics) {
                         for(search = topic->subtopics; search && search->next; search = search->next);
                         subtopic = search;
		      } else search = topic, subtopic = NULL;
		   }

                   message = search->messages, msgno = 1;
                   if((topic == starttopic) && (subtopic == startsubtopic)) wrapped = 1;
		} else wrapped = 2;
	     }
	  } while(!last && (wrapped != 2));

          if(last && topic && (!wrapped || (lastmsgno > startmsgno))) {

             /* ---->  Message found?  <---- */
             db[player].data->player.subtopic_id = (subtopic) ? subtopic->topic_id:0;
   	     db[player].data->player.topic_id    = topic->topic_id;
             *searchtopic = (subtopic) ? subtopic:topic, *searchsubtopic = (subtopic) ? topic:subtopic, *searchmsgno = lastmsgno;
             for(d = descriptor_list; d; d = d->next)
                 if(d->player == player) d->currentmsg = lastmsgno;
             return(last);
	  } else return(NULL);
       }
}

/* ---->  Lookup message by number in topic TOPIC  <---- */
struct bbs_message_data *lookup_message(dbref player,struct bbs_topic_data **topic,struct bbs_topic_data **subtopic,const char *messageno,struct bbs_message_data **last,short *no,unsigned char msg)
{
       struct   bbs_message_data *ptr,*ptr2 = NULL,*last2 = NULL;
       struct   descriptor_data *d,*p = getdsc(player);
       unsigned char ignored,unread = 1;
       struct   bbs_topic_data *temp;
       unsigned char wrapped;
       short    number,no2;
       char     dir = 0;

       (*no) = 0, (*last) = NULL;
       if(Blank(messageno)) return(NULL);
       if(p) {
          if(!strcasecmp("FIRST",messageno) || !strcasecmp("ALL",messageno)) number = FIRST;
              else if(!strcasecmp("LAST",messageno) || !strcasecmp("END",messageno) || !strcasecmp("LATEST",messageno)) number = LAST;
                 else if(!strcasecmp("NEXT",messageno)) number = (!p->currentmsg) ? 0:p->currentmsg, dir = 1;
                    else if(!strcasecmp("PREV",messageno) || !strcasecmp("PREVIOUS",messageno)) number = (!p->currentmsg) ? 0:p->currentmsg, dir = -1;
		       else if(!strcasecmp("CURRENT",messageno)) number = (!p->currentmsg) ? 0:p->currentmsg;
                          else if(!strcasecmp("IGNORE",messageno) || !strcasecmp("IGNORED",messageno)) number = 0, unread = 0;
   		             else if(strcasecmp("NEW",messageno) && strcasecmp("UNREAD",messageno)) {
                                number = atol(messageno);
                                if(number < 1) return(NULL);
			     } else number = 0, unread = 1;
       } else {
          number = atol(messageno);
          if(number < 1) return(NULL);
       }

       switch(number) {
              case FIRST:

                   /* ---->  First message in topic  <---- */
                   for(ptr = (*topic)->messages, *no = 1; ptr && msg && (ptr->flags & MESSAGE_REPLY); (*last) = ptr, (*no)++, ptr = ptr->next);
                   return((*topic)->messages);
              case LAST:

                   /* ---->  Last message in topic  <---- */
                   if(msg) {
                      ptr = (*topic)->messages, ptr2 = ptr, *no = 1;
                      while(ptr && ptr2) {
                            for(ptr2 = ptr->next; ptr2 && (ptr2->flags & MESSAGE_REPLY); ptr2 = ptr2->next);
                            if(ptr2) for((*last) = ptr, (*no)++, ptr = ptr->next; ptr && (ptr->flags & MESSAGE_REPLY); (*last) = ptr, (*no)++, ptr = ptr->next);
		      }
                      return(ptr);
		   } else for(ptr = (*topic)->messages, *no = 1; ptr && ptr->next; (*last) = ptr, (*no)++, ptr = ptr->next);
                   return(ptr);
              case DEFAULT:

                   /* ---->  Next unread/ignored message on BBS  <---- */
                   for(d = descriptor_list; d; d = d->next)
                       if((d->player == player) && d->edit)
                          return(NULL);

                   if(*subtopic) temp = *subtopic, *subtopic = *topic, *topic = temp;
                   if(!(*topic)) *topic = bbs, *subtopic = NULL;
                   wrapped = 0;

                   /* ---->  Search topics for unread message  <---- */
                   while(*topic) {
                         if(can_access_topic(player,*topic,NULL,0)) {
                            if(!(*subtopic))
                               for(ptr = (*topic)->messages, *no = 1; ptr; (*last) = ptr, (*no)++, ptr = ptr->next)
                                   if((!msg || !(ptr->flags & MESSAGE_REPLY)) && (bbs_unread_message(ptr,player,&ignored) || (!unread && ignored))) {
                                      db[player].data->player.subtopic_id = 0;
   		      		      db[player].data->player.topic_id    = (*topic)->topic_id;
                                      *subtopic = NULL;
                                      return(ptr);
				   }

                            /* ---->  Search topic's sub-topics for unread message  <---- */
                            if(!(*subtopic)) *subtopic = (*topic)->subtopics;
                            while(*subtopic) {
                                  if(can_access_topic(player,*subtopic,*topic,0))
                                     for(ptr = (*subtopic)->messages, *no = 1; ptr; (*last) = ptr, (*no)++, ptr = ptr->next)
                                         if((!msg || !(ptr->flags & MESSAGE_REPLY)) && (bbs_unread_message(ptr,player,&ignored) || (!unread && ignored))) {
                                            db[player].data->player.subtopic_id = (*subtopic)->topic_id;
   		        		    db[player].data->player.topic_id    = (*topic)->topic_id;
                                            temp = *subtopic, *subtopic = *topic, *topic = temp;
                                            return(ptr);
					 }
                                  *subtopic = (*subtopic)->next;
			    }
			 }

                         if(!(*topic = (*topic)->next))
                            if(!wrapped) *topic = bbs, wrapped = 1;
		   }
                   return(NULL);
              default:

                   /* ---->  Given message number in topic  <---- */
                   switch(dir) {
                          case -1:

                               /* ---->  Read previous message  <---- */
                               for(ptr = (*topic)->messages, *no = 0; ptr && (++(*no) != number); (*last) = ptr, ptr = ptr->next)
                                   if(msg) {
                                      if(ptr && !(ptr->flags & MESSAGE_REPLY))
                                         no2 = *no, ptr2 = ptr, last2 = (*last);
				   } else no2 = *no, ptr2 = ptr, last2 = (*last);
                               *no = no2, (*last) = last2;
                               return(ptr2);
                          case 0:

                               /* ---->  Read message with given number  <---- */
                               for(ptr = (*topic)->messages, *no = 0; ptr; (*last) = ptr, ptr = ptr->next)
                                   if(++(*no) == number) return(ptr);
                               break;
                          case 1:

                               /* ---->  Read next message  <---- */
                               for(ptr = (*topic)->messages, *no = 0; ptr && (++(*no) != number); (*last) = ptr, ptr = ptr->next);
                               if(msg) {
                                  if(ptr) for((*no)++, (*last) = ptr, ptr = ptr->next; ptr && (ptr->flags & MESSAGE_REPLY); (*no)++, (*last) = ptr, ptr = ptr->next);
			       } else if(ptr) (*no)++, (*last) = ptr, ptr = ptr->next;
                               return(ptr);
		   }
       }
       return(NULL);
}

/* ---->  Can given character access given topic?  <---- */
unsigned char can_access_topic(dbref player,struct bbs_topic_data *topic,struct bbs_topic_data *subtopic,unsigned char log)
{
	 unsigned char access;

	 if(!topic) return(0);
	 if(!(access = ((Validchar(topic->owner) && (player == topic->owner)) || (privilege(player,255) <= topic->accesslevel))) && in_command) {
	    if((access = ((Validchar(topic->owner) && (db[player].owner == topic->owner)) || (privilege(db[player].owner,255) <= topic->accesslevel)))) {
	       if(log) {
		  if(subtopic)
		     writelog((topic->accesslevel < 5) ? ADMIN_LOG:HACK_LOG,1,"HACK","%s%s(#%d) obtained unauthorised access to BBS sub-topic '%s' in the topic '%s' from within compound command %s(#%d) owned by %s(#%d).",Level4(player) ? "":"Mortal ",getname(player),player,topic->name,subtopic->name,getname(current_command),current_command,getname(db[current_command].owner),db[current_command].owner);
	       } else writelog((topic->accesslevel < 5) ? ADMIN_LOG:HACK_LOG,1,"HACK","%s%s(#%d) obtained unauthorised access to BBS topic '%s' from within compound command %s(#%d) owned by %s(#%d).",Level4(player) ? "":"Mortal ",getname(player),player,topic->name,getname(current_command),current_command,getname(db[current_command].owner),db[current_command].owner);
	    }
	 }
	 return(access);
}

/* ---->  Output to users who can access topic in BBS room  <---- */
void bbs_output_except(dbref exception,char accesslevel,unsigned char raw,unsigned char redirect,unsigned char wrap,char *fmt, ...)
{
     struct descriptor_data *d;

     va_start(output_ap,fmt);
     output_fmt = &fmt;
     for(d = descriptor_list; d; d = d->next)
         if((d->flags & CONNECTED) && Validchar(d->player) && ((db[d->player].location == bbsroom) || (Bbsinform(d->player) && !Quiet(d->player))) && (d->player != exception) && (privilege(db[d->player].owner,255) <= accesslevel))
            output(d,d->player,raw,redirect,wrap,NULL);
     va_end(output_ap);
}

/* ---->  Adjust CURRENTMSG of descriptors when adding/deleting messages  <---- */
void bbs_adjustcurrentmsg(unsigned short topic_id,unsigned short subtopic_id,unsigned short msg,unsigned char add)
{
     struct descriptor_data *d;

     if(add) {
        struct bbs_topic_data *topic;

        if(subtopic_id) {
           for(topic = bbs; topic && (topic->topic_id != subtopic_id); topic = topic->next);
           if(topic) for(topic = topic->subtopics; topic && (topic->topic_id != topic_id); topic = topic->next);
        } else for(topic = bbs; topic && (topic->topic_id != topic_id); topic = topic->next);

        if(topic && (topic->flags & TOPIC_CYCLIC)) {
           struct bbs_message_data *message;
           short  count = 0;

           for(message = topic->messages; message; message = message->next, count++);
           if(count < topic->messagelimit) return;
	}
     }

     for(d = descriptor_list; d; d = d->next) {
         if((d->flags & CONNECTED) && Validchar(d->player) && ((subtopic_id && (db[d->player].data->player.topic_id == subtopic_id) && (db[d->player].data->player.subtopic_id == topic_id)) || (!subtopic_id && (db[d->player].data->player.topic_id == topic_id) && !db[d->player].data->player.subtopic_id))) {
            if(!add) {
	       if(d->currentmsg > msg) d->currentmsg--;
               if(d->currentmsg < 1)   d->currentmsg = 1;
	    } else if(d->currentmsg <= msg) d->currentmsg++;
	 }
     }
}

/* ---->  Add message to topic with topic ID TOPIC_ID  <---- */
unsigned char bbs_addmessage(dbref player,unsigned short topic_id,unsigned short subtopic_id,char *subject,char *text,long date,unsigned char flags,unsigned short msgid)
{
       struct   bbs_message_data *message,*new,*last = NULL;
       struct   bbs_topic_data *topic;
       unsigned short id = 0;
       short    count = 1;
       time_t   now;

       /* ---->  Lookup topic/sub-topic  <---- */
       if(subtopic_id) {
          for(topic = bbs; topic && (topic->topic_id != subtopic_id); topic = topic->next);
          if(topic) for(topic = topic->subtopics; topic && (topic->topic_id != topic_id); topic = topic->next);
       } else for(topic = bbs; topic && (topic->topic_id != topic_id); topic = topic->next);
       if(!topic) return(0);
       for(message = topic->messages; message && (message->date <= date); last = message, count++, message = message->next);
       bbs_adjustcurrentmsg(topic_id,subtopic_id,count,1);

       /* ---->  Message data  <---- */
       gettime(now);
       MALLOC(new,struct bbs_message_data);
       new->readercount = 0;
       new->lastread    = now;
       new->message     = (char *) alloc_string(compress(text,1));
       new->readers     = NULL;
       new->subject     = (char *) alloc_string(compress(punctuate(subject,2,'\0'),1));
       new->expiry      = 0;
       new->flags       = flags;
       new->flags      |= MESSAGE_ALLOWAPPEND;
       new->owner       = db[player].owner;
       new->date        = date;
       new->name        = (char *) alloc_string(compress(getcname(NOTHING,db[player].owner,0,UPPER|DEFINITE),0));

       /* ---->  Add to linked list of messages  <---- */
       if(last) last->next = new;
          else topic->messages = new;
       if(message) new->next = message;
          else new->next = NULL;

       /* ---->  Allocate unique ID number to message  <---- */
       if(!msgid) {
          while(!id) {
                for(message = topic->messages, id = (lrand48() % 65535) + 1; message && (message->id != id); message = message->next);
                if(message) id = 0;
	  }
          new->id = id;
       } else new->id = msgid;
       return(count);
}

/* ---->  Append given text to specified BBS message  <---- */
void bbs_appendmessage(dbref player,struct bbs_message_data *message,struct bbs_topic_data *topic,struct bbs_topic_data *subtopic,const char *append,unsigned char anon)
{
     unsigned char truncated = 0,readers = 0, replied = 0;
     struct   bbs_reader_data *reader,*next,*last = NULL;
     time_t   now;

     gettime(now);
     if(anon) sprintf(scratch_return_string,"\n\n%%c%%l[Appended by an anonymous user on %%g%%l%%{@?realtime {@eval %d + ({@?timediff} * 3600)}}%%c%%l...]\n%%x%s",(int)now,punctuate((char *) append,2,'\0'));
        else sprintf(scratch_return_string,"\n\n%%c%%l[Appended by %s%%w%%l%s%%c%%l on %%g%%l%%{@?realtime {@eval %d + {@?bbsrtd}}}%%c%%l...]\n%%x%s",Article(player,LOWER,INDEFINITE),getcname(NOTHING,player,0,0),(int)now,punctuate((char *) append,2,'\0'));
     if(strlen(scratch_return_string) > MAX_LENGTH) scratch_return_string[TEXT_SIZE] = '\0', truncated = 1;

     sprintf(scratch_buffer,"%s%s",decompress(message->message),scratch_return_string);
     if(strlen(scratch_buffer) > MAX_LENGTH) {
        short temp;

        /* ---->  Insufficient space for append - Post reply and append to it  <---- */
        if(topic->flags & TOPIC_ADD)
	   if(!(!(topic->flags & TOPIC_CYCLIC) && (bbs_messagecount(topic->messages,NOTHING,&temp) >= topic->messagelimit)))
              replied = 1;

        if(replied) {

           /* ---->  Add indication to original message and mark unread  <---- */
           sprintf(scratch_buffer,"%s\n\n%%c%%l[Continued...]",decompress(message->message));
           if(strlen(scratch_buffer) <= MAX_LENGTH) {
              FREENULL(message->message);
              message->message =  (char *) alloc_string(compress(scratch_buffer,1));
              message->flags  &= ~MESSAGE_APPEND;
              readers          =  1;
	   } else message->flags &= ~MESSAGE_APPEND;

           if(topic->flags & TOPIC_CYCLIC) bbs_cyclicdelete(topic->topic_id,(subtopic) ? subtopic->topic_id:0);
           strcpy(scratch_buffer,decompress(message->subject));
           bbs_addmessage(message->owner,topic->topic_id,(subtopic) ? subtopic->topic_id:0,scratch_buffer,scratch_return_string + 1,message->date,message->flags,message->id);

           if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LRED"Your append has made the original message too large  -  It has been automatically left as a reply to the message.\n");
	}
     }

     if(!replied) {

        /* ---->  Append to message as normal  <---- */
        FREENULL(message->message);
        message->message = (char *) alloc_string(compress(scratch_buffer,1));
        message->flags  |= MESSAGE_APPEND;
        readers          = 1;
     }

     if(readers)
        for(reader = message->readers; reader; reader = next) {
            next = reader->next;
            if(!(reader->flags & READER_VOTE_MASK)) {
               if(last) last->next = reader->next;
                  else message->readers = reader->next;
               FREENULL(reader);
   	    } else {
               reader->flags &= ~(READER_READ|READER_IGNORE);
               last           =  reader;
	    }
	}

     if(!in_command && truncated) output(getdsc(player),player,0,1,0,ANSI_LRED"Your append has been truncated as it has made the message too large  -  Please check the message and make appropriate adjustments (If neccessary) using the '"ANSI_LWHITE"modify"ANSI_LRED"' BBS command.\n");
}

/* ---->  Cyclicly delete old message from topic with topic ID TOPIC_ID  <---- */
void bbs_cyclicdelete(unsigned short topic_id,unsigned short subtopic_id)
{
     struct bbs_message_data *message,*oldest,*last = NULL, *lastoldest = NULL;
     short  count = 0,oldcount = 0,limit = 0;
     struct bbs_reader_data *readers,*next;
     struct bbs_topic_data *topic;

     if(subtopic_id) {
        for(topic = bbs; topic && (topic->topic_id != subtopic_id); topic = topic->next);
        if(topic) for(topic = topic->subtopics; topic && (topic->topic_id != topic_id); topic = topic->next);
     } else for(topic = bbs; topic && (topic->topic_id != topic_id); topic = topic->next);
     if(!(topic && topic->messages && (topic->flags & TOPIC_CYCLIC))) return;

     message = topic->messages, oldest = topic->messages, oldcount = 1;
     for(; message; message = message->next, count++);
     if(count < topic->messagelimit) return;
     limit = ((count / BBS_CYCLIC_REGION) * 10);
     for(message = topic->messages, count = 0; message && (count < limit); last = message, message = message->next, count++)
         if((message->lastread <= oldest->lastread) && (bbs_readercount(message) < bbs_readercount(oldest))) oldest = message, lastoldest = last, oldcount = count + 1;
            else if(message->lastread < oldest->lastread) oldest = message, lastoldest = last, oldcount = count + 1;
         
     if(oldest && !(oldest->flags & MESSAGE_MODIFY)) {
        bbs_adjustcurrentmsg(topic_id,subtopic_id,oldcount,0);
        if(lastoldest) lastoldest->next = oldest->next;
           else topic->messages = oldest->next;

        /* ---->  Readers list  <---- */
        for(readers = oldest->readers; readers; readers = next) {
            next = readers->next;
            FREENULL(readers);
	}

        /* ---->  Message data  <---- */
        FREENULL(oldest->message);
        FREENULL(oldest->subject);
        FREENULL(oldest->name);
        FREENULL(oldest);
     }
}

/* ---->  Delete out-of-date messages from time restricted topics  <---- */
void bbs_delete_outofdate()
{
     struct bbs_message_data *message,*last,*next;
     struct bbs_topic_data *topic,*subtopic;
     struct bbs_reader_data *reader,*cache;
     int    deleted = 0;
     time_t now,limit;
     short  count = 0;

     gettime(now);
     for(topic = bbs; topic; topic = topic->next) {

         /* ---->  Messages within topic  <---- */
         if(topic->timelimit) {
            limit = now - (topic->timelimit * DAY);
            for(message = topic->messages, last = NULL, count = 1; message; message = next) {
                next = message->next;
                if((message->lastread < limit) && !(message->flags & MESSAGE_MODIFY)) {

                   /* ---->  Readers list  <---- */
                   for(reader = message->readers; reader; reader = cache) {
                       cache = reader->next;
                       FREENULL(reader);
		   }

                   /* ---->  Message data  <---- */
                   bbs_adjustcurrentmsg(topic->topic_id,0,count,0);
                   if(last) last->next = message->next;
                      else topic->messages = message->next;
                   FREENULL(message->message);
                   FREENULL(message->subject);
                   FREENULL(message->name);
                   FREENULL(message);
                   deleted++;
		} else {
                   bbs_update_readers(message,NOTHING,0,0);
                   last = message, count++;
		}
	    }
	 }

         /* ---->  Messages within sub-topics of topic  <---- */
         for(subtopic = topic->subtopics; subtopic; subtopic = subtopic->next)
             if(subtopic->timelimit) {
                limit = now - (subtopic->timelimit * DAY);
                for(message = subtopic->messages, last = NULL, count = 1; message; message = next) {
                    next = message->next;
                    if((message->lastread < limit) && !(message->flags & MESSAGE_MODIFY)) {

                       /* ---->  Readers list  <---- */
                       for(reader = message->readers; reader; reader = cache) {
                           cache = reader->next;
                           FREENULL(reader);
		       }

                       /* ---->  Message data  <---- */
                       bbs_adjustcurrentmsg(subtopic->topic_id,topic->topic_id,count,0);
                       if(last) last->next = message->next;
                          else subtopic->messages = message->next;
                       FREENULL(message->message);
                       FREENULL(message->subject);
                       FREENULL(message->name);
                       FREENULL(message);
                       deleted++;
		    } else {
                       bbs_update_readers(message,NOTHING,0,0);
                       last = message, count++;
		    }
		}
	     }
     }
     gettime(limit);
     writelog(MAINTENANCE_LOG,1,"MAINTENANCE","%d unread and out-of-date BBS message%s removed (BBS maintenance took %s.)",deleted,Plural(deleted),interval(limit - now,limit - now,ENTITIES,0));
}


/*  ========================================================================  */
/*    ---->                  Standard BBS commands                   <----    */
/*  ========================================================================  */


/* ---->  Add message (Start editor if MESSAGE parameter is omitted)  <---- */
/*        (val1:  0 = Normal, 1 = Anonymous.)                               */
void bbs_add(CONTEXT)
{
     struct bbs_topic_data *topic,*subtopic;
     static dbref postwho = NOTHING;
     static time_t posttime = 0;
     short  temp;
     time_t now;

     gettime(now);
     comms_spoken(player,0);
     setreturn(ERROR,COMMAND_FAIL);
     if(!Moron(player)) {
        if(Bbs(player)) {
           if(!(!Level4(player) && (postwho == player) && (now < posttime))) {
              if((topic = lookup_topic(player,NULL,&topic,&subtopic))) {
                 if(!subtopic || can_access_topic(player,subtopic,NULL,1)) {
	            if(can_access_topic(player,topic,subtopic,1)) {
	               if(!(!Level4(db[player].owner) && !(topic->flags & TOPIC_MORTALADD))) {
  	                  if(topic->flags & TOPIC_ADD) {
             	             if(!(val1 && !(topic->flags & TOPIC_ANON))) {
	                        if(!(!(topic->flags & TOPIC_CYCLIC) && (bbs_messagecount(topic->messages,NOTHING,&temp) >= topic->messagelimit))) {
             	                   if(!Blank(arg1)) {
                                      if(!((strlen(arg1) > 50) || strchr(arg1,'\n'))) {
                                         if(!instring("%{",arg1)) {
                                            if(!instring("%h",arg1)) {
                                               posttime = now + POST_TIME, postwho = player;
                                               if(!Blank(arg2)) {
                                                  ansi_code_filter(arg1,arg1,0);
                                                  if(topic->flags & TOPIC_CYCLIC) bbs_cyclicdelete(topic->topic_id,(subtopic) ? subtopic->topic_id:0);
                                                  temp = bbs_addmessage(player,topic->topic_id,(subtopic) ? subtopic->topic_id:0,arg1,arg2,now,(val1) ? MESSAGE_ANON:0,0);
                                                  if(!in_command) {
                                                     substitute(player,scratch_return_string,punctuate(arg1,2,'\0'),0,ANSI_LYELLOW,NULL,0);
                                                     if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                                                     if(subtopic) {
                                                        output(getdsc(player),player,0,1,0,ANSI_LGREEN"%sessage '"ANSI_LYELLOW"%s"ANSI_LGREEN"' added to the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)",(val1) ? "Anonymous m":"M",scratch_return_string,topic->name,subtopic->name,temp);
                                                        if(val1) bbs_output_except(player,MIN(topic->accesslevel,((subtopic) ? subtopic->accesslevel:255)),0,1,2,ANSI_LGREEN"An anonymous user adds a message with the subject '"ANSI_LYELLOW"%s"ANSI_LGREEN"' to the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)",scratch_return_string,topic->name,subtopic->name,temp);
                                                           else bbs_output_except(player,MIN(topic->accesslevel,((subtopic) ? subtopic->accesslevel:255)),0,1,2,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" adds a message with the subject '"ANSI_LYELLOW"%s"ANSI_LGREEN"' to the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0),scratch_return_string,topic->name,subtopic->name,temp);
						     } else {
                                                        output(getdsc(player),player,0,1,0,ANSI_LGREEN"%sessage '"ANSI_LYELLOW"%s"ANSI_LGREEN"' added to the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)",(val1) ? "Anonymous m":"M",scratch_return_string,topic->name,temp);
                                                        if(val1) bbs_output_except(player,MIN(topic->accesslevel,((subtopic) ? subtopic->accesslevel:255)),0,1,2,ANSI_LGREEN"An anonymous user adds a message with the subject '"ANSI_LYELLOW"%s"ANSI_LGREEN"' to the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)",scratch_return_string,topic->name,temp);
                                                           else bbs_output_except(player,MIN(topic->accesslevel,((subtopic) ? subtopic->accesslevel:255)),0,1,2,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" adds a message with the subject '"ANSI_LYELLOW"%s"ANSI_LGREEN"' to the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0),scratch_return_string,topic->name,temp);
						     }
						  }
                                                  setreturn(OK,COMMAND_SUCC);
					       } else {
                                                  if(editing(player)) return;
                                                  edit_initialise(player,(val1) ? 106:100,NULL,NULL,alloc_string(arg1),NULL,(topic->flags & TOPIC_CENSOR)|EDIT_LAST_CENSOR,MIN(topic->accesslevel,((subtopic) ? subtopic->accesslevel:255)));
                                                  substitute(player,scratch_return_string,punctuate(arg1,2,'\0'),0,ANSI_LYELLOW,NULL,0);
                                                  if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                                                  if(subtopic) output(getdsc(player),player,0,1,0,ANSI_LCYAN"\nAdding %snew message with the subject '"ANSI_LYELLOW"%s"ANSI_LCYAN"' to the sub-topic '"ANSI_LWHITE"%s"ANSI_LCYAN"' in the topic '"ANSI_LWHITE"%s"ANSI_LCYAN"'...\n",(val1) ? "anonymous ":"",scratch_return_string,topic->name,subtopic->name);
                                                     else output(getdsc(player),player,0,1,0,ANSI_LCYAN"\nAdding %snew message with the subject '"ANSI_LYELLOW"%s"ANSI_LCYAN"' to the topic '"ANSI_LWHITE"%s"ANSI_LCYAN"'...\n",(val1) ? "anonymous ":"",scratch_return_string,topic->name);
                                                  output(getdsc(player),player,0,1,0,ANSI_LWHITE"Please enter your message (Pressing "ANSI_LCYAN"RETURN"ANSI_LWHITE" or "ANSI_LCYAN"ENTER"ANSI_LWHITE" after each line.)  Once you're finished, type '"ANSI_LGREEN".view"ANSI_LWHITE"' to view and check your message.  If you're happy with it, type '"ANSI_LGREEN".save"ANSI_LWHITE"' to save it and add it to %s BBS, otherwise type '"ANSI_LGREEN".abort = yes"ANSI_LWHITE"'.\n",tcz_full_name);
                                                  setreturn(OK,COMMAND_SUCC);
					       }
					    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the subject of your message can't contain embedded HTML tags.");
					 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the subject of your message can't contain query command substitutions ('"ANSI_LWHITE"%%{<QUERY COMMAND>}"ANSI_LGREEN"'.)");
				      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of the subject for your message is 50 characters.  It also must not contain embedded NEWLINE's.");
				   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify a subject for the message you'd like to add.");
				} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, there are too many messages in the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (A maximum of "ANSI_LYELLOW"%d"ANSI_LGREEN" message%s allowed.)",(subtopic) ? "sub-":"",topic->name,topic->messagelimit,(topic->messagelimit == 1) ? " is":"s are");
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, anonymous messages may not be added to the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",(subtopic) ? "sub-":"",topic->name);
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, messages can't be added to the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",(subtopic) ? "sub-":"",topic->name);
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Mortals may not add messages to the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",(subtopic) ? "sub-":"",topic->name);
		    } else {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.  Please choose another %stopic (Type '"ANSI_LWHITE"%s%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%s%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(subtopic) ? "sub-":"",topic->name,clevels[(int) topic->accesslevel],(subtopic) ? "sub-":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"");
                       if((subtopic && (db[player].data->player.topic_id == subtopic->topic_id) && (db[player].data->player.subtopic_id == topic->topic_id)) || (!subtopic && (db[player].data->player.topic_id == topic->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		    }
		 } else {
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)  Please choose another topic (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",subtopic->name,clevels[(int) subtopic->accesslevel],topic->name,(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
                    if(db[player].data->player.topic_id == topic->topic_id) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		 }
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please choose a topic first by typing '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"' (Type '"ANSI_LYELLOW"%stopics"ANSI_LGREEN"' to see the list of available topics.)",(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please wait "ANSI_LWHITE"%s"ANSI_LGREEN" before adding another message to the BBS.",interval(posttime - now,posttime - now,ENTITIES,0));
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from adding new messages to %s BBS.",tcz_full_name);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Morons aren't allowed to add new messages to %s BBS.",tcz_full_name);
}

/* ---->  Make message anonymous (So other Mortals can't see who added it.)  <---- */
void bbs_anonymous(CONTEXT)
{
     struct bbs_topic_data *topic,*subtopic;
     struct bbs_message_data *message;
     short  msgno;
     char   *ptr;

     /* ---->  Grab first word as message number  <---- */
     setreturn(ERROR,COMMAND_FAIL);
     if(Blank(arg2)) {
        for(; *arg1 && (*arg1 == ' '); arg1++);
        for(ptr = arg1; *ptr && (*ptr != ' '); ptr++);
        if(*ptr) for(*ptr = '\0', ptr++; *ptr && (*ptr == ' '); ptr++);
        arg2 = ptr;
     }

     if(!Moron(player)) {
        if((topic = lookup_topic(player,NULL,&topic,&subtopic))) {
           if(!subtopic || can_access_topic(player,subtopic,NULL,1)) {
  	      if(can_access_topic(player,topic,subtopic,1)) {
                 if((message = lookup_message(player,&topic,&subtopic,arg1,&message,&msgno,0))) {
                    if(!((db[player].owner != message->owner) && !can_write_to(player,message->owner,0))) {
                       if(Blank(arg2)) {
                          if(!(message->flags & MESSAGE_ANON)) {
	                     if(!(topic->flags & TOPIC_ANON)) {
                         	if(subtopic) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, messages may not be made anonymous in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",topic->name,subtopic->name);
                                   else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, messages may not be made anonymous in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",topic->name);
                                return;
			     } else if(!Bbs(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from making messages on %s BBS anonymous.",tcz_full_name);
		                else message->flags |= MESSAGE_ANON;
			  } else message->flags &= ~MESSAGE_ANON;
		       } else if(string_prefix("yes",arg2) || string_prefix("on",arg2)) {
     	                  if(!(topic->flags & TOPIC_ANON)) {
                             if(subtopic) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, messages may not be made anonymous in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",topic->name,subtopic->name);
                                else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, messages may not be made anonymous in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",topic->name);
                             return;
			  } else if(!Bbs(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from making messages on %s BBS anonymous.",tcz_full_name);
		             else message->flags |= MESSAGE_ANON;
		       } else if(string_prefix("no",arg2) || string_prefix("off",arg2)) message->flags &= ~MESSAGE_ANON;

                       substitute(Validchar(message->owner) ? message->owner:player,scratch_return_string,punctuate(decompress(message->subject),2,'\0'),0,ANSI_LYELLOW,NULL,0);
                       if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                       if(!in_command) {
                          if(subtopic) output(getdsc(player),player,0,1,0,ANSI_LGREEN"The message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LWHITE"%d"ANSI_LGREEN") in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is %s anonymous.",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,topic->name,subtopic->name,(message->flags & MESSAGE_ANON) ? "now":"no-longer");
                             else output(getdsc(player),player,0,1,0,ANSI_LGREEN"The message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LWHITE"%d"ANSI_LGREEN") in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is %s anonymous.",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,topic->name,(message->flags & MESSAGE_ANON) ? "now":"no-longer");
		       }
                       if(Owner(player) != message->owner)
                          writelog(BBS_LOG,1,"ANONYMOUS","%s%s(#%d) made this message %sanonymous.",bbs_logmsg(message,topic,subtopic,msgno,1),getname(player),player,(message->flags & MESSAGE_ANON) ? "":"non-"); 
                       setreturn(OK,COMMAND_SUCC);
		    } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only make a message you own or a message owned by someone of a lower level than yourself anonymous.");
                       else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only make your own messages anonymous.");
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a message with that number doesn't exist.");
	      } else {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.  Please choose another %stopic (Type '"ANSI_LWHITE"%s%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%s%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(subtopic) ? "sub-":"",topic->name,clevels[(int) topic->accesslevel],(subtopic) ? "sub-":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"");
                 if((subtopic && (db[player].data->player.topic_id == subtopic->topic_id) && (db[player].data->player.subtopic_id == topic->topic_id)) || (!subtopic && (db[player].data->player.topic_id == topic->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
	      }
	   } else {
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)  Please choose another topic (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",subtopic->name,clevels[(int) subtopic->accesslevel],topic->name,(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
              if(db[player].data->player.topic_id == topic->topic_id) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
	   }
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please choose a topic first by typing '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"' (Type '"ANSI_LYELLOW"%stopics"ANSI_LGREEN"' to see the list of available topics.)",(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Morons aren't allowed to make messages anonymous on %s BBS.",tcz_full_name);
}

/* ---->  Go to BBS room or execute given BBS command from anywhere on TCZ  <---- */
void bbs_bbs(CONTEXT)
{
     int    cached_commandtype = command_type;
     struct bbs_topic_data *topic,*subtopic;
     short  count;

     setreturn(ERROR,COMMAND_FAIL);
     if(Valid(bbsroom)) {
        if(!Blank(params)) {
           command_type = BBS_COMMAND;
           process_basic_command(player,params,0);
           if(command_type & BAD_COMMAND)
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"bbs"ANSI_LGREEN"' can only be used to execute "ANSI_LYELLOW"%s BBS"ANSI_LGREEN" commands.",tcz_full_name);
           command_type = cached_commandtype;
	} else if(db[player].location != bbsroom) {
           if(!Invisible(db[player].location))
              output_except(db[player].location,player,NOTHING,0,1,2,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" pops off to browse "ANSI_LYELLOW"%s BBS"ANSI_LGREEN".",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0),tcz_full_name);
           move_enter(player,bbsroom,0);

           if(db[player].data->player.topic_id) {
              if(!db[player].data->player.subtopic_id && (topic = lookup_topic(player,NULL,&topic,&subtopic)) && topic->subtopics && !bbs_messagecount(topic->messages,player,&count))
                 bbs_topics(player,NULL,NULL,NULL,NULL,1,0);
                    else bbs_view(player,NULL,NULL,NULL,NULL,0,0);
	   } else bbs_topics(player,NULL,NULL,NULL,NULL,0,0);
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"For full details on how to use "ANSI_LYELLOW"%s BBS"ANSI_LGREEN", please type '"ANSI_LWHITE"help bbs"ANSI_LGREEN"'.\n",tcz_full_name);
           setreturn(OK,COMMAND_SUCC);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you're already in %s BBS room  -  For full details on how to use the BBS, type '"ANSI_LWHITE"help bbs"ANSI_LGREEN"'.",tcz_full_name);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s BBS is unavailable at the moment.",tcz_full_name);
}

/* ---->  Ignore all or selected unread messages remaining on the BBS  <---- */
void bbs_ignore(CONTEXT)
{
     struct   bbs_topic_data *igntopic = NULL,*ignsubtopic = NULL;
     struct   bbs_topic_data *topic = bbs,*subtopic = NULL;
     int      subtopictotal = 0,topictotal = 0,total = 0;
     unsigned char all = 0,current = 0,cr = 0,ignored;
     struct   descriptor_data *p = getdsc(player);
     struct   bbs_message_data *message;

     setreturn(ERROR,COMMAND_FAIL);
     if(!strcasecmp("all",params)) all = 1;
     if(!in_command && p && !p->pager && !IsHtml(p) && More(player)) pager_init(p);
     if(all || (igntopic = lookup_topic(player,params,&igntopic,&ignsubtopic))) {
        if(all || can_access_topic(player,(ignsubtopic) ? ignsubtopic:igntopic,NULL,1)) {
           for(; topic; topic = topic->next)
               if(can_access_topic(player,topic,NULL,0)) {
                  topictotal = 0;
                  if(Blank(params)) current = 1;
                  if(all || (topic == ((ignsubtopic) ? ignsubtopic:igntopic)))
                     for(subtopic = topic; subtopic; subtopic = (subtopic == topic) ? topic->subtopics:subtopic->next)
                         if(can_access_topic(player,subtopic,topic,0)) {
                            subtopictotal = 0;
                            if(all || (!current && !ignsubtopic) || (subtopic == igntopic)) {
                               for(message = subtopic->messages; message; message = message->next)
                                   if(bbs_unread_message(message,player,&ignored)) {
                                      bbs_update_readers(message,player,2,0);
                                      subtopictotal++, topictotal++, total++;
				   }
			    }

                            if(!in_command && subtopictotal) {
                               if(subtopic == topic)
                                  output(p,player,0,1,0,"%s  \005\004"ANSI_LYELLOW"%d"ANSI_LMAGENTA" unread message%s in the topic '"ANSI_LWHITE"%s"ANSI_LMAGENTA"' ignored.",(cr) ? "":"\n",subtopictotal,Plural(subtopictotal),subtopic->name), cr = 1;
                                     else output(p,player,0,1,0,"%s     \005\007"ANSI_LYELLOW"%d"ANSI_LRED" unread message%s in the sub-topic '"ANSI_LWHITE"%s"ANSI_LRED"' in the topic '"ANSI_LWHITE"%s"ANSI_LRED"' ignored.",(cr) ? "":"\n",subtopictotal,Plural(subtopictotal),subtopic->name,topic->name), cr = 1;
			    }
			 }

                  if(!in_command && topictotal) {
                     if(!ignsubtopic) output(p,player,0,1,0,"\n\005\002"ANSI_LGREEN"Total of "ANSI_LYELLOW"%d"ANSI_LGREEN" unread message%s in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' ignored.\n",topictotal,Plural(topictotal),topic->name);
                        else output(p,player,0,1,0,"");
		  }
	       }

           if(!in_command) {
              if(all) {
                 if(total) output(p,player,0,1,0,"%s\005\002"ANSI_LGREEN"Total of "ANSI_LYELLOW"%d"ANSI_LGREEN" unread message%s on %s BBS ignored.%s",(cr) ? "":"\n",total,Plural(total),tcz_full_name,(cr) ? "\n":"");
                    else output(p,player,0,1,2,ANSI_LGREEN"Sorry, there are no unread messages on %s BBS at the moment.",tcz_full_name);
	      } else if(!total) {
                 if(ignsubtopic) output(p,player,0,1,2,ANSI_LGREEN"Sorry, there are no unread messages in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",igntopic->name,ignsubtopic->name);
                    else output(p,player,0,1,2,ANSI_LGREEN"Sorry, there are no unread messages within the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",igntopic->name);
	      }
	   }
           setreturn(OK,COMMAND_SUCC);
	} else {
           output(p,player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.  Please choose another topic (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(ignsubtopic) ? ignsubtopic->name:igntopic->name,clevels[(ignsubtopic) ? ignsubtopic->accesslevel:igntopic->accesslevel],(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
           if(db[player].data->player.topic_id == ((ignsubtopic) ? ignsubtopic->topic_id:igntopic->topic_id)) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
	}
     } else if(Blank(params)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please choose a topic first by typing '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"' (Type '"ANSI_LYELLOW"%stopics"ANSI_LGREEN"' to see the list of available topics.)",(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
        else output(p,player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist (Please type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' for a list of available topics.)",params,(command_type == BBS_COMMAND) ? "bbs ":"");
}

/* ---->  List latest messages added to the BBS  <---- */
void bbs_latest(CONTEXT)
{
     struct   bbs_topic_data *topic,*subtopic,*given = NULL,*givensubtopic = NULL;
     unsigned char count = 0,twidth = output_terminal_width(player);
     struct   descriptor_data *p = getdsc(player);
     struct   bbs_message_data *message;
     dbref    owner = NOTHING;
     short    loop,loop2,mno;
     int      copied;

     struct rank_data {
            struct bbs_message_data *message;
            struct bbs_topic_data *subtopic;
            struct bbs_topic_data *topic;
            short number;
     } ranking[BBS_MAX_LATEST];

     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(arg1)) {
        if(*arg1 == '*') {
           if((owner = lookup_character(player,arg1 + 1,1)) == NOTHING) {
              output(p,player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1 + 1);
              return;
	   }
	} else if(!strcasecmp("me",arg1)) owner = player;
     }

     if(owner == NOTHING) {
        bbs_topicname_filter(arg1);
        if(!Blank(arg1)) {
           if(!(given = lookup_topic(player,arg1,&given,&givensubtopic))) {
              output(p,player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
              return;
	   } else if(givensubtopic && !can_access_topic(player,givensubtopic,NULL,1)) {
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)  Please choose another topic (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",givensubtopic->name,clevels[(int) givensubtopic->accesslevel],given->name,(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
              if(db[player].data->player.topic_id == given->topic_id) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
              return;
	   } else if(!can_access_topic(player,given,givensubtopic,1)) {
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.  Please choose another %stopic (Type '"ANSI_LWHITE"%s%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%s%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(givensubtopic) ? "sub-":"",given->name,clevels[(int) given->accesslevel],(givensubtopic) ? "sub-":"",(command_type == BBS_COMMAND) ? "bbs ":"",(givensubtopic) ? "sub":"",(command_type == BBS_COMMAND) ? "bbs ":"",(givensubtopic) ? "sub":"");
              if((givensubtopic && (db[player].data->player.topic_id == givensubtopic->topic_id) && (db[player].data->player.subtopic_id == given->topic_id)) || (!givensubtopic && (db[player].data->player.topic_id == given->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
              return;
	   }
	}
     }

     if(!Blank(arg2)) {
        int value = atol(arg2);

        if(value > BBS_MAX_LATEST) {
           output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only see a maximum of "ANSI_LWHITE"%d"ANSI_LGREEN" latest messages added to %s BBS.",BBS_MAX_LATEST,tcz_full_name);
           return;
	} else if(value < 0) {
           output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can't see a negative number of latest messages added to %s BBS.",tcz_full_name);
           return;
	} else count = value;
     }
     if(count <= 0) count = db[player].data->player.scrheight / 2;

     /* ---->  Rank BBS messages  <---- */
     for(loop = 0; loop < BBS_MAX_LATEST; loop++) {
         ranking[loop].subtopic = NULL;
         ranking[loop].message  = NULL;
         ranking[loop].number   = 0;
         ranking[loop].topic    = NULL;
     }
     for(topic = bbs; topic; topic = topic->next)
         if((!given || (topic == given)) && can_access_topic(player,topic,NULL,0)) {

            /* ---->  Rank messages within topic  <---- */
            for(message = topic->messages, mno = 1; message; message = message->next, mno++)
                if((owner == NOTHING) || ((message->owner == owner) && !(message->flags & MESSAGE_ANON))) {
                   for(loop = 0; (loop < BBS_MAX_LATEST) && ranking[loop].message && (message->date < ranking[loop].message->date); loop++);
                   if(loop < (BBS_MAX_LATEST - 1)) {
                      for(loop2 = (BBS_MAX_LATEST - 1); (loop2 > loop) && (loop2 > 0); loop2--) {
                          ranking[loop2].subtopic = ranking[loop2 - 1].subtopic;
                          ranking[loop2].message  = ranking[loop2 - 1].message;
                          ranking[loop2].number   = ranking[loop2 - 1].number;
                          ranking[loop2].topic    = ranking[loop2 - 1].topic;
		      }
                      ranking[loop].subtopic = NULL;
                      ranking[loop].message  = message;
                      ranking[loop].number   = mno;
                      ranking[loop].topic    = topic;
		   }
		}

            /* ---->  Messages within sub-topics of topic  <---- */
            for(subtopic = topic->subtopics; subtopic; subtopic = subtopic->next)
                if((!given || (!givensubtopic && (topic == given)) || (subtopic == givensubtopic)) && can_access_topic(player,subtopic,topic,0))
                   for(message = subtopic->messages, mno = 1; message; message = message->next, mno++)
                       if((owner == NOTHING) || ((message->owner == owner) && !(message->flags & MESSAGE_ANON))) {
                          for(loop = 0; (loop < BBS_MAX_LATEST) && ranking[loop].message && (message->date < ranking[loop].message->date); loop++);
                              if(loop < (BBS_MAX_LATEST - 1)) {
                                 for(loop2 = (BBS_MAX_LATEST - 1); (loop2 > loop) && (loop2 > 0); loop2--) {
                                     ranking[loop2].subtopic = ranking[loop2 - 1].subtopic;
                                     ranking[loop2].message  = ranking[loop2 - 1].message;
                                     ranking[loop2].number   = ranking[loop2 - 1].number;
                                     ranking[loop2].topic    = ranking[loop2 - 1].topic;
				 }
                                 ranking[loop].subtopic = topic;
                                 ranking[loop].message  = message;
                                 ranking[loop].number   = mno;
                                 ranking[loop].topic    = subtopic;
			      }
		       }
	 }

     /* ---->  Display results  <---- */
     html_anti_reverse(p,1);
     if(!in_command && p && !p->pager && !IsHtml(p) && More(player)) pager_init(p);
     if(IsHtml(p)) output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
     if(!in_command) {
        if(owner != NOTHING) {
           if(owner != player) output(p,player,2,1,1,"%sLatest messages added to "ANSI_LYELLOW"%s BBS"ANSI_LGREEN" by %s"ANSI_LWHITE"%s"ANSI_LGREEN"...%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH COLSPAN=4>"ANSI_LGREEN"<I>\016":ANSI_LGREEN"\n ",tcz_full_name,Article(owner,LOWER,DEFINITE),getcname(NOTHING,owner,0,0),IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
	      else output(p,player,2,1,1,"%sLatest messages added to "ANSI_LYELLOW"%s BBS"ANSI_LGREEN" by yourself...%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH COLSPAN=4>"ANSI_LGREEN"<I>\016":ANSI_LGREEN"\n ",tcz_full_name,IsHtml(p) ? "\016</I></TH></TR>\016":"\n");
	}

        if(!IsHtml(p)) {
           output(p,player,0,1,0,"\n Rank:   Topic/sub-topic:            Message number and title:");
           output(p,player,0,1,0,separator(twidth,0,'-','='));
	} else output(p,player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH WIDTH=10%%><FONT COLOR="HTML_LCYAN" SIZE=4><I>Rank:</I></FONT></TH><TH WIDTH=30%%><FONT COLOR="HTML_LCYAN" SIZE=4><I>Topic/sub-topic:</I></FONT></TH><TH COLSPAN=2><FONT COLOR="HTML_LCYAN" SIZE=4><I>Message number and title:</I></FONT></TH></TR>\016");
     }

     if(ranking[0].message) {
        unsigned char ignored;

        for(loop = 0; (loop < BBS_MAX_LATEST) && ranking[loop].message && ranking[loop].topic && (loop < count); loop++) {
            sprintf(scratch_return_string,"(%s)",rank(loop + 1));
            sprintf(scratch_return_string + 100,"(%d)",ranking[loop].number);
            sprintf(scratch_return_string + 200,"%s%s%s",(ranking[loop].subtopic) ? ranking[loop].subtopic->name:"",(ranking[loop].subtopic) ? "/":"",ranking[loop].topic->name);
            if(IsHtml(p)) sprintf(scratch_buffer,"\016<TR ALIGN=CENTER><TD WIDTH=10%% BGCOLOR="HTML_TABLE_RED">\016"ANSI_LRED"%s\016</TD><TD ALIGN=CENTER WIDTH=30%% BGCOLOR="HTML_TABLE_BLUE"><A HREF=\"%sSUBST=OK&COMMAND=%%7Cbbs+topic+%s&\" TARGET=TCZINPUT>\016%s\016</A></TD><TD WIDTH=10%% BGCOLOR="HTML_TABLE_GREEN">\016"ANSI_LGREEN"%s\016</TD><TD ALIGN=LEFT WIDTH=50%%>\016",scratch_return_string,html_server_url(p,1,2,"input"),html_encode(scratch_return_string + 200,scratch_return_string + 500,&copied,128),scratch_return_string + 200,scratch_return_string + 100);
               else sprintf(scratch_buffer,ANSI_LRED" %-8s"ANSI_LWHITE"%-28s"ANSI_LGREEN"%-7s",scratch_return_string,scratch_return_string + 200,scratch_return_string + 100);
            substitute(Validchar(ranking[loop].message->owner) ? ranking[loop].message->owner:player,scratch_return_string,decompress(ranking[loop].message->subject),0,ANSI_LYELLOW,NULL,0);
            if(ranking[loop].topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
            if(!IsHtml(p)) truncatestr(scratch_return_string,scratch_return_string,0,(ranking[loop].message->flags & MESSAGE_REPLY) ? 28:32);
            sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LWHITE"'%s"ANSI_LYELLOW"%s"ANSI_LWHITE"'",(ranking[loop].message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string);
            sprintf(scratch_return_string,"%s%s",bbs_unread_message(ranking[loop].message,player,&ignored) ? ANSI_LMAGENTA" (*UNREAD*)":(ignored) ? ANSI_LMAGENTA" (Ignored)":"",(ranking[loop].message->flags & MESSAGE_APPEND) ? ANSI_LBLUE" (Appended)":"");
            if(!Blank(scratch_return_string)) sprintf(scratch_buffer + strlen(scratch_buffer)," \016&nbsp;\016%s",scratch_return_string);
            output(p,player,2,1,44,"%s%s",scratch_buffer,IsHtml(p) ? "\016</TD></TR>\016":"\n");
	}
     } else if(IsHtml(p)) output(p,player,2,1,0,(owner == NOTHING) ? "\016<TR ALIGN=CENTER><TD COLSPAN=4>"ANSI_LCYAN"<I>*** &nbsp; SORRY, THERE ARE NO MESSAGES ON THE BBS AT THE MOMENT &nbsp; ***</I></TD></TR>\016":"\016<TR ALIGN=CENTER><TD COLSPAN=4>"ANSI_LCYAN"<I>*** &nbsp; NO MESSAGES FOUND &nbsp; ***</I></TD></TR>\016");
        else output(p,player,0,1,0,(owner == NOTHING) ? " ***  SORRY, THERE ARE NO MESSAGES ON THE BBS AT THE MOMENT  ***":" ***  NO MESSAGES FOUND  ***");

     /* ---->  BBS navigation buttons  <---- */
     if(!in_command) {
        if(IsHtml(p)) {
           strcpy(scratch_buffer,"<TR BGCOLOR="HTML_TABLE_GREY"><TD ALIGN=CENTER COLSPAN=4>");
           sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Cbbs+read+unread&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[NEXT UNREAD]\" BORDER=0></A> ",html_server_url(p,1,2,"input"),html_image_url("nextunread.gif"));
           sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Cbbs+topics&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[TOPICS]\" BORDER=0></A> ",html_server_url(p,1,2,"input"),html_image_url("topics.gif"));
           sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Cbbs+summary&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[SUMMARY]\" BORDER=0></A>",html_server_url(p,1,2,"input"),html_image_url("summary.gif"));
           output(p,player,1,2,0,"%s</TD></TR>",scratch_buffer);
	} else output(p,player,0,1,0,separator(twidth,1,'-','='));
     }

     if(IsHtml(p)) output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
     html_anti_reverse(p,0);
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Return name of latest message in given topic  <---- */
void bbs_query_latest(CONTEXT)
{
     struct   bbs_topic_data *topic,*subtopic;
     struct   bbs_message_data *message;
     unsigned char ignored;
     short    temp;

     setreturn(ERROR,COMMAND_FAIL);
     bbs_topicname_filter(params);
     if((topic = lookup_topic(player,params,&topic,&subtopic))) {
        if(!subtopic || can_access_topic(player,subtopic,NULL,1)) {
           if(can_access_topic(player,topic,subtopic,1)) {
     	      if((message = lookup_message(player,&topic,&subtopic,"LAST",&message,&temp,0))) {
                 substitute(Validchar(message->owner) ? message->owner:player,querybuf,decompress(message->subject),0,ANSI_LYELLOW,NULL,0);
                 if(topic->flags & TOPIC_CENSOR) bad_language_filter(querybuf,querybuf);
                 sprintf(cmpbuf,"%s"ANSI_LYELLOW"%s",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",querybuf);
                 sprintf(querybuf,"%s%s",bbs_unread_message(message,player,&ignored) ? ANSI_LMAGENTA" (*UNREAD*)":(ignored) ? ANSI_LMAGENTA" (Ignored)":"",(message->flags & MESSAGE_APPEND) ? ANSI_LBLUE" (Appended)":"");
                 if(!Blank(querybuf)) sprintf(cmpbuf + strlen(cmpbuf),ANSI_IBLACK" %s",querybuf);
                 setreturn(cmpbuf,COMMAND_SUCC);
	      } else setreturn(ANSI_LCYAN"***  NO MESSAGES  ***",COMMAND_SUCC);
	   } else {
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.",(subtopic) ? "sub-":"",topic->name,clevels[(int) topic->accesslevel]);
              if((subtopic && (db[player].data->player.topic_id == subtopic->topic_id) && (db[player].data->player.subtopic_id == topic->topic_id)) || (!subtopic && (db[player].data->player.topic_id == topic->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
	   }
	} else {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)",subtopic->name,clevels[(int) subtopic->accesslevel],topic->name);
           if(db[player].data->player.topic_id == topic->topic_id) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
	}
     }
}

/* ---->  Return number of messages in given topic             <---- */
/*        (VAL1:  0 = New messages only, 1 = Total messages.)        */
void bbs_query_newmessages(CONTEXT)
{
     struct   bbs_topic_data *topic,*subtopic;
     struct   bbs_message_data *message;
     unsigned char ignored;
     short    count = 0;

     setreturn(ERROR,COMMAND_FAIL);
     bbs_topicname_filter(params);
     if((topic = lookup_topic(player,params,&topic,&subtopic))) {
        if(!subtopic || can_access_topic(player,subtopic,NULL,1)) {
           if(can_access_topic(player,topic,subtopic,1)) {
              for(message = topic->messages; message; message = message->next)
		 if(val1 || bbs_unread_message(message,player,&ignored)) count++;
              sprintf(querybuf,"%d",count);
              setreturn(querybuf,COMMAND_SUCC);
	   } else {
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.",(subtopic) ? "sub-":"",topic->name,clevels[(int) topic->accesslevel]);
              if((subtopic && (db[player].data->player.topic_id == subtopic->topic_id) && (db[player].data->player.subtopic_id == topic->topic_id)) || (!subtopic && (db[player].data->player.topic_id == topic->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
	   }
	} else {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)",subtopic->name,clevels[(int) subtopic->accesslevel],topic->name);
           if(db[player].data->player.topic_id == topic->topic_id) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
	}
     }
}

/* ---->  Set/query current BBS reader time difference (Only used in BBS message reading code)  <---- */
/*        (Val1:  0 = Query, 1 = Set.)  */
/*        (Val2:  DBref of reader.)     */
void bbs_query_readertimediff(CONTEXT)
{
     static int timediff = 0;

     if(val1) {
        timediff = Validchar(val2) ? (db[val2].data->player.timediff * 3600):0;
        setreturn(OK,COMMAND_SUCC);
     } else {
        sprintf(querybuf,"%d",timediff);
        setreturn(querybuf,COMMAND_SUCC);
     }
}

/* ---->  Take character to BBS room, change topic to NEWS and display latest page of messages  <---- */
void bbs_news(CONTEXT)
{
     dbref cmd_cache = current_command;
     int   ic_cache = in_command;

     if(db[player].location != bbsroom) {
        in_command = 1, current_command = 0;
        bbs_topic(player,"News",NULL,NULL,NULL,0,0);
        in_command = ic_cache, current_command = cmd_cache;
        bbs_bbs(player,NULL,NULL,NULL,NULL,0,0);
     } else {
        in_command = 1, current_command = 0;
        bbs_topic(player,"News",NULL,NULL,NULL,0,0);
        in_command = ic_cache, current_command = cmd_cache;
        bbs_view(player,NULL,NULL,NULL,NULL,0,0);
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"For full details on how to use "ANSI_LYELLOW"%s BBS"ANSI_LGREEN", please type '"ANSI_LWHITE"help bbs"ANSI_LGREEN"'.\n",tcz_full_name);
     }
}

/* ---->  Reply to message (Start editor if MESSAGE parameter is ommited)  <---- */
/*        (val1:  0 = Normal, 1 = Anonymous.)                                    */
void bbs_reply(CONTEXT)
{
     struct bbs_topic_data *topic,*subtopic;
     struct bbs_message_data *message;
     static dbref postwho = NOTHING;
     static time_t posttime = 0;
     short  temp;
     char   *ptr;
     time_t now;

     /* ---->  Grab first word as message number  <---- */
     gettime(now);
     comms_spoken(player,0);
     setreturn(ERROR,COMMAND_FAIL);
     if(Blank(arg2)) {
        for(; *arg1 && (*arg1 == ' '); arg2++);
        for(ptr = arg1; *ptr && (*ptr != ' '); ptr++);
        if(*ptr) for(*ptr = '\0', ptr++; *ptr && (*ptr == ' '); ptr++);
        arg2 = ptr;
     }

     if(!Moron(player)) {
        if(Bbs(player)) {
           if(!(!Level4(player) && (postwho == player) && (now < posttime))) {
              if((topic = lookup_topic(player,NULL,&topic,&subtopic))) {
                 if(!subtopic || can_access_topic(player,subtopic,NULL,1)) {
  	            if(can_access_topic(player,topic,subtopic,1)) {
       	               if(!(!Level4(db[player].owner) && !(topic->flags & TOPIC_MORTALADD))) {
                          if(topic->flags & TOPIC_ADD) {
              	             if(!(val1 && !(topic->flags & TOPIC_ANON))) {
                 	        if(!(!(topic->flags & TOPIC_CYCLIC) && (bbs_messagecount(topic->messages,NOTHING,&temp) >= topic->messagelimit))) {
	                           if((message = lookup_message(player,&topic,&subtopic,arg1,&message,&temp,0))) {
                                      posttime = now + POST_TIME, postwho = player;
                                      if(!Blank(arg2)) {	      
                                         if(topic->flags & TOPIC_CYCLIC) bbs_cyclicdelete(topic->topic_id,(subtopic) ? subtopic->topic_id:0);
                                         strcpy(scratch_buffer,decompress(message->subject));
                                         temp = bbs_addmessage(player,topic->topic_id,(subtopic) ? subtopic->topic_id:0,scratch_buffer,arg2,now,(val1) ? MESSAGE_REPLY|MESSAGE_ANON:MESSAGE_REPLY,message->id);
                                         if(!in_command) {
                                            substitute(Validchar(message->owner) ? message->owner:player,scratch_return_string,scratch_buffer,0,ANSI_LYELLOW,NULL,0);
                                            if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                                            if(subtopic) {
                                               output(getdsc(player),player,0,1,0,ANSI_LGREEN"%seply to the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' left in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)",(val1) ? "Anonymous r":"R",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,topic->name,subtopic->name,temp);
                                               if(val1) bbs_output_except(player,MIN(topic->accesslevel,((subtopic) ? subtopic->accesslevel:255)),0,1,2,ANSI_LGREEN"An anonymous user leaves a reply to the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,topic->name,subtopic->name,temp);
                                                  else bbs_output_except(player,MIN(topic->accesslevel,((subtopic) ? subtopic->accesslevel:255)),0,1,2,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" leaves a reply to the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0),(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,topic->name,subtopic->name,temp);
					    } else {
                                               output(getdsc(player),player,0,1,0,ANSI_LGREEN"%seply to the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' left in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)",(val1) ? "Anonymous r":"R",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,topic->name,temp);
                                               if(val1) bbs_output_except(player,MIN(topic->accesslevel,((subtopic) ? subtopic->accesslevel:255)),0,1,2,ANSI_LGREEN"An anonymous user leaves a reply to the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,topic->name,temp);
                                                  else bbs_output_except(player,MIN(topic->accesslevel,((subtopic) ? subtopic->accesslevel:255)),0,1,2,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" leaves a reply to the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0),(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,topic->name,temp);
					    }
					 }
                                         setreturn(OK,COMMAND_SUCC);
				      } else if(!(message->flags & MESSAGE_MODIFY)) {
                                         if(editing(player)) return;
                                         message->flags |= MESSAGE_MODIFY;
                                         edit_initialise(player,(val1) ? 107:101,NULL,(union group_data *) message,NULL,NULL,(topic->flags & TOPIC_CENSOR)|EDIT_LAST_CENSOR,MIN(topic->accesslevel,((subtopic) ? subtopic->accesslevel:255)));
                                         substitute(Validchar(message->owner) ? message->owner:player,scratch_return_string,decompress(message->subject),0,ANSI_LYELLOW,NULL,0);
                                         if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                                         if(subtopic) output(getdsc(player),player,0,1,0,ANSI_LCYAN"\nLeaving %sreply to the message '%s"ANSI_LYELLOW"%s"ANSI_LCYAN"' in the sub-topic '"ANSI_LWHITE"%s"ANSI_LCYAN"' in the topic '"ANSI_LWHITE"%s"ANSI_LCYAN"'...\n",(val1) ? "an anonymous ":"",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,topic->name,subtopic->name);
                                            else output(getdsc(player),player,0,1,0,ANSI_LCYAN"\nLeaving %sreply to the message '%s"ANSI_LYELLOW"%s"ANSI_LCYAN"' in the topic '"ANSI_LWHITE"%s"ANSI_LCYAN"'...\n",(val1) ? "an anonymous ":"",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,topic->name);
                                         output(getdsc(player),player,0,1,0,ANSI_LWHITE"Please enter your reply to this message (Pressing "ANSI_LCYAN"RETURN"ANSI_LWHITE" or "ANSI_LCYAN"ENTER"ANSI_LWHITE" after each line.)  Once you're finished, type '"ANSI_LGREEN".view"ANSI_LWHITE"' to view and check your reply.  If you're happy with it, type '"ANSI_LGREEN".save"ANSI_LWHITE"' to save it and add it to %s BBS, otherwise type '"ANSI_LGREEN".abort = yes"ANSI_LWHITE"'.\n",tcz_full_name);
                                         setreturn(OK,COMMAND_SUCC);
				      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that message is currently being edited, appended or replied to  -  You can't reply to it at the moment.");
				   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a message with that number doesn't exist.");
				} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, there are too many messages in the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (A maximum of "ANSI_LYELLOW"%d"ANSI_LGREEN" message%s allowed.)",(subtopic) ? "sub-":"",topic->name,topic->messagelimit,(topic->messagelimit == 1) ? " is":"s are");
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, anonymous replies to messages may not be left in the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",(subtopic) ? "sub-":"",topic->name);
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, replies to messages can't be left in the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",(subtopic) ? "sub-":"",topic->name);
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Mortals may not reply to messages in the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",(subtopic) ? "sub-":"",topic->name);
		    } else {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.  Please choose another %stopic (Type '"ANSI_LWHITE"%s%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%s%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(subtopic) ? "sub-":"",topic->name,clevels[(int) topic->accesslevel],(subtopic) ? "sub-":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"");
                       if((subtopic && (db[player].data->player.topic_id == subtopic->topic_id) && (db[player].data->player.subtopic_id == topic->topic_id)) || (!subtopic && (db[player].data->player.topic_id == topic->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		    }
		 } else {
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)  Please choose another topic (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",subtopic->name,clevels[(int) subtopic->accesslevel],topic->name,(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
                    if(db[player].data->player.topic_id == topic->topic_id) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		 }
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please choose a topic first by typing '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"' (Type '"ANSI_LYELLOW"%stopics"ANSI_LGREEN"' to see the list of available topics.)",(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please wait "ANSI_LWHITE"%s"ANSI_LGREEN" before replying to another message on the BBS.",interval(posttime - now,posttime - now,ENTITIES,0));
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from leaving replies to messages on %s BBS.",tcz_full_name);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Morons aren't allowed to reply to messages on %s BBS.",tcz_full_name);
}

/* ---->  Display settings and owner of specified topic  <---- */
void bbs_settings(CONTEXT)
{
     unsigned char twidth = output_terminal_width(player);
     struct   descriptor_data *p = getdsc(player);
     struct   bbs_topic_data *topic,*subtopic;
     unsigned char accesslevel,inherited = 0;
     short    count;

     setreturn(ERROR,COMMAND_FAIL);
     if((topic = lookup_topic(player,params,&topic,&subtopic))) {
        html_anti_reverse(p,1);
        if(!in_command && p && !p->pager && !IsHtml(p) && More(player)) pager_init(p);
        if(IsHtml(p)) output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
           else output(p,player,0,1,0,"");

        if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_buffer,decompress(topic->desc));
           else strcpy(scratch_buffer,decompress(topic->desc));
        substitute(Validchar(topic->owner) ? topic->owner:player,scratch_return_string,scratch_buffer,0,ANSI_LWHITE,NULL,0);
        if(IsHtml(p)) output(p,player,2,1,0,"\016<TR><TH ALIGN=CENTER WIDTH=20%% BGCOLOR="HTML_TABLE_YELLOW"><FONT SIZE=4><I>\016"ANSI_LYELLOW""ANSI_UNDERLINE"%s%s%s"ANSI_DCYAN"\016</I></FONT></TH><TH ALIGN=LEFT BGCOLOR="HTML_TABLE_BLUE"><FONT SIZE=4><I>\016"ANSI_LWHITE"%s\016</I></FONT></TH></TR>\016",(subtopic) ? subtopic->name:"",(subtopic) ? "/":"",topic->name,scratch_return_string);
           else output(p,player,0,1,strlen(topic->name) + ((subtopic) ? (strlen(subtopic->name) + 1):0) + 4,ANSI_LYELLOW" "ANSI_UNDERLINE"%s%s%s"ANSI_DCYAN":  "ANSI_LWHITE"%s",(subtopic) ? subtopic->name:"",(subtopic) ? "/":"",topic->name,scratch_return_string);

        /* ---->  Topic's owner  <---- */
        if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
        if(subtopic) output(p,player,2,1,1,"%s"ANSI_LCYAN"This sub-topic in the topic '"ANSI_LYELLOW"%s"ANSI_LCYAN"' is owned by %s"ANSI_LWHITE"%s"ANSI_LCYAN".%s",IsHtml(p) ? "\016<TR ALIGN=LEFT><TD COLSPAN=2 BGCOLOR="HTML_TABLE_CYAN">\016":" ",subtopic->name,Article(topic->owner,LOWER,INDEFINITE),getcname(NOTHING,topic->owner,0,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
           else output(p,player,2,1,1,"%s"ANSI_LCYAN"This topic is owned by %s"ANSI_LWHITE"%s"ANSI_LCYAN".%s",IsHtml(p) ? "\016<TR ALIGN=LEFT><TD COLSPAN=2 BGCOLOR="HTML_TABLE_CYAN">\016":" ",Article(topic->owner,LOWER,INDEFINITE),getcname(NOTHING,topic->owner,0,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
        if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','-'));

        /* ---->  Topic's access level  <---- */
        if(IsHtml(p)) output(p,player,1,2,0,"<TR><TD ALIGN=LEFT COLSPAN=2 BGCOLOR="HTML_TABLE_BLACK">");
        if(subtopic && (subtopic->accesslevel < topic->accesslevel))
           accesslevel = subtopic->accesslevel, inherited = 1;
	      else accesslevel = topic->accesslevel;
        if(accesslevel < 8) {
           if(inherited) output(p,player,2,1,1,"%s"ANSI_LGREEN"This sub-topic may only be accessed by %s (This access restriction is inherited from the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.)%s",IsHtml(p) ? "":" ",clevels[accesslevel],subtopic->name,IsHtml(p) ? "\016<P>\016":"\n\n");
              else output(p,player,2,1,1,"%s"ANSI_LGREEN"This %stopic may only be accessed by %s.%s",IsHtml(p) ? "":" ",(subtopic) ? "sub-":"",clevels[accesslevel],IsHtml(p) ? "\016<P>\016":"\n\n");
	} else output(p,player,2,1,1,"%s"ANSI_LGREEN"Access to this %stopic is not restricted.%s",IsHtml(p) ? "":" ",(subtopic) ? "sub-":"",IsHtml(p) ? "\016<P>\016":"\n\n");

        /* ---->  Topic's time limit  <---- */
        if(topic->timelimit) output(p,player,2,1,1,"%s"ANSI_LGREEN"Messages which haven't been read for "ANSI_LYELLOW"%d"ANSI_LGREEN" day%s will be deleted automatically.%s",IsHtml(p) ? "":" ",topic->timelimit,Plural(topic->timelimit),IsHtml(p) ? "\016<P>\016":"\n\n");
	   else output(p,player,2,1,1,"%s"ANSI_LGREEN"Messages are not subject to a time limit and will not be deleted automatically.%s",IsHtml(p) ? "":" ",IsHtml(p) ? "\016<P>\016":"\n\n");

        /* ---->  Topic's message limit  <---- */
        count = bbs_messagecount(topic->messages,player,&count);
        output(p,player,2,1,1,"%s"ANSI_LGREEN"A maximum of "ANSI_LWHITE"%d"ANSI_LGREEN" message%s %s allowed in this %stopic at any one time (There %s "ANSI_LWHITE"%d"ANSI_LGREEN" message%s at present)  -  %s%s",IsHtml(p) ? "":" ",topic->messagelimit,Plural(topic->messagelimit),(topic->messagelimit == 1) ? "is":"are",(subtopic) ? "sub-":"",(count == 1) ? "is":"are",count,Plural(count),(topic->flags & TOPIC_CYCLIC) ? "Old messages will automatically be deleted when this topic is full and new messages are added (Cyclic deletion.)":"When this topic is full, no new messages may be added.",IsHtml(p) ? "\016<P>\016":"\n");

        /* ---->  Topic's sub-topic limit  <---- */
        if(!subtopic) {
           for(subtopic = topic->subtopics, count = 0; subtopic; subtopic = subtopic->next, count++);
           output(p,player,2,1,1,"%s"ANSI_LGREEN"A maximum of "ANSI_LWHITE"%d"ANSI_LGREEN" sub-topic%s %s allowed in this topic (There %s "ANSI_LWHITE"%d"ANSI_LGREEN" sub-topic%s at present.)%s",IsHtml(p) ? "\016<P>\016":"\n ",topic->subtopiclimit,Plural(topic->subtopiclimit),(topic->subtopiclimit == 1) ? "is":"are",(count == 1) ? "is":"are",count,Plural(count),IsHtml(p) ? "":"\n");
           subtopic = NULL;
	}
        if(IsHtml(p)) output(p,player,1,2,0,"</TD></TR><TR><TD ALIGN=LEFT COLSPAN=2 BGCOLOR="HTML_TABLE_BLACK">");
           else output(p,player,0,1,0,separator(twidth,0,'-','-'));

        /* ---->  Topic's parameters  <---- */
        output(p,player,2,1,1,"%s"ANSI_LGREEN"Messages may%s be added.%s",IsHtml(p) ? "":" ",(topic->flags & TOPIC_ADD) ? "":" not",IsHtml(p) ? "\016<BR>\016":"\n");
        output(p,player,2,1,1,"%s"ANSI_LGREEN"Mortals may%s add messages.%s",IsHtml(p) ? "":" ",(topic->flags & TOPIC_MORTALADD) ? "":" not",IsHtml(p) ? "\016<BR>\016":"\n");
        output(p,player,2,1,1,"%s"ANSI_LGREEN"Messages may%s be made anonymous.%s",IsHtml(p) ? "":" ",(topic->flags & TOPIC_ANON) ? "":" not",IsHtml(p) ? "\016<BR>\016":"\n");
        output(p,player,2,1,1,"%s"ANSI_LGREEN"Bad language in messages will%s be censored.%s",IsHtml(p) ? "":" ",(topic->flags & TOPIC_CENSOR) ? "":" not",IsHtml(p) ? "\016<BR>\016":"\n");
        output(p,player,2,1,1,"%s"ANSI_LGREEN"Messages will%s be automatically formatted.%s",IsHtml(p) ? "":" ",(topic->flags & TOPIC_FORMAT) ? "":" not",IsHtml(p) ? "\016<BR>\016":"\n");
        output(p,player,2,1,1,"%s"ANSI_LGREEN"This topic will%s be hilighted in the list of %stopics.%s",IsHtml(p) ? "":" ",(topic->flags & TOPIC_HILIGHT) ? "":" not",(subtopic) ? "sub-":"",IsHtml(p) ? "\016<BR>\016":"\n");
        if(!IsHtml(p)) output(p,player,0,1,1,separator(twidth,1,'-','='));
           else output(p,player,1,2,0,"</TD></TR></TABLE>%s",(!in_command) ? "<BR>":"");
        html_anti_reverse(p,0);
        setreturn(OK,COMMAND_SUCC);
     } else if(!Blank(params)) output(p,player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",params);
        else output(p,player,0,1,0,ANSI_LGREEN"Please choose a topic first by typing '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"' (Type '"ANSI_LYELLOW"%stopics"ANSI_LGREEN"' to see the list of available topics.)",(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
}

/* ---->  Summary of topics with new messages on the BBS  <---- */
void bbs_summary(CONTEXT)
{
     int      newcount = 0,topicnew,latestmsg,msgcount,copied;
     struct   bbs_list_data *head = NULL,*tail = NULL,*new;
     unsigned char twidth = output_terminal_width(player);
     struct   bbs_message_data *message,*last,*latest;
     struct   descriptor_data *p = getdsc(player);
     struct   bbs_topic_data *topic,*subtopic;
     unsigned char finished,ignored;

     setreturn(OK,COMMAND_SUCC);
     html_anti_reverse(p,1);
     if((!strcasecmp("page",params) && (strlen(params) == 4)) || !strncasecmp(params,"page ",5))
        for(params += 4; *params && (*params == ' '); params++);
     params = (char *) parse_grouprange(player,params,FIRST,1);
     if(IsHtml(p)) output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");

     if(!in_command) {
        if(!IsHtml(p)) {
           output(p,player,0,1,0,ANSI_LGREEN"\nSummary of unread messages in topics/sub-topics on "ANSI_LYELLOW"%s BBS"ANSI_LGREEN"...\n\n"ANSI_LCYAN" Topic/sub-topic:         Unread:  Number and title of first unread message:",tcz_full_name);
           output(p,player,0,1,0,separator(twidth,0,'-','='));
	} else output(p,player,2,1,1,"\016<TR ALIGN=CENTER><TH COLSPAN=4 BGCOLOR="HTML_TABLE_GREEN">"ANSI_LGREEN"<I>Summary of unread messages in topics/sub-topics on "ANSI_LYELLOW"%s BBS"ANSI_LGREEN"...</I></TH></TR><TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH WIDTH=30%%><FONT COLOR="HTML_LCYAN" SIZE=4><I>Topic/sub-topic:</I></FONT></TH><TH WIDTH=10%%><FONT COLOR="HTML_LCYAN"><I>Unread:</I></FONT></TH><TH COLSPAN=2><FONT COLOR="HTML_LCYAN"><I>Number and title of first unread message:</I></FONT></TH></TR>\016",tcz_full_name);
     }

     /* ---->  Place topics and sub-topics with unread messages (In order) into linked list  <---- */
     for(topic = bbs; topic; topic = topic->next)
         if(can_access_topic(player,topic,NULL,0)) {
            for(message = topic->messages, finished = 0; message && !finished; message = message->next)
                if(bbs_unread_message(message,player,&ignored)) {
                   MALLOC(new,struct bbs_list_data);
                   new->subtopic = NULL, new->topic = topic, new->next = NULL;
                   if(head) tail->next = new, tail = new;
	              else head = tail = new;
                   finished = 1;
		}

            for(subtopic = topic->subtopics; subtopic; subtopic = subtopic->next)
                if(can_access_topic(player,subtopic,topic,0))
                   for(message = subtopic->messages, finished = 0; message && !finished; message = message->next)
                       if(bbs_unread_message(message,player,&ignored)) {
                          MALLOC(new,struct bbs_list_data);
                          new->subtopic = topic, new->topic = subtopic, new->next = NULL;
                          if(head) tail->next = new, tail = new;
	                     else head = tail = new;
                          finished = 1;
		       }
	 }

     /* ---->  Display summary  <---- */
     set_conditions(player,0,0,0,0,NULL,500);
     union_initgrouprange((union group_data *) head);
     while(union_grouprange()) {
           for(message = grp->cunion->bbslist.topic->messages, latest = NULL, last = NULL, topicnew = 0, msgcount = 0; message; last = message, message = message->next, msgcount++)
               if(bbs_unread_message(message,player,&ignored)) {
                  if(!latest) latest = message, latestmsg = (msgcount + 1);
                  newcount++, topicnew++;
	       }
           if(topicnew > 999) topicnew = 999;
           if(!latest) latest = last, latestmsg = msgcount;
           sprintf(scratch_return_string,"(%d)",latestmsg);
           substitute(Validchar(latest->owner) ? latest->owner:player,scratch_return_string + 50,decompress(latest->subject),0,ANSI_LYELLOW,NULL,0);
           truncatestr(scratch_return_string + 50,scratch_return_string + 50,0,(latest->flags & MESSAGE_REPLY) ? 30:34);
           if(grp->cunion->bbslist.topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string + 50,scratch_return_string + 50);
           sprintf(scratch_return_string + 200,"%s%s%s",(grp->cunion->bbslist.subtopic) ? grp->cunion->bbslist.subtopic->name:"",(grp->cunion->bbslist.subtopic) ? "/":"",grp->cunion->bbslist.topic->name);
           if(IsHtml(p)) output(p,player,2,1,0,"\016<TR ALIGN=CENTER><TD ALIGN=CENTER WIDTH=30%% BGCOLOR="HTML_TABLE_BLUE"><A HREF=\"%sSUBST=OK&COMMAND=%%7Cbbs+topic+%s&\" TARGET=TCZINPUT>\016%s\016</A></TD><TD WIDTH=10%% BGCOLOR="HTML_TABLE_RED">"ANSI_LRED"%d</TD><TD WIDTH=10%% BGCOLOR="HTML_TABLE_GREEN">"ANSI_LGREEN"%s</TD><TD ALIGN=LEFT WIDTH=50%%>\016"ANSI_LWHITE"'%s"ANSI_LYELLOW"%s"ANSI_LWHITE"'%s\016</TD></TR>\016",html_server_url(p,1,2,"input"),html_encode(scratch_return_string + 200,scratch_buffer,&copied,128),scratch_return_string + 200,topicnew,scratch_return_string,(latest->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string + 50,(latest->flags & MESSAGE_APPEND) ? ANSI_LBLUE" \016&nbsp;\016 (Appended)":"");
              else output(p,player,0,1,42,ANSI_LWHITE" %-29s"ANSI_LRED"%3d"ANSI_LGREEN"  %-7s"ANSI_LWHITE"'%s"ANSI_LYELLOW"%s"ANSI_LWHITE"'%s",scratch_return_string + 200,topicnew,scratch_return_string,(latest->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string + 50,(latest->flags & MESSAGE_APPEND) ? ANSI_LBLUE"  (Appended)":"");
     }

     /* ---->  Free linked list  <---- */
     for(; head; head = new) {
         new = head->next;
         FREENULL(head);
     }

     if(grp->rangeitems == 0) output(p,player,2,1,1,IsHtml(p) ? "\016<TR ALIGN=CENTER><TD COLSPAN=4>"ANSI_LCYAN"<I>*** &nbsp; SORRY, THERE ARE NO UNREAD MESSAGES ON THE BBS AT THE MOMENT &nbsp; ***</I></TD></TR>\016":ANSI_LCYAN" ***  SORRY, THERE ARE NO UNREAD MESSAGES ON THE BBS AT THE MOMENT  ***\n");
     if(!in_command) {

        /* ---->  BBS navigation buttons  <---- */
        if(IsHtml(p)) {
           strcpy(scratch_buffer,"<TR BGCOLOR="HTML_TABLE_GREY"><TD ALIGN=CENTER COLSPAN=4>");
           sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Cbbs+read+unread&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[NEXT UNREAD]\" BORDER=0></A> ",html_server_url(p,1,2,"input"),html_image_url("nextunread.gif"));
           sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Cbbs+topics&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[TOPICS]\" BORDER=0></A> ",html_server_url(p,1,2,"input"),html_image_url("topics.gif"));
           sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Cbbs+latest&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[LATEST]\" BORDER=0></A>",html_server_url(p,1,2,"input"),html_image_url("latest.gif"));
           output(p,player,1,2,0,"%s</TD></TR>",scratch_buffer);
	}

        /* ---->  Topics listed/total unread messages within topics  <---- */
        if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
        if(grp->rangeitems != 0) {
           listed_items(scratch_return_string,1);
           output(p,player,2,1,1,"%sTopics listed: \016&nbsp;\016 "ANSI_DWHITE"%s\016 &nbsp; &nbsp; &nbsp; \016"ANSI_LWHITE"Total unread messages listed: \016&nbsp;\016 "ANSI_DWHITE"%d.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_MGREY"><TD COLSPAN=4>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",scratch_return_string,newcount,IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
	} else output(p,player,2,1,1,"%sTopics listed: \016&nbsp;\016 "ANSI_DWHITE"None.\016 &nbsp; &nbsp; &nbsp; \016"ANSI_LWHITE"Total unread messages listed: \016&nbsp;\016 "ANSI_DWHITE"0.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_MGREY"><TD COLSPAN=4>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
        if(IsHtml(p)) output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
     }
     html_anti_reverse(p,0);
}

/* ---->  Select topic to browse from list of available topics/sub-topics  <---- */
/*        (val1:  0 = Select topic, 1 = Select sub-topic.)                       */
void bbs_topic(CONTEXT)
{
     struct   bbs_topic_data *topic,*subtopic;
     struct   descriptor_data *d;
     unsigned char same = 0;
     short    count;

     setreturn(ERROR,COMMAND_FAIL);
     bbs_topicname_filter(params);
     if(!Blank(params)) {
        sprintf(scratch_return_string,"%s%s",(val1) ? "/":"",params);
        if((topic = lookup_topic(player,scratch_return_string,&topic,&subtopic))) {
           if((subtopic && (db[player].data->player.topic_id == subtopic->topic_id) && (db[player].data->player.subtopic_id == topic->topic_id)) || (!subtopic && (db[player].data->player.topic_id == topic->topic_id) && !db[player].data->player.subtopic_id)) same = 1;
           if(!subtopic || can_access_topic(player,subtopic,NULL,1)) {
              if(can_access_topic(player,topic,subtopic,1)) {
                 if(!same) {
                    for(d = descriptor_list; d; d = d->next)
                        if((d->player == player) && d->edit) {
                           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't change %stopic while using the editor.",(val1) ? "sub-":"");
                           return;
			}

                    for(d = descriptor_list; d; d = d->next)
                        if(d->player == player)
                           d->currentmsg = 0;

                    if(subtopic) {
                       db[player].data->player.subtopic_id = topic->topic_id;
                       db[player].data->player.topic_id    = subtopic->topic_id;
		    } else {
                       db[player].data->player.subtopic_id = 0;
                       db[player].data->player.topic_id    = topic->topic_id;
		    }
		 }

                 if(!in_command) {
                    if(subtopic) output(getdsc(player),player,0,1,0,ANSI_LGREEN"\nYour current selected sub-topic is now '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",topic->name,subtopic->name);
                       else output(getdsc(player),player,0,1,0,ANSI_LGREEN"\nYour current selected topic is now '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",topic->name);

                    if(!db[player].data->player.subtopic_id && topic->subtopics && !bbs_messagecount(topic->messages,player,&count))
                       bbs_topics(player,NULL,NULL,NULL,NULL,1,0);
                          else bbs_view(player,NULL,NULL,NULL,NULL,0,0);
		 }
                 setreturn(OK,COMMAND_SUCC);
	      } else {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.  Please choose another %stopic (Type '"ANSI_LWHITE"%s%stopics"ANSI_LGREEN"' for a list of available %stopics.)",(subtopic) ? "sub-":"",topic->name,clevels[(int) topic->accesslevel],(subtopic) ? "sub-":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"",(subtopic) ? "sub-":"");
                 if((subtopic && (db[player].data->player.topic_id == subtopic->topic_id) && (db[player].data->player.subtopic_id == topic->topic_id)) || (!subtopic && (db[player].data->player.topic_id == topic->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
	      }
	   } else {
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)  Please choose another topic (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",subtopic->name,clevels[(int) subtopic->accesslevel],topic->name,(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
              if(db[player].data->player.topic_id == topic->topic_id) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
	   }
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist  -  Please type '"ANSI_LYELLOW"%s%stopics"ANSI_LGREEN"' to see the list of available %stopics.",(val1) ? "sub-":"",params,(command_type == BBS_COMMAND) ? "bbs ":"",(val1) ? "sub":"",(val1) ? "sub-":"");
     } else if(val1) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify which sub-topic you'd like to browse (Type '"ANSI_LWHITE"%ssubtopics"ANSI_LGREEN"' to see the list of available sub-topics within your current selected topic), e.g:  '"ANSI_LYELLOW"%ssubtopic general"ANSI_LGREEN"' to select and browse the sub-topic '"ANSI_LWHITE"General"ANSI_LGREEN"' within your current selected topic.",(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
        else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify which topic you'd like to browse (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' to see the list of available topics), e.g:  '"ANSI_LYELLOW"%stopic news"ANSI_LGREEN"' to select and browse the topic '"ANSI_LWHITE"news"ANSI_LGREEN"'.",(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
}

/* ---->  List available topics/sub-topics on BBS  <---- */
/*        (val1:  0 = Top-level topics, 1 = Sub-topics within current topic.)  */
void bbs_topics(CONTEXT)
{
     unsigned short selected = (val1) ? db[player].data->player.subtopic_id:db[player].data->player.topic_id;
     unsigned char twidth = output_terminal_width(player),cached_scrheight,access;
     int      totalunread = 0,messages,unread,counter = 0,columns,copied;
     struct   bbs_topic_data *ptr = NULL,*topic = NULL,*subtopic = NULL;
     struct   descriptor_data *p = getdsc(player);
     char     *selname = NULL;
     short    count;

     setreturn(ERROR,COMMAND_FAIL);
     if(Blank(arg1) && !Blank(arg2)) arg1 = arg2;
     if(!Blank(arg1) && ((!strcasecmp("page",arg1) && (strlen(arg1) == 4)) || !strncasecmp(arg1,"page ",5)))
        for(arg1 += 4; *arg1 && (*arg1 == ' '); arg1++);
     arg1 = (char *) parse_grouprange(player,arg1,FIRST,1);

     if(!val1 || (topic = lookup_topic(player,arg1,&topic,&subtopic))) {
        if(!val1 || can_access_topic(player,(subtopic) ? subtopic:topic,NULL,1)) {
           if(!val1 || (subtopic && subtopic->subtopics) || (!subtopic && topic->subtopics)) {
              messages         = bbs_topiccount((val1) ? (subtopic) ? subtopic->subtopics:topic->subtopics:bbs);
              columns          = IsHtml(p) ? 3:(twidth / 26);
              cached_scrheight = db[player].data->player.scrheight;


              html_anti_reverse(p,1);
              db[player].data->player.scrheight = ((db[player].data->player.scrheight - 9) * columns) * 2;
              if(IsHtml(p)) output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");

              if(!in_command) {
                 if(IsHtml(p)) {
	            if(val1) output(p,player,2,1,1,"\016<TR><TH ALIGN=CENTER COLSPAN=6 BGCOLOR="HTML_TABLE_CYAN"><FONT SIZE=4><I>\016"ANSI_LCYAN"There %s "ANSI_LWHITE"%d"ANSI_LCYAN" sub-topic%s available in the topic '"ANSI_LWHITE"%s"ANSI_LCYAN"' on "ANSI_LYELLOW"%s BBS"ANSI_LCYAN"...\016</I></FONT></TH></TR>\016",(messages == 1) ? "is":"are",messages,Plural(messages),(subtopic) ? subtopic->name:topic->name,tcz_full_name);
                       else output(p,player,2,1,1,"\016<TR><TH ALIGN=CENTER COLSPAN=6 BGCOLOR="HTML_TABLE_CYAN"><FONT SIZE=4><I>\016"ANSI_LCYAN"There %s "ANSI_LWHITE"%d"ANSI_LCYAN" topic%s on "ANSI_LYELLOW"%s BBS"ANSI_LCYAN"...\016</I></FONT></TH></TR>\016",(messages == 1) ? "is":"are",messages,Plural(messages),tcz_full_name);
	         } else {
                    if(val1) output(p,player,0,1,1,"\n There %s "ANSI_LWHITE"%d"ANSI_LCYAN" sub-topic%s available in the topic '"ANSI_LWHITE"%s"ANSI_LCYAN"' on "ANSI_LYELLOW"%s BBS"ANSI_LCYAN"...",(messages == 1) ? "is":"are",messages,Plural(messages),(subtopic) ? subtopic->name:topic->name,tcz_full_name);
                       else output(p,player,0,1,1,"\n There %s "ANSI_LWHITE"%d"ANSI_LCYAN" topic%s on "ANSI_LYELLOW"%s BBS"ANSI_LCYAN"...",(messages == 1) ? "is":"are",messages,Plural(messages),tcz_full_name);
                    output(p,player,0,1,0,separator(twidth,0,'-','='));
		 }
	      }

              /* ---->  List topics/sub-topics  <---- */
              strcpy(scratch_buffer," "ANSI_LWHITE);
              union_initgrouprange((union group_data *) ((val1) ? (subtopic) ? subtopic->subtopics:topic->subtopics:bbs));
              while(union_grouprange()) {
                    counter++, unread = 0;
                    if(counter > columns) {
                       output(p,player,2,1,0,IsHtml(p) ? "\016<TR ALIGN=CENTER>\016%s\016</TR>\016":"%s\n",scratch_buffer);
                       strcpy(scratch_buffer,IsHtml(p) ? "":" "ANSI_LWHITE);
                       counter = 1;
		    }

                    messages = bbs_messagecount(grp->cunion->topic.messages,player,&count);
                    if((access = can_access_topic(player,&(grp->cunion->topic),NULL,0))) unread += count;
                    for(ptr = grp->cunion->topic.subtopics; ptr; ptr = ptr->next) {
                        messages += bbs_messagecount(ptr->messages,player,&count);
                        if(access && can_access_topic(player,ptr,NULL,0)) unread += count;
		    }
                    totalunread += unread;
                    if(messages > 999) messages = 999;
                    if(unread   > 999) unread   = 999;

                    sprintf(scratch_return_string,"%s"ANSI_DCYAN,grp->cunion->topic.name);
                    sprintf(scratch_return_string + 100,"("ANSI_LGREEN"%d"ANSI_DCYAN","ANSI_LWHITE"%d"ANSI_DCYAN")",unread,messages);
                    if(IsHtml(p)) sprintf(scratch_buffer + strlen(scratch_buffer),"\016<TD WIDTH=25%% BGCOLOR="HTML_TABLE_BLUE">%s<A HREF=\"%sSUBST=OK&COMMAND=%%7Cbbs+%stopic+%s&\" TARGET=TCZINPUT>\016%s%s%s\016</A></TD><TD WIDTH=8%% BGCOLOR="HTML_TABLE_MBLUE">\016"ANSI_DCYAN"%s\016</TD>\016",(selected == grp->cunion->topic.topic_id) ? ANSI_WMAGENTA"->"ANSI_LWHITE" ":(!can_access_topic(player,&(grp->cunion->topic),subtopic,0)) ? ANSI_LRED"!":(grp->cunion->topic.flags & TOPIC_HILIGHT) ? ANSI_LYELLOW"*":"",html_server_url(p,1,2,"input"),(val1) ? "sub":"",html_encode(grp->cunion->topic.name,scratch_return_string + 200,&copied,256),(selected == grp->cunion->topic.topic_id) ? "\016<B>\016":(grp->cunion->topic.flags & TOPIC_HILIGHT) ? "\016<B><I>\016":"",scratch_return_string,(selected == grp->cunion->topic.topic_id) ? "\016</B>\016":(grp->cunion->topic.flags & TOPIC_HILIGHT) ? "\016</B></I>\016":"",scratch_return_string + 100);
                       else sprintf(scratch_buffer + strlen(scratch_buffer),"%s%s%s%-20s%46s ",(selected == grp->cunion->topic.topic_id) ? ANSI_WMAGENTA"->":(!can_access_topic(player,&(grp->cunion->topic),subtopic,0)) ? ANSI_LRED" !":(grp->cunion->topic.flags & TOPIC_HILIGHT) ? ANSI_LYELLOW" *":"  ",(selected == grp->cunion->topic.topic_id) ? ANSI_LYELLOW:(grp->cunion->topic.flags & TOPIC_HILIGHT) ? ANSI_LCYAN:ANSI_LWHITE,((selected == grp->cunion->topic.topic_id) && Underline(player)) ? ANSI_UNDERLINE:"",scratch_return_string,scratch_return_string + 100);
                    if(selected == grp->cunion->topic.topic_id) selname = grp->cunion->topic.name;
	      }
              if(counter > 0) {
                 if(IsHtml(p)) while(++counter <= columns) strcat(scratch_buffer,"\016<TD WIDTH=25% BGCOLOR="HTML_TABLE_BLUE">&nbsp;</TD><TD WIDTH=8% BGCOLOR="HTML_TABLE_MBLUE">&nbsp;</TD>\016");
                 output(p,player,2,1,0,IsHtml(p) ? "\016<TR ALIGN=CENTER>\016%s\016</TR>\016":"%s\n",scratch_buffer);
	      }

              if(grp->rangeitems == 0) output(p,player,2,1,1,IsHtml(p) ? "\016<TH ALIGN=CENTER><TD COLSPAN=6>"ANSI_LCYAN"<I>*** &nbsp; SORRY, THERE ARE NO TOPICS AVAILABLE AT THE MOMENT &nbsp; ***</I></TD></TR>\016":ANSI_LCYAN" ***  SORRY, THERE ARE NO TOPICS AVAILABLE AT THE MOMENT  ***\n");
              if(!in_command) {

                 /* ---->  Topic selection instructions  <---- */
                 if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','-'));
                 if(!Blank(selname) && selected) output(p,player,2,1,1,"%sYour current selected %stopic is '"ANSI_LYELLOW"%s"ANSI_LWHITE"'. \016&nbsp;\016 To select a different topic, type '"ANSI_LGREEN"%s%stopic <TOPIC NAME>"ANSI_LWHITE"'.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_MGREY"><TD COLSPAN=6><I>\016"ANSI_LWHITE:ANSI_LWHITE" ",(val1) ? "sub-":"",selname,(command_type == BBS_COMMAND) ? "bbs ":"",(val1) ? "sub":"",IsHtml(p) ? "\016</I></TD></TR>\016":"\n");
                    else output(p,player,2,1,1,"%sPlease select a %stopic from the above list by typing '"ANSI_LGREEN"%s%stopic <TOPIC NAME>"ANSI_LWHITE"'.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_MGREY"><TD COLSPAN=6>"ANSI_LWHITE"<I>\016":ANSI_LWHITE" ",(val1) ? "sub-":"",(command_type == BBS_COMMAND) ? "bbs ":"",(val1) ? "sub":"",IsHtml(p) ? "\016</I></TD></TR>\016":"\n");
                 if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));

                 /* ---->  Topic navigation buttons (HTML)  <---- */
                 if(IsHtml(p)) {
                    strcpy(scratch_buffer,"<TR BGCOLOR="HTML_TABLE_GREY"><TD ALIGN=CENTER COLSPAN=6>");
                    if((grp->nogroups > 0) && (grp->groupno < grp->nogroups))
                       sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Cbbs+%stopics+page+%d&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[NEXT PAGE]\" BORDER=0></A> ",html_server_url(p,1,2,"input"),(val1) ? "sub":"",grp->groupno + 1,html_image_url("nextpage.gif"));
                          else sprintf(scratch_buffer + strlen(scratch_buffer),"<IMG SRC=\"%s\" ALT=\"{NEXT PAGE}\" BORDER=0> ",html_image_url("nonextpage.gif"));
                    if((grp->nogroups > 0) && (grp->groupno > 1))
                       sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Cbbs+%stopics+page+%d&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[PREVIOUS PAGE]\" BORDER=0></A> ",html_server_url(p,1,2,"input"),(val1) ? "sub":"",grp->groupno - 1,html_image_url("prevpage.gif"));
                          else sprintf(scratch_buffer + strlen(scratch_buffer),"<IMG SRC=\"%s\" ALT=\"{PREVIOUS PAGE}\" BORDER=0> ",html_image_url("noprevpage.gif"));
                    sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Cbbs+read+unread&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[NEXT UNREAD]\" BORDER=0></A><BR>",html_server_url(p,1,2,"input"),html_image_url("nextunread.gif"));
                    if(val1) sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Cbbs+topics&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[TOPICS]\" BORDER=0></A> ",html_server_url(p,1,2,"input"),html_image_url("topics.gif"));
                       else if(!val1 && (topic = lookup_topic(player,NULL,&topic,&subtopic)) && topic->subtopics)
                          sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Cbbs+subtopics&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[SUB-TOPICS]\" BORDER=0></A> ",html_server_url(p,1,2,"input"),html_image_url("subtopics.gif"));
                             else sprintf(scratch_buffer + strlen(scratch_buffer),"<IMG SRC=\"%s\" ALT=\"{SUB-TOPICS}\" BORDER=0> ",html_image_url("nosubtopics.gif"));
                    sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Csummary&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[SUMMARY]\" BORDER=0></A> ",html_server_url(p,1,2,"input"),html_image_url("summary.gif"));
                    sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Clatest&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[LATEST]\" BORDER=0></A>",html_server_url(p,1,2,"input"),html_image_url("latest.gif"));
                    output(p,player,1,2,0,"%s</TD></TR>",scratch_buffer);
		 }

                 /* ---->  Total messages listed/unread messages  <---- */
                 if(grp->rangeitems != 0) {
                    listed_items(scratch_return_string + 100,1);
                    if(totalunread) sprintf(scratch_return_string,"%d",totalunread);
                       else strcpy(scratch_return_string,"None");
                    output(p,player,2,1,1,"%s%sopics listed: %s "ANSI_DWHITE"%s%s"ANSI_LWHITE"Total unread messages: %s "ANSI_DWHITE"%s.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_MGREY"><TD COLSPAN=6>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",(val1) ? "Sub-t":"T",IsHtml(p) ? "\016&nbsp;\016":"",scratch_return_string + 100,IsHtml(p) ? "\016 &nbsp; &nbsp; \016":"   ",IsHtml(p) ? "\016&nbsp;\016":"",scratch_return_string,IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
  	         } else output(p,player,2,1,1,"%s%sopics listed: %s "ANSI_DWHITE"None.%s"ANSI_LWHITE"Total unread messages: %s "ANSI_DWHITE"None.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_MGREY"><TD COLSPAN=6>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",(val1) ? "Sub-t":"T",IsHtml(p) ? "\016&nbsp;\016":"",IsHtml(p) ? "\016 &nbsp; &nbsp; \016":"   ",IsHtml(p) ? "\016&nbsp;\016":"",IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
	      }
              if(IsHtml(p)) output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
              db[player].data->player.scrheight = cached_scrheight;
              html_anti_reverse(p,0);
	   } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' has no sub-topics.",topic->name);
	} else {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.  Please choose another topic (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(subtopic) ? subtopic->name:topic->name,clevels[(subtopic) ? subtopic->accesslevel:topic->accesslevel],(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
           if(db[player].data->player.topic_id == ((subtopic) ? subtopic->topic_id:topic->topic_id)) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
	}
     } else if(val1 && !Blank(arg1)) output(p,player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist (Please type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' for a list of available topics.)",arg1,(command_type == BBS_COMMAND) ? "bbs ":"");
        else output(p,player,0,1,0,ANSI_LGREEN"Please select a topic first from the list of available topics (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
}

/* ---->  Read next unread message on the BBS  <---- */
void bbs_unread(CONTEXT)
{
     if(val1 == 2) bbs_view(player,NULL,NULL,"ignored",NULL,0,0);
        else if(val1 && (db[player].location != bbsroom)) bbs_news(player,NULL,NULL,NULL,NULL,0,0);
           else bbs_view(player,NULL,NULL,"unread",NULL,0,0);
}

/* ---->  View/read messages in current selected topic  <---- */
/*        (val1:  0 = Anonymous, 1 = Show anonymous, 2 = Search.)  */
/*        (val2:  0 = All, 1 = Messages only, 2 = Replies to given message only.)  */
void bbs_view(CONTEXT)
{
     static   struct bbs_message_data *bbs_last_message = NULL;
     unsigned char twidth = output_terminal_width(player);
     struct   descriptor_data *p = getdsc(player);
     struct   bbs_message_data *message = NULL;
     static   dbref bbs_last_reader = NOTHING;
     struct   bbs_topic_data *topic,*subtopic;
     unsigned char lookup = 1;
     short    no = 0;

     /* ---->  Search for next/previous occurence of text, and read found message  <---- */
     setreturn(ERROR,COMMAND_FAIL);
     if(val1 == 2) {
        unsigned char searchmode,direction;
        const    char *ptr;
        char          *tmp;

        if(!Blank(arg1)) {
           if(!Blank(arg2) && !string_prefix("all",arg2)) {
              if(string_prefix("subjects",arg2)) searchmode = SEARCH_SUBJECT;
                 else if(string_prefix("messages",arg2) || string_prefix("messagetexts",arg2)) searchmode = SEARCH_MESSAGE;
                    else {
                       output(p,player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LYELLOW"subjects"ANSI_LGREEN"', '"ANSI_LYELLOW"messages"ANSI_LGREEN"' or '"ANSI_LYELLOW"all"ANSI_LGREEN"'.");
                       return;
		    }
	   } else searchmode = SEARCH_SUBJECT|SEARCH_MESSAGE;

           for(ptr = arg1, tmp = scratch_return_string; *ptr && (*ptr == ' '); ptr++);
           for(; *ptr && (*ptr != ' '); *tmp++ = *ptr++);
           for(; *ptr && (*ptr == ' '); ptr++);
           *tmp = '\0';

           if(string_prefix("next",scratch_return_string)) direction = 1, arg1 = (char *) ptr;
              else if(string_prefix("previous",scratch_return_string)) direction = 0, arg1 = (char *) ptr;
                 else direction = 1;

           if(!Blank(arg1)) {
              if(!(message = bbs_search(p,player,arg1,searchmode,direction,&topic,&subtopic,&no))) {
                 output(p,player,0,1,0,ANSI_LGREEN"Sorry, no %s occurences of the text '"ANSI_LWHITE"%s"ANSI_LGREEN"' found in messages on %s BBS.",(direction) ? "further":"previous",arg1,tcz_full_name);
                 return;
	      } else val1 = 0, lookup = 0;
	   } else {
              output(p,player,0,1,0,ANSI_LGREEN"Please specify the text to search for in messages on %s BBS, e.g:  '"ANSI_LYELLOW"%ssearch fred"ANSI_LGREEN"'.",tcz_full_name,(command_type == BBS_COMMAND) ? "bbs ":"");
              return;
	   }
	} else {
           output(p,player,0,1,0,ANSI_LGREEN"Please specify the text to search for in messages on %s BBS, e.g:  '"ANSI_LYELLOW"%ssearch fred"ANSI_LGREEN"'.",tcz_full_name,(command_type == BBS_COMMAND) ? "bbs ":"");
           return;
	}
     }

     /* ---->  View message list/read message  <---- */
     if(Blank(arg1) && !Blank(arg2)) arg1 = arg2;
     if(!lookup || (topic = lookup_topic(player,NULL,&topic,&subtopic))) {
        if(!subtopic || can_access_topic(player,subtopic,NULL,1)) {
           if(can_access_topic(player,topic,subtopic,1)) {
              if(!val1 || !in_command) {
                 if(!val1 || Level4(player)) {
                    if(Blank(arg1) || (val2 == 2) || (!strcasecmp("page",arg1) && (strlen(arg1) == 4)) || !strncasecmp(arg1,"page ",5)) {
                       if(!val1) {
                          short    loop = 0,count,total,vfor,vagainst,vabstain,forscore,againstscore,abstainscore;
                          struct   bbs_message_data *ptr = NULL;
                          unsigned char cached_scrheight,ignored;
                          short    unread;

                          /* ---->  View messages in current selected topic  <---- */
                          if(val2 != 2) {
                             if(!Blank(arg1)) for(arg1 += 4; *arg1 && (*arg1 == ' '); arg1++);
                             if(Blank(arg1) && !Blank(arg2)) arg1 = arg2;
                             arg1 = (char *) parse_grouprange(player,arg1,DEFAULT,1);
			  } else {
                             parse_grouprange(player,NULL,ALL,1);
                             if(!(message = lookup_message(player,&topic,&subtopic,arg1,&message,&unread,(val2 != 0)))) {
  		                output(p,player,0,1,0,ANSI_LGREEN"Sorry, either a message with that number doesn't exist, or the message number you specified is invalid.");
                                return;
			     }
			  }

                          html_anti_reverse(p,1);
                          cached_scrheight = db[player].data->player.scrheight;
                          db[player].data->player.scrheight = db[player].data->player.scrheight - ((topic->timelimit) ? 10:9) - ((topic->subtopics) ? 1:0);
                          if(IsHtml(p)) output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");

                          if(!in_command) {
                             if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_buffer,decompress(topic->desc));
                                else strcpy(scratch_buffer,decompress(topic->desc));
                             substitute(Validchar(topic->owner) ? topic->owner:player,scratch_return_string,scratch_buffer,0,ANSI_LWHITE,NULL,0);
                             if(!IsHtml(p)) {
                                output(p,player,0,1,0,"");
                                output(p,player,0,1,strlen(topic->name) + ((subtopic) ? (strlen(subtopic->name) + 1):0) + 4,ANSI_LYELLOW" "ANSI_UNDERLINE"%s%s%s"ANSI_DCYAN":  "ANSI_LWHITE"%s",(subtopic) ? subtopic->name:"",(subtopic) ? "/":"",topic->name,scratch_return_string);
                                output(p,player,0,1,0,separator(twidth,0,'-','='));
			     } else output(p,player,2,1,0,"\016<TR><TH ALIGN=CENTER WIDTH=20%% BGCOLOR="HTML_TABLE_YELLOW"><FONT SIZE=4><I>\016"ANSI_LYELLOW""ANSI_UNDERLINE"%s%s%s"ANSI_DCYAN"\016</I></FONT></TH><TH ALIGN=LEFT BGCOLOR="HTML_TABLE_BLUE"><FONT SIZE=4><I>\016"ANSI_LWHITE"%s\016</I></FONT></TH></TR>\016",(subtopic) ? subtopic->name:"",(subtopic) ? "/":"",topic->name,scratch_return_string);
			  }

                          total = bbs_messagecount(topic->messages,player,&unread);
                          if(val2) set_conditions(player,0,0,0,(message) ? message->id:0,NULL,(val2 == 2) ? 508:507);
                          union_initgrouprange((union group_data *) topic->messages);
                          while(union_grouprange()) {
                                if(val2) {
                                   for(ptr = topic->messages, loop = 1; ptr && (ptr != &(grp->cunion->message)); loop++, ptr = ptr->next);
				} else loop++;
                                sprintf(scratch_return_string,"(%d)",(val2) ? loop:(grp->before + loop));
                                substitute(Validchar(grp->cunion->message.owner) ? grp->cunion->message.owner:player,scratch_return_string + 100,decompress(grp->cunion->message.subject),0,ANSI_LYELLOW,NULL,0);
                                if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string + 100,scratch_return_string + 100);
                                if(IsHtml(p)) sprintf(scratch_buffer,"\016<TR><TD ALIGN=CENTER WIDTH=20%% BGCOLOR="HTML_TABLE_CYAN"><FONT SIZE=4><A HREF=\"%sCOMMAND=%%7Cbbs+read+%d&\" TARGET=TCZINPUT>\016%s\016</A></FONT></TD><TD ALIGN=LEFT>\016"ANSI_LWHITE"'%s"ANSI_LYELLOW"%s"ANSI_LWHITE"' left by ",html_server_url(p,1,2,"input"),(val2) ? loop:(grp->before + loop),scratch_return_string,(grp->cunion->message.flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string + 100);
                                   else sprintf(scratch_buffer,ANSI_LGREEN" %-7s"ANSI_LWHITE"'%s"ANSI_LYELLOW"%s"ANSI_LWHITE"' left by ",scratch_return_string,(grp->cunion->message.flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string + 100);
                                sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LGREEN"%s"ANSI_LWHITE" on "ANSI_LCYAN"%s%s",(grp->cunion->message.flags & MESSAGE_ANON) ? "<ANONYMOUS>":decompress(grp->cunion->message.name),date_to_string(grp->cunion->message.date + (db[player].data->player.timediff * HOUR),UNSET_DATE,player,FULLDATEFMT),bbs_unread_message(&(grp->cunion->message),player,&ignored) ? ANSI_LMAGENTA" (*UNREAD*)":(ignored) ? ANSI_LMAGENTA" (Ignored)":"");
                                if((count = bbs_readercount(&(grp->cunion->message))) > 0) sprintf(scratch_return_string,"%d",count);
                                   else strcpy(scratch_return_string,"None");
                                if(!(grp->cunion->message.flags & MESSAGE_REPLY) && ((count = bbs_replycount(topic,&(grp->cunion->message),grp->cunion->message.id,1)) > 0))
                                   sprintf(scratch_return_string + 100,ANSI_LBLUE" (Replies:  "ANSI_LWHITE"%d"ANSI_LBLUE")",count);
			              else scratch_return_string[100] = '\0';
                                sprintf(scratch_buffer + strlen(scratch_buffer),"%s%s"ANSI_LRED" (Readers:  "ANSI_LWHITE"%s"ANSI_LRED"%s)",(grp->cunion->message.flags & MESSAGE_APPEND) ? ANSI_LBLUE" (Appended)":"",scratch_return_string + 100,scratch_return_string,(grp->cunion->message.flags & MESSAGE_VOTING) ? "":".");
                                if(bbs_votecount(&(grp->cunion->message),&vfor,&vagainst,&vabstain,&forscore,&againstscore,&abstainscore) || (grp->cunion->message.flags & MESSAGE_VOTING)) {
                                   if(grp->cunion->message.expiry) {
                                      time_t now;

                                      gettime(now);
                                      now = (grp->cunion->message.expiry * DAY) - (now % DAY);
                                      sprintf(scratch_return_string + 300,", Expires: \016&nbsp;\016 "ANSI_LWHITE"%dd %dh"ANSI_LYELLOW,(int) now / DAY,((int) now % DAY) / HOUR);
				   } else strcpy(scratch_return_string + 300,ANSI_LYELLOW);

                                   if(!(grp->cunion->message.flags & MESSAGE_PRIVATE)) {
                                      int    total = forscore + againstscore + abstainscore;
                                      double forpercent,againstpercent,abstainpercent;

                                      forpercent     = (total == 0) ? 0:(((double) forscore / total) * 100);
                                      againstpercent = (total == 0) ? 0:(((double) againstscore / total) * 100);
                                      abstainpercent = (total == 0) ? 0:(((double) abstainscore / total) * 100);

                                      if(vfor) sprintf(scratch_return_string,"%d "ANSI_LYELLOW"(%s%.0f%%"ANSI_LYELLOW")",vfor,((forpercent >= againstpercent) && (forpercent >= abstainpercent)) ? ANSI_LGREEN:ANSI_LRED,forpercent);
		   		         else strcpy(scratch_return_string,"None"ANSI_LYELLOW);
                                      if(vagainst) sprintf(scratch_return_string + 100,"%d "ANSI_LYELLOW"(%s%.0f%%"ANSI_LYELLOW")",vagainst,((againstpercent >= forpercent) && (againstpercent >= abstainpercent)) ? ANSI_LGREEN:ANSI_LRED,againstpercent);
	                                 else strcpy(scratch_return_string + 100,"None"ANSI_LYELLOW);
                                      if(vabstain) sprintf(scratch_return_string + 200,", Abstain: \016&nbsp;\016 "ANSI_LWHITE"%d "ANSI_LYELLOW"(%s%.0f%%"ANSI_LYELLOW")",vabstain,((abstainpercent >= againstpercent) && (abstainpercent >= forpercent)) ? ANSI_LGREEN:ANSI_LRED,abstainpercent);
		                         else strcpy(scratch_return_string + 200,ANSI_LYELLOW);
                                      sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LYELLOW" (%sotes for: \016&nbsp;\016 "ANSI_LWHITE"%s, Against:  "ANSI_LWHITE"%s%s%s.)",(grp->cunion->message.flags & MESSAGE_MAJORITY) ? "Majority factor v":"V",scratch_return_string,scratch_return_string + 100,scratch_return_string + 200,scratch_return_string + 300);
				   } else sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LYELLOW" (Votes subject to a secret ballot%s.)",scratch_return_string + 300);
				}
                                output(p,player,2,1,8,"%s%s",scratch_buffer,IsHtml(p) ? "\016</TD></TR>\016":"\n");
			  }

                          if(grp->rangeitems == 0) output(p,player,2,1,1,IsHtml(p) ? "\016<TR ALIGN=CENTER><TD COLSPAN=2>"ANSI_LCYAN"<I>*** &nbsp; SORRY, THERE ARE NO MESSAGES IN THIS %sTOPIC AT THE MOMENT &nbsp; ***</I></TD></TR>\016":ANSI_LCYAN" ***  SORRY, THERE ARE NO MESSAGES IN THIS %sTOPIC AT THE MOMENT  ***\n",(subtopic) ? "SUB-":"");
                          if(!in_command) {

                             /* ---->  Message instructions  <---- */
                             if((grp->rangeitems > 0) || topic->subtopics) {
                                unsigned char msg = 0;
   
                                if(!IsHtml(p)) {
                                   output(p,player,0,1,0,separator(twidth,0,'-','-'));
		                   strcpy(scratch_buffer,ANSI_LWHITE" ");
				} else strcpy(scratch_buffer,"\016<TR BGCOLOR="HTML_TABLE_MGREY"><TD ALIGN=CENTER COLSPAN=2><I>\016"ANSI_LWHITE);
                                if(grp->rangeitems > 0) sprintf(scratch_buffer + strlen(scratch_buffer),"To read one of the above messages, simply type '"ANSI_LGREEN"%sread <NUMBER>"ANSI_LWHITE"'.",(command_type == BBS_COMMAND) ? "bbs ":""), msg = 1;
                                if(topic->subtopics) {
                                   struct bbs_topic_data *ptr = topic->subtopics;
                                   int    subcount = 0,subunread = 0;
                                   short  unread;

                                   for(; ptr; ptr = ptr->next, subcount++) {
                                       bbs_messagecount(ptr->messages,player,&unread);
                                       subunread += unread;
				   }
                                   if(subunread) sprintf(scratch_buffer + strlen(scratch_buffer),"%sThis topic also has "ANSI_LYELLOW"%d"ANSI_LWHITE" sub-topic%s (Type '"ANSI_LGREEN"%ssubtopics"ANSI_LWHITE"' to list %s), which contain%s "ANSI_LYELLOW"%d"ANSI_LWHITE" unread message%s.",(msg) ? IsHtml(p) ? " \016&nbsp;\016 ":"  ":"",subcount,Plural(subcount),(command_type == BBS_COMMAND) ? "bbs ":"",(subcount == 1) ? "it":"them",(subcount == 1) ? "s":"",subunread,Plural(subunread)), msg = 1;
                                      else sprintf(scratch_buffer + strlen(scratch_buffer),"%sThis topic also has "ANSI_LYELLOW"%d"ANSI_LWHITE" sub-topic%s (Type '"ANSI_LGREEN"%ssubtopics"ANSI_LWHITE"' to list %s.)",(msg) ? IsHtml(p) ? " \016&nbsp;\016 ":"  ":"",subcount,Plural(subcount),(command_type == BBS_COMMAND) ? "bbs ":"",(subcount == 1) ? "it":"them"), msg = 1;
				}
                                if(topic->timelimit) sprintf(scratch_buffer + strlen(scratch_buffer),"%s"ANSI_LCYAN"PLEASE NOTE: %s "ANSI_LWHITE"Messages in this %stopic which haven't been read for "ANSI_LYELLOW"%d day%s"ANSI_LWHITE" will be deleted.",(msg) ? IsHtml(p) ? " \016&nbsp;\016 ":"  ":"",IsHtml(p) ? "\016&nbsp;\016":"",(subtopic) ? "sub-":"",topic->timelimit,Plural(topic->timelimit));
                                output(p,player,2,1,1,"%s%s",scratch_buffer,IsHtml(p) ? "\016</I></TD></TR>\016":"\n");
			     }

                             /* ---->  Message navigation buttons (HTML)  <---- */
                             if(IsHtml(p)) {
                                strcpy(scratch_buffer,"<TR BGCOLOR="HTML_TABLE_GREY"><TD ALIGN=CENTER COLSPAN=2>");
                                if((total > 0) && (p->currentmsg < total))
                                   sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Cbbs+read+next&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[NEXT]\" BORDER=0></A> ",html_server_url(p,1,2,"input"),html_image_url("next.gif"));
                                      else sprintf(scratch_buffer + strlen(scratch_buffer),"<IMG SRC=\"%s\" ALT=\"{NEXT}\" BORDER=0> ",html_image_url("nonext.gif"));
                                if((total > 0) && (p->currentmsg > 1))
                                   sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Cbbs+read+prev&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[PREVIOUS]\" BORDER=0></A> ",html_server_url(p,1,2,"input"),html_image_url("prev.gif"));
                                      else sprintf(scratch_buffer + strlen(scratch_buffer),"<IMG SRC=\"%s\" ALT=\"{PREVIOUS}\" BORDER=0> ",html_image_url("noprev.gif"));

                                sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Cbbs+read+unread&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[NEXT UNREAD]\" BORDER=0></A> ",html_server_url(p,1,2,"input"),html_image_url("nextunread.gif"));
                                if((grp->nogroups > 0) && (grp->groupno < grp->nogroups))
                                   sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Cbbs+read+page+%d&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[NEXT PAGE]\" BORDER=0></A> ",html_server_url(p,1,2,"input"),grp->groupno + 1,html_image_url("nextpage.gif"));
                                      else sprintf(scratch_buffer + strlen(scratch_buffer),"<IMG SRC=\"%s\" ALT=\"{NEXT PAGE}\" BORDER=0> ",html_image_url("nonextpage.gif"));
                                if((grp->nogroups > 0) && (grp->groupno > 1))
                                   sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Cbbs+read+page+%d&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[PREVIOUS PAGE]\" BORDER=0></A> ",html_server_url(p,1,2,"input"),grp->groupno - 1,html_image_url("prevpage.gif"));
                                      else sprintf(scratch_buffer + strlen(scratch_buffer),"<IMG SRC=\"%s\" ALT=\"{PREVIOUS PAGE}\" BORDER=0> ",html_image_url("noprevpage.gif"));
                                sprintf(scratch_buffer + strlen(scratch_buffer),"<BR><A HREF=\"%sCOMMAND=%%7Cbbs+topics&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[TOPICS]\" BORDER=0></A> ",html_server_url(p,1,2,"input"),html_image_url("topics.gif"));
                                if(!val1 && ((subtopic && subtopic->subtopics) || (!subtopic && topic->subtopics)))
                                   sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Cbbs+subtopics&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[SUB-TOPICS]\" BORDER=0></A> ",html_server_url(p,1,2,"input"),html_image_url("subtopics.gif"));
                                      else sprintf(scratch_buffer + strlen(scratch_buffer),"<IMG SRC=\"%s\" ALT=\"{SUB-TOPICS}\" BORDER=0> ",html_image_url("nosubtopics.gif"));
                                sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Csummary&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[SUMMARY]\" BORDER=0></A> ",html_server_url(p,1,2,"input"),html_image_url("summary.gif"));
                                sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Clatest&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[LATEST]\" BORDER=0></A>",html_server_url(p,1,2,"input"),html_image_url("latest.gif"));
                                output(p,player,1,2,0,"%s</TD></TR>",scratch_buffer);
			     }

                             /* ---->  Total messages listed/unread messages  <---- */
                             if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
                             if(grp->rangeitems != 0) {
                                listed_items(scratch_return_string + 100,1);
                                if(unread) sprintf(scratch_return_string,"%d",unread);
                                   else strcpy(scratch_return_string,"None");
                                output(p,player,2,1,1,"%sMessages listed: %s "ANSI_DWHITE"%s%s"ANSI_LWHITE"Total unread messages: %s "ANSI_DWHITE"%s.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_MGREY"><TD COLSPAN=2>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",IsHtml(p) ? "\016&nbsp;\016":"",scratch_return_string + 100,IsHtml(p) ? "\016 &nbsp; &nbsp; \016":"   ",IsHtml(p) ? "\016&nbsp;\016":"",scratch_return_string,IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
			     } else output(p,player,2,1,1,"%sMessages listed: %s "ANSI_DWHITE"None.%s"ANSI_LWHITE"Total unread messages: %s "ANSI_DWHITE"None.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_MGREY"><TD COLSPAN=2>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",IsHtml(p) ? "\016&nbsp;\016":"",IsHtml(p) ? "\016 &nbsp; &nbsp; \016":"   ",IsHtml(p) ? "\016&nbsp;\016":"",IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
			  }

                          if(IsHtml(p)) output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
                          db[player].data->player.scrheight = cached_scrheight;
                          html_anti_reverse(p,0);
                          setreturn(OK,COMMAND_SUCC);
		       } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, the '"ANSI_LWHITE"readanon"ANSI_LGREEN"'/'"ANSI_LWHITE"viewanon"ANSI_LGREEN"' command can only be used to read a message and reveal the user who added it anonymously.");
		    } else if((val1 != 1) || !Blank(arg2)) {
                       short  vfor,vagainst,vabstain,forscore,againstscore,abstainscore;
                       struct descriptor_data *d;
                       short  count;

                       /* ---->  View specified message  <---- */
                       if(!lookup || (message = lookup_message(player,&topic,&subtopic,arg1,&message,&no,(val2 != 0)))) {
                          if(!val1 || (message->flags & MESSAGE_ANON)) {
                             if(val1 && (message->flags & MESSAGE_ANON) && (val1 && !((!Level4(message->owner) && Level4(player)) || (Level4(message->owner) && Level1(player))))) {
                                output(p,player,0,1,0,ANSI_LGREEN"Sorry, unable to reveal the identity of the anonymous user who posted that message.");
                                return;
			     }

                             /* ---->  Message header  <---- */
                             gettime(message->lastread);
                             html_anti_reverse(p,1);
                             if(!in_command && p && !p->pager && !IsHtml(p) && More(player)) pager_init(p);
                             for(d = descriptor_list; d; d = d->next)
                                 if(d->player == player)
                                    d->currentmsg = no;
                             if(IsHtml(p)) output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
                                else output(p,player,0,1,0,"");

                             substitute(Validchar(message->owner) ? message->owner:player,scratch_return_string,decompress(message->subject),0,ANSI_LYELLOW,NULL,0);
                             if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                             sprintf(scratch_buffer,"%s"ANSI_UNDERLINE"%s%s%s"ANSI_DCYAN"%s"ANSI_LGREEN"(%d) \016&nbsp;\016 "ANSI_LWHITE"'%s"ANSI_LYELLOW"%s"ANSI_LWHITE"' left by ",IsHtml(p) ? "\016<TR><TH ALIGN=CENTER WIDTH=20%% BGCOLOR="HTML_TABLE_YELLOW"><FONT SIZE=4><I>\016"ANSI_LYELLOW:ANSI_LYELLOW" ",(subtopic) ? subtopic->name:"",(subtopic) ? "/":"",topic->name,IsHtml(p) ? "\016</I></FONT></TH><TH ALIGN=LEFT BGCOLOR="HTML_TABLE_BLUE"><I>\016":":  ",no,(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string);
                             sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LGREEN"%s%s"ANSI_LWHITE" on "ANSI_LCYAN,((message->flags & MESSAGE_ANON) && !(val1 && ((!Level4(message->owner) && Level4(player)) || (Level4(message->owner) && Level1(player))))) ? "":decompress(message->name),(message->flags & MESSAGE_ANON) ? (val1 && ((!Level4(message->owner) && Level4(player)) || (Level4(message->owner) && Level1(player)))) ? ANSI_LYELLOW" (ANONYMOUS)":"<ANONYMOUS>":"");
                             output(p,player,2,1,strlen(topic->name) + ((subtopic) ? (strlen(subtopic->name) + 1):0) + digit_wrap(8,no),"%s%s"ANSI_LWHITE".%s",scratch_buffer,date_to_string(message->date + (db[player].data->player.timediff * HOUR),UNSET_DATE,player,FULLDATEFMT),IsHtml(p) ? "\016</I></TH></TR>\016":"\n");

                             /* ---->  Message text  <---- */
                             bbs_query_readertimediff(player,NULL,NULL,NULL,NULL,1,player);
                             wrap_leading = (topic->flags & TOPIC_FORMAT) ? 1:0;
                             if(IsHtml(p)) {
                                output(p,player,1,2,0,"<TR><TD ALIGN=LEFT COLSPAN=2><FONT SIZE=4>");
                                substitute_large(Validchar(message->owner) ? message->owner:player,player,(topic->flags & TOPIC_FORMAT) ? punctuate(decompress(message->message),2,'.'):decompress(message->message),ANSI_LWHITE,scratch_return_string,(topic->flags & TOPIC_CENSOR));
                                output(p,player,1,2,0,"</FONT></TD></TR>");
			     } else {
                                output(p,player,0,1,0,separator(twidth,0,'-','='));
                                substitute_large(Validchar(message->owner) ? message->owner:player,player,(topic->flags & TOPIC_FORMAT) ? punctuate(decompress(message->message),2,'.'):decompress(message->message),ANSI_LWHITE,scratch_return_string,(topic->flags & TOPIC_CENSOR));
			     }
                             bbs_query_readertimediff(player,NULL,NULL,NULL,NULL,1,NOTHING);
                             wrap_leading = 0;

                             /* ---->  Voting information (If voting allowed on message)  <---- */
                             if(message->flags & MESSAGE_VOTING) {
                                if(message->expiry) {
                                   time_t now;

                                   gettime(now);
                                   now = (message->expiry * DAY) - (now % DAY);
                                   sprintf(scratch_return_string," \016&nbsp;\016 Voting on this message closes in "ANSI_LCYAN"%s"ANSI_LWHITE" time.",interval(now,now,ENTITIES,0));
				} else *scratch_return_string = '\0';

                                if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','-'));
                                output(p,player,2,1,1,"%sYou can vote for this message by typing '"ANSI_LGREEN"%svote %d for"ANSI_LWHITE"' or against it by typing '"ANSI_LGREEN"%svote %d against"ANSI_LWHITE"'. \016&nbsp;\016 You can also abstain your vote by typing '"ANSI_LGREEN"%svote %d abstain"ANSI_LWHITE"'.%s%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_MGREY"><TD ALIGN=CENTER COLSPAN=2><I>\016"ANSI_LWHITE:ANSI_LWHITE" ",(command_type == BBS_COMMAND) ? "bbs ":"",no,(command_type == BBS_COMMAND) ? "bbs ":"",no,(command_type == BBS_COMMAND) ? "bbs ":"",no,scratch_return_string,IsHtml(p) ? "\016</I></TD></TR>\016":"\n");
			     }

                             /* ---->  Message navigation buttons (HTML)  <---- */
                             if(IsHtml(p)) {
                                short total,unread;

                                total = bbs_messagecount(topic->messages,player,&unread);
                                strcpy(scratch_buffer,"<TR BGCOLOR="HTML_TABLE_GREY"><TD ALIGN=CENTER COLSPAN=2>");
                                if((total > 0) && (no < total))
                                   sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Cbbs+read+next&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[NEXT]\" BORDER=0></A> ",html_server_url(p,1,2,"input"),html_image_url("next.gif"));
                                      else sprintf(scratch_buffer + strlen(scratch_buffer),"<IMG SRC=\"%s\" ALT=\"{NEXT}\" BORDER=0> ",html_image_url("nonext.gif"));
                                if((total > 0) && (no > 1))
                                   sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Cbbs+read+prev&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[PREVIOUS]\" BORDER=0></A> ",html_server_url(p,1,2,"input"),html_image_url("prev.gif"));
                                      else sprintf(scratch_buffer + strlen(scratch_buffer),"<IMG SRC=\"%s\" ALT=\"{PREVIOUS}\" BORDER=0> ",html_image_url("noprev.gif"));
                                sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Cbbs+read+unread&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[NEXT UNREAD]\" BORDER=0></A> ",html_server_url(p,1,2,"input"),html_image_url("nextunread.gif"));
                                sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Cbbs+reply+%d&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[LEAVE REPLY]\" BORDER=0></A> ",html_server_url(p,1,2,"input"),no,html_image_url("reply.gif"));

                                sprintf(scratch_buffer + strlen(scratch_buffer),"<BR><A HREF=\"%sCOMMAND=%%7Cbbs+topics&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[TOPICS]\" BORDER=0></A> ",html_server_url(p,1,2,"input"),html_image_url("topics.gif"));
                                if(!val1 && ((subtopic && subtopic->subtopics) || (!subtopic && topic->subtopics)))
                                   sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Cbbs+subtopics&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[SUB-TOPICS]\" BORDER=0></A> ",html_server_url(p,1,2,"input"),html_image_url("subtopics.gif"));
                                      else sprintf(scratch_buffer + strlen(scratch_buffer),"<IMG SRC=\"%s\" ALT=\"{SUB-TOPICS}\" BORDER=0> ",html_image_url("nosubtopics.gif"));
                                sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Csummary&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[SUMMARY]\" BORDER=0></A> ",html_server_url(p,1,2,"input"),html_image_url("summary.gif"));
                                sprintf(scratch_buffer + strlen(scratch_buffer),"<A HREF=\"%sCOMMAND=%%7Clatest&\" TARGET=TCZINPUT><IMG SRC=\"%s\" ALT=\"[LATEST]\" BORDER=0></A>",html_server_url(p,1,2,"input"),html_image_url("latest.gif"));
                                output(p,player,1,2,0,"%s</TD></TR>",scratch_buffer);
			     }

                             /* ---->  Message footer  <---- */
                             bbs_update_readers(message,player,1,((player != message->owner) && (Controller(player) != message->owner) && !((bbs_last_message == message) && (bbs_last_reader == player))));
                             if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
                             if(bbs_votecount(message,&vfor,&vagainst,&vabstain,&forscore,&againstscore,&abstainscore) || (message->flags & MESSAGE_VOTING)) {
                                if(message->expiry) {
                                   time_t now;

                                   gettime(now);
                                   now = (message->expiry * DAY) - (now % DAY);
                                   *(scratch_return_string + 2048) = '\0';
                                   if((now / DAY) > 0) sprintf((scratch_return_string + 2048) + strlen(scratch_return_string + 2048),"%d day%s",(int) now / DAY,Plural((int) now / DAY));
                                   if(((now % DAY) / HOUR) > 0) sprintf((scratch_return_string + 2048) + strlen(scratch_return_string + 2048),"%s%d hour%s",!Blank(scratch_return_string + 2048) ? ", ":"",((int) now % DAY) / HOUR,Plural((now % DAY) / HOUR));
                                   sprintf(scratch_return_string + 300,", Expires: \016&nbsp;\016 "ANSI_LWHITE"%s"ANSI_LYELLOW,scratch_return_string + 2048);
				} else strcpy(scratch_return_string + 300,ANSI_LYELLOW);

                                if(!(message->flags & MESSAGE_REPLY) && ((count = bbs_replycount(topic,message,message->id,1)) > 0))
                                   sprintf(scratch_return_string + 500,ANSI_LBLUE"(Replies:  "ANSI_LWHITE"%d"ANSI_LBLUE") ",count);
	   		              else scratch_return_string[500] = '\0';
                                if(!(message->flags & MESSAGE_PRIVATE)) {
                                   int    total = forscore + againstscore + abstainscore;
                                   double forpercent,againstpercent,abstainpercent;

                                   forpercent     = (total == 0) ? 0:(((double) forscore / total) * 100);
                                   againstpercent = (total == 0) ? 0:(((double) againstscore / total) * 100);
                                   abstainpercent = (total == 0) ? 0:(((double) abstainscore / total) * 100);

                                   if(vfor) sprintf(scratch_return_string,"%d "ANSI_LYELLOW"(%s%.0f%%"ANSI_LYELLOW")",vfor,((forpercent >= againstpercent) && (forpercent >= abstainpercent)) ? ANSI_LGREEN:ANSI_LRED,forpercent);
				      else strcpy(scratch_return_string,"None"ANSI_LYELLOW);
                                   if(vagainst) sprintf(scratch_return_string + 100,"%d "ANSI_LYELLOW"(%s%.0f%%"ANSI_LYELLOW")",vagainst,((againstpercent >= forpercent) && (againstpercent >= abstainpercent)) ? ANSI_LGREEN:ANSI_LRED,againstpercent);
	                              else strcpy(scratch_return_string + 100,"None"ANSI_LYELLOW);
                                   if(vabstain) sprintf(scratch_return_string + 200,", Abstain: \016&nbsp;\016 "ANSI_LWHITE"%d "ANSI_LYELLOW"(%s%.0f%%"ANSI_LYELLOW")",vabstain,((abstainpercent >= againstpercent) && (abstainpercent >= forpercent)) ? ANSI_LGREEN:ANSI_LRED,abstainpercent);
		                      else strcpy(scratch_return_string + 200,ANSI_LYELLOW);
                                   output(p,player,2,1,11,"%sReaders: \016&nbsp;\016 "ANSI_DWHITE"%d \016&nbsp;\016 %s"ANSI_LYELLOW"(%sotes for: \016&nbsp;\016 "ANSI_LWHITE"%s, Against: \016&nbsp;\016 "ANSI_LWHITE"%s%s%s.)%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_MGREY"><TD COLSPAN=2>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",bbs_readercount(message),scratch_return_string + 500,(message->flags & MESSAGE_MAJORITY) ? "Majority factor v":"V",scratch_return_string,scratch_return_string + 100,scratch_return_string + 200,scratch_return_string + 300,IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
				} else output(p,player,2,1,11,"%sReaders: \016&nbsp;\016 "ANSI_DWHITE"%d \016&nbsp;\016 %s"ANSI_LYELLOW"(Votes subject to a secret ballot%s.)%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_MGREY"><TD COLSPAN=2>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",bbs_readercount(message),scratch_return_string + 500,scratch_return_string + 300,IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
			     } else {
                                if(!(message->flags & MESSAGE_REPLY) && ((count = bbs_replycount(topic,message,message->id,1)) > 0))
                                   sprintf(scratch_return_string," \016&nbsp;\016 "ANSI_LBLUE"(Replies:  "ANSI_LWHITE"%d"ANSI_LBLUE".)",count);
			              else strcpy(scratch_return_string,".");
                                output(p,player,2,1,1,"%sReaders: \016&nbsp;\016 "ANSI_DWHITE"%d%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_MGREY"><TD COLSPAN=2>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",bbs_readercount(message),scratch_return_string,IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
			     }
                             bbs_last_message = message, bbs_last_reader = player;
                             if(IsHtml(p)) output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");

                             if(val1) {
                                sprintf(scratch_buffer,"%s%s(#%d) revealed the identity of the user who posted this message  -  REASON:  %s",bbs_logmsg(message,topic,subtopic,no,1),getname(player),player,punctuate(arg2,2,'.'));
                                if(subtopic) writelog(ADMIN_LOG,1,"ANONYMOUS","%s",scratch_buffer);
                                   else writelog(ADMIN_LOG,1,"ANONYMOUS","%s",scratch_buffer);
                                writelog(BBS_LOG,1,"ANONYMOUS","%s",scratch_buffer);
			     }
                             html_anti_reverse(p,0);
                             setreturn(OK,COMMAND_SUCC);
			  } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"readanon"ANSI_LGREEN"'/'"ANSI_LWHITE"viewanon"ANSI_LGREEN"' can only be used to read anonymous messages.");
		       } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, either a message with that number doesn't exist, or the message number you specified is invalid.");
		    } else output(p,player,0,1,0,ANSI_LGREEN"Please give a reason for revealing the identity of the user who posted this message.");
		 } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, the '"ANSI_LWHITE"readanon"ANSI_LGREEN"'/'"ANSI_LWHITE"viewanon"ANSI_LGREEN"' command can only be used by Apprentice Wizards/Druids and above.");
	      } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, the '"ANSI_LWHITE"readanon"ANSI_LGREEN"'/'"ANSI_LWHITE"viewanon"ANSI_LGREEN"' command can't be used from within a compound command.");
	   } else {
              output(p,player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.  Please choose another %stopic (Type '"ANSI_LWHITE"%s%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%s%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(subtopic) ? "sub-":"",topic->name,clevels[(int) topic->accesslevel],(subtopic) ? "sub-":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"");
              if((subtopic && (db[player].data->player.topic_id == subtopic->topic_id) && (db[player].data->player.subtopic_id == topic->topic_id)) || (!subtopic && (db[player].data->player.topic_id == topic->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
	   }
	} else {
           output(p,player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)  Please choose another topic (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",subtopic->name,clevels[(int) subtopic->accesslevel],topic->name,(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
           if(db[player].data->player.topic_id == topic->topic_id) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
	}
     } else output(p,player,0,1,0,ANSI_LGREEN"Please select a topic first from the list of available topics (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
}

/* ---->  Vote on message/make message votable  <---- */
void bbs_vote(CONTEXT)
{
     struct   descriptor_data *p = getdsc(player);
     struct   bbs_reader_data *new,*reader,*last;
     struct   bbs_topic_data *topic,*subtopic;
     struct   bbs_message_data *message;
     unsigned char reset = 0;
     short    msgno;
     char     *ptr;

     /* ---->  Grab first word as message number  <---- */
     comms_spoken(player,0);
     setreturn(ERROR,COMMAND_FAIL);
     if(Blank(arg2)) {
        for(; *arg1 && (*arg1 == ' '); arg1++);
        for(ptr = arg1; *ptr && (*ptr != ' '); ptr++);
        if(*ptr) for(*ptr = '\0', ptr++; *ptr && (*ptr == ' '); ptr++);
        arg2 = ptr;
     }

     if(!Moron(player)) {
        if((topic = lookup_topic(player,NULL,&topic,&subtopic))) {
           if(!subtopic || can_access_topic(player,subtopic,NULL,1)) {
	      if(can_access_topic(player,topic,subtopic,1)) {
                 if((message = lookup_message(player,&topic,&subtopic,arg1,&message,&msgno,0))) {
                    if(!Blank(arg2) && (string_prefix("list",arg2) || string_prefix("summary",arg2) || string_prefix("view",arg2))) {

#ifdef BBS_LIST_VOTES
                       /* ---->  List people who have voted on message (Deities only)  <---- */
                       if((db[player].owner != message->owner) && (db[player].owner != topic->owner) && !(subtopic && (db[player].owner == subtopic->owner)) && !can_write_to(player,message->owner,1)) {
                          if(Level4(db[player].owner)) output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only list the votes of a message you own, or a message owned by someone of a lower level than yourself.");
                             else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only list the votes of your own messages.");
#else
                       if(!Level1(db[player].owner)) {
                          output(p,player,0,1,0,ANSI_LGREEN"Sorry, only Deities and the Supreme Being may list the votes of a message.");
		       } else if(!can_write_to(player,message->owner,0)) {
                          output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only list the votes of your own messages or those owned by lower level characters.");
#endif
		       } else if(!(message->flags & MESSAGE_PRIVATE)) {
                          short    vfor,vagainst,vabstain,forscore,againstscore,abstainscore,ne_votes,votes;
                          double   forpercent,againstpercent,abstainpercent;
                          unsigned char twidth = output_terminal_width(player);
                          int      total;

                          html_anti_reverse(p,1);
                          if(!in_command && p && !p->pager && !IsHtml(p) && More(player)) pager_init(p);
                          substitute(Validchar(message->owner) ? message->owner:player,scratch_return_string,decompress(message->subject),0,ANSI_LYELLOW,NULL,0);
                          if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                          if(IsHtml(p)) {
                             output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_GREY">",(in_command) ? "":"<BR>");
                             if(subtopic) output(p,player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH><FONT SIZE=5 COLOR="HTML_LCYAN"><I>\016Votes on the message '%s"ANSI_LYELLOW"%s"ANSI_LCYAN"' (Message number "ANSI_LWHITE"%d"ANSI_LCYAN") in the sub-topic '"ANSI_LWHITE"%s"ANSI_LCYAN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'...\016</I></FONT></TH></TR>\016",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,topic->name,subtopic->name);
                                else output(p,player,2,1,0,"\016<TR ALIGN=CENTER><TH BGCOLOR="HTML_TABLE_CYAN"><FONT SIZE=5 COLOR="HTML_LCYAN"><I>\016Votes on the message '%s"ANSI_LYELLOW"%s"ANSI_LCYAN"' (Message number "ANSI_LWHITE"%d"ANSI_LCYAN") in the topic '"ANSI_LWHITE"%s"ANSI_LCYAN"'...\016</I></FONT></TH></TR>\016",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,topic->name);
                             twidth = 89;
			  } else {
                             if(subtopic) output(p,player,0,1,1,"\n Votes on the message '%s"ANSI_LYELLOW"%s"ANSI_LCYAN"' (Message number "ANSI_LWHITE"%d"ANSI_LCYAN") in the sub-topic '"ANSI_LWHITE"%s"ANSI_LCYAN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'...",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,topic->name,subtopic->name);
                                else output(p,player,0,1,1,"\n Votes on the message '%s"ANSI_LYELLOW"%s"ANSI_LCYAN"' (Message number "ANSI_LWHITE"%d"ANSI_LCYAN") in the topic '"ANSI_LWHITE"%s"ANSI_LCYAN"'...",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,topic->name);
                             output(p,player,0,1,0,separator(twidth,0,'-','='));
			  }

                          /* ---->  Votes for message  <---- */
                          for(votes = 0, ne_votes = 0, reader = message->readers; reader; reader = reader->next)
                              if((reader->flags & READER_VOTE_MASK) == READER_VOTE_FOR) {
                                 if(!Validchar(reader->reader)) {
                                    reader->reader = NOTHING;
                                    ne_votes++;
				 }
                                 votes++;
			      }
                          sprintf(scratch_buffer,"%d"ANSI_LGREEN" vote%s for%s",votes,Plural(votes),(ne_votes) ? " ":"...");
                          if(ne_votes) sprintf(scratch_buffer + strlen(scratch_buffer),"("ANSI_LWHITE"%d"ANSI_LGREEN" voter%s no-longer exist%s)...",ne_votes,Plural(ne_votes),(ne_votes == 1) ? "s":"");
                          output(p,player,2,1,0,"%s%s%s",IsHtml(p) ? "\016<TR><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_GREEN"><FONT SIZE=5><B>"ANSI_LWHITE"\016":ANSI_LWHITE" ",scratch_buffer,IsHtml(p) ? "\016</B></FONT></TD></TR>\016":"\n");
                          if(!IsHtml(p)) output(p,player,0,1,0,ANSI_DGREEN"%s",separator(twidth,0,'-','-') + strlen(ANSI_DCYAN));

                          if(votes) {
			     output_columns(p,player,NULL,NULL,twidth,1,20,2,0,1,FIRST,5,"***  NO VOTERS FOUND  ***",scratch_return_string);
			     for(reader = message->readers; reader; reader = reader->next)
				 if((reader->flags & READER_VOTE_MASK) == READER_VOTE_FOR)
				    if(Validchar(reader->reader))
				       output_columns(p,player,getname_prefix(reader->reader,20,scratch_buffer),privilege_colour(reader->reader),0,0,0,0,0,0,DEFAULT,0,NULL,scratch_return_string);
			     output_columns(p,player,NULL,NULL,0,0,0,0,0,0,LAST,0,NULL,scratch_return_string);
			  }

                          /* ---->  Votes against message  <---- */
                          if(!IsHtml(p)) output(p,player,0,1,0,ANSI_DRED"%s",separator(twidth,0,'=','='));
                          for(votes = 0, ne_votes = 0, reader = message->readers; reader; reader = reader->next)
                              if((reader->flags & READER_VOTE_MASK) == READER_VOTE_AGAINST) {
                                 if(!Validchar(reader->reader)) {
                                    reader->reader = NOTHING;
                                    ne_votes++;
				 }
                                 votes++;
			      }
                          sprintf(scratch_buffer,"%d"ANSI_LRED" vote%s against%s",votes,Plural(votes),(ne_votes) ? " ":"...");
                          if(ne_votes) sprintf(scratch_buffer + strlen(scratch_buffer),"("ANSI_LWHITE"%d"ANSI_LRED" voter%s no-longer exist%s)...",ne_votes,Plural(ne_votes),(ne_votes == 1) ? "s":"");
                          output(p,player,2,1,0,"%s%s%s",IsHtml(p) ? "\016<TR><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_RED"><FONT SIZE=5><B>"ANSI_LWHITE"\016":ANSI_LWHITE" ",scratch_buffer,IsHtml(p) ? "\016</B></FONT></TD></TR>\016":"\n");
                          if(!IsHtml(p)) output(p,player,0,1,0,ANSI_DRED"%s",separator(twidth,0,'-','-') + strlen(ANSI_DCYAN));

                          if(votes) {
			     output_columns(p,player,NULL,NULL,twidth,1,20,2,0,1,FIRST,5,"***  NO VOTERS FOUND  ***",scratch_return_string);
			     for(reader = message->readers; reader; reader = reader->next)
				 if((reader->flags & READER_VOTE_MASK) == READER_VOTE_AGAINST)
				    if(Validchar(reader->reader))
				       output_columns(p,player,getname_prefix(reader->reader,20,scratch_buffer),privilege_colour(reader->reader),0,0,0,0,0,0,DEFAULT,0,NULL,scratch_return_string);
			     output_columns(p,player,NULL,NULL,0,0,0,0,0,0,LAST,0,NULL,scratch_return_string);
			  }

                          /* ---->  Abstained votes  <---- */
                          if(!IsHtml(p)) output(p,player,0,1,0,ANSI_DRED"%s",separator(twidth,0,'=','='));
                          for(votes = 0, ne_votes = 0, reader = message->readers; reader; reader = reader->next)
                              if((reader->flags & READER_VOTE_MASK) == READER_VOTE_ABSTAIN) {
                                 if(!Validchar(reader->reader)) {
                                    reader->reader = NOTHING;
                                    ne_votes++;
				 }
                                 votes++;
			      }
                          sprintf(scratch_buffer,"%d"ANSI_LBLUE" vote%s abstained%s",votes,Plural(votes),(ne_votes) ? " ":"...");
                          if(ne_votes) sprintf(scratch_buffer + strlen(scratch_buffer),"("ANSI_LWHITE"%d"ANSI_LBLUE" voter%s no-longer exist%s)...",ne_votes,Plural(ne_votes),(ne_votes == 1) ? "s":"");
                          output(p,player,2,1,0,"%s%s%s",IsHtml(p) ? "\016<TR><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_BLUE"><FONT SIZE=5><B>"ANSI_LWHITE"\016":ANSI_LWHITE" ",scratch_buffer,IsHtml(p) ? "\016</B></FONT></TD></TR>\016":"\n");
                          if(!IsHtml(p)) output(p,player,0,1,0,ANSI_DBLUE"%s",separator(twidth,0,'-','-') + strlen(ANSI_DCYAN));

                          if(votes) {
			     output_columns(p,player,NULL,NULL,twidth,1,20,2,0,1,FIRST,5,"***  NO VOTERS FOUND  ***",scratch_return_string);
			     for(reader = message->readers; reader; reader = reader->next)
				 if((reader->flags & READER_VOTE_MASK) == READER_VOTE_ABSTAIN)
				    if(Validchar(reader->reader))
				       output_columns(p,player,getname_prefix(reader->reader,20,scratch_buffer),privilege_colour(reader->reader),0,0,0,0,0,0,DEFAULT,0,NULL,scratch_return_string);
			     output_columns(p,player,NULL,NULL,0,0,0,0,0,0,LAST,0,NULL,scratch_return_string);
			  }

                          /* ---->  Majority/majority factor  <---- */
                          if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'=','='));
                          bbs_votecount(message,&vfor,&vagainst,&vabstain,&forscore,&againstscore,&abstainscore);
                          total          = forscore + againstscore + abstainscore;
                          forpercent     = (total == 0) ? 0:(((double) forscore / total) * 100);
                          againstpercent = (total == 0) ? 0:(((double) againstscore / total) * 100);
                          abstainpercent = (total == 0) ? 0:(((double) abstainscore / total) * 100);
                          if(vabstain) sprintf(scratch_return_string,", %s%.0f%%"ANSI_LWHITE" abstained",((abstainpercent >= againstpercent) && (abstainpercent >= forpercent)) ? ANSI_LGREEN:ANSI_LRED,abstainpercent);
		             else *scratch_return_string = '\0';
                          output(p,player,2,1,1,"%sMajority%s: \016&nbsp;\016 %s%.0f%%"ANSI_LWHITE" for, %s%.0f%%"ANSI_LWHITE" against%s.%s",IsHtml(p) ? "\016<TR><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_DGREY"><FONT SIZE=4>"ANSI_LMAGENTA"\016":ANSI_LMAGENTA" ",(message->flags & MESSAGE_MAJORITY) ? " factor":"",((forpercent >= againstpercent) && (forpercent >= abstainpercent)) ? ANSI_LGREEN:ANSI_LRED,forpercent,((againstpercent >= forpercent) && (againstpercent >= abstainpercent)) ? ANSI_LGREEN:ANSI_LRED,againstpercent,scratch_return_string,IsHtml(p) ? "\016</TD></TR>\016":"\n");

                          /* ---->  Total number of votes  <---- */
                          if(IsHtml(p)) {
                             output(p,player,2,1,1,"\016<TR ALIGN=CENTER><TD BGCOLOR="HTML_TABLE_MGREY">"ANSI_LWHITE"<B>\016Total votes: \016&nbsp;\016 "ANSI_DWHITE"%d.\016</B></TD></TR>\016",vfor + vagainst + vabstain);
                             output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
			  } else {
                             output(p,player,0,1,0,separator(twidth,0,'-','='));
                             output(p,player,0,1,1,ANSI_LWHITE" Total votes:  "ANSI_DWHITE"%d.\n",vfor + vagainst + vabstain);
			  }
                          html_anti_reverse(p,0);
                          setreturn(OK,COMMAND_SUCC);
		       } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, the votes of that message are private (Subject to a secret ballot.)");
		    } else if(!Blank(arg2) && (string_prefix("reset",arg2) || string_prefix("clear",arg2))) {

                       /* ---->  Reset votes for message  <---- */
                       if(!((db[player].owner != message->owner) && !can_write_to(player,message->owner,0))) {
	                  if(Bbs(player)) {
                             if(!message->expiry) {
                                if(!in_command) {
                                   substitute(Validchar(message->owner) ? message->owner:player,scratch_return_string,decompress(message->subject),0,ANSI_LYELLOW,NULL,0);
                                   if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                                   if(subtopic) output(p,player,0,1,0,ANSI_LGREEN"Votes of the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LWHITE"%d"ANSI_LGREEN") in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' reset.",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,topic->name,subtopic->name);
                                      else output(p,player,0,1,0,ANSI_LGREEN"Votes of the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LWHITE"%d"ANSI_LGREEN") in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' reset.",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,topic->name);
				}
                                for(reader = message->readers; reader; reader = reader->next)
                                    if(reader->flags & READER_VOTE_MASK)
                                       reader->flags &= ~READER_VOTE_MASK;
                                setreturn(OK,COMMAND_SUCC);
			     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, an expiry time for voting on that message has been set.  You cannot reset the votes on the message until voting closes.");
			  } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from resetting the votes of messages on %s BBS.",tcz_full_name);
		       } else if(Level3(db[player].owner)) output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only reset the votes of a message you own, or a message owned by someone of a lower level than yourself.");
                          else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only reset the votes of your own messages.");
		    } else if(!Blank(arg2) && ((reset = (string_prefix("off",arg2) || string_prefix("prevent",arg2))) || string_prefix("on",arg2) || string_prefix("allow",arg2))) {

                       /* ---->  Allow/prevent voting on message  <---- */
                       if(!((db[player].owner != message->owner) && !can_write_to(player,message->owner,0))) {
	                  if(Bbs(player)) {
                             if(!message->expiry) {
  	                        if(!(!reset && (message->flags & MESSAGE_VOTING))) {
                                   if(!(reset && !(message->flags & MESSAGE_VOTING))) {
                                      if(!in_command) {
                                         substitute(Validchar(message->owner) ? message->owner:player,scratch_return_string,decompress(message->subject),0,ANSI_LYELLOW,NULL,0);
                                         if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                                         if(subtopic) output(p,player,0,1,0,ANSI_LGREEN"Users may %s vote on the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LWHITE"%d"ANSI_LGREEN") in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",(reset) ? "no-longer":"now",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,topic->name,subtopic->name);
                                            else output(p,player,0,1,0,ANSI_LGREEN"Users may %s vote on the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LWHITE"%d"ANSI_LGREEN") in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",(reset) ? "no-longer":"now",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,topic->name);
				      }
                                      if(reset) message->flags &= ~MESSAGE_VOTING;
                                         else message->flags |= MESSAGE_VOTING;
                                      setreturn(OK,COMMAND_SUCC);
				   } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, voting is already prevented on that message.");
				} else output(p,player,0,1,0,ANSI_LGREEN"Sorry, voting is already allowed on that message.");
			     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, an expiry time for voting on that message has been set.  You cannot %s voting on the message until voting closes.",(reset) ? "prevent":"allow");
			  } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from allowing/disallowing voting on messages on %s BBS.",tcz_full_name);
		       } else if(Level3(db[player].owner)) output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only %s voting on a message you own, or a message owned by someone of a lower level than yourself.",(reset) ? "prevent":"allow");
                          else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only %s voting on your own messages.",(reset) ? "prevent":"allow");
		    } else if(!Blank(arg2) && (!strcasecmp(arg2,"expiry") || !strncasecmp(arg2,"expiry ",7))) {

                       /* ---->  Set expiry time on voting on a message  <---- */
                       if(!((db[player].owner != message->owner) && !can_write_to(player,message->owner,0))) {
	                  if(Bbs(player)) {
                             time_t now;
                             int    expiry;

                             gettime(now);
                             for(; *arg2 && (*arg2 != ' '); arg2++);
                             for(; *arg2 && (*arg2 == ' '); arg2++);
                             if(!Blank(arg2) && (expiry = atol(arg2))) {
                                if(!message->expiry || (expiry >= message->expiry) || Root(player)) {
                                   if(expiry <= BBS_VOTE_EXPIRY) {
                                      if(!in_command) {
                                         substitute(Validchar(message->owner) ? message->owner:player,scratch_return_string,decompress(message->subject),0,ANSI_LYELLOW,NULL,0);
                                         if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                                         if(subtopic) output(p,player,0,1,0,ANSI_LGREEN"Users may now vote on the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LWHITE"%d"ANSI_LGREEN") in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' for the next "ANSI_LWHITE"%d"ANSI_LGREEN" day%s.",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,topic->name,subtopic->name,expiry,Plural(expiry));
                                            else output(p,player,0,1,0,ANSI_LGREEN"Users may now vote on the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LWHITE"%d"ANSI_LGREEN") in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' for the next "ANSI_LWHITE"%d"ANSI_LGREEN" day%s.",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,topic->name,expiry,Plural(expiry));
				      }

                                      if((now = ((now % DAY) / HOUR)) >= 12) expiry++;
                                      message->expiry  = expiry;
                                      message->flags  |= MESSAGE_VOTING;
                                      setreturn(OK,COMMAND_SUCC);
				   } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, the maximum expiry time for voting on a message is "ANSI_LWHITE"%d"ANSI_LGREEN" day%s.",BBS_VOTE_EXPIRY,Plural(BBS_VOTE_EXPIRY));
				} else output(p,player,0,1,0,ANSI_LGREEN"Sorry, the expiry time for voting on a message cannot be decreased or reset (You can, however, increase it.)");
			     } else output(p,player,0,1,0,ANSI_LGREEN"Please specify the expiry time for voting on this message (In days.)");
			  } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from setting the expiry time for voting on messages on %s BBS.",tcz_full_name);
		       } else if(Level3(db[player].owner)) output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only set the expiry time for voting on a message you own, or a message owned by someone of a lower level than yourself.");
                          else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only set the expiry time for voting on your own messages.");
		    } else if(!Blank(arg2) && ((reset = ((string_prefix("normal",arg2) && (strlen(arg2) > 2)) || string_prefix("standard",arg2))) || string_prefix("majority",arg2) || string_prefix("majorityfactor",arg2))) {

                       /* ---->  Enable/disable majority factor on message  <---- */
                       if(!((db[player].owner != message->owner) && !can_write_to(player,message->owner,0))) {
	                  if(Bbs(player)) {
	                     if(!(!reset && (message->flags & MESSAGE_MAJORITY))) {
                                if(!(reset && !(message->flags & MESSAGE_MAJORITY))) {
                                   if(!in_command) {
                                      substitute(Validchar(message->owner) ? message->owner:player,scratch_return_string,decompress(message->subject),0,ANSI_LYELLOW,NULL,0);
                                      if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                                      if(subtopic) output(p,player,0,1,0,ANSI_LGREEN"Votes of the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LWHITE"%d"ANSI_LGREEN") in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' will %s be subject to the majority factor.",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,topic->name,subtopic->name,(reset) ? "no-longer":"now");
                                         else output(p,player,0,1,0,ANSI_LGREEN"Votes of the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LWHITE"%d"ANSI_LGREEN") in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' will %s be subject to the majority factor.",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,topic->name,(reset) ? "no-longer":"now");
				   }
                                   if(reset) message->flags &= ~MESSAGE_MAJORITY;
                                      else message->flags |= MESSAGE_MAJORITY;
                                   setreturn(OK,COMMAND_SUCC);
				} else output(p,player,0,1,0,ANSI_LGREEN"Sorry, voting on that message isn't currently subject to the majority factor.");
			     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, voting on that message is already subject to the majority factor.");
			  } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from making votes of messages on %s BBS subject to the majority factor/normal majority.",tcz_full_name);
		       } else if(Level3(db[player].owner)) output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only enable/disable the use of the majority factor on a message you own, or a message owned by someone of a lower level than yourself.");
                          else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only enable/disable the use of the majority factor on your own messages.");
		    } else if(!Blank(arg2) && ((reset = (string_prefix("public",arg2) || string_prefix("visible",arg2))) || string_prefix("private",arg2) || string_prefix("secret",arg2) || string_prefix("invisible",arg2))) {

                       /* ---->  Make the votes of a message private (For secret ballot) or public (Default)  <---- */
                       if(!((db[player].owner != message->owner) && !can_write_to(player,message->owner,0))) {
	                  if(Bbs(player)) {
                             if(!reset || !(message->expiry)) {
  	                        if(!(!reset && (message->flags & MESSAGE_PRIVATE))) {
                                   if(!(reset && !(message->flags & MESSAGE_PRIVATE))) {
                                      if(!in_command) {
                                         substitute(Validchar(message->owner) ? message->owner:player,scratch_return_string,decompress(message->subject),0,ANSI_LYELLOW,NULL,0);
                                         if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                                         if(subtopic) output(p,player,0,1,0,ANSI_LGREEN"Votes of the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LWHITE"%d"ANSI_LGREEN") in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' are now %s",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,topic->name,subtopic->name,(reset) ? "public.":"private (Subject to a secret ballot.)");
                                            else output(p,player,0,1,0,ANSI_LGREEN"Votes of the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LWHITE"%d"ANSI_LGREEN") in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' are now %s",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,topic->name,(reset) ? "public.":"private (Subject to a secret ballot.)");
				      }
                                      if(reset) message->flags &= ~MESSAGE_PRIVATE;
                                         else message->flags |= MESSAGE_PRIVATE;
                                      setreturn(OK,COMMAND_SUCC);
				   } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, the votes of that message are already public.");
				} else output(p,player,0,1,0,ANSI_LGREEN"Sorry, the votes of that message are already private (Subject to a secret ballot.)");
			     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, an expiry time for voting on that message has been set.  You cannot make votes subject to a secret ballot public again until voting on the message closes (The results of the vote will automatically be made public at this time.)");
			  } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from making the votes of messages on %s BBS public/private.",tcz_full_name);
		       } else if(Level3(db[player].owner)) output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only make the votes of a message you own public/private, or a message owned by someone of a lower level than yourself.");
                          else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only make the votes of your own messages public/private.");
		    } else if(!Blank(arg2) && 
                       ((reset = ((string_prefix("for",arg2) || string_prefix("yes",arg2)) ? READER_VOTE_FOR:0)) ||
                        (reset = ((string_prefix("against",arg2) || string_prefix("no",arg2)) ? READER_VOTE_AGAINST:0)) ||
                        (reset = (string_prefix("abstained",arg2) ? READER_VOTE_ABSTAIN:0)))) {
                           if(!in_command) {
                              if(message->flags & MESSAGE_VOTING) {
                                 if(Controller(player) == player) {
                                    time_t now,total = db[player].data->player.totaltime;
                                    short  dummy;

                                    gettime(now);
                                    if(Connected(player))
                                       total += (now - db[player].data->player.lasttime);

                                    if(Level4(player) || (total >= (BBS_VOTE_CONSTRAINT * HOUR))) {

                                       /* ---->  Set/change user's vote in readers list  <---- */
                                       for(reader = message->readers, last = NULL; reader; last = reader, reader = reader->next)
                                           if(reader->reader == player) {
                                              if(reset == (reader->flags & READER_VOTE_MASK)) {
                                                 switch(reader->flags & READER_VOTE_MASK) {
                                                        case READER_VOTE_FOR:
                                                             output(p,player,0,1,0,ANSI_LGREEN"Sorry, you have already voted for that message.");
                                                             return;
                                                        case READER_VOTE_AGAINST:
                                                             output(p,player,0,1,0,ANSI_LGREEN"Sorry, you have already voted against that message.");
                                                             return;
                                                        case READER_VOTE_ABSTAIN:
                                                             output(p,player,0,1,0,ANSI_LGREEN"Sorry, you have already abstained your vote on that message.");
                                                             return;
						 }
					      } else if((reader->flags & READER_VOTE_MASK) || (bbs_votecount(message,&dummy,&dummy,&dummy,&dummy,&dummy,&dummy) < BBS_MAX_VOTES)) {
                                                 reader->flags &= ~READER_VOTE_MASK;
                                                 reader->flags |=  reset;
                                                 break;
					      } else {
                                                 output(p,player,0,1,0,ANSI_LGREEN"Sorry, there are too many votes on that message.");
                                                 return;
					      }
					   }

                                       if(!reader) {
                                          if(bbs_votecount(message,&dummy,&dummy,&dummy,&dummy,&dummy,&dummy) < BBS_MAX_VOTES) {

                                             /* ---->  Add user to readers list and set vote  <---- */
                                             MALLOC(new,struct bbs_reader_data);
                                             new->reader = player;
                                             new->flags  = reset;
                                             new->next   = NULL;
                                             if(last) last->next = new;
                                                else message->readers = new;
					  } else {
                                             output(p,player,0,1,0,ANSI_LGREEN"Sorry, there are too many votes on that message.");
                                             return;
					  }
				       }

                                       substitute(Validchar(message->owner) ? message->owner:player,scratch_return_string,decompress(message->subject),0,ANSI_LYELLOW,NULL,0);
                                       if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                                       if(subtopic) output(p,player,0,1,0,ANSI_LGREEN"Your %s the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LWHITE"%d"ANSI_LGREEN") in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' has been registered.",(reset == READER_VOTE_FOR) ? "vote for":(reset == READER_VOTE_AGAINST) ? "vote against":(reset == READER_VOTE_ABSTAIN) ? "abstained vote on":"unknown vote on",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,topic->name,subtopic->name);
                                          else output(p,player,0,1,0,ANSI_LGREEN"Your %s the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LWHITE"%d"ANSI_LGREEN") in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' has been registered.",(reset == READER_VOTE_FOR) ? "vote for":(reset == READER_VOTE_AGAINST) ? "vote against":(reset == READER_VOTE_ABSTAIN) ? "abstained vote on":"unknown vote on",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,topic->name);
                                       setreturn(OK,COMMAND_SUCC);
				    } else {
                                       sprintf(scratch_buffer,ANSI_LGREEN"Sorry, only Mortals with a total connected time of more than "ANSI_LWHITE"%s"ANSI_LGREEN" may vote on messages on %s BBS (Your total time connected is "ANSI_LYELLOW,tcz_full_name,interval(BBS_VOTE_CONSTRAINT * HOUR,BBS_VOTE_CONSTRAINT * HOUR,ENTITIES,0));
                                       output(p,player,0,1,0,"%s%s"ANSI_LGREEN".)",scratch_buffer,interval(total,total,ENTITIES,0));
                                       return;
				    }
				 } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, puppets may not vote on messages.");
			      } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can't vote on that message (Voting isn't allowed on it.)");
			   } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can't vote on a message from within a compound command.");
		    } else output(p,player,0,1,0,(message->flags & MESSAGE_VOTING) ? ANSI_LGREEN"Please vote either '"ANSI_LWHITE"for"ANSI_LGREEN"' or '"ANSI_LWHITE"against"ANSI_LGREEN"' this message, or '"ANSI_LWHITE"abstain"ANSI_LGREEN"' your vote.":ANSI_LGREEN"Sorry, you can't vote on that message (Voting isn't allowed on it.)");
		 } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, a message with that number doesn't exist.");
	      } else {
                 output(p,player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.  Please choose another %stopic (Type '"ANSI_LWHITE"%s%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%s%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(subtopic) ? "sub-":"",topic->name,clevels[(int) topic->accesslevel],(subtopic) ? "sub-":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"");
                 if((subtopic && (db[player].data->player.topic_id == subtopic->topic_id) && (db[player].data->player.subtopic_id == topic->topic_id)) || (!subtopic && (db[player].data->player.topic_id == topic->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
	      }
	   } else {
              output(p,player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)  Please choose another topic (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",subtopic->name,clevels[(int) subtopic->accesslevel],topic->name,(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
              if(db[player].data->player.topic_id == topic->topic_id) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
	   }
	} else output(p,player,0,1,0,ANSI_LGREEN"Please choose a topic first by typing '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"' (Type '"ANSI_LYELLOW"%stopics"ANSI_LGREEN"' to see the list of available topics.)",(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, Morons aren't allowed to vote on messages on %s BBS.",tcz_full_name);
}


/*  ========================================================================  */
/*    ---->                Message owner BBS commands                <----    */
/*  ========================================================================  */


/* ---->           Append to message           <---- */
/*        (val1:  0 = Normal, 1 = Anonymous.)        */
void bbs_append(CONTEXT)
{
     struct   bbs_topic_data *topic,*subtopic;
     struct   bbs_message_data *message;
     unsigned char state;
     short    msgno;
     char     *ptr;

     /* ---->  Grab first word as message number  <---- */
     comms_spoken(player,0);
     setreturn(ERROR,COMMAND_FAIL);
     if(Blank(arg2)) {
        for(; *arg1 && (*arg1 == ' '); arg1++);
        for(ptr = arg1; *ptr && (*ptr != ' '); ptr++);
        if(*ptr) for(*ptr = '\0', ptr++; *ptr && (*ptr == ' '); ptr++);
        arg2 = ptr;
     }

     if(!Moron(player)) {
        if(Bbs(player)) {
           if((topic = lookup_topic(player,NULL,&topic,&subtopic))) {
              if(!subtopic || can_access_topic(player,subtopic,NULL,1)) {
  	         if(can_access_topic(player,topic,subtopic,1)) {
	            if((message = lookup_message(player,&topic,&subtopic,arg1,&message,&msgno,0))) {
                       if(!((db[player].owner != message->owner) && (db[player].owner != topic->owner) && !(subtopic && (db[player].owner == subtopic->owner)) && !Level4(Owner(player)) && !can_write_to(player,message->owner,1))) {
   	                  if(!Blank(arg2)) {
                             if(!((!(state = 0) && (!strcasecmp("off",arg2) || !strcasecmp("no",arg2))) || ((state = 1) && (!strcasecmp("on",arg2) || !strcasecmp("yes",arg2))))) {
                                if(message->flags & MESSAGE_ALLOWAPPEND) {
                                   bbs_appendmessage(player,message,topic,subtopic,arg2,val1);
                                   if(!in_command) {
                                      substitute(Validchar(message->owner) ? message->owner:player,scratch_return_string,decompress(message->subject),0,ANSI_LYELLOW,NULL,0);
                                      if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                                      if(subtopic) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LWHITE"%d"ANSI_LGREEN")%s appended to in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,(val1) ? " anonymously":"",topic->name,subtopic->name);
                                         else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LWHITE"%d"ANSI_LGREEN")%s appended to in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,(val1) ? " anonymously":"",topic->name);
                                      if(subtopic) {
  			                 if(val1) bbs_output_except(player,MIN(topic->accesslevel,((subtopic) ? subtopic->accesslevel:255)),0,1,2,ANSI_LGREEN"An anonymous user appends to the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,topic->name,subtopic->name,msgno);
                                            else bbs_output_except(player,MIN(topic->accesslevel,((subtopic) ? subtopic->accesslevel:255)),0,1,2,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" appends to the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0),(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,topic->name,subtopic->name,msgno);
				      } else if(val1) bbs_output_except(player,MIN(topic->accesslevel,((subtopic) ? subtopic->accesslevel:255)),0,1,2,ANSI_LGREEN"An anonymous user appends to the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,topic->name,msgno);
                                         else bbs_output_except(player,MIN(topic->accesslevel,((subtopic) ? subtopic->accesslevel:255)),0,1,2,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" appends to the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0),(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,topic->name,msgno);
				   }
                                   if(Owner(player) != message->owner)
                                      writelog(BBS_LOG,1,"APPEND","%s%s(#%d) appended to this message.",bbs_logmsg(message,topic,subtopic,msgno,0),getname(player),player); 
                                   setreturn(OK,COMMAND_SUCC);
				} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that message can't be appended to.");
			     } else if(!((db[player].owner != message->owner) && !can_write_to(player,message->owner,0))) {
                                message->flags           &= ~MESSAGE_ALLOWAPPEND; 
                                if(state) message->flags |=  MESSAGE_ALLOWAPPEND; 
                                if(!in_command) {
                                   substitute(Validchar(message->owner) ? message->owner:player,scratch_return_string,decompress(message->subject),0,ANSI_LYELLOW,NULL,0);
                                   if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                                   if(subtopic) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LWHITE"%d"ANSI_LGREEN") can %s be appended to in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,(state) ? "now":"no-longer",topic->name,subtopic->name);
                                      else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LWHITE"%d"ANSI_LGREEN") can %s be appended to in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,(state) ? "now":"no-longer",topic->name);
				}
                                if(Owner(player) != message->owner)
                                   writelog(BBS_LOG,1,"APPEND","%s%s(#%d) changed whether this message can be appended to %s.",bbs_logmsg(message,topic,subtopic,msgno,0),getname(player),player,(state) ? "ON":"OFF"); 
                                setreturn(OK,COMMAND_SUCC);
			     } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you may only change whether a message you own or a message owned by someone of a lower level than yourself can be appended to.");
                                else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you may only change whether your own messages can be appended to.");
			  } else if(message->flags & MESSAGE_ALLOWAPPEND) {
                             if(!(message->flags & MESSAGE_MODIFY)) {
                                if(editing(player)) return;
                                message->flags |= MESSAGE_MODIFY;
                                sprintf(scratch_return_string,"%d",msgno);
                                edit_initialise(player,(val1) ? 108:103,NULL,(union group_data *) message,alloc_string(scratch_return_string),NULL,(topic->flags & TOPIC_CENSOR)|EDIT_LAST_CENSOR,MIN(topic->accesslevel,((subtopic) ? subtopic->accesslevel:255)));
                                substitute(Validchar(message->owner) ? message->owner:player,scratch_return_string,decompress(message->subject),0,ANSI_LYELLOW,NULL,0);
                                if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                                if(subtopic) output(getdsc(player),player,0,1,0,ANSI_LCYAN"\n%sppending to the message '%s"ANSI_LYELLOW"%s"ANSI_LCYAN"' (Message number "ANSI_LWHITE"%d"ANSI_LCYAN") in the sub-topic '"ANSI_LWHITE"%s"ANSI_LCYAN"' in the topic '"ANSI_LWHITE"%s"ANSI_LCYAN"'...\n",(val1) ? "Anonymously a":"A",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,topic->name,subtopic->name);
                                   else output(getdsc(player),player,0,1,0,ANSI_LCYAN"\n%sppending to the message '%s"ANSI_LYELLOW"%s"ANSI_LCYAN"' (Message number "ANSI_LWHITE"%d"ANSI_LCYAN") in the topic '"ANSI_LWHITE"%s"ANSI_LCYAN"'...\n",(val1) ? "Anonymously a":"A",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,topic->name);
                                output(getdsc(player),player,0,1,0,ANSI_LWHITE"Please enter the text you'd like to append to this message (Pressing "ANSI_LCYAN"RETURN"ANSI_LWHITE" or "ANSI_LCYAN"ENTER"ANSI_LWHITE" after each line.)  Once you're finished, type '"ANSI_LGREEN".view"ANSI_LWHITE"' to view and check your append.  If you're happy with it, type '"ANSI_LGREEN".save"ANSI_LWHITE"' to save and append it, otherwise type '"ANSI_LGREEN".abort = yes"ANSI_LWHITE"'.\n");
                                setreturn(OK,COMMAND_SUCC);
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that message is currently being edited, appended or replied to  -  You can't append to it at the moment.");
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that message can't be appended to.");
		       } else if(Level4(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you may only append to a message you own or a message owned by someone of a lower level than yourself.");
                          else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you may only append to your own messages.");
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a message with that number doesn't exist.");
		 } else {
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.  Please choose another %stopic (Type '"ANSI_LWHITE"%s%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%s%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(subtopic) ? "sub-":"",topic->name,clevels[(int) topic->accesslevel],(subtopic) ? "sub-":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"");
                    if((subtopic && (db[player].data->player.topic_id == subtopic->topic_id) && (db[player].data->player.subtopic_id == topic->topic_id)) || (!subtopic && (db[player].data->player.topic_id == topic->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		 }
	      } else {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)  Please choose another topic (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",subtopic->name,clevels[(int) subtopic->accesslevel],topic->name,(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
                 if(db[player].data->player.topic_id == topic->topic_id) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
	      }
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please choose a topic first by typing '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"' (Type '"ANSI_LYELLOW"%stopics"ANSI_LGREEN"' to see the list of available topics.)",(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from appending to messages on %s BBS.",tcz_full_name);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Morons aren't allowed to append to messages on %s BBS.",tcz_full_name);
}

/* ---->  Delete message  <---- */
void bbs_delete(CONTEXT)
{
     struct bbs_message_data *message,*last = NULL;
     struct descriptor_data *p = getdsc(player);
     struct bbs_topic_data *topic,*subtopic;
     struct bbs_reader_data *reader,*next;
     short  deleted = 0,errorcount = 0;
     short  count,start = -1;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Moron(player)) {
        if(Bbs(player)) {
           if(!in_command) {
              if((topic = lookup_topic(player,NULL,&topic,&subtopic))) {
                 if(!subtopic || can_access_topic(player,subtopic,NULL,1)) {
	            if(can_access_topic(player,topic,subtopic,1)) {
                       arg1 = (char *) parse_grouprange(player,arg1,UNSET,0);
                       if(grp->groupno != UNSET)
                          if((grp->rfrom == UNSET) || (grp->rto == UNSET)) {
                             if(grp->groupno == ALL) {
                                grp->rfrom = FIRST;
                                grp->rto   = LAST;
			     } else grp->rfrom = grp->rto = grp->groupno;
                             grp->groupno = ALL;
			  }

                       union_initgrouprange((union group_data *) topic->messages);
                       if((grp->rangeitems <= 1) || (!Blank(arg2) && !strcasecmp(arg2,"yes"))) {
                          while(union_grouprange())
                                if(!((db[player].owner != grp->cunion->message.owner) && (db[player].owner != topic->owner) && !(subtopic && (db[player].owner == subtopic->owner)) && !can_write_to(player,grp->cunion->message.owner,0))) {
	    	                   if(!(grp->cunion->message.flags & MESSAGE_MODIFY)) {
                                      for(message = topic->messages, last = NULL, count = 0; message && (message != &(grp->cunion->message)); last = message, message = message->next, count++);
                                      if(start < 0) start = count + 1;
                                      if(Owner(player) != grp->cunion->message.owner)
                                         writelog(BBS_LOG,1,"DELETE","%s%s(#%d) deleted this message.",bbs_logmsg(&(grp->cunion->message),topic,subtopic,start + deleted,0),getname(player),player);

                                      substitute(Validchar(message->owner) ? message->owner:player,scratch_return_string,decompress(message->subject),0,ANSI_LYELLOW,NULL,0);
                                      if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                                      if(subtopic) output(p,player,0,1,0,ANSI_LGREEN"Message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN") deleted from the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,start + deleted,topic->name,subtopic->name);
                                         else output(p,player,0,1,0,ANSI_LGREEN"Message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN") deleted from the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,start + deleted,topic->name);
                                      bbs_adjustcurrentmsg(topic->topic_id,(subtopic) ? subtopic->topic_id:0,count,0);

                                      /* ---->  Reader list  <---- */
                                      for(reader = message->readers; reader; reader = next) {
                                          next = reader->next;
                                          FREENULL(reader);
				      }

                                      /* ---->  Message data  <---- */
                                      if(last) last->next = message->next;
                                         else topic->messages = message->next;
                                      FREENULL(message->message);
                                      FREENULL(message->subject);
                                      FREENULL(message->name);
                                      FREENULL(message);
                                      deleted++;
				   } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, that message is currently being edited, appended or replied to  -  It can't be deleted at the moment."), errorcount++;
				} else if(Level3(db[player].owner)) output(p,player,0,1,0,ANSI_LGREEN"Sorry, you may only delete a message you own or a message owned by someone of a lower level than yourself."), errorcount++;
                                   else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you may only delete your own messages."), errorcount++;

                          if(!deleted && !errorcount) {
         	             output(p,player,0,1,0,ANSI_LGREEN"Sorry, either a message with that number doesn't exist, or the range you specified is invalid.");
                             setreturn(ERROR,COMMAND_FAIL);
			  } else setreturn(OK,COMMAND_SUCC);
		       } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you must type '"ANSI_LWHITE"[bbs] delete <RANGE> = yes"ANSI_LGREEN"' to delete two or more BBS messages.");
		    } else {
                       output(p,player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.  Please choose another %stopic (Type '"ANSI_LWHITE"%s%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%s%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(subtopic) ? "sub-":"",topic->name,clevels[(int) topic->accesslevel],(subtopic) ? "sub-":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"");
                       if((subtopic && (db[player].data->player.topic_id == subtopic->topic_id) && (db[player].data->player.subtopic_id == topic->topic_id)) || (!subtopic && (db[player].data->player.topic_id == topic->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		    }
	         } else {
                    output(p,player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)  Please choose another topic (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",subtopic->name,clevels[(int) subtopic->accesslevel],topic->name,(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
                    if(db[player].data->player.topic_id == topic->topic_id) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		 }
	      } else output(p,player,0,1,0,ANSI_LGREEN"Please choose a topic first by typing '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"' (Type '"ANSI_LYELLOW"%stopics"ANSI_LGREEN"' to see the list of available topics.)",(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
	   } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, messages on %s BBS can't be deleted from within a compound command.",tcz_full_name);
	} else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from deleting messages from %s BBS.",tcz_full_name);
     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, Morons aren't allowed to delete messages from %s BBS.",tcz_full_name);
}

/* ---->  Modify/change message (Using editor)  <---- */
void bbs_modify(CONTEXT)
{
     struct bbs_topic_data *topic,*subtopic;
     struct bbs_message_data *message;
     short  msgno,temp;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Moron(player)) {
        if(Bbs(player)) {
           if(!in_command) {
              if((topic = lookup_topic(player,NULL,&topic,&subtopic))) {
                 if(!subtopic || can_access_topic(player,subtopic,NULL,1)) {
 	            if(can_access_topic(player,topic,subtopic,1)) {
	               if((message = lookup_message(player,&topic,&subtopic,params,&message,&msgno,0))) {
                          if(!((db[player].owner != message->owner) && !can_write_to(player,message->owner,0))) {
  	                     if(!bbs_votecount(message,&temp,&temp,&temp,&temp,&temp,&temp) || Level4(db[player].owner) || (db[player].owner == topic->owner)) {
                                if(!(message->flags & MESSAGE_MODIFY)) {
                                   if(editing(player)) return;
                                   message->flags |= MESSAGE_MODIFY;
                                   sprintf(scratch_return_string,"%d",msgno);
                                   edit_initialise(player,102,decompress(message->message),(union group_data *) message,alloc_string(scratch_return_string),NULL,(topic->flags & TOPIC_CENSOR)|EDIT_LAST_CENSOR,MIN(topic->accesslevel,((subtopic) ? subtopic->accesslevel:255)));
                                   substitute(Validchar(message->owner) ? message->owner:player,scratch_return_string,decompress(message->subject),0,ANSI_LYELLOW,NULL,0);
                                   if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                                   if(subtopic) output(getdsc(player),player,0,1,0,ANSI_LCYAN"\nEditing the message '%s"ANSI_LYELLOW"%s"ANSI_LCYAN"' (Message number "ANSI_LWHITE"%d"ANSI_LCYAN") in the sub-topic '"ANSI_LWHITE"%s"ANSI_LCYAN"' in the topic '"ANSI_LWHITE"%s"ANSI_LCYAN"'...\n",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,topic->name,subtopic->name);
                                      else output(getdsc(player),player,0,1,0,ANSI_LCYAN"\nEditing the message '%s"ANSI_LYELLOW"%s"ANSI_LCYAN"' (Message number "ANSI_LWHITE"%d"ANSI_LCYAN") in the topic '"ANSI_LWHITE"%s"ANSI_LCYAN"'...\n",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,topic->name);
                                   output(getdsc(player),player,0,1,0,ANSI_LWHITE"Please make your changes to this message (See '"ANSI_LGREEN".help editor"ANSI_LWHITE"' for details on how to use the editor, and '"ANSI_LGREEN".help editcmd"ANSI_LWHITE"' for a list of editing commands.)  Once you're finished, type '"ANSI_LGREEN".view"ANSI_LWHITE"' to view and check your changes.  If you're happy with them, type '"ANSI_LGREEN".save"ANSI_LWHITE"' to save them, otherwise type '"ANSI_LGREEN".abort = yes"ANSI_LWHITE"'.\n");
                                   setreturn(OK,COMMAND_SUCC); 
				} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that message is currently being edited, appended or replied to  -  You can't modify it at the moment.");
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above or the owner of this topic (%s"ANSI_LYELLOW"%s"ANSI_LGREEN") can make modifications to a message that has been voted on.  Please either append to this message ('"ANSI_LWHITE"append %d"ANSI_LGREEN"') or reset the votes on it ('"ANSI_LWHITE"vote %d = reset"ANSI_LGREEN"'.)",Article(topic->owner,UPPER,INDEFINITE),getcname(NOTHING,topic->owner,0,0),temp,temp);
			  } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you may only make changes to a message you own or a message owned by someone of a lower level than yourself.");
                             else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you may only make changes to your own messages.");
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a message with that number doesn't exist.");
		    } else {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.  Please choose another %stopic (Type '"ANSI_LWHITE"%s%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%s%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(subtopic) ? "sub-":"",topic->name,clevels[(int) topic->accesslevel],(subtopic) ? "sub-":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"");
                       if((subtopic && (db[player].data->player.topic_id == subtopic->topic_id) && (db[player].data->player.subtopic_id == topic->topic_id)) || (!subtopic && (db[player].data->player.topic_id == topic->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		    }
	         } else {
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)  Please choose another topic (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",subtopic->name,clevels[(int) subtopic->accesslevel],topic->name,(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
                    if(db[player].data->player.topic_id == topic->topic_id) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		 }
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please choose a topic first by typing '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"' (Type '"ANSI_LYELLOW"%stopics"ANSI_LGREEN"' to see the list of available topics.)",(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a message on %s BBS can't be modified from within a compound command.",tcz_full_name);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from making modifications to messages on %s BBS.",tcz_full_name);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Morons aren't allowed to make changes to messages on %s BBS.",tcz_full_name);
}

/* ---->  Move message to another topic  <---- */
void bbs_move(CONTEXT)
{
     struct bbs_topic_data *topic,*subtopic,*newtopic,*newsubtopic;
     struct bbs_message_data *ptr,*message,*last = NULL;
     short  temp,count,msgno;
     char   *text;

     setreturn(ERROR,COMMAND_FAIL);
     if(Blank(arg2)) {
        for(; *arg1 && (*arg1 == ' '); arg1++);
        for(text = arg1; *text && (*text != ' '); text++);
        if(*text) for(*text = '\0', text++; *text && (*text == ' '); text++);
        arg2 = text;
     }

     bbs_topicname_filter(arg2);
     if(!in_command) {
        if(!Moron(player)) {
           if(Bbs(player)) {
              if((topic = lookup_topic(player,NULL,&topic,&subtopic))) {
                 if(!subtopic || can_access_topic(player,subtopic,NULL,1)) {
	            if(can_access_topic(player,topic,subtopic,1)) {
	               if((message = lookup_message(player,&topic,&subtopic,arg1,&last,&msgno,0))) {
                          if(!((db[player].owner != message->owner) && (db[player].owner != topic->owner) && !(subtopic && (db[player].owner == subtopic->owner)) && !can_write_to(player,message->owner,1))) {
                             if(Level1(player) || !(message->flags & MESSAGE_REPLY)) {
                                if(Level1(player) || (bbs_replycount(topic,message,message->id,0) < 2)) {
      	                           if(!Blank(arg2)) {
                                      if((newtopic = lookup_topic(player,arg2,&newtopic,&newsubtopic))) {
  		                         if(newtopic != topic) {
                                            if(!newsubtopic || can_access_topic(player,newsubtopic,NULL,1)) {
                        	               if(can_access_topic(player,newtopic,newsubtopic,1)) {
   	                       	                  if(!(!Level4(db[player].owner) && !(newtopic->flags & TOPIC_MORTALADD))) {
                                                     if(newtopic->flags & TOPIC_ADD) {
 	  	                                        if(!(!(newtopic->flags & TOPIC_CYCLIC) && (bbs_messagecount(newtopic->messages,NOTHING,&temp) >= newtopic->messagelimit))) {
                                                           if(newtopic->flags & TOPIC_CYCLIC) bbs_cyclicdelete(newtopic->topic_id,(newsubtopic) ? newsubtopic->topic_id:0);
                                                           substitute(Validchar(message->owner) ? message->owner:player,scratch_return_string,decompress(message->subject),0,ANSI_LYELLOW,NULL,0);
                                                           if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                                                           if(subtopic) sprintf(scratch_buffer,"sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'",topic->name,subtopic->name);
                                                              else sprintf(scratch_buffer,"topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'",topic->name);
                                                           if(newsubtopic) sprintf(scratch_buffer + KB,"sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'",newtopic->name,newsubtopic->name);
                                                              else sprintf(scratch_buffer + KB,"topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'",newtopic->name);
                                                           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' in the %s moved to the %s.",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,scratch_buffer,scratch_buffer + KB);
                                                           bbs_adjustcurrentmsg(topic->topic_id,(subtopic) ? subtopic->topic_id:0,msgno,0);

                                                           if(last) last->next = message->next;
                                                              else topic->messages = message->next;
                                                           for(ptr = newtopic->messages, last = NULL, count = 0; ptr && (ptr->date <= message->date); last = ptr, ptr = ptr->next, count++);
                                                           bbs_adjustcurrentmsg(newtopic->topic_id,(subtopic) ? subtopic->topic_id:0,count,1);

                                                           if(last) last->next = message;
                                                              else newtopic->messages = message;
                                                           if(ptr) message->next = ptr;
                                                              else message->next = NULL;

                                                           if(Owner(player) != message->owner)
                                                              writelog(BBS_LOG,1,"MOVE","%s%s(#%d) moved this message to the %stopic '%s%s%s'.",bbs_logmsg(message,topic,subtopic,msgno,0),getname(player),player,(newsubtopic) ? "sub-":"",(newsubtopic) ? newsubtopic->name:"",(newsubtopic) ? "/":"",newtopic->name); 
                                                           setreturn(OK,COMMAND_SUCC);
							} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, there are too many messages in the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (A maximum of "ANSI_LYELLOW"%d"ANSI_LGREEN" message%s allowed.)",(newsubtopic) ? "sub-":"",newtopic->name,newtopic->messagelimit,(newtopic->messagelimit == 1) ? " is":"s are");
						     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, messages can't be moved into the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",(newsubtopic) ? "sub-":"",newtopic->name);
						  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Mortals may not move messages into the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",(newsubtopic) ? "sub-":"",newtopic->name);
					       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.",(newsubtopic) ? "sub-":"",newtopic->name,clevels[(int) newtopic->accesslevel]);
					    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)",newsubtopic->name,clevels[(int) newsubtopic->accesslevel],newtopic->name);
					 } else {
                                            substitute(Validchar(message->owner) ? message->owner:player,scratch_return_string,decompress(message->subject),0,ANSI_LYELLOW,NULL,0);
                                            if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                                            output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' is already in the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,(newsubtopic) ? "sub-":"",newtopic->name);
					 }
				      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg2);
				   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify which topic/sub-topic you'd like to move this message to.");
				} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a message which has replies can't be moved to another topic/sub-topic.");
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a reply to a message can't be moved to another topic/sub-topic.");
			  } else if(Level4(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you may only move a message you own or a message owned by someone of a lower level than yourself.");
                             else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you may only move your own messages.");
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a message with that number doesn't exist.");
		    } else {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.  Please choose another %stopic (Type '"ANSI_LWHITE"%s%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%s%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(subtopic) ? "sub-":"",topic->name,clevels[(int) topic->accesslevel],(subtopic) ? "sub-":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"");
                       if((subtopic && (db[player].data->player.topic_id == subtopic->topic_id) && (db[player].data->player.subtopic_id == topic->topic_id)) || (!subtopic && (db[player].data->player.topic_id == topic->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		    }
		 } else {
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)  Please choose another topic (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",subtopic->name,clevels[(int) subtopic->accesslevel],topic->name,(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
                    if(db[player].data->player.topic_id == topic->topic_id) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		 }
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please choose a topic first by typing '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"' (Type '"ANSI_LYELLOW"%stopics"ANSI_LGREEN"' to see the list of available topics.)",(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from moving messages on %s BBS from one topic to another.",tcz_full_name);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Morons aren't allowed to move messages from one topic to another on %s BBS.",tcz_full_name);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a message on %s BBS can't be moved from one topic to another from within a compound command.",tcz_full_name);
}

/* ---->  Change subject of message  <---- */
void bbs_subject(CONTEXT)
{
     struct bbs_topic_data *topic,*subtopic;
     struct bbs_message_data *message;
     short  msgno;
     char   *ptr;

     /* ---->  Grab first word as message number  <---- */
     comms_spoken(player,0);
     setreturn(ERROR,COMMAND_FAIL);
     if(Blank(arg2)) {
        for(; *arg1 && (*arg1 == ' '); arg1++);
        for(ptr = arg1; *ptr && (*ptr != ' '); ptr++);
        if(*ptr) for(*ptr = '\0', ptr++; *ptr && (*ptr == ' '); ptr++);
        arg2 = ptr;
     }

     if(!Moron(player)) {
        if(Bbs(player)) {
           if(!in_command) {
              if((topic = lookup_topic(player,NULL,&topic,&subtopic))) {
                 if(!subtopic || can_access_topic(player,subtopic,NULL,1)) {
	            if(can_access_topic(player,topic,subtopic,1)) {
 	               if((message = lookup_message(player,&topic,&subtopic,arg1,&message,&msgno,0))) {
                          if(!((db[player].owner != message->owner) && !can_write_to(player,message->owner,0))) {
	                     if(!(message->flags & MESSAGE_REPLY)) {
	                        if(!Blank(arg2)) {
                                   if(strlen(arg2) <= 50) {
                                      if(!instring("%{",arg2)) {
                                         if(!instring("%h",arg2)) {
                                            ansi_code_filter(arg2,arg2,0);
                                            ptr = punctuate(arg2,2,'\0');
                                            substitute(Validchar(message->owner) ? message->owner:player,scratch_return_string,decompress(message->subject),0,ANSI_LYELLOW,NULL,0);
                                            if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                                            if(subtopic) sprintf(scratch_buffer,ANSI_LGREEN"Subject of message '"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LWHITE"%d"ANSI_LGREEN") in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' changed to '",scratch_return_string,msgno,topic->name,subtopic->name);
                                               else sprintf(scratch_buffer,ANSI_LGREEN"Subject of message '"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LWHITE"%d"ANSI_LGREEN") in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' changed to '",scratch_return_string,msgno,topic->name);
                                            substitute(Validchar(message->owner) ? message->owner:player,scratch_return_string,ptr,0,ANSI_LYELLOW,NULL,0);
                                            if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                                            output(getdsc(player),player,0,1,0,"%s"ANSI_LYELLOW"%s"ANSI_LGREEN"'.",scratch_buffer,scratch_return_string);
                                            if(Owner(player) != message->owner)
                                               writelog(BBS_LOG,1,"SUBJECT","%s%s(#%d) changed the subject of this message to '%s'.",bbs_logmsg(message,topic,subtopic,msgno,0),getname(player),player,ptr); 
                 	  	            FREENULL(message->subject);
                                            message->subject = (char *) alloc_string(compress(ptr,1));
                                            setreturn(OK,COMMAND_SUCC);
					 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the new subject for the message can't contain embedded HTML tags.");
				      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the new subject for the message can't contain query command substitutions ('"ANSI_LWHITE"%%{<QUERY COMMAND>}"ANSI_LGREEN"'.)");
				   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of the new subject for the message is 50 characters.");
				} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify a new subject for the message.");
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't change the subject of a reply to a message.");
			  } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you may only change the subject of a message you own or a message owned by someone of a lower level than yourself.");
                             else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you may only change the subject of your own messages.");
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a message with that number doesn't exist.");
		    } else {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.  Please choose another %stopic (Type '"ANSI_LWHITE"%s%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%s%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(subtopic) ? "sub-":"",topic->name,clevels[(int) topic->accesslevel],(subtopic) ? "sub-":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"");
                       if((subtopic && (db[player].data->player.topic_id == subtopic->topic_id) && (db[player].data->player.subtopic_id == topic->topic_id)) || (!subtopic && (db[player].data->player.topic_id == topic->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		    }
	         } else {
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)  Please choose another topic (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",subtopic->name,clevels[(int) subtopic->accesslevel],topic->name,(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
                    if(db[player].data->player.topic_id == topic->topic_id) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		 }
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please choose a topic first by typing '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"' (Type '"ANSI_LYELLOW"%stopics"ANSI_LGREEN"' to see the list of available topics.)",(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the subject of a message on %s BBS can't be changed from within a compound command.",tcz_full_name);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from changing the subject of messages on %s BBS.",tcz_full_name);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Morons aren't allowed to change the subject of messages on %s BBS.",tcz_full_name);
}


/*  ========================================================================  */
/*    ---->                    Admin BBS commands                    <----    */
/*  ========================================================================  */


/* ---->  Set/display access level required to access specified topic  <---- */
void bbs_accesslevel(CONTEXT)
{
     struct   bbs_topic_data *topic,*subtopic;
     unsigned char modify = 0,display = 0;

     setreturn(ERROR,COMMAND_FAIL);
     if((topic = lookup_topic(player,arg1,&topic,&subtopic))) {
        if(!Blank(arg2)) {
           if(!subtopic || can_access_topic(player,subtopic,NULL,1)) {
   	      if(can_access_topic(player,topic,subtopic,1)) {
                 if(!Moron(player)) {
                    if(Bbs(player)) {
                       if(!in_command) {
                          if(can_write_to(player,topic->owner,0) || (subtopic && can_write_to(player,subtopic->owner,0))) {
                             if(!(string_prefix("supremebeings",arg2) || string_prefix("thesupremebeings",arg2) || string_prefix("supreme beings",arg2) || string_prefix("the supreme beings",arg2) || string_prefix("god",arg2))) {
	                        if(!(string_prefix("deities",arg2) || string_prefix("gods",arg2))) {
                                   if(!(string_prefix("elders",arg2) || string_prefix("elder wizards",arg2) || string_prefix("elderwizards",arg2) || string_prefix("elderdruids",arg2) || string_prefix("elder druids",arg2))) {
                                      if(!(string_prefix("wizards",arg2) || string_prefix("druids",arg2))) {
                                         if(!(string_prefix("apprentices",arg2) || string_prefix("apprentice wizards",arg2) || string_prefix("apprenticewizards",arg2) || string_prefix("apprenticedruids",arg2) || string_prefix("apprentice druids",arg2))) {
                                            if(!(string_prefix("experienced",arg2) || string_prefix("experiencedbuilders",arg2) || string_prefix("experienced builders",arg2))) {
                                               if(!(string_prefix("builders",arg2))) {
                                                  if(!(string_prefix("mortals",arg2))) {
                                                     if(!(string_prefix("none",arg2) || string_prefix("unrestricted",arg2) || string_prefix("morons",arg2))) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LWHITE"morons"ANSI_LGREEN"', '"ANSI_LWHITE"mortals"ANSI_LGREEN"', '"ANSI_LWHITE"builders"ANSI_LGREEN"', '"ANSI_LWHITE"experienced builders"ANSI_LGREEN"', '"ANSI_LWHITE"apprentices"ANSI_LGREEN"', '"ANSI_LWHITE"wizards"ANSI_LGREEN"', '"ANSI_LWHITE"elders"ANSI_LGREEN"', '"ANSI_LWHITE"deities"ANSI_LGREEN"' or '"ANSI_LWHITE"supreme being"ANSI_LGREEN"'.");
                                                        else topic->accesslevel = 8, display = modify = 1;
						  } else topic->accesslevel = 7, display = modify = 1;
					       } else topic->accesslevel = 6, display = modify = 1;
					    } else topic->accesslevel = 5, display = modify = 1;
					 } else topic->accesslevel = 4, display = modify = 1;
				      } else topic->accesslevel = 3, display = modify = 1;
				   } else topic->accesslevel = 2, display = modify = 1;
				} else topic->accesslevel = 1, display = modify = 1;
			     } else topic->accesslevel = 0, display = modify = 1;
			  } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the access level of a topic you own or a %stopic owned by someone who's of a lower level than yourself.",(subtopic) ? "sub-":"");
                             else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the access level of a %stopic you own.",(subtopic) ? "sub-":"");
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the access level of a %stopic on %s BBS can't be changed from within a compound command.",(subtopic) ? "sub-":"",tcz_full_name);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from changing the access level of %stopics on %s BBS.",(subtopic) ? "sub-":"",tcz_full_name);
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Morons aren't allowed to change the access level of a %stopic on %s BBS.",(subtopic) ? "sub-":"",tcz_full_name);
	      } else {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.  Please choose another %stopic (Type '"ANSI_LWHITE"%s%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%s%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(subtopic) ? "sub-":"",topic->name,clevels[(int) topic->accesslevel],(subtopic) ? "sub-":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"");
                 if((subtopic && (db[player].data->player.topic_id == subtopic->topic_id) && (db[player].data->player.subtopic_id == topic->topic_id)) || (!subtopic && (db[player].data->player.topic_id == topic->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
	      }
	   } else {
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)  Please choose another topic (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",subtopic->name,clevels[(int) subtopic->accesslevel],topic->name,(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
              if(db[player].data->player.topic_id == topic->topic_id) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
	   }
	} else display = 1;
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);

     if(display) {
        if(subtopic) sprintf(scratch_return_string," in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'",subtopic->name);
           else *scratch_return_string = '\0';
        if(topic->accesslevel == 8) output(getdsc(player),player,0,1,0,ANSI_LGREEN"The %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"'%s may%s be accessed by anyone.",(subtopic) ? "sub-":"",topic->name,scratch_return_string,(modify) ? " now":"");
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"The %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"'%s may%s only be accessed by %s.",(subtopic) ? "sub-":"",topic->name,scratch_return_string,(modify) ? " now":"",clevels[(int) topic->accesslevel]);
        if(modify) writelog(BBS_LOG,1,"TOPIC","%s%s(#%d) changed the access level of this %stopic to %s.",bbs_logtopic(topic,subtopic),getname(player),player,(subtopic) ? "sub-":"",clevels[(int) topic->accesslevel]);
        setreturn(OK,COMMAND_SUCC);
     }
}

/* ---->  Add new topic  <---- */
void bbs_addtopic(CONTEXT)
{
     struct bbs_topic_data *subtopic = NULL,*subsubtopic;
     char   *end,*start;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Moron(player)) {
        if(Bbs(player)) {
           if(!in_command) {
              if(dumpstatus != 2) {

                 /* ---->  Add new sub-topic?  <---- */
                 if((end = (char *) strchr(arg1,'/')) || (end = (char *) strchr(arg1,'\\')) || (end = (char *) strchr(arg1,';'))) {
                    for(start = arg1, arg1 = end + 1; (end > start) && (*(end - 1) == ' '); *end-- = '\0');
                    *end = '\0';

                    if((subtopic = lookup_topic(player,start,&subtopic,&subsubtopic))) {
          	       if(can_access_topic(player,subtopic,NULL,1)) {
                          if(subsubtopic) {
                             output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only top-level topics may contain sub-topics (I.e:  Sub-topics can't contain further sub-topics.)");
                             return;
			  } else if(!Level3(db[player].owner) && !can_write_to(player,subtopic->owner,0)) {
                             output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only add a sub-topic to a topic you own.");
                             return;
			  }
		       } else {
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.",(subsubtopic) ? "sub-":"",subtopic->name,clevels[(int) subtopic->accesslevel]);
                          if((subsubtopic && (db[player].data->player.topic_id == subsubtopic->topic_id) && (db[player].data->player.subtopic_id == subtopic->topic_id)) || (!subsubtopic && (db[player].data->player.topic_id == subtopic->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
                          return;
		       }
		    } else {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",start);
                       return;
		    }
		 } else if(!Level3(db[player].owner)) {
	            output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Wizards/Druids and above may add a new topic to %s BBS.",tcz_full_name);
                    return;
 		 }

                 /* ---->  Add new topic/sub-topic  <---- */
                 if(bbs_topiccount((subtopic) ? subtopic->subtopics:bbs) <= BBS_MAX_TOPICS) {
                    bad_language_filter(arg1,arg1);
                    filter_spaces(arg1,arg1,0);
                    bbs_topicname_filter(arg1);
                    if(!Blank(arg1)) {
                       if(!Blank(arg2)) {
                          ansi_code_filter(arg1,arg1,0);
                          ansi_code_filter(arg2,arg2,0);
                          arg2 = (char *) punctuate(arg2,3,'.');
                          if(islower(*arg1)) *arg1 = toupper(*arg1);
                          if(!((strlen(arg1) > 13) || strchr(arg1,'\n'))) {
                             if(!instring("%{",arg1)) {
                                if(!instring("%h",arg1)) {
                                   if(!((strlen(arg2) > 140) || strchr(arg2,'\n'))) {
                                      if(!strchr(arg1,'/') && !strchr(arg1,'\\') && !strchr(arg1,';')) {
                                         if(!instring("%{",arg2)) {
                                            if(!instring("%h",arg2)) {
                                               struct   bbs_topic_data *new,*ptr,*last = NULL;
                                               unsigned short loop;
                                               int      result;
 
                                               for(ptr = (subtopic) ? subtopic->subtopics:bbs; ptr && ((result = strcasecmp(arg1,ptr->name)) >= 0); last = ptr, ptr = ptr->next)
                                                   if(!result) {
                                                      output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a %stopic with the name '"ANSI_LYELLOW"%s"ANSI_LGREEN"' already exists.",(subtopic) ? "sub-":"",arg1);
                                                      return;
						   }

                                               MALLOC(new,struct bbs_topic_data);
                                               if(last) last->next = new;
                                                  else if(subtopic) subtopic->subtopics = new;
                                                     else bbs = new;
                                               if(ptr) new->next = ptr;
                                                  else new->next = NULL;
                                               new->subtopiclimit = (subtopic) ? 0:BBS_DEFAULT_MAX_SUBTOPICS;
                                               new->messagelimit  = BBS_DEFAULT_MAX_MESSAGES;
                                               new->accesslevel   = 8;
                                               new->timelimit     = BBS_TOPIC_TIME_LIMIT;
                                               new->subtopics     = NULL;
                                               new->messages      = NULL;
                                               new->topic_id      = 0;
                                               new->owner         = db[player].owner;
                                               new->flags         = TOPIC_ADD|TOPIC_MORTALADD|TOPIC_FORMAT|TOPIC_CENSOR|TOPIC_ANON|TOPIC_CYCLIC;
                                               new->desc          = (char *) alloc_string(compress(arg2,0));
                                               new->name          = (char *) alloc_string(arg1);

                                               /* ---->  Find unique ID for new topic  <---- */
                                               for(loop = 1; loop > 0; loop++) {
                                                   for(ptr = (subtopic) ? subtopic->subtopics:bbs, result = 0; ptr && !result; ptr = ptr->next)
                                                       if(ptr->topic_id == loop) result = 1;
                                                   if(!result) {
                                                      new->topic_id = loop;
                                                      loop = 65535;
						   }
					       }
                                               if(!new->topic_id) {
                                                  writelog(BUG_LOG,1,"BUG","(bbs_addtopic() in bbs.c)  Unable to allocate unique ID to new topic '%s'  -  Topic ID set to 65535.",arg1);
                                                  new->topic_id = 65535;
					       }

                                               if(subtopic) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' added to the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",arg1,subtopic->name);
                                                  else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' added to %s BBS.",arg1,tcz_full_name);
                                               writelog(BBS_LOG,1,"TOPIC","%s%s(#%d) created this new %stopic.",bbs_logtopic(new,subtopic),getname(player),player,(subtopic) ? "sub-":"");
                                               setreturn(OK,COMMAND_SUCC);
					    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic's short description can't contain embedded HTML tags.",(subtopic) ? "sub-":"");
					 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic's short description can't contain query command substitutions ('"ANSI_LWHITE"%%{<QUERY COMMAND>}"ANSI_LGREEN"'.)",(subtopic) ? "sub-":"");
				      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the new %stopic's name mustn't contain '"ANSI_LWHITE"/"ANSI_LGREEN"', '"ANSI_LWHITE"\\"ANSI_LGREEN"' or '"ANSI_LWHITE";"ANSI_LGREEN"' characters.",(subtopic) ? "sub-":"");
				   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of the topic's short description is 140 characters.  It also must not contain embedded NEWLINE's.");
				} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic's name can't contain embedded HTML tags.",(subtopic) ? "sub-":"");
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic's name can't contain query command substitutions ('"ANSI_LWHITE"%%{<QUERY COMMAND>}"ANSI_LGREEN"'.)",(subtopic) ? "sub-":"");
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of the topic's name is 13 characters.  It also must not contain embedded NEWLINE's.");
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify a short description for this new %stopic.",(subtopic) ? "sub-":"");
		    } else if(subtopic) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the name of the new sub-topic you'd like to add to the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",subtopic->name);
                       else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the name of the new topic you'd like to add to %s BBS.",tcz_full_name);
		 } else if(subtopic) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, there are too many sub-topics in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'   -  You can't add a new sub-topic.",subtopic->name);
                    else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, there are too many topics on %s BBS  -  You can't add a new topic.",tcz_full_name);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't add a new topic/sub-topic while %s BBS is being dumped to disk  -  Please wait for a couple of minutes and then try again.",tcz_full_name);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a new topic/sub-topic can't be added to %s BBS from within a compound command.",tcz_full_name);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from adding new topics/sub-topics to %s BBS.",tcz_full_name);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Morons aren't allowed to add new topics/sub-topics to %s BBS.",tcz_full_name);
}

/* ---->  Make copy of message and place in another topic  <---- */
void bbs_copy(CONTEXT)
{
     struct bbs_topic_data *topic,*subtopic,*newtopic,*newsubtopic;
     struct bbs_message_data *ptr,*new,*message,*last = NULL;
     struct bbs_reader_data *tail,*reader,*rnew;
     short  count,msgno;
     char   *text;
     time_t now;

     setreturn(ERROR,COMMAND_FAIL);
     if(Blank(arg2)) {
        for(; *arg1 && (*arg1 == ' '); arg1++);
        for(text = arg1; *text && (*text != ' '); text++);
        if(*text) for(*text = '\0', text++; *text && (*text == ' '); text++);
        arg2 = text;
     }

     bbs_topicname_filter(arg2);
     if(!Moron(player)) {
        if(Bbs(player)) {
           if(!in_command) {
              if(Level4(db[player].owner)) {
                 if((topic = lookup_topic(player,NULL,&topic,&subtopic))) {
                    if(!subtopic || can_access_topic(player,subtopic,NULL,1)) {
		       if(can_access_topic(player,topic,subtopic,1)) {
	                  if((message = lookup_message(player,&topic,&subtopic,arg1,&last,&msgno,0))) {
                             if(!((db[player].owner != message->owner) && (db[player].owner != topic->owner) && !can_write_to(player,message->owner,1))) {
                                if(!(message->flags & MESSAGE_REPLY)) {
		                   if(!Blank(arg2)) {
	   	                      if((newtopic = lookup_topic(player,arg2,&newtopic,&newsubtopic))) {
			                 if(newtopic != topic) {
                                            if(!newsubtopic || can_access_topic(player,newsubtopic,NULL,1)) {
   		                               if(can_access_topic(player,newtopic,newsubtopic,1)) {
			                          if(!(!(newtopic->flags & TOPIC_CYCLIC) && (bbs_messagecount(newtopic->messages,player,&count) >= newtopic->messagelimit))) {
                                                     if(newtopic->flags & TOPIC_CYCLIC) bbs_cyclicdelete(newtopic->topic_id,(newsubtopic) ? newsubtopic->topic_id:0);
                                                     substitute(Validchar(message->owner) ? message->owner:player,scratch_return_string,decompress(message->subject),0,ANSI_LYELLOW,NULL,0);
                                                     if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                                                     if(subtopic) sprintf(scratch_buffer,"sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'",topic->name,subtopic->name);
                                                        else sprintf(scratch_buffer,"topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'",topic->name);
                                                     if(newsubtopic) sprintf(scratch_buffer + KB,"sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'",newtopic->name,newsubtopic->name);
                                                        else sprintf(scratch_buffer + KB,"topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'",newtopic->name);
                                                     output(getdsc(player),player,0,1,0,ANSI_LGREEN"Message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LWHITE"%d"ANSI_LGREEN") in the %s copied to the %s.",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,scratch_buffer,scratch_buffer + KB);

                                                     /* ---->  Message data  <---- */
                                                     gettime(now);
                                                     MALLOC(new,struct bbs_message_data);
                                                     new->readercount   = 0;
                                                     new->lastread      = now;
                                                     new->message       = (char *) alloc_string(message->message);
                                                     new->readers       = NULL;
                                                     new->subject       = (char *) alloc_string(message->subject);
                                                     new->expiry        = message->expiry;
                                                     new->flags         = (message->flags & ~MESSAGE_MODIFY);
                                                     new->owner         = player;
                                                     new->date          = message->date;
                                                     new->name          = (char *) alloc_string(message->name);

                                                     /* ---->  Readers list  <---- */
                                                     bbs_update_readers(message,NOTHING,0,0);
                                                     for(tail = NULL, reader = message->readers; reader; reader = reader->next)
                                                         if(reader->flags & READER_VOTE_MASK) {
                                                            MALLOC(rnew,struct bbs_reader_data);
                                                            rnew->reader = reader->reader;
                                                            rnew->flags  = (reader->flags & ~READER_READ);
                                                            rnew->next   = NULL;

                                                            if(tail) {
                                                               tail->next = rnew;
                                                               tail       = rnew;
							    } else new->readers = tail = rnew;
							 }

                                                     for(ptr = newtopic->messages, last = NULL, count = 0; ptr && (ptr->date <= message->date); last = ptr, ptr = ptr->next, count++);
                                                     bbs_adjustcurrentmsg(topic->topic_id,(subtopic) ? subtopic->topic_id:0,count,1);
                                                     if(last) last->next = new;
                                                        else newtopic->messages = new;
                                                     if(ptr) new->next = ptr;
                                                        else new->next = NULL;

                                                     if(Owner(player) != message->owner)
                                                        writelog(BBS_LOG,1,"COPY","%s%s(#%d) copied this message to the %stopic '%s%s%s'.",bbs_logmsg(message,topic,subtopic,msgno,0),getname(player),player,(newsubtopic) ? "sub-":"",(newsubtopic) ? newsubtopic->name:"",(newsubtopic) ? "/":"",newtopic->name); 
                                                     setreturn(OK,COMMAND_SUCC);
						  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, there are too many messages in the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (A maximum of "ANSI_LYELLOW"%d"ANSI_LGREEN" message%s allowed.)",(newsubtopic) ? "sub-":"",newtopic->name,newtopic->messagelimit,(newtopic->messagelimit == 1) ? " is":"s are");
					       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.",(newsubtopic) ? "sub-":"",newtopic->name,clevels[(int) newtopic->accesslevel]);
					    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)",newsubtopic->name,clevels[(int) newsubtopic->accesslevel],newtopic->name);
					 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a message may only be copied to another topic/sub-topic (I.e:  Any topic or sub-topic other than the topic/sub-topic which the message being copied is in.)"); 
				      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg2);
				   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify which topic you'd like to place the copy of this message in.");
				} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't make a copy of a reply to a message.");
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you may only make a copy of a message you own or a message owned by someone of a lower level than yourself.");
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a message with that number doesn't exist.");
		       } else {
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.  Please choose another %stopic (Type '"ANSI_LWHITE"%s%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%s%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(subtopic) ? "sub-":"",topic->name,clevels[(int) topic->accesslevel],(subtopic) ? "sub-":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"");
                          if((subtopic && (db[player].data->player.topic_id == subtopic->topic_id) && (db[player].data->player.subtopic_id == topic->topic_id)) || (!subtopic && (db[player].data->player.topic_id == topic->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		       }
		    } else {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)  Please choose another topic (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",subtopic->name,clevels[(int) subtopic->accesslevel],topic->name,(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
                       if(db[player].data->player.topic_id == topic->topic_id) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		    }
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please choose a topic first by typing '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"' (Type '"ANSI_LYELLOW"%stopics"ANSI_LGREEN"' to see the list of available topics.)",(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may make a copy of a message on %s BBS.",tcz_full_name);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a copy of a message on %s BBS can't be made from within a compound command.",tcz_full_name);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from making copies of messages on %s BBS.",tcz_full_name);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Morons aren't allowed to make copies of messages on %s BBS.",tcz_full_name);
}

/* ---->  Change topic's short description  <---- */
void bbs_desctopic(CONTEXT)
{
     struct bbs_topic_data *topic,*subtopic;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Moron(player)) {
        if(Bbs(player)) {
           if(!in_command) {
              if((topic = lookup_topic(player,arg1,&topic,&subtopic))) {
                 if(!subtopic || can_access_topic(player,subtopic,NULL,1)) {
        	    if(can_access_topic(player,topic,subtopic,1)) {
	               if(can_write_to(player,topic->owner,0) || (subtopic && can_write_to(player,subtopic->owner,0))) {
	                  if(!Blank(arg2)) {
                             if(!instring("%{",arg2)) {
                                if(!instring("%h",arg2)) {
                                   if(!((strlen((arg2 = (char *) punctuate(arg2,3,'.'))) > 140) || strchr(arg2,'\n'))) {
                                      ansi_code_filter(arg2,arg2,0);
                                      substitute(Validchar(topic->owner) ? topic->owner:player,scratch_return_string,arg2,0,ANSI_LWHITE,NULL,0);
                                      if(subtopic) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Short description of sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' changed to '"ANSI_LWHITE"%s"ANSI_LGREEN"'",topic->name,subtopic->name,scratch_return_string);
                                         else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Short description of topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' changed to '"ANSI_LWHITE"%s"ANSI_LGREEN"'",topic->name,scratch_return_string);
                                      FREENULL(topic->desc);
                                      topic->desc = (char *) alloc_string(compress(arg2,0));
                                      writelog(BBS_LOG,1,"TOPIC","%s%s(#%d) changed the short description of this %stopic to '%s'.",bbs_logtopic(topic,subtopic),getname(player),player,(subtopic) ? "sub-":"",arg2);
                                      setreturn(OK,COMMAND_SUCC);
				   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of the %stopic's new short description is 140 characters.  It also must not contain embedded NEWLINE's.",(subtopic) ? "sub-":"");
				} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic's new short description can't contain embedded HTML tags.",(subtopic) ? "sub-":"");
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic's new short description can't contain query command substitutions ('"ANSI_LWHITE"%%{<QUERY COMMAND>}"ANSI_LGREEN"'.)",(subtopic) ? "sub-":"");
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify a new short description for this %stopic.",(subtopic) ? "sub-":"");
		       } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the short description of a %stopic you own or a topic owned by someone who's of a lower level than yourself.",(subtopic) ? "sub-":"");
                          else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the short description of a %stopic you own.",(subtopic) ? "sub-":"");
		    } else {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.  Please choose another %stopic (Type '"ANSI_LWHITE"%s%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%s%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(subtopic) ? "sub-":"",topic->name,clevels[(int) topic->accesslevel],(subtopic) ? "sub-":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"");
                       if((subtopic && (db[player].data->player.topic_id == subtopic->topic_id) && (db[player].data->player.subtopic_id == topic->topic_id)) || (!subtopic && (db[player].data->player.topic_id == topic->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		    }
		 } else {
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)  Please choose another topic (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",subtopic->name,clevels[(int) subtopic->accesslevel],topic->name,(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
                    if(db[player].data->player.topic_id == topic->topic_id) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		 }
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the short description of a topic on %s BBS can't be changed from within a compound command.",tcz_full_name);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from changing the short description of topics on %s BBS.",tcz_full_name);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Morons aren't allowed to change the short description of topics on %s BBS.",tcz_full_name);
}

/* ---->  Set/display specified topic's message limit (Maximum number of messages allowed in topic at any one time)  <---- */
void bbs_messagelimit(CONTEXT)
{
     struct   bbs_topic_data *topic,*subtopic;
     unsigned char modify = 0,display = 0;
     int      newlimit;

     setreturn(ERROR,COMMAND_FAIL);
     if(Bbs(player)) {
        if(!Moron(player)) {
           if(!in_command) {
              if((topic = lookup_topic(player,arg1,&topic,&subtopic))) {
                 if(!Blank(arg2)) {
                    if(!subtopic || can_access_topic(player,subtopic,NULL,1)) {
                       if(can_access_topic(player,topic,subtopic,1)) {
                          if(can_write_to(player,topic->owner,0)) {
                             newlimit = atol(arg2);
                             if(!(!Level4(db[player].owner) && (newlimit > BBS_MAX_MESSAGES_MORTAL))) {
	                        if(!(!Level2(db[player].owner) && (newlimit > BBS_MAX_MESSAGES_ADMIN))) {
		                   if(newlimit <= BBS_MAX_MESSAGES) {
                                      topic->messagelimit = newlimit;
                                      display = modify = 1;
				   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum message limit for a %stopic is "ANSI_LWHITE"%d"ANSI_LGREEN".",(subtopic) ? "sub-":"",BBS_MAX_MESSAGES);
				} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Elder Wizards/Druids and above can only set a %stopic's message limit above "ANSI_LWHITE"%d"ANSI_LGREEN".",(subtopic) ? "sub-":"",BBS_MAX_MESSAGES_ADMIN);
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above can only set a %stopic's message limit above "ANSI_LWHITE"%d"ANSI_LGREEN".",(subtopic) ? "sub-":"",BBS_MAX_MESSAGES_MORTAL);
			  } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the message limit of a %stopic you own or a topic owned by someone who's of a lower level than yourself.",(subtopic) ? "sub-":"");
                             else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the message limit of a %stopic you own.",(subtopic) ? "sub-":"");
		       } else {
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.  Please choose another %stopic (Type '"ANSI_LWHITE"%s%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%s%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(subtopic) ? "sub-":"",topic->name,clevels[(int) topic->accesslevel],(subtopic) ? "sub-":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"");
                          if((subtopic && (db[player].data->player.topic_id == subtopic->topic_id) && (db[player].data->player.subtopic_id == topic->topic_id)) || (!subtopic && (db[player].data->player.topic_id == topic->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		       }
		    } else {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)  Please choose another topic (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",subtopic->name,clevels[(int) subtopic->accesslevel],topic->name,(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
                       if(db[player].data->player.topic_id == topic->topic_id) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		    }
		 } else display = 1;
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
   	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the message limit of a topic on %s BBS can't be changed from within a compound command.",tcz_full_name);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Morons aren't allowed to change the message limit of a topic on %s BBS.",tcz_full_name);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from changing the message limit of topics on %s BBS.",tcz_full_name);

     if(display) {
        if(subtopic) output(getdsc(player),player,0,1,0,ANSI_LGREEN"A maximum of "ANSI_LYELLOW"%d"ANSI_LGREEN" message%s %s%s allowed in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' at any one time.",topic->messagelimit,Plural(topic->messagelimit),(topic->messagelimit == 1) ? "is":"are",(modify) ? " now":"",topic->name,subtopic->name);
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"A maximum of "ANSI_LYELLOW"%d"ANSI_LGREEN" message%s %s%s allowed in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' at any one time.",topic->messagelimit,Plural(topic->messagelimit),(topic->messagelimit == 1) ? "is":"are",(modify) ? " now":"",topic->name);
        if(modify) writelog(BBS_LOG,1,"TOPIC","%s%s(#%d) changed the message limit of this %stopic to %d.",bbs_logtopic(topic,subtopic),getname(player),player,(subtopic) ? "sub-":"",topic->messagelimit);
        setreturn(OK,COMMAND_SUCC);
     }
}

/* ---->  Change owner of message  <---- */
void bbs_owner(CONTEXT)
{
     struct bbs_topic_data *topic,*subtopic;
     struct bbs_message_data *message;
     dbref  newowner;
     short  temp;
     char   *ptr;

     /* ---->  Grab first word as message number  <---- */
     comms_spoken(player,0);
     setreturn(ERROR,COMMAND_FAIL);
     if(Blank(arg2)) {
        for(; *arg1 && (*arg1 == ' '); arg1++);
        for(ptr = arg1; *ptr && (*ptr != ' '); ptr++);
        if(*ptr) for(*ptr = '\0', ptr++; *ptr && (*ptr == ' '); ptr++);
        arg2 = ptr;
     }

     if(!Moron(player)) {
        if(Bbs(player)) {
           if(!in_command) {
              if((topic = lookup_topic(player,NULL,&topic,&subtopic))) {
                 if(!subtopic || can_access_topic(player,subtopic,NULL,1)) {
	            if(can_access_topic(player,topic,subtopic,1)) {
   	               if((message = lookup_message(player,&topic,&subtopic,arg1,&message,&temp,0))) {
                          if(!((db[player].owner != message->owner) && !can_write_to(player,message->owner,0))) {
	                     if((newowner = lookup_character(player,arg2,1)) != NOTHING) {
		                if(can_write_to(player,newowner,0)) {
                                   ptr = punctuate(arg2,2,'\0');
                                   substitute(Validchar(message->owner) ? message->owner:player,scratch_return_string,decompress(message->subject),0,ANSI_LYELLOW,NULL,0);
                                   if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                                   if(subtopic) sprintf(scratch_buffer,ANSI_LGREEN"Owner of message '"ANSI_LYELLOW"%s"ANSI_LGREEN"' in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' changed to ",scratch_return_string,topic->name,subtopic->name);
                                      else sprintf(scratch_buffer,ANSI_LGREEN"Owner of message '"ANSI_LYELLOW"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' changed to ",scratch_return_string,topic->name);
                                   output(getdsc(player),player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(newowner,LOWER,DEFINITE),getcname(NOTHING,newowner,0,0));
                                   writelog(BBS_LOG,1,"OWNER","%s%s(#%d) changed the owner of this message to %s(#%d).",bbs_logmsg(message,topic,subtopic,temp,0),getname(player),player,getname(newowner),newowner); 
                                   message->owner = newowner;
                                   setreturn(OK,COMMAND_SUCC);
				} else if(Level3(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the owner of a message to yourself or someone who's of a lower level than you.");
                                   else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the owner of a message to yourself.");
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg2);
			  } else if(Level3(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you may only change the owner of a message you own or a message owned by someone of a lower level than yourself.");
                             else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you may only change the owner of a message you own.");
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a message with that number doesn't exist.");
		    } else {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.  Please choose another %stopic (Type '"ANSI_LWHITE"%s%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%s%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(subtopic) ? "sub-":"",topic->name,clevels[(int) topic->accesslevel],(subtopic) ? "sub-":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"");
                       if((subtopic && (db[player].data->player.topic_id == subtopic->topic_id) && (db[player].data->player.subtopic_id == topic->topic_id)) || (!subtopic && (db[player].data->player.topic_id == topic->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		    }
	         } else {
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)  Please choose another topic (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",subtopic->name,clevels[(int) subtopic->accesslevel],topic->name,(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
                    if(db[player].data->player.topic_id == topic->topic_id) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		 }
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please choose a topic first by typing '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"' (Type '"ANSI_LYELLOW"%stopics"ANSI_LGREEN"' to see the list of available topics.)",(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the owner of a message on %s BBS can't be changed from within a compound command.",tcz_full_name);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from changing the owner of messages on %s BBS.",tcz_full_name);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Morons aren't allowed to change the owner of messages on %s BBS.",tcz_full_name);
}

/* ---->  Change topic's owner  <---- */
void bbs_ownertopic(CONTEXT)
{
     struct bbs_topic_data *topic,*subtopic;
     dbref  newowner;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Moron(player)) {
        if(Bbs(player)) {
           if(!in_command) {
              if((topic = lookup_topic(player,arg1,&topic,&subtopic))) {
                 if(!subtopic || can_access_topic(player,subtopic,NULL,1)) {
        	    if(can_access_topic(player,topic,subtopic,1)) {
	               if(can_write_to(player,topic->owner,0)) {
     	                  if((newowner = lookup_character(player,arg2,1)) != NOTHING) {
	                     if(can_write_to(player,newowner,0)) {
                                if(subtopic) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Owner of sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' changed to %s"ANSI_LYELLOW"%s"ANSI_LGREEN".",topic->name,subtopic->name,Article(newowner,LOWER,DEFINITE),getcname(NOTHING,newowner,0,0));
                                   else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Owner of topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' changed to %s"ANSI_LYELLOW"%s"ANSI_LGREEN".",topic->name,Article(newowner,LOWER,DEFINITE),getcname(NOTHING,newowner,0,0));
                                writelog(BBS_LOG,1,"TOPIC","%s%s(#%d) changed the owner of this %stopic from %s(#%d) to %s(#%d).",bbs_logtopic(topic,subtopic),getname(player),player,(subtopic) ? "sub-":"",getname(topic->owner),topic->owner,getname(newowner),newowner);
                                topic->owner = newowner;
                                setreturn(OK,COMMAND_SUCC);
			     } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the owner of a %stopic to yourself or someone who's of a lower level than yourself.",(subtopic) ? "sub-":"");
                                else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the owner of a %stopic to yourself.",(subtopic) ? "sub-":"");
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg2);
		       } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the owner of a %stopic you own or a topic owned by someone who's of a lower level than yourself.",(subtopic) ? "sub-":"");
                          else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the owner of a %stopic you own.",(subtopic) ? "sub-":"");
		    } else {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.  Please choose another %stopic (Type '"ANSI_LWHITE"%s%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%s%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(subtopic) ? "sub-":"",topic->name,clevels[(int) topic->accesslevel],(subtopic) ? "sub-":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"");
                       if((subtopic && (db[player].data->player.topic_id == subtopic->topic_id) && (db[player].data->player.subtopic_id == topic->topic_id)) || (!subtopic && (db[player].data->player.topic_id == topic->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		    }
	         } else {
                    output(getdsc(player),player,player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)  Please choose another topic (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",subtopic->name,clevels[(int) subtopic->accesslevel],topic->name,(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
                    if(db[player].data->player.topic_id == topic->topic_id) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		 }
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the owner of a topic on %s BBS can't be changed from within a compound command.",tcz_full_name);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from changing the owner of topics on %s BBS.",tcz_full_name);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Morons aren't allowed to change the owner of topics on %s BBS.",tcz_full_name);
}

/* ---->  List readers of message  <---- */
void bbs_readers(CONTEXT)
{
     short  msgno,count,vfor,vagainst,vabstain;
     struct descriptor_data *p = getdsc(player);
     struct bbs_topic_data *topic,*subtopic;
     struct bbs_message_data *message;
     struct bbs_reader_data *reader;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Moron(player)) {
        if((topic = lookup_topic(player,NULL,&topic,&subtopic))) {
           if(!subtopic || can_access_topic(player,subtopic,NULL,1)) {
              if(can_access_topic(player,topic,subtopic,1)) {
  	         if((message = lookup_message(player,&topic,&subtopic,arg1,&message,&msgno,0))) {
                    unsigned char twidth = output_terminal_width(player);

#ifndef BBS_LIST_VOTES
                    if(!Level4(db[player].owner)) {
                       output(p,player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may list the readers of a message.");
                       return;
		    } else if(bbs_votecount(message,&vfor,&vagainst,&vabstain,&count,&count,&count) && (vfor + vagainst + vabstain)) {
                       if(!Level1(db[player].owner)) {
                          output(p,player,0,1,0,ANSI_LGREEN"Sorry, only Deities and the Supreme Being may list the readers of a message which has been voted on.");
                          return;
		       } else if(!can_write_to(player,message->owner,0)) {
                          output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only list the readers of your own messages or those owned by lower level characters.");
                          return;
		       }
		    } else if((db[player].owner != message->owner) && (db[player].owner != topic->owner) && !(subtopic && (db[player].owner == subtopic->owner)) && !can_write_to(player,message->owner,1)) {
                       output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only list the readers of your own messages or those owned by lower level characters.");
                       return;
		    }
#else
                    if((db[player].owner != message->owner) && (db[player].owner != topic->owner) && !(subtopic && (db[player].owner == subtopic->owner)) && !can_write_to(player,message->owner,1)) {
                       if(Level4(db[player].owner)) output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only list the readers of your own messages or those owned by lower level characters.");
                          else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only list the readers of your own messages.");
                       return;
		    }
#endif

                    /* ---->  Initialisation  <---- */
                    html_anti_reverse(p,1);
                    if(IsHtml(p)) twidth = 89;
                    parse_grouprange(player,arg2,FIRST,1);
                    set_conditions_ps(player,0,0,0,0,0,0,message->owner,NULL,513);
                    if(!in_command && p && !p->pager && !IsHtml(p) && More(player)) pager_init(p);
                    substitute(Validchar(message->owner) ? message->owner:player,scratch_return_string,decompress(message->subject),0,ANSI_LYELLOW,NULL,0);
                    if(topic->flags & TOPIC_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                    union_initgrouprange((union group_data *) message->readers);

                    /* ---->  Header  <---- */
                    if(IsHtml(p)) {
                       output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_GREY">",(in_command) ? "":"<BR>");
                       if(!in_command) {
                          if(subtopic) output(p,player,2,1,0,"\016<TR ALIGN=CENTER><TH BGCOLOR="HTML_TABLE_CYAN"><FONT SIZE=5 COLOR="HTML_LCYAN"><I>\016Readers of the message '%s"ANSI_LYELLOW"%s"ANSI_LCYAN"' (Message number "ANSI_LWHITE"%d"ANSI_LCYAN") in the sub-topic '"ANSI_LWHITE"%s"ANSI_LCYAN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'...\016</I></FONT></TH></TR>\016",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,topic->name,subtopic->name);
                             else output(p,player,2,1,0,"\016<TR ALIGN=CENTER><TH BGCOLOR="HTML_TABLE_CYAN"><FONT SIZE=5 COLOR="HTML_LCYAN"><I>\016Readers of the message '%s"ANSI_LYELLOW"%s"ANSI_LCYAN"' (Message number "ANSI_LWHITE"%d"ANSI_LCYAN") in the topic '"ANSI_LWHITE"%s"ANSI_LCYAN"'...\016</I></FONT></TH></TR>\016",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,topic->name);
		       }
		    } else if(!in_command) {
                       if(subtopic) output(p,player,0,1,1,"\n Readers of the message '%s"ANSI_LYELLOW"%s"ANSI_LCYAN"' (Message number "ANSI_LWHITE"%d"ANSI_LCYAN") in the sub-topic '"ANSI_LWHITE"%s"ANSI_LCYAN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'...",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,topic->name,subtopic->name);
                          else output(p,player,0,1,1,"\n Readers of the message '%s"ANSI_LYELLOW"%s"ANSI_LCYAN"' (Message number "ANSI_LWHITE"%d"ANSI_LCYAN") in the topic '"ANSI_LWHITE"%s"ANSI_LCYAN"'...",(message->flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,msgno,topic->name);
                       output(p,player,0,1,0,separator(twidth,0,'-','='));
		    }

                    bbs_update_readers(message,NOTHING,0,0);
                    if(!(message->flags & MESSAGE_EVERYONE)) {

                       /* ---->  Count readers  <---- */
                       for(count = 0, reader = message->readers; reader; reader = reader->next)
                           if((reader->flags & READER_READ) && !(reader->flags & READER_IGNORE) && (reader->reader != message->owner))
                              count++;
		       count += message->readercount;

                       /* ---->  Readers list  <---- */
      	               output_columns(p,player,NULL,NULL,twidth,1,20,2,0,1,FIRST,5,"***  NO READERS FOUND  ***",scratch_return_string);
                       while(union_grouprange())
 	                     output_columns(p,player,getname_prefix(grp->cunion->reader.reader,20,scratch_buffer),privilege_colour(grp->cunion->reader.reader),0,0,0,0,0,0,DEFAULT,0,NULL,scratch_return_string);
         	       output_columns(p,player,NULL,NULL,0,0,0,0,0,0,LAST,0,NULL,scratch_return_string);
		    } else {
                       output(p,player,2,1,1,"%sThis message has been marked as being read by everyone (Either the limit of "ANSI_LWHITE"%d"ANSI_LGREEN" reader%s has been exceeded, or the message has been on %s BBS for over "ANSI_LYELLOW"%s"ANSI_LGREEN".)%s",IsHtml(p) ? "\016<TR><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_BLACK">"ANSI_LGREEN"\016":ANSI_LGREEN" ",BBS_MAX_READERS,Plural(BBS_MAX_READERS),tcz_full_name,interval(BBS_READERS_EXPIRY * DAY,0,ENTITIES,0),IsHtml(p) ? "\016</TD></TR>\016":"\n");
		       count = message->readercount;
		    }

                    /* ---->  Footer  <---- */
                    if(!in_command) {
                       if(!IsHtml(p)) output(p,player,2,1,0,separator(twidth,1,'-','='));
     	               if(message->readercount && !(message->flags & MESSAGE_EVERYONE))
                          sprintf(scratch_buffer + strlen(scratch_buffer)," \016&nbsp;\016 "ANSI_DCYAN"("ANSI_LWHITE"%d"ANSI_LMAGENTA" "ANSI_LCYAN"reader%s no-longer exist%s."ANSI_DCYAN")",message->readercount,Plural(message->readercount),(message->readercount == 1) ? "s":"");
                             else *scratch_buffer = '\0';

                       if(grp->totalitems > 0)
                          output(p,player,2,1,1,"%sReaders: %s "ANSI_DWHITE"%s%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TD BGCOLOR="HTML_TABLE_MGREY">"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",IsHtml(p) ? "\016&nbsp;\016":"",listed_items(scratch_return_string,1),scratch_buffer,IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
		             else output(p,player,2,1,1,"%sReaders: %s "ANSI_DWHITE"None.%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TD BGCOLOR="HTML_TABLE_MGREY">"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",IsHtml(p) ? "\016&nbsp;\016":"",scratch_buffer,IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
		    }
                    if(IsHtml(p)) output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
                    html_anti_reverse(p,0);
                    setreturn(OK,COMMAND_SUCC);
		 } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, a message with that number doesn't exist.");
	      } else {
                 output(p,player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.  Please choose another %stopic (Type '"ANSI_LWHITE"%s%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%s%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(subtopic) ? "sub-":"",topic->name,clevels[(int) topic->accesslevel],(subtopic) ? "sub-":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"");
                 if((subtopic && (db[player].data->player.topic_id == subtopic->topic_id) && (db[player].data->player.subtopic_id == topic->topic_id)) || (!subtopic && (db[player].data->player.topic_id == topic->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
	      }
	   } else {
              output(p,player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)  Please choose another topic (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",subtopic->name,clevels[(int) subtopic->accesslevel],topic->name,(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
              if(db[player].data->player.topic_id == topic->topic_id) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
	   }
	} else output(p,player,0,1,0,ANSI_LGREEN"Please choose a topic first by typing '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"' (Type '"ANSI_LYELLOW"%stopics"ANSI_LGREEN"' to see the list of available topics.)",(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, Morons aren't allowed to list the readers of messages on %s BBS.",tcz_full_name);
}

/* ---->  Remove existing topic  <---- */
void bbs_removetopic(CONTEXT)
{
     struct bbs_topic_data *subtopic,*topic,*last;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Moron(player)) {
        if(Bbs(player)) {
           if(!in_command) {
              if(dumpstatus != 2) {
                 if((topic = lookup_topic(player,params,&last,&subtopic))) {
                    if(!subtopic || can_access_topic(player,subtopic,NULL,1)) {
                       if(can_access_topic(player,topic,subtopic,1)) {
		          if(can_write_to(player,topic->owner,0) || (subtopic && can_write_to(player,subtopic->owner,0))) {
	                     if(!topic->messages) {
	                        if(!topic->subtopics) {
                                   dbref ptr;

                                   /* ---->  Reset TOPIC_ID of characters with topic being removed selected as their current topic  <---- */
                                   writelog(BBS_LOG,1,"TOPIC","%s%s(#%d) deleted this %stopic.",bbs_logtopic(topic,subtopic),getname(player),player,(subtopic) ? "sub-":"");
                                   for(ptr = 0; ptr < db_top; ptr++)
                                       if((Typeof(ptr) == TYPE_CHARACTER) && (db[ptr].data->player.topic_id == topic->topic_id))
                                          db[ptr].data->player.topic_id = 0;

                                   /* ---->  Remove topic from BBS  <---- */
                                   if(subtopic) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' removed from the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",topic->name,subtopic->name);
                                      else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' removed from %s BBS.",topic->name,tcz_full_name);
                                   if(last) last->next = topic->next;
                                      else if(subtopic) subtopic->subtopics = topic->next;
                                         else bbs = topic->next;

                                   FREENULL(topic->desc);
                                   FREENULL(topic->name);
                                   FREENULL(topic);
                                   setreturn(OK,COMMAND_SUCC);
				} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that %stopic still contains sub-topics  -  Please remove them first before trying again.",(subtopic) ? "sub-":"");
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that %stopic still contains messages  -  Please remove them first before trying again.",(subtopic) ? "sub-":"");
			  } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only remove a %stopic you own or a topic owned by someone who's of a lower level than yourself.",(subtopic) ? "sub-":"");
                             else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only remove a %stopic you own.",(subtopic) ? "sub-":"");
		       } else {
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.  Please choose another %stopic (Type '"ANSI_LWHITE"%s%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%s%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(subtopic) ? "sub-":"",topic->name,clevels[(int) topic->accesslevel],(subtopic) ? "sub-":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"");
                          if((subtopic && (db[player].data->player.topic_id == subtopic->topic_id) && (db[player].data->player.subtopic_id == topic->topic_id)) || (!subtopic && (db[player].data->player.topic_id == topic->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		       }
		    } else {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)  Please choose another topic (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",subtopic->name,clevels[(int) subtopic->accesslevel],topic->name,(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
                       if(db[player].data->player.topic_id == topic->topic_id) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		    }
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",params);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't remove a topic while %s BBS is being dumped to disk  -  Please wait for a couple of minutes and then try again.",tcz_full_name);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a topic can't be removed from %s BBS from within a compound command.",tcz_full_name);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from removing topics from %s BBS.",tcz_full_name);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Morons aren't allowed to remove topics from %s BBS.",tcz_full_name);
}

/* ---->  Rename topic  <---- */
void bbs_renametopic(CONTEXT)
{
     struct bbs_topic_data *subtopic,*topic,*last;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Moron(player)) {
        if(Bbs(player)) {
           if(!in_command) {
              if(dumpstatus != 2) {
                 if((topic = lookup_topic(player,arg1,&last,&subtopic))) {
                    if(!subtopic || can_access_topic(player,subtopic,NULL,1)) {
              	       if(can_access_topic(player,topic,subtopic,1)) {
		          if(can_write_to(player,topic->owner,0) || (subtopic && can_write_to(player,subtopic->owner,0))) {
                             bad_language_filter(arg2,arg2);
                             filter_spaces(arg2,arg2,0);
                             bbs_topicname_filter(arg2);
                             if(islower(*arg2)) *arg2 = toupper(*arg2);
                             if(!Blank(arg2)) {
                                if(!((strlen(arg2) > 13) || strchr(arg2,'\n'))) {
                                   if(!strchr(arg2,'/') && !strchr(arg2,'\\') && !strchr(arg2,';')) {
                                      if(!instring("%{",arg2)) {
                                         if(!instring("%h",arg2)) {
                                            struct bbs_topic_data *ptr,*newlast = NULL;

                                            ansi_code_filter(arg2,arg2,0);
                                            for(ptr = (subtopic) ? subtopic->subtopics:bbs; ptr && !strcasecmp(arg2,ptr->name); ptr = ptr->next) {
                                                output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a %stopic with the name '"ANSI_LYELLOW"%s"ANSI_LGREEN"' already exists.",(subtopic) ? "sub-":"",arg2);
                                                return;
					    }
                	                    if(last) last->next = topic->next;
                                               else if(subtopic) subtopic->subtopics = topic->next;
                                                  else bbs = topic->next;

                                            if(subtopic) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' renamed to '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",topic->name,subtopic->name,arg2);
                                               else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' renamed to '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",topic->name,arg2);
                                            for(ptr = (subtopic) ? subtopic->subtopics:bbs; ptr && (strcasecmp(arg2,ptr->name) >= 0); newlast = ptr, ptr = ptr->next);

                                            if(newlast) newlast->next = topic;
                                               else if(subtopic) subtopic->subtopics = topic;
                                                  else bbs = topic;
                                            if(ptr) topic->next = ptr;
                                               else topic->next = NULL;

                                            writelog(BBS_LOG,1,"TOPIC","%s%s(#%d) renamed this %stopic to '%s%s%s'.",bbs_logtopic(topic,subtopic),getname(player),player,(subtopic) ? "sub-":"",(subtopic) ? subtopic->name:"",(subtopic) ? "/":"",arg2);
                                            FREENULL(topic->name);
                                            topic->name = (char *) alloc_string(arg2);
                                            setreturn(OK,COMMAND_SUCC);
					 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic's new name can't contain embedded HTML tags.",(subtopic) ? "sub-":"");
				      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic's new name can't contain query command substitutions ('"ANSI_LWHITE"%%{<QUERY COMMAND>}"ANSI_LGREEN"'.)",(subtopic) ? "sub-":"");
				   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic's new name mustn't contain '"ANSI_LWHITE"/"ANSI_LGREEN"', '"ANSI_LWHITE"\\"ANSI_LGREEN"' or '"ANSI_LWHITE";"ANSI_LGREEN"' characters.",(subtopic) ? "sub-":"");
				} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of the %stopic's new name is 13 characters in length.  It also must not contain embedded NEWLINE's.",(subtopic) ? "sub-":"");
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the new name you'd like to give to this %stopic.",(subtopic) ? "sub-":"");
			  } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only rename a %stopic you own or a topic owned by someone who's of a lower level than yourself.",(subtopic) ? "sub-":"");
                             else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only rename a %stopic you own.",(subtopic) ? "sub-":"");
		       } else {
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.  Please choose another %stopic (Type '"ANSI_LWHITE"%s%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%s%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(subtopic) ? "sub-":"",topic->name,clevels[(int) topic->accesslevel],(subtopic) ? "sub-":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"");
                          if((subtopic && (db[player].data->player.topic_id == subtopic->topic_id) && (db[player].data->player.subtopic_id == topic->topic_id)) || (!subtopic && (db[player].data->player.topic_id == topic->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		       }
		    } else {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)  Please choose another topic (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",subtopic->name,clevels[(int) subtopic->accesslevel],topic->name,(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
                       if(db[player].data->player.topic_id == topic->topic_id) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		    }
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't rename a topic while %s BBS is being dumped to disk  -  Please wait for a couple of minutes and then try again.",tcz_full_name);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, topics on %s BBS can't be renamed from within a compound command.",tcz_full_name);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from renaming topics on %s BBS.",tcz_full_name);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Morons aren't allowed to rename topics on %s BBS.",tcz_full_name);
}

/* ---->  Set parameter on/off, or display its current setting  <---- */
/*        (val1:  Flag of parameter to set.)                          */
void bbs_setparameter(CONTEXT)
{
     unsigned char reset = 0,modify = 0,display = 0;
     struct   bbs_topic_data *topic,*subtopic;

     setreturn(ERROR,COMMAND_FAIL);
     if((topic = lookup_topic(player,arg1,&topic,&subtopic))) {
        if(!Blank(arg2)) {
           if(!Moron(player)) {
              if(Bbs(player)) {
                 if(!in_command) {
                    if(!subtopic || can_access_topic(player,subtopic,NULL,1)) {
               	       if(can_access_topic(player,topic,subtopic,1)) {
                          if(can_write_to(player,topic->owner,0) || (subtopic && can_write_to(player,subtopic->owner,0))) {
	                     if(string_prefix("yes",arg2) || string_prefix("on",arg2)) {
                                if(val1 == TOPIC_HILIGHT) {
                                   if(!subtopic && !Level4(player)) {
                                      output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may hilight top-level BBS topics.");
                                      return;
				   } else if(subtopic && !Level4(player) && !can_write_to(player,subtopic->owner,1)) {
                                      output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only the owner of the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' and Apprentice Wizards/Druids and above may hilight the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",subtopic->name,topic->name);
                                      return;
				   }
				}

                                topic->flags |= val1;
                                modify = display = 1;
			     } else if(string_prefix("no",arg2) || string_prefix("off",arg2)) {
                                topic->flags &= ~val1;
                                modify = display = reset = 1;
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LWHITE"yes"ANSI_LGREEN"' or '"ANSI_LWHITE"no"ANSI_LGREEN"'.");
			  } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only modify the parameters of a %stopic you own or a topic owned by someone who's of a lower level than yourself.",(subtopic) ? "sub-":"");
                             else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only modify the parameters of a %stopic you own.",(subtopic) ? "sub-":"");
		       } else {
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.  Please choose another %stopic (Type '"ANSI_LWHITE"%s%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%s%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(subtopic) ? "sub-":"",topic->name,clevels[(int) topic->accesslevel],(subtopic) ? "sub-":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"");
                          if((subtopic && (db[player].data->player.topic_id == subtopic->topic_id) && (db[player].data->player.subtopic_id == topic->topic_id)) || (!subtopic && (db[player].data->player.topic_id == topic->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		       }
		    } else {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)  Please choose another topic (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",subtopic->name,clevels[(int) subtopic->accesslevel],topic->name,(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
                       if(db[player].data->player.topic_id == topic->topic_id) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		    }
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the parameters of a %stopic on %s BBS can't be modified from within a compound command.",(subtopic) ? "sub-":"",tcz_full_name);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from changing the parameters of %stopics on %s BBS.",(subtopic) ? "sub-":"",tcz_full_name);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Morons aren't allowed to modify the parameters of a %stopic on %s BBS.",(subtopic) ? "sub-":"",tcz_full_name);
	} else {
           reset   = ((topic->flags & val1) == 0);
           display = 1;
	}
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);

     if(display) {
        if(subtopic) sprintf(scratch_return_string," in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'",subtopic->name);
           else *scratch_return_string = '\0';
        switch(val1) {
               case TOPIC_MORTALADD:
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Mortals may%s%s add messages to the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"'%s.",(reset) ? (modify) ? " no-longer":" not":"",(modify) ? (reset) ? "":" now":"",(subtopic) ? "sub-":"",topic->name,scratch_return_string);
                    if(modify) writelog(BBS_LOG,1,"TOPIC","%s%s(#%d) changed whether Mortals may add messages to this %stopic to %s.",bbs_logtopic(topic,subtopic),getname(player),player,(subtopic) ? "sub-":"",(reset) ? "NO":"YES");
                    break;
               case TOPIC_HILIGHT:
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"The %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"'%s will%s%s be hilighted in the list of %stopics.",(subtopic) ? "sub-":"",topic->name,scratch_return_string,(reset) ? (modify) ? " no-longer":" not":"",(modify) ? (reset) ? "":" now":"",(subtopic) ? "sub-":"");
                    if(modify) writelog(BBS_LOG,1,"TOPIC","%s%s(#%d) changed whether this %stopic will be hilighted in the list of %stopics to %s.",bbs_logtopic(topic,subtopic),getname(player),player,(subtopic) ? "sub-":"",(subtopic) ? "sub-":"",(reset) ? "NO":"YES");
                    break;
               case TOPIC_CENSOR:
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Bad language in messages in the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"'%s will%s%s be censored.",(subtopic) ? "sub-":"",topic->name,scratch_return_string,(reset) ? (modify) ? " no-longer":" not":"",(modify) ? (reset) ? "":" now":"");
                    if(modify) writelog(BBS_LOG,1,"TOPIC","%s%s(#%d) changed whether bad language will be censored in this %stopic to %s.",bbs_logtopic(topic,subtopic),getname(player),player,(subtopic) ? "sub-":"",(reset) ? "NO":"YES");
                    break;
               case TOPIC_CYCLIC:
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Old messages will%s%s be automatically deleted from the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"'%s when it's full and new messages are added.",(reset) ? (modify) ? " no-longer":" not":"",(modify) ? (reset) ? "":" now":"",(subtopic) ? "sub-":"",topic->name,scratch_return_string);
                    if(modify) writelog(BBS_LOG,1,"TOPIC","%s%s(#%d) changed whether messages in this %stopic will be subject to cyclic deletion to %s.",bbs_logtopic(topic,subtopic),getname(player),player,(subtopic) ? "sub-":"",(reset) ? "NO":"YES");
                    break;
               case TOPIC_FORMAT:
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Messages in the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"'%s will%s%s be automatically formatted.",(subtopic) ? "sub-":"",topic->name,scratch_return_string,(reset) ? (modify) ? " no-longer":" not":"",(modify) ? (reset) ? "":" now":"");
                    if(modify) writelog(BBS_LOG,1,"TOPIC","%s%s(#%d) changed whether messages in this %stopic will be automatically formatted to %s.",bbs_logtopic(topic,subtopic),getname(player),player,(subtopic) ? "sub-":"",(reset) ? "NO":"YES");
                    break;
               case TOPIC_ANON:
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Messages may%s%s be made anonymous to the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"'%s.",(reset) ? (modify) ? " no-longer":" not":"",(modify) ? (reset) ? "":" now":"",(subtopic) ? "sub-":"",topic->name,scratch_return_string);
                    if(modify) writelog(BBS_LOG,1,"TOPIC","%s%s(#%d) changed whether anonymous messages are allowed in this %stopic to %s.",bbs_logtopic(topic,subtopic),getname(player),player,(subtopic) ? "sub-":"",(reset) ? "NO":"YES");
                    break;
               case TOPIC_ADD:
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Messages may%s%s be added to the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"'%s.",(reset) ? (modify) ? " no-longer":" not":"",(modify) ? (reset) ? "":" now":"",(subtopic) ? "sub-":"",topic->name,scratch_return_string);
                    if(modify) writelog(BBS_LOG,1,"TOPIC","%s%s(#%d) changed whether messages may be added to this %stopic to %s.",bbs_logtopic(topic,subtopic),getname(player),player,(subtopic) ? "sub-":"",(reset) ? "NO":"YES");
                    break;
	}
        setreturn(OK,COMMAND_SUCC);
     }
}

/* ---->  Set/display specified topic's sub-topic limit (Maximum number of sub-topics allowed in topic at any one time)  <---- */
void bbs_subtopiclimit(CONTEXT)
{
     struct   bbs_topic_data *topic,*subtopic;
     unsigned char modify = 0,display = 0;
     int      newlimit;

     setreturn(ERROR,COMMAND_FAIL);
     if((topic = lookup_topic(player,arg1,&topic,&subtopic))) {
        if(!Blank(arg2)) {
           if(!in_command) {
              if(Level3(player)) {
                 if(Bbs(player)) {
                    if(!subtopic) {
         	       if(can_access_topic(player,topic,NULL,1)) {
                          if(can_write_to(player,topic->owner,0)) {
                             newlimit = atol(arg2);
                             if(Level1(player) || (newlimit <= BBS_MAX_SUBTOPICS)) {
                                topic->subtopiclimit = newlimit;
                                display = modify = 1;
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Deities and the Supreme Being can set a sub-topic limit greater than "ANSI_LWHITE"%d"ANSI_LGREEN".",BBS_MAX_SUBTOPICS);
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the message limit of a topic you own or a topic owned by someone who's of a lower level than yourself.");
		       } else {
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.  Please choose another %stopic (Type '"ANSI_LWHITE"%s%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%s%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(subtopic) ? "sub-":"",topic->name,clevels[(int) topic->accesslevel],(subtopic) ? "sub-":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"");
                          if((subtopic && (db[player].data->player.topic_id == subtopic->topic_id) && (db[player].data->player.subtopic_id == topic->topic_id)) || (!subtopic && (db[player].data->player.topic_id == topic->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		       }
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the sub-topic limit of a sub-topic can't be changed (Sub-topics can't contain sub-topics.)");
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from changing the sub-topic limit of %stopics on %s BBS.",(subtopic) ? "sub-":"",tcz_full_name);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Wizards/Druids and above can change the sub-topic limit of a %stopic on %s BBS.",(subtopic) ? "sub-":"",tcz_full_name);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the sub-topic limit of a %stopic on %s BBS can't be changed from within a compound command.",(subtopic) ? "sub-":"",tcz_full_name);
	} else display = 1;
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);

     if(display) {
        if(subtopic) output(getdsc(player),player,0,1,0,ANSI_LGREEN"A maximum of "ANSI_LYELLOW"%d"ANSI_LGREEN" sub-topic%s %s%s allowed in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' at any one time.",topic->subtopiclimit,Plural(topic->subtopiclimit),(topic->subtopiclimit == 1) ? "is":"are",(modify) ? " now":"",topic->name,subtopic->name);
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"A maximum of "ANSI_LYELLOW"%d"ANSI_LGREEN" sub-topic%s %s%s allowed in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' at any one time.",topic->subtopiclimit,Plural(topic->subtopiclimit),(topic->subtopiclimit == 1) ? "is":"are",(modify) ? " now":"",topic->name);
        if(modify) writelog(BBS_LOG,1,"TOPIC","%s%s(#%d) changed the sub-topic limit of this %stopic to %d.",bbs_logtopic(topic,subtopic),getname(player),player,(subtopic) ? "sub-":"",topic->subtopiclimit);
        setreturn(OK,COMMAND_SUCC);
     }
}

/* ---->  Set/display specified topic's message time limit  <---- */
void bbs_timelimit(CONTEXT)
{
     struct   bbs_topic_data *topic,*subtopic;
     unsigned char modify = 0,display = 0;

     setreturn(ERROR,COMMAND_FAIL);
     if((topic = lookup_topic(player,arg1,&topic,&subtopic))) {
        if(!Blank(arg2)) {
           if(!Moron(player)) {
              if(Bbs(player)) {
                 if(!in_command) {
                    if(!subtopic || can_access_topic(player,subtopic,NULL,1)) {
            	       if(can_access_topic(player,topic,subtopic,1)) {
                          if(can_write_to(player,topic->owner,0) || (subtopic && can_write_to(player,subtopic->owner,0))) {
                             if(string_prefix("none",arg2) || string_prefix("unrestricted",arg2)) topic->timelimit = 0;
                                else topic->timelimit = atol(arg2);
                             display = modify = 1;
		          } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the message time limit of a %stopic you own or a topic owned by someone who's of a lower level than yourself.",(subtopic) ? "sub-":"");
                             else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the message time limit of a %stopic you own.",(subtopic) ? "sub-":"");
		       } else {
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s.  Please choose another %stopic (Type '"ANSI_LWHITE"%s%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%s%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",(subtopic) ? "sub-":"",topic->name,clevels[(int) topic->accesslevel],(subtopic) ? "sub-":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"",(command_type == BBS_COMMAND) ? "bbs ":"",(subtopic) ? "sub":"");
                          if((subtopic && (db[player].data->player.topic_id == subtopic->topic_id) && (db[player].data->player.subtopic_id == topic->topic_id)) || (!subtopic && (db[player].data->player.topic_id == topic->topic_id))) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		       }
		    } else {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' may only be accessed by %s (The sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' is within this topic.)  Please choose another topisc (Type '"ANSI_LWHITE"%stopics"ANSI_LGREEN"' and then '"ANSI_LWHITE"%stopic <TOPIC NAME>"ANSI_LGREEN"'.)",subtopic->name,clevels[(int) subtopic->accesslevel],topic->name,(command_type == BBS_COMMAND) ? "bbs ":"",(command_type == BBS_COMMAND) ? "bbs ":"");
                       if(db[player].data->player.topic_id == topic->topic_id) db[player].data->player.topic_id = 0, db[player].data->player.subtopic_id = 0;
		    }
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the message time limit of a %stopic on %s BBS can't be changed from within a compound command.",(subtopic) ? "sub-":"",tcz_full_name);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from changing the message time limit of %stopics on %s BBS.",(subtopic) ? "sub-":"",tcz_full_name);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Morons aren't allowed to change the message time limit of a %stopic on %s BBS.",(subtopic) ? "sub-":"",tcz_full_name);
	} else display = 1;
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);

     if(display) {
        if(subtopic) sprintf(scratch_return_string," in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'",subtopic->name);
           else *scratch_return_string = '\0';
        if(!topic->timelimit) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Messages in the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"'%s are %s subject to a time limit and will not be deleted automatically.",(subtopic) ? "sub-":"",topic->name,scratch_return_string,(modify) ? "no-longer":"not");
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Messages in the %stopic '"ANSI_LWHITE"%s"ANSI_LGREEN"'%s which haven't been read for "ANSI_LYELLOW"%d"ANSI_LGREEN" day%s will%s be deleted automatically.",(subtopic) ? "sub-":"",topic->name,scratch_return_string,topic->timelimit,Plural(topic->timelimit),(modify) ? " now":"");
        if(modify) writelog(BBS_LOG,1,"TOPIC","%s%s(#%d) changed the message time limit of this %stopic to %d.",bbs_logtopic(topic,subtopic),getname(player),player,(subtopic) ? "sub-":"",topic->timelimit);
        setreturn(OK,COMMAND_SUCC);
     }
}
