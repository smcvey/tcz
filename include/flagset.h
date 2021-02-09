/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| FLAGSET.H  -  Table of which flags can be set on which types of objects.    |
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

#ifndef __FLAGSET_H
#define __FLAGSET_H


/* ---->  Object flags list data  <---- */
struct flag_data {
       const char *string;     /*  Name of flag  */
       int        mask;        /*  Flag's mask  */
       int        flag;        /*  Flag's value  */
       char       quick_flag;  /*  Flag's designated letter (As seen after object names in brackets)  */
       int        flags;       /*  Flag's flags(!) - Includes privilege level required to set flag (@set)  */
       int        boolflag;    /*  Flag's equivelant BOOLFLAG value (Used when locking objects to a flag)  */
};


/* ---->  Preference flags ('set')  <---- */
#define PREFS_PROMPT    0x00000001  /*  Interactively prompt for internal commands where arguments are ommited  */
#define PREFS_EDITOR    0x00000002  /*  Interactively start editor for internal commands where arguments are ommited  */
#define PREFS_DEFAULT   (PREFS_EDITOR|PREFS_PROMPT)


/* ---->  Flags for (re)setting flags ('@set')  <---- */
#define LEVEL_MASK           0x000000FF  /*  Last byte (Level character must be to set that flag)  */
#define FLAG_SKIP            0x00000100  /*  Skip to next flag list, if flag name matches  */
#define FLAG_EXPERIENCED     0x00004000  /*  Flag may be set/reset by Experienced Builder  */
#define FLAG_NOT_INCOMMAND   0x00008000  /*  Flag may not be set from within compound command                       */
#define FLAG_NOT_CHCOMMAND   0x00010000  /*  Flag may not be set on a character from within a compound command   */
#define FLAG_INTERNAL        0x00020000  /*  Flag is used internally and can't be set  */
#define FLAG_APPRENTICE      0x00040000  /*  Flag may be set on Mortals/Morons by an Apprentice Wizard/Druid  */
#define FLAG_NOT_PUPPET      0x00080000  /*  Flag may not be set on a puppet  */
#define FLAG_NOT_OWN         0x00100000  /*  Flag can't be set/reset on yourself (Someone else must do it for you)  */
#define FLAG_NOT_DISPLAY     0x00200000  /*  Flag name shouldn't be displayed after 'Flags:' when examining things  */
#define FLAG_OWN_PUPPET      0x00400000  /*  Flag can't be set/reset on one of your puppets  */
#define FLAG_PERMISSION      0x00800000  /*  Flag can't be seen unless Admin or can_write_to() object (Used by 'scan'/'finger')  */
#define FLAG_REASON          0x01000000  /*  Reason must be given to set/reset flag  */
#define FLAG_REASON_MORTAL   0x02000000  /*  No reason needs to be given when flag set/reset on Mortal   */
#define FLAG_LOG_OBJECT      0x04000000  /*  Log set/reset of flag on objects only (Not characters) to 'Admin' log file  */
#define FLAG_LOG_CHARACTER   0x08000000  /*  Log set/reset of flag on characters only to 'Admin' log file  */
#define FLAG_LOG             (FLAG_LOG_OBJECT|FLAG_LOG_CHARACTER)  /*  Log set/reset of flag to 'Admin' log file  */
#define FLAG_NOT_CHARACTER   0x10000000  /*  Flag may not be set on a character (Can only be set on objects)  */
#define FLAG_NOT_OBJECT      0x20000000  /*  Flag may not be set on an object (Can only be set on characters)  */
#define FLAG_ADMIN_PUPPET    0x40000000  /*  Flag may be set on Admin controlled puppet (An override for FLAG_NOT_PUPPET)  */
#define FLAG_EXP_PUPPET      0x80000000  /*  Flag may be set on Experienced Builder or Assistant controlled puppet (An override for FLAG_NOT_PUPPET)  */

#include "boolean_flags.h"
#include "primary_flags.h"
#include "secondary_flags.h"

extern int flag_map[];
extern int flag_map2[];

/* ---->  Flag macros  <---- */
#define Abode(x)           ((Typeof(x) != TYPE_CHARACTER) && ((db[(x)].flags & ABODE) != 0))
#define Abort(x)           ((Typeof(x) == TYPE_FUSE) && ((db[(x)].flags2 & ABORTFUSE) != 0))
#define Apprentice(x)      (((db[(x)].flags & (APPRENTICE|WIZARD|ELDER|DEITY)) != 0) && !Retired(x))
#define Assistant(x)       ((db[(x)].flags & ASSISTANT) != 0)
#define Ansi(x)            ((db[(x)].flags & ANSI) || (db[(x)].flags2 & ANSI8))
#define Ashcan(x)          ((db[(x)].flags & ASHCAN) != 0)
#define Bbs(x)             ((db[(x)].flags2 & BBS) != 0)
#define Bbsinform(x)       ((db[(x)].flags & BBS_INFORM) != 0)
#define Being(x)           ((db[(x)].flags & BEING) != 0)
#define Boot(x)            (((db[(x)].flags & BOOT) != 0) || Root(x))
#define Builder(x)         (((db[(x)].flags & (BUILDER|EXPERIENCED|APPRENTICE|WIZARD|ELDER|DEITY)) != 0) || Retired(x))
#define Censor(x)          ((db[(x)].flags & CENSOR) != 0)
#define Combat(x)          ((Typeof(x) != TYPE_CHARACTER) && ((db[(x)].flags & COMBAT) != 0))
#define Connected(x)       ((db[(x)].flags2 & CONNECTED) != 0)
#define Container(x)       (((db[(x)].flags & (OPENABLE|OPEN)) != 0) && (Typeof(x) == TYPE_THING))
#define Deity(x)           ((Root(x) || ((db[(x)].flags & DEITY) != 0)) && !Retired(x))
#define Druid(x)           ((Typeof(x) == TYPE_CHARACTER) && ((db[(x)].flags & DRUID) != 0) && (((db[(x)].flags & (APPRENTICE|WIZARD|ELDER|DEITY)) != 0) || !Retired(x)))
#define Echo(x)            ((db[(x)].flags2 & LOCAL_ECHO) != 0)
#define Elder(x)           (((db[(x)].flags & (ELDER|DEITY)) != 0) && !Retired(x))
#define ForwardEmail(x)    ((db[(x)].flags2 & FORWARD_EMAIL) != 0)
#define Engaged(x)         ((Typeof(x) == TYPE_CHARACTER) && ((db[(x)].flags & ENGAGED) != 0))
#define Executable(x)      ((db[(x)].flags2 & NON_EXECUTABLE) == 0)
#define Experienced(x)     ((db[(x)].flags & EXPERIENCED) || Retired(x))
#define Expiry(x)          ((db[(x)].flags2 & EXPIRY) != 0)
#define FriendsInform(x)   ((db[(x)].flags & FRIENDS_INFORM) != 0)
#define Haven(x)           ((db[(x)].flags & HAVEN) != 0)
#define Help(x)            (Experienced(x) && ((db[(x)].flags & HELP) != 0))
#define Immovable(x)       ((Typeof(x) != TYPE_CHARACTER) && ((db[(x)].flags2 & IMMOVABLE) != 0))
#define Inheritable(x)     ((db[(x)].flags2 & INHERITABLE) != 0)
#define Invisible(x)       ((db[(x)].flags & INVISIBLE) != 0)
#define Listen(x)          ((db[(x)].flags & LISTEN) != 0)
#define Locked(x)          ((Typeof(x) != TYPE_CHARACTER) && ((db[(x)].flags & LOCKED) != 0))
#define Mail(x)            ((db[(x)].flags2 & MAIL) != 0)
#define Married(x)         ((Typeof(x) == TYPE_CHARACTER) && ((db[(x)].flags & MARRIED) != 0))
#define More(x)            ((db[(x)].flags2 & MORE_PAGER) != 0)
#define Moron(x)           ((db[(x)].flags & MORON) != 0)
#define Number(x)          ((db[(x)].flags & NUMBER) != 0)
#define Object(x)          ((db[(x)].flags & OBJECT) != 0)
#define ObjectType(x)      names[Typeof(x)]
#define Opaque(x)          ((Typeof(x) != TYPE_CHARACTER) && ((db[(x)].flags & OPAQUE) != 0))
#define Open(x)            ((db[(x)].flags & OPEN) != 0)
#define Openable(x)        ((db[(x)].flags & OPENABLE) != 0)
#define Original(x)        ((db[(x)].flags2 & ORIGINAL) != 0)
#define Pagebell(x)        ((db[(x)].flags2 & PAGEBELL) != 0)
#define Permanent(x)       ((db[(x)].flags & PERMANENT) != 0)
#define Private(x)         ((Typeof(x) != TYPE_CHARACTER) && ((db[(x)].flags & PRIVATE) != 0))
#define Quiet(x)           ((db[(x)].flags & QUIET) != 0)
#define Readonly(x)        ((db[(x)].flags & READONLY) != 0)
#define Readonlydesc(x)    ((db[(x)].flags2 & READONLYDESC) != 0)
#define Retired(x)         ((Typeof(x) == TYPE_CHARACTER) && ((db[(x)].flags2 & RETIRED) != 0))
#define RetiredWizard(x)   (Retired(x) && ((db[(x)].flags & DRUID) == 0))
#define RetiredDruid(x)    (Retired(x) && ((db[(x)].flags & DRUID) != 0))
#define Secret(x)          ((db[(x)].flags2 & SECRET) != 0)
#define Sendhome(x)        ((Typeof(x) != TYPE_CHARACTER) && ((db[(x)].flags2 & SENDHOME) != 0))
#define Sharable(x)        ((db[(x)].flags & SHARABLE) != 0)
#define Shout(x)           (((db[(x)].flags & SHOUT) != 0) || Root(x))
#define Skipobjects(x)     ((db[(x)].flags2 & SKIPOBJECTS) != 0)
#define Sticky(x)          ((Typeof(x) != TYPE_CHARACTER) && ((db[(x)].flags & STICKY) != 0))
#define Tom(x)             ((db[(x)].flags & TOM) != 0)
#define Tracing(x)         ((db[(x)].flags & TRACING) != 0)
#define Transferable(x)    ((Typeof(x) != TYPE_CHARACTER) && ((db[(x)].flags & TRANSFERABLE) != 0))
#define Transport(x)       ((Typeof(x) != TYPE_CHARACTER) && ((db[(x)].flags2 & TRANSPORT) != 0))
#define Underline(x)       ((Typeof(x) == TYPE_CHARACTER) && ((db[(x)].flags2 & UNDERLINE) != 0))
#define Validated(x)       ((Typeof(x) != TYPE_CHARACTER) && ((db[(x)].flags2 & VALIDATED) != 0))
#define Vehicle(x)         (Transport(x) && (Typeof(x) == TYPE_THING))
#define Visible(x)         ((db[(x)].flags & VISIBLE) != 0)
#define Visit(x)           ((db[(x)].flags2 & VISIT) != 0)
#define Warp(x)            ((db[(x)].flags2 & WARP) != 0)
#define Wizard(x)          (((db[(x)].flags & (WIZARD|ELDER|DEITY)) != 0) && !Retired(x))
#define Yell(x)            ((db[(x)].flags & YELL) != 0)


/* ---->  Article macros  <---- */
#define Articleof(x)                    ((db[(x)].flags2 & ARTICLE_MASK) >> ARTICLE_SHIFT)
#define HasArticle(x)                   ((db[(x)].flags2 & ARTICLE_MASK) != 0)
#define Article(object,upper,definite)  ((Valid(object) && HasArticle(object)) ? article[(((definite) == DEFINITE) ? 8:0) + ((upper) ? 4:0) + ((db[(object)].flags2 & ARTICLE_MASK) >> ARTICLE_SHIFT)]:"")


/* ---->  Pronoun macros  <---- */
#define Genderof(x)               ((Typeof(x) == TYPE_CHARACTER) ? ((db[(x)].flags & GENDER_MASK) >> GENDER_SHIFT):GENDER_UNASSIGNED)
#define Possessive(player,upper)  possessive[(Validchar(player) ? Genderof(player):0) + ((upper) ? 4:0)]
#define Subjective(player,upper)  subjective[(Validchar(player) ? Genderof(player):0) + ((upper) ? 4:0)]
#define Objective(player,upper)   objective[(Validchar(player)  ? Genderof(player):0) + ((upper) ? 4:0)]
#define Reflexive(player,upper)   reflexive[(Validchar(player)  ? Genderof(player):0) + ((upper) ? 4:0)]
#define Absolute(player,upper)    absolute[(Validchar(player)   ? Genderof(player):0) + ((upper) ? 4:0)]

#endif  /* __ FLAGSET_H  */

