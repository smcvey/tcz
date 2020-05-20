/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| COMMAND.C  -  Implements the fundamentals of compound commands and their    |
|               execution.                                                    |
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
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                        https://github.com/smcvey/tcz                        |
`-----------------------------------------------------------------------------'

*/


#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "descriptor_flags.h"
#include "friend_flags.h"
#include "object_types.h"
#include "objectlists.h"
#include "flagset.h"
#include "fields.h"
#include "search.h"
#include "match.h"


char   scratch_return_string[2 * BUFFER_LEN];
char   scratch_buffer[2 * BUFFER_LEN];
const  char *command_lineptr = NULL;  /*  Pointer to string containing command executed by current line of compound command  */
char   *command_return_value = NULL;  /*  Alternative return value of compound command (Set with '@returnvalue')  */
int    flow_control = FLOW_NORMAL;    /*  Used for flow control  */
int    command_type = ANY_COMMAND;    /*  Type of command(s) that may be executed by process_basic_command()  */
int    command_nesting_level = 0;     /*  Current compound command nesting level  */
struct temp_data *tempptr = NULL;     /*  Pointer to temporary variable chain  */
const  char *command_line = NULL;     /*  Pointer to string containing command executed by user on the command-line  */
dbref  current_command = NOTHING;     /*  DB ref. of current command being executed  */
char   command_item2[BUFFER_LEN];     /*  ($7)  Current item's desc/array index name/array element being processed by '@with'  */
char   command_item[BUFFER_LEN];      /*  ($6)  Current item's #ID/array element/array index name being processed by '@with'  */
dbref  command_parent = NOTHING;      /*  Current effective parent (Within compound command)  */
char   bootmessage[BUFFER_LEN];       /*  Used for boot message/shutdown message  */
char   *command_textptr = NULL;       /*  Pointer to rest of compound command (Past line currently being executed  -  Used by multi-lined versions of '@if', '@for', '@with', '@while' and '@foreach'.)  */
char   *command_result = NULL;        /*  Result ($0) of command executed  */
struct timeval command_time;          /*  Start time (In nanoseconds) of compound command  */
int    current_line_number;           /*  Current line of compound command being executed  */
short  command_timelimit;             /*  Current time limit (In seconds) of compound command  */
int    command_boolean;               /*  Return boolean of compound command (Either COMMAND_SUCC or COMMAND_FAIL)  */
int    in_command = 0;                /*  If > 0, compound command is currently being executed  */
char   *command_arg0;                 /*  Pointer to name by which compound command was executed (Used by '{@?cmdname}')  */
char   *command_arg1;                 /*  Pointer to first parameter ($1) of compound command  */
char   *command_arg2;                 /*  Pointer to second parameter ($2) of compound command  */
char   *command_arg3;                 /*  Pointer to both parameters ($3) (Including '=') of compound command  */
int    noloops = 0;                   /*  Total number of loops so far ($5)  */
int    loopno  = 0;                   /*  Current loop number ($4)  */


/* ---->  Set return value/result  <---- */
void setreturn(const char *result,int value)
{
     FREENULL(command_result);
     command_result  = (char *) malloc_string(result);
     command_boolean = value;
}

/* ---->  Return difference (In nanoseconds) between two times  <---- */
int usec_difference(struct timeval time1,struct timeval time2)
{
    return(((time1.tv_sec - time2.tv_sec) * 1000000) + (time1.tv_usec - time2.tv_usec));
}

/* ---->  Clear arg0, arg1, arg2 and arg3  <---- */
void command_clear_args(void)
{
     command_arg0 = "";
     command_arg1 = "";
     command_arg2 = "";
     command_arg3 = "";
}

/* ---->  Reset compound command related variables before executing compound command  <---- */
void command_reset(dbref player)
{
     if(Validchar(player)) {
        db[player].data->player.chpid = player;
        db[player].owner              = player;
     }

     FREENULL(command_return_value);
     gettimeofday(&command_time,NULL);
     command_nesting_level   = 0;
     current_line_number     = 0;
     command_timelimit       = STANDARD_EXEC_TIME;
     current_command         = NOTHING;
     command_textptr         = NULL;
     command_parent          = NOTHING;
     *command_item2          = '\0';
     *command_item           = '\0';
     parent_object           = NOTHING;
     nesting_level           = 0;
     flow_control            = FLOW_NORMAL;
     command_type            = ANY_COMMAND;
     command_arg0            = "";
     command_arg1            = "";
     command_arg2            = "";
     command_arg3            = "";
     in_command              = 0;
     noloops                 = 0;
     loopno                  = 0;
}

/* ---->  Execute sub-compound command, caching $0-$6, line number, boolean, etc.  <---- */
const char *command_sub_execute(dbref player,char *command,unsigned char cachebool,unsigned char cachegrp)
{
     char   *cached_arg0,*cached_arg1,*cached_arg2,*cached_arg3,*cached_citem,*cached_citem2,*cached_cmpbuf,*cached_return;
     int    cached_lineno,cached_noloops,cached_loopno,cached_boolean,cached_parent,cached_commandtype;
     struct grp_data *cached_grp = grp;
     struct grp_data newgrp;
     const  char *result;

     if(cachebool) {
        cached_return  = command_result;
        cached_boolean = command_boolean;
        command_result = NULL;
     }

     cached_commandtype = command_type;
     cached_noloops     = noloops;
     cached_cmpbuf      = (char *) alloc_string(cmpbuf);
     cached_parent      = parent_object;
     cached_loopno      = loopno;
     cached_lineno      = current_line_number;
     cached_citem2      = (char *) alloc_string(compress(command_item2,0));
     cached_citem       = (char *) alloc_string(compress(command_item,0));
     cached_arg0        = command_arg0;
     cached_arg1        = command_arg1;
     cached_arg2        = command_arg2;
     cached_arg3        = command_arg3;

     if(cachegrp) grp = (struct grp_data *) grouprange_initialise(&newgrp);
     process_basic_command(player,command,0);

     if(cachebool) {
        result          = command_result;
        command_result  = cached_return;
        command_boolean = cached_boolean;
     } else result = NULL;

     strcpy(cmpbuf,String(cached_cmpbuf));
     strcpy(command_item2,String(decompress(cached_citem2)));
     strcpy(command_item,String(decompress(cached_citem)));
     FREENULL(cached_cmpbuf);
     FREENULL(cached_citem2);
     FREENULL(cached_citem);

     if(!(flow_control & (FLOW_GOTO|FLOW_GOTO_LITERAL))) current_line_number = cached_lineno;
     command_type     = (command_type & FUSE_ABORT)|(cached_commandtype & ~FUSE_ABORT);
     parent_object    = cached_parent;
     command_arg3     = cached_arg3;
     command_arg2     = cached_arg2;
     command_arg1     = cached_arg1;
     command_arg0     = cached_arg0;
     noloops          = cached_noloops;
     loopno           = cached_loopno;
     if(cachegrp) grp = cached_grp;
     return(result);
}

/* ---->  Execute specified compound command, caching all compound command related variables  <---- */
void command_cache_execute(dbref player,dbref command,unsigned char temps,unsigned char restartable)
{
     char   *cached_arg0,*cached_arg1,*cached_arg2,*cached_arg3,*cached_citem,*cached_citem2,*cached_cmpbuf;
     int    cached_lineno,cached_noloops,cached_loopno,cached_parent,cached_commandtype;
     struct descriptor_data *p = getdsc(player);
     struct grp_data *cached_grp = grp;
     struct temp_data *cached_temp;
     struct timeval cached_time;
     short  cached_timelimit;
     struct grp_data newgrp;

     cached_time.tv_usec = command_time.tv_usec;
     cached_time.tv_sec  = command_time.tv_sec;
     cached_commandtype  = command_type;
     cached_timelimit    = command_timelimit;
     cached_noloops      = noloops;
     cached_cmpbuf       = (char *) alloc_string(cmpbuf);
     cached_parent       = parent_object;
     cached_loopno       = loopno;
     cached_lineno       = current_line_number;
     cached_citem2       = (char *) alloc_string(compress(command_item2,0));
     cached_citem        = (char *) alloc_string(compress(command_item,0));
     cached_arg0         = command_arg0;
     cached_arg1         = command_arg1;
     cached_arg2         = command_arg2;
     cached_arg3         = command_arg3;
     if(temps) cached_temp = tempptr;

     gettimeofday(&command_time,NULL);
     if(!(command_type & STARTUP_SHUTDOWN))
        command_timelimit = STANDARD_EXEC_TIME;

     if(temps) {
        if(p && p->prompt && p->prompt->temp) {
           tempptr = p->prompt->temp;
           p->prompt->temp = NULL;
	} else tempptr = NULL;
     }

     grp = (struct grp_data *) grouprange_initialise(&newgrp);
     command_execute(player,command,NULL,restartable);
     if(temps) {
        if(p && p->prompt) {
           p->prompt->temp = tempptr;
           tempptr = cached_temp;
	} else temp_clear(&tempptr,cached_temp);
     }

     strcpy(cmpbuf,String(cached_cmpbuf));
     strcpy(command_item2,String(decompress(cached_citem2)));
     strcpy(command_item,String(decompress(cached_citem)));
     FREENULL(cached_cmpbuf);
     FREENULL(cached_citem2);
     FREENULL(cached_citem);

     if(!(flow_control & (FLOW_GOTO|FLOW_GOTO_LITERAL))) current_line_number = cached_lineno;
     command_time.tv_usec = cached_time.tv_usec;
     command_time.tv_sec  = cached_time.tv_sec;
     command_timelimit    = cached_timelimit;
     parent_object        = cached_parent;
     command_type         = (command_type & FUSE_ABORT)|(cached_commandtype & ~FUSE_ABORT);
     command_arg3         = cached_arg3;
     command_arg2         = cached_arg2;
     command_arg1         = cached_arg1;
     command_arg0         = cached_arg0;
     noloops              = cached_noloops;
     loopno               = cached_loopno;
     grp                  = cached_grp;
}

/* ---->  If specified compound command can be found, execute it  <---- */
int command_can_execute(dbref player,char *arg0,char *arg1,char *arg2,char *arg3)
{
    struct   temp_data       *cached_temp = tempptr;
    int                      cached_compoundonly = compoundonly;
    unsigned char            absolute;
    dbref                    command;
    unsigned char            skip;
    struct   descriptor_data *p;

    /* ---->  Match for compound command  <---- */
    command_type &= ~MATCH_ABSOLUTE;
    if(!Blank(arg0) && strchr(arg0,':')) {
       output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't execute a compound command attached to another object (You can only execute compound commands directly by their name(s) or #ID.)");
       return(1);
    }

    command = match_object(player,player,NOTHING,arg0,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_COMMAND,MATCH_OPTION_DEFAULT_COMMAND,SEARCH_ALL,NULL,0);
    absolute = ((command_type & MATCH_ABSOLUTE) != 0);
    if(!Valid(command)) {
       match_done();
       return(0);
    }

    /* ---->  Can't execute INVISIBLE compound commands from command line  -  If found, skip and continue searching...  <---- */
    if(!absolute) while(1) {
       skip = 0;
       if(Skipobjects(Location(player)))
          if(!Wizard(command) && !Level4(Owner(command)) && (Owner(command) != player) && (Owner(command) != Owner(player)) && (Owner(command) != Owner(Location(player))))
             if(in_area(command,Location(player)) && !in_area(command,player))
                skip = 1;

       if(!skip && Executable(command) && (!Invisible(command) || (in_command && can_write_to(player,command,1)))) break;
          else {
             command = match_continue();
             if(!Valid(command)) {
                match_done();
                return(0);
	     }
	  }
    } else {
       if(Skipobjects(Location(player)))
          if(!Wizard(command) && !Level4(Owner(command)) && (Owner(command) != player) && (Owner(command) != Owner(player)) && (Owner(command) != Owner(Location(player))))
             if(in_area(command,Location(player)) && !in_area(command,player)) {
                match_done();
                return(0);
	     }

       if(!Executable(command) || !(!Invisible(command) || (in_command && can_write_to(player,command,1)))) {
          match_done();
          return(0);
       }
    }

    /* ---->  Ensure object is a compound command  <---- */
    if(!Valid(command) || (Typeof(command) != TYPE_COMMAND)) {
       match_done();
       return(0);
    }

    /* ---->  COMBAT command restrictions  <---- */
    if(Combat(command)) {
       if(!Combat(Location(player))) {
          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, combat commands can only be used within combat areas (Type '"ANSI_LWHITE"attributes"ANSI_LGREEN"' to see if you are in a combat area.)");
          return(1);
       } else if(!((Owner(command) == Owner(Location(player))) || friendflags_set(Owner(Location(player)),Owner(command),NOTHING,FRIEND_COMBAT))) {
          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that combat command can only be used within combat areas of its owner (Type '"ANSI_LWHITE"ex %s"ANSI_LGREEN"' to find out who its owner is.)",arg0);
          return(1);
       } else if(Haven(Location(player))) {
          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, combat commands cannot be used in a "ANSI_LYELLOW"HAVEN"ANSI_LGREEN" location.");
          return(1);
       }
    }

    /* ---->  Execute compound command  <---- */
    command_arg0 = arg0, command_arg1 = arg1, command_arg2 = arg2, command_arg3 = arg3;
    compoundonly = 0;
    if(could_satisfy_lock(player,command,0)) {
       command_nesting_level++;
       command_arg0 = arg0, command_arg1 = arg1, command_arg2 = arg2, command_arg3 = arg3;
       if(Valid(current_command) && (Owner(current_command) != Owner(command))) {
          short cached_timelimit = command_timelimit;

          tempptr = NULL;
          command_execute(player,command,NULL,1);
          command_timelimit = cached_timelimit;
          if((p = getdsc(player)) && p->prompt) {
             temp_clear(&(p->prompt->temp),tempptr);
             tempptr = cached_temp;
	  } else temp_clear(&tempptr,cached_temp);
       } else {
          command_execute(player,command,NULL,1);
          if(!cached_temp) {
             if((p = getdsc(player)) && p->prompt) {
                temp_clear(&(p->prompt->temp),tempptr);
                tempptr = NULL;
	     } else temp_clear(&tempptr,NULL);
	  }
       }
       if((command_nesting_level > 0) && ((command_nesting_level <= MAX_CMD_NESTED_MORTAL) || ((command_nesting_level <= MAX_CMD_NESTED_ADMIN) && Level4(Owner(player))))) command_nesting_level--;
    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't use the command '"ANSI_LWHITE"%s"ANSI_LGREEN"' (It's locked against you.)",arg0);

    compoundonly = cached_compoundonly;
    match_done();
    return(1);
}

/* ---->  Execute compound COMMAND or given sequence of COMMANDS  <---- */
void command_execute(dbref player,dbref command,const char *commands,unsigned char restartable)
{
     int      cached_flow,cached_owner,cached_chpid,cached_current_lineno,cached_parent,backslash = 0,cached_nesting_level,start_lineno = current_line_number + 1,cached_ic = in_command;
     char     *cached_arg0,*cached_arg1,*cached_arg2,*cached_arg3,*q,*cached_compound_command,*cached_textptr,*cached_cmpbuf,*cached_command_return_value;
     const    char *cached_command_lineptr = command_lineptr;
     unsigned char finished = 0,execute = 1,traced = 0,tracing = 0;
     struct   descriptor_data *p = getdsc(player);
     char     compound_command_buffer[TEXT_SIZE];
     struct   grp_data *cached_grp = grp;
     dbref    cached_current_command;
     unsigned char suppress,areacmd;
     struct   timeval currenttime;
     struct   grp_data newgrp;
     const    char *c;

     /* ---->  Max. compound command nesting level exceeded?  <---- */
     if(commands) command_nesting_level++;
     if((!Level4(Owner(player)) && (command_nesting_level > MAX_CMD_NESTED_MORTAL)) || (Level4(Owner(player)) && (command_nesting_level > MAX_CMD_NESTED_ADMIN))) {
        if(!(command_type & WARNED)) {
           output(p,player,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"Maximum compound command nesting level ("ANSI_LYELLOW"%d"ANSI_LWHITE") exceeded within compound command "ANSI_LYELLOW"%s"ANSI_LWHITE".",(Level4(db[player].owner)) ? MAX_CMD_NESTED_ADMIN:MAX_CMD_NESTED_MORTAL,unparse_object(player,command,0));
           writelog(EXECUTION_LOG,1,"RECURSION","Maximum compound command nesting level (%d) exceeded within compound command %s(#%d) by %s(#%d).",(Level4(db[player].owner)) ? MAX_CMD_NESTED_ADMIN:MAX_CMD_NESTED_MORTAL,getname(command),command,getname(player),player);
	}
        command_type |= WARNED, flow_control |= FLOW_RETURN;
        setreturn(LIMIT_EXCEEDED,COMMAND_FAIL);
        if(p) p->flags2 &= ~OUTPUT_SUPPRESS;
        return;
     }

     cached_current_command = current_command;
     if(!commands) {
        if(Valid(command) && (option_loglevel(OPTSTATUS) >= 4))
           writelog(COMMAND_LOG,1,"COMPOUND COMMAND","%s(#%d) owned by %s(#%d) executed by %s(#%d).",getname(command),command,getname(Owner(command)),Owner(command),getname(player),player);

        current_line_number = 0;
        current_command     = command;
     } else cached_cmpbuf  = (char *) alloc_string(cmpbuf);

     cached_command_return_value = command_return_value;
     command_return_value        = NULL;

     cached_textptr     = command_textptr;
     cached_parent      = command_parent;
     cached_chpid       = db[player].data->player.chpid;
     cached_owner       = db[player].owner;
     cached_arg0        = command_arg0;
     cached_arg1        = command_arg1;
     cached_arg2        = command_arg2;
     cached_arg3        = command_arg3;
     cached_flow        = flow_control;

     if(!commands)
        db[player].data->player.chpid = db[player].owner = player;

     command_parent = parent_object;
     in_command     = 1;
     areacmd        = ((command_type & AREA_CMD) != 0);

     grp = (struct grp_data *) grouprange_initialise(&newgrp);
     while(Valid(current_command) && execute) {

           /* ---->  Irreversible '@chpid'  -  Hack protection for fuses, area commands and Admin.  <---- */
           if(in_command && !Wizard(current_command) && (Level4(player) || (command_type & FUSE_CMD) || (command_type & AREA_CMD) || security))
              db[player].data->player.chpid = db[player].owner = db[current_command].owner;

           /* ---->  Match for further compound commands up the area tree  <---- */
           if(areacmd && restartable && !commands) {
              if(!Executable(current_command) || (Invisible(current_command) && !can_write_to(player,command,1)) || (RoomZero(Location(current_command)) && !Wizard(current_command)) || !could_satisfy_lock(player,current_command,0)) {
                 db[current_command].flags2 &= ~NON_EXECUTABLE;
                 current_command = match_continue();
                 continue;
	      }

              db[current_command].flags2 |= NON_EXECUTABLE;
              areacmd = 0;
	   }

           /* ---->  Trace execute sequence  <---- */
           if(!commands && (tracing = output_trace(player,current_command,0,0,0,NULL))) {
              sprintf(scratch_return_string,ANSI_LBLUE"[TRACE]  "ANSI_LYELLOW"%s"ANSI_LGREEN" executed by ",unparse_object(Owner(current_command),current_command,0));
              sprintf(scratch_return_string + strlen(scratch_return_string),"%s"ANSI_LWHITE"%s"ANSI_LGREEN,Article(player,LOWER,INDEFINITE),getcname(NOTHING,player,0,0));
              if(!traced && Valid(cached_current_command)) output_trace(player,current_command,0,1,9,"%s from within "ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_return_string,unparse_object(Owner(current_command),cached_current_command,0));
                 else output_trace(player,current_command,0,1,9,"%s in %s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_return_string,Article(Location(player),LOWER,INDEFINITE),unparse_object(Owner(current_command),Location(player),0));
              traced = 1;
	   }

           /* ---->  Compound command execution time limit exceeded?  <---- */
           compound_commands++;
           flow_control = FLOW_NORMAL;
           gettimeofday(&currenttime,NULL);
           if(usec_difference(currenttime,command_time) > (command_timelimit * 1000000)) {
              if(!areacmd && !commands && Valid(current_command)) db[current_command].flags2 &= ~NON_EXECUTABLE;
              if(!(command_type & WARNED)) {
                 output(getdsc(player),player,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"Execution time limit of "ANSI_LCYAN"%d second%s"ANSI_LWHITE" exceeded within compound command "ANSI_LYELLOW"%s"ANSI_LWHITE".",command_timelimit,Plural(command_timelimit),unparse_object(player,current_command,0));
		 if(Valid(current_command) && (player != Owner(current_command))) {
		    char buffer[BUFFER_LEN];

		    strcpy(buffer,unparse_object(Owner(current_command),current_command,1));
		    output(getdsc(Owner(current_command)),Owner(current_command),0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"Execution time limit of "ANSI_LCYAN"%d second%s"ANSI_LWHITE" exceeded by "ANSI_LYELLOW"%s"ANSI_LWHITE" within compound command "ANSI_LYELLOW"%s"ANSI_LWHITE".",command_timelimit,Plural(command_timelimit),getcname(Owner(current_command),player,1,0),buffer);
		 }
                 writelog(EXECUTION_LOG,1,"EXECUTION","Execution time limit of %d second%s exceeded by %s(#%d) within compound command %s(#%d) owned by %s(#%d).",command_timelimit,Plural(command_timelimit),getname(player),player,getname(current_command),current_command,getname(Owner(current_command)),Owner(current_command));
	      }
              command_type |= WARNED, flow_control |= FLOW_RETURN;
              if(commands) {
                 strcpy(cmpbuf,String(cached_cmpbuf));
                 FREENULL(cached_cmpbuf);
	      }

              db[player].data->player.chpid = cached_chpid;
              db[player].owner              = cached_owner;
              current_command               = cached_current_command;
              command_lineptr               = cached_command_lineptr;
              command_textptr               = cached_textptr;
              command_parent                = cached_parent;
              flow_control                  = cached_flow | (flow_control & FLOW_BREAKLOOP_ALL);
              in_command                    = cached_ic;
              grp                           = cached_grp;
              setreturn(LIMIT_EXCEEDED,COMMAND_FAIL);
              if(p) p->flags2 &= ~OUTPUT_SUPPRESS;       
              return;
	   }

           q = (commands) ? (char *) commands:(char *) getfield(current_command,DESC);
           if(!Blank(q)) {

              /* ---->  Make temporary copy of description in case compound command is modified/destroyed, parse into sub-commands and execute them  <---- */
              c = cached_compound_command = (char *) alloc_string(q);
              q = compound_command_buffer, finished = 0;
              do {
                 if(!commands) flow_control &= ~(FLOW_BREAKLOOP|FLOW_BREAKLOOP_ALL|FLOW_SKIP);
                 switch(*c) {
                        case '\\':
                             if(backslash) {
                                backslash = 0;
                                *q++ = '\\', *q++ = *c;
			     } else backslash = 1;
                             c++;
                             break;
                        case '\n':
                             if(backslash) {
                                backslash = 0;
                                *q++ = *c++;
                                break;
			     }
                        case '\0':

                             /* ---->  Otherwise it's a real newline, so fall through and execute it  <---- */
                             *q = '\0';
                             cached_current_lineno = ++current_line_number;
                             command_lineptr       = compound_command_buffer;

                             if(option_loglevel(OPTSTATUS) >= 4)
                                writelog(COMMAND_LOG,1,"SUB-COMMAND","%s",String(compound_command_buffer));

                             if(!BlankContent(compound_command_buffer)) {
                                gettimeofday(&currenttime,NULL);
                                if(usec_difference(currenttime,command_time) > (command_timelimit * 1000000)) {
                                   if(!(command_type & WARNED)) {
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
                                   finished = 1;
				} else {
                                   cached_nesting_level = nesting_level;
                                   command_textptr      = (char *) (c + 1);
                                   nesting_level        = 0;
                                   suppress             = (p && ((p->flags2 & OUTPUT_SUPPRESS) != 0));
                                   process_basic_command(player,compound_command_buffer,0);
                                   if(!(command_type & OUTPUT_CHANGE)) {
                                      if(p) {
                                         p->flags2              &= ~OUTPUT_SUPPRESS;
                                         if(suppress) p->flags2 |=  OUTPUT_SUPPRESS;
				      }
				   } else command_type &= ~OUTPUT_CHANGE;
                                   if(command_textptr != (c + 1)) {
                                      cached_current_lineno = current_line_number;
                                      c                     = command_textptr;
				   }
                                   nesting_level = cached_nesting_level;
                                   command_arg0  = cached_arg0;
                                   command_arg1  = cached_arg1;
                                   command_arg2  = cached_arg2;
                                   command_arg3  = cached_arg3;
				}
			     } else setreturn(ERROR,COMMAND_FAIL);

                             if(!finished) {
                                if(flow_control & (FLOW_GOTO|FLOW_GOTO_LITERAL)) {
                                   if(!(flow_control & FLOW_GOTO_LITERAL)) current_line_number = (cached_current_lineno + current_line_number);
                                   flow_control &= ~(FLOW_GOTO|FLOW_GOTO_LITERAL);
                                   if(current_line_number < 1) current_line_number = 1;
                                   if(commands) {
                                      if((current_line_number - start_lineno) >= 1) {
                                         current_line_number -= (start_lineno - 1);
                                         c = (char *) get_lineno(current_line_number--,(char *) cached_compound_command);
                                         if(!*c) current_line_number++, flow_control |= (FLOW_GOTO_LITERAL|FLOW_BREAKLOOP), finished = 1;
                                         current_line_number += start_lineno - 1;
				      } else flow_control |= (FLOW_GOTO_LITERAL|FLOW_BREAKLOOP), finished = 1;
			  	   } else {
                                      c = (char *) get_lineno(current_line_number--,(char *) cached_compound_command);
                                      if(!*c) finished = 1;
				   }
				} else {
                                   if(!*c) finished = 1;
                                   current_line_number = cached_current_lineno;
                                   if(*c) c++;
				}
                                q = compound_command_buffer;
			     }
                             break;
                        default:
                             if(backslash) {
                                backslash = 0;
                                *q++ = '\\';
			     }
                             *q++ = *c++;
		 }
	      } while(((flow_control & ((commands) ? FLOW_MASK_LOOP:FLOW_MASK)) == FLOW_NORMAL) && !finished);
              FREENULL(cached_compound_command);
	   } else setreturn(OK,COMMAND_SUCC);
           if(!areacmd && !commands && Valid(current_command)) db[current_command].flags2 &= ~NON_EXECUTABLE;
           if(!commands) flow_control &= ~(FLOW_BREAKLOOP|FLOW_BREAKLOOP_ALL|FLOW_GOTO|FLOW_GOTO_LITERAL|FLOW_SKIP);

           /* ---->  Link to next compound command via CSUCC/CFAIL  <---- */
           if(commands) {
              if((flow_control & FLOW_MASK) != FLOW_SKIP) {
   	         if((flow_control & FLOW_MASK) != FLOW_RETURN) {
                    if((flow_control & FLOW_MASK) > 0) {
                       if((flow_control & FLOW_BOOL) == FLOW_SUCC) {
                          if(command_result)
                             command_boolean = COMMAND_SUCC;
                                else setreturn(OK,COMMAND_SUCC);
		       } else if(command_result)
                          command_boolean = COMMAND_FAIL;
                             else setreturn(OK,COMMAND_FAIL);
		    }
  	         } else if((flow_control & FLOW_BOOL) == FLOW_SUCC) {
                    if(command_result)
                       command_boolean = COMMAND_SUCC;
                          else setreturn(OK,COMMAND_SUCC);
		 } else if((flow_control & FLOW_BOOL) == FLOW_FAIL) {
   	  	    if(command_result)
                       command_boolean = COMMAND_FAIL;
                          else setreturn(OK,COMMAND_FAIL);
		 }
	      }
              execute = 0;
	   } else if((flow_control & FLOW_MASK) != FLOW_RETURN) {
              if((flow_control & FLOW_MASK) > 0) {
                 if((flow_control & FLOW_BOOL) == FLOW_SUCC) {
                    if(command_result)
                       command_boolean = COMMAND_SUCC;
                          else setreturn(OK,COMMAND_SUCC);
		 } else if(command_result)
                    command_boolean = COMMAND_FAIL;
                       else setreturn(OK,COMMAND_FAIL);
	      }

              /* ---->  CSUCC/CFAIL link  <---- */
              switch(command_boolean) {
                     case COMMAND_FAIL:
 
                          /* ---->  Follow the failure route if possible  <---- */
                          if(getfield(current_command,FAIL)) {
                             substitute(player,scratch_return_string,(char *) getfield(current_command,FAIL),0,ANSI_LCYAN,NULL,0);
                             output(p,player,0,1,0,"%s",punctuate(scratch_return_string,2,'.'));
			  }
                          if(getfield(current_command,OFAIL) && !Invisible(db[player].location)) {
                             substitute(player,scratch_return_string,(char *) getfield(current_command,OFAIL),DEFINITE,ANSI_LCYAN,NULL,0);
                             output_except(db[player].location,player,NOTHING,0,1,2,"%s",punctuate(scratch_return_string,0,'.'));
			  }
                          if(db[current_command].exits == HOME) {
                             if(restartable) {
                                current_command = match_continue();
                                if(!Valid(current_command) || !Executable(current_command) || !could_satisfy_lock(player,current_command,0)) current_command = NOTHING;
			     } else current_command = NOTHING;
			  } else if(Valid(db[current_command].exits) && could_satisfy_lock(player,db[current_command].exits,0)) current_command = db[current_command].exits;
                             else current_command = NOTHING;
                          current_line_number = 0;
                          break;
                     case COMMAND_SUCC:

                          /* ---->  Follow the success route if possible  <---- */
                          if(getfield(current_command,SUCC)) {
                             substitute(player,scratch_return_string,(char *) getfield(current_command,SUCC),0,ANSI_LCYAN,NULL,0);
                             output(p,player,0,1,0,"%s",punctuate(scratch_return_string,2,'.'));
			  }
                          if(getfield(current_command,OSUCC) && !Invisible(db[player].location)) {
                             substitute(player,scratch_return_string,(char *) getfield(current_command,OSUCC),DEFINITE,ANSI_LCYAN,NULL,0);
                             output_except(db[player].location,player,NOTHING,0,1,2,"%s",punctuate(scratch_return_string,0,'.'));
			  }
                          if(db[current_command].contents == HOME) {
                             if(restartable) {
                                current_command = match_continue();
                                if(!Valid(current_command) || !Executable(current_command) || !could_satisfy_lock(player,current_command,0)) current_command = NOTHING;
			     } else current_command = NOTHING;
			  } else if(Valid(db[current_command].contents) && could_satisfy_lock(player,db[current_command].contents,0)) current_command = db[current_command].contents;
                             else current_command = NOTHING;
                          current_line_number = 0;
                          break;
                     default:
                          writelog(BUG_LOG,1,"BUG","(command_execute() in COMMAND.C)  Compound command %s(#%d) executed by %s(#%d) returned invalid boolean (%d.)",getname(current_command),current_command,getname(player),player,command_boolean);
                          current_command = NOTHING;
	      }

              /* ---->  Trace CSUCC/CFAIL link sequence  <---- */
              if(!commands && tracing && Valid(current_command))
                 output_trace(player,current_command,0,1,9,ANSI_LBLUE"[TRACE]  "ANSI_LGREEN"Executing %s...",(command_boolean == COMMAND_SUCC) ? ANSI_UNDERLINE"CSUCCESS"ANSI_LGREEN:ANSI_LRED""ANSI_UNDERLINE"CFAILURE"ANSI_LGREEN);
	   } else {
              if((flow_control & FLOW_BOOL) == FLOW_SUCC) {
                 if(command_result)
                    command_boolean = COMMAND_SUCC;
                       else setreturn(OK,COMMAND_SUCC);
	      } else if((flow_control & FLOW_BOOL) == FLOW_FAIL) {
                 if(command_result)
                    command_boolean = COMMAND_FAIL;
                       else setreturn(OK,COMMAND_FAIL);
	      }
              current_command = NOTHING;
	   }
           db[player].data->player.chpid = db[player].owner = player;
     }

     if(!areacmd && !commands && Valid(current_command))
        db[current_command].flags2 &= ~NON_EXECUTABLE;

     if(command_return_value) {
        FREENULL(command_result);
        command_result = (char *) malloc_string(command_return_value);
     } else command_return_value = cached_command_return_value;

     if(commands) {
        strcpy(cmpbuf,String(cached_cmpbuf));
        FREENULL(cached_cmpbuf);

        flow_control = (cached_flow | (flow_control & (FLOW_BREAK|FLOW_RETURN|FLOW_BREAKLOOP|FLOW_BREAKLOOP_ALL|FLOW_SUCC|FLOW_FAIL|FLOW_GOTO|FLOW_GOTO_LITERAL)));
     } else flow_control = (cached_flow | (flow_control & FLOW_BREAKLOOP_ALL));

     if(commands && ((command_nesting_level > 0) && ((command_nesting_level <= MAX_CMD_NESTED_MORTAL) || ((command_nesting_level <= MAX_CMD_NESTED_ADMIN) && Level4(db[player].owner))))) command_nesting_level--;
     db[player].data->player.chpid = cached_chpid;
     db[player].owner              = cached_owner;
     current_command               = cached_current_command;
     command_lineptr               = cached_command_lineptr;
     command_textptr               = cached_textptr;
     command_parent                = cached_parent;
     in_command                    = cached_ic;
     grp                           = cached_grp;

     /* ---->  Trace return sequence  <---- */
     if(!commands && traced && tracing && Valid(current_command))
        output_trace(player,current_command,0,1,9,ANSI_LBLUE"[TRACE]  "ANSI_LGREEN"Back to "ANSI_LWHITE"%s"ANSI_LGREEN"...",unparse_object(Owner(current_command),current_command,0));
     if(p) p->flags2 &= ~OUTPUT_SUPPRESS;
}

/* ---->  Execute '.action' compound command  <---- */
unsigned char command_execute_action(dbref player,dbref object,const char *name,const char *arg0,const char *arg1,const char *arg2,const char *arg3,int secure)
{
	 dbref command,csucc,cfail;

	 if(!Validchar(player)) return(0);
	 command = match_simple((object != NOTHING) ? object:player,name,COMMANDS,0,1);
	 if(Valid(command) && ((object != NOTHING) || ((Owner(command) == player) && (Location(command) == player)))) {
	    if(!Invisible(command) && Executable(command) && could_satisfy_lock(player,command,0)) {
	       char          *cached_arg0    = command_arg0;
	       char          *cached_arg1    = command_arg1;
	       char          *cached_arg2    = command_arg2;
	       char          *cached_arg3    = command_arg3;
	       unsigned char cached_security = security; 

	       /* ---->  Cache settings that will temporarily be changed  <---- */
	       command_arg0 = (char *) ((arg0) ? arg0:name);
	       command_arg1 = (char *) arg1;
	       command_arg2 = (char *) arg2;
	       command_arg3 = (char *) arg3;
	       if((csucc = db[command].contents) == HOME) db[command].contents = NOTHING;
	       if((cfail = db[command].exits)    == HOME) db[command].exits    = NOTHING;

	       /* ---->  Security options  <---- */
	       if(secure && !Wizard(command) && !Level4(Owner(command)))
		  security = secure;

	       /* ---->  Execute compound command  <---- */
	       command_nesting_level++;
	       db[command].flags2 |=  NON_EXECUTABLE;
	       command_cache_execute(player,command,1,0);
	       db[command].flags2 &= ~NON_EXECUTABLE;
	       if((command_nesting_level > 0) && ((command_nesting_level <= MAX_CMD_NESTED_MORTAL) || ((command_nesting_level <= MAX_CMD_NESTED_ADMIN) && Level4(db[player].owner)))) command_nesting_level--;

	       /* ---->  Restore cached settings  <---- */
	       db[command].contents = csucc;
	       db[command].exits    = cfail;
	       command_arg0         = cached_arg0;
	       command_arg1         = cached_arg1;
	       command_arg2         = cached_arg2;
	       command_arg3         = cached_arg3;
	       if(secure) security  = cached_security;
	       return(1);
	  }
       }
       return(0);
}

/* ---->  '@chpid'  -  Give your permissions to the person executing the compound command  <---- */
void command_chpid(CONTEXT)
{
     setreturn(ERROR,COMMAND_FAIL);
     if(!in_command) {
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@chpid"ANSI_LGREEN"' can only be used from within a compound command.");
        db[player].owner = player;
     } else {
        db[player].owner = db[current_command].owner;
        setreturn(OK,COMMAND_SUCC);
     }
}

/* ---->  Change current UID (Who you are building as)  <---- */
void command_chuid(CONTEXT)
{
     dbref who;

     setreturn(ERROR,COMMAND_FAIL);
     if(Blank(params)) params = "me";
     if(!in_command) {
        if((who = lookup_character(player,params,1)) != NOTHING) {
           if(Builder(player) || (who == player)) {
              if(can_write_to(player,who,0) || (friendflags_set(who,player,NOTHING,FRIEND_WRITE|FRIEND_CREATE) == (FRIEND_WRITE|FRIEND_CREATE))) {
                 if(who != player) output(getdsc(player),player,0,1,0,ANSI_LGREEN"You are now building as %s"ANSI_LWHITE"%s"ANSI_LGREEN"  -  To build as yourself again, please type '"ANSI_LWHITE"@chuid"ANSI_LGREEN"'.",Article(who,LOWER,DEFINITE),getcname(player,who,1,0));
		    else output(getdsc(player),player,0,1,0,(Uid(player) == player) ? ANSI_LGREEN"You are now building as yourself.":ANSI_LGREEN"You are now building as yourself again.");
                 db[player].data->player.uid = who;
                 setreturn(OK,COMMAND_SUCC);
	      } else if(Level3(player)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only build as a character who's of a lower level than yourself, or a character who has granted you '"ANSI_LWHITE"write"ANSI_LGREEN"' permission via the "ANSI_LYELLOW"WRITE"ANSI_LGREEN" friend flag and '"ANSI_LWHITE"create"ANSI_LGREEN"' permission via the "ANSI_LYELLOW"CREATE"ANSI_LGREEN" friend flag.");
                 else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only build as someone who has granted you '"ANSI_LWHITE"write"ANSI_LGREEN"' permission via the "ANSI_LYELLOW"WRITE"ANSI_LGREEN" friend flag and '"ANSI_LWHITE"create"ANSI_LGREEN"' permission via the "ANSI_LYELLOW"CREATE"ANSI_LGREEN" friend flag.");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Builders may build under the ownership of another character.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",params);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't set who you are building as from within a compound command.");
}

/* ---->  Set csuccess or cfailure of a compound command, fuse or alarm  <---- */
/*        (val1:  0 = Csuccess, 1 = Cfailure.)                                 */
void command_csucc_or_cfail(CONTEXT)
{
     dbref thing,destination;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_COMMAND,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if(HasField(thing,CSUCC)) {
        if(can_write_to(player,thing,0)) {
          if(!Readonly(thing)) {
             if(!Blank(arg2)) {
                destination = parse_link_command(player,thing,arg2,0);
                if(!Valid(destination) && (destination != HOME)) return;
	     } else destination = NOTHING;

             switch(Typeof(thing)) {
                    case TYPE_COMMAND:
                         break;
                    case TYPE_ALARM:
                    case TYPE_FUSE:
                         if(!((Typeof(thing) == TYPE_ALARM) && !val1)) {
                            if(destination == HOME) {
                               output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only compound commands can have their %s set to "ANSI_LYELLOW""HOME_STRING""ANSI_LGREEN".",(val1) ? "csuccess":"cfailure");
                               return;
			    }
                            break;
			 }
                    default:
                         output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't set the %s of %s.",(val1) ? "csuccess":"cfailure",object_type(thing,1));
                         return;
	     }

             if(val1) {
                if(Typeof(thing) == TYPE_ALARM) {
                   db[thing].destination = destination;
                   if(getfield(thing,DESC) && Valid(db[thing].destination))
                      event_add(NOTHING,thing,NOTHING,NOTHING,event_next_cron(getfield(thing,DESC)),NULL);
                         else event_remove(thing);
		} else db[thing].contents = destination;
	     } else db[thing].exits = destination;

             if(!in_command) {
                if(destination != NOTHING) {
                   sprintf(scratch_buffer,ANSI_LGREEN"%s of "ANSI_LWHITE"%s"ANSI_LGREEN" set to ",(val1) ? "Csuccess":"Cfailure",unparse_object(player,thing,0));
	           output(getdsc(player),player,0,1,0,"%s"ANSI_LYELLOW"%s"ANSI_LGREEN".",scratch_buffer,unparse_object(player,destination,0));
		} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s of "ANSI_LWHITE"%s"ANSI_LGREEN" reset.",(val1) ? "Csuccess":"Cfailure",unparse_object(player,thing,0));
	     }
             setreturn(OK,COMMAND_SUCC);
	  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is Read-Only  -  You can't change %s %s.",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0),(Typeof(thing) == TYPE_CHARACTER) ? "their":"its",(val1) ? "csuccess":"cfailure");
	} else if(Level3(db[player].owner)) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only set the %s of something you own or something owned by someone of a lower level than yourself.",(val1) ? "csuccess":"cfailure");
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only set the %s of something you own.",(val1) ? "csuccess":"cfailure");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s doesn't have a %s branch.",object_type(thing,1),(val1) ? "csuccess":"cfailure");
}

/* ---->  Set execution time limit  <---- */
void command_executionlimit(CONTEXT)
{
     unsigned char success = 0;
     int           limit;

     setreturn(ERROR,COMMAND_FAIL);
     if(in_command) {
        if((limit = atoi(params)) >= 1) {
           if(command_type & STARTUP_SHUTDOWN) {

              /* ---->  Within '.startup'/'.shutdown' compound command in #4 only  <---- */
              if(limit <= MAX_EXEC_TIME_STARTUP) {
      	         command_timelimit = limit, success = 1;
	         setreturn(OK,COMMAND_SUCC);
	      } else {
                 writelog(EXECUTION_LOG,1,"EXECUTION","Sorry, maximum execution time limit that can be set within '.startup'/'.shutdown' is %d second%s (%d is greater than this.)",MAX_EXEC_TIME_STARTUP,Plural(MAX_EXEC_TIME_STARTUP),limit);
                 writelog(SERVER_LOG,1,"EXECUTION","Sorry, maximum execution time limit that can be set within '.startup'/'.shutdown' is %d second%s (%d is greater than this.)",MAX_EXEC_TIME_STARTUP,Plural(MAX_EXEC_TIME_STARTUP),limit);
	      }
	   } else {

              /* ---->  Normal compound commands  <---- */
              if(limit <= MAX_EXEC_TIME) {
		 if((limit <= MAX_EXEC_TIME_UPPER_ADMIN) || Level1(db[player].owner)) {
		    if((limit <= MAX_EXEC_TIME_LOWER_ADMIN) || Level2(db[player].owner)) {
		       if((limit <= MAX_EXEC_TIME_MORTAL) || Level4(db[player].owner)) {
			  command_timelimit = limit, success = 1;
			  setreturn(OK,COMMAND_SUCC);
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids may set a compound command execution time limit greater than "ANSI_LWHITE"%d"ANSI_LGREEN" second%s.",MAX_EXEC_TIME_MORTAL,Plural(MAX_EXEC_TIME_MORTAL));
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Elder Wizards/Druids may set a compound command execution time limit greater than "ANSI_LWHITE"%d"ANSI_LGREEN" second%s.",MAX_EXEC_TIME_LOWER_ADMIN,Plural(MAX_EXEC_TIME_LOWER_ADMIN));
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Deities and the Supreme Being may set a compound command execution time limit greater than "ANSI_LWHITE"%d"ANSI_LGREEN" second%s.",MAX_EXEC_TIME_UPPER_ADMIN,Plural(MAX_EXEC_TIME_UPPER_ADMIN));
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the maximum compound command execution limit is "ANSI_LWHITE"%d"ANSI_LGREEN" second%s.",MAX_EXEC_TIME,Plural(MAX_EXEC_TIME));

   	      if((!Wizard(current_command) && (limit > 5)) || (limit > MAX_EXEC_TIME))
                 writelog(EXECUTION_LOG,1,"EXECUTION","%s of %d second%s %swithin compound command %s(#%d.)",(success) ? "Execution time limit":"Unsuccessful attempt to set execution time limit",limit,Plural(limit),(success) ? "set ":"",getname(current_command),current_command);
	   }
        } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"%s"ANSI_LGREEN"' isn't a valid execution time limit.",params);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@executionlimit"ANSI_LGREEN"' can only be used from within a compound command.");
}

/* ---->  Turn output within compound command on/off  <---- */
void command_output(CONTEXT)
{
     struct descriptor_data *p = getdsc(player);

     setreturn(ERROR,COMMAND_FAIL);
     if(in_command) {
        if(!strcasecmp("on",params) || !strcasecmp("yes",params)) {
           command_type    |=  OUTPUT_CHANGE;
           if(p) p->flags2 &= ~OUTPUT_SUPPRESS;
        } else if(!strcasecmp("off",params) || !strcasecmp("no",params)) {
           command_type    |=  OUTPUT_CHANGE;
           if(p) p->flags2 |=  OUTPUT_SUPPRESS;
        } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LYELLOW"on"ANSI_LGREEN"' or '"ANSI_LYELLOW"off"ANSI_LGREEN"'.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, output can only be turned on or off from within a compound command.");
}

/* ---->  Comments within compound commands  <---- */
void command_rem(CONTEXT)
{
     if(in_command) {
        if(command_boolean == COMMAND_INIT) setreturn(OK,COMMAND_SUCC);
           else if(!command_result)
              setreturn((command_boolean == COMMAND_SUCC) ? OK:ERROR,command_boolean);
     } else pagetell_send(player,NULL,NULL,arg1,arg2,1,1);  /*  If not used from within a compound command, user is typing 'rem' as a short form of 'remote'.  */
}

/* ---->  Set compound command's return value  <---- */
void command_returnvalue(CONTEXT)
{
     if(in_command) {
        FREENULL(command_return_value);
        command_return_value = (char *) malloc_string(params);
        if(command_boolean == COMMAND_INIT) setreturn(OK,COMMAND_SUCC);
           else if(!command_result)
              setreturn((command_boolean == COMMAND_SUCC) ? OK:ERROR,command_boolean);
     } else {
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@returnvalue"ANSI_LGREEN"' can only be used from within a compound command.");
        setreturn(ERROR,COMMAND_FAIL);
     }
}

/* ---->  Set return boolean  <---- */
void command_true_or_false(CONTEXT)
{
     if(val1) {
        if(command_result)
           command_boolean = COMMAND_SUCC;
              else setreturn(OK,COMMAND_SUCC);
     } else if(command_result)
        command_boolean = COMMAND_FAIL;
           else setreturn(ERROR,COMMAND_FAIL);
}

/* ---->  Remove your permissions from the person executing the compound command  <---- */
void command_unchpid(CONTEXT)
{
     setreturn(ERROR,COMMAND_FAIL);
     if(!in_command) {
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@unchpid"ANSI_LGREEN"' can only be used from within a compound command.");
        db[player].owner = player;
     } else {
        db[player].owner = db[player].data->player.chpid;
        setreturn(OK,COMMAND_SUCC);
     }
}

/* ---->  Use an object (Must have attached '.use' compound command  <---- */
void command_use(CONTEXT)
{
     char  *carg1 = "",*carg2 = "";
     dbref object;

     setreturn(ERROR,COMMAND_FAIL);
     if(Blank(arg2))
        if(!keyword("on",arg1,&arg2))
           if(!keyword("in",arg1,&arg2))
              if(!keyword("with",arg1,&arg2))
                 keyword("to",arg1,&arg2);

     if(!Blank(arg1)) {
        char cparams[TEXT_SIZE];

        object = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
        if(!Valid(object)) return;

        strcpy(cparams,String(arg2));
        split_params(arg2,&carg1,&carg2);

        if(command_execute_action(player,object,".use",NULL,carg1,carg2,cparams,0));
           else switch(Typeof(object)) {
                case TYPE_CHARACTER:

                     /* ---->  Default:  'scan' character  <---- */
                     if(1) {
                        char buffer[256];

                        sprintf(buffer,"*%s",getname(object));
                        look_scan(player,NULL,NULL,buffer,cparams,2,0);
		     }
                     break;
                case TYPE_COMMAND:

                     /* ---->  Default:  Execute compound command  <---- */
                     if(1) {
                        unsigned char abort = 0;
                        char buffer[TEXT_SIZE];

                        sprintf(buffer,"%s %s",arg1,cparams);
                        abort |= event_trigger_fuses(player,player,buffer,FUSE_ARGS);
                        abort |= event_trigger_fuses(player,Location(player),buffer,FUSE_ARGS);
                        if(!abort) process_basic_command(player,buffer,0);
		     }
                     break;
                case TYPE_THING:
                case TYPE_ROOM:

                     /* ---->  Default:  'look' at object/room  <---- */
                     if(1) {
                        char buffer[256];

                        sprintf(buffer,"%s",getname(object));
                        look_at(player,buffer,NULL,NULL,NULL,0,0);
		     }
                     break;
                case TYPE_EXIT:

                     /* ---->  Default:  Go through exit  <---- */
                     if(1) {
                        char buffer[TEXT_SIZE];
                        char *tmp;

                        strcpy(buffer,getname(object));
                        if((tmp = (char *) strchr(buffer,';'))) *tmp = '\0';
                        move_character(player,buffer,NULL,NULL,NULL,0,0);
		     }
                     break;
                default:

                     /* ---->  Object has no default usage  <---- */
	             output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, either %s"ANSI_LWHITE"%s"ANSI_LGREEN" has no use or cannot be used in that way.%s",Article(object,LOWER,DEFINITE),getcname(player,object,1,0),(Typeof(object) == TYPE_THING) ? "  For details or hints about this object's usage, try looking at it.":"");
	   }
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify which object you would like to use.");
}

/* ---->  Return name command was called by (I.e:  For multi-named compound commands)  <---- */
void command_query_cmdname(CONTEXT)
{
     setreturn((!Blank(command_arg0)) ? command_arg0:NOTHING_STRING,COMMAND_SUCC);
}

/* ---->  Return CSUCC or CFAIL of object  <---- */
/*        (val1:  0 = CSUCC, 1 = CFAIL.)         */
void command_query_csucc_or_cfail(CONTEXT)
{
     dbref thing;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(params)) thing = query_find_object(player,params,SEARCH_COMMAND,1,1);
        else if(in_command) thing = current_command;
           else return;

     if(val1) {
        if(!Valid(thing) || !HasField(thing,CFAIL)) return;
        setreturn(getnameid(player,db[thing].exits,NULL),COMMAND_SUCC);
     } else {
        if(!Valid(thing) || !HasField(thing,CSUCC)) return;
        switch(Typeof(thing)) {
               case TYPE_COMMAND:
               case TYPE_FUSE:
                    setreturn(getnameid(player,db[thing].contents,NULL),COMMAND_SUCC);
                    break;
               case TYPE_ALARM:
                    setreturn(getnameid(player,db[thing].destination,NULL),COMMAND_SUCC);
                    break;
               default:
                    writelog(BUG_LOG,1,"BUG","(command_query_csucc_or_cfail() in command.c)  Type of object %s(#%d) is unknown (0x%0X.)",getname(thing),thing,Typeof(thing));
	}
     }
}

/* ---->  VAL1:  0  -  Return total execution time so far (In microseconds)  <---- */
/*               1  -  Return execution time limit (In seconds)                    */
void command_query_execution(CONTEXT)
{
     if(!val1) {
        struct timeval currenttime;

        gettimeofday(&currenttime,NULL);
        sprintf(querybuf,"%d",usec_difference(currenttime,command_time));
     } else sprintf(querybuf,"%d",command_timelimit);
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Match and return type of command that would be executed  <---- */
void command_query_internal(CONTEXT)
{
     int    edit = 0, database = 0, general = 0, query = 0, bank = 0, bbs = 0, shrt = 0;
     struct arg_data arg;
     int    offset;
     char   *dest;

     unparse_parameters(params,1,&arg,0);
     for(params = arg.text[0]; *params && (*params == ' '); params++);
     setreturn("*NOTHING*",COMMAND_FAIL);

     if(!Blank(params)) {
        for(dest = querybuf; *params && (*params != ' '); *dest++ = *params++);
        *dest = '\0';

        if((*querybuf == '.') && search_edit_cmdtable(querybuf + 1))  edit     = 1;
        if(search_cmdtable(querybuf,at_cmds,at_table_size))           database = 1;
        if(search_cmdtable(querybuf,general_cmds,general_table_size)) general  = 1;
        if(search_cmdtable(querybuf,query_cmds,query_table_size))     query    = 1;
        if(search_cmdtable(querybuf,bank_cmds,bank_table_size))       bank     = 1;
        if(search_cmdtable(querybuf,bbs_cmds,bbs_table_size))         bbs      = 1;
        querybuf[2] = '\0';
        if(search_cmdtable(querybuf,short_cmds,short_table_size))     shrt     = 1;

        *querybuf = '\0', offset = 0;
        if(shrt)     offset += sprintf(querybuf + offset,"%sShort",(*querybuf) ? ";":"");
        if(general)  offset += sprintf(querybuf + offset,"%sGeneral",(*querybuf) ? ";":"");
        if(database) offset += sprintf(querybuf + offset,"%sDatabase",(*querybuf) ? ";":"");
        if(query)    offset += sprintf(querybuf + offset,"%sQuery",(*querybuf) ? ";":"");
        if(edit)     offset += sprintf(querybuf + offset,"%sEditor",(*querybuf) ? ";":"");
        if(bbs)      offset += sprintf(querybuf + offset,"%sBBS",(*querybuf) ? ";":"");
        if(bank)     offset += sprintf(querybuf + offset,"%sBank",(*querybuf) ? ";":"");

        if(BlankContent(querybuf)) return;
        setreturn(querybuf,COMMAND_SUCC);
     }
}

/* ---->  Return current UID of character (Who they are building as)  <---- */
void command_query_uid(CONTEXT)
{
     dbref character = query_find_character(player,params,0);
     if(!Validchar(character)) return;
     setreturn(getnameid(player,Uid(character),NULL),COMMAND_SUCC);
}


