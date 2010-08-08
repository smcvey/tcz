/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| COMMAND.H  -  Header file for COMMAND.C                                     |
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

  $Id: command.h,v 1.1.1.1 2004/12/02 17:43:13 jpboggis Exp $

*/


#ifndef __COMMAND_H
#define __COMMAND_H

#ifndef __DB_H
   #include "db.h"
#endif

#include <sys/time.h>
#include <unistd.h>


extern  char         scratch_return_string[];
extern  const char   *compound_command_line;
extern  int          command_nesting_level;
extern  char         *command_return_value;
extern  int          current_line_number;
extern  short        command_timelimit;
extern  char         scratch_buffer[];
extern  const char   *command_lineptr;
extern  char         *command_textptr;
extern  int          command_boolean;
extern  dbref        current_command;
extern  char         *command_result;
extern  char         command_item2[];
extern  char         command_item[];
extern  dbref        command_parent;
extern  struct timeval command_time;
extern  char         *command_arg0;
extern  char         *command_arg1;
extern  char         *command_arg2;
extern  char         *command_arg3;
extern  char         bootmessage[];
extern  const char   *command_line;
extern  int          flow_control;
extern  int          command_type;
extern  struct temp_data *tempptr;
extern  int          in_command;
extern  int          noloops;
extern  int          loopno;


/* ---->  Command return results  <---- */
#define OK               "OK"
#define ERROR            "Error"

#define UNSET_VALUE      "<UNSET RETURN VALUE>"
#define UNKNOWN_VARIABLE "<UNKNOWN VARIABLE>"
#define LIMIT_EXCEEDED   "<LIMIT EXCEEDED>"
#define INVALID_DATE     "<INVALID DATE>"

#define INVALID_STRING   "*INVALID*"
#define NOTHING_STRING   "*NOTHING*"
#define UNKNOWN_STRING   "*UNKNOWN*"
#define NOBODY_STRING    "*NOBODY*"
#define HOME_STRING      "*HOME*"


/* ---->  Command booleans and initial state  <---- */
#define COMMAND_INIT         2  /*  Initialial command state  */
#define COMMAND_SUCC         1  /*  Last command succeeded    */
#define COMMAND_FAIL         0  /*  Last command failed       */


/* ---->  Command types (command_type)  <---- */
#define OTHER_COMMAND        0x00000001  /*  Allows general commands to be used  */
#define AT_COMMAND           0x00000002  /*  Allows '@' commands to be used  */
#define QUERY_COMMAND        0x00000004  /*  Allows query ('@?') commands to be used  */
#define EDIT_COMMAND         0x00000008  /*  Allows editor commands to be used  */
#define BBS_COMMAND          0x00000010  /*  Allows BBS commands to be used  */
#define MAIL_COMMAND         0x00000020  /*  Allows mail commands to be used  */
#define BANK_COMMAND         0x00000040  /*  Allows bank commands to be used  */
#define STARTUP_SHUTDOWN     0x00000800  /*  '.startup' or '.shutdown' compound command currently executing  */
#define NO_AUTO_FORMAT       0x00001000  /*  Disable auto-formatting of URL's, graphical smileys, etc. on HTML Interface  */
#define MATCH_ABSOLUTE       0x00002000  /*  Absolute match to object (Cannot be continued.)  */
#define QUERY_SUBSTITUTION   0x00004000  /*  Current query command is within query substitution (I.e:  '%{@?name}'.)  */
#define OUTPUT_CHANGE        0x00008000  /*  Output status changed using '@output on|off'  */
#define COMM_CMD             0x00010000  /*  Current command is a communications command (I.e:  'say', 'page', etc.)  */
#define FUSE_CMD             0x00020000  /*  Fuse or alarm compound command currently executing  */
#define AREA_CMD             0x00040000  /*  Area compound command (Such as '.enter', '.leave', etc.) currently executing  */
#define WARNED               0x00080000  /*  Warning of exceeded execution time limit, nesting limit, recursion limit, etc. already given  */
#define NO_FLUSH_OUTPUT      0x00100000  /*  Do not flush output queue, if it exceeds normal maximum size (Used for HTML version of On-line Help System)  */
#define HTML_ACCESS          0x00200000  /*  Resources currently being accessed externally by HTML Interface (Help/Tutorials/Modules/Authors)  */
#define FUSE_ABORT           0x00400000  /*  Abort execution of command following fuse execution (Abort fuses)  */
#define NESTED_SUBSTITUTION  0x00800000  /*  Nested substitution (I.e:  '{%r%l}')  */
#define LARGE_SUBSTITUTION   0x01000000  /*  Large substitution in progress  */
#define LEADING_BACKGROUND   0x02000000  /*  Leading background text colour allowed (Used by '@echo', '@oecho', '@write' and '@notify'.)  */
#define ARITHMETIC_EXCEPTION 0x04000000  /*  Arithmetic exception raised and intercepted  */
#define NO_USAGE_UPDATE      0x08000000  /*  Used by 'examine', 'scan' and other routines where update of last usage date/time is not required  */
#define TEST_SUBST           0x10000000  /*  Used by <CONDITION> command of '@if' and '@while'  -  Forces expanded command to be tested (Similar to '@test') instead of executed  */
#define VARIABLE_SUBST       0x20000000  /*  Used by '{$<NAME>}' variable substitutions to take the whole of <NAME> as the variable name, rather than up to the first space/invalid variable substitution character  */
#define CMD_EXEC_PRIVS       0x40000000  /*  Used by match_object() to temporarily disable extra 'read' privileges from Experienced Builders/characters given 'read' privileges via the READ friend flag, so they can't execute unauthorised compound commands  */
#define BAD_COMMAND          0x80000000  /*  Set by query command substitution ('%{<QUERY COMMAND>}') if specified <QUERY COMMAND> isn't a query command, or can't be used within a query command substitution  */
#define ANY_COMMAND          0x000000FF  /*  The default:  Allows execution of ANY type of command  */


/* ---->  Command table flags  <---- */
/*                  0x0001      RESERVED:  OTHER_COMMAND  */
/*                  0x0002      RESERVED:  AT_COMMAND     */
/*                  0x0004      RESERVED:  QUERY_COMMAND  */
/*                  0x0008      RESERVED:  EDIT_COMMAND   */
/*                  0x0010      RESERVED:  BBS_COMMAND    */
/*                  0x0020      RESERVED:  MAIL_COMMAND   */
#define CMD_EXACT   0x8000  /*  Exact match on command name rather than prefix match (I.e:  '@shutdown', '@maintenance', etc.)  */
#define CMD_NOCONV  0x8000  /*  Short command not matched when in 'converse' mode  */
#define CMD_MASK    0x0FFF  /*  Command type mask (Above 'RESERVED:' flags)  */


/* ---->  Flow control  <---- */
#define FLOW_NONE            0x00000000

#define FLOW_MASK            0x00000073
#define FLOW_MASK_LOOP       0x0000007F

#define FLOW_NORMAL          0x00000000
#define FLOW_BREAK           0x00000001
#define FLOW_RETURN          0x00000002
#define FLOW_BREAKLOOP       0x00000004
#define FLOW_BREAKLOOP_ALL   0x00000008
#define FLOW_GOTO            0x00000010
#define FLOW_GOTO_LITERAL    0x00000020
#define FLOW_SKIP            0x00000040

#define FLOW_BOOL            0x00000080
#define FLOW_FAIL            0x00000080
#define FLOW_SUCC            0x00000000

#define FLOW_CONDITION       0x00010000
#define FLOW_COMMAND         0x00020000
#define FLOW_ELSE            0x00040000
#define FLOW_WITH_FRIENDS    0x00200000
#define FLOW_WITH_CONNECTED  0x00400000
#define FLOW_WITH_BANISHED   0x00800000

#define FLOW_IF              0x01000000
#define FLOW_FOR             0x02000000
#define FLOW_WITH            0x04000000
#define FLOW_WHILE           0x08000000
#define FLOW_FOREACH         0x10000000
#define FLOW_CASE            0x20000000
#define FLOW_BEGIN           0x40000000

#define FLOW_DATA1_MASK      0xFF000000
#define FLOW_DATA2_MASK      0x00FF0000
#define FLOW_DATA3_MASK      0x0000FF00


/* ---->  Fuse trigger flags  <---- */
#define FUSE_TOM                0x01        /*  Trigger TOM (Trigger On Movement) fuses  */
#define FUSE_ARGS               0x02        /*  Set $1, $2 and $3 from TCZ_COMMAND_STRING  */
#define FUSE_COMMAND            0x04        /*  Ignore FUSE_ARGS if currently within compound command  */
#define FUSE_ABORTING           0x08        /*  Leave FUSE_ABORT flag set on COMMAND_TYPE after return from trigger_fuses()  */
#define FUSE_CONVERSE           0x10        /*  Fuse triggered by user in 'converse' mode (Assume " for 'say'.)  */

#endif
