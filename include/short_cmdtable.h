/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| SHORT_CMDTABLE.H  -  Table of short commands (" for 'say', etc.)            |
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
| Module originally designed and written by:  J.P.Boggis 03/06/1999.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: short_cmdtable.h,v 1.1.1.1 2004/12/02 17:43:34 jpboggis Exp $

*/


/* ---->  Short command table (I.e:  '"' for 'say', '+' for 'think', etc.)  <---- */
struct cmd_table short_cmds[] = {
       {OTHER_COMMAND, ALT_CHANNEL_TOKEN,        (void *) channel_main,              0, 0, 0},
       {OTHER_COMMAND, ALT_POSE_TOKEN,           (void *) comms_pose,                0, 0, 0},
       {OTHER_COMMAND, ALT_SAY_TOKEN1,           (void *) comms_say,                 0, 0, 0},
       {OTHER_COMMAND, ALT_SAY_TOKEN2,           (void *) comms_say,                 0, 0, 0},
       {OTHER_COMMAND, ALT_THINK_TOKEN,          (void *) comms_think,               0, 0, 0},
       {OTHER_COMMAND, ASK_TOKEN,                (void *) comms_ask,                 0, 0, 0},
       {OTHER_COMMAND, CHANNEL_TOKEN,            (void *) channel_main,              0, 0, 0},
       {OTHER_COMMAND, POSE_TOKEN,               (void *) comms_pose,                0, 0, 0},
       {OTHER_COMMAND, ECHO_TOKEN,               (void *) comms_echo,                0, 0, 0},
       {OTHER_COMMAND, SAY_TOKEN,                (void *) comms_say,                 0, 0, 0},
#ifdef MORTAL_SHOUT
       {OTHER_COMMAND, SHOUT_TOKEN,              (void *) admin_shout,               0, 0, 0},
#else
       {OTHER_COMMAND, SHOUT_TOKEN,              (void *) look_notice,               6, 0, 0},
#endif
       {OTHER_COMMAND, THINK_TOKEN,              (void *) comms_think,               0, 0, 0},
       {OTHER_COMMAND, NULL,                     NULL,                               0, 0, 0},
};
