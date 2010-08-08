/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| PRIMARY_FLAGS.H  -  Definitions of primary object/character flags.          |
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
| Module originally designed and written by:  J.P.Boggis 15/03/2000.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: primary_flags.h,v 1.2 2005/01/25 19:23:28 tcz_monster Exp $

*/

#ifndef __PRIMARY_FLAGS_H
#define __PRIMARY_FLAGS_H

#include "flagset.h"

/* ---->  Primary object flags (db[x].flags)  <---- */
#define OBJECT          0x00000001  /*  DB entry is an object of some sort  */
#define PERMANENT       0x00000002  /*  Object is permanent (I.e:  Can't be destroyed)  */
#define CENSOR          0x00000004  /*  Character/room/thing censored?  */
#define DEITY           0x00000008  /*  Character is a Deity  */
#define WIZARD          0x00000010  /*  Character is a Wizard  */
#define QUIET           0x00000020  /*  Character doesn't want to hear natters/shouts/yells (If set on room/thing, no talking is allowed in it)  */
#define INVISIBLE       0x00000040  /*  Object is invisible, and cannot be seen  */
#define LISTEN          0x00000080  /*  Character is notified when others connect/disconnect  */
#define STICKY          0x00000100  /*  Object goes home when dropped  */
#define ENGAGED         0x00000100  /*  (Re-used STICKY)  Character is engaged to another user.  */
#define BUILDER         0x00000200  /*  This character can use building commands  */
#define BOOT            0x00000200  /*  (Re-used BUILDER)  Without this flag, Admin can't boot, ban or set site flags/max. connections  */
#define HELP            0x00000200  /*  (Re-used BUILDER)  Without this flag, Experienced Builders can't welcome or assist users  */
#define VISIBLE         0x00000400  /*  Allows others to examine object as if it was their own  */
#define OPENABLE        0x00000800  /*  Container/exit can be opened/closed  */
#define DRUID           0x00000800  /*  (Re-used OPENABLE)  Admin character is a 'Druid' rather than 'Wizard'  */
#define OPEN            0x00001000  /*  Container/exit is currently open  */
#define BEING           0x00001000  /*  (Re-used OPEN)  Character is a Being  */
#define OPAQUE          0x00002000  /*  If a container/exit is opaque  */
#define MARRIED         0x00002000  /*  (Re-used OPAQUE)  Character is married to another user  */
/*                      0x00004000      (1st bit of Gender)  */
#define COMBAT          0x00004000  /*  (Re-used GENDER)  Restrict combat command usage within location  */
/*                      0x00008000      (2nd bit of Gender, re-usable for object)  */
#define LOCKED          0x00010000  /*  Object is currently locked  */
/*                      0x00010000      (Re-used LOCKED)  Available for aliasing (As character only flag)  */
#define HAVEN           0x00020000  /*  Can't fight in room/can't be paged if set on character  */
#define TRANSFERABLE    0x00040000  /*  Any character may take ownership of object, without needing 'write' permission to owner  */
#define BBS_INFORM      0x00040000  /*  (Re-used TRANSFERABLE)  Inform user of posts to BBS, even if they aren't in the BBS room  */
#define PRIVATE         0x00080000  /*  Object is private and can't be read by users of the same level (Mainly applies to Admin.)  */
#define ABODE           0x00100000  /*  Any character can link to a room with this flag  */
#define ASSISTANT       0x00100000  /*  (Re-used ABODE)  Mortal user is an 'assistant', which allows them to use the 'welcome'/'assist' command (If there are insufficient Admin) and see when new users are created  */
#define TOM             0x00200000  /*  Trigger On Movement for fuses  */
#define EXPERIENCED     0x00200000  /*  (Re-used TOM)  When set on a BUILDER, marks them as 'experienced' and gives them a few extra privileges  */
#define NUMBER          0x00400000  /*  When reset, #<ID> numbers on what you own will be hidden  */
#define READONLY        0x00800000  /*  Object is read-only and cannot be modified  */
#define TRACING         0x01000000  /*  Compound command execution tracing/debugging  */
#define SHARABLE        0x02000000  /*  Users' 'read'/'write' privileges (FRIEND_READ/FRIEND_WRITE) only apply to objects with this flag set, if they are given the FRIEND_SHARABLE friend flag  */
#define FRIENDS_INFORM  0x02000000  /*  (Re-used SHARABLE)  Inform user when users add them to their friends/enemies lists or set/reset friend flags on them  */
#define ASHCAN          0x04000000  /*  Object marked for automatic deletion  */
#define ANSI            0x08000000  /*  Enable use ANSI colour?  */
#define ELDER           0x10000000  /*  Character is an Elder Wizard  */
#define APPRENTICE      0x20000000  /*  Character is an Apprentice Wizard  */
#define YELL            0x40000000  /*  Mortals can yell in this room/container  */
#define SHOUT           0x40000000  /*  (Re-used YELL)  Without this flag, character can't shout  */
#define MORON           0x80000000  /*  Characters with this flag are right twonks!  :-)  */


/* ---->  Character gender (Sex) flags  <---- */
#define GENDER_SHIFT         14
#define GENDER_UNASSIGNED    0x0
#define GENDER_NEUTER        0x1
#define GENDER_FEMALE        0x2
#define GENDER_MALE          0x3
#define GENDER_MASK          0xC000

extern struct flag_data flag_list[];

#endif /* __PRIMARY_FLAGS_H */
