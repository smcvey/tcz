/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| SEARCH_ARRAYS.C  -  Definitions for SEARCH.C, implementing object           |
|                     search/listing commands such as '@find', '@list', etc.  |
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

#include <stdlib.h>

#include "search.h"
#include "flagset.h"

/* ---->  Standard object type based search list  <---- */
struct search_data search_objecttype[] =
{
       {"Alarms",           SEARCH_ALARM,     0, 0},
       {"Arrays",           SEARCH_ARRAY,     0, 0},
       {"BannedCharacters", SEARCH_BANNED,    0, 0},
       {"BannedPlayers",    SEARCH_BANNED,    0, 0},
       {"Characters",       SEARCH_CHARACTER, 0, 0},
       {"Chars",            SEARCH_CHARACTER, 0, 0},
       {"Commands",         SEARCH_COMMAND,   0, 0},
       {"CompoundCommands", SEARCH_COMMAND,   0, 0},
       {"Contents",         SEARCH_CONTENTS,  0, 0},
       {"DynamicArrays",    SEARCH_ARRAY,     0, 0},
       {"Exits",            SEARCH_EXIT,      0, 0},
       {"Fuses",            SEARCH_FUSE,      0, 0},
       {"FloatingObjects",  SEARCH_FLOATING,  0, 0},
       {"Globals",          SEARCH_GLOBAL,    0, 0},
       {"GlobalCommands",   SEARCH_GLOBAL,    0, 0},
       {"GlobalCompoundCommands", SEARCH_GLOBAL, 0, 0},
       {"JunkedObjects",    SEARCH_JUNKED,    0, 0},
       {"JunkObjects",      SEARCH_JUNKED,    0, 0},
       {"Locations",        SEARCH_ROOM,      0, 0},
       {"Objects",          SEARCH_THING,     0, 0},
       {"Players",          SEARCH_CHARACTER, 0, 0},
       {"Properties",       SEARCH_PROPERTY,  0, 0},
       {"Puppets",          SEARCH_PUPPET,    0, 0},
       {"Rooms",            SEARCH_ROOM,      0, 0},
       {"Things",           SEARCH_THING,     0, 0},
       {"Variables",        SEARCH_VARIABLE,  0, 0},
       {"Vars",             SEARCH_VARIABLE,  0, 0},
       {NULL,               0,                0, 0}
};


/* ---->  Standard object field based search list  <---- */
struct search_data search_fieldtype[] =
{
       {"Areanames",           SEARCH_AREANAME, 0, SEARCH_NOT_CHARACTER},
       {"Adescriptions",       SEARCH_DESC,     0, SEARCH_ALARM},
       {"Anames",              SEARCH_NAME,     0, SEARCH_ALARM},
       {"Contentsstrings",     SEARCH_CSTRING,  0, SEARCH_NOT_CHARACTER},
       {"Cdescriptions",       SEARCH_DESC,     0, SEARCH_COMMAND},
       {"Cnames",              SEARCH_NAME,     0, SEARCH_COMMAND},
       {"Cstrings",            SEARCH_CSTRING,  0, SEARCH_NOT_CHARACTER},
       {"Descriptions",        SEARCH_DESC,     0, 0},
       {"Drops",               SEARCH_DROP,     0, SEARCH_NOT_CHARACTER},
       {"Emails",              SEARCH_EMAIL,    0, SEARCH_CHARACTER},
       {"Edescriptions",       SEARCH_DESC,     0, SEARCH_EXIT},
       {"Enames",              SEARCH_NAME,     0, SEARCH_EXIT},
       {"Estrings",            SEARCH_ESTRING,  0, SEARCH_NOT_CHARACTER},
       {"Exitstrings",         SEARCH_ESTRING,  0, SEARCH_NOT_CHARACTER},
       {"Exitsstrings",        SEARCH_ESTRING,  0, SEARCH_NOT_CHARACTER},
       {"E-mails",             SEARCH_EMAIL,    0, SEARCH_NOT_OBJECT},
       {"Failures",            SEARCH_FAIL,     0, SEARCH_NOT_CHARACTER},
       {"Fdescriptions",       SEARCH_DESC,     0, SEARCH_FUSE},
       {"Fnames",              SEARCH_NAME,     0, SEARCH_FUSE},
       {"Homepages",           SEARCH_WWW,      0, SEARCH_NOT_OBJECT},
       {"Lastsites",           SEARCH_LASTSITE, 0, SEARCH_NOT_OBJECT},
       {"Names",               SEARCH_NAME,     0, 0},
       {"Nameprefixes",        SEARCH_PREFIX,   0, SEARCH_NOT_OBJECT},
       {"Namesuffixes",        SEARCH_SUFFIX,   0, SEARCH_NOT_OBJECT},
       {"Odescriptions",       SEARCH_ODESC,    0, SEARCH_NOT_CHARACTER},
       {"Odrops",              SEARCH_ODROP,    0, SEARCH_NOT_CHARACTER},
       {"Ofailures",           SEARCH_OFAIL,    0, SEARCH_NOT_CHARACTER},
       {"Osuccesses",          SEARCH_OSUCC,    0, SEARCH_NOT_CHARACTER},
       {"Othersdescriptions",  SEARCH_ODESC,    0, SEARCH_NOT_CHARACTER},
       {"Othersdrops",         SEARCH_ODROP,    0, SEARCH_NOT_CHARACTER},
       {"Othersfailures",      SEARCH_OFAIL,    0, SEARCH_NOT_CHARACTER},
       {"Otherssuccesses",     SEARCH_OSUCC,    0, SEARCH_NOT_CHARACTER},
       {"Outside",             SEARCH_ODESC,    0, SEARCH_NOT_CHARACTER},
       {"Outsidedescriptions", SEARCH_ODESC,    0, SEARCH_NOT_CHARACTER},
       {"Prefixes",            SEARCH_PREFIX,   0, SEARCH_NOT_OBJECT},
       {"Pdescriptions",       SEARCH_DESC,     0, SEARCH_NOT_OBJECT},
       {"Pnames",              SEARCH_NAME,     0, SEARCH_NOT_OBJECT},
       {"Races",               SEARCH_RACE,     0, SEARCH_NOT_OBJECT},
       {"Rdescriptions",       SEARCH_DESC,     0, SEARCH_ROOM},
       {"Rnames",              SEARCH_NAME,     0, SEARCH_ROOM},
       {"Successes",           SEARCH_SUCC,     0, SEARCH_NOT_CHARACTER},
       {"Sites",               SEARCH_LASTSITE, 0, SEARCH_NOT_OBJECT},
       {"Strings",             SEARCH_CSTRING,  0, SEARCH_NOT_CHARACTER},
       {"Suffixes",            SEARCH_SUFFIX,   0, SEARCH_NOT_OBJECT},
       {"Tdescriptions",       SEARCH_DESC,     0, SEARCH_THING},
       {"Titles",              SEARCH_TITLE,    0, SEARCH_NOT_OBJECT},
       {"Tnames",              SEARCH_NAME,     0, SEARCH_THING},
       {"Vdescriptions",       SEARCH_DESC,     0, SEARCH_VARIABLE},
       {"Vnames",              SEARCH_NAME,     0, SEARCH_VARIABLE},
       {"Whomessages",         SEARCH_TITLE,    0, SEARCH_NOT_OBJECT},
       {"Whostrings",          SEARCH_TITLE,    0, SEARCH_NOT_OBJECT},
       {"Webpages",            SEARCH_WWW,      0, SEARCH_NOT_OBJECT},
       {"WorldWideWeb",        SEARCH_WWW,      0, SEARCH_NOT_OBJECT},
       {"WWW",                 SEARCH_WWW,      0, SEARCH_NOT_OBJECT},
       {"@areanames",          SEARCH_AREANAME, 0, SEARCH_NOT_CHARACTER},
       {"@cstrings",           SEARCH_CSTRING,  0, SEARCH_NOT_CHARACTER},
       {"@descriptions",       SEARCH_DESC,     0, 0},
       {"@drops",              SEARCH_DROP,     0, SEARCH_NOT_CHARACTER},
       {"@emails",             SEARCH_EMAIL,    0, SEARCH_NOT_OBJECT},
       {"@estrings",           SEARCH_ESTRING,  0, SEARCH_NOT_CHARACTER},
       {"@failures",           SEARCH_FAIL,     0, SEARCH_NOT_CHARACTER},
       {"@names",              SEARCH_NAME,     0, 0},
       {"@odescriptions",      SEARCH_ODESC,    0, SEARCH_NOT_CHARACTER},
       {"@odrops",             SEARCH_ODROP,    0, SEARCH_NOT_CHARACTER},
       {"@ofailures",          SEARCH_OFAIL,    0, SEARCH_NOT_CHARACTER},
       {"@osuccesses",         SEARCH_OSUCC,    0, SEARCH_NOT_CHARACTER},
       {"@prefixes",           SEARCH_PREFIX,   0, SEARCH_NOT_OBJECT},
       {"@races",              SEARCH_RACE,     0, SEARCH_NOT_OBJECT},
       {"@suffixes",           SEARCH_SUFFIX,   0, SEARCH_NOT_OBJECT},
       {"@whomessages",        SEARCH_TITLE,    0, SEARCH_NOT_OBJECT},
       {"@whostrings",         SEARCH_TITLE,    0, SEARCH_NOT_OBJECT},
       {"@www",                SEARCH_WWW,      0, SEARCH_NOT_OBJECT},
       {NULL,                  0,               0, 0}
};


/* ---->  Primary flags based search list  <---- */
struct search_data search_flagtype[] =
{
       {"Abodes",            ABODE,                         0,           0},
       {"Administrators",    APPRENTICE|WIZARD|ELDER|DEITY, 0,           SEARCH_NOT_OBJECT},
       {"Administration",    APPRENTICE|WIZARD|ELDER|DEITY, 0,           SEARCH_NOT_OBJECT},
       {"Ashcanned",         ASHCAN,                        0,           0},
       {"Ansicolours",       ANSI,                          0,           SEARCH_NOT_OBJECT},
       {"Ansicolors",        ANSI,                          0,           SEARCH_NOT_OBJECT},
       {"Apprentices",       APPRENTICE,                    0,           0},
       {"Apps",              APPRENTICE,                    0,           0},
       {"Assistants",        ASSISTANT,                     0,           SEARCH_NOT_OBJECT},
       {"BBSInform",         BBS_INFORM,                    0,           SEARCH_NOT_OBJECT},
       {"BBS-Inform",        BBS_INFORM,                    0,           SEARCH_NOT_OBJECT},
       {"BBSNotify",         BBS_INFORM,                    0,           SEARCH_NOT_OBJECT},
       {"BBS-Notify",        BBS_INFORM,                    0,           SEARCH_NOT_OBJECT},
       {"Beings",            BEING,                         0,           SEARCH_NOT_OBJECT},
       {"Boot",              BOOT,                          0,           SEARCH_NOT_OBJECT},
       {"Boys",              GENDER_MALE << GENDER_SHIFT,   GENDER_MASK, SEARCH_NOT_OBJECT},
       {"Builders",          BUILDER,                       0,           SEARCH_NOT_OBJECT},
       {"Censored",          CENSOR,                        0,           0},
       {"Censorship",        CENSOR,                        0,           0},
       {"Chown_OK",          TRANSFERABLE,                  0,           0},
       {"ChownOK",           TRANSFERABLE,                  0,           0},
       {"Colours",           ANSI,                          0,           SEARCH_NOT_OBJECT},
       {"Colors",            ANSI,                          0,           SEARCH_NOT_OBJECT},
       {"CombatAreas",       COMBAT,                        0,           SEARCH_NOT_CHARACTER},
       {"Constructors",      BUILDER,                       0,           SEARCH_NOT_OBJECT},
       {"Containers",        OPEN|OPENABLE,                 0,           0},
       {"Dark",              INVISIBLE,                     0,           0},
       {"Deities",           DEITY,                         0,           0},
       {"Deity",             DEITY,                         0,           0},
       {"Druids",            DRUID,                         0,           SEARCH_NOT_OBJECT},
       {"Elders",            ELDER,                         0,           0},
       {"Engaged",           ENGAGED,                       0,           SEARCH_NOT_OBJECT},
       {"Experienced",       EXPERIENCED,                   0,           SEARCH_NOT_OBJECT},
       {"Females",           GENDER_FEMALE << GENDER_SHIFT, GENDER_MASK, SEARCH_NOT_OBJECT},
       {"FriendsInform",     FRIENDS_INFORM,                0,           SEARCH_NOT_OBJECT},
       {"FriendsNotify",     FRIENDS_INFORM,                0,           SEARCH_NOT_OBJECT},
       {"Friends-Inform",    FRIENDS_INFORM,                0,           SEARCH_NOT_OBJECT},
       {"Friends-Notify",    FRIENDS_INFORM,                0,           SEARCH_NOT_OBJECT},
       {"Girls",             GENDER_FEMALE << GENDER_SHIFT, GENDER_MASK, SEARCH_NOT_OBJECT},
       {"Gods",              DEITY,                         0,           SEARCH_NOT_OBJECT},
       {"Havened",           HAVEN,                         0,           0},
       {"Help",              EXPERIENCED|HELP,              0,           SEARCH_NOT_OBJECT},
       {"Invisible",         INVISIBLE,                     0,           0},
       {"Listeners",         LISTEN,                        0,           SEARCH_NOT_OBJECT},
       {"Locked",            LOCKED,                        0,           SEARCH_NOT_CHARACTER},
       {"Locks",             LOCKED,                        0,           SEARCH_NOT_CHARACTER},
       {"Males",             GENDER_MALE << GENDER_SHIFT,   GENDER_MASK, SEARCH_NOT_OBJECT},
       {"Marry",             MARRIED,                       0,           SEARCH_NOT_OBJECT},
       {"Married",           MARRIED,                       0,           SEARCH_NOT_OBJECT},
       {"Men",               GENDER_MALE << GENDER_SHIFT,   GENDER_MASK, SEARCH_NOT_OBJECT},
       {"Morons",            MORON,                         0,           SEARCH_NOT_OBJECT},
       {"Neutered",          GENDER_NEUTER << GENDER_SHIFT, GENDER_MASK, SEARCH_NOT_OBJECT},
       {"Number",            NUMBER,                        0,           SEARCH_NOT_OBJECT},
       {"Object",            OBJECT,                        0,           0},
       {"Opaque",            OPAQUE,                        0,           SEARCH_NOT_CHARACTER},
       {"Openable",          OPENABLE,                      0,           SEARCH_NOT_CHARACTER},
       {"Opened",            OPEN,                          0,           SEARCH_NOT_CHARACTER},
       {"Permanent",         PERMANENT,                     0,           0},
       {"Private",           PRIVATE,                       0,           0},
       {"Quiet",             QUIET,                         0,           0},
       {"ReadOnly",          READONLY,                      0,           0},
       {"Read-Only",         READONLY,                      0,           0},
       {"Sharable",          SHARABLE,                      0,           0},
       {"Shout",             SHOUT,                         0,           SEARCH_NOT_OBJECT},
       {"Sticky",            STICKY,                        0,           0},
       {"Tom",               TOM,                           0,           SEARCH_NOT_CHARACTER},
       {"Tracing",           TRACING,                       0,           0},
       {"Transferable",      TRANSFERABLE,                  0,           0},
       {"Visible",           VISIBLE,                       0,           0},
       {"Wizards",           WIZARD,                        0,           0},
       {"Wizes",             WIZARD,                        0,           0},
       {"Wizs",              WIZARD,                        0,           0},
       {"Women",             GENDER_FEMALE << GENDER_SHIFT, GENDER_MASK, SEARCH_NOT_OBJECT},
       {"Yell",              YELL,                          0,           0},
       {NULL,                0,                             0,           0}
};


/* ---->  Secondary flags based search list  <---- */
struct search_data search_flagtype2[] =
{
       {"8ColorAnsi",        ANSI8,                         0,           SEARCH_NOT_OBJECT},
       {"8ColourAnsi",       ANSI8,                         0,           SEARCH_NOT_OBJECT},
       {"8-Color-Ansi",      ANSI8,                         0,           SEARCH_NOT_OBJECT},
       {"8-Colour-Ansi",     ANSI8,                         0,           SEARCH_NOT_OBJECT},
       {"A",                 ARTICLE_CONSONANT << ARTICLE_SHIFT, ARTICLE_MASK, 0},
       {"Abort",             ABORTFUSE,                     0,           SEARCH_FUSE},
       {"AbortFuses",        ABORTFUSE,                     0,           SEARCH_FUSE},
       {"An",                ARTICLE_VOWEL     << ARTICLE_SHIFT, ARTICLE_MASK, 0},
       {"BBS",               BBS,                           0,           SEARCH_NOT_OBJECT},
       {"ChannelOperator",   CHAT_OPERATOR,                 0,           SEARCH_NOT_OBJECT},
       {"Channel-Operator",  CHAT_OPERATOR,                 0,           SEARCH_NOT_OBJECT},
       {"ChatOperator",      CHAT_OPERATOR,                 0,           SEARCH_NOT_OBJECT},
       {"Chat-Operator",     CHAT_OPERATOR,                 0,           SEARCH_NOT_OBJECT},
       {"ChannelPrivate",    CHAT_PRIVATE,                  0,           SEARCH_NOT_OBJECT},
       {"Channel-Private",   CHAT_PRIVATE,                  0,           SEARCH_NOT_OBJECT},
       {"ChatPrivate",       CHAT_PRIVATE,                  0,           SEARCH_NOT_OBJECT},
       {"Chat-Private",      CHAT_PRIVATE,                  0,           SEARCH_NOT_OBJECT},
       {"Connected",         CONNECTED,                     0,           SEARCH_NOT_OBJECT},
       {"Consonants",        ARTICLE_CONSONANT << ARTICLE_SHIFT, ARTICLE_MASK, 0},
       {"CR",                LFTOCR_CR,                     LFTOCR_MASK, SEARCH_NOT_OBJECT},
       {"CR+LF",             LFTOCR_CRLF,                   LFTOCR_MASK, SEARCH_NOT_OBJECT},
       {"Echo",              LOCAL_ECHO,                    0,           SEARCH_NOT_OBJECT},
       {"Evaluate",          EDIT_EVALUATE,                 0,           SEARCH_NOT_OBJECT},
       {"Expiry",            EXPIRY,                        0,           SEARCH_NOT_CHARACTER},
       {"FChat",             FRIENDS_CHAT,                  0,           SEARCH_NOT_OBJECT},
       {"ForwardEmail",      FORWARD_EMAIL,                 0,           SEARCH_NOT_OBJECT},
       {"Friends-Chat",      FRIENDS_CHAT,                  0,           SEARCH_NOT_OBJECT},
       {"FriendsChat",       FRIENDS_CHAT,                  0,           SEARCH_NOT_OBJECT},
       {"GoHome",            SENDHOME,                      0,           SEARCH_NOT_CHARACTER},
       {"Home",              SENDHOME,                      0,           SEARCH_NOT_CHARACTER},
       {"HTML",              HTML,                          0,           SEARCH_NOT_OBJECT},
       {"Immovable",         IMMOVABLE,                     0,           SEARCH_NOT_CHARACTER},
       {"Inheritable",       INHERITABLE,                   0,           0},
       {"LF+CR",             LFTOCR_LFCR,                   LFTOCR_MASK, SEARCH_NOT_OBJECT},
       {"LocalEcho",         LOCAL_ECHO,                    0,           SEARCH_NOT_OBJECT},
       {"Mail",              MAIL,                          0,           SEARCH_NOT_OBJECT},
       {"More-Paging",       MORE_PAGER,                    0,           SEARCH_NOT_OBJECT},
       {"More-Pager",        MORE_PAGER,                    0,           SEARCH_NOT_OBJECT},
       {"MorePager",         MORE_PAGER,                    0,           SEARCH_NOT_OBJECT},
       {"MorePaging",        MORE_PAGER,                    0,           SEARCH_NOT_OBJECT},
       {"Overwrite",         EDIT_OVERWRITE,                0,           SEARCH_NOT_OBJECT},
       {"PageBell",          PAGEBELL,                      0,           SEARCH_NOT_OBJECT},
       {"Pager",             MORE_PAGER,                    0,           SEARCH_NOT_OBJECT},
       {"Plurals",           ARTICLE_PLURAL << ARTICLE_SHIFT, ARTICLE_MASK, 0},
       {"RetiredAdministators", RETIRED,                    0,           SEARCH_NOT_OBJECT},
       {"RetiredAdministation", RETIRED,                    0,           SEARCH_NOT_OBJECT},
       {"Secret",            SECRET,                        0,           0},
       {"SendHome",          SENDHOME,                      0,           SEARCH_NOT_CHARACTER},
       {"Some",              ARTICLE_PLURAL << ARTICLE_SHIFT, ARTICLE_MASK, 0},
       {"Transport",         TRANSPORT,                     0,           SEARCH_NOT_CHARACTER},
       {"Underline",         UNDERLINE,                     0,           SEARCH_NOT_OBJECT},
       {"Validated",         VALIDATED,                     0,           SEARCH_NOT_CHARACTER},
       {"Vehicles",          TRANSPORT,                     0,           SEARCH_NOT_CHARACTER},
       {"Visit",             VISIT,                         0,           SEARCH_NOT_CHARACTER},
       {"Vowels",            ARTICLE_VOWEL  << ARTICLE_SHIFT, ARTICLE_MASK, 0},
       {"Warp",              WARP,                          0,           SEARCH_NOT_CHARACTER},
       {NULL,                0,                             0,           0}
};
