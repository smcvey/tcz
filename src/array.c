/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| ARRAY.C  -  Implements Dynamic Arrays and functions for manipulating them.  |
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
| Module originally designed and written by:  J.P.Boggis 16/05/1995.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                        https://github.com/smcvey/tcz                        |
`-----------------------------------------------------------------------------'

*/


#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "structures.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "object_types.h"
#include "flagset.h"
#include "search.h"
#include "match.h"
#include "quota.h"


static struct   array_element *rootnode = NULL,*tail = NULL;
static struct   array_sort_data *arrayptr,*arraynext;
static unsigned char sortdir;


/* ---->  Count number of elements in ARRAY  <---- */
int array_element_count(struct array_element *array)
{
    int count = 0;

    for(; array; array = array->next) count++;
    return(count);
}

/* ---->  Unparse given element range  <---- */
const char *array_unparse_element_range(int from,int to,const char *ansi)
{
      static char buffer[512];

      /* ---->  FROM element  <---- */
      switch(from) {
             case INDEXED:
                  sprintf(buffer,"'"ANSI_LWHITE"%s%s'",(BlankContent(indexfrom)) ? "<BLANK INDEX>":indexfrom,ansi);
                  break;
             case INVALID:
                  sprintf(buffer,ANSI_LWHITE"<INVALID>%s",ansi);
                  break;
             case UNSET:
                  sprintf(buffer,ANSI_LWHITE"<UNSET>%s",ansi);
                  return(buffer);
             case FIRST:
                  sprintf(buffer,ANSI_LWHITE"FIRST%s",ansi);
                  break;
             case LAST:
                  sprintf(buffer,ANSI_LWHITE"LAST%s",ansi);
                  break;
             case END:
                  sprintf(buffer,ANSI_LWHITE"END%s",ansi);
                  break;
             case ALL:
                  sprintf(buffer,ANSI_LWHITE"ALL%s",ansi);
                  return(buffer);
                  break;
             default:
                  sprintf(buffer,ANSI_LWHITE"%d%s",from,ansi);
      }

      /* ---->  TO element  <---- */
      if((from != UNSET) && (to != UNSET) && ((from != to) || ((to == INDEXED) && !BlankContent(indexto)))) switch(to) {
         case DEFAULT:
              sprintf(buffer + strlen(buffer),".."ANSI_LWHITE"DEFAULT%s",ansi);
              break;
         case INDEXED:
              sprintf(buffer + strlen(buffer),"..%s'"ANSI_LWHITE"%s%s'",ansi,(BlankContent(indexto)) ? "<BLANK INDEX>":indexto,ansi);
              break;
         case INVALID:
              sprintf(buffer + strlen(buffer),".."ANSI_LWHITE"<INVALID>%s",ansi);
              break;
         case FIRST:
              sprintf(buffer + strlen(buffer),".."ANSI_LWHITE"FIRST%s",ansi);
              break;
         case LAST:
              sprintf(buffer + strlen(buffer),".."ANSI_LWHITE"LAST%s",ansi);
              break;
         case END:
              sprintf(buffer + strlen(buffer),".."ANSI_LWHITE"END%s",ansi);
              break;
         default:
              sprintf(buffer + strlen(buffer),".."ANSI_LWHITE"%d%s",to,ansi);
      } else strcat(buffer,ansi);
      return(buffer);
}

/* ---->  (Re)set given elements of the specified dynamic array (Add any new elements if neccessary (Max. of ARRAY_LIMIT elements))  <---- */
int array_set_elements(dbref player,dbref array,int from,int to,const char *text,unsigned char insert,int *count,int *newcount)
{
    struct   array_element *ptr,*new,*newlist,*tail,*last = NULL;
    unsigned char finished = 0,index_found = 0;
    int      position = 1;

    *count    = 0;
    *newcount = 0;
    if(from == NOTHING) return(1);

    /* ---->  Find start of range  <---- */
    if(!(command_type & NO_USAGE_UPDATE) && !RoomZero(Location(array))) gettime(db[array].lastused);
    ptr = db[array].data->array.start;
    switch(from) {
           case ALL:
                break;
           case END:
                for(; ptr; last = ptr, ptr = ptr->next, position++);
                break;
           case LAST:
                for(; ptr && ptr->next; last = ptr, ptr = ptr->next, position++);
                break;
           case INDEXED:
                for(; ptr && !(ptr->index && !strcasecmp(ptr->index,indexfrom)); last = ptr, ptr = ptr->next, position++);
                if(ptr) index_found = 1;
                break;
           default:
                for(; ptr && (position < from); last = ptr, ptr = ptr->next, position++);
    }

    if(insert) {

       /* ---->  Insert new element  <---- */
       if(!adjustquota(player,db[array].owner,ELEMENT_QUOTA)) return(ARRAY_INSUFFICIENT_QUOTA);
       MALLOC(new,struct array_element);
       new->index = NULL;
       new->text  = (char *) alloc_string(compress(text,0));
       new->next  = ptr;

       if(last) {
          last->next = new;
       } else db[array].data->array.start = new;
       (*newcount)++;
    } else {

       /* ---->  Set elements  <---- */
       switch(to) {
              case END:
                   if(!adjustquota(player,db[array].owner,ELEMENT_QUOTA)) return(ARRAY_INSUFFICIENT_QUOTA);
              case ALL:
                   for(; ptr; last = ptr, ptr = ptr->next) {
                       FREENULL(ptr->text);
                       ptr->text = (char *) alloc_string(compress(text,0));
                       (*count)++;
		   }
                   if(from == END) {
                      MALLOC(new,struct array_element);
                      new->index = NULL;
                      new->text  = (char *) alloc_string(compress(text,0));
                      new->next  = NULL;

                      if(last) {
                         last->next = new;
		      } else db[array].data->array.start = new;
                      (*newcount)++;
                      return(1);
		   }
                   break;
              case LAST:
                   for(; ptr; last = ptr, ptr = ptr->next) {
                       FREENULL(ptr->text);
                       ptr->text = (char *) alloc_string(compress(text,0));
                       (*count)++;
		   }
                   break;
              case INDEXED:

                   /* ---->  Create new indexed element (From)  <---- */
                   if(!ptr && (from == INDEXED) && !index_found && !BlankContent(indexfrom)) {
                      if(!adjustquota(player,db[array].owner,ELEMENT_QUOTA)) return(ARRAY_INSUFFICIENT_QUOTA);

                      MALLOC(new,struct array_element);
                      new->index = (char *) alloc_string(indexfrom);
                      new->text  = (char *) alloc_string(compress(text,0));
                      new->next  = NULL;
                      (*newcount)++;

                      if(last) {
                         last = last->next = new;
		      } else last = db[array].data->array.start = new;
		   }

                   /* ---->  Set existing elements in range  <---- */
                   for(; ptr && !finished; last = ptr, ptr = ptr->next, position++) {
                       if(BlankContent(indexto) || (ptr->index && !strcasecmp(ptr->index,indexto))) finished = 1;
                       FREENULL(ptr->text);
                       ptr->text = (char *) alloc_string(compress(text,0));
                       (*count)++;
		   }

                   /* ---->  Create new indexed element (To)  <---- */
                   if(!ptr && !finished && !BlankContent(indexto)) {
                      if(!adjustquota(player,db[array].owner,ELEMENT_QUOTA)) return(ARRAY_INSUFFICIENT_QUOTA);

                      MALLOC(new,struct array_element);
                      new->index = (char *) alloc_string(indexto);
                      new->text  = (char *) alloc_string(compress(text,0));
                      new->next  = NULL;
                      (*newcount)++;

                      if(last) {
                         last = last->next = new;
		      } else last = db[array].data->array.start = new;
		   }
                   break;
              default:

                   /* ---->  Create linked list of blank elements  <---- */
                   if((from > 0) && (position < from) && !ptr) {
                      if((from - position + 1) > ARRAY_LIMIT) return(ARRAY_TOO_MANY_BLANKS);
                      if(!adjustquota(player,db[array].owner,(from - position) * ELEMENT_QUOTA)) return(ARRAY_INSUFFICIENT_QUOTA);

                      for(newlist = NULL; position < from; position++) {
                          MALLOC(new,struct array_element);
                          new->index = NULL;
                          new->text  = NULL;
                          new->next  = NULL;
                          if(newlist) {
                             tail->next = new;
                             tail       = new;
			  } else newlist = tail = new;
                          (*newcount)++;
		      }

                      if(last) {
                         last->next = newlist;
		      } else db[array].data->array.start = newlist;
                      last = tail;
 		   }

                   /* ---->  Set existing elements in range  <---- */
                   for(; ptr && (position <= to); last = ptr, ptr = ptr->next, position++) {
                       FREENULL(ptr->text);
                       ptr->text = (char *) alloc_string(compress(text,0));
                       (*count)++;
		   }

                   /* ---->  Create linked list of new elements  <---- */
                   if((to > 0) && (position <= to) && !ptr) {
                      if((to - position + 1) > ARRAY_LIMIT) return(ARRAY_TOO_MANY_ELEMENTS);
                      if(!adjustquota(player,db[array].owner,(to - position + 1) * ELEMENT_QUOTA)) return(ARRAY_INSUFFICIENT_QUOTA);

                      for(newlist = NULL; position <= to; position++) {
                          MALLOC(new,struct array_element);
                          new->index = NULL;
                          new->text  = (char *) alloc_string(compress(text,0));
                          new->next  = NULL;
                          if(newlist) {
                             tail->next = new;
                             tail       = new;
			  } else newlist = tail = new;
                          (*newcount)++;
		      }

                      if(last) {
                         last->next = newlist;
		      } else db[array].data->array.start = newlist;
                      last = tail;
		   }
       }
    }
    return(1);
}

/* ---->  (Re)set index name of given element of specified dynamic array  <---- */
int array_set_index(dbref player,dbref array,int element,const char *indexname)
{
    struct array_element *ptr;
    int    position = 1;

    /* ---->  Find start of range  <---- */
    if(element == NOTHING) return(ARRAY_INVALID_RANGE);
    if(!(command_type & NO_USAGE_UPDATE) && !RoomZero(Location(array))) gettime(db[array].lastused);
    ptr = db[array].data->array.start;
    switch(element) {
           case ALL:
           case END:

                /* ---->  Can't set index name of END element or ALL elements  <---- */
                return(ARRAY_INVALID_RANGE);
           case LAST:
                for(; ptr && ptr->next; ptr = ptr->next);
                break;
           case INDEXED:
                for(; ptr && !(ptr->index && !strcasecmp(ptr->index,indexfrom)); ptr = ptr->next);
                break;
           default:
                for(; ptr && (position < element); ptr = ptr->next, position++);
    }

    /* ---->  Set index name of element  <---- */
    if(ptr) {
       FREENULL(ptr->index);
       ptr->index = (char *) alloc_string(indexname);
       return(1);
    } else return(0);
}

/* ---->  Get next/previous element (Used by '.element' command in the editor)  <---- */
int array_nextprev_element(dbref player,dbref array,int *element,char *in_index,char **out_index,unsigned char next)
{
    struct array_element *ptr,*last = NULL;
    int    position = 1;

    /* ---->  Find element  <---- */
    if(*element == NOTHING) return(0);
    ptr = db[array].data->array.start;
    if((*element == FIRST) || (*element == ALL)) {
       in_index = NULL;
       *element = 1;
    }
    switch(*element) {
           case END:
                for(; ptr; last = ptr, ptr = ptr->next, position++);
                break;
           case LAST:
                for(; ptr && ptr->next; last = ptr, ptr = ptr->next, position++);
                break;
           case INDEXED:
                for(; ptr && !(ptr->index && !strcasecmp(ptr->index,indexfrom)); last = ptr, ptr = ptr->next, position++);
                break;
           default:
                for(; ptr && (position < *element); last = ptr, ptr = ptr->next, position++);
    }

    if(next) {
       if(ptr && ptr->next) {
          ptr = ptr->next;
          *element = position + 1;
       }
    } else if(last) {
       ptr = last;
       *element = position - 1;
    }
    if(ptr && ptr->index) {
       (*out_index) = ptr->index;
       *element     = INDEXED;
    } else (*out_index) = NULL;
    return(1);
}

/* ---->  Destroy given elements of the specified dynamic array  <---- */
int array_destroy_elements(dbref player,dbref array,int from,int to,int *count)
{
    struct   array_element *ptr,*temp,*last = NULL;
    unsigned char finished = 0;
    struct   grp_data *chk;
    int      position = 1;

    *count = 0;
    if(from == NOTHING) return(1);

    /* ---->  Find start of range  <---- */
    if(!(command_type & NO_USAGE_UPDATE) && !RoomZero(Location(array))) gettime(db[array].lastused);
    ptr = db[array].data->array.start;
    switch(from) {
           case ALL:
                for(; ptr; ptr = last) {
                   last = ptr->next;
                   adjustquota(player,db[array].owner,0 - ELEMENT_QUOTA);
                   for(chk = grp; chk; chk = chk->next)
                       if(ptr == &chk->nunion->element)
                          chk->nunion = NULL;
                   FREENULL(ptr->index);
                   FREENULL(ptr->text);
                   FREENULL(ptr);
                   (*count)++;
		}
                db[array].data->array.start = NULL;
                return(1);
           case END:

                /* ---->  Can't destroy END element, as this doesn't exist!  <---- */
                return(ARRAY_INVALID_RANGE);
           case LAST:
                for(; ptr && ptr->next; last = ptr, ptr = ptr->next, position++);
                break;
           case INDEXED:
                for(; ptr && !(ptr->index && !strcasecmp(ptr->index,indexfrom)); last = ptr, ptr = ptr->next, position++);
                break;
           default:
                for(; ptr && (position < from); last = ptr, ptr = ptr->next, position++);
    }

    /* ---->  Destroy elements  <---- */
    switch(to) {
           case LAST:
           case END:
                for(temp = last; ptr; ptr = last) {
                    last = ptr->next;
                    adjustquota(player,db[array].owner,0 - ELEMENT_QUOTA);
                    for(chk = grp; chk; chk = chk->next)
                        if(ptr == &chk->nunion->element)
                           chk->nunion = NULL;
                    FREENULL(ptr->index);
                    FREENULL(ptr->text);
                    FREENULL(ptr);
                    (*count)++;
		}
                if(temp) temp->next = NULL;
                   else db[array].data->array.start = NULL;
                break;
           case INDEXED:

                /* ---->  Destroy existing elements in range  <---- */
                for(temp = last; ptr && !finished; ptr = last, position++) {
                    last = ptr->next;
                    if(BlankContent(indexto) || (ptr->index && !strcasecmp(ptr->index,indexto))) finished = 1;
                    adjustquota(player,db[array].owner,0 - ELEMENT_QUOTA);
                    for(chk = grp; chk; chk = chk->next)
                        if(ptr == &chk->nunion->element)
                           chk->nunion = NULL;
                    FREENULL(ptr->index);
                    FREENULL(ptr->text);
                    FREENULL(ptr);
                    (*count)++;
		}
                if(temp) {
                   if(ptr) temp->next = ptr;
                      else temp->next = NULL;
		} else if(ptr) db[array].data->array.start = ptr;
                   else db[array].data->array.start = NULL;
                break;
           default:

                /* ---->  Destroy existing elements in range  <---- */
                for(temp = last; ptr && (position <= to); ptr = last, position++) {
                    last = ptr->next;
                    adjustquota(player,db[array].owner,0 - ELEMENT_QUOTA);
                    for(chk = grp; chk; chk = chk->next)
                        if(ptr == &chk->nunion->element)
                           chk->nunion = NULL;
                    FREENULL(ptr->index);
                    FREENULL(ptr->text);
                    FREENULL(ptr);
                    (*count)++;
		}
                if(temp) {
                   if(ptr) temp->next = ptr;
                      else temp->next = NULL;
		} else if(ptr) db[array].data->array.start = ptr;
                   else db[array].data->array.start = NULL;
    }
    return(1);
}

/* ---->  Return contents of given element(s) of the specified dynamic array  <---- */
int array_subquery_elements(dbref player,dbref array,int from,int to,char *buffer)
{
    int      elements = 0,position = 1;
    struct   array_element *ptr;
    unsigned char finished = 0;
    struct   str_ops str_data;

    *buffer = '\0';
    if(from == NOTHING) return(1);
    str_data.length = 0;
    str_data.dest   = buffer;
    str_data.src    = NULL;

    /* ---->  Find start of range  <---- */
    if(!(command_type & NO_USAGE_UPDATE) && !RoomZero(Location(array))) gettime(db[array].lastused);
    ptr = db[array].data->array.start;
    switch(from) {
           case ALL:
                for(; ptr; ptr = ptr->next) {
                    if(elements > 0) strcat_limits(&str_data,"\n");
                    if(!Blank(ptr->text)) strcat_limits(&str_data,decompress(ptr->text));
                    elements++;
		}
                *(str_data.dest) = '\0';
                return(elements);
           case END:

                /* ---->  Can't return contents of END element, as this doesn't exist!  <---- */
                return(ARRAY_INVALID_RANGE);
           case LAST:
                for(; ptr && ptr->next; ptr = ptr->next, position++);
                break;
           case INDEXED:
                for(; ptr && !(ptr->index && !strcasecmp(ptr->index,indexfrom)); ptr = ptr->next, position++);
                break;
           default:
                for(; ptr && (position < from); ptr = ptr->next, position++);
    }

    /* ---->  Get contents of element(s)  <---- */
    switch(to) {
           case LAST:
           case END:
                for(; ptr; ptr = ptr->next) {
                    if(elements > 0) strcat_limits(&str_data,"\n");
                    if(!Blank(ptr->text)) strcat_limits(&str_data,decompress(ptr->text));
                    elements++;
		}
                *(str_data.dest) = '\0';
                break;
           case INDEXED:

                /* ---->  Get contents of existing elements in range  <---- */
                for(; ptr && !finished; ptr = ptr->next, position++) {
                    if(BlankContent(indexto) || (ptr->index && !strcasecmp(ptr->index,indexto))) finished = 1;
                    if(elements > 0) strcat_limits(&str_data,"\n");
                    if(!Blank(ptr->text)) strcat_limits(&str_data,decompress(ptr->text));
                    elements++;
		}
                *(str_data.dest) = '\0';
                break;
           default:

                /* ---->  Get contents of existing elements in range  <---- */
                for(; ptr && (position <= to); ptr = ptr->next, position++) {
                    if(elements > 0) strcat_limits(&str_data,"\n");
                    if(!Blank(ptr->text)) strcat_limits(&str_data,decompress(ptr->text));
                    elements++;
		}
                *(str_data.dest) = '\0';
    }
    return(elements);
}

/* ---->  Return index name(s) of given element(s) of the specified dynamic array  <---- */
int array_subquery_index(dbref player,dbref array,int from,int to,char *buffer)
{
    int      elements = 0,position = 1;
    struct   array_element *ptr;
    unsigned char finished = 0;
    struct   str_ops str_data;

    *buffer = '\0';
    if(from == NOTHING) return(1);
    str_data.length = 0;
    str_data.dest   = buffer;
    str_data.src    = NULL;

    /* ---->  Find start of range  <---- */
    if(!(command_type & NO_USAGE_UPDATE) && !RoomZero(Location(array))) gettime(db[array].lastused);
    ptr = db[array].data->array.start;
    switch(from) {
           case ALL:
                for(; ptr; ptr = ptr->next)
                    if(!Blank(ptr->index)) {
                       if(elements > 0) strcat_limits(&str_data,"\n");
                       strcat_limits(&str_data,ptr->index);
                       elements++;
		    }
                *(str_data.dest) = '\0';
                return(elements);
           case END:

                /* ---->  Can't return index name of END element, as this doesn't exist!  <---- */
                return(ARRAY_INVALID_RANGE);
           case LAST:
                for(; ptr && ptr->next; ptr = ptr->next, position++);
                break;
           case INDEXED:
                for(; ptr && !(ptr->index && !strcasecmp(ptr->index,indexfrom)); ptr = ptr->next, position++);
                break;
           default:
                for(; ptr && (position < from); ptr = ptr->next, position++);
    }

    /* ---->  Get index name(s) of element(s)  <---- */
    switch(to) {
           case LAST:
           case END:
                for(; ptr; ptr = ptr->next)
                    if(!Blank(ptr->index)) {
                       if(elements > 0) strcat_limits(&str_data,"\n");
                       strcat_limits(&str_data,ptr->index);
                       elements++;
		    }
                *(str_data.dest) = '\0';
                break;
           case INDEXED:

                /* ---->  Get index name(s) of existing elements in range  <---- */
                for(; ptr && !finished; ptr = ptr->next, position++)
                    if(!Blank(ptr->index)) {
                       if(BlankContent(indexto) || (ptr->index && !strcasecmp(ptr->index,indexto))) finished = 1;
                       if(elements > 0) strcat_limits(&str_data,"\n");
                       strcat_limits(&str_data,ptr->index);
                       elements++;
		    }
                *(str_data.dest) = '\0';
                break;
           default:

                /* ---->  Get index name(s) of existing elements in range  <---- */
                for(; ptr && (position <= to); ptr = ptr->next, position++)
                    if(!Blank(ptr->index)) {
                       if(elements > 0) strcat_limits(&str_data,"\n");
                       strcat_limits(&str_data,ptr->index);
                       elements++;
		    }
                *(str_data.dest) = '\0';
    }
    return(elements);
}

/* ---->  Return element number of element with given index name  <---- */
int array_subquery_indexno(dbref player,dbref array,int element)
{
    struct array_element *ptr = db[array].data->array.start;
    int    count = 0;

    switch(element) {
           case INDEXED:
                for(; ptr; ptr = ptr->next, count++)
                    if(ptr->index && !strcasecmp(ptr->index,indexfrom))
                       return(count + 1);
                return(NOTHING);
           case FIRST:
                return(1);
           case LAST:
           case END:
                for(; ptr; ptr = ptr->next, count++);
                return((element == END) ? (count + 1):count);
           default:
                return((element > 0) ? element:NOTHING);
    }
    return(NOTHING);
}

/* ---->  Summarise and display the elements of specified dynamic array  <---- */
int array_display_elements(dbref player,int from,int to,dbref array,int cr)
{
    int      distance,ptrelement,elements = 0,element = 1;
    struct   descriptor_data *p = getdsc(player);
    struct   array_element *ptr,*current;
    char     buffer[BUFFER_LEN];
    unsigned char finished = 0;
    char     *temp;

    /* ---->  Find start of range  <---- */
    if(!(current = db[array].data->array.start)) {
       output(p,player,0,1,0,"%s"ANSI_LGREEN"This dynamic array has no elements.%s",(cr == 1) ? "\n":"",(cr == 2) ? "":"\n");
       return(1);
    }
    if(from == INVALID) return(ARRAY_INVALID_RANGE);
    if(Typeof(array) != TYPE_ARRAY) return(0);
    switch(from) {
           case NOTHING:
           case UNSET:
           case ALL:
                break;
           case END:

                /* ---->  Can't display contents of END element, as this doesn't exist!  <---- */
                return(ARRAY_INVALID_RANGE);
           case LAST:
                for(; current && current->next; current = current->next, element++);
                break;
           case INDEXED:
                for(; current && !(current->index && !strcasecmp(current->index,indexfrom)); current = current->next, element++);
                break;
           default:
                for(; current && (element < from); current = current->next, element++);
    }

    /* ---->  Display summary of array's elements  <---- */
    if(IsHtml(p)) {
       html_anti_reverse(p,1);
       output(p,player,1,2,0,"<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">");
    }

    if((from == ALL) || (to == END) || (to == NOTHING) || (from == NOTHING)) to = LAST;
    while(((to == LAST) && current) || ((to == INDEXED) && current && !finished) || (((to != LAST) && (to != INDEXED)) && current && (element <= to))) {

          /* ---->  Display element(s)  <---- */
          ptrelement = element, ptr = current;
          if((to == INDEXED) && (BlankContent(indexto) || (current->index && !strcasecmp(current->index,indexto)))) finished = 1, elements++;
          strcpy(buffer,Blank(temp = decompress(ptr->text)) ? "":temp);
          for(distance = 0; (((to == LAST) && current) || ((to == INDEXED) && current && !finished) || (((to != LAST) && (to != INDEXED)) && current && (element <= to))) && !strcasecmp(buffer,Blank(temp = decompress(current->text)) ? "":temp) && ((!ptr->index && !current->index) || (ptr->index && current->index && !strcasecmp(ptr->index,current->index))); current = current->next, element++, elements++, distance++)
              if((to == INDEXED) && (BlankContent(indexto) || (current->index && !strcasecmp(current->index,indexto)))) finished = 1;
          if(cr == 1) output(p,player,2,1,0,IsHtml(p) ? "\016<BR>\016":"\n"), cr = 0;

          if(distance > 1) output(p,player,2,1,2,"%s"ANSI_DCYAN"["ANSI_LCYAN"%d"ANSI_DCYAN".."ANSI_LCYAN"%d"ANSI_DCYAN"]%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=LEFT><TH WIDTH=20% ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TT>\016":"",ptrelement,element - 1,IsHtml(p) ? "\016</TT></TH><TD>\016":"  ",(IsHtml(p) && BlankContent(buffer)) ? "\016&nbsp;\016":buffer,IsHtml(p) ? "\016</TD></TR>\016":"\n");
	     else if(ptr && !Blank(ptr->index)) output(p,player,2,1,2,"%s"ANSI_DCYAN"["ANSI_LCYAN"%s"ANSI_DCYAN"]%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=LEFT><TH WIDTH=20% ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN">\016":"",ptr->index,IsHtml(p) ? "\016</TH><TD>\016":"  ",!Blank(decompress(ptr->text)) ? decompress(ptr->text):IsHtml(p) ? "\016&nbsp;\016":"",IsHtml(p) ? "\016</TD></TR>\016":"\n");
                else output(p,player,2,1,2,"%s"ANSI_DCYAN"["ANSI_LCYAN"%d"ANSI_DCYAN"]%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=LEFT><TH WIDTH=20% ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TT>\016":"",ptrelement,IsHtml(p) ? "\016</TT></TH><TD>\016":"  ",(IsHtml(p) && BlankContent(buffer)) ? "\016&nbsp;\016":buffer,IsHtml(p) ? "\016</TD></TR>\016":"\n");
    }

    if(IsHtml(p)) {
       output(p,player,1,2,0,"</TABLE>");
       html_anti_reverse(p,0);
    }
    if((elements > 0) && (cr != 2)) output(p,player,2,1,0,IsHtml(p) ? "\016<BR>\016":"\n");
    return(elements);
}

/* ---->  Recursively traverse tertiary tree to sort array elements into order  <---- */
void array_traverse_elements(struct array_sort_data *current)
{
     if(!sortdir) {
        if(current->right)
           array_traverse_elements(current->right);
     } else {
        if(current->left)
           array_traverse_elements(current->left);
     }

     for(arrayptr = current, current = (!sortdir) ? current->left:current->right; arrayptr; arrayptr = arraynext) {
         arraynext = arrayptr->centre;
         if(rootnode) {
            tail->next = arrayptr->element;
            tail       = arrayptr->element;
            arrayptr->element->next = NULL;
	 } else {
            tail = rootnode = arrayptr->element;
            rootnode->next = NULL;
	 }
         FREENULL(arrayptr);
     }

     if(current)
        array_traverse_elements(current);
}

/* ---->  Secondary sort key  <---- */
unsigned char array_secondary_key(const char *ptr,const char **sortptr,int *ofs,int *len,unsigned char alpha,short offset,unsigned char elements,const char *separator)
{
	 unsigned short loop, pos = 0;

	 if(!Blank(ptr)) {
	    *sortptr = ptr, ptr = (elements) ? decompress(ptr):ptr;
	    if(!Blank(separator)) {
	       unsigned short itemlen;

	       loop = 0, *len = strlen(separator);
	       while(*ptr) {
		     for(loop++, itemlen = 0; *ptr && strncasecmp(ptr,separator,*len); ptr++, itemlen++);
    
		     /* ---->  Item found  <---- */
		     if(loop == offset) {
			*ofs = pos;
			*len = itemlen;
			return(1);
		     }

		     /* ---->  Skip <SEPARATOR>  <---- */
		     pos += itemlen;
		     if(*ptr && !strncasecmp(ptr,separator,*len)) ptr += *len, pos += *len;
	       }
	       *ofs = 0, *len = 0;
	       return(0);
	    } else if(alpha) {
	       for(; *ptr && ((pos + 1) < offset); ptr++, pos++);
	       *ofs = pos, *len = 0;
	       return(1);
	    } else {
	       for(; *ptr && !isdigit(*ptr); ptr++, pos++);
	       for(loop = 1; *ptr && (loop < offset); loop++) {
		   for(; *ptr && isdigit(*ptr); ptr++, pos++);
		   for(; *ptr && !isdigit(*ptr); ptr++, pos++);
	       }
	       *ofs = pos, *len = 0;
	       return(1);
	    }
	 }
	 *sortptr = NULL;
	 *ofs     = 0;
	 *len     = 0;
	 return(0);
}

/* ---->  Match on secondary sort key  <---- */
int array_secondary_match(const char *sortkey1,unsigned short sortofs1,unsigned short sortlen1,const char *sortkey2,unsigned short sortofs2,unsigned short sortlen2,unsigned char alpha,unsigned char elements,const char *separator)
{
    if(separator) {
       strncpy(scratch_buffer,(!sortkey1) ? "":((elements) ? decompress(sortkey1):sortkey1) + sortofs1,sortlen1);
       strncpy(scratch_return_string,(!sortkey2) ? "":((elements) ? decompress(sortkey2):sortkey2) + sortofs2,sortlen2);
       scratch_buffer[sortlen1]        = '\0';
       scratch_return_string[sortlen2] = '\0';
    } else {
       strcpy(scratch_buffer,(!sortkey1) ? "":((elements) ? decompress(sortkey1):sortkey1) + sortofs1);
       strcpy(scratch_return_string,(!sortkey2) ? "":((elements) ? decompress(sortkey2):sortkey2) + sortofs2);
    }
    if(!alpha) {
       int num1,num2;

       num1 = (isdigit(*scratch_buffer)) ? atol(scratch_buffer):0;
       num2 = (isdigit(*scratch_return_string)) ? atol(scratch_return_string):0;
       return(num2 - num1);
    } else return(strcasecmp(scratch_return_string,scratch_buffer));
}

/* ---->  Sort dynamic array elements into alphabetical or numerical order  <---- */
void array_sort(CONTEXT)
{
     struct   array_sort_data *start = NULL,*lowest = NULL,*highest = NULL,*current,*new,*last;
     unsigned char alpha,ascending,elements,found,right;
     const    char *ptr,*sortptr,*separator = NULL;
     int      sortofs,sortlen,offset = 1;
     struct   array_element *element;
     struct   arg_data arg;
     dbref    array;
     int      value;

     setreturn(ERROR,COMMAND_FAIL);
     array = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_ARRAY,MATCH_OPTION_DEFAULT);
     if(!Valid(array)) return;

     if(Typeof(array) == TYPE_ARRAY) {
        if(!Readonly(array)) {
           if(can_write_to(player,array,0)) {
              unparse_parameters(arg2,5,&arg,0);

              /* ---->  Sort <ORDER> (Alphabetical/numeric)  <---- */
              if((arg.count >= 1) && string_prefix("ALPHABETICAL",arg.text[0])) alpha = 1;
                 else if((arg.count >= 1) && (string_prefix("NUMERICAL",arg.text[0]) || string_prefix("NUMBER",arg.text[0]))) alpha = 0;
                    else {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the sort "ANSI_LYELLOW""ANSI_UNDERLINE"<ORDER>"ANSI_LGREEN" (Either '"ANSI_LWHITE"alpha"ANSI_LGREEN"' or '"ANSI_LWHITE"numerical"ANSI_LGREEN"'.)");
                       return;
		    }

              /* ---->  Sort <DIRECTION> (Ascending/descending)?  <---- */
              if((arg.count >= 2) && (string_prefix("ASCENDING",arg.text[1]) || string_prefix("FORWARDS",arg.text[1]))) ascending = 1;
                 else if((arg.count >= 2) && (string_prefix("DESCENDING",arg.text[1]) || string_prefix("REVERSE",arg.text[1]))) ascending = 0;
                    else {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the sort "ANSI_LYELLOW""ANSI_UNDERLINE"<DIRECTION>"ANSI_LGREEN" (Either '"ANSI_LWHITE"ascending"ANSI_LGREEN"' or '"ANSI_LWHITE"descending"ANSI_LGREEN"'.)");
                       return;
		    }

              /* ---->  Primary sort key (<KEY1>) (Index/elements)  <---- */
              if((arg.count >= 3) && string_prefix("ELEMENTS",arg.text[2])) elements = 1;
                 else if((arg.count >= 3) && (string_prefix("INDEXNAMES",arg.text[2]) || string_prefix("INDICES",arg.text[2]))) elements = 0;
                    else {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the primary sort key ("ANSI_LYELLOW""ANSI_UNDERLINE"<KEY1>"ANSI_LGREEN") (Either '"ANSI_LWHITE"index"ANSI_LGREEN"' or '"ANSI_LWHITE"elements"ANSI_LGREEN"'.)");
                       return;
		    }

              /* ---->  Optional secondary sort key (<NUMBER>|<ITEM NO> "<SEPARATOR>")  <---- */
              if((arg.count >= 4) && (arg.numb[3] > 0)) {
                 offset = arg.numb[3];
                 if((arg.count >= 5) && !Blank(arg.text[4])) separator = arg.text[4];
	      }

              /* ---->  Create tertiary tree of sorted elements  <---- */
              if(!(command_type & NO_USAGE_UPDATE) && !RoomZero(Location(array))) gettime(db[array].lastused);
              if((element = db[array].data->array.start)) {
                 for(; element; element = element->next) {
                     ptr     = (elements) ? element->text:element->index;
                     current = start, last = NULL, found = 0;

		     if(!start) {

                        /* ---->  Create root node of tertiary tree  <---- */
                        MALLOC(new,struct array_sort_data);
                        new->element = element;
                        new->centre  = new->right = new->left = NULL;
                        lowest       = highest = start = new;
                        array_secondary_key(ptr,&(new->sortptr),&(new->offset),&(new->len),alpha,offset,elements,separator);
		     } else {
                        array_secondary_key(ptr,&sortptr,&sortofs,&sortlen,alpha,offset,elements,separator);

		        if(lowest && ((value = array_secondary_match(lowest->sortptr,lowest->offset,lowest->len,sortptr,sortofs,sortlen,alpha,elements,separator)) <= 0)) {
                           MALLOC(new,struct array_sort_data);
                           new->sortptr = sortptr;
                           new->element = element;
                           new->offset  = sortofs;
                           new->right   = new->left = NULL;
                           new->len     = sortlen;

                           if(value < 0) {
                              lowest->left = new;
                              new->centre  = NULL;
                              lowest       = new;
			   } else {
                              new->centre    = lowest->centre;
                              lowest->centre = new;
			   }
			} else if(highest && ((value = array_secondary_match(highest->sortptr,highest->offset,highest->len,sortptr,sortofs,sortlen,alpha,elements,separator)) >= 0)) {
                           MALLOC(new,struct array_sort_data);
                           new->sortptr = sortptr;
                           new->element = element;
                           new->offset  = sortofs;
                           new->right   = new->left = NULL;
                           new->len     = sortlen;

                           if(value > 0) {
                              highest->right = new;
                              new->centre    = NULL;
                              highest        = new;
			   } else {
                              new->centre     = highest->centre;
                              highest->centre = new;
			   }
			} else {
                            while(current && !found) {
                                  if(!(value = array_secondary_match(current->sortptr,current->offset,current->len,sortptr,sortofs,sortlen,alpha,elements,separator))) {
                                     MALLOC(new,struct array_sort_data);
                                     new->sortptr    = sortptr;
                                     new->element    = element;
                                     new->centre     = current->centre;
                                     new->offset     = sortofs;
                                     new->right      = new->left = NULL;
                                     new->len        = sortlen;
                                     current->centre = new;
                                     found           = 1;
			          } else if(value > 0) {
                                     last    = current;
                                     current = current->right;
                                     right   = 1;
				  } else {
                                     last    = current;
                                     current = current->left;
                                     right   = 0;
				  }
			    }

                            if(!found) {
                               MALLOC(new,struct array_sort_data);
                               new->sortptr = sortptr;
                               new->element = element;
                               new->centre  = new->right = new->left = NULL;
                               new->offset  = sortofs;
                               new->len     = sortlen;
                               
			       if(last) {
                                  if(right) {
                                     last->right = new;
				  } else {
				     last->left = new;
				  }
			       }
			    }
			}
		     }
		 }

                 /* ---->  Traverse tertiary tree to construct sorted array  <---- */
                 rootnode = NULL, sortdir = ascending;
                 array_traverse_elements(start);
                 db[array].data->array.start = rootnode;
	      }
              if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s of dynamic array %s"ANSI_LWHITE"%s"ANSI_LGREEN" sorted into %s %s order.",(elements) ? "Elements":"Index names",Article(array,LOWER,DEFINITE),unparse_object(player,array,0),(ascending) ? "ascending":"descending",(alpha) ? "alphabetical":"numerical");
              setreturn(OK,COMMAND_SUCC);
	   } else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only sort the elements of a dynamic array you own or one that's owned by someone of a lower level than yourself.");
              else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only sort the elements of a dynamic array you own.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that dynamic array is Read-Only  -  You can't sort its elements.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only sort the elements of a dynamic array.");
}

/* ---->  Return contents of array element(s)  <---- */
void array_query_elementno(CONTEXT)
{
     dbref thing = query_find_object(player,params,SEARCH_ARRAY,0,1);
     if(!Valid(thing) || (Typeof(thing) != TYPE_ARRAY) || (elementfrom == NOTHING) || (elementfrom == INVALID)) return;
     if(array_subquery_elements(player,thing,elementfrom,elementto,querybuf) > 0)
        setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Return index name(s) of array element(s)  <---- */
void array_query_index(CONTEXT)
{
     dbref thing = query_find_object(player,params,SEARCH_ARRAY,0,1);
     if(!Valid(thing) || (Typeof(thing) != TYPE_ARRAY) || (elementfrom == NOTHING) || (elementfrom == INVALID)) return;
     if(array_subquery_index(player,thing,elementfrom,elementto,querybuf) > 0)
        setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Return element number of element with given index name  <---- */
void array_query_indexno(CONTEXT)
{
     dbref array = query_find_object(player,params,SEARCH_ARRAY,0,1);
     int   element;

     if(!Valid(array) || (Typeof(array) != TYPE_ARRAY)) return;
     if((element = array_subquery_indexno(player,array,elementfrom)) != NOTHING) {
        sprintf(querybuf,"%d",element);
        setreturn(querybuf,COMMAND_SUCC);
     }
}

/* ---->  Return number of elements in dynamic array  <---- */
void array_query_noelements(CONTEXT)
{
     dbref thing = query_find_object(player,arg1,SEARCH_ARRAY,0,0);
     if(!Valid(thing) || (Typeof(thing) != TYPE_ARRAY)) return;
     sprintf(querybuf,"%d",array_element_count(db[thing].data->array.start));
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Set index name of a dynamic array element  <---- */
/*        (VAL1:  Optional #ID of dynamic array.)          */
void array_index(CONTEXT)
{
     int   element,ascii = 0;
     dbref thing;
     char  *ptr;

     setreturn(ERROR,COMMAND_FAIL);
     if(val1 == NOTHING) {
        thing = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_ARRAY,MATCH_OPTION_DEFAULT);
        if(!Valid(thing)) return;
     } else thing = val1;

     if((element = elementfrom) == NOTHING) element = 1;
     if(Typeof(thing) == TYPE_ARRAY) {
        if(can_write_to(player,thing,0)) {
           if(!Readonly(thing)) {
	      if(element != INVALID) {

                 /* ---->  Check index name is valid  <---- */
                 for(; *arg2 && (*arg2 == ' '); arg2++);
                 if(strlen(arg2) > 128) arg2[128] = '\0';
                 for(ptr = arg2; *ptr; ptr++) {
                     if(!isdigit(*ptr)) ascii++;
                     switch(*ptr) {
                            case '[':
                            case ']':
                            case '-':
                            case '.':
                                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the index name of a dynamic array element mustn't contain the characters '"ANSI_LYELLOW"["ANSI_LGREEN"', '"ANSI_LYELLOW"]"ANSI_LGREEN"', '"ANSI_LYELLOW"-"ANSI_LGREEN"' or '"ANSI_LYELLOW"."ANSI_LGREEN"'.");
                                 return;
                            default:
			      break;
		     }
		 }
                 for(ptr--; (ptr > arg2) && (*ptr == ' '); *ptr = '\0', ptr--);

                 if(!(!Blank(arg2) && !ascii)) {
                    if(!Blank(arg2)) {
                       struct array_element *ptr = db[thing].data->array.start;

                       for(; ptr && (!ptr->index || strcasecmp(ptr->index,arg2)); ptr = ptr->next);
                       if(ptr) {
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, an element with the index name '"ANSI_LWHITE"%s"ANSI_LGREEN"' already exists in the dynamic array "ANSI_LWHITE"%s"ANSI_LGREEN".",arg2,unparse_object(player,thing,0));
                          return;
		       }
		    }

                    if(array_set_index(player,thing,element,arg2)) {
                       if(!in_command) {
                          sprintf(scratch_buffer,ANSI_LGREEN"Index name of element %s of dynamic array ",array_unparse_element_range(element,element,ANSI_LGREEN));
                          output(getdsc(player),player,0,1,0,"%s"ANSI_LWHITE"%s"ANSI_LGREEN" set to '"ANSI_LYELLOW"%s"ANSI_LGREEN"'.",scratch_buffer,unparse_object(player,thing,0),arg2);
		       }
                       setreturn(OK,COMMAND_SUCC);
		    } else if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that element is either invalid or doesn't exist.");
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the index name of a dynamic array element mustn't consist of just numbers.");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that element is invalid.");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't change the index name of one of its elements.",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0));
	} else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the index names of elements of a dynamic array you own or one that's owned by someone of a lower level than yourself.");
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the index names of elements of a dynamic array you own.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the index name of a dynamic array element.");
}

/* ---->  Insert new element into dynamic array  <---- */
void array_insert(CONTEXT)
{
     int   element,dummy1,dummy2;
     dbref thing;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_ARRAY,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if((element = elementfrom) == NOTHING) element = 1;
     if(Typeof(thing) == TYPE_ARRAY) {
        if(can_write_to(player,thing,0)) {
           if(!Readonly(thing)) {
	      if(elementfrom != INVALID) {
                 switch(array_set_elements(player,thing,element,element,arg2,1,&dummy1,&dummy2)) {
                        case ARRAY_INSUFFICIENT_QUOTA:
                             sprintf(scratch_return_string,"to insert a new element into "ANSI_LWHITE"%s"ANSI_LRED,unparse_object(player,thing,0));
                             warnquota(player,db[thing].owner,scratch_return_string);
                             break;
                        default:
                             if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"New element inserted into dynamic array "ANSI_LWHITE"%s"ANSI_LGREEN" in front of element %s.",unparse_object(player,thing,0),array_unparse_element_range(element,element,ANSI_LGREEN));
                             setreturn(OK,COMMAND_SUCC);
		 }
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that element (Or range of elements) is invalid.");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't insert a new element.",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0));
	} else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only insert new elements into a dynamic array you own or one that's owned by someone of a lower level than yourself.");
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only insert new elements into a dynamic array you own.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only insert new elements into a dynamic array.");
}

