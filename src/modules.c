/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| MODULES.C  -  Description and author information for all TCZ source code    |
|               modules (This information can be obtained on TCZ using the    |
|               'modules' command.)                                           |
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
| Module originally designed and written by:  J.P.Boggis 20/05/2000.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: modules.c,v 1.5 2005/01/25 19:05:42 tcz_monster Exp $

*/


#include <stdlib.h>
#include <string.h>
#include <time.h>


/* ---->  Standard TCZ headers  <---- */
#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "object_types.h"
#include "flagset.h"


/* ---->  Module authors and modules list  <---- */
#include "modules_authors.h"
#include "modules.h"


/* ---->  Start of alphabetically sorted modules/authors  <---- */
struct module_details *modules_start = NULL;
struct author_details *authors_start = NULL;


/* ---->  {J.P.Boggis 27/05/2000}  Determine type of module ('*.c' = 1, '*.h' = 2)  <---- */
int modules_type(const char *name)
{
    const char *ptr = name;

    if(!Blank(name)) {
       for(ptr = name + strlen(name) - 1; (ptr >= name) && (*ptr != '.'); ptr--);
       if(ptr >= name) {
          if(*(++ptr)) {
             if(!strcasecmp("c",ptr)) return(1);
             if(!strcasecmp("h",ptr)) return(2);
	  }
       }
    }
    return(0);
}

/* ---->  {J.P.Boggis 26/05/2000}  Sort modules into alphabetical order  <---- */
int modules_sort_modules(void)
{
    struct module_details *current,*last;
    int    module,count = 0;
    
    for(module = 0; modules[module].name; module++) {
        current = modules_start, last = NULL;
        for(; current && (modules_type(modules[module].name) > modules_type(current->name)); last = current, current = current->next);
        for(; current && (strcasecmp(modules[module].name,current->name) > 0); last = current, current = current->next);
        if(last) {
	   modules[module].next = last->next;
           last->next = &(modules[module]);
	} else modules_start = &(modules[module]);
    }
    return(count);
}

/* ---->   {J.P.Boggis 26/05/2000}  Sort authors into alphabetical order  <---- */
int modules_sort_authors(void)
{
    struct author_details *current,*last;
    int    author,count = 0;

    for(author = 0; authors[author].name; author++) {
        for(current = authors_start, last = NULL; current && (strcasecmp(authors[author].name,current->name) > 0); last = current, current = current->next);
        if(last) {
	   authors[author].next = last->next;
	   last->next = &(authors[author]);
        } else authors_start = &(authors[author]);
    }
    return(count);
}

/* ---->   {J.P.Boggis 26/05/2000}  List module info and list of authors  <---- */
void modules_modules_view(dbref player,const char *module)
{
     struct module_details *current,*exact = NULL,*prefix = NULL,*closest = NULL,*desc = NULL;
     const  char *initials,*original,*datefrom,*dateto;
     int    twidth = output_terminal_width(player);
     struct descriptor_data *d = getdsc(player);
     char   authorbuffer[BUFFER_LEN];
     struct author_details *cauthor;
     int    cached_scrheight;
     char   cdate[KB];
     struct tm *now;
     time_t stamp;
     char   *ptr;

     setreturn(ERROR,COMMAND_FAIL);
     if(player == NOBODY) {
        for(d = descriptor_list; d && (d->player != NOBODY); d = d->next);
        player = NOTHING;
        if(!d) return;
        if(!IsHtml(d)) d->player = NOTHING;
     }

     /* ---->  Find module by name  <---- */
     for(current = modules_start; current; current = ((current) ? current->next:NULL)) {
         if(!Blank(current->name)) {
            if(strcasecmp(current->name,module)) {
               if(string_prefix(current->name,module) && (!prefix || (strlen(prefix->name) > strlen(current->name))))
		  prefix = current;
	       if(instring(module,current->name) && (!closest || (strlen(closest->name) > strlen(current->name))))
		  closest = current;
	       if(!desc && !Blank(current->desc) && instring(module,current->desc))
		  desc = current;
	    } else exact = current, current = NULL;
	 }
     }

     /* ---->  Choose most appropriate matched name  <---- */
     if(!exact) {
        if(prefix) {
           exact = prefix;
	} else if(closest) {
           exact = closest;
	} else if(desc) {
           exact = desc;
	}
     }

     /* ---->  Display module details (If found)  <---- */
     if(exact) {
	if(IsHtml(d)) {
	   html_anti_reverse(d,1);
	   output(d,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
	   output(d,d->player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH COLSPAN=2><FONT COLOR="HTML_LCYAN" SIZE=4><I>Information about the module '"ANSI_LWHITE"%s"ANSI_LCYAN"' ("ANSI_LWHITE"*"ANSI_LCYAN" = Original author(s))...</I></FONT></TH></TR>\016",exact->name);
	} else {
	   if(!in_command && d && !d->pager && More(player)) pager_init(d);
	   output(d,d->player,0,1,0,"\n "ANSI_LCYAN"Information about the module '"ANSI_LWHITE"%s"ANSI_LCYAN"' ("ANSI_LWHITE"*"ANSI_LCYAN" = Original author(s))...",exact->name);
	   output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','='));
	}

        /* ---->  Module information  <---- */
        output(d,d->player,2,1,16,"%sModule name:%s%s%s",IsHtml(d) ? "\016<TR><TH WIDTH=15% ALIGN=RIGHT BGCOLOR="HTML_TABLE_GREEN">\016"ANSI_LGREEN:"  "ANSI_LGREEN,IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016"ANSI_LWHITE:"  "ANSI_LWHITE,!Blank(exact->name) ? exact->name:ANSI_DCYAN"N/A",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        output(d,d->player,2,1,16,"%sDescription:%s%s%s%s",IsHtml(d) ? "\016<TR><TH WIDTH=15% ALIGN=RIGHT BGCOLOR="HTML_TABLE_GREEN">\016"ANSI_LGREEN:"  "ANSI_LGREEN,IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016"ANSI_LWHITE:"  "ANSI_LWHITE,!Blank(exact->desc) ? exact->desc:"No description.",(Blank(exact->desc) || (*(exact->desc + strlen(exact->desc) - 1) == '.')) ? "":".",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        output(d,d->player,2,1,16,"%sDate created:%s%s%s",IsHtml(d) ? "\016<TR><TH WIDTH=15% ALIGN=RIGHT BGCOLOR="HTML_TABLE_GREEN">\016"ANSI_LGREEN:" "ANSI_LGREEN,IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016"ANSI_LWHITE:"  "ANSI_LWHITE,!Blank(exact->date) ? exact->date:ANSI_DCYAN"N/A",IsHtml(d) ? "\016</TD></TR>\016":"\n");

        /* ---->  Create list of module authors  <---- */
        if(!Blank(exact->authors)) {
           for(cauthor = authors; cauthor; cauthor->author = 0, cauthor = cauthor->next);
           strcpy(authorbuffer,exact->authors);
           ptr = authorbuffer;
	   while(*ptr) {
                 for(; *ptr && ((*ptr == ' ') || (*ptr == '\n') || (*ptr == '>')); ptr++);
                 if(*ptr) {

                    /* ---->  Initials of author  <---- */
                    for(initials = ptr++; *ptr && !((*ptr == ' ') || (*ptr == '\n') || (*ptr == '>')); ptr++);
                    if(*ptr) *ptr++ = '\0';

                    /* ---->  Find author in authors table  <---- */
                    if(!Blank(initials)) {
                       for(cauthor = authors; cauthor && (cauthor->initials && strcasecmp(cauthor->initials,initials)); cauthor = cauthor->next);
                       if(cauthor) {

			  /* ---->  Original author of module?  <---- */
                          for(; *ptr && ((*ptr == ' ') || (*ptr == '|')); ptr++);
                          if((*ptr != '\n') && (*ptr != '>')) {
                             for(original = ptr++; *ptr && !((*ptr == ' ') || (*ptr == '\n') || (*ptr == '>')); ptr++);
                             if(*ptr) *ptr++ = '\0';
			  }

                          /* ---->  Get date from  <---- */
                          for(; *ptr && ((*ptr == ' ') || (*ptr == '|')); ptr++);
                          if((*ptr != '\n') && (*ptr != '>')) {
                             for(datefrom = ptr++; *ptr && !((*ptr == ' ') || (*ptr == '\n') || (*ptr == '>')); ptr++);
                             if(*ptr) *ptr++ = '\0';
			  }

                          /* ---->  Get date to  <---- */
                          for(; *ptr && ((*ptr == ' ') || (*ptr == '|')); ptr++);
                          if((*ptr != '\n') && (*ptr != '>')) {
                             for(dateto = ptr++; *ptr && !((*ptr == ' ') || (*ptr == '\n') || (*ptr == '>')); ptr++);
                             if(*ptr) *ptr++ = '\0';
			  }

                          /* ---->  Set date from/to  <---- */
                          cauthor->datefrom = !strcasecmp(datefrom,"NULL") ? NULL:datefrom;
                          cauthor->dateto   = !strcasecmp(dateto,"NULL")   ? NULL:dateto;
                          cauthor->author   = 1;
			  cauthor->original = (original && ((*original == 'y') || (*original == 'Y')));
		       }
		    }
		 }
                 for(; *ptr && (*ptr != '>'); ptr++);
                 for(; *ptr && (*ptr == '>'); ptr++);
	   }
	}

        /* ---->  Current date (Used where author->dateto == NULL)  <---- */
        gettime(stamp);
        now = localtime(&stamp);
        sprintf(cdate,"%02d/%02d/%04d",now->tm_mday,now->tm_mon + 1,now->tm_year + 1900);

        /* ---->  Author list  <---- */
        if(IsHtml(d)) {
	   html_anti_reverse(d,1);
	   output(d,player,1,2,0,"<TR><TD ALIGN=CENTER COLSPAN=2 BGCOLOR="HTML_TABLE_DGREY"><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">");
	   output(d,d->player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Date From:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Date To:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Initials:</I></FONT></TH><TH COLSPAN=2><FONT COLOR="HTML_LCYAN" SIZE=4><I>Name:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Nickname:</I></FONT></TH></TR>\016");
	} else {
	   output(d,d->player,0,1,0,separator(d->terminal_width,0,'=','='));
	   output(d,d->player,0,1,0," Date From:  Date To:    Init:  Name:                Nickname:");
	   output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','-'));
	}

        if(Validchar(d->player)) {
           cached_scrheight                     = db[d->player].data->player.scrheight;
           db[d->player].data->player.scrheight = (db[d->player].data->player.scrheight - 13) * 2;
	}

        set_conditions(player,0,0,0,NOTHING,NULL,515);
        union_initgrouprange((union group_data *) authors_start);

	if(grp->distance > 0) {
	   while(union_grouprange())
		 output(d,player,2,1,0,IsHtml(d) ? "\016<TR><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_YELLOW">\016%s%s\016</TD><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_YELLOW">\016"ANSI_LYELLOW"%s\016</TD><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_GREEN"><%sA HREF=\"%sNAME=%s&\"%s TITLE=\"Click to view further information about this author...\">\016%s%s\016<%s/A></TD><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY" WIDTH=10>\016%s%s\016</TD><TD BGCOLOR="HTML_TABLE_YELLOW">\016%s%s\016</TD><TD BGCOLOR="HTML_TABLE_MAGENTA">\016%s%s\016</TD></TR>\016":" %s%-12s"ANSI_LYELLOW"%-12s%s%s%s%s%s%-5s%s%s%-2s%s%-21s%s%s\n",
                        !Blank(grp->cunion->author.datefrom) ? ANSI_LYELLOW:ANSI_DYELLOW,!Blank(grp->cunion->author.datefrom) ? grp->cunion->author.datefrom:"N/A",
                        !Blank(grp->cunion->author.dateto) ? grp->cunion->author.dateto:cdate,
			(Blank(grp->cunion->author.initials) && IsHtml(d)) ? "!":"",IsHtml(d) ? html_server_url(d,0,1,"author"):"",IsHtml(d) ? String(grp->cunion->author.initials):"",(Valid(player) && IsHtml(d)) ? " TARGET=_blank":"",!Blank(grp->cunion->author.initials) ? ANSI_LGREEN:ANSI_DGREEN,!Blank(grp->cunion->author.initials) ? grp->cunion->author.initials:"N/A",(Blank(grp->cunion->author.initials) && IsHtml(d)) ? "!":"",
			(grp->cunion->author.original) ? ANSI_LWHITE:"",(grp->cunion->author.original) ? "*":(IsHtml(d) ? " ":""),
                        !Blank(grp->cunion->author.name) ? ANSI_LYELLOW:ANSI_DYELLOW,!Blank(grp->cunion->author.name) ? grp->cunion->author.name:"N/A",
			!Blank(grp->cunion->author.nickname) ? ANSI_LMAGENTA:ANSI_DMAGENTA,!Blank(grp->cunion->author.nickname) ? grp->cunion->author.nickname:"N/A");

	   if(!in_command) {
	      if(!IsHtml(d)) output(d,player,0,1,0,separator(twidth,0,'-','='));
	      output(d,player,2,1,1,"%sAuthors listed: \016&nbsp;\016 "ANSI_DWHITE"%s%s",IsHtml(d) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_MGREY"><TD COLSPAN=6>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",listed_items(scratch_return_string,1),IsHtml(d) ? "\016</B></TD></TR>\016":"\n\n");
	   }
	} else {
	   output(d,player,2,1,0,"\016<TR ALIGN=CENTER><TD COLSPAN=6>"ANSI_LCYAN"<I> *** &nbsp; THIS MODULE HAS NO AUTHOR INFORMATION &nbsp; ***</I></TD></TR>\016");
	   if(!in_command) {
	      if(!IsHtml(d)) output(d,player,0,1,0,separator(twidth,0,'-','='));
	      output(d,player,2,1,1,"%sAuthors listed: \016&nbsp;\016 "ANSI_DWHITE"None.%s",IsHtml(d) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><TD COLSPAN=6>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",IsHtml(d) ? "\016</B></TD></TR>\016":"\n\n");
	   }
	}

        if(Validchar(d->player)) db[d->player].data->player.scrheight = cached_scrheight;
	if(IsHtml(d)) output(d,player,1,2,0,"</TABLE></TD></TR>");

	if(IsHtml(d)) {
	   output(d,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
	   html_anti_reverse(d,0);
	}
        setreturn(OK,COMMAND_SUCC);
     } else output(d,player,0,1,0,ANSI_LGREEN"Sorry, a module with the name '"ANSI_LWHITE"%s"ANSI_LGREEN"' cannot be found (Type '"ANSI_LYELLOW"modules"ANSI_LGREEN"' for a list.)",module);
}

/* ---->   {J.P.Boggis 26/05/2000}  List modules (Date created, name and brief description)  <---- */
void modules_modules_list(dbref player)
{
     int    twidth = output_terminal_width(player);
     struct descriptor_data *d = getdsc(player);
     int    cached_scrheight,adjust;
     char   buffer[BUFFER_LEN];

     setreturn(ERROR,COMMAND_FAIL);
     if(player == NOBODY) {
        for(d = descriptor_list; d && (d->player != NOBODY); d = d->next);
        player = NOTHING;
        if(!d) return;
        if(!IsHtml(d)) d->player = NOTHING;
     }

     if(IsHtml(d)) {
        html_anti_reverse(d,1);
        output(d,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
        output(d,d->player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Date:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Module:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Description:</I></FONT></TH></TR>\016");
     } else {
        if(!in_command && d && player != NOTHING && !d->pager && More(player)) pager_init(d);
        output(d,d->player,0,1,0,"\n Date:       Module:               Description:");
        output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','='));
     }

     if(Validchar(d->player)) {
        cached_scrheight                     = db[d->player].data->player.scrheight;
        db[d->player].data->player.scrheight = (db[d->player].data->player.scrheight - 10) * 2;
     }

     set_conditions(player,0,0,0,NOTHING,NULL,500);
     union_initgrouprange((union group_data *) modules_start);

     if(grp->distance > 0) {
        while(union_grouprange()) {

              /* ---->  Truncate description to fit screen width  <---- */
              if(!Blank(grp->cunion->module.desc)) {
		 if(!IsHtml(d)) {
		    strncpy(buffer,grp->cunion->module.desc,(twidth - 35));
		    if(strlen(grp->cunion->module.desc) >= (twidth - 35)) {
		       for(adjust = 3; ((twidth - 35 - adjust) > 0) && (*(buffer + twidth - 35 - adjust - 1) == ' '); adjust++);
		       strcpy(buffer + twidth - 35 - adjust,"...");
		    }
		 } else strcpy(buffer,grp->cunion->module.desc);
		 if(*(buffer + strlen(buffer) - 1) != '.')
		    strcat(buffer,".");
	      } else strcpy(buffer,"No description.");

	      output(d,player,2,1,35,IsHtml(d) ? "\016<TR><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_YELLOW">\016%s%s\016</TD><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_GREEN"><%sA HREF=\"%sNAME=%s&\"%s TITLE=\"Click to view a list of modules which this author has worked on...\">\016%s%s\016<%s/A></TD><TD>\016"ANSI_LWHITE"%s\016</TD></TR>\016":" %s%-12s%s%s%s%s%s%-22s%s"ANSI_LWHITE"%s\n",
                     !Blank(grp->cunion->module.date) ? ANSI_LYELLOW:ANSI_DYELLOW,!Blank(grp->cunion->module.date) ? grp->cunion->module.date:"N/A",
                     (Blank(grp->cunion->module.name) && IsHtml(d)) ? "!":"",IsHtml(d) ? html_server_url(d,0,1,"module"):"",IsHtml(d) ? String(grp->cunion->module.name):"",(Valid(player) && IsHtml(d)) ? " TARGET=_blank":"",!Blank(grp->cunion->module.name) ? ANSI_LGREEN:ANSI_DGREEN,!Blank(grp->cunion->module.name) ? grp->cunion->module.name:"N/A",(Blank(grp->cunion->module.name) && IsHtml(d)) ? "!":"",
                     buffer);
	}

        if(!in_command) {
           if(!IsHtml(d)) output(d,player,0,1,0,(char *) separator(twidth,0,'-','-'));
           output(d,player,2,1,1,"%sFor more information on any of the above modules, type '"ANSI_LGREEN"module <NAME>"ANSI_LWHITE"', e.g: '"ANSI_LGREEN"module tcz.c"ANSI_LWHITE"'.%s",IsHtml(d) ? "\016<TR BGCOLOR="HTML_TABLE_GREY"><TD ALIGN=CENTER COLSPAN=3>"ANSI_LWHITE"<I>\016":ANSI_LWHITE" ",IsHtml(d) ? "\016</TD></TR>\016":"\n");
           if(!IsHtml(d)) output(d,player,0,1,0,separator(twidth,0,'-','='));
           output(d,player,2,1,1,"%sModules listed: \016&nbsp;\016 "ANSI_DWHITE"%s%s",IsHtml(d) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_MGREY"><TD COLSPAN=3>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",listed_items(buffer,1),IsHtml(d) ? "\016</B></TD></TR>\016":"\n\n");
	}
     } else {
        output(d,player,2,1,0,"\016<TR ALIGN=CENTER><TD COLSPAN=3>"ANSI_LCYAN"<I> *** &nbsp; NO MODULES LISTED &nbsp; ***</I></TD></TR>\016");
        if(!in_command) {
           if(!IsHtml(d)) output(d,player,0,1,0,separator(twidth,0,'-','='));
           output(d,player,2,1,1,"%sModules listed: \016&nbsp;\016 "ANSI_DWHITE"None.%s",IsHtml(d) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><TD COLSPAN=3>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",IsHtml(d) ? "\016</B></TD></TR>\016":"\n\n");
	}
     }

     if(Validchar(d->player)) db[d->player].data->player.scrheight = cached_scrheight;
     if(IsHtml(d)) {
        output(d,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
        html_anti_reverse(d,0);
     }
     setreturn(OK,COMMAND_SUCC);
}

/* ---->   {J.P.Boggis 26/05/2000}  List author info and list of modules worked on by author  <---- */
void modules_authors_view(dbref player,const char *author)
{
     struct author_details *nameexact = NULL,*nameprefix = NULL,*nameclosest = NULL;
     struct author_details *nickexact = NULL,*nickprefix = NULL,*nickclosest = NULL;
     struct author_details *current,*exact = NULL,*prefix = NULL,*closest = NULL;
     int    twidth = output_terminal_width(player),adjust;
     const  char *initials,*datefrom,*dateto,*original;
     struct descriptor_data *d = getdsc(player);
     char   authorbuffer[BUFFER_LEN];
     struct author_details *cauthor;
     struct module_details *cmodule;
     char   buffer[BUFFER_LEN];
     int    cached_scrheight;
     char   cdate[KB];
     struct tm *now;
     time_t stamp;
     char   *ptr;

     setreturn(ERROR,COMMAND_FAIL);
     if(player == NOBODY) {
        for(d = descriptor_list; d && (d->player != NOBODY); d = d->next);
        player = NOTHING;
        if(!d) return;
        if(!IsHtml(d)) d->player = NOTHING;
     }

     /* ---->  Find author by name  <---- */
     for(current = authors_start; current; current = ((current) ? current->next:NULL)) {

	 /* ---->  Match by initials  <---- */
	 if(!Blank(current->initials)) {
	    if(strcasecmp(current->initials,author)) {
	       if(string_prefix(current->initials,author) && (!prefix || (strlen(prefix->initials) > strlen(current->initials))))
		  prefix = current;
	       if(instring(author,current->initials) && (!closest || (strlen(closest->initials) > strlen(current->initials))))
		  closest = current;
	    } else exact = current, current = NULL;
	 }

	 /* ---->  Match by name  <---- */
	 if(current && !Blank(current->name)) {
	    if(strcasecmp(current->name,author)) {
	       if(string_prefix(current->name,author) && (!nameprefix || (strlen(nameprefix->name) > strlen(current->name))))
		  nameprefix = current;
	       if(instring(author,current->name) && (!nameclosest || (strlen(nameclosest->name) > strlen(current->name))))
		  nameclosest = current;
	    } else nameexact = current, current = NULL;
	 }

	 /* ---->  Match by nickname  <---- */
	 if(current && !Blank(current->nickname)) {
	    if(strcasecmp(current->nickname,author)) {
	       if(string_prefix(current->nickname,author) && (!nickprefix || (strlen(nickprefix->nickname) > strlen(current->nickname))))
		  nickprefix = current;
	       if(instring(author,current->nickname) && (!nickclosest || (strlen(nickclosest->nickname) > strlen(current->nickname))))
		  nickclosest = current;
	    } else nickexact = current, current = NULL;
	 }
     }

     /* ---->  Choose most appropriate matched name  <---- */
     if(!exact) {
	if(prefix) exact = prefix;
	   else if(closest) exact = closest;
	      else if(nameexact) exact = nameexact;
		 else if(nameprefix) exact = nameprefix;
		    else if(nameclosest) exact = nameclosest;
		       else if(nickexact) exact = nickexact;
			  else if(nickprefix) exact = nickprefix;
			     else if(nickclosest) exact = nickclosest;
     }

     /* ---->  Display author details (If found)  <---- */
     if(exact) {
	if(IsHtml(d)) {
	   html_anti_reverse(d,1);
	   output(d,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
	   output(d,d->player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH COLSPAN=2><FONT COLOR="HTML_LCYAN" SIZE=4><I>Author information about "ANSI_LWHITE"%s"ANSI_LCYAN" ("ANSI_LYELLOW"%s"ANSI_LCYAN") ("ANSI_LWHITE"*"ANSI_LCYAN" = Original author)...</I></FONT></TH></TR>\016",exact->name,!Blank(exact->nickname) ? exact->nickname:ANSI_DCYAN"N/A");
	} else {
	   if(!in_command && d && !d->pager && More(player)) pager_init(d);
	   output(d,d->player,0,1,0,"\n "ANSI_LCYAN"Author information about "ANSI_LWHITE"%s"ANSI_LCYAN" ("ANSI_LYELLOW"%s"ANSI_LCYAN") ("ANSI_LWHITE"*"ANSI_LCYAN" = Original author)...",exact->name,!Blank(exact->nickname) ? exact->nickname:ANSI_DCYAN"N/A");
	   output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','='));
	}

        /* ---->  Author information  <---- */
        output(d,d->player,2,1,16,"%sAuthor name:%s%s%s",IsHtml(d) ? "\016<TR><TH WIDTH=15% ALIGN=RIGHT BGCOLOR="HTML_TABLE_GREEN">\016"ANSI_LGREEN:"  "ANSI_LGREEN,IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016"ANSI_LWHITE:"  "ANSI_LWHITE,!Blank(exact->name) ? exact->name:ANSI_DCYAN"N/A",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        output(d,d->player,2,1,16,"%sNickname:%s%s%s",IsHtml(d) ? "\016<TR><TH WIDTH=15% ALIGN=RIGHT BGCOLOR="HTML_TABLE_GREEN">\016"ANSI_LGREEN:"     "ANSI_LGREEN,IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016"ANSI_LWHITE:"  "ANSI_LWHITE,!Blank(exact->nickname) ? exact->nickname:ANSI_DCYAN"N/A",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        output(d,d->player,2,1,16,"%sInitials:%s%s%s",IsHtml(d) ? "\016<TR><TH WIDTH=15% ALIGN=RIGHT BGCOLOR="HTML_TABLE_GREEN">\016"ANSI_LGREEN:"     "ANSI_LGREEN,IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016"ANSI_LWHITE:"  "ANSI_LWHITE,!Blank(exact->initials) ? exact->initials:ANSI_DCYAN"N/A",IsHtml(d) ? "\016</TD></TR>\016":"\n");
        output(d,d->player,2,1,16,"%sE-mail:%s\016<%sA HREF=\"mailto:%s\">\016%s\016<%s/A>\016%s",IsHtml(d) ? "\016<TR><TH WIDTH=15% ALIGN=RIGHT BGCOLOR="HTML_TABLE_GREEN">\016"ANSI_LGREEN:"       "ANSI_LGREEN,IsHtml(d) ? "\016</TH><TD ALIGN=LEFT>\016"ANSI_LWHITE:"  "ANSI_LBLUE""ANSI_UNDERLINE,(Blank(exact->email) && IsHtml(d)) ? "!":"",String(exact->email),!Blank(exact->email) ? exact->email:ANSI_DCYAN"N/A",(Blank(exact->email) && IsHtml(d)) ? "!":"",IsHtml(d) ? "\016</TD></TR>\016":"\n");

        /* ---->  Create list of modules author has worked on  <---- */
        for(cmodule = modules; cmodule; cmodule->module = 0, cmodule = cmodule->next);
        for(cmodule = modules; cmodule; cmodule = cmodule->next)
            if(cmodule->authors) {
	       strcpy(authorbuffer,cmodule->authors);
	       ptr = authorbuffer;
	       while(*ptr) {
		     for(; *ptr && ((*ptr == ' ') || (*ptr == '\n') || (*ptr == '>')); ptr++);
		     if(*ptr) {

			/* ---->  Initials of author  <---- */
			for(initials = ptr++; *ptr && !((*ptr == ' ') || (*ptr == '\n') || (*ptr == '>')); ptr++);
			if(*ptr) *ptr++ = '\0';

			/* ---->  Lookup author in authors table  <---- */
			if(!Blank(initials) && !strcasecmp(initials,exact->initials)) {
			   for(cauthor = authors; cauthor && (cauthor->initials && strcasecmp(cauthor->initials,initials)); cauthor = cauthor->next);
			   if(cauthor) {

			      /* ---->  Original author of module?  <---- */
                              for(; *ptr && ((*ptr == ' ') || (*ptr == '|')); ptr++);
                              if((*ptr != '\n') && (*ptr != '>')) {
                                 for(original = ptr++; *ptr && !((*ptr == ' ') || (*ptr == '\n') || (*ptr == '>')); ptr++);
                                 if(*ptr) *ptr++ = '\0';
			      }

			      /* ---->  Get date from  <---- */
			      for(; *ptr && ((*ptr == ' ') || (*ptr == '|')); ptr++);
			      if((*ptr != '\n') && (*ptr != '>')) {
				 for(datefrom = ptr++; *ptr && !((*ptr == ' ') || (*ptr == '\n') || (*ptr == '>')); ptr++);
				 if(*ptr) *ptr++ = '\0';
			      }

			      /* ---->  Get date to  <---- */
			      for(; *ptr && ((*ptr == ' ') || (*ptr == '|')); ptr++);
			      if((*ptr != '\n') && (*ptr != '>')) {
				 for(dateto = ptr++; *ptr && !((*ptr == ' ') || (*ptr == '\n') || (*ptr == '>')); ptr++);
				 if(*ptr) *ptr++ = '\0';
			      }

			      /* ---->  Set date from/to  <---- */
                              if(strcasecmp(datefrom,"NULL"))
                                 cmodule->datefrom = (char *) alloc_string(datefrom);
                              if(strcasecmp(dateto,"NULL"))
                                 cmodule->dateto   = (char *) alloc_string(dateto);
			      cmodule->module   = 1;
			      cmodule->original = (original && ((*original == 'y') || (*original == 'Y')));
			   }
			}
		     }
		     for(; *ptr && (*ptr != '>'); ptr++);
		     for(; *ptr && (*ptr == '>'); ptr++);
	       }
	    }

        /* ---->  Current date (Used where module->dateto == NULL)  <---- */
        gettime(stamp);
        now = localtime(&stamp);
        sprintf(cdate,"%02d/%02d/%04d",now->tm_mday,now->tm_mon + 1,now->tm_year + 1900);

        /* ---->  Modules list  <---- */
        if(IsHtml(d)) {
	   html_anti_reverse(d,1);
	   output(d,player,1,2,0,"<TR><TD ALIGN=CENTER COLSPAN=2 BGCOLOR="HTML_TABLE_DGREY"><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">");
	   output(d,d->player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Date From:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Date To:</I></FONT></TH><TH COLSPAN=2><FONT COLOR="HTML_LCYAN" SIZE=4><I>Module:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Description:</I></FONT></TH></TR>\016");
	} else {
	   output(d,d->player,0,1,0,separator(d->terminal_width,0,'=','='));
	   output(d,d->player,0,1,0," Date From:  Date To:     Module:               Description:");
	   output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','-'));
	}

        if(Validchar(d->player)) {
           cached_scrheight                     = db[d->player].data->player.scrheight;
           db[d->player].data->player.scrheight = (db[d->player].data->player.scrheight - 14) * 2;
	}

        set_conditions(player,0,0,0,NOTHING,NULL,514);
        union_initgrouprange((union group_data *) modules_start);

	if(grp->distance > 0) {
	   while(union_grouprange()) {

		 /* ---->  Truncate description to fit screen width  <---- */
		 if(!Blank(grp->cunion->module.desc)) {
		    if(!IsHtml(d)) {
		       strncpy(buffer,grp->cunion->module.desc,(twidth - 48));
		       if(strlen(grp->cunion->module.desc) >= (twidth - 48)) {
			  for(adjust = 3; ((twidth - 48 - adjust) > 0) && (*(buffer + twidth - 48 - adjust - 1) == ' '); adjust++);
			  strcpy(buffer + twidth - 48 - adjust,"...");
		       }
		    } else strcpy(buffer,grp->cunion->module.desc);
		    if(*(buffer + strlen(buffer) - 1) != '.')
		       strcat(buffer,".");
		 } else strcpy(buffer,"No description.");

		 output(d,player,2,1,0,IsHtml(d) ? "\016<TR><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_YELLOW">\016%s%s\016</TD><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_YELLOW">\016"ANSI_LYELLOW"%s\016</TD><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY" WIDTH=10>\016%s%s\016</TD><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_GREEN"><%sA HREF=\"%sNAME=%s&\"%s TITLE=\"Click to view further information about this source code module...\">\016%s%s\016<%s/A></TD><TD>\016"ANSI_LWHITE"%s\016</TD></TR>\016":" %s%-12s"ANSI_LYELLOW"%-11s%s%-2s%s%s%s%s%s%-22s%s"ANSI_LWHITE"%s\n",
                        !Blank(grp->cunion->module.datefrom) ? ANSI_LYELLOW:ANSI_DYELLOW,!Blank(grp->cunion->module.datefrom) ? grp->cunion->module.datefrom:"N/A",
                        !Blank(grp->cunion->module.dateto) ? grp->cunion->module.dateto:cdate,
			(grp->cunion->module.original) ? ANSI_LWHITE:"",(grp->cunion->module.original) ? "*":(IsHtml(d) ? " ":""),
			(Blank(grp->cunion->module.name) && IsHtml(d)) ? "!":"",IsHtml(d) ? html_server_url(d,0,1,"module"):"",IsHtml(d) ? String(grp->cunion->module.name):"",(Valid(player) && IsHtml(d)) ? " TARGET=_blank":"",!Blank(grp->cunion->module.name) ? ANSI_LGREEN:ANSI_DGREEN,!Blank(grp->cunion->module.name) ? grp->cunion->module.name:"N/A",(Blank(grp->cunion->module.name) && IsHtml(d)) ? "!":"",
                        buffer);
	   }

	   if(!in_command) {
	      if(!IsHtml(d)) output(d,player,0,1,0,separator(twidth,0,'-','='));
	      output(d,player,2,1,1,"%sModules listed: \016&nbsp;\016 "ANSI_DWHITE"%s%s",IsHtml(d) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_MGREY"><TD COLSPAN=5>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",listed_items(buffer,1),IsHtml(d) ? "\016</B></TD></TR>\016":"\n\n");
	   }
	} else {
	   output(d,player,2,1,0,"\016<TR ALIGN=CENTER><TD COLSPAN=5>"ANSI_LCYAN"<I> *** &nbsp; THIS AUTHOR HAS NOT WORKED ON ANY SOURCE CODE MODULES &nbsp; ***</I></TD></TR>\016");
	   if(!in_command) {
	      if(!IsHtml(d)) output(d,player,0,1,0,separator(twidth,0,'-','='));
	      output(d,player,2,1,1,"%sModules listed: \016&nbsp;\016 "ANSI_DWHITE"None.%s",IsHtml(d) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><TD COLSPAN=5>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",IsHtml(d) ? "\016</B></TD></TR>\016":"\n\n");
	   }
	}

        if(Validchar(d->player)) db[d->player].data->player.scrheight = cached_scrheight;
	if(IsHtml(d)) output(d,player,1,2,0,"</TABLE></TD></TR>");

        for(cmodule = modules; cmodule; cmodule->module = 0, cmodule = cmodule->next) {
            if(cmodule->datefrom) FREENULL(cmodule->datefrom);
            if(cmodule->dateto)   FREENULL(cmodule->dateto);
	}

	if(IsHtml(d)) {
	   output(d,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
	   html_anti_reverse(d,0);
	}
        setreturn(OK,COMMAND_SUCC);
     } else output(d,player,0,1,0,ANSI_LGREEN"Sorry, a module author with the name '"ANSI_LWHITE"%s"ANSI_LGREEN"' cannot be found (Type '"ANSI_LYELLOW"authors"ANSI_LGREEN"' for a list.)",author);
}

/* ---->   {J.P.Boggis 26/05/2000}  List authors (Initials, name, nickname and E-mail)  <---- */
void modules_authors_list(dbref player)
{
     int    twidth = output_terminal_width(player);
     struct descriptor_data *d = getdsc(player);
     int    cached_scrheight,adjust;
     char   buffer[BUFFER_LEN];

     setreturn(ERROR,COMMAND_FAIL);
     if(player == NOBODY) {
        for(d = descriptor_list; d && (d->player != NOBODY); d = d->next);
        player = NOTHING;
        if(!d) return;
        if(!IsHtml(d)) d->player = NOTHING;
     }

     if(IsHtml(d)) {
        html_anti_reverse(d,1);
        output(d,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
        output(d,d->player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Initials:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Name:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Nickname:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>E-mail:</I></FONT></TH></TR>\016");
     } else {
        if(!in_command && d && player != NOTHING && !d->pager && More(player)) pager_init(d);
        output(d,d->player,0,1,0,"\n Init:  Name:                Nickname:            E-mail:");
        output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','='));
     }

     if(Validchar(d->player)) {
        cached_scrheight                     = db[d->player].data->player.scrheight;
        db[d->player].data->player.scrheight = (db[d->player].data->player.scrheight - 10) * 2;
     }

     set_conditions(player,0,0,0,NOTHING,NULL,500);
     union_initgrouprange((union group_data *) authors_start);

     if(grp->distance > 0) {
        while(union_grouprange()) {

              /* ---->  Truncate E-mail to fit screen width  <---- */
              if(!Blank(grp->cunion->author.email)) {
		 if(!IsHtml(d)) {
		    strncpy(buffer,grp->cunion->author.email,(twidth - 50));
		    if(strlen(grp->cunion->author.email) >= (twidth - 50)) {
		       for(adjust = 3; ((twidth - 50 - adjust) > 0) && (*(buffer + twidth - 50 - adjust - 1) == ' '); adjust++);
		       strcpy(buffer + twidth - 50 - adjust,"...");
		    }
		 } else strcpy(buffer,grp->cunion->author.email);
	      } else strcpy(buffer,ANSI_DBLUE"N/A");

	      output(d,player,2,1,50,IsHtml(d) ? "\016<TR><TD ALIGN=CENTER BGCOLOR="HTML_TABLE_GREEN"><%sA HREF=\"%sNAME=%s&\"%s TITLE=\"Click to view a list of modules which this author has worked on...\">\016%s%s\016<%s/A></TD><TD BGCOLOR="HTML_TABLE_YELLOW">\016%s%s\016</TD><TD BGCOLOR="HTML_TABLE_MAGENTA">\016%s%s\016</TD><TD BGCOLOR="HTML_TABLE_BLUE"><%sA HREF=\"mailto:%s\">\016%s\016<%s/A></TD></TR>\016":" %s%s%s%s%s%-7s%s%s%-21s%s%-21s"ANSI_LBLUE""ANSI_UNDERLINE"%s%s%s%s\n",
                     (Blank(grp->cunion->author.initials) && IsHtml(d)) ? "!":"",IsHtml(d) ? html_server_url(d,0,1,"author"):"",IsHtml(d) ? String(grp->cunion->author.initials):"",(Valid(player) && IsHtml(d)) ? " TARGET=_blank":"",!Blank(grp->cunion->author.initials) ? ANSI_LGREEN:ANSI_DGREEN,!Blank(grp->cunion->author.initials) ? grp->cunion->author.initials:"N/A",(Blank(grp->cunion->author.initials) && IsHtml(d)) ? "!":"",
                     !Blank(grp->cunion->author.name) ? ANSI_LYELLOW:ANSI_DYELLOW,!Blank(grp->cunion->author.name) ? grp->cunion->author.name:"N/A",
                     !Blank(grp->cunion->author.nickname) ? ANSI_LMAGENTA:ANSI_DMAGENTA,!Blank(grp->cunion->author.nickname) ? grp->cunion->author.nickname:"N/A",
                     (Blank(grp->cunion->author.email) && IsHtml(d)) ? "!":"",IsHtml(d) ? String(grp->cunion->author.email):"",buffer,(Blank(grp->cunion->author.email) && IsHtml(d)) ? "!":"");
	}

        if(!in_command) {
           if(!IsHtml(d)) output(d,player,0,1,0,(char *) separator(twidth,0,'-','-'));
           output(d,player,2,1,1,"%sTo view a list of modules worked on by a particular author, simply type '"ANSI_LGREEN"author <NAME>"ANSI_LWHITE"', e.g:  '"ANSI_LGREEN"author jpb"ANSI_LWHITE"'.%s",IsHtml(d) ? "\016<TR BGCOLOR="HTML_TABLE_GREY"><TD ALIGN=CENTER COLSPAN=4>"ANSI_LWHITE"<I>\016":ANSI_LWHITE" ",IsHtml(d) ? "\016</TD></TR>\016":"\n");
           if(!IsHtml(d)) output(d,player,0,1,0,separator(twidth,0,'-','='));
           output(d,player,2,1,1,"%sAuthors listed: \016&nbsp;\016 "ANSI_DWHITE"%s%s",IsHtml(d) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_MGREY"><TD COLSPAN=4>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",listed_items(scratch_return_string,1),IsHtml(d) ? "\016</B></TD></TR>\016":"\n\n");
	}
     } else {
        output(d,player,2,1,0,"\016<TR ALIGN=CENTER><TD COLSPAN=4>"ANSI_LCYAN"<I> *** &nbsp; NO AUTHORS LISTED &nbsp; ***</I></TD></TR>\016");
        if(!in_command) {
           if(!IsHtml(d)) output(d,player,0,1,0,separator(twidth,0,'-','='));
           output(d,player,2,1,1,"%sAuthors listed: \016&nbsp;\016 "ANSI_DWHITE"None.%s",IsHtml(d) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><TD COLSPAN=4>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",IsHtml(d) ? "\016</B></TD></TR>\016":"\n\n");
	}
     }

     if(Validchar(d->player)) db[d->player].data->player.scrheight = cached_scrheight;
     if(IsHtml(d)) {
        output(d,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
        html_anti_reverse(d,0);
     }
     setreturn(OK,COMMAND_SUCC);
}

/* ---->   {J.P.Boggis 26/05/2000}  List modules/authors of module  <---- */
void modules_modules(CONTEXT)
{
     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(arg2)) arg2 = (char *) parse_grouprange(player,arg2,FIRST,1);
        else arg1 = (char *) parse_grouprange(player,arg1,FIRST,1);

     if(!Blank(arg1))
        modules_modules_view(player,arg1);
           else modules_modules_list(player);
}

/* ---->   {J.P.Boggis 26/05/2000}  List authors/module authors  <---- */
void modules_authors(CONTEXT)
{
     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(arg2)) arg2 = (char *) parse_grouprange(player,arg2,FIRST,1);
        else arg1 = (char *) parse_grouprange(player,arg1,FIRST,1);

     if(!Blank(arg1))
        modules_authors_view(player,arg1);
           else modules_authors_list(player);
}
