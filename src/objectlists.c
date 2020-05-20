/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| OBJECTLISTS.C  -  Definitions of lists each type of object has, and which   |
|                   types of objects may be moved into the lists.             |
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
|                        https://github.com/smcvey/tcz                        |
`-----------------------------------------------------------------------------'

*/

#include "objectlists.h"

/* ---->  Array of linked lists that each type of object has  <---- */
int lists[] =
{
    /*  Free       */  0,
    /*  Thing      */  CONTENTS|VARIABLES|FUSES|COMMANDS|EXITS,
    /*  Exit       */  COMMANDS|VARIABLES|FUSES,
    /*  Character  */  CONTENTS|VARIABLES|FUSES|COMMANDS|EXITS,
    /*  Room       */  CONTENTS|VARIABLES|FUSES|COMMANDS|EXITS,
    /*  Command    */  COMMANDS|VARIABLES|FUSES,
    /*  Fuse       */  COMMANDS|VARIABLES,
    /*  Alarm      */  COMMANDS|VARIABLES,
    /*  Variable   */  VARIABLES,
    /*  Array      */  VARIABLES,
    /*  Property   */  VARIABLES,
};
     

/* ---->  Linked list each type of object is stored in  <---- */
int inlist[] =
{
    /*  Free       */  0,
    /*  Thing      */  CONTENTS,
    /*  Exit       */  EXITS,
    /*  Character  */  CONTENTS,
    /*  Room       */  CONTENTS,
    /*  Command    */  COMMANDS,
    /*  Fuse       */  FUSES,
    /*  Alarm      */  FUSES,
    /*  Variable   */  VARIABLES,
    /*  Array      */  VARIABLES,
    /*  Property   */  VARIABLES,
};
