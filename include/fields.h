/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| FIELDS.H  -  Definitions of which fields each object has (Desc, Succ, Fail, |
|              etc.) and which fields can be set/reset by users.              |
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

#ifndef __FIELDS_H
#define __FIELDS_H

/* ---->  Object field flags  <---- */
#define LOCK                 0x00000001
#define NAME                 0x00000002
#define DESC                 0x00000004
#define SUCC                 0x00000008
#define FAIL                 0x00000010
#define DROP                 0x00000020
#define ODESC                0x00000040
#define OSUCC                0x00000080
#define OFAIL                0x00000100
#define ODROP                0x00000200
#define AREANAME             0x00000400
#define CSTRING              0x00000800
#define ESTRING              0x00001000
#define VOLUME               0x00002000
#define MASS                 0x00004000
#define DESTINATION          0x00008000
#define EMAIL                0x00010000
#define CSUCC                0x00020000
#define CFAIL                0x00040000
#define WWW                  0x00080000


/* ---->  Aliased object field flags  <---- */
#define LASTSITE             OFAIL
#define PREFIX               SUCC
#define SUFFIX               OSUCC
#define TITLE                ODROP
#define RACE                 FAIL

extern int fields[];
extern int settablefields[];

#endif /* __FIELDS_H */
