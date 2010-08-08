/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| TEMP.C  -  Implements temporary variables ('@temp'.)                        |
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
| Module originally designed and written by:  J.P.Boggis 18/10/1994.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: temp.c,v 1.1.1.1 2004/12/02 17:43:03 jpboggis Exp $

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


/* ---->  Look up temporary variable by name, returning pointer to it if found  <---- */
struct temp_data *temp_lookup(const char *name)
{
       struct temp_data *current = tempptr;
       int    value;

       if(!Blank(name)) {
          while(current) {
                if(!(value = strcasecmp(current->name,name))) {
                   return(current);
		} else if(value < 0) {
                   current = current->right;
		} else current = current->left;
	  }
       }
       return(NULL);
}

/* ---->  Add/set description of temporary variable  <---- */
void temp_describe(CONTEXT)
{
     struct temp_data *current,*last,*new;
     char   direction;
     const  char *p1;
     int    value;

     setreturn(ERROR,COMMAND_FAIL);
     if(in_command) {
        if(*arg1 && !isdigit(*arg1)) {
           for(p1 = arg1; !Blank(p1); p1++)
               if(!(isalnum(*p1) || (*p1 == '_'))) {
                  output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the name of a temporary variable must only consist of alpha-numeric characters or underscores ('"ANSI_LYELLOW"0"ANSI_LGREEN"'-'"ANSI_LYELLOW"9"ANSI_LGREEN"', '"ANSI_LYELLOW"a"ANSI_LGREEN"'-'"ANSI_LYELLOW"z"ANSI_LGREEN"', '"ANSI_LYELLOW"A"ANSI_LGREEN"'-'"ANSI_LYELLOW"Z"ANSI_LGREEN"' and '"ANSI_LYELLOW"_"ANSI_LGREEN"'.)");
                  return;
	       }

           if(!Blank(arg1)) {
              if(strlen(arg1) <= 32) {
                 if((current = temp_lookup(arg1))) {

                    /* ---->  Change description of temporary variable  <---- */
                    FREENULL(current->desc);
                    current->desc = (char *) alloc_string(compress(arg2,0));
                    setreturn(arg2,COMMAND_SUCC);
	         } else {

                    /* ---->  Add new temporary variable  <---- */
                    for(current = tempptr, value = 0; current; current = (current->next == tempptr) ? NULL:current->next, value++);
                    if((!Level4(db[player].owner) && (value >= MAX_TEMP_MORTAL)) || (!Level2(db[player].owner) && (value >= MAX_TEMP_ADMIN))) {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you may only have a maximum of "ANSI_LWHITE"%d"ANSI_LGREEN" temporary variables defined at any one time.",(Level4(db[player].owner)) ? MAX_TEMP_ADMIN:MAX_TEMP_MORTAL);
                       return;
		    }

                    current = tempptr, last = NULL;
                    if(!current) {
                       MALLOC(new,struct temp_data);
                       new->left = new->right = new->last = NULL;
                       new->next = new->prev  = new;
                       new->name = (char *) alloc_string(arg1);
                       new->desc = (char *) alloc_string(compress(arg2,0));
                       tempptr   = new;
		    } else {
                       while(current) {
                             if(!(value = strcasecmp(current->name,arg1))) {
                                last      = NULL;
                                current   = NULL;
                                direction = 0;
			     } else if(value < 0) {
                                last      = current;
                                current   = current->right;
                                direction = 1;
			     } else {
                                last      = current;
                                current   = current->left;
                                direction = -1;
			     }
		       }

                       /* ---->  Add to tertiary tree  <---- */
                       if(direction) {
                          MALLOC(new,struct temp_data);
                          new->left = new->right = NULL;
                          new->name = (char *) alloc_string(arg1);
                          new->desc = (char *) alloc_string(compress(arg2,0));
                          new->last = last;

                          if(last) {
                             if(direction > 0) last->right = new;
			        else last->left = new;
			  }

                          /* ---->  Add to linked list  <---- */
                          new->next           = tempptr->next;
                          tempptr->next->prev = new;
                          new->prev           = tempptr;
                          tempptr->next       = new;
		       }
		    }
                    setreturn(arg2,COMMAND_SUCC);
		 }
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of a temporary variable's name is 32 characters.");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the name of the temporary variable.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the name of a temporary variable mustn't start with a number.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@temp"ANSI_LGREEN"' can only be used from within a compound command.");
}

/* ---->  Re-add entry to temporary variable tertiary tree  <---- */
void temp_readd(struct temp_data *ptr)
{
     struct temp_data *current,*last;
     char   direction;
     int    value;

     if(tempptr) {
        current = tempptr, last = NULL;
        while(current) {
              if(!(value = strcasecmp(current->name,ptr->name))) {
                 writelog(BUG_LOG,1,"BUG","(temp_readd() in temp.c)  Attempted to add node with temporary variable name which already exists in tertiary tree.");
                 return;
	      } else if(value < 0) {
                 last      = current;
                 current   = current->right;
                 direction = 1;
	      } else {
                 last      = current;
                 current   = current->left;
                 direction = -1;
	      }
	}

        if(last) {
           if(direction > 0) last->right = ptr;
	      else last->left = ptr;
	}
        ptr->last = last;
     } else {
        tempptr   = ptr;
        ptr->last = NULL;
     }
}

/* ---->  Remove temporary variable  <---- */
void temp_destroy(CONTEXT)
{
     struct temp_data *current;

     setreturn(ERROR,COMMAND_FAIL);
     if(in_command) {
        if(!Blank(params)) {
           if((current = temp_lookup(params))) {

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
              if(current == tempptr) tempptr = NULL;

              /* ---->  Add branches of removed node to tertiary tree  <---- */
              if(current->right) temp_readd(current->right);
              if(current->left)  temp_readd(current->left);

              /* ---->  Delete entry  <---- */
              FREENULL(current->name);
              FREENULL(current->desc);
              FREENULL(current);
              setreturn(OK,COMMAND_SUCC);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a temporary variable with the name '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",params);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the name of the temporary variable.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@desttemp"ANSI_LGREEN"' can only be used from within a compound command.");
}

/* ---->  Remove all temporary variables from TMP onwards  <---- */
void temp_clear(struct temp_data **tmp,struct temp_data *newtmp)
{
     struct temp_data *ptr,*next;

     if(tmp) {
        if(*tmp) {
           (*tmp)->prev->next = NULL;
           for(ptr = *tmp; ptr; ptr = next) {
               next = ptr->next;
               FREENULL(ptr->name);
               FREENULL(ptr->desc);
               FREENULL(ptr);
	   }
	}
        *tmp = newtmp;
     }
}
