/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| SMILEYS.H  -  Table of graphical smileys (Emoticons) for use by users of    |
|               the HTML Interface (I.e:  Optional replacement of :-) with a  |
|               graphical icon equivalent.)                                   |
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
| Module originally designed and written by:  J.P.Boggis 03/03/2000.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: smileys.h,v 1.1.1.1 2004/12/02 17:43:34 jpboggis Exp $

*/


/* ---->  Smiley array data structure  <---- */
struct smiley_data {
       const char *smiley;       /*  Text smiley  */
       const char *icon;         /*  Graphical icon filename  */
       int        smileylength;  /*  Length of text smiley (Generated at run-time)  */
       int        imglength;     /*  Length of <IMG SRC> tag (Generated at run-time)  */
       const char *img;          /*  <IMG SRC> tag (Generated at run-time)  */
};


/* ---->  Table of smileys  <---- */
struct smiley_data smileys[] = {
       {":()",  "smiley.laugh.gif", 0, 0, NULL},
       {":-()", "smiley.laugh.gif", 0, 0, NULL},
       {":)",   "smiley.happy.gif", 0, 0, NULL},
       {":-)",  "smiley.happy.gif", 0, 0, NULL},
       {":(",   "smiley.sad.gif", 0, 0, NULL},
       {":-(",  "smiley.sad.gif", 0, 0, NULL},
       {";)",   "smiley.happy.wink.gif", 0, 0, NULL},
       {";-)",  "smiley.happy.wink.gif", 0, 0, NULL},
       {";(",   "smiley.sad.wink.gif", 0, 0, NULL},
       {";-(",  "smiley.sad.wink.gif", 0, 0, NULL},
       {":|",   "smiley.dash.gif", 0, 0, NULL},
       {":-|",  "smiley.dash.gif", 0, 0, NULL},
       {":D",   "smiley.laugh.gif", 0, 0, NULL},
       {":-D",  "smiley.laugh.gif", 0, 0, NULL},
       {":O",   "smiley.o.gif", 0, 0, NULL},
       {":-O",  "smiley.o.gif", 0, 0, NULL},
       {":X",   "smiley.x.gif", 0, 0, NULL},
       {":-X",  "smiley.x.gif", 0, 0, NULL},
       {":P",   "smiley.tongue.gif", 0, 0, NULL},
       {":-P",  "smiley.tongue.gif", 0, 0, NULL},
       {":}",   "smiley.tilde.gif", 0, 0, NULL},
       {":-}",  "smiley.tilde.gif", 0, 0, NULL},
       {":{",   "smiley.tilde.gif", 0, 0, NULL},
       {":-{",  "smiley.tilde.gif", 0, 0, NULL},
       {":~",   "smiley.tilde.gif", 0, 0, NULL},
       {":-~",  "smiley.tilde.gif", 0, 0, NULL},
       {":]",   "smiley.happy.square.gif", 0, 0, NULL},
       {":-]",  "smiley.happy.square.gif", 0, 0, NULL},
       {":[",   "smiley.sad.square.gif", 0, 0, NULL},
       {":-[",  "smiley.sad.square.gif", 0, 0, NULL},
       {":>",   "smiley.happy.square.gif", 0, 0, NULL},
       {":->",  "smiley.happy.square.gif", 0, 0, NULL},
       {":<",   "smiley.sad.square.gif", 0, 0, NULL},
       {":-<",  "smiley.sad.square.gif", 0, 0, NULL},
       {"8)",   "smiley.happy.shades.gif", 0, 0, NULL},
       {"8-)",  "smiley.happy.shades.gif", 0, 0, NULL},
       {"8(",   "smiley.sad.shades.gif", 0, 0, NULL},
       {"8-(",  "smiley.sad.shades.gif", 0, 0, NULL},
       {"|)",   "smiley.happy.shades.gif", 0, 0, NULL},
       {"|-)",  "smiley.happy.shades.gif", 0, 0, NULL},
       {"|(",   "smiley.sad.shades.gif", 0, 0, NULL},
       {"|-(",  "smiley.sad.shades.gif", 0, 0, NULL},
       {">)",   "smiley.happy.slant.gif", 0, 0, NULL},
       {">-)",  "smiley.happy.slant.gif", 0},
       {">(",   "smiley.sad.slant.gif", 0, 0, NULL},
       {">-(",  "smiley.sad.slant.gif", 0, 0, NULL},
       {"*:)",  "smiley.happy.hair.gif", 0, 0, NULL},
       {"*:-)", "smiley.happy.hair.gif", 0, 0, NULL},
       {"*:(",  "smiley.sad.hair.gif", 0, 0, NULL},
       {"*:-(", "smiley.sad.hair.gif", 0, 0, NULL},
       {"*8)",  "smiley.happy.hair.gif", 0, 0, NULL},
       {"*8-)", "smiley.happy.hair.gif", 0, 0, NULL},
       {"*8(",  "smiley.sad.hair.gif", 0, 0, NULL},
       {"*8-(", "smiley.sad.hair.gif", 0, 0, NULL},
       {"*|)",  "smiley.happy.hair.gif", 0, 0, NULL},
       {"*|-)", "smiley.happy.hair.gif", 0, 0, NULL},
       {"*|(",  "smiley.sad.hair.gif", 0, 0, NULL},
       {"*|-(", "smiley.sad.hair.gif", 0, 0, NULL},
       {NULL,   NULL, 0, 0, NULL}
};
