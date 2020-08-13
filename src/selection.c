/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| SELECTION.C  -  Implements selection and iteration commands ('@case',       |
|                 '@if', '@for', '@foreach', '@while'), linked list           |
|                 processing ('@with') and flow control ('@break',            |
|                 '@breakloop', '@goto', '@return' and '@skip'.)              |
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
| Module originally designed and written by:  J.P.Boggis 15/06/1994.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                        https://github.com/smcvey/tcz                        |
`-----------------------------------------------------------------------------'

*/


#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "object_types.h"
#include "objectlists.h"
#include "flagset.h"
#include "search.h"
#include "fields.h"
#include "match.h"


struct block_data {
       struct   block_data *next;
       unsigned char       type;
};


/* ---->  Log execution time limit overruns  <---- */
void selection_log_execlimit(dbref player,char *buffer)
{
     if(!in_command) {
        if(!(command_type & WARNED)) {
           output(getdsc(player),player,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"Execution time limit of "ANSI_LCYAN"%d second%s"ANSI_LWHITE" exceeded.",command_timelimit,Plural(command_timelimit));
           writelog(EXECUTION_LOG,1,"EXECUTION","Execution time limit of %d second%s exceeded by %s(#%d) (On the command-line) by executing the command '%s'.",command_timelimit,Plural(command_timelimit),getname(player),player,!Blank(current_cmdptr) ? current_cmdptr:"<UNKNOWN>");
	}
     } else if(!(command_type & WARNED)) {
        output(getdsc(player),player,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"Execution time limit of "ANSI_LCYAN"%d second%s"ANSI_LWHITE" exceeded within compound command "ANSI_LYELLOW"%s"ANSI_LWHITE".",command_timelimit,Plural(command_timelimit),unparse_object(player,current_command,0));
        if(Valid(current_command) && (player != Owner(current_command))) {
           char buffer[BUFFER_LEN];

           strcpy(buffer,unparse_object(Owner(current_command),current_command,1));
           output(getdsc(Owner(current_command)),Owner(current_command),0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"Execution time limit of "ANSI_LCYAN"%d second%s"ANSI_LWHITE" exceeded by "ANSI_LYELLOW"%s"ANSI_LWHITE" within compound command "ANSI_LYELLOW"%s"ANSI_LWHITE".",command_timelimit,Plural(command_timelimit),getcname(Owner(current_command),player,1,0),buffer);
	}
        writelog(EXECUTION_LOG,1,"EXECUTION","Execution time limit of %d second%s exceeded by %s(#%d) within compound command %s(#%d) owned by %s(#%d).",command_timelimit,Plural(command_timelimit),getname(player),player,getname(current_command),current_command,getname(Owner(current_command)),Owner(current_command));
     }
     command_type |= WARNED, flow_control |= FLOW_RETURN;
     setreturn(LIMIT_EXCEEDED,COMMAND_FAIL);
}

/* ---->  Match for 'do' keyword  <---- */
unsigned char selection_do_keyword(const char *str)
{
	 if(Blank(str)) return(0);
	 if(*str && (*str == ' ')) {
	    for(; *str && (*str == ' '); str++);
	    if(*str && ((*str == 'd') || (*str == 'D')))
	       if(*(++str) && ((*str == 'o') || (*str == 'O')))
		  if(!*(++str) || (*str == ' ') || (*str == '\n')) return(1);
	 }
	 return(0);
}

/* ---->  Match for '; else' keyword  <---- */
unsigned char selection_else_keyword(const char *str)
{
	 if(Blank(str)) return(0);
	 for(; *str && (*str == ' '); str++);
	 if(*str && (*str == ';')) {
	    for(str++; *str && (*str == ' '); str++);
	    if(*str && ((*str == 'e') || (*str == 'E')))
	       if(*(++str) && ((*str == 'l') || (*str == 'L')))
		  if(*(++str) && ((*str == 's') || (*str == 'S')))
		     if(*(++str) && ((*str == 'e') || (*str == 'E')))
			if(!*(++str) || (*str == ' ')) return(1);
	 }
	 return(0);
}

/* ---->  Skip past 'do' keyword (Returns 1 if multi-lined, otherwise 0)  <---- */
unsigned char selection_skip_do_keyword(char *str)
{
	 if(Blank(str)) return(0);
	 if(!strncasecmp(str,"do",2) && (*(str - 1) == ' ')) str--;

	 do {
	    for(; *str && (*str != ' '); str++);
	    for(; *str && (*str == ' '); str++);
	 } while(*str && !selection_do_keyword(str - 1));

	 if(*str && selection_do_keyword(str - 1)) {
	    for(; *str && (*str != ' '); str++);
	    for(; *str && (*str == ' '); str++);
	    if(!*str) return(1);
	 }
	 return(0);
}

/* ---->  Seek end of block (Marked by '@end' (Or '@else', if ELSE = 1.)  <---- */
char *selection_seek_end(char *str,int *lineno,unsigned char elsepart,unsigned char casepart)
{
     char                *eolptr = str,*start = str,*str2;
     unsigned char       end = 0,type,onstack;
     struct   block_data *stack = NULL,*new;
     short               len;

     if(!str) return(str);
     while(*str && !end) {
           for(; *str && (*str == ' '); str++);

           if(casepart) {
              for(str2 = str; *str2 && (*str2 != ':') && (*str2 != '\n'); str2++);
              if(*str2 && (*str2 == ':')) {
                 for(; *str && (*str != ':') && (*str != '\n'); str++);
                 for(; *str && (*str == ':') && (*str != '\n'); str++);
                 for(; *str && (*str == ' ') && (*str != '\n'); str++);
	      }
	   }

           onstack = 0;
           if(*str && (*str == '@')) {
              str++, type = 0;
              if(*str) switch(*str) {
                 case 'b':
                 case 'B':

                      /* ---->  @begin  <---- */
                      if(((len = strlen(str + 1)) >= 4) && !strncasecmp(str + 1,"egin",4) && (!*(str + 5) || (*(str + 5) == ' ') || (*(str + 5) == '\n'))) {
                         type = FLOW_BEGIN >> 24;
                         onstack = 1;
                      }
                      break;
                 case 'c':
                 case 'C':

                      /* ---->  @case  <---- */
                      if(!strncasecmp(str + 1,"ase ",4)) 
                         type = FLOW_CASE >> 24;
                      break;
                 case 'e':
                 case 'E':

                      /* ---->  @end and @else  <---- */
                      if(((len = strlen(str + 1)) >= 2) && !strncasecmp(str + 1,"nd",2) && (!*(str + 3) || (*(str + 3) == ' ') || (*(str + 3) == '\n'))) type = 1;
                         else if((len >= 3) && !strncasecmp(str + 1,"lse",3) && (!*(str + 4) || (*(str + 4) == ' ') || (*(str + 4) == '\n'))) type = 2;

                      if(type > 0) {
                         if(type == 2) {
                            if(!(stack && ((stack->type << 24) == FLOW_IF)) && elsepart) {

                               /* ---->  '@else' part of '@if' statement  <---- */
                               for(; stack; stack = new) {
                                   new = stack->next;
                                   FREENULL(stack);
			       }
                               return(eolptr);
			    } else type = 0;
			 } else if(stack) {

                            /* ---->  '@end' of statement on stack  <---- */
                            new = stack, stack = stack->next;
                            FREENULL(new);
			 } else return(eolptr);  /*  '@end' of current block  */
		      }
                      break;
                 case 'f':
                 case 'F':

                      /* ---->  @for and @foreach  <---- */
                      if(!strncasecmp(str + 1,"or ",3)) type = FLOW_FOR >> 24;
                         else if(!strncasecmp(str + 1,"oreach ",7)) type = FLOW_FOREACH >> 24;
                      break;
                 case 'i':
                 case 'I':

                      /* ---->  @if  <---- */
                      if(!strncasecmp(str + 1,"f ",2))
                         type = FLOW_IF >> 24;
                      break;
                 case 'w':
                 case 'W':

                      /* ---->  @with and @while  <---- */
                      if(!strncasecmp(str + 1,"ith ",4)) type = FLOW_WITH >> 24;
                         else if(!strncasecmp(str + 1,"hile ",5)) type = FLOW_WHILE >> 24;
                      break;
	      }

              if(type > 0) {
                 if(!onstack) {
                    for(; *str && (*str != '\n'); str++);
                    for(str--; (str > start) && (*str == ' ') && (*(str - 1) != '\n'); str--);
                    for(; (str > start) && isalpha(*str) && (*(str - 1) != '\n'); str--);
		 }

                 if(onstack || ((str > start) && (*str == ' ') && (strlen(str) >= 3) && !strncasecmp(str," do",3) && (!*(str + 3) || (*(str + 3) == ' ') || (*(str + 3) == '\n')))) {

                    /* ---->  Add statement to stack  <---- */
                    MALLOC(new,struct block_data);
                    new->type = type;
                    new->next = stack;
                    stack     = new;
		 }
	      }
	   }
           for(; *str && (*str != '\n'); str++);
           eolptr = str;
           if(*str && (*str == '\n')) (*lineno)++, str++;
     }

     /* ---->  Free block stack  <---- */
     for(; stack; stack = new) {
         new = stack->next;
         FREENULL(stack);
     }
     return(str);
}

/* ---->  {J.P.Boggis 14/07/2000}  @begin  <---- */
void selection_begin(CONTEXT)
{
     if(in_command) {
        if(command_boolean == COMMAND_INIT) setreturn(OK,COMMAND_SUCC);
           else if(!command_result) setreturn((command_boolean == COMMAND_SUCC) ? OK:ERROR,command_boolean);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@begin"ANSI_LGREEN"' can only be used from within a compound command.");
}

/* ---->  Break out of present loop or all loops (If 'all' is specified)  <---- */
void selection_breakloop(CONTEXT)
{
     if(!Blank(params)) {
        if(!string_prefix("all",params)) {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please use either '"ANSI_LWHITE"@breakloop"ANSI_LGREEN"' (To break out of the current loop) or '"ANSI_LWHITE"@breakloop all"ANSI_LGREEN"' (To break out of all loops.)");
           setreturn(ERROR,COMMAND_FAIL);
           return;
	} else flow_control |= FLOW_BREAKLOOP_ALL;
     } else flow_control |= FLOW_BREAKLOOP;
     if(command_boolean == COMMAND_INIT) setreturn(OK,COMMAND_SUCC);
        else if(!command_result)
           setreturn((command_boolean == COMMAND_SUCC) ? OK:ERROR,command_boolean);
}

/* ---->  @case[first|random] <VALUE> do                <---- */
/*                [<VALUE>[,<VALUE>][...]:<COMMAND>]          */
/*                [<VALUE>[,<VALUE>][...]:@begin              */
/*                                        <COMMANDS>          */
/*                                        ...                 */
/*                                        @end                */
/*                [DEFAULT:<COMMAND>]                         */
/*        @end                                                */
/*                                                            */
/*     <VALUE>:  Description:                                 */
/*     ~~~~~~~~  ~~~~~~~~~~~~                                 */
/*        name:  Single exact match.                          */
/* one,two,...:  Multiple exact matches.                      */
/*    A-Z,1..9:  Exact range match.                           */
/*        A->Z:  Prefix range match.                          */
/*      *name*:  Wildcard match.                              */
/*     DEFAULT:  Default if no match found.                   */
/*                                                            */
/*          $6:  Matched case <VALUE>/first value of range.   */
/*          $7:  Second value of range.                       */
/*                                                            */
/*  {J.P.Boggis 14/07/2000}                                   */
/*  (VAL1:  0 = Non-random, 1 = First, 2 = Random.)           */
void selection_case(CONTEXT)
{
     const  char *rangevalue = NULL,*rangecommands = NULL, *rangevalue2 = NULL;
     int    rangeblock = 0, defaultblock = 0, exactblock = 0, wildblock = 0;
     int    rangeline  = 0, defaultline  = 0, exactline  = 0, wildline  = 0;
     int    block = 0, matched = 0, command = 0, prefix = 0, offset = 0;
     const  char *defaultvalue = NULL,*defaultcommands = NULL;
     const  char *exactvalue   = NULL,*exactcommands   = NULL;
     const  char *wildvalue    = NULL,*wildcommands    = NULL;
     char   *matchvalues,*endptr,*cendptr,*ptr,*ptr2;
     const  char *casevalue,*commands,*value,*range;
     int    endline = current_line_number + 1;
     int    startline = current_line_number;
     int    cendline,wildcard,which;
     char   matchbuffer[BUFFER_LEN];
     dbref  cached_pid;
     char   endchar;

     setreturn(ERROR,COMMAND_FAIL);
     cached_pid = Owner(player);

     if(in_command) {

        /* ---->  <VALUE>  <---- */
	for(; *params && (*params == ' '); params++);
        if(strcasecmp(params,"do") && strncasecmp(params,"do ",3)) {
    	   for(casevalue = params; *params && !selection_do_keyword(params); params++);
	} else casevalue = "";

        if(Blank(params)) {
	   output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LYELLOW""ANSI_UNDERLINE"do"ANSI_LGREEN"' keyword omitted.");
	   return;
	}
        *params++ = '\0';

        /* ---->  Text after 'do' keyword?  <---- */
	for(; *params && (*params != ' '); params++);
	for(; *params && (*params == ' '); params++);
	if(!Blank(params)) {
	   output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, case match values must be specified on separated lines, "ANSI_LWHITE"NOT"ANSI_LGREEN" directly after the'"ANSI_LYELLOW""ANSI_UNDERLINE"do"ANSI_LGREEN"' keyword.");
	   return;
	}

	/* ---->  Case match values  <---- */
        endptr  = selection_seek_end(matchvalues = command_textptr,&endline,0,1);
	endchar = *endptr, *endptr = '\0', endline--;

        if(!Blank(matchvalues)) {
           strcpy(matchbuffer,matchvalues);
           ptr = (char *) matchbuffer;

           while(*ptr && !matched) {

                 /* ---->  Case match values  <---- */
                 command = 0, wildcard = 0, which = 0, prefix = 0;
                 while(*ptr && !matched && !command) {
                       for(; *ptr && (*ptr == ' '); ptr++);
                       for(value = ptr; *ptr && (*ptr != ',') && (*ptr != ':'); ptr++);
		       if(*ptr && ((*ptr == ',') || (*ptr == ':'))) {
                          if(*ptr == ':') command = 1;
                          *ptr++ = '\0';
                          while (*value == '\n' && (value + 1)) value++;
                          filter_spaces((char *) value,(char *) value,0);

                          /* ---->  Single value or range of values?  <---- */
                          if(!Blank(value)) {

                             /* ---->  Wildcards?  <---- */
                             for(ptr2 = (char *) value; *ptr2 && (*ptr2 != '*') && (*ptr2 != '?'); ptr2++);
                             if(!*ptr2) {

				/* ---->  No wildcards:  Range of values?  <---- */
				for(ptr2 = (char *) (value + 1), range = NULL; !Blank(ptr2); ptr2 = (ptr2) ? (ptr2 + 1):NULL) {
                                    if(((*ptr2 == '-') && ((*(ptr2 + 1) != '>') || ((*(ptr2 + 1) == '>') && (prefix = 1)))) || ((*ptr2 == '.') && (*(ptr2 + 1) == '.'))) {
                                       if(*ptr2 == '-') {
                                          *ptr2++ = '\0';
                                          if(*ptr2 == '>') ptr2++;
				       } else if((*ptr2 == '.') && (*(ptr2 + 1) == '.')) {
                                          *ptr2++ = '\0';
                                          ptr2++;
				       }

				       range = ptr2;
                                       ptr2  = NULL;
				    }
				}
			     } else wildcard = 1;

                             /* ---->  Match  <---- */
			     if(!wildcard) {
			        if(!range) {
                                   if((!defaultvalue || ((val1 == 2) && ((lrand48() % 100) < 50))) && !strcasecmp("default",value)) {

				      /* ---->  Default value  <---- */
				      defaultvalue    = value;
                                      defaultline     = startline + offset + 1;
                                      which           = 0;
				   } else if((!exactvalue || ((val1 == 2) && ((lrand48() % 100) < 50))) && !strcasecmp(value,casevalue)) {

				      /* ---->  Exact value  <---- */
				      exactvalue    = value;
                                      exactline     = startline + offset + 1;
                                      which         = 1;
				      if(val1 == 1) matched = 1;
				   }
				} else {

                                   /* ---->  Range of values  <---- */
                                   if((!rangevalue || ((val1 == 2) && ((lrand48() % 100) < 50))) && match_range(casevalue,value,range,prefix)) {
			   	      rangevalue  = value;
                                      rangevalue2 = range;
                                      rangeline   = startline + offset + 1;
                                      which       = 2;
				      if(val1 == 1) matched = 1;
				   }
				}
			     } else {

				/* ---->  Wildcard match  <---- */
				if((!wildvalue || ((val1 == 2) && ((lrand48() % 100) < 50))) && match_wildcard(value,casevalue)) {
				   wildvalue    = value;
				   wildline     = startline + offset + 1;
				   which        = 3;
				   if(val1 == 1) matched = 1;
				}
			     }
			  }
		       }
		 }

                 /* ---->  Command to execute  <---- */
                 for(; *ptr && (*ptr == ' '); ptr++);
                 if(!strncasecmp(ptr,"@begin",6) && (!*ptr || (*(ptr + 6) == ' ') || (*(ptr + 6) == '\n'))) {

                    /* ---->  Get block command (Starting with '@begin')  <---- */
                    for(; *ptr && (*ptr != '\n'); ptr++);
                    for(; *ptr && (*ptr == '\n'); offset++, ptr++);
                    for(; *ptr && (*ptr == ' ');  ptr++);
                    cendptr  = selection_seek_end((char *) (commands = ptr),&cendline,0,0);
                    ptr = (*cendptr) ? cendptr + 1:cendptr;
         	    *cendptr = '\0';
                    offset += strcnt(commands,'\n');

                    /* ---->  Skip past '@end'  <---- */
                    for(; *ptr && (*ptr == ' '); ptr++);
                    if(!strncasecmp(ptr,"@end",4) && (!*ptr || (*(ptr + 4) == ' ') || (*(ptr + 4) == '\n'))) {
                       for(; *ptr && (*ptr != '\n'); ptr++);
                       for(; *ptr && (*ptr == '\n'); offset++, ptr++);
		    }
                    block = 1;
		 } else {

                    /* ---->  Get single-lined command  <---- */
                    for(commands = ptr; *ptr && (*ptr != '\n'); ptr++);
                    for(ptr2 = (ptr - 1); (ptr2 >= commands) && (*ptr2 == ' '); ptr2--) *ptr2 = '\0';
                    if(*ptr) {
                       *ptr++ = '\0';
                       offset++;
		    }
                    for(; *ptr && (*ptr == '\n'); offset++, ptr++);
                    block = 0;
		 }

                 /* ---->  Set command(s) to execute  <---- */
                 switch(which) {
                        case 1:
			     exactcommands = commands;
                             exactblock    = block;
                             break;
                        case 2:
			     rangecommands = commands;
                             rangeblock    = block;
                             break;
                        case 3:
			     wildcommands = commands;
                             wildblock    = block;
                             break;
                        default:
			     defaultcommands = commands;
                             defaultblock    = block;
                             break;
		 }
	   }

           /* ---->  Execute command of appropriate matched case value  <---- */
           *command_item2 = '\0';
           if(val1 == 2) {

              /* ---->  Pick random match from matched case values  <---- */
              matched = 0;
              if(exactvalue || rangevalue || wildvalue) {
		 while(!matched) {
		       switch(lrand48() % 3) {
			      case 0:

				   /* ---->  Pick exact match  <---- */
				   matched = 1;
				   break;
			      case 1:

				   /* ---->  Pick range match  <---- */
				   if(rangevalue) {
				      exactline     = rangeline;
				      exactblock    = rangeblock;
				      exactvalue    = rangevalue;
				      exactcommands = rangecommands;
				      strcpy(command_item2,String(rangevalue2));
				      matched = 1;
				   }
				   break;
			      case 2:

				   /* ---->  Pick wildcard match  <---- */
				   if(wildvalue) {
				      exactline     = wildline;
				      exactblock    = wildblock;
				      exactvalue    = wildvalue;
				      exactcommands = wildcommands;
				      matched = 1;
				   }
				   break;
			      default:
				   matched = 0;
				   break;
		       }
		 }
	      } else {

		 /* ---->  Default match  <---- */
		 exactline     = defaultline;
		 exactblock    = defaultblock;
		 exactvalue    = defaultvalue;
		 exactcommands = defaultcommands;
	      }
	   } else if(!exactvalue) {

              /* ---->  Pick most appropriate match from matched case values  <---- */
              if(!rangevalue) {
                 if(!wildvalue) {
                    if(defaultvalue) {
                       exactline     = defaultline;
                       exactblock    = defaultblock;
                       exactvalue    = defaultvalue;
                       exactcommands = defaultcommands;
		    }
		 } else {
                    exactline     = wildline;
                    exactblock    = wildblock;
                    exactvalue    = wildvalue;
                    exactcommands = wildcommands;
		 }
	      } else {
                 exactline     = rangeline;
                 exactblock    = rangeblock;
		 exactvalue    = rangevalue;
		 exactcommands = rangecommands;
                 strcpy(command_item2,String(rangevalue2));
	      }
	   }

           /* ---->  Execute command  <---- */
           if(exactcommands) {
              current_line_number = exactline;
              strcpy(command_item,String(exactvalue));
              flow_control = (flow_control & ~(FLOW_DATA1_MASK|FLOW_DATA2_MASK))|FLOW_CASE|FLOW_COMMAND;
              if(exactblock) command_execute(player,NOTHING,exactcommands,0);
                 else command_sub_execute(player,(char *) exactcommands,0,1);
              db[player].owner = cached_pid;
	   }

           if(!(flow_control & FLOW_GOTO_LITERAL)) {
              command_textptr     = endptr;
              *endptr             = endchar;
              current_line_number = endline;
           } else *endptr = endchar;
 	   flow_control &= ~(FLOW_DATA1_MASK|FLOW_DATA2_MASK|FLOW_BREAKLOOP);
        } else {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, case match values must be specified.");
           *endptr = endchar;
	}
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"%s"ANSI_LGREEN"' can only be used from within a compound command.",StringDefault(arg0,"@case"));
     setreturn(ERROR,COMMAND_FAIL);
}

/* ---->  Prevent abort fuse from aborting following command  <---- */
void selection_continue(CONTEXT)
{
     if(in_command) {
        command_type &= ~FUSE_ABORT;
        setreturn(OK,COMMAND_SUCC);
     } else {
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@continue"ANSI_LGREEN"' can only be used from within a compound command.");
        setreturn(ERROR,COMMAND_FAIL);
     }
}

/* ---->  {J.P.Boggis 14/07/2000}  @else [<CONDITION>]  <---- */
void selection_else(CONTEXT)
{
     if(in_command) {
        if(command_boolean == COMMAND_INIT) setreturn(OK,COMMAND_SUCC);
           else if(!command_result) setreturn((command_boolean == COMMAND_SUCC) ? OK:ERROR,command_boolean);
     } else {
	output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@else"ANSI_LGREEN"' can only be used from within a compound command.");
	setreturn(ERROR,COMMAND_FAIL);
     }
}

/* ---->  {J.P.Boggis 14/07/2000}  @end  <---- */
void selection_end(CONTEXT)
{
     if(in_command) {
        if(command_boolean == COMMAND_INIT) setreturn(OK,COMMAND_SUCC);
           else if(!command_result) setreturn((command_boolean == COMMAND_SUCC) ? OK:ERROR,command_boolean);
     } else {
	output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@end"ANSI_LGREEN"' can only be used from within a compound command.");
	setreturn(ERROR,COMMAND_FAIL);
     }
}

/* ---->  @if [true|false] <CONDITION> do [<COMMAND1>[; else <COMMAND2>]]  <---- */
/*            [<COMMANDS1>]                                                      */
/*        [@else [<CONDITION> do]]                                               */
/*            [<COMMANDS2>]                                                      */
/*        [@else ...]                                                            */
/*        [@end]                                                                 */
void selection_if(CONTEXT)
{
     char          *condition,*command,*endptr,*tmp,*elsecommand = NULL;
     unsigned char boolean = 1,finished = 0,block = 0;
     int           endline = current_line_number + 1;
     dbref         cached_pid;
     short         brackets;
     char          endchar;

     /* ---->  Optional true/false specified?  <---- */
     setreturn(ERROR,COMMAND_FAIL);
     for(; *params && (*params == ' '); params++);
     if(!strncasecmp(params,"true ",5)) params += 5;
        else if(!strncasecmp(params,"false ",6)) params += 6, boolean = 0;

     /* ---->  <CONDITION> command  <---- */
     for(; *params && (*params == ' '); params++);
     if(!strncasecmp(params,"do ",3) || !strcasecmp(params,"do")) {
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify "ANSI_LYELLOW""ANSI_UNDERLINE"<CONDITION>"ANSI_LGREEN" command.");
        return;
     }
     for(condition = params; *params && !selection_do_keyword(params); params++);
     if(Blank(params)) {
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LYELLOW""ANSI_UNDERLINE"do"ANSI_LGREEN"' keyword omitted.");
        return;
     }
     *params++ = '\0';
     for(; *params && (*params == ' '); params++);
     for(; *params && (*params != ' '); params++);
     for(; *params && (*params == ' '); params++);

     if(Blank(params)) {
        if(in_command) {
           endptr  = selection_seek_end(command = command_textptr,&endline,1,0);
           endchar = *endptr, *endptr = '\0', endline--, block = 1;
	} else {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the multiple line format of '"ANSI_LWHITE"@if"ANSI_LGREEN"' can only be used from within a compound command.");
           return;
	}
     } else {

        /* ---->  Get command to execute if condition is met  <---- */
        command = params;
        if(strncasecmp(params,"@if ",4)) {
           while(*params && !elsecommand)
                 switch(*params) {
                        case '{':
                             if(*(params - 1) != '\\') {
                                for(brackets = 1, params++; *params && brackets; params++)
                                    switch(*params) {
                                           case '{':
                                                if(*(params - 1) != '\\') brackets++;
                                                break;
                                           case '}':
                                                if(*(params - 1) != '\\') brackets--;
                                                break;
                                           default:
                                                break;
				    }
			     } else params++;
                             break;
                        case ' ':
                        case ';':
                             if(selection_else_keyword(params)) {
                                elsecommand = params + 1;
                                break;
			     }
                        default:
                             params++;
		 }
	} else for(; *params; params++);
        *params++ = '\0';

        /* ---->  Get command to execute if condition isn't met (ELSE part)  <---- */
        if(elsecommand) {
           for(; *params && (*params == ' '); params++);
           if(*params && (*params == ';')) for(params++; *params && (*params == ' '); params++);
           for(; *params && (*params != ' '); params++);
           for(; *params && (*params == ' '); params++);
           elsecommand = params;
	}
     }

     /* ---->  Execute <CONDITION> command and determine result  <---- */
     while(!finished) {
           cached_pid       =  Owner(player);
           flow_control     =  (flow_control & ~(FLOW_DATA1_MASK|FLOW_DATA2_MASK))|FLOW_IF|FLOW_CONDITION;
           command_type    |=  TEST_SUBST;
           command_sub_execute(player,condition,0,1);
           command_type    &= ~TEST_SUBST;
           db[player].owner =  cached_pid;
           if((!boolean && (!strcmp("0",command_result) || !strcasecmp(UNSET_VALUE,command_result) || !strcasecmp(ERROR,command_result))) || (boolean && !(!strcmp("0",command_result) || !strcasecmp(UNSET_VALUE,command_result) || !strcasecmp(ERROR,command_result)))) {
              if(!Blank(command)) {
                 flow_control = (flow_control & ~(FLOW_DATA1_MASK|FLOW_DATA2_MASK))|FLOW_IF|FLOW_COMMAND;
                 if(block) command_execute(player,NOTHING,command,1);
                    else command_sub_execute(player,command,0,1);
                 db[player].owner = cached_pid;
	      }
              finished = 1;
	   } else if(block) {

              /* ---->  Block '@else' part  <---- */
              *endptr = endchar;
              if((endchar != '\n') && (command == endptr)) endptr--;
              command_textptr = endptr, current_line_number = endline;
              if(*endptr) {
                 for(elsecommand = endptr + 1; *elsecommand && (*elsecommand == ' '); elsecommand++);
                 if(*elsecommand && (*elsecommand == '@')) {
                    if((strlen(elsecommand + 1) >= 4) && !strncasecmp(elsecommand + 1,"else",4) && (!*(elsecommand + 5) || (*(elsecommand + 5) == ' ') || (*(elsecommand + 5) == '\n'))) {

                       /* ---->  Trace sequence  <---- */
                       if(output_trace(player,current_command,0,0,0,NULL)) {
                          if((tmp = strchr(elsecommand,'\n')) && *tmp) *tmp = '\0';
                          output_trace(player,current_command,0,1,7,ANSI_DCYAN"["ANSI_LYELLOW""ANSI_UNDERLINE"%03d"ANSI_DCYAN"]  "ANSI_LGREEN"%s  "ANSI_LMAGENTA"("ANSI_UNDERLINE"EXEC"ANSI_LMAGENTA")",current_line_number + 1,elsecommand);
                          if(tmp) *tmp = '\n';
		       }

                       /* ---->  '@else' part  <---- */
                       for(elsecommand += 5; *elsecommand && (*elsecommand == ' '); elsecommand++);
                       if(*elsecommand && (*elsecommand != '\n')) {

                          /* ---->  '@else' with <CONDITION>  <---- */
                          if(!strncasecmp(elsecommand,"true ",5)) elsecommand += 5, boolean = 1;
                             else if(!strncasecmp(elsecommand,"false ",6)) elsecommand += 6, boolean = 0;
                                else boolean = 1;

                          for(; *elsecommand && (*elsecommand == ' '); elsecommand++);
                          for(condition = elsecommand; *elsecommand && (*elsecommand != '\n') && !selection_do_keyword(elsecommand); elsecommand++);
                          if(*elsecommand != '\n') {
                             for(*elsecommand++ = '\0'; *elsecommand && (*elsecommand != '\n'); elsecommand++);
			  } else *elsecommand = '\0';
                          current_line_number++;
                          endline = current_line_number + 1;
                          endptr  = selection_seek_end(command = (elsecommand + 1),&endline,1,0);
                          endchar = *endptr, *endptr = '\0', endline--;
		       } else if(*elsecommand) {

                          /* ---->  '@else' with no <CONDITION>  <---- */
                          current_line_number++;
                          endline = current_line_number + 1;
                          endptr  = selection_seek_end(command = (elsecommand + 1),&endline,0,0);
                          endchar = *endptr, *endptr = '\0', endline--;
                          if(!Blank(command)) {
                             flow_control = (flow_control & ~(FLOW_DATA1_MASK|FLOW_DATA2_MASK))|FLOW_IF|FLOW_ELSE;
                             command_execute(player,NOTHING,command,1);
                             db[player].owner = cached_pid;
			  }
                          finished = 1;
		       } else finished = 1, block = 0;
		    } else finished = 1;
		 }
	      } else finished = 1;
	   } else if(elsecommand) {

              /* ---->  '; else' part  <---- */
              flow_control = (flow_control & ~(FLOW_DATA1_MASK|FLOW_DATA2_MASK))|FLOW_IF|FLOW_ELSE;
              command_sub_execute(player,elsecommand,0,1);
              db[player].owner = cached_pid;
              finished = 1;
	   } else {
              setreturn(ERROR,COMMAND_FAIL);
              finished = 1;
	   }
     }

     /* ---->  Skip further '@else' parts  <---- */
     if(block) {
        if(!(flow_control & FLOW_GOTO_LITERAL)) {
           finished = 0;
           while(!finished) {
                 *endptr = endchar;
                 if((endchar != '\n') && (command == endptr)) endptr--;
                 command_textptr = endptr, current_line_number = endline;
                 if(*endptr) {
                    for(elsecommand = endptr + 1; *elsecommand && (*elsecommand == ' '); elsecommand++);
                    if(*elsecommand && (*elsecommand == '@')) {
                       if((strlen(elsecommand + 1) >= 4) && !strncasecmp(elsecommand + 1,"else",4) && *(elsecommand + 5) && ((*(elsecommand + 5) == ' ') || (*(elsecommand + 5) == '\n'))) {
                          for(elsecommand += 5; *elsecommand && (*elsecommand == ' '); elsecommand++);
                          for(condition = elsecommand; *elsecommand && (*elsecommand != '\n'); elsecommand++);
                          endline  = current_line_number + 2;
                          endptr   = selection_seek_end(command = (elsecommand + 1),&endline,1,0);
                          endchar  = *endptr, *endptr = '\0', endline--;
                          finished = 0;
		       } else finished = 1; 
		    }
		 } else finished = 1;
	   }
	} else *endptr = endchar;
     }
     flow_control = (flow_control & ~(FLOW_DATA1_MASK|FLOW_DATA2_MASK));
}

/* ---->  @for <FROM VALUE> to <TO VALUE> [step <STEP VALUE>] do [<COMMAND>]  <---- */
/*             [<COMMANDS>]                                                         */
/*        [@end]                                                                    */
void selection_for(CONTEXT)
{
     int              startline = current_line_number,endline = current_line_number + 1;
     char             *cached_command_textptr = command_textptr;
     int              value,tovalue,loop,increment = 1;
     unsigned char    finished = 0,block = 0;
     char             *command,*endptr,*ptr;
     struct   timeval currenttime;
     dbref            cached_pid;
     char             endchar;

     setreturn(ERROR,COMMAND_FAIL);
     loopno = 0, noloops = 0;
     if(!in_command && !Level4(player) && !Experienced(player)) {
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@for"ANSI_LGREEN"' can only be used from within a compound command (Unless you're an Experienced Builder or above.)");
        return;
     }

     /* ---->  <FROM VALUE>  <---- */
     for(; *params && (*params == ' '); params++);
     for(ptr = scratch_return_string; *params && (*params != ' '); *ptr++ = *params, params++);
     *ptr = '\0';
     if(BlankContent(scratch_return_string)) {
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify "ANSI_LYELLOW""ANSI_UNDERLINE"<FROM VALUE>"ANSI_LGREEN".");
        return;
     }
     value = atol(scratch_return_string);
     for(; *params && (*params == ' '); params++);

     /* ---->  'to' keyword  <---- */
     if(!strcasecmp(params,"to") || !strcasecmp(params,"downto")) {
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify "ANSI_LYELLOW""ANSI_UNDERLINE"<TO VALUE>"ANSI_LGREEN".");
        return;
     }
     if(strncasecmp(params,"to ",3) && strncasecmp(params,"downto ",7)) {
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please use the '"ANSI_LYELLOW""ANSI_UNDERLINE"to"ANSI_LGREEN"' keyword to separate "ANSI_LWHITE"<FROM VALUE>"ANSI_LGREEN" and "ANSI_LWHITE"<TO VALUE>"ANSI_LGREEN".");
        return;
     }

     /* ---->  <TO VALUE>  <---- */
     while(*params && (*params != ' ')) params++;
     while(*params && (*params == ' ')) params++;
     for(ptr = scratch_return_string; *params && (*params != ' '); *ptr++ = *params, params++);
     *ptr = '\0';
     if(BlankContent(scratch_return_string)) {
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify "ANSI_LYELLOW""ANSI_UNDERLINE"<TO VALUE>"ANSI_LGREEN".");
        return;
     }
     if((tovalue = atol(scratch_return_string)) < value) increment = -1;
     for(; *params && (*params == ' '); params++);

     /* ---->  Optional 'step' keyword  <---- */
     if(!strncasecmp(params,"step ",5)) {

        /* ---->  <STEP VALUE>  <---- */
        for(params += 4; *params && (*params == ' '); params++);
        for(ptr = scratch_return_string; *params && (*params != ' '); *ptr++ = *params, params++);
        *ptr = '\0';
        if(BlankContent(scratch_return_string)) {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify "ANSI_LYELLOW""ANSI_UNDERLINE"<STEP VALUE>"ANSI_LGREEN".");
           return;
	}
        loop = atol(scratch_return_string);
        if(loop < 0) loop = 0 - loop;
        if(loop == 0) {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, "ANSI_LWHITE"<STEP VALUE>"ANSI_LGREEN" can't be zero.");
           return;
	}
        if(loop) increment = (increment > 0) ? loop:0 - loop;
        for(; *params && (*params == ' '); params++);
     }
     loop = 0;

     /* ---->  'do' keyword  <---- */
     if(!selection_do_keyword(params - 1)) {
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LYELLOW""ANSI_UNDERLINE"do"ANSI_LGREEN"' keyword omitted.");
        return;
     }

     /* ---->  Command to execute  <---- */
     for(params += 2; *params && (*params == ' '); params++);
     if(Blank(params)) {
        if(in_command) {
           endptr  = selection_seek_end(command = command_textptr,&endline,0,0);
           endchar = *endptr, *endptr = '\0', endline--;
           if(Blank(command)) {
              *endptr = endchar;
              return;
	   }
           block = 1;
	} else {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the multiple line format of '"ANSI_LWHITE"@for"ANSI_LGREEN"' can only be used from within a compound command.");
           return;
	}
     } else command = params;

     setreturn(OK,COMMAND_SUCC);
     cached_pid    =  Owner(player);
     flow_control &= ~FLOW_BREAKLOOP_ALL;
     while(!finished && ((flow_control & FLOW_MASK_LOOP) == FLOW_NORMAL)) {
           if(((increment > 0) && (value > tovalue)) || ((increment < 0) && (value < tovalue))) finished = 1;
              else {
                 setreturn(ERROR,COMMAND_FAIL);
                 loop++, loopno  = value, noloops = loop;
                 current_line_number = startline;
                 db[player].owner    = cached_pid;
                 command_textptr     = cached_command_textptr;
                 flow_control        = (flow_control & ~(FLOW_DATA1_MASK|FLOW_DATA2_MASK))|FLOW_FOR|FLOW_COMMAND;
                 if(block) command_execute(player,NOTHING,command,1);
		    else command_sub_execute(player,command,0,1);
                 db[player].owner = cached_pid;

                 gettimeofday(&currenttime,NULL);
                 if(usec_difference(currenttime,command_time) > (command_timelimit * 1000000)) {
                    flow_control &= ~(FLOW_DATA1_MASK|FLOW_DATA2_MASK|FLOW_BREAKLOOP);
                    selection_log_execlimit(player,command);
                    if(block) {
                       command_textptr = endptr, *endptr = endchar;
                       current_line_number = endline;
		    }
                    return;
		 }
                 value += increment;
	      }
     }

     if(block) {
        if(!(flow_control & FLOW_GOTO_LITERAL)) {
           command_textptr = endptr, *endptr = endchar;
           current_line_number = endline;
	} else *endptr = endchar;
     }
     flow_control &= ~(FLOW_DATA1_MASK|FLOW_DATA2_MASK|FLOW_BREAKLOOP);
}

/* ---->  @foreach <GRP/RANGE> "<SEPARATOR>" "<LIST>" do [<COMMAND>]  <---- */
/*                 [<COMMANDS>]                                             */
/*        [@end]                                                            */
void selection_foreach(CONTEXT)
{
     int                    startline = current_line_number,endline = current_line_number + 1,loop = 0;
     struct   array_element *start = NULL,*tail = NULL,*new;
     char                   *cached_command_textptr = command_textptr;
     char                   *command,*endptr,*ptr,*item;
     unsigned char          block = 0,error = 0;
     struct   timeval       currenttime;
     dbref                  cached_pid;
     char                   endchar;
     struct   arg_data      arg;

     /* ---->  <COMMAND>  <---- */
     setreturn(ERROR,COMMAND_FAIL);
     params = (char *) parse_grouprange(player,params,ALL,1);
     if(!strncasecmp(params,"do ",3) || !strcasecmp(params,"do")) {
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the "ANSI_LYELLOW""ANSI_UNDERLINE"<SEPARATOR>"ANSI_LGREEN" and "ANSI_LYELLOW""ANSI_UNDERLINE"<LIST>"ANSI_LGREEN" to process.");
        return;
     }

     for(ptr = params + strlen(params) - 1; (ptr > params) && (*ptr == ' '); ptr--);
     for(; (ptr > params) && (*ptr != ' '); ptr--);
     if((ptr > params) && selection_do_keyword(ptr)) {
        for(; (ptr > params) && (*ptr == ' '); ptr--);
        ptr++, *ptr = '\0';
     } else {
        for(ptr = params; *ptr && !selection_do_keyword(ptr); ptr++);
        if(!*ptr) {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LYELLOW""ANSI_UNDERLINE"do"ANSI_LGREEN"' keyword omitted.");
           return;
	}
        *ptr++ = '\0';

        for(; *ptr && (*ptr == ' '); ptr++);
        for(; *ptr && (*ptr != ' '); ptr++);
        for(; *ptr && (*ptr == ' '); ptr++);
     }

     if(Blank(ptr)) {
        if(in_command) {
           endptr  = selection_seek_end(command = command_textptr,&endline,0,0);
           endchar = *endptr, *endptr = '\0', endline--;
           if(Blank(command)) {
              *endptr = endchar;
              return;
	   }
           block = 1;
	} else {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the multiple line format of '"ANSI_LWHITE"@foreach"ANSI_LGREEN"' can only be used from within a compound command.");
           return;
	}
     } else command = ptr;

     /* ---->  "<SEPARATOR>" and "<LIST>"  <---- */
     unparse_parameters(params,2,&arg,0);
     if(arg.count >= 2) {
        if(Blank(arg.text[0])) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the list "ANSI_LYELLOW""ANSI_UNDERLINE"<SEPARATOR>"ANSI_LGREEN"."), error = 1;
	   else if(Blank(arg.text[1])) error = 1;
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the "ANSI_LYELLOW""ANSI_UNDERLINE"<SEPARATOR>"ANSI_LGREEN" and "ANSI_LYELLOW""ANSI_UNDERLINE"<LIST>"ANSI_LGREEN" to process."), error = 1;

     if(error) {
        if(block) {
           command_textptr = endptr, *endptr = endchar;
           current_line_number = endline;
	}
        return;
     }

     /* ---->  Construct array of pointers to list items  <---- */
     ptr = arg.text[1];
     while(*ptr) {
           for(item = ptr; *ptr && strncasecmp(ptr,arg.text[0],arg.len[0]); ptr++);
           if(*ptr && !strncasecmp(ptr,arg.text[0],arg.len[0]))
              *ptr = '\0', ptr += arg.len[0];

           MALLOC(new,struct array_element);
           new->index = NULL;
           new->next  = NULL;
           new->text  = item;
           if(start) {
              tail->next = new;
              tail       = new;
	   } else start = tail = new;
     }

     /* ---->  Perform grouping/range op. on array, executing <COMMAND> for each element  <---- */
     set_conditions_ps(player,0,0,0,0,0,SEARCH_ARRAY,SEARCH_NAME,NULL,501);
     union_initgrouprange((union group_data *) start);
     cached_pid    =  Owner(player);
     flow_control &= ~FLOW_BREAKLOOP_ALL;
     while(union_grouprange() && ((flow_control & FLOW_MASK_LOOP) == FLOW_NORMAL)) {
           loop++, loopno = grp->before + loop, noloops = loop;
           strcpy(command_item2,(Blank(grp->cunion->element.index)) ? "":grp->cunion->element.index);
           strcpy(command_item,(Blank(ptr = decompress(grp->cunion->element.text))) ? "":ptr);

           current_line_number = startline;
           command_textptr     = cached_command_textptr;
           db[player].owner    = cached_pid;
           flow_control        = (flow_control & ~(FLOW_DATA1_MASK|FLOW_DATA2_MASK))|FLOW_FOREACH|FLOW_COMMAND;
           if(block) command_execute(player,NOTHING,command,1);
              else command_sub_execute(player,command,0,1);
           db[player].owner    = cached_pid;

           gettimeofday(&currenttime,NULL);
           if(usec_difference(currenttime,command_time) > (command_timelimit * 1000000)) {
              flow_control &= ~(FLOW_DATA1_MASK|FLOW_DATA2_MASK|FLOW_BREAKLOOP);
              selection_log_execlimit(player,command);
              if(block) {
                 command_textptr = endptr, *endptr = endchar;
                 current_line_number = endline;
	      }
              for(; start; start = tail) {
                  tail = start->next;
                  FREENULL(start);
	      }
              return;
	   }
     }

     if(block) {
        if(!(flow_control & FLOW_GOTO_LITERAL)) {
           command_textptr = endptr, *endptr = endchar;
           current_line_number = endline;
	} else *endptr = endchar;
     }

     flow_control &= ~(FLOW_DATA1_MASK|FLOW_DATA2_MASK|FLOW_BREAKLOOP);
     for(; start; start = tail) {
         tail = start->next;
         FREENULL(start);
     }
}

/* ---->  Goto given line number (Within current compound command)  <---- */
void selection_goto(CONTEXT)
{
     unsigned char literal = 0;
     int           line;

     setreturn(ERROR,COMMAND_FAIL);
     if(in_command) {
        if(*params && (*params == '-')) {
           for(; *params && (*params == '-'); params++);
	} else if(*params && (*params == '+')) {
           for(; *params && (*params == '+'); params++);
	} else literal = 1;
        for(; *params && (*params == ' '); params++);

        if(!Blank(params) && isdigit(*params)) {
           line = atol(params);
           if(line || !literal) {
              flow_control |= (literal) ? FLOW_GOTO_LITERAL:FLOW_GOTO;
              current_line_number = line;
              setreturn(OK,COMMAND_SUCC);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that line number is invalid.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that line number is invalid.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@goto"ANSI_LGREEN"' can only be used from within a compound command.");
}

/* ---->  Return or break out of current compound command  <---- */
/*              (val1:  0 = '@return', 1 = '@break'.)            */
void selection_return_break(CONTEXT)
{
     val1 = (val1) ? FLOW_BREAK:FLOW_RETURN;
     if(string_prefix("true",params) || string_prefix("success",params)) {
        if(command_result)
           command_boolean = COMMAND_SUCC;
              else setreturn(OK,COMMAND_SUCC);
        val1 |= FLOW_SUCC;
     } else if(string_prefix("false",params) || string_prefix("failure",params)) {
        if(command_result)
           command_boolean = COMMAND_FAIL;
              else setreturn(ERROR,COMMAND_FAIL);
        val1 |= FLOW_FAIL;
     } else if(string_prefix("current",params)) {
        if(command_boolean == COMMAND_INIT) setreturn(OK,COMMAND_SUCC);
           else if(!command_result) setreturn((command_boolean == COMMAND_SUCC) ? OK:ERROR,command_boolean);
        val1 |= ((command_boolean == COMMAND_SUCC) ? FLOW_SUCC:FLOW_FAIL);
     } else {
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LWHITE"true"ANSI_LGREEN"', '"ANSI_LWHITE"false"ANSI_LGREEN"' or '"ANSI_LWHITE"current"ANSI_LGREEN"'.");
        setreturn(ERROR,COMMAND_FAIL);
        return;
     }
     if((val1 & FLOW_MASK) < (flow_control & FLOW_MASK)) return;
     flow_control = (flow_control & ~(FLOW_MASK|FLOW_BOOL))|val1;
}

/* ---->  Skip rest of block (Within multi-lined '@if', '@for', '@with', '@while' or '@foreach')  <---- */
void selection_skip(CONTEXT)
{
     if(in_command) {
        flow_control |= FLOW_SKIP;
        if(command_boolean == COMMAND_INIT) setreturn(OK,COMMAND_SUCC);
           else if(!command_result)
              setreturn((command_boolean == COMMAND_SUCC) ? OK:ERROR,command_boolean);
     } else {
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@skip"ANSI_LGREEN"' can only be used from within a compound command.");
        setreturn(ERROR,COMMAND_FAIL);
     }
}

/* ---->  Fails/succeeds depending on the value returned by a query command/@eval  <---- */
/*        (0 = 'Error', anything else = 'OK')                                            */
/*        ***  MADE OBSOLETE BY SELECTION/ITERATION COMMANDS,  ***                       */
/*        ***  BUT STILL INCLUDED FOR BACKWARDS COMPATIBILITY  ***                       */
void selection_test(CONTEXT)
{
     /* ---->  Strip leading space  <---- */
     if(Blank(params)) {
        setreturn(OK,COMMAND_SUCC);
        return;
     }
     while(*params && (*params == ' ')) params++;

     /* ---->  Check for 0, '<UNSET RETURN VALUE>' or 'Error'  <---- */
     if(!strcmp("0",params) || !strcasecmp(ERROR,params) || !strcasecmp(UNSET_VALUE,params)) setreturn(ERROR,COMMAND_FAIL);
        else setreturn(OK,COMMAND_SUCC);
}

/* ---->  @while [true|false] <CONDITION> do [<COMMAND>]  <---- */
/*               [<COMMANDS>]                                   */
/*        [@end]                                                */
void selection_while(CONTEXT)
{
     int           startline = current_line_number,endline = current_line_number + 1,loop = 0;
     char          *cached_command_textptr = command_textptr;
     unsigned char boolean = 1,finished = 0,block = 0;
     char          *condition,*command,*endptr;
     struct        timeval currenttime;
     dbref         cached_pid;
     char          endchar;

     loopno = 0, noloops = 0;
     setreturn(ERROR,COMMAND_FAIL);
     if(!in_command && !Level4(player) && !Experienced(player)) {
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@while"ANSI_LGREEN"' can only be used from within a compound command (Unless you're an Experienced Builder or above.)");
        return;
     }

     /* ---->  Optional true/false specified?  <---- */
     for(; *params && (*params == ' '); params++);
     if(!strncasecmp(params,"true ",5)) params += 5, boolean = 1;
        else if(!strncasecmp(params,"false ",6)) params += 6, boolean = 0;

     /* ---->  <CONDITION> command  <---- */
     for(; *params && (*params == ' '); params++);
     if(!strncasecmp(params,"do ",3) || !strcasecmp(params,"do")) {
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify "ANSI_LYELLOW""ANSI_UNDERLINE"<CONDITION>"ANSI_LGREEN" command.");
        return;
     }
     for(condition = params; *params && !selection_do_keyword(params); params++);
     if(Blank(params)) {
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LYELLOW""ANSI_UNDERLINE"do"ANSI_LGREEN"' keyword omitted.");
        return;
     }
     *params++ = '\0';
     for(; *params && (*params == ' '); params++);
     for(; *params && (*params != ' '); params++);
     for(; *params && (*params == ' '); params++);

     /* ---->  <COMMAND> to execute if condition is met  <---- */
     if(Blank(params)) {
        if(in_command) {
           endptr  = selection_seek_end(command = command_textptr,&endline,0,0);
           endchar = *endptr, *endptr = '\0', endline--;
           if(Blank(command)) {
              *endptr = endchar;
              return;
	   }
           block = 1;
	} else {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the multiple line format of '"ANSI_LWHITE"@while"ANSI_LGREEN"' can only be used from within a compound command.");
           return;
	}
     } else command = params;

     cached_pid    =  Owner(player);
     flow_control &= ~FLOW_BREAKLOOP_ALL;
     while(!finished && ((flow_control & FLOW_MASK_LOOP) == FLOW_NORMAL)) {
           loop++, loopno = loop, noloops = loop;
           db[player].owner =  cached_pid;
           flow_control     =  (flow_control & ~(FLOW_DATA1_MASK|FLOW_DATA2_MASK))|FLOW_WHILE|FLOW_CONDITION;
           command_type    |=  TEST_SUBST;
           command_sub_execute(player,condition,0,1);
           command_type    &= ~TEST_SUBST;
           db[player].owner =  cached_pid;
           if((!boolean && (!strcmp("0",command_result) || !strcasecmp(UNSET_VALUE,command_result) || !strcasecmp(ERROR,command_result))) || (boolean && !(!strcmp("0",command_result) || !strcasecmp(UNSET_VALUE,command_result) || !strcasecmp(ERROR,command_result)))) {
              setreturn(ERROR,COMMAND_FAIL);
              current_line_number = startline;
              db[player].owner    = cached_pid;
              command_textptr     = cached_command_textptr;
              flow_control        = (flow_control & ~(FLOW_DATA1_MASK|FLOW_DATA2_MASK))|FLOW_WHILE|FLOW_COMMAND;
              if(block) command_execute(player,NOTHING,command,1);
                 else command_sub_execute(player,command,0,1);
              db[player].owner = cached_pid;
	   } else {
              setreturn(ERROR,COMMAND_FAIL);
              finished = 1;
	   }

           gettimeofday(&currenttime,NULL);
           if(usec_difference(currenttime,command_time) > (command_timelimit * 1000000)) {
              flow_control &= ~(FLOW_DATA1_MASK|FLOW_DATA2_MASK|FLOW_BREAKLOOP);
              selection_log_execlimit(player,command);
              if(block) {
                 command_textptr = endptr, *endptr = endchar;
                 current_line_number = endline;
	      }
              return;
	   }
     }

     if(block) {
        if(!(flow_control & FLOW_GOTO_LITERAL)) {
           command_textptr = endptr, *endptr = endchar;
           current_line_number = endline;
	} else *endptr = endchar;
     }
     flow_control &= ~(FLOW_DATA1_MASK|FLOW_DATA2_MASK|FLOW_BREAKLOOP);
}


/* ---->  @with <LIST TYPE> [<OBJECT>] do [<COMMAND>]   <---- */
/*              [<COMMANDS>]                                  */
/*        [@end]                                              */
/*                                                            */
/*        (val1:  0 = Normal, 1 = Grouping/range op. query.)  */
void selection_with(CONTEXT)
{
     int              startline = current_line_number,endline = current_line_number + 1;
     char             *ptr,*buffer = (val1) ? querybuf:scratch_return_string;
     char             *cached_command_textptr = command_textptr;
     dbref            object = NOTHING,currentobj,cached_pid;
     int              listtype = 0,field_flags = 0,loop;
     char             *command,*endptr,*index = NULL;
     struct   timeval currenttime;
     unsigned char    block = 0;
     char             endchar;

     setreturn(ERROR,COMMAND_FAIL);
     *command_item2 = '\0';
     *command_item  = '\0';
     loopno = 0, noloops = 0;

     if(!in_command && !val1 && !Level4(player) && !Experienced(player)) {
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@with"ANSI_LGREEN"' can only be used from within a compound command (Unless you're an Experienced Builder or above.)");
        return;
     }

     /* ---->  Parse grouping/range operators and get first word in returned string  <---- */
     params = (char *) parse_grouprange(player,params,ALL,1);
     for(; *params && (*params == ' '); params++);

     /* ---->  '[INDEX: ]' specified?  <---- */
     if(*params && !strncasecmp(params,"[index",6)) {
        int  result,dummy;
        char p;

        /* ---->  Parse field types and optional 'absolute' keyword  <---- */
        for(params += 6; *params && (*params == ' '); params++);
        while(*params && (*params == '|')) {
              for(params++; *params && (*params == ' '); params++);
              for(ptr = params; *params && !((*params == ':') || (*params == '|') || (*params == ']')); params++);
              p = *params, *params = '\0';

              if(!(result = parse_fieldtype(ptr,&dummy))) {
	 	 if(string_prefix("absolute",ptr)) {
                    grp->groupsize = DEFAULT;
                    grp->groupno   = ALL;
                    grp->rfrom     = 1;
                    grp->rto       = 1;
                    result++;
		 } else if(string_prefix("indexnames",ptr) || string_prefix("indices",ptr) || string_prefix("indexs",ptr)) {
                    field_flags   |= SEARCH_INDEX;
                    result++;
		 } else if(string_prefix("all",ptr)) {
                    field_flags    = SEARCH_ALL_TYPES;
                    result++;
		 }
	      } else field_flags |= result;

              /* ---->  Unknown field type?  <---- */
              if(!result) {
                 if(in_command && selection_skip_do_keyword(params)) {
                    command_textptr = selection_seek_end(command_textptr,&endline,0,0);
                    current_line_number = (endline - 1);
		 }
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"%s"ANSI_LGREEN"' is an unknown field type.",ptr);
                 return;
	      }
              *params = p;
	}

        if(*params && (*params == ':')) params++;
        for(; *params && (*params == ' '); params++);

        /* ---->  Wildcard specification  <---- */
        for(index = params; *params && !((*params == ']') && (*(params + 1) == ' ')); params++);
        *params++ = '\0';
        if(*params) for(params++; *params && (*params == ' '); params++);
     }
     if(!field_flags) field_flags = SEARCH_NAME;

     for(; *params && (*params == ' '); params++);
     for(ptr = params; *params && (*params != ' '); params++);
     if(*params) *params++ = '\0';

     /* ---->  Determine which type of list to process  <---- */
     if(Blank(ptr)) {
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify which type of list to process (E.g:  '"ANSI_LWHITE"@with contents <OBJECT> do..."ANSI_LGREEN"', '"ANSI_LWHITE"@with elements <ARRAY> do..."ANSI_LGREEN"', etc.)");
        if(in_command && selection_skip_do_keyword(params)) {
           command_textptr = selection_seek_end(command_textptr,&endline,0,0);
           current_line_number = (endline - 1);
	}
        return;
     } else if(string_prefix("connected",ptr)) {
        if(!Level4(Owner(player)) && !(in_command && Apprentice(current_command))) {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above can traverse the list of connected characters.");
           if(in_command && selection_skip_do_keyword(params)) {
              command_textptr = selection_seek_end(command_textptr,&endline,0,0);
              current_line_number = (endline - 1);
	   }
	   return;
 	}
        listtype = SEARCH_CONNECTED;
     } else if(string_prefix("elements",ptr)) {
        listtype = SEARCH_ARRAY|SEARCH_CONNECTED;
     } else if(string_prefix("indexnames",ptr) || string_prefix("indices",ptr)) {
        listtype = SEARCH_INDEX|SEARCH_CONNECTED;
     } else if(string_prefix("friendslist",ptr)) {
        listtype = SEARCH_FRIENDS|SEARCH_CONNECTED;
     } else if(string_prefix("fotherslist",ptr)) {
        listtype = SEARCH_FOTHERS|SEARCH_CONNECTED;        
     } else if(string_prefix("enemieslist",ptr)) {
        listtype = SEARCH_ENEMIES|SEARCH_CONNECTED;
     } else listtype = parse_objecttype(ptr);

     if(!listtype) {
        if(!val1) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"%s"ANSI_LGREEN"' is an unknown type of list.",ptr);
        if(in_command && selection_skip_do_keyword(params)) {
           command_textptr = selection_seek_end(command_textptr,&endline,0,0);
           current_line_number = (endline - 1);
	}
        return;
     }
     for(; *params && (*params == ' '); params++);
     *(params - 1) = ' ';

     /* ---->  Optional object which linked list is attached to  <---- */
     if((!val1 || *params) && !selection_do_keyword(params - 1)) {
        ptr = querybuf;
        while(*params && !selection_do_keyword(params - 1)) {
              for(; *params && (*params != ' '); *ptr++ = *params, params++);
              for(; *params && (*params == ' '); params++);
              if(*params && !selection_do_keyword(params - 1)) *ptr++ = ' ';
        }
        *ptr = '\0';

        object = match_preferred(player,player,querybuf,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
        if(!Valid(object)) {
           if(in_command && selection_skip_do_keyword(params)) {
              command_textptr = selection_seek_end(command_textptr,&endline,0,0);
              current_line_number = (endline - 1);
	   }
           return;
	}
     } else object = player;

     if(!val1) {
        for(; *params && (*params != ' '); params++);
        for(; *params && (*params == ' '); params++);
        command = params;
     } else command = NULL;
     loop       = 0;
     cached_pid = Owner(player);

     /* ---->  <COMMAND>  <--- */
     if(!val1 && Blank(command)) {
        if(in_command) {
           endptr  = selection_seek_end(command = command_textptr,&endline,0,0);
           endchar = *endptr, *endptr = '\0', endline--;

           if(Blank(command)) {
              command_textptr = endptr, *endptr = endchar;
              current_line_number = endline;
              return;
	   }
           block = 1;
	} else {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the multiple line format of '"ANSI_LWHITE"@with"ANSI_LGREEN"' can only be used from within a compound command.");
           return;
	}
     }

     /* ---->  Process list  <---- */
     if(listtype & SEARCH_CONNECTED) {
        switch(listtype & (SEARCH_ARRAY|SEARCH_INDEX|SEARCH_FRIENDS|SEARCH_FOTHERS|SEARCH_ENEMIES)) {
               case SEARCH_ARRAY:
               case SEARCH_INDEX:

                    /* ---->  Process elements or element index names of dynamic array  <---- */
                    if(Typeof(object) != TYPE_ARRAY) {
                       if(!val1) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only process the element%s of a dynamic array  -  %s"ANSI_LWHITE"%s"ANSI_LGREEN" isn't a dynamic array.",(listtype & SEARCH_INDEX) ? " index names":"s",Article(object,UPPER,DEFINITE),unparse_object(player,object,0));
                       if(block) {
                          command_textptr = endptr, *endptr = endchar;
                          current_line_number = endline;
		       }
                       return;
		    } else if(Private(object) && !can_read_from(player,object)) {
                       if(!val1) {
                          sprintf(buffer,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is set "ANSI_LYELLOW"PRIVATE"ANSI_LGREEN"  -  Only characters above the level of its owner (",Article(object,LOWER,DEFINITE),unparse_object(player,object,0));
                          output(getdsc(player),player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN") may process its element%s.",buffer,Article(Owner(object),LOWER,INDEFINITE),getcname(player,Owner(object),1,0),(listtype & SEARCH_INDEX) ? " index names":"s");
		       }
                       if(block) {
                          command_textptr = endptr, *endptr = endchar;
                          current_line_number = endline;
		       }
                       return;
		    }

                    set_conditions_ps(player,0,0,0,0,0,SEARCH_ARRAY,field_flags,(Blank(index)) ? NULL:index,(listtype & SEARCH_INDEX) ? 502:501);
                    union_initgrouprange((union group_data *) db[object].data->array.start);
                    if(val1) {
                       switch(val1) {
                              case 1:
                                   sprintf(buffer,"%d",grp->nogroups);
                                   break;
                              case 2:
                                   sprintf(buffer,"%d",grp->rangeitems);
                                   break;
                              case 3:
                                   sprintf(buffer,"%d",grp->groupitems);
                                   break;
                              case 4:
                                   sprintf(buffer,"%d",grp->totalitems);
                                   break;
                              default:
                                   strcpy(buffer,"0");
		       }
                       setreturn(buffer,COMMAND_SUCC);
                       return;
		    }

                    flow_control &= ~FLOW_BREAKLOOP_ALL;
                    while(union_grouprange() && ((flow_control & FLOW_MASK_LOOP) == FLOW_NORMAL)) {
                          loop++, loopno = grp->before + loop, noloops = loop;
                          if(listtype & SEARCH_INDEX) {
                             strcpy(command_item2,(Blank(ptr = decompress(grp->cunion->element.text))) ? "":ptr);
                             strcpy(command_item,(Blank(grp->cunion->element.index)) ? "":grp->cunion->element.index);
			  } else {
                             strcpy(command_item2,(Blank(grp->cunion->element.index)) ? "":grp->cunion->element.index);
                             strcpy(command_item,(Blank(ptr = decompress(grp->cunion->element.text))) ? "":ptr);
			  }

                          current_line_number = startline;
                          db[player].owner    = cached_pid;
                          command_textptr     = cached_command_textptr;
                          flow_control        = (flow_control & ~(FLOW_DATA1_MASK|FLOW_DATA2_MASK))|FLOW_WITH|FLOW_COMMAND;
                          if(block) command_execute(player,NOTHING,command,1);
                             else command_sub_execute(player,command,0,1);
                          db[player].owner = cached_pid;

                          gettimeofday(&currenttime,NULL);
                          if(usec_difference(currenttime,command_time) > (command_timelimit * 1000000)) {
                             flow_control &= ~(FLOW_DATA1_MASK|FLOW_DATA2_MASK|FLOW_BREAKLOOP);
                             selection_log_execlimit(player,command);
                             if(block) {
                                command_textptr = endptr, *endptr = endchar;
                                current_line_number = endline;
			     }
                             return;
			  }
		    }
                    break;
               case SEARCH_ENEMIES:
               case SEARCH_FOTHERS:
               case SEARCH_FRIENDS:

                    /* ---->  Process friends/enemies list  <---- */
                    if(Typeof(object) != TYPE_CHARACTER) {
                       if(!val1) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only process the %s list of a character  -  %s"ANSI_LWHITE"%s"ANSI_LGREEN" isn't a character.",(listtype & SEARCH_ENEMIES) ? "enemies":((listtype & SEARCH_FRIENDS) ? "friends":"fothers"),Article(object,UPPER,DEFINITE),unparse_object(player,object,0));
                       if(block) {
                          command_textptr = endptr, *endptr = endchar;
                          current_line_number = endline;
		       }
                       return;
		    } else if(!can_read_from(player,object)) {
                       if(!val1) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only process your own %s list.",(listtype & SEARCH_ENEMIES) ? "enemies":((listtype & SEARCH_FRIENDS) ? "friends":"fothers"));
                       if(block) {
                          command_textptr = endptr, *endptr = endchar;
                          current_line_number = endline;
		       }
                       return;
		    }
		    		if(listtype & SEARCH_FOTHERS)
		    			field_flags = SEARCH_ALL;

                    set_conditions_ps((listtype & SEARCH_FOTHERS) ? object : player,(listtype & SEARCH_ENEMIES || listtype & SEARCH_FOTHERS) ? 0:1,0,0,0,0,(listtype & SEARCH_FOTHERS) ? 0 : SEARCH_CHARACTER,field_flags,(Blank(index)) ? NULL:index,(listtype & SEARCH_FOTHERS) ? 304 : 504);
                    if(listtype & SEARCH_FOTHERS)
                    	entiredb_initgrouprange();
                    else
                    	union_initgrouprange((union group_data *) db[object].data->player.friends);
                    if(val1) {
                       switch(val1) {
                              case 1:
                                   sprintf(buffer,"%d",grp->nogroups);
                                   break;
                              case 2:
                                   sprintf(buffer,"%d",grp->rangeitems);
                                   break;
                              case 3:
                                   sprintf(buffer,"%d",grp->groupitems);
                                   break;
                              case 4:
                                   sprintf(buffer,"%d",grp->totalitems);
                                   break;
                              default:
                                   strcpy(buffer,"0");
		       }
                       setreturn(buffer,COMMAND_SUCC);
                       return;
		    }

                    flow_control &= ~FLOW_BREAKLOOP_ALL;
                    while( ((listtype & SEARCH_FOTHERS) ? entiredb_grouprange() : union_grouprange()) && ((flow_control & FLOW_MASK_LOOP) == FLOW_NORMAL)) {
                          loop++;
                          loopno  = grp->before + loop;
                          noloops = loop;
                          strcpy(command_item2,String(getfield((listtype & SEARCH_FOTHERS) ? grp->cobject : grp->cunion->friend.friend,DESC)));
                          strcpy(command_item,getnameid(player,(listtype & SEARCH_FOTHERS) ? grp->cobject : grp->cunion->friend.friend,NULL)); 

                          current_line_number = startline;
                          db[player].owner    = cached_pid;
                          command_textptr     = cached_command_textptr;
                          flow_control        = (flow_control & (~(FLOW_DATA1_MASK|FLOW_DATA2_MASK)|FLOW_WITH_BANISHED))|FLOW_WITH|FLOW_WITH_FRIENDS|FLOW_COMMAND;
                          if(block) command_execute(player,NOTHING,command,1);
                             else command_sub_execute(player,command,0,1);
                          db[player].owner = cached_pid;

                          gettimeofday(&currenttime,NULL);
                          if(usec_difference(currenttime,command_time) > (command_timelimit * 1000000)) {
                             flow_control &= ~(FLOW_DATA1_MASK|FLOW_DATA2_MASK|FLOW_BREAKLOOP);
                             selection_log_execlimit(player,command);
                             if(block) {
                                command_textptr = endptr, *endptr = endchar;
                                current_line_number = endline;
			     }
                             return;
			  }
		    }
                    break;
	       default:

                    /* ---->  Process list of connected characters  <---- */
                    set_conditions_ps(player,0,0,0,0,0,SEARCH_CHARACTER,field_flags,(Blank(index)) ? NULL:index,205);
                    union_initgrouprange((union group_data *) descriptor_list);
                    if(val1) {
                       switch(val1) {
                              case 1:
                                   sprintf(buffer,"%d",grp->nogroups);
                                   break;
                              case 2:
                                   sprintf(buffer,"%d",grp->rangeitems);
                                   break;
                              case 3:
                                   sprintf(buffer,"%d",grp->groupitems);
                                   break;
                              case 4:
                                   sprintf(buffer,"%d",grp->totalitems);
                                   break;
                              default:
                                   strcpy(buffer,"0");
		       }
                       setreturn(buffer,COMMAND_SUCC);
                       return;
		    }

                    flow_control &= ~FLOW_BREAKLOOP_ALL;
                    while(union_grouprange() && ((flow_control & FLOW_MASK_LOOP) == FLOW_NORMAL)) {
                          loop++;
                          loopno  = grp->before + loop;
                          noloops = loop;
                          strcpy(command_item2,String(getfield(grp->cunion->descriptor.player,DESC)));
                          strcpy(command_item,getnameid(player,grp->cunion->descriptor.player,NULL));

                          current_line_number = startline;
                          db[player].owner    = cached_pid;
                          command_textptr     = cached_command_textptr;
                          flow_control        = (flow_control & (~(FLOW_DATA1_MASK|FLOW_DATA2_MASK)|FLOW_WITH_BANISHED))|FLOW_WITH|FLOW_WITH_CONNECTED|FLOW_COMMAND;
                          if(block) command_execute(player,NOTHING,command,1);
                             else command_sub_execute(player,command,0,1);
                          db[player].owner = cached_pid;

                          gettimeofday(&currenttime,NULL);
                          if(usec_difference(currenttime,command_time) > (command_timelimit * 1000000)) {
                             flow_control &= ~(FLOW_DATA1_MASK|FLOW_DATA2_MASK|FLOW_BREAKLOOP);
                             selection_log_execlimit(player,command);
                             if(block) {
                                command_textptr = endptr, *endptr = endchar;
                                current_line_number = endline;
			     }
                             return;
			  }
		    }
	}
     } else {

        /* ---->  Process linked list attached to an object  <---- */
        if(listtype & SEARCH_COMMAND) {
           if(HasList(object,COMMANDS))  object = getfirst(object,COMMANDS,&currentobj);
              else listtype = 0;
	} else if(listtype & SEARCH_EXIT) {
           if(HasList(object,EXITS))     object = getfirst(object,EXITS,&currentobj);
              else listtype = 0;
	} else if(listtype & (SEARCH_VARIABLE|SEARCH_PROPERTY|SEARCH_ARRAY)) {
           if(HasList(object,VARIABLES)) object = getfirst(object,VARIABLES,&currentobj);
              else listtype = 0;
	} else if(listtype & (SEARCH_FUSE|SEARCH_ALARM)) {
           if(HasList(object,FUSES))     object = getfirst(object,FUSES,&currentobj);
              else listtype = 0;
	} else if(listtype & (SEARCH_CHARACTER|SEARCH_THING|SEARCH_ROOM|SEARCH_PUPPET)) {
           if(HasList(object,CONTENTS))  object = getfirst(object,CONTENTS,&currentobj);
              else listtype = 0;
	} else listtype = 0;
        if(!listtype) {
           if(block) {
              command_textptr = endptr, *endptr = endchar;
              current_line_number = endline;
	   }
           return;
	}

        set_conditions_ps(player,SEARCH_ALL_TYPES,0,0,0,0,listtype,field_flags,(Blank(index)) ? NULL:index,101);
        db_initgrouprange(object,currentobj);
        if(val1) {
           switch(val1) {
                  case 1:
                       sprintf(buffer,"%d",grp->nogroups);
                       break;
                  case 2:
                       sprintf(buffer,"%d",grp->rangeitems);
                       break;
                  case 3:
                       sprintf(buffer,"%d",grp->groupitems);
                       break;
                  case 4:
                       sprintf(buffer,"%d",grp->totalitems);
                       break;
                  default:
                       strcpy(buffer,"0");
	   }
           setreturn(buffer,COMMAND_SUCC);
           return;
	}

        flow_control &= ~FLOW_BREAKLOOP_ALL;
        while(db_grouprange() && ((flow_control & FLOW_MASK_LOOP) == FLOW_NORMAL)) {
              loop++;
              loopno  = grp->before + loop;
              noloops = loop;
              strcpy(command_item2,String(getfield(grp->cobject,DESC)));
              strcpy(command_item,getnameid(player,grp->cobject,NULL)); 

              current_line_number = startline;
              db[player].owner    = cached_pid;
              command_textptr     = cached_command_textptr;
              flow_control        = (flow_control & ~(FLOW_DATA1_MASK|FLOW_DATA2_MASK))|FLOW_WITH|FLOW_COMMAND;
              if(block) command_execute(player,NOTHING,command,1);
                 else command_sub_execute(player,command,0,1);
              db[player].owner = cached_pid;

              gettimeofday(&currenttime,NULL);
              if(usec_difference(currenttime,command_time) > (command_timelimit * 1000000)) {
                 flow_control &= ~(FLOW_DATA1_MASK|FLOW_DATA2_MASK|FLOW_BREAKLOOP);
                 selection_log_execlimit(player,command);
                 if(block) {
                    command_textptr = endptr, *endptr = endchar;
                    current_line_number = endline;
		 }
                 return;
	      }
	}
     }

     if(block) {
        if(!(flow_control & FLOW_GOTO_LITERAL)) {
           command_textptr = endptr, *endptr = endchar;
           current_line_number = endline;
	} else *endptr = endchar;
     }
     flow_control &= ~(FLOW_DATA1_MASK|FLOW_DATA2_MASK|FLOW_BREAKLOOP);
}
