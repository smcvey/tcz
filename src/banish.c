/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| BANISH.C:  Implements banishing of unsuitable character names, preventing   |
|            new characters from being created with the name, or existing     |
|            characters changing their name to it.                            |
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
| Module originally designed and written by:  J.P.Boggis 12/04/1997.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: banish.c,v 1.1.1.1 2004/12/02 17:40:28 jpboggis Exp $

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
#include "flagset.h"


static struct banish_data *rootnode = NULL;
static struct banish_data *tail     = NULL;


/* ---->  Add name to binary tree of banished names  <---- */
unsigned char banish_add(const char *name,struct banish_data *node)
{
	 struct   banish_data *new,*current = banish,*last = NULL;
	 char     namebuf[TEXT_SIZE];
	 unsigned char right;
	 int      value;

	 /* ---->  Filter name  <---- */
	 if(!node && Blank(name)) return(0);
	 filter_spaces(namebuf,name,1);

	 /* ---->  Add to binary tree  <---- */
	 if(!banish) {
	    if(!node) {
	       MALLOC(new,struct banish_data);
	       new->right = new->left = new->next = NULL;
	       new->name  = (char *) alloc_string(namebuf);
	       banish     = new;
	    } else banish = node;
	 } else {
	    if(node) name = node->name;
	    while(current)
		  if(!(value = strcasecmp(name,current->name))) return(0);
		     else if(value < 0) last = current, current = current->left, right = 0;
			else last = current, current = current->right, right = 1;

	    if(!node) {
	       MALLOC(new,struct banish_data);
	       new->right   = new->left = NULL;
	       new->name    = (char *) alloc_string(namebuf);
	       new->next    = banish->next;
	       banish->next = new;
	    } else new = node;

	    if(last) {
	       if(right) last->right = new;
		  else last->left = new;
	    }
	 }
	 return(1);
}

/* ---->  Lookup banished name  <---- */
struct banish_data *banish_lookup(const char *name,struct banish_data **last,unsigned char exact)
{
       struct banish_data *current = banish;
       int    value,length;

       if(last) *last = NULL;
       if(Blank(name)) return(NULL);
       length = strlen(name);
       if(current) {
          while(current) {
                if((!exact && strncasecmp(current->name,name,strlen(current->name))) || (exact && strcasecmp(current->name,name))) {
                   value = strcasecmp(current->name,name);
                   if(last) *last = current;
                   current = (value > 0) ? current->left:current->right;
		} else return(current);
	  }
       }
       return(NULL);
}

/* ---->  Remove name from binary tree of banished names  <---- */
unsigned char banish_remove(const char *name)
{
	 struct banish_data *current,*last,*ptr;

	 if(Blank(name)) return(0);
	 if(!(current = banish_lookup(name,&last,1))) return(0);

	 /* ---->  Remove node from binary tree and linked list  <---- */
	 if(last) {
	    if(last->right == current) last->right = NULL;
	    if(last->left  == current) last->left  = NULL;
	    for(ptr = banish; ptr && (ptr->next != current); ptr = ptr->next);
	    if(ptr) ptr->next = current->next;
	 } else banish = NULL;

	 /* ---->  Add branches of node back into binary tree  <--- */
	 if(current->right) banish_add(NULL,current->right);
	 if(current->left)  banish_add(NULL,current->left);

	 /* ---->  Delete node  <---- */
	 FREENULL(current->name);
	 FREENULL(current);
	 return(1);
}

/* ---->  Check for and list characters with names beginning with a given banished name  <---- */
void banish_check(dbref player,const char *name,unsigned char errmsg)
{
     struct list_data *start = NULL,*current,*new;
     int    length,count = 0;
     dbref  i;

     if(Blank(name)) return;
     length = strlen(name);
     for(i = 0; i < db_top; i++)
         if((Typeof(i) == TYPE_CHARACTER) && (!strncasecmp(db[i].name,name,length) || !strncasecmp(getcname(NOTHING,i,0,0),name,length))) {
            MALLOC(new,struct list_data);
            new->player = i;
            new->next   = NULL;
            if(start) current->next = new;
               else start = new;
            current     = new;
            count++;
	 }

     if(start) {
        pagetell_construct_list(NOTHING,NOTHING,(union group_data *) start,count,scratch_return_string,ANSI_LWHITE,ANSI_LGREEN,0,0,INDEFINITE);
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"\nThe character%s "ANSI_LWHITE"%s"ANSI_LGREEN" %s beginning with the banished name '"ANSI_LYELLOW"%s"ANSI_LGREEN"'.\n",Plural(count),scratch_return_string,(count == 1) ? "has a name":"have names",name);
        for(current = start; current; current = new) {
            new = current->next;
            FREENULL(current);
	}
     } else if(errmsg) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, no characters currently have names beginning with the banished name '"ANSI_LYELLOW"%s"ANSI_LGREEN"'.",name);
}

/* ---->  Recursively traverse binary tree to sort banished character names into alphabetical order  <---- */
void banish_traverse(struct banish_data *current)
{
     if(current->left)
        banish_traverse(current->left);

     if(rootnode) {
        current->sort = NULL;
        tail->sort    = current;
        tail          = current;
     } else {
        rootnode      = current;
        tail          = current;
        current->sort = NULL;
     }

     if(current->right)
        banish_traverse(current->right);
}

/* ---->  List currently banished character names  <---- */
void banish_list(dbref player,struct descriptor_data *d,const char *name)
{
     unsigned char cached_scrheight,twidth = output_terminal_width(player);
     int      width,length,counter = 0;
     char     *ptr;

     if((!strcasecmp("page",name) && (strlen(name) == 4)) || !strncasecmp(name,"page ",5))
        for(name += 4; *name && (*name == ' '); name++);
     name = (char *) parse_grouprange(player,name,FIRST,1);
     if(d && !d->pager && !IsHtml(d) && Validchar(player) && More(player)) pager_init(d);

     /* ---->  Sort banished names into alphabetical order  <---- */
     rootnode = NULL, tail = NULL;
     banish_traverse(banish);
     set_conditions(player,0,0,0,0,name,510);

     if(IsHtml(d)) {
        width = 4;
     } else if(d && (twidth > 1)) {
        width = (twidth - 1) / 23;
     } else width = 3;

     if(Validchar(player)) {
        cached_scrheight                  = db[player].data->player.scrheight;
        db[player].data->player.scrheight = ((db[player].data->player.scrheight - 9) * width) * 2;
     }
     union_initgrouprange((union group_data *) rootnode);

     if(IsHtml(d)) {
        html_anti_reverse(d,1);
        output(d,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
     }

     if(!in_command) {
        output(d,player,2,1,1,"%sCharacter names beginning with the following are currently banned...%s",IsHtml(d) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH COLSPAN=4><FONT COLOR="HTML_LCYAN" SIZE=4><I>\016":"\n ",IsHtml(d) ? "\016</I></FONT></TH></TR>\016":"\n");
        if(!IsHtml(d)) output(d,player,0,1,0,separator(twidth,0,'-','='));
     }

     strcpy(scratch_buffer,IsHtml(d) ? "\016<TR ALIGN=LEFT>\016":ANSI_LWHITE" ");
     while(union_grouprange()) {
           if(++counter > width) {
              strcat(scratch_buffer,IsHtml(d) ? "\016</TR>\016":"\n");
              output(d,player,2,1,0,"%s",scratch_buffer);
              strcpy(scratch_buffer,IsHtml(d) ? "\016<TR ALIGN=LEFT>\016":ANSI_LWHITE" ");
              counter = 1;
	   }

           if(!IsHtml(d)) {
              for(ptr = scratch_return_string, length = 20 - strlen(grp->cunion->banish.name); length > 0; *ptr++ = ' ', length--);
              *ptr = '\0';
	   }
           sprintf(scratch_buffer + strlen(scratch_buffer),"%s%s%s",IsHtml(d) ? "\016<TD WIDTH=25%>\016":(counter > 1) ? "  ":"",grp->cunion->banish.name,IsHtml(d) ? "":scratch_return_string);
     }
     if(Validchar(player)) db[player].data->player.scrheight = cached_scrheight;

     if(counter != 0) {
        if(IsHtml(d)) while(++counter <= width) strcat(scratch_buffer,"\016<TD WIDTH=25%>&nbsp;</TD>\016");
        strcat(scratch_buffer,IsHtml(d) ? "\016</TR>\016":"\n");
        output(d,player,2,1,0,"%s",scratch_buffer);
     }

     if(grp->rangeitems == 0)
        output(d,player,2,1,1,"%s",IsHtml(d) ? "\016<TR ALIGN=CENTER><TD COLSPAN=4>"ANSI_LCYAN"<I>*** &nbsp; NO BANISHED NAMES LISTED &nbsp; ***</I></TD></TR>\016":" ***  NO BANISHED NAMES LISTED  ***\n");

     if(!in_command) {
        if(!IsHtml(d)) output(d,player,2,1,0,separator(twidth,1,'-','='));
        output(d,player,2,1,1,"%sBanished names listed: %s "ANSI_DWHITE"%s%s",IsHtml(d) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><TD COLSPAN=4>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",IsHtml(d) ? "\016&nbsp;\016":"",listed_items(scratch_return_string,1),IsHtml(d) ? "\016</B></TD></TR>\016":"\n\n");
     }

     if(IsHtml(d)) {
        output(d,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
        html_anti_reverse(d,0);
     }
}

/* ---->  Banish use of character name  <---- */
void banish_main(CONTEXT)
{
     struct descriptor_data *p = getdsc(player);
     const  char *option;

     setreturn(ERROR,COMMAND_FAIL);
     for(; *arg1 && (*arg1 == ' '); arg1++);
     for(option = arg1; *arg1 && (*arg1 != ' '); arg1++);
     if(*arg1) for(*arg1++ = '\0'; *arg1 && (*arg1 == ' '); arg1++);

     if(!in_command) {
        if(Level4(player)) {
           if(!Blank(option)) {
              if(string_prefix("listnames",option) || string_prefix("viewnames",option) || string_prefix("searchnames",option)) {

                 /* ---->  List currently banished names  <---- */
                 if(banish) {
	            banish_list(player,p,arg1);
                    setreturn(OK,COMMAND_SUCC);
		 } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, no character names are currently banished.");
	      } else if(string_prefix("addname",option) || string_prefix("banishname",option) || string_prefix("banname",option)) {
                 struct banish_data *banned = NULL;
                 const  char *ptr;

                 /* ---->  Check name consists of valid characters  <---- */
                 for(ptr = arg1; *ptr && ValidCharName(*ptr); ptr++);

                 /* ---->  Add name to banished list  <---- */
                 if(dumpstatus != 6) {
                    if(!*ptr) {
                       if(!Blank(arg1)) {
                          if(strlen(arg1) <= 20) {
                             if((strlen(arg1) >= 3) || Level2(player)) {
                                if(!(banned = banish_lookup(arg1,NULL,0))) {
                                   if(!Blank(arg2)) {
                                      if(banish_add(arg1,NULL)) {
                                         writelog(BAN_LOG,1,"BANISH","%s(#%d) added the character name '%s' to the banished list  -  REASON:  %s",getname(player),player,arg1,punctuate(arg2,2,'.'));
                                         output(p,player,0,1,0,ANSI_LGREEN"Character names beginning with '"ANSI_LWHITE"%s"ANSI_LGREEN"' are now banished.",arg1);
                                         banish_check(player,arg1,0);
                                         setreturn(OK,COMMAND_SUCC);
				      } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, character names beginning with '"ANSI_LWHITE"%s"ANSI_LGREEN"' are already banished.",(banned) ? banned->name:arg1);
				   } else output(p,player,0,1,0,ANSI_LGREEN"Please specify the reason for banishing character names beginning with '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",arg1);
				} else output(p,player,0,1,0,ANSI_LGREEN"Sorry, character names beginning with '"ANSI_LWHITE"%s"ANSI_LGREEN"' are already banished.",(banned) ? banned->name:arg1);
			     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, the minimum length of a banished character name is 3 characters.");
			  } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of a banished character name is 20 characters.");
		       } else output(p,player,0,1,0,ANSI_LGREEN"Please specify which character name you'd like to add to the banished list.");
		    } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, the character name to banish may only consist of the letters '"ANSI_LWHITE"A"ANSI_LGREEN"'..'"ANSI_LWHITE"Z"ANSI_LGREEN"', the numbers '"ANSI_LWHITE"0"ANSI_LGREEN"'..'"ANSI_LWHITE"9"ANSI_LGREEN"', spaces (' ') and underscores ('"ANSI_LWHITE"_"ANSI_LGREEN"'.)");
		 } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, names cannot be added to the banished list while it's currently being dumped to disk.  Please try again in a few minutes time.");
	      } else if(string_prefix("removename",option) || string_prefix("deletename",option) || string_prefix("erasename",option) || string_prefix("unbanishname",option) || string_prefix("unbanname",option) || string_prefix("allowname",option) || string_prefix("allowedname",option)) {

                 /* ---->  Remove name from banished list  <---- */
                 if(dumpstatus != 6) {
                    if(!Blank(arg1)) {
                       if(!Blank(arg2)) {
                          if(banish_lookup(arg1,NULL,1) && banish_remove(arg1)) {
                             writelog(BAN_LOG,1,"BANISH","%s(#%d) removed the character name '%s' from the banished list.",getname(player),player,arg1);
                             output(p,player,0,1,0,ANSI_LGREEN"Character names beginning with '"ANSI_LWHITE"%s"ANSI_LGREEN"' are now allowed again.",arg1);
                             setreturn(OK,COMMAND_SUCC);
			  } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, character names beginning with '"ANSI_LWHITE"%s"ANSI_LGREEN"' are not currently banished.",arg1);
		       } else output(p,player,0,1,0,ANSI_LGREEN"Please specify the reason for allowing character names beginning with '"ANSI_LWHITE"%s"ANSI_LGREEN"' again.",arg1);
		    } else output(p,player,0,1,0,ANSI_LGREEN"Please specify which character name you'd like to remove from the banished list.");
		 } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, names cannot be removed from the banished list while it's currently being dumped to disk.  Please try again in a few minutes time.");
	      } else if(string_prefix("checkname",option) || string_prefix("lookupname",option)) {
                 if(!Blank(arg1)) {
                    banish_check(player,arg1,1);
                    setreturn(OK,COMMAND_SUCC);
		 } else output(p,player,0,1,0,ANSI_LGREEN"Please specify the banished name to check.");
	      } else output(p,player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LYELLOW"add"ANSI_LGREEN"', '"ANSI_LYELLOW"list"ANSI_LGREEN"' or '"ANSI_LYELLOW"remove"ANSI_LGREEN"'.");
	   } else output(p,player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LYELLOW"add"ANSI_LGREEN"', '"ANSI_LYELLOW"list"ANSI_LGREEN"' or '"ANSI_LYELLOW"remove"ANSI_LGREEN"'.");
	} else output(p,player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above can banish character names.");
     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, character names can't be banished from within a compound command.");
}
