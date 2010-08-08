/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| CONTAINER.C  -  Implements opening, closing, locking, unlocking, entering   |
|                 and leaving containers, vehicles and exits.                 |
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
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: container.c,v 1.1.1.1 2004/12/02 17:40:57 jpboggis Exp $

*/


#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "object_types.h"
#include "friend_flags.h"
#include "flagset.h"
#include "search.h"
#include "match.h"


/* ---->  Close a container/exit  <---- */
void container_close(CONTEXT)
{
     dbref  thing;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,params,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if(!((Typeof(thing) != TYPE_EXIT) && !Container(thing))) {
        if(Openable(thing)) {
           command_execute_action(player,thing,".close",NULL,getname(player),getnameid(player,thing,NULL),"",1);
           if(!Locked(thing)) {
              if(Open(thing)) {
                 if(event_trigger_fuses(player,thing,NULL,FUSE_ARGS|FUSE_COMMAND)) return;
                 db[thing].flags &= ~OPEN;
                 if(!in_command) {
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"You close %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(thing,LOWER,DEFINITE),getexit_firstname(player,thing,1));
                    if(!Invisible(db[player].location)) {
                       sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" closes ",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0));
                       output_except(db[player].location,player,NOTHING,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(thing,LOWER,(db[thing].location == db[player].location) ? DEFINITE:INDEFINITE),getexit_firstname(player,thing,0));
		    }
		 }
                 command_execute_action(player,thing,".closed",NULL,getname(player),getnameid(player,thing,NULL),"",1);
                 setreturn(OK,COMMAND_SUCC);
	      } else if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is already closed.",Article(thing,LOWER,DEFINITE),getexit_firstname(player,thing,1));
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is locked.",Article(thing,LOWER,DEFINITE),getexit_firstname(player,thing,1));
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" can't be closed.",Article(thing,LOWER,DEFINITE),getexit_firstname(player,thing,1));
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only close containers, vehicles and exits.");
}

/* ---->  Enter a container  <---- */
void container_enter(CONTEXT)
{
     dbref thing;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,params,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if(Container(thing) && Open(thing)) {
        if(!contains(thing,player)) {
           if(will_fit(player,thing)) {
              if(!Invisible(db[player].location)) {
                 sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" enters ",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0));
                 output_except(db[player].location,player,NOTHING,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(thing,LOWER,DEFINITE),getname(thing));
	      }
              move_enter(player,thing,1);
        
              if(!Invisible(thing)) {
                 char *cmd_arg0,*cmd_arg1,*cmd_arg2,*cmd_arg3;
                 char buffer[TEXT_SIZE],token[2];

                 output_except(db[player].location,player,NOTHING,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has arrived.",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0));
                 event_set_fuse_args((in_command && command_lineptr) ? command_lineptr:command_line,&cmd_arg0,&cmd_arg1,&cmd_arg2,&cmd_arg3,buffer,token,0);
                 command_execute_action(player,NOTHING,".entercmd",NULL,cmd_arg1,cmd_arg2,cmd_arg3,0);
	      }
              setreturn(OK,COMMAND_SUCC);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't %s  -  There isn't enough room for you inside it.",Vehicle(thing) ? "enter that vehicle":"climb into that container");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't %s that you're currently carrying  -  Try %s first.",Vehicle(thing) ? "enter a vehicle":"climb into a container",Vehicle(thing) ? "teleporting it elsewhere":"dropping it");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only %s that's open.",Vehicle(thing) ? "enter a vehicle":"climb into a container");
}

/* ---->  Try to leave a container  <---- */
void container_leave(CONTEXT)
{
     dbref loc;

     setreturn(ERROR,COMMAND_FAIL);
     if(Valid(db[player].location)) {
        if(Typeof(db[player].location) == TYPE_THING) {
           if(Open(db[player].location)) {
              loc = db[db[player].location].location;
              if(!Valid(loc)) {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, your current location is drifting around somewhere in the void  -  Please type "ANSI_LYELLOW"HOME"ANSI_LGREEN" to return to your home room.");
                 return;
	      } else if(Typeof(loc) == TYPE_CHARACTER) loc = db[loc].location;

              if(!friendflags_check(player,player,db[loc].owner,FRIEND_LINK,"Entry")) return;
              if(!will_fit(player,loc)) {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't get out because there isn't enough room for you outside  -  Please type "ANSI_LYELLOW"HOME"ANSI_LGREEN" to return to your home room.");
                 return;
	      }
              move_enter(player,loc,1);
     
              if(!Invisible(db[player].location)) {
                 char *cmd_arg0,*cmd_arg1,*cmd_arg2,*cmd_arg3;
                 char buffer[TEXT_SIZE],token[2];

                 output_except(db[player].location,player,NOTHING,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" has arrived.",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0));
                 event_set_fuse_args((in_command && command_lineptr) ? command_lineptr:command_line,&cmd_arg0,&cmd_arg1,&cmd_arg2,&cmd_arg3,buffer,token,0);
                 command_execute_action(player,NOTHING,".entercmd",NULL,cmd_arg1,cmd_arg2,cmd_arg3,0);
	      }
              setreturn(OK,COMMAND_SUCC);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't get out because the %s you're currently inside is closed  -  Please type '"ANSI_LWHITE"open here"ANSI_LGREEN"' to attempt to open it (If unsuccessful, you can type "ANSI_LYELLOW"HOME"ANSI_LGREEN" to return to your home room.)",Vehicle(db[player].location) ? "vehicle":"container");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only leave %s (If you're stuck in your current location, and can't get out, try typing "ANSI_LYELLOW"HOME"ANSI_LGREEN" to return to your home room.)",Vehicle(db[player].location) ? "vehicles that can be entered":"containers");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you aren't inside a container or vehicle at the moment  -  If you're stuck in a room that has no obvious exits, try typing "ANSI_LYELLOW"HOME"ANSI_LGREEN" to return to your home room.");
}

/* ---->  Lock a container/exit  <---- */
void container_lock(CONTEXT)
{
     dbref  thing;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,params,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if(!((Typeof(thing) != TYPE_EXIT) && !Container(thing))) {
        if(!((Typeof(thing) == TYPE_THING) && !Openable(thing))) {
           if(!((Typeof(thing) == TYPE_EXIT) && !Openable(thing))) {
              command_execute_action(player,thing,".lock",NULL,getname(player),getnameid(player,thing,NULL),"",1);
              if(eval_boolexp(player,getlock(thing,(Typeof(thing) == TYPE_EXIT) ? 0:1),0)) {
	         if(!Locked(thing)) {
                    if(event_trigger_fuses(player,thing,NULL,FUSE_ARGS|FUSE_COMMAND)) return;
                    db[thing].flags |= LOCKED;
                    if(!in_command) {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"You lock %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(thing,LOWER,DEFINITE),getexit_firstname(player,thing,1));
                       if(!Invisible(db[player].location)) {
                          sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" locks ",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0));
                          output_except(db[player].location,player,NOTHING,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(thing,LOWER,(db[thing].location == db[player].location) ? DEFINITE:INDEFINITE),getexit_firstname(player,thing,0));
		       }
		    }
                    command_execute_action(player,thing,".locked",NULL,getname(player),getnameid(player,thing,NULL),"",1);
                    setreturn(OK,COMMAND_SUCC);
		 } else if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is already locked.",Article(thing,LOWER,DEFINITE),getexit_firstname(player,thing,1));
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't lock %s"ANSI_LWHITE"%s"ANSI_LGREEN" without its key.",Article(thing,LOWER,DEFINITE),getexit_firstname(player,thing,1));
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't lock an exit which can't be opened/closed.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't lock a %s.",Vehicle(thing) ? "vehicle which can't be entered":"container which can't be opened/closed");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only lock containers, vehicles and exits.");
}

/* ---->  Open a container/exit  <---- */
void container_open(CONTEXT)
{
     dbref  thing;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,params,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if(!((Typeof(thing) != TYPE_EXIT) && !Container(thing))) {
        if(Openable(thing)) {
           command_execute_action(player,thing,".open",NULL,getname(player),getnameid(player,thing,NULL),"",1);
           if(!Locked(thing)) {
              if(!Open(thing)) {
                 if(event_trigger_fuses(player,thing,NULL,FUSE_ARGS|FUSE_COMMAND)) return;
                 db[thing].flags |= OPEN;
                 if(!in_command) {
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"You open %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(thing,LOWER,DEFINITE),getexit_firstname(player,thing,1));
                    if(!Invisible(db[player].location)) {
                       sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" opens ",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0));
                       output_except(db[player].location,player,NOTHING,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(thing,LOWER,(db[thing].location == db[player].location) ? DEFINITE:INDEFINITE),getexit_firstname(player,thing,0));
		    }
		 }
                 command_execute_action(player,thing,".opened",NULL,getname(player),getnameid(player,thing,NULL),"",1);
                 setreturn(OK,COMMAND_SUCC);
	      } else if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is already open.",Article(thing,LOWER,DEFINITE),getexit_firstname(player,thing,1));
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is locked.",Article(thing,LOWER,DEFINITE),getexit_firstname(player,thing,1));
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" can't be opened.",Article(thing,LOWER,DEFINITE),getexit_firstname(player,thing,1));
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only open containers, vehicles and exits.");
}

/* ---->  Unlock a container/exit  <---- */
void container_unlock(CONTEXT)
{
     dbref  thing;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,params,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
     if(!Valid(thing)) return;

     if(!((Typeof(thing) != TYPE_EXIT) && !Container(thing))) {
        if(!((Typeof(thing) == TYPE_THING) && !Openable(thing))) {
           if(!((Typeof(thing) == TYPE_EXIT) && !Openable(thing))) {
              command_execute_action(player,thing,".unlock",NULL,getname(player),getnameid(player,thing,NULL),"",1);
              if(eval_boolexp(player,getlock(thing,(Typeof(thing) == TYPE_EXIT) ? 0:1),0)) {
	         if(Locked(thing)) {
                    if(event_trigger_fuses(player,thing,NULL,FUSE_ARGS|FUSE_COMMAND)) return;
                    db[thing].flags &= ~LOCKED;
                    if(!in_command) {
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"You unlock %s"ANSI_LWHITE"%s"ANSI_LGREEN".",Article(thing,LOWER,DEFINITE),getexit_firstname(player,thing,1));
                       if(!Invisible(db[player].location)) {
                          sprintf(scratch_buffer,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" unlocks ",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0));
                          output_except(db[player].location,player,NOTHING,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_buffer,Article(thing,LOWER,(db[thing].location == db[player].location) ? DEFINITE:INDEFINITE),getexit_firstname(player,thing,0));
		       }
		    }
                    command_execute_action(player,thing,".unlocked",NULL,getname(player),getnameid(player,thing,NULL),"",1);
                    setreturn(OK,COMMAND_SUCC);
		 } else if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is already unlocked.",Article(thing,LOWER,DEFINITE),getexit_firstname(player,thing,1));
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't unlock %s"ANSI_LWHITE"%s"ANSI_LGREEN" without its key.",Article(thing,LOWER,DEFINITE),getexit_firstname(player,thing,1));
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only unlock an exit which can be opened/closed.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only unlock a %s.",Vehicle(thing) ? "vehicle which can be entered":"container which can be opened/closed");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only unlock containers, vehicles and exits.");
}
