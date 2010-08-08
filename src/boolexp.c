/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| BOOLEXP.C  -  Implements boolean expressions involving characters, objects  |
|               and flags.  Used for object locks (Things, Exits & Rooms.)    |
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

  $Id: boolexp.c,v 1.3 2005/06/29 20:39:33 tcz_monster Exp $

*/

#include <stdlib.h>
#include <sys/types.h>
#include <ctype.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "object_types.h"
#include "flagset.h"
#include "fields.h"
#include "search.h"
#include "match.h"


/* ---->  Can character satisfy the lock of the specified object (No success/failure messages are displayed.)  <---- */
unsigned char could_satisfy_lock(dbref player,dbref object,short level)
{
	 if(!Validchar(player) || !Valid(object)) return(0);

	 /* ---->  Protection against infinite compound command lock recursion  <---- */
	 if((Level4(Owner(player)) && (level > MAX_CMD_NESTED_ADMIN)) || (!Level4(Owner(player)) && (level > MAX_CMD_NESTED_MORTAL))) return(0);

	 /* ---->  Exit is not linked to a destination room  <---- */
	 if((Typeof(object) == TYPE_EXIT) && !Valid(Destination(object))) return(0);

	 /* ---->  Object is locked against character  <---- */
	 return(((Typeof(object) == TYPE_EXIT) && Openable(object)) ? Open(object):eval_boolexp(player,getlock(object,0),level));
}

/* ---->  Can character satisfy the lock of the specified object (Appropriate success/failure messages are displayed.)  <---- */
unsigned char can_satisfy_lock(dbref player,dbref object,const char *defaultfailuremsg,unsigned char cr)
{
	 if(!Validchar(player) || !Valid(Location(player)) || !Valid(object)) return(0);
   
	 if(!could_satisfy_lock(player,object,0)) {

	    /* ---->  Lock not satisfied  <---- */
	    if(getfield(object,FAIL)) {
	       substitute(player,scratch_buffer,(char *) getfield(object,FAIL),0,ANSI_LCYAN,NULL,0);
	       output(getdsc(player),player,0,1,0,"%s%s",punctuate(scratch_buffer,2,'.'),(cr) ? "\n":"");
	    } else if(defaultfailuremsg) output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s",defaultfailuremsg);

	    if(getfield(object,OFAIL) && (Typeof(object) != TYPE_COMMAND) && !Invisible(db[player].location) && (!Invisible(object) || (Typeof(object) == TYPE_EXIT))) {
	       substitute(player,scratch_buffer,(char *) getfield(object,OFAIL),DEFINITE,ANSI_LCYAN,NULL,0);
	       output_except(Location(player),player,NOTHING,0,1,2,"%s%s",punctuate(scratch_buffer,0,'.'),(cr) ? "\n":"");
	    }
	    return(0);
	 } else {

	    /* ---->  Lock satisfied  <---- */
	    if(getfield(object,SUCC)) {
	       substitute(player,scratch_buffer,(char *) getfield(object,SUCC),0,ANSI_LCYAN,NULL,0);
	       output(getdsc(player),player,0,1,0,"%s%s",punctuate(scratch_buffer,2,'.'),(cr) ? "\n":"");
	    }

	    if(getfield(object,OSUCC) && (Typeof(object) != TYPE_COMMAND) && !Invisible(Location(player)) && (!Invisible(object) || (Typeof(object) == TYPE_EXIT))) {
	       substitute(player,scratch_buffer,(char *) getfield(object,OSUCC),DEFINITE,ANSI_LCYAN,NULL,0);
	       output_except(Location(player),player,NOTHING,0,1,2,"%s%s",punctuate(scratch_buffer,0,'.'),(cr) ? "\n":"");
	    }
	    return(1);
	 }
}

/* ---->  Sanitise boolean expression  <---- */
struct boolexp *sanitise_boolexp(struct boolexp *ptr)
{
       struct boolexp *temp;

       if(!ptr) return(TRUE_BOOLEXP);
       switch(ptr->type) {
              case BOOLEXP_CONST:
                   if(!Valid(ptr->object) || !((Typeof(ptr->object) == TYPE_CHARACTER) || (Typeof(ptr->object) == TYPE_COMMAND) || (Typeof(ptr->object) == TYPE_THING)) || !Original(ptr->object)) {
                      FREENULL(ptr);
                      return(TRUE_BOOLEXP);
		   }
                   return(ptr);
              case BOOLEXP_AND:
              case BOOLEXP_OR:
                   ptr->sub1 = sanitise_boolexp(ptr->sub1);
                   ptr->sub2 = sanitise_boolexp(ptr->sub2);
                   if(ptr->sub1 == TRUE_BOOLEXP) {
                      temp      = ptr->sub2;
                      ptr->sub2 = TRUE_BOOLEXP;
                      FREENULL(ptr);
                      return(temp);
		   } else if(ptr->sub2 == TRUE_BOOLEXP) {
                      temp      = ptr->sub1;
                      ptr->sub1 = TRUE_BOOLEXP;
                      FREENULL(ptr);
                      return(temp);
		   }
                   return(ptr);
              case BOOLEXP_NOT:
                   if((ptr->sub1 = sanitise_boolexp(ptr->sub1)) == TRUE_BOOLEXP) {
                      FREENULL(ptr);
                      return(TRUE_BOOLEXP);
		   }
                   return(ptr);
              case BOOLEXP_FLAG:
                   return(ptr);
              default:
                   free_boolexp(&ptr);
                   return(TRUE_BOOLEXP);
       }
       return(TRUE_BOOLEXP);
}

/* ---->  Evaluate boolean expression with respect to given character  <---- */
int eval_boolexp(dbref player,struct boolexp *b,int level)
{
    dbref cached_owner,cached_chpid;

    if(!b || (b == TRUE_BOOLEXP)) return(1);
       else switch(b->type) {
          case BOOLEXP_AND:
               return(eval_boolexp(player,b->sub1,level + 1) && eval_boolexp(player,b->sub2,level + 1));
          case BOOLEXP_OR:
               return(eval_boolexp(player,b->sub1,level + 1) || eval_boolexp(player,b->sub2,level + 1));
          case BOOLEXP_NOT:
               return(!eval_boolexp(player,b->sub1,level + 1));
          case BOOLEXP_CONST:
               if(Typeof(b->object) != TYPE_COMMAND)
                  return((b->object == Owner(player)) || contains(b->object,player));
               if((!could_satisfy_lock(player,b->object,level + 1)) || Invisible(b->object)) return(0);
                  else {
                     cached_owner     = Owner(player);
                     cached_chpid     = db[player].data->player.chpid;
                     db[player].owner = db[player].data->player.chpid = Owner(b->object);
                     command_nesting_level++;
                     command_cache_execute(player,b->object,1,1);
                     if((command_nesting_level > 0) && ((command_nesting_level <= MAX_CMD_NESTED_MORTAL) || ((command_nesting_level <= MAX_CMD_NESTED_ADMIN) && Level4(Owner(player))))) command_nesting_level--;
                     db[player].owner              = cached_owner;
                     db[player].data->player.chpid = cached_chpid;
                     if(!((command_boolean == COMMAND_SUCC) || (command_boolean == COMMAND_FAIL))) {
                        writelog(BUG_LOG,1,"BUG","Compound command %s(#%d) in lock returned invalid boolean (%d.)",getname(b->object),b->object,command_boolean);
                        return(0);
		     } else return(command_boolean);
		  }
          case BOOLEXP_FLAG:
               switch(b->object) {
                      case BOOLFLAG_MALE:
                           return(Genderof(player) == GENDER_MALE);
                      case BOOLFLAG_FEMALE:
                           return(Genderof(player) == GENDER_FEMALE);
                      case BOOLFLAG_NEUTER:
                           return(Genderof(player) == GENDER_NEUTER);
                      case BOOLFLAG_UNSET:
                           return(Genderof(player) == GENDER_UNASSIGNED);
                      case BOOLFLAG_MORON:
                           return(Moron(Owner(player)));
                      case BOOLFLAG_BEING:
                           return(Being(Owner(player)));
                      case BOOLFLAG_BUILDER:
                           return(Builder(Owner(player)));
                      case BOOLFLAG_ASSISTANT:
                           return(Assistant(Owner(player)));
                      case BOOLFLAG_EXPERIENCED:
                           return(Experienced(Owner(player)));
                      case BOOLFLAG_DRUID:
                           return(Druid(Owner(player)));
                      case BOOLFLAG_APPRENTICE:
                           return(Level4(Owner(player)));
                      case BOOLFLAG_WIZARD:
                           return(Level3(Owner(player)));
                      case BOOLFLAG_ELDER:
                           return(Level2(Owner(player)));
                      case BOOLFLAG_DEITY:
                           return(Level1(Owner(player)));
                      case BOOLFLAG_ANSI:
                           return(Ansi(player));
                      case BOOLFLAG_ASHCAN:
                           return(Ashcan(player));
                      case BOOLFLAG_BBS:
                           return(Bbs(player));
                      case BOOLFLAG_BBS_INFORM:
                           return(Bbsinform(player));
                      case BOOLFLAG_CENSOR:
                           return(Censor(player));
                      case BOOLFLAG_CHAT_OPERATOR:
                           return((db[player].flags2 & CHAT_OPERATOR) != 0);
                      case BOOLFLAG_CONNECTED:
                           return(Connected(player));
                      case BOOLFLAG_ECHO:
                           return(Echo(player));
                      case BOOLFLAG_EDIT_EVALUATE:
                           return((db[player].flags2 & EDIT_EVALUATE) != 0);
                      case BOOLFLAG_EDIT_OVERWRITE:
                           return((db[player].flags2 & EDIT_OVERWRITE) != 0);
                      case BOOLFLAG_ENGAGED:
                           return(Engaged(player));
                      case BOOLFLAG_FRIENDS_INFORM:
                           return(FriendsInform(player));
                      case BOOLFLAG_HAVEN:
                           return(Haven(player));
                      case BOOLFLAG_HTML:
                           return(Html(player));
                      case BOOLFLAG_INHERITABLE:
                           return(Inheritable(player));
                      case BOOLFLAG_LFTOCR_CR:
                           return((db[player].flags2 & LFTOCR_MASK) == LFTOCR_CR);
                      case BOOLFLAG_LFTOCR_CRLF:
                           return((db[player].flags2 & LFTOCR_MASK) == LFTOCR_CRLF);
                      case BOOLFLAG_LFTOCR_LFCR:
                           return((db[player].flags2 & LFTOCR_MASK) == LFTOCR_LFCR);
                      case BOOLFLAG_LISTEN:
                           return(Listen(player));
                      case BOOLFLAG_PAGEBELL:
                           return(Pagebell(player));
                      case BOOLFLAG_PERMANENT:
                           return(Permanent(player));
                      case BOOLFLAG_PRIVATE:
                           return((db[player].flags & PRIVATE) != 0);
                      case BOOLFLAG_MAIL:
                           return(Mail(player));
                      case BOOLFLAG_MARRIED:
                           return(Married(player));
                      case BOOLFLAG_MORE:
                           return(More(player));
                      case BOOLFLAG_NUMBER:
                           return(Number(player));
                      case BOOLFLAG_QUIET:
                           return(Quiet(player));
                      case BOOLFLAG_READONLY:
                           return(Readonly(player));
                      case BOOLFLAG_READONLYDESC:
                           return(Readonlydesc(player));
                      case BOOLFLAG_RETIRED:
                           return(Retired(player));
                      case BOOLFLAG_SECRET:
                           return(Secret(player));
                      case BOOLFLAG_TRACING:
                           return(Tracing(player));
                      case BOOLFLAG_UNDERLINE:
                           return(Underline(player));
                      case BOOLFLAG_VISIBLE:
                           return(Visible(player));
                      case BOOLFLAG_YELL:
                           return(Yell(player));
                      case BOOLFLAG_ARTICLE_CONSONANT:
                           return(Articleof(player) == ARTICLE_CONSONANT);
                      case BOOLFLAG_ARTICLE_VOWEL:
                           return(Articleof(player) == ARTICLE_VOWEL);
                      case BOOLFLAG_ARTICLE_PLURAL:
                           return(Articleof(player) == ARTICLE_PLURAL);
                      default:
                           return(0);
	       }
          default:
               return(0);
       }
}


static char *parsebuf;
static dbref parse_character;


/* ---->  Skip over whitespace in PARSEBUF (Blank spaces)  <---- */
static void skip_whitespace()
{
       while(*parsebuf && (*parsebuf == ' ')) parsebuf++;
}


static struct boolexp *parse_boolexp_E();  /*  Defined below  */


/* ---->  F -> (E); F -> !F; F -> @F; F -> object identifier  <---- */
static struct boolexp *parse_boolexp_F()
{
       struct   boolexp *b;
       unsigned char pos;
       char     *p;

       skip_whitespace();
       switch(*parsebuf) {
              case '(':
                   parsebuf++;
                   b = parse_boolexp_E();
                   skip_whitespace();
                   if((b == TRUE_BOOLEXP) || (*parsebuf++ != ')')) {
                      free_boolexp(&b);
                      return(TRUE_BOOLEXP);
                   } else return(b);
              case NOT_TOKEN:
                   parsebuf++;
                   MALLOC(b,struct boolexp);
                   b->object = NOTHING;
                   b->type   = BOOLEXP_NOT;
                   b->sub1   = parse_boolexp_F();
                   b->sub2   = NULL;
                   if(b->sub1 == TRUE_BOOLEXP) {
                      FREENULL(b);
                      return(TRUE_BOOLEXP);
                   } else return(b);
              case COMMAND_TOKEN:

                   /* ---->  Lock to flag?  <---- */
                   p = boolexp_buf;
                   parsebuf++;
                   while(*parsebuf && (*parsebuf != AND_TOKEN) && (*parsebuf != OR_TOKEN) && (*parsebuf != ')')) *p++ = *parsebuf++;
     
                   /* --->  Strip trailing whitespace  <--- */
                   *p-- = '\0';
                   while(*p == ' ') *p-- = '\0';

                   MALLOC(b,struct boolexp);
                   b->object = NOTHING;
                   b->type   = BOOLEXP_FLAG;
                   b->sub1   = NULL;
                   b->sub2   = NULL;

                   /* ---->  Match flag with primary flags  <---- */
                   for(pos = 0; flag_list[pos].string; pos++)
                       if(flag_list[pos].boolflag && string_prefix(boolexp_buf,flag_list[pos].string)) {
                          b->object = flag_list[pos].boolflag;
                          return(b);
		       }

                   /* ---->  Match flag with secondary flags  <---- */
                   for(pos = 0; flag_list2[pos].string; pos++)
                       if(flag_list2[pos].boolflag && string_prefix(boolexp_buf,flag_list2[pos].string)) {
                          b->object = flag_list2[pos].boolflag;
                          return(b);
		       }

                   /* ---->  If we've got this far, flag name must either be invalid or not allowed in lock  <---- */
                   output(getdsc(parse_character),parse_character,0,1,0,ANSI_LGREEN"Sorry, the flag '"ANSI_LWHITE"%s"ANSI_LGREEN"' is either invalid or can't be used in a lock specification.",boolexp_buf);
                   FREENULL(b);
                   return(TRUE_BOOLEXP);
              default:

                   /* ---->  Lock to object/compound command DBref?  <---- */
                   p = boolexp_buf;
                   while(*parsebuf && (*parsebuf != AND_TOKEN) && (*parsebuf != OR_TOKEN) && (*parsebuf != ')')) *p++ = *parsebuf++;

                   /* --->  Strip trailing whitespace  <--- */
                   *p-- = '\0';
                   while(*p == ' ') *p-- = '\0';

                   MALLOC(b,struct boolexp);
                   b->type = BOOLEXP_CONST;
                   b->sub1 = NULL;
                   b->sub2 = NULL;

                   b->object = match_preferred(parse_character,parse_character,boolexp_buf,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT|MATCH_OPTION_RESTRICTED);
                   if(Valid(b->object))
                      switch(Typeof(b->object)) {

                             /* ---->  Ensure object can be part of a lock  <---- */
                             case TYPE_THING:
                             case TYPE_COMMAND:
                             case TYPE_CHARACTER:
                                  return(b);
                             default:
                                  output(getdsc(parse_character),parse_character,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"%s"ANSI_LGREEN"' can't be part of a lock.",boolexp_buf);
                                  FREENULL(b);
                                  return(TRUE_BOOLEXP);
                      }
       }
       /* Never gets here due to default, this is just to keep -Wall happy */
       return(TRUE_BOOLEXP);
}

/* --->  T -> F; T -> F & T <---- */
static struct boolexp *parse_boolexp_T()
{
       struct boolexp *b,*b2;

       if((b = parse_boolexp_F()) == TRUE_BOOLEXP) return(b);
          else {
             skip_whitespace();
             if(*parsebuf == AND_TOKEN) {
                parsebuf++;

                MALLOC(b2,struct boolexp);
                b2->object = NOTHING;
                b2->type   = BOOLEXP_AND;
                b2->sub1   = b;
                if((b2->sub2 = parse_boolexp_T()) == TRUE_BOOLEXP) {
                   free_boolexp(&b2);
                   return(TRUE_BOOLEXP);
                } else return(b2);
	     } else return(b);
	  }
}

/* ---->  E -> T; E -> T | E  <---- */
static struct boolexp *parse_boolexp_E()
{
       struct boolexp *b,*b2;

       if((b = parse_boolexp_T()) == TRUE_BOOLEXP) return(b);
          else {
             skip_whitespace();
             if(*parsebuf == OR_TOKEN) {
                parsebuf++;

                MALLOC(b2,struct boolexp);
                b2->type = BOOLEXP_OR;
                b2->sub1 = b;
                if((b2->sub2 = parse_boolexp_E()) == TRUE_BOOLEXP) {
                   free_boolexp(&b2);
                   return(TRUE_BOOLEXP);
                } else return(b2);
	     } else return(b);
	  }
}

/* ---->  Parse given boolean expression (As string) (For storage as 'struct boolexp')  <---- */
struct boolexp *parse_boolexp(dbref player,char *buf)
{
    parsebuf        = buf;
    parse_character = player;
    return(parse_boolexp_E());
}
