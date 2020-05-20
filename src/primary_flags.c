/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| PRIMARY_FLAGS.C  -  Definitions of primary object/character flags.          |
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

#include "primary_flags.h"

/* ---->  Primary flags  <---- */
struct flag_data flag_list[] =
{
       {"Abode",               ABODE,        ABODE,        'A',  8|FLAG_NOT_CHARACTER|FLAG_PERMISSION, 0},
       {"Apprentice",          APPRENTICE,   APPRENTICE,   'A',  3|FLAG_LOG|FLAG_REASON|FLAG_NOT_INCOMMAND|FLAG_NOT_PUPPET|FLAG_ADMIN_PUPPET|FLAG_NOT_OWN, BOOLFLAG_APPRENTICE},
       {"Builder",             BUILDER,      BUILDER,      'B',  4|FLAG_LOG_CHARACTER|FLAG_REASON|FLAG_REASON_MORTAL|FLAG_NOT_INCOMMAND|FLAG_NOT_PUPPET|FLAG_ADMIN_PUPPET|FLAG_NOT_OWN|FLAG_APPRENTICE, BOOLFLAG_BUILDER},
       {"Dark",                INVISIBLE,    INVISIBLE,    'D',  8|FLAG_NOT_CHCOMMAND|FLAG_NOT_DISPLAY, 0},
       {"Deity",               DEITY,        DEITY,        'D',  0|FLAG_LOG|FLAG_REASON|FLAG_NOT_INCOMMAND|FLAG_NOT_PUPPET|FLAG_NOT_OWN, BOOLFLAG_DEITY},
       {"Elder",               ELDER,        ELDER,        'E',  1|FLAG_LOG|FLAG_REASON|FLAG_NOT_INCOMMAND|FLAG_NOT_PUPPET|FLAG_NOT_OWN, BOOLFLAG_ELDER},
       {"Haven",               HAVEN,        HAVEN,        'H',  8, BOOLFLAG_HAVEN},
       {"Listen",              LISTEN,       LISTEN,       'L',  8|FLAG_PERMISSION, BOOLFLAG_LISTEN},
       {"Moron",               MORON,        MORON,        'M',  4|FLAG_LOG_CHARACTER|FLAG_REASON|FLAG_NOT_INCOMMAND|FLAG_NOT_OWN|FLAG_APPRENTICE, BOOLFLAG_MORON},
       {"Open",                OPEN,         OPEN,         '\0', 8|FLAG_PERMISSION|FLAG_NOT_CHARACTER, 0},
       {"Openable",            OPENABLE,     OPENABLE,     'O',  8|FLAG_PERMISSION|FLAG_NOT_CHARACTER, 0},
       {"Permanent",           PERMANENT,    PERMANENT,    'P',  5|FLAG_LOG|FLAG_PERMISSION|FLAG_EXPERIENCED, BOOLFLAG_PERMANENT},
       {"Quiet",               QUIET,        QUIET,        'Q',  8, BOOLFLAG_QUIET},
       {"Sharable",            SHARABLE,     SHARABLE,     'S',  8|FLAG_NOT_CHARACTER, 0},
       {"Assistant",           ASSISTANT,    ASSISTANT,    'x',  3|FLAG_LOG_CHARACTER|FLAG_REASON|FLAG_NOT_INCOMMAND|FLAG_NOT_PUPPET|FLAG_ADMIN_PUPPET|FLAG_EXP_PUPPET|FLAG_NOT_OWN|FLAG_NOT_OBJECT, BOOLFLAG_ASSISTANT},
       {"Wizard",              WIZARD,       WIZARD,       'W',  2|FLAG_LOG|FLAG_REASON|FLAG_NOT_INCOMMAND|FLAG_NOT_PUPPET|FLAG_NOT_OWN, BOOLFLAG_WIZARD},
       {"Experienced",         EXPERIENCED,  EXPERIENCED,  'X',  3|FLAG_LOG_CHARACTER|FLAG_REASON|FLAG_NOT_INCOMMAND|FLAG_NOT_PUPPET|FLAG_ADMIN_PUPPET|FLAG_EXP_PUPPET|FLAG_NOT_OWN|FLAG_NOT_OBJECT, BOOLFLAG_EXPERIENCED},
       {"Ansi Colour",         ANSI,         ANSI,         'a',  8|FLAG_INTERNAL, BOOLFLAG_ANSI},
       {"Being",               BEING,        BEING,        'b',  3|FLAG_LOG_CHARACTER|FLAG_REASON|FLAG_NOT_INCOMMAND|FLAG_NOT_OWN|FLAG_NOT_OBJECT, BOOLFLAG_BEING},
       {"Combat",              COMBAT,       COMBAT,       'c',  8|FLAG_PERMISSION|FLAG_NOT_CHARACTER, 0},
       {"Engaged",             ENGAGED,      ENGAGED,      'e',  8|FLAG_INTERNAL|FLAG_NOT_OBJECT, BOOLFLAG_ENGAGED},
       {"Married",             MARRIED,      MARRIED,      'm',  8|FLAG_INTERNAL|FLAG_NOT_OBJECT, BOOLFLAG_MARRIED},
       {"Number",              NUMBER,       NUMBER,       'n',  8|FLAG_PERMISSION, BOOLFLAG_NUMBER},
       {"Private",             PRIVATE,      PRIVATE,      'p',  8, BOOLFLAG_PRIVATE},
       {"Opaque",              OPAQUE,       OPAQUE,       'q',  8|FLAG_PERMISSION|FLAG_NOT_CHARACTER, 0},
       {"Read-Only",           READONLY,     READONLY,     'r',  5|FLAG_PERMISSION|FLAG_EXPERIENCED, BOOLFLAG_READONLY},
       {"Tom",                 TOM,          TOM,          't',  8|FLAG_PERMISSION|FLAG_NOT_CHARACTER, BOOLFLAG_EXPERIENCED},
       {"Visible",             VISIBLE,      VISIBLE,      'v',  8, BOOLFLAG_VISIBLE},
       {"Yell",                YELL,         YELL,         'y',  8|FLAG_LOG_CHARACTER|FLAG_REASON|FLAG_REASON_MORTAL|FLAG_NOT_CHCOMMAND|FLAG_NOT_OWN|FLAG_OWN_PUPPET|FLAG_APPRENTICE, BOOLFLAG_YELL},
       {"Transferable",        TRANSFERABLE, TRANSFERABLE, '&',  8|FLAG_PERMISSION|FLAG_NOT_CHARACTER, 0},
       {"Ashcan",              ASHCAN,       ASHCAN,       '!',  8|FLAG_LOG|FLAG_NOT_CHCOMMAND|FLAG_NOT_OWN|FLAG_NOT_PUPPET|FLAG_PERMISSION, BOOLFLAG_ASHCAN},
       {"Sticky",              STICKY,       STICKY,       '+',  8|FLAG_NOT_CHARACTER|FLAG_PERMISSION, 0},
       {"Censored",            CENSOR,       CENSOR,       '^',  8|FLAG_LOG_CHARACTER|FLAG_NOT_CHCOMMAND|FLAG_NOT_OWN|FLAG_OWN_PUPPET|FLAG_APPRENTICE, BOOLFLAG_CENSOR},
       {"Tracing",             TRACING,      TRACING,      '~',  8|FLAG_PERMISSION, BOOLFLAG_TRACING},
       {"Ansi",                ANSI,         ANSI,         '\0', 8|FLAG_INTERNAL|FLAG_NOT_DISPLAY, BOOLFLAG_ANSI},
       {"AnsiColour",          ANSI,         ANSI,         '\0', 8|FLAG_INTERNAL|FLAG_NOT_DISPLAY, BOOLFLAG_ANSI},
       {"AnsiColor",           ANSI,         ANSI,         '\0', 8|FLAG_INTERNAL|FLAG_NOT_DISPLAY, BOOLFLAG_ANSI},
       {"Apprentice Wizard",   APPRENTICE,   APPRENTICE,   '\0', 3|FLAG_LOG|FLAG_REASON|FLAG_NOT_INCOMMAND|FLAG_NOT_PUPPET|FLAG_ADMIN_PUPPET|FLAG_NOT_OWN|FLAG_NOT_DISPLAY, BOOLFLAG_APPRENTICE},
       {"ApprenticeWizard",    APPRENTICE,   APPRENTICE,   '\0', 3|FLAG_LOG|FLAG_REASON|FLAG_NOT_INCOMMAND|FLAG_NOT_PUPPET|FLAG_ADMIN_PUPPET|FLAG_NOT_OWN|FLAG_NOT_DISPLAY, BOOLFLAG_APPRENTICE},
       {"BBS",                 0,            0,            '\0', 0|FLAG_SKIP|FLAG_NOT_DISPLAY},
       {"BBSInform",           BBS_INFORM,   BBS_INFORM,   '\0', 8|FLAG_NOT_OBJECT|FLAG_NOT_DISPLAY|FLAG_PERMISSION, BOOLFLAG_BBS_INFORM},
       {"BBS-Inform",          BBS_INFORM,   BBS_INFORM,   '\0', 8|FLAG_NOT_OBJECT|FLAG_PERMISSION, BOOLFLAG_BBS_INFORM},
       {"BBSNotify",           BBS_INFORM,   BBS_INFORM,   '\0', 8|FLAG_NOT_OBJECT|FLAG_NOT_DISPLAY|FLAG_PERMISSION, BOOLFLAG_BBS_INFORM},
       {"BBS-Notify",          BBS_INFORM,   BBS_INFORM,   '\0', 8|FLAG_NOT_OBJECT|FLAG_NOT_DISPLAY|FLAG_PERMISSION, BOOLFLAG_BBS_INFORM},
       {"Boot",                BOOT,         BOOT,         '\0', 3|FLAG_LOG_CHARACTER|FLAG_REASON|FLAG_NOT_INCOMMAND|FLAG_NOT_OWN|FLAG_NOT_PUPPET|FLAG_NOT_DISPLAY|FLAG_NOT_OBJECT, BOOLFLAG_BUILDER},
       {"Censorship",          CENSOR,       CENSOR,       '\0', 8|FLAG_LOG_CHARACTER|FLAG_NOT_CHCOMMAND|FLAG_NOT_OWN|FLAG_OWN_PUPPET|FLAG_APPRENTICE|FLAG_NOT_DISPLAY, BOOLFLAG_CENSOR},
       {"Chown_OK",            TRANSFERABLE, TRANSFERABLE, '\0', 8|FLAG_NOT_DISPLAY|FLAG_NOT_CHARACTER, 0},
       {"ChownOK",             TRANSFERABLE, TRANSFERABLE, '\0', 8|FLAG_NOT_DISPLAY|FLAG_NOT_CHARACTER, 0},
       {"Druid",               DRUID,        DRUID,        '\0', 3|FLAG_LOG_CHARACTER|FLAG_REASON|FLAG_NOT_INCOMMAND|FLAG_NOT_PUPPET|FLAG_ADMIN_PUPPET|FLAG_NOT_OWN|FLAG_NOT_OBJECT, BOOLFLAG_DRUID},
       {"Elder Wizard",        ELDER,        ELDER,        '\0', 1|FLAG_LOG|FLAG_REASON|FLAG_NOT_INCOMMAND|FLAG_NOT_PUPPET|FLAG_NOT_OWN|FLAG_NOT_DISPLAY, BOOLFLAG_ELDER},
       {"ElderWizard",         ELDER,        ELDER,        '\0', 1|FLAG_LOG|FLAG_REASON|FLAG_NOT_INCOMMAND|FLAG_NOT_PUPPET|FLAG_NOT_OWN|FLAG_NOT_DISPLAY, BOOLFLAG_ELDER},
       {"ExperiencedBuilder",  EXPERIENCED,  EXPERIENCED,  '\0', 3|FLAG_NOT_DISPLAY|FLAG_LOG_CHARACTER|FLAG_REASON|FLAG_NOT_INCOMMAND|FLAG_NOT_PUPPET|FLAG_ADMIN_PUPPET|FLAG_EXP_PUPPET|FLAG_NOT_OWN|FLAG_NOT_OBJECT, BOOLFLAG_EXPERIENCED},
       {"Experienced Builder", EXPERIENCED,  EXPERIENCED,  '\0', 3|FLAG_NOT_DISPLAY|FLAG_LOG_CHARACTER|FLAG_REASON|FLAG_NOT_INCOMMAND|FLAG_NOT_PUPPET|FLAG_ADMIN_PUPPET|FLAG_EXP_PUPPET|FLAG_NOT_OWN|FLAG_NOT_OBJECT, BOOLFLAG_EXPERIENCED},
       {"FriendsInform",       FRIENDS_INFORM, FRIENDS_INFORM, '\0', 8|FLAG_NOT_OBJECT|FLAG_NOT_DISPLAY, BOOLFLAG_FRIENDS_INFORM},
       {"Friends-Inform",      FRIENDS_INFORM, FRIENDS_INFORM, '\0', 8|FLAG_NOT_OBJECT|FLAG_PERMISSION, BOOLFLAG_FRIENDS_INFORM},
       {"FriendsNotify",       FRIENDS_INFORM, FRIENDS_INFORM, '\0', 8|FLAG_NOT_OBJECT|FLAG_NOT_DISPLAY, BOOLFLAG_FRIENDS_INFORM},
       {"Friends-Notify",      FRIENDS_INFORM, FRIENDS_INFORM, '\0', 8|FLAG_NOT_OBJECT|FLAG_NOT_DISPLAY, BOOLFLAG_FRIENDS_INFORM},
       {"God",                 DEITY,        DEITY,        '\0', 0|FLAG_LOG|FLAG_REASON|FLAG_NOT_INCOMMAND|FLAG_NOT_PUPPET|FLAG_NOT_OWN|FLAG_NOT_DISPLAY, BOOLFLAG_DEITY},
       {"Help",                HELP,         HELP,         '\0', 3|FLAG_LOG_CHARACTER|FLAG_REASON|FLAG_NOT_INCOMMAND|FLAG_NOT_OWN|FLAG_NOT_PUPPET|FLAG_NOT_DISPLAY|FLAG_NOT_OBJECT, BOOLFLAG_BUILDER},
       {"Invisible",           INVISIBLE,    INVISIBLE,    '\0', 8|FLAG_NOT_CHCOMMAND, 0},
       {"Locked",              LOCKED,       LOCKED,       '\0', 8|FLAG_PERMISSION|FLAG_NOT_CHARACTER, 0},
       {"Marry",               MARRIED,      MARRIED,      '\0', 8|FLAG_INTERNAL|FLAG_NOT_OBJECT|FLAG_NOT_DISPLAY, BOOLFLAG_MARRIED},
       {"Object",              OBJECT,       OBJECT,       '\0', 8|FLAG_NOT_DISPLAY, 0},
       {"ReadOnly",            READONLY,     READONLY,     '\0', 5|FLAG_NOT_DISPLAY|FLAG_EXPERIENCED, BOOLFLAG_READONLY},
       {"Shout",               SHOUT,        SHOUT,        '\0', 3|FLAG_LOG_CHARACTER|FLAG_REASON|FLAG_NOT_INCOMMAND|FLAG_NOT_OWN|FLAG_NOT_PUPPET|FLAG_NOT_DISPLAY|FLAG_NOT_OBJECT, BOOLFLAG_YELL},
       {"Trace",               TRACING,      TRACING,      '\0', 8|FLAG_PERMISSION|FLAG_NOT_DISPLAY, BOOLFLAG_TRACING},
       {"x",                   ASSISTANT,    ASSISTANT,    '\0', 3|FLAG_LOG_CHARACTER|FLAG_REASON|FLAG_NOT_INCOMMAND|FLAG_NOT_PUPPET|FLAG_ADMIN_PUPPET|FLAG_EXP_PUPPET|FLAG_NOT_OWN|FLAG_NOT_OBJECT|FLAG_NOT_DISPLAY, BOOLFLAG_ASSISTANT},
       {"Q",                   OPAQUE,       OPAQUE,       '\0', 8|FLAG_NOT_CHARACTER|FLAG_NOT_DISPLAY, 0},
       {"X",                   EXPERIENCED,  EXPERIENCED,  '\0', 3|FLAG_NOT_DISPLAY|FLAG_LOG_CHARACTER|FLAG_REASON|FLAG_NOT_INCOMMAND|FLAG_NOT_PUPPET|FLAG_ADMIN_PUPPET|FLAG_EXP_PUPPET|FLAG_NOT_OWN|FLAG_NOT_OBJECT, BOOLFLAG_EXPERIENCED},
       {"&",                   TRANSFERABLE, TRANSFERABLE, '\0', 8|FLAG_NOT_CHCOMMAND|FLAG_NOT_DISPLAY, 0},
       {"!",                   ASHCAN,       ASHCAN,       '\0', 3|FLAG_NOT_CHCOMMAND|FLAG_NOT_OWN|FLAG_NOT_DISPLAY, BOOLFLAG_ASHCAN},
       {"+",                   STICKY,       STICKY,       '\0', 8|FLAG_NOT_CHARACTER|FLAG_NOT_DISPLAY, 0},
       {"^",                   CENSOR,       CENSOR,       '\0', 8|FLAG_LOG_CHARACTER|FLAG_NOT_CHCOMMAND|FLAG_NOT_OWN|FLAG_OWN_PUPPET|FLAG_APPRENTICE|FLAG_NOT_DISPLAY, BOOLFLAG_CENSOR},
       {"~",                   TRACING,      TRACING,      '\0', 8|FLAG_NOT_DISPLAY, BOOLFLAG_TRACING},

        /* ---->  Gender flags  <---- */
       {"Male",                GENDER_MASK,   GENDER_MALE       << GENDER_SHIFT,'\0', 8|FLAG_NOT_INCOMMAND|FLAG_NOT_OBJECT, BOOLFLAG_MALE},
       {"Female",              GENDER_MASK,   GENDER_FEMALE     << GENDER_SHIFT,'\0', 8|FLAG_NOT_INCOMMAND|FLAG_NOT_OBJECT, BOOLFLAG_FEMALE},
       {"Neuter",              GENDER_MASK,   GENDER_NEUTER     << GENDER_SHIFT,'\0', 8|FLAG_NOT_INCOMMAND|FLAG_NOT_OBJECT, BOOLFLAG_NEUTER},
       {"Gender-Unset",        GENDER_MASK,   GENDER_UNASSIGNED << GENDER_SHIFT,'\0', 8|FLAG_NOT_INCOMMAND|FLAG_NOT_OBJECT, BOOLFLAG_UNSET},
       {"GenderUnset",         GENDER_MASK,   GENDER_UNASSIGNED << GENDER_SHIFT,'\0', 8|FLAG_NOT_INCOMMAND|FLAG_NOT_OBJECT|FLAG_NOT_DISPLAY, BOOLFLAG_UNSET},
       {"Unset",               GENDER_MASK,   GENDER_UNASSIGNED << GENDER_SHIFT,'\0', 8|FLAG_NOT_INCOMMAND|FLAG_NOT_OBJECT|FLAG_NOT_DISPLAY, BOOLFLAG_UNSET},
       {NULL,                  0,            0,            '\0', 8|FLAG_INTERNAL,0}
};
