/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| DB.H  -  Global constants, data structures and declarations from DB.C       |
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
| Module originally modified for TCZ by:  J.P.Boggis 21/12/1993.              |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: db.h,v 1.2 2005/01/25 19:21:12 tcz_monster Exp $

*/


#ifndef __DB_H
#define __DB_H


#include <sys/types.h>
#include <stdio.h>

#include "structures.h"
#include "config.h"


/* ---->  'Safe' malloc() and free() #define's  <---- */
#define NMALLOC(result,type,number) do { \
                if(!((result) = (type *) malloc((number) * sizeof(type)))) \
                   server_emergency_dump("(NMALLOC)  Out of memory.",0); \
        } while(0)

#define MALLOC(result,type) do {\
               if(!((result) = (type *) malloc(sizeof(type)))) \
                  server_emergency_dump("(MALLOC)  Out of memory.",0); \
        } while(0)

#define FREENULL(x) if(x) {\
                 free((void *) x); \
                 x = NULL; \
        }


#define CHANNEL_CONTEXT struct descriptor_data *d,char *chname,char *params,int option
#define EDIT_CONTEXT    struct descriptor_data *d, int rfrom, int rto, int option, int numeric, int lines, unsigned char jump, char *text, char *params, int val1, char *editcmd, char *buffer
#define PASS_CONTEXT    player,params,arg0,arg1,arg2,val1,val2
#define UNSET_DATE      0xFFFFFFFF
#define OPTCONTEXT      dbref player,const char *title,const char *value,int status,int *error,int *critical
#define OPTSTATUS       NOTHING,NULL,NULL,1,NULL,NULL
#define INFINITY        0x7FFFFFFF
#define SITEMASK        0xFFFFFFFF
#define CONTEXT         dbref player, char *params, char *arg0, char *arg1, char *arg2, int val1, int val2
#define NOTINT          0x80000000


/* ---->  Time/date constants  <---- */
#define LEAPYEAR        31622400     /*  Seconds in a leap year  */
#define AVGYEAR         31557600     /*  Average seconds in a year  */
#define YEAR            31536000     /*  Seconds in a year    */
#define QUARTER         (YEAR / 4)   /*  Approx. seconds in a quarter (3 months)  */
#define MONTH           (YEAR / 12)  /*  Approx. seconds in a month  */
#define WEEK            604800       /*  Seconds in a week    */
#define DAY             86400        /*  Seconds in a day     */
#define HOUR            3600         /*  Seconds in an hour   */
#define MINUTE          60           /*  Seconds in a minute  */


/* ---->  Data constants  <---- */
#define GB              1073741824
#define MB              1048576
#define KB              1024


/* ---->  Interactive prompt SETARG settings  <---- */
#define PARAMS          0
#define ARG1            1
#define ARG2            2
#define ARG3            3


/* ---->  Help system flags  <---- */
#define HELP_MATCHED         0x01  /*  Topic already matched and displayed?  */
#define HELP_TUTORIAL        0x02  /*  Tutorial available for help topic?    */


/* ---->  Mail system flags  <---- */
#define MAIL_REPLY           0x01  /*  Mail in reply to another mail                          */
#define MAIL_URGENT          0x02  /*  Type '~' at beginning of subject to tag as urgent.     */
#define MAIL_IMPORTANT       0x04  /*  Type '!' at beginning of subject to tag as important.  */
#define MAIL_UNREAD          0x08  /*  Mail hasn't been read yet                              */
#define MAIL_FORWARD         0x10  /*  Mail forwarded from another character                  */
#define MAIL_KEEP            0x20  /*  Keep mail 2x as long as normal (Default:  If mail hasn't been read for 50 days, it's deleted (Providing it's been read of course.)  */
#define MAIL_MULTI           0x40  /*  Mail sent to multiple characters                       */
#define MAIL_MULTI_ROOT      0x80  /*  Item of mail is 'root' item of group mail              */


/* ---->  BBS topic flags  <---- */
#define TOPIC_MORTALADD      0x01  /*  Mortals may add messages  */
#define TOPIC_FORMAT         0x02  /*  Message text automatically formatted and punctuated  */
#define TOPIC_CENSOR         0x04  /*  Bad language censored in message text  */
#define TOPIC_HILIGHT        0x08  /*  Topic hilighted in list of topics  */
#define TOPIC_ANON           0x10  /*  Mortals may make messages anonymous  */
#define TOPIC_CYCLIC         0x20  /*  Cyclic message deletion in action (When topic is full, an old message will automatically be deleted when a new one is added)?  */
#define TOPIC_ADD            0x40  /*  Messages can be added to topic  */


/* ---->  BBS message flags  <---- */
#define MESSAGE_VOTING       0x001  /*  Users may vote for/against message   */
#define MESSAGE_REPLY        0x002  /*  Message is a reply to another message  */
#define MESSAGE_ANON         0x004  /*  Author of message would like to remain anonymous  */
#define MESSAGE_APPEND       0x008  /*  Message has been appended to         */
#define MESSAGE_MAJORITY     0x010  /*  Majority factor vote?                */
#define MESSAGE_MODIFY       0x020  /*  Message currently being modifed (Deletion not permitted)  */
#define MESSAGE_PRIVATE      0x040  /*  Private vote (I.e:  Secret ballot)?  */
#define MESSAGE_EVERYONE     0x080  /*  Message marked as being read by everyone (Reader limit was exceeded)  */
#define MESSAGE_RESTRICTED   0x100  /*  Voting on message is restricted to users who are of the same level or above the message owner  */ 
#define MESSAGE_ALLOWAPPEND  0x200  /*  Appending to message allowed?        */
#define MESSAGE_APPENDALL    0x400  /*  Allow anyone to append to message?   */


/* ---->  BBS message reader list flags  <---- */
#define READER_READ          0x01   /*  Message has been read by reader    */
#define READER_VOTE_MASK     0x06   /*  Mask for reader's vote on message  */
#define READER_VOTE_FOR      0x02   /*  Reader has voted for message       */
#define READER_VOTE_AGAINST  0x04   /*  Reader has voted against message   */
#define READER_VOTE_ABSTAIN  0x06   /*  Reader has abstained their vote on message  */
#define READER_IGNORE        0x80   /*  Message ignored by reader          */


/* ---->  Channel flags  <---- */
#define CHANNEL_PERMANENT       0x0001  /*  Permanent channel         */
#define CHANNEL_INVITE          0x0002  /*  Invite only channel       */
#define CHANNEL_PRIVATE         0x0004  /*  Private channel           */
#define CHANNEL_SECRET          0x0008  /*  Secret channel            */
#define CHANNEL_CENSOR          0x0010  /*  Censor bad language       */
#define CHANNEL_BROADCAST       0x0020  /*  Allow broadcast messages (Operators + Owner)  */
#define CHANNEL_BROADCAST_OWNER 0x0040  /*  Allow broadcast messages (Owner only)  */
#define CHANNEL_LOCKED          0x0080  /*  Channel settings locked (Operators + Owner)  */
#define CHANNEL_LOCKED_OWNER    0x0100  /*  Channel settings locked (Owner only)  */


/* ---->  Channel user flags  <---- */
#define CHANNEL_USER_JOINED   0x01  /*  User is joined to channel            */
#define CHANNEL_USER_ACTIVE   0x02  /*  User has channel turned on           */
#define CHANNEL_USER_INVITE   0x04  /*  User is permanent invite of channel  */
#define CHANNEL_USER_REQUEST  0x08  /*  User requesting invite to channel    */
#define CHANNEL_USER_BANNED   0x10  /*  User banned from channel             */
#define CHANNEL_USER_OPERATOR 0x20  /*  User is an operator of the channel   */
#define CHANNEL_USER_MOTD     0x40  /*  User should be shown the channel MOTD when they next use the channel  */
#define CHANNEL_USER_INVITED  0x80  /*  User invited onto channel (Guest)    */
#define CHANNEL_OUTPUT_FLAGS  (CHANNEL_USER_JOINED|CHANNEL_USER_ACTIVE)  /*  Channel user flags required to receive 'normal' output over channel  */
#define CHANNEL_PRIV_FLAGS    (CHANNEL_USER_INVITE|CHANNEL_USER_BANNED|CHANNEL_USER_OPERATOR)


/* ---->  Channel command restrictions  <---- */
#define CHANNEL_COMMAND_ALL      0   /*  Command is unrestricted  */
#define CHANNEL_COMMAND_INVITE   1   /*  Restrict command to invites, operators and owner  */
#define CHANNEL_COMMAND_OPERATOR 2   /*  Restrict command to operators and owner  */
#define CHANNEL_COMMAND_ADMIN    3   /*  Restrict command to TCZ Admin and owner  */
#define CHANNEL_COMMAND_OWNER    4   /*  Restrict command to owner  */


/* ---->  Option instance constants (option_match())  <---- */
#define OPTION_COMMANDLINE   0  /*  Options processed on command-line given to start TCZ  */
#define OPTION_CONFIGFILE    1  /*  Options processed from configuration file  */
#define OPTION_ADMINOPTIONS  2  /*  Option processed from '@admin options' while TCZ is running  */


/* ---->  User list flags (Allowed keywords)  <---- */
#define USERLIST_SELF            0x0001  /*  User ('me')  */
#define USERLIST_FRIEND          0x0002  /*  Friend ('friends')           */
#define USERLIST_ENEMY           0x0004  /*  Enemy ('enemies')            */
#define USERLIST_ASSISTANT       0x0008  /*  Assistant ('assistants')     */
#define USERLIST_EXPERIENCED     0x0010  /*  Experienced Builder ('experienced')  */
#define USERLIST_RETIRED         0x0020  /*  Retired Administrator ('retired')    */
#define USERLIST_ADMIN           0x0040  /*  Administrator ('admin')      */
#define USERLIST_ALL             0x0080  /*  All connected users ('all')  */
#define USERLIST_DISCONNECTED    0x0100  /*  Disconnected user(s)         */
#define USERLIST_OTHERS          0x0200  /*  For 'friends'/'enemies', also include users who have you in their friends list  */

#define USERLIST_INDIVIDUAL      0x0800  /*  Individual user (Refered to directly by name/#ID.)  */
#define USERLIST_GROUP           0x1000  /*  Friend group                 */ 
#define USERLIST_SHARED          0x2000  /*  Shared friend group          */
#define USERLIST_INTERNAL        0x4000  /*  Internal group ('friends', 'admin', etc.)  */
#define USERLIST_EXCLUDE         0x8000  /*  Excluded user ('!')          */

#define USERLIST_DEFAULT         (USERLIST_SELF|USERLIST_FRIEND|USERLIST_ENEMY|USERLIST_ASSISTANT|USERLIST_EXPERIENCED|USERLIST_RETIRED|USERLIST_ADMIN|USERLIST_GROUP)
#define USERLIST_PAGETELL        (USERLIST_DEFAULT|USERLIST_OTHERS)
#define USERLIST_FRIENDS         (USERLIST_DEFAULT|USERLIST_DISCONNECTED|USERLIST_ALL)
#define USERLIST_MAIL            (USERLIST_SELF|USERLIST_DISCONNECTED|USERLIST_ADMIN)
#define USERLIST_EXCLUDED        (USERLIST_FRIEND|USERLIST_ENEMY|USERLIST_ASSISTANT|USERLIST_EXPERIENCED|USERLIST_RETIRED|USERLIST_ADMIN|USERLIST_ALL|USERLIST_GROUP|USERLIST_OTHERS|USERLIST_EXCLUDE)
#define USERLIST_INTERNAL_MASK   (USERLIST_FRIEND|USERLIST_ENEMY|USERLIST_ASSISTANT|USERLIST_EXPERIENCED|USERLIST_RETIRED|USERLIST_ADMIN|USERLIST_ALL|USERLIST_GROUP|USERLIST_SHARED)

#define USERLIST_CUSTOM_INCLUDE  0x01   /*  Default custom flag (Reset to exclude user from list)  */
#define USERLIST_CUSTOM_ALL      0xFF   /*  Include/exclude all custom flags  */
#define USERLIST_CUSTOM_NONE     0x00   /*  Include/exclude no custom flags   */

#define USERLIST_NAMES           8      /*  Maximum number of individual user names to display in constructed list  */
#define USERLIST_GROUPS          4      /*  Maximum number of individual friend/enemy group names to display in constructed list  */


/* ---->  Internet sites flags  <---- */
#define SITE_ADMIN           0x01  /*  Admin. connections allowed from site?  */
#define SITE_BANNED          0x02  /*  Site banned?  */
#define SITE_CREATE          0x04  /*  Site can create new characters?  */
#define SITE_DNS             0x08  /*  Site host name looked up on nameserver?  */
#define SITE_GUESTS          0x10  /*  Guest characters may connect from site?  */
#define SITE_UNCONDITIONAL   0x20  /*  Unconditional ban/connection restriction (Done straight after accept())  */
#define SITE_NODNS           0x40  /*  Don't look up site domain name on nameserver  */
#define SITE_READONLY        0x80  /*  Site details Read-Only  */


/* ---->  Large substitution flags  <---- */
#define SUBST_HTML           0x01  /*  '%h' HTML tag evaluation        */
#define SUBST_HARDHTML       0x02  /*  Hard-coded HTML tag evaluation  */
#define SUBST_HTMLTAG        0x04  /*  Don't substitute text within HTML tag  */
#define SUBST_STRIP_TELNET   0x10  /*  Strip text between '%|'s        */
#define SUBST_STRIP_HTML     0x20  /*  Strip text between '%^'s        */


/* ---->  Text formatting flags  <---- */
#define TXT_BOLD             0x001  /*  Emboldened/bright text           */
#define TXT_BLINK            0x002  /*  Flashing text                    */
#define TXT_ITALIC           0x004  /*  Italic text                      */
#define TXT_BGBOLD           0x008  /*  Bright text background/inverse   */
#define TXT_NORMAL           0x010  /*  Normal (Plain) text              */
#define TXT_INVERSE          0x020  /*  Inverse text/text with background colour  */
#define TXT_UNDERLINE        0x040  /*  Underlined text                  */
#define TXT_FG_CHANGE        0x100  /*  Text foreground colour changed?  */
#define TXT_BG_CHANGE        0x200  /*  Text background colour changed?  */
#define TXT_FMT_MASK         0xFFF  /*  Mask for text formatting         */


/* ---->  Text colour  <---- */
#define TXT_BLACK            0x0      /*  Black text                   */
#define TXT_RED              0x1      /*  Red text                     */
#define TXT_GREEN            0x2      /*  Green text                   */
#define TXT_YELLOW           0x3      /*  Yellow text                  */
#define TXT_BLUE             0x4      /*  Blue text                    */
#define TXT_MAGENTA          0x5      /*  Magenta text                 */
#define TXT_CYAN             0x6      /*  Cyan text                    */
#define TXT_WHITE            0x7      /*  White text                   */
#define TXT_FG_MASK          0x0F000  /*  Mask for foreground colour   */
#define TXT_BG_MASK          0xF0000  /*  Mask for background colour   */
#define TXT_FG_SHIFT         12       /*  Shift for foreground colour  */
#define TXT_BG_SHIFT         16       /*  Shift for background colour  */
#define TXT_ANSI_MASK        (TXT_BOLD|TXT_BLINK|TXT_ITALIC|TXT_BGBOLD|TXT_INVERSE|TXT_UNDERLINE|TXT_FG_MASK|TXT_BG_MASK)


/* ---->  Text size  <---- */
#define TXT_SIZE_SMALL       0x0       /*  Small text           */
#define TXT_SIZE_NORMAL      0x1       /*  Normal text          */
#define TXT_SIZE_MEDIUM      0x2       /*  Medium text          */
#define TXT_SIZE_LARGE       0x3       /*  Large text           */
#define TXT_SIZE_VLARGE      0x4       /*  Very large text      */
#define TXT_SIZE_MASK        0xF00000  /*  Mask for text size   */
#define TXT_SIZE_SHIFT       20        /*  Shift for text size  */


/* ---->  Database dumping constants  <---- */
#define DUMP_NORMAL          0  /*  Normal dump (Done all in one go, e.g:  When TCZ is shutdown))  */
#define DUMP_SANITISE        1  /*  Dump with sanity check on loading (Done every 'database dumping interval' in '@admin')  */
#define DUMP_PANIC           2  /*  Panic dump  (Done when TCZ crashes :(                          */
#define DUMP_EMERGENCY       3  /*  Emergency database dump ('@dump emergency = yes')  */


/* ---->  Used with edit_initialise() (Enables absolute censoring of last command while editing mail (For sending))  <---- */
#define EDIT_LAST_CENSOR     0x80


/* ---->  Preference flags macros  <---- */
#define PrefFlags(x)       ((Typeof(x) == TYPE_CHARACTER) ? (db[(x)].data->player.prefflags):0)
#define PrefsEditor(x)     (PrefFlags(x) & PREFS_EDITOR)
#define PrefsPrompt(x)     (PrefFlags(x) & PREFS_PROMPT)


/* ---->  Security levels  <---- */
#define Level1(x)       ((Root(x) || ((db[(x)].flags & DEITY) != 0)) && !Retired(x))
#define Level2(x)       ((Root(x) || ((db[(x)].flags & (ELDER|DEITY)) != 0)) && !Retired(x))
#define Level3(x)       ((Root(x) || ((db[(x)].flags & (WIZARD|ELDER|DEITY)) != 0)) && !Retired(x))
#define Level4(x)       ((Root(x) || ((db[(x)].flags & (APPRENTICE|WIZARD|ELDER|DEITY)) != 0)) && !Retired(x))


/* ---->  Miscellaneous macros  <---- */
#define gettime(var)       (var = server_gettime(NULL,1))
#define Controller(x)      ((Married(x) || Engaged(x)) ? (x):db[(x)].data->player.controller)
#define Partner(x)         ((Married(x) || Engaged(x)) ? db[(x)].data->player.controller:NOTHING)
#define Plural(x)          (((x) == 1) ? "":"s")
#define Puppet(x)          (Controller(x) != (x))
#define String(s)          (((s) != NULL) ? (s):"")
#define StringDefault(s,d) (((s) != NULL) ? (s):(d))
#define Blank(x)           (!((x) && *(x)))
#define Uid(x)             ((x != NOTHING) ? db[(x)].data->player.uid:NOTHING)


/* ---->  Special DBref's  <---- */
#define NOTHING         (-1)        /*  Null DBref  */
#define INHERIT         (-1)        /*  Inherit value from parent object  */
#define NOBODY          0x80000000  /*  No current character  -  Used for user connection with no current character  */
#define HOME            (-2)        /*  Virtual room:  Represents mover's home  */


/* ---->  ANSI default colour (White on black)  <---- */
#define ANSI_NORMAL     "\x1B[0;37m\x1B[40m"


/* ---->  ANSI dark colours  <---- */
#define ANSI_DBLACK     "\x1B[0;30m"
#define ANSI_DRED       "\x1B[0;31m"
#define ANSI_DGREEN     "\x1B[0;32m"
#define ANSI_DYELLOW    "\x1B[0;33m"
#define ANSI_DBLUE      "\x1B[0;34m"
#define ANSI_DMAGENTA   "\x1B[0;35m"
#define ANSI_DCYAN      "\x1B[0;36m"
#define ANSI_DWHITE     "\x1B[0;37m"


/* ---->  ANSI light colours  <---- */
#define ANSI_LBLACK     "\x1B[0;30m\x1B[1m"
#define ANSI_LRED       "\x1B[0;31m\x1B[1m"
#define ANSI_LGREEN     "\x1B[0;32m\x1B[1m"
#define ANSI_LYELLOW    "\x1B[0;33m\x1B[1m"
#define ANSI_LBLUE      "\x1B[0;34m\x1B[1m"
#define ANSI_LMAGENTA   "\x1B[0;35m\x1B[1m"
#define ANSI_LCYAN      "\x1B[0;36m\x1B[1m"
#define ANSI_LWHITE     "\x1B[0;37m\x1B[1m"


/* ---->  ANSI inverse colours (Background)  <---- */
#define ANSI_IBLACK     "\x1B[40m"
#define ANSI_IRED       "\x1B[41m"
#define ANSI_IGREEN     "\x1B[42m"
#define ANSI_IYELLOW    "\x1B[43m"
#define ANSI_IBLUE      "\x1B[44m"
#define ANSI_IMAGENTA   "\x1B[45m"
#define ANSI_ICYAN      "\x1B[46m"
#define ANSI_IWHITE     "\x1B[47m"


/* ---->  ANSI warning message colours (Colour + blinking)  <---- */
#define ANSI_WRED       "\x1B[0;31m\x1B[1m\x1B[5m"
#define ANSI_WGREEN     "\x1B[0;32m\x1B[1m\x1B[5m"
#define ANSI_WYELLOW    "\x1B[0;33m\x1B[1m\x1B[5m"
#define ANSI_WBLUE      "\x1B[0;34m\x1B[1m\x1B[5m"
#define ANSI_WMAGENTA   "\x1B[0;35m\x1B[1m\x1B[5m"
#define ANSI_WCYAN      "\x1B[0;36m\x1B[1m\x1B[5m"
#define ANSI_WWHITE     "\x1B[0;37m\x1B[1m\x1B[5m"


/* ---->  ANSI formatting  <---- */
#define ANSI_DARK       "\x1B[0m"
#define ANSI_LIGHT      "\x1B[1m"
#define ANSI_BLINK      "\x1B[5m"
#define ANSI_UNDERLINE  "\x1B[4m"


/* ---->  HTML simulations of light ANSI colours  <---- */
#define HTML_LBLACK    "#525252"
#define HTML_LRED      "#FF0000"
#define HTML_LGREEN    "#00FF00"
#define HTML_LYELLOW   "#FFFF00"
#define HTML_LBLUE     "#0000FF"
#define HTML_LMAGENTA  "#FF00FF"
#define HTML_LCYAN     "#00FFFF"
#define HTML_LWHITE    "#FFFFFF"


/* ---->  HTML simulations of dark ANSI colours  <---- */
#define HTML_DBLACK    "#000000"
#define HTML_DRED      "#AD0000"
#define HTML_DGREEN    "#00AD00"
#define HTML_DYELLOW   "#ADAD00"
#define HTML_DBLUE     "#0000AD"
#define HTML_DMAGENTA  "#AD00AD"
#define HTML_DCYAN     "#00ADAD"
#define HTML_DWHITE    "#ADADAD"


/* ---->  HTML table background colours  <---- */
#define HTML_TABLE_BLACK    "#000000"
#define HTML_TABLE_DGREY    "#222222"
#define HTML_TABLE_GREY     "#333333"
#define HTML_TABLE_MGREY    "#444444"
#define HTML_TABLE_LGREY    "#555555"
#define HTML_TABLE_RED      "#440000"
#define HTML_TABLE_GREEN    "#004400"
#define HTML_TABLE_YELLOW   "#443300"
#define HTML_TABLE_BLUE     "#000033"
#define HTML_TABLE_MBLUE    "#111144"
#define HTML_TABLE_MAGENTA  "#440044"
#define HTML_TABLE_CYAN     "#004444"
#define HTML_TABLE_WHITE    "#FFFFFF"


/* ---->  Restrictions on strings  <---- */
#define TEXT_SIZE       (KB * 4)
#define BUFFER_LEN      ((TEXT_SIZE * 2) + KB)
#define MAX_LENGTH      (TEXT_SIZE  - 1)
#define MAX_BUFFER_LEN  (BUFFER_LEN - (KB + 1))


/* ---->  Basic object field macros, with checking for -1 (NOTHING)  <---- */
#define Destination(x) ((x != NOTHING) ? db[(x)].destination:NOTHING)
#define Location(x)    ((x != NOTHING) ? db[(x)].location:NOTHING)
#define Parent(x)      ((x != NOTHING) ? db[(x)].parent:NOTHING)
#define Owner(x)       ((x != NOTHING) ? db[(x)].owner:NOTHING)
#define Next(x)        ((x != NOTHING) ? db[(x)].next:NOTHING)


/* ---->  Standard locations macros  <---- */
#define RoomZero(x)    ((x) == ROOMZERO)
#define Global(x)      ((x) == GLOBAL_COMMANDS)
#define Start(x)       ((x) == START_LOCATION)
#define Root(x)        ((x) == ROOT)


/* ---->  Miscellaneous macros  <---- */
#define TimeDiff(time,character)  (Validchar(character) ? ((time) + (db[(player)].data->player.timediff * HOUR)):(time))


/* ---->  External declarations  <---- */
extern struct   destroy_data    *destroy_queue_tail;
extern short                    destroy_queue_size;
extern unsigned long            memory_restriction;
extern struct   destroy_data    *destroy_queue;
extern char                     *possessive[];
extern char                     *subjective[];
extern struct   event_data      *event_queue;
extern char                     *objective[];
extern char                     *reflexive[];
extern char                     *absolute[];
extern char                     *article[];
extern char                     *clevels[];
extern unsigned long            checksum;
extern char                     *names[];
extern dbref                    db_top;
extern struct   object          *db;

extern char                     *monthabbr[];
extern char                     *sexuality[];
extern unsigned char            leapmdays[];
extern char                     *statuses[];
extern char                     *genders[];
extern char                     *dayabbr[];
extern char                     *month[];
extern unsigned char            mdays[];
extern char                     *day[];

extern dbref                    db_free_chain;
extern dbref                    db_free_chain_end;

extern unsigned long            mailcounter;
extern unsigned long            upper_bytes_out;
extern unsigned long            lower_bytes_out;
extern unsigned long            upper_bytes_in;
extern unsigned long            lower_bytes_in;

extern dbref                    parent_object;
extern dbref                    redirect_dest;
extern dbref                    redirect_src;
extern int                      inherited;

extern unsigned char            dumpsanitise;
extern unsigned char            idle_state;
extern unsigned char            log_stderr;
extern long                     dumptiming;
extern unsigned char            dumpstatus;
extern char                     *dumpfile;
extern char                     *lockfile;
extern char                     *dumpfull;
extern long                     nextcycle;
extern unsigned long            bytesread;
extern int                      dumpchild;
extern unsigned char            dumptype;
extern unsigned long            htmlport;
extern unsigned char            lib_dir;

extern const    char            *email_forward_name;
extern unsigned long            tcz_server_network;
extern unsigned long            tcz_server_netmask;
extern const    char            *operatingsystem;
extern const    char            *tcz_server_name;
extern const    char            *tcz_admin_email;
extern const    char            *tcz_short_name;
extern const    char            *html_home_url;
extern const    char            *html_data_url;
extern const    char            *tcz_full_name;
extern const    char            *tcz_location;
extern const    char            *tcz_timezone;
extern unsigned long            tcz_server_ip;
extern const    char            *tcz_prompt;
extern unsigned long            telnetport;
extern unsigned long            htmlport;
extern int                      tcz_year;

extern int                      db_accumulated_restarts;
extern int                      db_accumulated_uptime;
extern int                      db_longest_uptime;
extern time_t                   db_creation_date;
extern time_t                   db_longest_date;

extern struct   site_data       *sitelist;
extern long                     stat_days;
extern int                      stat_ptr;
extern struct   request_data    *request;
extern struct   banish_data     *banish;
extern struct   stat_data       stats[];
extern struct   bbs_topic_data  *bbs;


/* ---->  {J.P.Boggis 07/08/2001}  Restart/dump/log serial numbers  <---- */
extern unsigned long            restart_serial_no;
extern unsigned long            dump_serial_no;
extern unsigned long            log_serial_no;
extern time_t                   dump_timedate;

extern int                      fields[];
extern int                      settablefields[];
extern int                      lists[];
extern int                      inlist[];
extern int                      mortaltelprivs[];
extern int                      admintelprivs[];
extern int                      objectquota[];
extern int                      maxquotalimit[];
extern char                     objname[];
extern char                     cmpbuf[];
extern char                     *cmpptr;
extern struct grp_data          grproot;
extern struct grp_data          *grp;

#define PUSH(thing,locative)    ((db[(thing)].next = (locative)), (locative) = (thing))

extern unsigned long            getsize               (dbref object,unsigned char decompr);
extern dbref                    getfirst              (dbref object,int list,dbref *cobj);

#define getnext(object,list,cobj) if(Valid(object)) { \
                                     if(Valid(db[(object)].next)) (object) = db[(object)].next; \
                                        else if(Valid(cobj)) (object) = getfirst(db[cobj].parent,(list),&(cobj)); \
                                           else (object) = NOTHING; \
                                  }


/* ---->  Function Prototypes <---- */
extern char    			*db_read_field	      (FILE *f);
extern dbref                    remove_first          (dbref first,dbref what);
extern dbref                    get_areaname_loc      (dbref loc);
extern int                      in_area               (dbref thing,dbref area);
extern int                      contains              (dbref thing,dbref container);
extern int                      member                (dbref thing,dbref list);
extern void                     abortmemory           (const char *reason);
extern unsigned char            hasprofile            (struct profile_data *profile);
extern unsigned char            *accesslevel_name     (char accesslevel);
extern const    char            *object_type          (dbref object,int progressive);
extern const    char            *list_type            (int list);
extern int                      can_teleport_object   (dbref player,dbref object,dbref destination);
extern void                     filter_comexitname    (dbref object);
extern void                     capitalise            (char *str);
extern struct   boolexp         *getlock              (dbref object,int key);
extern long                     getvalue              (dbref object,void *value,unsigned short size,long def);
extern const    char            *getfield             (dbref object,int field);
extern long                     get_mass_or_volume    (dbref object,unsigned char volume);
extern void                     setfield              (dbref object,int field,const char *str,unsigned char capitals);
extern void                     initialise_profile    (struct profile_data *profile);
extern void                     initialise_data       (dbref object);
extern void                     initialise_quotas     (unsigned char restart);

extern const    char            *alloc_string         (const char *);
extern const    char            *malloc_string        (const char *string);
extern void 			initialise_object     (struct object *obj);
extern dbref                    new_object            ();
extern void                     delete_object         (dbref object,unsigned char update,unsigned char queue);
extern void                     db_create             (void);
extern void                     db_write              (struct descriptor_data *p);
extern unsigned char            db_read               (FILE *);
extern void                     db_free               ();
extern dbref                    parse_dbref           (const char *);

extern struct boolexp           *getboolexp           (FILE *f);
extern void                     putboolexp            (FILE *f,struct boolexp *b);
extern void                     free_boolexp          (struct boolexp **ptr);


/* ---->  Administrative Options (Some of these may be phased out in the future because the same effect can be achieved in sites.tcz)  <---- */
extern long                     dumpinterval;
extern long                     timeadjust;
extern unsigned char            dumpcycle;
extern int                      dumpdatasize;
extern unsigned char            log_top_commands;
extern unsigned char            limit_connections;
extern short                    allowed;
extern unsigned char            creations;
extern unsigned char            connections;
extern dbref                    bankroom;
extern dbref                    homerooms;
extern dbref                    mailroom;
extern dbref                    bbsroom;
extern dbref                    aliases;
extern unsigned char            maintenance;
extern dbref                    maint_owner;
extern unsigned short           maint_morons;
extern unsigned short           maint_newbies;
extern unsigned short           maint_mortals;
extern unsigned short           maint_builders;
extern unsigned short           maint_objects;
extern unsigned short           maint_junk;
extern time_t                   quarter;  /*  End of financial quarter  */
extern time_t                   logins;   /*  Date counting of logins was implemented  */

extern long                     toplevel_commands;
extern long                     standard_commands;
extern long                     compound_commands;
extern long                     editor_commands;
extern short                    max_descriptors;
extern int                      compoundonly;
extern int                      security;
extern time_t                   activity;
extern long                     uptime;

#endif
