/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| COMMUNICATION.C  -  Implements on-line user-to-user communication and       |
|                     interaction.                                            |
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

  $Id: communication.c,v 1.3 2005/06/29 20:45:55 tcz_monster Exp $

*/


#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "descriptor_flags.h"
#include "friend_flags.h"
#include "object_types.h"
#include "objectlists.h"
#include "flagset.h"
#include "fields.h"


char  *session_title = NULL;
dbref session_who    = NOTHING;


/* ---->  Set SPOKEN_TEXT for appropriate descriptor (To prevent misuse of 'lastcommand')  <---- */
void comms_spoken(dbref player,int absolute)
{
     struct descriptor_data *p = getdsc(player);;

     if(in_command && !absolute) return;
     if(p) p->flags |= SPOKEN_TEXT;
}

/* ---->  Enter AFK (Away From Keyboard) mode  <---- */
/*        (VAL1:  0 = Normal 'afk', 1 = 'set afk'.)  */
void comms_afk(CONTEXT)
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
                 if(Validchar(player)) {
		    if(adjustment > 0) {
		       if(!in_command) output(p,player,0,1,0,ANSI_LGREEN"Auto-AFK facility turned "ANSI_LWHITE"on"ANSI_LGREEN".  You will now be automatically sent AFK when you idle for over "ANSI_LYELLOW"%d"ANSI_LGREEN" minute%s.",adjustment,Plural(adjustment));
		       db[player].data->player.afk = adjustment;
		    } else {
		       if(!in_command) output(p,player,0,1,0,ANSI_LGREEN"Auto-AFK facility turned "ANSI_LYELLOW"off"ANSI_LGREEN".  You will no-longer be automatically sent AFK when you idle.");
		       db[player].data->player.afk = 0;
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
        if(Validchar(player) && db[player].data->player.afk)
           output(p,player,0,1,0,ANSI_LGREEN"Auto-AFK facility is "ANSI_LWHITE"on"ANSI_LGREEN"  -  Max. idle time:  "ANSI_LWHITE"%d"ANSI_LGREEN" minute%s.",db[player].data->player.afk,Plural(db[player].data->player.afk));
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
#ifdef QMW_RESEARCH
		     qmwlogsocket("AFK:%d:%d:%d",
				  player, privilege(player,255),
				  db[player].location);
#endif /* #ifdef QMW_RESEARCH */
		  }

              if(!Invisible(db[player].location) && !Quiet(db[player].location))
                 output_except(db[player].location,player,NOTHING,0,1,2,"%s",construct_message(player,ANSI_LWHITE,ANSI_LGREEN,"",'.',1,OTHERS,"will be away from %p keyboard  -  Back soon...",0,DEFINITE));
              setreturn(OK,COMMAND_SUCC);
	   } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you must be connected to go AFK (Away From Keyboard.)");
	} else output(p,player,0,1,0,ANSI_LGREEN"Please specify your AFK (Away From Keyboard) message, e.g:  '"ANSI_LWHITE"afk Gone to get something to eat  -  back soon."ANSI_LGREEN"'.");
     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, Guest characters can't go AFK (Away From Keyboard)  -  Please ask an Apprentice Wizard/Druid or above to create a proper character for you.");
}

/* ---->  Write to entire area  <---- */
void comms_areawrite(CONTEXT)
{
     struct descriptor_data *d,*p = getdsc(player);

     setreturn(ERROR,COMMAND_FAIL);
     if(!(!Level3(db[player].owner) && !can_write_to(player,db[player].location,0))) {
        if(!(Level4(db[player].owner) && !Shout(db[player].owner) && !can_write_to(player,db[player].location,0))) {
           substitute(player,scratch_buffer,(char *) params,0,ANSI_LCYAN,NULL,0);
           for(d = descriptor_list; d; d = d->next)
	       if((d->flags & CONNECTED) && Validchar(d->player) && ((d == p) || (!Quiet(d->player) && !Quiet(db[d->player].location))) && in_area(d->player,db[player].location))
                  output(d,d->player,0,1,0,"%s",scratch_buffer);
           setreturn(OK,COMMAND_SUCC);
	} else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can't write in an area which is owned by someone of the same or higher level than yourself when you've been banned from shouting.");
     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you may only write to the area within a location which you own.");
}

/* ---->  Ask a question  <---- */
void comms_ask(CONTEXT)
{
     command_type |= COMM_CMD;
     setreturn(ERROR,COMMAND_FAIL);
     comms_spoken(player,0);
     if(!Quiet(db[player].location)) {
        if(!Blank(params)) {
           output(getdsc(player),player,0,1,2,"%s",construct_message(player,ANSI_LWHITE,(in_command) ? ANSI_LCYAN:ANSI_DWHITE,"ask",'?',NONE,PLAYER - 2,params,2,DEFINITE));
           output_except(db[player].location,player,NOTHING,0,1,2,"%s",construct_message(player,ANSI_LWHITE,(in_command) ? ANSI_LCYAN:ANSI_DWHITE,"asks",'?',NONE,OTHERS - 2,params,2,DEFINITE));
#ifdef QMW_RESEARCH
	   qmwlogsocket("ASK:%d:%d:%d:%s",
			player, privilege(player,255),
			db[player].location,params);
#endif /* #ifdef QMW_RESEARCH */
           setreturn(OK,COMMAND_SUCC);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"What question would you like to ask?");
     } else if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, this is a quiet %s  -  You can't ask questions in here.",(Typeof(db[player].location) == TYPE_ROOM) ? "room":"container");
     command_type &= ~COMM_CMD;
}

/* ---->  Sound beep  <---- */
void comms_beep(CONTEXT)
{
     setreturn(OK,COMMAND_SUCC);
     output(getdsc(player),player,2,1,0,"\x07");
}

/* ---->  Censor user-typed command from the 'lastcommands' list  <---- */
void comms_censor(CONTEXT)
{
     struct descriptor_data *p = getdsc(player);

     if(in_command) {
        if(Level4(Owner(player)) || (Valid(current_command) && Apprentice(current_command))) {
           if(p) {
              if(p->edit) p->flags |= ABSOLUTE;
              p->flags             |= SPOKEN_TEXT;
	   }
	} else output(p,player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@censor"ANSI_LGREEN"' may only be used by Apprentice Wizards/Druids and above.");
     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@censor"ANSI_LGREEN"' may only be used from within a compound command.");
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Find operator of a given channel  <---- */
dbref comms_chat_operator(int channel)
{
      struct descriptor_data *d;

      if(channel < 0) return(NOTHING);
      for(d = descriptor_list; d; d = d->next)
          if((d->channel == channel) && (db[d->player].flags2 & CHAT_OPERATOR))
             return(d->player);
      return(NOTHING);
}

/* ---->  Ensure channel has an operator  <---- */
void comms_chat_check(struct descriptor_data *op,const char *nameptr)
{
     struct descriptor_data *d = NULL;

     if(!op || !Validchar(op->player) || !(op->flags & CONNECTED)) return;
     for(d = descriptor_list; d; d = d->next)
         if((d->channel == op->channel) && (db[d->player].flags2 & CHAT_OPERATOR)) return;

     db[op->player].flags2 |=  CHAT_OPERATOR;
     db[op->player].flags2 &= ~CHAT_PRIVATE;
     output(op,op->player,0,1,8,ANSI_LCYAN"[CHAT] \016&nbsp;\016 "ANSI_DCYAN"("ANSI_LGREEN"%s"ANSI_DCYAN") \016&nbsp;\016 "ANSI_LGREEN"You're now the Channel Operator of this chatting channel.",nameptr,op->channel);
}

/* ---->  Get name of chatting channel  <---- */
const char *comms_chat_channelname(int channel)
{
      struct descriptor_data *d;
      static char name[256];

      if(channel < 0) return("<UNKNOWN>");
      for(d = descriptor_list; d && !((d->channel == channel) && Validchar(d->player) && (db[d->player].flags2 & CHAT_OPERATOR)); d = d->next);
      
      if(d && !Blank(d->subject)) sprintf(name,"%d"ANSI_DCYAN":"ANSI_LYELLOW"%s",d->channel,d->subject);
         else sprintf(name,"%d",(d) ? d->channel:channel);
      return(name);
}

/* ---->  Chatting channels  <---- */
void comms_chat(CONTEXT)
{
     struct descriptor_data *d = getdsc(player);
     const  char *p1,*nameptr;
     char   *p2;

     comms_spoken(player,1);
     setreturn(ERROR,COMMAND_FAIL);
     if(!d) return;

     /* ---->  List active chatting channels  <---- */
     if(!Blank(params) && (!strcasecmp(params,"list") || !strncasecmp(params,"list ",5))) {
        for(; *params && (*params != ' '); params++);
        for(; *params && (*params == ' '); params++);
        userlist_view(player,(char *) params,NULL,NULL,NULL,9,0);
        return;
     }

     /* ---->  Morons can't use chatting channels  <---- */
     if(Moron(player)) {
        if(db[player].flags2 & CHAT_OPERATOR) db[player].flags2 &= ~CHAT_OPERATOR;
        output(d,player,0,1,0,ANSI_LGREEN"Sorry, Morons can't use the chatting channels.");
        return;
     }

     /* ---->  No command/message specified?  <---- */
     if(Blank(params)) {
        if(d->channel != NOTHING) output(d,player,0,1,0,ANSI_LGREEN"What would you like to say over channel "ANSI_LWHITE"%d"ANSI_LGREEN"?",d->channel);
	   else output(d,player,0,1,0,ANSI_LGREEN"Please type '"ANSI_LYELLOW"chat <CHANNEL NUMBER>"ANSI_LGREEN"' if you'd like to join a chatting channel (E.g:  '"ANSI_LWHITE"chat 0"ANSI_LGREEN"' to join the default channel.)");
        return;
     }

     /* ---->  Grab command word from PARAMS  <---- */
     for(p1 = params; *p1 && (*p1 == ' '); p1++);
     for(p2 = scratch_buffer; *p1 && (*p1 != ' '); *p2++ = *p1, p1++);
     for(*p2 = '\0'; *p1 && (*p1 == ' '); p1++);

     /* ---->  Join a chatting channel?  <---- */
     if((isdigit(*params) && !*p1) || string_prefix("on",params) || string_prefix("default",params) || string_prefix("join",scratch_buffer)) {
        struct descriptor_data *ptr;
        int   newchannel;
        dbref operator;

        if(string_prefix("join",scratch_buffer)) {
           newchannel = atol(p1);
           if(!newchannel && (*p1 != '0')) {
              newchannel = NOTHING;
              for(ptr = descriptor_list; ptr; ptr = ptr->next)
                  if(Validchar(ptr->player) && (db[ptr->player].flags2 & CHAT_OPERATOR))
                     if(!Blank(ptr->subject) && string_prefix(ptr->subject,p1)) newchannel = ptr->channel;
			
              if(newchannel == NOTHING) {
                 output(d,player,0,1,0,ANSI_LGREEN"Sorry, there isn't a chatting channel for the subject '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",p1);
                 return;
	      }
	   }
	} else newchannel = atol(params);

        if(newchannel != d->channel) {
	   if(newchannel >= 0) {
              operator = comms_chat_operator(newchannel);
              if(!(Valid(operator) && (db[operator].flags2 & CHAT_PRIVATE))) {
                 if(d->channel != NOTHING) {
                    nameptr = comms_chat_channelname(d->channel);
                    output_chat(d->channel,d->player,0,1,ANSI_LCYAN"[CHAT] \016&nbsp;\016 "ANSI_DCYAN"("ANSI_LGREEN"%s"ANSI_DCYAN") \016&nbsp;\016 "ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has left the channel.",nameptr,Article(d->player,UPPER,DEFINITE),getcname(NOTHING,d->player,0,0));
		 }
                 nameptr = comms_chat_channelname(newchannel);
                 output_chat(newchannel,d->player,0,1,ANSI_LCYAN"[CHAT] \016&nbsp;\016 "ANSI_DCYAN"("ANSI_LGREEN"%s"ANSI_DCYAN") \016&nbsp;\016 "ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has joined the channel.",nameptr,Article(d->player,UPPER,INDEFINITE),getcname(NOTHING,d->player,0,0));
                 output(d,player,0,1,0,ANSI_LGREEN"Welcome to chatting channel "ANSI_LWHITE"%s"ANSI_LGREEN", %s"ANSI_LYELLOW"%s"ANSI_LGREEN".",nameptr,Article(d->player,LOWER,DEFINITE),getcname(NOTHING,d->player,0,0));
                 d->channel = newchannel;

                 if(!Validchar(operator)) {
                    db[d->player].flags2 |=  CHAT_OPERATOR;
                    db[d->player].flags2 &= ~CHAT_PRIVATE;
		 } else db[d->player].flags2 &= ~(CHAT_OPERATOR|CHAT_PRIVATE);
     	         setreturn(OK,COMMAND_SUCC);
	      } else output(d,player,0,1,0,ANSI_LGREEN"Sorry, the operator of that chatting channel (%s"ANSI_LWHITE"%s"ANSI_LGREEN") has made it private.",Article(operator,UPPER,INDEFINITE),getcname(NOTHING,operator,0,0));
	   } else output(d,player,0,1,0,ANSI_LGREEN"Sorry, you can't join a negative chatting channel.");
	} else output(d,player,0,1,0,ANSI_LGREEN"Sorry, you're already on that chatting channel.");
        return;
     }
	   
     if(d->channel == NOTHING) {
        output(d,player,0,1,0,ANSI_LGREEN"Sorry, you haven't joined a chatting channel yet.  (Type '"ANSI_LYELLOW"chat <CHANNEL NUMBER>"ANSI_LGREEN"' or '"ANSI_LYELLOW"chat join <SUBJECT>"ANSI_LGREEN"' to join one, e.g:  '"ANSI_LWHITE"chat 0"ANSI_LGREEN"' to join the default channel.)");
        return;
     }

     nameptr = comms_chat_channelname(d->channel);
     comms_chat_check(d,nameptr);
     if(string_prefix("off",params)) {

        /* ---->  Leave chatting channel  <---- */   
        output_chat(d->channel,d->player,0,1,ANSI_LCYAN"[CHAT] \016&nbsp;\016 "ANSI_DCYAN"("ANSI_LGREEN"%s"ANSI_DCYAN") \016&nbsp;\016 "ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has left the channel.",nameptr,Article(d->player,UPPER,DEFINITE),getcname(NOTHING,d->player,0,0));
        if(!in_command) output(d,player,0,1,0,ANSI_LGREEN"You're no-longer joined to a chatting channel.");
        db[d->player].flags2 &= ~(CHAT_OPERATOR|CHAT_PRIVATE);
        d->channel = NOTHING;
     } else if(string_prefix("private",params) || string_prefix("lock",params)) {

        /* ---->  Make channel private (Operator only)  <---- */
        dbref operator = comms_chat_operator(d->channel);
        if(!((player != operator) && (level(db[d->player].owner) <= level(db[operator].owner)))) {
	   if(!(db[operator].flags2 & CHAT_PRIVATE)) {
	      if(d->channel != 0) {
                 output_chat(d->channel,NOTHING,0,1,ANSI_LCYAN"[CHAT] \016&nbsp;\016 "ANSI_DCYAN"("ANSI_LGREEN"%s"ANSI_DCYAN") \016&nbsp;\016 "ANSI_LGREEN"This chatting channel is now private.",nameptr);
                 db[operator].flags2 |= CHAT_PRIVATE;
	      } else output(d,player,0,1,0,ANSI_LGREEN"Sorry, you can't make the default chatting channel private.");
	   } else output(d,player,0,1,0,ANSI_LGREEN"Sorry, this chatting channel is already private.");
	} else output(d,player,0,1,0,ANSI_LGREEN"Sorry, you aren't the operator of this chatting channel (The operator of a chatting channel is the first person who joined it.)");
     } else if(string_prefix("public",params) || string_prefix("unlock",params)) {

        /* ---->  Make channel public (Operator only)  <---- */
        dbref operator = comms_chat_operator(d->channel);
        if(!((player != operator) && (level(db[d->player].owner) <= level(db[operator].owner)))) {
	   if(db[operator].flags2 & CHAT_PRIVATE) {
              output_chat(d->channel,NOTHING,0,1,ANSI_LCYAN"[CHAT] \016&nbsp;\016 "ANSI_DCYAN"("ANSI_LGREEN"%s"ANSI_DCYAN") \016&nbsp;\016 "ANSI_LGREEN"This chatting channel is now public.",nameptr);
              db[operator].flags2 &= ~CHAT_PRIVATE;
	   } else output(d,player,0,1,0,ANSI_LGREEN"This chatting channel is already public.");
	} else output(d,player,0,1,0,ANSI_LGREEN"Sorry, you aren't the operator of this chatting channel (The operator of a chatting channel is the first person who joined it.)");
     } else if(string_prefix("kick",scratch_buffer) || string_prefix("junkit",scratch_buffer)) {

        /* ---->  Kick user off current channel (Operator only)  <---- */
        dbref  operator = comms_chat_operator(d->channel);
        struct descriptor_data *ptr;
        dbref  victim;

        if(!((d->channel == 0) && !Level4(db[d->player].owner))) {
           if(!((player != operator) && !Level4(db[d->player].owner))) {
              if((victim = lookup_character(d->player,p1,1)) != NOTHING) {
                 for(ptr = descriptor_list; ptr && ((ptr->player != victim) || (ptr->channel != d->channel)); ptr = ptr->next);
	         if(ptr && Connected(victim)) {
	            if(victim != player) {
	               if(ptr->channel == d->channel) {
	                  if(!(Level4(db[victim].owner) && (level(db[d->player].owner) <= level(db[victim].owner)))) {
                             sprintf(scratch_buffer,ANSI_LCYAN"[CHAT] \016&nbsp;\016 "ANSI_DCYAN"("ANSI_LGREEN"%s"ANSI_DCYAN") \016&nbsp;\016 "ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has been kicked off the channel by ",nameptr,Article(ptr->player,UPPER,DEFINITE),getcname(NOTHING,ptr->player,0,0));
                             output_chat(d->channel,NOTHING,0,1,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(d->player,LOWER,DEFINITE),getcname(NOTHING,d->player,0,0));
                             output(getdsc(victim),victim,0,1,8,ANSI_LCYAN"[CHAT] \016&nbsp;\016 "ANSI_DCYAN"("ANSI_LGREEN"%s"ANSI_DCYAN") \016&nbsp;\016 "ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has kicked you off the channel.",nameptr,Article(d->player,UPPER,DEFINITE),getcname(NOTHING,d->player,0,0));

                             db[victim].flags2 &= ~(CHAT_OPERATOR|CHAT_PRIVATE);
                             ptr->channel = NOTHING;
                             FREENULL(ptr->subject);
                             comms_chat_check(d,nameptr);
			  } else output(d,player,0,1,0,ANSI_LGREEN"Sorry, you can't kick a higher level character off this chatting channel.");
		       } else output(d,player,0,1,0,ANSI_LGREEN"Sorry, you can only kick users off this chatting channel.");
		    } else output(d,player,0,1,0,ANSI_LGREEN"Sorry, you can't kick yourself off the chatting channel.");
		 } else output(d,player,0,1,0,ANSI_LGREEN"Sorry, you can only kick someone who's connected off the chatting channel.");
	      } else output(d,player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",p1);
	   } else output(d,player,0,1,0,ANSI_LGREEN"Sorry, you aren't the operator of this chatting channel (The operator of a chatting channel is the first person who joined it.)");
	} else output(d,player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above can kick people off the default chatting channel.");
     } else if(string_prefix("who",scratch_buffer) || string_prefix("users",scratch_buffer) || string_prefix("userlist",scratch_buffer) || string_prefix("swho",scratch_buffer) || string_prefix("shortwho",scratch_buffer)) {

        /* ---->  List users on current channel  <---- */
        userlist_view(d->player,(char *) p1,NULL,NULL,NULL,8,0);
        return;
     } else if(string_prefix("list",scratch_buffer)) {

        /* ---->  List all currently active chatting channels and their operators  <---- */
        userlist_view(d->player,(char *) p1,NULL,NULL,NULL,9,0);
        return;
     } else if(string_prefix("subject",scratch_buffer)) {
        dbref  operator = comms_chat_operator(d->channel);
        struct descriptor_data *ptr;

        /* ---->  Give chatting channel a topic (Operator only)  <---- */
        p1 = punctuate((char *) p1,1,'.');
        if(!((player != operator) && (level(db[d->player].owner) <= level(db[operator].owner)))) {
	   if(d->channel != 0) {
              if(!strchr(p1,'\n') && (strlen(p1) <= 34)) {
                 for(ptr = descriptor_list; ptr && (ptr->player != operator); ptr = ptr->next);
                 if(ptr) {
                    FREENULL(ptr->subject);
                    if(!Censor(player) && !Censor(db[player].location)) bad_language_filter(scratch_return_string,p1);
                       else strcpy(scratch_return_string,p1);
                    ptr->subject = (char *) alloc_string(scratch_return_string);
                    nameptr = comms_chat_channelname(d->channel);
                    if(ptr->subject) output_chat(d->channel,NOTHING,0,1,ANSI_LCYAN"[CHAT] \016&nbsp;\016 "ANSI_DCYAN"("ANSI_LGREEN"%s"ANSI_DCYAN") \016&nbsp;\016 "ANSI_LGREEN"The subject of this chatting channel is now '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",nameptr,scratch_return_string);
                       else output_chat(d->channel,NOTHING,0,1,ANSI_LCYAN"[CHAT] \016&nbsp;\016 "ANSI_DCYAN"("ANSI_LGREEN"%s"ANSI_DCYAN") \016&nbsp;\016 "ANSI_LGREEN"This chatting channel no-longer has a subject.",nameptr);
		 } else output(d,player,0,1,0,ANSI_LGREEN"Sorry, this chatting channel doesn't presently have an operator  -  Its subject can't be changed.");
	      } else output(d,player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of the subject of a chatting channel is 34 characters.  It also must not contain embedded NEWLINE's.");
	   } else output(d,player,0,1,0,ANSI_LGREEN"Sorry, you can't change the subject of the default chatting channel.");
	} else output(d,player,0,1,0,ANSI_LGREEN"Sorry, you aren't the operator of this chatting channel (The operator of a chatting channel is the first person who joined it.)");
     } else if(!Blank(params)) {

        /* ---->  Say/pose/think something over the current channel  <---- */
        command_type |= COMM_CMD;
        strcpy(scratch_return_string,construct_message(player,ANSI_LWHITE,ANSI_DWHITE,"say",'.',-1,PLAYER,params,0,DEFINITE));
        output(d,player,0,1,8,ANSI_LCYAN"[CHAT] \016&nbsp;\016 "ANSI_DCYAN"("ANSI_LGREEN"%s"ANSI_DCYAN") \016&nbsp;\016 %s",nameptr,scratch_return_string);
        strcpy(scratch_return_string,construct_message(player,ANSI_LWHITE,ANSI_DWHITE,"says",'.',-1,OTHERS,params,0,INDEFINITE));
        output_chat(d->channel,d->player,0,1,ANSI_LCYAN"[CHAT] \016&nbsp;\016 "ANSI_DCYAN"("ANSI_LGREEN"%s"ANSI_DCYAN") \016&nbsp;\016 %s",nameptr,scratch_return_string);
        command_type &= ~COMM_CMD;
     } else output(d,player,0,1,0,ANSI_LGREEN"What would you like to say over chatting channel "ANSI_LWHITE"%s"ANSI_LGREEN"?",nameptr);
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Enter/leave 'converse' mode  <---- */
void comms_converse(CONTEXT)
{
     struct   descriptor_data *d = getdsc(player);
     unsigned char converse;

     setreturn(ERROR,COMMAND_FAIL);
     if(!d) return;

     converse = !(d->flags & CONVERSE);
     if((converse = !(d->flags & CONVERSE))) d->flags |= CONVERSE;
        else d->flags &= ~CONVERSE;

     if(converse) output(d,player,0,1,0,ANSI_LGREEN"\nYou are now in '"ANSI_LWHITE"converse"ANSI_LGREEN"' mode  -  To speak to other users in the same room, simply type your message and then press "ANSI_LYELLOW"RETURN"ANSI_LGREEN".\n\nYou can leave '"ANSI_LWHITE"converse"ANSI_LGREEN"' mode at any time by typing "ANSI_LYELLOW"CONVERSE"ANSI_LGREEN" again.  Emoting ("ANSI_LWHITE":"ANSI_LGREEN") and thinking ("ANSI_LWHITE"+"ANSI_LGREEN") can still be done in the usual way.\n\nYou can execute any %s command by typing '"ANSI_LYELLOW".<COMMAND>"ANSI_LGREEN"', e.g:  To tell a message to another user, simply type '"ANSI_LWHITE".tell <NAME> [=] message"ANSI_LGREEN"'.\n",tcz_short_name);
        else output(d,player,0,1,0,ANSI_LGREEN"\nYou are no-longer in '"ANSI_LWHITE"converse"ANSI_LGREEN"' mode  -  To speak to other users in the same room, you will need to use a "ANSI_LYELLOW"\""ANSI_LGREEN" or "ANSI_LYELLOW"'"ANSI_LGREEN" in front of your message, e.g:  '"ANSI_LWHITE"\"hello everyone"ANSI_LGREEN"', '"ANSI_LWHITE"'hello everyone"ANSI_LGREEN"', etc.\n\nYou can enter '"ANSI_LWHITE"converse"ANSI_LGREEN"' mode again at any time by typing "ANSI_LYELLOW"CONVERSE"ANSI_LGREEN".\n");
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Echo message to character's screen  <---- */
void comms_echo(CONTEXT)
{
     command_type |=  LEADING_BACKGROUND;
     output(getdsc(player),player,0,1,0,"%s",substitute(player,scratch_buffer,(char *) params,0,ANSI_LCYAN,NULL,0));
     command_type &= ~LEADING_BACKGROUND;
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Echo message to character's screen in list form  <---- */
void comms_echolist(CONTEXT)
{
     int  item = atol(arg1);
     char buffer[16];

     if(item < 0) return;     
     substitute(player,scratch_return_string,(char *) arg2,0,ANSI_LWHITE,NULL,0);
     if(item < 10000) sprintf(buffer,"(%d)",item);
        else strcpy(buffer,"(10k+)");
     output(getdsc(player),player,0,1,8,ANSI_LGREEN" %-7s\016&nbsp;\016"ANSI_LWHITE"%s",buffer,scratch_return_string);
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Start emergency command logging  <---- */
void comms_emergency(CONTEXT)
{
     struct descriptor_data *d,*p;
     dbref  user;
     time_t now;

     gettime(now);
     setreturn(ERROR,COMMAND_FAIL);
     if(!in_command) {
        if(!Moron(db[player].owner)) {
           if(!Blank(arg1)) {
              if((user = lookup_character(player,arg1,1)) != NOTHING) {
	         if(user != player) {
		    if(!Level4(user)) {
               	       if(!Blank(arg2)) {
     	                  if((d = getdsc(user)) && (d->flags & CONNECTED)) {
		   	     if(d->emergency_time <= now) {
			        command_type |= COMM_CMD;
                                for(p = descriptor_list; p; p = p->next)
                                    if(p->player == user)
     			               p->emergency_time = (now + (EMERGENCY_TIME * MINUTE));
			        sprintf(scratch_buffer,"%s"ANSI_LMAGENTA"%s"ANSI_LWHITE,Article(user,LOWER,INDEFINITE),getcname(NOTHING,user,0,0));
			        output_admin(0,0,1,13,ANSI_LRED"["ANSI_UNDERLINE"EMERGENCY"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"Emergency command logging started by %s"ANSI_LYELLOW"%s"ANSI_LWHITE" on %s  "ANSI_DRED"-  "ANSI_LRED"REASON:  "ANSI_LCYAN"%s",Article(player,LOWER,INDEFINITE),getcname(NOTHING,player,0,0),scratch_buffer,substitute(player,scratch_return_string,params = punctuate(arg2,1,'.'),0,ANSI_LCYAN,NULL,0));
			        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Emergency command logging started on %s"ANSI_LWHITE"%s"ANSI_LGREEN"  -  All commands typed by %s will be logged to the '"ANSI_LWHITE"Emergency"ANSI_LGREEN"' log file for the next "ANSI_LYELLOW"%d"ANSI_LGREEN" minutes.",Article(user,LOWER,DEFINITE),getcname(NOTHING,user,0,0),Objective(user,0),EMERGENCY_TIME);
			        writelog(ADMIN_LOG,1,"EMERGENCY","Emergency command logging started by %s(#%d) on %s(#%d)  -  REASON:  %s",getname(player),player,getname(user),user,arg2);
			        writelog(EMERGENCY_LOG,1,"EMERGENCY","Emergency command logging started by %s(#%d) on %s(#%d)  -  REASON:  %s",getname(player),player,getname(user),user,arg2);
			        writelog(COMPLAINT_LOG,1,"EMERGENCY","Emergency command logging started by %s(#%d) on %s(#%d)  -  REASON:  %s",getname(player),player,getname(user),user,arg2);
			        command_type &= ~COMM_CMD;
			        setreturn(OK,COMMAND_SUCC);
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, emergency command logging has already been started on %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(user,LOWER,DEFINITE),getcname(NOTHING,user,0,0));
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" isn't connected.",Article(user,LOWER,DEFINITE),getcname(NOTHING,user,0,0));
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify a reason for starting emergency command logging on %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(user,LOWER,DEFINITE),getcname(NOTHING,user,0,0));
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you cannot start emergency command logging on an Apprentice Wizard/Druid or above.  If you wish to make a complaint about an administrator, please use the '"ANSI_LYELLOW"complain"ANSI_LGREEN"' command and/or send an E-mail to "ANSI_LWHITE"%s"ANSI_LGREEN" with full details.",tcz_admin_email);
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't start emergency command logging on yourself.");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify who you would like to start emergency command logging on.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Morons aren't allowed to start emergency command logging.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't start emergency command logging from within a compound command.");
}

/* ---->  {J.P.Boggis 28/05/1999}  Make complaint, comment, suggestion or bug report  <---- */
/*        VAL1:  0 = Complaint (Ex. 'gripe')  -  'Complaints' log file  */
/*               1 = Comment     -  'Comments' log file                 */
/*               2 = Suggestion  -  'Suggestions' log file              */
/*               3 = Bug Report  -  'Bugs' log file                     */
void comms_comment(CONTEXT)
{
     static dbref comment_who = NOTHING;
     static time_t comment_time = 0;
     char   buffer[TEXT_SIZE];
     const  char *ptr;
     char   *tmp;
     time_t now;

     setreturn(ERROR,COMMAND_FAIL);
     gettime(now);
     if(!in_command) {
        if(!Blank(params)) {
           if(!(!Level4(player) && (comment_who == player) && (now < comment_time))) {
              command_type |= COMM_CMD;
              comment_time = now + COMMENT_TIME, comment_who = player;
              for(tmp = params; *tmp; tmp++)
                  if(*tmp == '\n') *tmp = ' ';
              ansi_code_filter((char *) params,(char *) params,1);
              sprintf(scratch_buffer,ANSI_LRED"["ANSI_UNDERLINE"%s"ANSI_LRED"] \016&nbsp;\016 "ANSI_LYELLOW"(From %s"ANSI_LWHITE"%s"ANSI_LYELLOW" in ",(val1 == 1) ? "COMMENT":(val1 == 2) ? "SUGGESTION":(val1 == 3) ? "BUG REPORT":"COMPLAINT",Article(player,LOWER,INDEFINITE),getcname(NOTHING,player,0,0));
              sprintf(scratch_buffer + strlen(scratch_buffer),"%s"ANSI_LWHITE"%s"ANSI_LYELLOW") \016&nbsp;\016 "ANSI_LWHITE,Article(db[player].location,LOWER,INDEFINITE),unparse_object(ROOT,db[player].location,0));
              output_admin(0,0,1,(val1 == 1) ? 11:(val1 == 2) ? 14:(val1 == 3) ? 14:13,"%s%s",scratch_buffer,ptr = punctuate((char *) params,2,'.'));

	      switch(val1) {
		     case 1:
                          strcpy(buffer,ANSI_LGREEN"Your comments have been noted.");
                          break;
                     case 2:
                          sprintf(buffer,ANSI_LGREEN"Your suggestion has been logged.  Thank you for your feedback, which will help us to improve %s for yourself and other users.",tcz_full_name);
                          break;
                     case 3:
                          strcpy(buffer,ANSI_LGREEN"Thank you for your bug report.  We will look into the problem and resolve it as soon as possible.");
                          break;
                     default:
                          strcpy(buffer,ANSI_LGREEN"Your complaint has been logged and will be dealt with as soon as possible.");
	      }
              output(getdsc(player),player,0,1,0,buffer);

              writelog((val1 == 1) ? COMMENT_LOG:(val1 == 2) ? SUGGESTION_LOG:(val1 == 3) ? BUG_LOG:COMPLAINT_LOG,1,(val1 == 1) ? "COMMENT":(val1 == 2) ? "SUGGESTION":(val1 == 3) ? "BUG REPORT":"COMPLAINT","(From %s(#%d) in %s(#%d))  %s",getname(player),player,getname(db[player].location),db[player].location,ptr);
              command_type &= ~COMM_CMD;
              setreturn(OK,COMMAND_SUCC);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please wait "ANSI_LWHITE"%s"ANSI_LGREEN" before %s.",interval(comment_time - now,comment_time - now,ENTITIES,0),(val1 == 1) ? "making another comment":(val1 == 2) ? "making another suggestion":(val1 == 3) ? "reporting another bug":"making another complaint");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s?",(val1 == 1) ? "What comment would you like to make":(val1 == 2) ? "What suggestion do you wish to make":(val1 == 3) ? "Which bug do you wish to report":"What would you like to make a complaint about");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't make a %s from within a compound command.",(val1 == 1) ? "comment":(val1 == 2) ? "suggestion":(val1 == 3) ? "bug report":"complaint");
}

/* ---->  Notify character with a message  <---- */
void comms_notify(CONTEXT)
{
     dbref recipient;

     setreturn(ERROR,COMMAND_FAIL);
     if(!((flow_control & FLOW_WITH_CONNECTED) && !(in_command && Wizard(current_command)))) {
        if(!((flow_control & FLOW_WITH_FRIENDS) && !(in_command && (Wizard(current_command) || Apprentice(current_command))))) {
           if((recipient = lookup_character(player,arg1,1)) != NOTHING) {
              if(Level3(db[player].owner) || (in_command && (Wizard(current_command) || Apprentice(current_command))) || (db[player].owner == recipient) || (db[player].owner == Controller(recipient)) || can_write_to(player,db[recipient].location,0)) {
                 if((player == recipient) || (!Quiet(recipient) && !Quiet(db[recipient].location)) || (level(db[player].owner) > level(recipient))) {
                    command_type |=  LEADING_BACKGROUND;
                    output(getdsc(recipient),recipient,0,1,0,"%s",substitute(recipient,scratch_buffer,(char *) arg2,0,ANSI_LCYAN,NULL,player));
                    command_type &= ~LEADING_BACKGROUND;
                    if(!in_command) {
                       if(player != recipient)
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"You notify %s"ANSI_LWHITE"%s"ANSI_LGREEN" with the message '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",Article(recipient,LOWER,DEFINITE),getcname(NOTHING,recipient,0,0),arg2);
                             else output(getdsc(player),player,0,1,0,ANSI_LGREEN"You notify yourself with the message '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",arg2);
		    }

                    if(Level4(db[player].owner) && !Shout(db[player].owner) && !(flow_control & FLOW_WITH_BANISHED)) {
                       writelog(SHOUT_LOG,1,"SHOUT","%s(#%d) using '@with connected':  %s",getname(player),player,String(current_cmdptr));
                       flow_control |= FLOW_WITH_BANISHED;
		    }
                    setreturn(OK,COMMAND_SUCC);
		 } else if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that character is either set "ANSI_LYELLOW"QUIET"ANSI_LGREEN" or is in a "ANSI_LYELLOW"QUIET"ANSI_LGREEN" location.");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only notify yourself, one of your puppets or a character who is on your own property.");
	   } else if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
	} else if(!(flow_control & FLOW_WITH_BANISHED)) {

           /* ---->  '@notify' within '@with friends do...'  <---- */
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@with friends/enemies do..."ANSI_LGREEN"' can't be used to notify all of your friends/enemies  -  Please use '"ANSI_LYELLOW"page friends|enemies [=] <MESSAGE>"ANSI_LGREEN"' or '"ANSI_LYELLOW"tell friends|enemies [=] <MESSAGE>"ANSI_LGREEN"' instead.");
           flow_control |= FLOW_WITH_BANISHED;
	}
     } else if(Level4(db[player].owner) && !Shout(db[player].owner) && !(flow_control & FLOW_WITH_BANISHED)) {

        /* ---->  '@notify' within '@with connected...'  <---- */
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you've been banned from shouting  -  You can't notify everyone connected using '"ANSI_LWHITE"@with connected..."ANSI_LGREEN"'.");
        flow_control |= FLOW_WITH_BANISHED;
     }
}

/* ---->  Echo message to everyone in same room's screen (Except you)  <---- */
void comms_oecho(CONTEXT)
{
    struct descriptor_data *d;

     setreturn(ERROR,COMMAND_FAIL);
     if(Level3(db[player].owner) || (in_command && (Wizard(current_command) || Apprentice(current_command))) || can_write_to(player,db[player].location,0)) {
        if(!Quiet(db[player].location)) {
           command_type |=  LEADING_BACKGROUND;
	   for (d = descriptor_list; d; d = d->next) {
	       if ((d->flags & CONNECTED) && Validchar(d->player) && (db[d->player].location == db[player].location) && (d->player != player)) {
		   output(d,d->player,0,1,0,"%s", (char *) substitute(d->player,scratch_buffer,(char *) params,0,ANSI_LCYAN,NULL,player));
	       }
	   }
           /* output_except(db[player].location,player,NOTHING,0,1,0,"%s",substitute(player,scratch_buffer,(char *) params,0,ANSI_LCYAN,NULL,0)); */
           command_type &= ~LEADING_BACKGROUND;
           setreturn(OK,COMMAND_SUCC);
	} else if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, this is a quiet %s  -  You can't '"ANSI_LWHITE"@oecho"ANSI_LGREEN"' in here.",(Typeof(db[player].location) == TYPE_ROOM) ? "room":"container");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only '"ANSI_LWHITE"@oecho"ANSI_LGREEN"' messages in locations you own.");
}

/* ---->  Emote message to everyone in the same room as you (Except you)  <---- */
void comms_oemote(CONTEXT)
{
     setreturn(ERROR,COMMAND_FAIL);
     comms_spoken(player,0);
     if(!Quiet(db[player].location)) {
        output_except(db[player].location,player,NOTHING,0,1,2,"%s",construct_message(player,ANSI_LWHITE,(in_command) ? ANSI_LCYAN:ANSI_DWHITE,"",'.',1,OTHERS,params,2,DEFINITE));
        setreturn(OK,COMMAND_SUCC);
     } else if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, this is a quiet %s  -  You can't '"ANSI_LWHITE"@oemote"ANSI_LGREEN"' in here.",(Typeof(db[player].location) == TYPE_ROOM) ? "room":"container");
}

/* ---->  Pose (Emote)  <---- */
void comms_pose(CONTEXT)
{
     setreturn(ERROR,COMMAND_FAIL);
     comms_spoken(player,0);
     command_type |= COMM_CMD;
     if(!Quiet(db[player].location)) {
        if(!Blank(params)) {
           output_except(Location(player),NOTHING,NOTHING,0,1,2,"%s",construct_message(player,ANSI_LWHITE,(in_command) ? ANSI_LCYAN:ANSI_DWHITE,"",'.',1,OTHERS,params,2,DEFINITE));
#ifdef QMW_RESEARCH
	   qmwlogsocket("EMOTE:%d:%d:%d:%s",
			player, privilege(player,255),
			db[player].location,params);
#endif /* #ifdef QMW_RESEARCH */
           setreturn(OK,COMMAND_SUCC);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"What action/pose would you like to do?");
     } else if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, this is a quiet %s  -  You can't emote in here.",(Typeof(db[player].location) == TYPE_ROOM) ? "room":"container");
     command_type &= ~COMM_CMD;
}

/* ---->  Say something  <---- */
void comms_say(CONTEXT)
{
     setreturn(ERROR,COMMAND_FAIL);
     comms_spoken(player,0);
     command_type |= COMM_CMD;
     if(!Quiet(db[player].location)) {
        if(!Blank(params)) {
           output(getdsc(player),player,0,1,2,"%s",construct_message(player,ANSI_LWHITE,(in_command) ? ANSI_LCYAN:ANSI_DWHITE,"",'.',NONE,PLAYER,params,2,DEFINITE));
           output_except(db[player].location,player,NOTHING,0,1,2,"%s",construct_message(player,ANSI_LWHITE,(in_command) ? ANSI_LCYAN:ANSI_DWHITE,"",'.',NONE,OTHERS,params,2,DEFINITE));
#ifdef QMW_RESEARCH
	   qmwlogsocket("SAY:%d:%d:%d:%s",
			player, privilege(player,255),
			db[player].location,params);
#endif /* #ifdef QMW_RESEARCH */
           setreturn(OK,COMMAND_SUCC);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"What would you like to say?");
     } else if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, this is a quiet %s  -  You can't talk in here.",(Typeof(db[player].location) == TYPE_ROOM) ? "room":"container");
     command_type &= ~COMM_CMD;
}

/* ---->  View session comments or set session comment/title  <---- */
void comms_session(CONTEXT)
{
     static   time_t session_timer = 0;
     struct   descriptor_data *d;
     char     *start,*ptr;
     unsigned char reset = 0;
     short    count;
     time_t   now;

     gettime(now);
     setreturn(ERROR,COMMAND_FAIL);
     for(; *params && (*params == ' '); params++);
     for(start = params, ptr = scratch_buffer; *params && (*params != ' '); *ptr++ = *params, params++);
     for(*ptr = '\0'; *params && (*params == ' '); params++);
     if(!Blank(scratch_buffer) && (string_prefix("title",scratch_buffer) || (string_prefix("reset",scratch_buffer) && (reset = 1)))) {

        /* ---->  Set/reset session title  <---- */
        if(!reset || Level4(db[player].owner)) {
           if(!Blank(params) && !(reset && (*params == '='))) {
              if(reset || (now >= session_timer) || ((player == session_who) && (now <= (session_timer - (SESSION_TIME * MINUTE) + MINUTE)))) {
                 ptr = punctuate(params,2,'.');
                 bad_language_filter(ptr,ptr);
                 if((strlen(ptr) <= 156) && !strchr(ptr,'\n')) {
                    if(!instring("%{",ptr)) {

                       /* ---->  Tell session users of title change/reset  <---- */
                       command_type |= COMM_CMD;
                       substitute(player,scratch_return_string,ptr,0,ANSI_LWHITE,NULL,0);
                       for(d = descriptor_list; d; d = d->next) {
                           if(!Blank(d->comment)) {
                              FREENULL(d->comment);
                              if(d->player != player) {
                                 if(reset && Blank(ptr)) output(d,d->player,0,1,0,ANSI_LMAGENTA"[%s"ANSI_LWHITE"%s"ANSI_LMAGENTA" has reset the current session  -  You can set a new session by typing '"ANSI_LYELLOW"session title <TITLE>"ANSI_LMAGENTA"'.]",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0));
                                    else output(d,d->player,0,1,0,ANSI_LMAGENTA"[%s"ANSI_LWHITE"%s"ANSI_LMAGENTA" has %sset the current session to '"ANSI_LWHITE"%s"ANSI_LMAGENTA"'  -  Please set an appropriate comment by typing '"ANSI_LYELLOW"session comment <COMMENT>"ANSI_LMAGENTA"' (If you no-longer wish to participate, simply ignore this message.)]",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0),(reset) ? "re":"",scratch_return_string);
			      }
			   }
		       }
                       command_type &= ~COMM_CMD;

                       /* ---->  Set new title/reset title  <---- */
                       if(!Blank(ptr)) {
                          if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"The session is now '"ANSI_LWHITE"%s"ANSI_LGREEN"'  -  Please set your comment by typing '"ANSI_LYELLOW"session comment <COMMENT>"ANSI_LGREEN"'.",scratch_return_string);
                          if(reset) writelog(ADMIN_LOG,1,"SESSION","%s(#%d) forcefully changed the session title from '%s' to '%s'",getname(player),player,(session_title) ? decompress(session_title):"",ptr);
                          FREENULL(session_title);
                          session_title = (char *) alloc_string(compress(ptr,0));
                          session_timer = now + (SESSION_TIME * MINUTE);
                          session_who   = player;
		       } else {
                          if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"The current session is now reset  -  You can set a new session by typing '"ANSI_LYELLOW"session title <COMMENT>"ANSI_LGREEN"'.");
                          if(reset) writelog(ADMIN_LOG,1,"SESSION","%s(#%d) reset the session title ('%s')",getname(player),player,(session_title) ? decompress(session_title):"");
                          FREENULL(session_title);
                          session_title = NULL;
                          session_timer = now;
                          session_who   = player;
		       }
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the session title can't contain query command substitutions ('"ANSI_LWHITE"%%{<QUERY COMMAND>}"ANSI_LGREEN"'.)");
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of the session title is 155 characters.  It also must not contain embedded NEWLINE's.");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the session may not be changed for another "ANSI_LWHITE"%s"ANSI_LGREEN".",interval(session_timer - now,session_timer - now,ENTITIES,0));
	   } else if(!Blank(arg2)) {
              dbref who;

              /* ---->  Reset unsuitable session comment (Set by given user)  <---- */
              if((who = lookup_character(player,arg2,1)) != NOTHING) {
	         if(can_write_to(player,who,1)) {
                    if(Connected(who) && (d = getdsc(who))) {
                       if(!Blank(d->comment)) {
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN"'s session comment has been reset.",Article(who,UPPER,DEFINITE),getcname(NOTHING,who,0,0));
                          output(d,who,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"%s"ANSI_LYELLOW"%s"ANSI_LWHITE" has reset your session comment.",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0));
                          writelog(ADMIN_LOG,1,"SESSION","%s(#%d) reset %s(#%d)'s session comment ('%s')",getname(player),player,getname(who),who,(d->comment) ? decompress(d->comment):""); 
                          for(d = descriptor_list; d; d = d->next)
                              if(d->player == who)
                                 FREENULL(d->comment);
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" has not made a session comment.",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0));
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only reset the session comment of a connected character.");
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only reset the session comment of someone who's of a lower level than yourself.");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg2);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify who's session comment you'd like to reset.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may reset/change the current session title, or reset the session comment of an individual user.");
     } else if(!Blank(scratch_buffer) && string_prefix("comment",scratch_buffer)) {

        /* ---->  Set your session comment  <---- */
        if(!Blank(session_title)) {
           if(!Blank(params)) {
              ptr = punctuate(params,2,'.');
              bad_language_filter(ptr,ptr);
              if((strlen(ptr) <= 56) && !strchr(ptr,'\n')) {
                 if(!instring("%{",ptr)) {
                    for(d = descriptor_list, count = 0; d; d = d->next)
                        if(d->player == player) {
                           FREENULL(d->comment);
                           if(!count) d->comment = (char *) alloc_string(compress(ptr,0));
                           count++;
			}
                    if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Your session comment is now '"ANSI_LWHITE"%s"ANSI_LGREEN"'",substitute(player,scratch_return_string,ptr,0,ANSI_LWHITE,NULL,0));
                    setreturn(OK,COMMAND_SUCC);
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, your session comment can't contain query command substitutions ('"ANSI_LWHITE"%%{<QUERY COMMAND>}"ANSI_LGREEN"'.)");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of your session comment is 55 characters.  It also must not contain embedded NEWLINE's.");
	   } else {
              for(d = descriptor_list, count = 0; d; d = d->next)
                  if(d->player == player) {
                     FREENULL(d->comment);
                     count++;
		  }
              output(getdsc(player),player,0,1,0,(count) ? ANSI_LGREEN"Your session comment has been reset.":ANSI_LGREEN"Please specify your session comment.");
	   }
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the current session hasn't been set yet  -  If you'd like to set it, please type '"ANSI_LWHITE"session title <TITLE>"ANSI_LGREEN"'.");
     } else if(!Blank(session_title)) {

        /* ---->  List session comments  <---- */
        userlist_view(player,start,NULL,NULL,NULL,13,0);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the current session hasn't been set yet  -  If you'd like to set it, please type '"ANSI_LWHITE"session title <TITLE>"ANSI_LGREEN"'.");
}

/* ---->  Think about something  <---- */
void comms_think(CONTEXT)
{
     setreturn(ERROR,COMMAND_FAIL);
     comms_spoken(player,0);
     command_type |= COMM_CMD;
     if(!Quiet(db[player].location)) {
        if(!Blank(params)) {
           output(getdsc(player),player,0,1,2,"%s",construct_message(player,ANSI_LWHITE,(in_command) ? ANSI_LCYAN:ANSI_DWHITE,"",'.',2,PLAYER,params,2,DEFINITE));
           output_except(db[player].location,player,NOTHING,0,1,2,"%s",construct_message(player,ANSI_LWHITE,(in_command) ? ANSI_LCYAN:ANSI_DWHITE,"",'.',2,OTHERS,params,2,DEFINITE));
#ifdef QMW_RESEARCH
	   qmwlogsocket("THINK:%d:%d:%d:%s",
			player, privilege(player,255),
			db[player].location,params);
#endif /* #ifdef QMW_RESEARCH */
           setreturn(OK,COMMAND_SUCC);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"What thought would you like to share with everyone in the room?");
     } else if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, this is a quiet %s  -  You can't share your thoughts in here.",(Typeof(db[player].location) == TYPE_ROOM) ? "room":"container");
     command_type &= ~COMM_CMD;
}

/* ---->  Attempt to 'wake' idle character  <---- */
void comms_wake(CONTEXT)
{
     static dbref wake_victim = NOTHING,wake_who = NOTHING;
     static time_t wake_time = 0;
     struct descriptor_data *p;
     dbref  target;
     int    flags;
     time_t now;

     gettime(now);
     setreturn(ERROR,COMMAND_FAIL);
     if(!Moron(player)) {
        if(!Blank(params)) {
           if((target = lookup_character(player,params,1)) != NOTHING) {
              if(player != target) {
                 if(!Haven(target)) {
                    if((p = getdsc(target)) && Connected(target)) {
                       if(!(p->flags & DISCONNECTED)) {
                          if(Level4(player) || ((now - p->last_time) > (WAKE_IDLE_TIME * MINUTE))) {
                             if(!((flags = friend_flags(target,player)) && !(flags & FRIEND_PAGETELL))) {
                                if(!(!Level4(player) && (wake_who == player) && (wake_victim == target) && (now < wake_time))) {
                                   wake_victim = target;
                                   wake_time   = now + WAKE_TIME;
                                   wake_who    = player;

                                   now += (db[target].data->player.timediff * HOUR);
                                   output(getdsc(player),player,0,1,0,ANSI_LGREEN"You attempt to wake %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(target,LOWER,DEFINITE),getcname(NOTHING,target,0,0));
                                   output(p,target,0,1,0,"");
                                   sprintf(scratch_buffer,ANSI_LMAGENTA""ANSI_BLINK""ANSI_UNDERLINE"!!! "ANSI_LRED""ANSI_BLINK""ANSI_UNDERLINE"\007WAKE UP"ANSI_LMAGENTA""ANSI_BLINK""ANSI_UNDERLINE" !!!"ANSI_LWHITE" \016&nbsp; &nbsp;\016 %s"ANSI_LYELLOW"%s"ANSI_LWHITE" tried to wake you on ",Article(player,UPPER,(db[player].location == db[target].location) ? DEFINITE:INDEFINITE),getcname(NOTHING,player,0,0));
                                   output(p,target,0,1,18,ANSI_LGREEN"%s%s"ANSI_LWHITE".\n",scratch_buffer,date_to_string(now,UNSET_DATE,target,FULLDATEFMT));
                                   setreturn(OK,COMMAND_SUCC);
				} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please wait "ANSI_LWHITE"%s"ANSI_LGREEN" before attempting to wake that user again.",interval(wake_time - now,wake_time - now,ENTITIES,0));
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" doesn't wish to be disturbed by you.",Article(target,LOWER,DEFINITE),getcname(NOTHING,target,0,0));
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" has been idle for less than "ANSI_LYELLOW"%d minute%s"ANSI_LGREEN".",Article(target,LOWER,DEFINITE),getcname(NOTHING,target,0,0),WAKE_IDLE_TIME,Plural(WAKE_IDLE_TIME));
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" has lost %s connection.  Please wait for %s to reconnect before trying again.",Article(target,LOWER,DEFINITE),getcname(NOTHING,target,0,0),Possessive(target,0),Objective(target,0));
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" isn't connected.",Article(target,LOWER,DEFINITE),getcname(NOTHING,target,0,0));
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" doesn't wish to be disturbed.",Article(target,LOWER,DEFINITE),getcname(NOTHING,target,0,0));
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't wake yourself (If you're typing the '"ANSI_LWHITE"wake"ANSI_LGREEN"' command, you must be awake (Or at least partially!))");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",params);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify who you would like to attempt to wake.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Morons aren't allowed to wake other characters.");
}

/* ---->  Whisper message to another character in the same room  <---- */
void comms_whisper(CONTEXT)
{
     static dbref  whisper_victim = NOTHING,whisper_who = NOTHING;
     static time_t whisper_time = 0;
     dbref  target = NOTHING;
     const  char *name;
     time_t now;

     setreturn(ERROR,COMMAND_FAIL);
     comms_spoken(player,1);
     gettime(now);
     if(!Quiet(db[player].location)) {
        if(!Blank(arg1)) {
           if(Blank(arg2)) {
              char     *start1,*start2,*end1,*end2,*src;
              unsigned char matched = 0;
              char     c;

              /* ---->  No '=' to separate name and message  <---- */
              for(src = arg1; *src && (*src == ' '); src++);
              for(start1 = src; *src && (*src != ' '); src++);
              for(end1 = src; *src && (*src == ' '); src++);
              for(start2 = src; *src && (*src != ' '); src++);
              for(end2 = src; *src && (*src == ' '); src++);

              /* ---->  Match first two words  <---- */
              if(!matched) {
                 c = *end2, *end2 = '\0';
                 if((target = lookup_character(player,name = start1,5)) != NOTHING)
                    arg1 = start1, arg2 = src, matched = 1;
                      else *end2 = c;
	      }

              /* ---->  Match first word  <---- */
              if(!matched) {
                 *end1 = '\0', arg1 = start1, arg2 = start2;
                 target = lookup_character(player,name = start1,1);
	      }
	   } else target = lookup_character(player,name = arg1,1);

           if(Valid(target)) {
              if(!(!Level4(player) && (whisper_who == player) && (whisper_victim == target) && (now < whisper_time))) {
                 if(!Blank(arg2)) {
                    if(Connected(target)) {
	               if(target != player) {
                          if(db[target].location == db[player].location) {
                             whisper_victim = target;
                             whisper_time   = now + WHISPER_TIME;
                             whisper_who    = player;

                             command_type |= COMM_CMD;
                             substitute(player,scratch_return_string,punctuate((char *) arg2,1,'\0'),0,ANSI_LWHITE,NULL,0);
                             output(getdsc(player),player,0,1,2,ANSI_LCYAN"You whisper \"%s"ANSI_LCYAN"\" to %s"ANSI_LWHITE"%s"ANSI_LCYAN".",scratch_return_string,Article(target,LOWER,DEFINITE),getcname(NOTHING,target,0,0));
                             command_type &= ~COMM_CMD;
#ifdef QMW_RESEARCH
			     qmwlogsocket("WHISPER:%d:%d:%d:%d:%d:%d:%s",
					  player, privilege(player,255),
					  db[player].location,
					  target,  privilege(target,255),
					  db[target].location,
					  (char *)arg2);
#endif /* #ifdef QMW_RESEARCH */

                             if(!Moron(player) || Quiet(db[player].location)) {
                                output(getdsc(target),target,0,1,2,ANSI_LCYAN"%s"ANSI_LWHITE"%s"ANSI_LCYAN" whispers \"%s"ANSI_LCYAN"\" to you.",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0),scratch_return_string);
#ifndef QUIET_WHISPERS
                                if(!Quiet(db[player].location) && (db[target].location == db[player].location)) {
                                   sprintf(scratch_buffer,ANSI_LCYAN"%s"ANSI_LWHITE"%s"ANSI_LCYAN" whispers something to ",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0));
                                   output_except(db[player].location,player,target,0,1,2,"%s%s"ANSI_LWHITE"%s"ANSI_LCYAN".",scratch_buffer,Article(target,LOWER,DEFINITE),getcname(NOTHING,target,0,0));
				}
#endif
			     } else {
                                output(getdsc(target),target,0,1,2,ANSI_LCYAN"%s"ANSI_LWHITE"%s"ANSI_LCYAN" whispers \"%s"ANSI_LCYAN"\" to you, not realising that everyone else in the room overheard.",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0),scratch_return_string);
#ifndef QUIET_WHISPERS
                                if(!Quiet(db[player].location) && (db[target].location == db[player].location)) {
                                   sprintf(scratch_buffer,ANSI_LCYAN"%s"ANSI_LWHITE"%s"ANSI_LCYAN" tries to whisper \"%s"ANSI_LCYAN"\" to ",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0),scratch_return_string);
                                   output_except(db[player].location,player,target,0,1,2,"%s%s"ANSI_LWHITE"%s"ANSI_LCYAN", but being a Moron, everyone else in the room overhears.",scratch_buffer,Article(target,LOWER,DEFINITE),getcname(NOTHING,target,0,0));
				}
#endif
			     }
                             setreturn(OK,COMMAND_SUCC);
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you may only whisper to characters in the same location as you (Please use either '"ANSI_LWHITE"page"ANSI_LGREEN"' or '"ANSI_LWHITE"tell"ANSI_LGREEN"' to talk to users in other locations.)");
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Whispering to yourself is the first sign of madness!");
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" isn't connected.",Article(target,LOWER,DEFINITE),getcname(NOTHING,target,0,0));
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"What would you like to whisper to %s"ANSI_LWHITE"%s"ANSI_LGREEN"?",Article(target,LOWER,DEFINITE),getcname(NOTHING,target,0,0));
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please wait "ANSI_LWHITE"%s"ANSI_LGREEN" before whispering another message to that user.",interval(whisper_time - now,whisper_time - now,ENTITIES,0));
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"\nSorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.\n\n"ANSI_LCYAN"USAGE:  "ANSI_LGREEN"whisper <NAME> [=] <MESSAGE>\n",name);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify who you would like to whisper a message to.");
     } else if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, this is a quiet %s  -  You can't whisper in here.",(Typeof(db[player].location) == TYPE_ROOM) ? "room":"container");
}

/* ---->  Write message to everyone in same room's screen (INCLUDING you)  <---- */
void comms_write(CONTEXT)
{
     setreturn(ERROR,COMMAND_FAIL);
     if(Level3(db[player].owner) || (in_command && (Wizard(current_command) || Apprentice(current_command))) || can_write_to(player,db[player].location,0)) {
        substitute(player,scratch_buffer,(char *) params,0,ANSI_LCYAN,NULL,0);
        if(!Quiet(db[player].location)) {
           command_type |=  LEADING_BACKGROUND;
           output_except(db[player].location,NOTHING,NOTHING,0,1,0,"%s",scratch_buffer);
           command_type &= ~LEADING_BACKGROUND;
           setreturn(OK,COMMAND_SUCC);
	} else if(in_command) {
           command_type |=  LEADING_BACKGROUND;
           output(getdsc(player),player,0,1,0,"%s",scratch_buffer);
           command_type &= ~LEADING_BACKGROUND;
           setreturn(OK,COMMAND_SUCC);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, this is a quiet %s  -  You can't '"ANSI_LWHITE"@write"ANSI_LGREEN"' in here.",(Typeof(db[player].location) == TYPE_ROOM) ? "room":"container");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only '"ANSI_LWHITE"@write"ANSI_LGREEN"' messages in locations you own.");
}

/* ---->  Yell message to everyone in given area  <---- */
void yell_message(dbref player,const char *msg,dbref loc,dbref area,const char *areaname)
{
     struct descriptor_data *d;

     for(d = descriptor_list; d; d = d->next) {
         if((d->flags & CONNECTED) && (d->player != player) && !Quiet(d->player) && !Quiet(db[d->player].location) && Yell(db[d->player].location) && (db[db[d->player].location].owner == db[loc].owner) && (get_areaname_loc(db[d->player].location) == area)) {
            if(!((Secret(player) || Secret(db[player].location)) && !can_write_to(d->player,db[player].location,1) && !can_write_to(d->player,player,1))) {
               if(!Blank(areaname)) output(d,d->player,0,1,2,"%s%s"ANSI_LYELLOW"%s"ANSI_LGREEN" in "ANSI_LYELLOW"%s"ANSI_LGREEN".",msg,Article(d->player,LOWER,INDEFINITE),unparse_object(d->player,loc,0),areaname);
	          else output(d,d->player,0,1,2,"%s%s"ANSI_LYELLOW"%s"ANSI_LGREEN".",msg,Article(d->player,LOWER,INDEFINITE),unparse_object(d->player,loc,0));
	    } else if(!Blank(areaname)) output(d,d->player,0,1,2,"%sa secret location in "ANSI_LYELLOW"%s"ANSI_LGREEN".",msg,areaname);
               else output(d,d->player,0,1,2,"%sa secret location.",msg);
	 }
     }
}

/* ---->  Yell message throughout locations owned by the owner of current location  <---- */
void comms_yell(CONTEXT)
{
     char  buffer[TEXT_SIZE];
     dbref area;

     setreturn(ERROR,COMMAND_FAIL);
     comms_spoken(player,1);
     if(!Quiet(db[player].location)) {
        if(!Moron(player)) {
           if(!(!Yell(player) && !((db[player].owner == db[db[player].location].owner) || (db[player].owner == Controller(db[db[player].location].owner))))) {
              if(!(!Yell(db[player].location) && !((db[player].owner == db[db[player].location].owner) || (db[player].owner == Controller(db[db[player].location].owner)) || Level4(db[player].owner)))) {
                 if(!Blank(params)) {
                    command_type |= COMM_CMD;
                    area = get_areaname_loc(db[player].location);
                    if(!Censor(player) && !Censor(db[player].location)) bad_language_filter((char *) params,params);
                    sprintf(scratch_return_string,"%s from ",construct_message(player,ANSI_LWHITE,ANSI_LGREEN,"yells",'\0',0,OTHERS - 2,params,0,INDEFINITE));
                    if(HasField(area,AREANAME)) {
                       yell_message(player,scratch_return_string,db[player].location,area,getfield(area,AREANAME));
                       if(!Blank(getfield(area,AREANAME))) sprintf(buffer," across "ANSI_LYELLOW"%s"ANSI_LGREEN" at the top of your voice.",getfield(area,AREANAME));
		    } else {
                       yell_message(player,scratch_return_string,db[player].location,NOTHING,NULL);
                       strcpy(buffer," at the top of your voice.");
		    }

                    output(getdsc(player),player,0,1,2,"%s%s",construct_message(player,ANSI_LWHITE,ANSI_LGREEN,"yell",'\0',0,PLAYER - 2,params,0,DEFINITE),buffer);
                    command_type &= ~COMM_CMD;
#ifdef QMW_RESEARCH
		    qmwlogsocket("YELL:%d:%d:%d:%s",
				 player, privilege(player,255),
				 db[player].location,params);
#endif /* #ifdef QMW_RESEARCH */
                    setreturn(OK,COMMAND_SUCC);
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"What message would you like to yell?");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"You yell as loud as you can, but the walls just seem to swallow up your words.");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you've been banned from yelling (Except on your own property.)");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Morons aren't allowed to yell messages.");
     } else if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, this is a quiet %s  -  You can't yell in here.",(Typeof(db[player].location) == TYPE_ROOM) ? "room":"container");
}
