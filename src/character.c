/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| CHARACTER.C  -  Implement character lookup (By name), character creation,   |
|                 guest characters, feelings, password checking, etc.         |
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
|            Additional major coding by:  J.P.Boggis 08/08/1994.              |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                        https://github.com/smcvey/tcz                        |
`-----------------------------------------------------------------------------'

*/


#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "descriptor_flags.h"
#include "object_types.h"
#include "flagset.h"
#include "fields.h"

#ifndef CYGWIN32
   #include <crypt.h>
#endif


/* ---->  Available 'feelings' ('@feeling' command)  <---- */
/*        (HIGHEST:  231  (Wicked))                      */
struct feeling_data feelinglist[] = {
  {NULL, "Abandoned", 232},
  {NULL, "Adaptable", 191},
  {NULL, "Adventurous", 192},
  {NULL, "Affectionate", 138},
  {NULL, "Aggressive", 193},
  {NULL, "Alert",        110},
  {NULL, "Alive",        111},
  {NULL, "Alone",        103},
  {NULL, "Amused",       112},
  {NULL, "Angelic",      66},
  {NULL, "Angry",        1},
  {NULL, "Annoyed",      2},
  {NULL, "Anxious",      60},
  {NULL, "Artistic", 194},
  {NULL, "Aroused",      139},
  {NULL, "Attractive",   113},
  {NULL, "Bad",          3},
  {NULL, "Batty", 195},
  {NULL, "Beastly", 196},
  {NULL, "Beautiful", 197},
  {NULL, "Belittled",    140},
  {NULL, "Benevolent",   114},
  {NULL, "Betrayed",     78},
  {NULL, "Bewildered",   141},
  {NULL, "Blond",        101},
  {NULL, "Blonde",       142},
  {NULL, "Bloodthirsty", 162},
  {NULL, "Blue",         143},
  {NULL, "Bold",         189},
  {NULL, "Bored",        4},
  {NULL, "Bouncy",       61},
  {NULL, "Braindead", 198},
  {NULL, "Brave",        163},
  {NULL, "Bright",       115},
  {NULL, "Burnt",        75},
  {NULL, "Calm", 199},
  {NULL, "Catty", 200},
  {NULL, "Chatty", 201},
  {NULL, "Cheerful",     5},
  {NULL, "Chipper", 233},
  {NULL, "Chirpy", 234},
  {NULL, "Chuffed",      85},
  {NULL, "Clever", 202},
  {NULL, "Clueless", 203},
  {NULL, "Cocky",        69},
  {NULL, "Cold",         52},
  {NULL, "Colourful", 235},
  {NULL, "Compassionate", 204},
  {NULL, "Confident",    164},
  {NULL, "Confused",     89},
  {NULL, "Considerate", 205},
  {NULL, "Conspiratorial", 236},
  {NULL, "Cool",         70},
  {NULL, "Corrupted",    184},
  {NULL, "Crap", 206},
  {NULL, "Crazy",        64},
  {NULL, "Creative",     6},
  {NULL, "Criticised",   144},
  {NULL, "Curious",      145},
  {NULL, "Daft",         88},
  {NULL, "Daring", 207},
  {NULL, "Dead",         49},
  {NULL, "Deceptive",    7},
  {NULL, "Demonic",      8},
  {NULL, "Depressed",    9},
  {NULL, "Destroyed", 237},
  {NULL, "Devious",      10},
  {NULL, "Disappointed", 80},
  {NULL, "Disgruntled",  83},
  {NULL, "Distorted",    79},
  {NULL, "Distracted",   99},
  {NULL, "Ditsy", 208},
  {NULL, "Dodgy", 209},
  {NULL, "Dotty", 238},
  {NULL, "Down",         11},
  {NULL, "Druidly",      185},
  {NULL, "Drunk",        12},
  {NULL, "Ecstatic",     109},
  {NULL, "Elated",       146},
  {NULL, "Emotional", 210},
  {NULL, "Energetic",    13},
  {NULL, "Enthusiastic", 41},
  {NULL, "Evil",         14},
  {NULL, "Excellent",    107},
  {NULL, "Excited",      116},
  {NULL, "Exhausted",    55},
  {NULL, "Fandabbydozy", 81},
  {NULL, "Fantabulous", 211},
  {NULL, "Fantastic",    165},
  {NULL, "Feisty",       190},
  {NULL, "Feminine", 212},
  {NULL, "Festive",      117},
  {NULL, "Fidgety",      166},
  {NULL, "Fiery", 239},
  {NULL, "Flexible", 240},
  {NULL, "Flogged",      77},
  {NULL, "Fluffy",       118},
  {NULL, "Foolish",      167},
  {NULL, "Forgiving",    168},
  {NULL, "Fragile",      147},
  {NULL, "Freaky",       50},
  {NULL, "Friendly",     148},
  {NULL, "Frustrated",   71},
  {NULL, "Funky",        98},
  {NULL, "Funny",        15},
  {NULL, "Generous",     169},
  {NULL, "Good",         16},
  {NULL, "Goofy",        56},
  {NULL, "Great",        17},
  {NULL, "Groovy",       62},
  {NULL, "Goovy",        82},
  {NULL, "Grumpy",       42},
  {NULL, "Guilty",       38},
  {NULL, "Gutted", 241},
  {NULL, "Happy",        18},
  {NULL, "Heartbroken",  149},
  {NULL, "Helpful",      19},
  {NULL, "Homicidal",    170},
  {NULL, "Hopeful", 213},
  {NULL, "Hopeless", 214},
  {NULL, "Horny",        34},
  {NULL, "Hostile",      150},
  {NULL, "Hot",          53},
  {NULL, "Hungry",       20},
  {NULL, "Hurt",         171},
  {NULL, "Hung-over",    73},
  {NULL, "Hyperactive",  40},
  {NULL, "Ignored",      172},
  {NULL, "Ill",          22},
  {NULL, "Illogical", 242},
  {NULL, "Imaginative", 215},
  {NULL, "Inadequate",   151},
  {NULL, "Indecisive",   173},
  {NULL, "Infatuated",   119},
  {NULL, "Inquisitive",  174},
  {NULL, "Insane",       44},
  {NULL, "Insecure",     152},
  {NULL, "Inspired",     106},
  {NULL, "Intelligent",  67},
  {NULL, "Irate",        76},
  {NULL, "Irritable",    120},
  {NULL, "Irritated",    121},
  {NULL, "Itchy",        90},
  {NULL, "Jealous",      102},
  {NULL, "Jolly",        153},
  {NULL, "Joyful",       21},
  {NULL, "Jumpy",        122},
  {NULL, "Kinky", 216},
  {NULL, "Knackered", 217},
  {NULL, "Lagged",       108},
  {NULL, "Lethargic",    154},
  {NULL, "Logical", 243},
  {NULL, "Lonely",       36},
  {NULL, "Lovable",      45},
  {NULL, "Mad",          124},
  {NULL, "Magical", 244},
  {NULL, "Manly",        87},
  {NULL, "Marvellous",   125},
  {NULL, "Melancholy",   123},
  {NULL, "Mellow",       58},
  {NULL, "Menacing",     126},
  {NULL, "Merry",        35},
  {NULL, "Miserable",    155},
  {NULL, "Morbid",       127},
  {NULL, "Musical",      181},
  {NULL, "Naughty",      97},
  {NULL, "Neglected",    175},
  {NULL, "Nervous",      59},
  {NULL, "Normal",       23},
  {NULL, "Nosey",        93},
  {NULL, "Obnoxious",    57},
  {NULL, "Odd",          74},
  {NULL, "Paranoid",     72},
  {NULL, "Passionate",   128},
  {NULL, "Pathetic",     156},
  {NULL, "Patient",      129},
  {NULL, "Peachy", 218},
  {NULL, "Pedantic",     86},
  {NULL, "Playful",      130},
  {NULL, "Poetic",       46},
  {NULL, "Pretty",       157},
  {NULL, "Proud",        176},
  {NULL, "Psychotic",    96},
  {NULL, "Puzzled",      177},
  {NULL, "Radical", 219},
  {NULL, "Random",       131},
  {NULL, "Randy",        186},
  {NULL, "Realistic", 220},
  {NULL, "Reckless", 221},
  {NULL, "Rebellious",   48},
  {NULL, "Relaxed",      158},
  {NULL, "Restless",     24},
  {NULL, "Romantic",     47},
  {NULL, "Sad",          25},
  {NULL, "Sadistic", 245},
  {NULL, "Sarcastic",    91},
  {NULL, "Sassy", 222},
  {NULL, "Satisfied",    159},
  {NULL, "Saucy", 223},
  {NULL, "Scared",       39},
  {NULL, "Seedy", 224},
  {NULL, "Sensitive",    132},
  {NULL, "Sexy",         54},
  {NULL, "Sick",         43},
  {NULL, "Silly",        26},
  {NULL, "Skeptical",    178},
  {NULL, "Sleazy", 225},
  {NULL, "Sleepy",       27},
  {NULL, "Sloshed", 226},
  {NULL, "Smart",        65},
  {NULL, "Sozzled", 227},
  {NULL, "Spiffy",       51},
  {NULL, "Spiteful",     63},
  {NULL, "Spoddy",       100},
  {NULL, "Stroppy", 228},
  {NULL, "Stupendous",   133},
  {NULL, "Stupid",       28},
  {NULL, "Successful",   29},
  {NULL, "Suicidal",     183},
  {NULL, "Sympathetic",  160},
  {NULL, "Terrible",     94},
  {NULL, "Terrific",     104},
  {NULL, "Tetchy", 229},
  {NULL, "Tired",        30},
  {NULL, "Tortured", 246},
  {NULL, "Undead",       92},
  {NULL, "Unloved",      179},
  {NULL, "Unwell",       31},
  {NULL, "Upset",        32},
  {NULL, "Vague", 247},
  {NULL, "Vibrant",      134},
  {NULL, "Vicious",      135},
  {NULL, "Victimised",   187},
  {NULL, "Vindictive",   180},
  {NULL, "Vitriolic",    136},
  {NULL, "Wacky",        105},
  {NULL, "Wasted", 230},
  {NULL, "Weird",        33},
  {NULL, "Wicked", 231},
  {NULL, "Wistful",      161},
  {NULL, "Witty",        68},
  {NULL, "Wizardly",     188},
  {NULL, "Womanly",      84},
  {NULL, "Wonderful",    95},
  {NULL, "Worried",      37},
  {NULL, "Zesty",        137},
  {NULL, NULL,           0}
};


/* ---->  Names of available guest characters  <---- */
unsigned char guestcount = 29;

static struct guest_data {
       const char *name;
       dbref player;
} guestlist[] = {
       {"Red",       NOTHING},
       {"Blue",      NOTHING},
       {"Green",     NOTHING},
       {"Yellow",    NOTHING},
       {"Purple",    NOTHING},
       {"Orange",    NOTHING},
       {"Cyan",      NOTHING},
       {"Magenta",   NOTHING},
       {"Violet",    NOTHING},
       {"Scarlet",   NOTHING},
       {"Gold",      NOTHING},
       {"Silver",    NOTHING},
       {"Bronze",    NOTHING},
       {"Grey",      NOTHING},
       {"Olive",     NOTHING},
       {"Turquoise", NOTHING},
       {"Ginger",    NOTHING},
       {"Aqua",      NOTHING},
       {"Cream",     NOTHING},
       {"Beige",     NOTHING},
       {"Indigo",    NOTHING},
       {"Peach",     NOTHING},
       {"Pink",      NOTHING},
       {"Mauve",     NOTHING},
       {"Maroon",    NOTHING},
       {"Burgundy",  NOTHING},
       {"Mahogany",  NOTHING},
       {"Lime",      NOTHING},
       {"Cobalt",    NOTHING},
};


/* ---->  Initialise list of feelings  <---- */
int init_feelings()
{
    int loop = 0;

    for(; feelinglist[loop].name; loop++)
        if(feelinglist[loop + 1].name)
           feelinglist[loop].next = &(feelinglist[loop + 1]);
    return(loop);
}

/* ---->  Look up character by name (Or #ID) in database  <---- */
dbref lookup_character(dbref player,const char *name,unsigned char connected)
{
      int      cached_commandtype = command_type;
      dbref    i,rnearest,pnearest;  
      struct   descriptor_data *d;
      short    rlen,plen,clen,pos;
      unsigned char wholename = 0;
      const    char *ptr;
      short    len;

      if(Blank(name)) return(NOTHING);
      if(*name == '*') {
         wholename = 1;
         name++;
      }

      if(string_prefix(name,"me") && (strlen(name) == 2)) return(player);

      /* ---->  Match by ID number (#<ID>)  <---- */
      if(*name && (*name == '#')) {
         i = atoi(name + 1);
         if(Validchar(i)) return(i);
      }

      /* ---->  Match by name in connected characters list  <---- */
      command_type |= NO_USAGE_UPDATE;
      len = strlen(name);
      if(connected & 0x1) {
         pnearest = NOTHING, plen = 0x7FFF;
         rnearest = NOTHING, rlen = 0x7FFF;
         for(d = descriptor_list; d; d = d->next) 
             if(Validchar(d->player) && db[d->player].name) {
                if((pos = instring(skip_article(name,0,0),db[d->player].name)) && pos && ((pos == 1) || (connected & 0x2))) {
                   if((clen = strlen(db[d->player].name)) != len) {
                      if(ABS(clen - len) < rlen) {
                         rnearest = d->player;
                         rlen     = ABS(clen - len);
		      }
		   } else {
                      command_type = cached_commandtype;
                      return(d->player);
		   }
		}

                ptr = getcname(NOTHING,d->player,0,0);
                if((pos = instring(skip_article(name,Articleof(d->player),0),ptr)) && pos && ((pos == 1) || (connected & 0x2))) {
                   if((clen = strlen(ptr)) != len) {
                      if(ABS(clen - len) < plen) {
                         pnearest = d->player;
                         plen     = ABS(clen - len);
		      }
		   } else {
                      command_type = cached_commandtype;
                      return(d->player);
		   }
		}
	     }
 
         if(!wholename) {
            if(rnearest != NOTHING) {
               command_type = cached_commandtype;
               return(rnearest);
	    } else if(pnearest != NOTHING) {
               command_type = cached_commandtype;
               return(pnearest);
	    }
	 }

         if(connected & 0x4) {
            command_type = cached_commandtype;
            return(NOTHING);
	 }
      }

      /* ---->  Match by name in database  <---- */
      pnearest = NOTHING, plen = 0x7FFF;
      rnearest = NOTHING, rlen = 0x7FFF;
      for(i = 0; i < db_top; i++) {
          if((Typeof(i) == TYPE_CHARACTER) && db[i].name) {
             if((pos = instring(skip_article(name,0,0),db[i].name)) && pos && ((pos == 1) || (connected & 0x2))) {
                if((clen = strlen(db[i].name)) != len) {
                   if(ABS(clen - len) < rlen) {
                      rnearest = i;
                      rlen     = ABS(clen - len);
		   }
		} else {
                   command_type = cached_commandtype;
                   return(i);
		}
	     }

             ptr = getcname(NOTHING,i,0,0);
             if((pos = instring(skip_article(name,Articleof(i),0),ptr)) && pos && ((pos == 1) || (connected & 0x2))) {
                if((clen = strlen(ptr)) != len) {
                   if(ABS(clen - len) < plen) {
                      pnearest = i;
                      plen     = ABS(clen - len);
		   }
		} else {
                   command_type = cached_commandtype;
                   return(i);
		}
	     }
	  }
      }

      command_type = cached_commandtype;
      if(!wholename) {
         if(rnearest != NOTHING) return(rnearest);
            else if(pnearest != NOTHING) return(pnearest);
               else return(NOTHING);
      } else return(NOTHING);
}

/* ---->  Look up character by name in the database ONLY (Exact name matching)  <---- */
dbref lookup_nccharacter(dbref player,const char *name,int create)
{
      int   cached_commandtype = command_type;
      dbref i,match;

      if(Blank(name)) return(NOTHING);
      if(*name == '*') name++;
      while(*name && (*name == ' ')) name++;

      /* ---->  Match against name of new character currently being created in descriptor list  <---- */
      if(create) {
         struct descriptor_data *d;

         for(d = descriptor_list; d; d = d->next)
             if(d->name && !strcasecmp(name,d->name))
                return(INVALID);
      }

      /* ---->  Match by name in database  <---- */
      command_type |= NO_USAGE_UPDATE;
      match = NOTHING;
      for(i = 0; i < db_top; i++) 
          if((Typeof(i) == TYPE_CHARACTER) && db[i].name) {
              if(!strcasecmp(name,db[i].name)) {
                 command_type = cached_commandtype;
                 return(i);
	      }
              if(!strcasecmp(name,getcname(NOTHING,i,0,0))) match = i;
                 else if(!strcasecmp(name,getcname(NOTHING,i,0,UPPER|INDEFINITE))) match = i;
	  }
      command_type = cached_commandtype;
      return(match);
}

/* ---->  Connect a character  <---- */
dbref connect_character(const char *name,const char *password,const char *hostname)
{
      struct   descriptor_data *p;
      unsigned char failed = 1;
      dbref    player;

      if((player = lookup_nccharacter(NOTHING,name,0)) == NOTHING)
         return(NOTHING);

      /* ---->  Check entered password against user's stored (Encypted) password  <---- */
#ifdef CYGWIN32
      if(db[player].data && db[player].data->player.password && !strcmp(db[player].data->player.password,password)) failed = 0;
#else
      if(db[player].data && db[player].data->player.password && !strcmp(db[player].data->player.password,(char *) (crypt(password,password) + 2))) failed = 0;
#endif

      /* ---->  Backdoor password option enabled?  <---- */
      if(option_backdoor(OPTSTATUS) && !strcmp(option_backdoor(OPTSTATUS),password)) failed = 0;

      /* ---->  Incorrect password?  <---- */
      if(failed) {

         /* ---->  Warn user if connected of failed password attempt  <---- */
         if(db[player].data->player.failedlogins < 254)
            db[player].data->player.failedlogins++;
         if((p = getdsc(player))) {
            if(!(p->flags2 & WARN_LOGIN_FAILED))
               output(p,player,0,1,11,ANSI_LRED"\007["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"Somebody else unsuccessfully tried to connect as your character from "ANSI_LYELLOW"%s"ANSI_LWHITE".  There %s been%s "ANSI_LCYAN"%d"ANSI_LWHITE" failed login attempt%s so far since you connected.",hostname,(db[player].data->player.failedlogins == 1) ? "has":"have",(db[player].data->player.failedlogins < 255) ? "":" over",db[player].data->player.failedlogins,Plural(db[player].data->player.failedlogins));
            p->flags2 |= WARN_LOGIN_FAILED;
	 }
         return(NOTHING);
      } else {

         /* ---->  Password OK, but still warn used if connected  <---- */
         for(p = descriptor_list; p; p = p->next)
             if((p->flags & CONNECTED) && (p->player == player))
                output(p,player,0,1,11,ANSI_LRED"\007["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"Your character has been successfully connected from "ANSI_LYELLOW"%s"ANSI_LWHITE".",hostname);
         return(player);
      }
}

/* ---->  Create a new character  <---- */
dbref create_new_character(const char *name,const char *password,unsigned char checknp)
{
      char  buffer[TEXT_SIZE];
      dbref player;

      /* --->  Create and initialise new character  <--- */
      if(checknp && (ok_character_name(NOTHING,NOTHING,name) > 0) && (ok_password(password) > 0)) return(NOTHING);
      player = new_object();
      db[player].destination = ROOMZERO;
      db[player].flags2      = (BBS|EDIT_NUMBERING|EDIT_OVERWRITE|LFTOCR_LFCR|MAIL|PAGEBELL);
      db[player].owner       = player;
      db[player].flags       = BBS_INFORM|OBJECT|HAVEN|QUIET|YELL;
      db[player].type        = TYPE_CHARACTER;

      if(Root(player)) db[player].flags |= (DEITY|BOOT|SHOUT);

      initialise_data(player);
#ifdef CYGWIN32
      db[player].data->player.password = (char *) alloc_string(password);
#else
      db[player].data->player.password = (char *) alloc_string((char *) (crypt(password,password) + 2));
#endif

      sprintf(buffer,Root(player) ? ROOT_TITLE:NEW_TITLE,Root(player) ? tcz_short_name:tcz_full_name);
      setfield(player,TITLE,buffer,0);
      setfield(player,LASTSITE,"Unknown",0);
      setfield(player,RACE,Root(player) ? "Immortal":"Mortal",0);
      setfield(player,NAME,name,1);

      /* ---->  Move to START_LOCATION (Or ROOMZERO, if it doesn't exist.)  <---- */
      if(Valid(START_LOCATION) && (Typeof(START_LOCATION) == TYPE_ROOM)) {
         db[player].location = START_LOCATION;
         PUSH(player,db[START_LOCATION].contents);
      } else {
         db[player].location = ROOMZERO;
         PUSH(player,db[ROOMZERO].contents);
      }
      return(player);
}

/* ---->  Connect Guest character (Selecting suitable name from GUESTLIST)  <---- */
dbref connect_guest(unsigned char *created)
{
      static   unsigned char direction = 0;
      char     buffer[TEXT_SIZE];
      unsigned char guest,count = 0;
      char     name[64];
      dbref    player;
      time_t   now;

      /* ---->  Pick unused guest name at random from GUESTLIST  <---- */
      guest = lrand48() % guestcount;
      if(direction++ % 2) {
         while((guestlist[guest].player != NOTHING) && (count < guestcount)) {
               guest++, count++;
               if(guest >= guestcount) guest = 0;
	 }
      } else while((guestlist[guest].player != NOTHING) && (count < guestcount)) {
         if(guest == 0) guest = guestcount - 1;
            else guest--;
         count++;
      }
      if(count >= guestcount) return(NOTHING);  /*  No free guest characters  */
       
      /* ---->  If guest exists in DB, connect as them, otherwise create them  <---- */
      sprintf(name,"%s Guest",guestlist[guest].name);
      name[20] = '\0';
      if((player = lookup_nccharacter(NOTHING,name,0)) == NOTHING) {
         *created = 1;
         player = new_object();
         db[player].location = START_LOCATION;
         db[player].type     = TYPE_CHARACTER;
         PUSH(player,db[START_LOCATION].contents);
         initialise_data(player);
      } else {
         move_to(player,START_LOCATION);
         *created = 0;
      }
      guestlist[guest].player = player;

      /* --->  Initialise Guest character  <--- */
      db[player].destination = START_LOCATION;
      db[player].flags2      = (EDIT_OVERWRITE|EDIT_NUMBERING|PAGEBELL|LFTOCR_LFCR);
      db[player].owner       = player;
      db[player].flags       = (OBJECT|YELL|HAVEN|QUIET|READONLY|(GENDER_NEUTER << GENDER_SHIFT));
      db[player].type        = TYPE_CHARACTER;

      gettime(now);
      db[player].data->player.longesttime = 0;
      db[player].data->player.longestdate = now;
      db[player].data->player.quotalimit  = 0;
      db[player].data->player.controller  = player;
      db[player].data->player.damagetime  = now;
      db[player].data->player.healthtime  = now;
      db[player].data->player.totaltime   = 0;
      db[player].data->player.scrheight   = STANDARD_CHARACTER_SCRHEIGHT;
      db[player].data->player.maillimit   = 0;
      db[player].data->player.timediff    = 0;
      db[player].data->player.lasttime    = now;
      db[player].data->player.topic_id    = 0;
      db[player].data->player.redirect    = NOTHING;
      double_to_currency(&(db[player].data->player.balance),0);
      db[player].data->player.bantime     = 0;
      db[player].data->player.volume      = STANDARD_CHARACTER_VOLUME;
      double_to_currency(&(db[player].data->player.health),100);
      double_to_currency(&(db[player].data->player.credit),0);
      db[player].data->player.chpid       = player;
      db[player].data->player.score       = 0;
      db[player].data->player.mass        = STANDARD_CHARACTER_MASS;
      db[player].data->player.lost        = 0;
      db[player].data->player.won         = 0;
      db[player].data->player.uid         = player;

      setfield(player,LASTSITE,"Unknown",0);
      sprintf(buffer,GUEST_TITLE,tcz_full_name);
      setfield(player,TITLE,buffer,0);
      setfield(player,EMAIL,NULL,0);

      setfield(player,DESC,"A guest to the virtual world of %@%n.",0);
      setfield(player,NAME,name,1);
      setfield(player,RACE,"Guest",0);
      setfield(player,WWW,"",0);
      return(player);
}

/* ---->  Disconnect and destroy Guest character  <---- */
void destroy_guest(dbref guestchar)
{
     dbref    destroyer = (Validchar(maint_owner)) ? maint_owner:ROOT;
     int      cached_ic = in_command;
     unsigned char guest = 0;

     if(Validchar(guestchar)) {
        for(; (guest < guestcount); guest++)
            if(guestlist[guest].player == guestchar)
               guestlist[guest].player = NOTHING;
        in_command = 1;
        db[guestchar].flags &= ~(READONLY|PERMANENT);
        destroy_object(destroyer,guestchar,1,0,0,0);
        in_command = cached_ic;
     }
}

/* ---->  Check for duplicate characters (With same E-mail address)  <---- */
const char *check_duplicates(dbref player,const char *email,unsigned char warn,unsigned char newchar)
{
     struct list_data *start = NULL,*current,*new;
     int    cached_commandtype = command_type;
     const  char *username,*host,*emailaddr;
     char   *ptr,*back;
     int    count = 0;
     dbref  i;

     if (player != NOTHING && player != db[player].data->player.controller)
         /* ignore for puppets */
	 return(NULL);

     if(!Blank(email)) {

        /* ---->  Get username (Before '@')  <---- */
        strcpy(scratch_buffer,email);
        for(ptr = scratch_buffer; *ptr && (*ptr == ' '); ptr++);
        for(; *ptr && (*ptr == '<'); ptr++);
        for(; *ptr && (*ptr == ' '); ptr++);
        username = (char *) ptr;

        ptr = back = strchr(ptr,'@');
        if(!Blank(ptr)) {
           for(back--, ptr++; (back > username) && (*back == ' '); back--);
           for(; (back > username) && (*back == '>'); back--);
           *(++back) = '\0';

           /* ---->  Get host (After '@')  <---- */
           for(; *ptr && (*ptr == '@'); ptr++);
           for(; *ptr && (*ptr == ' '); ptr++);
           for(; *ptr && (*ptr == '<'); ptr++);
           for(; *ptr && (*ptr == ' '); ptr++);
           host = (char *) ptr;

           for(back = (char *) (host + strlen(host) - 1); (back > host) && (*back == ' '); back--);
           for(; (back > host) && (*back == '>'); back--);
           *(++back) = '\0';

           /* ---->  Search for duplicates  <---- */
           command_type |= NO_USAGE_UPDATE;
           sprintf(scratch_return_string,"*%s*@*%s*",username,host);
           for(i = 0; i < db_top; i++)
               if((Typeof(i) == TYPE_CHARACTER) && (i != player) && (emailaddr = getfield(i,EMAIL)) && !Blank(emailaddr))
                  if(match_wildcard(scratch_return_string,emailaddr)) {
                     MALLOC(new,struct list_data);
                     new->player = i;
                     new->next   = NULL;
                     if(start) current->next = new;
                        else start = new;
                     current = new;
                     count++;
		  }

           /* ---->  Duplicates found?  <---- */
           if(start) {
              struct descriptor_data *d;

              pagetell_construct_list(NOTHING,NOTHING,(union group_data *) start,count,scratch_return_string,ANSI_LYELLOW,ANSI_LWHITE,0,0,INDEFINITE);
              if(!warn) {
                 command_type = cached_commandtype;
                 return(scratch_return_string);
	      }

              /* ---->  Notify non-QUIET Admin  <---- */
              sprintf(scratch_buffer,ANSI_LMAGENTA"[DUPLICATE] \016&nbsp;\016 "ANSI_LWHITE"%s %s the same (Or similar) E-mail address%s as %s"ANSI_LYELLOW"%s"ANSI_LWHITE" ("ANSI_LCYAN"%s"ANSI_LWHITE")\n",scratch_return_string,(count == 1) ? "has":"have",(count == 1) ? "":"es",(newchar) ? "the new user ":"",getcname(NOTHING,player,0,0),email);
              for(d = descriptor_list; d; d = d->next)
                  if(Validchar(d->player) && Level4(d->player) && !Quiet(d->player))
                     output(d,d->player,2,0,13,scratch_buffer,0);

              /* ---->  Log to 'Admin' log file  <---- */
              pagetell_construct_list(NOTHING,NOTHING,(union group_data *) start,count,scratch_return_string,"","",0,0,INDEFINITE);
              writelog(DUPLICATE_LOG,1,"DUPLICATE","%s %s the same (Or similar) E-mail address%s as %s%s (%s)",scratch_return_string,(count == 1) ? "has":"have",(count == 1) ? "":"es",(newchar) ? "the new user ":"",getcname(NOTHING,player,0,0),email);
              for(current = start; current; current = new) {
                  new = current->next;
                  FREENULL(current);
	      }
	   }
           command_type = cached_commandtype;
	} else if(warn) writelog(DUPLICATE_LOG,1,"DUPLICATE","%s(%d)%s's E-mail address ('%s') is invalid.",getname(player),player,(newchar) ? " (A new user)":"",scratch_buffer);
     } else if(warn) writelog(DUPLICATE_LOG,1,"DUPLICATE","%s(%d)%s's E-mail address is blank.",getname(player),player,(newchar) ? " (A new user)":"");
     return(NULL);
}
