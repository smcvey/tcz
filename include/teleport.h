/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| TELEPORT.H  -  Object teleportation restrictions for Mortals/Admin for each |
|                object type (Used when user does not have write permission   |
|                to destination object.)                                      |
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

  $Id: teleport.h,v 1.1.1.1 2004/12/02 17:43:37 jpboggis Exp $

*/


/* ---->  Mortal teleport/drop privs (Objects that an object of the given type can be teleported to without permission)  <---- */
int mortaltelprivs[] =
{
    /*  Free       */  0,
    /*  Thing      */  SEARCH_ROOM|SEARCH_THING,
    /*  Exit       */  0,
    /*  Character  */  0,
    /*  Room       */  0,
    /*  Command    */  0,
    /*  Fuse       */  0,
    /*  Alarm      */  0,
    /*  Variable   */  SEARCH_CHARACTER,
    /*  Array      */  SEARCH_CHARACTER,
    /*  Property   */  SEARCH_CHARACTER,
};


/* ---->  Admin teleport/drop privs (Objects that an object of the given type can be teleported to without permission)  <---- */
int admintelprivs[] =
{
    /*  Free       */  0,
    /*  Thing      */  SEARCH_ROOM|SEARCH_THING,
    /*  Exit       */  SEARCH_ROOM|SEARCH_THING,
    /*  Character  */  SEARCH_ROOM|SEARCH_THING,
    /*  Room       */  SEARCH_ROOM|SEARCH_THING,
    /*  Command    */  SEARCH_ROOM|SEARCH_THING|SEARCH_COMMAND,
    /*  Fuse       */  0,
    /*  Alarm      */  0,
    /*  Variable   */  SEARCH_CHARACTER|SEARCH_VARIABLE|SEARCH_THING|SEARCH_ROOM|SEARCH_ALARM|SEARCH_FUSE|SEARCH_COMMAND,
    /*  Array      */  SEARCH_CHARACTER|SEARCH_ARRAY|SEARCH_THING|SEARCH_ROOM|SEARCH_ALARM|SEARCH_FUSE|SEARCH_COMMAND,
    /*  Property   */  SEARCH_CHARACTER|SEARCH_VARIABLE|SEARCH_THING|SEARCH_ROOM|SEARCH_ALARM|SEARCH_FUSE|SEARCH_COMMAND,
};
