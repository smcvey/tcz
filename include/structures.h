/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| STRUCTURES.H  -  Data structures used throughout TCZ.                       |
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
| Module originally designed and written by:  J.P.Boggis 30/07/2000.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                        https://github.com/smcvey/tcz                        |
`-----------------------------------------------------------------------------'

*/


#ifndef _STRUCTURES_H
#define _STRUCTURES_H


#include <stdlib.h>

#include "config.h"


/* ---->  Type definitions  <---- */
typedef char boolexp_type;  /*  Type of boolean expression (boolexp.c)  */
typedef int  dbref;         /*  Database object reference number (#ID)  */


/* ---->  Aliases binary tree data structure ('@alias')  <---- */
struct alias_data {
       struct   alias_data *next;     /*  Pointer to next alias in linked list  */
       struct   alias_data *prev;     /*  Pointer to previous alias in linked list  */
       struct   alias_data *right;    /*  Pointer to next alphabetically greater alias in binary tree  */
       struct   alias_data *left;     /*  Pointer to next alphabetically smaller alias in binary tree  */
       struct   alias_data *last;     /*  Pointer to previous node in binary tree  */
       char                *command;  /*  Command substituted  */
       char                *alias;    /*  Alias name(s)  */
       unsigned short      id;        /*  Alias ID  */
};


/* ---->  Parameters data structure (unparse_parameters(), used by query commands)  <---- */
struct arg_data {
       char          *text[PARAMETERS];  /*  Text parameters  */
       int           numb[PARAMETERS];   /*  Numeric parameters  */
       short         len[PARAMETERS];    /*  Length of text parameters  */
       unsigned char count;              /*  Number of parameters  */
};


/* ---->  Array element data structure  <---- */
struct array_element
{
       struct array_element *next;   /*  Next array element  */
       char                 *index;  /*  Index name  */
       char                 *text;   /*  Text of element  */
};


/* ---->  Dynamic array binary tree sort data structure ('@array')  <---- */
struct array_sort_data {
       struct  array_element   *element;
       struct  array_sort_data *centre;
       struct  array_sort_data *right;
       struct  array_sort_data *left;
       const   char            *sortptr;
       int                     offset;
       int                     len;
};


/* ---->  Author details data structure  <---- */
struct author_details {
       struct   author_details *next;      /*  Next author in alphabetical order  */
       const    char           *datefrom;  /*  Date from when author was actively working on module(s)  */
       const    char           *dateto;    /*  Date to when author was actively working on module(s)  */
       unsigned char           author;     /*  Author of current module ('module NAME')  */
       unsigned char           original;   /*  Original author of current module ('module NAME')  */
       const    char           *initials;  /*  Initials of author                 */
       const    char           *name;      /*  Name of author                     */
       const    char           *nickname;  /*  Nickname of author                 */
       const    char           *email;     /*  E-mail address of author           */
};


/* ---->  Banished names data structure  <---- */
struct banish_data {
       struct banish_data *sort;   /*  Pointer to next name in sorted list  */
       struct banish_data *next;   /*  Next name in linked list             */
       struct banish_data *right;  /*  Binary tree right pointer (>)        */
       struct banish_data *left;   /*  Binary tree left pointer  (<)        */
       char               *name;   /*  Banished name                        */
};


/* ---->  Data structure used by BBS 'summary' command to construct continuous list of topics/sub-topics  <---- */
struct bbs_list_data {
       struct bbs_list_data  *next;      /*  Next entry in list  */
       struct bbs_topic_data *topic;     /*  BBS topic  */
       struct bbs_topic_data *subtopic;  /*  BBS sub-topic  */
};


/* ---->  BBS message data structure  <---- */
struct bbs_message_data {
       struct   bbs_message_data *next;        /*  Next message in linked list  */
       struct   bbs_reader_data  *readers;     /*  Message's reader/vote list  */
       short                     readercount;  /*  Number of users (Excluding owner) who have read message  */
       char                      *message;     /*  Message  */
       char                      *subject;     /*  Message's subject  */
       long                      lastread;     /*  Date message was last read  */
       unsigned char             expiry;       /*  Expiry of vote on message (In days)  */
       char                      *name;        /*  Full name of owner  */
       unsigned short            flags;        /*  Message's flags  */
       dbref                     owner;        /*  Message's owner  */
       long                      date;         /*  Message's time & date of creation  */
       unsigned short            id;           /*  ID number of message in topic  */
};


/* ---->  BBS message reader list data structure  <---- */
struct bbs_reader_data {
       struct   bbs_reader_data *next;   /*  Next reader in linked list  */
       dbref                    reader;  /*  DBref of message's reader  */
       unsigned char            flags;   /*  Reader flags (Message read/reader's vote)  */
};


/* ---->  BBS topic data structure  <---- */
struct bbs_topic_data {
       struct   bbs_topic_data   *next;          /*  Next topic in linked list  */
       struct   bbs_message_data *messages;      /*  Linked list of topic's messages  */
       struct   bbs_topic_data   *subtopics;     /*  Linked list of topic's sub-topics  */
       unsigned short            subtopiclimit;  /*  Maximum number of sub-topics allowed in topic  */
       short                     messagelimit;   /*  Maximum number of messages allowed in topic  */
       char                      accesslevel;    /*  Access level required to access topic (-1 = Builder)  */
       unsigned char             timelimit;      /*  Time limit (In days) for messages  */
       unsigned short            topic_id;       /*  Topic's unique ID number  */
       char                      *desc;          /*  Topic's short description  */
       char                      *name;          /*  Topic's name  */
       dbref                     owner;          /*  Topic's owner (Character who created it using 'addtopic')  */
       unsigned char             flags;          /*  Topic's flags  */
};


/* ---->  Boolean expression data structure  <---- */
struct boolexp {
       struct boolexp *sub1;
       struct boolexp *sub2;
       dbref          object;
       boolexp_type   type;
};


/* ---->  Calculation operation data structure (Used by '@calc'/'@eval')  <---- */
struct calc_ops {
       char   *calcstring;
       int    comparison;
       double calcfloat;
       int    calctype;
       long   calcint;
};


/* ---->  Channel data structure  <---- */
struct channel_data {
       struct   channel_data      *sort;              /*  Next channel in sorted list  */
       struct   channel_data      *next;              /*  Next channel in linked list  */
       struct   channel_data      *right;             /*  Right pointer of binary tree  */
       struct   channel_data      *left;              /*  Left pointer of binary tree  */
       unsigned char              inviterestriction;  /*  Restriction on 'invite' channel command  */
       unsigned char              sendrestriction;    /*  Restriction on sending messages over channel  */
       unsigned char              motdrestriction;    /*  Restriction on setting message of the day (MOTD)  */
       unsigned char              accesslevel;        /*  Access level of channel  */
       struct   channel_data      *permanent;         /*  Next permanent channel in linked list  */
       struct   channel_user_data *userlist;          /*  User list of channel  */
       char                       *banner;            /*  Banner of channel  */
       char                       *title;             /*  Title of channel  */
       unsigned char              indent;             /*  Hanging indent setting for channel banner  */
       dbref                      owner;              /*  Owner of channel  */
       char                       *motd;              /*  MOTD (Message Of The Day)  */
       char                       *name;              /*  Name of channel  */
       unsigned short             flags;              /*  Flags of channel  */
};


/* ---->  Channel user data structure (I.e:  User joined to channel)  <---- */
struct channel_user_data
{
       struct   channel_user_data *next;   /*  Next user in list  */
       dbref                      player;  /*  DBref of character  */
       unsigned char              flags;   /*  Channel user flags  */
};


/* ---->  Command table data structure (Used for fast lookup of built-in commands)  <---- */
struct cmd_table {
       short            flags;  /*  Flags of command table entry  */
       char             *name;  /*  Name of command  */
       void     (*func) (dbref,char *,char *,char *,char *,int,int);  /*  character, params, arg0, arg1, arg2, val1, val2  */
       int              val1;   /*  1st parameter value  */
       int              val2;   /*  2nd parameter value  */
       unsigned char    len;    /*  Length of command name  */
};


/* ---->  Interactive command-line prompt for command parameters data structure  <---- */
struct cmdprompt_data {
       char       *prompt;      /*  User prompt (E.g:  'Enter description: ')  */
       const char *defaultval;  /*  Default value, set if user enters nothing at prompt  */
       const char *blankmsg;    /*  Optional error message, displayed if user enters nothing at prompt  */
       const char *command;     /*  Internal command to execute (E.g:  '@describe')  */
       int        setarg;       /*  Argument number to set with value entered at user prompt (See below)  */
       const char *params;      /*  (0)  Complete command arguments  */
       const char *arg1;        /*  (1)  First command argument  (ARG1 = arg2 [= arg3])  */
       const char *arg2;        /*  (2)  Second command argument (arg1 = ARG2 [= arg3])  */
       const char *arg3;        /*  (3)  Third command argument  (arg1 = arg2 [= ARG3])  */
};


/* ---->  Currency value to 2dp data structure (Gets around rounding error problem when using float/double)  */
struct currency_data {
       int           decimal;   /*  Decimal value  */
       unsigned char fraction;  /*  Fraction to 2dp  */
};


/* ---->  Additional fields for an alarm data structure  <---- */
struct db_alarm_data {
       char *desc;  /*  Alarm's description  */
};


/* ---->  Additional fields for a dynamic array data structure  <---- */
struct db_array_data
{
       struct array_element *start;  /*  Pointer to first element  */
};


/* ---->  Additional fields for a compound command data structure  <---- */
struct db_command_data {
       char           *desc;   /*  Compound command's description  */
       char           *drop;   /*  Compound command's drop message  */
       char           *succ;   /*  Compound command's success message  */
       char           *fail;   /*  Compound command's failure message  */
       char           *odrop;  /*  Compound command's others drop message  */
       char           *osucc;  /*  Compound command's others success message  */
       char           *ofail;  /*  Compound command's others failure message  */
       struct boolexp *lock;   /*  Compound command's lock  */
};


/* ---->  Additional fields for an exit data structure  <---- */
struct db_exit_data {
       char           *desc;   /*  Exit's description  */
       char           *drop;   /*  Exit's drop message  */
       char           *succ;   /*  Exit's success message  */
       char           *fail;   /*  Exit's failure message  */
       char           *odrop;  /*  Exit's others drop message  */
       char           *osucc;  /*  Exit's others success message  */
       char           *ofail;  /*  Exit's others failure message  */
       struct boolexp *lock;   /*  Exit's lock  */
};


/* ---->  Additional fields for a fuse data structure  <---- */
struct db_fuse_data {
       char           *desc;  /*  Fuse's description  */
       char           *drop;  /*  Fuse's drop message  */
       struct boolexp *lock;  /*  Fuse's lock  */
};


/* ---->  Additional fields for a character data structure  <---- */
struct db_player_data {
       char                   *desc;             /*  Character's description  */
       char                   *drop;             /*  Character's E-mail address  */
       char                   *succ;             /*  Character's name prefix  */
       char                   *fail;             /*  Character's race  */
       char                   *odrop;            /*  Character's title  */
       char                   *osucc;            /*  Character's name suffix  */
       char                   *ofail;            /*  Address of last site character connected from  */
       char                   *odesc;            /*  Character's WWW homepage address  */
       char                   *shortdateformat;  /*  Preferred short date format  */
       char                   *longdateformat;   /*  Preferred long date format  */
       char                   *htmlbackground;   /*  Alternative HTML background image  */
       char                   *dateseparator;    /*  Preferred date/time separator  */
       time_t                 disclaimertime;    /*  Date/time when user last agreed to the terms and conditions of the disclaimer  */
       unsigned char          failedlogins;      /*  Failed logins since last successful login  */
       unsigned char          htmlcmdwidth;      /*  HTML command input box width  */
       time_t                 longestdate;       /*  Date when longest time connected occurred  */
       unsigned long          longesttime;       /*  Longest time connected  */
       unsigned short         restriction;       /*  Maximum credit payable from within compound command  */
       unsigned short         subtopic_id;       /*  Unique ID of current sub-topic within BBS topic currently being browsed  */
       char                   *timeformat;       /*  Preferred time format  */
       struct   currency_data expenditure;       /*  Total expenditure this quarter  */
       long                   quotalimit;        /*  Building Quota limit  */
       dbref                  controller;        /*  DBref of controller (If puppet)  */
       time_t                 damagetime;        /*  Time when character can next '@damage'         */
       time_t                 healthtime;        /*  Time when character's health was last updated  */
       unsigned long          totaltime;         /*  Total time connected  */
       short                  scrheight;         /*  Screen height  */
       char                   *password;         /*  Character's password (Encrypted)  */
       unsigned char          maillimit;         /*  Character's mailbox limit  */
       unsigned short         htmlflags;         /*  HTML preference flags  */
       int                    dateorder;         /*  Preferred date/time order  */
       int                    prefflags;         /*  Preference flags  */
       struct   alias_data    *aliases;          /*  Character's aliases  */
       struct   profile_data  *profile;          /*  Character's profile  */
       struct   friend_data   *friends;          /*  Character's list of friends/enemies  */
       unsigned long          idletime;          /*  Total time spent idling (Over 5 minutes)  */
       time_t                 lasttime;          /*  Last time connected  */
       time_t                 pwexpiry;          /*  Date/time when user last changed their password  */
       dbref                  redirect;          /*  Who to redirect mail to  */
       char                   timediff;          /*  Time difference  */
       unsigned short         topic_id;          /*  Unique ID of current BBS topic  */
       struct   currency_data balance;           /*  Character's bank balance  */
       long                   bantime;           /*  Time at which character's ban will be lifted (0 = Not banned)  */
       unsigned char          feeling;           /*  Character's current feeling (0 = None.)  */
       unsigned short         payment;           /*  Accumulated time (Seconds) since last payment  */
       struct   currency_data credit;            /*  Character's credit  */
       struct   currency_data health;            /*  Character's health  */
       struct   currency_data income;            /*  Total income this quarter  */
       unsigned long          logins;            /*  Total number of logins  */
       int                    volume;            /*  Character's volume  */
       dbref                  chpid;             /*  Character's current effective ID  */
       int                    quota;             /*  Building quota currently in use by character  */
       int                    score;             /*  Character's score  */
       struct   mail_data     *mail;             /*  Character's mail  */
       int                    mass;              /*  Character's mass  */
       unsigned short         lost;              /*  Battles lost  */
       char                   afk;               /*  Idle time (In minutes) after which user will be automatically sent AFK (0 = Disable.)  */
       dbref                  uid;               /*  Current user #ID character is building under  */
       unsigned short         won;               /*  Battles won  */
};


/* ---->  Additional fields for a property data structure  <---- */
struct db_property_data {
       char *desc;  /*  Property's description  */
};


/* ---->  Additional fields for a room data structure  <---- */
struct db_room_data {
       char                 *desc;      /*  Room's description  */
       char                 *drop;      /*  Room's drop message  */
       char                 *succ;      /*  Room's success message  */
       char                 *fail;      /*  Room's failure message  */
       char                 *odrop;     /*  Room's others drop message  */
       char                 *osucc;     /*  Room's others success message  */
       char                 *ofail;     /*  Room's others failure message  */
       char                 *odesc;     /*  Room's outside description  */
       struct boolexp       *lock;      /*  Room's lock  */
       char                 *areaname;  /*  Room's area  */
       char                 *cstring;   /*  Room's (Alternative) 'Contents:' string  */
       char                 *estring;   /*  Room's (Alternative) 'Obvious Exits' string  */
       int                  volume;     /*  Room's volume  */
       int                  mass;       /*  Room's mass  */
       struct currency_data credit;     /*  Credits dropped in room  */
};


/* ---->  Standard database object additional fields layout data structure  <---- */
struct db_standard_data {
       char *desc;   /*  Object's description  */
       char *drop;   /*  Object's drop message  */
       char *succ;   /*  Object's success message  */
       char *fail;   /*  Object's failure message  */
       char *odrop;  /*  Object's others drop message  */
       char *osucc;  /*  Object's others success message  */
       char *ofail;  /*  Object's others failure message  */
       char *odesc;  /*  Object's outside description  */
};


/* ---->  Additional fields for a thing data structure  <---- */
struct db_thing_data {
       char                 *desc;      /*  Thing's description  */
       char                 *drop;      /*  Thing's drop message  */
       char                 *succ;      /*  Thing's success message  */
       char                 *fail;      /*  Thing's failure message  */
       char                 *odrop;     /*  Thing's others drop message  */
       char                 *osucc;     /*  Thing's others success message  */
       char                 *ofail;     /*  Thing's others failure message  */
       char                 *odesc;     /*  Thing's outside description  */
       struct boolexp       *lock;      /*  Thing's lock  */
       struct boolexp       *lock_key;  /*  Thing's lock key (Lock must be satisfied to lock/unlock)  */
       char                 *areaname;  /*  Thing's area name  */
       char                 *cstring;   /*  Thing's (Alternative) 'Contents:' string  */
       char                 *estring;   /*  Thing's (Alternative) 'Obvious Exits' string  */
       int                  volume;     /*  Thing's volume  */ 
       int                  mass;       /*  Thing's mass  */
       struct currency_data credit;     /*  Credits dropped in thing  */
};


/* ---->  Additional fields for a variable data structure  <---- */
struct db_variable_data {
       char *desc;   /*  Variable's description  */
       char *drop;   /*  Variable's drop message  */
       char *succ;   /*  Variable's success message  */
       char *fail;   /*  Variable's failure message  */
       char *odrop;  /*  Variable's others drop message  */
       char *osucc;  /*  Variable's others success message  */
       char *ofail;  /*  Variable's others failure message  */
       char *odesc;  /*  Variable's outside description  */
};


/* ---->  Additional fields per object type union data structure  <---- */
union db_extra_data
{
      struct db_standard_data standard;  /*  Standard Fields  */
      struct db_property_data property;  /*  Property  */
      struct db_variable_data variable;  /*  Variable  */
      struct db_command_data  command;   /*  Compound command  */
      struct db_player_data   player;    /*  Character  */
      struct db_alarm_data    alarm;     /*  Alarm  */
      struct db_array_data    array;     /*  Array  */
      struct db_thing_data    thing;     /*  Thing  */
      struct db_exit_data     exit;      /*  Exit  */
      struct db_fuse_data     fuse;      /*  Fuse  */
      struct db_room_data     room;      /*  Room  */
};


/* ---->  Editor session data structure  <---- */
struct edit_data {
       union    group_data *data;       /*  Pointer to mail item, BBS message/topic, etc. being edited  */
       char                objecttype;  /*  Type of object being edited  */
       unsigned char       permanent;   /*  Settings of .editsucc and .editfail are permanent?  */
       unsigned long       checksum;    /*  Checksum of object currently being edited  */
       int                 element;     /*  Element of dynamic array currently being edited  */
       char                *prompt;     /*  Old user prompt (Before entering editor)  */
       dbref               object;      /*  Object currently being edited  */
       char                *index;      /*  Index name of dynamic array element  */
       char                *data1;      /*  Used for temporary storage  */
       char                *data2;      /*  Used for temporary storage  */
       unsigned char       field;       /*  Field currently being edited  */
       char                *text;       /*  Edit text  */
       dbref               succ;        /*  Compound command to execute on exiting editor  */
       dbref               fail;        /*  Compound command to execute on aborting edit (.abort)  */
       short               line;        /*  Line number currently being edited   */
       int                 temp;        /*  Used for temporary storage  */
};


/* ---->  Linked list of fuses/alarms to execute (Ordered by time) data structure  <---- */
struct event_data {
       struct event_data *next;    /*  Pointer to next event in queue  */
       char              *string;  /*  Text parameters of event (Fuse)  */
       dbref             command;  /*  Compound command to execute  */
       dbref             object;   /*  Fuse/alarm  */
       dbref             player;   /*  User who involked event (Executed under their ID)  */
       dbref             data;     /*  Object to which fuse/alarm is attached  */
       time_t            time;     /*  Execution time/date of event  */
};


/* ---->  Feeling command definition data structure  <---- */
struct feeling_command_data {
       const char *keywords;  /*  Allowed keywords, separated by ';'  */
       const char *dkeyword;  /*  Default keyword (Used if keywords is NULL.)  */
       const char *daction;   /*  Default keyword (Used if <ACTION> is ommited.)  */
       const char *dtext;     /*  Default text (Used if <NAMES|TEXT> is ommited.)  */
       int        flags;      /*  Feeling command flags  */
       const char *dcolour;   /*  Default colour code for '%x'  -  Default colour  */
       const char *ucolour;   /*  Default colour code for '%n'  -  Feeling user name  */
       const char *rcolour;   /*  Default colour code for '$3'  -  Recipient names  */
       const char *decho;     /*  Message to output to feeling user  */
       const char *uecho;     /*  Message to output to recipient user(s)  */
       const char *recho;     /*  Message to output to other users in the same location  */
};


/* ---->  'Feeling' array data structure ('@feeling')  <---- */
struct feeling_data {
       struct   feeling_data *next;  /*  Next entry in list  */
       const    char         *name;  /*  Name of feeling  */
       unsigned char         id;     /*  ID of feeling  */
};


/* ---->  Feeling command list data structure  <---- */
struct feeling_list_data {
       struct feeling_data         *next;      /*  Next feeling command in sorted list  */
       struct feeling_data         *left;      /*  Binary tree left pointer  */
       struct feeling_data         *right;     /*  Binary tree right pointer  */
       struct feeling_command_data *feeling;   /*  Pointer to feeling command definition  */
       const  char                 *name;      /*  Feeling command name  */
       int                         reference;  /*  Reference number to same feeling command (0 = First reference)  */
};


/* ---->  Friends list data structure  <---- */
struct friend_data {
       struct   friend_data *next;   /*  Next friend in list  */
       dbref                friend;  /*  DBref of friend  */
       unsigned long        flags;   /*  Friend flags  */
};


/* ---->  Global commands linked list data structure  <---- */
struct global_data {
       struct global_data *centre;  /*  Tertiary tree centre pointer (=)  */
       struct global_data *right;   /*  Tertiary tree right pointer  (>)  */
       struct global_data *left;    /*  Tertiary tree left pointer   (<)  */
       struct global_data *last;    /*  Tertiary tree previous node       */
       struct global_data *next;    /*  Linked list next pointer     (>)  */
       struct global_data *prev;    /*  Linked list previous pointer (<)  */
       dbref              command;  /*  #ID of compound command           */
       char               *name;    /*  Single name of compound command   */
};


/* ---->  Grouping/range operator data structure  <---- */
struct grp_data {

       /* ---->  General  <---- */
       struct   grp_data      *next;
       int                    totalitems;     /*  Total number of items processed  */
       int                    rangeitems;     /*  Total number of items in given range  */
       int                    groupitems;     /*  Total number of items in group  */
       int                    groupsize;      /*  Size of each group (In items)  */
       int                    distance;       /*  Distance (In items) between CDESC and DEFDESC  */
       int                    nogroups;       /*  Total number of groups of given group size  */
       int                    groupno;        /*  Group to be processed  */
       int                    before;         /*  Number of items before RFROM  */
       int                    rfrom;          /*  Range from  */
       int                    rto;            /*  Range to  */

       /* ---->  DB entry processing pointers  <---- */
       dbref                  cobject;        /*  Current DB object being processed  */
       dbref                  nobject;        /*  Next object to be processed (Cached)  */
       dbref                  cobj;           /*  Current object (To which list currently being processed is attached)  */
       int                    list;           /*  List object(s) being processed are in, e.g:  CONTENTS  */

       /* ---->  Union linked list processing pointers  <---- */
       union    group_data    *cunion;        /*  Current linked list item being processed  */
       union    group_data    *nunion;        /*  Next linked list item to be processed  */

       /* ---->  Conditions  <---- */
       int                    object_flags2;  /*  Secondary flags object must have to meet condition  */
       int                    object_mask2;   /*  Secondary flags object mustn't have to meet condition  */
       const    char          *object_name;   /*  Wildcard spec. object's name/desc must meet  */
       int                    object_flags;   /*  Primary flags object must have to meet condition  */
       int                    object_mask;    /*  Primary flags object mustn't have to meet condition  */
       int                    object_type;    /*  Type object must be to meet condition  */
       int                    object_who;     /*  (Descriptor list)  Single character to do grouping/range op on (For who lists)  */
       int                    object_exc;     /*  Primary flags to exclude  */
       unsigned short         condition;      /*  Condition(s) that must be met to process each list item  */
       dbref                  player;         /*  Character who's doing the grouping/range op.  */

       /* ---->  Creation date/last used date boundaries  <---- */
       unsigned long          clower,cupper;  /*  Creation date lower and upper boundaries  */
       unsigned long          llower,lupper;  /*  Last used date lower and upper boundaries  */
       unsigned long          time;           /*  Current time  */
};


/* ---->  Page of help text data structure  <---- */
struct helppage {
       struct helppage *next;  /*  Pointer to next page of help in topic  */
       char            *text;  /*  Help text of help topic  */
};


/* ---->  Help topic data structure  <---- */
struct helptopic {
       struct   helptopic *next;       /*  Pointer to next help topic in linked list  */
       struct   helppage  *page;       /*  Pointer to first page of help in topic  */
       char               *topicname;  /*  Name of help topic (Used for matching)  */
       char               *title;      /*  Title of help topic  */
       unsigned char      pages;       /*  Number of pages in help topic  */
       unsigned char      flags;       /*  Help topic flags  */
};


/* ---->  'more' pager data structure for a single line  <---- */
struct line_data {
       struct   line_data *next;  /*  Pointer to next line  */
       struct   line_data *prev;  /*  Pointer to previous line  */
       unsigned char      *text;  /*  Text of line  */
       int                len;    /*  Length of line  */
};


/* ---->  Character list data structure (Used in pagetell.c and bbs.c)  <---- */
struct list_data {
       struct list_data *next;   /*  Next user in list  */
       dbref            player;  /*  DBref of character  */
};


/* ---->  Mail item data structure  <---- */
struct mail_data {
       struct   mail_data *next;      /*  Next item of mail in linked list  */
       char               *redirect;  /*  Name of redirector  */
       char               *subject;   /*  Subject of mail  */
       char               *message;   /*  Message  */
       long               lastread;   /*  Date mail was last read  */
       char               *sender;    /*  Name of sender  */
       unsigned char      flags;      /*  Mail flags  */
       long               date;       /*  Date mail was sent  */
       dbref              who;        /*  DBref of sender  */
       unsigned long      id;         /*  ID of mail (For group-mail)  */
};


/* ---->  Match data structure  <---- */
struct match_data {
       struct   match_data *next;       /*  Next match on stack  */
       struct   match_data *prev;       /*  Previous match on stack  */

       dbref               start;       /*  Starting object in current phase (If next object = START, match has looped)  */
       dbref               end;         /*  Ending object (If starting object is this, end match (Used for contraining area match))  */
       dbref               currentobj;  /*  Current object (Inheritance)  */
       dbref               current;     /*  Current object (Match)  */
       dbref               parent;      /*  Current parent object  */
       dbref               exclude;     /*  Exclude object  */
       unsigned char       recursed;    /*  Current recursion level  */

       dbref               owner;       /*  Owner of match  */
       unsigned char       phase;       /*  Current phase  */
       unsigned char       endphase;    /*  Ending phase  */
       char                order;       /*  Current position in match order  */
       int                 types;       /*  Types of object to match against  */
       int                 subtypes;    /*  Sub-types of object to match against  */
       int                 options;     /*  Matching options  */

       char                *search;     /*  Search string (Before ':')  */
       char                *index;      /*  Dynamic array index  */
       char                *remainder;  /*  Remainder of search string (After ':')  */
       char                indexchar;   /*  Character at dynamic array index pointer  */
};


/* ---->  Character list data structure (Used in mail.c)  <---- */
/*        IMPORTANT:  This structure must remain in order       */
struct mlist_data {
       struct mlist_data *next;     /*  Next user in list  */
       dbref             player;    /*  DBref of character  */
       dbref             redirect;  /*  DBref of character to which mail is redirected  */
};


/* ---->  Module details data structure  <---- */
struct module_details {
       struct   module_details *next;  /*  Next module in alphabetical order  */
       const    char           *datefrom;        /*  Date from when current author was actively working on module  */
       const    char           *dateto;          /*  Date to when current author was actively working on module  */
       unsigned char           module;           /*  Module worked on by current author ('author NAME')  */
       unsigned char           original;         /*  Original author of module ('author NAME')  */
       const    char           *name;            /*  Name of module                     */
       const    char           *date;            /*  Pointer to module creation date    */
       const    char           *desc;            /*  Pointer to module description      */
       const    char           *authors;         /*  Pointer to module authors          */
};


/* ---->  Basic database object data structure  <---- */
struct object {
       union    db_extra_data *data;        /*  Pointer to object's extra data (NULL if not applicable to object type)  */
       char                   *name;        /*  Object's name  */
       unsigned char          type;         /*  Object type  */

       int                    flags;        /*  Primary object flags  */
       int                    flags2;       /*  Secondary object flags  */

       dbref                  destination;  /*  Destination of object  */
       dbref                  location;     /*  Current location of object  */
       dbref                  parent;       /*  Parent of object  */
       dbref                  owner;        /*  Owner of object  */

       dbref                  variables;    /*  Pointer to first variable/property/array in object's variables list  */
       dbref                  contents;     /*  Pointer to first item in object's contents list or CSUCC branch of compound command  */
       dbref                  commands;     /*  Pointer to first compound command in object's compound commands list  */
       dbref                  exits;        /*  Pointer to first exit in object's exits list or CFAIL branch of compound command  */
       dbref                  fuses;        /*  Pointer to first fuse/alarm in object's fuses list  */
       dbref                  next;         /*  Pointer to next object in whatever linked list object is in (Contents, exits, variables, etc.)  */

       long                   lastused;     /*  Date and time of last use of object  */
       long                   created;      /*  Creation date/time of object  */
       unsigned char          expiry;       /*  Expiry time (In days) of object (0 = No expiry)  */

       unsigned long          checksum;     /*  Object 'checksum' (Used by '@undestroy', not dumped to disk)  */
};


/* ---->  Option list dynamic array data structure (options.c)  <---- */
struct option_list_data {
       struct option_list_data *next;     /*  Next option  */
       const  char             *longopt;  /*  Long option name  */
       const  char             *optarg;   /*  Option arguments  */
       char                    opt;       /*  Option character  */
};


/* ---->  'more' pager data structure  <---- */
struct pager_data {
       struct line_data *current;  /*  Pointer to current line   */
       struct line_data *head;     /*  Pointer to first line  */
       struct line_data *tail;     /*  Pointer to last line  */
       char             *prompt;   /*  'more' pager prompt  */
       int              lines;     /*  Total number of lines  */
       int              line;      /*  Current line number  */
       unsigned long    size;      /*  Total size of paged text  */
};


/* ---->  Profile data structure  <---- */
struct profile_data {
       char           *qualifications;  /*  Qualifications           */
       char           *achievements;    /*  Achievements             */
       char           *nationality;     /*  Nationality              */
       char           *occupation;      /*  Occupation               */
       char           *interests;       /*  Interests                */
       unsigned char  sexuality;        /*  Sexuality                */
       unsigned char  statusirl;        /*  Status (IRL)             */
       unsigned char  statusivl;        /*  Status (IVL)             */
       char           *comments;        /*  Other comments           */
       char           *dislikes;        /*  Dislikes                 */
       char           *country;         /*  Home country             */
       char           *hobbies;         /*  Hobbies                  */
       char           *picture;         /*  URL of picture           */
       unsigned short height;           /*  Height                   */
       int            weight;           /*  Weight                   */
       char           *drink;           /*  Favourite drinks         */
       char           *likes;           /*  Likes                    */
       char           *music;           /*  Favourite music          */
       char           *other;           /*  Other information        */
       char           *sport;           /*  Favourite sports         */
       char           *city;            /*  Home town/city           */
       char           *eyes;            /*  Eye colour               */
       char           *food;            /*  Favourite foods          */
       char           *hair;            /*  Hair colour              */
       char           *irl;             /*  IRL (In Real Life) name  */
       unsigned long  dob;              /*  Date Of Birthday in LONGDATE format (DD/MM/YYYY)  */
};


/* ---->  Interactive '@prompt' session data structure  <---- */
struct prompt_data {
       struct temp_data *temp;    /*  Temporary variables created during interactive '@prompt' session  */
       char             *prompt;  /*  User prompt during interactive '@prompt' session  */
       dbref            process;  /*  Compound command to execute each time user input is received  */
       dbref            succ;     /*  Compound command to execute when PROCESS compound command succeeds (Ending interactive '@prompt' session)  */
       dbref            fail;     /*  Compound command to execute when interactive '@prompt' session is aborted (Using the 'ABORT' command.)  */
};


/* ---->  Rank ('@rank') tertiary tree node data structure  <---- */
struct rank_data {
       struct rank_data *centre;
       struct rank_data *right;
       struct rank_data *left;
       dbref            object;
};


/* ---->  Page/tell recall data structure  <---- */
struct recall_data {
       char          *message;  /*  Message sent by user  */
       dbref         pager;     /*  User who sent message  */
       long          time;      /*  Time when message was sent  */
       unsigned char tell;      /*  Type of message:  0 = page, 1 = tell  */
};


/* ---->  New character request data structure  <---- */
struct request_data {
       struct   request_data *next;    /*  Pointer to next request  */
       unsigned long         address;  /*  Internet site request was made from  */
       char                  *email;   /*  E-mail address  */
       char                  *name;    /*  Preferred character name  */
       time_t                date;     /*  Time/date request was made  */
       dbref                 user;     /*  User currently dealing with request  */
       unsigned short ref;             /*  Reference number  */
};


/* ---->  Registered Internet site data structure  <---- */
struct site_data {
       struct site_data *next;            /*  Next site in linked list  */
       short            max_connections;  /*  Maximum connections allowed from site  */
       char             *description;     /*  Description of site  */
       unsigned long    connected;        /*  Number of characters connected from site  */
       unsigned short   created;          /*  Number of characters created from site  */
       unsigned char    flags;            /*  Site's flags  */
       unsigned long    mask;             /*  IP address mask of site  */
       unsigned long    addr;             /*  IP address of site  */
};


/* ---->  Server statistics data structure  <---- */
struct stat_data {
       unsigned long charconnected;  /*  Number of characters connected  */
       unsigned long objdestroyed;   /*  Number of objects destroyed  */
       unsigned long charcreated;    /*  Number of characters created  */
       unsigned long objcreated;     /*  Number of objects created  */
       unsigned long shutdowns;      /*  Number of shutdowns/crashes  */
       unsigned long peak;           /*  Peak number of connected (Simultaneously) users  */
       time_t        time;           /*  Date of statistics entry  */
};


/* ---->  String operation data structure (Used by str???_limits())  <---- */
struct str_ops {
       unsigned char backslash;  /*  Next character protected by '\'?  */
       int           length;     /*  Current length of destination string  */
       char          *dest;      /*  Destination pointer  */
       char          *src;       /*  Source pointer  */
};


/* ---->  Substitution return data structure (For large substitutions)  <---- */
struct substitution_data {
       char          cur_ansi[16];  /*  Current ANSI code in effect  */
       int           textflags;     /*  Text flags  */
       char          *cur_bg;       /*  Current background colour  */
       unsigned char flags;         /*  Substitution flags  */
};


/* ---->  Temporary variable data structure (Used by '@temp')  <---- */
struct temp_data {
       struct temp_data *next;   /*  Pointer to next temporary variable in linked list  */
       struct temp_data *prev;   /*  Pointer to previous temporary variable in linked list  */
       struct temp_data *right;  /*  Pointer to next alphabetically greater temporary variable in binary tree  */
       struct temp_data *left;   /*  Pointer to next alphabetically smaller temporary variable in binary tree  */
       struct temp_data *last;   /*  Pointer to previous node in binary tree  */
       char             *name;   /*  Name of temporary variable  */
       char             *desc;   /*  Contents (Description) of temporary variable  */
};


/* ---->  Terminal definition data structure  <---- */
struct termcap_data {
       struct termcap_data *next;       /*  Next entry in list  */
       char                *underline;  /*  Control code to enable underline  */
       char                *inverse;    /*  Control code to enable inversed text  */
       unsigned char       height;      /*  Default height of terminal  */
       char                *normal;     /*  Control code for 'normal' text (No bold, underline, etc.)  */
       char                *blink;      /*  Control code for flashing text  */
       unsigned char       width;       /*  Default width of terminal  */
       char                *bold;       /*  Control code for bold text  */
       char                *name;       /*  Terminal name(s) (As defined in /etc/termcap)  */
       char                *tc;         /*  Get rest of terminal details from another entry  */
};


/* ---->  Single line of text data structure (Input/Output)  <---- */
struct text_data {
       struct  text_data *next;  /*  Next line of text  */
       char              *text;  /*  Pointer to text data of line  */
       int               len;    /*  Length of text data  */
};


/* ---->  Queue of text data structure (Input/Output)  <---- */
struct text_queue_data {
       struct text_data *start;  /*  Start of pending queue of output  */
       struct text_data *end;    /*  End of pending queue of output  */
       int              size;    /*  Size of pending queue of output  */
};


/* ---->  Data structure used to process GROUP_DATA linked list (x->data->next)  <---- */
struct union_data {
       union group_data *next;
};


/* ---->  User list data structure  <---- */
struct userlist_data {
       struct userlist_data *next;        /*  Next entry in list  */
       dbref                user;         /*  #ID of user (Or group owner for friend groups)  */
       unsigned short       userflags;    /*  User list flags  */
       unsigned char        customflags;  /*  Custom flags  */
       unsigned char        group;        /*  ID of group  */
       char                 *name;        /*  Name of friend group (If unallocated, this is looked up)  */
};


/* ---->  Descriptor list data structure  <---- */
struct descriptor_data {
       struct   descriptor_data *next;            /*  Next descriptor in descriptor list  */
       struct   descriptor_data *prev;            /*  Previous descriptor in descriptor list  */
       struct   recall_data     messages[MAX_STORED_MESSAGES];  /*  Stored pages/tells for recall via 'recall'  */
       struct   cmdprompt_data  *cmdprompt;       /*  'Interactive' '@prompt' session data  */
       struct   helptopic       *helptopic;       /*  Last help topic accessed by user  */
       struct   descriptor_data *monitor;         /*  Pointer to monitoring descriptor ('@monitor')  */
       struct   termcap_data    *termcap;         /*  Pointer to user's terminal capabilities entry for their terminal type  */
       struct   prompt_data     *prompt;          /*  'Interactive' '@prompt' session data  */
       struct   text_queue_data output;           /*  Queued output from TCZ to user  */
       struct   pager_data      *pager;           /*  Pointer to 'more' paged output  */
       struct   edit_data       *edit;            /*  Editor data  */
       struct   site_data       *site;            /*  Pointer to user's Internet site (In registered Internet site database)  */
       struct   text_queue_data input;            /*  Queued input from user to TCZ  */
       unsigned char            terminal_height;  /*  User's terminal height  */
       time_t                   emergency_time;   /*  Time when emergency command logging on the user expires  */
       unsigned char            terminal_width;   /*  User's terminal width  */
       char                     *terminal_type;   /*  User's terminal type  */
       unsigned char            warning_level;    /*  Idle warning level  */
       short                    terminal_xpos;    /*  Current X position on user's terminal  */
       unsigned char            *raw_input_at;    /*  Pointer to current position in raw input data from descriptor  */
       char                     *last_command;    /*  Last command typed by user   */
       char                     *user_prompt;     /*  User's current prompt  */
       char                     *afk_message;     /*  User's AFK (Away From Keyboard) message  */
       char                     *lastmessage;     /*  Last message sent via page/tell  */
       unsigned char            messagecount;     /*  Count of stored pages/tells  */
       short                    currentpage;      /*  Number of last page of help read by user ('help next/prev')  */
       time_t                   assist_time;      /*  Time when user may next ask for asssistance  */
       int                      page_clevel;      /*  d->clevel to return to after last page in simple 'more' pager is shown  */
       short                    currentmsg;       /*  Number of last message read by user on the BBS ('read next/prev')  */
       time_t                   start_time;       /*  Time at which user connected to TCZ  */
       int                      descriptor;       /*  User's descriptor  */
       unsigned char            *raw_input;       /*  Pointer to raw input data from descriptor  */
       int                      page_title;       /*  Simple 'more' pager title screen number  */
       char                     *helpname;        /*  Topic name of last help topic viewed by user  */
       char                     *hostname;        /*  Host name of user's Internet site  */
       char                     *password;        /*  Stored password (Used for character creation and password change only)  */
       time_t                   last_time;        /*  Time of last input from user  */
       time_t                   name_time;        /*  Time when user may change their name again  */
       time_t                   next_time;        /*  Next time user may execute a command  */
       char                     *subject;         /*  Subject of chatting channel user is operator of  */
       time_t                   afk_time;         /*  Time when user went AFK  */
       char                     *assist;          /*  Reason for seeking assistance  */
       char                     *chname;          /*  Name of last used channel  */
       unsigned char            *negbuf;          /*  Buffer for 'split' Telnet negotiations  */
       int                      channel;          /*  Current chatting channel in use by user  */
       unsigned long            address;          /*  User's IP address  */
       unsigned short           neglen;           /*  Length of Telnet negotiation buffer  */
       dbref                    player;           /*  DBref of Character in use by user  */
       char                     clevel;           /*  Connection level:  Used by initial prompting (At login screen)  */
       dbref                    summon;           /*  Who character has been summoned by  */
       int                      flags2;           /*  Secondary descriptor flags  */
       int                      flags;            /*  Primary descriptor flags  */
       char                     *name;            /*  Stored character name (Used at login only)  */
       int                      page;             /*  Simple 'more' pager page number  */
};


/* ---->  Destroy queue data structure  <---- */
struct destroy_data {
       struct   destroy_data *next;      /*  Next entry in destroyed objects queue  */
       struct   destroy_data *prev;      /*  Previous entry in destroyed objects queue  */
       struct   object       obj;        /*  Object data of destroyed object  */
       unsigned char         recovered;  /*  Object recovered?  */
       int                   quota;      /*  Building Quota used by object  */
       dbref                 id;         /*  Original DBref of object  */

       struct checksum_data {            /*  Object checksum reference numbers  */
              unsigned long  destination;
              unsigned long  controller;
              unsigned long  location;
              unsigned long  redirect;
              unsigned long  contents;
              unsigned long  parent;
              unsigned long  exits;
              unsigned long  owner;
              unsigned long  uid;
       } checksum;
};


/* ---->  Editor command table data structure  <---- */
struct edit_cmd_table {
       short            flags;
       char             *name;
       void     (*func) (struct descriptor_data *, int, int, int, int, int, unsigned char, char *, char *, int, char *, char *);  /*  d, rfrom, rto, option, numeric, lines, jump, text, params, val1, editcmd, buffer  */
       int              val1;
       unsigned char    len;
};


/* ---->  Group data structure (Used by union grouprange grouping/range ops)  <---- */
union group_data {
      struct descriptor_data   descriptor;    /*  Descriptor list         */
      struct bbs_list_data     bbslist;       /*  BBS summary             */
      struct channel_data      channel;       /*  Channels                */
      struct destroy_data      destroy;       /*  Destroyed objects       */
      struct array_element     element;       /*  Dynamic array elements  */
      struct feeling_data      feeling;       /*  Feelings                */
      struct bbs_message_data  message;       /*  BBS messages            */
      struct request_data      request;       /*  New character requests  */
      struct channel_user_data chuser;        /*  Channel users           */
      struct author_details    author;        /*  Author information      */
      struct banish_data       banish;        /*  Banished names          */
      struct friend_data       friend;        /*  Friends/enemies         */
      struct module_details    module;        /*  Module information      */
      struct bbs_reader_data   reader;        /*  BBS message readers     */
      struct alias_data        alias;         /*  Aliases                 */
      struct event_data        event;         /*  Events (Alarms/Fuses)   */
      struct mlist_data        mlist;         /*  Mail user list          */
      struct bbs_topic_data    topic;         /*  BBS topics/sub-topics   */
      struct union_data        data;          /*  Unspecified data        */
      struct list_data         list;          /*  User list               */
      struct mail_data         mail;          /*  Mail items              */
      struct site_data         site;          /*  Internet sites          */
};


#endif  /* STRUCTURES_H */
