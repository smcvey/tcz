 /*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| DB.C  -  Implements reading/writing database to/from disk and low-level     |
|          database manipulation routines.                                    |
|                                                                             |
|          Includes Zero Memory Database Dumping, which allows the database   |
|          to be dumped in the background without additional memory overhead. |
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
|                        https://github.com/smcvey/tcz                        |
`-----------------------------------------------------------------------------'

*/


#include <sys/wait.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>


#include "structures.h"
#include "logfiles.h"
#include "externs.h"
#include "command.h"
#include "config.h"
#include "db.h"

#include "object_types.h"
#include "friend_flags.h"
#include "fields.h"
#include "signallist.h"
#include "objectlists.h"
#include "flagset.h"
#include "search.h"
#include "match.h"
#include "quota.h"
#include "teleport.h"


/* ---->  Database/registered Internet site DB revision numbers  <---- */
#define DATABASE_REVISION 104     /*  Revision of database format  */
#define LOWEST_REVISION   29      /*  Lowest database format revision supported  */
#define SITE_DB_REVISION  2       /*  Revision of registered Internet sites database  */
#define DB_GROWSIZE       1024    /*  Number of new objects to increase size of database by, if all free objects have been used  */
#define FIELD_SEPARATOR   '\027'  /*  Separator used to separate fields in the database file  */


/* ---->  Profile/date constants  <---- */
char          *monthabbr[13] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", "???"};
char          *sexuality[10] = {"reset", "Heterosexual", "Homosexual", "Bisexual", "Asexual", "Gay", "Lesbian", "Transvestite", "Unsure", "Please ask"};
char          *statuses[11]  = {"reset", "Available", "Unavailable", "Single", "Engaged", "Married", "Divorced", "Widowed", "Dating", "Committed", "Separated"};
unsigned char leapmdays[13]  = {31,29,31,30,31,30,31,31,30,31,30,31,0};
char          *dayabbr[8]    = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "???"};
char          *genders[4]    = {"Not set", "Neuter", "Female", "Male"};
char          *month[13]     = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December", "???"};
unsigned char mdays[13]      = {31,28,31,30,31,30,31,31,30,31,30,31,0};
char          *day[8]        = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "???"};


/* ---->  Article constants  <---- */
char    *article[16]   = {"", "a ",   "an ",  "some ",  /*  <--  Indefinite  */
                          "", "A ",   "An ",  "Some ",  /*  <--  article     */
                          "", "the ", "the ", "the ",   /*  <==  Definite    */  
                          "", "The ", "The ", "The "};  /*  <==  article     */


/* ---->  Pronoun constants (Unset, Neuter, Female, Male)  <---- */
char    *possessive[8] = {"its",    "its",    "her",     "his",       /*  %p  */
                          "Its",    "Its",    "Her",     "His"};      /*  %P  */
char    *subjective[8] = {"it",     "it",     "she",     "he",        /*  %s  */
                          "It",     "It",     "She",     "He"};       /*  %S  */
char    *objective[8]  = {"it",     "it",     "her",     "him",       /*  %o  */
                          "It",     "It",     "Her",     "Him"};      /*  %O  */
char    *reflexive[8]  = {"itself", "itself", "herself", "himself",   /*  %v  */
                          "Itself", "Itself", "Herself", "Himself"};  /*  %V  */
char    *absolute[8]   = {"its",    "its",    "hers",    "his",       /*  %a  */
                          "Its",    "Its",    "Hers",    "His"};      /*  %A  */


/* ---->  Object/character rank names  <---- */
char    *names[]       = {"Free","Thing","Exit","Character","Room","Compound command","Fuse","Alarm","Variable","Dynamic array","Property"};
char    *clevels[]     = {"The Supreme Being","Deities and above","Elder Wizards/Druids and above","Wizards/Druids and above","Apprentice Wizards/Druids and above","Experienced Builders/Assistants and above","Builders and above","Mortals and above","Morons and above"};


/* ---->  Destroy queue, event queue and database   <---- */
struct   destroy_data *destroy_queue_tail = NULL;
unsigned long         memory_restriction  = 0;
short                 destroy_queue_size  = 0;
dbref                 db_free_chain_end   = NOTHING;
struct   destroy_data *destroy_queue      = NULL;
dbref                 db_free_chain       = NOTHING;
struct   event_data   *event_queue        = NULL;
unsigned long         checksum            = 0;
dbref                 db_top              = 0;
struct   object       *db                 = NULL;

char                  objname[BUFFER_LEN + KB];
struct   grp_data     *grp = &grproot;
char                  cmpbuf[BUFFER_LEN];
struct   grp_data     grproot;
char                  *cmpptr             = cmpbuf;


/* ---->  Bandwidth usage statistics  <---- */
unsigned long      upper_bytes_out = 0;
unsigned long      lower_bytes_out = 0;
unsigned long      upper_bytes_in  = 0;
unsigned long      lower_bytes_in  = 0;


/* ---->  Inheritance and output redirect  <---- */
dbref   parent_object              = NOTHING;
dbref   redirect_dest              = NOTHING;
dbref   redirect_src               = NOTHING;
int     inherited                  = 0;


/* ---->  Database dumping  <---- */
static  int           dumpparent   = NOTHING;
static  unsigned long totaldata    = 0;
int                   dumpchild    = NOTHING;
static  long          dumpdata     = 0;
static  int           dumpsize     = 0;

unsigned char         dumpsanitise = 0;
unsigned char         idle_state   = 0;
unsigned char         log_stderr   = 1;
long                  dumptiming   = 0;
unsigned char         dumpstatus   = 0;
char                  *dumpfile    = NULL;
char                  *lockfile    = NULL;
char                  *dumpfull    = NULL;
long                  nextcycle    = 0;
unsigned char         dumperror    = 0;
unsigned char         dumptype     = 0;
unsigned char         lib_dir      = 0;

static   char         writebuffer[BUFFER_LEN];
unsigned long         bytesread    = 0;


/* ---->  Server name and networking parameters  <---- */
const    char  *email_forward_name = NULL;                /*  TCZ E-mail forwarding address (Appended after '@')  */
const    char  *operatingsystem    = NULL;                /*  Host operating system  */
const    char  *tcz_server_name    = NULL;                /*  DNS name or IP address of TCZ server  */
const    char  *tcz_admin_email    = NULL;                /*  TCZ Admin. E-mail address  */
const    char  *tcz_short_name     = NULL;                /*  TCZ short abbreviated name (TCZ)  */
const    char  *tcz_full_name      = NULL;                /*  TCZ full name (The Chatting Zone)  */
const    char  *html_home_url      = NULL;                /*  URL of TCZ web site  */
const    char  *tcz_location       = NULL;                /*  Location of TCZ server (Full name and location of ISP, company, university, etc.)  */
const    char  *tcz_timezone       = NULL;                /*  Server time zone  */
const    char  *tcz_prompt         = NULL;                /*  Standard user prompt ('TCZ>')  */

unsigned long  tcz_server_network  = TCZ_SERVER_NETWORK;  /*  Network address of TCZ server  */
unsigned long  tcz_server_netmask  = TCZ_SERVER_NETMASK;  /*  Network mask of TCZ server  */
unsigned long  tcz_server_ip       = TCZ_SERVER_IP;       /*  IP address of TCZ server  */
unsigned long  telnetport          = TELNETPORT;          /*  Telnet port number  */
int      tcz_year                  = 1993;                /*  Current year  */


/* ---->  Administrative Options  <---- */
long           dumpinterval        = DUMP_INTERVAL;              /*  Database dumping interval                 */
long           timeadjust          = 0;                          /*  Server time adjustment                    */
unsigned char  dumpcycle           = DUMP_CYCLE_INTERVAL;        /*  Dump cycle interval                       */
int            dumpdatasize        = DUMP_CYCLE_DATA * KB;       /*  Data dumped per cycle                     */
unsigned char  limit_connections   = 0;                          /*  Limit connections?                        */
short          allowed             = 100;                        /*  Max. connections allowed                  */
dbref          homerooms           = NOTHING;                    /*  DBref of home rooms container  */
unsigned char  creations           = 1;                          /*  Allow creation of new users  */
unsigned char  connections         = 1;                          /*  Allow connections by non-administrators  */
dbref          bankroom            = NOTHING;                    /*  DBref of bank  */
dbref          mailroom            = NOTHING;                    /*  DBref of post office.  */
dbref          bbsroom             = NOTHING;                    /*  DBref of BBS  */
dbref          aliases             = NOTHING;                    /*  DBref of global aliases owner (This character MUST be a Deity for security reasons.)  */
unsigned char  maintenance         = 0;                          /*  Destroy unused characters/objects automatically?  */
dbref          maint_owner         = ROOT;                       /*  Ownership of destroyed character's objects will be changed to this character  */
unsigned short maint_morons        = CHAR_MAINTENANCE_MORONS;    /*  Maintenance time for unused Moron characters  */
unsigned short maint_newbies       = CHAR_MAINTENANCE_NEWBIES;   /*  Maintenance time for unused Newbie characters (Those who have a total connect time of less than 1 day)  */
unsigned short maint_mortals       = CHAR_MAINTENANCE_MORTALS;   /*  Maintenance time for unused Mortal characters  */
unsigned short maint_builders      = CHAR_MAINTENANCE_BUILDERS;  /*  Maintenance time for unused Builder characters  */
unsigned short maint_objects       = OBJ_MAINTENANCE;            /*  Maintenance time for unused objects (Excluding characters)  */
unsigned short maint_junk          = JUNK_MAINTENANCE;           /*  Maintenance time for junked objects located in #0 (ROOMZERO)  */
time_t         quarter             = 0;                          /*  Date of end of financial quarter  */
time_t         logins              = 0;                          /*  Date counting of logins was implemented  */

long     toplevel_commands         = 0;    /*  Number of top-level commands executed since TCZ was started.  */
long     standard_commands         = 0;    /*  Number of standard commands executed (Inc. those executed within compound commands) since TCZ was started.  */
long     compound_commands         = 0;    /*  Number of compound commands executed since TCZ was started    */
long     editor_commands           = 0;    /*  Number of editor commands executed since TCZ was started.     */
short    max_descriptors           = 256;  /*  Maximum number of available descriptors (Automatically calculated at run-time, if possible)  */
time_t   activity                  = 0;    /*  Time of last user activity (Used to enter/leave semi-idle state)  */
long     uptime                    = 0;    /*  Total time TCZ has been running (Since restart)               */


int      security                  = 0;    /*  Security enabled (Irreversible '@chpid' plus no write to self)  */
int      compoundonly              = 0;    /*  Only allow execution of compound commands (Used by '{@?tczlink}' query command.)  */


/* ---->  TCZ statistics ('@stats tcz')  <---- */
struct   stat_data stats[STAT_HISTORY + 2];
long     stat_days                 =  0;
int      stat_ptr                  = -1;

int      db_accumulated_restarts   = 1;     /*  Accumulated number of restarts since the database was created  */
int      db_accumulated_uptime     = 0;     /*  Accumulated uptime since database was created  */
int      db_longest_uptime         = 0;     /*  Longest recorded uptime  */
time_t   db_creation_date          = 0;     /*  Date when database was created  */
time_t   db_longest_date           = 0;     /*  Date when longest uptime occurred  */

struct   request_data *request     = NULL;  /*  Requests for new characters  */
struct   bbs_topic_data *bbs       = NULL;  /*  TCZ BBS topic list           */
struct   banish_data *banish       = NULL;  /*  Banished character namess    */


/* ---->  {J.P.Boggis 07/08/2001}  Restart/dump/log serial numbers  <---- */ 
unsigned long restart_serial_no	   = 1;
unsigned long dump_serial_no	   = 0;
unsigned long log_serial_no        = 0;
time_t        dump_timedate        = 0;


/* ---->  Remove first occurence (And any other occurences for database consistency) of OBJECT from linked list starting at object LIST  <---- */
dbref remove_first(dbref list,dbref object)
{
      dbref newptr,cache,ptr,newlist = NOTHING;

      ptr = list;
      while(Valid(ptr) && (Typeof(ptr) != TYPE_FREE)) {
            if(ptr != Next(ptr)) cache = Next(ptr);
               else cache = NOTHING;

            if(ptr != object) {
               if(Valid(newlist)) {
                  db[newptr].next  = ptr;
                  newptr           = ptr;
                  db[ptr].next     = NOTHING;
	       } else {
                  newlist = newptr = ptr;
                  db[ptr].next     = NOTHING;
	       }
	    } else if((Typeof(object) == TYPE_COMMAND) && Global(effective_location(object)))
               global_delete(object);
            ptr = cache;
      }
      return(newlist);
}

/* ---->  Get ID of first room with area name (Starting from given loc.)  <---- */
dbref get_areaname_loc(dbref location)
{
      const char *ptr;

      for(; Valid(location); location = Location(location)) {
          if((ptr = getfield(location,AREANAME)) && *ptr)
             return(location);
      }
      return(NOTHING);
}

/* ---->  See if THING is in the area of location AREA  <---- */
int in_area(dbref thing,dbref area)
{
    for(; Valid(thing); thing = db[thing].location)
        if(thing == area) return(1);
    return(0);
}

/* ---->  Does the container CONTAINER contain THING?  <---- */
int contains(dbref thing,dbref container)
{
    if(thing == HOME) thing = Destination(container);

    for(; Valid(thing); thing = Location(thing))
        if(thing == container) return(1);
    return(0);
}

/* ---->  Is THING a member of the linked list LIST  <---- */
int member(dbref thing,dbref list)
{
    for(; Valid(list); list = Next(list)) {
        if(Next(list) == list)  db[list].next = NOTHING;
        if(list       == thing) return(1);
    }
    return(0);
}

/* ---->  Insufficient memory:  Log cause, remove lock file and then abort()  <---- */
void abortmemory(const char *reason)
{
     writelog(SERVER_LOG,1,"ABORT","%s",reason);
     if(lockfile) unlink(lockfile);
     abort();
}

/* ---->  Does given character currently have a profile?  <---- */
unsigned char hasprofile(struct profile_data *profile)
{
	 if(!profile) return(0);
	 if(profile->qualifications) return(1);
	 if(profile->achievements) return(1);
	 if(profile->nationality) return(1);
	 if(profile->occupation) return(1);
	 if(profile->interests) return(1);
	 if(profile->sexuality) return(1);
	 if(profile->statusirl) return(1);
	 if(profile->statusivl) return(1);
	 if(profile->comments) return(1);
	 if(profile->dislikes) return(1);
	 if(profile->country) return(1);
	 if(profile->hobbies) return(1);
	 if(profile->picture) return(1);
	 if(profile->height) return(1);
	 if(profile->weight) return(1);
	 if(profile->drink) return(1);
	 if(profile->likes) return(1);
	 if(profile->music) return(1);
	 if(profile->other) return(1);
	 if(profile->sport) return(1);
	 if(profile->city) return(1);
	 if(profile->eyes) return(1);
	 if(profile->food) return(1);
	 if(profile->hair) return(1);
	 if(profile->irl) return(1);
	 if(profile->dob != UNSET_DATE) return(1);
	 return(0);
}

/* ---->  Get memory used by boolean expression  <---- */
unsigned long getsizeboolexp(struct boolexp *lock)
{
         unsigned long size = 0;

	 size += sizeof(struct boolexp);
	 if(lock->sub1) size += getsizeboolexp(lock->sub1);
	 if(lock->sub2) size += getsizeboolexp(lock->sub2);
	 return(size);
}

/* ---->  Get memory used by OBJECT (In bytes)  <---- */
unsigned long getsize(dbref object,unsigned char decompr)
{
	 union    db_extra_data *data;
	 unsigned long size = 0;

	 size += sizeof(struct object);
	 data  = db[object].data;
	 if(db[object].name) size += (strlen(db[object].name) + 1);
	 switch(Typeof(object)) {
		case TYPE_PROPERTY:
		     if(data) {
			size += sizeof(struct db_property_data);
			if(data->property.desc) size += (strlen((decompr) ? decompress(data->property.desc):data->property.desc) + 1);
		     }
		     break;
		case TYPE_VARIABLE:
		     if(data) {
			size += sizeof(struct db_variable_data);
			if(data->variable.desc)  size += (strlen((decompr) ? decompress(data->variable.desc):data->variable.desc) + 1);
			if(data->variable.drop)  size += (strlen((decompr) ? decompress(data->variable.drop):data->variable.drop) + 1);
			if(data->variable.succ)  size += (strlen((decompr) ? decompress(data->variable.succ):data->variable.succ) + 1);
			if(data->variable.fail)  size += (strlen((decompr) ? decompress(data->variable.fail):data->variable.fail) + 1);
			if(data->variable.odrop) size += (strlen((decompr) ? decompress(data->variable.odrop):data->variable.odrop) + 1);
			if(data->variable.osucc) size += (strlen((decompr) ? decompress(data->variable.osucc):data->variable.osucc) + 1);
			if(data->variable.ofail) size += (strlen((decompr) ? decompress(data->variable.ofail):data->variable.ofail) + 1);
			if(data->variable.odesc) size += (strlen((decompr) ? decompress(data->variable.odesc):data->variable.odesc) + 1);
		     }
		     break;
		case TYPE_COMMAND:
		     if(data) {
			size += sizeof(struct db_command_data);
			if(data->command.desc)  size += (strlen((decompr) ? decompress(data->command.desc):data->command.desc) + 1);
			if(data->command.drop)  size += (strlen((decompr) ? decompress(data->command.drop):data->command.drop) + 1);
			if(data->command.succ)  size += (strlen((decompr) ? decompress(data->command.succ):data->command.succ) + 1);
			if(data->command.fail)  size += (strlen((decompr) ? decompress(data->command.fail):data->command.fail) + 1);
			if(data->command.odrop) size += (strlen((decompr) ? decompress(data->command.odrop):data->command.odrop) + 1);
			if(data->command.osucc) size += (strlen((decompr) ? decompress(data->command.osucc):data->command.osucc) + 1);
			if(data->command.ofail) size += (strlen((decompr) ? decompress(data->command.ofail):data->command.ofail) + 1);
			if(data->command.lock)  size += getsizeboolexp(data->command.lock);
		     }
		     break;
		case TYPE_CHARACTER:
		     if(data) {
			struct alias_data  *aptr = data->player.aliases,*aptr2;
			struct friend_data *fptr = data->player.friends;
			struct mail_data   *mptr = data->player.mail;

			size += sizeof(struct db_player_data);
			if(data->player.desc)     size += (strlen((decompr) ? decompress(data->player.desc):data->player.desc) + 1);
			if(data->player.drop)     size += (strlen((decompr) ? decompress(data->player.drop):data->player.drop) + 1);
			if(data->player.succ)     size += (strlen((decompr) ? decompress(data->player.succ):data->player.succ) + 1);
			if(data->player.fail)     size += (strlen((decompr) ? decompress(data->player.fail):data->player.fail) + 1);
			if(data->player.odrop)    size += (strlen((decompr) ? decompress(data->player.odrop):data->player.odrop) + 1);
			if(data->player.osucc)    size += (strlen((decompr) ? decompress(data->player.osucc):data->player.osucc) + 1);
			if(data->player.ofail)    size += (strlen((decompr) ? decompress(data->player.ofail):data->player.ofail) + 1);
			if(data->player.odesc)    size += (strlen((decompr) ? decompress(data->player.odesc):data->player.odesc) + 1);
			if(data->player.password) size += (strlen(data->player.password) + 1);

			/* ---->  Friends  <---- */
			for(; fptr; fptr = fptr->next) size += sizeof(struct friend_data);

			/* ---->  Aliases  <---- */
			for(; aptr; aptr = (aptr->next == data->player.aliases) ? NULL:aptr->next) {
			    size += sizeof(struct alias_data);
			    for(aptr2 = data->player.aliases; aptr2 && !((aptr->id == aptr2->id) || (aptr == aptr2)); aptr2 = (aptr2->next == data->player.aliases) ? NULL:aptr2->next);
			    if(aptr2 && (aptr == aptr2)) {
			       if(aptr->command) size += (strlen((decompr) ? decompress(aptr->command):aptr->command) + 1);
			       if(aptr->alias)   size += (strlen((decompr) ? decompress(aptr->alias):aptr->alias) + 1);
			    }
			}

			/* ---->  Mail  <---- */
			for(; mptr; mptr = mptr->next) {
			    size += sizeof(struct mail_data);
			    if(mptr->redirect) size += (strlen((decompr) ? decompress(mptr->redirect):mptr->redirect) + 1);
			    if(mptr->subject)  size += (strlen((decompr) ? decompress(mptr->subject):mptr->subject) + 1);
			    if(mptr->message)  size += (strlen((decompr) ? decompress(mptr->message):mptr->message) + 1);
			    if(mptr->sender)   size += (strlen((decompr) ? decompress(mptr->sender):mptr->sender) + 1);
			}

			/* ---->  Profile  <---- */
			if(data->player.profile) {
			   size += sizeof(struct profile_data);
			   if(data->player.profile->qualifications) size += (strlen((decompr) ? decompress(data->player.profile->qualifications):data->player.profile->qualifications) + 1);
			   if(data->player.profile->achievements)   size += (strlen((decompr) ? decompress(data->player.profile->achievements):data->player.profile->achievements) + 1);
			   if(data->player.profile->nationality)    size += (strlen((decompr) ? decompress(data->player.profile->nationality):data->player.profile->nationality) + 1);
			   if(data->player.profile->occupation)     size += (strlen((decompr) ? decompress(data->player.profile->occupation):data->player.profile->occupation) + 1);
			   if(data->player.profile->interests)      size += (strlen((decompr) ? decompress(data->player.profile->interests):data->player.profile->interests) + 1);
			   if(data->player.profile->comments)       size += (strlen((decompr) ? decompress(data->player.profile->comments):data->player.profile->comments) + 1);
			   if(data->player.profile->dislikes)       size += (strlen((decompr) ? decompress(data->player.profile->dislikes):data->player.profile->dislikes) + 1);
			   if(data->player.profile->country)        size += (strlen((decompr) ? decompress(data->player.profile->country):data->player.profile->country) + 1);
			   if(data->player.profile->hobbies)        size += (strlen((decompr) ? decompress(data->player.profile->hobbies):data->player.profile->hobbies) + 1);
			   if(data->player.profile->picture)        size += (strlen((decompr) ? decompress(data->player.profile->picture):data->player.profile->picture) + 1);
			   if(data->player.profile->drink)          size += (strlen((decompr) ? decompress(data->player.profile->drink):data->player.profile->drink) + 1);
			   if(data->player.profile->likes)          size += (strlen((decompr) ? decompress(data->player.profile->likes):data->player.profile->likes) + 1);
			   if(data->player.profile->music)          size += (strlen((decompr) ? decompress(data->player.profile->music):data->player.profile->music) + 1);
			   if(data->player.profile->other)          size += (strlen((decompr) ? decompress(data->player.profile->other):data->player.profile->other) + 1);
			   if(data->player.profile->sport)          size += (strlen((decompr) ? decompress(data->player.profile->sport):data->player.profile->sport) + 1);
			   if(data->player.profile->city)           size += (strlen((decompr) ? decompress(data->player.profile->city):data->player.profile->city) + 1);
			   if(data->player.profile->eyes)           size += (strlen((decompr) ? decompress(data->player.profile->eyes):data->player.profile->eyes) + 1);
			   if(data->player.profile->food)           size += (strlen((decompr) ? decompress(data->player.profile->food):data->player.profile->food) + 1);
			   if(data->player.profile->hair)           size += (strlen((decompr) ? decompress(data->player.profile->hair):data->player.profile->hair) + 1);
			   if(data->player.profile->irl)            size += (strlen((decompr) ? decompress(data->player.profile->irl):data->player.profile->irl) + 1);
			}
		     }
		     break;
		case TYPE_ALARM:
		     if(data) {
			size += sizeof(struct db_alarm_data);
			if(data->alarm.desc) size += (strlen((decompr) ? decompress(data->alarm.desc):data->alarm.desc) + 1);
		     }
		     break;
		case TYPE_ARRAY:
		     if(data) {
			struct array_element *current = data->array.start;

			/* ---->  Array elements  <---- */
			size += sizeof(struct db_array_data);
			for(; current; current = current->next) {
			    size += sizeof(struct array_element);
			    if(current->index) size += (strlen(current->index) + 1);
			    if(current->text)  size += (strlen((decompr) ? decompress(current->text):current->text) + 1);
			}
		     }
		     break;
		case TYPE_THING:
		     if(data) {
			size += sizeof(struct db_thing_data);
			if(data->thing.desc)     size += (strlen((decompr) ? decompress(data->thing.desc):data->thing.desc) + 1);
			if(data->thing.drop)     size += (strlen((decompr) ? decompress(data->thing.drop):data->thing.drop) + 1);
			if(data->thing.succ)     size += (strlen((decompr) ? decompress(data->thing.succ):data->thing.succ) + 1);
			if(data->thing.fail)     size += (strlen((decompr) ? decompress(data->thing.fail):data->thing.fail) + 1);
			if(data->thing.odrop)    size += (strlen((decompr) ? decompress(data->thing.odrop):data->thing.odrop) + 1);
			if(data->thing.osucc)    size += (strlen((decompr) ? decompress(data->thing.osucc):data->thing.osucc) + 1);
			if(data->thing.ofail)    size += (strlen((decompr) ? decompress(data->thing.ofail):data->thing.ofail) + 1);
			if(data->thing.odesc)    size += (strlen((decompr) ? decompress(data->thing.odesc):data->thing.odesc) + 1);
			if(data->thing.areaname) size += (strlen((decompr) ? decompress(data->thing.areaname):data->thing.areaname) + 1);
			if(data->thing.cstring)  size += (strlen((decompr) ? decompress(data->thing.cstring):data->thing.cstring) + 1);
			if(data->thing.estring)  size += (strlen((decompr) ? decompress(data->thing.estring):data->thing.estring) + 1);
			if(data->thing.lock)     size += getsizeboolexp(data->thing.lock);
			if(data->thing.lock_key) size += getsizeboolexp(data->thing.lock_key);
		     }
		     break;
		case TYPE_EXIT:
		     if(data) {
			size += sizeof(struct db_exit_data);
			if(data->exit.desc)  size += (strlen((decompr) ? decompress(data->exit.desc):data->exit.desc) + 1);
			if(data->exit.drop)  size += (strlen((decompr) ? decompress(data->exit.drop):data->exit.drop) + 1);
			if(data->exit.succ)  size += (strlen((decompr) ? decompress(data->exit.succ):data->exit.succ) + 1);
			if(data->exit.fail)  size += (strlen((decompr) ? decompress(data->exit.fail):data->exit.fail) + 1);
			if(data->exit.odrop) size += (strlen((decompr) ? decompress(data->exit.odrop):data->exit.odrop) + 1);
			if(data->exit.osucc) size += (strlen((decompr) ? decompress(data->exit.osucc):data->exit.osucc) + 1);
			if(data->exit.ofail) size += (strlen((decompr) ? decompress(data->exit.ofail):data->exit.ofail) + 1);
			if(data->exit.lock)  size += getsizeboolexp(data->exit.lock);
		     }
		     break;
		case TYPE_FUSE:
		     if(data) {
			size += sizeof(struct db_fuse_data);
			if(data->fuse.desc) size += (strlen((decompr) ? decompress(data->fuse.desc):data->fuse.desc) + 1);
			if(data->fuse.drop) size += (strlen((decompr) ? decompress(data->fuse.drop):data->fuse.drop) + 1);
			if(data->fuse.lock) size += getsizeboolexp(data->fuse.lock);
		     }
		     break;
		case TYPE_ROOM:
		     if(data) {
			size += sizeof(struct db_room_data);
			if(data->room.desc)     size += (strlen((decompr) ? decompress(data->room.desc):data->room.desc) + 1);
			if(data->room.drop)     size += (strlen((decompr) ? decompress(data->room.drop):data->room.drop) + 1);
			if(data->room.succ)     size += (strlen((decompr) ? decompress(data->room.succ):data->room.succ) + 1);
			if(data->room.fail)     size += (strlen((decompr) ? decompress(data->room.fail):data->room.fail) + 1);
			if(data->room.odrop)    size += (strlen((decompr) ? decompress(data->room.odrop):data->room.odrop) + 1);
			if(data->room.osucc)    size += (strlen((decompr) ? decompress(data->room.osucc):data->room.osucc) + 1);
			if(data->room.ofail)    size += (strlen((decompr) ? decompress(data->room.ofail):data->room.ofail) + 1);
			if(data->room.odesc)    size += (strlen((decompr) ? decompress(data->room.odesc):data->room.odesc) + 1);
			if(data->room.areaname) size += (strlen((decompr) ? decompress(data->room.areaname):data->room.areaname) + 1);
			if(data->room.cstring)  size += (strlen((decompr) ? decompress(data->room.cstring):data->room.cstring) + 1);
			if(data->room.estring)  size += (strlen((decompr) ? decompress(data->room.estring):data->room.estring) + 1);
			if(data->room.lock)     size += getsizeboolexp(data->room.lock);
		     }
		     break;
	 }
	 return(size);
}

/* ---->  Get DBREF of first object in given LIST of OBJECT  <---- */
dbref getfirst(dbref object,int list,dbref *cobj)
{
      static dbref ptr;

      ptr = NOTHING;
      while((object != NOTHING) && (ptr == NOTHING)) {
            *cobj = object;
            switch(list) {
                   case VARIABLES:
                        if(HasList(object,VARIABLES))
                           ptr = db[object].variables;
                        break;
                   case COMMANDS:
                        if(HasList(object,COMMANDS))
                           ptr = db[object].commands;
                        break;
                   case CONTENTS:
                        if(HasList(object,CONTENTS))
                           ptr = db[object].contents;
                        break;
                   case EXITS:
                        if(HasList(object,EXITS))
                           ptr = db[object].exits;
                        break;
                   case FUSES:
                        if(HasList(object,FUSES))
                           ptr = db[object].fuses;
                        break;
                   default:
                        break;
	    }

            /* ---->  Handle inheritance  <---- */
            object = db[object].parent;
      }
      if(ptr == NOTHING) *cobj = object;
      return(ptr);
}

/* ---->  Return type of object as string  <---- */
const char *object_type(dbref object,int progressive)
{
      switch(Typeof(object)) {
             case TYPE_PROPERTY:
                  return((progressive) ? "a property":"property");
                  break;
             case TYPE_VARIABLE:
                  return((progressive) ? "a variable":"variable");
                  break;
             case TYPE_COMMAND:
                  return((progressive) ? "a compound command":"compound command");
                  break;
             case TYPE_CHARACTER:
                  return((progressive) ? "a character":"character");
                  break;
             case TYPE_ALARM:
                  return((progressive) ? "an alarm":"alarm");
                  break;
             case TYPE_ARRAY:
                  return((progressive) ? "a dynamic array":"dynamic array");
                  break;
             case TYPE_THING:
                  return((progressive) ? "a thing":"thing");
                  break;
             case TYPE_EXIT:
                  return((progressive) ? "an exit":"exit");
                  break;
             case TYPE_FUSE:
                  return((progressive) ? "a fuse":"fuse");
                  break;
             case TYPE_ROOM:
                  return((progressive) ? "a room":"room");
                  break;
             case TYPE_FREE:
                  return((progressive) ? "a free object":"free object");
                  break;
             default:
                  return((progressive) ? "an unknown type of object":"unknown type of object");
                  break;
      }
}

/* ---->  Return type of linked list as string  <---- */
const char *list_type(int list)
{
      switch(list) {
             case VARIABLES:
                  return("variables");
                  break;
             case COMMANDS:
                  return("compound commands");
                  break;
             case CONTENTS:
                  return("contents");
                  break;
             case EXITS:
                  return("exits");
                  break;
             case FUSES:
                  return("fuses");
                  break;
             default:
                  return("<UNKNOWN TYPE OF LIST>");
                  break;
      }
}

/* ---->  Can given object be teleported/dropped in given destination by specified character  <---- */
int can_teleport_object(dbref player,dbref object,dbref destination)
{
    if(!Valid(object) || !Valid(destination)) return(0);
    if(!can_write_to(player,destination,0)) {
       switch(Typeof(destination)) {
              case TYPE_PROPERTY:
                   if(Level4(db[player].owner)) return((admintelprivs[Typeof(object)] & SEARCH_PROPERTY) != 0);
                      else return((mortaltelprivs[Typeof(object)] & SEARCH_PROPERTY) != 0);
                   break;
              case TYPE_VARIABLE:
                   if(Level4(db[player].owner)) return((admintelprivs[Typeof(object)] & SEARCH_VARIABLE) != 0);
                      else return((mortaltelprivs[Typeof(object)] & SEARCH_VARIABLE) != 0);
                   break;
              case TYPE_COMMAND:
                   if(Level4(db[player].owner)) return((admintelprivs[Typeof(object)] & SEARCH_COMMAND) != 0);
                      else return((mortaltelprivs[Typeof(object)] & SEARCH_COMMAND) != 0);
                   break;
              case TYPE_CHARACTER:
                   if(Level4(db[player].owner)) return((admintelprivs[Typeof(object)] & SEARCH_CHARACTER) != 0);
                      else return((mortaltelprivs[Typeof(object)] & SEARCH_CHARACTER) != 0);
                   break;
              case TYPE_ALARM:
                   if(Level4(db[player].owner)) return((admintelprivs[Typeof(object)] & SEARCH_ALARM) != 0);
                      else return((mortaltelprivs[Typeof(object)] & SEARCH_ALARM) != 0);
                   break;
              case TYPE_ARRAY:
                   if(Level4(db[player].owner)) return((admintelprivs[Typeof(object)] & SEARCH_ARRAY) != 0);
                      else return((mortaltelprivs[Typeof(object)] & SEARCH_ARRAY) != 0);
                   break;
              case TYPE_THING:
                   if(Level4(db[player].owner)) return((admintelprivs[Typeof(object)] & SEARCH_THING) != 0);
                      else return((db[player].location == destination) ? ((mortaltelprivs[Typeof(object)] & SEARCH_THING) != 0):0);
                   break;
              case TYPE_EXIT:
                   if(Level4(db[player].owner)) return((admintelprivs[Typeof(object)] & SEARCH_EXIT) != 0);
                      else return((mortaltelprivs[Typeof(object)] & SEARCH_EXIT) != 0);
                   break;
              case TYPE_FUSE:
                   if(Level4(db[player].owner)) return((admintelprivs[Typeof(object)] & SEARCH_FUSE) != 0);
                      else return((mortaltelprivs[Typeof(object)] & SEARCH_FUSE) != 0);
                   break;
              case TYPE_ROOM:
                   if(Level4(db[player].owner)) return((admintelprivs[Typeof(object)] & SEARCH_ROOM) != 0);
                      else return((db[player].location == destination) ? ((mortaltelprivs[Typeof(object)] & SEARCH_ROOM) != 0):0);
                   break;
              default:
                   return(0);
       }
       return(0);
    } else return(1);
}

/* ---->  Filter names of compound commands and exits  <---- */
void filter_comexitname(dbref object)
{
     char *p1,*p2;

     if(!(db[object].name && *db[object].name)) return;
     strcpy(objname,db[object].name);

     p1 = p2 = objname;
     while(*p2) {
           while(*p2 && (*p2 == ' ')) p2++;
           while(*p2 && (*p2 == LIST_SEPARATOR)) p2++;  /*  Strip blank entries  */
           if(p1 != objname) *p1++ = LIST_SEPARATOR;
           while(*p2 && (*p2 != LIST_SEPARATOR)) *p1++ = *p2++;
           if(p1 != objname) {
              do p1--; while((p1 != objname) && (*p1 == ' '));
              if(*p1 && (*p1 != ' ')) p1++;
	   }
     }
     *p1 = '\0';
     if(*p1) {
        FREENULL(db[object].name);
        db[object].name = (char *) alloc_string(objname);
     }
}

/* ---->  Capitalise first alphabetic letter of given string  <---- */
void capitalise(char *str)
{
     if(!(str && *str)) return;
     while(*str && !isalpha(*str)) str++;
     if(*str && islower(*str)) *str = toupper(*str);
}

/* ---->  Get lock of object (Taking inheritance into account)  <---- */
struct boolexp *getlock(dbref object,int key)
{
       static struct boolexp *lock;
       static time_t         now;

       gettime(now);
       inherited = -1, lock = TRUE_BOOLEXP;
       while((lock == TRUE_BOOLEXP) && (object != NOTHING)) {
             inherited++;
             if(!key) {
                if(HasField(object,LOCK)) {
                   switch(Typeof(object)) {
                          case TYPE_EXIT:
                               lock = db[object].data->exit.lock;
                               break;
                          case TYPE_FUSE:
                               lock = db[object].data->fuse.lock;
                               break;
                          case TYPE_ROOM:
                               lock = db[object].data->room.lock;
                               break;
                          case TYPE_THING:
                               lock = db[object].data->thing.lock;
                               break;
                          case TYPE_COMMAND:
                               lock = db[object].data->command.lock;
                               break;
                          default:
                               lock = TRUE_BOOLEXP;
		   }
                   if(!(command_type & NO_USAGE_UPDATE) && !RoomZero(Location(object))) db[object].lastused = now;
		} else lock = TRUE_BOOLEXP;
	     } else {
                lock = (Typeof(object) == TYPE_THING) ? db[object].data->thing.lock_key:TRUE_BOOLEXP;
                if(!(command_type & NO_USAGE_UPDATE) && !RoomZero(Location(object))) db[object].lastused = now;
	     }

             /* ---->  Handle inheritance  <---- */
             object = db[object].parent;
       }
       if(inherited < 0) inherited = 0;
       return(lock);
}

long getmassplayer(dbref object)
{
     static long   result;
     static time_t now;

     gettime(now);
     inherited = -1, result = INHERIT;
     while((result == INHERIT) && (object != NOTHING)) {
           inherited++;
	   result = db[object].data->player.mass;
           if(!(command_type & NO_USAGE_UPDATE) && !RoomZero(Location(object))) db[object].lastused = now;

           /* ---->  Handle inheritance  <---- */
           object = db[object].parent;
     }
     if(inherited < 0) inherited = 0;
     return((result == INHERIT) ? STANDARD_CHARACTER_MASS : result);
}

long getvolumeplayer(dbref object)
{
     static long   result;
     static time_t now;

     gettime(now);
     inherited = -1, result = INHERIT;
     while((result == INHERIT) && (object != NOTHING)) {
           inherited++;
           result = db[object].data->player.volume;
           if(!(command_type & NO_USAGE_UPDATE) && !RoomZero(Location(object))) db[object].lastused = now;

           /* ---->  Handle inheritance  <---- */
           object = db[object].parent;
     }
     if(inherited < 0) inherited = 0;
     return((result == INHERIT) ? STANDARD_CHARACTER_VOLUME : result);
}

long getmassroom(dbref object)
{
     static long   result;
     static time_t now;

     gettime(now);
     inherited = -1, result = INHERIT;
     while((result == INHERIT) && (object != NOTHING)) {
           inherited++;
           result = db[object].data->room.mass;
           if(!(command_type & NO_USAGE_UPDATE) && !RoomZero(Location(object))) db[object].lastused = now;

           /* ---->  Handle inheritance  <---- */
           object = db[object].parent;
     }
     if(inherited < 0) inherited = 0;
     return((result == INHERIT) ? STANDARD_ROOM_MASS : result);
}

long getvolumeroom(dbref object)
{
     static long   result;
     static time_t now;

     gettime(now);
     inherited = -1, result = INHERIT;
     while((result == INHERIT) && (object != NOTHING)) {
           inherited++;
           result = db[object].data->room.volume;
           if(!(command_type & NO_USAGE_UPDATE) && !RoomZero(Location(object))) db[object].lastused = now;

           /* ---->  Handle inheritance  <---- */
           object = db[object].parent;
     }
     if(inherited < 0) inherited = 0;
     return((result == INHERIT) ? STANDARD_ROOM_VOLUME : result);
}

long getmassthing(dbref object)
{
     static long   result;
     static time_t now;

     gettime(now);
     inherited = -1, result = INHERIT;
     while((result == INHERIT) && (object != NOTHING)) {
           inherited++;
           result = db[object].data->thing.mass;
           if(!(command_type & NO_USAGE_UPDATE) && !RoomZero(Location(object))) db[object].lastused = now;

           /* ---->  Handle inheritance  <---- */
           object = db[object].parent;
     }
     if(inherited < 0) inherited = 0;
     return((result == INHERIT) ? STANDARD_THING_MASS : result);
}

long getvolumething(dbref object)
{
     static long   result;
     static time_t now;

     gettime(now);
     inherited = -1, result = INHERIT;
     while((result == INHERIT) && (object != NOTHING)) {
           inherited++;
           result = db[object].data->thing.volume;
           if(!(command_type & NO_USAGE_UPDATE) && !RoomZero(Location(object))) db[object].lastused = now;

           /* ---->  Handle inheritance  <---- */
           object = db[object].parent;
     }
     if(inherited < 0) inherited = 0;
     return((result == INHERIT) ? STANDARD_THING_VOLUME : result);
}

/* ---->  Get object field (Taking inheritance into account)  <---- */
const char *getfield(dbref object,int field)
{
      static char   *ptr;
      static time_t now;

      gettime(now);
      inherited = -1, ptr = NULL;
      while((ptr == NULL) && (object != NOTHING)) {
            inherited++;
            if(HasField(object,field)) {
               switch(field) {
                      case NAME:
                           ptr = db[object].name;
                           break;
                      case DESC:
                           ptr = db[object].data->standard.desc;
                           break;
                      case DROP:
                      case EMAIL:
                           ptr = db[object].data->standard.drop;
                           break;
                      case FAIL:
                           ptr = db[object].data->standard.fail;
                           break;
                      case SUCC:
                           ptr = db[object].data->standard.succ;
                           break;
                      case ODESC:
                      case WWW:
                           ptr = db[object].data->standard.odesc;
                           break;
                      case ODROP:
                           ptr = db[object].data->standard.odrop;
                           break;
                      case OFAIL:
                           ptr = db[object].data->standard.ofail;
                           break;
                      case OSUCC:
                           ptr = db[object].data->standard.osucc;
                           break;
                      case CSTRING:
                           if(Typeof(object) == TYPE_THING) ptr = db[object].data->thing.cstring;
		              else if(Typeof(object) == TYPE_ROOM) ptr = db[object].data->room.cstring;
                           break;
                      case ESTRING:
                           if(Typeof(object) == TYPE_THING) ptr = db[object].data->thing.estring;
		              else if(Typeof(object) == TYPE_ROOM) ptr = db[object].data->room.estring;
                           break;
                      case AREANAME:
                           if(Typeof(object) == TYPE_THING) ptr = db[object].data->thing.areaname;
		              else if(Typeof(object) == TYPE_ROOM) ptr = db[object].data->room.areaname;
                           break;
                      default:
                           ptr = NULL;
	       }
               if((field != NAME) && !(command_type & NO_USAGE_UPDATE) && !RoomZero(Location(object))) db[object].lastused = now;
	    } else ptr = NULL;

            /* ---->  Handle inheritance  <---- */
            object = db[object].parent;
      }
      if(inherited < 0) inherited = 0;
      return((field == NAME) ? ptr:decompress(ptr));
}

/* ---->  Get mass or volume of object  <---- */
long get_mass_or_volume(dbref object,unsigned char volume)
{
     if(!Valid(object)) return(0);
     switch(Typeof(object)) {
            case TYPE_CHARACTER:
		return ((volume) ? getvolumeplayer(object) : getmassplayer(object));
                 break;
            case TYPE_THING:
		return ((volume) ? getvolumething(object) : getmassthing(object));
                 break;
            case TYPE_ROOM:
		return ((volume) ? getvolumeroom(object) : getmassroom(object));
                 break;
            default:
                 return((volume) ? STANDARD_VOLUME:STANDARD_MASS);
     }
}

/* ---->  Set object field  <---- */
void setfield(dbref object,int field,const char *str,unsigned char capitals)
{
     if(HasField(object,field)) {
        switch(field) {
               case NAME:
                    FREENULL(db[object].name);
                    db[object].name = (char *) alloc_string(str);
                    if(capitals) capitalise(db[object].name);
                    if((Typeof(object) == TYPE_COMMAND) || (Typeof(object) == TYPE_EXIT)) filter_comexitname(object);
                    break;
               case DESC:
                    FREENULL(db[object].data->standard.desc);
                    db[object].data->standard.desc = (char *) alloc_string(compress(str,((Typeof(object) != TYPE_COMMAND) && (Typeof(object) != TYPE_VARIABLE))));
                    break;
               case DROP:
               case EMAIL:
                    FREENULL(db[object].data->standard.drop);
                    db[object].data->standard.drop = (char *) alloc_string(compress(str,((Typeof(object) != TYPE_COMMAND) && (Typeof(object) != TYPE_VARIABLE) && (field != EMAIL))));
                    break;
               case FAIL:
                    FREENULL(db[object].data->standard.fail);
                    db[object].data->standard.fail = (char *) alloc_string(compress(str,(Typeof(object) != TYPE_VARIABLE)));
                    break;
               case SUCC:
                    FREENULL(db[object].data->standard.succ);
                    db[object].data->standard.succ = (char *) alloc_string(compress(str,(Typeof(object) != TYPE_VARIABLE)));
                    break;
               case ODESC:
               case WWW:
                    FREENULL(db[object].data->standard.odesc);
                    db[object].data->standard.odesc = (char *) alloc_string(compress(str,((Typeof(object) != TYPE_COMMAND) && (Typeof(object) != TYPE_VARIABLE) && (field != WWW))));
                    break;
               case ODROP:
                    FREENULL(db[object].data->standard.odrop);
                    db[object].data->standard.odrop = (char *) alloc_string(compress(str,0));
                    break;
               case OFAIL:
                    FREENULL(db[object].data->standard.ofail);
                    db[object].data->standard.ofail = (char *) alloc_string(compress(str,0));
                    break;
               case OSUCC:
                    FREENULL(db[object].data->standard.osucc);
                    db[object].data->standard.osucc = (char *) alloc_string(compress(str,0));
                    break;
               case CSTRING:
                    if(Typeof(object) == TYPE_THING) {
                       FREENULL(db[object].data->thing.cstring);
                       db[object].data->thing.cstring = (char *) alloc_string(compress(str,1));
		    } else if(Typeof(object) == TYPE_ROOM) {
                       FREENULL(db[object].data->room.cstring);
                       db[object].data->room.cstring = (char *) alloc_string(compress(str,1));
		    }
                    break;
               case ESTRING:
                    if(Typeof(object) == TYPE_THING) {
                       FREENULL(db[object].data->thing.estring);
                       db[object].data->thing.estring = (char *) alloc_string(compress(str,1));
		    } else if(Typeof(object) == TYPE_ROOM) {
                       FREENULL(db[object].data->room.estring);
                       db[object].data->room.estring = (char *) alloc_string(compress(str,1));
		    }
                    break;
               case AREANAME:
                    if(Typeof(object) == TYPE_THING) {
                       FREENULL(db[object].data->thing.areaname);
                       db[object].data->thing.areaname = (char *) alloc_string(compress(str,1));
		    } else if(Typeof(object) == TYPE_ROOM) {
                       FREENULL(db[object].data->room.areaname);
                       db[object].data->room.areaname = (char *) alloc_string(compress(str,1));
		    }
                    break;
               default:
                    break;
	}
        if(!(command_type & NO_USAGE_UPDATE) && !RoomZero(Location(object))) gettime(db[object].lastused);
     }
}

/* ---->  Initialise PROFILE_DATA  <---- */
void initialise_profile(struct profile_data *profile)
{
     if(!profile) return;
     profile->qualifications = NULL;
     profile->achievements   = NULL;
     profile->nationality    = NULL;
     profile->occupation     = NULL;
     profile->interests      = NULL;
     profile->sexuality      = 0;
     profile->statusirl      = 0;
     profile->statusivl      = 0;
     profile->dislikes       = NULL;
     profile->comments       = NULL;
     profile->country        = NULL;
     profile->hobbies        = NULL;
     profile->picture        = NULL;
     profile->height         = 0;
     profile->weight         = 0;
     profile->drink          = NULL;
     profile->likes          = NULL;
     profile->music          = NULL;
     profile->other          = NULL;
     profile->sport          = NULL;
     profile->city           = NULL;
     profile->eyes           = NULL;
     profile->food           = NULL;
     profile->hair           = NULL;
     profile->irl            = NULL;
     profile->dob            = UNSET_DATE;
}

/* ---->  Initialise DATA of given object  <---- */
void initialise_data(dbref object)
{
     if(db[object].data) return;
     switch(Typeof(object)) {
            case TYPE_ARRAY:
                 if((db[object].data = (union db_extra_data *) malloc(sizeof(struct db_array_data))) == (union db_extra_data *) NULL)
                    abortmemory("(initialise_data() in db.c)  Insufficient memory to allocate dynamic array extra data.");
                 db[object].data->array.start = NULL;
                 break;
            case TYPE_CHARACTER:
                 if((db[object].data = (union db_extra_data *) malloc(sizeof(struct db_player_data))) != (union db_extra_data *) NULL) {
                    time_t now;

                    gettime(now);
                    double_to_currency(&(db[object].data->player.expenditure),0);
                    double_to_currency(&(db[object].data->player.balance),DEFAULT_BALANCE);
                    double_to_currency(&(db[object].data->player.credit),DEFAULT_CREDIT);
                    double_to_currency(&(db[object].data->player.health),100);
                    double_to_currency(&(db[object].data->player.income),0);

                    db[object].data->player.shortdateformat = SHORTDATEFMT;
                    db[object].data->player.disclaimertime  = now;
                    db[object].data->player.htmlbackground  = NULL;
                    db[object].data->player.longdateformat  = LONGDATEFMT;
                    db[object].data->player.dateseparator   = DATESEPARATOR;
                    db[object].data->player.failedlogins    = 0;
                    db[object].data->player.longesttime     = 0;
                    db[object].data->player.longestdate     = now;
                    db[object].data->player.restriction     = DEFAULT_RESTRICTION;
                    db[object].data->player.subtopic_id     = 0;
                    db[object].data->player.controller      = object;
                    db[object].data->player.damagetime      = now;
                    db[object].data->player.healthtime      = now;
                    db[object].data->player.quotalimit      = 0;
                    db[object].data->player.timeformat      = TIMEFMT;
                    db[object].data->player.dateorder       = DATEORDER;
                    db[object].data->player.prefflags       = PREFS_DEFAULT;
                    db[object].data->player.totaltime       = 0;
                    db[object].data->player.scrheight       = STANDARD_CHARACTER_SCRHEIGHT;
                    db[object].data->player.maillimit       = MAIL_LIMIT_MORTAL;
                    db[object].data->player.password        = NULL;
                    db[object].data->player.timediff        = 0;
                    db[object].data->player.lasttime        = now;
                    db[object].data->player.topic_id        = 0;
                    db[object].data->player.idletime        = 0;
                    db[object].data->player.pwexpiry        = now;
                    db[object].data->player.redirect        = NOTHING;
                    db[object].data->player.aliases         = NULL;
                    db[object].data->player.bantime         = 0;
                    db[object].data->player.feeling         = 0;
                    db[object].data->player.friends         = NULL;
                    db[object].data->player.payment         = 0;
                    db[object].data->player.profile         = NULL;
                    db[object].data->player.logins          = 0;
                    db[object].data->player.volume          = STANDARD_CHARACTER_VOLUME;
                    db[object].data->player.chpid           = object;
                    db[object].data->player.quota           = 0;
                    db[object].data->player.odesc           = NULL;
                    db[object].data->player.odrop           = NULL;
                    db[object].data->player.ofail           = NULL;
                    db[object].data->player.osucc           = NULL;
                    db[object].data->player.score           = 0;
                    db[object].data->player.desc            = NULL;
                    db[object].data->player.drop            = NULL;
                    db[object].data->player.fail            = NULL;
                    db[object].data->player.mass            = STANDARD_CHARACTER_MASS;
                    db[object].data->player.lost            = 0;
                    db[object].data->player.mail            = NULL;
                    db[object].data->player.succ            = NULL;
                    db[object].data->player.afk             = AFK_TIME;
                    db[object].data->player.won             = 0;
                    db[object].data->player.uid             = object;
		 } else abortmemory("(initialise_data() in db.c)  Insufficient memory to allocate character extra data.");
                 break;
            case TYPE_PROPERTY:
                 if((db[object].data = (union db_extra_data *) malloc(sizeof(struct db_property_data))) == (union db_extra_data *) NULL)
                    abortmemory("(initialise_data() in db.c)  Insufficient memory to allocate property extra data.");
                 db[object].data->property.desc = NULL;
                 break;
            case TYPE_VARIABLE:
                 if((db[object].data = (union db_extra_data *) malloc(sizeof(struct db_variable_data))) == (union db_extra_data *) NULL)
                    abortmemory("(initialise_data() in db.c)  Insufficient memory to allocate variable extra data.");
                 db[object].data->variable.desc  = NULL;
                 db[object].data->variable.drop  = NULL;
                 db[object].data->variable.fail  = NULL;
                 db[object].data->variable.succ  = NULL;
                 db[object].data->variable.odesc = NULL;
                 db[object].data->variable.odrop = NULL;
                 db[object].data->variable.ofail = NULL;
                 db[object].data->variable.osucc = NULL;
                 break;
            case TYPE_COMMAND:
                 if((db[object].data = (union db_extra_data *) malloc(sizeof(struct db_command_data))) == (union db_extra_data *) NULL)
                    abortmemory("(initialise_data() in db.c)  Insufficient memory to allocate compound command extra data.");
                 db[object].data->command.desc  = NULL;
                 db[object].data->command.drop  = NULL;
                 db[object].data->command.fail  = NULL;
                 db[object].data->command.lock  = TRUE_BOOLEXP;
                 db[object].data->command.succ  = NULL;
                 db[object].data->command.odrop = NULL;
                 db[object].data->command.ofail = NULL;
                 db[object].data->command.osucc = NULL;
                 break;
            case TYPE_ALARM:
                 if((db[object].data = (union db_extra_data *) malloc(sizeof(struct db_alarm_data))) == (union db_extra_data *) NULL)
                    abortmemory("(initialise_data() in db.c)  Insufficient memory to allocate alarm extra data.");
                 db[object].data->alarm.desc = NULL;
                 break;
            case TYPE_THING:
                 if((db[object].data = (union db_extra_data *) malloc(sizeof(struct db_thing_data))) == (union db_extra_data *) NULL)
                    abortmemory("(initialise_data() in db.c)  Insufficient memory to allocate thing extra data.");
                 double_to_currency(&(db[object].data->thing.credit),0);
                 db[object].data->thing.areaname = NULL;
                 db[object].data->thing.lock_key = TRUE_BOOLEXP;
                 db[object].data->thing.cstring  = NULL;
                 db[object].data->thing.estring  = NULL;
                 db[object].data->thing.volume   = STANDARD_THING_VOLUME;
                 db[object].data->thing.odesc    = NULL;
                 db[object].data->thing.odrop    = NULL;
                 db[object].data->thing.ofail    = NULL;
                 db[object].data->thing.osucc    = NULL;
                 db[object].data->thing.desc     = NULL;
                 db[object].data->thing.drop     = NULL;
                 db[object].data->thing.fail     = NULL;
                 db[object].data->thing.lock     = TRUE_BOOLEXP;
                 db[object].data->thing.mass     = STANDARD_THING_MASS;
                 db[object].data->thing.succ     = NULL;
                 break;
            case TYPE_EXIT:
                 if((db[object].data = (union db_extra_data *) malloc(sizeof(struct db_exit_data))) == (union db_extra_data *) NULL)
                    abortmemory("(initialise_data() in db.c)  Insufficient memory to allocate exit extra data.");
                 db[object].data->exit.desc  = NULL;
                 db[object].data->exit.drop  = NULL;
                 db[object].data->exit.fail  = NULL;
                 db[object].data->exit.lock  = TRUE_BOOLEXP;
                 db[object].data->exit.succ  = NULL;
                 db[object].data->exit.odrop = NULL;
                 db[object].data->exit.ofail = NULL;
                 db[object].data->exit.osucc = NULL;
                 break;
            case TYPE_FUSE:
                 if((db[object].data = (union db_extra_data *) malloc(sizeof(struct db_fuse_data))) == (union db_extra_data *) NULL)
                    abortmemory("(initialise_data() in db.c)  Insufficient memory to allocate fuse extra data.");
                 db[object].data->fuse.desc = NULL;
                 db[object].data->fuse.drop = NULL;
                 db[object].data->fuse.lock = TRUE_BOOLEXP;
                 break;
            case TYPE_ROOM:
                 if((db[object].data = (union db_extra_data *) malloc(sizeof(struct db_room_data))) == (union db_extra_data *) NULL)
                    abortmemory("(initialise_data() in db.c)  Insufficient memory to allocate room extra data.");
                 double_to_currency(&(db[object].data->room.credit),0);
                 db[object].data->room.areaname = NULL;
                 db[object].data->room.cstring  = NULL;
                 db[object].data->room.estring  = NULL;
                 db[object].data->room.volume   = STANDARD_ROOM_VOLUME;
                 db[object].data->room.odesc    = NULL;
                 db[object].data->room.odrop    = NULL;
                 db[object].data->room.ofail    = NULL;
                 db[object].data->room.osucc    = NULL;
                 db[object].data->room.desc     = NULL;
                 db[object].data->room.drop     = NULL;
                 db[object].data->room.fail     = NULL;
                 db[object].data->room.lock     = TRUE_BOOLEXP;
                 db[object].data->room.mass     = STANDARD_ROOM_MASS;
                 db[object].data->room.succ     = NULL;
                 break;
            default:
                 db[object].data = NULL;
     }
}

/* ---->  Initialise Building Quota (In use) for all characters in database  <---- */
void initialise_quotas(unsigned char restart)
{
     dbref  newowner = (Validchar(maint_owner)) ? maint_owner:ROOT;
     struct array_element *ptr;
     int    count = 0;
     int    i;

     for(i = 0; i < db_top; i++) {

         /* ---->  Check object's current owner is valid  <---- */
         if((Typeof(i) != TYPE_FREE) && !Validchar(Owner(i))) {
            writelog((restart) ? SERVER_LOG:SANITY_LOG,!restart,(restart) ? "RESTART":"SANITY","Object %s(#%d)'s owner is invalid  -  Object set ASHCAN, DARK and owner set to %s(#%d).",getname(i),i,getname(newowner),newowner);
            db[i].flags |= ASHCAN|INVISIBLE;
            db[i].owner  = newowner;
	 }

         /* ---->  Add object's quota value to owner's quota in use  <---- */
         switch(Typeof(i)) {
                case TYPE_ARRAY:
                     for(ptr = db[i].data->array.start; ptr; ptr = ptr->next)
                         db[db[i].owner].data->player.quota += ELEMENT_QUOTA;
                case TYPE_PROPERTY:
                case TYPE_VARIABLE:
                case TYPE_COMMAND:
                case TYPE_ALARM:
                case TYPE_THING:
                case TYPE_EXIT:
                case TYPE_FUSE:
                case TYPE_ROOM:
                     db[db[i].owner].data->player.quota += ObjectQuota(i);
                     break;
                case TYPE_CHARACTER:
                     count++;
                     break;
                case TYPE_FREE:
                     break;
                default:
                     writelog(BUG_LOG,!restart,"BUG","(initialise_quotas() in db.c)  Type of object %s(#%d) is unknown (0x%0X.)",getname(i),i,Typeof(i));
	 }
     }
     writelog((restart) ? SERVER_LOG:ADMIN_LOG,!restart,(restart) ? "RESTART":"QUOTA","Building Quota for %d character%s %sinitialised%s",count,Plural(count),(restart) ? "":"re-",(restart) ? ".":" (Using the '@quota initialise' command.)");
}

/* ---->  Allocate string and return pointer to it  <---- */
const char *alloc_string(const char *string)
{
      char *s;

      if(Blank(string)) return(NULL);
      if((s = (char *) malloc(strlen(string) + 1)) == NULL)
         abortmemory("(alloc_string() in db.c)  Insufficient memory to allocate string.");
      strcpy(s,string);
      return(s);
}

/* ---->  Allocate string (Even if it's blank) and return pointer to it  <---- */
const char *malloc_string(const char *string)
{
      char *s;

      if(!string) return(NULL);
      if((s = (char *) malloc(strlen(string) + 1)) == NULL)
         abortmemory("(malloc_string() in db.c)  Insufficient memory to allocate string.");
      strcpy(s,string);
      return(s);
}

/* ---->  Inititalise new object  <---- */
void initialise_object(struct object *obj)
{
     time_t now;

     gettime(now);
     obj->destination = NOTHING;
     obj->variables   = NOTHING;
     obj->checksum    = 0;
     obj->contents    = NOTHING;
     obj->commands    = NOTHING;
     obj->location    = NOTHING;
     obj->lastused    = now;
     obj->created     = now;
     obj->expiry      = 0;
     obj->parent      = NOTHING;
     obj->flags2      = 0;
     obj->flags       = 0;
     obj->owner       = NOTHING;
     obj->exits       = NOTHING;
     obj->fuses       = NOTHING;
     obj->type        = TYPE_FREE;
     obj->data        = NULL;
     obj->next        = NOTHING;
     obj->name        = NULL;
}

/* ---->  Increase size of database (By DB_GROWSIZE) to make room for new objects  <---- */
static void db_grow(dbref newtop)
{
       struct object *newdb;
       int index;

       if(newtop > db_top) {

          /* ---->  If the database exists, extend it  -  Otherwise create one  <---- */
          if(db) {

             /* ---->  Extend database  <---- */
             db_top = (newtop + (DB_GROWSIZE - 1)) & (~(DB_GROWSIZE - 1));
             if((newdb = (struct object *) realloc((void *) db,db_top * sizeof(struct object))) == NULL)
                abortmemory("(db_grow() in db.c)  Insufficient memory to expand database.");
                   else {
                      memset(&newdb[newtop],0,(db_top - newtop) * sizeof(struct object));
                      db = newdb;
                      for(index = db_top - 1; index >= newtop; index--) {
                          initialise_object(db + index);
                          db[index].type = TYPE_FREE;
                          db[index].next = index + 1;
	   	      }
                      db[db_top - 1].next = NOTHING;
                      db_free_chain       = newtop;
                      db_free_chain_end   = db_top - 1;
		   }
	  } else {

             /* ---->  Make the initial database  <---- */
             db_top = newtop;
             if((db = (struct object *) malloc(db_top * sizeof(struct object))) == NULL)
                abortmemory("(db_grow() in db.c)  Insufficient memory to allocate database.");
          }
       }
}

/* ---->  Create new object and return it's DBREF  <---- */
dbref new_object()
{
      dbref newobj;

      /* ---->  Find new object  <---- */
      if(db_free_chain == NOTHING) {
         newobj = db_top;
         db_grow(db_top + 1);
      } else {
         newobj        = db_free_chain;
         db_free_chain = Next(newobj);
      }

      initialise_object(db + newobj);
      db[newobj].checksum = ++checksum;
      return(newobj);
}

/* ---->  Delete object (Mark it as free and add it to the free list)  <---- */
void delete_object(dbref object,unsigned char update,unsigned char queue)
{
     static struct destroy_data *new,*tail;
     static struct object *obj,*dest;
     static dbref  dobj;

     /* ---->  Free malloc'ed extra data (As per object type)  <---- */
     if(!Valid(object) || (Typeof(object) == TYPE_FREE)) return;
     obj = db + object;

     /* ---->  Add object to start of destroy queue  <---- */
     if(queue) {
        MALLOC(new,struct destroy_data);
        new->obj.destination = obj->destination;
        new->obj.checksum    = obj->checksum;
        new->obj.contents    = obj->contents;
        new->obj.lastused    = obj->lastused;
        new->obj.location    = obj->location;
        new->obj.created     = obj->created;
        new->obj.expiry      = obj->expiry;
        new->obj.parent      = obj->parent;
        new->obj.flags2      = obj->flags2;
        new->obj.flags       = obj->flags;
        new->obj.exits       = obj->exits;
        new->obj.owner       = obj->owner;
        new->recovered       = 0;
        new->obj.data        = obj->data;
        new->obj.name        = obj->name;
	new->obj.type        = obj->type;
        new->quota           = (Typeof(object) != TYPE_ARRAY) ? ObjectQuota(object):(ObjectQuota(object) + (array_element_count(db[object].data->array.start) * ELEMENT_QUOTA));
        new->next            = destroy_queue;
        new->prev            = NULL;
        new->id              = object;

        /* ---->  Checksums  <---- */
        new->checksum.destination = (Valid(obj->destination)) ? db[obj->destination].checksum:0;
        new->checksum.contents    = (Valid(obj->contents))    ? db[obj->contents].checksum:0;
        new->checksum.location    = (Valid(obj->location))    ? db[obj->location].checksum:0;
        new->checksum.parent      = (Valid(obj->parent))      ? db[obj->parent].checksum:0;
        new->checksum.owner       = (Validchar(obj->owner))   ? db[obj->owner].checksum:0;
        new->checksum.exits       = (Valid(obj->exits))       ? db[obj->exits].checksum:0;

        if(Typeof(object) == TYPE_CHARACTER) {
           new->checksum.controller = (Validchar(obj->data->player.controller)) ? db[obj->data->player.controller].checksum:0;
           new->checksum.redirect   = (Validchar(obj->data->player.redirect))   ? db[obj->data->player.redirect].checksum:0;
           new->checksum.uid        = (Validchar(obj->data->player.uid))        ? db[obj->data->player.uid].checksum:0;
	} else new->checksum.controller = new->checksum.redirect = new->checksum.uid = 0;

        /* ---->  Add to queue  <---- */
        if(!destroy_queue_tail) destroy_queue_tail  = new;
        if(destroy_queue)       destroy_queue->prev = new;
        destroy_queue                               = new;
        destroy_queue_size++;
     }

     /* ---->  Destroy object on end of destroy queue (If size of destroy queue > DESTROY_QUEUE_SIZE)  <---- */
     while(!queue || (destroy_queue && destroy_queue_tail && (destroy_queue_size > DESTROY_QUEUE_SIZE))) {
           if(queue) {
              dest = &(destroy_queue_tail->obj);
              dobj = destroy_queue_tail->id;
              tail = destroy_queue_tail;
              destroy_queue_tail = destroy_queue_tail->prev;
              if(!destroy_queue_tail) destroy_queue = NULL;
                 else destroy_queue_tail->next = NULL;
              if(tail) FREENULL(tail);
              destroy_queue_size--;
	   } else dest = obj, dobj = object, queue = 1;

           switch(dest->type) {
                  case TYPE_ROOM:
                       if(dest->data) {
                          FREENULL(dest->data->room.cstring);
                          FREENULL(dest->data->room.estring);
                          FREENULL(dest->data->room.areaname);
                          if(dest->data->room.lock) free_boolexp(&(dest->data->room.lock));
		       }
                       break;
                  case TYPE_THING:
                       if(dest->data) {
                          FREENULL(dest->data->thing.cstring);
                          FREENULL(dest->data->thing.estring);
                          FREENULL(dest->data->thing.areaname);
                          if(dest->data->thing.lock) free_boolexp(&(dest->data->thing.lock));
                          if(dest->data->thing.lock_key) free_boolexp(&(dest->data->thing.lock_key));
		       }
                       break;
                  case TYPE_CHARACTER:
                       if(dest->data) {

                          /* ---->  Reset partner  <---- */
                          if(((dest->flags & MARRIED) || (dest->flags & ENGAGED)) && Validchar(dest->data->player.controller))
                             db[dest->data->player.controller].flags &= ~(MARRIED|ENGAGED);
                          FREENULL(dest->data->player.password);

                          /* ---->  Free character's list of aliases  <---- */
                          if(dest->data->player.aliases) {
                             struct alias_data *ptr = dest->data->player.aliases,*ptr2;

                             ptr->prev->next = NULL;
                             while(ptr) {
                                   dest->data->player.aliases = dest->data->player.aliases->next;
                                   for(ptr2 = ptr->next; ptr2 && (ptr2->id != ptr->id); ptr2 = ptr2->next);
                                   if(!ptr2) FREENULL(ptr->command);
                                   FREENULL(ptr->alias);
                                   FREENULL(ptr);
                                   ptr = dest->data->player.aliases;
			     }
			  }
 
                          /* ---->  Free character's list of friends  <---- */
                          if(dest->data->player.friends) {
                             struct friend_data *ptr = dest->data->player.friends;

                             while(ptr) {
                                   dest->data->player.friends = dest->data->player.friends->next;
                                   FREENULL(ptr);
                                   ptr = dest->data->player.friends;
			     }
			  }

                          /* ---->  Free character's mail  <---- */
                          if(dest->data->player.mail) {
                             struct   mail_data *ptr,*mail = dest->data->player.mail;
                             unsigned char group;
                             dbref    i;

                             while(mail) {
                                   dest->data->player.mail = mail->next;

                                   /* ---->  Handle group mail  <---- */
                                   if(mail->flags & MAIL_MULTI) {
                                      if(mail->flags & MAIL_MULTI_ROOT) {
                                         for(i = 0, group = 0; (i < db_top) && !group; i++)
                                             if(Typeof(i) == TYPE_CHARACTER)
                                                for(ptr = db[i].data->player.mail; ptr && !group; ptr = ptr->next)
                                                    if((ptr->flags & MAIL_MULTI) && (ptr->id == mail->id)) {
                                                       ptr->flags |= MAIL_MULTI_ROOT;
                                                       group       = 1;
						    }

                                         if(!group) {
                                            FREENULL(mail->redirect);
                                            FREENULL(mail->subject);
                                            FREENULL(mail->message);
                                            FREENULL(mail->sender);
					 }
				      }
				   } else {
                                      FREENULL(mail->redirect);
                                      FREENULL(mail->subject);
                                      FREENULL(mail->message);
                                      FREENULL(mail->sender);
				   }
                                   FREENULL(mail);
                                   mail = dest->data->player.mail;
			     }
			  }

                          /* ---->  Free character's profile  <---- */
                          if(dest->data->player.profile) {
                             FREENULL(dest->data->player.profile->qualifications);
                             FREENULL(dest->data->player.profile->achievements);
                             FREENULL(dest->data->player.profile->nationality);
                             FREENULL(dest->data->player.profile->occupation);
                             FREENULL(dest->data->player.profile->interests);
                             FREENULL(dest->data->player.profile->comments);
                             FREENULL(dest->data->player.profile->country);
                             FREENULL(dest->data->player.profile->hobbies);
                             FREENULL(dest->data->player.profile->drink);
                             FREENULL(dest->data->player.profile->music);
                             FREENULL(dest->data->player.profile->other);
                             FREENULL(dest->data->player.profile->sport);
                             FREENULL(dest->data->player.profile->city);
                             FREENULL(dest->data->player.profile->eyes);
                             FREENULL(dest->data->player.profile->food);
                             FREENULL(dest->data->player.profile->hair);
                             FREENULL(dest->data->player.profile->irl);
                             FREENULL(dest->data->player.profile);
			  }
		       }
                       break;
                  case TYPE_ARRAY:
                       if(dest->data && dest->data->array.start) {
                          struct array_element *ptr = dest->data->array.start;
                          struct grp_data *chk;

                          while(ptr) {
                                dest->data->array.start = dest->data->array.start->next;
                                for(chk = grp; chk; chk = chk->next)
                                    if(ptr == &chk->nunion->element)
                                       chk->nunion = NULL;
                                FREENULL(ptr->index);
                                FREENULL(ptr->text);
                                FREENULL(ptr);
                                ptr = dest->data->array.start;
			  }
		       }
                       break;
                  case TYPE_PROPERTY:
                  case TYPE_VARIABLE:
                  case TYPE_ALARM:
                  case TYPE_FREE:
                       break;
                  case TYPE_COMMAND:
	    	       if(dest->data && dest->data->command.lock) free_boolexp(&(dest->data->command.lock));
                       break;
                  case TYPE_EXIT:
	               if(dest->data && dest->data->exit.lock) free_boolexp(&(dest->data->exit.lock));
                       break;
                  case TYPE_FUSE:
		       if(dest->data && dest->data->fuse.lock) free_boolexp(&(dest->data->fuse.lock));
                       break;
                  default:
                       writelog(BUG_LOG,1,"BUG","(delete_object() in db.c)  Tried to delete unknown (0x%0X) object %s(#%d)  -  Object set ASHCAN.",dest->type,String(dest->name),dobj);
                       dest->flags |= ASHCAN;
	   }

           /* ---->  Free object's fields  <---- */
           FREENULL(/* (char *) */ dest->name);
	if (dest->data) {
           if((Fields(dest->type) & DESC)  != 0) FREENULL(dest->data->standard.desc);
           if((Fields(dest->type) & DROP)  != 0) FREENULL(dest->data->standard.drop);
           if((Fields(dest->type) & FAIL)  != 0) FREENULL(dest->data->standard.fail);
           if((Fields(dest->type) & SUCC)  != 0) FREENULL(dest->data->standard.succ);
           if((Fields(dest->type) & ODESC) != 0) FREENULL(dest->data->standard.odesc);
           if((Fields(dest->type) & ODROP) != 0) FREENULL(dest->data->standard.odrop);
           if((Fields(dest->type) & OFAIL) != 0) FREENULL(dest->data->standard.ofail);
           if((Fields(dest->type) & OSUCC) != 0) FREENULL(dest->data->standard.osucc);
           FREENULL(dest->data);
	}
     }

     /* ---->  Remove object from physical database and add to free chain  <---- */
     initialise_object(obj);
     obj->type = TYPE_FREE;

     if(!Valid(db_free_chain)) {
        db_free_chain     = object;
        db_free_chain_end = object;
     } else {
        (db + db_free_chain_end)->next = object;
        db_free_chain_end              = object;
     }
     obj->next = NOTHING;
     if(update) stats_tcz_update_record(0,0,0,0,1,0,0);
}

/* ---->  Create new (Empty) database  <---- */
void db_create(void)
{
     char  description[BUFFER_LEN];
     char  name[BUFFER_LEN];
     dbref room,character;

     writelog(SERVER_LOG,0,"RESTART","Generating new database...");

     /* ---->  (#0)  The Junkpile  <---- */
     room = create_room(ROOT,NULL,NULL,"The Junkpile","Unwanted objects end up in this location when junked using the '%g%l%ujunk%x' command (See '%g%l%ujunk%x'.)",0,1);
     if(Valid(room) && (room == ROOMZERO)) {
	db[room].flags  |=  (ABODE|QUIET|HAVEN|INVISIBLE);
	db[room].flags2 |=  SECURE;
	db[room].flags2 &= ~(WARP|VISIT);
	db[room].data->room.mass   = TCZ_INFINITY;
	db[room].data->room.volume = TCZ_INFINITY;
	writelog(SERVER_LOG,0,"RESTART","Created room #%d (The Junkpile.)",room);
     } else {
        writelog(SERVER_LOG,0,"RESTART","Unable to create required room #%d (The Junkpile.)",ROOMZERO);
        exit(1);
     }

     /* ---->  (#1)  Admin. character (Root, password 'admin'.)  <---- */
     character = create_new_character("Root","admin",0);
     if(Validchar(character) && (character == ROOT)) {
        writelog(SERVER_LOG,0,"RESTART","Created character #%d (Root, password 'admin'.)",room);
     } else {
        writelog(SERVER_LOG,0,"RESTART","Unable to create required character #%d (Root.)",ROOT);
        exit(1);
     }

     /* ---->  (#2)  Top Area  <---- */
     sprintf(description,"A starting point for the virtual world of %s (Place your newly created %%c%%lglobal areas%%x in this location.)  %%r%%lDON'T%%x place global compound commands in here!  They must go in %%y%%lGlobal Commands%%x (%%y%%l#4%%x) for efficient lookup.",tcz_full_name);
     room = create_room(ROOT,NULL,NULL,"Top Area",description,0,1);
     if(Valid(room) && (room == 2)) {
	db[room].flags  |=  HAVEN;
	db[room].flags2 |=  SECRET;
	db[room].flags2 &= ~(WARP|FINANCE|TRANSPORT|VISIT);
	writelog(SERVER_LOG,0,"RESTART","Created room #%d (Top Area.)",room);
     } else {
        writelog(SERVER_LOG,0,"RESTART","Unable to create required room #2 (Top Area.)");
        exit(1);
     }

     /* ---->  (#3)  New Character Start  <---- */
     sprintf(name,"Welcome to the virtual world of %s!",tcz_full_name);
     sprintf(description,"Welcome to the virtual world of %s, %%n.\n\nFor more information, please type '%%g%%l%%uhelp%%x'.\nAlso, see '%%g%%l%%unewbie%%x' (Tutorial for new users.)\n\nType '%%g%%l%%uhome%%x' to go to your home room.",tcz_short_name);
     room = create_room(ROOT,NULL,NULL,name,description,0,1);
     if(Valid(room) && (room == START_LOCATION)) {
	db[room].flags  |=  HAVEN;
	db[room].flags2 &= ~(WARP|FINANCE|TRANSPORT|VISIT);
	writelog(SERVER_LOG,0,"RESTART","Created room #%d (New Character Start.)",room);
     } else {
	writelog(SERVER_LOG,0,"RESTART","Unable to create required room #%d (New Character Start.)",START_LOCATION);
        exit(1);
     }


     /* ---->  (#4)  Global Commands  <---- */
     sprintf(description,"Place %%c%%lglobal compound commands%%x in this room.  They will be accessible from anywhere on %s.",tcz_short_name);
     room = create_room(ROOT,NULL,NULL,"Global Commands",description,0,1);
     if(Valid(room) && (room == GLOBAL_COMMANDS)) {
	db[room].flags  |=  HAVEN;
	db[room].flags2 |=  SECRET;
	db[room].flags2 &= ~(WARP|FINANCE|TRANSPORT|VISIT);
	writelog(SERVER_LOG,0,"RESTART","Created room #%d (Global Commands.)",room);
     } else {
	writelog(SERVER_LOG,0,"RESTART","Unable to create required room #%d (Global Commands.)",GLOBAL_COMMANDS);
        exit(1);
     }

     /* ---->  Bulletin Board System (BBS)  <---- */
     sprintf(name,"%s BBS",tcz_full_name);
     sprintf(description,"You may access %%y%%l%s BBS%%x in this room (See '%%g%%l%%ubbs%%x' for further details.)",tcz_short_name);
     room = create_room(ROOT,NULL,NULL,name,description,0,1);
     if(Valid(room)) {
        db[room].flags  |=  (QUIET|HAVEN|INVISIBLE);
        db[room].flags2 &= ~(WARP|FINANCE|TRANSPORT|VISIT);
        bbsroom          =  room;
        writelog(SERVER_LOG,0,"RESTART","Created room #%d (%s BBS.)",room,tcz_full_name);
     } else writelog(SERVER_LOG,0,"RESTART","Unable to create room '%s BBS'.",tcz_full_name);

     /* ---->  Bank  <---- */
     sprintf(name,"The Bank of %s",tcz_short_name);
     sprintf(description,"You may perform transactions with %%y%%lThe Bank of %s%%x in this room (See '%%g%%l%%ubank%%x' for further details.)",tcz_short_name);
     room = create_room(ROOT,NULL,NULL,name,description,0,1);
     if(Valid(room)) {
	db[room].flags  |=  HAVEN;
	db[room].flags2 &= ~(WARP|FINANCE|TRANSPORT|VISIT);
	bankroom         =  room;
	writelog(SERVER_LOG,0,"RESTART","Created room #%d (The Bank of %s.)",room,tcz_short_name);
     } else writelog(SERVER_LOG,0,"RESTART","Unable to create room 'The Bank of %s'.",tcz_short_name);

     /* ---->  Post Office (Mail room)  <---- */
     sprintf(name,"%s Post Office",tcz_short_name);
     sprintf(description,"You may read and send %s mail in this room (See '%%g%%l%%umail%%x' for further details.)",tcz_short_name);
     room = create_room(ROOT,NULL,NULL,name,description,0,1);
     if(Valid(room)) {
	db[room].flags  |=  (QUIET|HAVEN);
	db[room].flags2 &= ~(WARP|FINANCE|TRANSPORT|VISIT);
	mailroom         =  room;
	writelog(SERVER_LOG,0,"RESTART","Created room #%d (%s Post Office.)",room,tcz_short_name);
     } else writelog(SERVER_LOG,0,"RESTART","Unable to create room '%s Post Office'.",tcz_short_name);

     /* ---->  Home Rooms  <---- */
     room = create_room(ROOT,NULL,NULL,"Home Rooms","User home rooms are stored in this room (See '%g%l%uhome%x' and '%g%l%u@homeroom%x' for further details.)",0,1);
     if(Valid(room)) {
        db[room].flags  |=  YELL;
        db[room].flags2 &= ~(WARP|FINANCE|TRANSPORT|VISIT|ABODE);
        homerooms        =  room;
        writelog(SERVER_LOG,0,"RESTART","Created room #%d (Home Rooms.)",room);
     } else writelog(SERVER_LOG,0,"RESTART","Unable to create room 'Home Rooms'.");

     /* ---->  Minimal database created  <---- */
     writelog(SERVER_LOG,0,"RESTART","Generation of new database complete.");
}

/* ---->  Write DBref to database file  <---- */
static void db_write_dbref(FILE *f,dbref value)
{
#ifdef DATABASE_DUMP
       if(!dumperror) {
          sprintf(writebuffer,"%d%c",value,FIELD_SEPARATOR);
          if(fputs(writebuffer,f) == EOF) dumperror = 1;
          dumpsize   = strlen(writebuffer);
          dumpdata  += dumpsize;
          totaldata += dumpsize;
          if(log_stderr) progress_meter(totaldata,MB,PROGRESS_UNITS,1);
       }
#else
       return;
#endif
}

/* ---->  Write integer value to database file  <---- */
static void db_write_int(FILE *f,int value)
{
#ifdef DATABASE_DUMP
       if(!dumperror) {
          sprintf(writebuffer,"%d%c",value,FIELD_SEPARATOR);
          if(fputs(writebuffer,f) == EOF) dumperror = 1;
          dumpsize   = strlen(writebuffer);
          dumpdata  += dumpsize;
          totaldata += dumpsize;
          if(log_stderr) progress_meter(totaldata,MB,PROGRESS_UNITS,1);
       }
#else
       return;
#endif
}

/* ---->  Write long value to database file  <---- */
static void db_write_long(FILE *f,long value)
{
#ifdef DATABASE_DUMP
       if(!dumperror) {
          sprintf(writebuffer,"%ld%c",value,FIELD_SEPARATOR);
          if(fputs(writebuffer,f) == EOF) dumperror = 1;
          dumpsize   = strlen(writebuffer);
          dumpdata  += dumpsize;
          totaldata += dumpsize;
          if(log_stderr) progress_meter(totaldata,MB,PROGRESS_UNITS,1);
       }
#else
       return;
#endif
}

/* ---->  Write double value (Floating point) to database file  <---- */
/*
static void db_write_double(FILE *f,double value)
{
#ifdef DATABASE_DUMP
       if(!dumperror) {
          sprintf(writebuffer,"%f%c",value,FIELD_SEPARATOR);
          if(fputs(writebuffer,f) == EOF) dumperror = 1;
          dumpsize   = strlen(writebuffer);
          dumpdata  += dumpsize;
          totaldata += dumpsize;
          if(log_stderr) progress_meter(totaldata,MB,PROGRESS_UNITS,1);
       }
#else
       return;
#endif
}
*/

/* ---->  Write currency value (struct currency_data) to database file  <---- */
static void db_write_currency(FILE *f,struct currency_data currency)
{
#ifdef DATABASE_DUMP
       db_write_long(f,currency.decimal);
       db_write_int(f,currency.fraction);
#else
       return;
#endif
}

/* ---->  Write string to database file  <---- */
static void db_write_string(FILE *f,const char *str)
{
#ifdef DATABASE_DUMP
       if(!dumperror) {
          if(!Blank(str)) {
             if(fputs(str,f) == EOF) dumperror = 1;
             dumpsize   = strlen(str) + 1;
             dumpdata  += dumpsize;
             totaldata += dumpsize;
	  } else dumpdata++, totaldata++;
          if(log_stderr) progress_meter(totaldata,MB,PROGRESS_UNITS,1);
          if(putc(FIELD_SEPARATOR,f) == EOF) dumperror = 1;
       }
#else
       return;
#endif
}

/* ---->  Write statistics array entry to database file  <---- */
static void db_write_statent(FILE *f,int pos)
{
#ifdef DATABASE_DUMP
       if(!dumperror) {
          db_write_long(f,stats[pos].charconnected);
          db_write_long(f,stats[pos].objdestroyed);
          db_write_long(f,stats[pos].charcreated);
          db_write_long(f,stats[pos].objcreated);
          db_write_long(f,stats[pos].shutdowns);
          db_write_long(f,stats[pos].time);
          db_write_long(f,stats[pos].peak);
       }
#else
       return;
#endif
}

/* ---->  Write TCZ statistics to database file  <---- */
static void db_write_stats(FILE *f)
{
#ifdef DATABASE_DUMP
       int    loop;
       time_t now;

       /* ---->  Write daily stats  <---- */
       db_write_int(f,stat_ptr);
       for(loop = 0; !dumperror && (loop <= stat_ptr); loop++) db_write_statent(f,loop);

       /* ---->  Write max/total/days  <---- */
       db_write_statent(f,STAT_MAX);
       db_write_statent(f,STAT_TOTAL);
       db_write_long(f,stat_days);

       /* ---->  Write database creation date and accumulated uptime  <---- */
       gettime(now);
       if((now - uptime) > db_longest_uptime)
          db_longest_uptime = (now - uptime);
       db_write_int(f,db_accumulated_restarts + 1);
       db_write_int(f,db_longest_uptime);
       db_write_int(f,db_accumulated_uptime + (now - uptime));
       db_write_int(f,db_creation_date);
       db_write_int(f,db_longest_date);
#else
       return;
#endif
}

/* ---->  Write dynamic array to database file  <---- */
static void db_write_array(FILE *f,struct array_element *start)
{
#ifdef DATABASE_DUMP
       struct array_element *ptr;
       long   elements = 0;

       /* ---->  Count number of elements in Dynamic Array  <---- */
       for(ptr = start; ptr; ptr = ptr->next) elements++;
       db_write_long(f,elements);

       /* ---->  Write each element to the database file  <---- */
       for(ptr = start; !dumperror && ptr; ptr = ptr->next) {
           db_write_string(f,ptr->index);
           db_write_string(f,decompress(ptr->text));
       }
#else
       return;
#endif
}

/* ---->  Write command aliases to database file  <---- */
static void db_write_aliases(FILE *f,struct alias_data *alias)
{
#ifdef DATABASE_DUMP
       struct alias_data *ptr;
       int    count = 0;
 
       for(ptr = alias; ptr; ptr = (ptr->next == alias) ? NULL:ptr->next, count++);
       db_write_int(f,count);

       for(ptr = alias; ptr && !dumperror; ptr = (ptr->next == alias) ? NULL:ptr->next) {
           db_write_string(f,ptr->alias);
           db_write_string(f,decompress(ptr->command));
       }
#else
       return;
#endif
}

/* ---->  Write list of character names (struct list_data) to database file  <---- */
/*
static void db_write_list(FILE *f,struct list_data *list)
{
#ifdef DATABASE_DUMP
       struct list_data *ptr = list;
       int    count = 0;
 
       for(; ptr; ptr = ptr->next, count++);
       db_write_int(f,count);

       for(; !dumperror && list; list = list->next) db_write_dbref(f,list->player);
#else
       return;
#endif
}
*/

/* ---->  Write BBS topic to database file  <---- */
static void db_write_bbs(FILE *f,struct bbs_topic_data *topic)
{
#ifdef DATABASE_DUMP
       struct bbs_message_data *messages;
       struct bbs_reader_data *readers;
       struct bbs_topic_data *subtopic;
       int    count = 0,subcount;

       if(topic) {

          /* ---->  Topic data  <---- */
          db_write_int(f,topic->topic_id);
          db_write_int(f,topic->accesslevel);
          db_write_string(f,decompress(topic->desc));
          db_write_string(f,topic->name);
          db_write_dbref(f,topic->owner);
          db_write_int(f,topic->flags);
          db_write_int(f,topic->timelimit);
          db_write_int(f,topic->messagelimit);
          db_write_int(f,topic->subtopiclimit);

          /* ---->  Sub-topics  <---- */
          for(subtopic = topic->subtopics; !dumperror && subtopic; subtopic = subtopic->next)
              db_write_bbs(f,subtopic);
          db_write_int(f,0);

          /* ---->  Messages  <---- */
          for(count = 0, messages = topic->messages; messages; messages = messages->next, count++);
          db_write_int(f,count);
          for(messages = topic->messages; !dumperror && messages; messages = messages->next) {

              /* ---->  Reader list  <---- */
              for(subcount = 0, readers = messages->readers; readers; readers = readers->next, subcount++);
              db_write_int(f,subcount);
              for(readers = messages->readers; !dumperror && readers; readers = readers->next) {
                  db_write_dbref(f,readers->reader);
                  db_write_int(f,readers->flags);
	      }

              /* ---->  Message data  <---- */
              db_write_string(f,decompress(messages->message));
              db_write_string(f,decompress(messages->subject));
              db_write_string(f,decompress(messages->name));
              db_write_int(f,messages->readercount);
              db_write_int(f,messages->flags);
              db_write_dbref(f,messages->owner);
              db_write_int(f,messages->date);
              db_write_long(f,messages->lastread);
              db_write_int(f,messages->id);
              db_write_int(f,messages->expiry);
	  }
       }
#else
       return;
#endif
}

/* ---->  Write friends list to database file  <---- */
static void db_write_friends(FILE *f,struct friend_data *friends)
{           
#ifdef DATABASE_DUMP
       struct friend_data *ptr = friends;
       int    count = 0;
 
       for(; ptr; ptr = ptr->next, count++);
       db_write_int(f,count);

       for(; !dumperror && friends; friends = friends->next) {
           db_write_dbref(f,friends->friend);
           db_write_int(f,friends->flags);
       }
#else
       return;
#endif
}

/* ---->  Write mailbox to database file  <---- */
static void db_write_mail(FILE *f,struct mail_data *mail)
{
#ifdef DATABASE_DUMP
       struct mail_data *ptr = mail;
       int    count = 0;
 
       for(; ptr; ptr = ptr->next, count++);

       db_write_int(f,count);
       for(; !dumperror && mail; mail = mail->next) {
           db_write_long(f,mail->lastread);
           db_write_int(f,mail->flags & ~MAIL_MULTI_ROOT);
           db_write_int(f,mail->date);
           db_write_dbref(f,mail->who);
           db_write_long(f,mail->id);

           db_write_string(f,decompress(mail->redirect));
           db_write_string(f,decompress(mail->subject));
           db_write_string(f,decompress(mail->message));
           db_write_string(f,decompress(mail->sender));
       }
#else
       return;
#endif
}

/* ---->  Write profile to database file  <---- */
static void db_write_profile(FILE *f,struct profile_data *profile)
{
#ifdef DATABASE_DUMP
       if(hasprofile(profile)) {
          db_write_int(f,1);
          db_write_string(f,decompress(profile->qualifications));
          db_write_string(f,decompress(profile->achievements));
          db_write_string(f,decompress(profile->nationality));
          db_write_string(f,decompress(profile->occupation));
          db_write_string(f,decompress(profile->interests));
          db_write_int(f,profile->sexuality);
          db_write_string(f,decompress(profile->comments));
          db_write_string(f,decompress(profile->country));
          db_write_string(f,decompress(profile->hobbies));
          db_write_int(f,profile->height);
          db_write_int(f,profile->weight);
          db_write_string(f,decompress(profile->drink));
          db_write_string(f,decompress(profile->music));
          db_write_string(f,decompress(profile->other));
          db_write_string(f,decompress(profile->sport));
          db_write_string(f,decompress(profile->city));
          db_write_string(f,decompress(profile->eyes));
          db_write_string(f,decompress(profile->food));
          db_write_string(f,decompress(profile->hair));
          db_write_string(f,decompress(profile->irl));
          db_write_string(f,decompress(profile->likes));
          db_write_string(f,decompress(profile->dislikes));
          db_write_int(f,profile->statusirl);
          db_write_int(f,profile->statusivl);
          db_write_long(f,profile->dob);
          db_write_string(f,decompress(profile->picture));          
       } else {
          if(profile) FREENULL(profile);
          db_write_int(f,0);
       }
#else
       return;
#endif
}

/* ---->  Write requests for new characters to database file  <---- */
static void db_write_requests(FILE *f,struct request_data *request)
{
#ifdef DATABASE_DUMP
       struct request_data *ptr;
       int    count;

       /* ---->  Count requests  <---- */
       for(ptr = request, count = 0; ptr; ptr = ptr->next, count++);
       db_write_int(f,count);

       /* ---->  Write requests to database file  <---- */
       for(; !dumperror && request; request = request->next) {
           db_write_string(f,decompress(request->email));
           db_write_long(f,request->address);
           db_write_int(f,request->date);
           db_write_dbref(f,request->user);
           db_write_int(f,request->ref);
           db_write_string(f,decompress(request->name));
       }
#else
       return;
#endif
}

/* ---->  Write sub part of a boolean expression to database file  <---- */
void db_write_subboolexp(FILE *f,struct boolexp *bool)
{
#ifdef DATABASE_DUMP
     static char temp_buf[32];

     switch(bool->type) {
            case BOOLEXP_AND:
                 strcat_limits(&exp_data,"(");
                 db_write_subboolexp(f,bool->sub1);
                 strcat_limits_char(&exp_data,AND_TOKEN);
                 db_write_subboolexp(f,bool->sub2);
                 strcat_limits(&exp_data,")");
                 break;
            case BOOLEXP_OR:
                 strcat_limits(&exp_data,"(");
                 db_write_subboolexp(f,bool->sub1);
                 strcat_limits_char(&exp_data,OR_TOKEN);
                 db_write_subboolexp(f,bool->sub2);
                 strcat_limits(&exp_data,")");
                 break;
            case BOOLEXP_NOT:
                 strcat_limits(&exp_data,"(");
                 strcat_limits_char(&exp_data,NOT_TOKEN);
                 db_write_subboolexp(f,bool->sub1);
                 strcat_limits(&exp_data,")");
                 break;
            case BOOLEXP_CONST:
                 sprintf(temp_buf,"%d",bool->object);
                 strcat_limits(&exp_data,temp_buf);
                 break;
            case BOOLEXP_FLAG:
                 strcat_limits(&exp_data,"(");
                 strcat_limits_char(&exp_data,FLAG_TOKEN);
                 sprintf(temp_buf,"%d",bool->object);
                 strcat_limits(&exp_data,temp_buf);
                 strcat_limits(&exp_data,")");
                 break;
            default:
                 break;
     }
#else
       return;
#endif
}

/* ---->  Write boolean expression to database file  <---- */
void db_write_boolexp(FILE *f,struct boolexp *bool)
{
#ifdef DATABASE_DUMP
     exp_data.dest   = boolexp_buf;
     exp_data.length = 0;

     if(bool != TRUE_BOOLEXP) {
        db_write_subboolexp(f,bool);
        *exp_data.dest = '\0';
        db_write_string(f,boolexp_buf);
     } else putc(FIELD_SEPARATOR,f);
#else
       return;
#endif
}

/* ---->  Write object to database file  <---- */
int db_write_object(FILE *f,dbref i)
{
#ifdef DATABASE_DUMP
    int    cached_commandtype = command_type;
    dbref  parent_cache;
    struct object *obj;

    /* ---->  If object is TYPE_FREE, do not write to database file  <---- */
    if(!Valid(i) || (Typeof(i) == TYPE_FREE)) return(0);
    obj = db + i, command_type |= NO_USAGE_UPDATE;

    /* ---->  Object's #ID  <---- */
    sprintf(writebuffer,"#%d",i);
    db_write_string(f,writebuffer);

    /* ---->  Database revision  <---- */
    db_write_int(f,DATABASE_REVISION);

    /* ---->  Standard object data  <---- */
    parent_cache = obj->parent;
    obj->parent  = NOTHING;

    db_write_int(f,obj->type);
    db_write_int(f,obj->flags);
    db_write_int(f,obj->flags2);
    db_write_string(f,getfield(i,NAME));
    db_write_dbref(f,parent_cache);
    db_write_dbref(f,obj->location);
    db_write_dbref(f,obj->destination);
    db_write_dbref(f,obj->contents);
    db_write_dbref(f,obj->exits);
    db_write_dbref(f,obj->next);
    db_write_dbref(f,(Typeof(i) == TYPE_CHARACTER) ? i:obj->owner);
    db_write_dbref(f,obj->commands);
    db_write_dbref(f,obj->variables);
    db_write_dbref(f,obj->fuses);
    db_write_long(f,obj->lastused);
    db_write_long(f,obj->created);
    db_write_int(f,obj->expiry);

    /* ---->  Standard object fields  <---- */
    db_write_string(f,getfield(i,DESC));
    db_write_string(f,getfield(i,SUCC));
    db_write_string(f,getfield(i,FAIL));
    db_write_string(f,getfield(i,DROP));
    db_write_string(f,getfield(i,ODESC));
    db_write_string(f,getfield(i,OSUCC));
    db_write_string(f,getfield(i,OFAIL));
    db_write_string(f,getfield(i,ODROP));

    /* ---->  Write additional fields (As per object type)  <---- */
    switch(Typeof(i)) {
           case TYPE_ROOM:
                db_write_boolexp(f,obj->data->room.lock);
                if(obj->data) {
                   db_write_int(f,1);
                   db_write_int(f,obj->data->room.mass);
                   db_write_int(f,obj->data->room.volume);
                   db_write_currency(f,obj->data->room.credit);
                   db_write_string(f,getfield(i,AREANAME));
                   db_write_string(f,getfield(i,CSTRING));
                   db_write_string(f,getfield(i,ESTRING));
                } else {
                   writelog(BUG_LOG,1,"BUG","(db_write_object() in db.c)  Room %s has NULL data pointer.",unparse_object(ROOT,i,0));
                   db_write_int(f,0);
		}
                break;
           case TYPE_THING:
                db_write_boolexp(f,obj->data->thing.lock);
                if(obj->data) {
                   db_write_int(f,1);
                   db_write_string(f,getfield(i,CSTRING));
                   db_write_string(f,getfield(i,ESTRING));
                   db_write_string(f,getfield(i,AREANAME));
                   db_write_int(f,obj->data->thing.mass);
                   db_write_int(f,obj->data->thing.volume);
                   db_write_currency(f,obj->data->thing.credit);
                   db_write_boolexp(f,getlock(i,1));
		} else {
                   writelog(BUG_LOG,1,"BUG","(db_write_object() in db.c)  Thing %s has NULL data pointer.",unparse_object(ROOT,i,0));
                   db_write_int(f,0);
		}
                break;
           case TYPE_CHARACTER:
                if(obj->data) {
                   db_write_int(f,1);
                   db_write_string(f,obj->data->player.password);
                   db_write_int(f,obj->data->player.score);
                   db_write_int(f,obj->data->player.mass);
                   db_write_int(f,obj->data->player.volume);
                   db_write_dbref(f,obj->data->player.controller);
                   db_write_int(f,obj->data->player.won);
                   db_write_int(f,obj->data->player.lost);
                   db_write_int(f,obj->data->player.scrheight);
                   db_write_long(f,obj->data->player.bantime);
                   db_write_aliases(f,obj->data->player.aliases);
                   db_write_friends(f,obj->data->player.friends);
                   db_write_int(f,obj->data->player.timediff);
                   db_write_int(f,obj->data->player.quotalimit);
                   db_write_currency(f,obj->data->player.health);
                   db_write_int(f,obj->data->player.damagetime);
                   db_write_int(f,obj->data->player.healthtime);
                   db_write_currency(f,obj->data->player.credit);
                   db_write_long(f,obj->data->player.longesttime);
                   db_write_long(f,obj->data->player.totaltime);
                   db_write_long(f,obj->data->player.lasttime);
                   db_write_int(f,obj->data->player.maillimit);
                   db_write_mail(f,obj->data->player.mail);
                   db_write_int(f,obj->data->player.topic_id);
                   db_write_dbref(f,obj->data->player.redirect);
                   db_write_dbref(f,obj->data->player.uid);
                   db_write_int(f,obj->data->player.feeling);
                   db_write_int(f,obj->data->player.subtopic_id);
                   db_write_profile(f,obj->data->player.profile);
                   db_write_currency(f,obj->data->player.balance);
                   db_write_currency(f,obj->data->player.income);
                   db_write_currency(f,obj->data->player.expenditure);
                   db_write_int(f,obj->data->player.htmlflags);
                   db_write_string(f,obj->data->player.htmlbackground);
                   db_write_int(f,obj->data->player.failedlogins);
                   db_write_int(f,obj->data->player.logins);
                   db_write_int(f,obj->data->player.payment);
                   db_write_int(f,obj->data->player.restriction);
                   db_write_long(f,obj->data->player.idletime);
                   db_write_long(f,obj->data->player.pwexpiry);
                   db_write_int(f,obj->data->player.htmlcmdwidth);
                   db_write_int(f,obj->data->player.afk);
                   db_write_int(f,obj->data->player.disclaimertime);
                   db_write_int(f,obj->data->player.longestdate);
                   db_write_string(f,obj->data->player.longdateformat);
                   db_write_string(f,obj->data->player.shortdateformat);
                   db_write_string(f,obj->data->player.dateseparator);
                   db_write_int(f,obj->data->player.dateorder);
                   db_write_string(f,obj->data->player.timeformat);
                   db_write_int(f,obj->data->player.prefflags);
		} else {
                   writelog(BUG_LOG,1,"BUG","(db_write_object() in db.c)  Character %s has NULL data pointer.",unparse_object(ROOT,i,0));
                   db_write_int(f,0);
		}
                break;
           case TYPE_ARRAY:
                if(obj->data) db_write_array(f,obj->data->array.start);
                   else db_write_int(f,0);
                break;
           case TYPE_COMMAND:
                db_write_boolexp(f,obj->data->command.lock);
                break;
           case TYPE_EXIT:
                db_write_boolexp(f,obj->data->exit.lock);
                break;
           case TYPE_FUSE:
                db_write_boolexp(f,obj->data->fuse.lock);
                break;
           case TYPE_PROPERTY:
           case TYPE_VARIABLE:
           case TYPE_ALARM:
                break;
           case TYPE_FREE:
                writelog(BUG_LOG,1,"BUG","(db_write_object() in db.c)  Tried to write free object #%d to DB.",i);
                break;
           default:
                writelog(BUG_LOG,1,"BUG","(db_write_object() in db.c)  Tried to write object %s of (Unknown) type %d.",unparse_object(ROOT,i,0),Typeof(i));
    }
    command_type = cached_commandtype;
    obj->parent  = parent_cache;
#endif
    return(1);
}

/* ---->  (J.P.Boggis 15/08/1999)  Database dumping process crash signal handler  <---- */
void dump_crash_handler(int sig)
{
     writelog(DUMP_LOG,1,"DUMP","Database dumping process received signal %d (%s:  %s)  -  Exiting...",signal,SignalName(sig),SignalDesc(sig));
     sprintf(cmpbuf,"%sdump.%d.pid",(lib_dir) ? "lib/":"",(int) telnetport);
     unlink(cmpbuf);
     exit(1);
}

/* ---->  (J.P.Boggis 15/08/1999)  Database dumping process alarm handler (Used to check that main TCZ process has not crashed/terminated)  <---- */
void dump_alarm_handler()
{
     if(kill(dumpparent,0)) {
        writelog(DUMP_LOG,1,"DUMP","Parent (Main) TCZ process has crashed/terminated  -  Closing user sockets.");
        sprintf(bootmessage,EMERGENCY_SHUTDOWN""ANSI_LMAGENTA"%s has crashed!  :-(\n"ANSI_LGREEN"Keep trying, we'll be back up again soon...\n\n\n",tcz_full_name,tcz_full_name);
        server_close_sockets();
     } else alarm(15);
     signal(SIGALRM,dump_alarm_handler);
}

/* ---->  Write database to file  <---- */
void db_write(struct descriptor_data *p)
{
#ifdef DATABASE_DUMP
     static short    dmin = 0x7FFF, dmax = 0, dcount = 0, laststatus = 0;
     static struct   bbs_topic_data *dumptopic = NULL;
     static struct   banish_data    *dumpbanish = NULL;
     static struct   site_data      *dumpsite = NULL;
     static unsigned char           compressed_db = 0;
     static int      dumpobject, dumpmax;
     static int      dumpnumber = 0;
     static char     tmpfile2[512];
     static char     tmpfile[512];
     static long     dtotal = 0;
     static int      logged = 0;
     static FILE     *dump  = NULL;
     static time_t   now;

     gettime(now);
     if((dumptype == DUMP_SANITISE) && (dumpstatus < 250) && (now < nextcycle)) {
        laststatus = dumpstatus;
        return;
     }
     if(dumperror) {
        writelog(DUMP_LOG,1,"DUMP","An error occurred while dumping the database to disk  -  Database dump aborted.");
        writelog(SERVER_LOG,1,"DUMP","Database dump #%d (#%ld) failed  -  Check 'Dump' log file for details.",dumpnumber,dump_serial_no + 1);
        writelog(DUMP_LOG,1,"DUMP","Database dump #%d (#%ld) failed.",dumpnumber,dump_serial_no + 1);
        output_admin(0,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"]  \007"ANSI_LYELLOW"An error occurred while dumping the database to disk  -  Database dump aborted (Check '"ANSI_LWHITE"Dump'"ANSI_LYELLOW" log file for details.)");
        dumpstatus = 255;
     }

     command_type |= NO_USAGE_UPDATE;
     switch(dumpstatus) {
            case 0:
                 break;
            case 1:

                 /* ---->  Initialise dump and write '@admin' settings/server statistical data  <---- */
                 writelog(DUMP_LOG,1,"DUMP","Initialising database dump #%d (#%ld)...",++dumpnumber,dump_serial_no + 1);
                 nextcycle  = now + DUMP_CYCLE_INTERVAL;
                 dumptiming = now;
                 dumperror  = 0;
                 totaldata  = 0;
                 dumpdata   = 0;

                 sprintf(tmpfile,"%s.%d.DUMP",dumpfile,(int) telnetport);
                 unlink(tmpfile);

#ifdef DB_COMPRESSION
                 if(option_compress_disk(OPTSTATUS)) {
                    compressed_db = 1;
                    sprintf(tmpfile,DB_COMPRESS" > %s.%d.DUMP",dumpfile,(int) telnetport);
                    dump = popen(tmpfile,"w");
		 } else {
                    dump = fopen(tmpfile,"w");
                    compressed_db = 0;
		 }
#else
                 dump = fopen(tmpfile,"w");
                 compressed_db = 0;
#endif
                 if(dump != NULL) {

                    /* ---->  Set every object in DB to ORIGINAL  <---- */
                    for(dumpobject = 0; dumpobject < db_top; dumpobject++)
                        if(Typeof(dumpobject) != TYPE_FREE) db[dumpobject].flags2 |= ORIGINAL;

                    dumpobject = 0;
                    dumpbanish = banish;
                    dumptopic  = bbs;
                    dumpsite   = NULL;
                    dumpmax    = db_top;

                    /* ---->  Write header and number of objects in DB  <---- */
                    dump_timedate = dumptiming;
                    sprintf(tmpfile,"***==--->  (Format %d)  %s (%s) Database (TCZ/%s v"TCZ_VERSION".%d)  <---==***\n",DATABASE_REVISION,tcz_full_name,tcz_short_name,CODENAME,TCZ_REVISION);
                    if(fputs(tmpfile,dump) == EOF) dumperror = 1;
                    dumpdata += strlen(tmpfile);

                    sprintf(tmpfile,"    Date:  %s\n",date_to_string(dump_timedate,UNSET_DATE,NOTHING,FULLDATEFMT));
                    if(fputs(tmpfile,dump) == EOF) dumperror = 1;
                    dumpdata += strlen(tmpfile);

                    sprintf(tmpfile," Objects:  %d\n",db_top);
                    if(fputs(tmpfile,dump) == EOF) dumperror = 1;
                    dumpdata += strlen(tmpfile);

                    sprintf(tmpfile,"Sanitise:  %d\n",((dumptype == DUMP_SANITISE) || (dumptype == DUMP_PANIC)) ? 1:0);
                    if(fputs(tmpfile,dump) == EOF) dumperror = 1;
                    dumpdata += strlen(tmpfile);

                    sprintf(tmpfile,"  Serial:  #%ld\n",dump_serial_no + 1);
                    if(fputs(tmpfile,dump) == EOF) dumperror = 1;
                    dumpdata += strlen(tmpfile);

                    /* ---->  Write '@admin' settings  <---- */
                    writelog(DUMP_LOG,1,"DUMP","Writing '@admin' settings...");
                    if(log_stderr && (dumpchild < 0)) progress_meter(0,MB,PROGRESS_UNITS,0);
                    db_write_int(dump,dumpinterval);
                    db_write_int(dump,limit_connections);
                    db_write_int(dump,allowed);
                    db_write_dbref(dump,bbsroom);
                    db_write_dbref(dump,mailroom);
                    db_write_dbref(dump,homerooms);
                    db_write_long(dump,timeadjust);
                    db_write_int(dump,dumpcycle);
                    db_write_int(dump,dumpdatasize);
                    db_write_int(dump,maintenance);
                    db_write_dbref(dump,maint_owner);
                    db_write_int(dump,maint_morons);
                    db_write_int(dump,maint_newbies);
                    db_write_int(dump,maint_mortals);
                    db_write_int(dump,maint_builders);
                    db_write_int(dump,maint_objects);
                    db_write_long(dump,mailcounter);
                    db_write_int(dump,maint_junk);
                    db_write_dbref(dump,bankroom);
                    db_write_long(dump,quarter);
                    db_write_int(dump,creations);
                    db_write_int(dump,connections);
                    db_write_dbref(dump,aliases);
                    db_write_long(dump,logins);

                    /* ---->  {J.P.Boggis 07/08/2001}  Write restart/dump/log serial numbers  <---- */
                    db_write_long(dump,restart_serial_no);
                    db_write_long(dump,dump_serial_no + 1);
                    db_write_int(dump,dump_timedate);  /*  Database dump time & date  */

                    /* ---->  Write statistics and new character requests  <---- */
                    writelog(DUMP_LOG,1,"DUMP","Writing statistics & new character requests...");
                    if(!dumperror) db_write_stats(dump);
                    if(!dumperror) db_write_requests(dump,request);
                    dumpstatus = 2;
                    logged     = 0;
                 } else {
                    writelog(DUMP_LOG,1,"DUMP","Unable to open dump file '%s' for writing (%s)  -  Database not dumped.",tmpfile,strerror(errno));
                    dumperror = 1;
		 }
                 break;
            case 2:  /*  IMPORTANT:  BBS dumping code must remain as dump status 2  */

                 /* ---->  Dump BBS topics  <---- */
                 if(logged != dumpstatus) writelog(DUMP_LOG,1,"DUMP","Writing BBS topics/subtopics..."), logged = dumpstatus;
                 while(!dumperror && (dumpstatus == 2) && ((dumptype != DUMP_SANITISE) || (dumpdata < dumpdatasize))) {
                       if(dumptopic) {
                          db_write_bbs(dump,dumptopic);
                          dumptopic = dumptopic->next;
   	  	       } else {
                          db_write_int(dump,0);
                          dumpstatus = 6;
		       }
		 }
                 if(dumptype == DUMP_SANITISE) nextcycle = now + (((float) dumpdata / dumpdatasize) * dumpcycle);
                 dumpdata = 0;
                 break;
            case 3:

                 /* ---->  Dump database objects  <---- */
                 if(logged != dumpstatus) writelog(DUMP_LOG,1,"DUMP","Writing database objects..."), logged = dumpstatus;
                 while(!dumperror && (dumpstatus == 3) && (dumpobject < dumpmax) && ((dumptype != DUMP_SANITISE) || (dumpdata < dumpdatasize))) {
                       db_write_object(dump,dumpobject);
                       for(dumpobject++; (dumpobject < dumpmax) && (Typeof(dumpobject) == TYPE_FREE); dumpobject++);
		 }
                 if(dumptype == DUMP_SANITISE) nextcycle = now + (((float) dumpdata / dumpdatasize) * dumpcycle);
                 if(dumpobject >= dumpmax) dumpstatus = 4;
                 dumpdata = 0;
                 break;
            case 4:

                 /* ---->  Dump footer  <---- */
                 db_write_string(dump,"***==--->  END OF DUMP  <---==***");
                 dumpstatus = 5;
                 break;
            case 5:

                 /* ---->  End database dump  <---- */
                 if(dumptype == DUMP_SANITISE) nextcycle = now + (((float) dumpdata / dumpdatasize) * dumpcycle);
                 dumpdata = 0;
                 fflush(dump);
                 if(compressed_db) pclose(dump);
                    else fclose(dump);
                 switch(dumptype) {
                        case DUMP_SANITISE:
                        case DUMP_NORMAL:
                             if(1) {
                                char newfile[512];
                                sprintf(tmpfile,"%s.%d.DUMP",dumpfile,(int) telnetport);
                                sprintf(newfile,"%s.new%s",dumpfile,(compressed_db) ? DB_EXTENSION:"");
                                if(rename(tmpfile,newfile) < 0) {
                                   writelog(DUMP_LOG,1,"DUMP","Unable to rename database dump file '%s' to '%s' (%s.)",tmpfile,newfile,strerror(errno));
                                   dumperror = 1;
				}
			     }
                             break;
                        case DUMP_PANIC:
                             if(1) {
                                char newfile[512];
                                sprintf(tmpfile,"%s.%d.DUMP",dumpfile,(int) telnetport);
                                sprintf(newfile,"%s.PANIC%s",dumpfile,(compressed_db) ? DB_EXTENSION:"");
                                if(rename(tmpfile,newfile) < 0) {
                                   writelog(DUMP_LOG,1,"DUMP","Unable to rename PANIC database dump file '%s' to '%s' (%s.)",tmpfile,newfile,strerror(errno));
                                   dumperror = 1;
				}
			     }
                             break;
                        default:
                             break;
		 }
                 dumpstatus = 96;
                 break;
            case 6:  /*  IMPORTANT:  Banished names dumping code must remain as dump status 6  */

                 /* ---->  Dump banished names  <---- */
                 if(logged != dumpstatus) writelog(DUMP_LOG,1,"DUMP","Writing banished names..."), logged = dumpstatus;
                 while(!dumperror && (dumpstatus == 6) && ((dumptype != DUMP_SANITISE) || (dumpdata < dumpdatasize)))
                       if(dumpbanish) {
                          if(!Blank(dumpbanish->name)) db_write_string(dump,dumpbanish->name);
                          dumpbanish = dumpbanish->next;
   	  	       } else {
                          db_write_string(dump,NULL);
                          dumpstatus = 3;
		       }
                 if(dumptype == DUMP_SANITISE) nextcycle = now + (((float) dumpdata / dumpdatasize) * dumpcycle);
                 dumpdata = 0;
                 break;
            case 96:

                 /* ---->  Initialise registered Internet sites dump to 'lib/sites.tcz'  <---- */
                 sprintf(tmpfile,SITE_FILE".%d.DUMP",(int) telnetport);
                 unlink(tmpfile);

#ifdef DB_COMPRESSION
                 if(option_compress_disk(OPTSTATUS)) {
                    compressed_db = 1;
                    sprintf(tmpfile,DB_COMPRESS" > "SITE_FILE".%d.DUMP",(int) telnetport);
                    dump = popen(tmpfile,"w");
		 } else {
                    dump = fopen(tmpfile,"w");
                    compressed_db = 0;
		 }
#else
                 dump = fopen(tmpfile,"w");
                 compressed_db = 0;
#endif

                 if(dump != NULL) {
                    for(dumpsite = sitelist, dumpmax = 0; dumpsite; dumpsite = dumpsite->next) dumpmax++;
                    sprintf(writebuffer,"# %s (%s) registered Internet sites database.\n# Revision:  %d\n#     Date:  %s\n#   Serial:  #%ld\n#\n#                          Max\n# IP Address:    Flags:    Con:   Connected:  Created:  Description:\n# =============================================================================\n",tcz_full_name,tcz_short_name,SITE_DB_REVISION,date_to_string(now,UNSET_DATE,NOTHING,FULLDATEFMT),dump_serial_no + 1);
                    if(fputs(writebuffer,dump) == EOF) dumperror = 1;
                    dumpdata += strlen(writebuffer), dumpsite = sitelist;
                    dumpobject = 0, dumpstatus = 97;
		 } else {
                    writelog(DUMP_LOG,1,"DUMP","Unable to open registered Internet sites dump file '%s' for writing  -  Registered Internet sites not dumped.",SITE_FILE);
                    dumperror = 1;
		 }
                 break;
            case 97:  /*  IMPORTANT:  Registered Internet sites dumping code must remain as dump status 99  */

                 /* ---->  Dump registered Internet sites to 'lib/sites.tcz'  <---- */
                 if(logged != dumpstatus) writelog(DUMP_LOG,1,"DUMP","Writing registered Internet sites..."), logged = dumpstatus;
                 while(!dumperror && (dumpstatus == 97) && dumpsite && ((dumptype != DUMP_SANITISE) || (dumpdata < dumpdatasize))) {
                       decompress(dumpsite->description);
                       if(dumpsite->max_connections != NOTHING) sprintf(tmpfile + 450,"%d",dumpsite->max_connections);
                          else strcpy(tmpfile + 450,"-");
                       sprintf(writebuffer,"%-17s%c%c%c%c%c%c%c%c  %-7s%-12ld%-10d%s\n",ip_to_text(dumpsite->addr,dumpsite->mask,tmpfile),(dumpsite->flags & SITE_ADMIN) ? 'A':'-',(dumpsite->flags & SITE_BANNED) ? 'B':'-',(dumpsite->flags & SITE_CREATE) ? 'C':'-',(dumpsite->flags & SITE_DNS) ? 'D':'-',(dumpsite->flags & SITE_GUESTS) ? 'G':'-',(dumpsite->flags & SITE_NODNS) ? 'N':'-',(dumpsite->flags & SITE_READONLY) ? 'R':'-',(dumpsite->flags & SITE_UNCONDITIONAL) ? 'U':'-',tmpfile + 450,dumpsite->connected,dumpsite->created,cmpbuf);
                       if(fputs(writebuffer,dump) == EOF) dumperror = 1;
                       dumpdata += strlen(writebuffer), dumpsite = dumpsite->next;
                       dumpobject++;
		 }
                 if(dumptype == DUMP_SANITISE) nextcycle = now + (((float) dumpdata / dumpdatasize) * dumpcycle);
                 if(!dumpsite) dumpstatus = 98;
                 dumpdata = 0;
                 break;
            case 98:

                 /* ---->  Registered Internet sites dump footer  <---- */
                 sprintf(writebuffer,"# =============================================================================\n# Total registered Internet sites:  %d.\n",dumpobject);
                 if(fputs(writebuffer,dump) == EOF) dumperror = 1;
                 dumpdata += strlen(writebuffer), dumpstatus = 99;
                 break;
            case 99:

                 /* ---->  End registered Internet sites dump  <---- */
                 fflush(dump);
                 if(compressed_db) pclose(dump);
                    else fclose(dump);
                 if(dumptype == DUMP_PANIC) {
                    sprintf(tmpfile,SITE_FILE".PANIC%s",(compressed_db) ? DB_EXTENSION:"");
                    sprintf(tmpfile2,SITE_FILE".%d.DUMP",(int) telnetport);
                    if(rename(tmpfile2,tmpfile) < 0) {
                       writelog(DUMP_LOG,1,"DUMP","Unable to rename PANIC registered Internet sites dump file '%s' to '%s' (%s.)",tmpfile2,tmpfile,strerror(errno));
                       dumperror = 1;
		    }
		 } else {
                    sprintf(tmpfile,SITE_FILE"%s",(compressed_db) ? DB_EXTENSION:"");
                    sprintf(tmpfile2,SITE_FILE".%d.DUMP",(int) telnetport);
                    if(rename(tmpfile2,tmpfile) < 0) {
                       writelog(DUMP_LOG,1,"DUMP","Unable to rename registered Internet sites dump file '%s' to '%s' (%s.)",tmpfile2,tmpfile,strerror(errno));
                       dumperror = 1;
		    }
		 }
                 dumpstatus = 100;
                 break;
            case 100:

                 /* ---->  Update database dumping statistics  <---- */
                 if(1) {
                    short total;

                    dcount++;
                    if(log_stderr && (dumpchild < 0)) progress_meter(totaldata,MB,PROGRESS_UNITS,2);
                    if((dtotal += (total = (now - dumptiming))) < 0) dtotal = 0;
                    if(total < dmin) dmin = total;
                    if(total > dmax) dmax = total;
                    if(dumpchild <= 0) writelog(DUMP_LOG,1,"DUMP","Database dump #%d (#%ld) completed in %s.",dumpnumber,dump_serial_no + 1,interval(total,0,ENTITIES,0));

		    dump_serial_no++;
                    compressed_db = 0;
                    dumptiming    = now;
                    dumpobject    = dumpmax = 0;
                    dumpstatus    = 0;
                    dumpbanish    = NULL;
                    dumperror     = 0;
                    nextcycle     = 0;
                    dumptopic     = NULL;
                    dumpsite      = NULL;
                    dumpdata      = 0;
		 }
                 break;
            case 250:

                 /* ---->  Report on dump progress (For '@admin')  <---- */
                 switch(laststatus) {
                        case 1:
                             sprintf(cmpbuf, ANSI_LGREEN "    Database dumping status:  " ANSI_LWHITE "Initialising database dump.\n");
                             break;
                        case 2:
                             if(dumptopic) sprintf(cmpbuf, ANSI_LGREEN "    Database dumping status:  " ANSI_LWHITE "Dumping BBS topic '" ANSI_LYELLOW "%s" ANSI_LWHITE "'.\n", String(dumptopic->name));
			        else sprintf(cmpbuf, ANSI_LGREEN "    Database dumping status:  " ANSI_LWHITE "Finishing BBS topic database dump.\n");
                             break;
                        case 3:
                             if(dumpobject < dumpmax) sprintf(cmpbuf, ANSI_LGREEN "    Database dumping status:  " ANSI_LWHITE "%d/%d objects dumped.\n", dumpobject + 1, dumpmax);
			        else sprintf(cmpbuf, ANSI_LGREEN "    Database dumping status:  " ANSI_LWHITE "%d/%d objects dumped.\n", dumpmax, dumpmax);
                             break;
                        case 4:
                        case 5:
                             sprintf(cmpbuf, ANSI_LGREEN "    Database dumping status:  " ANSI_LWHITE "Finishing database dump.\n");
                             break;
                        case 6:
                             if(dumpbanish) sprintf(cmpbuf, ANSI_LGREEN "    Database dumping status:  " ANSI_LWHITE "Dumping banished name '" ANSI_LYELLOW "%s" ANSI_LWHITE "'.\n", String(dumpbanish->name));
			        else sprintf(cmpbuf, ANSI_LGREEN "    Database dumping status:  " ANSI_LWHITE "Finishing banished names database dump.\n");
                             break;
                        case 96:
                             sprintf(cmpbuf, ANSI_LGREEN "    Database dumping status:  " ANSI_LWHITE "Initialising site database dump.\n");
                             break;
                        case 97:
                             if(dumpsite) sprintf(cmpbuf, ANSI_LGREEN "    Database dumping status:  " ANSI_LWHITE "%d/%d sites dumped.\n", dumpobject + 1, dumpmax);
			        else sprintf(cmpbuf, ANSI_LGREEN "    Database dumping status:  " ANSI_LWHITE "Finishing Internet site database dump.\n");
                             break;
                        case 98:
                        case 99:
                             sprintf(cmpbuf, ANSI_LGREEN "    Database dumping status:  " ANSI_LWHITE "Finishing Internet site database dump.\n");
                             break;
                        case 100:
                             sprintf(cmpbuf, ANSI_LGREEN "    Database dumping status:  " ANSI_LWHITE "Updating database dump statistics.\n");
                             break;
                        case 253:
                        case 254:
                             sprintf(cmpbuf, ANSI_LGREEN "    Database dumping status:  " ANSI_LWHITE "Dump in progress (See '" ANSI_LYELLOW "Dump" ANSI_LWHITE "' log file.)\n");
                             break;
                        case 255:
                             sprintf(cmpbuf, ANSI_LGREEN "    Database dumping status:  " ANSI_LWHITE "Aborting database dump.\n");
                             break;
		 }
                 break;
            case 251:

                 /* ---->  Report on database dump duration  <---- */
                 if(1) {
                    long average = (dcount > 0) ? (dtotal / dcount):0;
                    sprintf(cmpbuf, ANSI_LGREEN "     Database dump duration:  " ANSI_LWHITE "%d:%02d.%02d" ANSI_DCYAN "/" ANSI_LWHITE "%ld:%02ld.%02ld" ANSI_DCYAN "/" ANSI_LWHITE "%d:%02d.%02d" ANSI_DCYAN "  (" ANSI_LCYAN "Min" ANSI_DCYAN "/" ANSI_LCYAN "Avg" ANSI_DCYAN "/" ANSI_LCYAN "Max" ANSI_DCYAN ")\n", (dtotal > 0) ? (dmin / HOUR) : 0, (dtotal > 0) ? (dmin % HOUR / MINUTE) : 0, (dtotal > 0) ? (dmin % MINUTE) : 0, average / HOUR, average % HOUR / MINUTE, average % MINUTE, dmax / HOUR, dmax % HOUR / MINUTE, dmax % MINUTE);
		 }
                 break;
            case 253:

                 /* ---->  Fork dump currently running  <---- */
                 break;
            case 254:

                 /* ---->  Fork dumping process  <---- */
#ifndef DB_FORK
                 writelog(BUG_LOG,1,"BUG","Attempt to fork database dump when DB_FORK is not #define'd.");
#else
                 writelog(DUMP_LOG,1,"DUMP","Forking database dumping process...");
                 dumpchild = fork();
                 if(dumpchild == 0) {
                    FILE *lf;

                    /* ---->  Code executed by child dumping process  <---- */
#ifdef RESTRICT_MEMORY
                    nice(15 + PRIORITY);
#else
		    nice(15);
#endif
                    close(0);

		    /* ---->  Disable automatic time adjustment  <---- */
                    auto_time_adjust = 0;

                    /* ---->  Change signal handling  <---- */
                    dumpparent = getppid();
                    signal(SIGCONT,SIG_IGN);
                    signal(SIGUSR1,SIG_IGN);
                    signal(SIGUSR2,SIG_IGN);
                    signal(SIGILL,dump_crash_handler);
                    signal(SIGINT,dump_crash_handler);
                    signal(SIGIOT,SIG_IGN);
                    signal(SIGTERM,dump_crash_handler);
                    signal(SIGTRAP,SIG_IGN);
                    signal(SIGVTALRM,SIG_IGN);
                    signal(SIGXCPU,dump_crash_handler);
                    signal(SIGXFSZ,dump_crash_handler);
                    signal(SIGBUS,dump_crash_handler);
                    signal(SIGSEGV,dump_crash_handler);
                    signal(SIGQUIT,SIG_IGN);
                    signal(SIGCHLD,SIG_DFL);
                    signal(SIGPIPE,SIG_IGN);
                    signal(SIGFPE,server_SIGFPE_handler);
                    signal(SIGALRM,dump_alarm_handler);
                    alarm(15);

#ifndef LINUX
                    /* ---->  Linux specific signals  <---- */
                    signal(SIGEMT,SIG_IGN);
                    signal(SIGSYS,SIG_IGN);
#endif

                    /* ---->  Create dump lock file  <---- */
                    sprintf(tmpfile,"%sdump.%d.pid",(lib_dir) ? "lib/":"",(int) telnetport);
                    if((lf = fopen(tmpfile,"w"))) {
                       fprintf(lf,"%d",getpid());
                       fflush(lf);
                       fclose(lf);
		    } else writelog(SERVER_LOG,0,"WARNING","Unable to create the dump lock file '%s'.",tmpfile);

                    /* ---->  Begin database dump  <---- */
                    dumpstatus = 1;
                    dumptype   = DUMP_NORMAL;
                    while(dumpstatus > 0)
                          db_write(NULL);

                    /* ---->  Delete dump lock file  <---- */
                    sprintf(tmpfile,"%sdump.%d.pid",(lib_dir) ? "lib/":"",(int) telnetport);
                    unlink(tmpfile);
                    exit(0);

                    /* ---->  Code executed by main TCZ process  <---- */
		 } else if(dumpchild < 0) {

                    /* ---->  Fork child process failed  -  Attempt to dump database internally  <---- */
                    writelog(DUMP_LOG,1,"Unable to fork database dumping process (%s)  -  Attempting to dump database internally...",strerror(errno));
                    writelog(SERVER_LOG,1,"Unable to fork database dumping process (%s)  -  Attempting to dump database internally...",strerror(errno));
                    dumpchild  = NOTHING;
                    dumptype   = DUMP_SANITISE;
                    dumpstatus = 1;
		 } else {

                    /* ---->  Fork child process successful (Parent process)  <---- */
                    dumpstatus = 253;
                    dumptiming = now;
                    dumperror  = 0;
                    dumpnumber++;
		 }
#endif
                 break;
            case 255:
            default:

                 /* ---->  Abort current database dump  <---- */
#ifdef DB_FORK
                 if(dumpchild > 0) {
                    int dumppid = dumpchild;

                    dumpchild = NOTHING;
                    kill(dumppid,9);
                    sprintf(tmpfile,"%sdump.%d.pid",(lib_dir) ? "lib/":"",(int) telnetport);
                    unlink(tmpfile);
		 }
#endif

                 /* ---->  Delete temporary dump files  <---- */
                 if(1) {
                    char tmpfile[512];

                    /* ---->  Close gzip/compress pipe  <---- */
                    if(dump) {
                       if(compressed_db) pclose(dump);
                          else fclose(dump);
		    }

                    sprintf(tmpfile,SITE_FILE".%d.DUMP",(int) telnetport);
                    unlink(tmpfile);
                    sprintf(tmpfile,"%s.%d.DUMP",dumpfile,(int) telnetport);
                    unlink(tmpfile);
		 }

                 writelog(DUMP_LOG,1,"DUMP","Database dump aborted.");
                 compressed_db = 0;
                 dumptiming    = now;
                 dumpstatus    = 0;
                 nextcycle     = 0;
                 dumperror     = 0;
                 dumpdata      = 0;

                 /* ---->  If in child dumping process, inform main TCZ process by returning error code 1  <---- */
                 if(!dumpchild) exit(1);
                 break;
     }
     command_type &= ~NO_USAGE_UPDATE;
     laststatus    = dumpstatus;
#else
     return;
#endif
}

/* ---->  Read DBref from database file  <---- */
static dbref db_read_dbref(FILE *f)
{
       const char *str;

       if(log_stderr) progress_meter(bytesread,MB,PROGRESS_UNITS,1);
       if(dumperror || (!(str = db_read_field(f)))) return(NOTHING);
       return(atol(str));
}

/* ---->  Read integer from database file  <---- */
static int db_read_int(FILE *f)
{
       const char *str;

       if(log_stderr) progress_meter(bytesread,MB,PROGRESS_UNITS,1);
       if(dumperror || (!(str = db_read_field(f)))) return(0);
       return(atol(str));
}

/* ---->  Read long integer from database file  <---- */
static long db_read_long(FILE *f)
{
       const char *str;

       if(log_stderr) progress_meter(bytesread,MB,PROGRESS_UNITS,1);
       if(dumperror || (!(str = db_read_field(f)))) return(0);
       return(atol(str));
}

/* ---->  Read double value (Floating point) from database file  <---- */
static double db_read_double(FILE *f)
{
       if(log_stderr) progress_meter(bytesread,MB,PROGRESS_UNITS,1);
       if(dumperror) return(0);
       return(tofloat(db_read_field(f),NULL));
}

/* ---->  Read currency value (struct currency_data) from database file  <---- */
static struct currency_data *db_read_currency(FILE *f,struct currency_data *currency)
{
       if(dumperror) return(0);
       currency->decimal  = db_read_long(f);
       currency->fraction = db_read_int(f);
       return(currency);
}

/* ---->  Read string from database file  <---- */
static const char *db_read_string(FILE *f,unsigned char allocate,unsigned char compression)
{
       char *p,*p1,*p2;
       int  count = 0;

       if(dumperror) return(NULL);
       if(log_stderr) progress_meter(bytesread,MB,PROGRESS_UNITS,1);
       if((p1 = p2 = p = db_read_field(f))) {
          while(*p1) {
                while(*p1 && ((*p1 < 32) && (*p1 != '\x1B') && (*p1 != '\n'))) p1++;
                if(*p1) *p2++ = *p1++, count++;
	  }
          *p2 = '\0';
          if(count > MAX_LENGTH) p[MAX_LENGTH] = '\0';
       }
       return((allocate) ? (char *) alloc_string((compression) ? compress(p,0):p):(compression) ? compress(p,0):p);
}

/* ---->  Read statistics array entry from database file  <---- */
static void db_read_statent(FILE *f,int pos)
{
       if(!dumperror) {
          stats[pos].charconnected = db_read_long(f);
          stats[pos].objdestroyed  = db_read_long(f);
          stats[pos].charcreated   = db_read_long(f);
          stats[pos].objcreated    = db_read_long(f);
          stats[pos].shutdowns     = db_read_long(f);
          stats[pos].time          = db_read_long(f);
          stats[pos].peak          = db_read_long(f);
       }
}

/* ---->  Read TCZ statistics from database file  <---- */
static void db_read_stats(FILE *f,int version)
{
       int loop;

       /* ---->  Read in daily stats  <---- */
       stat_ptr = db_read_int(f);
       if(stat_ptr >= STAT_HISTORY) stat_ptr = (STAT_HISTORY - 1);
       if(stat_ptr < -1)            stat_ptr = -1; 
       for(loop = 0; !dumperror && (loop <= stat_ptr) && (loop < STAT_HISTORY); loop++)
           db_read_statent(f,loop);

       /* ---->  Read in max/total/days  <---- */
       db_read_statent(f,STAT_MAX);
       db_read_statent(f,STAT_TOTAL);
       stat_days = db_read_long(f);

       /* ---->  Write database creation date and accumulated uptime  <---- */
       if(version > 93) {
          if(version > 94) {
             db_accumulated_restarts = db_read_int(f);
             db_longest_uptime       = db_read_int(f);
	  }
          db_accumulated_uptime      = db_read_int(f);
          db_creation_date           = db_read_int(f);
          if(version > 95)
             db_longest_date         = db_read_int(f);
       } else {
          db_accumulated_restarts    = 1;
          db_accumulated_uptime      = 0;
          db_longest_uptime          = 0;
          gettime(db_creation_date);
          db_longest_date = db_creation_date;
       }
}

/* ---->  Read Dynamic Array from database file  <---- */
static struct array_element *db_read_array(FILE *f,int version)
{
       struct array_element *new,*current = NULL,*start = NULL;
       long   elements;
  
       /* ---->  Read each element from the database file  <---- */
       for(elements = db_read_long(f); !dumperror && (elements > 0); elements--) {
           if((new = malloc(sizeof(struct array_element))) == NULL)
              abortmemory("(db_read_array() in db.c)  Insufficient memory to allocate dynamic array element.");
           if(version > 35) new->index = (char *) db_read_string(f,1,0);
              else new->index = NULL;
           new->text = (char *) db_read_string(f,1,1);
           new->next = NULL;
           if(current) {
              current->next = new;
              current       = new;
	   } else current = start = new;
       }
       return(start);
}

/* ---->  Read character's aliases from database file  <---- */
static struct alias_data *db_read_aliases(FILE *f,dbref player,int version)
{
       const char *alias,*command;
       int   count;

       in_command = 1;
       count = db_read_int(f);
       if(count > 0) 
          for(; !dumperror && (count > 0); count--) {
              alias = strcpy(scratch_buffer,db_read_string(f,0,0));
              if(version < 51) {

                 /* ---->  Convert old-style aliases  <---- */
                 strcpy(scratch_return_string,db_read_string(f,0,0));
                 strcat(scratch_return_string," !*");
                 if(strlen(scratch_return_string) > MAX_LENGTH)
                    scratch_return_string[MAX_LENGTH] = '\0';
                 command = scratch_return_string;
              } else command = (char *) db_read_string(f,0,0);
              alias_alias(player,NULL,NULL,(char *) alias,(char *) command,1,0);
	  }
       in_command = 0;
       return(db[player].data->player.aliases);
}

/* ---->  Read list of character names from database file  <---- */
/*
static struct list_data *db_read_list(FILE *f)
{
       struct list_data *new,*list = NULL,*current = NULL;
       int    count = 0;
 
       count = db_read_int(f);
       if(count > 0) 
          for(; !dumperror && (count > 0); count--) {
              if((new = malloc(sizeof(struct list_data))) == NULL)
                 abortmemory("(db_read_list() in db.c)  Insufficient memory to allocate character DBref.");
              new->player = db_read_dbref(f);
              new->next   = NULL;
              if(current) {
                 current->next = new;
                 current       = new;
	      } else list = current = new;
	  }
       return(list);
}
*/

/* ---->  Read BBS topics, sub-topics and messages from database file  <---- */
static struct bbs_topic_data *read_bbs(FILE *f,int version)
{
       struct   bbs_topic_data *tnew,*topics = NULL,*tcurrent = NULL;
       struct   bbs_message_data *mnew,*mcurrent;
       struct   bbs_reader_data *rnew,*rcurrent;
       int      current_id,count,subcount;
       unsigned char repeat;

       while(!dumperror && ((current_id = db_read_int(f)) > 0)) {

             /* ---->  Topic data  <---- */
             if((tnew = malloc(sizeof(struct bbs_topic_data))) == NULL)
                abortmemory("(read_bbs() in db.c)  Insufficient memory to allocate BBS topic.");
             tnew->topic_id = current_id;
             if(version < 55) {

                /* ---->  Convert old style access level to privilege() compatible access level  <---- */
                switch(db_read_int(f)) {
                       case -1:
                            tnew->accesslevel = 6;
                            break;
                       case 0:
                            tnew->accesslevel = 8;
                            break;
                       case 1:
                            tnew->accesslevel = 4;
                            break;
                       case 2:
                            tnew->accesslevel = 3;
                            break;
                       case 3:
                            tnew->accesslevel = 2;
                            break;
                       case 4:
                            tnew->accesslevel = 1;
                            break;
                       case 5:
                            tnew->accesslevel = 0;
                            break;
                       default:
                            tnew->accesslevel = 8;
		}
             } else tnew->accesslevel = db_read_int(f);
             tnew->desc          = (char *) db_read_string(f,1,1);
             tnew->name          = (char *) db_read_string(f,1,0);
             tnew->owner         = db_read_dbref(f);
             tnew->flags         = db_read_int(f);
             tnew->timelimit     = db_read_int(f);
             tnew->messagelimit  = (version >  43) ? db_read_int(f):BBS_DEFAULT_MAX_MESSAGES;
             tnew->subtopiclimit = (version >= 55) ? db_read_int(f):BBS_DEFAULT_MAX_SUBTOPICS;
             tnew->subtopics     = NULL;
             tnew->messages      = NULL;
             tnew->next          = NULL;
             if(version < 48) tnew->flags |= TOPIC_ANON;
             if(version < 51) tnew->flags |= TOPIC_CYCLIC;
             if(version < 55) tnew->flags |= TOPIC_ADD;

             /* ---->  Sub-topics  <---- */
             if(version >= 55) tnew->subtopics = read_bbs(f,version);

             /* ---->  Messages  <---- */
             count = db_read_int(f), mcurrent = NULL;
             if(count > 0) 
                for(; !dumperror && (count > 0); count--) {
                    if((mnew = malloc(sizeof(struct bbs_message_data))) == NULL)
                       abortmemory("(read_bbs() in db.c)  Insufficient memory to allocate BBS message.");

                    /* ---->  Reader list  <---- */
                    mnew->readers = NULL, rcurrent = NULL;
                    for(repeat = (version < 55) ? 2:1; repeat; repeat--)
                        for(subcount = db_read_int(f); !dumperror && (subcount > 0); subcount--) {
                            MALLOC(rnew,struct bbs_reader_data);
                            rnew->reader = db_read_dbref(f);
                            rnew->flags  = (version >= 55) ? db_read_int(f):(repeat > 1) ? (READER_READ|READER_VOTE_AGAINST):(READER_READ|READER_VOTE_FOR);
                            rnew->next   = NULL;

                            if(rcurrent) {
                               rcurrent->next = rnew;
                               rcurrent       = rnew;
			    } else mnew->readers = rcurrent = rnew;
			}

                    /* ---->  Message data  <---- */
                    mnew->message       = (char *) db_read_string(f,1,1);
                    mnew->subject       = (char *) db_read_string(f,1,1);
                    mnew->name          = (char *) db_read_string(f,1,1);
                    mnew->readercount   = db_read_int(f);
                    mnew->flags         = db_read_int(f);
                    mnew->owner         = db_read_dbref(f);
                    mnew->date          = db_read_int(f);
                    mnew->lastread      = db_read_long(f);
                    mnew->id            = (version > 57) ? db_read_int(f):count;
                    mnew->expiry        = (version > 76) ? db_read_int(f):0;
                    mnew->flags        &= ~MESSAGE_MODIFY;
                    mnew->next          = NULL;
                    if(version < 55) mnew->flags |= MESSAGE_EVERYONE;
                    if(version < 65) mnew->flags |= MESSAGE_ALLOWAPPEND;
                    if(version < 79) mnew->expiry = 0;

                    if(mcurrent) {
                       mcurrent->next = mnew;
                       mcurrent       = mnew;
		    } else tnew->messages = mcurrent = mnew;
		}

             if(tcurrent) {
                tcurrent->next = tnew;
                tcurrent       = tnew;
	     } else topics = tcurrent = tnew;
       }
       return(topics);
}

/* ---->  Read character's list of friends from database file  <---- */
static struct friend_data *db_read_friends(FILE *f,int version)
{
       struct friend_data *new,*friends = NULL,*current = NULL;
       int    count = 0;

       count = db_read_int(f);
       if(count > 0) 
          for(; !dumperror && (count > 0); count--) {
              if((new = malloc(sizeof(struct friend_data))) == NULL)
                 abortmemory("(db_read_friends() in db.c)  Insufficient memory to allocate friend.");
              new->friend = db_read_dbref(f);
              if(version > 40) {
                 new->flags = db_read_int(f);
                 if(version < 51) new->flags |= FRIEND_PAGETELLFRIENDS;
	      } else new->flags = FRIEND_STANDARD;
              new->next   = NULL;
              if(current) {
                 current->next = new;
                 current       = new;
	      } else friends = current = new;
	  }
       return(friends);
}

/* ---->  Read character's mail from database file  <---- */
static struct mail_data *db_read_mail(FILE *f)
{
       struct   mail_data *new,*ptr,*mail = NULL,*current = NULL;
       int      count = 0;
       unsigned char group;
       dbref    i;

       count = db_read_int(f);
       if(count > 0) 
          for(; !dumperror && (count > 0); count--) {
              if((new = malloc(sizeof(struct mail_data))) == NULL)
                 abortmemory("(db_read_mail() in db.c)  Insufficient memory to allocate mail.");
              new->lastread = db_read_long(f);
              new->flags    = db_read_int(f);
              new->date     = db_read_int(f);
              new->who      = db_read_dbref(f);
              new->id       = db_read_long(f);
              new->next     = NULL;

              /* ---->  Handle group mail  <---- */
              if(new->flags & MAIL_MULTI) {
                 for(i = 0, group = 0; (i < db_top) && !group; i++)
                     if(Typeof(i) == TYPE_CHARACTER) {

                        /* ---->  Check for existence of group mail in current mailbox (The one which is being loaded from the database file)  <---- */
                        for(ptr = mail; ptr && !group; ptr = ptr->next)
                            if((ptr->flags & MAIL_MULTI) && (ptr->id == new->id)) {
                               new->redirect = ptr->redirect;
                               new->subject  = ptr->subject;
                               new->message  = ptr->message;
                               new->sender   = ptr->sender;
                               group         = 1;
			    }

                        /* ---->  Check for existence of group mail in the database  <---- */
                        for(ptr = db[i].data->player.mail; ptr && !group; ptr = ptr->next)
                            if((ptr->flags & MAIL_MULTI) && (ptr->id == new->id)) {
                               new->redirect = ptr->redirect;
                               new->subject  = ptr->subject;
                               new->message  = ptr->message;
                               new->sender   = ptr->sender;
                               group         = 1;
			    }
		     }

                 if(!group) {

                    /* ---->  Group mail doesn't already exist  -  Create as root node  <---- */
                    new->redirect = (char *) db_read_string(f,1,1);
                    new->subject  = (char *) db_read_string(f,1,1);
                    new->message  = (char *) db_read_string(f,1,1);
                    new->sender   = (char *) db_read_string(f,1,1);
                    new->flags   |= MAIL_MULTI_ROOT;
		 } else {

                    /* ---->  Group mail exists  -  Read past text fields in database file  <---- */
                    db_read_string(f,0,0);
                    db_read_string(f,0,0);
                    db_read_string(f,0,0);
                    db_read_string(f,0,0);
		 }
	      } else {
                 new->redirect = (char *) db_read_string(f,1,1);
                 new->subject  = (char *) db_read_string(f,1,1);
                 new->message  = (char *) db_read_string(f,1,1);
                 new->sender   = (char *) db_read_string(f,1,1);
	      }

              if(current) {
                 current->next = new;
                 current       = new;
	      } else mail = current = new;
	  }
       return(mail);
}

/* ---->  Read character's profile from database file  <---- */
static struct profile_data *db_read_profile(FILE *f,int version)
{
       struct profile_data *profile = NULL;

       if(db_read_int(f)) {
          MALLOC(profile,struct profile_data);
          initialise_profile(profile);
          profile->qualifications = (char *) db_read_string(f,1,1);
          profile->achievements   = (char *) db_read_string(f,1,1);
          profile->nationality    = (char *) db_read_string(f,1,1);
          profile->occupation     = (char *) db_read_string(f,1,1);
          profile->interests      = (char *) db_read_string(f,1,1);
          profile->sexuality      = db_read_int(f);
          profile->comments       = (char *) db_read_string(f,1,1);
          profile->country        = (char *) db_read_string(f,1,1);
          profile->hobbies        = (char *) db_read_string(f,1,1);
          profile->height         = db_read_int(f);
          profile->weight         = db_read_int(f);
          if(version < 71) {
             if(profile->height) profile->height = ((1 << 15) + ((profile->height / 100) << 7) + (profile->height % 100));
             if(profile->weight) profile->weight = ((1 << 31) + ((profile->weight / 1000) << 16) + (profile->weight % 1000));
	  }
          profile->drink          = (char *) db_read_string(f,1,1);
          profile->music          = (char *) db_read_string(f,1,1);
          profile->other          = (char *) db_read_string(f,1,1);
          profile->sport          = (char *) db_read_string(f,1,1);
          profile->city           = (char *) db_read_string(f,1,1);
          profile->eyes           = (char *) db_read_string(f,1,1);
          profile->food           = (char *) db_read_string(f,1,1);
          profile->hair           = (char *) db_read_string(f,1,1);
          profile->irl            = (char *) db_read_string(f,1,1);
          if(version >= 60) {
             profile->likes       = (char *) db_read_string(f,1,1);
             profile->dislikes    = (char *) db_read_string(f,1,1);
	  }
          if(version >= 61) profile->statusirl = db_read_int(f);
          if(version >= 70) profile->statusivl = db_read_int(f);
          if(version >= 64) {
             profile->dob     = db_read_int(f);
             profile->picture = (char *) db_read_string(f,1,1);
	  }
       }
       return(profile);
}

/* ---->  Read requests for new characters from database file  <---- */
static struct request_data *db_read_requests(FILE *f,int version)
{
       char           *email,*name;
       unsigned long  address;
       int            count;
       dbref          user;
       time_t         date;
       unsigned short ref;

       for(count = db_read_int(f); !dumperror && (count > 0); count--) {
           email = (char *) db_read_string(f,1,0);
           if(!Blank(email)) {
              address = db_read_long(f);
              date    = db_read_int(f);
              user    = db_read_dbref(f);
              ref     = db_read_int(f);
              if(version >= 68) name = (char *) db_read_string(f,1,0);
                 else name = NULL;
              request_add(email,name,address,NULL,date,user,ref);
              FREENULL(email);
              FREENULL(name);
	   }
       }
       return(request);
}

/* ---->  Read boolean expression from database  <---- */
static struct boolexp *db_read_subboolexp(char **buffer_ptr)
{
       struct boolexp *b;
       int    c;

       c = *(*buffer_ptr)++;
       switch(c) {
              case '\0':
                   return(TRUE_BOOLEXP);
                   break;
              case '(':
                   if((b = (struct boolexp *) malloc(sizeof(struct boolexp))) == NULL)
                      abortmemory("(db_read_subboolexp() in db.c)  Insufficient memory to allocate sub-part of boolean expression.");
                   if((c = *(*buffer_ptr)++) == '!') {
                      b->object = NOTHING;
                      b->type   = BOOLEXP_NOT;
                      b->sub1   = db_read_subboolexp(buffer_ptr);
                      b->sub2   = NULL;
                      if((*(*buffer_ptr)++) != ')') goto error;  /*  Yeeeuck!  A goto!  (It's not mine  -  I'm innocent!)  :-)  */
                      return(b);
		   } else if(c == '@') {
                      b->type   = BOOLEXP_FLAG;
                      b->object = 0;
                      b->sub1   = NULL;
                      b->sub2   = NULL;

                      /*  NOTE:  Possibly non-portable code...                     */
                      /*         Will need to be changed if db_write_dbref/db_read_dbref change.  */
                      while(isdigit(c = *(*buffer_ptr)++)) b->object = b->object * 10 + c - '0';
                                
                      if(c != ')') goto error;  /*  Another one  -  aaaargh!  */
                      return(b);
		   } else {
                      (*buffer_ptr)--;
                      b->object = NOTHING;
                      b->sub1   = db_read_subboolexp(buffer_ptr);
                      b->sub2   = NULL;
                      switch(c = *(*buffer_ptr)++) {
                             case AND_TOKEN:
                                  b->type = BOOLEXP_AND;
                                  break;
                             case OR_TOKEN:
                                  b->type = BOOLEXP_OR;
                                  break;
                              default:
                                  goto error;
                                  break;
		      }
                      b->sub2 = db_read_subboolexp(buffer_ptr);
                      if(*(*buffer_ptr)++ != ')') goto error;  /*  No comment!  :-)  */
                      return(b);
		   }
                   /* break; */
              default:

                   /* ---->  Assume it's a DBref  <---- */
                   (*buffer_ptr)--;
                   if((b = (struct boolexp *) malloc(sizeof(struct boolexp))) == NULL)
                      abortmemory("(db_read_subboolexp() in db.c)  Insufficient memory to allocate sub-part of boolean expression.");
                   b->object = 0;
                   b->type   = BOOLEXP_CONST;
                   b->sub1   = NULL;
                   b->sub2   = NULL;

                   /*  NOTE:  Possibly non-portable code...                     */
                   /*         Will need to be changed if db_write_dbref/db_read_dbref change.  */
                   while(isdigit(c = *(*buffer_ptr)++)) b->object = b->object * 10 + c - '0';

                   (*buffer_ptr)--;
                   return(b);
       }
       error:abortmemory("(db_read_subboolexp() in db.c)  Insufficient memory to allocate boolean expression.");
       return(TRUE_BOOLEXP);
}

/* ---->  Parse DB ref.  <---- */
dbref parse_dbref(const char *s)
{
      const char *p = s;
      int   chars;
      long  x;

      /* ---->  If it's NULL, return NOTHING  <---- */
      if(s == NULL) return(NOTHING);

      /* ---->  If there's a leading hash, skip it  <---- */
      if(*p == NUMBER_TOKEN) p++;

      if(sscanf(p,"%ld%n",&x,&chars) == 1) {
         if((x >= 0) && (p[chars] == '\0')) return(x);
            else return(NOTHING);
      } else return(NOTHING);
}

/* ---->  Read boolean expression from database file  <---- */
struct boolexp *db_read_boolexp(FILE *f)
{
       char *buf_ptr;

       buf_ptr = db_read_field(f);
       return(db_read_subboolexp(&buf_ptr));
}

/* ---->  Free boolean expression from memory  <---- */
void free_boolexp(struct boolexp **ptr)
{
     if((*ptr) != TRUE_BOOLEXP) {
        if((*ptr)->sub1) free_boolexp(&((*ptr)->sub1));
        if((*ptr)->sub2) free_boolexp(&((*ptr)->sub2));
        FREENULL((*ptr));
     }
}

/* ---->  Filter obsolete ';' usage in pre-revision 75 database (Replaced by '@areawrite')  <---- */
const char *filter_areawrite(char *buffer,const char *src)
{
      char *dest;

      if(!src) return(NULL);
      dest = buffer;
      while(*src) {
            while(*src && (*src == ' ') && (*src != '\n')) *dest++ = *src++;
            if(*src && (*src == ';')) {
               strcpy(dest,"@areawrite ");
               dest += 11, src++;
	    }
            while(*src && (*src != '\n')) *dest++ = *src++;
            while(*src && (*src == '\n')) *dest++ = *src++;
      }
      *dest = '\0';
      buffer[TEXT_SIZE] = '\0';
      return(buffer);
}

/* ---->  Filter obsolete pre-v4.2 '@executionlimit'/'@recursionlimit' from given text  <---- */
const char *filter_executionlimit(char *dest,const char *src)
{
      const char *ptr;
      char  *tmp;

      *dest = '\0';
      if(!src) return(NULL);
      tmp = dest;

      while(*src) {

            /* ---->  Scan for '@executionlimit' or '@recursionlimit' statement  <---- */
            for(ptr = src; *ptr && (*ptr != '\n') && (*ptr != '@'); ptr++);
            if(*ptr && (*ptr == '@') && (string_prefix(ptr + 1,"rec") || string_prefix(ptr + 1,"exe"))) {

               /* ---->  Skip line  <---- */
               for(; *src && (*src != '\n'); src++);
               if(*src) src++;
	    } else {

               /* ---->  Copy line  <---- */
               for(; *src && (*src != '\n'); *tmp++ = *src++);
               if(*src) *tmp++ = *src++;
	    }
      }
      *tmp = '\0';
      return(dest);
}

/* ---->  Read object from database file (Supports version 29 and greater)  <---- */
dbref db_read_object(FILE *f,dbref i,int version)
{
     struct   object *o = db + i;
     unsigned char email = 2;
     int      objectversion;
     int      convert = 0;

     /* ---->  Read and check object version  <---- */
     if(version > 84) {
        objectversion = db_read_int(f);
        if(objectversion > DATABASE_REVISION) {
           writelog(SERVER_LOG,1,"READ","Cannot read object #%d because it is revision %d (Revisions up to %d are supported.)",i,objectversion,DATABASE_REVISION);
           return(NOTHING);
        } else if(objectversion < LOWEST_REVISION) {
           writelog(SERVER_LOG,1,"READ","Cannot read object #%d because it is revision %d (Revisions under %d are no-longer supported.)",i,objectversion,LOWEST_REVISION);
           return(NOTHING);
        } else if(objectversion != version) {
           writelog(SERVER_LOG,1,"READ","Converting object #%d from revision %d to %d.",i,objectversion,version);
           version = objectversion;
        }
     }

     /* ---->  Ensure object doesn't already exist in database  <---- */
     if(Typeof(i) != TYPE_FREE) {
        writelog(SERVER_LOG,1,"READ","Cannot read object #%d because it already exists in the database.",i);
        return(NOTHING);
     }

     /* ---->  Object type  <---- */
     if(version < 102) {
        o->flags  = db_read_int(f);
        o->flags2 = db_read_int(f);

        /* ---->  Convert old types stored in flags to separate type number  <---- */
        switch(o->flags2 & 0x3F000000) {
               case 0x01000000:

                    /* ---->  Thing  <---- */
                    o->type = TYPE_THING;
                    break;
               case 0x02000000:

                    /* ---->  Exit  <---- */
                    o->type = TYPE_EXIT;
                    break;
               case 0x03000000:

                    /* ---->  Character  <---- */
                    o->type = TYPE_CHARACTER;
                    break;
               case 0x04000000:

                    /* ---->  Room  <---- */
                    o->type = TYPE_ROOM;
                    break;
               case 0x05000000:

                    /* ---->  Compound Command  <---- */
                    o->type = TYPE_COMMAND;
                    break;
               case 0x06000000:

                    /* ---->  Fuse  <---- */
                    o->type = TYPE_FUSE;
                    break;
               case 0x07000000:

                    /* ---->  Alarm  <---- */
                    o->type = TYPE_ALARM;
                    break;
               case 0x08000000:

                    /* ---->  Variable  <---- */
                    o->type = TYPE_VARIABLE;
                    break;
               case 0x09000000:

                    /* ---->  Dynamic Array  <---- */
                    if(version < 34) {

                       /* ---->  Pre-v34 TYPE_WEAPON (Never implemented):  Convert to TYPE_THING and mark as ASHCAN  <---- */
                       o->type   = TYPE_THING;
		       o->flags |= ASHCAN|OBJECT;
		       convert   = 1;
		    } else o->type = TYPE_ARRAY;
                    break;
               case 0x0A000000:

                    /* ---->  Property  <---- */
                    o->type = TYPE_PROPERTY;
                    break;
               case 0x0C000000:

                    /* ---->  Pre-v34 TYPE_ARRAY  <---- */
                    if(version < 34) o->type = TYPE_ARRAY;
                       else o->type = TYPE_FREE;
		    break;
	       case 0x0D000000:

                    /* ---->  Pre-v34 TYPE_PROPERTY  <---- */
                    if(version < 34) o->type = TYPE_PROPERTY;
                       else o->type = TYPE_FREE;
		    break;
               default:

                    /* ---->  Free (0x00000000) or unknown type  <---- */
                    o->type = TYPE_FREE;
	}

        o->flags2 &= ~(0x3F000000);  /*  Clear old type flags  */
     } else {
        o->type   = db_read_int(f);
        o->flags  = db_read_int(f);
        o->flags2 = db_read_int(f);
     }

     if(Typeof(i) != TYPE_FREE)      o->checksum = ++checksum;
     if(Typeof(i) != TYPE_CHARACTER) o->flags2  &= ~NON_EXECUTABLE;
     if((version < 59) && (Typeof(i) == TYPE_CHARACTER)) {
        if(db[i].flags2 & 0x400) email = 1;
        db[i].flags2 ^= PAGEBELL;  /*  Invert old PAGEBELL flag (Now indicates set rather than reset)  */
        db[i].flags2 &= ~(0x400);  /*  Remove old EMAIL_PUBLIC flag  */
     }

     /* ---->  Object name  <---- */
     o->data = NULL;
     o->name = NULL;
     initialise_data(i);
     setfield(i,NAME,db_read_string(f,0,0),0);

     /* ---->  Description and others description (If applicable)  <---- */
     if(version < 30) {
        setfield(i,DESC,db_read_string(f,0,0),0);
        setfield(i,ODESC,db_read_string(f,0,0),0);
     }

     /* ---->  Parent object  <---- */
     if(version >= 32) o->parent = db_read_dbref(f);
        else o->parent = NOTHING;

     /* ----->  Standard object DBref pointers  <----- */
     o->location    = db_read_dbref(f);  
     o->destination = db_read_dbref(f);
     o->contents    = db_read_dbref(f);
     o->exits       = db_read_dbref(f);
     o->next        = db_read_dbref(f);

     /* ---->  Object locks  <---- */
     if(version < 47) switch(Typeof(i)) {
        case TYPE_EXIT:
             o->data->exit.lock = db_read_boolexp(f);
             break;
        case TYPE_FUSE:
             o->data->fuse.lock = db_read_boolexp(f);
             break;
        case TYPE_ROOM:
             o->data->room.lock = db_read_boolexp(f);
             break;
        case TYPE_THING:
             o->data->thing.lock = db_read_boolexp(f);
             break;
        case TYPE_COMMAND:
             o->data->command.lock = db_read_boolexp(f);
             break;
        default:
             if(1) {
                struct boolexp *ptr = db_read_boolexp(f);
                free_boolexp(&ptr);
	     }
     }

     /* ---->  Convert old HOME number (-3) to new (-2)  <---- */
     if(version < 83) {
        if(o->exits       == -3) o->exits       = HOME;
        if(o->contents    == -3) o->contents    = HOME;
        if(o->destination == -3) o->destination = HOME;
     }

     /* ---->  Pre-v30 DB fixed standard object fields  <---- */
     if(version < 30) {
        setfield(i,FAIL,db_read_string(f,0,0),0);
        setfield(i,SUCC,db_read_string(f,0,0),0);
        setfield(i,DROP,db_read_string(f,0,0),0);
        setfield(i,OFAIL,db_read_string(f,0,0),0);
        setfield(i,OSUCC,db_read_string(f,0,0),0);
        setfield(i,ODROP,db_read_string(f,0,0),0);
     }

     /* ----->  Standard object DBref pointers  <---- */
     o->owner       = db_read_dbref(f); 
     if(version < 37) db_read_int(f);
     o->commands    = db_read_dbref(f);
     o->variables   = db_read_dbref(f);
     o->fuses       = db_read_dbref(f);
     if(version < 33) db_read_dbref(f);

     /* ---->  Creation/last usage time/date  <---- */
     if(version < 38) db[i].flags &= ~(0x02000000);  /*  Remove obsolete FIGHTING flag  */
     if(version > 51) {
        o->lastused = db_read_long(f);
        o->created  = db_read_long(f);
        if(version > 53) o->expiry = db_read_int(f);
     } else {
        time_t now;

        gettime(now);
        o->lastused = now;
        o->created  = now;
     }

     /* ---->  Read dynamic standard object fields (Based on object type)  <---- */
     if(version > 29) {
        setfield(i,DESC,db_read_string(f,0,0),0);
        setfield(i,SUCC,db_read_string(f,0,0),0);
        setfield(i,FAIL,db_read_string(f,0,0),0);
        if((version < 59) && (Typeof(i) == TYPE_CHARACTER)) {
           const char *emailaddr = db_read_string(f,0,0);
           if(!Blank(emailaddr)) {
              if(strchr(emailaddr,'\n')) setfield(i,EMAIL,emailaddr,0);
                 else setfield(i,EMAIL,(char *) settextfield(emailaddr,email,'\n',NULL,scratch_return_string),0);
	   }
        } else setfield(i,(Typeof(i) == TYPE_CHARACTER) ? EMAIL:DROP,db_read_string(f,0,0),0);
        setfield(i,(Typeof(i) == TYPE_CHARACTER) ? WWW:ODESC,db_read_string(f,0,0),0);
        setfield(i,OSUCC,db_read_string(f,0,0),0);
        setfield(i,OFAIL,db_read_string(f,0,0),0);
        setfield(i,ODROP,db_read_string(f,0,0),0);
     }

     /* ---->  Read additional fields (As per object type)  <---- */
     if((version < 48) && (Typeof(i) != TYPE_EXIT) && (Typeof(i) != TYPE_THING)) o->flags &= ~OPAQUE;
     switch(Typeof(i)) {
            case TYPE_ROOM:
                 if(version >= 47) o->data->room.lock = db_read_boolexp(f);
                 if((version < 85) || ((version >= 85) && db_read_int(f))) {
                    o->data->room.mass   = db_read_int(f);
                    o->data->room.volume = db_read_int(f);
                    if(version > 80) {
                       if(version > 84) db_read_currency(f,&(o->data->room.credit));
                          else db_read_long(f);
		    }
                    setfield(i,AREANAME,db_read_string(f,0,0),0);
                    if(version > 83) {
                       setfield(i,CSTRING,db_read_string(f,0,0),0);
                       setfield(i,ESTRING,db_read_string(f,0,0),0);
                    }
                 }

                 if(version < 52) {
                    if(o->data->room.mass   < 0) o->data->room.mass   = STANDARD_ROOM_MASS;
                    if(o->data->room.volume < 0) o->data->room.volume = STANDARD_ROOM_VOLUME;
                 }
                 if(version  < 81) o->flags2 |= FINANCE;
                 if((version < 54) && !RoomZero(i) && !Start(i) && !Global(i)) o->flags2 |= (WARP|VISIT);
                 if(RoomZero(i)) {
                    db[i].flags2           |= SECURE;
                    db[i].data->room.mass   = TCZ_INFINITY;
                    db[i].data->room.volume = TCZ_INFINITY;
                 }
                 if(Global(i))   db[i].flags2 |=  SECRET;
                 if(Sendhome(i)) db[i].flags2 &= ~SENDHOME;
                 o->flags |= OBJECT;
                 break;
            case TYPE_THING:
                 if(version >= 47) o->data->thing.lock = db_read_boolexp(f);
                 if(convert != 1) {
                    if((version < 85) || ((version >= 85) && db_read_int(f))) {
                       setfield(i,CSTRING,db_read_string(f,0,0),0);
                       if(version > 83) setfield(i,ESTRING,db_read_string(f,0,0),0);
                       setfield(i,AREANAME,db_read_string(f,0,0),0);
                       o->data->thing.mass   = db_read_int(f);
                       o->data->thing.volume = db_read_int(f);
                       if(version > 80) {
                          if(version > 84) db_read_currency(f,&(o->data->thing.credit));
                             else db_read_long(f);
		       }
                       o->data->thing.lock_key = db_read_boolexp(f);
                    }
		 } else {

                    /* ---->  Convert weapon to thing:  Read past weapon fields in DB  <---- */
                    db_read_int(f);
                    db_read_int(f);
                    db_read_int(f);
                    db_read_double(f);
                    db_read_int(f);
                    db_read_int(f);
		 }

                 if(version < 52) {
                    if(o->data->thing.mass   < 0) o->data->thing.mass   = STANDARD_THING_MASS;
                    if(o->data->thing.volume < 0) o->data->thing.volume = STANDARD_THING_VOLUME;
                 }
                 if(version < 84) double_to_currency(&(o->data->thing.credit),0);
                 o->flags |= OBJECT;
                 break;
            case TYPE_CHARACTER:
                 if((version < 85) || ((version >= 85) && db_read_int(f))) {
                    o->data->player.password = (char *) db_read_string(f,1,0);
                    o->data->player.score    = db_read_int(f);
                    if(version < 37) {
                       unsigned char loop;

                       for(loop = 0; loop < 10; loop++) db_read_int(f);
                    }
                    if(version < 37) {
                       o->data->player.longesttime = 0;
                       o->data->player.totaltime   = atol((getfield(i,OFAIL)) ? getfield(i,OFAIL):"");
                       o->data->player.lasttime    = atol((getfield(i,FAIL)) ? getfield(i,FAIL):"");
                       double_to_currency(&(o->data->player.credit),tofloat(getfield(i,ODROP),NULL));
                       setfield(i,RACE,db_read_string(f,0,0),0);
                    }
                    o->data->player.mass   = db_read_int(f);
                    o->data->player.volume = db_read_int(f);
                    if(version < 52) {
                       if(o->data->player.mass   < 0) o->data->player.mass   = STANDARD_CHARACTER_MASS;
                       if(o->data->player.volume < 0) o->data->player.volume = STANDARD_CHARACTER_VOLUME;
                    }
                    o->data->player.controller = db_read_dbref(f);
                    if(version < 37) setfield(i,TITLE,db_read_string(f,0,0),0);
                    o->data->player.won        = db_read_int(f);
                    o->data->player.lost       = db_read_int(f);
                    if(version > 41) o->data->player.scrheight = db_read_int(f);
                       else db_read_int(f);
                    o->data->player.bantime    = db_read_long(f);
                    if(version < 37) setfield(i,LASTSITE,db_read_string(f,0,0),0);
                    db_read_aliases(f,i,version);
                    if(version > 29) o->data->player.friends  = db_read_friends(f,version);
                    if(version > 30) o->data->player.timediff = db_read_int(f);
                    if(version >= 37) {
                       o->data->player.quotalimit  = db_read_int(f);
                       if(version > 84) db_read_currency(f,&(o->data->player.health));
                          else double_to_currency(&(o->data->player.health),db_read_double(f));
                       o->data->player.damagetime  = db_read_int(f);
                       o->data->player.healthtime  = db_read_int(f);
                       if(version > 84) db_read_currency(f,&(o->data->player.credit));
                          else double_to_currency(&(o->data->player.credit),db_read_double(f));
                       o->data->player.longesttime = db_read_long(f);
                       o->data->player.totaltime   = db_read_long(f);
                       o->data->player.lasttime    = db_read_long(f);
                    }

                    if(version > 38) {
                       o->data->player.maillimit  = db_read_int(f);
                       if(version < 48) {
                          if(Level4(i) && (o->data->player.maillimit < MAIL_LIMIT_ADMIN))
                             o->data->player.maillimit = MAIL_LIMIT_ADMIN;
                          db_read_int(f);
                          db_read_int(f);
                       }
                       o->data->player.mail = db_read_mail(f);
                    }

                    if(version > 41) o->data->player.topic_id = db_read_int(f);
                    if(version > 48) o->data->player.redirect = db_read_dbref(f);
                    if(version > 51) o->data->player.uid      = db_read_dbref(f);
                    if(version > 52) {
                       o->data->player.feeling = db_read_int(f);
                       if(version < 64) {
                          unsigned long dob = db_read_long(f);
            
                          if(dob != UNSET_DATE) {
                             set_profile_init(i);
                             o->data->player.profile->dob = dob;
                          }
                       }
                       o->data->player.subtopic_id = db_read_long(f);
                    }
                    if(version > 55) o->data->player.profile = db_read_profile(f,version);
                    if(version >= 58) {
                       if(version >= 74) {
                          if(version > 84) {
                             db_read_currency(f,&(o->data->player.balance));
                             db_read_currency(f,&(o->data->player.income));
                             db_read_currency(f,&(o->data->player.expenditure));
                          } else {
                             double_to_currency(&(o->data->player.balance),db_read_double(f));
                             double_to_currency(&(o->data->player.income),db_read_double(f));
                             double_to_currency(&(o->data->player.expenditure),db_read_double(f));
                          }
                       } else {
                          double_to_currency(&(o->data->player.balance),db_read_double(f));
                          db_read_double(f);
                          db_read_double(f);
                       }
                    } else double_to_currency(&(o->data->player.balance),Puppet(i) ? 0:DEFAULT_BALANCE);

                    if(version >= 60) {
                      o->data->player.htmlflags      = db_read_int(f);
                      o->data->player.htmlbackground = (char *) db_read_string(f,1,1);
                    }

                    if(version >= 73) {
                       o->data->player.failedlogins = db_read_int(f);
                       if(version < 104) db_read_long(f);  /*  Pre-version 104 totaladjust field (Removed)  */
                       o->data->player.logins       = db_read_int(f);
                    }

                    if(version >= 74) o->data->player.payment        = db_read_int(f);
                    if(version >= 76) o->data->player.restriction    = db_read_int(f);
                    if(version >= 79) o->data->player.idletime       = db_read_long(f);
                    if(version >= 80) o->data->player.pwexpiry       = db_read_long(f);
                    if(version >= 90) o->data->player.htmlcmdwidth   = db_read_int(f);
                    if(version >= 91) o->data->player.afk            = db_read_int(f);
                    if(version >= 92) o->data->player.disclaimertime = db_read_int(f);
                    if(version >= 97) o->data->player.longestdate    = db_read_int(f);

                    /* ---->  Preferred date/time formats  <---- */
                    if(version >= 100) {
                       o->data->player.longdateformat  = (char *) db_read_string(f,1,0);
                       o->data->player.shortdateformat = (char *) db_read_string(f,1,0);
                       o->data->player.dateseparator   = (char *) db_read_string(f,1,0);
                       o->data->player.dateorder       = db_read_int(f);
                       o->data->player.timeformat      = (char *) db_read_string(f,1,0);
		    } else {
                       o->data->player.longdateformat  = LONGDATEFMT;
                       o->data->player.shortdateformat = SHORTDATEFMT;
                       o->data->player.dateseparator   = DATESEPARATOR;
                       o->data->player.dateorder       = DATEORDER;
                       o->data->player.timeformat      = TIMEFMT;
		    }

                    /* ---->  Preference flags  <---- */
                    if(version >= 101) o->data->player.prefflags = db_read_int(f);
                       else o->data->player.prefflags = PREFS_DEFAULT;
		 }

                 /* ---->  Compatibility (No more fields read after this point for characters)  <---- */
                 o->data->player.quota = 0;
                 o->flags2 &= ~(CONNECTED|CHAT_OPERATOR|CHAT_PRIVATE);
                 if((version < 45) && Level4(i)) o->flags   |=  SHOUT|BOOT;
                 if(version  < 30) o->flags2                |=  FRIENDS_CHAT;
                 if(version == 45) o->data->player.scrheight =  STANDARD_CHARACTER_SCRHEIGHT;
                 if(version  < 48) o->flags                 &= ~TRANSFERABLE;
                 if(version  < 58) double_to_currency(&(o->data->player.credit),Puppet(i) ? 0:DEFAULT_CREDIT);
                 if(version < 52) {
                    if(Level1(i)) o->flags &= ~DRUID;
                    o->flags2 |=  (BBS|MAIL|MORE_PAGER|EDIT_NUMBERING);
                    o->flags2 &= ~(0x00000100);
		 }
                 if((version < 69) && Experienced(i)) o->flags &= ~HELP;
                 if((version < 72) && Retired(i)) o->flags &= ~(APPRENTICE|WIZARD|ELDER|DEITY);
                 if(version  < 88) o->flags &= ~BEING;
                 if(version  < 98) {
                    o->data->player.idletime = 0;
                    o->data->player.logins   = 1;
		 }
                 o->flags &= ~(PRIVATE|INVISIBLE);
                 o->flags |=  OBJECT;
                 break;
            case TYPE_ARRAY:
                 o->data->array.start = db_read_array(f,version);
                 if(version < 33) o->next = NOTHING;
                 o->flags |= OBJECT;
                 break;
            case TYPE_COMMAND:
                 if(version <  47) o->flags &= ~(BUILDER|YELL);
                 if(version >= 47) o->data->command.lock = db_read_boolexp(f);
                 if(version <  75) {
                    dbref cached_parent = db[i].parent;

                    db[i].parent = NOTHING;
                    if(version < 60) setfield(i,DESC,(char *) filter_executionlimit(scratch_return_string,getfield(i,DESC)),0);
                    if(version < 75) setfield(i,DESC,(char *) filter_areawrite(scratch_return_string,getfield(i,DESC)),0);
                    db[i].parent = cached_parent;
		 }
                 if(version < 79) o->flags |= CENSOR;
                 o->flags |= OBJECT;
                 break;
            case TYPE_EXIT:
                 if(version >= 47) o->data->exit.lock = db_read_boolexp(f);
                 if(version <  52) o->flags          |= OPEN;
                 if(version <  54) o->flags2         |= TRANSPORT;
                 o->flags |= OBJECT;
                 break;
            case TYPE_FUSE:
                 if(version >= 47) o->data->fuse.lock = db_read_boolexp(f);
                 if(version <  47) o->flags          &= ~(BUILDER|YELL);
                 if(version <  52) o->flags          &= ~INVISIBLE;
                 o->flags |=  OBJECT;
                 break;
            case TYPE_ALARM:
                 if(version < 47) o->flags &= ~(BUILDER|YELL);
            case TYPE_PROPERTY:
            case TYPE_VARIABLE:
                 o->flags |= OBJECT;
            case TYPE_FREE:
                 break;
            default:
                 writelog(SERVER_LOG,1,"READ","Cannot read data (db[x].data) of object %s(#%d) because its type is unknown (%d.)",db[i].name,i,Typeof(i));
                 return(NOTHING);
     }
     return(i);
}

/* ---->  Read objects in database into memory  <---- */
unsigned char db_read(FILE *f)
{
	 int    version,c = 1;
	 dbref  i;
	 time_t start,finish;
	 char   *end,*p1;
	 char   temp[16];
	 
	 dumperror = 0;
	 while(!isdigit(c) && (c != '\n')) {
	       if(c == EOF) return(0);
	       c = getc(f);
	 }

	 if(c != '\n') {
	    int loop = 0;

	    for(p1 = temp; isdigit(c) && (c != '\n') && (loop < 15); loop++) {
		if(c == EOF) return(0);
		*p1++ = c, c = getc(f);
	    }
	 }
	 *p1 = '\0';

	 if(c != '\n') while(c != '\n') {
	    if(c == EOF) return(0);
	    c = getc(f);
	 }
	 version = atoi(temp);
   
	 if((version < LOWEST_REVISION) || (version > DATABASE_REVISION)) {
	    writelog(SERVER_LOG,0,"RESTART","Unable to read database.  Revision %d is %s supported.",version,(version > DATABASE_REVISION) ? "not":"no-longer");
	    return(0);
	 } else if(version < DATABASE_REVISION)
	    writelog(SERVER_LOG,0,"RESTART","Converting revision %d database to revision %d.",version,DATABASE_REVISION);

	 /* ---->  Read past dump date  <---- */
	 if((version > 44) && !fgets(cmpbuf,BUFFER_LEN - 1,f)) return(0);

	 /* ---->  Read the number of objects in the database  <---- */
	 if(fscanf(f,(version > 44) ? " Objects:  %d":"Objects: %d",&db_top) != 1) return(0);
	 if((c = getc(f)) == EOF) return(0);  /*  Skip NEWLINE  */

	 /* ---->  Read database consistency checks/corrections setting  <---- */
	 if(version >= 40) {
	    int sanitise;

	    if(fscanf(f,(version > 44) ? "Sanitise:  %d":"Sanitise: %d",&sanitise) != 1) return(0);
	    if((c = getc(f)) == EOF) return(0);  /*  Skip NEWLINE  */
	    dumpsanitise = sanitise;
	 } else dumpsanitise = 1;  /*  Force full sanity check on old database formats  */

	 /* ---->  Read past serial number (Dumped within database, serial in header not required)  <---- */
	 if((version >= 103) && !fgets(cmpbuf,BUFFER_LEN - 1,f)) return(0);

	 /* ---->  Allocate and initialise the main database structure  <---- */
	 if((db = (struct object *) malloc(db_top * sizeof(struct object))) == NULL) {
	    writelog(SERVER_LOG,0,"RESTART","Insufficient memory to allocate database with %d object%s.",db_top,Plural(db_top));
	    return(0);
	 } else for(i = 0; i < db_top; i++) {
	    initialise_object(db + i);
	    db[i].type = TYPE_FREE;
	 }

	 /* ---->  Read '@admin' settings  <---- */
	 command_type |= NO_USAGE_UPDATE;
	 dumpinterval       = db_read_int(f);
	 if(version < 46)     db_read_int(f);
	 if(version < 35)     db_read_int(f);
	 if(version < 46)     db_read_int(f);
	 if(version < 35)     db_read_int(f);
	 limit_connections  = (db_read_int(f) > 0);
	 allowed            = db_read_int(f);
	 bbsroom            = (version > 41) ? db_read_dbref(f):NOTHING;
	 mailroom           = (version > 98) ? db_read_dbref(f):NOTHING;
	 homerooms          = (version > 98) ? db_read_dbref(f):NOTHING;

	 if(version < 89)     db_read_int(f);
	 if(version < 48) for(i = 0; i < 4; db_read_long(f), i++);
	 if(version < 46) for(i = 0; i < 7; db_read_long(f), i++);
	 if(version > 42) {
	    timeadjust  = db_read_long(f);
	    dumptiming += timeadjust;
	    uptime     += timeadjust;
	 }

	 if(version > 47) {
	    dumpcycle      = db_read_int(f);
	    dumpdatasize   = db_read_int(f);
	    maintenance    = db_read_int(f);
	    maint_owner    = db_read_dbref(f);
	    maint_morons   = db_read_int(f);
	    if(version > 49) maint_newbies = db_read_int(f);
	    maint_mortals  = db_read_int(f);
	    maint_builders = db_read_int(f);
	    if(version > 51) maint_objects = db_read_int(f);
	 }

	 if(version > 48) mailcounter = db_read_long(f);
	 if(version > 52) maint_junk  = db_read_int(f);
	 if(version >= 58) {
	    bankroom   = db_read_dbref(f);
	    if(version >= 73) quarter = db_read_long(f);
	    if(version < 74) {
	       db_read_long(f);
	       db_read_double(f);
	       db_read_double(f);
	    }
	 }

	 if(version > 61) {
	    creations = db_read_int(f);
	    connections = db_read_int(f);
	 }

	 if(version > 65) aliases = db_read_dbref(f);
	 if(version > 78) logins  = db_read_long(f);
	    else gettime(logins);

         /* ---->  {J.P.Boggis 07/08/2001}  Read restart/dump/log serial numbers  <---- */
         if(version >= 103) {
            restart_serial_no = db_read_long(f);
            dump_serial_no    = db_read_long(f);
            dump_timedate     = db_read_int(f);

	    restart_serial_no++;
	    writelog(SERVER_LOG,0,"RESTART","Database serial number #%ld, dumped on %s.",dump_serial_no,date_to_string(dump_timedate,UNSET_DATE,NOTHING,FULLDATEFMT));
	    writelog(SERVER_LOG,0,"RESTART","%s server restart number %ld.",tcz_short_name,restart_serial_no);
	 }
	 if(dumperror) return(0);

	 /* ---->  Initialise progress meter  <---- */
	 bytesread = 0;
	 gettime(start);
	 if(log_stderr) progress_meter(0,MB,PROGRESS_UNITS,0);

	 /* ---->  Read statistics, new character requests and BBS  <---- */
	 db_read_stats(f,version);
	 if(dumperror) return(0);
	 request = (version > 56) ? (struct request_data *)   db_read_requests(f,version):NULL;
	 if(dumperror) return(0);
	 bbs     = (version > 41) ? (struct bbs_topic_data *) read_bbs(f,version):NULL;
	 if(dumperror) return(0);

	 /* ---->  Read banished names  <---- */
	 if(version > 62) {
	    const char *str;
	    while((str = db_read_string(f,0,0)) && *str)
		  banish_add(str,NULL);
	 }

	 /* ---->  Read the objects up to end of dump  <---- */
	 while((c = getc(f)) != EOF) {
	       switch(c) {
		      case '#':
			   i = db_read_dbref(f);
			   db_read_object(f,i,version);
			   break;
		      case '*':
			   end = (char *) db_read_string(f,0,0);
			   if(strcmp(end,"**==--->  END OF DUMP  <---==***")) {
			      if(log_stderr) progress_meter(bytesread,MB,PROGRESS_UNITS,2);
			      command_type &= ~NO_USAGE_UPDATE;
			      return(0);
			   } else {

			      /* ---->  Data read statistics  <---- */
			      gettime(finish);
			      if(log_stderr) progress_meter(bytesread,MB,PROGRESS_UNITS,2);
			      if(finish <= start) finish = start + 1;
			      writelog(SERVER_LOG,0,"RESTART","Total of %.2fMb data read in %s (%dKb/sec.)",(double) bytesread / MB,interval(finish - start,0,ENTITIES,0),(bytesread / KB) / (finish - start));

			      /* ---->  Initialise list of free objects in database  <---- */
			      for(i = 0; i < db_top; i++)
				  if(Typeof(i) == TYPE_FREE) {
				     if(db_free_chain == NOTHING) db_free_chain = i;
					else db[db_free_chain_end].next = i;
				     db_free_chain_end = i;
				  }

			      /* ---->  Ensure Supreme Being is a Deity  <---- */
			      db[ROOT].flags &= ~(MORON|EXPERIENCED|DRUID|APPRENTICE|WIZARD|ELDER);
			      db[ROOT].flags |=  DEITY;

			      /* ---->  Insert dynamic arrays in pre-v33 DB's into variables list  <---- */
			      if(version < 33)
				 for(i = 0; i < db_top; i++)
				     if(Typeof(i) == TYPE_ARRAY)
					PUSH(i,db[db[i].location].variables);

			      command_type &= ~NO_USAGE_UPDATE;
			      return(1);
			   }
			   break;
		      default:
			   dumperror = 1;
	       }
	       if(dumperror) {
		  if(log_stderr) progress_meter(bytesread,MB,PROGRESS_UNITS,2);
		  command_type &= ~NO_USAGE_UPDATE;
		  return(0);
	       }
	 }

	 if(log_stderr) progress_meter(bytesread,MB,PROGRESS_UNITS,2);
	 command_type &= ~NO_USAGE_UPDATE;
	 return(0);
}

/* ---->  Read field from database  <---- */
char *db_read_field(FILE *f)
{
     static char buffer[TEXT_SIZE * 4];
     int    length = 0,chr = 0;
     char   *ptr = buffer;

     while(((chr = getc(f)) != EOF) && (chr != FIELD_SEPARATOR)) {
           if(length < ((TEXT_SIZE * 4) - 1))
              *ptr++ = chr;
           bytesread++;
     }
     *ptr = '\0';
     return((chr != EOF) ? buffer:NULL);
}
