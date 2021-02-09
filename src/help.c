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
|                        https://github.com/smcvey/tcz                        |
`-----------------------------------------------------------------------------'

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
                   if(!BlankContent(helptext)) {
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
                   if(!BlankContent(helptext)) {
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
          if(!BlankContent(helptext)) {
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

/* ---->  Display given page of help topic/tutorial from linked list  <---- */
int help_display_topic(struct descriptor_data *d,struct helptopic *topic,unsigned char page,const char *topicname,int help)
{
    struct   substitution_data subst;
    struct   helppage *helppage;
    int      loop,twidth;
    char     *p1,*p2;

    if(!topic || !d) return(0);
    twidth = output_terminal_width(d->player);
    for(loop = 1, helppage = topic->page; helppage && (loop < page); helppage = helppage->next, loop++);
    if(!helppage) helppage = topic->page, page = 1;
    if(!helppage) return(0);

    if(!Blank(helppage->text)) {

       output(d,d->player,0,1,0,"\n%s",(char *) separator(twidth,0,'-','='));

       /* ---->  Display title of help topic  <---- */       
       if(!Blank(topic->title)) {
          if(topic->pages > 1) sprintf(scratch_return_string," %s (Page %d of %d)...",decompress(topic->title),page,topic->pages);
             else sprintf(scratch_return_string," %s...",decompress(topic->title));
          substitute(d->player,scratch_buffer,scratch_return_string,0,ANSI_LYELLOW,NULL,0);
       } else if(topic->pages > 1) sprintf(scratch_buffer,ANSI_LYELLOW" %s on '"ANSI_LWHITE"%s"ANSI_LYELLOW"' (Page %d of %d)...",(help) ? "Help":"Tutorial",String(topicname),page,topic->pages);
         else sprintf(scratch_buffer,ANSI_LYELLOW" %s on '"ANSI_LWHITE"%s"ANSI_LYELLOW"'...",(help) ? "Help":"Tutorial",String(topicname));
       output(d, d->player, 2, 1, 1, "%s\n", scratch_buffer);

       sprintf(scratch_buffer,separator(twidth,0,'-','-'));
       if(topic->flags & HELP_TUTORIAL)
          strcpy(scratch_buffer + strlen(scratch_buffer) - 22,"["ANSI_LCYAN""ANSI_UNDERLINE"TUTORIAL AVAILABLE"ANSI_DCYAN"]--");
       output(d,d->player,0,1,0,"%s",scratch_buffer);

       /* ---->  Format help text  <---- */
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
                         *p2++ = '%', *p2++ = 'c';
                         for(loop = 0; loop < twidth; loop++) *p2++ = '-';
                         *p2++ = '%', *p2++ = 'x';
                         while(*p1 && (*p1 != '\n')) p1++;
                      } else if(!strncasecmp(p1," <-DOUBLE->",11)) {

                         /* ---->  Insert separating double line ('=====')?  <---- */
                         *p2++ = '%', *p2++ = 'c';
                         for(loop = 0; loop < twidth; loop++) *p2++ = '=';
                         *p2++ = '%', *p2++ = 'x';
                         while(*p1 && (*p1 != '\n')) p1++;
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

       if((topic->pages > 1) && (page < topic->pages) && !Blank(topicname)) {
          output(d,d->player,0,1,0,(char *) separator(twidth,0,'-','-'));
          sprintf(scratch_return_string," Type '%%g%%l%%u%s next%%x' (Or '%%g%%l%%u%s %s %d%%x') to see the next page...",(help) ? "help":"tutorial",(help) ? "help":"tutorial",String(topicname),page + 1);
          output(d,d->player,0,1,0,"%s",substitute(d->player,scratch_buffer,scratch_return_string,0,ANSI_LWHITE,NULL,0));
       }
       output(d,d->player,0,1,0,separator(twidth,1,'-','='));
       return(1);
    } else return(0);
}

/* ---->  Try and match topic given by user with a registered help topic (Return pointer to found topic if matched)  <---- */
struct helptopic *help_match_topic(const char *topic,int help)
{
     struct helptopic *pos,*local,*generic,*pnearest = NULL,*inearest = NULL;
     int    plen = TCZ_INFINITY,ilen = TCZ_INFINITY,clen,len,temp;

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
        p->player = NOTHING;
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
     if(!next && !prev && !current && !BlankContent(topicname)) {
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
           if(BlankContent(topicname)) strcpy(topicname,"index");
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
           
              if(!BlankContent(topicname) && !isdigit(*topicname) && strncasecmp(topicname,"menu",4)) {
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

