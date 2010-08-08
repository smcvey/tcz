/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| OBJECT_TYPES.H  -  Definitions of object type flags and related macros.     |
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
| Module originally designed and written by:  J.P.Boggis 04/12/2003.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: object_types.h,v 1.1.1.1 2004/12/02 17:43:28 jpboggis Exp $

*/


/* ---->  Object types (db[<OBJECT>].type)  <---- */
#define TYPE_FREE       0x00  /*  Free (Unused) object in database  */
#define TYPE_THING      0x01  /*  Thing  */
#define TYPE_EXIT       0x02  /*  Exit  */
#define TYPE_CHARACTER  0x03  /*  Character  */
#define TYPE_ROOM       0x04  /*  Room  */
#define TYPE_COMMAND    0x05  /*  Compound Command  */
#define TYPE_FUSE       0x06  /*  Fuse  */
#define TYPE_ALARM      0x07  /*  Alarm  */
#define TYPE_VARIABLE   0x08  /*  Variable  */
#define TYPE_ARRAY      0x09  /*  Dynamic Array  */
#define TYPE_PROPERTY   0x0A  /*  Property  */


/* ---->  Object macros  <---- */
#define Typeof(object)                  (db[object].type)
#define Valid(object)                   ((object >= 0) && (object < db_top) && ValidType(object))
#define ValidObjectType(objecttype)     (objecttype <= TYPE_PROPERTY)
#define ValidType(object)               (ValidObjectType(Typeof(object)))
#define Validchar(object)               (Valid(object) && (Typeof(object) == TYPE_CHARACTER))
#define ValidCharName(name)             (isalnum(name) || ((name) == '_') || ((name) == ' '))
