/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| SUBSTITUTE.C  -  Implements command ('{<COMMAND>}'), variable               |
|                  ('$<VARIABLE>' / '$(<VARIABLE)'), query ('%{@?<QUERY>}')   |
|                  and %-type substitutions (Pronouns, text colour and        |
|                  formatting.)                                               |
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
| Module originally designed and written by:  J.P.Boggis 13/10/1994.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: substitute.c,v 1.2 2005/06/29 20:28:04 tcz_monster Exp $

*/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "object_types.h"
#include "flagset.h"
#include "search.h"
#include "fields.h"
#include "match.h"


#define VariableCharacter(x)  (isalnum(x) || (x == '#') || (x == ':') || (x == '.') || (x == '_') || (x == '[') || (x == ']'))


/* ---->  Is specified command allowed within {} command substitution?  <---- */
unsigned char substitute_allowable_command(char *command)
{
	 while(*command && (*command == ' ')) command++;
	 if(*command && !strcasecmp("@chp",command)) return(0);
	 return(1);
}

/* ---->  Substitute return value of command '{<COMMAND>}'  <---- */
void substitute_sub_command(dbref player,struct str_ops *str_data)
{
     char  *ptr,*command;
     int   brackets = 1;
     char  closingchar;

     /* ---->  Seek closing '}'  <---- */
     command = ++str_data->src;
     while(*(str_data->src) && brackets) {
           switch(*(str_data->src)) {
                  case '{':
          	       if(*((str_data->src) - 1) != '\\') brackets++;
                       str_data->src++;
                       break;
                  case '}':
                       if(*((str_data->src) - 1) != '\\') brackets--;
                       if(brackets) str_data->src++;
                       break;
                  default:
                       str_data->src++;
	   }
     }
     closingchar      = *(str_data->src);
     *(str_data->src) = '\0';

     /* ---->  Execute TCZ command within {}'s  <---- */
     if(substitute_allowable_command(command)) {
        command_nesting_level++;
        strcat_limits(str_data,(ptr = (char *) command_sub_execute(player,command,1,0)));
        if((command_nesting_level > 0) && ((command_nesting_level <= MAX_CMD_NESTED_MORTAL) || ((command_nesting_level <= MAX_CMD_NESTED_ADMIN) && Level4(db[player].owner)))) command_nesting_level--;
        FREENULL(ptr);
     }

     *(str_data->src) = closingchar;
     if(*(str_data->src)) str_data->src++;
}


/* ---->  Evaluate {}'s in given string and return resulting string  <---- */
void substitute_command(dbref player,char *src,char *dest)
{
     struct str_ops str_data;
     char   *p1,*p2;
     char   c;

     str_data.dest      = dest;
     str_data.src       = src;
     str_data.backslash = 0;
     str_data.length    = 0;

     /* ---->  Don't evaluate {}'s/$'s in a <CONDITION>/<COMMAND> part of '@if', '@for', '@force', '@foreach', '@remote', '@with' or '@while'  <---- */
     for(p1 = src; *p1 && (*p1 == ' '); p1++);
     for(p2 = p1; *p1 && (*p1 != ' '); p1++);
     c = *p1, *p1 = '\0';
     if(*p2 && (*p2 == '@')) {
        switch(*(p2 + 1)) {
               case 'f':
               case 'F':
                    if(!strncasecmp(p2 + 2,"or",2) || !strncasecmp(p2 + 2,"oreach",6)) {

                       /* ---->  '@for' and '@foreach' pre-processing  <---- */
                       for(*p1 = c; *p1 && !selection_do_keyword(p1); p1++);
                       if(*p1 && (*p1 == ' ')) *p1 = '\x02';
		    } else if(!strncasecmp(p2 + 2,"orce",4)) {

                       /* ---->  '@force' pre-processing  <---- */
                       for(*p1 = c; *p1 && (*p1 != '='); p1++);
                       if(*p1) *p1 = '\x03';
		    } else *p1 = c;
                    break;
               case 'i':
               case 'I':
                    if(!strncasecmp(p2 + 2,"f",1)) {

                       /* ---->  '@if' pre-processing  <---- */
                       *p1 = c;
                       if(*p1 && (*p1 == ' ')) *p1 = '\x02';
		    } else *p1 = c;
                    break;
               case 'r':
               case 'R':
		    if(!strncasecmp(p2 + 2,"emote",5)) {

                       /* ---->  '@remote' pre-processing  <---- */
                       for(*p1 = c; *p1 && (*p1 != '='); p1++);
                       if(*p1) *p1 = '\x03';
		    } else *p1 = c;
		    break;
               case 'w':
               case 'W':
                    if(!strncasecmp(p2 + 2,"ith",3)) {

                       /* ---->  '@with' pre-processing  <---- */
                       for(*p1 = c; *p1 && !selection_do_keyword(p1); p1++);
                       if(*p1 && (*p1 == ' ')) *p1 = '\x02';
		    } else if(!strncasecmp(p2 + 2,"hile",4)) {

                       /* ---->  '@while' pre-processing  <---- */
                       *p1 = c;
                       if(*p1 && (*p1 == ' ')) *p1 = '\x02';
		    } else *p1 = c;
                    break;
               default:
                    *p1 = c;
	}
     } else *p1 = c;
     p1 = str_data.dest;

     /* ---->  Substitute in return values of commands in {}'s  <---- */
     while(*str_data.src && (str_data.length < MAX_LENGTH)) switch(*str_data.src) {
           case '\\':
                if(str_data.backslash) {
                   str_data.backslash = 0;
                   *str_data.dest++ = '\\';
                   str_data.length++;
		} else str_data.backslash = 1;
                str_data.src++;
                break;
           case '$':
                if(str_data.backslash) {
                   str_data.backslash = 0;
                   *str_data.dest++ = '$';
                   str_data.length++;
		} else {
                   *str_data.dest++ = '\x01';
                   str_data.length++;
		}
                str_data.src++;
                break;
           case '\x01':
                str_data.backslash = 0;
                str_data.src++;
                break;
           case '\x02':
           case '\x03':

                /* ---->  Don't evaluate {}'s/$'s from this point onwards  <---- */
                str_data.backslash = 0;
                str_data.length++;
                *str_data.dest++ = (*str_data.src == '\x02') ? ' ':'=';
                str_data.src++;
                while(*str_data.src && (str_data.length < MAX_LENGTH))
                      if(*str_data.src == '\\') {
                         if(str_data.backslash) {
                            str_data.backslash = 0;
                            *str_data.dest++ = '\\';
                            str_data.length++;
			 } else str_data.backslash = 1;
                         str_data.src++;
		      } else {
                         *str_data.dest++ = *str_data.src;
                         str_data.backslash = 0;
                         str_data.length++;
                         str_data.src++;
		      }
                break;
           case '{':

                /* ---->  Substitute return value of command  <---- */
                if(str_data.backslash) {
                   str_data.backslash = 0;
                   *str_data.dest++ = '{';
                   str_data.length++;
                   str_data.src++;
		} else substitute_sub_command(player,&str_data);
                break;
           default:
                *str_data.dest++ = *str_data.src;
                str_data.backslash = 0;
                str_data.length++;
                str_data.src++;
     }
     *str_data.dest = '\0';
}


/* ---->  Substitute named variable ('$<VARIABLE>', '$(<VARIABLE>)')  <---- */
void substitute_sub_variable(dbref player,struct str_ops *str_data)
{
     static char               buffer[16],subst[TEXT_SIZE + 1];
     static char               *src,*ptr,*split;
     static unsigned char      brackets,colon;
     static int                olen,nlen;
     static dbref              object;
     static struct   temp_data *temp;

     /* ---->  Seek end of variable  <---- */
     *(str_data->src++) = '$', split = NULL, colon = 0;
     if(isdigit(*(str_data->src))) {
        subst[0] = *(str_data->src++), subst[1] = '\0', ptr = subst + 1;
     } else if(*(str_data->src) == '(') {
        for(ptr = subst, str_data->src++, brackets = 1; *(str_data->src) && (*(str_data->src) != ')'); *ptr++ = *(str_data->src), str_data->src++)
            if(*(str_data->src) == ':') split = ptr, colon = 1;
     } else if(command_type & VARIABLE_SUBST) {
        for(ptr = subst; *(str_data->src); *ptr++ = *(str_data->src), str_data->src++)
            if(*(str_data->src) == ':') split = ptr, colon = 1;
     } else for(ptr = subst; *(str_data->src) && VariableCharacter(*(str_data->src)); *ptr++ = *(str_data->src), str_data->src++)
        if(*(str_data->src) == ':') split = ptr, colon = 1;
     if(!Blank(split)) *split++ = '\0';
     command_type &= ~VARIABLE_SUBST;
     *ptr = '\0', ptr = subst;

     if(isdigit(*subst) && !*(subst + 1)) {
        switch(*subst) {
               case '0':  /* ---->  Return result of last executed command  <---- */
                    strcat_limits(str_data,command_result);
                    break;
               case '1':  /* ---->  1st parameter given to compound command  <---- */
                    strcat_limits(str_data,command_arg1);
                    break;
               case '2':  /* ---->  2nd parameter given to compound command  <---- */
                    strcat_limits(str_data,command_arg2);
                    break;
               case '3':  /* ---->  Both parameters (With '=') given to compound command  <---- */
                    strcat_limits(str_data,command_arg3);
                    break;
               case '4':  /* ---->  Current loop number  <---- */
                    sprintf(buffer,"%d",loopno);
                    strcat_limits(str_data,buffer);
                    break;
               case '5':  /* ---->  Total number of loops so far  <---- */
                    sprintf(buffer,"%d",noloops);
                    strcat_limits(str_data,buffer);
                    break;
               case '6':  /* ---->  Name/#ID of object currently being processed, or contents/index name of dynamic array element currently being processed  <---- */
                    strcat_limits(str_data,command_item);
                    break;
               case '7':  /* ---->  Description of object currently being processed, or index name/contents of dynamic array element currently being processed  <---- */
                    strcat_limits(str_data,command_item2);
                    break;
               default:
                    strcat_limits(str_data,UNKNOWN_VARIABLE);
	}
     } else {

        /* ---->  Substitute temporary variable?  <---- */
        if(ptr)   for(; *ptr   && (*ptr == ' ');   ptr++);
        if(split) for(; *split && (*split == ' '); split++);
        if((temp = temp_lookup(ptr))) {
           if(Blank(split)) {
              strcat_limits(str_data,String(decompress(temp->desc)));
              if(brackets && *(str_data->src) && (*(str_data->src) == ')')) str_data->src++;
              return;
	   } else ptr = decompress(temp->desc);
	}

        /* ---->  Substitute property, variable or dynamic array element  <---- */
        if(!Blank(split) || colon) {
           if(!Blank(ptr)) {
              olen = strlen(subst), nlen = strlen(ptr);
              if(nlen != olen) {
                 if(nlen > olen) {
                    for(nlen -= olen, src = split + strlen(split); src >= split; src--)
                        if((src + nlen) <= (subst + TEXT_SIZE))
                           *(src + nlen) = *src;
                    subst[TEXT_SIZE] = '\0';
                    strcpy(subst,ptr);
                    *(src + nlen) = ':';
		 } else {
                    sprintf(subst,"%s:",ptr);
                    for(src = subst + strlen(subst); *split; *src++ = *split++);
                    *src = '\0';
		 }
	      } else strcpy(subst,ptr), *(split - 1) = ':';
	   } else {
              for(*subst = ':', src = subst + 1; *split; *src++ = *split++);
              *src = '\0';
	   }
           object = match_object(player,player,NOTHING,subst,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_VARIABLE|SEARCH_PROPERTY|SEARCH_ARRAY,MATCH_OPTION_DEFAULT & ~MATCH_OPTION_NOTIFY,SEARCH_ALL,NULL,0);
	} else object = match_object(player,player,NOTHING,subst,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_VARIABLE|SEARCH_PROPERTY|SEARCH_ARRAY,MATCH_OPTION_DEFAULT & ~MATCH_OPTION_NOTIFY,SEARCH_ALL,NULL,0);

        if(Valid(object)) {
           if(!Private(object) || can_read_from(player,object)) {
              if(Typeof(object) == TYPE_ARRAY) {
                 if(array_subquery_elements(player,object,elementfrom,elementto,querybuf) >= 0) strcat_limits(str_data,querybuf);
	      } else if(getfield(object,DESC)) strcat_limits(str_data,getfield(object,DESC));
	   } else strcat_limits(str_data,UNKNOWN_VARIABLE);
	} else strcat_limits(str_data,UNKNOWN_VARIABLE);
        if(brackets && *(str_data->src) && (*(str_data->src) == ')')) str_data->src++;
     }
}

/* ---->  Evaluate $'s in given string and return resulting string  <---- */
void substitute_variable(dbref player,char *src,char *dest)
{
     struct str_ops str_data;

     str_data.dest   = dest;
     str_data.src    = src;
     str_data.length = 0;

     while(*str_data.src && (str_data.length < MAX_LENGTH)) 
           switch(*str_data.src) {
                  case '\x01':
                       if(!((str_data.src > src) && (*(str_data.src - 1) == '\x05'))) {
      	                  substitute_sub_variable(player,&str_data);
                          break;
		       }
                  case '\x02':
                       if(!((str_data.src > src) && (*(str_data.src - 1) == '\x05'))) {
                          *str_data.dest++ = ' ';
                          str_data.length++;
                          str_data.src++;
                          break;
		       }
                  case '\x03':
                       if(!((str_data.src > src) && (*(str_data.src - 1) == '\x05'))) {
                          *str_data.dest++ = '=';
                          str_data.length++;
                          str_data.src++;
                          break;
		       }
                  default:
                       *str_data.dest++ = *str_data.src;
                       str_data.length++;
                       str_data.src++;
	   }
     *str_data.dest = '\0';
}

/* ---->  Substitute result of query command (%{<COMMAND>})  <---- */
void substitute_query(dbref player,struct str_ops *str_data)
{
     dbref cached_owner = db[player].owner,cached_chpid = db[player].data->player.chpid;
     int   cached_commandtype = command_type,brackets = 1; 
     char  buffer[TEXT_SIZE];
     char  *ptr,*start;

     /* ---->  Command to execute?  <---- */
     if(*(str_data->src) && (*(str_data->src) != '{')) return;
     (str_data->src)++;

     /* ---->  Grab command to execute (From within {}'s)  <---- */
     ptr   = buffer;
     start = str_data->src;
     while(*(str_data->src) && brackets) {
           switch(*(str_data->src)) {
                  case '{':
                       brackets++;
                       *ptr++ = *(str_data->src);
                       break;
                  case '}':
                       brackets--;
                       if(brackets) *ptr++ = *(str_data->src);
                       break;
                  default:
                       *ptr++ = *(str_data->src);
	   }
           str_data->src++;
     }
     if(str_data->src > start) (str_data->src)--;
     *ptr = '\0';

     /* ---->  Execute command  <---- */
     command_nesting_level++;
     command_type = (QUERY_COMMAND|QUERY_SUBSTITUTION);
     db[player].owner = player, db[player].data->player.chpid = player;
     if(Validchar(player)) ptr = (char *) command_sub_execute(player,buffer,1,0);
        else command_type = BAD_COMMAND;
     db[player].owner = cached_owner, db[player].data->player.chpid = cached_chpid;
     if((command_nesting_level > 0) && ((command_nesting_level <= MAX_CMD_NESTED_MORTAL) || ((command_nesting_level <= MAX_CMD_NESTED_ADMIN) && Level4(db[player].owner)))) command_nesting_level--;

     /* ---->  Command wasn't a query command?  <---- */
     if(command_type == BAD_COMMAND) {
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"%%{<COMMAND}"ANSI_LGREEN"' may only be used to perform query commands.");
        command_type = cached_commandtype;
        FREENULL(ptr);
        return;
     }

     /* ---->  Substitute return value  <---- */
     command_type = cached_commandtype;
     if(!Blank(ptr)) {
        strcat_limits(str_data,ptr);
        FREENULL(ptr);
     }
}

/* ---->  Substitute %-type substitutions (For pronouns, colour, formatting, etc.) in given string  <---- */
const char *substitute(dbref player,char *dest,char *src,unsigned char addname,const char *def_ansi,struct substitution_data *subst,dbref sender)
{
      unsigned char            striptelnet = (subst) ? ((subst->flags & SUBST_STRIP_TELNET) != 0):0;
      unsigned char            striphtml   = (subst) ? ((subst->flags & SUBST_STRIP_HTML) != 0):0;
      unsigned char            hardhtml    = (subst) ? ((subst->flags & SUBST_HARDHTML) != 0):0;
      unsigned char            htmltag     = (subst) ? ((subst->flags & SUBST_HTMLTAG) != 0):0;
      unsigned char            html        = (subst) ? ((subst->flags & SUBST_HTML) != 0):0;
      const    char            *cur_bg     = (subst) ? subst->cur_bg:ANSI_IBLACK;
      int                      textflags   = (subst) ? subst->textflags:0;
      static   char            buf1[128],buf2[128],buf3[2048];
      struct   descriptor_data *p = getdsc(player);
      char                     cur_ansi[16];
      unsigned char            defaults = 0;
      struct   str_ops         str_data;
      char                     *p1,*p2;
      int                      copied;

      if (!sender) sender = player;
      *dest = '\0';
      if(player == NOBODY) {
         for(p = descriptor_list; p && !(IsHtml(p) && (p->player == NOBODY)); p = p->next);
         player = NOTHING;
         if(!p) return("");
      }

      str_data.dest = dest; str_data.src = src; str_data.length = 0;
      if(!subst) {

         /* ---->  Optionally add name of character in front (If ADDNAME != 0)  <---- */
         if(addname) {
            strcat_limits_exact(&str_data,ANSI_LWHITE);
            if(Validchar(player) && HasArticle(player)) strcat_limits(&str_data,Article(player,UPPER,addname));
            strcat_limits(&str_data,getcname(NOTHING,player,0,0));
            strcat_limits_exact(&str_data,def_ansi);
            strcat_limits(&str_data,pose_string(&str_data.src,"*"));
	 } else if(*str_data.src && (*str_data.src != '\x1B'))
            strcat_limits_exact(&str_data,def_ansi);
         strcpy(cur_ansi,def_ansi);
      } else {
         if(striptelnet || striphtml) {
            str_data.src++, copied = 1;
            while(*str_data.src && copied) {
                  for(; *str_data.src && (*str_data.src != '%'); str_data.src++);
                  if(*str_data.src && *(++str_data.src) && ((striptelnet && (*str_data.src == '|')) || (striphtml && (*str_data.src == '^')))) copied = 0;
	    }

            if(*str_data.src && ((striptelnet && (*str_data.src == '|')) || (striphtml && (*str_data.src == '^')))) {
               subst->flags &= ~(SUBST_STRIP_HTML|SUBST_STRIP_TELNET);
               striptelnet = 0, striphtml = 0;
               str_data.src++;
	    }
            if(!*str_data.src) return("");
	 } /* else if(!IsHtml(p)) {
            if(*str_data.src && !strncmp(str_data.src,"%|",2)) {
               subst->flags |= SUBST_STRIP_TELNET;
               return("");
	    }
	 } else if(*str_data.src && !strncmp(str_data.src,"%^",2)) {
            subst->flags |= SUBST_STRIP_HTML;
            return("");
	    } */
	 /* the above code was commented out because while it appears to be
	    a quick optimisation to make things faster, it ends up printing
	    absolutely no output if you're on telnet and the first two
	    characters are %| */

         strcpy(cur_ansi,subst->cur_ansi);
         if(html) strcat_limits_char(&str_data,'\016');
         if(!(htmltag && (html || hardhtml))) {
            strcat_limits_exact(&str_data,cur_ansi);
            if(textflags & TXT_BOLD)      strcat_limits_exact(&str_data,ANSI_LIGHT);
            if(textflags & TXT_BLINK)     strcat_limits_exact(&str_data,ANSI_BLINK);
            if(textflags & TXT_UNDERLINE) strcat_limits_exact(&str_data,ANSI_UNDERLINE);
            if(textflags & TXT_INVERSE) {
               while(*str_data.src && (*str_data.src == ' ') && (str_data.length < MAX_LENGTH))
                     *str_data.dest++ = ' ', str_data.length++, str_data.src++;
               strcat_limits_exact(&str_data,cur_bg);
	    }
	 } else defaults = 1;
      }

      /* ---->  Substitute %-types  <---- */
      while(*str_data.src && (str_data.length < MAX_LENGTH)) 
            switch(*str_data.src) {
                   case ' ':
                   case '\n':

                        /* ---->  Space or Newline  <---- */
                        if(*str_data.src != '\n') {
                           for(p2 = str_data.src; *p2 && (*p2 == ' '); p2++);
                           if(*p2 && (*p2 == '\n')) {
                              if((textflags & TXT_BLINK) || (textflags & TXT_UNDERLINE)) strcat_limits_exact(&str_data,ANSI_DBLACK);
                              if(textflags & TXT_INVERSE) strcat_limits_exact(&str_data,ANSI_IBLACK);
			   }
                           while(*str_data.src && (*str_data.src == ' ') && (str_data.length < MAX_LENGTH))
                                 *str_data.dest++ = ' ', str_data.length++, str_data.src++;
			} else {
                           if((textflags & TXT_BLINK) || (textflags & TXT_UNDERLINE)) strcat_limits_exact(&str_data,ANSI_DBLACK);
                           if(textflags & TXT_INVERSE) strcat_limits_exact(&str_data,ANSI_IBLACK);
			}
                        if(*str_data.src == '\n') {
                           *str_data.dest++ = '\n', str_data.length++, str_data.src++;
                           while(*str_data.src && (*str_data.src == ' ') && (str_data.length < MAX_LENGTH))
                                 *str_data.dest++ = ' ', str_data.length++, str_data.src++;
                           if((textflags & TXT_BLINK) || (textflags & TXT_UNDERLINE)) {
                              strcat_limits_exact(&str_data,cur_ansi);
                              if(textflags & TXT_BOLD)      strcat_limits_exact(&str_data,ANSI_LIGHT);
                              if(textflags & TXT_BLINK)     strcat_limits_exact(&str_data,ANSI_BLINK);
                              if(textflags & TXT_UNDERLINE) strcat_limits_exact(&str_data,ANSI_UNDERLINE);
			   }
                           if(textflags & TXT_INVERSE) strcat_limits_exact(&str_data,cur_bg);
			}
                        break;
                   case '\016':

                        /* ---->  Hard-coded HTML toggle  <---- */
                        if(!html && !(command_type & NESTED_SUBSTITUTION)) {
                           hardhtml = !hardhtml;
                           *str_data.dest++ = '\016';
                           str_data.length++;
                           str_data.src++;
			} else str_data.src++;
                        break;
                   case '<':

                        /* ---->  Open HTML tag  <---- */
                        if(html || hardhtml) htmltag = 1;
                        *str_data.dest++ = *str_data.src++;
                        str_data.length++;
                        break;
                   case '>':

                        /* ---->  Close HTML tag  <---- */
                        if(html || hardhtml) {
                           *str_data.dest++ = *str_data.src++;
                           str_data.length++;
                           htmltag = 0;
                           if(defaults) {
                              strcat_limits_exact(&str_data,cur_ansi);
                              if(textflags & TXT_BOLD)      strcat_limits_exact(&str_data,ANSI_LIGHT);
                              if(textflags & TXT_BLINK)     strcat_limits_exact(&str_data,ANSI_BLINK);
                              if(textflags & TXT_UNDERLINE) strcat_limits_exact(&str_data,ANSI_UNDERLINE);
                              if(textflags & TXT_INVERSE)   strcat_limits_exact(&str_data,cur_bg);
			   }		
			} else {
                           *str_data.dest++ = *str_data.src++;
                           str_data.length++;
			}
                        break;
                   case '%':

                        /* ---->  Substitution  <---- */
                        str_data.src++, p1 = NULL;
                        if(*str_data.src && !htmltag) {
                           switch(*str_data.src) {
                                  case '{':  /* ---->  Query command substitution  <---- */
                                       if(!(command_type & NESTED_SUBSTITUTION))
                                          substitute_query(Validchar(player) ? player:Validchar(maint_owner) ? maint_owner:ROOT,&str_data);
                                       break;
                                  case 'a':  /* ---->  Absolute pronoun  <---- */
                                       p1 = str_data.dest;
				       strcat_limits(&str_data,Absolute(sender,LOWER));
                                       break;
                                  case 'A':  /* ---->  Absolute pronoun  <---- */
                                       p1 = str_data.dest;
				       strcat_limits(&str_data,Absolute(sender,UPPER));
                                       break;
                                  case 'b':
                                  case 'B':  /* ---->  Blue text  <---- */
                                       strcat_limits_exact(&str_data,ANSI_DBLUE);
                                       if(textflags & TXT_INVERSE) strcat_limits_exact(&str_data,cur_bg = ANSI_IBLACK);
                                       strcpy(cur_ansi,ANSI_DBLUE);
                                       textflags = 0;
                                       break;
                                  case 'c':
                                  case 'C':  /* ---->  Cyan text  <---- */
                                       strcat_limits_exact(&str_data,ANSI_DCYAN);
                                       if(textflags & TXT_INVERSE) strcat_limits_exact(&str_data,cur_bg = ANSI_IBLACK);
                                       strcpy(cur_ansi,ANSI_DCYAN);
                                       textflags = 0;
                                       break;
                                  case 'd':
                                  case 'D':  /* ---->  Dark (Normal) text  <---- */
                                       if(!Blank(cur_ansi) && (*cur_ansi == '\x1B')) {
 
                                          /* ---->  Split colour code from hi-light  <---- */
                                          for(p2 = cur_ansi; *p2 && (*p2 != 'm'); p2++);
                                          if(*p2 && (*p2 == 'm')) p2++;
                                          *p2 = '\0';

                                          /* ---->  Copy new code into text  <---- */
                                          strcat_limits_exact(&str_data,cur_ansi);
                                          if(textflags & TXT_BLINK)     strcat_limits_exact(&str_data,ANSI_BLINK);
                                          if(textflags & TXT_UNDERLINE) strcat_limits_exact(&str_data,ANSI_UNDERLINE);
                                          if(textflags & TXT_INVERSE)   strcat_limits_exact(&str_data,cur_bg);
		 		       }
                                       textflags &= ~TXT_BOLD;
                                       break;
                                  case 'f':
                                  case 'F':  /* ---->  Flashing (Blinking) text  <---- */
                                       strcat_limits_exact(&str_data,ANSI_BLINK);
                                       textflags |= TXT_BLINK;
                                       break;
                                  case 'g':
                                  case 'G':  /* ---->  Green text  <---- */
                                       strcat_limits_exact(&str_data,ANSI_DGREEN);
                                       if(textflags & TXT_INVERSE) strcat_limits_exact(&str_data,cur_bg = ANSI_IBLACK);
                                       strcpy(cur_ansi,ANSI_DGREEN);
                                       textflags = 0;
                                       break;
                                  case 'h':
                                  case 'H':  /* ---->   Toggle evaluation of HTML tags  <---- */
                                       if(!hardhtml && !(command_type & NESTED_SUBSTITUTION) && !(command_type & COMM_CMD)) {
                                          strcat_limits_char(&str_data,'\016');
                                          html = !html;
			 	       }
                                       break;
                                  case 'i':
                                  case 'I':  /* ---->  Inversed text/text background colour   <---- */
                                       if(*(str_data.src + 1) == '%') {
                                          str_data.src += 2;
                                          switch(*str_data.src) {
                                                 case 'b':
                                                 case 'B':  /* ---->  Blue background  <---- */
                                                      strcat_limits_exact(&str_data,cur_bg = ANSI_IBLUE);
                                                      break;
                                                 case 'c':
                                                 case 'C':  /* ---->  Cyan background  <---- */
                                                      strcat_limits_exact(&str_data,cur_bg = ANSI_ICYAN);
                                                      break;
                                                 case 'g':
                                                 case 'G':  /* ---->  Green background  <---- */
                                                      strcat_limits_exact(&str_data,cur_bg = ANSI_IGREEN);
                                                      break;
                                                 case 'm':
                                                 case 'M':  /* ---->  Magenta (Purple) background  <---- */
                                                      strcat_limits_exact(&str_data,cur_bg = ANSI_IMAGENTA);
                                                      break;
                                                 case 'r':
                                                 case 'R':  /* ---->  Red background  <---- */
                                                      strcat_limits_exact(&str_data,cur_bg = ANSI_IRED);
                                                      break;
                                                 case 'w':
                                                 case 'W':  /* ---->  White background  <---- */
                                                      strcat_limits_exact(&str_data,cur_bg = ANSI_IWHITE);
                                                      break;
                                                 case 'y':
                                                 case 'Y':  /* ---->  Yellow background  <---- */
                                                      strcat_limits_exact(&str_data,cur_bg = ANSI_IYELLOW);
                                                      break;
                                                 case 'z':
                                                 case 'Z':  /* ---->  Black background  <---- */
                                                      strcat_limits_exact(&str_data,cur_bg = ANSI_IBLACK);
                                                      break;
                                                 default:
                                                      str_data.src -= 2;
					  }
				       } else switch(cur_ansi[5]) {
                                          case '0':
                                               strcat_limits_exact(&str_data,cur_bg = ANSI_IWHITE);
                                               break;
                                          case '1':
                                               strcat_limits_exact(&str_data,cur_bg = ANSI_IWHITE);
                                               break;
                                          case '2':
                                               strcat_limits_exact(&str_data,cur_bg = ANSI_IWHITE);
                                               break;
                                          case '3':
                                               strcat_limits_exact(&str_data,cur_bg = ANSI_IRED);
                                               break;
                                          case '4':
                                               strcat_limits_exact(&str_data,cur_bg = ANSI_ICYAN);
                                               break;
                                          case '5':
                                               strcat_limits_exact(&str_data,cur_bg = ANSI_IRED);
                                               break;
                                          case '6':
                                               strcat_limits_exact(&str_data,cur_bg = ANSI_IBLUE);
                                               break;
                                          case '7':
                                               strcat_limits_exact(&str_data,cur_bg = ANSI_IBLUE);
                                               break;
                                          default:
                                               strcat_limits_exact(&str_data,cur_bg = ANSI_IWHITE);
				       }
                                       textflags |= TXT_INVERSE;
                                       break;
                                  case 'l':
                                  case 'L':  /* ---->  Hilighted (Emboldened) text  <---- */
                                       strcat_limits_exact(&str_data,ANSI_LIGHT);
                                       textflags |= TXT_BOLD;
                                       break;
                                  case 'm':
                                  case 'M':  /* ---->  Magenta (Purple) text  <---- */
                                       strcat_limits_exact(&str_data,ANSI_DMAGENTA);
                                       if(textflags & TXT_INVERSE) strcat_limits_exact(&str_data,cur_bg = ANSI_IBLACK);
                                       strcpy(cur_ansi,ANSI_DMAGENTA);
                                       textflags = 0;
                                       break;
                                  case 'n':
                                  case 'N':  /* ---->  Character's name  <---- */
                                       strcat_limits_exact(&str_data,ANSI_LWHITE);
                                       if(textflags & TXT_BLINK)     strcat_limits_exact(&str_data,ANSI_BLINK);
                                       if(textflags & TXT_UNDERLINE) strcat_limits_exact(&str_data,ANSI_UNDERLINE);
                                       if(textflags & TXT_INVERSE)   strcat_limits_exact(&str_data,cur_bg);
                                       strcat_limits(&str_data,getcname(NOTHING,sender,0,0));
                                       strcat_limits_exact(&str_data,cur_ansi);
                                       if(textflags & TXT_BOLD)      strcat_limits_exact(&str_data,ANSI_LIGHT);
                                       if(textflags & TXT_BLINK)     strcat_limits_exact(&str_data,ANSI_BLINK);
                                       if(textflags & TXT_UNDERLINE) strcat_limits_exact(&str_data,ANSI_UNDERLINE);
                                       if(textflags & TXT_INVERSE)   strcat_limits_exact(&str_data,cur_bg);
                                       break;
                                  case 'o':  /* ---->  Objective pronoun  <---- */
                                       p1 = str_data.dest;
				       strcat_limits(&str_data,Objective(sender,LOWER));
                                       break;
                                  case 'O':  /* ---->  Objective pronoun  <---- */
                                       p1 = str_data.dest;
				       strcat_limits(&str_data,Objective(sender,UPPER));
                                       break;
                                  case 'p':  /* ---->  Possessive pronoun  <---- */
                                       p1 = str_data.dest;
				       strcat_limits(&str_data,Possessive(sender,LOWER));
                                       break;
                                  case 'P':  /* ---->  Possessive pronoun  <---- */
                                       p1 = str_data.dest;
				       strcat_limits(&str_data,Possessive(sender,UPPER));
                                       break;
                                  case 'r':
                                  case 'R':  /* ---->  Red text  <---- */
                                       strcat_limits_exact(&str_data,ANSI_DRED);
                                       if(textflags & TXT_INVERSE) strcat_limits_exact(&str_data,cur_bg = ANSI_IBLACK);
                                       strcpy(cur_ansi,ANSI_DRED);
                                       textflags = 0;
                                       break;
                                  case 's':  /* ---->  Subjective pronoun  <---- */
                                       p1 = str_data.dest;
				       strcat_limits(&str_data,Subjective(sender,LOWER));
                                       break;
                                  case 'S':  /* ---->  Subjective pronoun  <---- */
                                       p1 = str_data.dest;
				       strcat_limits(&str_data,Subjective(sender,UPPER));
                                       break;
                                  case 'u':
                                  case 'U':  /* ---->  Underlined text  <---- */
                                       strcat_limits_exact(&str_data,ANSI_UNDERLINE);
                                       textflags |= TXT_UNDERLINE;
                                       break;
                                  case 'v':  /* ---->  Reflexive pronoun  <---- */
                                       p1 = str_data.dest;
				       strcat_limits(&str_data,Reflexive(sender,LOWER));
                                       break;
                                  case 'V':  /* ---->  Reflexive pronoun  <---- */
                                       p1 = str_data.dest;
				       strcat_limits(&str_data,Reflexive(sender,UPPER));
                                       break;
                                  case 'w':
                                  case 'W':  /* ---->  White text  <---- */
                                       strcat_limits_exact(&str_data,ANSI_DWHITE);
                                       if(textflags & TXT_INVERSE) strcat_limits_exact(&str_data,cur_bg = ANSI_IBLACK);
                                       strcpy(cur_ansi,ANSI_DWHITE);
                                       textflags = 0;
                                       break;
                                  case 'x':
                                  case 'X':  /* ---->  Default text colour/formatting  <---- */
                                       strcat_limits_exact(&str_data,def_ansi);
                                       strcpy(cur_ansi,def_ansi);
                                       textflags = 0;
                                       break;
                                  case 'y':
                                  case 'Y':  /* ---->  Yellow text  <---- */
                                       strcat_limits_exact(&str_data,ANSI_DYELLOW);
                                       if(textflags & TXT_INVERSE) strcat_limits_exact(&str_data,cur_bg = ANSI_IBLACK);
                                       strcpy(cur_ansi,ANSI_DYELLOW);
                                       textflags = 0;
                                       break;
                                  case 'z':
                                  case 'Z':  /* ---->  Black text  <---- */
                                       strcat_limits_exact(&str_data,ANSI_DBLACK);
                                       if(textflags & TXT_INVERSE) strcat_limits_exact(&str_data,cur_bg = ANSI_IBLACK);
                                       strcpy(cur_ansi,ANSI_DBLACK);
                                       textflags = 0;
                                       break;
                                  case '0':  /* ---->  Reset hanging indent  <---- */
                                       if(!(command_type & NESTED_SUBSTITUTION))
                                          strcat_limits_exact(&str_data,"\x05\x0B");
                                       break;
                                  case '1':  /* ---->  Increment hanging indent by 1  <---- */
                                       if(!(command_type & NESTED_SUBSTITUTION))
                                          strcat_limits_exact(&str_data,"\x05\x01");
                                       break;
                                  case '2':  /* ---->  Increment hanging indent by 2  <---- */
                                       if(!(command_type & NESTED_SUBSTITUTION))
                                          strcat_limits_exact(&str_data,"\x05\x02");
                                       break;
                                  case '3':  /* ---->  Increment hanging indent by 3  <---- */
                                       if(!(command_type & NESTED_SUBSTITUTION))
                                          strcat_limits_exact(&str_data,"\x05\x03");
                                       break;
                                  case '4':  /* ---->  Increment hanging indent by 4  <---- */
                                       if(!(command_type & NESTED_SUBSTITUTION))
                                          strcat_limits_exact(&str_data,"\x05\x04");
                                       break;
                                  case '5':  /* ---->  Increment hanging indent by 5  <---- */
                                       if(!(command_type & NESTED_SUBSTITUTION))
                                          strcat_limits_exact(&str_data,"\x05\x05");
                                       break;
                                  case '6':  /* ---->  Increment hanging indent by 6  <---- */
                                       if(!(command_type & NESTED_SUBSTITUTION))
                                          strcat_limits_exact(&str_data,"\x05\x06");
                                       break;
                                  case '7':  /* ---->  Increment hanging indent by 7  <---- */
                                       if(!(command_type & NESTED_SUBSTITUTION))
                                          strcat_limits_exact(&str_data,"\x05\x07");
                                       break;
                                  case '8':  /* ---->  Increment hanging indent by 8  <---- */
                                       if(!(command_type & NESTED_SUBSTITUTION))
                                          strcat_limits_exact(&str_data,"\x05\x08");
                                       break;
                                  case '9':  /* ---->  Increment hanging indent by 9  <---- */
                                       if(!(command_type & NESTED_SUBSTITUTION))
                                          strcat_limits_exact(&str_data,"\x05\x09");
                                       break;
                                  case '@':  /* ---->  Miscellaneous substitution  <---- */
                                       if(*(str_data.src + 1) == '%') {
                                          str_data.src += 2;
                                          switch(*str_data.src) {
                                                 case 'a':
                                                 case 'A':  /* ---->  TCZ server IP address  <---- */
                                                      strcat_limits(&str_data,ip_to_text(tcz_server_ip,SITEMASK,buf1));
                                                      break;
                                                 case 'c':
                                                 case 'C':  /* ---->  TCZ compile date/time  <---- */
                                                      strcat_limits(&str_data,filter_spaces(buf1,__DATE__" "__TIME__,0));
                                                      break;
                                                 case 'd':
                                                 case 'D':  /* ---->  Current date  <---- */
                                                      if(1) {
                                                         time_t now;

                                                         gettime(now);
                                                         if(Validchar(player)) now += db[player].data->player.timediff * HOUR;
                                                         strcat_limits(&str_data,date_to_string(now,UNSET_DATE,player,LONGDATEFMT));
						      }
                                                      break;
                                                 case 'e':
                                                 case 'E':  /* ---->  TCZ Admin. E-mail address  <---- */
                                                      strcat_limits(&str_data,tcz_admin_email);
                                                      break;
                                                 case 'f':
                                                 case 'F':  /* ---->  TCZ E-mail forwarding server address  <---- */
                                                      strcat_limits(&str_data,email_forward_name);
                                                      break;
                                                 case 'h':
                                                 case 'H':  /* ---->  TCZ HTML port number  <---- */
                                                      sprintf(buf1,"%ld",htmlport);
                                                      strcat_limits(&str_data,buf1);
                                                      break;
                                                 case 'l':
                                                 case 'L':  /* ---->  TCZ's location  <---- */
                                                      strcat_limits(&str_data,tcz_location);
                                                      break;
                                                 case 'm':
                                                 case 'M':  /* ---->  Short name of TCZ MUD (I.e:  'TCZ')  <---- */
                                                      strcat_limits(&str_data,tcz_short_name);
                                                      break;
                                                 case 'n':
                                                 case 'N':  /* ---->  Full name of TCZ MUD (I.e:  'The Chatting Zone'.)  <---- */
                                                      strcat_limits(&str_data,tcz_full_name);
                                                      break;
                                                 case 'p':
                                                 case 'P':  /* ---->  TCZ Telnet port number  <---- */
                                                      sprintf(buf1,"%ld",telnetport);
                                                      strcat_limits(&str_data,buf1);
                                                      break;
                                                 case 's':
                                                 case 'S':  /* ---->  TCZ server address  <---- */
                                                      strcat_limits(&str_data,tcz_server_name);
                                                      break;
                                                 case 't':
                                                 case 'T':  /* ---->  Current time  <---- */
                                                      if(1) {
                                                         time_t now;

                                                         gettime(now);
                                                         if(Validchar(player)) now += db[player].data->player.timediff * HOUR;
                                                         strcat_limits(&str_data,date_to_string(now,UNSET_DATE,player,TIMEFMT));
			 			      }
                                                      break;
                                                 case 'v':
                                                 case 'V':  /* ---->  TCZ version number  <---- */
                                                      sprintf(querybuf,TCZ_VERSION".%d",TCZ_REVISION);
                                                      strcat_limits(&str_data,querybuf);
                                                      break;
                                                 case 'w':
                                                 case 'W':  /* ---->  TCZ web site address  <---- */
                                                      strcat_limits(&str_data,html_home_url);
                                                      break;
                                                 case 'z':
                                                 case 'Z':  /* ---->  Server system time zone  <---- */
                                                      strcat_limits(&str_data,tcz_timezone);
                                                      break;
					  }
				       }
                                       break;
                                  case '<':  /* ---->  Beginning of On-line Help System link  <---- */
                                       if(!IsHtml(p)) {
                                          if(Validchar(player) && Underline(player)) {
                                             strcat_limits_exact(&str_data,ANSI_UNDERLINE);
                                             textflags |= TXT_UNDERLINE;
					  }
                                          strcat_limits(&str_data,(p && p->edit) ? ".help ":"help ");
				       } else if(!(command_type & NESTED_SUBSTITUTION)) {
                                          for(p2 = buf1, copied = 0, str_data.src++; *(str_data.src) && (*(str_data.src) != '%'); str_data.src++, copied++)
                                              if(copied < 127) *p2++ = *(str_data.src);
                                          *p2 = '\0', p2 = buf1;
                                          if(!strncasecmp(p2,"help ",5)) p2 += 5;
                                             else if(!strncasecmp(p2,"tutorial ",9)) p2 += 9;
                                          sprintf(buf3,"%s<A HREF=\"%sTOPIC=%s&%s\"%s>\016",(html || hardhtml) ? "":"\016",html_server_url(p,0,2,"help"),html_encode(p2,buf2,&copied,127),html_get_preferences(p),(command_type & HTML_ACCESS) ? "":" TARGET=_blank");
                                          strcat_limits_exact(&str_data,buf3);
                                          sprintf(buf3,"%s%s\016</A>%s",(command_type & HTML_ACCESS) ? "":(p && p->edit) ? ".help ":"help ",buf1,(html || hardhtml) ? "":"\016");
                                          strcat_limits_exact(&str_data,buf3);
                                          str_data.src--;
				       }
                                       break;
                                  case '+':  /* ---->  Beginning of tutorial link  <---- */
                                       if(!IsHtml(p)) {
                                          if(Validchar(player) && Underline(player)) {
                                             strcat_limits_exact(&str_data,ANSI_UNDERLINE);
                                             textflags |= TXT_UNDERLINE;
					  }
                                          strcat_limits(&str_data,(p && p->edit) ? ".tutorial ":"tutorial ");
				       } else if(!(command_type & NESTED_SUBSTITUTION)) {
                                          for(p2 = buf1, copied = 0, str_data.src++; *(str_data.src) && (*(str_data.src) != '%'); str_data.src++, copied++)
                                              if(copied < 127) *p2++ = *(str_data.src);
                                          *p2 = '\0', p2 = buf1;
                                          if(!strncasecmp(p2,"help ",5)) p2 += 5;
                                             else if(!strncasecmp(p2,"tutorial ",9)) p2 += 9;
                                          sprintf(buf3,"%s<A HREF=\"%sTOPIC=%s&%s\"%s>\016",(html || hardhtml) ? "":"\016",html_server_url(p,0,2,"tutorial"),html_encode(p2,buf2,&copied,127),html_get_preferences(p),(command_type & HTML_ACCESS) ? "":" TARGET=_blank");
                                          strcat_limits_exact(&str_data,buf3);
                                          sprintf(buf3,"%s%s\016</A>%s",(command_type & HTML_ACCESS) ? "":(p && p->edit) ? ".tutorial ":"tutorial ",buf1,(html || hardhtml) ? "":"\016");
                                          strcat_limits_exact(&str_data,buf3);
                                          str_data.src--;
				       }
                                       break;
                                  case '(':  /* ---->  Beginning of On-line Help System link  <---- */
                                       if(!IsHtml(p)) {
                                          if(*(str_data.src + 1) && (*(str_data.src + 1) == '^')) {
                                             for(; *str_data.src && !((*str_data.src == ' ') || (*str_data.src == '%')); str_data.src++);
                                             for(; *str_data.src && (*str_data.src == ' '); str_data.src++);
                                             str_data.src--;
					  }
                                          if(Validchar(player) && Underline(player)) {
                                             strcat_limits_exact(&str_data,ANSI_UNDERLINE);
                                             textflags |= TXT_UNDERLINE;
					  }
				       } else if(!(command_type & NESTED_SUBSTITUTION)) {
                                          static unsigned char modifier;

                                          if(*(str_data.src + 1) && (*(str_data.src + 1) == '^')) str_data.src++, modifier = 1;
                                             else modifier = 0;
                                          for(p2 = buf1, copied = 0, str_data.src++; *(str_data.src) && (*(str_data.src) != '%'); str_data.src++, copied++)
                                              if(copied < 127) *p2++ = *(str_data.src);
                                          *p2 = '\0', p2 = buf1;
                                          if(!strncasecmp(p2,"help ",5)) p2 += 5;
                                             else if(!strncasecmp(p2,"tutorial ",9)) p2 += 9;
                                                else if(!strncasecmp(p2,"tutor ",6)) p2 += 6;
                                          sprintf(buf3,"%s<A HREF=\"%sTOPIC=%s&%s\"%s>\016",(html || hardhtml) ? "":"\016",html_server_url(p,0,2,"help"),html_encode(p2,buf2,&copied,127),html_get_preferences(p),(command_type & HTML_ACCESS) ? "":" TARGET=_blank");
                                          strcat_limits_exact(&str_data,buf3);

                                          if(modifier) {
                                             for(p2 = buf1; *p2 && (*p2 != ' '); p2++);
                                             for(; *p2 && (*p2 == ' '); p2++);
					  } else p2 = buf1;
                                          sprintf(buf3,"%s\016</A>%s",p2,(html || hardhtml) ? "":"\016");
                                          strcat_limits_exact(&str_data,buf3);
                                          str_data.src--;
				       }
                                       break;
                                  case '-':  /* ---->  Beginning of Tutorial link  <---- */
                                       if(!IsHtml(p)) {
                                          if(Validchar(player) && Underline(player)) {
                                             strcat_limits_exact(&str_data,ANSI_UNDERLINE);
                                             textflags |= TXT_UNDERLINE;
					  }
				       } else if(!(command_type & NESTED_SUBSTITUTION)) {
                                          for(p2 = buf1, copied = 0, str_data.src++; *(str_data.src) && (*(str_data.src) != '%'); str_data.src++, copied++)
                                              if(copied < 127) *p2++ = *(str_data.src);
                                          *p2 = '\0', p2 = buf1;
                                          if(!strncasecmp(p2,"help ",5)) p2 += 5;
                                             else if(!strncasecmp(p2,"tutorial ",9)) p2 += 9;
                                                else if(!strncasecmp(p2,"tutor ",6)) p2 += 6;
                                          sprintf(buf3,"%s<A HREF=\"%sTOPIC=%s&%s\"%s>\016",(html || hardhtml) ? "":"\016",html_server_url(p,0,2,"tutorial"),html_encode(p2,buf2,&copied,127),html_get_preferences(p),(command_type & HTML_ACCESS) ? "":" TARGET=_blank");
                                          strcat_limits_exact(&str_data,buf3);
                                          sprintf(buf3,"%s\016</A>%s",buf1,(html || hardhtml) ? "":"\016");
                                          strcat_limits_exact(&str_data,buf3);
                                          str_data.src--;
				       }
                                       break;
                                  case '$':  /* ---->  Line is a title, which should be underlined with 'set underline on' and in HELP --> HTML conversion  <---- */
				       if(!(command_type & NESTED_SUBSTITUTION)) {
                                          if(IsHtml(p)) {
                                             sprintf(buf3,"%s<FONT SIZE=4>%s",(html || hardhtml) ? "":"\016",(html || hardhtml) ? "":"\016");
                                             strcat_limits_exact(&str_data,buf3);
                                             for(p2 = buf3, copied = 0, str_data.src++; *(str_data.src) && ((*(str_data.src) != '%') || (*(str_data.src + 1) == '%')); str_data.src++, copied++) {
                                                 if(*str_data.src == '%') str_data.src++;
                                                 if(*str_data.src && (copied < 1023)) *p2++ = *(str_data.src);
					     }
                                             *p2 = '\0';
                                             strcat_limits_exact(&str_data,ANSI_UNDERLINE);
                                             strcat_limits(&str_data,buf3);
                                             sprintf(buf3,"%s</FONT>%s",(html || hardhtml) ? "":"\016",(html || hardhtml) ? "":"\016");
                                             strcat_limits_exact(&str_data,buf3);
                                             textflags |= TXT_UNDERLINE;
                                             str_data.src--;
					  } else if(Validchar(player) && Underline(player)) {
                                             strcat_limits_exact(&str_data,ANSI_UNDERLINE);
                                             textflags |= TXT_UNDERLINE;
					  }
				       }
                                       break;
                                  case '~':  /* ---->  '~~~~' underline, which should be ignored with 'set underline on' and in HELP --> HTML conversion  <---- */
                                       if((IsHtml(p) || (Validchar(player) && Underline(player))) && !(command_type & NESTED_SUBSTITUTION))
                                          strcat_limits_char(&str_data,'\x06');
                                       break;
                                  case '>':  /* ---->  End of On-line Help System link  <---- */
                                  case ')':  /* ---->  End of On-line Help System link  <---- */
                                       textflags &= ~TXT_UNDERLINE;
                                       strcat_limits_exact(&str_data,cur_ansi);
                                       if(textflags & TXT_BOLD)    strcat_limits_exact(&str_data,ANSI_LIGHT);
                                       if(textflags & TXT_BLINK)   strcat_limits_exact(&str_data,ANSI_BLINK);
                                       if(textflags & TXT_INVERSE) strcat_limits_exact(&str_data,cur_bg);
                                       break;
                                  case '[':  /* ---->  Start of HTML preformat section  <---- */
                                       if(IsHtml(p) && !(command_type & NESTED_SUBSTITUTION))
                                          strcat_limits_exact(&str_data,(html) ? "<TT>":"\016<TT>\016");
                                       break;
                                  case ']':  /* ---->  End of HTML preformat section  <---- */
                                       if(IsHtml(p) && !(command_type & NESTED_SUBSTITUTION))
                                          strcat_limits_exact(&str_data,(html) ? "</TT>":"\016</TT>\016");
                                       break;
                                  case '|':  /* ---->  HTML filter (Text stripped to next '%|' if using Telnet)  <---- */
                                       if(!IsHtml(p)) {
                                          striptelnet = 1, str_data.src++;
                                          while(*str_data.src && striptelnet) {
                                                for(; *str_data.src && (*str_data.src != '%'); str_data.src++);
                                                if(*str_data.src && *(++str_data.src) && (*str_data.src == '|')) striptelnet = 0;
					  }
				       }
                                       break;
                                  case '^':  /* ---->  Telnet filter (Text stripped to next '%^' if using web browser)  <---- */
                                       if(IsHtml(p)) {
                                          striphtml = 1, str_data.src++;
                                          while(*str_data.src && striphtml) {
                                                for(; *str_data.src && (*str_data.src != '%'); str_data.src++);
                                                if(*str_data.src && *(++str_data.src) && (*str_data.src == '^')) striphtml = 0;
					  }
				       }
                                       break;
                                  case '#':  /* ---->  Automatic formatting/punctuation control  <---- */
                                  default:
                                       *str_data.dest++ = '%';
                                       str_data.length++;
                                       if((*str_data.src != '%') && (str_data.length < MAX_LENGTH)) {
                                          *str_data.dest++ = *str_data.src;
                                          str_data.length++;
				       }
			   }
                           if(p1 && isupper(*str_data.src) && islower(*p1)) *p1 = toupper(*p1);
                           if(*str_data.src) str_data.src++;
		        } else {
                           *str_data.dest++ = '%';
                           str_data.length++;
			}
                        break;
	           default:
                        *str_data.dest++ = *str_data.src;
                        str_data.length++;
                        str_data.src++;
	    }

      if(!subst && (html || hardhtml)) {
         if(!(str_data.length < (MAX_LENGTH - 1))) {
            dest[MAX_LENGTH - 1] = '\016';
            dest[MAX_LENGTH]     = '\0';
	 } else {
            strcat_limits(&str_data,"\016");
            *str_data.dest = '\0';
	 }
      } else *str_data.dest = '\0';

      if(subst) {
         subst->textflags = textflags;
         subst->flags     = 0;
         subst->cur_bg    = (char *) cur_bg;
         if(html)        subst->flags |= SUBST_HTML;
         if(htmltag)     subst->flags |= SUBST_HTMLTAG;
         if(hardhtml)    subst->flags |= SUBST_HARDHTML;
         if(striptelnet) subst->flags |= SUBST_STRIP_TELNET;
         if(striphtml)   subst->flags |= SUBST_STRIP_HTML;
         strcpy(subst->cur_ansi,cur_ansi);
      }
      return(dest);
}

/* ---->  Large substitution (Used where substitution may exceed 3Kb limit)  <---- */
void substitute_large(dbref player,dbref who,const char *str,const char *def_ansi,char *buffer,unsigned char censor)
{
     struct substitution_data subst;
     char   substbuf[TEXT_SIZE];
     struct descriptor_data *p;
     short  loop;
     char   *ptr;

     if(who == NOBODY) {
        for(p = descriptor_list; p && !(IsHtml(p) && (p->player == NOBODY)); p = p->next);
        player = NOTHING;
     } else p = getdsc(who);

     subst.cur_bg = ANSI_IBLACK, subst.textflags = 0, subst.flags = 0;
     strcpy(subst.cur_ansi,def_ansi);
     if(!Blank(str)) {
        command_type |= LARGE_SUBSTITUTION;
        while(*str) {
              ptr = buffer;
              if(wrap_leading > 0) for(loop = 0; loop < wrap_leading; *ptr++ = ' ', loop++);
              for(; *str && (*str != '\n'); *ptr++ = *str++);
              if(*str) for(str++; *str && (*str == '\n'); *ptr++ = *str++);
              *ptr = '\0';

              substitute(player,substbuf,buffer,0,def_ansi,&subst,0);
              if(!Blank(substbuf) && !((ptr = (char *) strchr(substbuf,'\x06')) && !((ptr > substbuf) && (*(ptr - 1) == '\x05')))) {
                 if(censor) bad_language_filter(substbuf,substbuf);
                 output(p,who,0,1,0,"%s",substbuf);
	      }
	}
        command_type &= ~LARGE_SUBSTITUTION;
        output(p,who,(p && (p->terminal_xpos > 0)) ? 0:2,1,0,"");
     }
}
