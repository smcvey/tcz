/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| SEARCH.H  -  Definitions for SEARCH.C, implementing object search/listing   |
|              commands such as '@find', '@list', etc.                        |
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

#ifndef __SEARCH_H
#define __SEARCH_H


/* ---->  Search flags for object types  <---- */
#define SEARCH_ALL           0xFFFFFFFF
#define SEARCH_ALL_OBJECTS   (SEARCH_CHARACTER|SEARCH_PROPERTY|SEARCH_VARIABLE|SEARCH_COMMAND|SEARCH_ALARM|SEARCH_ARRAY|SEARCH_THING|SEARCH_EXIT|SEARCH_FUSE|SEARCH_ROOM)
#define SEARCH_ALL_TYPES     (SEARCH_ALL & ~(SEARCH_NOT_OBJECT|SEARCH_NOT_CHARACTER))

#define SEARCH_CHARACTER     0x00000001
#define SEARCH_PROPERTY      0x00000002
#define SEARCH_VARIABLE      0x00000004
#define SEARCH_COMMAND       0x00000008
#define SEARCH_PUPPET        0x00000010
#define SEARCH_ALARM         0x00000020
#define SEARCH_ARRAY         0x00000040
#define SEARCH_THING         0x00000080
#define SEARCH_EXIT          0x00000100
#define SEARCH_FUSE          0x00000200
#define SEARCH_ROOM          0x00000400

#define SEARCH_GLOBAL        0x00100000  /*  Search for global compound commands  */
#define SEARCH_BANNED        0x00200000  /*  Search for banned characters  */
#define SEARCH_JUNKED        0x00400000  /*  Search for objects which have been junked (I.e:  Objects located in #0 (ROOMZERO))  */
#define SEARCH_FLOATING      0x00800000  /*  Search for floating objects (I.e:  Objects not in any linked list attached to another object)  */
#define SEARCH_ANYTHING      0x01000000  /*  Override for above to allow SEARCH_ALL to work correctly  */

#define SEARCH_FRIENDS       0x02000000  /*  '@with':  Process list of friends  */
#define SEARCH_FOTHERS       0x04000000  /*  '@with':  Process fothers list  */
#define SEARCH_ENEMIES       0x08000000  /*  '@with':  Process list of enemies  */
#define SEARCH_CONNECTED     0x10000000  /*  '@with':  Process list of connected characters  */

#define SEARCH_NOT_OBJECT    0x40000000  /*  Do not search objects (For flags that can only be set on characters.)  */
#define SEARCH_NOT_CHARACTER 0x80000000  /*  Do not search characters (For flags that can only be set on objects.)  */

#define SEARCH_CONTENTS      (SEARCH_CHARACTER|SEARCH_THING|SEARCH_ROOM)
#define SEARCH_PREFERRED     (SEARCH_CHARACTER|SEARCH_THING)


/* ---->  Search flags for field types (Fields attached to objects)  <---- */
#define SEARCH_NAME          0x00000001
#define SEARCH_DESC          0x00000002
#define SEARCH_ODESC         0x00000004
#define SEARCH_SUCC          0x00000008
#define SEARCH_OSUCC         0x00000010
#define SEARCH_FAIL          0x00000020
#define SEARCH_OFAIL         0x00000040
#define SEARCH_DROP          0x00000080
#define SEARCH_ODROP         0x00000100
#define SEARCH_AREANAME      0x00000200
#define SEARCH_CSTRING       0x00000400
#define SEARCH_ESTRING       0x00000800
#define SEARCH_INDEX         0x00001000

#define SEARCH_WWW           SEARCH_ODESC
#define SEARCH_RACE          SEARCH_FAIL
#define SEARCH_EMAIL         SEARCH_DROP
#define SEARCH_TITLE         SEARCH_ODROP
#define SEARCH_PREFIX        SEARCH_SUCC
#define SEARCH_SUFFIX        SEARCH_OSUCC
#define SEARCH_LASTSITE      SEARCH_OFAIL


/* ---->  Creation date/last usage date constants  <---- */
#define OP_LT 1
#define OP_GT 2
#define OP_LE 3
#define OP_GE 4


/* ---->  Search list entry  <---- */
struct search_data {
       const char *name;
       int        value;
       int        mask;
       int        alt;
};

extern struct search_data search_objecttype[];
extern struct search_data search_fieldtype[];
extern struct search_data search_flagtype[];
extern struct search_data search_flagtype2[];

#endif /* __SEARCH_H */
