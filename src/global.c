/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| GLOBAL.C  -  Implements tertiary tree in #4 (Global Commands) for           |
|              efficient lookup of global compound commands.                  |
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
| Module originally designed and written by:  J.P.Boggis 14/05/1996.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: global.c,v 1.1.1.1 2004/12/02 17:41:24 jpboggis Exp $

*/


#include <stdlib.h>
#include <string.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "object_types.h"
#include "flagset.h"


struct  global_data *global = NULL,*global_tail = NULL;


/* ---->  Return effective location of compound command  <---- */
dbref effective_location(dbref command)
{
      if(Valid(command)) {
         command = Location(command);
         while(Valid(command) && (Typeof(command) == TYPE_COMMAND))
               command = Location(command);
         return(command);
      } else return(NOTHING);
}

/* ---->  Add compound command to global binary tree  <---- */
void global_add(dbref command)
{
     struct global_data *current,*last,*new;
     struct cmd_table *cmd = NULL;
     const  char *cmdtable = NULL;
     char   buffer[TEXT_SIZE];
     const  char *ptr;
     dbref  globalcmd;
     char   centre;
     int    value;
     char   *tmp;

     if(Valid(command) && Valid(Owner(command)) && (Typeof(command) == TYPE_COMMAND)) {
        if(Transferable(command)) db[command].flags &= ~TRANSFERABLE;

        /* ---->  Mortal owned global compound command not set READONLY and VALIDATED  <---- */
        if(!Level4(Owner(command))) {
           if(!Readonly(command) || !Validated(command)) {
              writelog(ADMIN_LOG,1,"WARNING","Global compound command %s(#%d) is Mortal owned (By %s(#%d)) and is not set %s (This compound command will be excluded from global lookup until %s set.)",getname(command),command,getname(Owner(command)),Owner(command),(!Readonly(command) && !Validated(command)) ? "READONLY and VALIDATED":!Readonly(command) ? "READONLY":"VALIDATED",(!Readonly(command) && !Validated(command)) ? "these flags are":"this flag is");
              if(!in_command && Validchar(current_character) && Connected(current_character) && Level4(current_character)) {
                 sprintf(buffer,"%s"ANSI_LYELLOW"%s"ANSI_LWHITE,Article(Owner(command),LOWER,INDEFINITE),getcname(current_character,Owner(command),1,0));
                 output(getdsc(current_character),current_character,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"Global compound command "ANSI_LYELLOW"%s"ANSI_LWHITE" is Mortal owned (By %s) and is not set "ANSI_LCYAN"%s"ANSI_LWHITE" (This compound command will be excluded from global lookup until %s set.)\n",unparse_object(current_character,command,0),(Owner(command) == current_character) ? "you":buffer,(!Readonly(command) && !Validated(command)) ? "READONLY"ANSI_LWHITE" and "ANSI_LCYAN"VALIDATED":!Readonly(command) ? "READONLY":"VALIDATED",(!Readonly(command) && !Validated(command)) ? "these flags are":"this flag is");
	      }
              return;
	   }
	}

        /* ---->  Global compound command name conflicts with built-in command and is not set WIZARD and VALIDATED  <---- */
        if(!Blank(db[command].name)) {

           /* ---->  Conflict with general built-in command?  <---- */
           cmdtable = "general";
           cmd = search_cmdtable(db[command].name,general_cmds,general_table_size);

           /* ---->  Conflict with BBS built-in command?  <---- */
           if(!cmd) {
              cmdtable = "BBS";
              cmd = search_cmdtable(db[command].name,bbs_cmds,bbs_table_size);
	   }

           /* ---->  Conflict with bank built-in command?  <---- */
           if(!cmd) {
              cmdtable = "bank";
              cmd = search_cmdtable(db[command].name,bank_cmds,bank_table_size);
	   }

	   if(cmd && (!Wizard(command) || !Validated(command))) {
              writelog(ADMIN_LOG,1,"WARNING","Global compound command %s(#%d) owned by %s(#%d) conflicts with built-in %s command '%s' and is not set %s (This compound command will be excluded from global lookup until %s set.)",getname(command),command,getname(Owner(command)),Owner(command),StringDefault(cmdtable,"<UNKNOWN TYPE>"),StringDefault(cmd->name,"<UNKNOWN COMMAND>"),(!Wizard(command) && !Validated(command)) ? "WIZARD and VALIDATED":!Wizard(command) ? "WIZARD":"VALIDATED",(!Wizard(command) && !Validated(command)) ? "these flags are":"this flag is");
              if(!in_command && Validchar(current_character) && Connected(current_character) && Level4(current_character)) {
                 sprintf(buffer,"owned by %s"ANSI_LYELLOW"%s"ANSI_LWHITE,Article(Owner(command),LOWER,INDEFINITE),getcname(current_character,Owner(command),1,0));
                 output(getdsc(current_character),current_character,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"Global compound command "ANSI_LYELLOW"%s"ANSI_LWHITE" %s conflicts with built-in %s command '"ANSI_LGREEN"%s"ANSI_LWHITE"' and is not set "ANSI_LCYAN"%s"ANSI_LWHITE" (This compound command will be excluded from global lookup until %s set.)\n",unparse_object(current_character,command,0),(Owner(command) == current_character) ? "(Owned by you)":buffer,StringDefault(cmdtable,"<UNKNOWN TYPE>"),StringDefault(cmd->name,"<UNKNOWN COMMAND>"),(!Wizard(command) && !Validated(command)) ? "WIZARD"ANSI_LWHITE" and "ANSI_LCYAN"VALIDATED":!Wizard(command) ? "WIZARD":"VALIDATED",(!Wizard(command) && !Validated(command)) ? "these flags are":"this flag is");
	      }
              return;
	   }
	}

        ptr = db[command].name;
        while(*ptr) {
              for(; *ptr && (*ptr == ' '); ptr++);
              for(tmp = buffer; *ptr && (*ptr != LIST_SEPARATOR); *tmp++ = *ptr++);
              for(*tmp = '\0', tmp = buffer + strlen(buffer) - 1; (tmp >= buffer) && (*tmp == ' '); *tmp-- = '\0');

              if(!Blank(buffer)) {
                 current = global, last = NULL;
                 if(!current) {
                    MALLOC(new,struct global_data);
                    new->left = new->right = new->centre = new->last = new->next = new->prev = NULL;
                    new->command = command;
                    new->name    = (char *) alloc_string(buffer);
                    global       = global_tail = new;
		 } else {
                    while(current) {
                          if(!(value = strcasecmp(current->name,buffer))) {
                             last    = current;
                             current = current->centre;
                             centre  = 0;
			  } else if(value < 0) {
                             last    = current;
                             current = current->right;
                             centre  = 1;
			  } else {
                             last    = current;
                             current = current->left;
                             centre  = -1;
			  }
		    }

                    /* ---->  Add to binary tree  <---- */
                    MALLOC(new,struct global_data);
                    new->command = command;
                    new->left    = new->right = new->centre = new->next = NULL;
                    new->name    = (char *) alloc_string(buffer);
                    new->last    = last;

                    if(last) {
                       switch(centre) {
			      case -1:
				   last->left = new;
				   break;
			      case 0:
				   last->centre = new;
				   break;
			      case 1:
				   last->right = new;
				   break;
		       }
		    }

                    /* ---->  Add to linked list  <---- */
                    new->prev         = global_tail;
                    global_tail->next = new;
                    global_tail       = new;
		 }
	      }
              for(; *ptr && (*ptr == LIST_SEPARATOR); ptr++);
	}

        /* ---->  Add compound commands within module to binary tree  <---- */
        for(globalcmd = db[command].commands; Valid(globalcmd); globalcmd = Next(globalcmd))
            global_add(globalcmd);
     }
}

/* ---->  Re-add entry to global binary tree  <---- */
void global_readd(struct global_data *ptr)
{
     struct global_data *current,*last;
     char   centre;
     int    value;

     current = global, last = NULL;
     while(current) {
           if(!(value = strcasecmp(current->name,ptr->name))) {
              last    = current;
              current = current->centre;
              centre  = 0;
	   } else if(value < 0) {
              last    = current;
              current = current->right;
              centre  = 1;
	   } else {
              last    = current;
              current = current->left;
              centre  = -1;
	   }
     }

     if(last) {
        switch(centre) {
	       case -1:
		    last->left = ptr;
		    break;
	       case 0:
		    last->centre = ptr;
		    break;
	       case 1:
		    last->right = ptr;
		    break;
	}
     }
     ptr->last = last;
}

/* ---->  Remove compound command from global binary tree  <---- */
void global_delete(dbref command)
{
     struct global_data *current,*next,*last = NULL;

     if(Valid(command) && (Typeof(command) == TYPE_COMMAND))
        for(current = global; current; current = next) {
            next = current->next;
            if((current->command == command) || in_area(current->command,command)) {

               /* ---->  Remove from binary tree  <---- */
               if(current->last) {
                  if(current->last->left == current) {
                     current->last->left = NULL;
		  } else if(current->last->right == current) {
                     current->last->right = NULL;
		  } else if(current->last->centre == current) {
                     current->last->centre = NULL;
		  }
	       }
               if(current == global) global = NULL;

               /* ---->  Remove from linked list  <---- */
               if(current->next) current->next->prev = last;
                  else global_tail = last;
               if(current->prev) current->prev->next = current->next;
                  else global = current->next;

               /* ---->  Add branches of removed node to binary tree  <---- */
               if(current->centre) global_readd(current->centre);
               if(current->right)  global_readd(current->right);
               if(current->left)   global_readd(current->left);

               /* ---->  Delete entry  <---- */
               FREENULL(current->name);
               FREENULL(current);
	    } else last = current;
	}
}

/* ---->  Initialise global compound command binary tree (Add all compound commands which are located in #4, or compound command modules within #4)  <---- */
int global_initialise(dbref location,unsigned char init)
{
    int   count = 0;
    dbref current;

    if(init && global) {
       struct global_data *next;

       for(; global; global = next) {
	   next = global->next;
	   FREENULL(global->name);
	   FREENULL(global);
       }

       global      = NULL;
       global_tail = NULL;
    }

    if(Valid(location)) {
       for(current = db[location].commands; Valid(current); current = Next(current)) {
           global_add(current);
           count++;

	   /* ---->  Recurse into 'modulised' compound commands  <---- */
	   if(Valid(db[current].commands))
	      count += global_initialise(current,0);
       }
    }
    return(count);
}

/* ---->  Lookup compound command in global binary tree  <---- */
dbref global_lookup(const char *name,int occurence)
{
     struct global_data *current = global;
     int    value;

     if(!Blank(name)) {
        while(current) {
              if(!(value = strcasecmp(current->name,name))) {
                 while(current && ((Valid(current->command) && !Level4(Owner(current->command)) && !Validated(current->command)) || --occurence)) current = current->centre;
                 if(current) return(current->command);
	      } else if(value < 0) {
                 current = current->right;
	      } else current = current->left;
	}
     }
     return(NOTHING);
}

