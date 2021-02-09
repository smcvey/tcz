/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| UNPARSE.C  -  Implements unparsing of object names, flags and boolean       |
|               expressions for display or return (I.e:  '$0'.)               |
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


#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "object_types.h"
#include "flagset.h"
#include "fields.h"


char   boolexp_buf[BUFFER_LEN];
struct str_ops exp_data;


/* ---->  Return list of flag names set on an object  <---- */
const char *unparse_flaglist(dbref object,unsigned char permission,char *buffer)
{
      int i;

      if(!Valid(object)) return(NULL);

      /* ---->  Normal (User) flags list  <---- */
      *buffer = '\0';
      for(i = 0; flag_list[i].string != NULL; i++)
          if(((db[object].flags & flag_list[i].mask) == flag_list[i].flag) && !(flag_list[i].flags & FLAG_NOT_DISPLAY) && (permission || !(flag_list[i].flags & FLAG_PERMISSION)) && !((flag_list[i].mask == GENDER_MASK) && (Typeof(object) != TYPE_CHARACTER)) && !(((flag_list[i].flags & FLAG_NOT_CHARACTER) && (Typeof(object) == TYPE_CHARACTER)) || ((flag_list[i].flags & FLAG_NOT_OBJECT) && (Typeof(object) != TYPE_CHARACTER))))
             sprintf(buffer + strlen(buffer),"%s%s",(*buffer) ? ", ":"",flag_list[i].string);

      /* ---->  Secondary (Internal) flags list  <---- */      
      for(i = 0; flag_list2[i].string != NULL; i++)
          if(((db[object].flags2 & flag_list2[i].mask) == flag_list2[i].flag) && !(flag_list2[i].flags & FLAG_NOT_DISPLAY) && (permission || !(flag_list2[i].flags & FLAG_PERMISSION)) && !(((flag_list2[i].flags & FLAG_NOT_CHARACTER) && (Typeof(object) == TYPE_CHARACTER)) || ((flag_list2[i].flags & FLAG_NOT_OBJECT) && (Typeof(object) != TYPE_CHARACTER))))
             sprintf(buffer + strlen(buffer),"%s%s",(*buffer) ? ", ":"",flag_list2[i].string);

      /* ---->  Pseudo-flags (Boot, Shout, Help)  <---- */
#ifdef MORTAL_SHOUT
      if(Shout(object)) sprintf(buffer + strlen(buffer),"%sShout",(*buffer) ? ", ":"");
#else
      if(Level4(object) && Shout(object)) sprintf(buffer + strlen(buffer),"%sShout",(*buffer) ? ", ":"");
#endif
      if(Level4(object)      && Boot(object)) sprintf(buffer + strlen(buffer),"%sBoot",(*buffer) ? ", ":"");
      if(Experienced(object) && Help(object)) sprintf(buffer + strlen(buffer),"%sHelp",(*buffer) ? ", ":"");

      if(!Blank(buffer)) strcat(buffer,".");
         else strcpy(buffer,"None.");
      return(buffer);
}

/* ---->  Unparse object's flags  <---- */
char *unparse_flags(dbref object)
{
     int    mask  = ~(MORON|ASSISTANT|EXPERIENCED|APPRENTICE|WIZARD|ELDER|DEITY);
     int    inc   =  (ASSISTANT|EXPERIENCED);
     int    mask2 = ~(RETIRED);
     int    inc2  =  (RETIRED);

     static char type_codes[] = "?TEPRCFAVDpWaSIU";
     static char buffer[KB];
     char   *ptr;
     int    flag;

     if(Valid(object)) {
        ptr    = buffer;
        *ptr++ = ' ';
        *ptr++ = ((Typeof(object) == TYPE_CHARACTER) && Puppet(object)) ? 'p':type_codes[Typeof(object)];

	/* ---->  Unparse flags of object  <---- */
	if(Moron(object)) {
           *ptr++ = 'M';
	} else if((Typeof(object) == TYPE_CHARACTER) && Retired(object)) {
           *ptr++ = (RetiredDruid(object)) ? 'r':'R';
	} else if((Typeof(object) == TYPE_CHARACTER) && Assistant(object)) {
           *ptr++ = 'x';
	} else if((Typeof(object) == TYPE_CHARACTER) && Experienced(object)) {
           *ptr++ = 'X';
	} else if(Level1(object)) {
           *ptr++ = (Druid(object)) ? 'd':'D';
	} else if(Level2(object)) {
           *ptr++ = (Druid(object)) ? 'e':'E';
	} else if(Level3(object)) {
           *ptr++ = (Druid(object)) ? 'w':'W';
	} else if(Level4(object)) {
           *ptr++ = (Druid(object)) ? 'a':'A';
	}

	if(Typeof(object) == TYPE_CHARACTER) inc = inc2 = 0;

	/* ---->  Primary flags  <---- */
	for(flag = 0; !Blank(flag_list[flag].string); flag++)
	    if(((db[object].flags & (mask|inc) & flag_list[flag].mask) == flag_list[flag].flag) && (flag_list[flag].quick_flag != '\0') && !(((flag_list[flag].flags & FLAG_NOT_CHARACTER) && (Typeof(object) == TYPE_CHARACTER)) || ((flag_list[flag].flags & FLAG_NOT_OBJECT) && (Typeof(object) != TYPE_CHARACTER))))
	       *ptr++ = flag_list[flag].quick_flag;

	/* ---->  Secondary flags  <---- */
	for(flag = 0; !Blank(flag_list2[flag].string); flag++)
	    if(((db[object].flags2 & (mask2|inc2) & flag_list2[flag].mask) == flag_list2[flag].flag) && (flag_list2[flag].quick_flag != '\0') && !(((flag_list2[flag].flags & FLAG_NOT_CHARACTER) && (Typeof(object) == TYPE_CHARACTER)) || ((flag_list2[flag].flags & FLAG_NOT_OBJECT) && (Typeof(object) != TYPE_CHARACTER))))
	       *ptr++ = flag_list2[flag].quick_flag;

	*ptr = '\0';
	return(buffer);
     }
     return("???");
}

/* ---->  Unparse object, showing #ID and flags if appropriate  <---- */
const char *unparse_object(dbref player,dbref object,unsigned char article_setting)
{
      static char buf[BUFFER_LEN];
      static int  elements;

      if((object < -3) || (object >= db_top)) return(INVALID_STRING);
      switch(object) {
             case NOTHING:
                  return(NOTHING_STRING);
             case NOBODY:
                  return(NOBODY_STRING);
             case HOME:
                  return(HOME_STRING);
             default:
                  if(!Validchar(player)) {
                     sprintf(buf,"%s%s%s",(article_setting) ? Article(object,article_setting & 0x01,article_setting & 0x6):"",getname(object),Moron(object) ? " the Moron":((Typeof(object) == TYPE_COMMAND) && (db[object].commands != NOTHING)) ? " (Module)":"");
                     return(buf);
		  } else if(db[player].flags & NUMBER) {
		     sprintf(buf,"%s%s%s",(article_setting) ? Article(object,article_setting & 0x01,article_setting & 0x6):"",getname(object),(Moron(object) ? " the Moron":""));
                     return(buf);
		  } else if(Level4(db[player].owner) || Experienced(db[player].owner) || can_read_from(player,object) || can_link_or_home_to(player,object)) {

                     /* ---->  Object's name, DBref and flags  <---- */
                     if(Typeof(object) == TYPE_ARRAY) {
                        elements = array_element_count(db[object].data->array.start);
                        strcpy(buf,getname(object));
                        if(elements > 0) sprintf(buf + strlen(buf),"[%d]",elements);
                        sprintf(buf + strlen(buf),"(#%d%s)%s%s",object,unparse_flags(object),(Moron(object) ? " the Moron":""),((Typeof(object) == TYPE_COMMAND) && (db[object].commands != NOTHING)) ? " (Module)":"");
		     } else sprintf(buf,"%s%s(#%d%s)%s",(article_setting) ? Article(object,article_setting & 0x01,article_setting & 0x6):"",getname(object),object,unparse_flags(object),Moron(object) ? " the Moron":((Typeof(object) == TYPE_COMMAND) && (db[object].commands != NOTHING)) ? " (Module)":"");
                     return(buf);
                  } else {
                     sprintf(buf,"%s%s%s",(article_setting) ? Article(object,article_setting & 0x01,article_setting & 0x6):"",getname(object),Moron(object) ? " the Moron":((Typeof(object) == TYPE_COMMAND) && (db[object].commands != NOTHING)) ? " (Module)":"");
                     return(buf);
                  }
      }
}

/* ---->  Returns name or #ID of object (#<ID> if you control object)  <---- */
const char *getnameid(dbref player,dbref object,char *buffer)
{
      static char buf[32];

      if((object < -3) || (object >= db_top)) return(INVALID_STRING);
      switch(object) {
             case NOTHING:
                  return(NOTHING_STRING);
             case NOBODY:
                  return(NOBODY_STRING);
             case HOME:
                  return(HOME_STRING);
             default:
		 /* if(Validchar(player) && (Level4(Owner(player)) || Experienced(Owner(player)) || can_read_from(player,object) || can_link_or_home_to(player,object))) { */
		 if(Validchar(player) && (Level4(Owner(player)) || Experienced(Owner(player)) || can_read_from(player,object) || can_link_or_home_to(player, object) || Validchar(object))) {
                     sprintf((buffer) ? buffer:buf,"#%d",object);
                     return((buffer) ? buffer:buf);
		 } else return(getname(object));
      }
}

/* ---->  Unparse sub parts of boolean expression  <---- */
static void unparse_subboolexp(dbref player,struct boolexp *b,boolexp_type outer_type,int for_return)
{
       int pos;

       if(!b || (b == TRUE_BOOLEXP)) strcat_limits(&exp_data,"*UNLOCKED*");
          else switch(b->type) {
             case BOOLEXP_AND:
                  if(outer_type == BOOLEXP_NOT) strcat_limits(&exp_data,"(");
                  unparse_subboolexp(player,b->sub1,b->type,for_return);
                  strcat_limits_char(&exp_data,AND_TOKEN);
                  unparse_subboolexp(player,b->sub2,b->type,for_return);
                  if(outer_type == BOOLEXP_NOT) strcat_limits(&exp_data,")");
                  break;
             case BOOLEXP_OR:
                  if((outer_type == BOOLEXP_NOT) || (outer_type == BOOLEXP_AND)) strcat_limits(&exp_data,"(");
                  unparse_subboolexp(player,b->sub1,b->type,for_return);
                  strcat_limits_char(&exp_data,OR_TOKEN);
                  unparse_subboolexp(player,b->sub2,b->type,for_return);
                  if((outer_type == BOOLEXP_NOT) || (outer_type == BOOLEXP_AND)) strcat_limits(&exp_data,")");
                  break;
             case BOOLEXP_NOT:
                  strcat_limits_char(&exp_data,NOT_TOKEN);
                  unparse_subboolexp(player,b->sub1,b->type,for_return);
                  break;
             case BOOLEXP_CONST:
                  if(for_return) strcat_limits(&exp_data,getnameid(player,b->object,NULL));
                     else strcat_limits(&exp_data,unparse_object(player,b->object,0));
                  break;
             case BOOLEXP_FLAG:
                  strcat_limits_char(&exp_data,FLAG_TOKEN);
   
                  /* ---->  Match boolean flag in flag_list[]   <---- */
                  for(pos = 0; flag_list[pos].string; pos++)
                      if(flag_list[pos].boolflag && (b->object == flag_list[pos].boolflag)) {
                         strcat_limits(&exp_data,flag_list[pos].string);
                         return;
		      }

                  /* ---->  Match boolean flag in flag_list2[]  <---- */
                  for(pos = 0; flag_list2[pos].string; pos++)
                      if(flag_list2[pos].boolflag && (b->object == flag_list2[pos].boolflag)) {
                         strcat_limits(&exp_data,flag_list2[pos].string);
                         return;
		      }

                  /* ---->  Unknown boolean flag  <---- */
                  strcat_limits(&exp_data,"<UNKNOWN FLAG>");
                  break;
             default:
  
                  /* ---->  Bad type  <---- */
                  strcat_limits(&exp_data,"<UNKNOWN BOOLEAN OPERATOR>");
	  }
}

/* ---->  Unparse boolean expression for display(0) or return(1)  <---- */
char *unparse_boolexp(dbref player,struct boolexp *b,int for_return)
{
     exp_data.dest   = boolexp_buf;
     exp_data.length = 0;     

     unparse_subboolexp(player,b,BOOLEXP_CONST,for_return);
     *exp_data.dest = '\0';

     return(boolexp_buf);
}

/* ---->  Unparse given parameters (String parameters in ""'s, numeric otherwise.)  <---- */
void unparse_parameters(char *str,unsigned char pcount,struct arg_data *arg,unsigned char keywords)
{
     unsigned char quotes,closing;
     char          *p1,*p2,*p3;

     p1 = p2 = str;
     arg->count = 0;
     setreturn(ERROR,COMMAND_FAIL);
     if(p1) while(*p1 && (arg->count < PARAMETERS)) {

        /* ---->  Skip over leading spaces  <---- */
        for(; *p1 && (*p1 == ' '); p1++,p2++);

        /* ---->  String enclosed in double quotes?  <---- */
        if(*p1 && (*p1 == '\"')) quotes = 1, p1++, p2++;
           else if(*p1 && (*p1 == '\x02')) quotes = 2, p1++, p2++;
	      else quotes = 0;

        /* ---->  Get parameter  <---- */
        if(quotes == 1) {
           closing = 0;
           while(*p2 && !closing)
                 if(*p2 && (*p2 == '\"') && (!*(p2 + 1) || (*(p2 + 1) == ' '))) {

                    /* ---->  Scan ahead and find next char  <---- */
                    for(p3 = (p2 + 1); *p3 && (*p3 == ' '); p3++);
        
                    /* ---->  Found closing quote?  <---- */
                    if(!*p3 || (*p3 && (arg->count < (pcount - 1)))) closing = 1;
                       else p2++;
		 } else p2++;
	} else if(quotes == 2) while(*p2 && (*p2 != '\x02')) p2++;
           else while(*p2 && ((arg->count >= (pcount - 1)) || (*p2 != ' '))) p2++;
        if(*p2) *p2++ = '\0';

        /* ---->  Set pointer in array to text, and numeric value  <---- */
        arg->text[arg->count] = p1;
        arg->len[arg->count]  = strlen(p1);
        if(keywords) {
           switch(*p1) {
                  case 'a':
                  case 'A':
                       if(!strcasecmp("ALL",p1)) arg->numb[arg->count] = ALL;
                       break;
                  case 'f':
                  case 'F':
                       if(!strcasecmp("FIRST",p1)) arg->numb[arg->count] = 1;
                       break;
                  case 'l':
                  case 'L':
                       if(!strcasecmp("LAST",p1)) arg->numb[arg->count] = LAST;
                       break;
                  case 'e':
                  case 'E':
                       if(!strcasecmp("END",p1)) arg->numb[arg->count] = END;
                       break;
                  default:
                       if(*p1 && !isdigit(*p1)) arg->numb[arg->count] = NOTINT;
                          else arg->numb[arg->count] = Blank(p1) ? 0:atol(p1);
	   }
	
	} else if(*p1 && !isdigit(*p1)) arg->numb[arg->count] = NOTINT;
           else arg->numb[arg->count] = Blank(p1) ? 0:atol(p1);
        arg->count++;
        p1 = p2;
     }

     /* ---->  Fill rest of array with blank data  <---- */
     for(quotes = arg->count; quotes < PARAMETERS; quotes++)
         arg->text[quotes] = "", arg->numb[quotes] = 0;
}

/* ---->  Get the name of an object  <---- */
const char *getname(dbref object)
{
      if((object < -2) || (object >= db_top)) return(INVALID_STRING);
      switch(object) {
             case NOTHING:
                  return(NOTHING_STRING);
             case NOBODY:
                  return(NOBODY_STRING);
             case HOME:
                  return(HOME_STRING);
             default:
                  return(((Typeof(object) != TYPE_FREE) && !Blank(db[object].name)) ? db[object].name:"<UNUSED OBJECT>");
      }
}

/* ---->  Get name of character, with name prefix if total length is less than that specified  <---- */
const char *getname_prefix(dbref player,int limit,char *buffer)
{
      char *ptr;

      if(Validchar(player)) {
         ptr = (char *) getfield(player,PREFIX);
         if(!Blank(ptr) && ((strlen(ptr) + 1 + strlen(getname(player))) <= limit))
            sprintf(buffer,"%s %s",ptr,getname(player));
               else strcpy(buffer,getname(player));
      } else strcpy(buffer,getname(player));
      return(buffer);
}

/* ---->  Get name of character, with name prefix and suffix  <---- */
const char *getcname(dbref player,dbref object,unsigned char unparse,unsigned char article_setting)
{
      static int cached_commandtype;
      static char *p1,*cache;

      if((object < -1) || (object >= db_top)) return(INVALID_STRING);
      if(object == NOTHING) return(NOTHING_STRING);
      cached_commandtype = command_type;
      command_type |= NO_USAGE_UPDATE;

      /* ---->  Name prefix  <---- */
      cache  = cmpptr, cmpptr = objname + KB;
      if((Typeof(object) == TYPE_CHARACTER) && !Blank(getfield(object,PREFIX)))
         sprintf(objname,"%s%s ",(article_setting) ? Article(object,article_setting & 0x01,article_setting & 0x6):"",getfield(object,PREFIX));
            else strcpy(objname,(article_setting) ? Article(object,article_setting & 0x01,article_setting & 0x6):"");

      /* ---->  Name  <---- */
      if(unparse) strcat(objname,unparse_object(player,object,0));
         else if(db[object].name) strcat(objname,db[object].name);
            else strcat(objname,"<UNNAMED>");

      /* ---->  Name suffix  <---- */
      if(Typeof(object) == TYPE_CHARACTER) {
         if(Moron(object) && !unparse) {
            strcat(objname," the Moron");
	 } else if(!Blank(getfield(object,SUFFIX)) && (!unparse || !Moron(object))) {
            p1 = (char *) getfield(object,SUFFIX);
            strcat(objname,pose_string(&p1,""));
            if(*p1) strcat(objname,p1);
	 }
      }
      command_type = cached_commandtype, cmpptr = cache;
      return(objname);
}

/* ---->  Return forwarding E-mail address of character ('tcz.user.name@tczserver.domain')  <---- */
const char *forwarding_address(dbref player,unsigned char tail,char *buffer)
{
#ifdef EMAIL_FORWARDING
      char  *tmp = buffer;
      const char *ptr;

      if(!Validchar(player) || !ForwardEmail(player) || !option_emailforward(OPTSTATUS)) return(NULL);
      for(ptr = db[player].name; *ptr; ptr++)
          if(*ptr != ' ') *tmp++ = tolower(*ptr);
             else *tmp++ = '.';

      if(tail) for(*tmp++ = '@', ptr = email_forward_name; *ptr; *tmp++ = *ptr++);
      *tmp = '\0';
      return(buffer);
#else
      return(NULL);
#endif
}

/* ---->  Get first name of exit  <---- */
const char *getexit_firstname(dbref player,dbref exit,unsigned char unparse)
{
      const char *rtn;
      char  *ptr,*tmp;

      if(!unparse) {
         strcpy(objname,getname(exit));
         if((Typeof(exit) == TYPE_EXIT) && (ptr = (char *) strchr(objname,LIST_SEPARATOR)) && *ptr) *ptr = '\0';
         return(objname);
     } else {
         if(Typeof(exit) == TYPE_EXIT) {
            strcpy(objname,tmp = (char *) getname(exit));
            if((Typeof(exit) == TYPE_EXIT) && (ptr = (char *) strchr(objname,LIST_SEPARATOR)) && *ptr) *ptr = '\0';
            db[exit].name = objname;
            rtn = unparse_object(player,exit,0);
            db[exit].name = tmp;
            return(rtn);
	 } else return(unparse_object(player,exit,0));
      }
 }

/* ---->  Get name of exit (As seen in 'Contents:' list of room)  <---- */
const char *getexitname(dbref player,dbref exit)
{
      int    counter = 0,standard = 0;
      char   *ptr,*str,*temp,*p1,*p2;
      struct descriptor_data *d;
      char   c,u;

      str = (char *) getname(exit);
      while(*str && ((*str == LIST_SEPARATOR) || (*str == ' '))) str++;
      p1 = ptr = str;

      /* ---->  Find short name of exit (Standard names have priority)  <---- */
      while(*p1) {
            counter++;
            while(*p1 && ((*p1 == LIST_SEPARATOR) || (*p1 == ' '))) p1++;
            p2 = p1;

            while(*p2 && (*p2 != LIST_SEPARATOR)) p2++;
            c   = *p2;
            *p2 = '\0';            

            if(*p1) {
               if(!strcasecmp(p1,"out")) {
                  standard = 1;
                  ptr = p1;
	       } else if(!strcasecmp(p1,"exit") && (!standard || (standard > 2))) {
                  standard = 2;
                  ptr = p1;
	       } else if(!strcasecmp(p1,"leave") && (!standard || (standard > 3))) {
                  standard = 3;
                  ptr = p1;
	       } else if(!strcasecmp(p1,"back") && !standard) {
                  standard = 4;
                  ptr = p1;
	       } else if(!standard && (counter == 2)) ptr = p1;
	    }

            *p2 = c;
            p1  = p2;
      }

      /* ---->  Construct exit name  <---- */
      temp = (char *) strchr(str,LIST_SEPARATOR);
      for(; (temp > str) && (*(temp - 1) == ' '); temp--);
      if(temp != NULL) c = *temp, *temp = '\0';

      for(d = descriptor_list; d && !((d->player == player) && (d->flags & CONNECTED)); d = d->next);
      if(!Number(player) && (Level4(db[player].owner) || Experienced(db[player].owner) || can_read_from(player,exit)))
         sprintf(objname,ANSI_LWHITE"%s%s(#%d %s)%s",Article(exit,UPPER,INDEFINITE),str,exit,unparse_flags(exit),(ptr == str) ? "":" (");
            else sprintf(objname,ANSI_LWHITE"%s%s%s",Article(exit,UPPER,INDEFINITE),str,(ptr == str) ? "":" (");
      if(temp != NULL) *temp = c;
      
      if(ptr != str) {
         temp = (char *) strchr(ptr,LIST_SEPARATOR);
         if(temp != NULL) c = *temp, *temp = '\0';
         u = *ptr;
         if(islower(*ptr)) *ptr = toupper(*ptr);
	 sprintf(objname + strlen(objname),ANSI_LGREEN""ANSI_UNDERLINE"%s"ANSI_LWHITE") ",ptr);
	 *ptr = u;
         if(temp != NULL) *temp = c;
      }         
      return(objname);
}
