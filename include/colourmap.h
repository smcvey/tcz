/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| COLOURMAP.H  -  Defines colour map table used by map.c and {@?colourmap}    |
|                 query command.  This table consists of case-sensitive       |
|                 map code characters and the string they substitute.         |
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
| Module originally designed and written by:  J.P.Boggis 04/02/2000.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: colourmap.h,v 1.1.1.1 2004/12/02 17:43:12 jpboggis Exp $

*/


#include "db.h"


struct colourmap_data {
       char          map;     /*  Map character (Case sensitive)  */
       unsigned char colour;  /*  Text colour code?               */
       const    char *subst;  /*  Text to substitute              */
};


/* ---->  Colour map conversion table  <---- */
struct colourmap_data colourmap[] = {
       {'0', 0, ANSI_IBLACK},
       {'1', 0, ANSI_IRED},
       {'2', 0, ANSI_IGREEN},
       {'3', 0, ANSI_IYELLOW},
       {'4', 0, ANSI_IBLUE},
       {'5', 0, ANSI_IMAGENTA},
       {'6', 0, ANSI_ICYAN},
       {'7', 0, ANSI_IWHITE},
       {'b', 1, ANSI_DBLUE},
       {'B', 1, ANSI_LBLUE},
       {'c', 1, ANSI_DCYAN},
       {'C', 1, ANSI_LCYAN},
       {'d', 0, ANSI_DARK},
       {'D', 0, ANSI_DARK},
       {'f', 0, ANSI_BLINK},
       {'F', 0, ANSI_BLINK},
       {'g', 1, ANSI_DGREEN},
       {'G', 1, ANSI_LGREEN},
       {'l', 0, ANSI_LIGHT},
       {'L', 0, ANSI_LIGHT},
       {'m', 1, ANSI_DMAGENTA},
       {'M', 1, ANSI_LMAGENTA},
       {'r', 1, ANSI_DRED},
       {'R', 1, ANSI_LRED},
       {'u', 0, ANSI_UNDERLINE},
       {'U', 0, ANSI_UNDERLINE},
       {'w', 1, ANSI_DWHITE},
       {'W', 1, ANSI_LWHITE},
       {'y', 1, ANSI_DYELLOW},
       {'Y', 1, ANSI_LYELLOW},
       {'z', 1, ANSI_DBLACK},
       {'Z', 1, ANSI_LBLACK},
       {'x', 0, ANSI_NORMAL},
       {'X', 0, ANSI_NORMAL},
       {'#', 0, " "},   /*  Used to blank out comments  */
       {'!', 0, NULL},  /*  Used in {@?colourmap} query command to prevent repeating  */
       {'\0', 0, NULL}
};
