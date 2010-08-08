/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| HTML_TAGS.H  -  Table of HTML tags recognised by the HTML Interface.  The   |
|                 main purpose is to close any HTML tags which the user       |
|                 accidentally leaves open (E.g:  <FONT> without a closing    |
|                 </FONT>)  -  This helps prevent bad user HTML from messing  |
|                 up the interface.                                           |
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

  $Id: html_tags.h,v 1.1.1.1 2004/12/02 17:43:25 jpboggis Exp $

*/


#define HTML_TAG_FONT       1  /*  <FONT> tag  */
#define HTML_TAG_PRE        2  /*  <PRE> (Pre-format) tag  */
#define HTML_TAG_TD         3  /*  <TD> (Table Data cell) tag  */
#define HTML_TAG_TH         4  /*  <TH> (Table Header cell) tag  */
#define HTML_TAG_TT         5  /*  <TT> (TeleType) tag */
#define HTML_TAG_A          6  /*  <A> (Anchor) tag  */

#define EXCEPTION_SKIP      1  /*  Skip over tag (Don't send to user's browser)  */
#define EXCEPTION_PRE_TO_TT 2  /*  Convert <PRE> tag to <TT> tag   */
#define EXCEPTION_FONT      3  /*  Prefix with closing </FONT> tag   */
#define EXCEPTION_IGNORE    4  /*  Ignore tag (Send to user's browser without placing it on the HTML tag stack)  */


/* ---->  HTML tag table data  <---- */
struct html_tagtable_data {
       const    char *tag;
       unsigned char closetag;
       unsigned char exception;
       unsigned char tagtype;
       unsigned char len;
};


/* ---->  HTML tag table  <---- */
/*                    .- Closing Tag?  */
/*                    |  .- Exceptions  */
/*                    |  |  .- Tag Type  */
/*                    |  |  |  .- Length (Computed at run-time)  */
struct html_tagtable_data tags[] = {
       {"A",          1, 0, HTML_TAG_A, 0},
       {"ABBR",       1, 0, 0, 0},
       {"ACRONYM",    1, 0, 0, 0},
       {"ADDRESS",    1, 0, 0, 0},
       {"APPLET",     1, EXCEPTION_SKIP, 0, 0},
       {"AREA",       0, EXCEPTION_IGNORE, 0, 0},
       {"B",          1, EXCEPTION_FONT, 0, 0},
       {"BASE",       0, EXCEPTION_SKIP, 0, 0},
       {"BASEFONT",   0, EXCEPTION_SKIP, 0, 0},
       {"BDO",        1, 0, 0, 0},
       {"BIG",        1, 0, 0, 0},
       {"BLINK",      0, EXCEPTION_IGNORE, 0, 0},
       {"BLOCKQUOTE", 1, 0, 0, 0},
       {"BODY",       1, EXCEPTION_SKIP, 0, 0},
       {"BR",         0, EXCEPTION_IGNORE, 0, 0},
       {"BUTTON",     1, 0, 0, 0},
       {"CAPTION",    1, 0, 0, 0},
       {"CENTER",     1, 0, 0, 0},
       {"CITE",       1, 0, 0, 0},
       {"CODE",       1, 0, 0, 0},
       {"COL",        0, EXCEPTION_IGNORE, 0, 0},
       {"COLGROUP",   0, EXCEPTION_IGNORE, 0, 0},
       {"DD",         0, EXCEPTION_IGNORE, 0, 0},
       {"DEL",        1, 0, 0, 0},
       {"DFN",        1, 0, 0, 0},
       {"DIR",        1, 0, 0, 0},
       {"DIV",        1, 0, 0, 0},
       {"DL",         1, 0, 0, 0},
       {"DT",         0, EXCEPTION_IGNORE, 0, 0},
       {"EM",         1, 0, 0, 0},
       {"EMBED",      1, 0, 0, 0},
       {"FIELDSET",   1, 0, 0, 0},
       {"FONT",       1, EXCEPTION_FONT, HTML_TAG_FONT, 0},
       {"FORM",       1, 0, 0, 0},
       {"FRAME",      0, EXCEPTION_SKIP, 0, 0},
       {"FRAMESET",   1, EXCEPTION_SKIP, 0, 0},
       {"H1",         1, 0, 0, 0},
       {"H2",         1, 0, 0, 0},
       {"H3",         1, 0, 0, 0},
       {"H4",         1, 0, 0, 0},
       {"H5",         1, 0, 0, 0},
       {"H6",         1, 0, 0, 0},
       {"HEAD",       1, EXCEPTION_SKIP, 0, 0},
       {"HR",         0, EXCEPTION_IGNORE, 0, 0},
       {"HTML",       1, EXCEPTION_SKIP, 0, 0},
       {"I",          1, EXCEPTION_FONT, 0, 0},
       {"IFRAME",     1, 0, 0, 0},
       {"ILAYER",     1, 0, 0, 0},
       {"IMG",        0, EXCEPTION_IGNORE, 0, 0},
       {"INPUT",      0, EXCEPTION_IGNORE, 0, 0},
       {"INS",        1, 0, 0, 0},
       {"ISINDEX",    0, EXCEPTION_SKIP, 0, 0},
       {"KBD",        1, 0, 0, 0},
       {"KEYGEN",     0, EXCEPTION_IGNORE, 0, 0},
       {"LABEL",      1, 0, 0, 0},
       {"LAYER",      1, 0, 0, 0},
       {"LEGEND",     1, 0, 0, 0},
       {"LI",         0, EXCEPTION_IGNORE, 0, 0},
       {"LINK",       0, EXCEPTION_IGNORE, 0, 0},
       {"MAP",        1, 0, 0, 0},
       {"MARQUEE",    1, EXCEPTION_SKIP, 0, 0},
       {"MENU",       1, 0, 0, 0},
       {"META",       0, EXCEPTION_SKIP, 0, 0},
       {"MULTICOL",   0, EXCEPTION_IGNORE, 0, 0},
       {"NOBR",       1, 0, 0, 0},
       {"NOEMBED",    1, 0, 0, 0},
       {"NOFRAMES",   1, EXCEPTION_SKIP, 0, 0},
       {"NOLAYER",    1, 0, 0, 0},
       {"NOSCRIPT",   1, EXCEPTION_SKIP, 0, 0},
       {"OBJECT",     1, 0, 0, 0},
       {"OL",         1, 0, 0, 0},
       {"OPTGROUP",   1, 0, 0, 0},
       {"OPTION",     0, EXCEPTION_IGNORE, 0, 0},
       {"P",          0, EXCEPTION_IGNORE, 0, 0},
       {"PARAM",      0, EXCEPTION_IGNORE, 0, 0},
       {"PLAINTEXT",  0, EXCEPTION_IGNORE, 0, 0},
       {"PRE",        1, EXCEPTION_PRE_TO_TT, HTML_TAG_PRE, 0},
       {"Q",          1, 0, 0, 0},
       {"S",          1, 0, 0, 0},
       {"SAMP",       1, 0, 0, 0},
       {"SCRIPT",     1, EXCEPTION_SKIP, 0, 0},
       {"SELECT",     1, 0, 0, 0},
       {"SERVER",     1, EXCEPTION_SKIP, 0, 0},
       {"SMALL",      1, 0, 0, 0},
       {"SPACER",     0, EXCEPTION_IGNORE, 0, 0},
       {"SPAN",       1, 0, 0, 0},
       {"STRIKE",     1, 0, 0, 0},
       {"STRONG",     1, 0, 0, 0},
       {"STYLE",      1, 0, 0, 0},
       {"SUB",        1, 0, 0, 0},
       {"SUP",        1, 0, 0, 0},
       {"TABLE",      1, 0, 0, 0},
       {"TBODY",      0, EXCEPTION_IGNORE, 0, 0},
       {"TD",         0, EXCEPTION_IGNORE, HTML_TAG_TD, 0},
       {"TEXTAREA",   1, 0, 0, 0},
       {"TFOOT",      0, EXCEPTION_IGNORE, 0, 0},
       {"TH",         0, EXCEPTION_IGNORE, HTML_TAG_TH, 0},
       {"THEAD",      0, EXCEPTION_IGNORE, 0, 0},
       {"TITLE",      1, EXCEPTION_SKIP, 0, 0},
       {"TR",         1, 0, 0, 0},
       {"TT",         1, EXCEPTION_PRE_TO_TT, HTML_TAG_TT, 0},
       {"U",          1, EXCEPTION_FONT, 0, 0},
       {"UL",         1, 0, 0, 0},
       {"VAR",        1, 0, 0, 0},
       {"WBR",        0, EXCEPTION_IGNORE, 0, 0},
       {"XMP",        1, 0, 0, 0},
       {NULL,         0, 0, 0, 0},
};
