/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| MAIL.C  -  Implements the internal mail system.                             |
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
| Module originally designed and written by:  J.P.Boggis 01/11/1995.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: mail.c,v 1.2 2005/06/29 21:33:19 tcz_monster Exp $

*/

/*  WHEN MAIL SYSTEM FINISHED AND IMPLEMENTED:  */
/*     server.c  -  Re-implement check of mail on connection.              */
/*     admin.c   -  Implement start mail maintenance into '@maintenance'.  */

/*  No newlines allowed in subject field                          */
/*  Support for MAIL flag (Bans from sending/receiving mail)      */
/*  (maillimit == 0) doesn't ban character from sending mail (MAIL flag does that  */
/*  Support for MAIL friend flag (Both sending and receiving)     */
/*  When warning character, also send mail to them automatically  */
/*  Implement .mail compound command (Similar to .page/.tell) - Executed when mail is received  */

#include <string.h>
#include <stdlib.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "object_types.h"
#include "friend_flags.h"
#include "flagset.h"


/* ---->  TARGETGROUP #define's (As defined in PAGETELL.C)  <---- */
#define  STANDARD  0
#define  ADMIN     4

unsigned long mailcounter = 0;


/* ---->  Lookup message by number in mailbox  <---- */
static struct mail_data *lookup_message(dbref player,const char *messageno,struct mail_data **last,short *no)
{
       struct mail_data *ptr;
       short  number;

       (*last) = NULL;
       if(Blank(messageno)) return(NULL);
       if(string_prefix("FIRST",messageno) || string_prefix("ALL",messageno)) number = FIRST;
          else if(string_prefix("LAST",messageno) || string_prefix("END",messageno) || string_prefix("LATEST",messageno)) number = LAST;
             else {
                number = atol(messageno);
                if(number < 1) return(NULL);
	     }

       switch(number) {
              case FIRST:
                   *no = 1;
                   return(db[player].data->player.mail);
              case LAST:
                   for(ptr = db[player].data->player.mail, *no = 1; ptr && ptr->next; (*last) = ptr, (*no)++, ptr = ptr->next);
                   return(ptr);
              default:
                   for(ptr = db[player].data->player.mail, *no = 0; ptr; (*last) = ptr, ptr = ptr->next)
                       if(++(*no) == number) return(ptr);
       }
       return(NULL);
}

/* ---->  Mail message to another character (Or list of characters)  <---- */
unsigned char mail_send_message(dbref player,struct mlist_data **list,const char *text,const char *subject,const char *grouplist,int listsize,unsigned char targetgroup)
{
	 const    char       *cached_message = NULL,*cached_subject = NULL,*cached_sender = NULL,*cached_redirect = NULL;
	 struct   mlist_data *ptr,*next,*tail,*newlist = NULL;
	 unsigned char       cr = 0,group = 0,redirected = 0;
	 struct   mail_data  *new,*mail,*last,*root = NULL;
	 short               count,flags = 0;
	 time_t              now;

	 gettime(now);
	 if((*list)->next) group = 1;
	 if(*subject && (*subject == '!')) subject++, flags = MAIL_IMPORTANT;
	    else if(*subject && (*subject == '~')) subject++, flags = MAIL_URGENT;

	 for(ptr = *list; ptr; ptr = next) {
	     next = ptr->next;
	     if(!Validchar(ptr->redirect)) ptr->redirect = NOTHING;
		else redirected = 1;
	     for(mail = (Validchar(ptr->player)) ? db[ptr->player].data->player.mail:NULL, count = 0; mail; mail = mail->next, count++);
	     if(!Validchar(ptr->player)) {
		output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that character no-longer exists.");
		listsize--, cr = 1;
		FREENULL(ptr);
	     } else if(!friendflags_check(player,player,ptr->player,FRIEND_MAIL,NULL)) {
		listsize--, cr = 1;
		FREENULL(ptr);
	     } else if(count >= db[ptr->player].data->player.maillimit) {
		output(getdsc(player),player,0,1,0,ANSI_LGREEN"\nSorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s mailbox is full.  You can't send mail to %s.",Article(ptr->player,LOWER,DEFINITE),getcname(NOTHING,ptr->player,0,0),Objective(ptr->player,0));
		listsize--, cr = 1;
		FREENULL(ptr);
	     } else {
		if(newlist) tail->next = ptr, tail = ptr;
		   else newlist = tail = ptr;
		db[ptr->player].flags |= OBJECT;

		for(mail = db[ptr->player].data->player.mail, last = NULL; mail && (mail->date <= now); last = mail, mail = mail->next);
		MALLOC(new,struct mail_data);

		if(!root) {
		   new->subject = (char *) (cached_subject = alloc_string(compress(punctuate((char *) subject,2,'\0'),0)));
		   new->message = (char *) (cached_message = alloc_string(compress(punctuate((char *) text,2,'.'),0)));
		   new->sender  = (char *) (cached_sender  = alloc_string(compress(getcname(NOTHING,player,0,UPPER|DEFINITE),1)));
		   if(group) new->redirect = (char *) (cached_redirect = alloc_string(compress(grouplist,0)));
		      else if(Validchar(ptr->redirect)) new->redirect = (char *) (cached_redirect = alloc_string(compress(getcname(NOTHING,ptr->redirect,0,UPPER|DEFINITE),0)));
			 else new->redirect = NULL;
		} else {
		   new->redirect = (char *) cached_redirect;
		   new->subject  = (char *) cached_subject;
		   new->message  = (char *) cached_message;
		   new->sender   = (char *) cached_sender;
		}
		new->flags = MAIL_UNREAD|flags;
		new->date  = new->lastread = now;
		new->who   = player;

		if(group) {
		   new->flags |= MAIL_MULTI;
		   if(!root) {
		      new->flags |= MAIL_MULTI_ROOT;
		      mailcounter++;
		      root = new;
		   }
		}
		new->id = (group) ? mailcounter:0;

		if(last) last->next = new;
		   else db[ptr->player].data->player.mail = new;
		if(mail) new->next = mail;
		   else new->next = NULL;

		substitute(ptr->player,scratch_buffer,punctuate((char *) subject,2,'\0'),0,ANSI_LYELLOW,NULL,0);
		output(getdsc(ptr->player),ptr->player,0,1,0,ANSI_LGREEN"You have %snew mail from %s"ANSI_LWHITE"%s"ANSI_LGREEN" ('%s"ANSI_LGREEN"'.)",(new->flags & MAIL_IMPORTANT) ? "important ":(new->flags & MAIL_URGENT) ? "urgent ":"",Article(player,LOWER,INDEFINITE),getcname(NOTHING,player,0,0),scratch_buffer);
	     }
	 }
	 *list = newlist;

	 if(!in_command) {
	    if(listsize > 0) {
	       pagetell_construct_list(player,player,(union group_data *) *list,listsize,scratch_return_string,ANSI_LWHITE,ANSI_LGREEN,0,targetgroup,DEFINITE);
	       output(getdsc(player),player,0,1,0,ANSI_LGREEN"\nMessage mailed to %s%s  If you've accidentally mailed the wrong person, simply type '"ANSI_LWHITE"mail stop <NAME1> = <NAME2>"ANSI_LGREEN"' (Where "ANSI_LYELLOW"<NAME1>"ANSI_LGREEN" is the name of the person you accidentally mailed) to remove it from their mailbox and re-send it to the character "ANSI_LYELLOW"<NAME2>"ANSI_LGREEN".\n",scratch_return_string,(redirected) ? (group) ? " (Some (Or all) of the mail has been redirected.)":" (It has been redirected.)":".");
	    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"%sMail not sent.%s",(cr) ? "\n":"",(cr) ? "\n":"");
	 } else if(cr) output(getdsc(player),player,0,1,0,"");
	 return(listsize > 0);
}

/* ---->  Forward message from your mailbox to another character (Or list of characters)  <---- */
void mail_forward_message(dbref player,struct mlist_data **list,struct mail_data *fmail,short msgno,int listsize,unsigned char targetgroup)
{
     const    char       *cached_message = NULL,*cached_subject = NULL,*cached_sender = NULL,*cached_redirect = NULL;
     struct   mlist_data *ptr,*next,*tail,*newlist = NULL;
     unsigned char       cr = 0,group = 0,redirected = 0;
     struct   mail_data  *new,*mail,*last,*root = NULL;
     short               count,flags = 0;
     time_t              now;

     gettime(now);
     if((*list)->next) group = 1;
     flags = fmail->flags & (MAIL_REPLY|MAIL_URGENT|MAIL_IMPORTANT);

     for(ptr = *list; ptr; ptr = next) {
         next = ptr->next;
         if(!Validchar(ptr->redirect)) ptr->redirect = NOTHING;
            else redirected = 1;
         for(mail = (Validchar(ptr->player)) ? db[ptr->player].data->player.mail:NULL, count = 0; mail; mail = mail->next, count++);
         if(!Validchar(ptr->player)) {
            output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that character no-longer exists.");
            listsize--, cr = 1;
            FREENULL(ptr);
	 } else if(!friendflags_check(player,player,ptr->player,FRIEND_MAIL,NULL)) {
            listsize--, cr = 1;
            FREENULL(ptr);
	 } else if(count >= db[ptr->player].data->player.maillimit) {
            output(getdsc(player),player,0,1,0,ANSI_LGREEN"\nSorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s mailbox is full.  You can't forward mail to %s.",Article(ptr->player,LOWER,DEFINITE),getcname(NOTHING,ptr->player,0,0),Objective(ptr->player,0));
            listsize--, cr = 1;
            FREENULL(ptr);
	 } else {
            if(newlist) tail->next = ptr, tail = ptr;
	       else newlist = tail = ptr;
            db[ptr->player].flags |= OBJECT;

            for(mail = db[ptr->player].data->player.mail, last = NULL; mail && (mail->date <= now); last = mail, mail = mail->next);
            MALLOC(new,struct mail_data);

            if(!root) {
               new->redirect = (char *) (cached_redirect = alloc_string(compress(getcname(NOTHING,player,0,UPPER|DEFINITE),1)));
               new->subject  = (char *) (cached_subject  = alloc_string(fmail->subject));
               new->message  = (char *) (cached_message  = alloc_string(fmail->message));
               new->sender   = (char *) (cached_sender   = alloc_string(fmail->sender));
	    } else {
               new->redirect = (char *) cached_redirect;
               new->subject  = (char *) cached_subject;
               new->message  = (char *) cached_message;
               new->sender   = (char *) cached_sender;
	    }
            new->flags = MAIL_UNREAD|MAIL_FORWARD|flags;
            new->date  = new->lastread = now;
            new->who   = player;

            if(group) {
               new->flags |= MAIL_MULTI;
               if(!root) {
                  new->flags |= MAIL_MULTI_ROOT;
                  mailcounter++;
                  root = new;
	       }
	    }
            new->id = (group) ? mailcounter:0;

            if(last) last->next = new;
               else db[ptr->player].data->player.mail = new;
            if(mail) new->next = mail;
               else new->next = NULL;

            substitute(ptr->player,scratch_buffer,punctuate((char *) decompress(fmail->subject),2,'\0'),0,ANSI_LYELLOW,NULL,0);
            output(getdsc(ptr->player),ptr->player,0,1,0,ANSI_LGREEN"You have %snew mail from %s"ANSI_LWHITE"%s"ANSI_LGREEN" ('%s%s"ANSI_LGREEN"'.)",(new->flags & MAIL_IMPORTANT) ? "important ":(new->flags & MAIL_URGENT) ? "urgent ":"",Article(player,LOWER,INDEFINITE),getcname(NOTHING,player,0,0),(new->flags & MAIL_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_buffer);
	 }
     }
     *list = newlist;

     if(!in_command) {
        if(listsize > 0) {
           pagetell_construct_list(player,player,(union group_data *) *list,listsize,scratch_return_string,ANSI_LWHITE,ANSI_LGREEN,0,targetgroup,DEFINITE);
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"\nMessage "ANSI_LYELLOW"%d"ANSI_LGREEN" forwarded to %s%s  If you've accidentally forwarded mail to the wrong person, simply type '"ANSI_LWHITE"mail stop <NAME1> = <NAME2>"ANSI_LGREEN"' (Where "ANSI_LYELLOW"<NAME1>"ANSI_LGREEN" is the name of the person you accidentally forwarded the mail to) to remove it from their mailbox and re-forward it to the character "ANSI_LYELLOW"<NAME2>"ANSI_LGREEN".\n",msgno,scratch_return_string,(redirected) ? (group) ? " (Some (Or all) of the mail has been redirected.)":" (It has been redirected.)":".");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"%sMail not forwarded.%s",(cr) ? "\n":"",(cr) ? "\n":"");
     } else if(cr) output(getdsc(player),player,0,1,0,"");
}

/* ---->  Reply to message in your mailbox  <---- */
unsigned char mail_reply_message(dbref player,dbref who,dbref redirect,struct mail_data *rmail,short msgno,const char *message)
{
	 short  count,flags = rmail->flags & (MAIL_IMPORTANT|MAIL_URGENT);
	 struct mail_data *new,*mail,*last;
	 time_t now;

	 gettime(now);
	 for(mail = (Validchar(who)) ? db[who].data->player.mail:NULL, count = 0; mail; mail = mail->next, count++);
	 if(!Validchar(who)) {
	    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the person who sent that mail to you no-longer exists.");
	    return(0);
	 } else if(!friendflags_check(player,player,who,FRIEND_MAIL,NULL)) {
	    return(0);
	 } else if(count >= db[who].data->player.maillimit) {
	    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s mailbox is full.  You can't reply to mail from %s.",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0),Objective(who,0));
	    return(0);
	 } else {
	    for(mail = db[who].data->player.mail, last = NULL; mail && (mail->date <= now); last = mail, mail = mail->next);
	    MALLOC(new,struct mail_data);

	    if(Validchar(redirect)) new->redirect = (char *) alloc_string(compress(getcname(NOTHING,redirect,0,UPPER|DEFINITE),0));
	       else new->redirect = NULL;
	    new->subject = (char *) alloc_string(rmail->subject);
	    new->message = (char *) alloc_string(compress(punctuate((char *) message,2,'.'),0));
	    new->sender  = (char *) alloc_string(compress(getcname(NOTHING,player,0,UPPER|DEFINITE),0));
	    new->flags   = MAIL_UNREAD|MAIL_REPLY|flags;
	    new->date    = new->lastread = now;
	    new->who     = player;
	    new->id      = 0;

	    if(last) last->next = new;
	       else db[who].data->player.mail = new;
	    if(mail) new->next = mail;
	       else new->next = NULL;

	    substitute(who,scratch_buffer,punctuate((char *) decompress(rmail->subject),2,'\0'),0,ANSI_LYELLOW,NULL,0);
	    output(getdsc(who),who,0,1,0,ANSI_LGREEN"You have %snew mail from %s"ANSI_LWHITE"%s"ANSI_LGREEN" ('%s%s"ANSI_LGREEN"'.)",(new->flags & MAIL_IMPORTANT) ? "important ":(new->flags & MAIL_URGENT) ? "urgent ":"",Article(player,LOWER,INDEFINITE),getcname(NOTHING,player,0,0),(new->flags & MAIL_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_buffer);
	 }
	 if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Reply to message "ANSI_LYELLOW"%d"ANSI_LGREEN" mailed to %s"ANSI_LWHITE"%s"ANSI_LGREEN"%s",msgno,Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0),(Validchar(redirect)) ? " (It has been redirected.)":".");
	 return(1);
}

/* ---->  Display summary of given character's mail  <---- */
void mail_update(dbref player,const char *name,const char *arg2)
{
     int    total = 0,unread = 0,fromyou = 0,unreadfromyou = 0;
     struct descriptor_data *p = getdsc(player);
     struct mail_data *mail;
     dbref  subject;

     if(!Blank(arg2)) name = arg2;
     if(!Blank(name)) {
        if((subject = lookup_character(player,name,1)) == NOTHING) {
           output(p,player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",name);
           return;
	}
     } else subject = player;

     for(mail = db[subject].data->player.mail; mail; mail = mail->next) {
         if(mail->flags & MAIL_UNREAD) {
            if(mail->who == player) unreadfromyou++;
            unread++;
	 }
         total++;
         if(mail->who == player) fromyou++;
     }

     /* ---->  Summary of mail  <---- */
     if(subject != player) sprintf(scratch_buffer,"\n"ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has ",Article(subject,UPPER,DEFINITE),getcname(NOTHING,subject,0,0));
        else strcpy(scratch_buffer,ANSI_LGREEN"\nYou have ");
     if(total > 0) {
        sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LYELLOW"%d"ANSI_LGREEN" item%s of mail",total,Plural(total));
        if(unread > 0) output(p,player,0,1,0,"%s, of which "ANSI_LYELLOW"%d"ANSI_LGREEN" %sn't been read.",scratch_buffer,unread,(unread == 1) ? "has":"have");
           else if(player != subject) output(p,player,0,1,0,"%s (%s has read all of %s mail.)",scratch_buffer,Subjective(subject,1),Possessive(subject,0));
              else output(p,player,0,1,0,"%s (You have read all your mail.)",scratch_buffer);
     } else output(p,player,0,1,0,"%sno mail.",scratch_buffer);
    
     /* ---->  Summary of mail received from you  <---- */
     if(subject != player) {
        if(fromyou > 0) {
           sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LGREEN"%s has "ANSI_LYELLOW"%d"ANSI_LGREEN" item%s of mail",Subjective(subject,1),fromyou,Plural(fromyou));
           if(unreadfromyou > 0) output(p,player,0,1,0,"%s from you, of which "ANSI_LYELLOW"%d"ANSI_LGREEN" %sn't been read.",scratch_buffer,unreadfromyou,(unreadfromyou == 1) ? "has":"have");
	      else output(p,player,0,1,0,"%s from you (%s has read all of the mail you've sent %s.)",scratch_buffer,Subjective(subject,1),Objective(subject,0));
	} else output(p,player,0,1,0,ANSI_LGREEN"%s has no mail from you in %s mailbox.",Subjective(subject,1),Possessive(subject,0));
     }

     /* ---->  Summary of mail sent which hasn't been read yet  <---- */
     if((player == subject) || Level4(db[player].owner)) {
        int   sentunread = 0;
        dbref loop;

        for(loop = 0; loop < db_top; loop++)
            if((Typeof(loop) == TYPE_CHARACTER) && db[loop].data->player.mail)
               for(mail = db[loop].data->player.mail; mail; mail = mail->next)
                   if((mail->flags & MAIL_UNREAD) && (mail->who == subject)) sentunread++;

        if(sentunread > 0) {
           if(player != subject) sprintf(scratch_buffer,ANSI_LGREEN"%s has ",Subjective(subject,1));
              else strcpy(scratch_buffer,ANSI_LGREEN"You have ");
           output(p,player,0,1,0,"%ssent "ANSI_LYELLOW"%d"ANSI_LGREEN" item%s of mail which %sn't been read yet.",scratch_buffer,sentunread,Plural(sentunread),(sentunread == 1) ? "has":"have");
	} else if(player != subject) output(p,player,0,1,0,ANSI_LGREEN"All of the mail %s has sent has been read.",Subjective(subject,0));
           else output(p,player,0,1,0,ANSI_LGREEN"All of the mail you've sent has been read.");
     }

     /* ---->  Mail redirect (If set)  <---- */
     if(Validchar(db[subject].data->player.redirect)) {
        if(player != subject) sprintf(scratch_buffer,ANSI_LGREEN"Mail sent to %s will be redirected to ",Objective(subject,0));
           else strcpy(scratch_buffer,ANSI_LGREEN"Mail sent to you will be redirected to ");
        output(p,player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN"%s",scratch_buffer,Article(subject,LOWER,INDEFINITE),getcname(NOTHING,subject,0,0),(player != subject) ? ".":" (You can stop your mail from being redirected by typing '"ANSI_LYELLOW"mail redirect"ANSI_LGREEN"'.)");
     }
     output(p,player,0,1,0,"");
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  View/read mail in your mailbox  <---- */
void mail_view(dbref player,char *arg1,char *arg2)
{
     unsigned char            twidth = output_terminal_width(player);
     struct   descriptor_data *p = getdsc(player);
     dbref                    who = player;
     time_t                   now;

     setreturn(ERROR,COMMAND_FAIL);
     gettime(now);
     if(!Blank(arg2)) arg1 = arg2;
     if(Blank(arg1) || (!strcasecmp("page",arg1) && (strlen(arg1) == 4)) || !strncasecmp(arg1,"page ",5)) {

        /* ---->  View messages in mailbox  <---- */
        short              loop = 0,mailcount = 0,unread = 0,daysleft;
        unsigned char      cached_scrheight;
        struct   mail_data *ptr;

        if(!Blank(arg1)) for(arg1 += 4; *arg1 && (*arg1 == ' '); arg1++);
        arg1 = (char *) parse_grouprange(player,arg1,DEFAULT,1);
        cached_scrheight = db[player].data->player.scrheight;
        for(ptr = db[who].data->player.mail; ptr; ptr = ptr->next, mailcount++)
            if(ptr->flags & MAIL_UNREAD) unread++;
        db[player].data->player.scrheight = db[player].data->player.scrheight - ((!db[who].data->player.maillimit || (mailcount < db[who].data->player.maillimit)) ? 6:9);

        if(!in_command) {
           if(who != player) sprintf(scratch_buffer,"\n "ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has ",Article(who,UPPER,DEFINITE),getcname(NOTHING,who,0,0));
              else strcpy(scratch_buffer,ANSI_LCYAN"\n You have ");
           if(mailcount > 0) {
              sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LWHITE"%d"ANSI_LCYAN" item%s of mail",mailcount,Plural(mailcount));
              if(unread > 0) output(p,player,0,1,0,"%s, of which "ANSI_LWHITE"%d"ANSI_LCYAN" %sn't been read.",scratch_buffer,unread,(unread == 1) ? "has":"have");
                 else if(player != who) output(p,player,0,1,0,"%s (%s has read all of %s mail.)",scratch_buffer,Subjective(who,1),Possessive(who,0));
                    else output(p,player,0,1,0,"%s (You have read all your mail.)",scratch_buffer);
	   } else output(p,player,0,1,0,"%sno mail.",scratch_buffer);
           output(p,player,0,1,0,separator(twidth,0,'-','='));
	}

        union_initgrouprange((union group_data *) db[who].data->player.mail);
        while(union_grouprange()) {
              loop++;
              sprintf(scratch_return_string,"(%d)",grp->before + loop);
              substitute(player,scratch_return_string + 100,decompress(grp->cunion->mail.subject),0,ANSI_LYELLOW,NULL,0);
              sprintf(scratch_buffer,ANSI_LGREEN" %-7s"ANSI_LWHITE"'%s"ANSI_LYELLOW"%s"ANSI_LWHITE"'",scratch_return_string,(grp->cunion->mail.flags & MAIL_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string + 100);

              if(!(grp->cunion->mail.flags & MAIL_UNREAD))
                 if((daysleft = ((now - grp->cunion->mail.lastread) / DAY)) > 0) {
                    if((daysleft = ((grp->cunion->mail.flags & MAIL_KEEP) ? (MAIL_TIME_LIMIT * 2):MAIL_TIME_LIMIT) - daysleft) < 0) daysleft = 0;
                    if(daysleft <= 7) sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_WRED" (%d day%s left)",daysleft,Plural(daysleft));
		 }
              sprintf(scratch_buffer + strlen(scratch_buffer),"%s%s%s",(grp->cunion->mail.flags & MAIL_UNREAD) ? ANSI_LMAGENTA" (*UNREAD*)":"",(grp->cunion->mail.flags & MAIL_URGENT) ? ANSI_LYELLOW" (*URGENT*)":(grp->cunion->mail.flags & MAIL_IMPORTANT) ? ANSI_LBLUE" (*IMPORTANT*)":"",(grp->cunion->mail.flags & MAIL_KEEP) ? ANSI_LBLUE" (Keep)":"");
              sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LWHITE" from "ANSI_LGREEN"%s "ANSI_LWHITE,decompress(grp->cunion->mail.sender));
              if(grp->cunion->mail.redirect) sprintf(scratch_buffer + strlen(scratch_buffer),"(%s%s"ANSI_LWHITE") ",(grp->cunion->mail.flags & MAIL_FORWARD) ? "Forwarded to you by "ANSI_LGREEN:(grp->cunion->mail.flags & MAIL_MULTI) ? "Group mail to ":(who == player) ? "Redirected to you by "ANSI_LGREEN:"Redirected to them by "ANSI_LGREEN,decompress(grp->cunion->mail.redirect));
              output(p,player,0,1,8,"%son "ANSI_LCYAN"%s"ANSI_LWHITE".",scratch_buffer,date_to_string(grp->cunion->mail.date + (db[player].data->player.timediff * HOUR),UNSET_DATE,player,FULLDATEFMT));
	}

        if(grp->rangeitems == 0) output(p,player,0,1,0,(who == player) ? ANSI_LCYAN" ***  YOUR MAILBOX IS EMPTY  ***":ANSI_LCYAN"***  NO MAIL IN MAILBOX  ***");
        if(!in_command) {
           if(db[who].data->player.maillimit && (mailcount >= db[who].data->player.maillimit)) {
              output(p,player,0,1,0,separator(twidth,0,'-','-'));
              output(p,player,0,1,11,ANSI_WYELLOW" WARNING: \016&nbsp;\016 "ANSI_LRED"Your mailbox is full  -  Please delete any mail you no-longer need (Using the '"ANSI_LWHITE"mail delete"ANSI_LRED"' command), so people can send new mail to you.");
	   }
           output(p,player,0,1,0,separator(twidth,0,'-','='));
           if(grp->rangeitems != 0) output(p,player,0,1,0,ANSI_LWHITE" Mail listed:  "ANSI_DWHITE"%s\n",listed_items(scratch_return_string,1));
	      else output(p,player,0,1,0,ANSI_LWHITE" Mail listed:  "ANSI_DWHITE"None.\n");
	}
        db[player].data->player.scrheight = cached_scrheight;
        setreturn(OK,COMMAND_SUCC);
     } else if(Connected(player)) {

        /* ---->  View specified message  <---- */
        struct mail_data *mail;
        short  no;

        if((mail = lookup_message(who,arg1,&mail,&no))) {

           /* ---->  Message header  <---- */
           output(p,player,0,1,0,"");
           wrap_leading   =  digit_wrap(5,no);
           mail->lastread =  now;
           mail->flags   &= ~MAIL_UNREAD;
           substitute(player,scratch_return_string,decompress(mail->subject),0,ANSI_LYELLOW,NULL,0);
           sprintf(scratch_buffer,ANSI_LGREEN" (%d)  "ANSI_LWHITE"'%s"ANSI_LYELLOW"%s"ANSI_LWHITE"'%s%s",no,(mail->flags & MAIL_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,(mail->flags & MAIL_URGENT) ? ANSI_LYELLOW" (*URGENT*)":(mail->flags & MAIL_IMPORTANT) ? ANSI_LBLUE" (*IMPORTANT*)":"",(mail->flags & MAIL_KEEP) ? ANSI_LBLUE" (Keep)":"");
           sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LWHITE" from "ANSI_LGREEN"%s "ANSI_LWHITE,decompress(mail->sender));
           if(mail->redirect) sprintf(scratch_buffer + strlen(scratch_buffer),"(%s%s"ANSI_LWHITE") ",(grp->cunion->mail.flags & MAIL_FORWARD) ? "Forwarded to you by "ANSI_LGREEN:(mail->flags & MAIL_MULTI) ? "Group mail to ":(who == player) ? "Redirected to you by "ANSI_LGREEN:"Redirected to them by "ANSI_LGREEN,decompress(mail->redirect));
           output(p,player,0,1,0,"%son "ANSI_LCYAN"%s"ANSI_LWHITE".",scratch_buffer,date_to_string(mail->date + (db[player].data->player.timediff * HOUR),UNSET_DATE,player,FULLDATEFMT));

           /* ---->  Message text  <---- */
           wrap_leading = 1;
           output(p,player,0,1,0,separator(twidth,0,'-','='));
           substitute_large(player,player,decompress(mail->message),ANSI_LWHITE,scratch_return_string,0);

           /* ---->  Message footer  <---- */
           output(p,player,0,1,0,separator(twidth,0,'-','='));
           output(p,player,0,1,0,ANSI_LCYAN" This mail was sent to %s "ANSI_LYELLOW"%s"ANSI_LCYAN" %s.\n",(who == player) ? "you":"them",interval(ABS(now - mail->date),0,ENTITIES,0),((now - mail->date) < 0) ? "into the future":"ago");
           setreturn(OK,COMMAND_SUCC);
           wrap_leading = 0;
	} else output(p,player,0,1,0,ANSI_LGREEN"Sorry, either a message with that number doesn't exist, or the message number you specified is invalid.");
     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you must be connected to read your mail.");
}

/* ---->  Send or forward mail to another character (Or list of characters)  <---- */
void mail_send(dbref player,const char *list,const char *params,unsigned char forward)
{
     struct   mlist_data      *ptr,*tail,*next,*head = NULL,*newlist = NULL;
     unsigned char            cr = 0,error = 0,targetgroup = STANDARD;
     static   dbref           mail_victim = NOTHING;
     char                     *p2,*subject,*message;
     struct   descriptor_data *p = getdsc(player);
     int                      listsize = 0,count;
     static   dbref           mail_who = NOTHING;
     dbref                    redirect,who,i;
     static   time_t          mail_time = 0;
     struct   mail_data       *mail,*tmp;
     short                    msgno;
     time_t                   now;
     const    char            *p1;

     gettime(now);
     if(forward) {
        if(in_command) {
           output(p,player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"mail forward"ANSI_LGREEN"' can't be used within a compound command.");
           return;
	} else if(Blank(params)) {
           for(; *list && (*list == ' '); list++);
           for(p2 = (char *) list; *p2 && (*p2 != ' '); p2++);
           if(*p2) for(*p2 = '\0', p2++; *p2 && (*p2 == ' '); p2++);
           params = (char *) p2;
	}
        p1 = list, list = params, params = p1;
     } else split_params((char *) params,&subject,&message);

     if(db[player].data->player.maillimit > 0) {
        if(!Moron(player)) {
           if(!Blank(list)) {
              if(forward) {
                 if(!(mail = lookup_message(player,params,&mail,&msgno))) {
                    output(p,player,0,1,0,ANSI_LGREEN"Sorry, either a message with that number doesn't exist, or the message number you specified is invalid.");
                    return;
		 } else if(mail->flags & MAIL_UNREAD) {
                    output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can't forward a message which you haven't even read yourself yet.");
                    return;
		 }
	      }
              if(forward || (strlen(subject) <= 50)) {
                 if(forward || !Blank(subject)) {
                    comms_spoken(player,1);

                    /* ---->  Construct list of people to mail/forward message to  <---- */
                    if(!strcasecmp(list,"friend") || !strcasecmp(list,"friends")) {
                       output(p,player,0,1,0,(forward) ? ANSI_LGREEN"Sorry, you can't forward a message to all of your friends.":ANSI_LGREEN"Sorry, you can't mail a message to all of your friends.");
                       return;
	            } else if(!strcasecmp(list,"enemies") || !strcasecmp(list,"enemies")) {
                       output(p,player,0,1,0,(forward) ? ANSI_LGREEN"Sorry, you can't forward a message to all of your enemies.":ANSI_LGREEN"Sorry, you can't mail a message to all of your enemies.");
                       return;
	            } else if(!strcasecmp(list,"admin") || !strcasecmp(list,"administrators") || !strcasecmp(list,"administration")) {

                       /* ---->  Mail/forward message to Admin (Apprentice Wizards/Druids and above)  <---- */
                       for(i = 0; i < db_top; i++)
                           if(Validchar(i) && Level4(i) && (i != player)) {
                              for(who = db[i].data->player.redirect; Validchar(who) && (db[who].data->player.redirect != NOTHING); who = db[who].data->player.redirect); 
                              if(!Validchar(who) || (who == i) || !Level4(who)) {
                                 for(ptr = head; ptr && (ptr->player != i); ptr = ptr->next);
                                 if(!ptr) {

                                    /* ---->  Admin character isn't already in list;  Add them to it  <---- */
                                    listsize++;
                                    MALLOC(ptr,struct mlist_data);
                                    ptr->redirect = NOTHING;
                                    ptr->player   = i;
                                    ptr->next     = NULL;
                                    if(head) {
                                       tail->next = ptr;
                                       tail       = ptr;
			            } else head = tail = ptr;
				 }
			      }
			   }
                       targetgroup = ADMIN;
		    } else while(*list) {

                       /* ---->  Mail/forward message to a single character or list of characters  <---- */
                       for(; *list && ((*list == ' ') || (*list == ',') || (*list == ';') || (*list == '&')); list++);
                       for(p2 = scratch_buffer; *list && !((*list == ',') || (*list == ';') || (*list == '&')); *p2++ = *list, list++);
                       for(; ((p2 - 1) >= scratch_buffer) && (*(p2 - 1) == ' '); p2--);  /*  Strip trailing blanks  */
                       *p2 = '\0';

                       if(!BlankContent(scratch_buffer)) {

                          /* ---->  Look up name as character in the DB  <---- */
                          if((who = lookup_character(player,scratch_buffer,0)) == NOTHING)
                             output(p,player,0,1,0,"%s"ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",(cr) ? "":"\n",scratch_buffer), cr = 1;
		       } else who = NOTHING;

  	               if(Validchar(who)) {

                          /* ---->  Managed to find them in DB  -  Check they aren't already in list  <---- */
                          for(ptr = head; ptr && (ptr->player != who); ptr = ptr->next);
                          if(!ptr) {

                             /* ---->  Character isn't in list;  Add them to it  <---- */
                             listsize++;
                             MALLOC(ptr,struct mlist_data);
                             ptr->redirect = NOTHING;
                             ptr->player   = who;
                             ptr->next     = NULL;
                             if(head) {
                                tail->next = ptr;
                                tail       = ptr;
			     } else head = tail = ptr;
			  }
		       }
		    }

                    /* ---->  List of people to mail/forward message to blank?  <---- */
                    if(!head && (targetgroup == STANDARD)) {
                       sprintf(scratch_buffer,(forward) ? "%s"ANSI_LGREEN"Please specify who (Or a list of who) you'd like to forward the message to (E.g:  '"ANSI_LWHITE"mail forward 1 fred"ANSI_LGREEN"'.)":"%s"ANSI_LGREEN"Please specify who (Or a list of who) you'd like to mail a message to (E.g:  '"ANSI_LWHITE"mail fred = A test mail"ANSI_LGREEN"'.)",(cr) ? "":"\n");
                       error = 1;
		    }
                    if(cr) output(p,player,0,1,0,""), cr = 0;

                    /* ---->  Mail 'bombing' protection  <---- */
                    if(!error)
                       if(!Level4(player) && (mail_who == player) && (now < mail_time))
                          for(ptr = head; ptr; ptr = (ptr) ? ptr->next:NULL)
                              if(ptr->player == mail_victim) {
                                 output(p,player,0,1,0,ANSI_LGREEN"Please wait "ANSI_LWHITE"%s"ANSI_LGREEN" before mailing another message to that user/list of users.",interval(mail_time - now,mail_time - now,ENTITIES,0));
                                 error = 1, ptr = NULL;
			      }
                    if(!error) {
                       mail_victim = head->player;
                       mail_time   = now + MAIL_TIME;
                       mail_who    = player;
		    }

                    /* ---->  Mortals can't send mail to more than MAIL_LIMIT_GROUP_MORTAL users in one go (Group mail)  <---- */
                    if(!error && !Level4(db[player].owner)) {
                       for(ptr = head, count = 0; ptr; ptr = ptr->next, count++);
                       if(count > MAIL_LIMIT_GROUP_MORTAL) {
                          output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can't send group mail to more than "ANSI_LWHITE"%d"ANSI_LGREEN" users in one go.",MAIL_LIMIT_GROUP_MORTAL);
                          return;
		       }
		    }

                    if(!error) {

                       /* ---->  Work out who user can mail/forward message to in list  <---- */
                       for(ptr = head; ptr; ptr = next) {
                           next = ptr->next;
                           for(who = db[ptr->player].data->player.redirect, redirect = ptr->player; Validchar(who) && (db[who].data->player.redirect != NOTHING); redirect = who, who = db[who].data->player.redirect);
                           if(!Validchar(who)) who = ptr->player, redirect = NOTHING;
                           for(tmp = db[who].data->player.mail, count = 0; tmp; tmp = tmp->next, count++);
                           if(!friendflags_check(player,player,who,FRIEND_MAIL,NULL)) {
                              listsize--, cr = 1;
                              FREENULL(ptr);
			   } else if(count >= db[who].data->player.maillimit) {
                              output(p,player,0,1,0,ANSI_LGREEN"\nSorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s mailbox is full.  You can't %s mail to %s.",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0),(forward) ? "forward":"send",Objective(who,0));
                              listsize--, cr = 1;
                              FREENULL(ptr);
			   } else {
                              if(newlist) tail->next = ptr, tail = ptr;
			         else newlist = tail = ptr;
                              ptr->redirect = redirect;
                              ptr->player   = who;
                              db[ptr->player].flags |= OBJECT;
			   }
		       }
                       head = newlist;

                       /* ---->  Everyone in list blocking mail via friend flags, mailbox full, etc.  <---- */
                       if(listsize <= 0) output(p,player,0,1,0,(forward) ? ANSI_LGREEN"\nMessage not forwarded.\n":ANSI_LGREEN"\nMessage not mailed.\n"), error = 1;
		          else if(cr) output(p,player,0,1,0,"");
		    }

                    /* ---->  Mail/forward message  <---- */
                    if(!error) {
                       if(forward) {
                          if(Connected(player)) {
                             mail_forward_message(player,&head,mail,msgno,listsize,targetgroup);
                             setreturn(OK,COMMAND_SUCC);
			  } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you must be connected to forward an item of your mail to another character.");
		       } else if(!Blank(message)) {
                          char buffer[TEXT_SIZE];
                          pagetell_construct_list(NOTHING,NOTHING,(union group_data *) head,listsize,buffer,ANSI_LGREEN,ANSI_LWHITE,0,targetgroup,INDEFINITE);
                          mail_send_message(player,&head,message,subject,buffer,listsize,targetgroup);
                          setreturn(OK,COMMAND_SUCC);
		       } else if(!editing(player)) {
                          pagetell_construct_list(NOTHING,NOTHING,(union group_data *) head,listsize,scratch_return_string,ANSI_LGREEN,ANSI_LWHITE,0,targetgroup,INDEFINITE);
                          edit_initialise(player,104,NULL,(union group_data *) head,alloc_string(subject),alloc_string(scratch_return_string),EDIT_LAST_CENSOR,(listsize << 4)|(targetgroup & 0xF));
                          output(p,player,0,1,0,ANSI_LCYAN"\nSending %smail to %s...\n",(head->next) ? "group ":"",pagetell_construct_list(player,player,(union group_data *) head,listsize,scratch_return_string,ANSI_LYELLOW,ANSI_LCYAN,0,targetgroup,DEFINITE));
                          output(p,player,0,1,0,ANSI_LWHITE"Please enter your message (Pressing "ANSI_LCYAN"RETURN"ANSI_LWHITE" or "ANSI_LCYAN"ENTER"ANSI_LWHITE" after each line.)  Once you're finished, type '"ANSI_LGREEN".view"ANSI_LWHITE"' to view and check your message.  If you're happy with it, type '"ANSI_LGREEN".save"ANSI_LWHITE"' to save and mail it, otherwise type '"ANSI_LGREEN".abort = yes"ANSI_LWHITE"'.\n");
                          setreturn(OK,COMMAND_SUCC);
                          return;
		       }
		    }

                    /* ---->  Wipe list  <---- */
                    for(ptr = head; ptr; ptr = tail) {
                        db[ptr->player].flags |= OBJECT;
                        tail = ptr->next;
                        FREENULL(ptr);
		    }
                    if(!error) setreturn(OK,COMMAND_SUCC);
		 } else output(p,player,0,1,0,ANSI_LGREEN"Please specify a subject for your mailed message, e.g:  '"ANSI_LWHITE"mail fred = "ANSI_LYELLOW"A test mail"ANSI_LGREEN"'.");
	      } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of your mailed message's subject is 50 characters.");
  	   } else output(p,player,0,1,0,(forward) ? ANSI_LGREEN"Please specify who (Or a list of who) you'd like to forward the message to (E.g:  '"ANSI_LWHITE"mail forward 1 fred"ANSI_LGREEN"'.)":ANSI_LGREEN"Please specify who (Or a list of who) you'd like to mail a message to (E.g:  '"ANSI_LWHITE"mail fred = A test mail"ANSI_LGREEN"'.)");
	} else output(p,player,0,1,0,(forward) ? ANSI_LGREEN"Sorry, Morons aren't allowed to forward mail to other characters.":ANSI_LGREEN"Sorry, Morons aren't allowed to send mail to other characters.");
     } else if(!Blank(db[player].name) && instring("guest",getname(player))) output(p,player,0,1,0,(forward) ? ANSI_LGREEN"Sorry, Guest characters aren't allowed to forward mail to other characters.":ANSI_LGREEN"Sorry, Guest characters aren't allowed to send mail to other characters.");
        else output(p,player,0,1,0,(forward) ? ANSI_LGREEN"Sorry, you have been banned from forwarding mail to other characters.":ANSI_LGREEN"Sorry, you have been banned from sending mail to other characters.");
}

/* ---->  Reply to mail from another character  <---- */
void mail_reply(dbref player,const char *number,const char *message)
{
     static dbref mail_victim = NOTHING;
     static dbref mail_who = NOTHING;
     dbref  who,redirect = NOTHING;
     struct mail_data *tmp,*mail;
     static long mail_time = 0;
     short  msgno,count;
     time_t now;

     gettime(now);
     if(!in_command) {
        if(db[player].data->player.maillimit > 0) {
           if(!Moron(player)) {
              if(!Blank(number)) {
                 if((mail = lookup_message(player,number,&mail,&msgno))) {
	            if(!(mail->flags & MAIL_UNREAD)) {
                       if(Valid(mail->who)) {
                          for(who = db[mail->who].data->player.redirect, redirect = mail->who; Validchar(who) && (db[who].data->player.redirect != NOTHING); redirect = who, who = db[who].data->player.redirect);
                          if(!Validchar(who)) who = mail->who, redirect = NOTHING;
                          for(tmp = db[who].data->player.mail, count = 0; tmp; tmp = tmp->next, count++);
                          if(!friendflags_check(player,player,who,FRIEND_MAIL,NULL)) return;
		   	     else if(count >= db[who].data->player.maillimit) {
                                output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s mailbox is full.  You can't reply to mail from %s.",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0),Objective(who,0));
                                return;
			     }

                          if(!Level4(player) && (mail_who == player) && (mail_victim == who) && (now < mail_time)) {
                             output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please wait "ANSI_LWHITE"%s"ANSI_LGREEN" before mailing another reply to a message from %s"ANSI_LWHITE"%s"ANSI_LGREEN".",interval(mail_time - now,mail_time - now,ENTITIES,0),Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0));
                             return;
		          } else if(!Blank(message)) {
                             mail_reply_message(player,who,redirect,mail,msgno,message);
                             setreturn(OK,COMMAND_SUCC);
		          } else if(!editing(player)) {
                             struct mail_data *temp;

                             MALLOC(temp,struct mail_data);
                             temp->lastread = who;
                             temp->subject  = mail->subject;
                             temp->flags    = mail->flags;
                             temp->who      = mail->who;
                             temp->id       = redirect;
                             edit_initialise(player,105,NULL,(union group_data *) temp,NULL,NULL,EDIT_LAST_CENSOR,msgno);
                             output(getdsc(player),player,0,1,0,ANSI_LCYAN"\nSending reply to message "ANSI_LYELLOW"%d"ANSI_LCYAN" from %s"ANSI_LWHITE"%s"ANSI_LCYAN"...\n",msgno,Article(mail->who,LOWER,DEFINITE),getcname(NOTHING,mail->who,0,0));
                             output(getdsc(player),player,0,1,0,ANSI_LWHITE"Please enter your reply (Pressing "ANSI_LCYAN"RETURN"ANSI_LWHITE" or "ANSI_LCYAN"ENTER"ANSI_LWHITE" after each line.)  Once you're finished, type '"ANSI_LGREEN".view"ANSI_LWHITE"' to view and check your reply.  If you're happy with it, type '"ANSI_LGREEN".save"ANSI_LWHITE"' to save and mail it, otherwise type '"ANSI_LGREEN".abort = yes"ANSI_LWHITE"'.\n");
                             setreturn(OK,COMMAND_SUCC);
			  }
                          mail_victim = who;
                          mail_time   = now + MAIL_TIME;
                          mail_who    = player;
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the person who sent that mail to you no-longer exists.");
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't reply to a message which you haven't even read yet.");
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, either a message with that number doesn't exist, or the message number you specified is invalid.");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify which message you'd like to reply to (E.g:  '"ANSI_LWHITE"mail reply 1"ANSI_LGREEN"'.)");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Morons aren't allowed to reply to mail.");
	} else if(!Blank(db[player].name) && instring("guest",getname(player))) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, Guest characters aren't allowed to reply to mail.");
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have been banned from replying to mail.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"mail reply"ANSI_LGREEN"' can't be used within a compound command.");
}

/* ---->  Keep item of mail twice as long as normal  <---- */
void mail_keep(dbref player,const char *number,const char *arg2)
{
     struct mail_data *mail;
     short  msgno;

     if(!Blank(arg2)) number = arg2;
     if(!in_command) {
        if(!Blank(number)) {
           if((mail = lookup_message(player,number,&mail,&msgno))) {
              if(!(mail->flags & MAIL_UNREAD)) {
                 mail->flags |= MAIL_KEEP;
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Message "ANSI_LWHITE"%d"ANSI_LGREEN" will be kept for twice as long as normal (Mail which hasn't been read for "ANSI_LYELLOW"%d"ANSI_LGREEN" day%s will normally be deleted automatically.)",msgno,MAIL_TIME_LIMIT,Plural(MAIL_TIME_LIMIT));
                 setreturn(OK,COMMAND_SUCC);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't keep a message which you haven't even read yet.");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, either a message with that number doesn't exist, or the message number you specified is invalid.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify which message you'd like to keep.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"mail keep"ANSI_LGREEN"' can't be used within a compound command.");
}

/* ---->  Set user's mail limit (How much mail they can have in their mailbox)  <---- */
void mail_limit(dbref player,const char *name,const char *amount)
{
     int   limit;
     dbref user;

     if(!in_command) {
        if(Level4(player)) {
           if(!Blank(name)) {
              user = lookup_character(player,name,1);
              if(user != NOTHING) {
                 if(!Blank(amount)) {
                    if((user != player) || Level1(player)) {
                       if(can_write_to(player,user,1)) {
                          if((limit = atol(amount)) >= 0) {
                             if(!(!Level3(player) && (limit > MAIL_LIMIT_MORTAL))) {
		  	        if(!(!Level2(player) && (limit > MAIL_LIMIT_ADMIN))) {
   			           if(limit <= MAX_MAIL_LIMIT) {
                                      output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN"'s mailbox limit set to "ANSI_LYELLOW"%d"ANSI_LGREEN".",Article(user,UPPER,DEFINITE),getcname(NOTHING,user,0,0),limit);
                                      if(user != player) output(getdsc(player),player,0,1,0,ANSI_LRED"\n[Your mailbox limit is now "ANSI_LWHITE"%d"ANSI_LRED".]\n",limit);
                                      writelog(ADMIN_LOG,1,"MAIL","%s(#%d) set %s(#%d)'s mailbox limit to %d.",getname(player),player,getname(user),user,limit);
                                      db[user].data->player.maillimit = limit;
                                      setreturn(OK,COMMAND_SUCC);
				   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only set a character's mailbox limit to a maximum of "ANSI_LWHITE"%d"ANSI_LGREEN".",MAX_MAIL_LIMIT);
				} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, as a Wizard/Druid, you can only set a character's mailbox limit to a maximum of "ANSI_LWHITE"%d"ANSI_LGREEN".",MAIL_LIMIT_ADMIN);
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, as an Apprentice Wizard/Druid, you can only set a character's mailbox limit to a maximum of "ANSI_LWHITE"%d"ANSI_LGREEN".",MAIL_LIMIT_MORTAL);
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a character's mailbox limit can't be set to a negative number.");
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the mailbox limit of a lower level character.");
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't set your own mailbox limit  -  If you need your mailbox limit changed, please ask someone of a higher level than yourself to do it for you.");
		 } else {
                    output(getdsc(player),player,0,1,0,"\n"ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN"'s mailbox limit is "ANSI_LYELLOW"%d"ANSI_LGREEN"%s\n",Article(user,UPPER,DEFINITE),getcname(NOTHING,user,0,0),db[user].data->player.maillimit,(db[user].data->player.maillimit) ? ".":" (This character has been banned from sending mail to other characters.)");
                    setreturn(OK,COMMAND_SUCC);
		 }
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",name);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify who's mailbox limit you'd like to view/change.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above can view/change the mailbox limit of a character.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"mail limit"ANSI_LGREEN"' can't be used within a compound command.");
}

/* ---->  List, send, reply to, delete, etc. mail  <---- */
void mail_main(CONTEXT)
{
     char  command[32];
     short count = 0;
     const char *p1;
     char  *ptr;

     setreturn(ERROR,COMMAND_FAIL);
#ifndef BETA
     output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the hard-coded mail system is under development and has been disabled.");
     return;
#endif

     for(; *arg1 && (*arg1 == ' '); arg1++);
     for(ptr = command, p1 = arg1; *p1 && (*p1 != ' ') && (count < 31); command[count] = *p1, count++, p1++);
     command[count] = '\0';
     for(; *p1 && (*p1 != ' '); p1++);
     for(; *p1 && (*p1 == ' '); p1++);
     if(BlankContent(command) || string_prefix("view",command) || string_prefix("read",command) || string_prefix("list",command) || string_prefix("headers",command)) {
        mail_view(player,(char *) p1,(char *) arg2);
     } else if(string_prefix("page",command)) {
        mail_view(player,(char *) arg1,(char *) arg2);
     } else if(string_prefix("update",command) || string_prefix("summary",command)) {
        mail_update(player,p1,arg2);
     } else if(string_prefix("reply",command)) {
        mail_reply(player,p1,arg2);
     } else if(string_prefix("forwardmail",command)) {
        mail_send(player,p1,arg2,1);
     } else if(string_prefix("keepmail",command)) {
        mail_keep(player,p1,arg2);
     } else if(string_prefix("limitmail",command)) {
        mail_limit(player,p1,arg2);
     } else {
        if(!string_prefix("send",command)) p1 = arg1;
        mail_send(player,p1,arg2,0);
     }
}
