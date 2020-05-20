/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| QUOTA.H  - Cost in Building Quota per object and maximum Building Quota     |
|            limit that can be set by each level of Admin.                    |
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

#include "db.h"
#include "quota.h"

/* ---->  Building Quota used by each type of object  <---- */
int objectquota[] =
{
    /*  Free       */  0,
    /*  Thing      */  THING_QUOTA,
    /*  Exit       */  EXIT_QUOTA,
    /*  Character  */  0,
    /*  Room       */  ROOM_QUOTA,
    /*  Command    */  COMMAND_QUOTA,
    /*  Fuse       */  FUSE_QUOTA,
    /*  Alarm      */  ALARM_QUOTA,
    /*  Variable   */  VARIABLE_QUOTA,
    /*  Array      */  ARRAY_QUOTA,
    /*  Property   */  PROPERTY_QUOTA,
};


/* ---->  Max. quota limit that can be set by each level of admin  <---- */
int maxquotalimit[6] =
{
    /*  <= Mortal                */  0,  /*  Mortals/Morons can't set quota limit  */
    /*  Apprentice Wizard/Druid  */  500,
    /*  Wizard/Druid             */  1000,
    /*  Elder Wizard/Druid       */  TCZ_INFINITY,
    /*  Deity                    */  TCZ_INFINITY,
    /*  Supreme Being            */  TCZ_INFINITY,
};
