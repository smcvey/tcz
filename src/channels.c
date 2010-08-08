/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| CHANNELS.C  -  Implements a flexible channel communication system           |
|                (Replacing the original, simple 'chat' chatting channels.)   |
|                                                                             |
|                Main features:                                               |
|                ~~~~~~~~~~~~~~                                               |
|                *  Extremely easy to use.                                    |
|                *  Named channels.                                           |
|                *  Permanent channels.                                       |
|                *  Join multiple channels simultaneously.                    |
|                *  Channel user lists (Invited, Banned, Operators, etc.)     |
|                *  Association with BBS topics/sub-topics.                   |
|                *  Support for 'soft-coded' uses, such as 'radio stations',  |
|                   etc.                                                      |
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
| Module originally designed and written by:  J.P.Boggis 18/04/1997.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: channels.c,v 1.2 2005/06/29 20:40:18 tcz_monster Exp $

*/

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <stdio.h>

#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "object_types.h"
#include "flagset.h"
#include "search.h"


/* ---->  Channel command table entry  <---- */
struct channel_cmd_table {
       char     *name;
       void     (*func)(struct descriptor_data *, char *, char *, int);  /*  d, chname, params, option  */
       int      option;
       unsigned char len;
};


/* ---->  Channel command table  <---- */
struct channel_cmd_table channel_cmds[] = {
       {"banner",            (void *) channel_banner,  0, 0},
       {"censored",          (void *) channel_censor,  0, 0},
       {"censorship",        (void *) channel_censor,  0, 0},
       {"create",            (void *) channel_create,  0, 0},
       {"delete",            (void *) channel_destroy, 0, 0},
       {"emote",             (void *) channel_speak,   1, 0},
       {"erase",             (void *) channel_destroy, 0, 0},
       {"join",              (void *) channel_join,    0, 0},
       {"leave",             (void *) channel_leave,   0, 0},
       {"motd",              (void *) channel_motd,    0, 0},
       {"name",              (void *) channel_rename,  0, 0},
       {"new",               (void *) channel_create,  0, 0},
       {"off",               (void *) channel_off,     0, 0},
       {"on",                (void *) channel_on,      0, 0},
       {"pose",              (void *) channel_speak,   1, 0},
       {"remove",            (void *) channel_destroy, 0, 0},
       {"rename",            (void *) channel_rename,  0, 0},
       {"say",               (void *) channel_speak,   0, 0},
       {"silence",           (void *) channel_on,      0, 0},
       {"think",             (void *) channel_speak,   2, 0},
       {"title",             (void *) channel_title,   0, 0},
       {"unsilence",         (void *) channel_off,     0, 0},
       {NULL,                NULL,                     0, 0},
};

char   *channel_restrictions[] = {"anyone joined to the channel",
                                  "permanent invites, operators and the owner of the channel",
                                  "operators and the owner of the channel",
                                  "Apprentice Wizards/Druids and above and the owner of the channel",
                                  "the owner of the channel"};

struct   channel_data *channels,*channelroot,*permanent;
unsigned short channel_size = 0;
unsigned char  chfound = 0;


/* ---->  Sort channel command table into strict alphabetical order    <---- */
/*        (channel_search_cmdtable() relies on entries in the channel        */
/*        command table being in strict alphabetical order (A binary         */
/*        search is used for maximum efficiency.))                           */
short channel_sort_cmdtable()
{
      struct   channel_cmd_table temp;
      unsigned short loop,top,highest;

      for(loop = 0; channel_cmds[loop].name; channel_cmds[loop].len = strlen(channel_cmds[loop].name), loop++);
      for(top = (channel_size = loop) - 1; top > 0; top--) {

          /* ---->  Find highest entry in unsorted part of list  <---- */
          for(loop = 1, highest = 0; loop <= top; loop++)
              if(strcasecmp(channel_cmds[loop].name,channel_cmds[highest].name) > 0)
                 highest = loop;

          /* ---->  Swap highest entry in unsorted part of list with bottom entry of sorted part of list  <---- */
          if(highest < top) {
             temp.name = channel_cmds[top].name, temp.func = channel_cmds[top].func, temp.option = channel_cmds[top].option, temp.len = channel_cmds[top].len;
             channel_cmds[top].name = channel_cmds[highest].name, channel_cmds[top].func = channel_cmds[highest].func, channel_cmds[top].option = channel_cmds[highest].option, channel_cmds[top].len = channel_cmds[highest].len;
             channel_cmds[highest].name = temp.name, channel_cmds[highest].func = temp.func, channel_cmds[highest].option = temp.option, channel_cmds[highest].len = temp.len;
	  }
      }
      return(channel_size);
}

/* ---->  Search for COMMAND in channel command table, returning  <---- */
/*        CHANNEL_CMD_TABLE entry if found, otherwise NULL.             */
struct channel_cmd_table *channel_search_cmdtable(const char *command)
{
       int      top = channel_size - 1,middle = 0,bottom = 0,nearest = NOTHING,result;
       unsigned short len,nlen = 0xFFFF;

       if(Blank(command)) return(NULL);
       len = strlen(command);
       while(bottom <= top) {
             middle = (top + bottom) / 2;
             if((result = strcasecmp(channel_cmds[middle].name,command)) != 0) {
                if((channel_cmds[middle].len < nlen) && (len <= channel_cmds[middle].len) && !strncasecmp(channel_cmds[middle].name,command,len))
                   nearest = middle, nlen = channel_cmds[middle].len;

                if(result < 0) bottom = middle + 1;
                   else top = middle - 1;
	     } else return(&channel_cmds[middle]);
       }

       /* ---->  Nearest matching command  <---- */
       if(nearest != NOTHING) return(&channel_cmds[nearest]);
       return(NULL);
}

/* ---->  Lookup channel by name, using binary tree  <---- */
struct channel_data *channel_lookup(const char *name,struct channel_data **last,unsigned char exact)
{
       struct channel_data *current = channelroot;
       int    value,length;

       chfound = 0;
       if(last) *last = NULL;
       if(Blank(name)) return(NULL);
       length = strlen(name);
       if(current) {
          while(current)
                if((!exact && strncasecmp(current->name,name,strlen(name))) || (exact && strcasecmp(current->name,name))) {
                   value = strcasecmp(current->name,name);
                   if(last) *last = current;
                   current = (value > 0) ? current->left:current->right;
		} else {
                   chfound = 1;
                   return(current);
		}
       }
       return(NULL);
}

/* ---->  Add channel to binary tree of channel names  <---- */
unsigned char channel_add(struct channel_data *channel)
{
	 struct   channel_data *current = channelroot,*last = NULL;
	 unsigned char right;
	 int      value;

	 if(!channel || Blank(channel->name)) return(0);
	 if(channelroot) {
	    while(current)
		  if(!(value = strcasecmp(channel->name,current->name))) return(0);
		     else if(value < 0) last = current, current = current->left, right = 0;
			else last = current, current = current->right, right = 1;

	    if(last) {
	       if(right) last->right = channel;
		  else last->left = channel;
	    }
	 } else channelroot = channel;
	 return(1);
}

/* ---->  Remove channel from binary tree of channel names  <---- */
unsigned char channel_remove(struct channel_data *channel,struct channel_data *last)
{
	 if(!channel) return(0);

	 /* ---->  Remove node from binary tree  <---- */
	 if(last) {
	    if(last->right == channel) last->right = NULL;
	    if(last->left  == channel) last->left  = NULL;
	 } else channelroot = NULL;

	 /* ---->  Add branches of node back into binary tree  <--- */
	 if(channel->right) channel_add(channel->right);
	 if(channel->left)  channel_add(channel->left);
	 return(1);
}

static struct channel_data *rootnode = NULL,*tail = NULL;

/* ---->  Recursively traverse binary tree to sort channel names into alphabetical order  <---- */
void traverse_channels(struct channel_data *channel)
{
     if(channel->left) traverse_channels(channel->left);
     if(rootnode) {
        channel->sort = NULL;
        tail->sort    = channel;
        tail          = channel;
     } else rootnode = channel, tail = channel, channel->sort = NULL;
     if(channel->right) traverse_channels(channel->right);
}

/* ---->  Make banner for channel  <---- */
const char *channel_makebanner(const char *banner,unsigned char size,char *buffer,int *indent)
{
      unsigned char ansi = 0,caps = 1;
      char     *temp = buffer;
      int      length = 0;

      *buffer = '\0', *indent = 0;
      if(Blank(banner)) return(NULL);

      while(*banner && (*banner == '\x1B')) {
            for(; *banner && (*banner != 'm'); *temp++ = *banner++);
            if(*banner && (*banner == 'm')) *temp++ = *banner++;
            ansi = 1;
      }
      if(Blank(banner)) return(NULL);

      if(!ansi) {
         strcpy(temp,ANSI_LCYAN);
         temp += strlen(ANSI_LCYAN);
      }
      *temp++ = '[';

      while(*banner && (length < size))
            if(*banner == '\x1B') {
               for(; *banner && (*banner != 'm'); *temp++ = *banner++);
               if(*banner && (*banner == 'm')) *temp++ = *banner++;
	    } else if(isascii(*banner) && isprint(*banner)) {
               if(caps && isalnum(*banner))
                  *temp++ = toupper(*banner++), caps = 0;
                     else *temp++ = *banner++;
               length++;
	    } else banner++;
      *temp++ = ']', *temp = '\0', *indent = length + 4;
      return(buffer);
}

/* ---->  Return flags of user of CHANNEL  <----- */
int channel_flags(struct channel_data *channel,dbref player)
{
    struct channel_user_data *start = NULL,*tail = NULL,*current,*next;
    int    flags = 0;

    for(current = channel->userlist; current; current = next) {
        next = current->next;
        if(Validchar(current->player) && (Connected(current->player) || (current->player == channel->owner) || (current->flags & CHANNEL_USER_JOINED))) {
           if(current->player == player) flags = current->flags;
           if(start) {
              tail->next    = current;
              current->next = NULL;
              tail          = current;
	   } else start = tail = current;
	} else FREENULL(current);
    }
    tail->next = NULL;
    channel->userlist = start;
    return(flags);
}

/* ---->  Return privilege of user of CHANNEL  <---- */
unsigned char channel_privilege(struct channel_data *channel,dbref player)
{
	 int flags;

	 if(!channel) return(CHANNEL_COMMAND_ALL);
	 if(Owner(player) == channel->owner) return(CHANNEL_COMMAND_OWNER);
	    else if(!(flags = channel_flags(channel,player))) return(CHANNEL_COMMAND_ALL);
	       else if(flags & CHANNEL_USER_OPERATOR) return(CHANNEL_COMMAND_OPERATOR);
		  else if(flags & CHANNEL_USER_INVITE) return(CHANNEL_COMMAND_INVITE);
		     else return(CHANNEL_COMMAND_ALL);
}

/*  =======================================================================  */
/*                C O M M U N I C A T I O N   C O M M A N D S                */
/*  =======================================================================  */

/* ---->  Output raw message over channel  <---- */
void channel_output(channel,exact,exception,flags,level,fmt,p0,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17,p18,p19,p20,p21,p22,p23,p24,p25,p26,p27,p28,p29,p30,p31,p32,p33,p34) struct channel_data *channel; dbref exact; dbref exception; int flags; unsigned char level; char *fmt;
{
     char   chnamebuf[CHANNEL_BANNER_MAX * 2];
     struct channel_user_data *ptr;
     char   fmtbuffer[TEXT_SIZE];
     struct descriptor_data *d;

     if(!channel || !channel->userlist) return;
     sprintf(fmtbuffer,"%%s"ANSI_LGREEN" \016&nbsp;\016 %s",fmt),
     cmpptr = chnamebuf, decompress(channel->banner), cmpptr = cmpbuf;
     for(d = descriptor_list; d; d = d->next)
         if(Validchar(d->player) && (d->flags & CONNECTED) && (d->player != exception) && ((exact == NOTHING) || (d->player == exact))) {
            for(ptr = channel->userlist; ptr && (ptr->player != d->player); ptr = ptr->next);
            if(ptr && ((flags == SEARCH_ALL) || ((ptr->flags & flags) == flags)))
               output(d,d->player,0,1,channel->indent,fmtbuffer,chnamebuf,p0,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,p17,p18,p19,p20,p21,p22,p23,p24,p25,p26,p27,p28,p29,p30,p31,p32,p33,p34);
	 }
}

/* ---->  Show channel MOTD to user, if changed  <---- */
void channel_show_motd(struct channel_data *channel,dbref player,int flags)
{
     struct channel_user_data *current;

     if(in_command) return;
     if(channel && channel->motd && (flags & CHANNEL_USER_MOTD)) {
        if(channel->flags & CHANNEL_CENSOR)
           bad_language_filter(scratch_buffer,decompress(channel->motd));
              else strcpy(scratch_buffer,channel->motd);
        channel_output(channel,player,NOTHING,SEARCH_ALL,CHANNEL_COMMAND_ALL,"Message of the day for the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"':\n\n"ANSI_LYELLOW"%s\n",channel->name,substitute(channel->owner,scratch_return_string,scratch_buffer,0,ANSI_LYELLOW,NULL,0));
        for(current = channel->userlist; current && (current->player != player); current = current->next);
        if(current) current->flags &= ~CHANNEL_USER_MOTD;
     }
}

/* ---->  Broadcast message over channel  <---- */

/* ---->             Speak over channel              <---- */
/*        (option:  0 = Say, 1 = Emote, 2 = Think.)        */
void channel_speak(CHANNEL_CONTEXT)
{
     char   buffer[TEXT_SIZE + 128];
     struct channel_data *channel;
     int    flags;

     comms_spoken(d->player,0);
     if((channel = channel_lookup(chname,NULL,0)) && (chname = channel->name)) {
        if((flags = channel_flags(channel,d->player)) & CHANNEL_USER_JOINED) {
           if(flags & CHANNEL_USER_ACTIVE) {
              if((channel_privilege(channel,Owner(d->player)) >= channel->sendrestriction) || (Level4(Owner(d->player)) && (channel->sendrestriction == CHANNEL_COMMAND_ADMIN))) {
                 if(!Blank(params)) {
                    switch(option) {
                           case 1:
                                sprintf(buffer,":%s",params);
                                break;
                           case 2:
                                sprintf(buffer,"+%s",params);
                                break;
                           default:
                                strcpy(buffer,params);
		    }

                    command_type |= COMM_CMD;
                    if((channel->flags & CHANNEL_CENSOR) && !Censor(d->player) && !Censor(Location(d->player)))
                       bad_language_filter(buffer,buffer);
                    channel_show_motd(channel,d->player,flags);
                    strcpy(scratch_return_string,construct_message(d->player,ANSI_LWHITE,ANSI_DWHITE,"say",'.',-1,PLAYER,buffer,0,DEFINITE));
                    channel_output(channel,d->player,NOTHING,SEARCH_ALL,CHANNEL_COMMAND_ALL,"%s",scratch_return_string);

                    strcpy(scratch_return_string,construct_message(d->player,ANSI_LWHITE,ANSI_DWHITE,"says",'.',-1,OTHERS,buffer,0,INDEFINITE));
                    channel_output(channel,NOTHING,d->player,CHANNEL_OUTPUT_FLAGS,CHANNEL_COMMAND_ALL,"%s",scratch_return_string);
                    command_type &= COMM_CMD;
                    setreturn(OK,COMMAND_SUCC);
		 } else output(d,d->player,0,1,0,ANSI_LGREEN"What would you like to %s over the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"'?",(option == 1) ? "emote":(option == 2) ? "think about":"say",chname);
	      } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, only %s may %s over the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",(option == 1) ? "emote":(option == 2) ? "think":"speak",chname);
	   } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you have switched off the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"'.  Please type '"ANSI_LYELLOW"-%s.on"ANSI_LGREEN"' to switch it back on again before trying to speak over it.",chname,chname);
	} else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you are not currently joined to the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",chname);
     } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.  Please type '"ANSI_LYELLOW"-list"ANSI_LGREEN"' for a list of available channels.",chname);
}

/*  =======================================================================  */
/*                 M A N I P U L A T I O N   C O M M A N D S                 */
/*  =======================================================================  */

/* ---->  Join channel  <---- */
void channel_join(CHANNEL_CONTEXT)
{
     struct   channel_user_data *current,*last = NULL;
     struct   channel_data *channel;
     unsigned char invited;
     int      flags;

     if((channel = channel_lookup(chname,NULL,0)) && (chname = channel->name)) {
        if(!((flags = channel_flags(channel,d->player)) & CHANNEL_USER_JOINED)) {
           if(!(channel->flags & CHANNEL_PRIVATE) || (flags & CHANNEL_USER_OPERATOR) || (Owner(d->player) == channel->owner)) {
              invited = (((flags & (CHANNEL_USER_INVITE|CHANNEL_USER_OPERATOR)) != 0) || (Owner(d->player) == channel->owner));
              if(invited || (privilege(d->player,255) <= channel->accesslevel)) {
                 for(current = channel->userlist; current && (current->player != d->player); last = current, current = current->next);
                 if(!current) {
                    MALLOC(current,struct channel_user_data);
                    current->player = d->player;
                    current->flags  = CHANNEL_USER_ACTIVE;
                    current->next   = NULL;
                    if(last) last->next = current;
                       else channel->userlist = current;
		 }
                 current->flags |= (CHANNEL_USER_JOINED|CHANNEL_USER_MOTD);

                 if(!in_command) {
                    if(channel->title) {
                       if(channel->flags & CHANNEL_CENSOR)
                          bad_language_filter(scratch_return_string,decompress(channel->title));
                             else strcpy(scratch_buffer,channel->title);
                       channel_output(channel,d->player,NOTHING,SEARCH_ALL,CHANNEL_COMMAND_ALL,"Welcome to the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' ("ANSI_LYELLOW"%s"ANSI_LGREEN"), %s"ANSI_LWHITE"%s"ANSI_LGREEN".",chname,substitute(channel->owner,scratch_return_string,scratch_buffer,0,ANSI_LYELLOW,NULL,0),Article(d->player,LOWER,DEFINITE),getcname(NOTHING,d->player,0,0));
		    } else channel_output(channel,d->player,NOTHING,SEARCH_ALL,CHANNEL_COMMAND_ALL,"Welcome to the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"', %s"ANSI_LYELLOW"%s"ANSI_LGREEN".",chname,Article(d->player,LOWER,DEFINITE),getcname(NOTHING,d->player,0,0));
		 }
                 channel_output(channel,NOTHING,d->player,CHANNEL_OUTPUT_FLAGS,CHANNEL_COMMAND_ALL,"%s"ANSI_LWHITE"%s"ANSI_LGREEN"%s has joined the channel.",Article(d->player,UPPER,INDEFINITE),getcname(NOTHING,d->player,0,0),(current->flags & CHANNEL_USER_ACTIVE) ? "":" (Silenced)");
                 channel_show_motd(channel,d->player,current->flags);

                 if(!(current->flags & CHANNEL_USER_ACTIVE))
                    channel_output(channel,d->player,SEARCH_ALL,CHANNEL_COMMAND_ALL,"You have silenced this channel (Turned it off) and will not receive messages sent over it by other users.  Type '"ANSI_LWHITE"-%s.on"ANSI_LGREEN"' to turn it back on again.",chname);
                 setreturn(OK,COMMAND_SUCC);
	      } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, only %s may join the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",clevels[channel->accesslevel],chname);
	   } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' is private.",chname);
	} else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you're already joined to the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",chname);
     } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.  Please type '"ANSI_LYELLOW"-list"ANSI_LGREEN"' for a list of available channels.",chname);
}

/* ---->  Leave channel  <---- */
void channel_leave(CHANNEL_CONTEXT)
{
     struct   channel_user_data *current,*last = NULL;
     struct   channel_data *channel;
     struct   descriptor_data *ptr;
     unsigned char joined;
     int      flags;

     if((channel = channel_lookup(chname,NULL,0)) && (chname = channel->name)) {
        if((flags = channel_flags(channel,d->player)) & CHANNEL_USER_JOINED) {
           if(Owner(d->player) != channel->owner) {
              channel_show_motd(channel,d->player,flags);
              if(!in_command) channel_output(channel,d->player,NOTHING,SEARCH_ALL,CHANNEL_COMMAND_ALL,"You are no-longer joined to the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",chname);
              channel_output(channel,NOTHING,d->player,CHANNEL_OUTPUT_FLAGS,CHANNEL_COMMAND_ALL,"%s"ANSI_LWHITE"%s"ANSI_LGREEN"%s has left the channel.",Article(d->player,UPPER,INDEFINITE),getcname(NOTHING,d->player,0,0),(current->flags & CHANNEL_USER_ACTIVE) ? "":" (Silenced)");

              for(current = channel->userlist; current && (current->player != d->player); last = current, current = current->next);
              if(current) {
                 current->flags &= ~CHANNEL_USER_JOINED;
                 if(!(current->flags & CHANNEL_PRIV_FLAGS)) {
                    if(last) last->next = current->next;
                       else channel->userlist = current->next;
                    FREENULL(current);
		 }
	      }

              /* ---->  Connected users still joined to channel?  <---- */
              for(ptr = descriptor_list, joined = 0; ptr && !joined; ptr = ptr->next)
                  if(Validchar(ptr->player) && (ptr->flags & CONNECTED) && ((flags = channel_flags(channel,ptr->player)) & CHANNEL_USER_JOINED))
                     joined = 0;

              if(!joined) {
                 if(channel->flags   & CHANNEL_PRIVATE)    channel->flags &= ~CHANNEL_PRIVATE;
                 if(!(channel->flags & CHANNEL_PERMANENT)) channel_destroy(d,chname,NULL,1);
	      }
              setreturn(OK,COMMAND_SUCC);
	   } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you can't leave the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"', because you are its owner.  If you no-longer need this channel, please destroy it by typing '"ANSI_LYELLOW"-%s.destroy"ANSI_LGREEN"'.",chname,chname);
	} else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you are not currently joined to the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",chname);
     } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.  Please type '"ANSI_LYELLOW"-list"ANSI_LGREEN"' for a list of available channels.",chname);
}

/* ---->  List available channels  <---- */
void channel_list(CHANNEL_CONTEXT)
{
     unsigned char titlewidth,twidth = output_terminal_width(d->player);
     struct   descriptor_data *p = getdsc(d->player);
     struct   channel_user_data *current;
     int      users,guests,operators;
     const    char *status,*access;
     int      loop,userflags;
     char     *ptr;

     titlewidth = (twidth - 34);
     params = (char *) parse_grouprange(d->player,params,FIRST,1);

     if(channels) traverse_channels(channelroot);
        else rootnode = NULL;
     set_conditions(d->player,0,0,0,0,NULL,511);
     union_initgrouprange((union group_data *) rootnode);
     while(union_grouprange()) {

           /* ---->  Channel title  <---- */
           if(grp->cunion->channel.title) {
              substitute(d->player,scratch_return_string,decompress(grp->cunion->channel.title),0,ANSI_LYELLOW,NULL,0);
              ansi_code_filter(scratch_return_string,scratch_return_string,0);
	   } else strcpy(scratch_return_string,ANSI_LYELLOW"This channel has no title.");

           /* ---->  Status on channel  <---- */
           userflags = channel_flags(&(grp->cunion->channel),d->player);
           if(userflags & CHANNEL_USER_JOINED) {
              if(userflags & CHANNEL_USER_ACTIVE) status = "[JOINED]";
	         else status = "[SILENCED]";
	   } else if(d->player == grp->cunion->channel.owner) status = "[OWNER]";
              else if(userflags & CHANNEL_USER_OPERATOR) status = "[OPERATOR]";
                 else if(userflags & CHANNEL_USER_INVITED) status = "[INVITED]";
                    else if(grp->cunion->channel.flags & CHANNEL_SECRET) status = "[SECRET]";
                       else status = "";

           /* ---->  Public/Invite/Private channel  <---- */
           if(grp->cunion->channel.flags & CHANNEL_INVITE) access = "(Invite)";
              else if(grp->cunion->channel.flags & CHANNEL_PRIVATE) access = "(Private)";
                 else access = "(Public)";

           /* ---->  Count users on channel  <---- */
           users = 0, guests = 0, operators = 0;
           for(current = grp->cunion->channel.userlist; current; current = current->next)
               if((current->flags & CHANNEL_USER_OPERATOR) || (current->player == grp->cunion->channel.owner)) operators++;
                  else if(current->flags & CHANNEL_USER_INVITED) guests++;
                     else users++;

           /* ---->  List channel  <---- */
           if(IsHtml(p)) {
	   } else {

              /* ---->  Truncate length of channel title  <---- */
              ptr = scratch_return_string, loop = titlewidth;
              while(*ptr && (*ptr != '\n') && (loop > 0))
                    if(*ptr == '\x1B') {
                       for(; *ptr && (*ptr != 'm'); ptr++);
                       if(*ptr) ptr++;
		    } else ptr++, loop--;
              if(*ptr) *ptr = '\0';
              output(p,p->player,0,1,0,ANSI_LWHITE" %-20s  "ANSI_LGREEN"%-9s  "ANSI_LYELLOW"%s\n"ANSI_LCYAN" %20s  "ANSI_LWHITE"%-8d%-8d%-12d"ANSI_LCYAN"%s",grp->cunion->channel.name,access,scratch_return_string,status,users,guests,operators,getcname(0,grp->cunion->channel.owner,0,0));
	   }
     }
     chfound = 0;
}

/* ---->  Switch channel on  <---- */
void channel_on(CHANNEL_CONTEXT)
{
     struct channel_user_data *current;
     struct channel_data *channel;
     int    flags;

     if((channel = channel_lookup(chname,NULL,0)) && (chname = channel->name)) {
        if((flags = channel_flags(channel,d->player)) & CHANNEL_USER_JOINED) {
           if(!(flags & CHANNEL_USER_ACTIVE)) {
              for(current = channel->userlist; current && (current->player != d->player); current = current->next);
              if(current) current->flags |= CHANNEL_USER_ACTIVE;
              if(!in_command) channel_output(channel,d->player,NOTHING,SEARCH_ALL,CHANNEL_COMMAND_ALL,"The channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' is now switched "ANSI_LYELLOW"on"ANSI_LGREEN".",chname);
              channel_output(channel,NOTHING,d->player,CHANNEL_OUTPUT_FLAGS,CHANNEL_COMMAND_ALL,"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has switched on the channel (%s will now hear messages sent over it again.)",Article(d->player,UPPER,INDEFINITE),getcname(NOTHING,d->player,0,0),Subjective(d->player,1));
              channel_show_motd(channel,d->player,flags);
              setreturn(OK,COMMAND_SUCC);
	   } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' is already switched on.",chname);
	} else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you are not currently joined to the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",chname);
     } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.  Please type '"ANSI_LYELLOW"-list"ANSI_LGREEN"' for a list of available channels.",chname);
}

/* ---->  Switch channel off  <---- */
void channel_off(CHANNEL_CONTEXT)
{
     struct channel_user_data *current;
     struct channel_data *channel;
     int    flags;

     if((channel = channel_lookup(chname,NULL,0)) && (chname = channel->name)) {
        if((flags = channel_flags(channel,d->player)) & CHANNEL_USER_JOINED) {
           if(flags & CHANNEL_USER_ACTIVE) {
              for(current = channel->userlist; current && (current->player != d->player); current = current->next);
              if(current) current->flags &= ~CHANNEL_USER_ACTIVE;
              channel_show_motd(channel,d->player,flags);
              if(!in_command) channel_output(channel,d->player,NOTHING,SEARCH_ALL,CHANNEL_COMMAND_ALL,"The channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' is now switched "ANSI_LYELLOW"off"ANSI_LGREEN".",chname);
              channel_output(channel,NOTHING,d->player,CHANNEL_OUTPUT_FLAGS,CHANNEL_COMMAND_ALL,"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has switched off the channel (%s will not hear messages sent over it.)",Article(d->player,UPPER,INDEFINITE),getcname(NOTHING,d->player,0,0),Subjective(d->player,1));
              setreturn(OK,COMMAND_SUCC);
	   } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' is already switched off.",chname);
	} else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you are not currently joined to the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",chname);
     } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.  Please type '"ANSI_LYELLOW"-list"ANSI_LGREEN"' for a list of available channels.",chname);
}

/*  =======================================================================  */
/*                C U S T O M I S A T I O N   C O M M A N D S                */
/*  =======================================================================  */

/* ---->  Set banner of channel  <---- */
void channel_banner(CHANNEL_CONTEXT)
{
     struct channel_data *channel,*current;
     int    flags,count;
     const  char *ptr;

     if(!in_command || Level4(current_command) || (Owner(current_command) == channel->owner)) {
        if((channel = channel_lookup(chname,NULL,0)) && (chname = channel->name)) {
           if((flags = channel_flags(channel,d->player)) & CHANNEL_USER_JOINED) {
              if(can_write_to(d->player,channel->owner,0)) {
                 if(!(channel->flags & CHANNEL_LOCKED) || (!(channel->flags & CHANNEL_LOCKED_OWNER) && (flags & CHANNEL_USER_OPERATOR)) || (Owner(d->player) == channel->owner)) {
                    if(!Blank(params)) {
                       if(!Level3(Owner(d->player)) && !Censor(d->player) && !Censor(Location(d->player)))
                          bad_language_filter(params,params);
                       substitute(d->player,scratch_buffer,params,0,ANSI_LCYAN,NULL,0);
                       if(strlen(scratch_buffer) <= CHANNEL_BANNER_MAX) {
                          if((ptr = channel_makebanner(scratch_buffer,CHANNEL_BANNER_LENGTH,scratch_return_string,&count))) {

                             /* ---->  Check banner isn't similar to that of another channel  <---- */
                             ansi_code_filter(scratch_buffer,ptr,1);
                             for(current = channels, count = 0; current && !count; current = (count) ? current:current->next)
                                 if((current != channel) && current->banner) {
                                    ansi_code_filter(scratch_buffer + 2048,decompress(current->banner),1);
                                    if(!strcasecmp(scratch_buffer,scratch_buffer + 2048)) count = 1;
				 }

                             if(!count || !current) {

                                /* ---->  Set new banner  <---- */
                                FREENULL(channel->banner);
                                channel->banner = (char *) alloc_string(compress(ptr,1));
                                channel->indent = count;
                                channel_show_motd(channel,d->player,flags);
                                if(!in_command) channel_output(channel,NOTHING,NOTHING,CHANNEL_OUTPUT_FLAGS,CHANNEL_COMMAND_ALL,"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has changed the banner of the channel.",Article(d->player,UPPER,INDEFINITE),getcname(NOTHING,d->player,0,0));
                                setreturn(OK,COMMAND_SUCC);
			     } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the banner '%s"ANSI_LGREEN"' for the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' is too similar to the banner of the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' ('%s"ANSI_LGREEN"')",ptr,chname,current->name,decompress(current->banner));
			  } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the banner '"ANSI_LWHITE"%s"ANSI_LGREEN"' is invalid.",params);
		       } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of the banner of the channel '"ANSI_LWHITE"%s"ANSI_LGREEN" is %d characters (Including substitutions.)",chname,CHANNEL_BANNER_MAX);
		    } else output(d,d->player,0,1,0,ANSI_LGREEN"Please specify the new banner for the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",chname);
		 } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the settings of the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' have been locked.",chname);
	      } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, only the owner of the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' may change its banner.",chname);
	   } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you are not currently joined to the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",chname);
	} else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.  Please type '"ANSI_LYELLOW"-list"ANSI_LGREEN"' for a list of available channels.",chname);
     } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the banner of the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' can't be changed from within a compound command.",chname);
}

/* ---->  Create new channel  <---- */
void channel_create(CHANNEL_CONTEXT)
{
     struct channel_data *new,*current;
     char   buffer[TEXT_SIZE];
     char   *banner,*title;
     const  char *ptr;
     int    count;

     setreturn(ERROR,COMMAND_FAIL);
     for(current = channels, count = 0; current; current = current->next)
         if(current->owner == d->player) count++;
     if((count < CHANNEL_MAX) || Level4(db[d->player].owner)) {
        if(!Blank(chname)) {
           ansi_code_filter(chname,chname,1);
           for(ptr = chname, count = 0; *ptr && !count; ptr++)
               if(!isalnum(*ptr)) count = 1;

           if(!count) {
              if(!Level3(Owner(d->player)) && !Censor(d->player) && !Censor(Location(d->player)))
                 bad_language_filter(chname,chname);
              if(!strchr(chname,'*')) {
                 *chname = toupper(*chname);
                 if(strlen(chname) <= CHANNEL_NAME_LENGTH) {
                    if(!channel_lookup(chname,NULL,1)) {
                       MALLOC(new,struct channel_data);
                       new->inviterestriction = CHANNEL_COMMAND_OPERATOR;
                       new->sendrestriction   = CHANNEL_COMMAND_ALL;
                       new->accesslevel       = 8;
                       new->permanent         = NULL;
                       new->banner            = (char *) alloc_string(compress(channel_makebanner(chname,CHANNEL_BANNER_LENGTH,buffer,&count),1));
                       new->indent            = count;
                       new->title             = NULL;
                       new->owner             = d->player;
                       new->right             = NULL;
                       new->flags             = CHANNEL_CENSOR;
                       new->left              = NULL;
                       new->motd              = NULL;
                       new->name              = (char *) alloc_string(chname);
                       new->next              = NULL;

                       /* ---->  Create initial user list  <---- */
                       MALLOC(new->userlist,struct channel_user_data);
                       new->userlist->player = d->player;
                       new->userlist->flags |= (CHANNEL_USER_JOINED|CHANNEL_USER_ACTIVE);
                       new->userlist->next   = NULL;

                       /* ---->  Add channel to linked list of channels  <---- */
                       if(channels) {
                          for(current = channels; current && current->next; current = current->next);
                          current->next = new;
		       } else channels = new;

                       /* ---->  Add channel to binary tree  <---- */
                       channel_add(new);

                       /* ---->  Welcome user to new channel  <---- */
                       if(!in_command) channel_output(new,d->player,NOTHING,SEARCH_ALL,CHANNEL_COMMAND_ALL,"New channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' created.",new->name);
                       setreturn(OK,COMMAND_SUCC);
                       chfound = 1;

                       /* ---->  Optionally set banner and title of channel  <---- */
                       split_params(params,&banner,&title);
                       if(!Blank(banner)) channel_banner(d,chname,banner,0);
                       if(!Blank(title))  channel_title(d,chname,title,0);
		    } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' already exists.",chname);
		 } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of a new channel's name is %d characters.",CHANNEL_NAME_LENGTH);
	      } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the name of the new channel must not contain bad language ('"ANSI_LWHITE"%s"ANSI_LGREEN"'.)",chname);
	   } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the name of the new channel must only consist of the letters "ANSI_LWHITE"A"ANSI_LGREEN" - "ANSI_LWHITE"Z"ANSI_LGREEN" and the numbers "ANSI_LWHITE"0"ANSI_LGREEN" - "ANSI_LWHITE"9"ANSI_LGREEN".");
	} else output(d,d->player,0,1,0,ANSI_LGREEN"Please specify the name of the new channel, i.e:  '"ANSI_LWHITE"-mychannel.create"ANSI_LGREEN"' to create a channel named '"ANSI_LWHITE"mychannel"ANSI_LGREEN"'.");
     } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you may only create a maximum of "ANSI_LWHITE"%d"ANSI_LGREEN" temporary channels.",CHANNEL_MAX);
}

/* ---->  Set whether bad language may be used on the channel  <---- */
void channel_censor(CHANNEL_CONTEXT)
{
     struct channel_data *channel;
     int    flags,newflags;

     if(!in_command || Level4(current_command) || (Owner(current_command) == channel->owner)) {
        if((channel = channel_lookup(chname,NULL,0)) && (chname = channel->name)) {
           if((flags = channel_flags(channel,d->player)) & CHANNEL_USER_JOINED) {
              if(!Blank(params)) {
                 if((flags & CHANNEL_USER_OPERATOR) || can_write_to(d->player,channel->owner,0)) {
                    if(!(channel->flags & CHANNEL_LOCKED) || (!(channel->flags & CHANNEL_LOCKED_OWNER) && (flags & CHANNEL_USER_OPERATOR)) || (Owner(d->player) == channel->owner)) {
                       if(!strcasecmp("yes",params) || !strcasecmp("on",params)) newflags = CHANNEL_CENSOR;
                          else if(!strcasecmp("no",params) || !strcasecmp("off",params)) newflags = 0;
                             else {
                                output(d,d->player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LYELLOW"yes"ANSI_LGREEN"' or '"ANSI_LYELLOW"no"ANSI_LGREEN"'.");
                               return;
			     }
                       channel->flags &= ~CHANNEL_CENSOR;
                       channel->flags |=  newflags;
                       if(!in_command) channel_output(channel,NOTHING,NOTHING,CHANNEL_OUTPUT_FLAGS,CHANNEL_COMMAND_ALL,"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has %sallowed the use of bad language on the channel.",Article(d->player,UPPER,INDEFINITE),getcname(NOTHING,d->player,0,0),(newflags) ? "dis":"");
                       channel_show_motd(channel,d->player,channel->flags);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the settings of the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' have been locked%s.",chname);
		 } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, only operators and the owner of the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' may change whether bad language may be used on the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' or not.",chname);
	      } else {
                 channel_show_motd(channel,d->player,channel->flags);
                 output(getdsc(d->player),d->player,0,1,0,ANSI_LGREEN"Bad language may%s be used over the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",(channel->flags & CHANNEL_CENSOR) ? " not":"",chname);
                 setreturn(OK,COMMAND_SUCC);
	      }
	   } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you are not currently joined to the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",chname);
	} else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.  Please type '"ANSI_LYELLOW"-list"ANSI_LGREEN"' for a list of available channels.",chname);
     } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the censorship of bad language on the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' can't be changed from within a compound command.",chname);
}

/* ---->  Destroy channel  <---- */
void channel_destroy(CHANNEL_CONTEXT)
{
}

/* ---->  Set MOTD (Message Of The Day) of channel  <---- */
void channel_motd(CHANNEL_CONTEXT)
{
     struct channel_user_data *current;
     struct channel_data *channel;
     struct descriptor_data *ptr;
     int    flags;

     if(!in_command || Level4(current_command) || (Owner(current_command) == channel->owner)) {
        if((channel = channel_lookup(chname,NULL,0)) && (chname = channel->name)) {
           if((flags = channel_flags(channel,d->player)) & CHANNEL_USER_JOINED) {
              if(!Blank(params)) {
                 if((flags & CHANNEL_USER_OPERATOR) || can_write_to(d->player,channel->owner,0)) {
                    if(!(channel->flags & CHANNEL_LOCKED) || (!(channel->flags & CHANNEL_LOCKED_OWNER) && (flags & CHANNEL_USER_OPERATOR)) || (Owner(d->player) == channel->owner)) {
                       if(strcasecmp("reset",params)) {
                          if(can_write_to(d->player,channel->owner,0) || !instring("%{",params)) {
                             FREENULL(channel->motd);
                              channel->motd = (char *) alloc_string(compress(punctuate(params,2,'.'),0));
                              for(current = channel->userlist; current; current = current->next)
                                  current->flags |= CHANNEL_USER_MOTD;
                              if(!in_command) channel_output(channel,NOTHING,NOTHING,CHANNEL_OUTPUT_FLAGS,CHANNEL_COMMAND_ALL,"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has changed the message of the day (MOTD)...",Article(d->player,UPPER,INDEFINITE),getcname(NOTHING,d->player,0,0));
                              for(ptr = descriptor_list; ptr; ptr = ptr->next)
                              if(Validchar(ptr->player) && (ptr->flags & CONNECTED)) {
                                 for(current = channel->userlist; current && (current->player != ptr->player); current = current->next);
                                 if(current) channel_show_motd(channel,ptr->player,current->flags);
			      }
			  } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, only the owner of the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' may use query substitutions in the message of the day (MOTD.)",chname);
		       } else if(channel->motd) {
                          FREENULL(channel->motd);
                          if(!in_command) output(d,d->player,0,1,0,ANSI_LGREEN"The channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' no-longer has a message of the day (MOTD.)",chname);
		       } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't currently have a message of the day (MOTD.)",chname);
                       setreturn(OK,COMMAND_SUCC);
		    } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the settings of the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' have been locked.",chname);
		 } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, only operators and the owner of the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' may change its message of the day (MOTD.)",chname);
	      } else if(channel->motd) {
                 for(current = channel->userlist; current && (current->player != d->player); current = current->next);
                 if(current) current->flags |= CHANNEL_USER_MOTD;
                 channel_show_motd(channel,d->player,current->flags);
                 setreturn(OK,COMMAND_SUCC);
	      } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' has no message of the day (MOTD.)",chname);
	   } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you are not currently joined to the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",chname);
	} else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.  Please type '"ANSI_LYELLOW"-list"ANSI_LGREEN"' for a list of available channels.",chname);
     } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the message of the day (MOTD) of the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' can't be changed from within a compound command.",chname);
}

/* ---->  Rename channel  <---- */
void channel_rename(CHANNEL_CONTEXT)
{
     struct channel_data *channel,*lookup,*last;
     int    flags,count;
     const  char *ptr;

     if(!in_command || Level4(current_command) || (Owner(current_command) == channel->owner)) {
        if((channel = channel_lookup(chname,&last,0)) && (chname = channel->name)) {
           if((flags = channel_flags(channel,d->player)) & CHANNEL_USER_JOINED) {
              if(can_write_to(d->player,channel->owner,0)) {
                 if(!(channel->flags & CHANNEL_LOCKED) || (!(channel->flags & CHANNEL_LOCKED_OWNER) && (flags & CHANNEL_USER_OPERATOR)) || (Owner(d->player) == channel->owner)) {
                    if(!Blank(params)) {
                       ansi_code_filter(params,params,1);
                       for(ptr = params, count = 0; *ptr && !count; ptr++)
                           if(!isalnum(*ptr)) count = 1;

                       if(!count) {
                          if(!Level3(Owner(d->player)) && !Censor(d->player) && !Censor(Location(d->player)))
                             bad_language_filter(params,params);
                          if(!strchr(params,'*')) {
                             *params = toupper(*params);
                             if(strlen(params) <= CHANNEL_NAME_LENGTH) {
                                if(!(lookup = channel_lookup(params,NULL,1)) || (lookup == channel)) {

                                   /* ---->  Remove channel from binary tree and add with new name  <---- */
                                   channel_remove(channel,last);
                                   FREENULL(channel->name);
                                   channel->name = (char *) alloc_string(params);
                                   channel_add(channel);

                                   channel_show_motd(channel,d->player,flags);
                                   if(!in_command) channel_output(channel,NOTHING,NOTHING,CHANNEL_OUTPUT_FLAGS,CHANNEL_COMMAND_ALL,"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has changed the name of the channel from '"ANSI_LWHITE"%s"ANSI_LGREEN"' to '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",Article(d->player,UPPER,INDEFINITE),getcname(NOTHING,d->player,0,0),chname,params);
                                   setreturn(OK,COMMAND_SUCC);
		                } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, another channel with the name '"ANSI_LWHITE"%s"ANSI_LGREEN"' already exists.",params);
			     } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of the new name of the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' is %d characters.",chname,CHANNEL_NAME_LENGTH);
			  } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the new name of the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' must not contain bad language ('"ANSI_LWHITE"%s"ANSI_LGREEN"'.)",chname,params);
		       } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the new name of the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' must only consist of the letters "ANSI_LWHITE"A"ANSI_LGREEN" - "ANSI_LWHITE"Z"ANSI_LGREEN" and the numbers "ANSI_LWHITE"0"ANSI_LGREEN" - "ANSI_LWHITE"9"ANSI_LGREEN".",chname);
		    } else output(d,d->player,0,1,0,ANSI_LGREEN"Please specify the new name for the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",chname);
		 } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the settings of the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' have been locked.",chname);
	      } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, only the owner of the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' may change its name.",chname);
	   } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you are not currently joined to the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",chname);
	} else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.  Please type '"ANSI_LYELLOW"-list"ANSI_LGREEN"' for a list of available channels.",chname);
     } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' can't be renamed from within a compound command.",chname);
}

/* ---->  Set title of channel  <---- */
void channel_title(CHANNEL_CONTEXT)
{
     struct channel_data *channel;
     int    flags;
     char   *ptr;

     if(!in_command || Level4(current_command) || (Owner(current_command) == channel->owner)) {
        if((channel = channel_lookup(chname,NULL,0)) && (chname = channel->name)) {
           if((flags = channel_flags(channel,d->player)) & CHANNEL_USER_JOINED) {
              if(can_write_to(d->player,channel->owner,0)) {
                 if(!(channel->flags & CHANNEL_LOCKED) || (!(channel->flags & CHANNEL_LOCKED_OWNER) && (flags & CHANNEL_USER_OPERATOR)) || (Owner(d->player) == channel->owner)) {
                    if(!Blank(params)) {
                       if(!strchr(params,'\n')) {
                          if(strlen_ignansi(scratch_buffer) <= CHANNEL_TITLE_LENGTH) {
                             if(strlen(scratch_buffer) <= CHANNEL_TITLE_MAX) {
                                FREENULL(channel->title);
                                channel->title = (char *) alloc_string(compress(ptr = punctuate(params,2,'.'),0));
                                channel_show_motd(channel,d->player,flags);
                                if(channel->flags & CHANNEL_CENSOR) bad_language_filter(ptr,ptr);
                                if(!in_command) channel_output(channel,NOTHING,NOTHING,CHANNEL_OUTPUT_FLAGS,CHANNEL_COMMAND_ALL,"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has changed the title of the channel to '"ANSI_LYELLOW"%s"ANSI_LGREEN"'.",Article(d->player,UPPER,INDEFINITE),getcname(NOTHING,d->player,0,0),substitute(channel->owner,scratch_return_string,ptr,0,ANSI_LYELLOW,NULL,0));
			     } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of the title of the channel '"ANSI_LWHITE"%s"ANSI_LGREEN" is %d characters (Including ANSI colour substitutions.)",chname,CHANNEL_TITLE_MAX);
			  } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of the title of the channel '"ANSI_LWHITE"%s"ANSI_LGREEN" is %d characters (Excluding ANSI colour substitutions.)",chname,CHANNEL_TITLE_LENGTH);
		       } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the title of the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' must not contain embedded NEWLINE's.",params);
		    } else if(channel->title) {
                       FREENULL(channel->title);
                       if(!in_command) output(d,d->player,0,1,0,ANSI_LGREEN"The channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' no-longer has a title.",chname);
		    } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't currently have a title.",chname);
                    setreturn(OK,COMMAND_SUCC);
	         } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the settings of the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' have been locked.",chname);
	      } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, only the owner of the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' may change its title.",chname);
   	   } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you are not currently joined to the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",chname);
        } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.  Please type '"ANSI_LYELLOW"-list"ANSI_LGREEN"' for a list of available channels.",chname);
     } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the title of the channel '"ANSI_LWHITE"%s"ANSI_LGREEN"' can't be changed from within a compound command.",chname);
}

/* ---->      Channel communication      <---- */
/*        (val1:  0 = '-', 1 = 'chat'.)        */
void channel_main(CONTEXT)
{
     struct descriptor_data *d = getdsc(player);
     char   *chname = NULL,*command = NULL;
     struct channel_cmd_table *cmd;
     char   buffer[128];

     setreturn(ERROR,COMMAND_FAIL);
#ifndef BETA
     output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, hard-coded channels are under development and have been disabled.");
     return;
#endif

     if(Connected(player) && d) {
        if(!val1) {

           /* ---->  Channel name  <---- */
           for(chname = params; *params && !((*params == '.') || (*params == ' ')); params++);

           /* ---->  Channel command  <---- */
           if(*params && (*params == '.'))
              for(*params++ = '\0', command = params; *params && (*params != ' '); params++);
           if(*params)
              for(*params++ = '\0'; *params && (*params == ' '); params++);
	}

        /* ---->  No channel name given  <---- */
        if(Blank(chname)) {
           if(d->chname) strncpy(buffer,decompress(d->chname),CHANNEL_NAME_LENGTH);
              else *buffer = '\0';
           chname = buffer;
	}

        /* ---->  Match and execute channel command  <---- */
        chfound = 0;
        if(!Blank(chname)) {
           if(strcasecmp("list",chname)) {
              if(!Blank(command)) {
                 if((cmd = channel_search_cmdtable(command)))
                    cmd->func(d,(char *) chname,(char *) params,cmd->option);
	               else output(d,player,0,1,0,ANSI_LGREEN"Unknown channel command '"ANSI_LWHITE"%s"ANSI_LGREEN"', %s"ANSI_LYELLOW"%s"ANSI_LGREEN".  "ANSI_LBLUE"(Type "ANSI_LCYAN""ANSI_UNDERLINE"HELP CHANNELCMDS"ANSI_LBLUE" for help!)\n",command,Article(d->player,LOWER,INDEFINITE),getcname(NOTHING,d->player,0,0));
	      } else channel_speak(d,chname,params,0);
	   } else channel_list(d,chname,params,0);
	} else output(d,player,0,1,0,ANSI_LGREEN"Sorry, you have not used a channel since you connected to %s.  Please specify the name of the channel you would like to join or use.",tcz_full_name);

        /* ---->  Set name of channel last used by user  <---- */
        if(chfound) {
           if(strlen(chname) > 20) chname[21] = '\0';
           FREENULL(d->chname);
           d->chname = (char *) alloc_string(compress(chname,0));
	}
     } else output(d,player,0,1,0,ANSI_LGREEN"Sorry, you must be connected to use channels.");
}
