/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| FLAG_MAP.C  -  Table of which flags can be set on which types of objects.   |
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
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: flagset.h,v 1.1.1.1 2004/12/02 17:43:22 jpboggis Exp $

*/

#include "flagset.h"

/* ---->  Flags which may be set/reset on each type of object (Primary flags)  <---- */
int flag_map[] =
{
    /*  Free       */  0,
    /*  Thing      */  ABODE|ASHCAN|CENSOR|COMBAT|ELDER|HAVEN|INVISIBLE|LOCKED|OPAQUE|OPEN|OPENABLE|PERMANENT|PRIVATE|QUIET|READONLY|SHARABLE|STICKY|TRANSFERABLE|VISIBLE|YELL,
    /*  Exit       */  ASHCAN|INVISIBLE|OPEN|OPENABLE|OPAQUE|PERMANENT|PRIVATE|READONLY|SHARABLE|STICKY|TRANSFERABLE|VISIBLE,
    /*  Character  */  ANSI|APPRENTICE|ASHCAN|ASSISTANT|BBS_INFORM|BEING|BUILDER|CENSOR|DEITY|DRUID|ELDER|ENGAGED|EXPERIENCED|FRIENDS_INFORM|GENDER_MASK|HAVEN|LISTEN|MARRIED|MORON|NUMBER|PERMANENT|PRIVATE|QUIET|READONLY|SHOUT|TRACING|VISIBLE|WIZARD|YELL,
    /*  Room       */  ABODE|ASHCAN|CENSOR|COMBAT|ELDER|HAVEN|INVISIBLE|OPENABLE|PERMANENT|PRIVATE|QUIET|READONLY|SHARABLE|STICKY|TRANSFERABLE|VISIBLE|YELL,
    /*  Command    */  APPRENTICE|ASHCAN|CENSOR|COMBAT|ELDER|INVISIBLE|PERMANENT|PRIVATE|READONLY|SHARABLE|TRACING|TRANSFERABLE|VISIBLE|WIZARD,
    /*  Fuse       */  ASHCAN|INVISIBLE|PERMANENT|PRIVATE|READONLY|SHARABLE|STICKY|TOM|TRANSFERABLE|VISIBLE|WIZARD,
    /*  Alarm      */  ASHCAN|PERMANENT|PRIVATE|READONLY|SHARABLE|VISIBLE|WIZARD,
    /*  Variable   */  ASHCAN|PERMANENT|PRIVATE|READONLY|SHARABLE|TRANSFERABLE|VISIBLE,
    /*  Array      */  ASHCAN|PERMANENT|PRIVATE|READONLY|SHARABLE|TRANSFERABLE|VISIBLE,
    /*  Property   */  ASHCAN|PERMANENT|PRIVATE|READONLY|SHARABLE|TRANSFERABLE|VISIBLE,
};


/* ---->  Flags which may be set/reset on each type of object (Secondary flags)  <---- */
int flag_map2[] =
{
    /*  Free       */  0,
    /*  Thing      */  ARTICLE_MASK|EXPIRY|FINANCE|IMMOVABLE|INHERITABLE|READONLYDESC|SECRET|SECURE|SENDHOME|SKIPOBJECTS|TRANSPORT|VISIT|WARP,
    /*  Exit       */  ARTICLE_MASK|EXPIRY|IMMOVABLE|INHERITABLE|READONLYDESC|TRANSPORT,
    /*  Character  */  ARTICLE_MASK|ANSI8|BBS|EDIT_EVALUATE|EDIT_NUMBERING|EDIT_OVERWRITE|LFTOCR_CR|LFTOCR_CRLF|LFTOCR_LFCR|INHERITABLE|MAIL|READONLYDESC|RETIRED|SECRET,
    /*  Room       */  ARTICLE_MASK|EXPIRY|FINANCE|IMMOVABLE|INHERITABLE|READONLYDESC|SECRET|SECURE|SKIPOBJECTS|TRANSPORT|VISIT|WARP,
    /*  Command    */  EXPIRY|IMMOVABLE|INHERITABLE|READONLYDESC|VALIDATED,
    /*  Fuse       */  ABORTFUSE|EXPIRY|IMMOVABLE|INHERITABLE,
    /*  Alarm      */  EXPIRY|IMMOVABLE|INHERITABLE|READONLYDESC,
    /*  Variable   */  EXPIRY|IMMOVABLE|INHERITABLE|READONLYDESC,
    /*  Array      */  EXPIRY|IMMOVABLE|INHERITABLE,
    /*  Property   */  EXPIRY|IMMOVABLE|INHERITABLE|READONLYDESC,
};

