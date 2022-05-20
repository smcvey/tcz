/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| QUERY.C  -  Implements {@?query} commands.                                  |
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


#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#include "structures.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "descriptor_flags.h"
#include "friend_flags.h"
#include "object_types.h"
#include "objectlists.h"
#include "flagset.h"
#include "search.h"
#include "fields.h"
#include "match.h"
#include "quota.h"


/* ---->  Buffer for return of query result  <---- */
char querybuf[BUFFER_LEN];


/* ---->  Find object to perform query command on  <---- */
dbref query_find_object(dbref player,const char *name,int preferred,unsigned char need_control,unsigned char private)
{
      dbref thing;

      setreturn(ERROR,COMMAND_FAIL);
      if(Blank(name)) return(player);
      thing = match_preferred(player,player,name,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,preferred,MATCH_OPTION_DEFAULT);
      if(!Valid(thing)) return(NOTHING);

      if((need_control & 0x1) && !can_read_from(player,thing)) {
         if(!Level4(db[player].owner)) output(getdsc(player),player,0,1,0,(Typeof(thing) == TYPE_CHARACTER) ? ANSI_LGREEN"Sorry, you may only perform that query command on yourself or one of your puppets.":ANSI_LGREEN"Sorry, you may only perform that query command on an object owned by yourself or one of your puppets.");
            else output(getdsc(player),player,0,1,0,(Typeof(thing) == TYPE_CHARACTER) ? ANSI_LGREEN"Sorry, you may only perform that query command on a character who's of a lower level than yourself.":ANSI_LGREEN"Sorry, you may only perform that query command on an object owned by a character who's of a lower level than yourself.");
         return(NOTHING);
      } else if(!((!Private(thing) || !private || (need_control & 0x2)) || can_write_to(player,thing,1))) {
         if(Typeof(thing) != TYPE_CHARACTER) {
            sprintf(querybuf,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is set "ANSI_LYELLOW"PRIVATE"ANSI_LGREEN"  -  Only characters above the level of its owner (",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0));
            output(getdsc(player),player,0,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LGREEN") may perform query commands on it.",querybuf,Article(db[thing].owner,LOWER,INDEFINITE),getcname(player,db[thing].owner,1,0));
         } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is set "ANSI_LYELLOW"PRIVATE"ANSI_LGREEN"  -  Only characters of a higher level may perform query commands on them.",Article(thing,LOWER,DEFINITE),getcname(player,thing,1,0));
      } else return(thing);
      return(NOTHING);
}

/* ---->  Find character to perform query on  <---- */
/*        (PERMISSION:  0 = No permission, 1 = Read permission.)  */
dbref query_find_character(dbref player,const char *name,unsigned char permission)
{
      dbref character;

      setreturn(ERROR,COMMAND_FAIL);
      if(!Blank(name)) {
         character = lookup_character(player,name,1);
         if(Validchar(character) && permission && !can_read_from(player,character)) {
            output(getdsc(player),player,0,1,0,!Level4(db[player].owner) ? ANSI_LGREEN"Sorry, you may only perform that query command on yourself or one of your puppets.":ANSI_LGREEN"Sorry, you may only perform that query command on a character who's of a lower level than yourself.");
            return(NOTHING);
	 } else if(!Validchar(character)) {
            output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",name);
            return(NOTHING);
	 } else return(character);
      } else character = player;
      return(character);
}

/* ---->  Return area that room is in  <---- */
void query_address(CONTEXT)
{
     struct str_ops str_data;
     int    found = 0;
     dbref  thing;

     thing = query_find_object(player,params,SEARCH_PREFERRED,0,0);
     if(Valid(thing)) {
        str_data.length = 0;
        str_data.dest   = querybuf;
        for(thing = db[thing].location; Valid(thing); thing = Location(thing), found++) {
            if(found > 0) strcat_limits(&str_data,", ");
            if(Secret(thing) && !can_write_to(player,thing,1)) {
               strcat_limits(&str_data,"Secret Location");
               thing = NOTHING;
	    } else strcat_limits(&str_data,getcname(player,thing,1,(found > 0) ? LOWER|INDEFINITE:UPPER|INDEFINITE));
	}
        *str_data.dest = '\0';
        setreturn(querybuf,COMMAND_SUCC);
     }
}

/* ---->  Returns whether object is within the specified area  <---- */
void query_area(CONTEXT)
{
     dbref area,thing;

     thing = query_find_object(player,arg1,SEARCH_PREFERRED,0,0);
     if(!Valid(thing)) return;
     area = query_find_object(player,arg2,SEARCH_PREFERRED,0,0);
     if(!Valid(area)) return;
     if(in_area(thing,area)) setreturn(OK,COMMAND_SUCC);
}

/* ---->  Return name of area given object is in  <---- */
void query_areaname(CONTEXT)
{
     const char *ptr;

     dbref thing = query_find_object(player,params,SEARCH_PREFERRED,0,0);
     if(!Valid(thing) || !HasField(thing,AREANAME)) return;
     thing = get_areaname_loc(thing);
     if(Valid(thing)) {
        if(!(ptr = getfield(thing,AREANAME))) ptr = NOTHING_STRING;
        setreturn(ptr,COMMAND_SUCC);
     } else setreturn(NOTHING_STRING,COMMAND_SUCC);
}

/* ---->  Return #<ID> of area given object is in  <---- */
void query_areanameid(CONTEXT)
{
     dbref thing = query_find_object(player,params,SEARCH_PREFERRED,0,0);
     if(!Valid(thing) || !HasField(thing,AREANAME)) return;
     thing = get_areaname_loc(thing);
     if(Valid(thing)) setreturn(getnameid(player,thing,NULL),COMMAND_SUCC);
        else setreturn(NOTHING_STRING,COMMAND_SUCC);
}

/* ---->  {J.P.Boggis 08/07/2000}  Return character's average time spent active  <---- */
void query_averageactive(CONTEXT)
{
     time_t total,idle,active,now;
     struct descriptor_data *w;

     dbref character = query_find_character(player,params,0);
     if(!Validchar(character)) return;
     w = getdsc(character);

     gettime(now);
     total = db[character].data->player.totaltime;
     if(Connected(character)) total += (now - db[character].data->player.lasttime);

     idle    = db[character].data->player.idletime;
     if(Connected(character) && w) idle += (now - w->last_time);
     active  = total - idle;
     active /= ((db[character].data->player.logins > 1) ? db[character].data->player.logins:1);

     if(active > 0) sprintf(querybuf,"%d",(int) active);
        else strcpy(querybuf,"0");
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  {J.P.Boggis 08/07/2000}  Return character's average time spent idling  <---- */
void query_averageidle(CONTEXT)
{
     struct descriptor_data *w;
     time_t total,idle,now;

     dbref character = query_find_character(player,params,0);
     if(!Validchar(character)) return;
     w = getdsc(character);

     gettime(now);
     total = db[character].data->player.totaltime;
     if(Connected(character)) total += (now - db[character].data->player.lasttime);

     idle  = db[character].data->player.idletime;
     if(Connected(character) && w) idle += (now - w->last_time);
     idle /= ((db[character].data->player.logins > 1) ? db[character].data->player.logins:1);

     if(idle > 0) sprintf(querybuf,"%d",(int) idle);
        else strcpy(querybuf,"0");
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  {J.P.Boggis 02/07/2000}  Return average time between character's logins  <---- */
void query_averagelogins(CONTEXT)
{
     time_t average,now;

     dbref character = query_find_character(player,params,0);
     if(!Validchar(character)) return;

     gettime(now);
     average = ((now - db[character].created) / ((db[character].data->player.logins > 1) ? db[character].data->player.logins:1));
     sprintf(querybuf,"%d",(int) average);
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  {J.P.Boggis 02/07/2000}  Return character's average time connected  <---- */
void query_averagetime(CONTEXT)
{
     time_t total,now;

     dbref character = query_find_character(player,params,0);
     if(!Validchar(character)) return;

     gettime(now);
     total = db[character].data->player.totaltime;
     if(Connected(character)) total += (now - db[character].data->player.lasttime);
     total = (total / ((db[character].data->player.logins > 1) ? db[character].data->player.logins:1));

     if(total > 0) sprintf(querybuf,"%d",(int) total);
        else strcpy(querybuf,"0");
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  {J.P.Boggis 07/03/2000}  Boolean value ('True', 'Yes', 'OK' or '1' all return success, everything else failure  <---- */
void query_boolean(CONTEXT)
{
     struct arg_data arg;

     setreturn(ERROR,COMMAND_FAIL);
     unparse_parameters(params,1,&arg,0);
     if(!Blank(arg.text[0])) {
        if(string_prefix("True",arg.text[0]) || string_prefix("Yes",arg.text[0]) || string_prefix(OK,arg.text[0]) || string_prefix("1",arg.text[0]))
           setreturn(OK,COMMAND_SUCC);
     }
}

/* ---->  {@?censor "<STRING>"}  <---- */
void query_censor(CONTEXT)
{
     struct arg_data arg;

     unparse_parameters(params,1,&arg,0);
     if(arg.count < 1) setreturn("",COMMAND_SUCC);
        else setreturn(bad_language_filter(querybuf,arg.text[0]),COMMAND_SUCC);
}

/* ---->  Return contents/exits string of room/thing  <---- */
/*        (VAL1:  0 = Cstring, 1 = Estring.)                */
void query_cestring(CONTEXT)
{
     dbref thing = query_find_object(player,params,SEARCH_PREFERRED,0,0);
     int   query_field;

     query_field = (val1) ? ESTRING:CSTRING;
     
     if(!HasField(thing,query_field)) return;
     if(Blank(getfield(thing,query_field))) {
        setreturn((val1) ? "Obvious exits:":"Contents:",COMMAND_SUCC);
        return;
     } else setreturn(String(getfield(thing,query_field)),COMMAND_SUCC);
}

/* ---->  {J.P.Boggis 07/03/2000}  Text character classification  <---- */
/*        VAL1:  0 = {@?alpha}    -  Alphabetical (a-z, A-Z)  */
/*               1 = {@?digit}    -  Digit (0-9)      */
/*               2 = {@?alnum}    -  Alphabetical (a-z, A-Z) or digit (0-9)  */
/*               3 = {@?punct}    -  Punctuation      */
/*               4 = {@?islower}  -  Lowercase (a-z)  */
/*               5 = {@?isupper}  -  Uppercase (A-Z)  */
void query_charclass(CONTEXT)
{
     struct arg_data arg;
     const  char *ptr;

     setreturn(ERROR,COMMAND_FAIL);
     unparse_parameters(params,1,&arg,0);
     strcpy(querybuf,arg.text[0]);
     if(Blank(arg.text[0])) return;

     switch(val1) {
            case 0:

                 /* ---->  Alphabetical (a-z, A-Z)  <---- */
                 for(ptr = arg.text[0]; *ptr && isalpha(*ptr); ptr++);
                 if(!*ptr) setreturn(querybuf,COMMAND_SUCC);
                 break;
            case 1:

                 /* ---->  Digit (0-9)  <---- */
                 for(ptr = arg.text[0]; *ptr && isdigit(*ptr); ptr++);
                 if(!*ptr) setreturn(querybuf,COMMAND_SUCC);
                 break;
            case 2:

                 /* ---->  Alphabetical (a-z, A-Z) or digit (0-9)  <---- */
                 for(ptr = arg.text[0]; *ptr && isalnum(*ptr); ptr++);
                 if(!*ptr) setreturn(querybuf,COMMAND_SUCC);
                 break;
            case 3:

                 /* ---->  Punctuation  <---- */
                 for(ptr = arg.text[0]; *ptr && ispunct(*ptr); ptr++);
                 if(!*ptr) setreturn(querybuf,COMMAND_SUCC);
                 break;
            case 4:

                 /* ---->  Lowercase (a-z)  <---- */
		 for(ptr = arg.text[0];
		     *ptr && (!isalpha(*ptr) || islower(*ptr)); ptr++);
                 if(!*ptr) setreturn(querybuf,COMMAND_SUCC);
                 break;
            case 5:

                 /* ---->  Uppercase (A-Z)  <---- */
		 for(ptr = arg.text[0];
		     *ptr && (!isalpha(*ptr) || isupper(*ptr)); ptr++);
                 if(!*ptr) setreturn(querybuf,COMMAND_SUCC);
                 break;
     }
}

/* ---->  Is specified character connected?  <---- */
void query_connected(CONTEXT)
{
     dbref character = query_find_character(player,params,0);
     if(!Validchar(character)) return;
     setreturn(Connected(character) ? OK:ERROR,COMMAND_SUCC);
}

/* ---->  Return controller of a puppet  <---- */
void query_controller(CONTEXT)
{
     dbref character = query_find_character(player,params,0);
     if(!Validchar(character)) return;
     setreturn(getnameid(player,Controller(character),NULL),COMMAND_SUCC);
}

/* ---->  Return creation or last usage date and time of object  <---- */
/*        (val1:  0 = Creation date, 1 = Last usage date.)             */
void query_created_or_lastused(CONTEXT)
{
     dbref thing = query_find_object(player,arg1,SEARCH_PREFERRED,0,1);
     if(!Valid(thing)) return;
     sprintf(querybuf,"%ld",(!Blank(arg2) && string_prefix("longdates",arg2)) ? epoch_to_longdate((val1) ? db[thing].lastused:db[thing].created):((val1) ? db[thing].lastused:db[thing].created));
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Return character's credit in pocket, bank balance or payment restriction  <---- */
/*        (VAL1:  0 = Pocket/Location, 1 = Balance, 2 = Restriction.)  */
void query_credit(CONTEXT)
{
     dbref object;

     if(val1 > 0) {
        object = query_find_character(player,params,0);
        if(!Validchar(object)) return;
     } else {
        object = query_find_object(player,arg1,SEARCH_PREFERRED,0,0);
        if(!Valid(object)) return;
     }

     switch(val1) {
            case 0:
                 switch(Typeof(object)) {
                        case TYPE_CHARACTER:
                             sprintf(querybuf,"%.2f",currency_to_double(&(db[object].data->player.credit)));
                             break;
                        case TYPE_ROOM:
                             sprintf(querybuf,"%.2f",currency_to_double(&(db[object].data->room.credit)));
                             break;
                        case TYPE_THING:
                             sprintf(querybuf,"%.2f",currency_to_double(&(db[object].data->thing.credit)));
                             break;
                        default:
                             return;
		 }
                 break;
            case 1:
                 sprintf(querybuf,"%.2f",currency_to_double(&(db[object].data->player.balance)));
                 break;
            case 2:
                 sprintf(querybuf,"%d",db[object].data->player.restriction);
                 break;
     }
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Convert date/time in string form to either EPOCH or LONGDATE format integer date/time representation  <---- */
void query_datetime(CONTEXT)
{
     unsigned char epoch = 1,datetime = 0,invalid;
     char          *start,*ptr;

     if(!Blank(arg2)) {
        for(ptr = arg2; *ptr && (*ptr == ' '); ptr++);
        while(*ptr) {
              for(start = ptr = (char *) arg2; *ptr && (*ptr != ' '); ptr++);
              if(*ptr) for(*ptr++ = '\0'; *ptr && (*ptr == ' '); ptr++);
              if(!Blank(start)) {
                 if(string_prefix("longdates",start)) epoch = 0;
                    else if(string_prefix("epoch",start)) epoch = 1;
                       else if(string_prefix("dates",start)) datetime = 1;
                          else if(string_prefix("times",start)) datetime = 2;
	      }
	}
     }

     if(epoch) {
        time_t date;

        date = string_to_date(player,arg1,1,datetime,&invalid);
        if(invalid) {
           setreturn(INVALID_DATE,COMMAND_FAIL);
           return;
	}
        sprintf(querybuf,"%d",(int) date);
     } else {
        unsigned long longdate;
 
        longdate = string_to_date(player,arg1,0,datetime,&invalid);
        if(invalid) {
           setreturn(INVALID_DATE,COMMAND_FAIL);
           return;
	}
        sprintf(querybuf,"%ld",longdate);
     }
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  {@?delete <NUMBER> "<SEPARATOR>" "<LIST>"}  <---- */
void query_delete(CONTEXT)
{
     char              temp[TEXT_SIZE],separator[TEXT_SIZE],item[TEXT_SIZE];
     unsigned char     last = 0,skip = 1;
     short             loop = 0,loop2;
     char              *ptr,*tmp;
     struct   arg_data arg;

     unparse_parameters(params,3,&arg,1);
     if(!((arg.count < 3) || ((arg.numb[0] <= 0) && (arg.numb[0] != LAST)) || Blank(arg.text[1]) || Blank(arg.text[2]))) {
        ptr = arg.text[2], *temp = '\0';
        if(arg.numb[0] == LAST) last = 1;
        while(*ptr) {

              /* ---->  Get separator  <---- */
              loop++, tmp = separator;
              if(*ptr && !skip && !strncasecmp(ptr,arg.text[1],arg.len[1]))
                 for(loop2 = 0; loop2 < arg.len[1]; loop2++)
                     if(*ptr) *tmp++ = *ptr++;
 	      *tmp = '\0';

              /* ---->  Get item  <---- */
              for(tmp = item; *ptr && strncasecmp(ptr,arg.text[1],arg.len[1]); *tmp++ = *ptr++);
	      *tmp = '\0';

              /* ---->  Delete item?  <---- */
              if((last && *ptr) || (!last && (loop != arg.numb[0]))) {
                 if(!BlankContent(separator) && !((arg.numb[0] == 1) && (loop == 2))) strcat(temp,separator);
                 if(!BlankContent(item)) strcat(temp,item);
	      }
              skip = 0;
	}
        setreturn(temp,COMMAND_SUCC);
     } else setreturn(arg.text[2],COMMAND_SUCC);
}

/* ---->  Return description of an object  <---- */
void query_description(CONTEXT)
{
     dbref thing = query_find_object(player,params,SEARCH_PREFERRED,0,1);
     if(!Valid(thing) || (((Typeof(thing) == TYPE_ARRAY) && ((elementfrom == NOTHING) || (elementfrom == INVALID))) || ((Typeof(thing) != TYPE_ARRAY) && !HasField(thing,DESC)))) return;
     if((Typeof(thing) == TYPE_COMMAND) && !can_read_from(player,thing)) return;

     if(Typeof(thing) == TYPE_ARRAY) {
        if(array_subquery_elements(player,thing,elementfrom,elementto,querybuf) < 1) return;
        setreturn(querybuf,COMMAND_SUCC);
     } else setreturn(String(getfield(thing,DESC)),COMMAND_SUCC);
}

/* ---->  Return destination of an exit, drop-to location of a room or home location of an object  <---- */
void query_destination(CONTEXT)
{
     dbref thing = query_find_object(player,params,SEARCH_PREFERRED,1,1);
     if(!Valid(thing) || !HasField(thing,DESTINATION)) return;
     setreturn(getnameid(player,db[thing].destination,NULL),COMMAND_SUCC);
}

/* ---->  Return drop message of an object  <---- */
void query_drop(CONTEXT)
{
     dbref thing = query_find_object(player,params,SEARCH_PREFERRED,1,1);
     if(!Valid(thing) || (Typeof(thing) == TYPE_CHARACTER) || !HasField(thing,DROP)) return;
     setreturn(String(getfield(thing,DROP)),COMMAND_SUCC);
}

/* ---->  Return character's E-mail address  <---- */
void query_email(CONTEXT)
{
     unsigned char all = 0;
     const    char *email;
     int           number;

     dbref character = query_find_character(player,arg1,0);
     if(!Validchar(character) || !HasField(character,EMAIL)) return;
     if(!Blank(arg2)) {
        if(!((!strcasecmp("all",arg2) && (all = 1)) || (!strcasecmp("public",arg2) && (number = 1)) || (!strcasecmp("private",arg2) && (number = 2)) || ((number = atol(arg2)) && (number > 0) && (number <= EMAIL_ADDRESSES))))  return;
     } else number = 2;

     if((number == 2) && !Level4(db[player].owner) && !can_write_to(player,character,1)) return;
     if((command_type & QUERY_SUBSTITUTION) && (all || (number == 2))) return;
     if(!all) {
        email = gettextfield(number,'\n',getfield(character,EMAIL),0,querybuf);
        if(Blank(email)) return;
     } else {
        if(!Level4(db[player].owner) && !can_write_to(player,character,1))
           email = settextfield("",2,'\n',getfield(character,EMAIL),querybuf);
              else email = getfield(character,EMAIL);
        if(Blank(email)) return;
     }
     setreturn(email,COMMAND_SUCC);
}

/* ---->  Evaluate given lock  <---- */
void query_evallock(CONTEXT)
{
     struct boolexp *key;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(params)) {
        if(strcasecmp(params,"*UNLOCKED*")) {
           if((key = parse_boolexp(player,(char *) params)) != TRUE_BOOLEXP)
              if(eval_boolexp(player,key,0)) setreturn(OK,COMMAND_SUCC);
           free_boolexp(&key);
        } else setreturn(OK,COMMAND_SUCC);
     }
}

/* ---->  Does specified object/dynamic array element exist?  <---- */
void query_exists(CONTEXT)
{
     dbref thing;

     setreturn(ERROR,COMMAND_FAIL);
     thing = match_preferred(player,player,params,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT & ~MATCH_OPTION_NOTIFY);
     if(Valid(thing)) {
        if((Typeof(thing) == TYPE_ARRAY) && (elementfrom != NOTHING)) {
           if(elementfrom == NOTHING) return;
           if(array_subquery_elements(player,thing,elementfrom,elementto,querybuf) > 0)
              setreturn(OK,COMMAND_SUCC);
	} else setreturn(OK,COMMAND_SUCC);
     }
}

/* ---->  Return expiry time (In days) of object  <---- */
void query_expiry(CONTEXT)
{
     dbref thing = query_find_object(player,params,SEARCH_PREFERRED,0,1);
     if(!Valid(thing)) return;
     sprintf(querybuf,"%d",db[thing].expiry);
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Return fail message of an object  <---- */
void query_fail(CONTEXT)
{
     dbref thing = query_find_object(player,params,SEARCH_PREFERRED,1,1);
     if(!Valid(thing) || (Typeof(thing) == TYPE_CHARACTER) || !HasField(thing,FAIL)) return;
     setreturn(String(getfield(thing,FAIL)),COMMAND_SUCC);
}

/* ---->  Return character's current 'feeling'  <---- */
void query_feeling(CONTEXT)
{
     int loop;

     dbref character = query_find_character(player,params,0);
     if(!Validchar(character)) return;
     for(loop = 0; feelinglist[loop].name && (feelinglist[loop].id != db[character].data->player.feeling); loop++);
     if(feelinglist[loop].name) {
        strcpy(querybuf,feelinglist[loop].name);
        *querybuf = tolower(*querybuf);
        setreturn(querybuf,COMMAND_SUCC);
     } else setreturn("",COMMAND_SUCC);
}

/* ---->  Filter substitutions out of given string  <---- */
void query_filter(CONTEXT)
{
     struct arg_data arg;
     const  char *ptr;
     char   *tmp;
       
     unparse_parameters(params,1,&arg,0);
     tmp = querybuf, ptr = arg.text[0];

     filter_substitutions(ptr,tmp);

     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Return expenditure, income or profit of character  <---- */
/*        (VAL1:  0 = Expenditure, 1 = Income, 2 = Profit.)        */
void query_finance(CONTEXT)
{
     double amount,divisor = 1;
     dbref  character;
     time_t now;

     character = query_find_character(player,arg1,0);
     if(!Validchar(character)) return;

     /* ---->  Return expenditure, income or profit?  <---- */
     switch(val1) {
            case 0:
                 amount = currency_to_double(&(db[character].data->player.expenditure));
                 break;
            case 1:
                 amount = currency_to_double(&(db[character].data->player.income));
                 break;
            case 2:
                 amount  = currency_to_double(&(db[character].data->player.income));
                 amount -= currency_to_double(&(db[character].data->player.expenditure));
                 break;
     }

     /* ---->  Total this quarter, or average per day, week or month?  <---- */
     gettime(now);
     if(!(Blank(arg2) || string_prefix("quarter",arg2))) {
        if(string_prefix("day",arg2)) {
           divisor = (double) (now - (quarter - QUARTER)) / DAY;
	} else if(string_prefix("week",arg2)) {
           divisor = (double) (now - (quarter - QUARTER)) / WEEK;
	} else if(string_prefix("month",arg2)) {
           divisor = (double) (now - (quarter - QUARTER)) / MONTH;
	} else {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LWHITE"day"ANSI_LGREEN"', '"ANSI_LWHITE"week"ANSI_LGREEN"', '"ANSI_LWHITE"month"ANSI_LGREEN"' or '"ANSI_LWHITE"quarter"ANSI_LGREEN"'.");
           return;
	}
     }
     if(divisor < 1) divisor = 1;

     sprintf(querybuf,"%.2f",amount / divisor);
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Return first name of an exit or command   <---- */
/*        (NOTE:  A little bit obsolete, seeing as        */
/*                '{@?itemno 1 ; "{@?name <OBJECT>}"}'    */
/*                can be used to do the same thing.       */
/*                Provided for backwards compatibility.)  */
void query_first_name(CONTEXT)
{
     dbref thing;
     char *ptr;

     thing = query_find_object(player,params,SEARCH_COMMAND|SEARCH_EXIT,0,0);
     if(!Valid(thing)) return;
     if((Typeof(thing) == TYPE_EXIT) || (Typeof(thing) == TYPE_COMMAND)) {
        strcpy(querybuf,getname(thing));
        if((ptr = strchr(querybuf,LIST_SEPARATOR)) != NULL) *ptr = '\0';
        setreturn(querybuf,COMMAND_SUCC);
     } else setreturn(getname(thing),COMMAND_SUCC);
}

/* ---->  Return flags of object  <---- */
void query_flags(CONTEXT)
{
     dbref object = query_find_object(player,arg1,SEARCH_PREFERRED,((Level4(db[player].owner) || Experienced(db[player].owner)) ? 0:1)|0x2,0);
     char  *ptr; 

     if(!Valid(object)) return;
     
     if(!Blank(arg2) && string_prefix("short",arg2)) {
        for(ptr = (char *) unparse_flags(object); !Blank(ptr) && (*ptr == ' '); ptr++);
        setreturn(ptr,COMMAND_SUCC);
     } else {
        unparse_flaglist(object,1,querybuf);
        if(!BlankContent(querybuf) && (*(ptr = (querybuf + strlen(querybuf) - 1)) == '.')) *ptr = '\0';
        setreturn(querybuf,COMMAND_SUCC);
     }
}

/* ---->  {@?format "<STRING>"}  <---- */
void query_format(CONTEXT)
{
     struct arg_data arg;

     unparse_parameters(params,1,&arg,0);
     if(arg.count < 1) setreturn("",COMMAND_SUCC);
        else setreturn(punctuate(arg.text[0],1,'.'),COMMAND_SUCC);
}

/* ---->  Is given user a friend/enemy of PLAYER?  <---- */
/*        (val1:  0 = Friend, 1 = Enemy.)                */
void query_friend(CONTEXT)
{
     dbref         character = query_find_character(player,arg1,0);
     unsigned char flist = 1,fothers = 1;
     int           flags = 0,flags2 = 0;

     if(!Blank(arg2)) {
        if(string_prefix("flist",arg2) || string_prefix("list",arg2)) {
           fothers = 0;
	} else if(string_prefix("fothers",arg2) || string_prefix("others",arg2)) {
           flist = 0;
	} else {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LWHITE"flist"ANSI_LGREEN"' or '"ANSI_LWHITE"fothers"ANSI_LGREEN"'.");
           return;
	}
     }

     if(flist)   flags  |= friend_flags(player,character);
     if(fothers) flags2 |= friend_flags(character,player);

     if(flist && fothers && (flags & FRIEND_EXCLUDE)) return;
     if(!Validchar(character) || !(flags|flags2) || (val1 && !((flags|flags2) & FRIEND_ENEMY)) || (!val1 && ((flags|flags2) & FRIEND_ENEMY))) return;
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Match to full name of character in database  <---- */
void query_fullname(CONTEXT)
{
     unsigned char article_setting = 0,upper;
     dbref         character;

     upper     = (isupper(arg0[2])) ? 1:0;
     character = lookup_character(player,arg1,2);
     if(Validchar(character)) {
        if(!Blank(arg2)) {
           if(!strcasecmp("DEFINITE",arg2)) article_setting = DEFINITE;
              else if(!strcasecmp("INDEFINITE",arg2)) article_setting = INDEFINITE;
           if(upper || isupper(*arg2)) article_setting |= UPPER;
	}
        setreturn(getcname(NOTHING,character,0,article_setting),COMMAND_SUCC);
     } else setreturn(NOTHING_STRING,COMMAND_FAIL);
}

/* ---->  {@?head "<SEPARATOR>" "<LIST>"}  <---- */
void query_head(CONTEXT)
{
     char   temp[TEXT_SIZE];
     struct arg_data arg;
     char   *ptr,*tmp;

     unparse_parameters(params,2,&arg,0);
     if(!((arg.count < 2) || Blank(arg.text[0]) || Blank(arg.text[1]))) {
        for(ptr = arg.text[1], tmp = temp; *ptr && strncasecmp(ptr,arg.text[0],arg.len[0]); *tmp++ = *ptr, ptr++);
        *tmp = '\0';
        setreturn(temp,COMMAND_SUCC);
     } else setreturn("",COMMAND_SUCC);
}

/* ---->  Return ID of an object  <---- */
void query_id(CONTEXT)
{
    /* dbref thing = query_find_object(player,params,SEARCH_PREFERRED,((Level4(db[player].owner) || Experienced(db[player].owner)) ? 0:1)|0x2,0); */
    dbref thing = query_find_object(player,params,SEARCH_PREFERRED, 0x2,0);
     if(!Valid(thing)) return;
     setreturn(getnameid(player,thing,NULL),COMMAND_SUCC);
}

/* ---->  Return character's idle time (In secs)  <---- */
void query_idletime(CONTEXT)
{
     struct descriptor_data *p;
     time_t now;

     dbref character = query_find_character(player,params,0);
     if(!Validchar(character)) return;
     if((p = getdsc(character))) {
        gettime(now);
        sprintf(querybuf,"%d",(int) (now - p->last_time));
        setreturn(querybuf,COMMAND_SUCC);
     } else setreturn("0",COMMAND_SUCC);
}

/* ---->  {@?insert "<ITEM>" <POSITION> "<SEPARATOR>" "<LIST>"}  <---- */
void query_insert(CONTEXT)
{
     char              temp[BUFFER_LEN],separator[TEXT_SIZE],item[TEXT_SIZE];
     int               loop = 1,count;
     unsigned char     inserted = 0;
     char              *ptr,*tmp;
     struct   arg_data arg;

     unparse_parameters(params,4,&arg,1);
     if(!((arg.count < 4) || ((arg.numb[1] <= 0) && (arg.numb[1] != LAST) && (arg.numb[1] != END)) || Blank(arg.text[2]))) {
        ptr = arg.text[3], *temp = '\0';
        if(Blank(ptr)) arg.numb[1] = 1;
           else if(arg.numb[1] == END) arg.numb[1] = TCZ_INFINITY;

        while(*ptr) {

              /* ---->  Get item  <---- */
              for(tmp = item; *ptr && strncasecmp(ptr,arg.text[2],arg.len[2]); *tmp++ = *ptr++);
              *tmp = '\0';

              /* ---->  Get separator  <---- */
              for(tmp = separator, count = arg.len[2]; *ptr && (count > 0); count--, *tmp++ = *ptr++);
              *tmp = '\0';

              /* ---->  Insert new item in front of current item?  <---- */
              if((loop == arg.numb[1]) || ((arg.numb[1] == LAST) && !*ptr && BlankContent(separator))) {
                 strcat(temp,arg.text[0]);
                 strcat(temp,arg.text[2]);
                 inserted = 1;
	      }
              if(*item)      strcat(temp,item);
              if(*separator) strcat(temp,separator);
              loop++;
	}

        /* ---->  Insert new item onto end of list?  <---- */
        if(!inserted) {
           if(!Blank(arg.text[3])) strcat(temp,arg.text[2]);
           strcat(temp,arg.text[0]);
	}
        temp[MAX_LENGTH] = '\0';
        setreturn(temp,COMMAND_SUCC);
     } else setreturn(arg.text[3],COMMAND_SUCC);
}

/* ---->  Return specified time in secs in the hours, mins, secs, etc.  <---- */
void query_interval(CONTEXT)
{
     int           intval,entities;
     unsigned char signedint;
     unsigned long longval;

     if(!Blank(arg1)) {
       signedint  = (*arg1 == '-');
       longval    = (signedint) ? 0:strtoul(arg1,NULL,10);
       intval     = (signedint) ? atol(arg1):0;
       if(!Blank(arg2)) {
          entities = atol(arg2);
          if(entities > ENTITIES) entities = ENTITIES;
       } else entities = ENTITIES;
       setreturn(interval(longval,intval,entities,0),COMMAND_SUCC);
     } else setreturn("0 seconds",COMMAND_SUCC);
}

/* ---->    {@?item "<NAME>" "<SEPARATOR>" "<LIST>"}    <---- */
/*        (val1:  0 = Partial match, 1 = Exact match.)        */
void query_item(CONTEXT)
{
     struct arg_data arg;
     char   result[16];
     short  loop;
     char   *ptr;

     unparse_parameters(params,3,&arg,0);
     if(!((arg.count < 3) || Blank(arg.text[0]) || Blank(arg.text[1]) || Blank(arg.text[2]))) {
        ptr = arg.text[2], loop = 1;
        while(*ptr) {

              /* ---->  Skip <SEPARATOR>  <---- */
              for(; *ptr && !strncasecmp(ptr,arg.text[1],arg.len[1]); ptr += arg.len[1], loop++);

              /* ---->  Match current item  <---- */
	      if(*ptr) {
                 if(val1) {

                    /* ---->  Exact match current item  <---- */
	            if(!strncasecmp(ptr,arg.text[0],arg.len[0]) && (!ptr[arg.len[0]] || !strncasecmp(ptr + arg.len[0],arg.text[1],arg.len[1]))) {
                       sprintf(result,"%d",loop);
                       setreturn(result,COMMAND_SUCC);
	               return;
		    }
                    for(; *ptr && strncasecmp(ptr,arg.text[1],arg.len[1]); ptr++);
		 } else for(; *ptr && strncasecmp(ptr,arg.text[1],arg.len[1]); ptr++) {
                    if(!strncasecmp(ptr,arg.text[0],arg.len[0])) {

                       /* ---->  Partial match current item  <---- */
	               sprintf(result,"%d",loop);
                       setreturn(result,COMMAND_SUCC);
	               return;
		    }
		 }
	      }
	}
     }
     setreturn("0",COMMAND_SUCC);
}

/* ---->  {@?itemno <NUMBER> "<SEPARATOR>" "<LIST>"}  <---- */
void query_itemno(CONTEXT)
{
     char              temp[TEXT_SIZE];
     char              *ptr,*tmp;
     unsigned char     last = 0;
     short             loop = 0;
     struct   arg_data arg;

     unparse_parameters(params,3,&arg,1);
     if(!((arg.count < 3) || ((arg.numb[0] <= 0) && (arg.numb[0] != LAST)) || Blank(arg.text[1]) || Blank(arg.text[2]))) {
        ptr = arg.text[2];
        if(arg.numb[0] == LAST) last = 1;
        while(*ptr) {
              for(loop++, tmp = temp; *ptr && strncasecmp(ptr,arg.text[1],arg.len[1]); *tmp++ = *ptr++);
	      *tmp = '\0';
 
              /* ---->  Item found  <---- */
              if(!last && (loop == arg.numb[0])) {
                 setreturn(temp,COMMAND_SUCC);
                 return;
	      }

	      /* ---->  Skip <SEPARATOR>  <---- */
	      if(*ptr && !strncasecmp(ptr,arg.text[1],arg.len[1])) {
                 ptr += arg.len[1];
                 if(!*ptr) *temp = '\0';
	      }
	}
        setreturn((last) ? temp:"",COMMAND_SUCC);
     } else setreturn("",COMMAND_SUCC);
}

/* ---->  Return key of an object  <---- */
void query_key(CONTEXT)
{
     dbref thing = query_find_object(player,params,SEARCH_THING,0,1);
     if(!Valid(thing) || (Typeof(thing) != TYPE_THING)) return;
     setreturn(unparse_boolexp(player,getlock(thing,1),1),COMMAND_SUCC);
}

/* ---->  Return last command typed by character  <---- */
void query_lastcommand(CONTEXT)
{
     struct descriptor_data *c;

     dbref character = query_find_character(player,params,0);
     if(!Validchar(character) || (command_type & QUERY_SUBSTITUTION)) return;

     if(Root(player) || (character == player) || (player == Controller(character)) || (level_app(Owner(player)) > level_app(Owner(character)))) {
        if(Connected(character)) {
           if(!(c = getdsc(character))) return;

           if(c && (c->flags & CONNECTED) && Validchar(c->player)) {
              if(!((c->flags & SPOKEN_TEXT) || ((c->flags & ABSOLUTE) && !(c->flags2 & ABSOLUTE_OVERRIDE)))) {
                 if(strlen(decompress(c->last_command)) > MAX_LENGTH) strncpy(querybuf,decompress(c->last_command),MAX_LENGTH);
                    else strcpy(querybuf,decompress(c->last_command));
              } else strcpy(querybuf,"(Privacy Upheld)");
           } else strcpy(querybuf,NOTHING_STRING);
           setreturn(querybuf,COMMAND_SUCC);
        } else if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" isn't connected.",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0));
     } else if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only query the last command typed by your puppet(s) or a character of a lower level than yourself.");
}

/* ---->  Return time character last connected  <---- */
void query_lastconnected(CONTEXT)
{
     dbref  character;
     time_t now;

     character = query_find_character(player,arg1,0);
     if(!Validchar(character)) return;

     gettime(now);
     sprintf(querybuf,"%ld",(!Blank(arg2) && string_prefix("longdates",arg2)) ? epoch_to_longdate((db[character].data->player.lasttime == 0) ? now:db[character].data->player.lasttime):((db[character].data->player.lasttime == 0) ? now:db[character].data->player.lasttime));
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Return site character last connected from  <---- */
void query_lastsite(CONTEXT)
{
     dbref character = query_find_character(player,params,1);

     if(!Validchar(character) || (command_type & QUERY_SUBSTITUTION) || !HasField(character,LASTSITE) || (!Level4(db[player].owner) && !can_write_to(player,character,0))) return;
     setreturn(!Blank(getfield(character,LASTSITE)) ? getfield(character,LASTSITE):"Unknown",COMMAND_SUCC);
}

/* ---->  {@?leftstr <COUNT> "<STRING>"}  <---- */
void query_leftstr(CONTEXT)
{
     char   temp[TEXT_SIZE];
     struct arg_data arg;
     char   *ptr,*tmp;
     short  loop;

     unparse_parameters(params,2,&arg,0);
     if(!((arg.count < 2) || (arg.numb[0] <= 0))) {
        ptr = arg.text[1], tmp = temp;
        for(loop = 0; *ptr && (loop < arg.numb[0]); loop++, *tmp++ = *ptr++);
        *tmp = '\0';
        setreturn(temp,COMMAND_SUCC);
     } else setreturn("",COMMAND_SUCC);
}

/* ---->  Return current line number  <---- */
void query_line(CONTEXT)
{
     sprintf(querybuf,(flow_control & FLOW_GOTO) ? "%+d":"%d",current_line_number);
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Return location of object  <---- */
void query_location(CONTEXT)
{
     dbref thing = query_find_object(player,params,SEARCH_PREFERRED,0,0);
     if(!Valid(thing) || (!can_read_from(player,db[thing].location) && !can_read_from(player,thing))) return;
     setreturn(getnameid(player,db[thing].location,NULL),COMMAND_SUCC);
}

/* ---->  Return lock of an object  <---- */
void query_lock(CONTEXT)
{
     dbref thing = query_find_object(player,params,SEARCH_PREFERRED,0,1);
     if(!Valid(thing)) return;
     setreturn(unparse_boolexp(player,getlock(thing,0),1),COMMAND_SUCC);
}

/* ---->  {J.P.Boggis 01/07/2000}  Return date of longest connect time of character  <---- */
void query_longestdate(CONTEXT)
{
     dbref  character;

     character = query_find_character(player,arg1,0);
     if(!Validchar(character)) return;

     sprintf(querybuf,"%ld",(!Blank(arg2) && string_prefix("longdates",arg2)) ? epoch_to_longdate(db[character].data->player.longestdate):db[character].data->player.longestdate);
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Return longest connect time of character  <---- */
void query_longesttime(CONTEXT)
{
     time_t total,now;
     dbref  character;

     character = query_find_character(player,params,0);
     if(!Validchar(character)) return;

     gettime(now);
     if(Connected(character)) {
        if((total = (now - db[character].data->player.lasttime)) == now) total = 0;
        if(db[character].data->player.longesttime < total) db[character].data->player.longesttime = total;
     }
     sprintf(querybuf,"%ld",(db[character].data->player.longesttime < 0) ? 0:db[character].data->player.longesttime);
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Return mass or volume of an object  <---- */
/*        (Val1:  0 = Mass, 1 = Volume)             */
void query_mass_or_volume(CONTEXT)
{
     dbref thing = query_find_object(player,params,SEARCH_PREFERRED,(Level4(Owner(player)) || Experienced(Owner(player))) ? 0:1,0);

     if(!Valid(thing)) return;
     sprintf(querybuf,"%ld",get_mass_or_volume(thing,val1));
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  {@?midstr <INDEX> <COUNT> "<STRING>"}  <---- */
void query_midstr(CONTEXT)
{
     char   temp[TEXT_SIZE];
     struct arg_data arg;
     char   *ptr,*tmp;
     short  loop;

     unparse_parameters(params,3,&arg,0);
     if(!((arg.count < 3) || (arg.numb[0] <= 0) || (arg.numb[1] <= 0) || Blank(arg.text[2]))) {
        ptr = arg.text[2], tmp = temp;
        for(loop = 1; *ptr && (loop < arg.numb[0]); loop++, ptr++);
  	for(loop = 0; *ptr && (loop < arg.numb[1]); loop++, *tmp++ = *ptr++);
        *tmp = '\0';
        setreturn(temp,COMMAND_SUCC);
     } else setreturn("",COMMAND_SUCC);
}

/* ---->  {@?modify "<NEW ITEM>" <NUMBER> "<SEPARATOR>" "<LIST>"}  <---- */
void query_modify(CONTEXT)
{
     char              temp[BUFFER_LEN],separator[TEXT_SIZE],item[TEXT_SIZE];
     unsigned char     skip = 1,last = 0;
     short             loop = 0,loop2;
     char              *ptr,*tmp;
     struct   arg_data arg;

     unparse_parameters(params,4,&arg,1);
     if(!((arg.count < 4) || ((arg.numb[1] <= 0) && (arg.numb[1] != LAST)) || Blank(arg.text[2]) || Blank(arg.text[3]))) {
        ptr = arg.text[3], *temp = '\0';
        if(arg.numb[1] == LAST) last = 1;
        while(*ptr) {
              loop++;

              /* ---->  Get separator  <---- */
              tmp = separator;
              if(*ptr && !skip && !strncasecmp(ptr,arg.text[2],arg.len[2]))
                 for(loop2 = 0; loop2 < arg.len[2]; loop2++)
                     if(*ptr) *tmp++ = *ptr++;
              *tmp = '\0';

              /* ---->  Get item  <---- */
              for(tmp = item; *ptr && strncasecmp(ptr,arg.text[2],arg.len[2]); *tmp++ = *ptr++);
              *tmp = '\0';

              /* ---->  Modify item?  <---- */
              if(!BlankContent(separator)) strcat(temp,separator);
              if((last && *ptr) || (!last && (loop != arg.numb[1]))) {
	         if(!BlankContent(item)) strcat(temp,item);
	      } else strcat(temp,arg.text[0]);
              skip = 0;
	}
        temp[MAX_LENGTH] = '\0';
        setreturn(temp,COMMAND_SUCC);
     } else setreturn(arg.text[3],COMMAND_SUCC);
}

/* ---->            Return name of an object             <---- */
/*        (val1:  0 = Name, 1 = Realname, 2 = Article.)        */
void query_name(CONTEXT)
{
     dbref         thing = query_find_object(player,arg1,SEARCH_PREFERRED,2,0);
     unsigned char article_setting = 0,upper;

     if(!Valid(thing)) return;
     upper = (isupper(arg0[2])) ? 1:0;
     if(!((val1 == 1) && (Typeof(thing) == TYPE_CHARACTER))) {
        if((val1 == 2) && !HasArticle(thing)) {
           setreturn("",COMMAND_SUCC);
           return;
	}
        if(!Blank(arg2)) {
           if(string_prefix("DEFINITE",arg2)) article_setting = DEFINITE;
              else if(string_prefix("INDEFINITE",arg2)) article_setting = INDEFINITE;
           if(upper || isupper(*arg2)) article_setting |= UPPER;
	}
        if(val1 == 2) {
           sprintf(querybuf,"%s",Article(thing,article_setting & 0x1,article_setting & 0x6));
           setreturn(querybuf,COMMAND_SUCC);
	} else setreturn(getcname(NOTHING,thing,0,article_setting),COMMAND_SUCC);
     } else setreturn(getname(thing),COMMAND_SUCC);
}

/* ---->  Match to name of character in database with connected characters taking priority  <---- */
void query_namec(CONTEXT)
{
     unsigned char article_setting = 0,upper;
     dbref         character;

     character = lookup_character(player,arg1,1);
     upper     = (isupper(arg0[2])) ? 1:0;
     if(Validchar(character)) {
        if(!Blank(arg2)) {
           if(!strcasecmp("DEFINITE",arg2)) article_setting = DEFINITE;
              else if(!strcasecmp("INDEFINITE",arg2)) article_setting = INDEFINITE;
           if(upper || isupper(*arg2)) article_setting |= UPPER;
	}
        setreturn(getcname(NOTHING,character,0,article_setting),COMMAND_SUCC);
     } else setreturn(NOTHING_STRING,COMMAND_FAIL);
}

/* ---->  Return compound command nesting level  <---- */
void query_nested(CONTEXT)
{
     sprintf(querybuf,"%d",command_nesting_level);
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Return NEWLINE character ('\n')  <---- */
void query_newline(CONTEXT)
{
     setreturn("\n",COMMAND_SUCC);
}

/* ---->  Return next object (Or NUMBER'th next object if specified) in the list specified object is in  <---- */
void query_next(CONTEXT)
{
     dbref currentobj,original,thing,prev;
     int   count,last = 0;

     thing = original = query_find_object(player,arg1,SEARCH_PREFERRED,0,0);
     if(!Valid(thing)) return;

     if(!Blank(arg2)) {
        if(!strcasecmp(arg2,"FIRST")) count = 1;
           else if(!strcasecmp(arg2,"LAST")) {
              count = TCZ_INFINITY;
              last  = 1;
	   } else {
              count = atol(arg2);
              if(count < 0) count = 0;
	   }
     } else count = 1;

     currentobj = db[thing].location;
     while(Valid(thing) && (Typeof(thing) != Typeof(original))) getnext(thing,WhichList(thing),currentobj);
     for(prev = thing; !((count <= 0) || !Valid(thing));) {
         if(Typeof(thing) == Typeof(original)) {
            prev = thing;
            count--;
	 }
         getnext(thing,WhichList(thing),currentobj);
     }
     setreturn(getnameid(player,(last) ? prev:thing,NULL),COMMAND_SUCC);
}

/* ---->  {@?noitems "<SEPARATOR>" "<LIST>"}  <---- */
void query_noitems(CONTEXT)
{
     struct arg_data arg;
     char   result[16];
     short  loop;
     char   *ptr;

     unparse_parameters(params,2,&arg,0);
     if(!((arg.count < 2) || Blank(arg.text[0]) || Blank(arg.text[1]))) {
        ptr = arg.text[1], loop = 0;
        while(*ptr) {
              if(*ptr) loop++;
              while(*ptr && strncasecmp(ptr,arg.text[0],arg.len[0])) ptr++;
	      if(*ptr && !strncasecmp(ptr,arg.text[0],arg.len[0])) {
                 ptr += arg.len[0];
                 if(!*ptr) loop++;
	      }
	}
        sprintf(result,"%d",loop);
        setreturn(result,COMMAND_SUCC);
     } else setreturn("0",COMMAND_SUCC);
}

/* ---->   {@?nowords "<SENTENCE>"}  <---- */
void query_nowords(CONTEXT)
{
     struct arg_data arg;
     char   result[16];
     short  loop = 0;
     char   *ptr;

     unparse_parameters(params,1,&arg,0);
     if(!((arg.count < 1) || Blank(arg.text[0]))) {
        ptr = arg.text[0];
        while(*ptr) {
              for(; *ptr && (*ptr == ' '); ptr++);
              if(*ptr) loop++;
	      for(; *ptr && (*ptr != ' '); ptr++);
	}
        sprintf(result,"%d",loop);
        setreturn(result,COMMAND_SUCC);
     } else setreturn("0",COMMAND_SUCC);
}

/* ---->  {J.P.Boggis 07/03/2000}  Is string a number?  <---- */
void query_number(CONTEXT)
{
     struct arg_data arg;
     int    trailing = 0;
     int    leading  = 0;
     int    decimal  = 0;
     char   *ptr;

     setreturn(ERROR,COMMAND_FAIL);
     unparse_parameters(params,1,&arg,0);

     for(params = arg.text[0]; *params && (*params == ' '); params++);
     strcpy(querybuf,params);
     if((ptr = strchr(querybuf,' '))) {
        if(*(ptr + 1)) return;
        *ptr = '\0';
     }

     ptr = querybuf;
     if(*ptr == '-') ptr++;

     if(Blank(ptr)) return;
     for(; *ptr; ptr++) {
         if(*ptr == '.') {
            if(decimal || !leading) return;
            decimal = 1;
	 } else if(*ptr == ' ') {
            for(; ptr && (*ptr == ' '); ptr++);
            if(*ptr) return;
	 } else if(isdigit(*ptr)) {
            leading = 1;
            if(decimal) trailing = 1;
	 } else return;
     }
     if(decimal && !trailing) return;
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Return ID of first (Or NUMBER'th if specified) object of specified type attached to given object  <---- */
/*        (val1:  Type of object)                                                                                 */
void query_object(CONTEXT)
{
     dbref prev,thing,currentobj;
     int   count,last = 0;

     thing = query_find_object(player,arg1,SEARCH_PREFERRED,1,1);
     if(!Valid(thing)) return;

     if(!Blank(arg2)) {
        if(!strcasecmp(arg2,"FIRST")) count = 1;
           else if(!strcasecmp(arg2,"LAST")) {
              count = TCZ_INFINITY;
              last  = 1;
	   } else {
              count = atol(arg2);
              if(count < 0) count = 0;
	   }
     } else count = 1;

     setreturn(NOTHING_STRING,COMMAND_SUCC);
     switch(val1) {
            case TYPE_EXIT:
                 if(!HasList(thing,EXITS)) return;
                 thing = getfirst(thing,EXITS,&currentobj);
                 break;
            case TYPE_COMMAND:
                 if(!HasList(thing,COMMANDS)) return;
                 thing = getfirst(thing,COMMANDS,&currentobj);
                 break;
            case TYPE_PROPERTY:
            case TYPE_VARIABLE:
            case TYPE_ARRAY:
                 if(!HasList(thing,VARIABLES)) return;
                 thing = getfirst(thing,VARIABLES,&currentobj);
                 break;
            case SEARCH_ALL:  /*  (First object in contents list)  */
                 if(!HasList(thing,CONTENTS)) return;
                 for(thing = prev = getfirst(thing,CONTENTS,&currentobj); !((count <= 1) || !Valid(thing)); count--) {
                     prev = thing;
                     getnext(thing,CONTENTS,currentobj);
		 }
                 setreturn(getnameid(player,(last) ? prev:thing,NULL),COMMAND_SUCC);
                 return;
            case TYPE_FUSE:
            case TYPE_ALARM:
                 if(!HasList(thing,FUSES)) return;
                 thing = getfirst(thing,FUSES,&currentobj);
                 break;
            case TYPE_CHARACTER:
            case TYPE_THING:
            case TYPE_ROOM:
                 if(!HasList(thing,CONTENTS)) return;
                 thing = getfirst(thing,CONTENTS,&currentobj);
                 break;
            default:
                 return;
     }
     while(Valid(thing) && (Typeof(thing) != val1)) getnext(thing,WhichList(thing),currentobj);
     for(prev = thing; !((count <= 1) || !Valid(thing));) {
         if(Typeof(thing) == val1) {
            prev = thing;
            count--;
	 }
         getnext(thing,WhichList(thing),currentobj);
     }
     setreturn(getnameid(player,(last) ? prev:thing,NULL),COMMAND_SUCC);
}

/* ---->  Return outside description of an object  <---- */
void query_odesc(CONTEXT)
{
     dbref thing = query_find_object(player,params,SEARCH_PREFERRED,0,1);
     if(!Valid(thing) || (Typeof(thing) == TYPE_CHARACTER) || !HasField(thing,ODESC)) return;
     setreturn(String(getfield(thing,ODESC)),COMMAND_SUCC);
}

/* ---->  Return other drop message of an object  <---- */
void query_odrop(CONTEXT)
{
     dbref thing = query_find_object(player,params,SEARCH_PREFERRED,1,1);
     if(!Valid(thing) || (Typeof(thing) == TYPE_CHARACTER) || !HasField(thing,ODROP)) return;
     setreturn(String(getfield(thing,ODROP)),COMMAND_SUCC);
}

/* ---->  Return other fail message of an object  <---- */
void query_ofail(CONTEXT)
{
     dbref thing = query_find_object(player,params,SEARCH_PREFERRED,1,1);
     if(!Valid(thing) || (Typeof(thing) == TYPE_CHARACTER) || !HasField(thing,OFAIL)) return;
     setreturn(String(getfield(thing,OFAIL)),COMMAND_SUCC);
}

/* ---->  Return other success message of an object  <---- */
void query_osucc(CONTEXT)
{
     dbref thing = query_find_object(player,params,SEARCH_PREFERRED,1,1);
     if(!Valid(thing) || (Typeof(thing) == TYPE_CHARACTER) || !HasField(thing,OSUCC)) return;
     setreturn(String(getfield(thing,OSUCC)),COMMAND_SUCC);
}

/* ---->  Return owner of an object  <---- */
void query_owner(CONTEXT)
{
     dbref thing = query_find_object(player,params,SEARCH_PREFERRED,0,0);
     if(!Valid(thing)) return;
     setreturn(getnameid(player,db[thing].owner,NULL),COMMAND_SUCC);
}

/* ---->   {@?pad <LENGTH> "<PADTEXT>" ["<TEXT>"]}   <---- */
/*        (val1:  0 = Centre, 1 = Left, 2 = Right.)        */
void query_pad(CONTEXT)
{
     int    offset,copied,length = 0,rlength = 0;
     char   temp[TEXT_SIZE];
     struct arg_data arg;
     char   *dest = temp;

     unparse_parameters(params,3,&arg,0);
     if(arg.numb[0] > 512) arg.numb[0] = 512;
        else if(arg.numb[0] <= 0) arg.numb[0] = output_terminal_width(player);
     if(!Blank(arg.text[1])) {
        ansi_code_filter(arg.text[1],arg.text[1],1);
        arg.len[1] = strlen(arg.text[1]);
     }
     if((arg.count < 3) || Blank(arg.text[1]) || Blank(arg.text[2])) {
        if(Blank(arg.text[1])) arg.text[1] = " ";
        for(copied = 1; copied && (length < arg.numb[0]); dest = strcpy_ignansi(dest,arg.text[1],&length,&rlength,&copied,arg.numb[0]));
     } else switch(val1) {
        case 0:

             /* ---->  {@?padcentre}  <---- */
             if((offset = ((arg.numb[0] - strlen_ignansi(arg.text[2])) / 2)) > 0)
                for(copied = 1; copied && (length < offset); dest = strcpy_ignansi(dest,arg.text[1],&length,&rlength,&copied,offset));
             dest = strcpy_ignansi(dest,arg.text[2],&length,&rlength,&copied,arg.numb[0]);
             for(offset = (length % arg.len[1]), copied = 1; copied && (length < arg.numb[0]); offset = 0)
                 dest = strcpy_ignansi(dest,arg.text[1] + offset,&length,&rlength,&copied,arg.numb[0]);
             break;
        case 1:

             /* ---->  {@?padleft}  <---- */
             dest = strcpy_ignansi(dest,arg.text[2],&length,&rlength,&copied,arg.numb[0]);
             offset = (length < arg.len[1]) ? length:(length / arg.len[1]);
             for(offset = (length % arg.len[1]), copied = 1; copied && (length < arg.numb[0]); offset = 0)
                 dest = strcpy_ignansi(dest,arg.text[1] + offset,&length,&rlength,&copied,arg.numb[0]);
             break;
        case 2:

             /* ---->  {@?padright}  <---- */
             if((offset = (arg.numb[0] - strlen_ignansi(arg.text[2]))) > 0)
                for(copied = 1; copied && (length < offset); dest = strcpy_ignansi(dest,arg.text[1],&length,&rlength,&copied,offset));
             dest = strcpy_ignansi(dest,arg.text[2],&length,&rlength,&copied,arg.numb[0]);
             break;
     }
     setreturn(temp,COMMAND_SUCC);
}

/* ---->  Return parent of object  <---- */
void query_parent(CONTEXT)
{
     dbref thing;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(params)) thing = query_find_object(player,params,SEARCH_PREFERRED,1,1);
        else if(in_command) thing = current_command;
           else return;

     if(!Valid(thing)) return;
     setreturn(getnameid(player,db[thing].parent,NULL),COMMAND_SUCC);
}

/* ---->  Return partner of character  <---- */
void query_partner(CONTEXT)
{
     dbref character = query_find_character(player,params,0);
     if(!Validchar(character)) return;
     setreturn((Married(character) || Engaged(character)) ? getnameid(player,Partner(character),NULL):NOTHING_STRING,COMMAND_SUCC);
}

/* ---->  Return peak number of users  <---- */
void query_peak(CONTEXT)
{
     if(Blank(params) || (string_prefix("todayspeak",params) || string_prefix("maximumpeak",params))) {
        sprintf(querybuf,"%ld",stats[stat_ptr].peak);
     } else if(string_prefix("currentpeak",params)) {
        struct descriptor_data *d;
        int    count = 0;

        for(d = descriptor_list; d; d = d->next)
            if(d->flags & CONNECTED) count++;
        sprintf(querybuf,"%d",count);
     } else if(string_prefix("averagepeak",params) || string_prefix("avgpeak",params)) {
        int apptr,apdays = 0,avgpeak = 0;

        for(apptr = stat_ptr; (apptr >= 0) && (apdays < 7); avgpeak += stats[apptr].peak, apptr--, apdays++);
        avgpeak /= apdays;

        sprintf(querybuf,"%d",avgpeak);
     } else if(!(string_prefix("recordpeak",params) || string_prefix("highestpeak",params))) {
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LYELLOW"current"ANSI_LGREEN"', '"ANSI_LYELLOW"today"ANSI_LGREEN"', '"ANSI_LYELLOW"average"ANSI_LGREEN"' or '"ANSI_LYELLOW"record"ANSI_LGREEN"'.");
        setreturn(ERROR,COMMAND_FAIL);
        return;
     } else sprintf(querybuf,"%ld",stats[STAT_MAX].peak);
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Return date/time of pending alarm  <---- */
void query_pending(CONTEXT)
{
     struct event_data *ptr;

     dbref thing = query_find_object(player,arg1,SEARCH_ALARM,1,1);
     if(!Valid(thing) || (Typeof(thing) != TYPE_ALARM)) return;

     for(ptr = event_queue; ptr && ptr->object != thing; ptr = ptr->next);
     if(ptr) {
        sprintf(querybuf,"%ld",(!Blank(arg2) && string_prefix("longdates",arg2)) ? epoch_to_longdate(ptr->time):ptr->time);
        setreturn(querybuf,COMMAND_SUCC);
     }
}

/* ---->  Return character's name prefix  <---- */
void query_prefix(CONTEXT)
{
     dbref character = query_find_character(player,params,0);
     if(!Validchar(character) || (Typeof(character) != TYPE_CHARACTER)) return;
     setreturn(String(getfield(character,PREFIX)),COMMAND_SUCC);
}

/* ---->  Return character's privileges  <---- */
void query_privileges(CONTEXT)
{
     dbref character = query_find_character(player,arg1,0);

     if(!Validchar(character)) {
        setreturn("7",COMMAND_SUCC);
	return;
     }

     if((strlen(arg2) > 0) && string_prefix("@chpid",arg2)) character = db[character].owner;
     sprintf(querybuf,"%d",privilege(character,255));
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Return information from character's profile  <---- */
void query_profile(CONTEXT)
{
     dbref character = query_find_character(player,arg1,0);
     if(!Validchar(character)) return;
     if(!Blank(arg2)) {
        if(string_prefix("qualifications",arg2)) {
           if(hasprofile(db[character].data->player.profile) && db[character].data->player.profile->qualifications)
              setreturn(decompress(db[character].data->player.profile->qualifications),COMMAND_SUCC);
	} else if(string_prefix("achievements",arg2) || string_prefix("accomplishments",arg2)) {
           if(hasprofile(db[character].data->player.profile) && db[character].data->player.profile->achievements)
              setreturn(decompress(db[character].data->player.profile->achievements),COMMAND_SUCC);
	} else if(string_prefix("nationality",arg2)) {
           if(hasprofile(db[character].data->player.profile) && db[character].data->player.profile->nationality)
              setreturn(decompress(db[character].data->player.profile->nationality),COMMAND_SUCC);
	} else if(string_prefix("occupation",arg2) || string_prefix("job",arg2)) {
           if(hasprofile(db[character].data->player.profile) && db[character].data->player.profile->occupation)
              setreturn(decompress(db[character].data->player.profile->occupation),COMMAND_SUCC);
	} else if(string_prefix("interests",arg2)) {
           if(hasprofile(db[character].data->player.profile) && db[character].data->player.profile->interests)
              setreturn(decompress(db[character].data->player.profile->interests),COMMAND_SUCC);
	} else if(string_prefix("sex",arg2) || string_prefix("gender",arg2)) {
           strcpy(querybuf,genders[Genderof(character)]);
           *querybuf = tolower(*querybuf);
           setreturn(querybuf,COMMAND_SUCC);
	} else if(string_prefix("sexuality",arg2) || string_prefix("orientation",arg2) || string_prefix("sexualorientation",arg2)) {
           if(hasprofile(db[character].data->player.profile) && db[character].data->player.profile->sexuality) {
              strcpy(querybuf,sexuality[db[character].data->player.profile->sexuality]);
              *querybuf = tolower(*querybuf);
              setreturn(querybuf,COMMAND_SUCC);
	   }
	} else if(string_prefix("statusirl",arg2) || string_prefix("irlstatus",arg2)) {
           if(hasprofile(db[character].data->player.profile) && db[character].data->player.profile->statusirl) {
              strcpy(querybuf,statuses[db[character].data->player.profile->statusirl]);
              *querybuf = tolower(*querybuf);
              setreturn(querybuf,COMMAND_SUCC);
	   }
	} else if(string_prefix("statusivl",arg2) || string_prefix("ivlstatus",arg2)) {
           if(!((Engaged(character) || Married(character)) && Validchar(Partner(character)))) {
              if(hasprofile(db[character].data->player.profile) && db[character].data->player.profile->statusivl) {
                 strcpy(querybuf,statuses[db[character].data->player.profile->statusivl]);
                 *querybuf = tolower(*querybuf);
                 setreturn(querybuf,COMMAND_SUCC);
	      }
	   } else setreturn(Engaged(character) ? "engaged":"married",COMMAND_SUCC);
	} else if(string_prefix("comments",arg2)) {
           if(hasprofile(db[character].data->player.profile) && db[character].data->player.profile->comments)
              setreturn(decompress(db[character].data->player.profile->comments),COMMAND_SUCC);
	} else if(string_prefix("country",arg2) || string_prefix("homecountry",arg2)) {
           if(hasprofile(db[character].data->player.profile) && db[character].data->player.profile->country)
              setreturn(decompress(db[character].data->player.profile->country),COMMAND_SUCC);
	} else if(string_prefix("hobbies",arg2) || string_prefix("activities",arg2) || string_prefix("hobby",arg2) || string_prefix("activity",arg2)) {
           if(hasprofile(db[character].data->player.profile) && db[character].data->player.profile->hobbies)
              setreturn(decompress(db[character].data->player.profile->hobbies),COMMAND_SUCC);
	} else if(string_prefix("likes",arg2)) {
           if(hasprofile(db[character].data->player.profile) && db[character].data->player.profile->likes)
              setreturn(decompress(db[character].data->player.profile->likes),COMMAND_SUCC);
	} else if(string_prefix("dislikes",arg2) || string_prefix("hates",arg2)) {
           if(hasprofile(db[character].data->player.profile) && db[character].data->player.profile->dislikes)
              setreturn(decompress(db[character].data->player.profile->dislikes),COMMAND_SUCC);
	} else if(string_prefix("height",arg2)) {
           if(hasprofile(db[character].data->player.profile) && db[character].data->player.profile->height) {
              int           major,minor;
              unsigned char metric;
              
              metric = ((db[character].data->player.profile->height & 0x8000) != 0);
              major  = ((db[character].data->player.profile->height & 0x7FFF) >> 7);
              minor  = (db[character].data->player.profile->height & 0x007F);
              if(major > 0) sprintf(querybuf,"%d%s",major,(metric) ? "m":"'");
                 else *querybuf = '\0';
              if(minor > 0) sprintf(querybuf + strlen(querybuf),"%s%d%s",(*querybuf) ? " ":"",minor,(metric) ? "cm":"\"");
              setreturn(querybuf,COMMAND_SUCC);
	   }
	} else if(string_prefix("weight",arg2)) {
           if(hasprofile(db[character].data->player.profile) && db[character].data->player.profile->weight) {
              int           major,minor;
              unsigned char metric;
              
              metric = ((db[character].data->player.profile->weight & 0x80000000) != 0);
              major  = ((db[character].data->player.profile->weight & 0x7FFF0000) >> 16);
              minor  = (db[character].data->player.profile->weight  & 0x0000FFFF);
              if(major > 0) sprintf(querybuf,"%d%s",major,(metric) ? "Kg":"lbs");
                 else *querybuf = '\0';
              if(minor > 0) sprintf(querybuf + strlen(querybuf),"%s%d%s",(*querybuf) ? " ":"",minor,(metric) ? "g":"oz");
              setreturn(querybuf,COMMAND_SUCC);
	   }
	} else if(string_prefix("drinks",arg2) || string_prefix("favouritedrinks",arg2)) {
           if(hasprofile(db[character].data->player.profile) && db[character].data->player.profile->drink)
              setreturn(decompress(db[character].data->player.profile->drink),COMMAND_SUCC);
	} else if(string_prefix("music",arg2) || string_prefix("favouritemusic",arg2)) {
           if(hasprofile(db[character].data->player.profile) && db[character].data->player.profile->music)
              setreturn(decompress(db[character].data->player.profile->music),COMMAND_SUCC);
	} else if(string_prefix("other",arg2) || string_prefix("miscellaneous",arg2)) {
           if(hasprofile(db[character].data->player.profile) && db[character].data->player.profile->other)
              setreturn(decompress(db[character].data->player.profile->other),COMMAND_SUCC);
	} else if(string_prefix("sports",arg2) || string_prefix("favouritesports",arg2)) {
           if(hasprofile(db[character].data->player.profile) && db[character].data->player.profile->sport)
              setreturn(decompress(db[character].data->player.profile->sport),COMMAND_SUCC);
	} else if(string_prefix("city",arg2) || string_prefix("town",arg2) || string_prefix("homecity",arg2) || string_prefix("hometown",arg2)) {
           if(hasprofile(db[character].data->player.profile) && db[character].data->player.profile->city)
              setreturn(decompress(db[character].data->player.profile->city),COMMAND_SUCC);
	} else if(string_prefix("eyes",arg2) || string_prefix("eyecolour",arg2) || string_prefix("eyecolor",arg2)) {
           if(hasprofile(db[character].data->player.profile) && db[character].data->player.profile->eyes)
              setreturn(decompress(db[character].data->player.profile->eyes),COMMAND_SUCC);
	} else if(string_prefix("foods",arg2) || string_prefix("favouritefoods",arg2)) {
           if(hasprofile(db[character].data->player.profile) && db[character].data->player.profile->food)
              setreturn(decompress(db[character].data->player.profile->food),COMMAND_SUCC);
	} else if(string_prefix("hair",arg2) || string_prefix("haircolour",arg2) || string_prefix("haircolor",arg2)) {
           if(hasprofile(db[character].data->player.profile) && db[character].data->player.profile->hair)
              setreturn(decompress(db[character].data->player.profile->hair),COMMAND_SUCC);
	} else if(string_prefix("name",arg2) || string_prefix("irl",arg2) || string_prefix("realname",arg2) || string_prefix("irlname",arg2)) {
           if(hasprofile(db[character].data->player.profile) && db[character].data->player.profile->irl)
              setreturn(decompress(db[character].data->player.profile->irl),COMMAND_SUCC);
	} else if(string_prefix("birthday",arg2) || string_prefix("dob",arg2) || string_prefix("dateofbirth",arg2) || string_prefix("birthdate",arg2)) {
           if(hasprofile(db[character].data->player.profile) && (db[character].data->player.profile->dob != UNSET_DATE))
              setreturn(date_to_string(UNSET_DATE,db[character].data->player.profile->dob,player,SHORTDATEFMT),COMMAND_SUCC);
	} else if(string_prefix("birthdayepoch",arg2) || string_prefix("dobepoch",arg2) || string_prefix("dateofbirthepoch",arg2) || string_prefix("birthdateepoch",arg2)) {
           if(hasprofile(db[character].data->player.profile) && (db[character].data->player.profile->dob != UNSET_DATE)) {
              sprintf(querybuf,"%d",(int) longdate_to_epoch(db[character].data->player.profile->dob));
              setreturn(querybuf,COMMAND_SUCC);
	   }
	} else if(string_prefix("birthdaylongdate",arg2) || string_prefix("doblongdate",arg2) || string_prefix("dateofbirthlongdate",arg2) || string_prefix("birthdatelongdate",arg2)) {
           if(hasprofile(db[character].data->player.profile) && (db[character].data->player.profile->dob != UNSET_DATE)) {
              sprintf(querybuf,"%ld",db[character].data->player.profile->dob);
              setreturn(querybuf,COMMAND_SUCC);
	   }
	} else if(string_prefix("url",arg2) || string_prefix("pictureurl",arg2) || string_prefix("imageurl",arg2) || string_prefix("portraiturl",arg2) || string_prefix("galleryurl",arg2)) {
           if(hasprofile(db[character].data->player.profile) && db[character].data->player.profile->picture)
              setreturn(decompress(db[character].data->player.profile->picture),COMMAND_SUCC);
	} else if(string_prefix("email",arg2) || string_prefix("emailaddress",arg2)) {
           const char *email = gettextfield(0,'\n',getfield(character,EMAIL),(Level4(db[player].owner) || can_write_to(player,character,1)) ? 0:2,scratch_return_string);
           if(!Blank(email)) setreturn(email,COMMAND_SUCC);
           return;
	} else if(string_prefix("wwwpages",arg2) || string_prefix("homepages",arg2) || string_prefix("webpages",arg2) || string_prefix("worldwidewebpages",arg2)) {
           const char *ptr = getfield(character,WWW);
           if(ptr) setreturn(ptr,COMMAND_SUCC);
	} else if(string_prefix("age",arg2)) {
           if(hasprofile(db[character].data->player.profile) && (db[character].data->player.profile->dob != UNSET_DATE)) {
              time_t now;

              gettime(now);
              sprintf(querybuf,"%ld",longdate_difference(db[character].data->player.profile->dob,epoch_to_longdate(now)) / 12);
              setreturn(querybuf,COMMAND_SUCC);
	   }
	} else if(player == character) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, your profile doesn't have the field '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",arg2);
           else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s profile doesn't have the field '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",Article(character,LOWER,DEFINITE),getcname(NOTHING,character,0,0),arg2);
     } else if(hasprofile(db[character].data->player.profile)) setreturn(OK,COMMAND_SUCC);
}

/* ---->  Return character's prompt  <---- */
void query_prompt(CONTEXT)
{
     const  char *ptr = tcz_prompt;
     struct descriptor_data *p;
     dbref  character;

     character = query_find_character(player,params,1);
     if(!Validchar(character)) return;

     if((p = getdsc(character))) ptr = (p->edit) ? p->edit->prompt:(p->prompt) ? p->prompt->prompt:p->user_prompt;
     if(!ptr) ptr = tcz_prompt;
     sprintf(querybuf,"%s%s",(*ptr != '\x1B') ? ANSI_LWHITE:"",ptr);
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Pronoun queries ('{@?subjective}', '{@?objective}', '{@?possessive}', '{@?reflexive}' and '{@?absolute}')  <---- */
/*        (val1:  0 = Subjective, 1 = Objective, 2 = Possessive, 3 = Reflexive, 4 = Absolute.)  */
void query_pronoun(CONTEXT)
{
     dbref         character;
     unsigned char upper;

     character = query_find_character(player,params,0);
     upper     = (isupper(arg0[2])) ? 1:0;
     if(!Validchar(character)) return;

     switch(val1) {
            case 0:  /*  Subjective  */
                 setreturn(Subjective(character,upper),COMMAND_SUCC);
                 break;
            case 1:  /*  Objective  */
                 setreturn(Objective(character,upper),COMMAND_SUCC);
                 break;
            case 2:  /*  Possessive  */
                 setreturn(Possessive(character,upper),COMMAND_SUCC);
                 break;
            case 3:  /*  Reflexive  */
                 setreturn(Reflexive(character,upper),COMMAND_SUCC);
                 break;
            case 4:  /*  Absolute  */
                 setreturn(Absolute(character,upper),COMMAND_SUCC);
                 break;
     }
}

/* ---->  {J.P.Boggis 29/01/2000}  Protect string from %-substitutions  <---- */
void query_protect(CONTEXT)
{
     char   *ptr = querybuf;
     const  char *string;
     struct arg_data arg;
     const  char *subst;

     int s_backslash = 0;
     int s_bracket   = 0;
     int s_percent   = 0;
     int s_dollar    = 0;

     unparse_parameters(params,2,&arg,0);
     subst  = arg.text[0];
     string = arg.text[1];
     if(!string || !subst) return;

     if(!Blank(subst)) {
        if(!string_prefix("ALL",subst)) {
           if(!string_prefix("CMD",subst)) {
              if(strchr(subst,'b') || strchr(subst,'B')) s_backslash = 1;
              if(strchr(subst,'c') || strchr(subst,'C')) s_bracket   = 1;
              if(strchr(subst,'s') || strchr(subst,'S')) s_percent   = 1;
              if(strchr(subst,'v') || strchr(subst,'V')) s_dollar    = 1;
	   } else s_backslash = s_bracket = s_dollar = 1;
	} else s_backslash = s_bracket = s_percent = s_dollar = 1;
     }
 
     for(; *string; string++) {
	 switch(*string) {
		case '\\':
		     if(s_backslash) *ptr++ = '\\';
		     break;
		case '{':
		     if(s_bracket) *ptr++ = '\\';
		     break;
		case '%':
		     if(s_percent) *ptr++ = '%';
		     break;
		case '$':
		     if(s_dollar) *ptr++ = '\\';
		     break;
		default:
		     break;
	 }
	 *ptr++ = *string;
     }
     *ptr = '\0';
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Return amount of Building Quota in use by character/object  <---- */
void query_quota(CONTEXT)
{
     dbref thing = query_find_object(player,params,SEARCH_PREFERRED,0,0);
     if(!Valid(thing)) return;
     sprintf(querybuf,"%d",(Typeof(thing) == TYPE_CHARACTER) ? db[thing].data->player.quota:(Typeof(thing) != TYPE_ARRAY) ? ObjectQuota(thing):ObjectQuota(thing) + (array_element_count(db[thing].data->array.start) * ELEMENT_QUOTA));
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Return character's Building Quota limit  <---- */
void query_quotalimit(CONTEXT)
{
     dbref character = query_find_character(player,params,0);
     if(!Validchar(character)) return;
     sprintf(querybuf,"%ld",(!Level4(character)) ? db[character].data->player.quotalimit:TCZ_INFINITY);
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Return character's race  <---- */
void query_race(CONTEXT)
{
     dbref character = query_find_character(player,params,0);
     if(!Validchar(character) || !HasField(character,RACE)) return;
     setreturn(String(getfield(character,RACE)),COMMAND_SUCC);
}

/* ---->  Return a random number  <---- */
void query_rand(CONTEXT)
{
     if(!Blank(arg2)) {
        int temp,lower,upper,difference;

        /* ---->  Random number within given range of numbers  <---- */
        lower = atol(arg1), upper = atol(arg2), difference = ABS(lower - upper);
        if(lower > upper) temp = lower, lower = upper, upper = temp;
        if(difference != 0)
           sprintf(querybuf,"%d",lower + (((int) lrand48()) % ++difference));
	      else sprintf(querybuf,"%d",lower);
        setreturn(querybuf,COMMAND_SUCC);
     } else {
        int range,number;

        /* ---->  Random number between 0 and given number - 1  <---- */
        if((range = atol(params)) != 0) {
           number = lrand48() % ABS(range);
           sprintf(querybuf,"%s%d",((range < 0) && number) ? "-":"",number);
           setreturn(querybuf,COMMAND_SUCC);
	} else setreturn("0",COMMAND_SUCC);
     }
}

/* ---->  Return number as a rank (1st, 2nd, etc.)  <---- */
void query_rank(CONTEXT)
{
     setreturn(rank(atoi(params)),COMMAND_SUCC);
}

/* ---->  Return date/time stored in EPOCH or LONGDATE format as a string  <---- */
void query_realtime(CONTEXT)
{
     char          *start,*ptr,*customformat = NULL;
     const    char *dateformat = FULLDATEFMT;
     unsigned char epoch = 1;
     time_t        now = -1;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(arg2)) {
        char *dttype;

        /* ---->  Date/time type (EPOCH or LONGDATE)  <---- */
        split_params((char *) arg2,&dttype,&customformat);
        for(ptr = dttype; *ptr && (*ptr == ' '); ptr++);
        while(*ptr) {
              for(start = ptr; *ptr && (*ptr != ' '); ptr++);
              if(*ptr) for(*ptr++ = '\0'; *ptr && (*ptr == ' '); ptr++);

              if(!Blank(start)) {
		 if(string_prefix("long",start) || string_prefix("dates",start)) {
                    dateformat = LONGDATEFMT;
		 } else if(string_prefix("longdates",start)) {
                    epoch = 0;
		 } else if(string_prefix("epoch",start)) {
                    epoch = 1;
		 } else if(string_prefix("shortdates",start)) {
                    dateformat = SHORTDATEFMT;
		 } else if(string_prefix("times",start)) {
                    dateformat = TIMEFMT;
		 } else if(string_prefix("fulldates",start)) {
                    dateformat = FULLDATEFMT;
		 }
	      }
	}

        if(!Blank(customformat))
           dateformat = customformat;
     }

     while(*arg1 && (*arg1 == ' ')) arg1++;
     if(epoch) {
        unsigned char convert = 0;

        if(!Blank(arg1)) {
           if(strcasecmp(arg1,INVALID_DATE)) {
              if(!(now = atol(arg1)) && (*arg1 != '0'))
                 convert = 1;
	   } else return;
	} else convert = 1;

        if(convert) {
           gettime(now);
           now += (db[player].data->player.timediff * HOUR);
	}

	setreturn(date_to_string(now,UNSET_DATE,Blank(customformat) ? player:NOTHING,dateformat),COMMAND_SUCC);
     } else {
        unsigned char convert = 1;
        unsigned long longdate;

        if(!Blank(arg1))
           if((longdate = atol(arg1)) || (*arg1 == '0'))
              convert = 0;
        if(convert || Blank(arg1)) {
           gettime(now);
           now += (db[player].data->player.timediff * HOUR);
           longdate = epoch_to_longdate(now);
	}

	setreturn(date_to_string(UNSET_DATE,longdate,Blank(customformat) ? player:NOTHING,dateformat),COMMAND_SUCC);
     }
}

/* ---->  {@?replace "<SEARCHSTR>" "<REPLACESTR>" <OCCURENCE> "<STRING>"}  <---- */
/*        (val1:  0 = {@?replace}, 1 = .replace)                                 */
char query_replace(CONTEXT)
{
     unsigned char     all = 0,last = 0,line = 0,overflow = 0;
     char              temp[TEXT_SIZE],temp2[TEXT_SIZE];
     short             loop = 0,loop2,replaced = 0;
     char              *ptr,*tmp;
     struct   str_ops  str_data;
     struct   arg_data arg;

     unparse_parameters(params,4,&arg,1);
     if(val1 && (arg.count == 3)) {
        arg.text[3] = arg.text[2];
        arg.text[2] = "FIRST";
        arg.numb[2] = 1;
        arg.count   = 4;
     }

     if((arg.count < 4) || Blank(arg.text[0]) || ((arg.numb[2] <= 0) && (arg.numb[2] != LAST) && (arg.numb[2] != ALL) && !(!Blank(arg.text[2]) && (*(arg.text[2]) == '+')))) {
        output(getdsc(player),player,0,1,11,ANSI_LGREEN"[REPLACE]  "ANSI_LWHITE"Error  -  Invalid parameter(s).");
        setreturn(arg.text[3],COMMAND_FAIL);
        return(0);
     } else {
        str_data.length = 0;
        str_data.dest   = temp;
        ptr             = arg.text[3];

        /* ---->  Replace occurence on every line?  <---- */
        if(!Blank(arg.text[2]) && (*(arg.text[2]) == '+')) {
           for(line = 1; *(arg.text[2]) && (*(arg.text[2]) == '+'); (arg.text[2])++);
           for(; *(arg.text[2]) && (*(arg.text[2]) == ' '); (arg.text[2])++);
           if(!strcasecmp("ALL",arg.text[2])) arg.numb[2] = ALL;
              else if(!strcasecmp("FIRST",arg.text[2])) arg.numb[2] = 1;
                 else if(!strcasecmp("LAST",arg.text[2])) arg.numb[2] = LAST;
                    else if(((arg.numb[2] = atol(arg.text[2])) <= 0) || !strcasecmp("END",arg.text[2])) {
                       output(getdsc(player),player,0,1,11,ANSI_LGREEN"[REPLACE]  "ANSI_LWHITE"Error  -  Invalid parameter(s).");
                       setreturn(arg.text[3],COMMAND_FAIL);
                       return(0);
		    }
               line = 1;
	    }
            if(arg.numb[2] == LAST) last = 1;
               else if(arg.numb[2] == ALL) all = 1, line = 0;

            while(*ptr) {
     	          for(; !(val1 && overflow) && *ptr && strncasecmp(ptr,arg.text[0],arg.len[0]); overflow = !strcat_limits_char(&str_data,*ptr++))
                      if(line && (*ptr == '\n')) loop = 0;
                  if(val1 && overflow) return(-1);

                  /* ---->  <SEARCHSTR> found...  <---- */
	  	  if(*ptr) {
		     for(loop++, loop2 = 0, tmp = temp2; *ptr && (loop2 < arg.len[0]); *tmp++ = *ptr++, loop2++)
                         if(line && (*ptr == '\n')) loop = 0;
 		     *tmp = '\0';

                     /* ---->  Replace last occurence?  <---- */
                     loop2 = 0;
	  	     if(last)
		        for(tmp = ptr; *tmp && !loop2; tmp++)
			    if(!strncasecmp(tmp,arg.text[0],arg.len[0])) loop2 = 1;

                     /* ---->  Replace this occurence of <SEARCHSTR> with <REPLACESTR>?  <---- */
	  	     if(all || (!last && (loop == arg.numb[2])) || (last && !loop2))
                        overflow = !strcat_limits(&str_data,arg.text[1]), replaced++;
			   else overflow = !strcat_limits(&str_data,temp2);
                     if(val1 && overflow) return(-1);
		  }
	    }
            *str_data.dest = '\0';

            if(!in_command) {
               if(replaced < 1) output(getdsc(player),player,0,1,11,ANSI_LGREEN"[REPLACE]  "ANSI_LWHITE"No occurences of '"ANSI_LYELLOW"%s"ANSI_LWHITE"' found  -  Nothing replaced.",arg.text[0]);
	          else if(all) output(getdsc(player),player,0,1,11,ANSI_LGREEN"[REPLACE]  "ANSI_LWHITE"All ("ANSI_LYELLOW"%d"ANSI_LWHITE") occurences of '"ANSI_LYELLOW"%s"ANSI_LWHITE"' replaced with '"ANSI_LYELLOW"%s"ANSI_LWHITE"'.",replaced,arg.text[0],arg.text[1]);
	             else if(last) output(getdsc(player),player,0,1,11,ANSI_LGREEN"[REPLACE]  "ANSI_LWHITE"Last occurence of '"ANSI_LYELLOW"%s"ANSI_LWHITE"' replaced with '"ANSI_LYELLOW"%s"ANSI_LWHITE"'%s.",arg.text[0],arg.text[1],(line) ? " on each line in the specified range":"");
	                else output(getdsc(player),player,0,1,11,ANSI_LGREEN"[REPLACE]  "ANSI_LWHITE"%s occurence of '"ANSI_LYELLOW"%s"ANSI_LWHITE"' replaced with '"ANSI_LYELLOW"%s"ANSI_LWHITE"'%s.",rank(arg.numb[2]),arg.text[0],arg.text[1],(line) ? " on each line in the specified range":"");
	    }
            setreturn(temp,COMMAND_SUCC);
            return(1);
     }
}

/* ---->  {@?result <VALUE> = <TRUE> = <FALSE>} <---- */
void query_result(CONTEXT)
{
     struct arg_data arg;

     for(; *arg1 && (*arg1 == ' '); arg1++);
     unparse_parameters(arg2,2,&arg,0);

     if(!Blank(arg1) && (!strcmp("0",arg1) || !strcasecmp(ERROR,arg1) || !strcasecmp(UNSET_VALUE,arg1))) setreturn(arg.text[1],COMMAND_FAIL);
        else setreturn(arg.text[0],COMMAND_SUCC);
}

/* ---->  {@?rightstr <COUNT> "<STRING>"}  <---- */
void query_rightstr(CONTEXT)
{
     char   temp[TEXT_SIZE];
     struct arg_data arg;
     char   *ptr,*tmp;
     short  loop;

     unparse_parameters(params,2,&arg,0);
     if(!((arg.count < 2) || (arg.numb[0] <= 0))) {
        ptr = arg.text[1] + arg.len[1] - 1, tmp = temp;
        for(loop = 1; (ptr > arg.text[1]) && (loop < arg.numb[0]); loop++, ptr--);
        for(loop = 0; *ptr && (loop < arg.numb[0]); loop++, *tmp++ = *ptr++);
        *tmp = '\0';
        setreturn(temp,COMMAND_SUCC);
     } else setreturn("",COMMAND_SUCC);
}

/* ---->  Return character's score  <---- */
void query_score(CONTEXT)
{
     dbref character = query_find_character(player,params,0);

     if(!Validchar(character)) return;
     sprintf(querybuf,"%d",db[character].data->player.score);
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Return character's screen height  <---- */
void query_screenheight(CONTEXT)
{
     dbref character;

     character = query_find_character(player,params,0);
     if(!Validchar(character)) return;
     sprintf(querybuf,"%d",db[character].data->player.scrheight);
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Return character's screen width (Word wrap setting)  <---- */
void query_screenwidth(CONTEXT)
{
     dbref character;
     int   width;

     character = query_find_character(player,params,0);
     if(!Validchar(character)) return;
     width = output_terminal_width(character) + 1;
     sprintf(querybuf,"%d",width);
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->     Return size of an object (In bytes)     <---- */
/*        (val1:  0 = Compressed, 1 = Decompressed)        */
void query_size(CONTEXT)
{
     dbref thing = query_find_object(player,params,SEARCH_PREFERRED,0,0);
     if(!Valid(thing)) return;
     sprintf(querybuf,"%ld",getsize(thing,val1));
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Return separating line  <---- */
void query_separator(CONTEXT)
{
     unsigned char            twidth = output_terminal_width(player);
     const    char            *ptr;

     if(!Blank(params)) {
        if(string_prefix("single",params)) {
           strcpy(querybuf,separator(twidth,0,'-','-'));
           for(ptr = querybuf; *ptr && (*ptr != '-'); ptr++);
           setreturn(ptr,COMMAND_SUCC);
           return;
        } else if(string_prefix("double",params)) {
           strcpy(querybuf,separator(twidth,0,'=','='));
           for(ptr = querybuf; *ptr && (*ptr != '='); ptr++);
           setreturn(ptr,COMMAND_SUCC);
           return;
        }
     }
     strcpy(querybuf,separator(twidth,0,'-','='));
     for(ptr = querybuf; *ptr && (*ptr != '-'); ptr++);
     setreturn(ptr,COMMAND_SUCC);
}

/* ---->  Specified flag(s)/friend flag(s) set on specified object/character?  <---- */
void query_set(CONTEXT)
{
     dbref         thing = query_find_object(player,arg1,SEARCH_PREFERRED,0,0);
     unsigned char negate,flag_set,found,skip,fothers = 0;
     int           count = 0,flags = 0,dummy;
     char          *ptr,*flaglist,*list;
     short         i;

     if(!Valid(thing)) return;
     split_params((char *) arg2,&flaglist,&list);
     if(!Blank(list)) {
        if(string_prefix("fothers",list) || string_prefix("others",list)) {
           fothers = 1;
	} else if(!(string_prefix("flist",list) || string_prefix("list",list))) {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LWHITE"flist"ANSI_LGREEN"' or '"ANSI_LWHITE"fothers"ANSI_LGREEN"'.");
           return;
	}
     }

     if(!Blank(flaglist)) {

        /* ---->  Parse flags  <---- */
        for(; *flaglist && (*flaglist == ' '); flaglist++);
        if(*flaglist && (*flaglist != '*')) while(*flaglist) {
           while(*flaglist && (*flaglist == ' ')) flaglist++;
           if(*flaglist && (*flaglist == '!')) {
              for(; *flaglist && (*flaglist == '!'); flaglist++);
              for(; *flaglist && (*flaglist == ' '); flaglist++);
              negate = 1;
	   } else negate = 0;
           if(*flaglist) {
 
              /* ---->  Grab flag name  <---- */
              for(ptr = scratch_buffer; *flaglist && (*flaglist != ' '); *ptr++ = *flaglist++);
              *ptr = '\0', found = 0;

              if(Blank(list)) {

                 /* ---->  Search primary flags  <---- */
                 for(i = 0, skip = 0; !skip && (flag_list[i].string != NULL) && !found; i++) {
                     if(string_prefix(flag_list[i].string,scratch_buffer)) {
                        if(!(flag_list[i].flags & FLAG_SKIP)) {
                           flag_set = ((db[thing].flags & flag_list[i].mask) == flag_list[i].flag);
                           if((negate && flag_set) || (!negate && !flag_set)) return;
                           count++, found = 1;
			} else skip = 1;
		     }
		 }

                 /* ---->  Search secondary flags  <---- */
                 for(i = 0, skip = 0; (flag_list2[i].string != NULL) && !found; i++) {
                     if(string_prefix(flag_list2[i].string,scratch_buffer)) {
                        if(!(flag_list2[i].flags & FLAG_SKIP)) {
                           flag_set = ((db[thing].flags2 & flag_list2[i].mask) == flag_list2[i].flag);
                           if((negate && flag_set) || (!negate && !flag_set)) return;
                           count++, found = 1;
			} else skip = 1;
		     }
		 }
	      }

              if(!found && (Typeof(thing) == TYPE_CHARACTER) && parse_friendflagtype(scratch_buffer,&flags,&dummy)) {
                 if(fothers) {
                    if(!(dummy = friend_flags(thing,player))) dummy = FRIEND_STANDARD;
		 } else if(!(dummy = friend_flags(player,thing))) dummy = FRIEND_STANDARD;
                 if((negate && (dummy & flags)) || (!negate && !(dummy & flags))) return;
                 count++, found = 1;
	      }
              if(!found) return;
	   }
	}
        if(count) setreturn(OK,COMMAND_SUCC);
     }
}

/* ---->  {@?sort[alpha|numeric] [ascending|descending] [<KEY>] "<SEPARATOR>" "<LIST>"}  <---- */
/*        (val1:  0 = Alphabetical sort, 1 = Numerical sort)       <---- */
void query_sort(CONTEXT)
{
     char              temp[TEXT_SIZE],temp2[TEXT_SIZE],temp3[TEXT_SIZE],temp4[TEXT_SIZE];
     short             loop = 1,loop2,item;
     char              *ptr,*tmp,*tmp2;
     unsigned char     ascending = 1;
     int               value;
     struct   arg_data arg;

     if(val1) {
        unparse_parameters(params,4,&arg,0);
        if(arg.count < 4) {
           setreturn(arg.text[3],COMMAND_SUCC);
           return;
	} else {
           arg.text[1] = arg.text[2], arg.text[2] = arg.text[3];
           arg.len[1]  = arg.len[2],  arg.len[2]  = arg.len[3];
	}
     } else unparse_parameters(params,3,&arg,0);

     if(!((arg.count < 3) || Blank(arg.text[0]) || Blank(arg.text[1]) || Blank(arg.text[2]))) {
        *temp = '\0';
        if(string_prefix("DESCENDING",arg.text[0]) || string_prefix("REVERSE",arg.text[0])) ascending = 0;

        strcpy(temp4,arg.text[2]);
        while(!BlankContent(temp4) && (loop > 0)) {

              /* ---->  Skip <SEPARATOR>  <---- */
              ptr = temp4, tmp = temp2, item = 0, loop = 0;
              if(*ptr && !strncasecmp(ptr,arg.text[1],arg.len[1]))
                 ptr += arg.len[1], loop++;
 
              if(*ptr) loop++, item = loop;
              for(; *ptr && strncasecmp(ptr,arg.text[1],arg.len[1]); *tmp++ = *ptr++);
              *tmp = '\0';

              /* ---->  Numerical sort key  <---- */
              if(val1) {
                 for(tmp2 = temp2, loop2 = 1; *tmp2 && !isdigit(*tmp2); tmp2++);
                 while(*tmp2 && (loop2 < arg.numb[1])) {
                       for(; *tmp2 && isdigit(*tmp2);  tmp2++);
                       for(; *tmp2 && !isdigit(*tmp2); tmp2++);
                       loop2++;
		 }
                 value = atol(tmp2);
	      }

              /* ---->  Find lowest (Ascending)/highest (Descending) item in list  <---- */
              while(*ptr) {
 
		    /* ---->  Skip <SEPARATOR>  <---- */
                    if(*ptr && !strncasecmp(ptr,arg.text[1],arg.len[1]))
                       ptr += arg.len[1], loop++;

                    if(*ptr) {
                       for(tmp = temp3; *ptr && strncasecmp(ptr,arg.text[1],arg.len[1]); *tmp++ = *ptr++);
                       *tmp = '\0';

                       if(!BlankContent(temp3)) {
                          if(val1) {

                             /* ---->  Numerical sort  <---- */
                             for(tmp = temp3, loop2 = 1; *tmp && !isdigit(*tmp); tmp++);
                             while(*tmp && (loop2 < arg.numb[1])) {
                                   for(; *tmp && isdigit(*tmp);  tmp++);
                                   for(; *tmp && !isdigit(*tmp); tmp++);
                                   loop2++;
			     }
                             if((!ascending && (value < atol(tmp))) || (ascending && (value > atol(tmp))))
                                strcpy(temp2,temp3), item = loop, tmp2 = temp2 + (tmp - temp3), value = atol(tmp);

                             /* ---->  Alphabetical sort  <---- */
		          } else if((ascending && (strcasecmp(temp3,temp2) < 0)) || (!ascending && (strcasecmp(temp3,temp2) > 0)))
                             strcpy(temp2,temp3), item = loop;
		       }
		    }
	      }

              /* ---->  Add this item to new (Sorted) list and remove it from the old one  <---- */
              if(!item) item = 1;
              if(!BlankContent(temp2)) {
                 if(!BlankContent(temp)) {
                    strcat(temp,arg.text[1]);
                    strcat(temp,temp2);
		 } else strcpy(temp,temp2);
                 sprintf(temp2,"%d \x02%s\x02 \x02%s\x02",item,arg.text[1],temp4);
                 query_delete(player,temp2,NULL,NULL,NULL,val1,0);
                 strcpy(temp4,command_result);
	      } else {
                 sprintf(temp2,"%d \x02%s\x02 \x02%s\x02",item,arg.text[1],temp4);
                 query_delete(player,temp2,NULL,NULL,NULL,val1,0);
                 strcpy(temp4,command_result);
	      }
	}
        setreturn(temp,COMMAND_SUCC);
     } else setreturn(arg.text[2],COMMAND_SUCC);
}

/* ---->      Return #ID of special room (If set)      <---- */
/*        (val1:  0 = BBS, 1 = Post Office, 2 = Bank)        */
void query_specialroom(CONTEXT)
{
     dbref room = NOTHING;

     switch(val1) {
            case 0:

                 /* ---->  BBS  <---- */
                 if(Valid(bbsroom)) room = bbsroom;
                 break;
            case 1:

                 /* ---->  Post Office  <---- */
                 if(Valid(mailroom)) room = mailroom;
                 break;
            case 2:

                 /* ---->  Bank  <---- */
                 if(Valid(bankroom)) room = bankroom;
                 break;
     }

     if(!Valid(room)) setreturn(ERROR,COMMAND_FAIL);
        else setreturn(getnameid(player,room,NULL),COMMAND_SUCC);
}

/* ---->  Return status (Rank) of user  <---- */
void query_status(CONTEXT)
{
      dbref         character = query_find_character(player,arg1,0);
      unsigned char colour = 0,shortened = 0;

      if(Validchar(character)) {
         if(!Blank(arg2)) {
            if(string_prefix("short",arg2)) shortened = 1;
               else if(string_prefix("colour",arg2) || string_prefix("color",arg2)) colour = 1;
	 }

         if(Moron(character)) {
            setreturn((colour) ? MORON_COLOUR:(shortened) ? "M":"Moron",COMMAND_SUCC);
	 } else if(Level1(character)) {
            setreturn((colour) ? DEITY_COLOUR:(shortened) ? "D":"Deity",COMMAND_SUCC);
	 } else if(Level2(character)) {
	    if(Druid(character)) setreturn((colour) ? ELDER_DRUID_COLOUR:(shortened) ? "e":"Elder Druid",COMMAND_SUCC);
	       else setreturn((colour) ? ELDER_COLOUR:(shortened) ? "E":"Elder Wizard",COMMAND_SUCC);
	 } else if(Level3(character)) {
	    if(Druid(character)) setreturn((colour) ? DRUID_COLOUR:(shortened) ? "w":"Druid",COMMAND_SUCC);
	       else setreturn((colour) ? WIZARD_COLOUR:(shortened) ? "W":"Wizard",COMMAND_SUCC);
	 } else if(Level4(character)) {
	    if(Druid(character)) setreturn((colour) ? APPRENTICE_DRUID_COLOUR:(shortened) ? "a":"Apprentice Druid",COMMAND_SUCC);
	       else setreturn((colour) ? APPRENTICE_COLOUR:(shortened) ? "A":"Apprentice Wizard",COMMAND_SUCC);
	 } else if(Retired(character)) {
	    if(RetiredDruid(character)) setreturn((colour) ? RETIRED_DRUID_COLOUR:(shortened) ? "r":"Retired Druid",COMMAND_SUCC);
	       else setreturn((colour) ? RETIRED_COLOUR:(shortened) ? "R":"Retired Wizard",COMMAND_SUCC);
	 } else if(Experienced(character)) {
            setreturn((colour) ? EXPERIENCED_COLOUR:(shortened) ? "X":"Experienced Builder",COMMAND_SUCC);
	 } else if(Assistant(character)) {
            setreturn((colour) ? ASSISTANT_COLOUR:(shortened) ? "x":"Assistant",COMMAND_SUCC);
	 } else if(Builder(character)) {
            setreturn((colour) ? BUILDER_COLOUR:(shortened) ? "B":"Builder",COMMAND_SUCC);
	 } else if(Being(character)) {
            setreturn((colour) ? MORTAL_COLOUR:(shortened) ? "b":"Being",COMMAND_SUCC);
	 } else if(Puppet(character)) {
            setreturn((colour) ? MORTAL_COLOUR:(shortened) ? "p":"Puppet",COMMAND_SUCC);
	 } else setreturn((colour) ? MORTAL_COLOUR:(shortened) ? "-":"Mortal",COMMAND_SUCC);
      } else setreturn(ERROR,COMMAND_FAIL);
}

/* ---->   {@?[upper|lower|capitalise] "<STRING>"}   <---- */
/*        (val1:  0 = Upper, 1 = Lower, 2 = Capitalise.)        */
void query_strcase(CONTEXT)
{
     char   temp[TEXT_SIZE];
     struct arg_data arg;
     char   *ptr,*tmp;

     unparse_parameters(params,1,&arg,0);
     if(arg.count >= 1) {
        switch(val1) {
               case 0:
	            for(ptr = arg.text[0], tmp = temp; *ptr; ptr++)
                        if(islower(*ptr)) *tmp++ = toupper(*ptr);
                           else *tmp++ = *ptr;
                    *tmp = '\0';
                    break;
               case 1:
                    for(ptr = arg.text[0], tmp = temp; *ptr; ptr++)
                        if(isupper(*ptr)) *tmp++ = tolower(*ptr);
                           else *tmp++ = *ptr;
                    *tmp = '\0';
                    break;
               case 2:
                    strcpy(temp,arg.text[0]);
                    for(ptr = temp; *ptr && (*ptr == ' '); ptr++);
                    if(*ptr && islower(*ptr)) *ptr = toupper(*ptr);
                    break;
	}
        setreturn(temp,COMMAND_SUCC);
     } else setreturn("",COMMAND_SUCC);
}

/* ---->  {@?strlen "<STRING>"}  (Or {@?length "<STRING>"})  <---- */
void query_strlen(CONTEXT)
{
     struct arg_data arg;
     char   result[16];

     unparse_parameters(params,1,&arg,0);
     if (arg.count == 0) 
             sprintf(result,"0");
     else
             sprintf(result,"%d",arg.len[0]);
     setreturn(result,COMMAND_SUCC);
}

/* ---->  {@?strpos "<SUBSTRING>" "<STRING>"}  <---- */
void query_strpos(CONTEXT)
{
     struct arg_data arg;
     char   result[16];
     short  loop = 0;
     char   *ptr;

     unparse_parameters(params,2,&arg,0);
     if(!((arg.count < 2) || Blank(arg.text[0]) || Blank(arg.text[1]))) {
        for(ptr = arg.text[1]; *ptr; ptr++) {
	    loop++;
            if(!strncasecmp(ptr,arg.text[0],arg.len[0])) {
	       sprintf(result,"%d",loop);
               setreturn(result,COMMAND_SUCC);
	       return;
	    }
	}
  	setreturn("0",COMMAND_SUCC);
     } else setreturn("0",COMMAND_SUCC);
}

/* ---->  {@?strprefix "<SUBSTRING>" "<STRING>"}  <---- */
void query_strprefix(CONTEXT)
{
     struct arg_data arg;

     ansi_code_filter((char *) params,(char *) params,1);
     unparse_parameters(params,2,&arg,0);
     if(!((arg.count < 2) || Blank(arg.text[0]))) {
        if(!strncasecmp(arg.text[1],arg.text[0],arg.len[0])) setreturn(OK,COMMAND_SUCC);
           else setreturn(ERROR,COMMAND_FAIL);
     } else setreturn(ERROR,COMMAND_FAIL);
}

/* ---->  Return success message of an object  <---- */
void query_succ(CONTEXT)
{
     dbref thing = query_find_object(player,params,SEARCH_PREFERRED,1,1);
     if(!Valid(thing) || (Typeof(thing) == TYPE_CHARACTER) || !HasField(thing,SUCC)) return;
     setreturn(String(getfield(thing,SUCC)),COMMAND_SUCC);
}

/* ---->  Return character's name suffix  <---- */
void query_suffix(CONTEXT)
{
     dbref character = query_find_character(player,params,0);
     if(!Validchar(character) || (Typeof(character) != TYPE_CHARACTER)) return;
     setreturn(String(getfield(character,SUFFIX)),COMMAND_SUCC);
}

/* ---->  {@?tail "<SEPARATOR>" "<LIST>"}  <---- */
void query_tail(CONTEXT)
{
     struct arg_data arg;
     char   *ptr;

     unparse_parameters(params,2,&arg,0);
     if(!((arg.count < 2) || Blank(arg.text[0]) || Blank(arg.text[1]))) {
        for(ptr = arg.text[1]; *ptr; ptr++)
            if(!strncasecmp(ptr,arg.text[0],arg.len[0])) {
               ptr += arg.len[0];
               setreturn(ptr,COMMAND_SUCC);
	       return;
	    }
     }
     setreturn("",COMMAND_SUCC);
}

/* ---->  Return character's terminal type  <---- */
void query_terminaltype(CONTEXT)
{
     struct descriptor_data *d;
     dbref  character;

     character = query_find_character(player,params,0);
     if(!Validchar(character)) return;
     for(d = descriptor_list; d && (d->player != character); d = d->next);
     if(!d) return;
     sprintf(querybuf,"%s",!Blank(d->terminal_type) ? d->terminal_type:"none");
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Return current time in either EPOCH or LONGDATE format integer date/time representation  <---- */
/*        (VAL1:  0 = {@?time}, 1 = {@?quarter}.)  */
void query_time(CONTEXT)
{
     time_t now;

     switch(val1) {
            case 0:
                 gettime(now);
                 break;
            case 1:
                 now = quarter;
                 if(string_prefix("start",arg1)) now -= QUARTER;
                 arg1 = NULL;                 
                 break;
     }
     if(!Blank(arg2)) arg1 = arg2;
     if(!Blank(arg1) && string_prefix("longdates",arg1))
        sprintf(querybuf,"%ld",epoch_to_longdate(now));
           else sprintf(querybuf,"%d",(int) now);
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Return character's time difference setting  <---- */
void query_timediff(CONTEXT)
{
     dbref character = query_find_character(player,params,0);
     if(!Validchar(character) || (Typeof(character) != TYPE_CHARACTER)) return;
     sprintf(querybuf,"%d",db[character].data->player.timediff);
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  {J.P.Boggis 08/07/2000}  Return character's time spent active  <---- */
void query_totalactive(CONTEXT)
{
     time_t total,idle,active,now;
     struct descriptor_data *w;

     dbref character = query_find_character(player,params,0);
     if(!Validchar(character)) return;
     w = getdsc(character);

     gettime(now);
     total = db[character].data->player.totaltime;
     if(Connected(character)) total += (now - db[character].data->player.lasttime);

     idle = db[character].data->player.idletime;
     if(Connected(character) && w) idle += (now - w->last_time);
     active = total - idle;

     if(active > 0) sprintf(querybuf,"%d",(int) active);
        else strcpy(querybuf,"0");
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  {J.P.Boggis 08/07/2000}  Return character's time spent idling  <---- */
void query_totalidle(CONTEXT)
{
     struct descriptor_data *w;
     time_t total,idle,now;

     dbref character = query_find_character(player,params,0);
     if(!Validchar(character)) return;
     w = getdsc(character);

     gettime(now);
     total = db[character].data->player.totaltime;
     if(Connected(character)) total += (now - db[character].data->player.lasttime);

     idle = db[character].data->player.idletime;
     if(Connected(character) && w) idle += (now - w->last_time);

     if(idle > 0) sprintf(querybuf,"%d",(int) idle);
        else strcpy(querybuf,"0");
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  {J.P.Boggis 02/07/2000}  Return character's total number of logins  <---- */
void query_totallogins(CONTEXT)
{
     dbref character = query_find_character(player,params,0);
     if(!Validchar(character)) return;

     sprintf(querybuf,"%ld",db[character].data->player.logins);
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Return character's total time connected  <---- */
void query_totaltime(CONTEXT)
{
     time_t total,now;

     dbref character = query_find_character(player,params,0);
     if(!Validchar(character)) return;

     gettime(now);
     total = db[character].data->player.totaltime;
     if(Connected(character)) total += (now - db[character].data->player.lasttime);
     if(total > 0) sprintf(querybuf,"%d",(int) total);
        else strcpy(querybuf,"0");
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Did last command executed succeed?  <---- */
/*        (val1:  0 = True, 1 = False.)             */
void query_true_or_false(CONTEXT)
{
     if(val1) setreturn((command_boolean == COMMAND_FAIL) ? OK:ERROR,COMMAND_SUCC);
        else setreturn((command_boolean == COMMAND_SUCC) ? OK:ERROR,COMMAND_SUCC);
}

/* ---->  Return type of specified object  <---- */
void query_typeof(CONTEXT)
{
     dbref object = query_find_object(player,arg1,SEARCH_PREFERRED,0,0);

     if(!Valid(object)) return;
     if(!Blank(arg2)) {
        switch(Typeof(object)) {
               case TYPE_PROPERTY:
                    if(string_prefix("property",arg2) || string_prefix("properties",arg2)) setreturn(OK,COMMAND_SUCC);
                    break;
               case TYPE_VARIABLE:
                    if(string_prefix("variable",arg2)) setreturn(OK,COMMAND_SUCC);
                    break;
               case TYPE_COMMAND:
                    if(string_prefix("command",arg2) || string_prefix("compound command",arg2) || string_prefix("compoundcommand",arg2)) setreturn(OK,COMMAND_SUCC);
                    break;
               case TYPE_CHARACTER:
                    if(string_prefix("player",arg2) || string_prefix("character",arg2)) setreturn(OK,COMMAND_SUCC);
                    break;
               case TYPE_ALARM:
                    if(string_prefix("alarm",arg2)) setreturn(OK,COMMAND_SUCC);
                    break;
               case TYPE_ARRAY:
		    if(string_prefix("array",arg2) || string_prefix("dynamic array",arg2) || string_prefix("dynamicarray",arg2)) setreturn(OK,COMMAND_SUCC);
                    break;
               case TYPE_THING:
                    if(string_prefix("thing",arg2) || string_prefix("object",arg2)) setreturn(OK,COMMAND_SUCC);
                    break;
               case TYPE_EXIT:
                    if(string_prefix("exit",arg2)) setreturn(OK,COMMAND_SUCC);
                    break;
	       case TYPE_FREE:
                    if(string_prefix("free",arg2)) setreturn(OK,COMMAND_SUCC);
                    break;
               case TYPE_FUSE:
                    if(string_prefix("fuse",arg2)) setreturn(OK,COMMAND_SUCC);
                    break;
               case TYPE_ROOM:
                    if(string_prefix("room",arg2)) setreturn(OK,COMMAND_SUCC);
                    break;
        }
     } else setreturn(ObjectType(object),COMMAND_SUCC);
}

/* ---->  Return OK if specified command is global  <---- */
void query_global(CONTEXT)
{
     dbref globalcmd = NOTHING;
     dbref object = query_find_object(player,arg1,SEARCH_PREFERRED,0,0);

     if(Valid(object) && (Typeof(object) == TYPE_COMMAND)) {
        globalcmd = global_lookup(arg1,1);

        /* searched command might have multiple names */
        if(globalcmd == NOTHING) {
            gettextfield(1,';',db[object].name,0,scratch_return_string);
            globalcmd = global_lookup(scratch_return_string,1);
        }
        if((globalcmd != NOTHING) && (globalcmd == object))
            setreturn(OK,COMMAND_SUCC);
        else 
            setreturn(ERROR,COMMAND_FAIL);
     } else {
        setreturn(ERROR,COMMAND_FAIL);
     }    
}
/* ---->  Return uptime or start time of server  <---- */
void query_uptime(CONTEXT)
{
     time_t now;

     gettime(now);
     if(Blank(params) || string_prefix("uptime",params)) {

        /* ---->  Current uptime  <---- */
        sprintf(querybuf,"%d",(int) (now - uptime));
     } else if(string_prefix("starttime",params) || string_prefix("startdate",params)) {

        /* ---->  Restart time/date  <---- */
        sprintf(querybuf,"%d",(int) uptime);
     } else if(string_prefix("dbcreated",params) || string_prefix("dbcreationtime",params) || string_prefix("dbcreationdate",params)) {

        /* ---->  Database creation time/date  <---- */
        sprintf(querybuf,"%d",(int) db_creation_date + (Validchar(player) ? (db[player].data->player.timediff * HOUR):0));
     } else if(string_prefix("dbuptime",params)) {

        /* ---->  Accumulated uptime  <---- */
        sprintf(querybuf,"%d",(int) (db_accumulated_uptime + (now - uptime)));
     } else if(string_prefix("dbaveragetime",params) || string_prefix("dbaveragedate",params) || string_prefix("dbavgtime",params) || string_prefix("dbavgdate",params)) {

        /* ---->  Average uptime  <---- */
        sprintf(querybuf,"%d",(int) (db_accumulated_uptime + (now - uptime)) / ((db_accumulated_restarts > 1) ? db_accumulated_restarts:1));
     } else if(string_prefix("longesttime",params) || string_prefix("longestuptime",params)) {

        /* ---->  Longest uptime  <---- */
        if((now - uptime) > db_longest_uptime) {
           db_longest_uptime = (now - uptime);
           db_longest_date   = now;
	}

        sprintf(querybuf,"%d",(int) db_longest_uptime);
     } else if(string_prefix("longestdate",params)) {

        /* ---->  Time/date of longest uptime  <---- */
        if((now - uptime) > db_longest_uptime) {
           db_longest_uptime = (now - uptime);
           db_longest_date   = now;
	}

        sprintf(querybuf,"%d",(int) db_longest_date + (Validchar(player) ? (db[player].data->player.timediff * HOUR):0));
     } else {
        output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LYELLOW"dbaverage"ANSI_LGREEN"', '"ANSI_LYELLOW"dbcreated"ANSI_LGREEN"', '"ANSI_LYELLOW"dbuptime"ANSI_LGREEN"', '"ANSI_LYELLOW"longestdate"ANSI_LGREEN"', '"ANSI_LYELLOW"longestuptime"ANSI_LGREEN"', '"ANSI_LYELLOW"startdate"ANSI_LGREEN"' or '"ANSI_LYELLOW"uptime"ANSI_LGREEN"'.");
        setreturn(ERROR,COMMAND_FAIL);
        return;
     }
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  {J.P.Boggis 17/06/2000}  Return TCZ version  <---- */
void query_version(CONTEXT)
{
     setreturn(ERROR,COMMAND_FAIL);
     if(Blank(params)) {
        char datebuf[128];
	     
        sprintf(querybuf,"%s (%s) (TCZ v"TCZ_VERSION".%d) "CODEBASE" (%s) %s",tcz_full_name,tcz_short_name,TCZ_REVISION,operatingsystem,filter_spaces(datebuf,__DATE__" "__TIME__,0));
        setreturn(querybuf,COMMAND_SUCC);
     } else if(string_prefix("codebase",params) || string_prefix("code base",params)) {
        setreturn(CODEBASE,COMMAND_SUCC);
     } else if(string_prefix("codedescription",params) || string_prefix("code description",params)) {
        setreturn(CODEDESC,COMMAND_SUCC);
     } else if(string_prefix("compiledate",params) || string_prefix("compiletime",params) || string_prefix("compileddate",params) || string_prefix("compiledtime",params) || string_prefix("compilationdate",params) || string_prefix("compilationtime",params) || string_prefix("compile date",params) || string_prefix("compile time",params) || string_prefix("compilation date",params) || string_prefix("compilation time",params)) {
        char datebuf[128];
	
        setreturn(filter_spaces(datebuf,__DATE__" "__TIME__,0),COMMAND_SUCC);
     } else if(string_prefix("version",params)) {
        sprintf(querybuf,TCZ_VERSION".%d",TCZ_REVISION);
        setreturn(querybuf,COMMAND_SUCC);
     } else if(string_prefix("os",params) || string_prefix("o/s",params) || string_prefix("system",params) || string_prefix("operating",params) || string_prefix("operating system",params) || string_prefix("operatingsystem",params)) {
        setreturn(operatingsystem,COMMAND_SUCC);
     }
}

/* ---->  Return the active time (In seconds) until next bank payment  <---- */
void query_wagetime(CONTEXT)
{
        dbref object;

        object = query_find_character(player,params,0);
        if(!Validchar(object)) return;

        sprintf(querybuf,"%d",(HOUR - db[object].data->player.payment));
        setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Return weight (In Kilograms) of object  <---- */
void query_weight(CONTEXT)
{
     dbref thing = query_find_object(player,params,SEARCH_PREFERRED,0,0);
     if(!Valid(thing)) return;
     sprintf(querybuf,"%d",getweight(thing));
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  {@?wildcard "<WILDCARD>" "<STRING>"}  <---- */
void query_wildcard(CONTEXT)
{
     struct arg_data arg;

     unparse_parameters(params,2,&arg,0);
     if(!((arg.count < 2) || Blank(arg.text[0]))) {
        if(match_wildcard(arg.text[0],arg.text[1])) {
           setreturn(OK,COMMAND_SUCC);
	} else setreturn(ERROR,COMMAND_FAIL);
     } else setreturn(ERROR,COMMAND_FAIL);
}

/* ---->  Return character's title (WHO message)  <---- */
void query_who(CONTEXT)
{
     dbref         character = query_find_character(player,params,0);
     unsigned long longdate;
     time_t        now;

     gettime(now);
     longdate = epoch_to_longdate(now);
     if(!Validchar(character) || !HasField(character,TITLE)) return;
     if(hasprofile(db[character].data->player.profile) && ((db[character].data->player.profile->dob & 0xFFFF) == (longdate & 0xFFFF))) {
        sprintf(querybuf,"is %ld today  -  HAPPY BIRTHDAY!",longdate_difference(db[character].data->player.profile->dob,longdate) / 12);
        setreturn(querybuf,COMMAND_SUCC);
     } else setreturn(String(getfield(character,TITLE)),COMMAND_SUCC);
}

/* ---->         {@?word "<WORD>" "<SENTENCE>"}         <---- */
/*        (val1:  0 = Partial match, 1 = Exact match.)        */
void query_word(CONTEXT)
{
     struct arg_data arg;
     char   result[16];
     short  loop = 0;
     char   *ptr;

     unparse_parameters(params,2,&arg,0);
     if(!((arg.count < 2) || Blank(arg.text[0]) || Blank(arg.text[1]))) {
        ptr = arg.text[1];
        while(*ptr) { 
	      for(; *ptr && (*ptr == ' '); ptr++);
              if(*ptr) loop++;
              if(val1) {
	         if(!strncasecmp(ptr,arg.text[0],arg.len[0]) && (!ptr[arg.len[0]] || (ptr[arg.len[0]] == ' '))) {
                    sprintf(result,"%d",loop);
                    setreturn(result,COMMAND_SUCC);
	            return;
		 }
                 for(ptr++; *ptr && (*ptr != ' '); ptr++);
	      } else for(; *ptr && (*ptr != ' '); ptr++)
	         if(!strncasecmp(ptr,arg.text[0],arg.len[0])) {
                    sprintf(result,"%d",loop);
                    setreturn(result,COMMAND_SUCC);
	            return;
		 }
	      for(; *ptr && (*ptr != ' '); ptr++);
	}
        setreturn("0",COMMAND_SUCC);
     } else setreturn("0",COMMAND_SUCC);
}

/* ---->  {@?wordno <NUMBER> "<SENTENCE>"}  <---- */
void query_wordno(CONTEXT)
{
     char              temp[TEXT_SIZE];
     char              *ptr,*tmp;
     unsigned char     last = 0;
     short             loop = 0;
     struct   arg_data arg;

     unparse_parameters(params,2,&arg,1);
     if(!((arg.count < 2) || ((arg.numb[0] <= 0) && (arg.numb[0] != LAST)) || Blank(arg.text[1]))) {
        if(arg.numb[0] == LAST) last = 1;
        ptr = arg.text[1], *temp = '\0';
        while(*ptr) {

	      /* ---->  Skip blank spaces  <---- */
              for(; *ptr && (*ptr == ' '); ptr++);
              if(*ptr) loop++;

              /* ---->  Store word in TEMP  <---- */
              for(tmp = temp; *ptr && (*ptr != ' '); *tmp++ = *ptr++);
	      *tmp = '\0';

              /* ---->  Word found?  <---- */
	      if(!last && (loop == arg.numb[0])) {
                 setreturn(temp,COMMAND_SUCC);
		 return;
	      }
	}
        setreturn((last) ? temp:"",COMMAND_SUCC);
     } else setreturn("",COMMAND_SUCC);
}

/* ---->  Return character's WWW home page address  <---- */
void query_www(CONTEXT)
{
     dbref character = query_find_character(player,params,0);
     if(!Validchar(character) || !HasField(character,WWW)) return;
     setreturn(String(getfield(character,WWW)),COMMAND_SUCC);
}
