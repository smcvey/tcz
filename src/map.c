/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| MAP.C  -  Implements hard-coded map of TCZ ('map' command) and              |
|           '{@?colourmap}' query command.                                    |
|                                                                             |
|           lib/map.tcz         Plain-text ASCII map file.                    |
|           lib/colourmap.tcz   Colour mapping file (See 'help @?colourmap'   |
|                               for format)  -  Defines ANSI colours for      |
|                               ASCII map.                                    |
|                                                                             |
|           NOTE:  HTML Interface users will see graphical map instead of     |
|                  text-based map (See TCZMAP_IMG & TCZMAP_PATH in config.h)  |
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
| Module originally designed and written by:  J.P.Boggis 27/01/2000.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: map.c,v 1.1.1.1 2004/12/02 17:41:54 jpboggis Exp $

*/


#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "object_types.h"
#include "flagset.h"


char  *map_text   = NULL;  /*  Map ASCII                   */
char  *map_colour = NULL;  /*  Map colourmap               */
int   map_width   = 0;     /*  Width of map in characters  */
int   map_height  = 0;     /*  Height of map in lines      */


/* ---->  Colour map conversion table  <---- */
#include "colourmap.h"


/* ---->  Takes line of ASCII text and line of colour mapping codes and  <---- */
/*        converts to fully ANSI coloured text-string.                         */

/*        text    = ASCII text string.  */
/*        colours = Colour map for ASCII text string.  */
/*        repeat  = If end of colour map is reached, repeat from start  */
const char *map_colourmap(const char *text,char *colours,int repeat,char *buffer,int limit)
{
      char          last  = ' ', lastc = ' ';
      char          *cptr = colours;
      int           len   = 0,loop;
      unsigned char skip  = 0;
      char          *bptr;
      const    char *ptr;

      if(!buffer || !text || !colours) return(NULL);
      bptr = buffer;

      for(; *text && (len < limit); text++) {

          /* ---->  Colour map  <---- */
          if(!*cptr && repeat) cptr = colours;
          if(*cptr) {
             switch(*cptr) {
                    case '#':

                         /* ---->  Comment  <---- */
                         *bptr++ = ' ';
                         skip = 1, len++;
                         break;
                    case '!':

                         /* ---->  No repeat ({@?colourmap})  <---- */
                         if(repeat) {
                            for(; *cptr; cptr++);
                            repeat = 0;
			 }
                         break;
                    default:

                         /* ---->  Lookup colour map code  <---- */
                         if((*cptr == 'd') || (*cptr == 'D'))
                            *cptr = tolower(lastc);

                         if((*cptr == 'l') || (*cptr == 'L'))
                            *cptr = toupper(lastc);

	                 if(*cptr != last) {
                            for(loop = 0; (colourmap[loop].map != '\0') && (colourmap[loop].map != *cptr); loop++);
                            if((colourmap[loop].map == *cptr) && !Blank(colourmap[loop].subst)) {
                               for(ptr = colourmap[loop].subst; *ptr && (len < limit); *bptr++ = *ptr++, len++);
                               if(colourmap[loop].colour) lastc = *cptr;
			    }
                            last = *cptr;
			 }
	     }
             if(*cptr) cptr++;
	  }

          /* ---->  Text character  <---- */
          if(!skip) {
             *bptr++ = *text;
             len++;
	  } else skip = 0;
      }

      *bptr = '\0';
      return(buffer);
}

/* ---->  (Re)load map source files from lib directory  <---- */
int map_reload(dbref player,int reload)
{
    FREENULL(map_text);
    FREENULL(map_colour);
    map_width  = 0;
    map_height = 0;

    /* ---->  Attempt to load ASCII map  <---- */
    if(!(map_text = help_reload_text(MAP_FILE,0,0,0))) {
       if(reload && Validchar(player))
          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, unable to %sload %s map from the file '"ANSI_LWHITE""MAP_FILE""ANSI_LGREEN"' ("ANSI_LYELLOW"%s"ANSI_LGREEN".)",(reload) ? "re":"",tcz_short_name,strerror(errno));
       if(!reload) writelog(SERVER_LOG,0,"RESTART","Unable to load %s map from the file '"MAP_FILE"' (%s.)",tcz_short_name,strerror(errno));
       return(0);
    } else {

       /* ---->  Work out width and height of map  <---- */
       const char *ptr = map_text;
       int   width = 0;

       if(*ptr) map_height = 1;
       for(; *ptr; ptr++)
           if(*ptr == '\n') {
              map_height++;
              if(width > map_width)
                 map_width = width;
              width = 0;
	   } else width++;
    }

    /* ---->  Attempt to load colour map file  <---- */
    if(!(map_colour = help_reload_text(COLOURMAP_FILE,0,0,0))) {
       if(reload && Validchar(player))
          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, unable to %sload %s colour map from the file '"ANSI_LWHITE""COLOURMAP_FILE""ANSI_LGREEN"' ("ANSI_LYELLOW"%s"ANSI_LGREEN".)",(reload) ? "re":"",tcz_short_name,strerror(errno));
       if(!reload) writelog(SERVER_LOG,0,"RESTART","Unable to load %s colour map from the file '"COLOURMAP_FILE"' (%s.)",tcz_short_name,strerror(errno));
    }

    if(reload && Validchar(player))
       output(getdsc(player),player,0,1,0,ANSI_LWHITE"%s Map"ANSI_LGREEN" has been reloaded.",tcz_full_name);
    if(!reload) writelog(SERVER_LOG,0,"RESTART","%s map loaded from '"MAP_FILE"'/'"COLOURMAP_FILE"'.",tcz_short_name);
    return(1);
}

/* ---->  HTML map  <---- */
void map_html(struct descriptor_data *p)
{
     output(p,NOTHING,1,0,0,"<BR><CENTER><TABLE BORDER CELLPADDING=0 BGCOLOR="HTML_TABLE_WHITE">");
     output(p,NOTHING,1,0,0,"<TR ALIGN=CENTER><TH BGCOLOR="HTML_TABLE_CYAN"><FONT SIZE=6 COLOR=#00DDFF><B><I>%s Map</I></B></FONT></TH></TR>",tcz_full_name);
     output(p,NOTHING,1,0,0,"<TR ALIGN=CENTER><TD><A HREF=\"%s/"TCZMAP_PATH"\" TARGET=_blank><IMG SRC=\"%s\" BORDER=0 ALT=\"%s Map\"></A></TD></TR></TABLE>",html_home_url,html_image_url(TCZMAP_IMG),tcz_full_name);
     output(p,NOTHING,1,0,0,"<BR><FONT COLOR=#00FF00><I>Click on map to enlarge...</I></FONT></CENTER><BR>");
}

/* ---->  {@?colourmap "<MAP>" "<TEXT>"} query command  <---- */
void map_query_colourmap(CONTEXT)
{
     struct arg_data arg;

     unparse_parameters(params,2,&arg,0);
     if(arg.count >= 2) {
        map_colourmap(arg.text[1],arg.text[0],1,querybuf,sizeof(querybuf) - 1);
        setreturn(querybuf,COMMAND_SUCC);
        return;
     }
     setreturn(ERROR,COMMAND_FAIL);
}

/* ---->  'map' command  <---- */
void map_main(CONTEXT)
{
     struct descriptor_data *p = getdsc(player);

     setreturn(ERROR,COMMAND_FAIL);
     if(IsHtml(p) && Blank(params)) {

        /* ---->  Graphical map for HTML Interface users  <---- */
        map_html(p);
        setreturn(OK,COMMAND_FAIL);
     } else if(!Blank(map_text) && Validchar(player)) {
        int   xoffset = 0,yoffset = 0,width = map_width,height = map_height;
        int   theight = db[player].data->player.scrheight - 6;
        char  buffer[TEXT_SIZE + 1],cbuffer[TEXT_SIZE + 1];
        const char *ptr = map_text,*cptr = map_colour;
        int   twidth = output_terminal_width(player) - 2;
        const char *title,*origparams = params;
        char  titlebuffer[TEXT_SIZE];
        int   count,blen,cblen;
        int   error = 0,adjust;
        char  *bptr,*cbptr;

        /* ---->  Text-based map (Also available to HTML users if parameters are given to 'map' command.)  <---- */
        if(!IsHtml(p)) {
           if(Blank(params) || (count = string_compare("central",params,6)) || (count = string_compare("centre",params,5)) || (count = string_compare("center",params,0)) || (count = string_compare("middle",params,0)) ||
              string_compare("east",params,0) || string_compare("west",params,0) || string_compare("left",params,0) || string_compare("right",params,0)) {

              /* ---->  Centre / Middle section  <---- */
              if(*params && !strcasecmp("ce",params)) count = 1;
              for(; (count > 0) && *params; count--, params++);
              for(; *params && !isalpha(*params); params++);
              if(!*params || string_compare("central",params,6) || string_compare("centre",params,5) || string_compare("center",params,0) || string_compare("middle",params,0)) {
                 sprintf(titlebuffer,"%s Map ("ANSI_LWHITE"Central"ANSI_LYELLOW")",tcz_full_name);
                 title   = titlebuffer;
                 xoffset = ((map_width - MIN(twidth,map_width)) / 2);
                 yoffset = ((map_height - MIN(theight,map_height)) / 2);
                 width   = MIN(twidth,map_width);
                 height  = MIN(theight,map_height);
	      } else if(string_compare("west",params,0) || string_compare("left",params,0)) {
                 sprintf(titlebuffer,"%s Map ("ANSI_LWHITE"Central-West"ANSI_LYELLOW")",tcz_full_name);
                 title   = titlebuffer;
                 xoffset = 0;
                 yoffset = ((map_height - MIN(theight,map_height)) / 2);
                 width   = MIN(twidth,map_width);
                 height  = MIN(theight,map_height);
	      } else if(string_compare("east",params,0) || string_compare("right",params,0)) {
                 sprintf(titlebuffer,"%s Map ("ANSI_LWHITE"Central-East"ANSI_LYELLOW")",tcz_full_name);
                 title   = titlebuffer;
                 xoffset = map_width - MIN(twidth,map_width);
                 yoffset = ((map_height - MIN(theight,map_height)) / 2);
                 width   = MIN(twidth,map_width);
                 height  = MIN(theight,map_height);
	      } else error = 1;
  	   } else if((count = string_compare("north",params,0)) || (count = string_compare("top",params,0))) {

              /* ---->  North / Top section  <---- */
              for(; (count > 0) && *params; count--, params++);
              for(; *params && !isalpha(*params); params++);
              if(!*params || string_compare("central",params,6) || string_compare("centre",params,5) || string_compare("center",params,0) || string_compare("middle",params,0)) {
                 sprintf(titlebuffer,"%s Map ("ANSI_LWHITE"North-Central"ANSI_LYELLOW")",tcz_full_name);
                 title   = titlebuffer;
                 xoffset = ((map_width - MIN(twidth,map_width)) / 2);
                 yoffset = 0;
                 width   = MIN(twidth,map_width);
                 height  = MIN(theight,map_height);
  	      } else if(string_compare("west",params,0) || string_compare("left",params,0)) {
                 sprintf(titlebuffer,"%s Map ("ANSI_LWHITE"North-West"ANSI_LYELLOW")",tcz_full_name);
                 title   = titlebuffer;
                 xoffset = 0;
                 yoffset = 0;
                 width   = MIN(twidth,map_width);
                 height  = MIN(theight,map_height);
	      } else if(string_compare("east",params,0) || string_compare("right",params,0)) {
                 sprintf(titlebuffer,"%s Map ("ANSI_LWHITE"North-East"ANSI_LYELLOW")",tcz_full_name);
                 title   = titlebuffer;
                 xoffset = map_width - MIN(twidth,map_width);
                 yoffset = 0;
                 width   = MIN(twidth,map_width);
                 height  = MIN(theight,map_height);
	      } else error = 1;
  	   } else if((count = string_compare("south",params,0)) || (count = string_compare("bottom",params,0))) {

              /* ---->  South / Bottom section  <---- */
              for(; (count > 0) && *params; count--, params++);
              for(; *params && !isalpha(*params); params++);
              if(!*params || string_compare("central",params,6) || string_compare("centre",params,5) || string_compare("center",params,0) || string_compare("middle",params,0)) {
                 sprintf(titlebuffer,"%s Map ("ANSI_LWHITE"South-Central"ANSI_LYELLOW")",tcz_full_name);
                 title   = titlebuffer;
                 xoffset = ((map_width - MIN(twidth,map_width)) / 2);
                 yoffset = map_height - MIN(theight,map_width);
                 width   = MIN(twidth,map_width);
                 height  = MIN(theight,map_height);
	      } else if(string_compare("west",params,0) || string_compare("left",params,0)) {
                 sprintf(titlebuffer,"%s Map ("ANSI_LWHITE"South-West"ANSI_LYELLOW")",tcz_full_name);
                 title   = titlebuffer;
                 xoffset = 0;
                 yoffset = map_height - MIN(theight,map_width);
                 width   = MIN(twidth,map_width);
                 height  = MIN(theight,map_height);
	      } else if(string_compare("east",params,0) || string_compare("right",params,0)) {
                 sprintf(titlebuffer,"%s Map ("ANSI_LWHITE"South-East"ANSI_LYELLOW")",tcz_full_name);
                 title   = titlebuffer;
                 xoffset = map_width - MIN(twidth,map_width);
                 yoffset = map_height - MIN(theight,map_width);
                 width   = MIN(twidth,map_width);
                 height  = MIN(theight,map_height);
	      } else error = 1;
	   } else error = 1;

           if(error) {
              output(p,player,0,1,0,ANSI_LGREEN"Sorry, the map section '"ANSI_LWHITE"%s"ANSI_LGREEN"' is unrecognised (Try using compass directions.)",origparams);
              return;
	   }
	} else {
           sprintf(titlebuffer,"%s Map"ANSI_LWHITE""ANSI_LYELLOW,tcz_full_name);
           title   = titlebuffer;
           xoffset = 0;
           yoffset = 0;
           width   = map_width;
           height  = map_height;
           twidth  = map_width;
           theight = map_height;
	}

        /* ---->  Map header  <---- */
        adjust = strlen(title) - strlen(ANSI_LWHITE""ANSI_LYELLOW);
        count  = (width - adjust) / 2;
        error  = ((twidth % 2) == 0);
        if(IsHtml(p)) {
           html_anti_reverse(p,1);
           output(p,player,1,1,0,"<P><TABLE BORDER=5 BGCOLOR="HTML_TABLE_BLACK"><TR><TD>");
	}
        output(p,player,0,1,0,"%s"ANSI_DCYAN".%s.%s",IsHtml(p) ? "\016<TT>\016":"",strpad('-',width,buffer),IsHtml(p) ? "\016</TT>\016":"");
        output(p,player,0,1,0,"%s"ANSI_DCYAN"|%s"ANSI_LYELLOW"%s"ANSI_DCYAN"%s%s|%s",IsHtml(p) ? "\016<TT>\016":"",strpad(' ',count,buffer),title,buffer,((adjust % 2) == error) ? " ":"",IsHtml(p) ? "\016</TT>\016":"");
        output(p,player,0,1,0,"%s"ANSI_DCYAN"|%s|%s",IsHtml(p) ? "\016<TT>\016":"",strpad('-',width,buffer),IsHtml(p) ? "\016</TT>\016":"");

        /* ---->  Skip to starting line of map section  <---- */
        for(; (yoffset > 0) && *ptr; yoffset--) {
            for(; *cptr && (*cptr != '\n'); cptr++);
            for(; *ptr  && (*ptr  != '\n'); ptr++);
            if(*cptr) cptr++;
            if(*ptr)  ptr++;
	}

        /* ---->  Get and draw each line of map section  <---- */
        for(; (height > 0) && *ptr; height--) {
            bptr   = buffer, cbptr = cbuffer;
            adjust = xoffset, count = width;
            blen   = 0, cblen = 0;

            /* ---->  X offset adjustment  <---- */
            while((adjust > 0) && *ptr && (*ptr != '\n')) {
                  if(*cptr && (*cptr != '\n')) cptr++;
                  ptr++, adjust--;
	    }

            /* ---->  Copy map text and colour map for line  <---- */
            while((count > 0) && (blen < TEXT_SIZE) && *ptr && (*ptr != '\n')) {
                  if((cblen < TEXT_SIZE) && *cptr && (*cptr != '\n'))
                     *cbptr++ = *cptr++, cblen++;
                  count--, *bptr++ = *ptr++, blen++;
	    }

            /* ---->  Pad with blanks if less than required width  <---- */
            while((count > 0) && (blen < TEXT_SIZE))
                  count--, *bptr++ = ' ', blen++;

            /* ---->  Draw map line  <---- */
            *bptr = '\0', *cbptr = '\0';
            output(p,player,0,1,0,"%s"ANSI_DCYAN"|"ANSI_DWHITE"%s"ANSI_DCYAN"|%s",IsHtml(p) ? "\016<TT>\016":"",map_colourmap(buffer,cbuffer,0,scratch_return_string,BUFFER_LEN - 1),IsHtml(p) ? "\016</TT>\016":"");

            /* ---->  Skip to next line  <---- */
            for(; *cptr && (*cptr != '\n'); cptr++);
            for(; *ptr  && (*ptr  != '\n'); ptr++);
            if(*cptr) cptr++;
            if(*ptr)  ptr++;
	}
        output(p,player,0,1,0,"%s"ANSI_DCYAN"`%s'%s%s",IsHtml(p) ? "\016<TT>\016":"",strpad('-',width,buffer),IsHtml(p) ? "":"\n",IsHtml(p) ? "\016</TT>\016":"");
        
        if(IsHtml(p)) {
           output(p,player,1,1,0,"</TD></TR></TABLE><P>");
           html_anti_reverse(p,0);
	}
        setreturn(OK,COMMAND_SUCC);
     } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, "ANSI_LYELLOW"%s"ANSI_LGREEN" map isn't currently available.",tcz_full_name);
}
