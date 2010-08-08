/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| HELP.C  -  Implements the built-in On-line Help and Tutorial Systems.       |
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
| Module originally designed and written by:  J.P.Boggis 06/07/1994.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: help.c,v 1.2 2005/06/29 21:15:13 tcz_monster Exp $

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
#include "flagset.h"
#include "html.h"


struct helptopic *generictutorials = NULL;  /*  Linked list of generic tutorials  */
struct helptopic *localtutorials   = NULL;  /*  Linked list of local tutorials  */
struct helptopic *generichelp      = NULL;  /*  Linked list of generic help topics  */
struct helppage  *titlescreen      = NULL;  /*  Linked list of title screens  */
struct helptopic *localhelp        = NULL;  /*  Linked list of local help topics  */
char   *disclaimer                 = NULL;  /*  Pointer to the disclaimer  */

       int titlescreens            = 0;     /*  Total number of title screens  */
static int totalentries            = 0;     /*  Total number of entries in linked list  */
static int totaltopics             = 0;     /*  Total number of help topics registered  */
static int totalpages              = 0;     /*  Total number of pages (In ALL topics)  */


/* ---->  Filter for help topic names  <---- */
char *help_topic_filter(char *topicname)
{
     static char buffer[256];
     char   *p1;

     *buffer = '\0';
     if(Blank(topicname)) return(buffer);     
     if(strlen(topicname) > 255) topicname[255] = '\0';
     p1 = buffer;

     /* ---->  Skip over leading blank space  <---- */
     while(*topicname && (*topicname == ' ')) topicname++;
     if(*topicname) 
        switch(*topicname) {
               case '!':
                    if(*(topicname + 1) && (*(topicname + 1) == '!')) {
                       topicname++;
                       *p1++ = '!';
		    }
               case '.':
               case ',':
               case '`':
               case '|':
               case ':':
               case ';':
               case '=':
               case '+':
               case '~':
               case '>':
               case '<':
               case '\"':
               case '\'':

                    /* ---->  Special lead-in character?  <---- */
                    *p1++ = *topicname;

                    if(*topicname != '.') {
                       topicname++;
                       while(*topicname && (*topicname == ' ')) topicname++;
                       if(!*topicname) {
                          *p1 = '\0';
                          return(buffer);
		       } else p1 = buffer;
	   	    } else topicname++;
               default:

                    /* ---->  Filter out everything except alphanumeric characters  <---- */
                    while(*topicname)
                          if(isdigit(*topicname)) {
                             while(*topicname && isdigit(*topicname)) *p1++ = *topicname++;
                             if(*topicname && (*topicname == ' ')) {
                                while(*topicname && (*topicname == ' ')) topicname++;
                                if(*topicname && isdigit(*topicname)) *p1++ = ' ';
			     }
			  } else {
                             if(isalpha(*topicname) || (*topicname == '@') || (*topicname == '?')) *p1++ = *topicname;
			     topicname++;
			  }
	}
     *p1 = '\0';
     return(buffer);
}

/* ---->  Add new help topic to sorted linked list of topics and return a pointer to it  <---- */
struct helptopic *help_add_topic(struct helptopic **topics,const char *name,int reloading,int help,int local)
{
       struct helptopic *new,*pos,*last;
       int    namelen;
     
       if(!(*topics)) {

          /* ---->  Create new linked list  <---- */
          MALLOC(new,struct helptopic);
          new->topicname = (char *) alloc_string(name);
          new->flags     = 0;
          new->pages     = 0;
          new->title     = NULL;
          new->page      = NULL;
          new->next      = NULL;
          *topics        = new;
          totalentries++;
          return(new);
       } else {

          /* ---->  Add to existing sorted linked list of topics  <---- */
          pos = *topics, last = *topics, namelen = strlen(name);
          while(pos && (strlen(pos->topicname) < namelen))
                last = pos, pos  = pos->next;

          while(pos && (strcmp(name,pos->topicname) > 0) && (strlen(pos->topicname) <= namelen))
                last = pos, pos  = pos->next;

          if(!pos) {

             /* ---->  Add topic name to end of linked list  <---- */
             MALLOC(new,struct helptopic);
             new->topicname = (char *) alloc_string(name);
             new->flags     = 0;
             new->pages     = 0;
             new->title     = NULL;
             new->page      = NULL;
             new->next      = NULL;
             last->next     = new;
             totalentries++;
             return(new);
	  } else if(!strcmp(name,pos->topicname)) {
             struct helppage *nextpage;

             /* ---->  Replace existing topic in linked list  <---- */
             if(!reloading)
                writelog(SERVER_LOG,0,"RESTART","Warning, more than one %s %s has the name '%s'.",(local) ? "local":"generic",(help) ? "help topic":"tutorial",pos->topicname);
             last = *topics;

             do {
                if(last != pos)
                   if((last->title == pos->title) || (last->page == pos->page)) {
                      last->flags = 0;
                      last->title = NULL;
                      last->pages = 0;
                      last->page  = NULL;
		   }
                last = last->next;
	     } while(last);

             for(; pos->page; pos->page = nextpage) {
                 nextpage = pos->page->next;
                 FREENULL(pos->page->text);
                 FREENULL(pos->page);
                 totalpages--;
	     }
             FREENULL(pos->title);
             return(pos);
	  } else {

             /* ---->  Insert topic name into linked list  <---- */
             MALLOC(new,struct helptopic);
             new->topicname = (char *) alloc_string(name);
             new->flags     = 0;
             new->pages     = 0;
             new->title     = NULL;
             new->page      = NULL;
             new->next      = pos;
             if(last != pos) last->next = new;
             if(pos == *topics) *topics = new;
             totalentries++;
             return(new);
	  }
       }
}

/* ---->  Register help topic with given topic name(s), title and help text  <---- */
void help_register_topic(struct helptopic **topics,char *topicnames,struct helppage *page,const char *title,unsigned char tutorial,int reloading,int help,int local)
{
     unsigned char      pagecount = 0;        /*  Count of help pages in topic           */
     char               *titleptr;            /*  Pointer to title of help topic         */
     int                first = 1;            /*  First name of topic?                   */
     struct   helppage  *pageptr;   /*  Pointer to help pages of help topic    */
     char               *p1,*p2;
     struct   helptopic *new;       /*  Pointer to newly allocated help topic  */

     if(Blank(topicnames)) return;           /*  No names for help topic being registered?  */
     if(!page || Blank(page->text)) return;  /*  No help text for the topic                 */     

     /* ---->  Strip trailing NEWLINE's and blanks from help text  <---- */
     for(pageptr = page; pageptr; pageptr = pageptr->next) pagecount++;

     /* ---->  Add name(s) of topic to linked list  <---- */
     p1 = topicnames, pageptr = NULL, titleptr = NULL;
     do {
        for(p2 = p1; *p2 && (*p2 != '\n'); p2++);
        if(*p2) *p2 = '\0';

        new = help_add_topic(topics,p1,reloading,help,local);
        if(first) {
           first      = 0;
           new->flags = (tutorial) ? HELP_TUTORIAL:0;
           new->title = titleptr = (char *) alloc_string(compress(title,0));
           new->pages = pagecount;
           new->page  = pageptr = page;
           totaltopics++;
        } else {
           new->flags = (tutorial) ? HELP_TUTORIAL:0;
           new->title = titleptr;
           new->pages = pagecount;
           new->page  = pageptr;
	}

        if((reloading > 0) && Validchar(reloading))
           output(getdsc(reloading),reloading,0,1,0,ANSI_LGREEN"%s %s '"ANSI_LYELLOW"%s"ANSI_LGREEN"' reloaded.",(local) ? "Local":"Generic",(help) ? "help topic":"tutorial",p1);
        p1 = p2, p1++;
     } while(!Blank(p1));

     /* ---->  Set any NULL pointers in linked list to title and/or help pages of the help topic that was just registered/reloaded  <---- */
     new = *topics;
     do {
        if(!new->title || !new->page) {
           new->flags = tutorial;
           new->title = titleptr;
           new->pages = pagecount;
           new->page  = pageptr;
	}
        new = new->next;
     } while(new);
     totalpages += pagecount;
}

/* ---->  Register help topic(s) with name(s) matching wildcard spec. WILDSPEC from given file  <---- */
int help_register_topics(struct helptopic **topics,char *filename,const char *wildspec,int reloading,int help,int local)
{
    char     topicnames[BUFFER_LEN],helptext[BUFFER_LEN],title[TEXT_SIZE];
    struct   helppage *page = NULL,*ptr,*new;
    int      topicfound = 0,registered = 0;
    char     buffer[BUFFER_LEN];
    unsigned char tutorial = 0;
    char     *p1,*p2;
    FILE     *f;

    *topicnames = '\0', *helptext = '\0', *title = '\0';
    if((f = fopen(filename,"r")) == NULL) {
       writelog(SERVER_LOG,0,(reloading) ? "RELOAD":"RESTART","Unable to access the %s %s file '%s'.",(local) ? "local":"generic",(help) ? "help":"tutorial",filename);
       return(0);
    } else {
       if(!reloading)
          writelog(SERVER_LOG,0,"RESTART","Registering %s %s from the file '%s'...",(local) ? "local":"generic",(help) ? "help topics":"tutorials",filename);

       while(fgets(buffer,MAX_LENGTH,f)) {
             for(p1 = buffer; *p1 && (*p1 == ' '); p1++);
	     if(!strcasecmp(p1,"%TOPIC\n") || !strncasecmp(p1,"%TOPIC ",7)) {

                /* ---->  Register previously found help topic?  <---- */
                if(topicfound == 2) {

                   /* ---->  Last page of help topic  <---- */
                   if(!Blank(helptext)) {
                      for(p2 = helptext + strlen(helptext) - 1; *p2 && ((*p2 == '\n') || (*p2 == ' ')); p2--);
                      if(*(++p2)) *p2 = '\0';

                      if(!page) {
                         MALLOC(page,struct helppage);
                         page->text = (char *) alloc_string(compress(helptext,0));
                         page->next = NULL;
		      } else {
                         for(ptr = page; ptr && ptr->next; ptr = ptr->next);
                         MALLOC(new,struct helppage);
                         ptr->next = new;
                         new->text = (char *) alloc_string(compress(helptext,0));
                         new->next = NULL;
		      }
                      *helptext = '\0';
		   }
                   help_register_topic(topics,topicnames,page,title,tutorial,reloading,help,local);
                   page = NULL, topicfound = 0, *topicnames = '\0', tutorial = 0, registered++;
		}

                /* ---->  Name(s) of help topic (Used for matching)  <---- */
                while(*p1 && (*p1 != ' ')) p1++;
                while(*p1 && (*p1 == ' ')) p1++;
                for(p2 = p1; *p2 && (*p2 != '\n'); p2++);
                if(*p2 && (*p2 == '\n')) *p2 = '\0';        

                strcpy(scratch_return_string,help_topic_filter(p1));
                if((strlen(scratch_return_string) > 0) && match_wildcard(wildspec,scratch_return_string)) {
                   if(!topicfound) topicfound = 1, *topicnames = '\0', *helptext = '\0';
                   sprintf(topicnames + strlen(topicnames),"%s\n",scratch_return_string);
                   topicnames[MAX_LENGTH] = '\0';
		}
	     } else if(topicfound) {
                if(!strcasecmp(p1,"%PAGE\n") || !strncasecmp(p1,"%PAGE ",6)) {

                   /* ---->  Next page of help topic  <---- */
                   if(!Blank(helptext)) {
                      for(p2 = helptext + strlen(helptext) - 1; *p2 && ((*p2 == '\n') || (*p2 == ' ')); p2--);
                      if(*(++p2)) *p2 = '\0';

                      if(!page) {
                         MALLOC(page,struct helppage);
                         page->text = (char *) alloc_string(compress(helptext,0));
                         page->next = NULL;
		      } else {
                         for(ptr = page; ptr && ptr->next; ptr = ptr->next);
                         MALLOC(new,struct helppage);
                         ptr->next = new;
                         new->text = (char *) alloc_string(compress(helptext,0));
                         new->next = NULL;
		      }
                      *helptext = '\0';
		   }
		} else if(!strcasecmp(p1,"%TITLE\n") || !strncasecmp(p1,"%TITLE ",7)) {

                   /* ---->  Title of help topic  <---- */        
                   while(*p1 && (*p1 != ' ')) p1++;
                   while(*p1 && (*p1 == ' ')) p1++;
                   for(p2 = title; *p1 && (*p1 != '\n'); *p2++ = *p1, p1++);
                   *p2 = '\0';
		} else if(!strcasecmp(p1,"%TUTORIAL\n") || !strncasecmp(p1,"%TUTORIAL ",10)) {
                   tutorial = 1;
		} else if(strcasecmp(p1,"%COMMENT\n") && strncasecmp(p1,"%COMMENT ",9)) {
        
                   /* ---->  Help text of current page of help topic  <---- */
                   topicfound = 2;
                   sprintf(helptext + strlen(helptext),"%s%s",(*buffer != '\n') ? " ":"",buffer);
                   helptext[MAX_LENGTH] = '\0';
		}
	     }
       }

       if(topicfound == 2) {

          /* ---->  Last page of help topic  <---- */
          if(!Blank(helptext)) {
             for(p2 = helptext + strlen(helptext) - 1; *p2 && ((*p2 == '\n') || (*p2 == ' ')); p2--);
             if(*(++p2)) *p2 = '\0';

             if(!page) {
                MALLOC(page,struct helppage);
                page->text = (char *) alloc_string(compress(helptext,0));
                page->next = NULL;
	     } else {
                for(ptr = page; ptr && ptr->next; ptr = ptr->next);
                MALLOC(new,struct helppage);
                ptr->next = new;
                new->text = (char *) alloc_string(compress(helptext,0));
                new->next = NULL;
	     }
             *helptext = '\0';
	  }
          help_register_topic(topics,topicnames,page,title,tutorial,reloading,help,local);
          registered++;
       }
    }
    fclose(f);
    return(registered);
}

/* ---->  Write On-line Help System status (Available/unavailable) to server.log log file  <---- */
void help_status(void)
{
     if(totaltopics > 0) {
        writelog(SERVER_LOG,0,"RESTART","Total of %d help topic%s/tutorial%s registered (%d page%s)  -  On-line Help System is available.",totaltopics,Plural(totaltopics),Plural(totaltopics),totalpages,Plural(totalpages));
     } else {
        writelog(SERVER_LOG,0,"RESTART","No help topics/tutorials registered  -  On-line Help System is unavailable.");
     }
}

/* ---->  Help system navigation buttons  <---- */
const char *help_buttons(struct descriptor_data *d,struct helptopic *topic,unsigned char page,unsigned char connected,int help,char *buffer)
{
      char buf[128];
      int  copied;

      *buffer = '\0';
      if(topic && (page < topic->pages) && !(command_type & HTML_ACCESS))
         sprintf(buffer + strlen(buffer),"<A HREF=\"%sTOPIC=%s+%d&%s\"%s TITLE=\"Click to view next page...\"><IMG SRC=\"%s\" ALT=\"[NEXT]\" BORDER=0></A> ",html_server_url(d,0,2,(help) ? "help":"tutorial"),html_encode(topic->topicname,buf,&copied,127),page + 1,html_get_preferences(d),(connected) ? " TARGET=_blank":"",html_image_url("next.gif"));
            else sprintf(buffer + strlen(buffer),"<IMG SRC=\"%s\" ALT=\"{NEXT}\" BORDER=0> ",html_image_url("nonext.gif"));

      if(topic && (page > 1) && !(command_type & HTML_ACCESS))
         sprintf(buffer + strlen(buffer),"<A HREF=\"%sTOPIC=%s+%d&%s\"%s TITLE=\"Click to view previous page...\"><IMG SRC=\"%s\" ALT=\"[PREVIOUS]\" BORDER=0></A> ",html_server_url(d,0,2,(help) ? "help":"tutorial"),html_encode(topic->topicname,buf,&copied,127),page - 1,html_get_preferences(d),(connected) ? " TARGET=_blank":"",html_image_url("prev.gif"));
            else sprintf(buffer + strlen(buffer),"<IMG SRC=\"%s\" ALT=\"{PREVIOUS}\" BORDER=0> ",html_image_url("noprev.gif"));

      if(help && topic && (topic->flags & HELP_TUTORIAL))
         sprintf(buffer + strlen(buffer),"<A HREF=\"%sTOPIC=%s&%s\"%s TITLE=\"Click for a tutorial...\"><IMG SRC=\"%s\" ALT=\"[TUTORIAL]\" BORDER=0></A> ",html_server_url(d,0,2,"tutorial"),html_encode(topic->topicname,buf,&copied,127),html_get_preferences(d),(connected) ? " TARGET=_blank":"",html_image_url("tutorial.gif"));
            else sprintf(buffer + strlen(buffer),"<IMG SRC=\"%s\" ALT=\"{TUTORIAL}\" BORDER=0> ",html_image_url("notutorial.gif"));

      sprintf(buffer + strlen(buffer),"<A HREF=\"%sTOPIC=INDEX&%s\"%s TITLE=\"Click for %s On-line Help...\"><IMG SRC=\"%s\" ALT=\"[HELP INDEX]\" BORDER=0></A> ",html_server_url(d,0,2,"help"),html_get_preferences(d),(connected) ? " TARGET=_blank":"",tcz_full_name,html_image_url("help.index.gif"));
      sprintf(buffer + strlen(buffer),"<A HREF=\"%sTOPIC=INDEX&%s\"%s TITLE=\"Click for %s tutorials...\"><IMG SRC=\"%s\" ALT=\"[TUTORIALS]\" BORDER=0></A> ",html_server_url(d,0,2,"tutorial"),html_get_preferences(d),(connected) ? " TARGET=_blank":"",tcz_full_name,html_image_url("tutorials.gif"));
      if(topic) sprintf(buffer + strlen(buffer),"<A HREF=\"%sSEARCHMODE=%s&%s\"%s TITLE=\"Click to search...\"><IMG SRC=\"%s\" ALT=\"[SEARCH]\" BORDER=0></A> ",html_server_url(d,0,2,"search"),(help) ? "HELP":"TUTORIAL",html_get_preferences(d),(connected) ? " TARGET=_blank":"",html_image_url("search.gif"));
         else sprintf(buffer + strlen(buffer),"<IMG SRC=\"%s\" ALT=\"{SEARCH}\" BORDER=0> ",html_image_url("nosearch.gif"));
      return(buffer);
}

/* ---->  Display given page of help topic/tutorial from linked list  <---- */
int help_display_topic(struct descriptor_data *d,struct helptopic *topic,unsigned char page,const char *topicname,int help)
{
    unsigned char connected = (IsHtml(d) && (d->flags & CONNECTED) && Validchar(d->player));
    struct   substitution_data subst;
    struct   helppage *helppage;
    int      loop,twidth;
    char     *p1,*p2;

    if(!topic || !d) return(0);
    twidth = output_terminal_width(d->player);
    if(!(command_type & HTML_ACCESS)) {
       for(loop = 1, helppage = topic->page; helppage && (loop < page); helppage = helppage->next, loop++);
       if(!helppage) helppage = topic->page, page = 1;
    } else helppage = topic->page;
    if(!helppage) return(0);

    if(!Blank(helppage->text)) {

       /* ---->  Help system navigation buttons  <---- */
       if(IsHtml(d)) output(d,d->player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4><TR><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_GREEN"><FONT SIZE=4 COLOR="HTML_LGREEN"><I>%s On-line %s</I></FONT></TD></TR><TR><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY">%s</TD></TR>",(connected) ? "<BR>":"",tcz_full_name,(help) ? "Help System":"Tutorials",help_buttons(d,topic,page,connected,help,scratch_return_string));
          else output(d,d->player,0,1,0,"\n%s",(char *) separator(twidth,0,'-','='));

       /* ---->  Display title of help topic  <---- */       
       if(!Blank(topic->title)) {
          if((topic->pages > 1) && !(command_type & HTML_ACCESS)) sprintf(scratch_return_string," %s \016&nbsp;\016(Page %d of %d)...",decompress(topic->title),page,topic->pages);
             else sprintf(scratch_return_string," %s...",decompress(topic->title));
          substitute(d->player,scratch_buffer,scratch_return_string,0,ANSI_LYELLOW,NULL,0);
       } else if((topic->pages > 1) && !(command_type & HTML_ACCESS)) sprintf(scratch_buffer,ANSI_LYELLOW" %s on '"ANSI_LWHITE"%s"ANSI_LYELLOW"' (Page %d of %d)...",(help) ? "Help":"Tutorial",String(topicname),page,topic->pages);
          else sprintf(scratch_buffer,ANSI_LYELLOW" %s on '"ANSI_LWHITE"%s"ANSI_LYELLOW"'...",(help) ? "Help":"Tutorial",String(topicname));
       output(d,d->player,2,1,1,"%s%s%s",IsHtml(d) ? "\016<TR><TH ALIGN=CENTER BGCOLOR="HTML_TABLE_YELLOW"><FONT SIZE=5><I>\016":"",scratch_buffer,IsHtml(d) ? "\016</I></FONT></TH></TR>\016":"\n");

       if(!IsHtml(d)) {
          sprintf(scratch_buffer,separator(twidth,0,'-','-'));
          if(topic->flags & HELP_TUTORIAL)
             strcpy(scratch_buffer + strlen(scratch_buffer) - 22,"["ANSI_LCYAN""ANSI_UNDERLINE"TUTORIAL AVAILABLE\016"ANSI_DCYAN"]--");
          output(d,d->player,0,1,0,"%s",scratch_buffer);
       } else output(d,d->player,1,2,0,(command_type & HTML_ACCESS) ? "</TABLE><BR>":"<TR><TD ALIGN=LEFT>");

       /* ---->  Format help text  <---- */
       do {
          subst.cur_bg = ANSI_IBLACK, subst.textflags = 0, subst.flags = 0;
          strcpy(subst.cur_ansi,ANSI_LWHITE);
          command_type |= LARGE_SUBSTITUTION;
          p1 = decompress(helppage->text);
          if(!Blank(p1)) {
             while(*p1) {
                   p2 = scratch_return_string;
                   while(*p1 && (*p1 != '\n')) {
                         if(!strncasecmp(p1," <-SEPARATOR->",14) || !strncasecmp(p1," <-SINGLE->",11)) {

                            /* ---->  Insert separating line ('-----')?  <---- */
                            if(!IsHtml(d)) {
                               *p2++ = '%', *p2++ = 'c';
                               for(loop = 0; loop < twidth; loop++) *p2++ = '-';
                               *p2++ = '%', *p2++ = 'x';
                               while(*p1 && (*p1 != '\n')) p1++;
  			    } else {
                               if(command_type & HTML_ACCESS) strcpy(p2,"\016<HR>\016%x"), p2 += 8;
                                  else strcpy(p2,"\016</TD></TR><TR><TD ALIGN=LEFT>\016%x"), p2 += 33;
                               while(*p1 && (*p1 != '\n')) p1++;
                               if(*p1 && (*p1 == '\n')) p1++;
			    }
			 } else if(!strncasecmp(p1," <-DOUBLE->",11)) {

                            /* ---->  Insert separating double line ('=====')?  <---- */
                            if(!IsHtml(d)) {
                               *p2++ = '%', *p2++ = 'c';
                               for(loop = 0; loop < twidth; loop++) *p2++ = '=';
                               *p2++ = '%', *p2++ = 'x';
                               while(*p1 && (*p1 != '\n')) p1++;
			    } else {
                               if(command_type & HTML_ACCESS) strcpy(p2,"\016<HR>\016%x"), p2 += 8;
                                  else strcpy(p2,"\016</TD></TR><TR><TD ALIGN=LEFT>\016%x"), p2 += 33;
                               while(*p1 && (*p1 != '\n')) p1++;
                               if(*p1 && (*p1 == '\n')) p1++;
			    }
			 } else while(*p1 && (*p1 != '\n')) *p2++ = *p1++;
		   }
                   if(*p1) for(p1++; *p1 && (*p1 == '\n'); *p2++ = *p1++);
                   *p2 = '\0';

                   substitute(d->player,scratch_buffer,scratch_return_string,0,ANSI_LWHITE,&subst,0);
                   if(!((p2 = (char *) strchr(scratch_buffer,'\x06')) && !((p2 > scratch_buffer) && (*(p2 - 1) == '\x05')))) output(d,d->player,0,1,0,"%s",scratch_buffer);
 	     }
             command_type &= ~LARGE_SUBSTITUTION;
             output(d,d->player,2,1,0,"");
	  }

          if(command_type & HTML_ACCESS) {
             helppage = helppage->next;
             if(helppage) output(d,d->player,0,1,0,"");
	  }
       } while((command_type & HTML_ACCESS) && helppage);

       if(command_type & HTML_ACCESS) output(d,d->player,1,2,0,"<BR><TABLE WIDTH=100%% BORDER CELLPADDING=4 BGCOLOR="HTML_TABLE_GREY">");
       if(!IsHtml(d)) {
          if((topic->pages > 1) && (page < topic->pages) && !Blank(topicname)) {
             output(d,d->player,0,1,0,(char *) separator(twidth,0,'-','-'));
             sprintf(scratch_return_string," Type '%%g%%l%%%cnext%%>%%x' (Or '%%g%%l%%%c%s %d%%>%%x') to see the next page...",(help) ? '<':'+',(help) ? '<':'+',String(topicname),page + 1);
             output(d,d->player,0,1,0,"%s",substitute(d->player,scratch_buffer,scratch_return_string,0,ANSI_LWHITE,NULL,0));
	  }
          output(d,d->player,0,1,0,separator(twidth,1,'-','='));
       } else output(d,d->player,1,2,0,"</TD></TR><TR><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY">%s</TD></TR></TABLE>%s",help_buttons(d,topic,page,connected,help,scratch_return_string),(connected) ? "<BR>":"");
       return(1);
    } else return(0);
}

/* ---->  Try and match topic given by user with a registered help topic (Return pointer to found topic if matched)  <---- */
struct helptopic *help_match_topic(const char *topic,int help)
{
     struct helptopic *pos,*local,*generic,*pnearest = NULL,*inearest = NULL;
     int    plen = INFINITY,ilen = INFINITY,clen,len,temp;

     local   = (help) ? localhelp:localtutorials;
     generic = (help) ? generichelp:generictutorials;
     if(!local) {
        local   = generic;
        generic = NULL;
     }

     if((totalentries < 1) || (!local && !generic) || Blank(topic)) return(NULL);

     for(len = strlen(topic), pos = local; pos; pos = (pos->next) ? pos->next:generic) {
         if(!Blank(pos->topicname)) {
            if(strcasecmp(topic,pos->topicname)) {
               if((temp = instring(topic,pos->topicname)) > 0) {
                  if(temp > 1) {

		     /* ---->  Within topic name match  <---- */
		     if(ABS((clen = strlen(pos->topicname)) - len) < ilen) {
			ilen     = ABS(clen - len);
			inearest = pos;
		     }
		  } else {

		     /* ---->  Prefix match  <---- */
		     if(ABS((clen = strlen(pos->topicname)) - len) < plen) {
			plen     = ABS(clen - len);
			pnearest = pos;
		     }
		  }
	       }
	    } else {

               /* ---->  Exact match  <---- */
               return(pos);
	    }
	 }

         if(pos == generic) generic = NULL;
     }

     return((pnearest) ? pnearest:inearest);   
}

/* ---->  Search on-line help and display results (HTML)  <---- */
void help_search(struct descriptor_data *d,char *topic,int page,int help)
{
     int  copied,nolocal = 0;
     char buffer[TEXT_SIZE];

     output(d,NOTHING,1,0,0,"<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_GREY"><TR><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_GREEN"><FONT SIZE=4 COLOR="HTML_LGREEN"><I>%s On-line %s</I></FONT></TD></TR><TR><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY">%s</TD></TR>",tcz_full_name,(help) ? "Help System":"Tutorials",help_buttons(d,NULL,0,0,help,buffer));
     if(!Blank(topic)) {
        int    offset = ((page * (HTML_MATCHES - 1)) + 1),matches = 0;
        struct helptopic *search,*local,*generic,*ptr;

        /* ---->  Perform search  <---- */
        if(strlen(topic) > 128) topic[128] = '\0';
        strcpy(buffer,help_topic_filter(topic));
        strcpy(topic,buffer);
        output(d,NOTHING,1,0,0,"<TR><TH BGCOLOR="HTML_TABLE_YELLOW"><FONT SIZE=5 COLOR="HTML_LYELLOW"><I>Results of search for '<FONT COLOR=%s>%s</FONT>'...</I></FONT></TH></TR><TR><TD ALIGN=LEFT BGCOLOR=%s>",(IsHtml(d) && (d->html->flags & HTML_WHITE_AS_BLACK)) ? HTML_DBLACK:HTML_LWHITE,topic,(IsHtml(d) && (d->html->flags & HTML_WHITE_AS_BLACK)) ? HTML_TABLE_WHITE:HTML_TABLE_BLACK);
        if(page > 0) output(d,NOTHING,1,0,0,"<CENTER><FONT SIZE=2><I>&nbsp;<BR><A HREF=\"%sTOPIC=%s&PAGE=%d&SEARCHMODE=%s&%s\" TITLE=\"Click to view previous page of search results...\">(Previous page of results)</A></I></FONT></CENTER><P>",html_server_url(d,0,2,"search"),html_encode(topic,buffer,&copied,128),page - 1,(help) ? "HELP":"TUTORIALS",html_get_preferences(d));

        if((totalentries > 0) && ((help) ? (generichelp || localhelp):(generictutorials || localtutorials))) {
           local   = (help) ? localhelp:localtutorials;
           generic = (help) ? generichelp:generictutorials;
           if(!local) local = generic, generic = NULL, nolocal = 1;
           for(ptr = local; ptr; ptr->flags &= ~HELP_MATCHED, ptr = (ptr->next) ? ptr->next:generic)
               if(ptr == generic) generic = NULL;

           if(!nolocal) generic = (help) ? generichelp:generictutorials;
           for(search = local; search; search = (search->next) ? search->next:generic) {
               if(!Blank(search->topicname) && !(search->flags & HELP_MATCHED) && instring(topic,search->topicname) && (matches < (HTML_MATCHES + 1))) {
                  if(offset) offset--;
                  for(ptr = search; ptr; ptr = ptr->next)
                      if((ptr->title == search->title) || (ptr->page == search->page))
                         ptr->flags |= HELP_MATCHED;

                  if(!offset) {
                     if(matches < HTML_MATCHES) {
                        if(!matches) output(d,NOTHING,1,0,0,"%s<UL>",(page > 0) ? "":"&nbsp;<BR>");
                        if(search->title) output(d,NOTHING,2,0,0,"\016<LI><A HREF=\"%sTOPIC=%s&%s\">\016%s...\016</A>\016",html_server_url(d,0,2,(help) ? "help":"tutorial"),html_encode(search->topicname,buffer,&copied,128),html_get_preferences(d),substitute(Validchar(maint_owner) ? maint_owner:ROOT,scratch_return_string,decompress(search->title),0,ANSI_LCYAN,NULL,0));
                           else output(d,NOTHING,2,0,0,"\016<LI><A HREF=\"%sTOPIC=%s&%s\">\016%s on '%s'...\016</A>\016",html_server_url(d,0,2,(help) ? "help":"tutorial"),html_encode(search->topicname,buffer,&copied,128),html_get_preferences(d),(help) ? "Help":"Tutorial",search->topicname);
                        matches++;
		     } else matches++;
		  }
	       }
               if(search == generic) generic = NULL;
	   }
	}

        if(matches > HTML_MATCHES) {
           output(d,NOTHING,1,0,0,"</UL><P><CENTER><FONT SIZE=2><I><A HREF=\"%sTOPIC=%s&PAGE=%d&SEARCHMODE=%s&%s\" TITLE=\"Click to view next page of search results...\">(Next page of results)</A></I><BR>&nbsp;</FONT></CENTER>",html_server_url(d,0,2,"search"),html_encode(topic,buffer,&copied,128),page + 1,(help) ? "HELP":"TUTORIALS",html_get_preferences(d));
	} else if(!matches) {
           output(d,NOTHING,1,0,0,"<CENTER><FONT SIZE=5 COLOR="HTML_LRED">&nbsp;<BR><B>No matching help topics found.</B><BR>&nbsp;</FONT></CENTER><P>");
	} else output(d,NOTHING,1,0,0,"</UL><BR>&nbsp;");
        output(d,NOTHING,1,0,0,"</TD></TR>");
     } else output(d,NOTHING,1,0,0,"<TR><TH ALIGN=CENTER BGCOLOR="HTML_TABLE_YELLOW"><FONT SIZE=5 COLOR="HTML_LYELLOW"><I>On-line %s Search Facility</I></FONT></TH></TR>",(help) ? "Help System":"Tutorials");

     /* ---->  Search form  <---- */
     output(d,NOTHING,1,0,0,"<TR><TD BGCOLOR="HTML_TABLE_MGREY"><FORM NAME=SEARCHFORM METHOD=POST ACTION=\"%s\"><INPUT NAME=DATA TYPE=HIDDEN VALUE=SEARCH><INPUT NAME=ID TYPE=HIDDEN VALUE=%08X><CENTER><TABLE CELLPADDING=0><TR><TD><B><FONT COLOR="HTML_LMAGENTA">Please enter topic/subject/command to search for:</FONT></B><BR>",html_server_url(d,0,0,NULL),IsHtml(d) ? d->html->identifier:0);
     output(d,NOTHING,1,0,0,"<INPUT NAME=TOPIC TYPE=TEXT SIZE=60 MAXLENGTH=128 VALUE=\"%s\"> &nbsp; &nbsp; <INPUT TYPE=SUBMIT VALUE=\"Search...\"></TD></TR></TABLE>",html_encode_basic(String(topic),buffer,&copied,128));
     output(d,NOTHING,1,0,0,"<I><B><FONT COLOR="HTML_LGREEN">Search:</FONT></B> &nbsp; &nbsp; <FONT COLOR="HTML_LCYAN"><INPUT NAME=SEARCHMODE TYPE=RADIO VALUE=HELP%s> &nbsp; On-line Help. &nbsp; &nbsp; <INPUT NAME=SEARCHMODE TYPE=RADIO VALUE=TUTORIAL%s> &nbsp; Tutorials.</FONT></I></CENTER></TD></TR></FORM>",(help) ? " CHECKED":"",(help) ? "":" CHECKED");
     output(d,NOTHING,1,0,0,"<TR><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY">%s</TD></TR></TABLE></BODY></HTML>",help_buttons(d,NULL,0,0,help,buffer));
}

/* ---->  Return pointer to random title screen  <---- */
/*        (TITLESCREENNO:  0 = Random, 1..<N> = Title screen <N>)  */
const char *help_get_titlescreen(int titlescreenno)
{
      struct helppage *ptr = titlescreen;
      int    count;

      if(!ptr) return(NULL);
      for(count = (titlescreenno) ? titlescreenno:((lrand48() % titlescreens) + 1); count > 0; count--) {
          if(ptr && ptr->next) ptr = ptr->next;
             else ptr = titlescreen;
      }
      return((ptr) ? ptr->text:NULL);
}

/* ---->  (Re)load text  <---- */
char *help_reload_text(const char *filename,unsigned char limit,unsigned char compression,u_char addcr)
{
     int  sizelimit = (limit) ? MAX_LENGTH:((2 * BUFFER_LEN) - 1);
     char text[BUFFER_LEN],line[BUFFER_LEN];
     char *ptr,*ptr2,*loadedtext = NULL;
     int  fsize = 0,count,tabcount;
     FILE *f;
      
     if((f = fopen(filename,"r")) != NULL) {
        *text = '\0';
        while(fgets(line,sizelimit,f) && ((fsize + strlen(text) + 1) <= sizelimit)) {

              /* ---->  Strip hard-coded ANSI codes from line  <---- */
	      ansi_code_filter(line,line,1);

              /* ---->  Strip non-printable characters from string  <---- */
              for(count = 0, ptr = (text + strlen(text)), ptr2 = line; *ptr2 && (count < (sizelimit - fsize - 2)); ptr2++) {
                  if(*ptr2 == '\t') {

                     /* ---->  TAB  <---- */
                     tabcount = count;
                     while((count < (((tabcount / 8) + 1) * 8)) && (count < (sizelimit - fsize - 2))) {
                         *ptr++ = ' ';
                         count++;
		     }
		  } else if((*ptr2 == '\n') || (isascii(*ptr2) && isprint(*ptr2))) {
                     *ptr++ = *ptr2;
                     count++;
		  }
	      }

              *ptr = '\0';
              fsize += count;
	}

        if(fsize < 1) {
           fclose(f);
           return(NULL);
	} else {
	   if(addcr) {
              for(ptr = text + strlen(text) - 1; (ptr > text) && (*ptr == '\n'); *ptr-- = '\0');
              for(ptr++; addcr && (ptr < (text + sizelimit)); *ptr++ = '\n', addcr--);
              *ptr = '\0';
	   }
 
           if(compression) ptr = compress(text,0);
              else ptr = text;

           NMALLOC(loadedtext,char,strlen(ptr) + 1);
           strcpy(loadedtext,ptr);
	}
        fclose(f);
        return(loadedtext);
     } else return(NULL);
}

/* ---->  Reload title screens  <---- */
unsigned char help_reload_titles(void)
{
	 unsigned char     finished = 0;
	 char              buffer[256];
	 int               loaded = 0;
	 struct   helppage *new;
	 char              *ptr;

	 /* ---->  Clear title screens  <---- */
	 for(new = titlescreen; new; new = titlescreen) {
	     titlescreen = titlescreen->next;
	     FREENULL(new->text);
	     FREENULL(new);
	 }
	 titlescreen = NULL;

	 /* ---->  Load title screens  <---- */
	 while(!finished) {
	       sprintf(buffer,TITLE_FILE,loaded + 1);
	       if((ptr = help_reload_text(buffer,0,1,2))) {
		  MALLOC(new,struct helppage);
		  new->text = ptr;
		  if(titlescreen) new->next = titlescreen;
		     else new->next = NULL;
		  titlescreen = new;
		  loaded++;
	       } else finished = 1;
	 }
	 titlescreens = loaded;
	 return(loaded > 0);
}

/* ---->  Reload title screens, termcap, disclaimer or given help topic/tutorial  <---- */
void help_reload(CONTEXT)
{
     int local;

     setreturn(ERROR,COMMAND_FAIL);
     if(Level1(Owner(player))) {
        if(!in_command) {
           if(Blank(arg2)) {
              for(arg1 = params;  *arg1 && (*arg1 == ' '); arg1++);
              for(arg2 = arg1;    *arg2 && (*arg2 != ' '); arg2++);
              for(*arg2++ = '\0'; *arg2 && (*arg2 == ' '); arg2++);
	   }

           if(!Blank(arg1) && (string_prefix("titlescreens",arg1) || string_prefix("titles",arg1) || string_prefix("motd",arg1))) {

              /* ---->  Reload title screens  <---- */
              if(help_reload_titles()) {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Title screens reloaded.");
                 writelog(SERVER_LOG,1,"RELOAD","Title screens reloaded by %s(#%d).",getname(player),player);
	      } else {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, couldn't reload title screens.");
                 writelog(SERVER_LOG,1,"RELOAD","Unable to reload title screens (Attempted by %s(#%d).)",getname(player),player);
                 return;
	      }
	   } else if(!Blank(arg1) && string_prefix("termcap",arg1)) {
              struct descriptor_data *d;

              /* ---->  Reload database of terminal definitions  <---- */
              for(d = descriptor_list; d; d = d->next) d->termcap = NULL;
              if(!reload_termcap(1)) {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, couldn't reload database of terminal definitions.");
                 for(d = descriptor_list; d; d = d->next)
                     FREENULL(d->terminal_type);
                 return;
	      } else {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Database of terminal definitions reloaded.");
                 for(d = descriptor_list; d; d = d->next) {
                     if(!Blank(d->terminal_type)) {
                        if(!set_terminal_type(d,d->terminal_type,0))
                           FREENULL(d->terminal_type);
		     }
		 }
	      }
	   } else if(!Blank(arg1) && string_prefix("disclaimer",arg1)) {

              /* ---->  Reload disclaimer  <---- */
              FREENULL(disclaimer);
              disclaimer = (char *) help_reload_text(DISC_FILE,0,1,1);
              if(disclaimer) {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Disclaimer reloaded.");
                 writelog(SERVER_LOG,1,"RELOAD","Disclaimer reloaded by %s(#%d).",getname(player),player);
	      } else {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, couldn't reload the disclaimer.");
                 writelog(SERVER_LOG,1,"RELOAD","Unable to reload the disclaimer (Attempted by %s(#%d).)",getname(player),player);
                 return;
	      }
	   } else if(!Blank(arg1) && string_prefix("map",arg1)) {

              /* ---->  Reload TCZ map  <---- */
              if(!map_reload(player,1)) return;
	   } else if(!Blank(arg1) && 
              (((string_prefix("help",arg1) || string_prefix("helptext",arg1) || string_prefix("helptopics",arg1)) && (local = NOTHING)) ||
              ((string_prefix("localhelp",arg1) || string_prefix("localhelptext",arg1) || string_prefix("localhelptopics",arg1)) && (local = 1)) ||
              ((string_prefix("generichelp",arg1) || string_prefix("generichelptext",arg1) || string_prefix("generichelptopics",arg1)) && !(local = 0)))) {
                 if(!Blank(arg2)) {
                    int registered;

		    /* ---->  Reload local/generic help topic(s)  <---- */
                    if(local == NOTHING) {
                       if(!(registered = help_register_topics(&localhelp,LOCAL_HELP_FILE,arg2,player,1,1)))
                          registered = help_register_topics(&generichelp,GENERIC_HELP_FILE,arg2,player,1,0);
		    } else registered = help_register_topics((local) ? &localhelp:&generichelp,(local) ? LOCAL_HELP_FILE:GENERIC_HELP_FILE,arg2,player,1,(local) ? 1:0);

		    if(!registered) {
		       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, couldn't reload the specified %s help topic(s).",(local == NOTHING) ? "local/generic":(local) ? "local":"generic");
		       writelog(SERVER_LOG,1,"RELOAD","Unable to reload %s help topic(s) matching wildcard specification '%s' (Attempted by %s(#%d).)",(local == NOTHING) ? "local/generic":(local) ? "local":"generic",arg2,getname(player),player);
		       return;
		    } else writelog(SERVER_LOG,1,"RELOAD","%s help topic(s) matching wildcard specification '%s' reloaded by %s(#%d).",(local == NOTHING) ? "Local/generic":(local) ? "Local":"Generic",arg2,getname(player),player);
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify which %s help topic(s) you would like to reload.",(local == NOTHING) ? "local/generic":(local) ? "local":"generic");
	   } else if(!Blank(arg1) && 
              (((string_prefix("tutorials",arg1) || string_prefix("tutorialtext",arg1) || string_prefix("tutorialtopics",arg1)) && (local = NOTHING)) ||
              ((string_prefix("localtutorials",arg1) || string_prefix("localtutorialtext",arg1) || string_prefix("localtutorialtopics",arg1)) && (local = 1)) ||
              ((string_prefix("generictutorials",arg1) || string_prefix("generictutorialtext",arg1) || string_prefix("generictutorialtopics",arg1)) && !(local = 0)))) {
                 if(!Blank(arg2)) {
                    int registered;

		    /* ---->  Reload local/generic tutorial(s)  <---- */
                    if(local == NOTHING) {
                       if(!(registered = help_register_topics(&localtutorials,LOCAL_TUTORIAL_FILE,arg2,player,0,1)))
                          registered = help_register_topics(&generictutorials,GENERIC_TUTORIAL_FILE,arg2,player,0,0);
		    } else registered = help_register_topics((local) ? &localtutorials:&generictutorials,(local) ? LOCAL_TUTORIAL_FILE:GENERIC_TUTORIAL_FILE,arg2,player,0,(local) ? 1:0);
	   
		    if(!registered) {
		       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, couldn't reload the specified %s tutorial(s).",(local == NOTHING) ? "local/generic":(local) ? "local":"generic");
		       writelog(SERVER_LOG,1,"RELOAD","Unable to reload %s tutorial(s) matching wildcard specification '%s' (Attempted by %s(#%d).)",(local == NOTHING) ? "local/generic":(local) ? "local":"generic",arg2,getname(player),player);
		       return;
		    } else writelog(SERVER_LOG,1,"RELOAD","%s tutorial(s) matching wildcard specification '%s' reloaded by %s(#%d).",(local == NOTHING) ? "Local/generic":(local) ? "Local":"Generic",arg2,getname(player),player);
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify which %s tutorial(s) you would like to reload.",(local == NOTHING) ? "local/generic":(local) ? "local":"generic");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LWHITE"help"ANSI_LGREEN"', '"ANSI_LWHITE"tutorial"ANSI_LGREEN"', '"ANSI_LWHITE"localhelp"ANSI_LGREEN"', '"ANSI_LWHITE"generichelp"ANSI_LGREEN"', '"ANSI_LWHITE"localtutorial"ANSI_LGREEN"', '"ANSI_LWHITE"generictutorial"ANSI_LGREEN"', '"ANSI_LWHITE"titles"ANSI_LGREEN"', '"ANSI_LWHITE"termcap"ANSI_LGREEN"' or '"ANSI_LWHITE"disclaimer"ANSI_LGREEN"' (E.g:  '"ANSI_LYELLOW"@reload help @command"ANSI_LGREEN"'.)");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@reload"ANSI_LGREEN"' can't be used from within a compound command.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Deities and the Supreme Being may use '"ANSI_LWHITE"@reload"ANSI_LGREEN"'  -  If you're having trouble using the On-line Help System, please ask an Apprentice Wizard/Druid or above for help.\n");
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Give help/tutorial on given topic/subject  <---- */
/*        (val1:  0 = Help, 1 = Tutorial.)                 */
void help_main(CONTEXT)
{
     struct   descriptor_data *p = getdsc(player);
     unsigned char next = 0,prev = 0,current = 0;
     struct   descriptor_data *d = NULL;
     struct   helptopic *topic = NULL;
     char     topicname[256];
     const    char *p2;
     int      page = 1;
     char     *p1;

     if(player == NOBODY) {
        for(p = descriptor_list; p && (p->player != NOBODY); p = p->next);
        player = NOTHING;
        if(!p) return;
        if(!IsHtml(p)) p->player = NOTHING;
     }

     /* ---->  Next/previous page of help topic?  <---- */
     setreturn(ERROR,COMMAND_FAIL);
     strcpy(topicname,help_topic_filter(params));

     if(Validchar(player)) {
        for(d = descriptor_list; d && (d->player != player); d = d->next);
        if((next = !strcasecmp("next",topicname)) || (prev = (!strcasecmp("prev",topicname) || !strcasecmp("previous",topicname))) || (current = !strcasecmp("current",topicname))) {
           if(d && d->helptopic && !Blank(d->helpname)) {
              page  = d->currentpage;
              topic = d->helptopic;
              strcpy(topicname,d->helpname);
              if(current) {
                 if(page > d->helptopic->pages) page = d->helptopic->pages;
	      } else if(next && (++page > d->helptopic->pages)) {
                 if(val1) output(p,player,0,1,0,ANSI_LGREEN"Sorry, you have just read the last page of the tutorial '"ANSI_LWHITE"%s"ANSI_LGREEN"'  -  There are no further pages available.",d->helpname);
                    else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you have just read the last page of available help in the help topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'  -  There are no further pages of help available.",d->helpname);
                 return;
	      } else if(prev && (--page < 1)) {
                 if(val1) output(p,player,0,1,0,ANSI_LGREEN"Sorry, you have just read the first page of the tutorial '"ANSI_LWHITE"%s"ANSI_LGREEN"'  -  There are no more previous pages available.",d->helpname);
                    else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you have just read the first page of available help in the help topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'  -  There are no more previous pages of help available.",d->helpname);
                 return;
	      }
	   } else {
              output(p,player,0,1,0,(val1) ? ANSI_LGREEN"Sorry, the tutorial you last browsed doesn't have multiple pages.":ANSI_LGREEN"Sorry, the help topic you last browsed doesn't have multiple pages.");
              return;
	   }
	}
     }

     /* ---->  Parse page number  <---- */
     if(!next && !prev && !current && !Blank(topicname)) {
        char *menu = NULL;

        if(!strncasecmp(topicname,"menu",4)) {
           if(isdigit(*(topicname + 4))) {
              menu = topicname + 4;
	   } else if(*(topicname + 4) == ' ') {
              for(menu = topicname + 4; *menu && (*menu == ' '); menu++);
	   }
	}

        for(p1 = topicname + strlen(topicname) - 1; (p1 >= topicname) && (*p1 == ' '); *p1-- = '\0');
        if((p1 >= topicname) && isdigit(*p1)) {
           for(; (p1 >= topicname) && isdigit(*p1); p1--);
           if((p1 >= topicname) && *p1 && !isdigit(*p1)) p1++;
           if((p1 >= topicname) && (p1 != topicname) && (p1 != menu) && !Blank(p1)) {
              if((page = atol(p1)) < 1) page = 1;
              for(*(p1--) = '\0'; (p1 >= topicname) && (*p1 == ' '); *p1-- = '\0');
	   }
	}
     }

     /* ---->  Lookup and display given page of help topic/tutorial (If it exists)  <---- */
     if((!val1) ? (generichelp || localhelp):(generictutorials || localtutorials)) {
        if(!topic) {
           if(Blank(topicname)) strcpy(topicname,"index");
           topic = help_match_topic(topicname,!val1);
           if(topic && (topic->pages < page)) {
              output(p,player,0,1,0,ANSI_LGREEN"Sorry, the %s '"ANSI_LWHITE"%s"ANSI_LGREEN"' only has "ANSI_LYELLOW"%d"ANSI_LGREEN" page%s%s.",(val1) ? "tutorial":"help topic",topicname,topic->pages,Plural(topic->pages),(val1) ? "":" of help");
              return;
	   }
	}

        if(topic) {
           if(!prev && !next && !current) {
              for(p1 = topicname, p2 = params; *p2; p1++, p2++) {
                  *p1 = *p2;
                  if(isupper(*p1)) *p1 = tolower(*p1);
	      }
              *p1 = '\0';
           
              if(!Blank(topicname) && !isdigit(*topicname) && strncasecmp(topicname,"menu",4)) {
                 for(p1 = topicname + strlen(topicname) - 1; (p1 >= topicname) && (isdigit(*p1) || (*p1 == ' ')); p1--);
                 *(++p1) = '\0';
	      }
	   }

           for(d = descriptor_list; d; d = d->next)
               if(d->player == player) {
                  if(!next && !prev && !current) {
                     FREENULL(d->helpname);
                     d->helpname = (char *) alloc_string(topicname);
		  }
                  d->currentpage = page;
                  d->helptopic   = topic;
	       }

           help_display_topic(p,topic,page,topicname,!val1);
           setreturn(OK,COMMAND_SUCC);
	} else output(p,player,0,1,0,ANSI_LGREEN"Sorry, there isn't %s available on the subject '"ANSI_LYELLOW"%s"ANSI_LGREEN"'.",(val1) ? "a tutorial":"any help",params);
     } else output(p,player,0,1,0,(val1) ? ANSI_LGREEN"Sorry, there are no tutorials available at the moment  -  Please ask an Apprentice Wizard/Druid or above for help.":ANSI_LGREEN"Sorry, the On-Line Help System is unavailable at the moment  -  Please ask an Apprentice Wizard/Druid or above for help.");
}

