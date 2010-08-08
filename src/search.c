/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| SEARCH.C  -  Implements searches for objects matching a given               |
|              specification and object listing.                              |
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
| Module originally designed and written by:  J.P.Boggis 29/12/1994.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: search.c,v 1.1.1.1 2004/12/02 17:42:24 jpboggis Exp $

*/


#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "object_types.h"
#include "friend_flags.h"
#include "objectlists.h"
#include "flagset.h"
#include "search.h"

#include "fields.h"
#include "match.h"


/* ---->  Parse creation date/last usage date boundaries  <---- */
unsigned char parse_date_boundaries(const char *ptr,char *buffer)
{
	 unsigned long cupper = 0xFFFFFFFF,lupper = 0xFFFFFFFF;
	 unsigned long clower = 0,llower = 0;
	 unsigned char op,created,equal = 0;
	 int           value;
	 char          *tmp;

	 if(Blank(ptr)) return(1);
	 if(*ptr == '&') {
	    for(tmp = buffer, ptr++; *ptr && !isdigit(*ptr) && (*ptr != ' ') && (*ptr != '<') && (*ptr != '>') && (*ptr != '='); *tmp++ = *ptr++);
	    *tmp = '\0';
	    if(!Blank(buffer)) {
	       if(string_prefix("created",buffer) || string_prefix("creationdate",buffer) || string_prefix("creationtime",buffer)) created = 1;
		 else if(string_prefix("lastused",buffer) || string_prefix("lastusagedate",buffer) || string_prefix("lastusagetime",buffer)) created = 0;
		     else return(0);

	       while(*ptr) {

		     /* ---->  Operator (<, >, <=, >=)  <---- */
		     if(*ptr == '=') equal = 1, ptr++;
		     if(*ptr) switch(*ptr) {
			case '<':
			     if(*(++ptr)) {
				if(*ptr == '=') op = OP_LE, ptr++;
				   else if(isdigit(*ptr)) op = (equal) ? OP_LE:OP_LT;
				      else return(0);
			     } else return(0);
			     break;
			case '>':
			     if(*(++ptr)) {
				if(*ptr == '=') op = OP_GE, ptr++;
				   else if(isdigit(*ptr)) op = (equal) ? OP_GE:OP_GT;
				      else return(0);
			     } else return(0);
			     break;
			default:
			     if(isdigit(*ptr)) op = OP_GE;
				return(0);
		     }

		     /* ---->  Value  <---- */
		     for(tmp = buffer; *ptr && isdigit(*ptr); *tmp++ = *ptr++);
		     *tmp = '\0';
		     if((value = ABS(atol(buffer))) > 0) {
			for(tmp = buffer; *ptr && !isdigit(*ptr) && (*ptr != ' ') && (*ptr != '<') && (*ptr != '>') && (*ptr != '='); *tmp++ = *ptr++);
			*tmp = '\0';

			/* ---->  Time interval (Seconds, minutes, hours, days, weeks, months, years)  */
			if(!Blank(buffer)) {
			   if(string_prefix("years",buffer)) value *= YEAR;
			      else if(string_prefix("months",buffer)) value *= MONTH;
				 else if(string_prefix("weeks",buffer)) value *= WEEK;
				    else if(string_prefix("days",buffer)) value *= DAY;
				       else if(string_prefix("hours",buffer)) value *= HOUR;
					  else if(string_prefix("minutes",buffer)) value *= MINUTE;
					     else if(!(string_prefix("seconds",buffer) || string_prefix("secs",buffer))) return(0);
			} else value *= MINUTE;

			/* ---->  Adjust date boundaries  <---- */
			switch(op) {
			       case OP_LT:
				    if(created) cupper = (grp->time - value - 1);
				       else lupper = (grp->time - value - 1);
				    break;
			       case OP_GT:
				    if(created) clower = (grp->time - value + 1);
				       else llower = (grp->time - value + 1);
				    break;
			       case OP_LE:
				    if(created) cupper = (grp->time - value);
				       else lupper = (grp->time - value);
				    break;
			       case OP_GE:
				    if(created) clower = (grp->time - value);
				       else llower = (grp->time - value);
				    break;
			}
		     }
		     if(*ptr && !isdigit(*ptr) && (*ptr != ' ') && (*ptr != '<') && (*ptr != '>') && (*ptr != '=')) return(0);
		     if(!*ptr) {
			grp->cupper = cupper;
			grp->clower = clower;
			grp->lupper = lupper;
			grp->llower = llower;
			return(1);
		     }
	       }
	       grp->cupper = cupper;
	       grp->clower = clower;
	       grp->lupper = lupper;
	       grp->llower = llower;
	       return(1);
	    }
	 }
	 return(0);
}

/* ---->  Returns the object type that STR matches  <---- */
int parse_objecttype(const char *str)
{
    int pos;

    if(!(str && *str)) return(0);
    for(pos = 0; search_objecttype[pos].name; pos++)
        if(string_prefix(search_objecttype[pos].name,str))
           return(search_objecttype[pos].value);

    return(0);
}

/* ---->  Returns the object field that STR matches  <---- */
int parse_fieldtype(const char *str,int *object_type)
{
    int pos;

    if(!(str && *str)) return(0);
    for(pos = 0; search_fieldtype[pos].name; pos++)
        if(string_prefix(search_fieldtype[pos].name,str)) {
           *object_type |= search_fieldtype[pos].alt;
           return(search_fieldtype[pos].value);
	}

    return(0);
}

/* ---->  Returns the primary TCZ flag that STR matches  <---- */
int parse_flagtype(const char *str,int *object_type,int *mask)
{
    int pos;

    if(!(str && *str)) return(0);
    for(pos = 0; search_flagtype[pos].name; pos++)
        if(string_prefix(search_flagtype[pos].name,str)) {
           *mask        |= search_flagtype[pos].mask;
           *object_type |= search_flagtype[pos].alt;
           return(search_flagtype[pos].value);
	}
    return(0);
}

/* ---->  Returns the secondary TCZ flag that STR matches  <---- */
int parse_flagtype2(const char *str,int *object_type,int *mask)
{
    int pos;

    if(!(str && *str)) return(0);
    for(pos = 0; search_flagtype2[pos].name; pos++)
        if(string_prefix(search_flagtype2[pos].name,str)) {
           *mask        |= search_flagtype2[pos].mask;
           *object_type |= search_flagtype2[pos].alt;
           return(search_flagtype2[pos].value);
	}
    return(0);
}

/* ---->  {J.P.Boggis 29/07/2000}  Find room within location by owner  <---- */
dbref search_room_by_owner(dbref owner,dbref location,int level)
{
      dbref current,object;

      if(!Validchar(owner) || !Valid(location) || !HasList(location,CONTENTS)) return(NOTHING);
      if(level > MATCH_RECURSION_LIMIT) return(NOTHING);

      object = getfirst(location,CONTENTS,&current);
      while(Valid(object) && !((Typeof(object) == TYPE_ROOM) && (Owner(object) == owner))) {
            if((Typeof(object) == TYPE_ROOM) && Valid(db[object].contents))
               search_room_by_owner(owner,object,level + 1);
            getnext(object,CONTENTS,current);
      }

      if(Valid(object) && (Typeof(object) == TYPE_ROOM) && (Owner(object) == owner)) return(object);
      return(NOTHING);
}

/* ---->  List exits that lead to the current/specified room (From other rooms)  <---- */
void search_entrances(CONTEXT)
{
     unsigned char            cached_scrheight,twidth = output_terminal_width(player),first = 1;
     struct   descriptor_data *p = getdsc(player);
     dbref                    thing;

     setreturn(ERROR,COMMAND_FAIL);
     if(in_command && !Experienced(db[player].owner) && !Level4(db[player].owner)) {
        output(p,player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above can use @from/@entrances from within a compound command.");
        return;
     }

     params = (char *) parse_grouprange(player,params,ALL,1);
     if(!Blank(params)) {
        thing = match_preferred(player,player,params,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
        if(!Valid(thing)) return;
     } else thing = Location(player);

     /* ---->  Security constraints  <---- */
     if(!Level4(db[player].owner) && !Experienced(db[player].owner) && !can_write_to(player,thing,0) && !(!in_command && friendflags_set(db[thing].owner,player,thing,FRIEND_READ))) {
        output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only list the entrances of property that belongs to you.");
        return;
     }

     if(IsHtml(p)) {
        html_anti_reverse(p,1);
        output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
     } else if(!in_command && p && !p->pager && !IsHtml(p) && More(player)) pager_init(p);

     if(!in_command) {
        output(p,player,2,1,1,"%sEntrances to %s"ANSI_LWHITE"%s"ANSI_LCYAN"...%s",IsHtml(p) ? "\016<TR><TH ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><FONT COLOR="HTML_LCYAN" SIZE=4><I>\016":"\n ",Article(thing,LOWER,DEFINITE),unparse_object(player,thing,0),IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
        if(!IsHtml(p)) output(p,player,0,1,0,(char *) separator(twidth,0,'-','='));
     }

     command_type                     |= NO_USAGE_UPDATE;
     cached_scrheight                  = db[player].data->player.scrheight;
     db[player].data->player.scrheight = ((db[player].data->player.scrheight - 7) / 3) * 2;
     set_conditions(player,0,0,NOTHING,thing,NULL,400);
     entiredb_initgrouprange();

     if(grp->distance > 0) {
        while(entiredb_grouprange()) {
              if(!first && !IsHtml(p)) output(p,player,0,1,0,"");
	      sprintf(scratch_buffer,"%sFrom %s"ANSI_LYELLOW"%s"ANSI_LGREEN": \016&nbsp;\016 ",IsHtml(p) ? "\016<TR><TD ALIGN=LEFT>\016"ANSI_LGREEN:ANSI_LGREEN" ",Article(db[grp->cobject].location,LOWER,INDEFINITE),unparse_object(player,db[grp->cobject].location,0));
              output(p,player,2,1,3,"%s%s%s",scratch_buffer,punctuate((char *) getexitname(db[player].owner,grp->cobject),0,'.'),IsHtml(p) ? "\016</TD></TR>\016":"\n");
              first = 0;
	}

        if(!in_command) {
           if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
           output(p,player,2,1,1,"%sTotal entrances: \016&nbsp;\016 "ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><TD>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",listed_items(scratch_return_string,1),IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
	}
     } else {
        output(p,player,2,1,0,"%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TD>"ANSI_LCYAN"<I>*** &nbsp; NO ENTRANCES FOUND &nbsp; ***</I></TD></TR>\016":" ***  NO ENTRANCES FOUND  ***\n");
        if(!in_command) {
           if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
           output(p,player,2,1,1,"%sTotal entrances: \016&nbsp;\016 "ANSI_DWHITE"None.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><TD>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
	}
     }

     if(IsHtml(p)) {
        output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
        html_anti_reverse(p,0);
     }
     db[player].data->player.scrheight = cached_scrheight;
     command_type                     &= ~NO_USAGE_UPDATE;
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Find object(s) of given type(s) that match the given wildcard spec. in the given fields  <---- */
void search_find(CONTEXT)
{
     int                      object_flags = 0,field_flags = 0,flags_inc = 0,flags_mask = 0,flags_inc2 = 0,flags_mask2 = 0,flags_exc = 0;
     unsigned char            cached_scrheight,twidth = output_terminal_width(player),cr = 1;
     struct   descriptor_data *p = getdsc(player);
     int                      result,valid = 0;
     const    char            *p1;
     char                     *p2;

     setreturn(ERROR,COMMAND_FAIL);
     if(!(in_command && !(Level4(db[player].owner) || Experienced(db[player].owner)))) {

        /* ---->  Start parsing object/field/flag types to search from first parameter  <---- */
        p1 = arg1 = (char *) parse_grouprange(player,arg1,ALL,1);
        if(*p1 && !Blank(arg2)) {
           while(*p1) {
                 while(*p1 && (*p1 == ' ')) p1++;
                 if(*p1) {

                    /* ---->  Determine what object type/field/flag word is  <---- */
                    for(p2 = scratch_buffer; *p1 && (*p1 != ' '); *p2++ = *p1++);
                    *p2 = '\0';
                    if((*scratch_buffer != '&') || !(result = parse_date_boundaries(scratch_buffer,scratch_return_string))) {
                       if((result = parse_fieldtype(scratch_buffer,&object_flags))) {
                          valid = field_flags |= result;
		       } else if((result = parse_objecttype(scratch_buffer))) {
                          valid = object_flags |= result;
		       } else if((result = parse_flagtype(scratch_buffer,&object_flags,&flags_mask))) {
                          valid = flags_inc |= result;
		       } else if((result = parse_flagtype2(scratch_buffer,&object_flags,&flags_mask2))) {
                          valid = flags_inc2 |= result;
		       } else if(string_prefix("mortals",scratch_buffer)) {
			  object_flags |= SEARCH_CHARACTER;
			  flags_exc    |= BUILDER|APPRENTICE|WIZARD|ELDER|DEITY;
			  valid = 1, result++;
		       }
		    }
                    if(!result) output(p,player,0,1,0,"%s"ANSI_LGREEN"Sorry, '"ANSI_LWHITE"%s"ANSI_LGREEN"' is an %s.",(cr) ? "\n":"",scratch_buffer,(*scratch_buffer == '&') ? "invalid creation/last used date specification":"unknown object type/field/flag"), cr = 0;
		 }
	   }
 
           if(!valid) {
	      output(p,player,0,1,0,"%s"ANSI_LGREEN"Please specify the object type(s)/field(s)/flag(s) to search for/in.\n",(cr) ? "\n":"");
              return;
	   }
	} else arg2 = arg1;

        setreturn(OK,COMMAND_SUCC);
        command_type                      |= NO_USAGE_UPDATE;
        cached_scrheight                   = db[player].data->player.scrheight;
        db[player].data->player.scrheight -= 6;

        if(!(object_flags & SEARCH_ALL_OBJECTS)) object_flags |= SEARCH_ALL_OBJECTS;
        if(!field_flags) field_flags = SEARCH_NAME;
        if(!Level4(db[player].owner)) field_flags &= ~(SEARCH_EMAIL|SEARCH_LASTSITE);
        set_conditions_ps(player,flags_inc,flags_mask,flags_inc2,flags_mask2,flags_exc,object_flags,field_flags,arg2,302);
        entiredb_initgrouprange();

        /* ---->  Show results of search  <---- */
        if(IsHtml(p)) {
           html_anti_reverse(p,1);
           output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
	} else if(!in_command && p && !p->pager && !IsHtml(p) && More(player)) pager_init(p);

        if(!in_command) {
           output(p,player,2,1,1,"%sResult(s) of search...%s",IsHtml(p) ? "\016<TR><TH ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><FONT COLOR="HTML_LCYAN" SIZE=4><I>\016":"\n ",IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
           if(!IsHtml(p)) output(p,player,0,1,0,(char *) separator(twidth,0,'-','='));
	}

        if(grp->distance > 0) {
           *scratch_return_string = '\0';
           while(entiredb_grouprange()) {
                 if((field_flags & SEARCH_RACE) && (p1 = getfield(grp->cobject,RACE)) && *p1) sprintf(scratch_return_string,ANSI_DCYAN" \016&nbsp;\016 ("ANSI_LWHITE"%s"ANSI_DCYAN")",p1);
                    else if((field_flags & SEARCH_TITLE) && (p1 = getfield(grp->cobject,TITLE)) && *p1) sprintf(scratch_return_string,ANSI_DCYAN" \016&nbsp;\016 ("ANSI_LWHITE"%s"ANSI_DCYAN")",p1);
                       else if(field_flags & SEARCH_EMAIL) {
                          if(Blank(arg2)) {
                             if(!((p1 = gettextfield(2,'\n',getfield(grp->cobject,EMAIL),0,scratch_buffer)) && *p1))
                                p1 = gettextfield(0,'\n',getfield(grp->cobject,EMAIL),2,scratch_buffer);
			  } else p1 = match_wildcard_list(arg2,'\n',getfield(grp->cobject,EMAIL),scratch_buffer);
                          if(!Blank(p1)) sprintf(scratch_return_string,ANSI_DCYAN" \016&nbsp;\016 ("ANSI_LWHITE"%s"ANSI_DCYAN")",p1);
		       } else if((field_flags & SEARCH_LASTSITE) && (p1 = getfield(grp->cobject,LASTSITE)) && *p1) sprintf(scratch_return_string,ANSI_DCYAN" \016&nbsp;\016 ("ANSI_LWHITE"%s"ANSI_DCYAN")",p1);
                          else if((field_flags & SEARCH_WWW) && (p1 = getfield(grp->cobject,WWW)) && *p1) sprintf(scratch_return_string,ANSI_DCYAN" \016&nbsp;\016 ("ANSI_LWHITE"%s"ANSI_DCYAN")",p1);
                             else *scratch_return_string = '\0';
                 output(p,player,2,1,3,"%s%s%s%s",IsHtml(p) ? "\016<TR><TD ALIGN=LEFT>\016"ANSI_DWHITE:ANSI_DWHITE" ",getcname(player,grp->cobject,1,UPPER|INDEFINITE),scratch_return_string,IsHtml(p) ? "\016</TD></TR>\016":"\n");
	   }

           if(!in_command) {
              if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
              output(p,player,2,1,0,"%sTotal items found: \016&nbsp;\016 "ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><TD>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",listed_items(scratch_return_string,1),IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
	   }
        } else {
           output(p,player,2,1,0,"%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TD>"ANSI_LCYAN"<I>*** &nbsp; NOTHING FOUND &nbsp; ***</I></TD></TR>\016":" ***  NOTHING FOUND  ***\n");
           if(!in_command) {
              if(!IsHtml(p)) output(p,player,0,1,0,(char *) separator(twidth,0,'-','='));
              output(p,player,2,1,0,"%sTotal items found: \016&nbsp;\016 "ANSI_DWHITE"None.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><TD>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
	   }
	}

        if(IsHtml(p)) {
           output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
           html_anti_reverse(p,0);
	}
        db[player].data->player.scrheight = cached_scrheight;
        command_type                     &= ~NO_USAGE_UPDATE;
     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, only Experienced Builders and above may use '"ANSI_LWHITE"@find"ANSI_LGREEN"' from within a compound command.");
}

/* ---->  Find object(s) of given type(s) that belong to a specific character  <---- */
void search_list(CONTEXT)
{
     int                      object_flags = 0,flags_inc = 0,flags_mask = 0,flags_inc2 = 0,flags_mask2 = 0,flags_exc = 0;
     unsigned char            cached_scrheight,twidth = output_terminal_width(player),cr = 1,all = 0;
     dbref                    object = NOTHING,who = player;
     struct   descriptor_data *p = getdsc(player);
     int                      result,valid = 0;
     const    char            *p1;
     char                     *p2;

     setreturn(ERROR,COMMAND_FAIL);
     if(!(in_command && !(Level4(db[player].owner) || Experienced(db[player].owner)))) {
        if(!strcasecmp("all",arg1)) {
           arg1 = (char *) parse_grouprange(player,arg1 + 3,ALL,1);
           all = 1;
	} else {
           arg1 = (char *) parse_grouprange(player,arg1,ALL,1);
           if(!strcasecmp("all",arg1)) all = 1;
	}

        if(!(all && !(Level4(db[player].owner) || Experienced(db[player].owner)))) {
           if(strcasecmp("email",arg1)) {

              /* ---->  Find character/object  <---- */
              if(!(all && Blank(arg2))) {
                 if(!Blank(arg1) && !Blank(arg2)) {
                    if(!all) {
                       who = match_preferred(player,player,arg1,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
                       if(!Valid(who)) return;
 
                       /* ---->  Security constraints  <---- */
                       if(!Level4(db[player].owner) && !Experienced(db[player].owner) && !can_write_to(player,who,1) && !(!in_command && friendflags_set(who,player,NOTHING,FRIEND_READ))) {
                          output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only list your own objects or those that belong to your puppets.");
                          return;
		       }
  
                       if(Typeof(who) != TYPE_CHARACTER) object = who, who = player;
		    } else who = NOTHING;
		 } else if(!all) who = Uid(player);
                    else who = NOTHING;

                 /* ---->  Start parsing object/flag types from first parameter  <---- */
                 if(Blank(arg2)) arg2 = arg1;
                 p1 = arg2;
                 while(*p1) {
                       while(*p1 && (*p1 == ' ')) p1++;
                       if(*p1) {

                          /* ---->  Determine what object type/flag word is  <---- */
                          for(p2 = scratch_buffer; *p1 && (*p1 != ' '); *p2++ = *p1++);
                          *p2 = '\0';
                          if((*scratch_buffer != '&') || !(result = parse_date_boundaries(scratch_buffer,scratch_return_string))) {
                             if((result = parse_objecttype(scratch_buffer))) {
                                valid = object_flags |= result;
			     } else if((result = parse_flagtype(scratch_buffer,&object_flags,&flags_mask))) {
                                valid = flags_inc |= result;
			     } else if((result = parse_flagtype2(scratch_buffer,&object_flags,&flags_mask2))) {
                                valid = flags_inc2 |= result;
			     } else if(string_prefix("mortals",scratch_buffer)) {
				object_flags |= SEARCH_CHARACTER;
				flags_exc    |= BUILDER|APPRENTICE|WIZARD|ELDER|DEITY;
				valid = 1, result++;
			     } else if(string_prefix("all",scratch_buffer)) {
				object_flags = SEARCH_ALL_TYPES;
				flags_mask2  = 0, flags_mask = 0;
				flags_inc2   = 0, flags_inc  = 0;
				flags_exc    = 0, valid      = 1;
				result++;
			     }
			  }
	  
                          /* ---->  Complain about unknown object types/fields/flags  <---- */
                          if(!result) output(p,player,0,1,0,"%s"ANSI_LGREEN"Sorry, '"ANSI_LWHITE"%s"ANSI_LGREEN"' is an %s.",(cr) ? "\n":"",scratch_buffer,(*scratch_buffer == '&') ? "invalid creation/last used date specification":"unknown object type/flag"), cr = 0;
		       }
		 }
	      } else valid = 1;

              if(valid) {
                 setreturn(OK,COMMAND_SUCC);
                 command_type                      |= NO_USAGE_UPDATE;
                 cached_scrheight                   = db[player].data->player.scrheight;
                 db[player].data->player.scrheight -= 7;

                 if(!(object_flags & SEARCH_ALL_OBJECTS)) object_flags |= SEARCH_ALL_OBJECTS;
                 set_conditions_ps(who,flags_inc,flags_mask,flags_inc2,flags_mask2,flags_exc,object_flags,object,NULL,301);
                 entiredb_initgrouprange();

                 /* ---->  Show results of search  <---- */
                 if(IsHtml(p)) {
                    html_anti_reverse(p,1);
                    output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
		 } else if(!in_command && p && !p->pager && !IsHtml(p) && More(player)) pager_init(p);

                 if(!in_command) {
                    strcpy(scratch_buffer,"Result(s) of search ");
                    if(object != NOTHING) sprintf(scratch_buffer + strlen(scratch_buffer),"(Objects of specified type(s) inside %s"ANSI_LWHITE"%s"ANSI_LCYAN")",Article(object,LOWER,DEFINITE),unparse_object(player,object,0));
                       else if(who == NOTHING) strcat(scratch_buffer,"(Objects of specified type(s) belonging to anyone)");
                          else if(who != player) sprintf(scratch_buffer + strlen(scratch_buffer),"(Objects of specified type(s) belonging to %s"ANSI_LWHITE"%s"ANSI_LCYAN")",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0));
                             else strcat(scratch_buffer,"(Objects of specified type(s) belonging to you)");
                    output(p,player,2,1,1,"%s%s...%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER><FONT COLOR="HTML_LCYAN" SIZE=4><I>\016":"\n ",scratch_buffer,IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
                    if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
		 }

                 if(grp->distance > 0) {
                    while(entiredb_grouprange())
                          output(p,player,2,1,3,"%s%s%s",IsHtml(p) ? "\016<TR><TD ALIGN=LEFT>\016"ANSI_DWHITE:ANSI_DWHITE" ",getcname(player,grp->cobject,1,UPPER|INDEFINITE),IsHtml(p) ? "\016</TD></TR>\016":"\n");

                    if(!in_command) {
                       if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
                       output(p,player,2,1,0,"%sTotal items found: \016&nbsp;\016 "ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><TD>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",listed_items(scratch_return_string,1),IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
		    }
		 } else {
                    output(p,player,2,1,0,"%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TD>"ANSI_LCYAN"<I>*** &nbsp; NOTHING FOUND &nbsp; ***</I></TD></TR>\016":" ***  NOTHING FOUND  ***\n");
                    if(!in_command) {
                       if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
                       output(p,player,2,1,0,"%sTotal items found: \016&nbsp;\016 "ANSI_DWHITE"None.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><TD>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
		    }
		 }

                 if(IsHtml(p)) {
                    output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
                    html_anti_reverse(p,0);
		 }
                 db[player].data->player.scrheight = cached_scrheight;
                 command_type                     &= ~NO_USAGE_UPDATE;
	      } else output(p,player,0,1,0,"%s"ANSI_LGREEN"Please specify the object type(s)/flag(s) to list/search for.\n",(cr) ? "\n":"");
	   } else output(p,player,0,1,0,(Level4(db[player].owner)) ? ANSI_LGREEN"Please use '"ANSI_LYELLOW"@find email = *"ANSI_LGREEN"' to list the E-mail addresses of characters (HINT:  Try using '"ANSI_LWHITE"*<HOST NAME>*"ANSI_LGREEN"' instead of '"ANSI_LWHITE"*"ANSI_LGREEN"' to list characters at a particular site.)":ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above can list the E-mail addresses of characters.");
	} else output(p,player,0,1,0,ANSI_LGREEN"Sorry, only Experienced Builders and above may list everything in the database.");
     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, only Experienced Builders and above may use '"ANSI_LWHITE"@list"ANSI_LGREEN"' from within a compound command.");
}
