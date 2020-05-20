/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| FRIEND_FLAGS.C  -  Flags which can be set on friends in your friends list.  |
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

#include <stdlib.h>

#include "friend_flags.h"

/* ---->  Friend flags  <---- */
struct friendflag_data friendflags[] = {
       {"Beep",            "Noisy",            FRIEND_BEEP           },
       {"Commands",        "CompoundCommands", FRIEND_COMMANDS       },
       {"Combat",          "Combat",           FRIEND_COMBAT         },
       {"Create",          "Creation",         FRIEND_CREATE         },
       {"Destroy",         "Destruction",      FRIEND_DESTROY        },
       {"Enemy",           NULL,               FRIEND_ENEMY          },
       {"Exclude",         "Ignore",           FRIEND_EXCLUDE        },
       {"Fchat",           "FriendChat",       FRIEND_FCHAT          },
       {"Inform",          "Notify",           FRIEND_INFORM         },
       {"Link",            "Teleport",         FRIEND_LINK           },
       {"Mail",            NULL,               FRIEND_MAIL           },
       {"PageTell",        "Tell",             FRIEND_PAGETELL       },
       {"PageTellFriends", "FriendsPageTell",  FRIEND_PAGETELLFRIENDS},
       {"Read",            NULL,               FRIEND_READ           },
       {"Sharable",        "Shared",           FRIEND_SHARABLE       },
       {"Visit",           "Join",             FRIEND_VISIT          },
       {"Write",           NULL,               FRIEND_WRITE          },
       {NULL,              NULL,               0                     },
};
