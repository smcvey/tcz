/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| PAGER.C  -  Implements a 'more' paging facility for paging large output.    |
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
| Module originally designed and written by:  J.P.Boggis 02/05/1996.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: pager.c,v 1.1.1.1 2004/12/02 17:42:07 jpboggis Exp $

*/


#include <stdlib.h>
#include <string.h>

#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "object_types.h"
#include "flagset.h"


/* ---->  Initialise 'more' pager  <---- */
void pager_init(struct descriptor_data *d)
{
     if(!d->pager && (d->flags & CONNECTED) && (d->clevel == 0)) {
        MALLOC(d->pager,struct pager_data);
        d->pager->current = d->pager->head = d->pager->tail = NULL;
        d->pager->prompt  = NULL;
        d->pager->lines   = 0;
        d->pager->line    = 1;
        d->pager->size    = 0;
     }
}

/* ---->  Add line to 'more' pager data  <---- */
void pager_add(struct descriptor_data *d,unsigned char *text,int len)
{
     int           loop = 0,count;
     struct        line_data *new;
     unsigned char *ptr,*start;

     if(d->pager && !d->pager->prompt) {
        ptr = text;
        while(loop < len) {
              for(count = 0, start = ptr + loop; (loop < len) && !((ptr[loop] == '\n') || (ptr[loop] == '\r')); loop++, count++);
              if((loop < len) && (ptr[loop] == '\n')) {
                 loop++, count++;
                 if((loop < len) && (ptr[loop] == '\r')) loop++, count++;
	      } else if((loop < len) && (ptr[loop] == '\r')) {
                 loop++, count++;
                 if((loop < len) && (ptr[loop] == '\n')) loop++, count++;
	      }

              if(count > 0) {
                 MALLOC(new,struct line_data);
                 NMALLOC(new->text,unsigned char,count);
                 memcpy(new->text,start,count);
                 new->next = NULL;
                 new->len  = count;

                 if(!d->pager->head) {
                    d->pager->current = d->pager->head = d->pager->tail = new;
                    d->pager->size   += count;
                    new->prev         = NULL;
                    d->pager->lines++;
		 } else {
                    d->pager->tail->next = new;
                    new->prev            = d->pager->tail;
                    d->pager->tail       = new;
                    d->pager->size      += count;
                    d->pager->lines++;
		 }

                 /* ---->  If paged text is too large, remove leading lines until within limits  <---- */
                 if(d->pager->size > OUTPUT_MAX) {
                    while(((d->pager->size + sizeof(OUTPUT_FLUSHED)) > OUTPUT_MAX) && d->pager->head) {
                          d->pager->current    = d->pager->head;
                          d->pager->head       = d->pager->head->next;
                          d->pager->size      -= d->pager->current->len;
                          d->pager->head->prev = NULL;
                          FREENULL(d->pager->current->text);
                          FREENULL(d->pager->current);
                          d->pager->current = d->pager->head;
                          d->pager->lines--;
		    }

                    MALLOC(new,struct line_data);
                    NMALLOC(new->text,unsigned char,sizeof(OUTPUT_FLUSHED));
                    memcpy(new->text,OUTPUT_FLUSHED,sizeof(OUTPUT_FLUSHED));
                    new->next            = d->pager->head;
                    new->prev            = NULL;
                    new->len             = sizeof(OUTPUT_FLUSHED);
                    d->pager->head->prev = new;
                    d->pager->head       = new;
                    d->pager->current    = d->pager->head;
                    d->pager->size      += sizeof(OUTPUT_FLUSHED);
                    d->pager->lines++;
		 }
	      }
	}
     }
}

/* ---->  Free 'more' pager from memory  <---- */
void pager_free(struct descriptor_data *d)
{
     struct line_data *current,*next;

     if(d->pager) {
        for(current = d->pager->head; current; current = next) {
            next = current->next;
            FREENULL(current->text);
            FREENULL(current);
	}
        FREENULL(d->pager->prompt);
        FREENULL(d->pager);
     }
}

/* ---->  Construct 'more' pager prompt  <---- */
void pager_prompt(struct descriptor_data *d)
{
     if(d->pager) {
        FREENULL(d->pager->prompt);
        if((d->terminal_width < 59) || (d->clevel == 7) || (d->clevel == 23) || (d->clevel == 24)) strcpy(scratch_return_string,ANSI_DCYAN"["ANSI_LCYAN"Press "ANSI_LWHITE"RETURN"ANSI_LCYAN"/"ANSI_LWHITE"ENTER"ANSI_LCYAN" to continue..."ANSI_DCYAN"]");
           else if(d->terminal_width >= 79) sprintf(scratch_return_string,ANSI_DCYAN"["ANSI_LCYAN""ANSI_UNDERLINE"MORE"ANSI_DCYAN":  "ANSI_LYELLOW"(Page %d of %d)  "ANSI_DCYAN"-  "ANSI_LCYAN"("ANSI_LWHITE""ANSI_UNDERLINE"N"ANSI_LCYAN")ext/("ANSI_LWHITE""ANSI_UNDERLINE"P"ANSI_LCYAN")revious page, ("ANSI_LWHITE""ANSI_UNDERLINE"R"ANSI_LCYAN")e-display or ("ANSI_LWHITE""ANSI_UNDERLINE"S"ANSI_LCYAN")kip..."ANSI_DCYAN"] ",((d->pager->line - 1) / (db[d->player].data->player.scrheight - 2)) + 1,((d->pager->lines - 2) / (db[d->player].data->player.scrheight - 2)) + 1);
              else sprintf(scratch_return_string,ANSI_DCYAN"["ANSI_LCYAN""ANSI_UNDERLINE"MORE"ANSI_DCYAN":  "ANSI_LYELLOW"(Page %d of %d)  "ANSI_DCYAN"-  "ANSI_LCYAN"Press "ANSI_LWHITE"RETURN"ANSI_LCYAN" to continue..."ANSI_DCYAN"] ",((d->pager->line - 1) / (db[d->player].data->player.scrheight - 2)) + 1,((d->pager->lines - 2) / (db[d->player].data->player.scrheight - 2)) + 1);
        d->pager->prompt = (char *) alloc_string(scratch_return_string);
     }
}

/* ---->  Display current page of output  <---- */
void pager_display(struct descriptor_data *d)
{
     if(d->pager && d->pager->head && d->pager->head->text) {
        short  loop = db[d->player].data->player.scrheight - 1;
        struct line_data *ptr;

        pager_prompt(d);
        if(d->pager->lines >= db[d->player].data->player.scrheight) {
           for(ptr = d->pager->current; (loop > 0) && ptr; ptr = ptr->next, loop--)
               server_queue_output(d,ptr->text,ptr->len);
	} else {
           for(ptr = d->pager->head; ptr; ptr = ptr->next)
               server_queue_output(d,ptr->text,ptr->len);
           pager_free(d);
	}
     } else if(d->pager) pager_free(d);
}

/* ---->  Process 'more' pager data (Allow user to browse it, page by page.)  <---- */
void pager_process(struct descriptor_data *d,const char *command)
{
     const char *start;
     short loop;
     char  *ptr;

     if(!command) {
        pager_display(d);
        return;
     }
     for(; *command && (*command == ' '); command++);
     for(ptr = scratch_return_string, start = command; *command && (*command != ' '); *ptr++ = *command++);
     for(; *command && (*command == ' '); command++);
     *ptr = '\0';

     if(string_prefix("nextpage",scratch_return_string) || (string_prefix("forwards",scratch_return_string) && (strlen(scratch_return_string) > 1))) {
        if(d->pager->current) {
           for(loop = 2; (loop < db[d->player].data->player.scrheight) && d->pager->current; loop++)
               d->pager->current = d->pager->current->next, d->pager->line++;
           if((d->pager->lines - d->pager->line) < (db[d->player].data->player.scrheight - 1)) {
              if((d->pager->lines - d->pager->line) < (db[d->player].data->player.scrheight - 2))
                 if(d->pager->current) d->pager->current = d->pager->current->next;
              pager_display(d);
              pager_free(d);
	   } else if(!d->pager->current) pager_free(d);
              else pager_display(d);
	} else pager_free(d);
     } else if(string_prefix("prevpage",scratch_return_string) || string_prefix("backwards",scratch_return_string) || string_prefix("previouspage",scratch_return_string)) {
        if(d->pager->current) {
           for(loop = 2; (loop < db[d->player].data->player.scrheight) && d->pager->current && d->pager->current->prev; loop++)
               d->pager->current = d->pager->current->prev, d->pager->line--;
           pager_display(d);
	}
     } else if(string_prefix("redisplay",scratch_return_string) || string_prefix("again",scratch_return_string)) {
        pager_display(d);
     } else if(string_prefix("help",scratch_return_string) || string_prefix("manual",scratch_return_string)) {

        /* ---->  Call On-Line Help System  <---- */
        if(Validchar(d->player)) help_main(d->player,(char *) command,NULL,NULL,NULL,0,0);
           else output(d,d->player,0,1,0,ANSI_LGREEN"\nSorry, you may only use the On-Line Help System when you are connected to %s.\n",tcz_full_name);
     } else if(string_prefix("first",scratch_return_string) || string_prefix("top",scratch_return_string) || string_prefix("home",scratch_return_string)) {
        d->pager->current = d->pager->head, d->pager->line = 1;
        pager_display(d);
     } else if(string_prefix("last",scratch_return_string) || string_prefix("bottom",scratch_return_string) || string_prefix("end",scratch_return_string)) {
        struct line_data *last = NULL;
        short  lineno = 1;

        d->pager->current = d->pager->head, d->pager->line = 1;
        while(d->pager->current) {
              last = d->pager->current, lineno = d->pager->line;
              for(loop = 2; (loop < db[d->player].data->player.scrheight) && d->pager->current; loop++)
                  d->pager->current = d->pager->current->next, d->pager->line++;
	}

        if(last) {
           d->pager->current = last;
           d->pager->line    = lineno;
	} else d->pager->current = d->pager->head, d->pager->line = 1;
        pager_display(d);
     } else if(string_prefix("skip",scratch_return_string) || string_prefix("quit",scratch_return_string) || string_prefix("abort",scratch_return_string)) {
        pager_free(d);
     } else if(*scratch_return_string == PROMPT_CMD_TOKEN) {

        /* ---->  Execute TCZ command  <---- */
        if(Validchar(d->player)) {
           const    char *ptr = (!strcasecmp("x",scratch_return_string + 1) || string_prefix("execute",scratch_return_string + 1)) ? command:(start + 1);
           unsigned char abort = 0;

           abort |= event_trigger_fuses(d->player,d->player,ptr,FUSE_ARGS);
           abort |= event_trigger_fuses(d->player,Location(d->player),ptr,FUSE_ARGS);
           if(!abort) process_basic_command(d->player,(char *) ptr,0);
	} else output(d,d->player,0,1,0,ANSI_LGREEN"\nSorry, you may only execute %s commands when you are connected to %s.\n",tcz_short_name,tcz_full_name);
     } else output(d,d->player,0,1,0,ANSI_LGREEN"\nTo see the next page, please press "ANSI_LYELLOW"RETURN"ANSI_LGREEN"/"ANSI_LYELLOW"ENTER"ANSI_LGREEN" or type "ANSI_LWHITE""ANSI_UNDERLINE"N"ANSI_LGREEN".\n\nYou can also re-display the current page by typing "ANSI_LWHITE""ANSI_UNDERLINE"R"ANSI_LGREEN", go back to the previous page by typing "ANSI_LWHITE""ANSI_UNDERLINE"P"ANSI_LGREEN", skip the rest of the pages and exit the 'more' paging facility by typing "ANSI_LWHITE""ANSI_UNDERLINE"S"ANSI_LGREEN", go to the first page by typing "ANSI_LWHITE""ANSI_UNDERLINE"F"ANSI_LGREEN", go to the last page by typing "ANSI_LWHITE""ANSI_UNDERLINE"L"ANSI_LGREEN" or execute a %s command by typing '"ANSI_LWHITE""ANSI_UNDERLINE"."ANSI_LWHITE"<COMMAND>"ANSI_LGREEN"'.\n",tcz_short_name);
}

/* ---->  Turn automatic 'more' paging facility on/off, or execute given <COMMAND> with forced 'more' paging  <---- */
/*        (val1:  0 = 'more' command, 1 = 'set more' command.)  */
void pager_more(CONTEXT)
{
     struct descriptor_data *p = getdsc(player);

     setreturn(ERROR,COMMAND_FAIL);
     if(!(val1 && !Validchar(player))) {
        if(!(command_type & QUERY_SUBSTITUTION) || Blank(params)) {
           for(; *params && (*params == ' '); params++);
           if(!Blank(params) && (!strcasecmp("on",params) || !strcasecmp("yes",params))) {
              db[player].flags2 |= MORE_PAGER;
              if(!in_command) output(p,player,0,1,0,ANSI_LGREEN"Automatic 'more' paging facility turned "ANSI_LWHITE"on"ANSI_LGREEN" (You may also force the output of any command to be paged by typing '"ANSI_LYELLOW"more <COMMAND>"ANSI_LGREEN"'.)");
	   } else if(!Blank(params) && (!strcasecmp("off",params) || !strcasecmp("no",params) || !strcasecmp("cancel",params))) {
              db[player].flags2 &= ~MORE_PAGER;
              if(!in_command) output(p,player,0,1,0,ANSI_LGREEN"Automatic 'more' paging facility turned "ANSI_LWHITE"off"ANSI_LGREEN" (However, you may force the output of any command to be paged by typing '"ANSI_LYELLOW"more <COMMAND>"ANSI_LGREEN"'.)");
	   } else if(!val1) {
              if(p) {
	         if(in_command || (command_type & QUERY_SUBSTITUTION) || !Blank(params)) {
                    if(!(in_command || (command_type & QUERY_SUBSTITUTION)) || (!IsHtml(p) && More(player))) pager_init(p);
                    if(!Blank(params)) process_basic_command(player,params,0);
                       else setreturn((command_type & QUERY_SUBSTITUTION) ? "":OK,COMMAND_SUCC);
		 } else output(p,player,0,1,0,ANSI_LGREEN"Please specify which %s command you would like to execute and page the output of.",tcz_short_name);
                 return;
	      }
	   } else if(!Blank(params)) {
              output(p,player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LWHITE"on"ANSI_LGREEN"' or '"ANSI_LWHITE"off"ANSI_LGREEN"'.");
              return;
	   } else if(!in_command) output(p,player,0,1,0,ANSI_LGREEN"%sore' paging facility is "ANSI_LWHITE"%s"ANSI_LGREEN".",(val1) ? "'M":"The 'm",More(player) ? "on":"off");
           setreturn(OK,COMMAND_SUCC);
	} else output(p,player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"more"ANSI_LGREEN"' cannot be used with parameters from within a query substitution.");
     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you must be connected to set your 'more' paging preference.");
}
