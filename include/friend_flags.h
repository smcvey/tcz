/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| FRIEND_FLAGS.H  -  Flags which can be set on friends in your friends list.  |
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
| Module originally designed and written by:  J.P.Boggis 20/05/2000.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                        https://github.com/smcvey/tcz                        |
`-----------------------------------------------------------------------------'

*/

#ifndef __FRIEND_FLAGS_H
#define __FRIEND_FLAGS_H

/* ---->  Friend flags (Set/reset using 'fset')  <---- */
#define FRIEND_PAGETELL         0x00000001  /*  User can page/tell messages to you  */
#define FRIEND_MAIL             0x00000002  /*  User can send mail to you  */
#define FRIEND_FCHAT            0x00000004  /*  You'll get messages sent by user over the friends/enemies chatting channel  */
#define FRIEND_LINK             0x00000008  /*  User can '@link'/'@tel' themselves to rooms of yours which are set ABODE  */
#define FRIEND_INFORM           0x00000010  /*  You'll be informed when user connects/disconnects, regardless of LISTEN  */
#define FRIEND_ENEMY            0x00000020  /*  User is an enemy  */
#define FRIEND_BEEP             0x00000040  /*  Your computer will beep when user connects (Providing you're LISTEN, or they're 'fset' INFORM)  */
#define FRIEND_PAGETELLFRIENDS  0x00000080  /*  You will receive group pages/tells sent by this user to their friends/enemies.  */
#define FRIEND_VISIT            0x00000100  /*  Friend/enemy may visit you at will  */
#define FRIEND_READ             0x00000200  /*  User has 'read' permission to your objects  */
#define FRIEND_WRITE            0x00000400  /*  User has 'write' permission to your objects  */
#define FRIEND_SHARABLE         0x00000800  /*  User may only 'read'/'write' to objects of yours which have the SHARABLE flag set  */
#define FRIEND_CREATE           0x00001000  /*  User may create new objects under your ownership and/or use '@owner' to change the ownership of their objects to you (Providing they have 'write' permission (Via FRIEND_WRITE))  */
#define FRIEND_DESTROY          0x00002000  /*  User may destroy your objects (Providing they also have 'write' permission (Via FRIEND_WRITE))  */
#define FRIEND_COMMANDS         0x00004000  /*  Without this friend flag, WRITE/CREATE/DESTROY privileges DO NOT apply to compound commands  */
#define FRIEND_EXCLUDE          0x00008000  /*  User with this friend flag doesn't appear as a friend, although they are on user's list  */
#define FRIEND_COMBAT           0x00010000  /*  COMBAT compound commands of user may be used in your combat areas  */
/*                              0x80000000      RESERVED:  Used by 'friend_flags' function return value  */

#define FRIEND_STANDARD         (FRIEND_PAGETELL|FRIEND_MAIL|FRIEND_FCHAT|FRIEND_LINK|FRIEND_PAGETELLFRIENDS)


/* ---->  Friend flags list data   <---- */
struct friendflag_data {
       const char *name;
       const char *alt;
       int   flag;
};

extern struct friendflag_data friendflags[];

#endif /* __FRIEND_FLAGS_H */
