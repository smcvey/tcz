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
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: quota.h,v 1.1.1.1 2004/12/02 17:43:32 jpboggis Exp $

*/


#ifndef _QUOTA_H
#define _QUOTA_H


/* ---->  Standard Building Quota for new Builders  <---- */
#define STANDARD_CHARACTER_QUOTA       500


/* ---->  Amount of Building Quota used per object type  <---- */
#define PROPERTY_QUOTA                 1         /*  Building Quota used per property  */
#define VARIABLE_QUOTA                 5         /*  Building Quota used per variable  */
#define ELEMENT_QUOTA                  1         /*  Extra Building Quota used per dynamic array element  */
#define COMMAND_QUOTA                  10        /*  Building Quota used per compound command  */
#define ALARM_QUOTA                    100       /*  Building Quota used per alarm  */
#define ARRAY_QUOTA                    1         /*  Building Quota used per dynamic array with no elements  */
#define THING_QUOTA                    10        /*  Building Quota used per thing  */
#define EXIT_QUOTA                     2         /*  Building Quota used per exit  */
#define FUSE_QUOTA                     5         /*  Building Quota used per fuse  */
#define ROOM_QUOTA                     10        /*  Building Quota used per room  */

extern int objectquota[];
extern int maxquotalimit[];

/* ---->  Building Quota macros  <---- */
#define MaxQuotaLimit(player)  (Validchar(player) ? maxquotalimit[level_app(player)]:0)
#define ObjectQuota(object)    (Valid(object) ? objectquota[Typeof(object)]:0)

#endif  /*  _QUOTA_H         */
