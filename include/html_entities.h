/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| HTML_ENTITIES.H  -  Table of HTML entities recognised by the HTML           |
|                     Interface.  These allow HTML entities to be converted   |
|                     to the nearest ASCII text for the benefit of non-HTML   |
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
| Module originally designed and written by:  J.P.Boggis 21/06/1996.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: html_entities.h,v 1.1.1.1 2004/12/02 17:43:24 jpboggis Exp $

*/


/* ---->  HTML character entity data  <---- */
struct html_entity_data {
       const    char *entity;
       const    char *text;
       unsigned char number;
       unsigned char len;
};


/* ---->  HTML character entity table  <---- */
struct html_entity_data entity[] = {
       {NULL,     "\n",  10,  0},
       {NULL,     "!",   161, 0},
       {NULL,     "C",   162, 0},
       {"pound",  "$",   163, 0},
       {NULL,     "#",   164, 0},
       {NULL,     "Y",   165, 0},
       {NULL,     "|",   166, 0},
       {NULL,     "\"",  168, 0},
       {NULL,     "<<",  171, 0},
       {NULL,     "-",   173, 0},
       {NULL,     "+/-", 177, 0},
       {NULL,     "2",   178, 0},
       {NULL,     "3",   179, 0},
       {NULL,     "'",   180, 0},
       {NULL,     "u",   181, 0},
       {NULL,     ",",   184, 0},
       {NULL,     "1",   185, 0},
       {NULL,     ">>",  187, 0},
       {NULL,     "1/4", 188, 0},
       {NULL,     "1/2", 189, 0},
       {NULL,     "3/4", 190, 0},
       {NULL,     "?",   191, 0},
       {NULL,     "*",   215, 0},
       {NULL,     "/",   247, 0},
       {"AElig",  "AE",  198, 0},
       {"Aacute", "A",   192, 0},
       {"Acirc",  "A",   194, 0},
       {"Agrave", "A",   193, 0},
       {"Aring",  "A",   196, 0},
       {"Atilde", "A",   195, 0},
       {"Auml",   "A",   197, 0},
       {"Ccedil", "C",   199, 0},
       {"ETH",    "D",   208, 0},
       {"Eacute", "E",   200, 0},
       {"Ecirc",  "E",   202, 0},
       {"Egrave", "E",   201, 0},
       {"Euml",   "E",   203, 0},
       {"GT",     ">",   0,   0},
       {"Iacute", "I",   204, 0},
       {"Icirc",  "I",   206, 0},
       {"Igrave", "I",   205, 0},
       {"Iuml",   "I",   207, 0},
       {"LT",     "<",   0,   0},
       {"QUOT",   "\"",  0,   0},
       {"NBSP",   NULL,  0,   0},
       {"Ntilde", "N",   209, 0},
       {"Oacute", "O",   210, 0},
       {"Ocirc",  "O",   212, 0},
       {"Ograve", "O",   211, 0},
       {"Oslash", "0",   216, 0},
       {"Otilde", "O",   213, 0},
       {"Ouml",   "O",   214, 0},
       {"REG",    "(R)", 0,   0},
       {"SHY",    NULL,  0,   0},
       {"THORN",  "P",   222, 0},
       {"Uacute", "U",   217, 0},
       {"Ucirc",  "U",   219, 0},
       {"Ugrave", "U",   218, 0},
       {"Uuml",   "U",   220, 0},
       {"Yacute", "Y",   221, 0},
       {"aacute", "a",   224, 0},
       {"acirc",  "a",   226, 0},
       {"aelig",  "ae",  230, 0},
       {"agrave", "a",   225, 0},
       {"amp",    "&",   38,  0},
       {"aring",  "a",   229, 0},
       {"atilde", "a",   227, 0},
       {"auml",   "a",   228, 0},
       {"ccedil", "c",   231, 0},
       {"copy",   "(C)", 169, 0},
       {"eacute", "e",   232, 0},
       {"ecirc",  "e",   234, 0},
       {"egrave", "e",   233, 0},
       {"eth",    "o",   240, 0},
       {"euml",   "e",   235, 0},
       {"gt",     ">",   62,  0},
       {"iacute", "i",   236, 0},
       {"icirc",  "i",   238, 0},
       {"igrave", "i",   237, 0},
       {"iuml",   "i",   239, 0},
       {"lt",     "<",   60,  0},
       {"quot",   "\"",  34,  0},
       {"nbsp",   NULL,  0,   0},
       {"ntilde", "n",   241, 0},
       {"oacute", "o",   242, 0},
       {"ocirc",  "o",   244, 0},
       {"ograve", "o",   243, 0},
       {"oslash", "0",   248, 0},
       {"otilde", "o",   245, 0},
       {"ouml",   "o",   246, 0},
       {"reg",    "(R)", 174, 0},
       {"shy",    NULL,  0,   0},
       {"szlig",  "B",   223, 0},
       {"thorn",  "p",   254, 0},
       {"uacute", "u",   249, 0},
       {"ucirc",  "u",   251, 0},
       {"ugrave", "u",   250, 0},
       {"uuml",   "u",   252, 0},
       {"yacute", "y",   253, 0},
       {"yuml",   "y",   255, 0},
       {NULL,     NULL,  0,   0},
};
