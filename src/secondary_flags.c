/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| SECONDARY_FLAGS.C  -  Definitions of secondary object/character flags.      |
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
| Module originally designed and written by:  J.P.Boggis 15/03/2000.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                        https://github.com/smcvey/tcz                        |
`-----------------------------------------------------------------------------'

*/

#include <stdlib.h>

#include "secondary_flags.h"

/* ---->  Telnet line termination preference flags  <---- */
struct flag_data flag_list_prefs[] =
{
       {"LFCR",                LFTOCR_MASK,    LFTOCR_LFCR,    '\0', 8|FLAG_INTERNAL|FLAG_NOT_DISPLAY, BOOLFLAG_LFTOCR_LFCR},
       {"LF+CR",               LFTOCR_MASK,    LFTOCR_LFCR,    '\0', 8|FLAG_INTERNAL|FLAG_PERMISSION, BOOLFLAG_LFTOCR_LFCR},
       {"CR",                  LFTOCR_MASK,    LFTOCR_CR,      '\0', 8|FLAG_INTERNAL|FLAG_PERMISSION, BOOLFLAG_LFTOCR_CR},
       {"CRLF",                LFTOCR_MASK,    LFTOCR_CRLF,    '\0', 8|FLAG_INTERNAL|FLAG_PERMISSION|FLAG_NOT_DISPLAY, BOOLFLAG_LFTOCR_CRLF},
       {"CR+LF",               LFTOCR_MASK,    LFTOCR_CRLF,    '\0', 8|FLAG_INTERNAL|FLAG_PERMISSION, BOOLFLAG_LFTOCR_CRLF},
};


/* ---->  Secondary object flags  <---- */
struct flag_data flag_list2[] =
{
       {"Abort",               ABORTFUSE,      ABORTFUSE,      'a',  4|FLAG_NOT_CHARACTER, 0},
       {"BBS",                 BBS,            BBS,            'b',  4|FLAG_LOG_CHARACTER|FLAG_REASON|FLAG_NOT_INCOMMAND|FLAG_NOT_OWN|FLAG_APPRENTICE|FLAG_PERMISSION, BOOLFLAG_BBS},
       {"Read-Only-Desc",      READONLYDESC,   READONLYDESC,   'd',  8|FLAG_PERMISSION, BOOLFLAG_READONLYDESC},
       {"Inheritable",         INHERITABLE,    INHERITABLE,    'i',  8|FLAG_NOT_CHCOMMAND|FLAG_NOT_OWN|FLAG_PERMISSION, BOOLFLAG_INHERITABLE},
       {"SkipObjects",         SKIPOBJECTS,    SKIPOBJECTS,    'o',  8|FLAG_PERMISSION|FLAG_NOT_CHARACTER|FLAG_NOT_INCOMMAND, 0},
       {"Secret",              SECRET,         SECRET,         's',  8, BOOLFLAG_SECRET},
       {"Warp",                WARP,           WARP,           'w',  8|FLAG_NOT_CHARACTER, 0},
       {"Expiry",              EXPIRY,         EXPIRY,         'x',  8|FLAG_NOT_CHARACTER, 0},
       {"Finance",             FINANCE,        FINANCE,        'F',  8|FLAG_NOT_CHARACTER, 0},
       {"SendHome",            SENDHOME,       SENDHOME,       'H',  8|FLAG_NOT_CHARACTER, 0},
       {"Immovable",           IMMOVABLE,      IMMOVABLE,      'I',  8|FLAG_NOT_CHARACTER, 0},
       {"Mail",                MAIL,           MAIL,           'M',  4|FLAG_LOG_CHARACTER|FLAG_REASON|FLAG_NOT_INCOMMAND|FLAG_NOT_OWN|FLAG_APPRENTICE|FLAG_PERMISSION, BOOLFLAG_MAIL},
       {"Visit",               VISIT,          VISIT,          'V',  8|FLAG_NOT_CHARACTER, 0},
       {"Retired",             RETIRED,        RETIRED,        'R',  4|FLAG_LOG_CHARACTER|FLAG_NOT_INCOMMAND|FLAG_NOT_PUPPET|FLAG_NOT_OWN|FLAG_REASON|FLAG_NOT_OBJECT, BOOLFLAG_RETIRED},
       {"Secure",              SECURE,         SECURE,         'S',  8|FLAG_NOT_CHARACTER, 0},
       {"Transport",           TRANSPORT,      TRANSPORT,      'T',  8|FLAG_NOT_CHARACTER, 0},
       {"Validated",           VALIDATED,      VALIDATED,      '@',  4|FLAG_NOT_CHARACTER|FLAG_NOT_INCOMMAND|FLAG_LOG_OBJECT, 0},
       {"8ColourANSI",         ANSI8,          ANSI8,          '\0', 8|FLAG_NOT_OBJECT|FLAG_INTERNAL|FLAG_NOT_DISPLAY, 0},
       {"8-Colour-ANSI",       ANSI8,          ANSI8,          '\0', 8|FLAG_NOT_OBJECT|FLAG_INTERNAL|FLAG_PERMISSION, 0},
       {"ChannelOperator",     CHAT_OPERATOR,  CHAT_OPERATOR,  '\0', 8|FLAG_INTERNAL|FLAG_NOT_DISPLAY|FLAG_NOT_OBJECT, BOOLFLAG_CHAT_OPERATOR},
       {"Channel-Operator",    CHAT_OPERATOR,  CHAT_OPERATOR,  '\0', 8|FLAG_INTERNAL|FLAG_NOT_OBJECT, BOOLFLAG_CHAT_OPERATOR},
       {"ChatOperator",        CHAT_OPERATOR,  CHAT_OPERATOR,  '\0', 8|FLAG_INTERNAL|FLAG_NOT_OBJECT|FLAG_NOT_DISPLAY, BOOLFLAG_CHAT_OPERATOR},
       {"Chat-Operator",       CHAT_OPERATOR,  CHAT_OPERATOR,  '\0', 8|FLAG_INTERNAL|FLAG_NOT_OBJECT|FLAG_NOT_DISPLAY, BOOLFLAG_CHAT_OPERATOR},
       {"ChannelPrivate",      CHAT_PRIVATE,   CHAT_PRIVATE,   '\0', 8|FLAG_INTERNAL|FLAG_NOT_OBJECT|FLAG_NOT_DISPLAY, BOOLFLAG_CHAT_PRIVATE},
       {"Channel-Private",     CHAT_PRIVATE,   CHAT_PRIVATE,   '\0', 8|FLAG_INTERNAL|FLAG_NOT_OBJECT|FLAG_NOT_DISPLAY, BOOLFLAG_CHAT_PRIVATE},
       {"ChatPrivate",         CHAT_PRIVATE,   CHAT_PRIVATE,   '\0', 8|FLAG_INTERNAL|FLAG_NOT_OBJECT|FLAG_NOT_DISPLAY, BOOLFLAG_CHAT_PRIVATE},
       {"Chat-Private",        CHAT_PRIVATE,   CHAT_PRIVATE,   '\0', 8|FLAG_INTERNAL|FLAG_NOT_OBJECT|FLAG_NOT_DISPLAY, BOOLFLAG_CHAT_PRIVATE},
       {"Echo",                LOCAL_ECHO,     LOCAL_ECHO,     '\0', 8|FLAG_INTERNAL|FLAG_PERMISSION|FLAG_NOT_OBJECT, BOOLFLAG_ECHO},
       {"Evaluate",            EDIT_EVALUATE,  EDIT_EVALUATE,  '\0', 8|FLAG_INTERNAL|FLAG_PERMISSION|FLAG_NOT_OBJECT, BOOLFLAG_EDIT_EVALUATE},
       {"Financial",           FINANCE,        FINANCE,        '\0', 8|FLAG_NOT_DISPLAY|FLAG_NOT_CHARACTER, 0},
       {"ForwardEmail",        FORWARD_EMAIL,  FORWARD_EMAIL,  '\0', 8|FLAG_INTERNAL|FLAG_NOT_DISPLAY|FLAG_NOT_OBJECT, 0},
       {"FriendsChat",         FRIENDS_CHAT,   FRIENDS_CHAT,   '\0', 8|FLAG_INTERNAL|FLAG_NOT_DISPLAY|FLAG_NOT_OBJECT, 0},
       {"Friends-Chat",        FRIENDS_CHAT,   FRIENDS_CHAT,   '\0', 8|FLAG_INTERNAL|FLAG_NOT_OBJECT, 0},
       {"GoHome",              SENDHOME,       SENDHOME,       '\0', 8|FLAG_NOT_DISPLAY|FLAG_NOT_CHARACTER, 0},
       {"Home",                SENDHOME,       SENDHOME,       '\0', 8|FLAG_NOT_DISPLAY|FLAG_NOT_CHARACTER, 0},
       {"LineNumbers",         EDIT_NUMBERING, EDIT_NUMBERING, '\0', 8|FLAG_INTERNAL|FLAG_NOT_DISPLAY|FLAG_NOT_OBJECT, BOOLFLAG_EDIT_NUMBERING},
       {"Line-Numbers",        EDIT_NUMBERING, EDIT_NUMBERING, '\0', 8|FLAG_INTERNAL|FLAG_PERMISSION|FLAG_NOT_OBJECT, BOOLFLAG_EDIT_NUMBERING},
       {"MorePager",           MORE_PAGER,     MORE_PAGER,     '\0', 8|FLAG_INTERNAL|FLAG_NOT_DISPLAY, BOOLFLAG_MORE},
       {"More-Pager",          MORE_PAGER,     MORE_PAGER,     '\0', 8|FLAG_INTERNAL|FLAG_PERMISSION, BOOLFLAG_MORE},
       {"NonExecutable",       NON_EXECUTABLE, NON_EXECUTABLE, '\0', 8|FLAG_INTERNAL|FLAG_NOT_DISPLAY|FLAG_NOT_CHARACTER, 0},
       {"Non-Executable",      NON_EXECUTABLE, NON_EXECUTABLE, '\0', 8|FLAG_INTERNAL|FLAG_NOT_CHARACTER, 0},
       {"Numbering",           EDIT_NUMBERING, EDIT_NUMBERING, '\0', 8|FLAG_INTERNAL|FLAG_NOT_DISPLAY|FLAG_NOT_OBJECT, BOOLFLAG_EDIT_NUMBERING},
       {"Numbers",             EDIT_NUMBERING, EDIT_NUMBERING, '\0', 8|FLAG_INTERNAL|FLAG_NOT_DISPLAY|FLAG_NOT_OBJECT, BOOLFLAG_EDIT_NUMBERING},
       {"Overwrite",           EDIT_OVERWRITE, EDIT_OVERWRITE, '\0', 8|FLAG_INTERNAL|FLAG_PERMISSION|FLAG_NOT_OBJECT, BOOLFLAG_EDIT_OVERWRITE},
       {"PageBell",            PAGEBELL,       PAGEBELL,       '\0', 8|FLAG_INTERNAL|FLAG_PERMISSION|FLAG_NOT_OBJECT, BOOLFLAG_PAGEBELL},
       {"Pager",               MORE_PAGER,     MORE_PAGER,     '\0', 8|FLAG_INTERNAL|FLAG_NOT_DISPLAY, BOOLFLAG_MORE},
       {"ReadOnlyDesc",        READONLYDESC,   READONLYDESC,   '\0', 8|FLAG_NOT_DISPLAY, BOOLFLAG_READONLYDESC},
       {"Read-OnlyDesc",       READONLYDESC,   READONLYDESC,   '\0', 8|FLAG_NOT_DISPLAY, BOOLFLAG_READONLYDESC},
       {"ReadOnly-Desc",       READONLYDESC,   READONLYDESC,   '\0', 8|FLAG_NOT_DISPLAY, BOOLFLAG_READONLYDESC},
       {"Security",            SECURE,         SECURE,         '\0', 8|FLAG_NOT_DISPLAY|FLAG_NOT_CHARACTER, 0},
       {"Underline",           UNDERLINE,      UNDERLINE,      '\0', 8|FLAG_INTERNAL|FLAG_PERMISSION|FLAG_NOT_OBJECT, BOOLFLAG_UNDERLINE},
       {"Connected",           CONNECTED,      CONNECTED,      '\0', 8|FLAG_INTERNAL|FLAG_NOT_OBJECT, BOOLFLAG_CONNECTED},

       /* ---->  Article flags  <---- */
       {"Consonant (A)",       ARTICLE_MASK,   ARTICLE_CONSONANT << ARTICLE_SHIFT,'\0', 8|FLAG_PERMISSION, BOOLFLAG_ARTICLE_CONSONANT},
       {"Plural (Some)",       ARTICLE_MASK,   ARTICLE_PLURAL    << ARTICLE_SHIFT,'\0', 8|FLAG_PERMISSION, BOOLFLAG_ARTICLE_PLURAL},
       {"Vowel (An)",          ARTICLE_MASK,   ARTICLE_VOWEL     << ARTICLE_SHIFT,'\0', 8|FLAG_PERMISSION, BOOLFLAG_ARTICLE_VOWEL},
       {"Consonant",           ARTICLE_MASK,   ARTICLE_CONSONANT << ARTICLE_SHIFT,'\0', 8|FLAG_NOT_DISPLAY, BOOLFLAG_ARTICLE_CONSONANT},
       {"Plural",              ARTICLE_MASK,   ARTICLE_PLURAL    << ARTICLE_SHIFT,'\0', 8|FLAG_NOT_DISPLAY, BOOLFLAG_ARTICLE_PLURAL},
       {"Vowel",               ARTICLE_MASK,   ARTICLE_VOWEL     << ARTICLE_SHIFT,'\0', 8|FLAG_NOT_DISPLAY, BOOLFLAG_ARTICLE_VOWEL},
       {"A",                   ARTICLE_MASK,   ARTICLE_CONSONANT << ARTICLE_SHIFT,'\0', 8|FLAG_NOT_DISPLAY, BOOLFLAG_ARTICLE_CONSONANT},
       {"An",                  ARTICLE_MASK,   ARTICLE_VOWEL     << ARTICLE_SHIFT,'\0', 8|FLAG_NOT_DISPLAY, BOOLFLAG_ARTICLE_VOWEL},
       {"Some",                ARTICLE_MASK,   ARTICLE_PLURAL    << ARTICLE_SHIFT,'\0', 8|FLAG_NOT_DISPLAY, BOOLFLAG_ARTICLE_PLURAL},
       {NULL,                  0,              0,              '\0', 8|FLAG_INTERNAL, 0}
};
