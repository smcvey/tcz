/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| TERMCAP.C  -  Database of terminal definitions (Constructed from system     |
|               /etc/termcap at run-time) for fast and efficient lookup of    |
|               terminal details.                                             |
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
| Module originally designed and written by:  J.P.Boggis 26/11/1995.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: termcap.c,v 1.2 2005/06/29 20:11:14 tcz_monster Exp $

*/


#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"


static struct termcap_data *termcap = NULL;


/* ---->  Set terminal type  <---- */
unsigned char set_terminal_type(struct descriptor_data *d,char *termtype,unsigned char log)
{
         struct termcap_data *current;
         char   buffer[BUFFER_LEN];
         char   *ptr;

	 /* ---->  Filter given terminal name  <---- */
	 if(Blank(termtype)) return(0);
	 filter_spaces(buffer,termtype,0);
	 for(ptr = buffer; *ptr; ptr++)
	     if(isupper(*ptr)) *ptr = tolower(*ptr);

	 if(d && (!strcmp(buffer,"none") || !strcmp(buffer,"dumb"))) {
	    d->termcap = NULL;
	    return(1);
	 }

	 /* ---->  Look up in database of terminal definitions  <---- */
	 for(current = termcap; current && !string_matched_list(buffer,current->name,'|',0); current = current->next);
	 if(!current) {
	     /* if(log) writelog(TERMCAP_LOG,1,"TERMCAP","Terminal type '%s' isn't in the terminal database.",buffer); */
	    return(0);
	 }

	 FREENULL(d->terminal_type);
	 d->terminal_type = (char *) alloc_string(buffer);
	 d->termcap       = current;

	 if(!d->terminal_width)  d->terminal_width  = current->width - 1;
	 if(!d->terminal_height) d->terminal_height = current->height;

	 /* ---->  Set title of Xterm  <---- */
	 if(!strcmp(buffer,"xterm")) {
	    sprintf(buffer,"\033]2;"HTML_TITLE"\007",tcz_full_name,tcz_year);
	    server_queue_output(d,buffer,strlen(buffer));
	 }
	 return(1);
}

/* ---->  Clear database of terminal definitions  <---- */
void clear_termcap()
{
     struct termcap_data *next,*ptr;

     for(ptr = termcap; ptr; ptr = next) {
         next = ptr->next;
         FREENULL(ptr->underline);
         FREENULL(ptr->inverse);
         FREENULL(ptr->normal);
         FREENULL(ptr->blink);
         FREENULL(ptr->bold);
         FREENULL(ptr->name);
         FREENULL(ptr->tc);
         FREENULL(ptr);
     }
     termcap = NULL;
}

/* ---->  (Re)load database of terminal definitions (Constructed from /etc/termcap)  <---- */
unsigned char reload_termcap(unsigned char reload)
{
	 struct termcap_data *new,*current = NULL;
         char   buffer3[BUFFER_LEN];
         char   buffer2[BUFFER_LEN];
	 char   buffer[BUFFER_LEN];
	 struct str_ops str_data;
	 char   *ptr,*ptr2;
	 short  count = 0;
	 int    value;
	 char   chr;
	 FILE   *f;

	 str_data.dest   = buffer3;
	 str_data.length = 0;

	 if((f = fopen(TERMCAP_FILE,"r")) == NULL) {
	    writelog(SERVER_LOG,0,(reload) ? "RELOAD":"RESTART","Unable to access terminal capabilities file '%s'  -  No terminal information available.",TERMCAP_FILE);
	    return(0);
	 } else {
	    writelog(SERVER_LOG,0,(reload) ? "RELOAD":"RESTART","Registering terminal definitions from the file '%s'...",TERMCAP_FILE);
	    clear_termcap();
	    while(fgets(buffer,BUFFER_LEN,f)) {
		  for(ptr = buffer; *ptr && ((*ptr == ' ') || (*ptr == '\x09')); ptr++);
		  if(*ptr && (*ptr != '#')) {
		     for(ptr2 = ptr; *ptr2 && *(ptr2 + 1); ptr2++);
		     for(; *ptr2 && ((*ptr2 == '\n') || (*ptr2 == '\r')); *ptr2 = '\0', ptr2--);
		     for(; *ptr2 && (*ptr2 == ' ') && (ptr2 > buffer); *ptr2 = '\0', ptr2--);
		     if(ptr2 > buffer) chr = *ptr2, *ptr2 = '\0';
			else chr = '\0';
		     strcat_limits(&str_data,ptr);

		     if(chr != '\\') {
			*(str_data.dest) = '\0';

			/* ---->  Name(s) of terminal  <---- */
			for(ptr = buffer3, ptr2 = buffer2; *ptr && (*ptr != ':'); *ptr2++ = *ptr, ptr++);
			*ptr2 = '\0';

			if(!Blank(buffer2)) {

			   /* ---->  Create and initialise new entry  <---- */
			   MALLOC(new,struct termcap_data);
			   new->underline = NULL;
			   new->inverse   = NULL;
			   new->height    = STANDARD_CHARACTER_SCRHEIGHT;
			   new->normal    = NULL;
			   new->blink     = NULL;
			   new->width     = STANDARD_CHARACTER_SCRWIDTH;
			   new->bold      = NULL;
			   new->name      = (char *) alloc_string(buffer2);
			   new->next      = NULL;
			   new->tc        = NULL;
			   if(termcap) {
			      current->next = new;
			      current       = new;
			   } else termcap = current = new;
			   count++;

			   /* ---->  Terminal parameters  <---- */
			   while(*ptr) {

				 /* ---->  Get field  <---- */
				 for(; *ptr && (*ptr == ':'); ptr++);
				 for(; *ptr && (*ptr == ' '); ptr++);
				 ptr2 = buffer2;

				 while(*ptr && (*ptr != ':')) {
				       switch(*ptr) {
					      case '\\':

						   /* ---->  Special code/protecting backslash  <---- */
						   if(*(++ptr)) {
						      switch(*ptr) {
							     case 'E':
							     case 'e':
								  *ptr2++ = '\x1B';
								  break;
							     case 'n':
							     case 'N':
								  *ptr2++ = '\x0A';
								  break;
							     case 'r':
							     case 'R':
								  *ptr2++ = '\x0D';
								  break;
							     case 't':
							     case 'T':
								  *ptr2++ = '\x09';
								  break;
							     case 'b':
							     case 'B':
								  *ptr2++ = '\x08';
								  break;
							     case 'f':
							     case 'F':
								  *ptr2++ = '\x0C';
								  break;
							     case '0':
							     case '1':
							     case '2':
							     case '3':
							     case '4':
							     case '5':
							     case '6':
							     case '7':
							     case '8':
							     case '9':
								  if(!(*(ptr + 1) && !isdigit(*(ptr + 1)))) {
								     chr = ((*ptr - 48) << 6);
								     if(*(ptr + 1) && isdigit(*(ptr + 1))) {
									chr |= ((*(++ptr) - 48) << 3);
									if(*(ptr + 1) && isdigit(*(ptr + 1))) {
									   chr |= (*(++ptr) - 48);
									   *ptr2++ = chr;
									}
								     }
								  } else *ptr2++ = '\x00';
								  break;
							     default:
								  *ptr2++ = *ptr;
						      }
						      ptr++;
						   }
						   break;
					      case '^':

						   /* ---->  Control character  <---- */
						   if(*(++ptr)) switch(*ptr) {
						      case '@':
							   *ptr2++ = '\x00';
							   break;
						      case '\\':
							   *ptr2++ = '\x1C';
							   break;
						      case '`':
							   *ptr2++ = '\x1D';
							   break;
						      case '=':
							   *ptr2++ = '\x1E';
							   break;
						      case '-':
							   *ptr2++ = '\x1F';
							   break;
						      default:
							   if((*ptr >= 65) && (*ptr <= 90)) *ptr2++ = (*ptr - 64);
							      else if((*ptr >= 97) && (*ptr <= 122)) *ptr2++ = (*ptr - 96);
						   }
						   ptr++;
						   break;
					      default:
						   *ptr2++ = *ptr;
						   ptr++;
				       }
				 }
				 *ptr2 = '\0';

				 /* ---->  Handle field  <---- */
				 if(strlen(buffer2) > 2)
				    switch(*(ptr2 = buffer2)) {
					   case 'c':

						/* ---->  Default width  <---- */
						if((*(ptr2 + 1) == 'o') && (*(ptr2 + 2) == '#') && !Blank(ptr2 += 3)) {
						   value = atol(ptr2);
						   if((value >= 20) && (value <= 255)) new->width = value;
						}
						break;
					   case 'l':

						/* ---->  Default height  <---- */
						if((*(ptr2 + 1) == 'i') && (*(ptr2 + 2) == '#') && !Blank(ptr2 += 3)) {
						   value = atol(ptr2);
						   if((value >= 10) && (value <= 255)) new->height = value;
						}
						break;
					   case 'm':
						switch(*(ptr2 + 1)) {
						       case 'b':

							    /* ---->  Blink (Flashing)  <---- */
							    if(*(ptr2 + 2) == '=') new->blink = (char *) alloc_string(ptr2 + 3);
							    break;
						       case 'd':

							    /* ---->  Bold  <---- */
							    if(*(ptr2 + 2) == '=') new->bold = (char *) alloc_string(ptr2 + 3);
							    break;
						       case 'e':

							    /* ---->  Normal  <---- */
							    if(*(ptr2 + 2) == '=') new->normal = (char *) alloc_string(ptr2 + 3);
							    break;
						       case 'r':

							    /* ---->  Inverse  <---- */
							    if(*(ptr2 + 2) == '=') new->inverse = (char *) alloc_string(ptr2 + 3);
							    break;
						}
						break;
					   case 'u':

						/* ---->  Underline  <---- */
						if((*(ptr2 + 1) == 's') && (*(ptr2 + 2) == '=')) new->underline = (char *) alloc_string(ptr2 + 3);
						break;
					   case 't':

						/* ---->  Get rest of params from another entry  <---- */
						if((*(ptr2 + 1) == 'c') && (*(ptr2 + 2) == '=')) new->tc = (char *) alloc_string(ptr2 + 3);
						break;
				    }
			   }
			}

			str_data.dest   = buffer3;
			str_data.length = 0;
		     }
		  }
	    }

	    /* ---->  Handle entries which need to get params from another entry (Using 'tc=termtype')  <---- */
	    for(current = termcap; current; current = current->next) {
		if(current->tc) {
		   for(new = termcap; new; new = (new) ? new->next:NULL)
		       if((new != current) && string_matched_list(current->tc,new->name,'|',0)) {
			  if(!current->underline) current->underline = (char *) alloc_string(new->underline);
			  if(!current->inverse)   current->inverse   = (char *) alloc_string(new->inverse);
			  if(!current->normal)    current->normal    = (char *) alloc_string(new->normal);
			  if(!current->blink)     current->blink     = (char *) alloc_string(new->blink);
			  if(!current->bold)      current->bold      = (char *) alloc_string(new->bold);
			  new = NULL;
		       }
		   FREENULL(current->tc);
		}

		/* ---->  No 'normal' code  <---- */
		if(Blank(current->normal)) {
		   FREENULL(current->underline);
		   FREENULL(current->inverse);
		   FREENULL(current->normal);
		   FREENULL(current->blink);
		   FREENULL(current->bold);
		}
	    }

	    writelog(SERVER_LOG,0,(reload) ? "RELOAD":"RESTART","  %d terminal definitions %sloaded.",count,(reload) ? "re":"");
	    fclose(f);
	    return(termcap != NULL);
	 }
}
