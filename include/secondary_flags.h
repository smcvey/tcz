/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| SECONDARY_FLAGS.H  -  Definitions of secondary object/character flags.      |
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
|                        https://github.com/smcvey/tcz                        |
`-----------------------------------------------------------------------------'

*/

#ifndef __SECONDARY_FLAGS_H
#define __SECONDARY_FLAGS_H

#include "flagset.h"

/* ---->  Secondary object flags (db[x].flags2)  <---- */
#define EDIT_OVERWRITE  0x00000001  /*  Text overwrite mode (Editor)?  */
#define VISIT           0x00000001  /*  (Re-used EDIT_OVERWRITE)  Users may only be unconditionally visited (Via VISIT friend flag) if the room/thing they are in has this flag set  */
#define EDIT_EVALUATE   0x00000002  /*  Evaluate {}'s, $'s and \'s (Editor)?  */
#define WARP            0x00000002  /*  (Re-used EDIT_EVALUATE)  Rooms/things without this flag will never be 'warp'ed to, even if they are set ABODE  */
#define LFTOCR_LFCR     0x00000004  /*  Lines should be terminated with LF+CR  */
#define LFTOCR_CRLF     0x00000008  /*  Lines should be terminated with CR+LF  */
#define LFTOCR_CR       0x0000000C  /*  Lines should be terminated with CR  */
#define LFTOCR_MASK     0x0000000C  /*  Used to mask off LFTOCR bits  */
#define CHAT_OPERATOR   0x00000010  /*  Operator of current chatting channel?  */
#define EXPIRY          0x00000010  /*  (Re-used CHAT_OPERATOR)  Object expiry based on creation date rather than last usage date  */
#define CHAT_PRIVATE    0x00000020  /*  Operator has made channel private?  */
#define TRANSPORT       0x00000020  /*  (Re-used CHAT_PRIVATE)   Object can be used as transport (Vehicle)/transport allowed in given room/thing  */
#define CONNECTED       0x00000040  /*  Is character currently connected to TCZ?  */
#define ABORTFUSE       0x00000040  /*  (Re-used CONNECTED)  Fuse intercepts user command and prevents execution (Unless '@continue' command is used with the fuse's compound command.)  */
#define ANSI8           0x00000080  /*  Character would prefer 8 colour ANSI rather than 16  */
#define VALIDATED       0x00000080  /*  (Re-used ANSI8)  Mortal compound command validated for use as a global command  */
#define RETIRED         0x00000100  /*  Admin character has retired (Has Experienced Builder privs.)  */
#define FINANCE         0x00000100  /*  (Re-used RETIRED)  Credits can be dropped in object  */
#define PAGEBELL        0x00000200  /*  Character's terminal should beep when paged  */
#define IMMOVABLE       0x00000200  /*  (Re-used NO_PAGEBELL)  Object cannot be moved  */
/*                      0x00000400      (AVAILABLE  -  Ex. PUBLIC_EMAIL)  */
#define UNDERLINE       0x00000800  /*  Underlines should be used instead of underscores (~)  */
#define SENDHOME        0x00000800  /*  (Re-used UNDERLINE)  Object should be sent to its home on restart of TCZ.  */
#define FRIENDS_CHAT    0x00001000  /*  Friends chatting channel on/off?  */
#define SECURE          0x00001000  /*  (Re-used FRIENDS_CHAT)  Credit dropped in object cannot be taken (Except by object's owner.)  */
/* #define HTML            0x00002000  Unused  */
#define INHERITABLE     0x00004000  /*  Someone else can make object a parent without being able to control it  */
#define ORIGINAL        0x00008000  /*  Used during DB dump  -  Object without this flag has been destroyed and re-created during dump (Also used by boolean expression (Object locks) sanity check)  */
#define LOCAL_ECHO      0x00010000  /*  TCZ should echo text typed by user back to their terminal?  */
#define SKIPOBJECTS     0x00010000  /*  (Re-used LOCAL_ECHO)  Non-Admin/owner owned compound commands attached to objects in room/thing will not be executed  */
/*                      0x00020000      (1st bit of Article)  */
/*                      0x00040000      (2nd bit of Article)  */
#define SECRET          0x00080000  /*  Character's current location will not be given away on WHERE and SCAN (If flag is set on character, or room/thing they're in)  */
#define READONLYDESC    0x00100000  /*  Object's description read-only?  */
#define EDIT_NUMBERING  0x00200000  /*  Display line numbers (Editor)?  */
#define MORE_PAGER      0x00400000  /*  Use 'more' paging facility to page large output?  */
#define NON_EXECUTABLE  0x00800000  /*  Compound command/fuse with this flag set cannot be executed  */
#define FORWARD_EMAIL   0x00800000  /*  (Re-used NON-EXECUTABLE)  E-mail forwarding ('tcz.user.name@tczserver.domain') is available for user (Dependant on setting of valid E-mail address.)  */
#define BBS             0x40000000  /*  Character may post messages to the BBS  */
#define MAIL            0x80000000  /*  Character may send mail to other characters  */


/* ---->  Object article flags  <---- */
#define ARTICLE_SHIFT        17
#define ARTICLE_CONSONANT    0x1         /*  Consonant article   (A)  */
#define ARTICLE_VOWEL        0x2         /*  Vowel article      (An)  */
#define ARTICLE_PLURAL       0x3         /*  Plural article   (Some)  */
#define ARTICLE_MASK         0x60000

#define LOWER                0x0         /*  Lowercase                */
#define UPPER                0x1         /*  Uppercase (Capitalised)  */
#define INDEFINITE           0x2         /*  Indefinite article       */
#define DEFINITE             0x4         /*  Definite article         */

extern struct flag_data flag_list_prefs[];
extern struct flag_data flag_list2[];

#endif /* __SECONDARY_FLAGS_H */
