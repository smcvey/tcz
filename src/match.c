/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| MATCH.C  -  Hierarchical object matching.  Used throughout TCZ for matching |
|             objects by #ID, name or reference.                              |
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
| Module originally designed and written by:  J.P.Boggis 12/12/1997.          |
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

#include "friend_flags.h"
#include "object_types.h"
#include "objectlists.h"
#include "flagset.h"
#include "search.h"
#include "match.h"


static dbref match_result(dbref result,unsigned char found);


/* ---->  Match stack  <---- */
static struct match_data *match_start;  /*  Start of match stack  */
static struct match_data *match;        /*  End of match stack    */


/* ---->  Dynamic array matching  <---- */
char indexfrom[129];  /*  From index name  */
char indexto[129];    /*  To index name    */
int  elementfrom;     /*  From element     */
int  elementto;       /*  To element       */

int name_to_type_size = 0;

u_char match_recursed;  /*  Set to 1 if MATCH_RECURSION_LIMIT is reached  */


/* ---->  Sort NAME -> TYPE conversion table into alphabetical order  <---- */
short sort_name_to_type(void)
{
      unsigned short             loop,top,highest;
      struct   name_to_type_data temp;

      for(loop = 0; name_to_type[loop].name; loop++)
          name_to_type[loop].len = strlen(name_to_type[loop].name);
      name_to_type_size = loop;

      for(top = name_to_type_size - 1; top > 0; top--) {

          /* ---->  Find highest entry in unsorted part of list  <---- */
          for(loop = 1, highest = 0; loop <= top; loop++)
              if(strcasecmp(name_to_type[loop].name,name_to_type[highest].name) > 0)
                 highest = loop;

          /* ---->  Swap highest entry in unsorted part of list with bottom entry of sorted part of list  <---- */
          if(highest < top) {
             temp.name = name_to_type[top].name, temp.type = name_to_type[top].type;
             name_to_type[top].name = name_to_type[highest].name, name_to_type[top].type = name_to_type[highest].type;
             name_to_type[highest].name = temp.name, name_to_type[highest].type = temp.type;
	  }
      }
      return(name_to_type_size);
}

/* ---->  Search for NAME in NAME -> TYPE conversion table  <---- */
int search_name_to_type(const char *name)
{
    int            top = name_to_type_size - 1,middle = 0,bottom = 0,nearest = NOTHING,result;
    unsigned short len,nlen = 0xFFFF;

    if(Blank(name)) return(NOTHING);
    len = strlen(name);
    while(bottom <= top) {
          middle = (top + bottom) / 2;
          if((result = strcasecmp(name_to_type[middle].name,name)) != 0) {
                if((name_to_type[middle].len < nlen) && (len <= name_to_type[middle].len) && !strncasecmp(name_to_type[middle].name,name,len))
                   nearest = middle, nlen = name_to_type[middle].len;

                if(result < 0) bottom = middle + 1;
                   else top = middle - 1;
	     } else return(name_to_type[middle].type);
       }

       /* ---->  Nearest matching  <---- */
       if(nearest != NOTHING) return(name_to_type[nearest].type);
       return(NOTHING);
}

/* ---->  Parse [FROM-TO] array elements  <---- */
void match_parse_array_elements(const char *range)
{
     /* ---->  Initialise dynamic array variables  <---- */
     elementfrom  = NOTHING;
     elementto    = NOTHING;
     *indexfrom   = '\0';
     *indexto     = '\0';

     /* ---->  Determine [FROM-TO] elements of dynamic array  <---- */
     if(!Blank(range)) {
        int temp,ascii;
        char *ptr;

        /* ---->  <FROM> element  <---- */
        for(; *range && (*range == '['); range++);
        for(; *range && (*range == ' '); range++);
        for(ptr = indexfrom, temp = 0, ascii = 0; *range && (temp < 128) && !((*range == '-') || (*range == '.') || (*range == ']')); *ptr++ = *range++, temp++)
            if(!isdigit(*range)) ascii++;
        *ptr = '\0';
        for(ptr--; (ptr > indexfrom) && (*ptr == ' '); *ptr = '\0', ptr--);

        if(!strcasecmp("FIRST",indexfrom)) elementfrom = 1;
           else if(!strcasecmp("LAST",indexfrom)) elementfrom = LAST;
              else if(!strcasecmp("END",indexfrom)) elementfrom = END;
                 else if(!strcasecmp("ALL",indexfrom)) {
                    elementfrom = ALL;
                    elementto   = ALL;
		 } else if(!ascii) {
                    elementfrom = (BlankContent(indexfrom)) ? NOTHING:atol(indexfrom);
                    if(elementfrom < 1) elementfrom = INVALID;
		 } else elementfrom = INDEXED;

        /* ---->  'To' element  <---- */
        for(; *range && (*range == ' '); range++);
        if(*range && (*range == '.')) range++;
        if(*range && ((*range == '-') || (*range == '.'))) {
           for(range++; *range && (*range == ' '); range++);
           for(ptr = indexto, temp = 0, ascii = 0; *range && (temp < 128) && !((*range == ']')); *ptr++ = *range++, temp++)
               if(!isdigit(*range)) ascii++;
           *ptr = '\0';
           for(ptr--; (ptr > indexto) && (*ptr == ' '); *ptr = '\0', ptr--);

           if(!strcasecmp("FIRST",indexto)) elementto = 1;
              else if(!strcasecmp("LAST",indexto)) elementto = LAST;
                 else if(!strcasecmp("END",indexto)) elementto = END;
                    else if(!strcasecmp("ALL",indexto)) {
                       elementfrom = ALL;
                       elementto   = ALL;
		    } else if(!ascii) {
                       elementto = (BlankContent(indexto)) ? NOTHING:atol(indexto);
                       if(elementto < 1) elementfrom = INVALID;
		    } else elementto = INDEXED;
	}

        /* ---->  Sanitise FROM-TO elements  <---- */
        switch(elementfrom) {
               case INVALID:
               case INDEXED:
                    break;
               case LAST:
                    switch(elementto) {
                           case INDEXED:
                           case INVALID:
                           case NOTHING:
                           case LAST:
                           case END:
                                break;
                           case ALL:
                                elementfrom = ALL;
                                break;
                           default:
                                temp        = elementfrom;
                                elementfrom = elementto;
                                elementto   = temp;
		    }
                    break;
               case END:
                    switch(elementto) {
                           case INDEXED:
                           case INVALID:
                           case NOTHING:
                           case END:
                                break;
                           case ALL:
                                elementfrom = ALL;
                                break;
                           case LAST:
                           default:
                                temp        = elementfrom;
                                elementfrom = elementto;
                                elementto   = temp;
		    }
                    break;
               case ALL:
                    elementto = ALL;
                    break;
               default:
                    switch(elementto) {
                           case INDEXED:
                           case NOTHING:
                           case LAST:
                           case END:
                                break;
                           case ALL:
                                elementfrom = ALL;
                                break;
                           default:
                                if(elementfrom > elementto) {
                                   temp        = elementfrom;
                                   elementfrom = elementto;
                                   elementto   = temp;
				}
		    }
	}
        if(elementto   == NOTHING) elementto  = elementfrom;
        if(elementfrom != INDEXED) *indexfrom = '\0';
        if(elementto   != INDEXED) *indexto   = '\0';
     }
}

/* ---->  Push new entry onto match stack  <---- */
void match_push()
{
     struct match_data *new;

     MALLOC(new,struct match_data);
     new->currentobj = NOTHING;
     new->current    = NOTHING;
     new->parent     = NOTHING;
     new->exclude    = NOTHING;
     new->recursed   = 0;

     new->start      = NOTHING;
     new->end        = NOTHING;
     new->owner      = NOTHING;
     new->phase      = MATCH_PHASE_KEYWORD;
     new->endphase   = MATCH_PHASE_GLOBAL;
     new->order      = NOTHING;
     new->types      = SEARCH_ALL;
     new->subtypes   = SEARCH_ALL;
     new->options    = MATCH_OPTION_DEFAULT;

     new->search     = NULL;
     new->index      = NULL;
     new->remainder  = NULL;
     new->indexchar  = '\0';
     new->next       = NULL;

     if(match) {
        match->next = new;
        new->prev   = match;
        match       = new;
     } else {
        match = match_start = new;
        new->prev = NULL;
     }
}

/* ---->  Pop entry off match stack  <---- */
void match_pop()
{
     struct match_data *prev;

     if(!match) match = match_start;
     if(match && match->next)
        for(; match && match->next; match = match->next);

     if(match) {
        prev = match->prev;
        if(!(match->options & MATCH_OPTION_PRESERVE)) {
           FREENULL(match->remainder);
           FREENULL(match->search);
        }
        FREENULL(match);

        if(prev) {
           prev->next = NULL;
           match      = prev;
        } else match = match_start = NULL;
     } else match = match_start = NULL;     
}

/* ---->  Match object name, using specified match type  <---- */
unsigned char match_name(const char *searchstr,dbref object,int type,unsigned char exact)
{
	 static const char *ptr,*ptr2,*name;
	 static char buffer[TEXT_SIZE];
	 static int result;
	 static char *tmp;

	 if(Blank(searchstr) || !Valid(object)) return(NOTHING);
	 if((Typeof(object) == TYPE_ARRAY) && (match->index))
	    *(match->index) = '\0';

	 result = 0;
	 if(Typeof(object) == TYPE_CHARACTER)
	    name = getcname(NOTHING,object,0,UPPER|INDEFINITE);
	       else name = getname(object);
	 if(Blank(name)) return(NOTHING);
	 if(exact) type = MATCH_TYPE_EXACT|(type & MATCH_TYPE_LIST);

	 if(type & MATCH_TYPE_LIST) {

	    /* ---->  Match within LIST_SEPARATOR (';') separated list  <---- */
	    for(; *name && (*name == ' '); name++);
	    while(*name && !result) {

		  /* ---->  Get current item  <---- */
		  for(tmp = buffer; *name && (*name != LIST_SEPARATOR); *tmp++ = *name++);
		  for(*tmp-- = '\0'; (tmp >= buffer) && (*tmp == ' '); *tmp-- = '\0');

		  /* ---->  Match current item  <---- */
		  switch(type & ~MATCH_TYPE_LIST) {
			 case MATCH_TYPE_EXACT:
			      result = !strcasecmp(HasArticle(object) ? skip_article(buffer,Articleof(object),1):buffer,HasArticle(object) ? skip_article(searchstr,Articleof(object),1):searchstr);
			      break;
			 case MATCH_TYPE_PARTIAL:
			      result = instring(HasArticle(object) ? skip_article(searchstr,Articleof(object),1):searchstr,buffer);
			      break;
			 case MATCH_TYPE_PREFIX:
			      result = string_prefix(HasArticle(object) ? skip_article(buffer,Articleof(object),1):buffer,HasArticle(object) ? skip_article(searchstr,Articleof(object),1):searchstr);
			      break;
			 case MATCH_TYPE_FIRST:
			      ptr  = HasArticle(object) ? skip_article(searchstr,Articleof(object),1):searchstr;
			      ptr2 = HasArticle(object) ? skip_article(buffer,Articleof(object),1):buffer;
			      for(; *ptr && *ptr2 && (tolower(*ptr) == tolower(*ptr2)) && (*ptr != ' '); ptr++, ptr2++);
			      result = (!*ptr || (*ptr == ' '));
			      break;
		  }

		  /* ---->  Move onto next item  <---- */
		  if(!result) {
		     for(; *name && (*name != LIST_SEPARATOR); name++);
		     for(; *name && (*name == LIST_SEPARATOR); name++);
		     for(; *name && (*name == ' '); name++);
		  }
	    }

	    /* ---->  Match SEARCHSTR with object name  <---- */
	 } else switch(type & ~MATCH_TYPE_LIST) {
	    case MATCH_TYPE_EXACT:
		 result = !strcasecmp(HasArticle(object) ? skip_article(name,Articleof(object),1):name,HasArticle(object) ? skip_article(searchstr,Articleof(object),1):searchstr);
		 break;
	    case MATCH_TYPE_PARTIAL:
		 result = instring(HasArticle(object) ? skip_article(searchstr,Articleof(object),1):searchstr,name);
		 break;
	    case MATCH_TYPE_PREFIX:
		 result = string_prefix(HasArticle(object) ? skip_article(name,Articleof(object),1):name,HasArticle(object) ? skip_article(searchstr,Articleof(object),1):searchstr);
		 break;
	    case MATCH_TYPE_FIRST:
		 ptr  = HasArticle(object) ? skip_article(searchstr,Articleof(object),1):searchstr;
		 ptr2 = HasArticle(object) ? skip_article(name,Articleof(object),1):name;
		 for(; *ptr && *ptr2 && (tolower(*ptr) == tolower(*ptr2)) && (*ptr != ' '); ptr++, ptr2++);
		 result = (!*ptr || (*ptr == ' '));
		 break;
	 }

	 if((Typeof(object) == TYPE_ARRAY) && (match->index))
	    *(match->index) = match->indexchar;
	 return(result);
}

/* ---->  Simple match for object of given name attached to given object  <---- */
/*  OBJECT   = Location to search.                                */
/*  NAME     = Name of object to match.                           */
/*  LIST     = List to search (CONTENTS, COMMANDS, EXITS, etc.)   */
/*  RECURSE  = Recurse into modules (Only appropriate for compound commands.)  */
/*  MULTIPLE = Match multiple object names in ';' separated list  */
dbref match_simple(dbref object,const char *name,int list,unsigned char recurse,unsigned char multiple)
{
      dbref                match,current_object;
      int                  cached_current;
      static unsigned char recursed;

      recursed       = recurse;
      current_object = object;

      object = getfirst(object,list,&current_object);
      if(Valid(object)) do {
         if((!multiple && strcasecmp((Typeof(object) == TYPE_CHARACTER) ? getcname(NOTHING,object,0,UPPER|INDEFINITE):db[object].name,((Typeof(object) == TYPE_ROOM) || (Typeof(object) == TYPE_EXIT) || (Typeof(object) == TYPE_THING) || (Typeof(object) == TYPE_CHARACTER)) ? skip_article(name,Articleof(object),1):name)) ||
            (multiple  && !string_matched_list(((Typeof(object) == TYPE_ROOM) || (Typeof(object) == TYPE_EXIT) || (Typeof(object) == TYPE_THING) || (Typeof(object) == TYPE_CHARACTER)) ? skip_article(name,Articleof(object),1):name,(Typeof(object) == TYPE_CHARACTER) ? getcname(NOTHING,object,0,UPPER|INDEFINITE):db[object].name,LIST_SEPARATOR,0))) {

               /* ---->  Recurse into "modulised" compound commands  <---- */
               if(recursed && (recursed <= MATCH_RECURSION_LIMIT) && (Typeof(object) == TYPE_COMMAND) && getfirst(object,COMMANDS,&current_object)) {
                  cached_current = current_object;
                  match = match_simple(object,name,COMMANDS,recursed + 1,multiple);
                  if(Valid(match)) {
                     parent_object = current_object;
                     return(match);
	          } else current_object = cached_current;
                  return(match);
	       }
	    } else {
               parent_object = current_object;
               return(object);
	    }

         getnext(object,list,current_object);
      } while(Valid(object));
      return(NOTHING);
}

/* ---->  Begin new match, pushing current results onto stack  <---- */
/*  OWNER      = Owner of match                                        */
/*  START      = Starting object                                       */
/*  END        = Ending object                              (NOTHING)  */
/*  SEARCHSTR  = Search string                                         */
/*  STARTPHASE = Starting phase                 (MATCH_PHASE_KEYWORD)  */
/*  ENDPHASE   = Ending phase                    (MATCH_PHASE_GLOBAL)  */
/*  TYPES      = Types of objects to match for           (SEARCH_ALL)  */
/*  OPTIONS    = Matching options              (MATCH_OPTION_DEFAULT)  */
/*  SUBTYPES   = Sub-type(s) of object to match for      (SEARCH_ALL)  */
/*  CALLER     = Pointer to MATCH_DATA of calling match        (NULL)  */
/*  LEVEL      = Current recursion level                          (0)  */
dbref match_object(dbref owner,dbref start,dbref end,const char *searchstr,unsigned char startphase,unsigned char endphase,int types,int options,int subtypes,struct match_data *caller,unsigned char level)
{
      static char buffer[TEXT_SIZE];
      static const char *ptr;
      static char *dest,*tmp;
      dbref  object;

      if(level == 0) match_recursed = 0;

      if(!(options & MATCH_OPTION_CONTINUE)) {
         match_push();

         /* ---->  No match types  <---- */
         if(!types || !subtypes || !Valid(start) || !Validchar(owner))
            return(match_result(NOTHING,1));

         /* ---->  Match recursion limit exceeded?  <---- */
         if(level > MATCH_RECURSION_LIMIT) {
            if((options & MATCH_OPTION_ATTACHED) && (options & MATCH_OPTION_NOTIFY)) {
               if(in_command && Valid(current_command))
                  output(getdsc(owner),owner,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"Maximum match recursion limit of "ANSI_LYELLOW"%d"ANSI_LWHITE" exceeded within compound command "ANSI_LYELLOW"%s"ANSI_LWHITE".",MATCH_RECURSION_LIMIT,unparse_object(owner,current_command,0));
                     else output(getdsc(owner),owner,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"Maximum match recursion limit of "ANSI_LYELLOW"%d"ANSI_LWHITE" exceeded.",MATCH_RECURSION_LIMIT);
	    }

            if(in_command && Valid(current_command))
               writelog(EXECUTION_LOG,1,"RECURSION","Maximum match recursion limit of %d exceeded within compound command %s(#%d) by %s(#%d).",MATCH_RECURSION_LIMIT,getname(current_command),current_command,getname(owner),owner);
                  else writelog(EXECUTION_LOG,1,"RECURSION","Maximum match recursion limit of %d exceeded by %s(#%d).",MATCH_RECURSION_LIMIT,getname(owner),owner);

	    match_recursed = 1;
            return(match_result(NOTHING,1));
	 }

         /* ---->  No search string given  <---- */
         if(Blank(searchstr)) {
            if(options & MATCH_OPTION_NOTIFY) {
               if(options & MATCH_OPTION_ATTACHED)
                  output(getdsc(owner),owner,0,1,2,ANSI_LGREEN"Please specify the name of the object you would like to search for within %s"ANSI_LYELLOW"%s"ANSI_LGREEN".",Article(start,LOWER,DEFINITE),unparse_object(owner,start,0));
                     else output(getdsc(owner),owner,0,1,2,ANSI_LGREEN"Please specify the name of a character or object.");
	    }
            return(match_result(NOTHING,1));
         }

         /* ---->  Find correct starting object (Based on phase)  <---- */
         if((startphase > MATCH_PHASE_SELF) && !(options & MATCH_OPTION_ABSOLUTE_LOCATION)) {
            int phase = MATCH_PHASE_SELF;

            while((phase < startphase) && Valid(start)) {
                  start = Location(start);
		  phase = phase + 1;
	    }
         }

         /* ---->  Initialisation  <---- */
         match->recursed = level;
         match->parent   = start;
         match->start    = start;
         match->end      = end;
         match->owner    = owner;
         if(startphase   > 0) match->phase    = startphase;
         if(endphase     > 0) match->endphase = endphase;
         match->types    = types;
         match->subtypes = subtypes;
         if(options      > 0) match->options  = options;
         if(!caller && (*searchstr == MATCH_EXACT) && *(searchstr + 1)) {
            match->options |= MATCH_OPTION_FORCE_EXACT;
            searchstr++;
	 }

         if(caller) {

            /* ---->  Sub-match within object list (No need to initialise search string)  <---- */
            match->remainder = caller->remainder;
            match->search    = caller->search;
            match->index     = caller->index;
            match->options  |= MATCH_OPTION_PRESERVE;
         } else {
            
            /* ---->  Get object search string (Before ':', with leading/trailing spaces stripped)  <---- */
            for(ptr = searchstr; *ptr && (*ptr == ' '); ptr++);
            for(dest = buffer; *ptr && (*ptr != ':'); *dest++ = *ptr++);
            for(*dest-- = '\0', tmp = dest; (tmp >= buffer) && (*tmp == ' '); *tmp-- = '\0');
            NMALLOC(match->search,unsigned char,strlen(buffer) + 1);
            strcpy(match->search,buffer);

            /* ---->  Set dynamic array index pointer  <---- */
            dest = match->search + strlen(match->search) - 1;
            while((dest >= match->search) && (*dest != '[')) dest--;
            if(dest) match->index = dest, match->indexchar = *dest;

            /* ---->  Get remainder of object search string (After ':', with leading/trailing spaces stripped)  <---- */
            if(*ptr) {
               for(ptr++; *ptr && (*ptr == ' '); ptr++);
               for(dest = buffer; *ptr; *dest++ = *ptr++);
               for(*dest-- = '\0', tmp = dest; (tmp >= buffer) && (*tmp == ' '); *tmp-- = '\0');
               NMALLOC(match->remainder,unsigned char,strlen(buffer) + 1);
               strcpy(match->remainder,buffer);
            }

            /* ---->  Blank search string?  <---- */
            if(Blank(match->search)) {
               if(match->options & MATCH_OPTION_NOTIFY) {
                  if(match->options & MATCH_OPTION_ATTACHED) {
                     output(getdsc(match->owner),match->owner,0,1,2,ANSI_LGREEN"Please specify the name of the object you would like to search for within %s"ANSI_LYELLOW"%s"ANSI_LGREEN".",Article(match->start,LOWER,DEFINITE),unparse_object(match->owner,match->start,0));
		  } else if(!Blank(match->remainder)) {
                     output(getdsc(match->owner),match->owner,0,1,2,ANSI_LGREEN"Please specify the name of what you would like to search for '"ANSI_LYELLOW"%s"ANSI_LGREEN"' within.",match->remainder);
		  } else output(getdsc(match->owner),match->owner,0,1,2,ANSI_LGREEN"Please specify the name of a character or object.");
	       }
               return(match_result(NOTHING,1));
	    }
	 }
      } else match->options &= ~MATCH_OPTION_NOTIFY;

      /* ---->  Match keyword or direct reference  <---- */
      if(match && (match->phase == MATCH_PHASE_KEYWORD)) {
         match->phase++;
         if((*(match->search) == MATCH_TOKEN) && !match->recursed) {

            /* ---->  Specific object type(s) only  <---- */
            if(!(match->options & MATCH_OPTION_PREFERRED)) {
               int newtypes = 0;
               int newtype;

               ptr = match->search;
               while(*ptr && (*ptr != ':')) {
                     for(ptr++; *ptr && ((*ptr == ' ') || (*ptr == ';') || (*ptr == ',')); ptr++);
                     for(dest = buffer; *ptr && !((*ptr == ':') || (*ptr == ';') || (*ptr == ',')); *dest++ = *ptr++);
	             for(*dest-- = '\0', tmp = dest; (tmp >= buffer) && (*tmp == ' '); *tmp-- = '\0');
                     if(!BlankContent(buffer)) {
                        if((newtype = search_name_to_type(buffer)) == NOTHING) {
                           if(match->options & MATCH_OPTION_NOTIFY)
                              output(getdsc(match->owner),match->owner,0,1,2,ANSI_LGREEN"Sorry, the object type '"ANSI_LWHITE"%s"ANSI_LGREEN"' is unknown.",buffer);
			} else newtypes |= (type_to_search[newtype]);
		     }
	       }

               if(newtypes) match->subtypes = newtypes;
               FREENULL(match->search);
               match->search    = match->remainder;
               match->remainder = NULL;
               return(match_result(match_object(match->owner,match->start,match->end,match->search,match->phase,match->endphase,match->types,match->options,match->subtypes,NULL,match->recursed + 1),0));
	    } else return(match_result(NOTHING,1));
	 } else if((*(match->search) == LOOKUP_TOKEN) && (match->subtypes & SEARCH_CHARACTER)) {

            /* ---->  *<NAME>  <---- */
            dbref character = lookup_character(match->owner,match->search,1);
            command_type |= MATCH_ABSOLUTE;
            if(!Validchar(character)) {
               if(match->options & MATCH_OPTION_NOTIFY)
                  output(getdsc(match->owner),match->owner,0,1,2,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",match->search + 1);
               character = NOTHING;
	    } else match->parent = Location(character);
            return(match_result(character,1));
	 } else if(*(match->search) == NUMBER_TOKEN) {

            /* ---->  #ID  <---- */
            command_type |= MATCH_ABSOLUTE;
            if(isdigit(*(match->search + 1)))
               object = atol(match->search + 1);
                  else object = NOTHING;
            
            if(!Valid(object) || (Typeof(object) == TYPE_FREE)) {
               if(match->options & MATCH_OPTION_NOTIFY)
                   output(getdsc(match->owner),match->owner,0,1,2,ANSI_LGREEN"Sorry, the object "ANSI_LWHITE"%s"ANSI_LGREEN" doesn't exist.",match->search);
                object = NOTHING;
	    } else if(!(Abode(object) || Visible(object) || (Owner(match->owner) == Owner(object)) || Level4(Owner(match->owner)) || (((Typeof(object) != TYPE_COMMAND) || (!(command_type & CMD_EXEC_PRIVS) && !(match->options & MATCH_OPTION_RESTRICTED))) && (Experienced(Owner(match->owner)) || (!in_command && friendflags_set(Owner(object),match->owner,object,FRIEND_READ)))) || can_write_to(match->owner,object,0))) {

               /* ---->  Match by #ID not allowed  <---- */
               if(match->options & MATCH_OPTION_NOTIFY)
                   output(getdsc(match->owner),match->owner,0,1,2,ANSI_LGREEN"Sorry, the object "ANSI_LWHITE"%s"ANSI_LGREEN" cannot be found.",match->search);
                object = NOTHING;
	    } else match->parent = Location(object);
            return(match_result(object,1));
	 } else if((match->subtypes & SEARCH_CHARACTER) && !strcasecmp(match->search,"me")) {

            /* ---->  'me'  <---- */
            command_type |= MATCH_ABSOLUTE;
            match->parent = Location(match->owner);
            return(match_result(match->owner,1));
	 } else if((match->subtypes & SEARCH_ROOM) && !strcasecmp(match->search,"here")) {
              
            /* ---->  'here'  <---- */
            command_type |= MATCH_ABSOLUTE;
            match->parent = Location(Location(match->owner));
            return(match_result(Location(match->owner),1));
	 } else if((match->subtypes & SEARCH_COMMAND) && !strcasecmp(match->search,"myself")) {

            /* ---->  'myself'  <---- */
            command_type |= MATCH_ABSOLUTE;
            if(in_command) {
               if(Valid(current_command)) {
                  match->parent = Location(current_command);
                  return(match_result(current_command,1));
	       }
	    } else if(match->options & MATCH_OPTION_NOTIFY)
               output(getdsc(match->owner),match->owner,0,1,0,ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"MYSELF"ANSI_LGREEN" matching keyword may only be used from within a compound command.");
            return(match_result(NOTHING,1));
	 } else if((match->subtypes & SEARCH_COMMAND) && !strcasecmp(match->search,"myleaf")) {

            /* ---->  'myleaf'  <---- */
            command_type |= MATCH_ABSOLUTE;
            if(in_command) {
               if(Valid(parent_object)) {
                  match->parent = parent_object;
                  return(match_result(parent_object,1));
	       }
	    } else if(match->options & MATCH_OPTION_NOTIFY)
               output(getdsc(match->owner),match->owner,0,1,0,ANSI_LGREEN"Sorry, the "ANSI_LYELLOW"MYLEAF"ANSI_LGREEN" matching keyword may only be used from within a compound command.");
            return(match_result(NOTHING,1));
	 }
      }

      /* ---->  Exit if match recursion limit has been reached (Already logged)  <---- */
      if(match_recursed) return(match_result(NOTHING,1));

      /* ---->  Match within current search phase  <---- */
      if(match) {
         while((match->phase <= match->endphase) && !match_recursed) {
               if(match->phase < MATCH_PHASE_GLOBAL) {

                  /* ---->  Ending object reached?  <---- */
                  if((match->start == match->end) && (match->end != NOTHING))
                     return(match_result(NOTHING,1));

                  /* ---->  Match within contents of current object (Self, Location & Area)  <---- */
                  do {
                     if(match->order < 0) {
                        match->order   = 0;
                        match->current = getfirst(match->start,inlist[match_order[(int) match->order].phase[match->phase - 1]],&(match->currentobj));
		     }

                     if(Valid(match->current)) {
                        if(!(options & MATCH_OPTION_CONTINUE)) {
                           if(Typeof(match->current) == match_order[(int) match->order].phase[match->phase - 1]) {
                              if(match->options & MATCH_OPTION_LISTS) {

                                 /* ---->  Match within appropriate lists of attached objects  <---- */
                                 if(!(match->options & MATCH_OPTION_ATTACHED) && (match->current != match->exclude)) {
#ifdef DEBUG_MATCH
                                    output(getdsc(match->owner),match->owner,0,1,(match->recursed * 2),ANSI_LMAGENTA"%sRecursing into %s"ANSI_LWHITE"%s"ANSI_LMAGENTA".",strpad(' ',(match->recursed * 2),buffer),Article(match->current,LOWER,DEFINITE),getcname(match->owner,match->current,1,0));
#endif
                                    object = match_object(match->owner,match->current,match->end,match->search,MATCH_PHASE_SELF,MATCH_PHASE_SELF,match->types & match_contents[Typeof(match->current)],match->options & ~(MATCH_OPTION_NOTIFY|MATCH_OPTION_LISTS),match->subtypes,match,match->recursed + 1);
                                    if(Valid(object)) return(match_result(object,0));
#ifdef DEBUG_MATCH
                                    output(getdsc(match->owner),match->owner,0,1,(match->recursed * 2),ANSI_LMAGENTA"%sLeaving %s"ANSI_LWHITE"%s"ANSI_LMAGENTA".",strpad(' ',(match->recursed * 2),buffer),Article(match->current,LOWER,DEFINITE),getcname(match->owner,match->current,1,0));
#endif
				 }
			      } else {

                                 /* ---->  Match name of current object (If correct type)  <---- */
                                 if(match->types & type_to_search[Typeof(match->current)])
                                    if(match->subtypes & type_to_search[Typeof(match->current)])
                                       if(!((match->options & MATCH_OPTION_SKIP_INVISIBLE) && (Invisible(match->current) || ((Typeof(match->current) == TYPE_COMMAND) && !Executable(match->current)) || ((Typeof(match->current) == TYPE_CHARACTER) && !Connected(match->current)))) && !((match->options & MATCH_OPTION_SKIP_ROOMZERO) && RoomZero(match->current))) {
#ifdef DEBUG_MATCH            
                                          output(getdsc(match->owner),match->owner,0,1,(match->recursed * 2),ANSI_LRED"%sMatching against %s"ANSI_LWHITE"%s"ANSI_LRED".",strpad(' ',(match->recursed * 2),buffer),Article(match->current,LOWER,DEFINITE),getcname(match->owner,match->current,1,0));
#endif
                                          if(match_name(match->search,match->current,(match->options & MATCH_OPTION_COMMAND) ? (MATCH_TYPE_FIRST|MATCH_TYPE_LIST):match_types[Typeof(match->current)],((match->options & MATCH_OPTION_FORCE_EXACT) != 0)))
                                             return(match_result(match->current,1));
				       }
		   	      }
			   }
			} else options &= ~MATCH_OPTION_CONTINUE;
		     }

                     /* ---->  Get next object in list (Move to next list, if no next object)  <---- */
                     getnext(match->current,inlist[match_order[(int) match->order].phase[match->phase - 1]],match->currentobj);
                     while(!Valid(match->current) && (match_order[(int) match->order].phase[match->phase - 1] != NOTHING) && !match_recursed)
                           if(match_order[(int) ++(match->order)].phase[match->phase - 1] == NOTHING) {
                              if(!(match->options & MATCH_OPTION_LISTS) && !(match->options & MATCH_OPTION_ATTACHED)) {
                                 match->options |= MATCH_OPTION_LISTS;
                                 match->order    = 0;
                                 match->current  = getfirst(match->start,inlist[match_order[(int) match->order].phase[match->phase - 1]],&(match->currentobj));
			      } else match->options &= ~MATCH_OPTION_LISTS;
			   } else match->current = getfirst(match->start,inlist[match_order[(int) match->order].phase[match->phase - 1]],&(match->currentobj));

#ifdef DEBUG_MATCH
                     output(getdsc(match->owner),match->owner,0,1,(match->recursed * 2),ANSI_LGREEN"%sProcessing object %s"ANSI_LWHITE"%s"ANSI_LRED".",strpad(' ',(match->recursed * 2),buffer),Article(match->current,LOWER,DEFINITE),getcname(match->owner,match->current,1,0));
#endif
                  } while((match_order[(int) match->order].phase[match->phase - 1] != NOTHING) && !match_recursed);

                  /* ---->  No match found:  Move to next match phase  <---- */
                  match->exclude  =  match->start;
                  match->order    =  NOTHING;
                  match->start    =  Location(match->start);
                  match->parent   =  match->start;
                  match->current  =  NOTHING;
                  match->options &= ~MATCH_OPTION_LISTS;
                  if(match->phase < MATCH_PHASE_AREA) match->phase++;
                     else if(!Valid(match->start)) match->phase++;
               } else if((match->types & SEARCH_COMMAND) && (match->subtypes & SEARCH_COMMAND)) {

                  /* ---->  Match in global compound command tertiary tree  <---- */
                  if(match->order < 0) match->order = 0;
                  match->current = global_lookup(match->search,++(match->order));
                  if(Valid(match->current)) {
                     match->parent = GLOBAL_COMMANDS;
                     return(match_result(match->current,1));
                  } else match->phase++;
               } else match->phase++;
	 }
      }

      /* ---->  No match found  <---- */
      if(match->options & MATCH_OPTION_NOTIFY) {
         if(match->options & MATCH_OPTION_ATTACHED)
            output(getdsc(match->owner),match->owner,0,1,2,ANSI_LGREEN"Sorry, the object '"ANSI_LWHITE"%s"ANSI_LGREEN"' cannot be found within %s"ANSI_LYELLOW"%s"ANSI_LGREEN".",match->search,Article(start,LOWER,DEFINITE),unparse_object(match->owner,start,0));
               else output(getdsc(match->owner),match->owner,0,1,2,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"%s"ANSI_LGREEN"' cannot be found.",match->search);
      }
      return(match_result(NOTHING,1));
}

/* ---->  Begin new match, giving particular type(s) of object(s) preference  <---- */
/*  OWNER      = Owner of match                                        */
/*  START      = Starting object                                       */
/*  SEARCHSTR  = Search string                                         */
/*  STARTPHASE = Starting phase                 (MATCH_PHASE_KEYWORD)  */
/*  ENDPHASE   = Ending phase                    (MATCH_PHASE_GLOBAL)  */
/*  TYPES      = Types of objects to match for           (SEARCH_ALL)  */
/*  PREFERRED  = Preferred types of object             (SEARCH_THING)  */
/*  OPTIONS    = Matching options              (MATCH_OPTION_DEFAULT)  */
dbref match_preferred(dbref owner,dbref start,const char *searchstr,unsigned char startphase,unsigned char endphase,int types,int preferred,int options)
{
      dbref result = NOTHING;

      if(preferred > 0) {
         int prefoptions = (options & ~MATCH_OPTION_NOTIFY);

         if(types == SEARCH_PREFERRED) prefoptions |= MATCH_OPTION_SKIP_INVISIBLE;
         if(!(!Blank(searchstr) && strchr(searchstr,':')))
            result = match_object(owner,start,NOTHING,searchstr,startphase,endphase,types & preferred,prefoptions|MATCH_OPTION_PREFERRED,SEARCH_ALL,NULL,0);
               else preferred = 0;
      }

      if(!Valid(result))
         result = match_object(owner,start,NOTHING,searchstr,(!preferred || (startphase > MATCH_PHASE_KEYWORD)) ? startphase:MATCH_PHASE_KEYWORD,endphase,types,options,SEARCH_ALL,NULL,0);
      return(result);
}

/* ---->  Handle return of match result, popping match if nothing found  <---- */
static dbref match_result(dbref result,unsigned char found)
{
      if(found) {
         if(Valid(result)) {

            /* ---->  Matching object found  <---- */
            if(match->options & MATCH_OPTION_PARENT)
               for(parent_object = match->parent; Valid(parent_object) && (Typeof(parent_object) == TYPE_COMMAND); parent_object = Location(parent_object));

            /* ---->  Parse [FROM-TO] index of dynamic array  <---- */
            if(Typeof(result) == TYPE_ARRAY)
               match_parse_array_elements(match->index);

            /* ---->  Match remainder (After ':') in multiple match  <---- */
            if(match->remainder)
               result = match_object(match->owner,result,match->end,match->remainder,MATCH_PHASE_SELF,MATCH_PHASE_SELF,match->types,(match->options)|MATCH_OPTION_ATTACHED,match->subtypes,NULL,match->recursed + 1);
	 } else {

            /* ---->  Match not found (At top-level)  <---- */
            if(match->options & MATCH_OPTION_PARENT)
               parent_object = NOTHING;

            elementfrom = NOTHING;
            elementto   = NOTHING;
            *indexfrom  = '\0';
            *indexto    = '\0';
	 }
      }

      if((!Valid(result) && match->recursed) || (match->options & MATCH_OPTION_SINGLE))
         match_pop();
      return(result);
}

/* ---->  Find next occurrence  <---- */
dbref match_continue(void) {
      dbref         result   = NOTHING;
      unsigned char finished = 0;

      if(match && (match->options & MATCH_OPTION_MULTIPLE)) {
         do {
            if(!match->recursed) finished = 1;
            result = match_object(NOTHING,NOTHING,NOTHING,NULL,0,0,0,MATCH_OPTION_CONTINUE,0,NULL,0);
         } while((result == NOTHING) && match && !finished);
      }

      return(result);
}

/* ---->  Clear match results, popping remaining results off stack  <---- */
void match_done(void) {
     unsigned char finished = 0;

     if(match) do {
        if(!match->recursed) finished = 1;
        match_pop();
     } while(match && !finished);
}

/* ---->  Perform match and return result  <---- */
/*        (VAL1:  0 = No error messages, 1 = Display error messages.)  */
/*        (VAL2:  0 = {@?match}, 1 = {@?myleaf})                       */
void match_query(CONTEXT)
{
     int   preftypes = 0,preftype;
     dbref object,cached_parent;
     char  *dest;

     /* ---->  '{@?myleaf}' with no parameters  <---- */
     if(val2 && Blank(params)) {
        setreturn((in_command) ? getnameid(player,parent_object,NULL):ERROR,COMMAND_SUCC);
        return;
     }

     /* ---->  Optional preferred type(s)  <---- */
     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(arg2)) {
        while(*arg2) {
              for(; *arg2 && ((*arg2 == ' ') || (*arg2 == ';') || (*arg2 == ',')); arg2++);
              for(dest = querybuf; *arg2 && !((*arg2 == ';') || (*arg2 == ',')); *dest++ = *arg2++);
	      for(*dest-- = '\0'; (dest >= querybuf) && (*dest == ' '); *dest-- = '\0');

              if(!BlankContent(querybuf)) {
                 if((preftype = search_name_to_type(querybuf)) == NOTHING)
                    output(getdsc(player),player,0,1,2,ANSI_LGREEN"Sorry, the preferred object type '"ANSI_LWHITE"%s"ANSI_LGREEN"' is unknown.",querybuf);
	               else preftypes |= (type_to_search[preftype]);
	      }
	}
        if(!preftypes) preftypes = SEARCH_PREFERRED;
     }

     /* ---->  Perform match  <---- */
     cached_parent = parent_object;
     object = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,preftypes,((val1) ? MATCH_OPTION_DEFAULT:(MATCH_OPTION_DEFAULT & ~MATCH_OPTION_NOTIFY)) | ((val2) ? MATCH_OPTION_PARENT:0));
     if(val2) object = parent_object;
     if(Valid(object)) setreturn(getnameid(player,object,NULL),COMMAND_SUCC);
        else setreturn(NOTHING_STRING,COMMAND_SUCC);
     parent_object = cached_parent;
}

/* ---->  '{@?my self|leaf}' query command  <---- */
void match_query_my(CONTEXT)
{
     setreturn(ERROR,COMMAND_FAIL);
     if(string_prefix("self",params)) match_query_myself(player,NULL,NULL,NULL,NULL,0,0);
        else if(string_prefix("leaf",params)) match_query(player,NULL,NULL,NULL,NULL,0,1);
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LWHITE"self"ANSI_LGREEN"' or '"ANSI_LWHITE"leaf"ANSI_LGREEN"'.");
}

/* ---->  Return ID of currently executed compound command  <---- */
void match_query_myself(CONTEXT)
{
     setreturn((in_command) ? getnameid(player,current_command,NULL):ERROR,COMMAND_SUCC);
}

