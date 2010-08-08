/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| HTML.H  -  Definitions for HTML.C, which implements TCZ's World Wide Web    |
|            Interface and HTML support.                                      |
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
| Module originally designed and written by:  J.P.Boggis 09/12/2003.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: html.h,v 1.1.1.1 2004/12/02 17:43:24 jpboggis Exp $

*/


/* ---->  HTML code generation flags  <---- */
#define HTML_CODE_HEADER          0x01   /*  Include HTML header information  */
#define HTML_CODE_HTML            0x02   /*  Include <HTML> ... </HTML> tags  */
#define HTML_CODE_HEAD            0x04   /*  Include <HEAD> ... </HEAD> tags  */
#define HTML_CODE_TITLE           0x08   /*  Include <TITLE> tags  */
#define HTML_CODE_BODY            0x10   /*  Include <BODY> ... </BODY> tags  */
#define HTML_CODE_LOGO            0x20   /*  Include graphical TCZ logo  */
#define HTML_CODE_BACK            0x40   /*  Include [BACK] link  */
#define HTML_CODE_ERROR           (HTML_CODE_HTML|HTML_CODE_HEAD|HTML_CODE_TITLE|HTML_CODE_BODY|HTML_CODE_LOGO|HTML_CODE_BACK)


/* ---->  HTML text flags  <---- */
#define HTML_FONT            0x01000000  /*  <FONT> tag in use             */
#define HTML_NEWLINE         0x02000000  /*  Last character was a NEWLINE  */
#define HTML_TAG             0x04000000  /*  Open HTML tag                 */
#define HTML_PREFORMAT       0x08000000  /*  Preformatted text             */
#define HTML_COMMENT         0x10000000  /*  HTML comment                  */
#define HTML_SPAN_BACKGROUND 0x80000000  /*  <SPAN STYLE="background:<COLOUR>;"> tag in use  */


/* ---->  HTML flags  <---- */
#define HTML_START           0x0001  /*  Input pending, start reading  */
#define HTML_INPUT_PENDING   0x0002  /*  Further input pending  */
#define HTML_SIMULATE_ANSI   0x0004  /*  Simulate ANSI colour via font colour tags  */
#define HTML_WHITE_AS_BLACK  0x0008  /*  Display white text as black text      */
#define HTML_BGRND           0x0010  /*  Use alternative background image  */
#define HTML_UNDERLINE       0x0020  /*  Enable use of underline tags for underlined text (Otherwise, use italic tags.)  */
#define HTML_INPUT           0x0040  /*  HTML input window  */
#define HTML_OUTPUT          0x0080  /*  HTML output window (Continuously connected)  */
#define HTML_JAVA            0x0100  /*  Enable JAVA Script enhancements  */
#define HTML_SCROLL          0x0200  /*  Enable old JAVA Script automatic scrolling using scroll(x,y);  */
#define HTML_SCROLLBY        0x0400  /*  Enable JAVA Script 1.2 automatic scrolling using scrollBy(x,y) function  */
#define HTML_FOCUS           0x0800  /*  Enable use of focus() method to automatically set focus back to 'TCZ command:' window  */
#define HTML_SSL             0x1000  /*  Enable SSL (Secure Socket Layer) connection  */
#define HTML_LINKS           0x2000  /*  Enable automatic HTML links  */
#define HTML_SMILEY          0x4000  /*  Enable graphical smileys (Emoticons)  */
#define HTML_STYLE           0x8000  /*  Enable Cascading Style Sheets enhancements (Text backgrounds, etc.)  */

#define HTML_CMDWIDTH        80      /*  Default width of command input box (Characters)  */

#define HTML_DEFAULT_FLAGS    (HTML_SIMULATE_ANSI|HTML_UNDERLINE|HTML_JAVA|HTML_SCROLLBY|HTML_FOCUS|HTML_LINKS|HTML_SMILEY|HTML_STYLE)  /*  Default HTML flags for new character  */
#define HTML_PREFERENCE_FLAGS (HTML_SIMULATE_ANSI|HTML_UNDERLINE|HTML_BGRND|HTML_JAVA|HTML_SCROLL|HTML_SCROLLBY|HTML_FOCUS|HTML_LINKS|HTML_SMILEY|HTML_STYLE|HTML_SSL|HTML_WHITE_AS_BLACK)  /*  HTML settable preference flags  */


/* ---->  HTML protocol identifiers  <---- */
#define HTML_PROTOCOL_HTTP_1_0  1  /*  HTTP 1.0  */
#define HTML_PROTOCOL_HTTP_1_1  2  /*  HTTP 1.1  */


/* ---->  HTML tag stack data structure  <---- */
struct html_tag_data {
       struct   html_tag_data *next;  /*  Next tag on stack  */
       unsigned short         ptr;    /*  Pointer to tag in tag table  */
};


/* ---->  HTML connection data structure  <---- */
struct html_data {
       struct         html_tag_data *tag;  /*  HTML tag stack              */
       char           *background;         /*  URL of background image     */
       unsigned long  identifier;          /*  Connection identifier       */
       int            txtflags;            /*  Text formatting flags       */
       unsigned char  cmdwidth;            /*  Width of command input box  */
       unsigned char  protocol;            /*  HTTP protocol               */
       unsigned long  id1,id2;             /*  64-bit identity code        */
       unsigned short flags;               /*  HTML connection flags       */
};
