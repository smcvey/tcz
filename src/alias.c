/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| ALIAS.C  -  Implements custom command aliases.                              |
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
| Module originally designed and written by:  J.P.Boggis 21/02/1995.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: alias.c,v 1.2 2005/01/25 18:51:30 tcz_monster Exp $

*/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "object_types.h"
#include "objectlists.h"
#include "flagset.h"
#include "fields.h"


/* ---->  Match command with character's aliases and substitute appropriate aliased command if matched  <---- */
void alias_substitute(dbref player,char *src,char *dest)
{
     struct alias_data *alias;
     struct str_ops str_data;
     char   *p1,*p2,c;
     const  char *ptr;

     /* ---->  Grab command from given string  <---- */
     p1 = src;
     if(*p1 && !isalnum(*p1) && (*p1 != COMMAND_TOKEN)) p1++;
        else for(; *p1 && (*p1 != ' '); p1++);
     c = *p1, *p1 = '\0';

     /* ---->  Match against global and character's aliases  <---- */
     alias = (struct alias_data *) alias_lookup(player,src,1,1);
     *p1   = c;

     /* ---->  Substitute alias  <---- */
     if(alias && alias->command) {
        str_data.dest   = dest;
        str_data.length = 0;

        ptr = (char *) decompress(alias->command);
        for(; *p1 && (*p1 == ' '); p1++);
        for(p2 = p1 + strlen(p1) - 1; (p2 >= p1) && (*p2 == ' '); *p2-- = '\0');
        while(*ptr && (str_data.length < MAX_LENGTH))
              if((*ptr == '!') && *(ptr + 1) && (*(ptr + 1) == '*')) {

                 /* ---->  Substitute alias parameter  <---- */
                 strcat_limits(&str_data,p1);
                 ptr += 2;
	      } else {

                 /* ---->  Substitute alias  <---- */
                 *(str_data.dest)++ = *ptr;
                 str_data.length++;
                 ptr++;
	      }
        *(str_data.dest) = '\0';
        for(p1 = (str_data.dest) - 1; (p1 >= dest) && (*p1 == ' '); *p1-- = '\0');
     } else strcpy(dest,src);
}

/* ---->  Look up temporary variable by name, returning pointer to it if found  <---- */
struct alias_data *alias_lookup(dbref player,const char *command,unsigned char inherit,unsigned char global)
{
       struct alias_data *current;
       dbref  object;
       int    value;

       /* ---->  Match character's aliases (Including inherited aliases)  <---- */
       inherited = 0;
       if(Blank(command)) return(NULL);
       for(object = player; Validchar(object); object = (inherit) ? NOTHING:db[object].parent) {
           if(object != player) inherited++;
           current = db[object].data->player.aliases;
           while(current)
                if(!(value = strcasecmp(current->alias,command))) return(current);
	           else if(value < 0) current = current->right;
                      else current = current->left;
       }

       /* ---->  Match global aliases  <---- */
       if(global && Validchar(aliases)) {
          current = db[aliases].data->player.aliases;
          while(current)
                if(!(value = strcasecmp(current->alias,command))) return(current);
	           else if(value < 0) current = current->right;
                      else current = current->left;
       }
       return(NULL);
}

/* ---->  Re-add entry/add new entry to character's aliases tertiary tree  <---- */
void alias_readd(dbref player,struct alias_data *ptr)
{
     struct alias_data *current,*last;
     char   direction;
     int    value;

     if(!ptr || !Validchar(player)) return;
     if(db[player].data->player.aliases) {
        current = db[player].data->player.aliases;
        last = NULL;
        
	while(current) {
              if(!(value = strcasecmp(current->alias,ptr->alias))) {
                 writelog(BUG_LOG,1,"BUG","(alias_readd() in set.c)  Attempted to add node with alias name which already exists in %s(#%d)'s binary tree.",getname(player),player);
                 return;
	      } else if(value < 0) {
                 last = current;
                 current = current->right;
                 direction = 1;
	      } else {
                 last = current,
                 current = current->left;
                 direction = -1;
	      }
	}

        if(last) {
           if(direction > 0) {
              last->right = ptr;
	   } else {
              last->left = ptr;
	   }
	}
        ptr->last = last;
     } else {
        db[player].data->player.aliases = ptr;
        ptr->last                       = NULL;
     }
}

/* ---->  Remove alias and references from tertiary tree  <---- */
unsigned char alias_remove(dbref player,struct alias_data *current)
{
	 struct alias_data *ptr;

	 if(current) {
   
	    /* ---->  Remove from linked list  <---- */
	    if(current->next) current->next->prev = current->prev;
	    if(current->prev) current->prev->next = current->next;

	    /* ---->  Remove from tertiary tree  <---- */
	    if(current->last) {
	       if(current->last->left == current) {
		  current->last->left = NULL;
	       } else if(current->last->right == current) {
		  current->last->right = NULL;
	       }
	    }

	    if(current == db[player].data->player.aliases)
	       db[player].data->player.aliases = NULL;

	    /* ---->  Add branches of removed node to tertiary tree  <---- */
	    if(current->right) alias_readd(player,current->right);
	    if(current->left)  alias_readd(player,current->left);

	    /* ---->  Delete entry  <---- */
	    for(ptr = db[player].data->player.aliases; ptr && (ptr->id != current->id); ptr = (ptr->next == db[player].data->player.aliases) ? NULL:ptr->next);
	    if(!ptr) FREENULL(current->command);
	    FREENULL(current->alias);
	    FREENULL(current);
	    return(1);
	} else return(0);
}

/* ---->  Return aliased command  <---- */
void alias_query(CONTEXT)
{
     dbref  character = player;
     struct alias_data *alias;

     if(!Blank(arg2)) {
        character = query_find_character(player,arg1,1);
        if(!Validchar(character)) return;
        params = arg2;
     }

     if(!Blank(params)) {
        if((alias = alias_lookup(character,params,1,Level4(db[player].owner)))) {
           setreturn((alias->command) ? decompress(alias->command):"",COMMAND_SUCC);
        } else setreturn(NOTHING_STRING,COMMAND_FAIL);
     } else setreturn(NOTHING_STRING,COMMAND_FAIL);
}

/* ---->  List character's aliases  <---- */
void alias_aliases(CONTEXT)
{
     unsigned char twidth = output_terminal_width(player),cached_scrheight;
     struct   descriptor_data *p = getdsc(player);
     struct   alias_data *ptr,*next = NULL;
     dbref    who,object;

     setreturn(ERROR,COMMAND_FAIL);
     params = (char *) parse_grouprange(player,params,FIRST,1);
     if(!Blank(params)) {
        if((who = lookup_character(player,params,1)) == NOTHING) {
           output(p,player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",params);
           return;
	}
     } else who = player;

     for(object = db[player].parent; Validchar(object) && (object != who); object = db[object].parent);
     if(!((object != who) && !can_read_from(player,who))) {
        if(p && !IsHtml(p) && !p->pager && Validchar(p->player) && More(p->player)) pager_init(p);
        if(IsHtml(p)) {
	   html_anti_reverse(p,1);
	   output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
	}

        if(!in_command) {
           if(player != who) {
              if(object == who) output(p,player,2,1,1,"%sYou inherit the following aliases from %s"ANSI_LWHITE"%s"ANSI_LCYAN"...%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER><FONT COLOR="HTML_LCYAN" SIZE=4><I>\016":"\n ",Article(who,UPPER,DEFINITE),getcname(NOTHING,who,0,0),IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
                 else output(p,player,2,1,1,"%s%s"ANSI_LWHITE"%s"ANSI_LCYAN" has the following aliases...%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER><FONT COLOR="HTML_LCYAN" SIZE=4><I>\016":"\n ",Article(who,UPPER,DEFINITE),getcname(NOTHING,who,0,0),IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
	   } else output(p,player,2,1,1,"%sYou have the following aliases...%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER><FONT COLOR="HTML_LCYAN" SIZE=4><I>\016":"\n ",IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
           if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
	}

        if(db[who].data->player.aliases) {
           next = db[who].data->player.aliases->prev->next;
           db[who].data->player.aliases->prev->next = NULL;
	}

        cached_scrheight = db[who].data->player.scrheight;
        db[who].data->player.scrheight = db[player].data->player.scrheight - (Validchar(db[who].parent) ? 9:6);
        set_conditions(who,0,0,0,NOTHING,NULL,512);
        union_initgrouprange((union group_data *) db[who].data->player.aliases);
        while(union_grouprange()) {
              *scratch_return_string = '\0';
              for(ptr = db[who].data->player.aliases; ptr; ptr = (ptr->next == db[who].data->player.aliases) ? NULL:ptr->next)
                  if(ptr->id == grp->cunion->alias.id)
                     sprintf(scratch_return_string + strlen(scratch_return_string),"%s%s",(*scratch_return_string) ? ANSI_LGREEN", "ANSI_LYELLOW:"",ptr->alias);
              output(p,player,2,1,3,"%s%s"ANSI_LGREEN" \016&nbsp;\016 ---> \016&nbsp;\016 "ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TD ALIGN=LEFT>\016"ANSI_LYELLOW:ANSI_LYELLOW" ",scratch_return_string,decompress(grp->cunion->alias.command),IsHtml(p) ? "\016</TD></TR>\016":"\n");
	}
        if(next) db[who].data->player.aliases->prev->next = next;

        if(grp->rangeitems == 0) output(p,player,2,1,0,IsHtml(p) ? "\016<TR ALIGN=CENTER><TD>"ANSI_LCYAN"<I>*** &nbsp; NO ALIASES FOUND &nbsp; ***</I></TD></TR>\016":" ***  NO ALIASES FOUND  ***\n");
        if(!in_command) {
           if(Validchar(db[who].parent)) {
              if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','-'));
              if(who != player) {
                 sprintf(scratch_buffer,"%s%s"ANSI_LYELLOW"%s"ANSI_LWHITE" inherits further aliases from ",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_DGREY"><TD ALIGN=CENTER>"ANSI_LWHITE"<I>\016":ANSI_LWHITE" ",Article(who,UPPER,DEFINITE),getcname(NOTHING,who,0,0));
                 output(p,player,2,1,1,"%s"ANSI_LYELLOW"%s"ANSI_LWHITE" (Type '"ANSI_LGREEN"aliases %s"ANSI_LWHITE"' for a list.)%s",scratch_buffer,Article(db[who].parent,LOWER,INDEFINITE),getcname(NOTHING,db[who].parent,0,0),getname(db[who].parent),IsHtml(p) ? "\016</TD></TR>\016":"\n");
	      } else output(p,player,2,1,1,"%sYou inherit further aliases from %s"ANSI_LYELLOW"%s"ANSI_LWHITE" (Type '"ANSI_LGREEN"aliases %s"ANSI_LWHITE"' for a list.)%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_DGREY"><TD ALIGN=CENTER>"ANSI_LWHITE"<I>\016":ANSI_LWHITE" ",Article(db[who].parent,LOWER,INDEFINITE),getcname(NOTHING,db[who].parent,0,0),getname(db[who].parent),IsHtml(p) ? "\016</TD></TR>\016":"\n");
	   }

           if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
           if(grp->rangeitems != 0) output(p,player,2,1,1,"%sAliases listed: \016&nbsp;\016 "ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><TD>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",listed_items(scratch_return_string,1),IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
	      else output(p,player,2,1,1,"%sAliases listed: \016&nbsp;\016 "ANSI_DWHITE"None.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><TD>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
	}

        db[who].data->player.scrheight = cached_scrheight;
        if(IsHtml(p)) {
           output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
	   html_anti_reverse(p,0);
	}
        setreturn(OK,COMMAND_SUCC);
     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only list your own aliases.");
}

/* ---->  Add/change an alias (Or remove it if NULL second param)  <---- */
void alias_alias(CONTEXT)
{
     unsigned char used = 0,uniquealias = 0,changed = 0,notified = 0;
     struct   descriptor_data *p = getdsc(player);
     struct   alias_data *alias = NULL,*ptr,*new;
     int      processed = 0,unique,total,id = 0;
     const    char *start,*command = NULL;

     setreturn(ERROR,COMMAND_FAIL);
     for(; *arg1 && (*arg1 == ' '); arg1++);
     if(!in_command || (val1 || Wizard(current_command))) {
        if(!in_command || (val1 || (player != aliases))) {
           if(!Readonly(player)) {
              if(!Blank(arg1)) {
                 if(!Blank(arg2)) {
                    command = alloc_string(compress(arg2,0));

                    /* ---->  Search for aliases which alias same command  <---- */
                    for(ptr = db[player].data->player.aliases; ptr && (!ptr->command || strcmp(ptr->command,command)); ptr = (ptr->next == db[player].data->player.aliases) ? NULL:ptr->next);
                    if(!ptr) {

                       /* ---->  Nothing found  -  Allocate unique ID  <---- */
                       while(!id) {
                             while(!id) id = (lrand48() % 0xFFFF);
                             for(ptr = db[player].data->player.aliases; ptr && (ptr->id != id); ptr = (ptr->next == db[player].data->player.aliases) ? NULL:ptr->next);
                             if(ptr) id = 0;
		       }
                       uniquealias = 1;
		    } else id = ptr->id;
	         }

                 while(*arg1) {
                       if(!((strlen(arg1) == 1) && (*arg1 == LIST_SEPARATOR))) {
                          for(start = arg1; *arg1 && !((*arg1 == ' ') || (*arg1 == LIST_SEPARATOR)); arg1++);
                          if(*arg1) for(*arg1++ = '\0'; *arg1 && ((*arg1 == ' ') || (*arg1 == LIST_SEPARATOR)); arg1++);
		       } else start = arg1++;

                       if(!Blank(start)) {
                          if(!Blank(arg2)) {

                             /* ---->  Add/change alias(es)  <---- */
                             if((alias = alias_lookup(player,start,0,0))) {
                                unsigned char dealloc = 0,notify = 0;
                                int      change_id = alias->id;

                                /* ---->  Alias exists  -  Change alias and all references  <---- */
                                for(ptr = db[player].data->player.aliases; ptr; ptr = (ptr->next == db[player].data->player.aliases) ? NULL:ptr->next)
                                    if(ptr->id == change_id) {
                                       if(!dealloc) {
                                          if(strcmp(ptr->command,command)) notify = 1;
                                          if(ptr->command != command) {
                                             FREENULL(ptr->command);
                                             dealloc = 1;
				          }
				       }
                                       ptr->command = (char *) command;
                                       ptr->id      = id;
                                       if(!in_command && notify) output(p,player,0,1,0,ANSI_LGREEN"Alias '"ANSI_LWHITE"%s"ANSI_LGREEN"' changed to the command '"ANSI_LYELLOW"%s"ANSI_LGREEN"'.",ptr->alias,arg2), notified = 1;
                                       processed++, used = 1, changed = 1;
		   	  	    }

                                /* ---->  Add new alias  <---- */
			     } else if(strlen(start) <= MAX_ALIAS_LENGTH) {
 
                                /* ---->  Count total and unique aliases  <---- */
                                total = 0, unique = 0;
                                for(ptr = db[player].data->player.aliases; ptr; ptr = (ptr->next == db[player].data->player.aliases) ? NULL:ptr->next) {
                                    for(new = db[player].data->player.aliases; new && !((new->id == ptr->id) || (new == ptr)); new = (new->next == db[player].data->player.aliases) ? NULL:new->next);
                                    if(new && (new == ptr)) unique++;
                                    total++;
		   	        }
                                if(!uniquealias) unique--;

                                if(total < MAX_ALIASES) {
                                   if(Level2(Owner(player)) || (Level4(Owner(player)) && (unique < MAX_ALIASES_ADMIN)) || (!Level4(Owner(player)) && (unique < MAX_ALIASES_MORTAL))) {
                                      processed++, used = 1;
                                      MALLOC(new,struct alias_data);
                                      new->command = (char *) command;
                                      new->alias   = (char *) alloc_string(start);
                                      new->right   = new->left = new->last = new->next = new->prev = NULL;
                                      new->id      = id;

                                      /* ---->  Add to linked list and tertiary tree  <---- */
                                      if(db[player].data->player.aliases) {
                                         alias_readd(player,new);
                                         for(ptr = db[player].data->player.aliases; ptr && (ptr->next != db[player].data->player.aliases); ptr = ptr->next);
                                         new->next = db[player].data->player.aliases;
                                         new->prev = ptr;
                                         ptr->next = new;
                                         db[player].data->player.aliases->prev = new;
		  	  	      } else {
                                         db[player].data->player.aliases = new;
                                         new->next = new->prev = new;
				      }
                                      if(!in_command) output(p,player,0,1,0,ANSI_LGREEN"Alias '"ANSI_LWHITE"%s"ANSI_LGREEN"' added for the command '"ANSI_LYELLOW"%s"ANSI_LGREEN"'.",start,arg2), notified = 1;
 				   } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, the maximum number of unique aliases for %s is %d (Please remove some unwanted aliases first.)",(Level4(Owner(player))) ? "Apprentice Wizards/Druids and Wizards/Druids":"Mortals",(Level4(Owner(player))) ? MAX_ALIASES_ADMIN:MAX_ALIASES_MORTAL), processed = 1;
			        } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, the maximum number of individual aliases is %d (Please remove some unwanted aliases first.)",MAX_ALIASES), processed = 1;
			     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of an alias name is %d characters (The alias name '"ANSI_LWHITE"%s"ANSI_LGREEN"' exceeds this limit.)",MAX_ALIAS_LENGTH,start), processed = 1;
		          } else {

                             /* ---->  Remove alias(es)  <---- */
                             if((alias = alias_lookup(player,start,0,0))) {
                                processed++;
                                if(!in_command) output(p,player,0,1,0,ANSI_LGREEN"Alias '"ANSI_LWHITE"%s"ANSI_LGREEN"' removed.",alias->alias);
                                alias_remove(player,alias);
			     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, the alias '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",start), processed = 1;
			  }
		       }
		 }

                 if(command && !used) FREENULL(command);
                 if(!processed) {
                    output(getdsc(player),player,0,1,0,(val1) ? ANSI_LGREEN"Please specify which alias (Or a list of aliases) you'd like to remove.":ANSI_LGREEN"Please specify which alias (Or a list of aliases) you'd like to add, change or remove.");
                    return;
	         } else if(!in_command && !notified && changed)
                    output(p,player,0,1,0,ANSI_LGREEN"No aliases added or changed.");
	         setreturn(OK,COMMAND_SUCC);
	      } else output(getdsc(player),player,0,1,0,(val1) ? ANSI_LGREEN"Please specify which alias (Or a list of aliases) you'd like to remove.":ANSI_LGREEN"Please specify which alias (Or a list of aliases) you'd like to add, change or remove.");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, your "ANSI_LYELLOW"READONLY"ANSI_LGREEN" flag is set.  You cannot add, change or remove aliases.");
	} else output(getdsc(player),player,0,1,0,(val1) ? ANSI_LGREEN"Sorry, global aliases can't be removed from within a compound command.":ANSI_LGREEN"Sorry, global aliases can't be added, changed or removed from within a compound command.");
     } else output(getdsc(player),player,0,1,0,(val1) ? ANSI_LGREEN"Sorry, aliases can't be removed from within a compound command.":ANSI_LGREEN"Sorry, aliases can't be added, changed or removed from within a compound command.");
}

/* ---->  Remove alias  <---- */
void alias_unalias(CONTEXT)
{
     alias_alias(player,NULL,NULL,params,"",1,0);
}
