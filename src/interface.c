/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| INTERFACE.C  -  Implements handling and pre-processing of commands typed by |
|                 users/executed within compound commands and event timing    |
|                 for alarms, fuses, automated server maintenance and server  |
|                 shutdown.                                                   |
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
| Module originally modified for TCZ by:  J.P.Boggis 21/12/1993.              |
|                Major optimisations by:  J.P.Boggis 16/03/1996.              |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: interface.c,v 1.4 2005/06/29 21:21:40 tcz_monster Exp $

*/


#include <sys/types.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#ifdef LINUX
   #include <sys/time.h>
#else
   #include <time.h>
#endif

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "descriptor_flags.h"
#include "object_types.h"
#include "objectlists.h"
#include "flagset.h"
#include "fields.h"
#include "search.h"
#include "match.h"

#include "general_cmdtable.h"
#include "query_cmdtable.h"
#include "short_cmdtable.h"
#include "bank_cmdtable.h"
#include "bbs_cmdtable.h"
#include "emailforward.h"
#include "at_cmdtable.h"


/* ---->  Command table sizes (Computed at run-time)  <---- */
unsigned short general_table_size = 0;
unsigned short short_table_size   = 0;
unsigned short query_table_size   = 0;
unsigned short bank_table_size    = 0;
unsigned short bbs_table_size     = 0;
unsigned short at_table_size      = 0;

int     nesting_level      = 0;  /*  Command nesting level (Number of nested {} substitutions.)  */


/* ---->  Sort given command table into strict alphabetical order  <---- */
/*        (search_cmdtable() relies on entries in the command            */
/*        tables being in strict alphabetical order (A binary            */
/*        search is used for maximum efficiency.))                       */
short sort_cmdtable(struct cmd_table *table,unsigned short *count)
{
      unsigned short loop,top,highest;
      struct   cmd_table temp;

      for(loop = 0; table[loop].name; table[loop].len = strlen(table[loop].name), loop++);
      *count = loop;
      for(top = *count - 1; top > 0; top--) {

          /* ---->  Find highest entry in unsorted part of list  <---- */
          for(loop = 1, highest = 0; loop <= top; loop++)
              if(strcasecmp(table[loop].name,table[highest].name) > 0)
                 highest = loop;

          /* ---->  Swap highest entry in unsorted part of list with bottom entry of sorted part of list  <---- */
          if(highest < top) {
             temp.flags = table[top].flags, temp.name = table[top].name, temp.func = table[top].func, temp.val1 = table[top].val1, temp.val2 = table[top].val2, temp.len = table[top].len;
             table[top].flags = table[highest].flags, table[top].name = table[highest].name, table[top].func = table[highest].func, table[top].val1 = table[highest].val1, table[top].val2 = table[highest].val2, table[top].len = table[highest].len;
             table[highest].flags = temp.flags, table[highest].name = temp.name, table[highest].func = temp.func, table[highest].val1 = temp.val1, table[highest].val2 = temp.val2, table[highest].len = temp.len;
	  }
      }
      return(*count);
}

/* ---->  Search for COMMAND in given command table, returning CMD_TABLE  <---- */
/*        entry if found, otherwise NULL.                                       */
struct cmd_table *search_cmdtable(const char *command,struct cmd_table *table,unsigned short entries)
{
       int      top = entries - 1,middle = 0,bottom = 0,nearest = NOTHING,result;
       unsigned short len,nlen = 0xFFFF;

       if(Blank(command)) return(NULL);
       len = strlen(command);
       while(bottom <= top) {
             middle = (top + bottom) / 2;
             if((result = strcasecmp(table[middle].name,command)) != 0) {
                if((table[middle].len < nlen) && (len <= table[middle].len) && !strncasecmp(table[middle].name,command,len))
                   nearest = middle, nlen = table[middle].len;

                if(result < 0) bottom = middle + 1;
                   else top = middle - 1;
	     } else return(&table[middle]);
       }

       /* ---->  Nearest matching command  <---- */
       if(nearest != NOTHING) return(&table[nearest]);
       return(NULL);
}

/* ---->  Initialise TCZ MUD server  <---- */
int init_tcz(unsigned char load_db,unsigned char compressed)
{
    int   count = 0;
    dbref i;

    /* ---->  Initialisation  <---- */
    *command_item2 = '\0';
    *command_item  = '\0';
    *bootmessage   = '\0';
    dumpinterval   = DUMP_INTERVAL;
    dumptiming     = uptime;

    /* ---->  Load database  <---- */
    if(load_db) {
       time_t now,total;
       int    count;
       FILE   *f;
       dbref  i;

       gettime(now);
       if(compressed) {
          sprintf(scratch_return_string,DB_DECOMPRESS" < %s",dumpfull);
          if((f = popen(scratch_return_string,"r")) == NULL) return(-1);
       } else if((f = fopen(dumpfull,"r")) == NULL) return(-1);
       writelog(SERVER_LOG,0,"RESTART","Loading %sdatabase '%s'...",(compressed) ? "compressed ":"",dumpfull);
       if(!db_read(f)) return(-1);
       if(compressed) pclose(f);
          else fclose(f);
       gettime(total);

       /* ---->  Count objects in DB  <---- */
       for(i = 0, count = 0; i < db_top; i++)
           if(Typeof(i) != TYPE_FREE)
              count++;

       now += timeadjust;
       writelog(SERVER_LOG,0,"RESTART","Database load completed in %s (%d/%d objects.)",interval(total - now,0,ENTITIES,0),count,db_top);
    }

    /* ---->  Compression statistics  <---- */
#ifdef EXTRA_COMPRESSION
    compression_ratio();
#endif

    /* ---->  Automatic database sanity checks and corrections (Can also be performed manually within TCZ, if neccessary)  <---- */
    if(load_db) {
       writelog(SERVER_LOG,0,"RESTART","Checking database for corruption%s...",(dumpsanitise || option_check(OPTSTATUS)) ? " (Full check)":" (General check)");
       sanity_general(ROOT,1);
       if(dumpsanitise || option_check(OPTSTATUS)) {
          sanity_fixlists(ROOT,1);

          /* ---->  Sanitise locks  <---- */
          for(i = 0; i < db_top; i++)
              switch(Typeof(i)) {
                     case TYPE_EXIT:
                          db[i].data->exit.lock = sanitise_boolexp(db[i].data->exit.lock);
                          break;
                     case TYPE_FUSE:
                          db[i].data->fuse.lock = sanitise_boolexp(db[i].data->fuse.lock);
                          break;
                     case TYPE_ROOM:
                          db[i].data->room.lock = sanitise_boolexp(db[i].data->room.lock);
                          break;
                     case TYPE_THING:
                          db[i].data->thing.lock_key = sanitise_boolexp(db[i].data->thing.lock_key);
                          db[i].data->thing.lock     = sanitise_boolexp(db[i].data->thing.lock);
                          break;
                     case TYPE_COMMAND:
                          db[i].data->command.lock = sanitise_boolexp(db[i].data->command.lock);
                          break;
	      }
       }
    }
    event_initialise();

    /* ---->  Initialise command tables  <---- */
    count += sort_cmdtable(general_cmds,&general_table_size);
    count += sort_cmdtable(short_cmds,&short_table_size);
    count += sort_cmdtable(query_cmds,&query_table_size);
    count += sort_cmdtable(bank_cmds,&bank_table_size);
    count += sort_cmdtable(bbs_cmds,&bbs_table_size);
    count += sort_cmdtable(at_cmds,&at_table_size);
    count += channel_sort_cmdtable();
    count += sort_edit_cmdtable();
    writelog(SERVER_LOG,0,"RESTART","Command tables initialised and sorted (%d entr%s.)",count,(count == 1) ? "y":"ies");

    /* ---->  Initialise HTML tables  <---- */
    html_sort_tags();
    html_sort_entities();
    html_init_smileys();

    /* ---->  Initialise module/author tables  <---- */
    modules_sort_modules();
    modules_sort_authors();

    /* ---->  Initialise match conversion tables  <---- */
    sort_name_to_type();

    /* ---->  Initialise global compound command binary tree  <---- */
    count = global_initialise(GLOBAL_COMMANDS,1);
    writelog(SERVER_LOG,0,"RESTART","Global compound command tree initialised (%d compound command%s added from %s.)",count,Plural(count),unparse_object(ROOT,GLOBAL_COMMANDS,UPPER|INDEFINITE));

    /* ---->  Initialise feelings list  <---- */
    count = init_feelings();
    writelog(SERVER_LOG,0,"RESTART","Feelings list initialised (%d feeling%s available.)",count,Plural(count));

    /* ---->  Initialiase finance  <---- */
    if(!quarter) {
       gettime(quarter);
       finance_quarter();
    }

    /* ---->  Initialise yearly events  <---- */
    count = yearly_event_sort(NOTHING);
    writelog(SERVER_LOG,0,"RESTART","Yearly events initialised (%d event%s.)",count,Plural(count));

    /* ---->  Disable user logins option (-n)  <---- */
    if(!option_logins(OPTSTATUS)) {
       connections = 0;
       creations   = 0;
    }
    return(0);
}

/* ---->  Process given command  <---- */
void process_basic_command(dbref player,char *original_command,unsigned char converse)
{
     unsigned char executed = 0,absolute = 0,shortcmd = 0,traced = 0,test = 0;
     char     *tracecmd = NULL,*arg0,*arg1,*arg2,*q,*ptr;
     char     command[TEXT_SIZE],params[TEXT_SIZE];
     struct   descriptor_data *p = getdsc(player);
     dbref    cached_curchar,cache = NOTHING;
     static   char buffer[BUFFER_LEN];
     const    char *cached_curcmdptr;
     struct   cmd_table *cmd = NULL;

     /* ---->  Validate character  <---- */
     if(original_command) while(*original_command && (*original_command == ' ')) original_command++;
     if(!Validchar(player)) {
        writelog(BUG_LOG,1,"BUG","(process_basic_command() in interface.c)  Invalid character %s, executing command '%s'.",unparse_object(ROOT,player,0),String(original_command));
        setreturn(ERROR,COMMAND_FAIL);
        command_type &= ~TEST_SUBST;
        return;
     } else if(Blank(original_command)) {
        setreturn(ERROR,COMMAND_FAIL);
        command_type &= ~TEST_SUBST;
        return;
     }

     /* ---->  Remove leading whitespace  <---- */
     standard_commands++;
     cached_curcmdptr = current_cmdptr;
     cached_curchar   = current_character;
     current_cmdptr   = original_command;
     if(command_type & TEST_SUBST) {
        command_type &= ~TEST_SUBST;
        if(*original_command && (*original_command == '@') && (!strncasecmp(original_command,"@test ",6) || !strcasecmp(original_command,"@test")))
           for(original_command += 5; *original_command && (*original_command == ' '); original_command++);
        test = 1;
     }

     /* ---->  Check command (In {}'s) nesting limit hasn't been exceeded  <---- */
     if((!Level4(db[player].owner) && (nesting_level > MAX_NESTED_MORTAL)) || (Level4(db[player].owner) && (nesting_level > MAX_NESTED_ADMIN))) {
        if(!(command_type & WARNED)) {
           output(p,player,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"You've exceeded the maximum number of nested commands allowed ("ANSI_LYELLOW"%d"ANSI_LWHITE".)",(Level4(db[player].owner)) ? MAX_NESTED_ADMIN:MAX_NESTED_MORTAL);
           command_type |= WARNED;
	}
        current_character = cached_curchar, current_cmdptr = cached_curcmdptr;
        setreturn(LIMIT_EXCEEDED,COMMAND_FAIL);
        return;
     }

     /* ---->  Check compound command nesting level hasn't been exceeded  <---- */
     if(in_command && ((!Level4(db[player].owner) && (command_nesting_level > MAX_CMD_NESTED_MORTAL)) || (Level4(db[player].owner) && (command_nesting_level > MAX_CMD_NESTED_ADMIN)))) {
        if(!(command_type & WARNED)) {
           output(getdsc(player),player,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"Maximum compound command nesting level ("ANSI_LYELLOW"%d"ANSI_LWHITE") exceeded within compound command "ANSI_LYELLOW"%s"ANSI_LWHITE".",(Level4(db[player].owner)) ? MAX_CMD_NESTED_ADMIN:MAX_CMD_NESTED_MORTAL,unparse_object(player,current_command,0));
           writelog(EXECUTION_LOG,1,"RECURSION","Maximum compound command nesting level (%d) exceeded within compound command %s(#%d) by %s(#%d).",(Level4(db[player].owner)) ? MAX_CMD_NESTED_ADMIN:MAX_CMD_NESTED_MORTAL,getname(current_command),current_command,getname(player),player);
	}
        command_type |= WARNED, flow_control |= FLOW_RETURN;
        setreturn(LIMIT_EXCEEDED,COMMAND_FAIL);
        return;
     }

     /* ---->  Strip excess blank space  <---- */
     q = ptr = original_command;
     while(*ptr) {

           /* ---->  Scan over word  <---- */
           while(*ptr && !((*ptr == '\n') || (*ptr == ' '))) *q++ = *ptr++;

           /* ---->  Skip over spaces  <---- */
           if(*ptr && (*ptr != '\n') && (*ptr == ' ')) {
              while(*ptr && (*ptr != '\n') && (*ptr == ' ')) ptr++;
              *q++ = ' ';
	   }

           /* ---->  Strip trailing blanks  <---- */
           if(!*ptr || (*ptr == '\n')) {
              for(q--; (q > original_command) && (*q == ' '); q--);
              q++;
	   }

           /* ---->  Copy rest literally  <---- */
           if(*ptr && (*ptr == '\n')) while(*ptr) *q++ = *ptr++;
     }
     *q = '\0';

     /* ---->  {$VARIABLE} variable substitution?  <---- */
     if(!compoundonly && *original_command && (*original_command == '$') && !isdigit(*(original_command + 1))) {
        struct str_ops str_data;

        command_type   |= VARIABLE_SUBST;
        str_data.src    = (char *) original_command;
        str_data.dest   = command;
        str_data.length = 0;
        substitute_sub_variable(player,&str_data);
        *(str_data.dest)  = '\0';
        current_character = cached_curchar;
        current_cmdptr    = cached_curcmdptr;
        setreturn(command,COMMAND_SUCC);
        return;
     }

     /* ---->  {%} substitution (E.g:  '{%@%n}')  <---- */
     if(!compoundonly && *original_command && (*original_command == '%')) {
        command_type     |=  NESTED_SUBSTITUTION;
        substitute(player,command,original_command,0,"",NULL,0);
        command_type     &= ~NESTED_SUBSTITUTION;
        current_character =  cached_curchar;
        current_cmdptr    =  cached_curcmdptr;
        setreturn(command,COMMAND_SUCC);
        return;
     }

     /* ---->  Trace sequence  <---- */
     traced = output_trace(player,current_command,0,0,0,NULL);
     if(traced && (nesting_level < 1)) output_trace(player,current_command,0,1,7,ANSI_DCYAN"["ANSI_LYELLOW""ANSI_UNDERLINE"%03d"ANSI_DCYAN"]  "ANSI_LGREEN"%s  "ANSI_LMAGENTA"("ANSI_UNDERLINE"EXEC"ANSI_LMAGENTA")",current_line_number,String(original_command));

     /* ---->  If editor has been started within compound command and current command is an editor command, process it  <---- */
     if(!compoundonly && p && p->edit && (command_type & EDIT_COMMAND) && (*original_command == EDIT_CMD_TOKEN)) {

        /* ---->  Trace editor command  <---- */
        if(traced) output_trace(player,current_command,0,1,7,ANSI_DCYAN"("ANSI_LCYAN"%03d"ANSI_DCYAN")  "ANSI_LGREEN"Passing command to the editor...",current_line_number);
        edit_process_command(p,original_command);
        current_character = cached_curchar, current_cmdptr = cached_curcmdptr;
        return;
     }

     nesting_level++;
     if(!compoundonly && (test || !converse)) {
        unsigned char loghack = 0;
        char     *hptr;

        if(in_command || (nesting_level > 1)) {
           if(Valid(current_command) && (Typeof(current_command) != TYPE_FREE) && (Apprentice(current_command) || Level4(Owner(current_command))))
              alias_substitute((in_command) ? Owner(current_command):player,original_command,command);
                 else strcpy(command,original_command);
	
           for(hptr = command; *hptr && (*hptr == ' '); hptr++);
           if(in_command && Valid(current_command) && !Wizard(current_command) && (Owner(player) != player) && (*hptr == '$')) loghack = 1;
           substitute_command(player,command,params);
           substitute_variable(player,params,command);
	} else {
           alias_substitute(player,original_command,params);
           for(hptr = command; *hptr && (*hptr == ' '); hptr++);
           if(in_command && Valid(current_command) && !Wizard(current_command) && (Owner(player) != player) && (*hptr == '$')) loghack = 1;
           substitute_command(player,params,command);
           for(q = command; *q; q++) switch(*q) {
               case '\x01':
                    if(!((q > command) && (*(q - 1) == '\x05'))) *q = '$';
                    break;
               case '\x02':
                    if(!((q > command) && (*(q - 1) == '\x05'))) *q = ' ';
                    break;
               case '\x03':
                    if(!((q > command) && (*(q - 1) == '\x05'))) *q = '=';
                    break;
	   }
	}

        if(loghack)
           writelog(HACK_LOG,1,"HACK","Possible substitution hack in compound command %s(#%d) owned by %s(#%d), executed by %s(#%d) under the ID of %s(#%d):  %s",getname(current_command),current_command,getname(db[current_command].owner),db[current_command].owner,getname(player),player,getname(Owner(player)),Owner(player),command);

        if(test) {
           if(((!*(command + 1) || (*(command + 1) == ' ')) && (*command == '0')) || !strcasecmp(command,ERROR) || !strcasecmp(command,UNSET_VALUE)) {
              setreturn(ERROR,COMMAND_FAIL);
	   } else {
              setreturn(OK,COMMAND_SUCC);
	   }
	}
     } else strcpy(command,original_command);

     /* ---->  Censor bad language on command-line  <---- */
     if(!compoundonly && (Censor(player) || Censor(db[player].location)))
        if(!(in_command && Valid(current_command) && !Censor(current_command)))
           bad_language_filter(command,command);

     /* ---->  Trace sequence  <---- */
     if(traced) {
        for(shortcmd = 1, ptr = params; (shortcmd < nesting_level) && (shortcmd < 33); *ptr++ = ' ', shortcmd++);
        *ptr = '\0', shortcmd = 0;
        sprintf(buffer,ANSI_DCYAN"("ANSI_LCYAN"%03d"ANSI_DCYAN")  "ANSI_LYELLOW"%s%s  ",current_line_number,params,String(command));
        tracecmd = (char *) alloc_string(buffer);
     }

     if(!test) {

        /* ---->  Match for short command (I.e:  '"' for 'say', '+' for 'think', etc.)  <---- */
        buffer[0] = *command, buffer[1] = '\0';
        if(!compoundonly && (cmd = search_cmdtable(buffer,short_cmds,short_table_size)) && !(converse && (cmd->flags & CMD_NOCONV))) {
           if(!(command_type & (cmd->flags & CMD_MASK))) {
              command_type = BAD_COMMAND;
              setreturn(ERROR,COMMAND_FAIL);
	   } else cmd->func(player,command + 1,buffer,command,"",cmd->val1,cmd->val2);

        /* ---->  Match for exit name, compound command or built-in command  <---- */
	} else if(!((*command != COMMAND_TOKEN) && !converse && !compoundonly && (command_type & OTHER_COMMAND) && move_character(player,command,NULL,NULL,NULL,1,0))) {

            /* ---->  Parse arguments  <---- */
            /*          Assign arg0          */
            arg0 = command;

            /* ---->  Get arg1...  <---- */
            arg1 = command;
            if(*arg1 && !(shortcmd = ((*arg1 == *TELL_TOKEN) || (*arg1 == *REMOTE_TOKEN) || (*arg1 == *ALT_TELL_TOKEN) || (*arg1 == *ALT_REMOTE_TOKEN)))) {
               while(*arg1 && !((*arg1 == '\n') || (*arg1 == ' ') || (*arg1 == '='))) arg1++;
               if(*arg1 != '=') {
                  if(*arg1) *arg1++ = '\0';
                  for(; *arg1 && (*arg1 == ' '); arg1++);
                  strcpy(params,arg1);
	       } else strcpy(params,arg1);
	    } else if(*arg1) {
               arg1++;
               if(*arg1 && (*arg1 == ' ')) *arg1++ = '\0'; 
               for(; *arg1 && (*arg1 == ' '); arg1++);
               strcpy(params,arg1);
	    } else *params = '\0';

            /* ---->  Find end of arg1, start of arg2  <---- */
            for(arg2 = arg1; *arg2 && (*arg2 != '='); arg2++);
            for(ptr = (arg2 - 1); (ptr >= arg1) && (*ptr == ' '); ptr--) *ptr = '\0';

            /* ---->  Go past '=' and leading blank space if present  <---- */
            if(*arg2) *arg2++ = '\0';
            while(*arg2 && (*arg2 == ' ')) arg2++;

            /* ---->  Strip trailing blank space from arg2  <---- */
            for(ptr = arg2; *ptr; ptr++);
            for(ptr = (arg2 - 1); (ptr >= arg2) && (*ptr == ' '); ptr--) *ptr = '\0';

            cache = current_command;
            if(*arg0 != ABSOLUTE_TOKEN) {
               if((*arg0 != COMMAND_TOKEN) && !converse && (command_type & OTHER_COMMAND)) {
                  if(shortcmd) {
                     char *cached_arg0   = arg0;
                     char *cached_arg1   = arg1;
                     char *paramptr      = params;
                     char *cacheptr      = NULL;
                     char cache;

                     for(; *paramptr && (*paramptr != ' '); paramptr++);
                     for(; *paramptr && (*paramptr == ' '); paramptr++);
                     for(; *arg1 && (*arg1 != ' '); arg1++);
                     if(*arg1) {
                        cache    = *arg1;
                        cacheptr = arg1;
                        *arg1++  = '\0';
		     }

                     for(; *arg1 && (*arg1 == ' '); arg1++);
		     if(!(!in_command && (*arg0 == '.'))) {
                        if(!command_can_execute(player,arg0,arg1,arg2,paramptr)) {
                           if(compoundonly) output(p,player,0,1,0,ANSI_LGREEN"Sorry, the compound command '"ANSI_LWHITE"%s"ANSI_LGREEN"' cannot be found (This was executed by the link you clicked on.)",arg0), executed = 1;
                           arg0 = cached_arg0;
                           arg1 = cached_arg1;
                           if(cacheptr) *cacheptr = cache;
			} else executed = 1;
		     } else {
                        arg0 = cached_arg0;
                        arg1 = cached_arg1;
                        if(cacheptr) *cacheptr = cache;
		     }
		  } else if(!(!in_command && (*arg0 == '.'))) {
                     if(command_can_execute(player,arg0,arg1,arg2,params)) {
                        executed = 1;
		     } else if(compoundonly) {
                        output(p,player,0,1,0,ANSI_LGREEN"Sorry, the compound command '"ANSI_LWHITE"%s"ANSI_LGREEN"' cannot be found (This was executed by the link you clicked on.)",arg0);
                        executed = 1;
		     }
		  }
	       }
	    } else absolute = 1, arg0++;

            if(converse && !compoundonly) {
  	       if(*arg0 != CONVERSE_CMD_TOKEN) {
 
                  /* ---->  'Converse' mode  <---- */
	          if(strcasecmp("help",arg0) && strcasecmp("man",arg0) && strcasecmp("manual",arg0)) {
     	             if(strcasecmp("converse",arg0) && strcasecmp("conv",arg0) && strcasecmp("conversemode",arg0)) {
                        sprintf(command + strlen(command)," %s",params);
                        if(*command == *ALT_POSE_TOKEN) comms_pose(player,command + 1,NULL,NULL,NULL,0,0);
                           else comms_say(player,command,NULL,NULL,NULL,0,0);
		     } else comms_converse(player,NULL,NULL,NULL,NULL,0,0);
		  } else help_main(player,params,arg0,arg1,arg2,0,0);
                  executed = 2;
	       } else {

                  /* ---->  Execute TCZ command in 'converse' mode  <---- */
                  for(ptr = arg0 + 1; *ptr && (*ptr != ' '); ptr++);
                  *ptr = '\0';
                  if(!strcasecmp("x",arg0 + 1) || string_prefix("execute",arg0 + 1)) {
                     for(ptr = params; *ptr && (*ptr != ' '); ptr++);
                     for(; *ptr && (*ptr == ' '); ptr++);
		  } else ptr = params;
                  if(Blank(ptr)) {
	             output(p,player,0,1,0,ANSI_LGREEN"Which %s command would you like to execute?",tcz_short_name);
                     setreturn(ERROR,COMMAND_FAIL);
		  } else {
                     unsigned char abort = 0;

                     abort |= event_trigger_fuses(player,player,ptr,FUSE_ARGS);
                     abort |= event_trigger_fuses(player,Location(player),ptr,FUSE_ARGS);
                     if(!abort) process_basic_command(player,ptr,0);
		  }
                  executed = 2;
	       }
	    }

            if(!executed && !compoundonly) {

               /* ---->  Choose appropriate command table  <---- */
               if(arg0[0] == COMMAND_TOKEN) {
                  if(arg0[1] == '?') cmd = search_cmdtable(arg0,query_cmds,query_table_size);
                     else cmd = search_cmdtable(arg0,at_cmds,at_table_size);
	       } else if(shortcmd) {
                  buffer[0] = *arg0, buffer[1] = '\0';
                  cmd = search_cmdtable(buffer,general_cmds,general_table_size);
	       } else if(!in_command || (command_type == BBS_COMMAND)) {
                  cmd = ((command_type == BBS_COMMAND) || (!absolute && (db[player].location == bbsroom))) ? search_cmdtable(arg0,bbs_cmds,bbs_table_size):NULL;
                  if(!cmd) cmd = ((command_type == BANK_COMMAND) || (!absolute && (db[player].location == bankroom))) ? search_cmdtable(arg0,bank_cmds,bank_table_size):NULL;
                  if(!cmd) cmd = search_cmdtable(arg0,general_cmds,general_table_size);
	       } else {
                  cmd = search_cmdtable(arg0,general_cmds,general_table_size);
                  if(!cmd && (db[player].location == bbsroom))  cmd = search_cmdtable(arg0,bbs_cmds,bbs_table_size);
                  if(!cmd && (db[player].location == bankroom)) cmd = search_cmdtable(arg0,bank_cmds,bank_table_size);
	       }

               /* ---->  Command found in appropriate command table?  <---- */
               if(cmd) {
                  if(command_type & (cmd->flags & CMD_MASK)) {
                     if((cmd->flags & CMD_EXACT) && (strlen(arg0) != strlen(cmd->name))) {
                        output(p,player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"%s"ANSI_LGREEN"' must be typed in full.",cmd->name);
                        setreturn(ERROR,COMMAND_FAIL);
		     } else cmd->func(player,params,arg0,arg1,arg2,cmd->val1,cmd->val2);
		  } else {
                     command_type = BAD_COMMAND;
                     setreturn(ERROR,COMMAND_FAIL);
		  }
	       } else {
                  if(!(in_command && (command_type != BBS_COMMAND))) {
                     time_t total,now;

                     if(command_type != BBS_COMMAND) {
                        gettime(now);
                        total = db[player].data->player.totaltime + (now - db[player].data->player.lasttime);
                        if((Start(Location(player)) || (total <= NEWBIE_TIME)) && !Level4(Owner(player)) && !Experienced(Owner(player)) && !Assistant(Owner(player)) && !Retired(Owner(player)))
                           output(p,player,0,1,0,ANSI_LGREEN"\nSorry, the command '"ANSI_LWHITE"%s"ANSI_LGREEN"' is unknown, %s"ANSI_LYELLOW"%s"ANSI_LGREEN".\n\nPlease type "ANSI_LWHITE""ANSI_UNDERLINE"TUTORIAL NEWBIE"ANSI_LGREEN" to read our tutorial for new users and "ANSI_LWHITE""ANSI_UNDERLINE"HELP CHATTING"ANSI_LGREEN" for a summary of commands used to talk to other users.\n\n"ANSI_LRED"If you're really stuck, type "ANSI_LYELLOW""ANSI_UNDERLINE"ASSIST"ANSI_LRED" and an administrator will be along to help you shortly.\n",arg0,Article(player,LOWER,DEFINITE),getcname(NOTHING,player,0,0));
                              else output(p,player,0,1,2,ANSI_LGREEN"Unknown command '"ANSI_LWHITE"%s"ANSI_LGREEN"', %s"ANSI_LYELLOW"%s"ANSI_LGREEN". \016&nbsp;\016 "ANSI_LBLUE"(Type "ANSI_LCYAN""ANSI_UNDERLINE"HELP"ANSI_LBLUE" for help!)\n",arg0,Article(player,LOWER,DEFINITE),getcname(NOTHING,player,0,0));
		     } else output(p,player,0,1,2,ANSI_LGREEN"Sorry, the BBS command '"ANSI_LWHITE"%s"ANSI_LGREEN"' is unknown, %s"ANSI_LYELLOW"%s"ANSI_LGREEN". \016&nbsp;\016 "ANSI_LBLUE"(Type "ANSI_LCYAN""ANSI_UNDERLINE"HELP BBS"ANSI_LBLUE" for help!)\n",arg0,Article(player,LOWER,DEFINITE),getcname(NOTHING,player,0,0));
		  } else output(p,player,0,1,2,ANSI_LRED"Unknown command '"ANSI_LWHITE"%s%s%s"ANSI_LRED"' in line "ANSI_LWHITE"%d"ANSI_LRED" of "ANSI_LYELLOW"%s"ANSI_LRED" (This was expanded from '"ANSI_LWHITE"%s"ANSI_LRED"'.)\n",command,!Blank(params) ? " ":"",params,current_line_number,unparse_object(player,current_command,0),original_command);

                  /* ---->  Log failed commands  <---- */
                  strcpy(buffer,unparse_object(ROOT,Location(player),0));
                  if(!can_write_to(player,Owner(Location(player)),0))
                     writelog(UserLog(Owner(Location(player))),1,"UNKNOWN COMMAND","(%s in %s)  >>>>>  %s%s%s",getcname(ROOT,player,1,0),buffer,command,!Blank(params) ? " ":"",params);

                  if(option_loglevel(OPTSTATUS) == 1) {
                     sprintf(buffer + strlen(buffer)," owned by %s",getcname(ROOT,Owner(Location(player)),1,0));
                     writelog(COMMAND_LOG,1,"UNKNOWN COMMAND","(%s in %s)  >>>>>  %s%s%s",getcname(ROOT,player,1,0),buffer,command,!Blank(params) ? " ":"",params);
		  }
                  setreturn(ERROR,COMMAND_FAIL);
	       }
	    }
	}

        /* ---->  '{@?tczlink}' query command being used to execute non-compound command  <---- */
        if(!executed && compoundonly)
           output(p,player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"{@?tczlink}"ANSI_LGREEN"' can only be used to execute compound commands.");

        /* ---->  Unset return value returned by internal command?  <---- */
        if(!command_result) {
           if(in_command) writelog(BUG_LOG,1,"BUG","(Process_basic_command() in interface.c)  Command '%s' returned NULL return value within compound command %s(#%d).",original_command,getname(current_command),current_command);
              else writelog(BUG_LOG,1,"BUG","(Process_basic_command() in interface.c)  Command '%s' returned NULL return value.",original_command);
           setreturn(ERROR,command_boolean);
	}
     }

     /* ---->  Trace sequence (Expansion)  <---- */
     if(traced) output_trace(player,current_command,0,1,6 + nesting_level,"%s%s",String(tracecmd),(command_boolean == COMMAND_SUCC) ? ANSI_LGREEN"("ANSI_UNDERLINE"SUCC"ANSI_LGREEN")":ANSI_LRED"("ANSI_UNDERLINE"FAIL"ANSI_LRED")");
     FREENULL(tracecmd);
     current_character = cached_curchar;
     current_cmdptr    = cached_curcmdptr;
     if((nesting_level > 0) && ((nesting_level <= MAX_NESTED_MORTAL) || ((nesting_level <= MAX_NESTED_ADMIN) && Level4(Owner(player))))) nesting_level--;
}

/* ---->  Check character, owner and location is valid  <---- */
unsigned char sanitise_character(dbref player)
{
	 if(!Validchar(player)) {
	    writelog(BUG_LOG,1,"BUG","(sanitise_character() in interface.c)  Invalid character %s.  [ABORTED]",unparse_object(ROOT,player,0));
	    return(0);
	 }
	 if(!Validchar(Owner(player))) {
	    writelog(BUG_LOG,1,"BUG","(sanitise_character() in interface.c)  %s's owner (%s(#%d)) is invalid.  [CORRECTED]",getcname(ROOT,player,1,0),getname(Owner(player)),Owner(player));
	    db[player].owner = player;
	 }
	 if(!Valid(Location(player)) || ((Typeof(Location(player)) != TYPE_ROOM) && (Typeof(Location(player)) != TYPE_THING))) {
	    writelog(BUG_LOG,1,"BUG","(sanitise_character() in interface.c)  Character %s is in an invalid location (%s(#%d).)  [CORRECTED]",getcname(ROOT,player,1,0),getname(Location(player)),Location(player));
	    db[player].location = ROOMZERO;
	 }
	 return(1);
}

/* ---->  Execute command typed by user  <---- */
void tcz_command(struct descriptor_data *d,dbref player,const char *command)
{
     const    char *ptr = command;
     unsigned char abort = 0;
     time_t        now;

     gettime(now);
     if(!sanitise_character(player)) return;
     db[player].owner = player;

     d->flags2 &= ~OUTPUT_SUPPRESS;
     for(; *ptr && (*ptr == ' '); ptr++);

     /* ---->  Log top-level commands, breeching user privacy (For debugging use only)  <---- */
     if(!Blank(ptr) && (option_loglevel(OPTSTATUS) >= 3) && !in_command)
        writelog(COMMAND_LOG,1,"COMMAND","(%s(#%d) (%s) in %s)  >>>>>  %s",getname(player),player,IsHtml(d) ? "HTML":"Telnet",unparse_object(ROOT,Location(player),0),ptr);

     /* ---->  Emergency command logging on user  <---- */
     if(!Blank(ptr) && (now < d->emergency_time) && !in_command)
        writelog(EMERGENCY_LOG,1,"EMERGENCY","(%s(#%d) (%s) in %s)  >>>>>  %s",getname(player),player,IsHtml(d) ? "HTML":"Telnet",unparse_object(ROOT,Location(player),0),ptr);

     /* ---->  Monitor on user  <---- */
     if(d->monitor && !Blank(ptr))
        if(!Blank(ptr)) {
           if(d->flags & MONITOR_CMDS) {
              int cached_command_type = command_type;

              wrap_leading = 11;
              command_type |= NO_AUTO_FORMAT;
              sprintf(scratch_return_string,ANSI_LBLUE"[MONITOR] \016&nbsp;\016 "ANSI_DCYAN"(%s"ANSI_LWHITE"%s"ANSI_DCYAN" in ",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0));
              sprintf(scratch_return_string + strlen(scratch_return_string),"%s"ANSI_LYELLOW"%s"ANSI_DCYAN") \016&nbsp;\016 "ANSI_LGREEN"%s\n",Article(db[player].location,LOWER,INDEFINITE),unparse_object(d->monitor->player,db[player].location,0),ptr);
              output_queue_string(d->monitor,scratch_return_string,0);
              command_type = cached_command_type;
              wrap_leading = 0;
	   }
           writelog(MONITOR_LOG,1,"MONITOR","(%s(#%d) (%s) in %s)  >>>>>  %s",getname(player),player,IsHtml(d) ? "HTML":"Telnet",unparse_object(ROOT,Location(player),0),ptr);
	}

     /* ---->  Update hourly payment counter and zero idle time of user  <---- */
     finance_payment(d);

     /* ---->  Trigger fuses attached to character and in their current location  <---- */
     db[player].lastused = now;
     toplevel_commands++;
     command_reset(player);
     command_line = command;
     setreturn(UNSET_VALUE,COMMAND_INIT);
     abort |= event_trigger_fuses(player,player,command,(d->flags & CONVERSE) ? (FUSE_ARGS|FUSE_CONVERSE):FUSE_ARGS);
     if(!d->pager && !d->edit)
        abort |= event_trigger_fuses(player,Location(player),command,(d->flags & CONVERSE) ? (FUSE_ARGS|FUSE_CONVERSE):FUSE_ARGS);
     sanitise_grouprange();

     /* ---->  Execute command user typed  <---- */
     command_reset(player);
     if(!abort) {
        if(d->pager) {

           /* ---->  'more' pager command  <---- */
           struct descriptor_data *monitor = d->monitor;

           d->monitor = NULL;
           pager_process(d,command);
           d->monitor = monitor;
	} else if(d->edit) {

           /* ---->  Editor command  <---- */
	   edit_process_command(d,(char *) command);
	} else if(d->prompt) {

           /* ---->  Interactive '@prompt' session input  <---- */
           prompt_interactive_input(d,(char *) command);
	} else if(d->cmdprompt) {

           /* ---->  Command arguments prompt session input  <---- */
           prompt_user_input(d,(char *) command);

           /* ---->  Normal TCZ command  <---- */
	} else process_basic_command(player,(char *) command,((d->flags & CONVERSE) != 0));
     }
     sanitise_grouprange();

     /* ---->  Log top-level command, upholding user privacy (Spoken commands, etc. will not be logged)  <---- */
     if(!Blank(ptr) && (option_loglevel(OPTSTATUS) >= 2) && !in_command)
	if(!(((d->flags & SPOKEN_TEXT) || ((d->flags & ABSOLUTE) && !(d->flags2 & ABSOLUTE_OVERRIDE)))))
  	   if(option_loglevel(OPTSTATUS) == 2)
              writelog(COMMAND_LOG,1,"COMMAND","(%s(#%d) (%s) in %s)  >>>>>  %s",getname(player),player,IsHtml(d) ? "HTML":"Telnet",unparse_object(ROOT,Location(player),0),ptr);

     /* ---->  Temporary variables still exist?  <---- */
     if(tempptr) {
        writelog(BUG_LOG,1,"BUG","(tcz_command() in interface.c)  Temporary variables still exist at end of processing command-line command ('%s' from %s(#%d).)",String(command),getname(player),player);
        temp_clear(&tempptr,NULL);
     }

     /* ---->  Character not owned by themselves  <---- */
     if(db[player].owner != player) {
        writelog(BUG_LOG,1,"BUG","(tcz_command() in interface.c)  Character isn't owned by itself.");
        db[player].owner = player;
     }
     security = 0, compoundonly = 0;
     d->flags2 &= ~OUTPUT_SUPPRESS;
     setreturn(UNSET_VALUE,COMMAND_INIT);
}

/* ---->  Update last time connected (1), total time connected (2) or both (3) of given character  <---- */
void update_lasttotal(dbref player,int update)
{
     time_t total,last,now = -1;

     /* ---->  Update total/longest time connected?  <---- */
     if(update & 2) {
        if(now < 0) gettime(now);
        last = (db[player].data->player.lasttime == 0) ? now:db[player].data->player.lasttime;
        db[player].data->player.totaltime += (now - last);

        if((total = (now - db[player].data->player.lasttime)) == now) total = 0;
        if(db[player].data->player.longesttime < total) {
           db[player].data->player.longesttime = total;
           db[player].data->player.longestdate = now;
	}
     }

     /* ---->  Update last time connected  <---- */
     if(update & 1) {
        if(now < 0) gettime(now);
        db[player].data->player.lasttime = now;
     }
}

/* ---->  Update last time connected/total time of all connected characters  <---- */
void update_all_lasttotal()
{
     struct descriptor_data *d;

     for(d = descriptor_list; d; d = d->next)
         if(Validchar(d->player) && (server_count_connections(d->player,1) == 1)) {
            update_lasttotal(d->player,3);
            db[d->player].flags2 &= ~(CONNECTED|CHAT_OPERATOR|CHAT_PRIVATE|HTML);
            d->flags &= ~CONNECTED;
	 } else d->flags &= ~CONNECTED;
}

/* ---->  Connect character  <---- */
void tcz_connect_character(struct descriptor_data *d,dbref player,int create)
{
     dbref    command,cached_owner,cached_chpid;
     struct   descriptor_data *p;
     unsigned char number;
     short    cc;

     if(((cc = server_count_connections(player,1)) == 1) || (db[player].data->player.lasttime == 0)) {
        update_lasttotal(player,1);
        d->start_time = db[player].data->player.lasttime;
     } else gettime(d->start_time);
     server_sort_descriptor(d);
     if(cc == 1) db[player].flags2 &= ~(CHAT_OPERATOR|CHAT_PRIVATE);
     db[player].flags2 |=  CONNECTED;
     db[player].flags  &= ~ASHCAN;

     /* ---->  Inherit time of next command execute from existing (Still connected) descriptor  <---- */
     for(p = descriptor_list; p; p = p->next)
         if((p->player == player) && (p->flags & CONNECTED) && (p->next_time > d->next_time))
            d->next_time = p->next_time;
     d->next_time = 0 - d->next_time;

     /* ---->  Execute '.login' compound commands in area  <---- */
     setreturn(UNSET_VALUE,COMMAND_INIT);
     command = match_object(d->player,Location(d->player),NOTHING,".login",MATCH_PHASE_AREA,MATCH_PHASE_GLOBAL,SEARCH_COMMAND,MATCH_OPTION_DEFAULT_AREA_COMMAND,SEARCH_ALL,NULL,0);
     if(Valid(command) && Executable(command)) {
        number              = Number(d->player);
        cached_owner        = db[d->player].owner;
        cached_chpid        = db[d->player].data->player.chpid;
        command_type       |= AREA_CMD;
        if((d->player != db[command].owner) && !Level4(d->player)) db[d->player].flags |= NUMBER;
        if(!(Wizard(command) || Level4(db[command].owner))) db[d->player].owner = db[d->player].data->player.chpid = db[command].owner;
        command_clear_args();
        command_arg0 = ".login";
        command_cache_execute(d->player,command,1,1);
        command_type                    &= ~AREA_CMD;
        if(!number) db[d->player].flags &= ~NUMBER;
        db[d->player].owner              = cached_owner;
        db[d->player].data->player.chpid = cached_chpid;
     }
    match_done();

     /* ---->  Execute character's personal '.login' compound command  <---- */     
     command = match_simple(d->player,".login",COMMANDS,0,1);
     if(Valid(command) && Executable(command)) {
        number              = Number(d->player);
        cached_owner        = db[d->player].owner;
        cached_chpid        = db[d->player].data->player.chpid;
        if((d->player != db[command].owner) && !Level4(d->player)) db[d->player].flags |= NUMBER;
        if(!(Wizard(command) || Level4(db[command].owner))) db[d->player].owner = db[d->player].data->player.chpid = db[command].owner;
        command_clear_args();
        command_arg0 = ".login";
        command_cache_execute(d->player,command,1,0);
        if(!number) db[d->player].flags &= ~NUMBER;
        db[d->player].owner              = cached_owner;
        db[d->player].data->player.chpid = cached_chpid;
     }

     /* ---->  Show Message(s) Of The Day (MOTD)  <---- */
     look_motd(player,NULL,NULL,NULL,NULL,create,0);

     /* ---->  Increase character's total number of logins  <---- */
     if(cc == 1) db[player].data->player.logins++;
     if(!d || (d->clevel > 0)) return;
}

/* ---->  Disconnect character  <---- */
void tcz_disconnect_character(struct descriptor_data *d)
{
     int      connections = server_count_connections(d->player,1);
     dbref    command,cached_owner,cached_chpid;
     unsigned char number;

     /* ---->  Execute '.logout' compound commands in area  <---- */
     if(connections <= 1) {
        setreturn(UNSET_VALUE,COMMAND_INIT);
        command = match_object(d->player,Location(d->player),NOTHING,".logout",MATCH_PHASE_AREA,MATCH_PHASE_GLOBAL,SEARCH_COMMAND,MATCH_OPTION_DEFAULT_AREA_COMMAND,SEARCH_ALL,NULL,0);
        if(Valid(command) && Executable(command)) {
           number              = Number(d->player);
           cached_owner        = db[d->player].owner;
           cached_chpid        = db[d->player].data->player.chpid;
           command_type       |= AREA_CMD;
           if((d->player != db[command].owner) && !Level4(d->player)) db[d->player].flags |= NUMBER;
           if(!(Wizard(command) || Level4(db[command].owner))) db[d->player].owner = db[d->player].data->player.chpid = db[command].owner;
           command_clear_args();
           command_arg0 = ".logout";
           command_cache_execute(d->player,command,1,1);
           command_type                    &= ~AREA_CMD;
           if(!number) db[d->player].flags &= ~NUMBER;
           db[d->player].owner              = cached_owner;
           db[d->player].data->player.chpid = cached_chpid;
	}
        match_done();

        /* ---->  Execute character's personal '.logout' compound command  <---- */     
        command = match_simple(d->player,".logout",COMMANDS,0,1);
        if(Valid(command) && Executable(command)) {
           number              = Number(d->player);
           cached_owner        = db[d->player].owner;
           cached_chpid        = db[d->player].data->player.chpid;
           if((d->player != db[command].owner) && !Level4(d->player)) db[d->player].flags |= NUMBER;
           if(!(Wizard(command) || Level4(db[command].owner))) db[d->player].owner = db[d->player].data->player.chpid = db[command].owner;
           command_clear_args();
           command_arg0 = ".logout";
           command_cache_execute(d->player,command,1,0);
           if(!number) db[d->player].flags &= ~NUMBER;
           db[d->player].owner              = cached_owner;
           db[d->player].data->player.chpid = cached_chpid;
	}
     }

     /* ---->  Display boot reason message  <---- */
     if(!Blank(bootmessage)) {
        add_cr = 1, output_queue_string(d,bootmessage,0), add_cr = 0;
        *bootmessage = '\0';
     }

     if(connections <= 1) {
        update_lasttotal(d->player,3);
        db[d->player].flags2 &= ~(CONNECTED|CHAT_OPERATOR|CHAT_PRIVATE|HTML);
        db[d->player].data->player.failedlogins = 0;
     }
}

/* ---->  Hourly chiming clock  <---- */
#ifdef CHIMING_CLOCK
void clock_notify(time_t now)
{
     static char *strike[12] = {"once","twice","three times","four times","five times","six times","seven times","eight times","nine times","ten times","eleven times","twelve times"};
     static char *hours[12]  = {"one", "two",  "three",      "four",      "five",      "six",      "seven",      "eight",      "nine",      "ten",      "eleven",      "twelve"};
     struct descriptor_data *d;
     struct tm *rtime;
     time_t hour;

     for(d = descriptor_list; d; d = d->next)
         if(Validchar(d->player) && !Quiet(d->player)) {
            hour  = now + (db[d->player].data->player.timediff * HOUR);
            rtime = localtime(&hour);
            if(rtime->tm_hour > 12) rtime->tm_hour -= 12;
               else if(rtime->tm_hour == 0) rtime->tm_hour = 12;
            rtime->tm_hour--;
            output(d,d->player,0,0,0,CLOCK_PREFIX"%s"CLOCK_MIDDLE"%s"CLOCK_SUFFIX,strike[rtime->tm_hour],hours[rtime->tm_hour]);
	 }
}
#endif

/* ---->  Birthday congratulations  <---- */
void birthday_notify(time_t now,dbref birthday)
{
     unsigned long longdate = (epoch_to_longdate(now) & 0xFFFF);
     struct   descriptor_data *d;

     for(d = descriptor_list; d; d = d->next)
         if(Validchar(d->player) && !(d->flags & BIRTHDAY) && hasprofile(db[d->player].data->player.profile) && ((db[d->player].data->player.profile->dob & 0xFFFF) == longdate) && ((birthday == NOTHING) || (birthday == d->player))) {
            output(d,d->player,2,0,0,"%s",BIRTHDAY_MESSAGE);
            d->flags |= BIRTHDAY;
	 }
}

/* ---->  Character name invalid for E-mail forwarding (I.e:  'root')  <---- */
unsigned char valid_emailforward(const char *name)
{
	 int loop = 0;

	 if(Blank(name)) return(0);
	 for(loop = 0; emailforwardnames[loop]; loop++)
	     if(!strcasecmp(emailforwardnames[loop],name))
		return(0);
	 return(1);
}


/* ---->  Admin E-mail list data structure  <---- */
struct elist_data {
       struct elist_data *next;
       char   *email;
};


/* ---->  Construct and dump Admin mailing list  <---- */
void dump_adminemail(FILE *dump)
{
     struct   elist_data *start = NULL,*last,*ptr,*new;
     const    char *email,*emailaddr,*src;
     unsigned char comma = 0;
     int      count,value;
     char     *dest;
     dbref    loop;

     /* ---->  Construct list of sorted unique Admin E-mail addresses  <---- */
     for(loop = 0; loop < db_top; loop++)
         if((Typeof(loop) == TYPE_CHARACTER) && Level4(loop) && !Puppet(loop))
            if((email = getfield(loop,EMAIL)) && *email) {

               /* ---->  Get and process E-mail address  <---- */
               for(count = 1, emailaddr = NULL; (Blank(emailaddr) || !strcasecmp(emailaddr,"forward")) && (count < EMAIL_ADDRESSES); count++)
                   emailaddr = gettextfield(count,'\n',email,0,scratch_return_string);

               if(!Blank(emailaddr)) {
                  src = emailaddr, dest = scratch_buffer;
                  for(; *src && (*src == ' '); src++);
                  for(; *src && (isalnum(*src) || (*src == '.') || (*src == '-') || (*src == '_') || (*src == '@') || (*src == '!')); *dest++ = *src++);
                  *dest = '\0';

                  /* ---->  Sort into list  <---- */
                  for(ptr = start, last = NULL; ptr && ((value = strcasecmp(scratch_buffer,ptr->email)) > 0); last = ptr, ptr = ptr->next);
                  if(!(ptr && !value)) {
                     MALLOC(new,struct elist_data);
                     new->email = (char *) alloc_string(scratch_buffer);
                     new->next  = NULL;

                     if(last) {
                        new->next  = ptr;
                        last->next = new;
		     } else {
                        new->next  = start;
                        start      = new;
		     }
		  }
	       }
	    }

     /* ---->  Dump Admin E-mail list  <---- */
     strcpy(scratch_return_string,tcz_admin_email);
     if((dest = (char *) strchr(scratch_return_string,'@'))) *dest = '\0';
     fprintf(dump,"%s:",scratch_return_string);
     for(ptr = start; ptr; ptr = ptr->next)
         fprintf(dump,"%s%s",(comma) ? ",":"",ptr->email), comma = 1;

     /* ---->  Free Admin E-mail list  <---- */
     for(ptr = start; ptr; ptr = start) {
         start = ptr->next;
         FREENULL(ptr->email);
         FREENULL(ptr);
     }
}

/* ---->  Dump batch of E-mail forwarding addresses of characters in the database  <---- */
unsigned char dump_emailforward()
{
#ifdef EMAIL_FORWARDING
	 #define validemailchar(x) (isalnum(x) || (x == '.') || (x == '_') || (x == '-') || (x == '@') || (x == '!'))

	 const    char *email,*emailaddr;
         static   int  addresscount = 0;
	 static   FILE *dump = NULL;
	 unsigned char valid,host;
	 static   dbref ptr = 0;
	 int      count;
	 char     *tmp;

	 if(!option_emailforward(OPTSTATUS)) return(1);
	 if(!dump && (dump = fopen(FORWARD_FILE,"w")) == NULL) {
	    dbref ptr2;

	    for(ptr2 = 0; ptr2 < db_top; ptr2++)
		if(Typeof(ptr2) == TYPE_CHARACTER)
		   db[ptr2].flags2 &= ~FORWARD_EMAIL;
	    ptr = 0;
	    return(1);
	 }

	 for(count = 0; (count < EMAIL_FORWARD_QUANTITY) && (ptr < db_top); ptr++) {
	     if((Typeof(ptr) == TYPE_CHARACTER) && !Moron(ptr) && !Puppet(ptr) && (db[ptr].data->player.bantime != -1) && (Controller(ptr) == ptr) && !instring("guest",getname(ptr)) && (email = getfield(ptr,EMAIL)) && *email) {
                db[ptr].flags2 |= FORWARD_EMAIL;
		if(valid_emailforward(forwarding_address(ptr,0,scratch_return_string))) {
		   db[ptr].flags2 &= ~FORWARD_EMAIL;
		   emailaddr = gettextfield(1,'\n',email,0,scratch_buffer), valid = 1;
		   if(emailaddr && !strcasecmp("forward",emailaddr)) emailaddr = gettextfield(2,'\n',email,0,scratch_buffer);
		      else if(!(email = gettextfield(2,'\n',email,0,scratch_return_string)) || !*email) valid = 0;
		   sprintf(scratch_return_string,"@%s",email_forward_name);
		   if(valid && !Blank(emailaddr) && !instring(scratch_return_string,emailaddr)) {

		      /* ---->  Get valid part of E-mail address  <---- */ 
		      for(valid = 1, host = 0; *emailaddr && (*emailaddr == ' '); emailaddr++);
		      for(tmp = scratch_return_string; valid && *emailaddr && !((*emailaddr == ' ') || (*emailaddr == '\n')); *tmp++ = *emailaddr++)
			  if(!validemailchar(*emailaddr)) valid = 0;
			     else if(*emailaddr == '@') host = 1;
		      *tmp = '\0';

		      /* ---->  If valid E-mail address, write to forwarding aliases file  <---- */
		      if(valid && host && !Blank(scratch_return_string)) {
			 db[ptr].flags2 |= FORWARD_EMAIL;
			 fprintf(dump,"%s:%s\n",forwarding_address(ptr,0,scratch_buffer),scratch_return_string);
			 addresscount++;
			 count++;
		      }
		   }
		} else db[ptr].flags2 &= ~FORWARD_EMAIL;
	     }
	 }

	 if(ptr >= db_top) {
	    dump_adminemail(dump);
	    addresscount++;
	    
	    if(dump) {
	       fflush(dump);
	       fclose(dump);
	    }

	    writelog(EMAIL_LOG,1,"E-MAIL","%d E-mail forwarding address%s written to the file '"FORWARD_FILE"'.",addresscount,(addresscount == 1) ? "":"es");
	    writelog(DUMP_LOG,1,"E-MAIL","%d E-mail forwarding address%s written to the file '"FORWARD_FILE"'.",addresscount,(addresscount == 1) ? "":"es");

	    addresscount = 0;
	    dump = NULL;
	    ptr = 0;
	    return(1);
	 } else return(0);
#else
	 return(1);
#endif
}

/* ---->  Handle alarms, fuses and timing  <---- */
void tcz_time_sync(unsigned char init)
{
     static time_t bbs_check = 1,character_check = 1,object_check = 1;
     static time_t request_check = 1,email_forward = 1,warn_failed = 1;
     static time_t event_check = 1,vote_expiry = 1,clock = 1;
     static dbref player,object,command,data;
     int    execution_counter = 0;
     static char token[2];
     static time_t now;
     static char *str;

     /* ---->  Initialise timing  <---- */
     if(init > 0) {
        switch(init) {
               case 1:
                    gettime(now);

                    /* ---->  Chiming clock  <---- */
                    clock = (now / HOUR) * HOUR;
                    while(clock < now) clock += HOUR;

                    /* ---->  BBS expired messages  <---- */
                    if(bbs_check) {
                       bbs_check = ((now / DAY) * DAY) + (BBS_CHECK_HOUR * HOUR);
                       while(bbs_check < now) bbs_check += DAY;
		    }

                    /* ---->  BBS message vote expiry  <---- */
                    if(vote_expiry) {
                       vote_expiry = (now / DAY);
                       while(vote_expiry < now) vote_expiry += DAY;
		    }

                    /* ---->  Object maintenance  <---- */
                    if(object_check) {
                       object_check = ((now / WEEK) * WEEK) + (OBJ_MAINTENANCE_DAY_OFFSET * DAY) + (OBJ_MAINTENANCE_HOUR * HOUR);
                       while(object_check < now) object_check += WEEK;
		    }

                    /* ---->  Character maintenance  <---- */
                    if(character_check) {
                       character_check = ((now / WEEK) * WEEK) + (CHAR_MAINTENANCE_DAY_OFFSET * DAY) + (CHAR_MAINTENANCE_HOUR * HOUR);
                       while(character_check < now) character_check += WEEK;
		    }

                    /* ---->  Write list of E-mail forwarding address  <---- */
#ifdef EMAIL_FORWARDING
                    if(email_forward && option_emailforward(OPTSTATUS)) {
                       email_forward = ((now / WEEK) * WEEK) + (EMAIL_FORWARD_HOUR * HOUR);
                       while(email_forward < now) email_forward += WEEK;
		    }
#endif

                    /* ---->  New character requests  <---- */
                    if(request_check) {
                       request_check = ((now / DAY) * DAY) + (REQUEST_EXPIRY_HOUR * HOUR);
                       while(request_check < now) request_check += DAY;
		    }

                    /* ---->  Yearly events  <---- */
                    event_check = (now / MINUTE) * MINUTE;
                    while(event_check < now) event_check += MINUTE;

                    /* ---->  Reset WARN_LOGIN_FAILED on descriptors  <---- */
                    warn_failed = (now + (WARN_LOGIN_INTERVAL * 60));
                    break;
               case 2:
                    character_check = 0;
                    break;
               case 3:
                    object_check = 0;
                    break;
               case 4:
                    email_forward = 0;
                    break;
               case 5:
                    request_check = 0;
                    break;
	}
        return;
     }

     gettime(now);
#ifdef DEMO

     /* ---->  DEMO TCZ:  1 day up-time restriction  <---- */
     if((now - uptime) > DAY) {
        shutdown_counter = 0;
        writelog(SERVER_LOG,1,"SHUTDOWN","The demonstration version of TCZ is restricted to a maximum uptime of 1 day.  This limit has been reached  -  Shutting down...");
        sprintf(bootmessage,SYSTEM_SHUTDOWN""ANSI_LGREEN"The demonstration version of TCZ is restricted to a maximum uptime of "ANSI_LYELLOW"1 day"ANSI_LGREEN".  This limit has been reached  -  Shutting down...\n\n\n",tcz_full_name);
        return;
     }
#endif

     /* ---->  Shutdown warning messages  <---- */
     if(shutdown_counter > 0) {
        if(shutdown_timing <= now) {
           if((now - shutdown_timing) >= 60) {
              shutdown_counter--;
              if(shutdown_counter == 0) {
                 writelog(SERVER_LOG,1,"SHUTDOWN","Shutdown by %s(#%d)  -  Shutting down %s server...",getname(shutdown_who),shutdown_who,tcz_short_name);
                 sprintf(bootmessage,SYSTEM_SHUTDOWN""ANSI_LWHITE"%s"ANSI_LCYAN"%s"ANSI_LWHITE" has shutdown %s %s\n\n\n",tcz_full_name,Article(shutdown_who,UPPER,INDEFINITE),getcname(NOTHING,shutdown_who,0,0),tcz_full_name,String(shutdown_reason));
                 FREENULL(shutdown_reason);
                 return;
	      } else if(!shutdown_timing || (shutdown_counter <= 10))
                 output_all(1,1,0,0,"\007\n\x05\x09\x05\x03"ANSI_LRED"["ANSI_BLINK""ANSI_UNDERLINE"SHUTDOWN"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"%s will be going down in "ANSI_LYELLOW"%d minute%s"ANSI_LWHITE" %s\n", tcz_full_name, shutdown_counter, Plural(shutdown_counter), !Blank(shutdown_reason) ? shutdown_reason : "");
              shutdown_timing = now;
	   }
	} else shutdown_timing = -1;
     }

     /* ---->  Database dump  <---- */
#ifdef DATABASE_DUMP
     if((dumpstatus <= 0) && (now > (dumptiming + dumpinterval)))
        if(option_dumping(OPTSTATUS)) {
           dumpstatus = 1, dumptype = DUMP_SANITISE;
#ifdef DB_FORK
           if(option_forkdump(OPTSTATUS))
              dumpstatus = 254, dumptype = DUMP_NORMAL;
#endif
	}

     if(dumpstatus > 0) db_write(NULL);
#endif

     /* ---->  Hourly chiming clock  <---- */
     if(now >= clock) {

        /* ---->  Midnight  <---- */
        if(((now % DAY) / HOUR) == 0) {
           struct tm *rtime;

           /* ---->  Update current year  <---- */
           gettime(now);
           rtime = localtime(&now);
           tcz_year = (rtime->tm_year + 1900);

           /* ---->  Update TCZ statistics (Roll over to new day)  <---- */
           stats_tcz_update();
	}

        /* ---->  Chiming clock  <---- */
#ifdef CHIMING_CLOCK
        clock_notify(now);
#endif

        /* ---->  Birthday notification  <---- */
        birthday_notify(now,NOTHING);
        while(clock <= now) clock += HOUR;
     }

     /* ---->  New financial quarter?  <---- */
     if(now >= quarter) finance_quarter();

     /* ---->  Reset WARN_LOGIN_FAILED on descriptors  <---- */
     if(now >= warn_failed) {
        struct descriptor_data *p;

        for(p = descriptor_list; p; p = p->next)
            p->flags2 &= ~(WARN_LOGIN_FAILED);
     }

     /* ---->  Write list of E-mail forwarding addresses of all characters in database  <---- */
#ifdef EMAIL_FORWARDING
     if((now >= email_forward) && option_emailforward(OPTSTATUS)) {
        if(!dump_emailforward()) {
           email_forward = (now + EMAIL_FORWARD_INTERVAL);
	} else {
           while(email_forward <= now) email_forward += DAY;
	}
     }
#endif

     /* ---->  Delete out-of-date messages from BBS (In time restricted topics)  <---- */
     if(now >= bbs_check) {
        bbs_delete_outofdate();
        while(bbs_check <= now) bbs_check += DAY;
     }

     /* ---->  Remove expired requests for new characters  <---- */
     if(now >= request_check) {
        request_expired();
        while(request_check <= now) request_check += DAY;
     }

     /* ---->  Destroy unused characters  <---- */
#ifndef DEMO
     if(now >= character_check) {
        if(!admin_character_maintenance()) {
           if(!character_check) {
              character_check = ((now / WEEK) * WEEK) + (CHAR_MAINTENANCE_DAY_OFFSET * DAY) + (CHAR_MAINTENANCE_HOUR * HOUR);
              while(character_check < now) character_check += WEEK;
	   } else while(character_check <= now) character_check += WEEK;
	}
     }

     /* ---->  Destroy unused/junked objects  <---- */
     if(now >= object_check) {
        if(!admin_object_maintenance()) {
           if(!object_check) {
              object_check = ((now / WEEK) * WEEK) + (OBJ_MAINTENANCE_DAY_OFFSET * DAY) + (OBJ_MAINTENANCE_HOUR * HOUR);
              while(object_check < now) object_check += WEEK;
	   } else while(object_check <= now) object_check += WEEK;
	}
     }
#endif

     /* ---->  Update expiry time of votes on BBS messages  <---- */
     if(now >= vote_expiry) {
        bbs_update_vote_expiry();
        while(vote_expiry < now) vote_expiry += DAY;
     }

     /* ---->  Yearly events  <---- */
     if(now >= event_check) {
        yearly_event_show(NOTHING,1);
        event_check += MINUTE;
     }

     /* ---->  Pending events  <---- */
     setreturn(UNSET_VALUE,COMMAND_INIT);
     while((execution_counter++ < EVENT_PROCESSING_LIMIT) && event_pending_at(now,&player,&object,&command,&data,&str) && Valid(object)) {
           switch(Typeof(object)) {
                  case TYPE_ALARM:
                       command = db[object].destination;
                       player  = db[command].owner;
	               if(Validchar(player)) {
                          if(Valid(object)) {
                             if(Valid(command) && (Typeof(command) == TYPE_COMMAND)) {
                                if(option_loglevel(OPTSTATUS) >= 5)
                                   writelog(COMMAND_LOG,1,"ALARM","Compound command %s(#%d) owned by %s(#%d) executed by %s(#%d) via alarm %s(#%d) owned by %s(#%d).",getname(command),command,getname(Owner(command)),Owner(command),getname(player),player,getname(object),object,getname(Owner(object)),Owner(object));

                                command_line = "";
                                command_reset(player);
                                current_character = player;
                                if(sanitise_character(player)) {
                                   command_type |=  FUSE_CMD;
                                   command_cache_execute(player,command,1,0);
                                   command_type &= ~FUSE_CMD;
				}
                                if(getfield(object,DESC) && Valid(db[object].destination))
                                   event_add(NOTHING,object,NOTHING,NOTHING,event_next_cron(getfield(object,DESC)),NULL);
                                if(dumpstatus > 0) db_write(NULL);
			     } else writelog(SERVER_LOG,1,"EVENT","Pending alarm %s(#%d):  Compound command #%d is invalid.",getname(object),object,command);
			  } else writelog(SERVER_LOG,1,"EVENT","Pending alarm %s(#%d) is invalid.",getname(object),object);
		       } else writelog(SERVER_LOG,1,"EVENT","Pending alarm %s(#%d):  Character #%d is invalid.",getname(object),object,player);
                       break;
                  case TYPE_FUSE:
	               if(Validchar(player)) {
                          if(Valid(object)) {
                             if(Valid(command) && (Typeof(command) == TYPE_COMMAND)) {
                                if(option_loglevel(OPTSTATUS) >= 5)
                                   writelog(COMMAND_LOG,1,"FUSE","Compound command %s(#%d) owned by %s(#%d) executed by %s(#%d) via STICKY fuse %s(#%d) owned by %s(#%d).",getname(command),command,getname(Owner(command)),Owner(command),getname(player),player,getname(object),object,getname(Owner(object)),Owner(object));

                                command_line = str;
                                command_reset(player);
                                current_character = player;
                                event_set_fuse_args(str,&command_arg0,&command_arg1,&command_arg2,&command_arg3,str,token,0);
                                if(sanitise_character(player)) {
                                   parent_object = Valid(data) ? data:NOTHING;
                                   db[object].flags2 |=  NON_EXECUTABLE;
                                   command_type      |=  FUSE_CMD;
                                   if(!Wizard(command) && !Level4(Owner(command))) security = 1;
                                   command_cache_execute(player,command,1,0);
                                   command_type      &= ~FUSE_CMD;
                                   db[object].flags2 &= ~NON_EXECUTABLE;
                                   security           = 0;
				}
                                if(dumpstatus > 0) db_write(NULL);
			     } else writelog(SERVER_LOG,1,"EVENT","Pending fuse %s(#%d):  Compound command #%d is invalid.",getname(object),object,command);
			  } else writelog(SERVER_LOG,1,"EVENT","Pending fuse %s(#%d) is invalid.",getname(object),object);
		       } else writelog(SERVER_LOG,1,"EVENT","Pending fuse %s(#%d):  Character #%d is invalid.",getname(object),object,player);
                       break;
                  default:
                       writelog(SERVER_LOG,1,"EVENT","Invalid object in queue of pending events:  %s(#%d).",getname(object),object);
	   }
           FREENULL(str);
     }
}
