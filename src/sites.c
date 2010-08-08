/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| SITES.C  -  Implements a database of registered Internet sites, which may   |
|             be restricted (All the way up to a total ban) and given a more  |
|             meaningful description than their usual Internet name/IP        |
|             address.                                                        |
|                                                                             |
|             The registered Internet site database can be browsed, modified, |
|             added to, etc. on-line via the '@site' command.                 |
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
| Module originally designed and written by:  J.P.Boggis 28/12/1994.          |
|                           '@site' code by:  J.P.Boggis 04/09/1995.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: sites.c,v 1.2 2005/01/25 19:14:05 tcz_monster Exp $

*/


#include <netinet/in.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <netdb.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "object_types.h"
#include "flagset.h"
#include "search.h"

#ifdef CYGWIN32
   #include <sys/socket.h>
#endif


struct site_data *sitelist = NULL;
static int sites = 0;

struct site_flag_data {
       const    char *name;
       unsigned char level;
       unsigned char flag;
} siteflags[] = {
       {"Admin",        2,SITE_ADMIN},
       {"Banned",       2,SITE_BANNED},
       {"Create",       4,SITE_CREATE},
       {"DNS",          3,SITE_DNS},
       {"Guests",       4,SITE_GUESTS},
       {"NoDNS",        3,SITE_NODNS},
       {"ReadOnly",     2,SITE_READONLY},
       {"Unconditional",2,SITE_UNCONDITIONAL},
       {NULL,           0,0}
};


/* ---->  Returns full names of flags set on a registered Internet site  <---- */
const char *siteflags_description(short flags)
{
      unsigned char i;

      *scratch_return_string = '\0';
      for(i = 0; siteflags[i].name != NULL; i++)
          if((flags & siteflags[i].flag) == siteflags[i].flag)
             sprintf(scratch_return_string + strlen(scratch_return_string),"%s%s",(*scratch_return_string) ? ", ":"",siteflags[i].name);

      if(Blank(scratch_return_string)) strcpy(scratch_return_string,"None");
      return(scratch_return_string);
}

/* ---->  Convert text form IP address to integer form (IP + mask)  <---- */
unsigned long text_to_ip(const char *ipaddress,unsigned long *mask)
{
	 unsigned long address = 0;
	 short         loop = 3;
	 int           addr;

	 *mask = 0;
	 if(Blank(ipaddress)) return(0);
	 for(; *ipaddress && (*ipaddress == ' '); ipaddress++);
	 while((loop >= 0) && *ipaddress && (*ipaddress != ' ')) {
	       if(!isdigit(*ipaddress) && (*ipaddress != '*')) {
		  *mask = 0;
		  return(0);
	       }
	       if(*ipaddress) {
		  if(((addr = atol(ipaddress)) < 0) || (addr > 255)) {
		     *mask = 0;
		     return(0);
		  }
		  address |= (addr << (loop * 8));
		  if(*ipaddress != '*') *mask |= (0xFF << (loop * 8));
	       }
	       for(; *ipaddress && !((*ipaddress == ' ') || (*ipaddress == '.')); ipaddress++);
	       for(; *ipaddress && (*ipaddress == '.'); ipaddress++);
	       loop--;
	 }
	 return(address);
}

/* ---->  Convert integer form IP address (IP + mask) into text form ('n.n.n.n')  <---- */
const char *ip_to_text(unsigned long address,unsigned long mask,char *buffer)
{
      if(mask & 0xFF000000) sprintf(buffer,"%ld.",(address & 0xFF000000) >> 24);
         else strcpy(buffer,"*.");
      if(mask & 0x00FF0000) sprintf(buffer + strlen(buffer),"%ld.",(address & 0x00FF0000) >> 16);
         else strcat(buffer,"*.");
      if(mask & 0x0000FF00) sprintf(buffer + strlen(buffer),"%ld.",(address & 0x0000FF00) >> 8);
         else strcat(buffer,"*.");
      if(mask & 0x000000FF) sprintf(buffer + strlen(buffer),"%ld",address & 0x000000FF);
         else strcat(buffer,"*");
      return(buffer);
}

/* ---->  Lookup registered Internet site (Must match IP address and mask exactly)  <---- */
struct site_data *lookup_site_details(unsigned long address,unsigned long mask,struct site_data **last)
{
       struct site_data *ptr = sitelist;

       for(*last = NULL; ptr; *last = ptr, ptr = ptr->next)
           if((ptr->addr == address) && (ptr->mask == mask))
              return(ptr);
       return(NULL);
}

/* ---->  Add new Internet site to linked list of sites  <---- */
void add_site(unsigned long address,unsigned long mask,short max_connections,char *description,unsigned long connected,unsigned short created,unsigned char flags)
{
     struct site_data *site = sitelist,*new,*last = NULL;

     for(; site && (mask < site->mask); last = site, site = site->next);
     for(; site && (address >= site->addr) && (mask == site->mask); last = site, site = site->next);

     /* ---->  Add new Internet site  <---- */
     MALLOC(new,struct site_data);
     new->max_connections = max_connections;
     new->description     = (char *) alloc_string(compress(description,1));
     new->connected       = connected;
     new->created         = created;
     new->flags           = flags;
     new->addr            = address;
     new->mask            = mask;
     new->next            = site;
     if(last) last->next = new;
        else sitelist = new;
     sites++;
}


/*  ========================================================================  */
/*    ---->          '@site' command and related routines.           <----    */
/*  ========================================================================  */


/* ---->  View details (And stats) of registered Internet site  <---- */
void site_view(dbref player,const char *ipaddress,const char *arg2)
{
     unsigned char            twidth = output_terminal_width(player);
     struct   descriptor_data *d,*p = getdsc(player);
     unsigned long            address,mask;
     short                    count = 0;
     struct   site_data       *site;

     if(!Blank(arg2)) ipaddress = arg2;
     if(!((!(address = text_to_ip(ipaddress,&mask)) && !mask) || !(site = lookup_site_details(address,mask,&site)))) {
        if(IsHtml(p)) {
	   html_anti_reverse(p,1);
           output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
	}

        if(!in_command) {
           output(p,player,2,1,0,"%sFull details of registered Internet site "ANSI_LWHITE"%s"ANSI_LCYAN"...%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER COLSPAN=2><FONT SIZE=4><I>\016"ANSI_LCYAN:"\n ",ip_to_text(address,mask,scratch_return_string),IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
           if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
	}

        /* ---->  Site details  <---- */
        if(site->max_connections != NOTHING) output(p,player,2,1,23,"%sMaximum connections:%s"ANSI_LWHITE"%d.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=55% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LGREEN:ANSI_LGREEN" ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",site->max_connections,IsHtml(p) ? "\016</TD></TR>\016":"\n");
           else output(p,player,2,1,23,"%sMaximum connections:%s"ANSI_LWHITE"N/A.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=55% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LGREEN:ANSI_LGREEN" ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",IsHtml(p) ? "\016</TD></TR>\016":"\n");
        output(p,player,2,1,23,"%sIP Address/mask:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=55% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LGREEN:ANSI_LGREEN"     ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",scratch_return_string,IsHtml(p) ? "\016</TD></TR>\016":"\n");
        output(p,player,2,1,23,"%sDescription:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=55% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LGREEN:ANSI_LGREEN"         ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",decompress(site->description),IsHtml(p) ? "\016</TD></TR>\016":"\n");
        output(p,player,2,1,23,"%sFlags:%s"ANSI_LWHITE"%s.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=55% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LGREEN:ANSI_LGREEN"               ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",siteflags_description(site->flags),IsHtml(p) ? "\016</TD></TR>\016":"\n");

        /* ---->  Site statistics  <---- */
        for(d = descriptor_list, count = 0; d; d = d->next)
            if((d->flags & CONNECTED) && d->site && (d->site == site))
               count++;
        if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','-'));
        output(p,player,2,1,0,"%sTotal characters connected from this site:%s"ANSI_LWHITE"%d.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=55% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"         ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",site->connected,IsHtml(p) ? "\016</TD></TR>\016":"\n");
        output(p,player,2,1,0,"%sTotal characters created from this site:%s"ANSI_LWHITE"%d.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=55% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"           ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",site->created,IsHtml(p) ? "\016</TD></TR>\016":"\n");
        output(p,player,2,1,0,"%sTotal characters presently on-line from this site:%s"ANSI_LWHITE"%d.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=55% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW" ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT>\016":"  ",count,IsHtml(p) ? "\016</TD></TR>\016":"\n");
        
        if(IsHtml(p)) {
           output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
	   html_anti_reverse(p,0);
	} else if(!in_command) output(p,player,0,1,0,separator(twidth,1,'-','='));
        setreturn(OK,COMMAND_SUCC);
     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, either the IP address you specified is invalid, or a site with that IP address doesn't exist.");
}

/* ---->  List registered Internet sites  <---- */
void site_list(dbref player,const char *ipaddress,const char *flags)
{
     unsigned char            twidth = output_terminal_width(player);
     unsigned char            cached_scrheight,not,matched,cr = 1;
     int                      flags_inc = 0,flags_exc = 0;
     struct   descriptor_data *d,*p = getdsc(player);
     unsigned long            address,mask;
     short                    count;
     char                     *ptr;

     /* ---->  List Internet sites which have specified site flag only?  <---- */
     while(!Blank(flags)) {
           not = 0, matched = 0;
           for(; *flags && (*flags == ' '); flags++);
           if(*flags && (*flags == '!')) not = 1, flags++;
           for(ptr = scratch_buffer; *flags && (*flags != ' '); *ptr++ = *flags, flags++);
           for(*ptr = '\0', count = 0; siteflags[count].name && !matched; count++) {
               if(string_prefix(siteflags[count].name,scratch_buffer)) {
                  if(!not) flags_inc |= siteflags[count].flag, matched = 1;
                     else flags_exc |= siteflags[count].flag, matched = 1;
	       }
	   }
           if(!matched) output(p,player,0,1,0,ANSI_LGREEN"%sSorry, '"ANSI_LWHITE"%s"ANSI_LGREEN"' is an unknown Internet site flag.",(cr) ? "\n":"",scratch_buffer), cr = 0;
     }
     if(!flags_inc) flags_inc = SEARCH_ALL;

     if(IsHtml(p)) {
        html_anti_reverse(p,1);
        output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
     } else if(p && !p->pager && Validchar(p->player) && More(p->player)) pager_init(p);

     ipaddress        = (char *) parse_grouprange(player,ipaddress,FIRST,1);
     address          = text_to_ip(ipaddress,&mask);
     cached_scrheight = db[player].data->player.scrheight;
     db[player].data->player.scrheight = ((db[player].data->player.scrheight - 8) / 2) * 2;

     if(!in_command) {
        if(IsHtml(p)) output(p,player,2,1,0,"\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH WIDTH=25%%><FONT COLOR="HTML_LCYAN" SIZE=4><I>IP Address:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Flags:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Max Con:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Connected:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>Created:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN" SIZE=4><I>On-line:</I></FONT></TH></TR>\016");
           else output(p,player,0,1,0,"\n IP Address:      Flags:      Max Con:  Connected:  Created:  On-line:");
        if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
     }

     set_conditions_ps(player,address,mask,flags_inc,flags_exc,NOTHING,NOTHING,NOTHING,(!address && !mask) ? (!Blank(ipaddress)) ? ipaddress:NULL:NULL,503);
     union_initgrouprange((union group_data *) sitelist);
     while(union_grouprange()) {
           for(d = descriptor_list, count = 0; d; d = d->next)
               if((d->flags & CONNECTED) && d->site && (d->site == &grp->cunion->site))
                  count++;
           if(grp->cunion->site.max_connections != NOTHING) sprintf(cmpbuf,ANSI_LWHITE"%d",grp->cunion->site.max_connections);
              else sprintf(cmpbuf,"%sN/A"ANSI_LWHITE,IsHtml(p) ? ANSI_DCYAN:"");
           output(p,player,2,1,0,"%s%-17s%s"ANSI_DCYAN"["ANSI_LCYAN"%c%c%c%c%c%c%c%c"ANSI_DCYAN"]%s%-21s%s%-12d%s%-10d%s%d%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_BLUE"><TD BGCOLOR="HTML_TABLE_YELLOW">\016"ANSI_LYELLOW:ANSI_LYELLOW" ",ip_to_text(grp->cunion->site.addr,grp->cunion->site.mask,scratch_return_string),IsHtml(p) ? "\016</TD><TD><TT>\016":"",
                 (grp->cunion->site.flags & SITE_ADMIN)         ? 'A':'-',
                 (grp->cunion->site.flags & SITE_BANNED)        ? 'B':'-',
                 (grp->cunion->site.flags & SITE_CREATE)        ? 'C':'-',
                 (grp->cunion->site.flags & SITE_DNS)           ? 'D':'-',
                 (grp->cunion->site.flags & SITE_GUESTS)        ? 'G':'-',
                 (grp->cunion->site.flags & SITE_NODNS)         ? 'N':'-',
                 (grp->cunion->site.flags & SITE_READONLY)      ? 'R':'-',
                 (grp->cunion->site.flags & SITE_UNCONDITIONAL) ? 'U':'-',
                 IsHtml(p) ? "\016</TT></TD><TD>\016":"  ",cmpbuf,IsHtml(p) ? "\016</TD><TD>\016":"",grp->cunion->site.connected,IsHtml(p) ? "\016</TD><TD>\016":"",grp->cunion->site.created,IsHtml(p) ? "\016</TD><TD>\016":"",count,IsHtml(p) ? "\016</TD></TR>\016":"\n");
           output(p,player,2,1,9,"%s`---> \016&nbsp;\016 "ANSI_LGREEN"%s.%s",IsHtml(p) ? "\016<TR><TD ALIGN=LEFT COLSPAN=6>&nbsp;\016"ANSI_LCYAN:ANSI_DCYAN"  ",decompress(grp->cunion->site.description),IsHtml(p) ? "\016</TD></TR>\016":"\n");
     }

     if(grp->rangeitems == 0) output(p,player,2,1,0,IsHtml(p) ? "\016<TR ALIGN=CENTER><TD COLSPAN=6>"ANSI_LCYAN"<I>*** &nbsp; NO REGISTERED INTERNET SITES FOUND &nbsp; ***</I></TD></TR>\016":" ***  NO REGISTERED INTERNET SITES FOUND  ***\n");
     if(!in_command) {
        if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
        if(grp->rangeitems != 0) output(p,player,2,1,0,"%sRegistered Internet sites found: \016&nbsp;\016 "ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><TD COLSPAN=6>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",listed_items(scratch_return_string,1),IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
	   else output(p,player,2,1,0,"%sRegistered Internet sites found: \016&nbsp;\016 "ANSI_DWHITE"None.%s",IsHtml(p) ? "\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_GREY"><TD COLSPAN=6>"ANSI_LWHITE"<B>\016":ANSI_LWHITE" ",IsHtml(p) ? "\016</B></TD></TR>\016":"\n\n");
     }

     db[player].data->player.scrheight = cached_scrheight;
     if(IsHtml(p)) {
        output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
        html_anti_reverse(p,0);
     }
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  View statistics of registered Internet site  <---- */
void site_stats(dbref player)
{
     int                      unconditional = 0,create = 0,admin = 0,readonly = 0,banned = 0,nodns = 0,dns = 0,noguests = 0,total = 0;
     unsigned char            twidth = output_terminal_width(player);
     struct   descriptor_data *p = getdsc(player);
     struct   site_data       *site;

     /* ---->  Gather site statistics  <---- */
     for(site = sitelist; site; site = site->next) {
         if(site->flags   & SITE_UNCONDITIONAL) unconditional++;
         if(!(site->flags & SITE_GUESTS))       noguests++;
         if(site->flags   & SITE_READONLY)      readonly++;
         if(!(site->flags & SITE_CREATE))       create++;
         if(site->flags   & SITE_BANNED)        banned++;
         if(!(site->flags & SITE_ADMIN))        admin++;
         if(site->flags   & SITE_NODNS)         nodns++;
         if(site->flags   & SITE_DNS)           dns++;
         total++;
     }

     /* ---->  Display site statistics  <---- */
     if(IsHtml(p)) {
        html_anti_reverse(p,1);
        output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
     }

     if(!in_command) {
        output(p,player,2,1,1,"%sThere %s "ANSI_LWHITE"%d"ANSI_LCYAN" registered Internet site%s...%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER COLSPAN=3><FONT SIZE=4><I>\016"ANSI_LCYAN:"\n ",(total == 1) ? "is":"are",total,Plural(total),IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
        if(!IsHtml(p)) output(p,player,0,1,1,separator(twidth,0,'-','='));
     }

     /* ---->  Site details  <---- */
     output(p,player,2,1,0,"%sUnconditional:%s"ANSI_LWHITE"%-12d%s"ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW" ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",unconditional,IsHtml(p) ? "\016</TD><TD WIDTH=33% BGCOLOR="HTML_TABLE_DGREY">\016":"",stats_percent(unconditional,total),IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,0,"%sCreate banned:%s"ANSI_LWHITE"%-12d%s"ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW" ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",create,IsHtml(p) ? "\016</TD><TD WIDTH=33% BGCOLOR="HTML_TABLE_DGREY">\016":"",stats_percent(create,total),IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,0,"%sGuests banned:%s"ANSI_LWHITE"%-12d%s"ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW" ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",noguests,IsHtml(p) ? "\016</TD><TD WIDTH=33% BGCOLOR="HTML_TABLE_DGREY">\016":"",stats_percent(noguests,total),IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,0,"%sAdmin banned:%s"ANSI_LWHITE"%-12d%s"ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"  ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",admin,IsHtml(p) ? "\016</TD><TD WIDTH=33% BGCOLOR="HTML_TABLE_DGREY">\016":"",stats_percent(admin,total),IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,0,"%sRead-Only:%s"ANSI_LWHITE"%-12d%s"ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"     ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",readonly,IsHtml(p) ? "\016</TD><TD WIDTH=33% BGCOLOR="HTML_TABLE_DGREY">\016":"",stats_percent(readonly,total),IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,0,"%sBanned:%s"ANSI_LWHITE"%-12d%s"ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"        ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",banned,IsHtml(p) ? "\016</TD><TD WIDTH=33% BGCOLOR="HTML_TABLE_DGREY">\016":"",stats_percent(banned,total),IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,0,"%sNoDNS:%s"ANSI_LWHITE"%-12d%s"ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"         ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",nodns,IsHtml(p) ? "\016</TD><TD WIDTH=33% BGCOLOR="HTML_TABLE_DGREY">\016":"",stats_percent(nodns,total),IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,0,"%sDNS:%s"ANSI_LWHITE"%-12d%s"ANSI_DWHITE"%s%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=33% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"           ",IsHtml(p) ? "\016</I></TH><TD WIDTH=33%>\016":"  ",dns,IsHtml(p) ? "\016</TD><TD WIDTH=33% BGCOLOR="HTML_TABLE_DGREY">\016":"",stats_percent(dns,total),IsHtml(p) ? "\016</TD></TR>\016":"\n");

     if(IsHtml(p)) {
        output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
        html_anti_reverse(p,0);
     } else if(!in_command) output(p,player,0,1,0,separator(twidth,1,'-','='));
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Add new Internet site  <---- */
void site_add(dbref player,char *ipaddress,char *desc)
{
     unsigned long      address,mask;
     struct   site_data *site;
     char               *ptr;

     /* ---->  Grab first word as IP address  <---- */
     if(Blank(desc)) {
        for(; *ipaddress && (*ipaddress == ' '); ipaddress++);
        for(ptr = ipaddress; *ptr && (*ptr != ' '); ptr++);
        if(*ptr) for(*ptr = '\0', ptr++; *ptr && (*ptr == ' '); ptr++);
        desc = ptr;
     }

     if(!((dumpstatus >= 96) && (dumpstatus <= 99))) {
        if(!(!(address = text_to_ip(ipaddress,&mask)) && !mask)) {
           if(!lookup_site_details(address,mask,&site)) {
              if(!Blank(desc)) {
                 if(!Censor(player) && !Censor(db[player].location)) bad_language_filter(desc,desc);
                 add_site(address,mask,NOTHING,ptr = punctuate(desc,2,'\0'),0,0,SITE_ADMIN|SITE_CREATE|SITE_GUESTS);
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Internet site '"ANSI_LWHITE"%s"ANSI_LGREEN"' added with description '"ANSI_LYELLOW"%s"ANSI_LGREEN"'.",ip_to_text(address,mask,scratch_return_string),ptr);
                 writelog(SITE_LOG,1,"SITE","%s(#%d) added Internet site '%s' with description '%s'.",getname(player),player,scratch_return_string,ptr);
                 setreturn(OK,COMMAND_SUCC);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify a description for this new site in the form of '"ANSI_LWHITE"<UNIVERSITY/COMPANY/PROVIDER/BBS>, <TOWN/CITY>, <STATE/PROVINCE/COUNTY>, <COUNTRY>"ANSI_LGREEN"'.");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, an Internet site with that IP address already exists.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that IP address is invalid.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't add a new Internet site while the registered Internet sites database is being dumped to disk  -  Please wait for a couple of minutes and then try again.");
}

/* ---->  Process given site flag name and then set/reset it on given Internet site  <---- */
unsigned char process_siteflag(dbref player,struct site_data *site,const char *flagname,unsigned char reset,const char *reason)
{
	 short         flag = 0;
	 unsigned char pos;

	 /* ---->  Look up flag in site flags list  <---- */
	 pos = 0;
	 while(!flag && siteflags[pos].name)
	       if(string_prefix(siteflags[pos].name,flagname)) flag = siteflags[pos].flag;
		  else pos++;

	 if(flag) {
	    if(!((flag != SITE_READONLY) && (site->flags & SITE_READONLY))) {
	       if(privilege(player,4) <= siteflags[pos].level) {
		  if(!(Blank(reason) && (flag & (SITE_ADMIN|SITE_BANNED|SITE_CREATE|SITE_GUESTS|SITE_UNCONDITIONAL)))) {

		     /* ---->  Set/reset Internet site flag on Internet site  <---- */
		     if(reset) site->flags &= ~flag;
			else site->flags   |=  flag;
		     ip_to_text(site->addr,site->mask,cmpbuf);
		     output(getdsc(player),player,0,1,0,ANSI_LYELLOW"%s"ANSI_LGREEN" site flag %s on Internet site '"ANSI_LWHITE"%s"ANSI_LGREEN"'  "ANSI_LCYAN"--->  "ANSI_LWHITE"%s"ANSI_LGREEN".",siteflags[pos].name,(reset) ? "reset":"set",cmpbuf,siteflags_description(site->flags));
		     writelog(SITE_LOG,1,"SITE","%s(#%d) %s %s site flag on Internet site '%s'%s%s",getname(player),player,(reset) ? "reset":"set",siteflags[pos].name,cmpbuf,(Blank(reason)) ? "":"  -  REASON:  ",(Blank(reason)) ? ".":reason);
		     return(1);
		  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the reason(s) for %sting this Internet site's "ANSI_LYELLOW"%s"ANSI_LGREEN" site flag.",(reset) ? "reset":"set",siteflags[pos].name);
	       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only %s may %s that site flag.",clevels[siteflags[pos].level],(reset) ? "reset":"set");
	    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the Internet site '"ANSI_LWHITE"%s"ANSI_LGREEN"' is Read-Only  -  You can't %s site flags on it.",ip_to_text(site->addr,site->mask,scratch_return_string),(reset) ? "reset":"set");
	 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"%s"ANSI_LGREEN"' is an unknown site flag.",flagname);
	 return(0);
}

/* ---->  Set/reset Internet site flags on registered Internet site  <---- */
void site_set(dbref player,char *ipaddress,const char *params)
{
     char               *p1,*flaglist,*reason;
     unsigned char      failed = 0,reset;
     unsigned long      address,mask;
     struct   site_data *site;
    
     split_params((char *) params,&flaglist,&reason);
     if(!Blank(reason)) reason = punctuate(reason,2,'.');
     if(!((!(address = text_to_ip(ipaddress,&mask)) && !mask) || !(site = lookup_site_details(address,mask,&site)))) {
        if(!Blank(flaglist)) {

           /* ---->  Set/reset flag(s) on specified Internet site  <---- */
           while(*flaglist) {
                 while(*flaglist && (*flaglist == ' ')) flaglist++;
                 if(*flaglist && (*flaglist == '!')) flaglist++, reset = 1;
		    else reset = 0;

                 for(p1 = scratch_buffer; *flaglist && (*flaglist != ' '); *p1++ = *flaglist, flaglist++);
                 *p1 = '\0';

                 if(!Blank(scratch_buffer)) failed |= process_siteflag(player,site,scratch_buffer,reset,reason);
                    else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify which Internet site flag you'd like to reset.");
	   }
           if(!failed) setreturn(OK,COMMAND_SUCC);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify which site flag(s) you'd like to set/reset on the Internet site '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",ip_to_text(address,mask,scratch_return_string));
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, either the IP address you specified is invalid, or a site with that IP address doesn't exist.");
}

/* ---->  Change/set description of Internet site  <---- */
void site_desc(dbref player,char *ipaddress,char *desc)
{
     unsigned long      address,mask;
     struct   site_data *site;
     char               *ptr;

     /* ---->  Grab first word as IP address  <---- */
     if(Blank(desc)) {
        for(; *ipaddress && (*ipaddress == ' '); ipaddress++);
        for(ptr = ipaddress; *ptr && (*ptr != ' '); ptr++);
        if(*ptr) for(*ptr = '\0', ptr++; *ptr && (*ptr == ' '); ptr++);
        desc = ptr;
     }

     if(!((!(address = text_to_ip(ipaddress,&mask)) && !mask) || !(site = lookup_site_details(address,mask,&site)))) {
        if(!(site->flags & SITE_READONLY)) {
           if(!Blank(desc)) {
              FREENULL(site->description);
              if(!Censor(player) && !Censor(db[player].location)) bad_language_filter(desc,desc);
              site->description = (char *) alloc_string(compress(ptr = punctuate(desc,2,'\0'),0));
              ip_to_text(address,mask,scratch_return_string);
              writelog(SITE_LOG,1,"SITE","%s(#%d) changed Internet site '%s's description to '%s'.",getname(player),player,scratch_return_string,ptr);
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Internet site '"ANSI_LWHITE"%s"ANSI_LGREEN"'s description changed to '"ANSI_LYELLOW"%s"ANSI_LGREEN"'.",scratch_return_string,ptr);
              setreturn(OK,COMMAND_SUCC);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify a description for this site in the form of '"ANSI_LWHITE"<UNIVERSITY/COMPANY/PROVIDER>, <TOWN/CITY>, <STATE/PROVINCE/COUNTY>, <COUNTRY>"ANSI_LGREEN"'.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that Internet site is Read-Only  -  You can't change its description.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, either the IP address you specified is invalid, or a site with that IP address doesn't exist.");
}

/* ---->  Change/set maximum connections allowed (Simultaneously) from Internet site  <---- */
void site_connections(dbref player,char *ipaddress,const char *params)
{
     char               *maxc,*reason;
     unsigned long      address,mask;
     struct   site_data *site;

     split_params((char *) params,&maxc,&reason);
     if(!Blank(reason)) reason = punctuate(reason,2,'.');
     if(!((!(address = text_to_ip(ipaddress,&mask)) && !mask) || !(site = lookup_site_details(address,mask,&site)))) {
        if(Level2(player)) {
           if(!(site->flags & SITE_READONLY)) {
              if(!Blank(reason)) {
                 if(Blank(maxc) || string_prefix("unlimited",maxc) || string_prefix("none",maxc) || string_prefix("N/A",maxc) || string_prefix("U/L",maxc)) {
                    site->max_connections = NOTHING;
                    ip_to_text(address,mask,scratch_return_string);
                    writelog(SITE_LOG,1,"SITE","%s(#%d) changed Internet site '%s's maximum connections allowed to unlimited  -  REASON:  %s",getname(player),player,scratch_return_string,reason);
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Maximum connections allowed (Simultaneously) from Internet site '"ANSI_LWHITE"%s"ANSI_LGREEN"' are now "ANSI_LYELLOW"unlimited"ANSI_LGREEN".",scratch_return_string);
                    setreturn(OK,COMMAND_SUCC);
		 } else {
                    if((site->max_connections = atol(maxc)) < 0) site->max_connections = 0;
                    ip_to_text(address,mask,scratch_return_string);
                    writelog(SITE_LOG,1,"SITE","%s(#%d) changed Internet site '%s's maximum connections allowed to %d  -  REASON:  %s",getname(player),player,scratch_return_string,site->max_connections,reason);
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Maximum connections allowed (Simultaneously) from Internet site '"ANSI_LWHITE"%s"ANSI_LGREEN"' changed to "ANSI_LYELLOW"%d"ANSI_LGREEN".",scratch_return_string,site->max_connections);
                    setreturn(OK,COMMAND_SUCC);
		 }
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the reason(s) for modifying the maximum number of connections allowed (Simultaneously) from this site.");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that Internet site is Read-Only  -  You can't change the maximum number of connections allowed from it (Simultaneously.)");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Elder Wizards/Druids and above may change the maximum number of connections allowed (Simultaneously) from an Internet site.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, either the IP address you specified is invalid, or a site with that IP address doesn't exist.");
}

/* ---->  Change IP address of Internet site  <---- */
void site_address(dbref player,char *ipaddress,char *newaddr)
{
     struct   site_data *site,*ptr,*last = NULL;
     unsigned long      address,mask;
     char               *p1;

     /* ---->  Grab first word as IP address  <---- */
     if(Blank(newaddr)) {
        for(; *ipaddress && (*ipaddress == ' '); ipaddress++);
        for(p1 = ipaddress; *p1 && (*p1 != ' '); p1++);
        if(*p1) for(*p1 = '\0', p1++; *p1 && (*p1 == ' '); p1++);
        newaddr = p1;
     }

     if(!((dumpstatus >= 96) && (dumpstatus <= 99))) {
        if(!((!(address = text_to_ip(ipaddress,&mask)) && !mask) || !(site = lookup_site_details(address,mask,&last)))) {
           if(!((!(address = text_to_ip(newaddr,&mask)) && !mask) || (ptr = lookup_site_details(address,mask,&ptr)))) {
              if(!(site->flags & SITE_READONLY)) {
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Internet site '"ANSI_LWHITE"%s"ANSI_LGREEN"'s IP address changed to '"ANSI_LYELLOW"%s"ANSI_LGREEN"'.",ip_to_text(site->addr,site->mask,scratch_return_string),ip_to_text(address,mask,scratch_return_string + 200));
                 writelog(SITE_LOG,1,"SITE","%s(#%d) changed Internet site '%s's IP address to '%s'.",getname(player),player,scratch_return_string,scratch_return_string + 200);

                 if(last) last->next = site->next;
                    else sitelist = site->next;
                 for(ptr = sitelist, last = NULL; ptr && (mask < ptr->mask); last = ptr, ptr = ptr->next);
                 for(; ptr && (address >= ptr->addr) && (mask == ptr->mask); last = ptr, ptr = ptr->next);

                 site->addr = address;
                 site->mask = mask;
                 site->next = ptr;
                 if(last) last->next = site;
                    else sitelist = site;
                 setreturn(OK,COMMAND_SUCC);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that Internet site is Read-Only  -  You can't delete it.");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, either the new IP address you specified is invalid, or a site with that IP address already exists.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, either the IP address you specified is invalid, or a site with that IP address doesn't exist.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't change the IP address of an Internet site while the registered Internet sites database is being dumped to disk  -  Please wait for a couple of minutes and then try again.");
}

/* ---->  Delete registered Internet site  <---- */
void site_delete(dbref player,const char *ipaddress,const char *arg2)
{
     unsigned long      address,mask;
     struct   site_data *site,*last;

     if(!Blank(arg2)) ipaddress = arg2;
     if(!((dumpstatus >= 96) && (dumpstatus <= 99))) {
        if(Level3(player)) {
           if(!((!(address = text_to_ip(ipaddress,&mask)) && !mask) || !(site = lookup_site_details(address,mask,&last)))) {
              if(!(site->flags & SITE_READONLY)) {
                 ip_to_text(address,mask,scratch_return_string);
                 writelog(SITE_LOG,1,"SITE","%s(#%d) deleted Internet site '%s'.",getname(player),player,scratch_return_string);
                 output(getdsc(player),player,0,1,0,ANSI_LGREEN"Internet site '"ANSI_LWHITE"%s"ANSI_LGREEN"' deleted.",scratch_return_string);
                 if(last) last->next = site->next;
                    else sitelist = site->next;
                 FREENULL(site->description);
                 FREENULL(site);
                 setreturn(OK,COMMAND_SUCC);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, that Internet site is Read-Only  -  You can't delete it.");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, either the IP address you specified is invalid, or a site with that IP address doesn't exist.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Wizards/Druids and above may delete Internet sites.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't delete an Internet site while the registered Internet sites database is being dumped to disk  -  Please wait for a couple of minutes and then try again.");
}

/* ---->  View, add, modify or delete registered Internet sites  <---- */
void site_process(CONTEXT)
{
     char  command[32];
     short count = 0;
     const char *p1;
     char  *ptr;

     setreturn(ERROR,COMMAND_FAIL);
     if(!in_command || Wizard(current_command)) {
        if(Level4(db[player].owner)) {

           /* ---->  Site command  <---- */
           for(; *arg1 && (*arg1 == ' '); arg1++);
           for(ptr = command, p1 = arg1; *p1 && (*p1 != ' ') && (count < 31); command[count] = *p1, count++, p1++);
           command[count] = '\0';
           for(; *p1 && (*p1 != ' '); p1++);
           for(; *p1 && (*p1 == ' '); p1++);
           if(!Blank(command)) {
	      if(string_prefix("addsite",command)) {
                 site_add(player,(char *) p1,(char *) arg2);
	      } else if(string_prefix("connections",command) || string_prefix("maxconnections",command) || string_prefix("maximumconnections",command)) {
                 site_connections(player,(char *) p1,arg2);
	      } else if(string_prefix("descsite",command) || string_prefix("describesite",command)) {
                 site_desc(player,(char *) p1,(char *) arg2);
	      } else if(string_prefix("listsites",command)) {
                 site_list(player,p1,arg2);
	      } else if(string_prefix("stats",command) || string_prefix("statistics",command) || string_prefix("sitestats",command) || string_prefix("sitestatistics",command)) {
                 site_stats(player);
	      } else if(string_prefix("setsite",command)) {
                 site_set(player,(char *) p1,arg2);
	      } else if(string_prefix("ipaddress",command) || string_prefix("address",command)) {
                 site_address(player,(char *) p1,(char *) arg2);
	      } else if((strlen(command) > 3) && (string_prefix("deletesite",command) || string_prefix("erasesite",command) || string_prefix("removesite",command))) {
                 site_delete(player,p1,arg2);
	      } else {
                 if(!string_prefix("viewsite",command)) p1 = arg1;
                 site_view(player,p1,arg2);
	      }
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify a site command or the IP address of the site you'd like to view the details of.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may view, add, modify or delete registered Internet sites.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, registered Internet sites can't be viewed, added, modified or deleted from within a compound command.");
}


/*  ========================================================================  */
/*    ---->      Miscellaneous site routines (Inc. read/write)       <----    */
/*  ========================================================================  */


/* ---->  Parse Internet site details from given string  <---- */
unsigned char parse_sitedetails(const char *str,short *revision)
{
	 char              buffer[BUFFER_LEN];
         static const char *site_date = NULL;
         static const char *serial_no = NULL;
	 char              *ptr;
	 struct site_data  temp;

	 if(Blank(str)) return(0);
	 while(*str && ((*str == ' ') || (*str == '\n'))) str++;
	 if(*str == '#') {
	    for(; *str && (*str == '#'); str++);
	    for(; *str && (*str == ' '); str++);
	    if(!strncasecmp(str,"Revision:",9)) {
   	       if(*revision == 1) {
	          for(; *str && !isdigit(*str); str++);
		  if(*str) *revision = atol(str);
	       }
	    } else if(!strncasecmp(str,"Serial:",7)) {
	       for(; *str && (*str != ':'); str++);
               for(; *str && (*str == ':'); str++);
               for(; *str && (*str == ' '); str++);
               if(*str && (ptr = strchr(str,'\n'))) *ptr = '\0';
               if(!Blank(str)) serial_no = alloc_string(str);
	    } else if(!strncasecmp(str,"Date:",5)) {
	       for(; *str && (*str != ':'); str++);
               for(; *str && (*str == ':'); str++);
               for(; *str && (*str == ' '); str++);
               if(*str && (ptr = strchr(str,'\n'))) *ptr = '\0';
               if(!Blank(str)) site_date = alloc_string(str);
	    }

	    if(!Blank(site_date) && !Blank(serial_no)) {
	       writelog(SERVER_LOG,0,"RESTART","Site database serial number %s, dumped on %s.",serial_no,site_date);
               FREENULL(site_date);
               FREENULL(serial_no);
	    }
            return(0);
	 }

	 if(!*str || (*str && (*str == '#'))) return(0);
         if(site_date || serial_no) {
            FREENULL(site_date);
            FREENULL(serial_no);
	 }

	 temp.max_connections = NOTHING;
	 temp.connected       = 0;
	 temp.created         = 0;
	 temp.flags           = (*revision < 2) ? SITE_GUESTS:0;
	 temp.addr            = 0;
	 temp.mask            = 0;
	 *buffer              = '\0';

	 /* ---->  IP address  <---- */
	 for(ptr = buffer; *str && (*str != ' '); *ptr++ = *str, str++);
	 *ptr = '\0';
	 temp.addr = text_to_ip(buffer,&temp.mask);
	 if(!temp.addr && !temp.mask) return(0);
	 for(; *str && (*str == ' '); str++);

	 /* ---->  Site flags  <---- */
	 for(; *str && (*str != ' '); str++)
	     switch(toupper(*str)) {
		    case 'A':

			 /* ---->  Admin. connections allowed from site?  <---- */
			 temp.flags |= SITE_ADMIN;
			 break;
		    case 'B':
   
			 /* ---->  Site banned?  <---- */
			 temp.flags |= SITE_BANNED;
			 break;
		    case 'C':

			 /* ---->  Character creation allowed from site?  <---- */
			 temp.flags |= SITE_CREATE;
			 break;
		    case 'D':

			 /* ---->  Machine names from site will be looked up on name server?  <---- */
			 temp.flags |= SITE_DNS;
			 break;
		    case 'G':

			 /* ---->  Guest characters are allowed to connect from site?  <---- */
			 temp.flags |= SITE_GUESTS;
			 break;
		    case 'N':

			 /* ---->  Machine names from site WILL NOT be looked up on name server?  <---- */
			 temp.flags |= SITE_NODNS;
			 break;
		    case 'R':

			 /* ---->  Site ReadOnly?  <---- */
			 temp.flags |= SITE_READONLY;
			 break;
		    case 'U':

			 /* ---->  Unconditional site ban/max. connection restriction?  <---- */
			 temp.flags |= SITE_UNCONDITIONAL;
			 break;
		    case '-':
		    default:
			 break;
	     }
	 for(; *str && (*str == ' '); str++);

	 /* ---->  Maximum connections allowed from site (Simultaneously)  <---- */
	 for(ptr = buffer; *str && (*str != ' '); *ptr++ = *str, str++);
	 *ptr = '\0';
	 if(*buffer && !((*buffer == '-') || (*buffer == '*'))) temp.max_connections = atol(buffer);
	    else temp.max_connections = NOTHING;  /*  Unrestricted  */
	 for(; *str && (*str == ' '); str++);

	 /* ---->  Total number of characters connected from site  <---- */
	 if(isdigit(*str)) {
	    for(ptr = buffer; *str && (*str != ' '); *ptr++ = *str, str++);
	    *ptr = '\0';
	    temp.connected = atol(buffer);
	 }
	 for(; *str && (*str == ' '); str++);

	 /* ---->  Total number of characters created from site  <---- */
	 if(isdigit(*str)) {
	    for(ptr = buffer; *str && (*str != ' '); *ptr++ = *str, str++);
	    *ptr = '\0';
	    temp.created = atol(buffer);
	 }
	 for(; *str && (*str == ' '); str++);

	 /* ---->  Site's description  <---- */
	 for(ptr = buffer; *str && (*str != '\n'); *ptr++ = *str, str++);
	 *ptr = '\0';
	 bad_language_filter(buffer,buffer);
	 strcpy(buffer,punctuate(buffer,2,'\0'));
	 add_site(temp.addr,temp.mask,temp.max_connections,buffer,temp.connected,temp.created,temp.flags);
	 return(1);
}

/* ---->  Register Internet sites from 'lib/sites.tcz'  <---- */
unsigned char register_sites()
{
	 char          buffer[BUFFER_LEN];
	 short         revision = 1;
	 unsigned char compressed;
	 FILE          *f;

	 if((f = popen(DB_DECOMPRESS" < "SITE_FILE""DB_EXTENSION,"r")) == NULL) {
	    if((f = fopen(SITE_FILE,"r")) == NULL) {
	       writelog(SERVER_LOG,0,"RESTART","Unable to access registered Internet sites file '%s'  -  No Internet sites registered.",SITE_FILE);
	       return(0);
	    } else compressed = 0;
	 } else compressed = 1;

	 writelog(SERVER_LOG,0,"RESTART","Registering Internet sites from the %sfile '"SITE_FILE"%s'...",(compressed) ? "compressed ":"",(compressed) ? DB_EXTENSION:"");
	 while(fgets(buffer,BUFFER_LEN,f)) parse_sitedetails(buffer,&revision);
	 writelog(SERVER_LOG,0,"RESTART","  %d Internet site%s registered.",sites,Plural(sites));
	 if(compressed) pclose(f);
	    else fclose(f);
	 return(compressed);
}

/* ---->  Unparse address of site  <---- */
void unparse_siteaddress(unsigned long addr,struct hostent *he,struct site_data *siteptr,char *buffer)
{
     if(he && !Blank(he->h_name)) {
        strcpy(buffer,he->h_name);
        sprintf(buffer + strlen(buffer)," (%ld.%ld.%ld.%ld)",(addr >> 24) & 0xff,(addr >> 16) & 0xff,(addr >> 8) & 0xff,addr & 0xff);
     } else sprintf(buffer,"%ld.%ld.%ld.%ld",(addr >> 24) & 0xff,(addr >> 16) & 0xff,(addr >> 8) & 0xff,addr & 0xff);

     if(siteptr && !Blank(siteptr->description))
        sprintf(buffer + strlen(buffer)," (%s)",decompress(siteptr->description));
}

/* ---->  Look up given site address in linked list of registered sites and return its host name/Internet address  <---- */
struct site_data *lookup_site(struct in_addr *a,char *buffer)
{
       struct   site_data *siteptr = sitelist;
       struct   hostent   *he      = NULL;
       unsigned long      addr;

       if(a) addr = ntohl(a->s_addr);
          else return(NULL);
       while(siteptr && ((addr & siteptr->mask) != siteptr->addr)) siteptr = siteptr->next;

       if(option_dns(OPTSTATUS)) {
          if(!siteptr || !(siteptr->flags & SITE_NODNS))
             he = gethostbyaddr((char *) a,sizeof(*a),AF_INET);
       } else {
          if(siteptr && (siteptr->flags & SITE_DNS) && !(siteptr->flags & SITE_NODNS))
             he = gethostbyaddr((char *) a,sizeof(*a),AF_INET);
       }
       unparse_siteaddress(addr,he,siteptr,buffer);
       return(siteptr);
}
