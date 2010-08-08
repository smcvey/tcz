/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| PREFERENCES.C  -  Implements the 'set' command, which is used to set basic  |
|                   user and terminal preferences.                            |
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
| Module originally designed and written by:  J.P.Boggis 24/09/2000.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: preferences.c,v 1.2 2005/06/29 20:19:59 tcz_monster Exp $

*/


#include <string.h>
#include <ctype.h>

#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "descriptor_flags.h"
#include "object_types.h"
#include "flagset.h"
#include "html.h"


/* ---->  Set whether to use ANSI colour or not  <---- */
void prefs_ansi(dbref player,struct descriptor_data *d,const char *status)
{
     setreturn(ERROR,COMMAND_FAIL);
     if(!d) d = getdsc(player);
     if(d && (!(d->flags & CONNECTED) && IsHtml(d))) return;

     if(Blank(status)) {
        if(!in_command && d) {
           int ansi;
 
           if(Validchar(player)) ansi = (db[player].flags & ANSI)|(db[player].flags2 & ANSI8);
              else ansi = d->flags & ANSI_MASK;

           if(ansi & ANSI) output(d,player,0,1,0,ANSI_LGREEN"ANSI colour is "ANSI_LWHITE"on"ANSI_LGREEN"  -  Number of colours:  "ANSI_LWHITE"%s"ANSI_LGREEN".",(ansi & ANSI8) ? "8":"16");
	      else output(d,player,0,1,0,ANSI_LGREEN"ANSI colour is "ANSI_LWHITE"off"ANSI_LGREEN".");
        }
     } else if(string_prefix("on",status) || string_prefix("16colours",status) || string_prefix("16 colours",status) || string_prefix("16colors",status) || string_prefix("16 colors",status)) {
        if(Validchar(player)) {
           db[player].flags  |=  ANSI;
           db[player].flags2 &= ~ANSI8;
	}
	if(d) {
           d->flags &= ~ANSI_MASK;
           d->flags |=  ANSI;
	}
        if(!in_command) output(d,player,0,1,0,ANSI_LGREEN"Ansi colour is now "ANSI_LWHITE"on"ANSI_LGREEN"  -  Number of colours:  "ANSI_LWHITE"16"ANSI_LGREEN".");
     } else if(string_prefix("8colours",status) || string_prefix("8 colours",status) || string_prefix("8colors",status) || string_prefix("8 colors",status)) {
        if(Validchar(player)) {
           db[player].flags  |= ANSI;
           db[player].flags2 |= ANSI8;
	}
	if(d) {
           d->flags &= ~ANSI_MASK;
           d->flags |=  ANSI|ANSI8;
	}
        if(!in_command) output(d,player,0,1,0,ANSI_LGREEN"Ansi colour is now "ANSI_LWHITE"on"ANSI_LGREEN"  -  Number of colours:  "ANSI_LWHITE"8"ANSI_LGREEN".");
     } else if(string_prefix("off",status)) {
        if(Validchar(player)) {
           db[player].flags  &= ~ANSI;
           db[player].flags2 &= ~ANSI8;
	}
	if(d) d->flags &= ~ANSI_MASK;
        if(!in_command) output(d,player,0,1,0,ANSI_LGREEN"Ansi colour is now "ANSI_LWHITE"off"ANSI_LGREEN".");
     } else {
        output(d,player,0,1,0,ANSI_LGREEN"Please specify either "ANSI_LWHITE"on"ANSI_LGREEN", "ANSI_LWHITE"16 colours"ANSI_LGREEN", "ANSI_LWHITE"8 colours"ANSI_LGREEN" or "ANSI_LWHITE"off"ANSI_LGREEN".");
        return;
     }
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Set preferred line termination sequence (LF, LF+CR, CR+LF or CR)  <---- */
void prefs_lftocr(dbref player,struct descriptor_data *d,const char *status)
{
     setreturn(ERROR,COMMAND_FAIL);
     if(!d) d = getdsc(player);
     if(d && (!(d->flags & CONNECTED) && IsHtml(d))) return;

     if(Blank(status)) {
        if(!in_command && d) {
           int lftocr;
 
           if(Validchar(player)) lftocr = db[player].flags2 & LFTOCR_MASK;
              else lftocr = d->flags & TERMINATOR_MASK;

	   switch(lftocr) {
                  case LFTOCR_CR:
         	       output(d,player,0,1,0,ANSI_LGREEN"Lines of text output to your terminal will be terminated with "ANSI_LWHITE"CR"ANSI_LGREEN".");
                       break;
                  case LFTOCR_LFCR:
         	       output(d,player,0,1,0,ANSI_LGREEN"Lines of text output to your terminal will be terminated with "ANSI_LWHITE"LF"ANSI_LGREEN" and "ANSI_LWHITE"CR"ANSI_LGREEN".");
                       break;
		  case LFTOCR_CRLF:
     	               output(d,player,0,1,0,ANSI_LGREEN"Lines of text output to your terminal will be terminated with "ANSI_LWHITE"CR"ANSI_LGREEN" and "ANSI_LWHITE"LF"ANSI_LGREEN".");
                       break;
		  default:
     	               output(d,player,0,1,0,ANSI_LGREEN"Lines of text output to your terminal will be terminated with "ANSI_LWHITE"LF"ANSI_LGREEN".");
	   }
        }
     } else if(string_prefix("off",status) || (string_prefix("lf",status) && (strlen(status) < 3))) {
        if(Validchar(player)) db[player].flags2 &= ~LFTOCR_MASK;
	if(d) d->flags &= ~TERMINATOR_MASK;
        if(!in_command) output(d,player,0,1,0,ANSI_LGREEN"Lines of text output to your terminal will be terminated with "ANSI_LWHITE"LF"ANSI_LGREEN".");
     } else if(string_prefix("on",status) || string_prefix("lfcr",status)) {
        if(Validchar(player)) {
           db[player].flags2 &= ~LFTOCR_MASK;
	   db[player].flags2 |=  LFTOCR_LFCR;
	}
	if(d) {
           d->flags &= ~TERMINATOR_MASK;
           d->flags |=  LFTOCR_LFCR;
	}
        if(!in_command) output(d,player,0,1,0,ANSI_LGREEN"Lines of text output to your terminal will be terminated with "ANSI_LWHITE"LF"ANSI_LGREEN" and "ANSI_LWHITE"CR"ANSI_LGREEN".");
     } else if(string_prefix("cr",status)) {
        if(Validchar(player)) {
           db[player].flags2 &= ~LFTOCR_MASK;
	   db[player].flags2 |=  LFTOCR_CR;
	}
	if(d) {
           d->flags &= ~TERMINATOR_MASK;
           d->flags |=  LFTOCR_CR;
	}
        if(!in_command) output(d,player,0,1,0,ANSI_LGREEN"Lines of text output to your terminal will be terminated with "ANSI_LWHITE"CR"ANSI_LGREEN".");
     } else if(string_prefix("crlf",status)) {
        if(Validchar(player)) {
           db[player].flags2 &= ~LFTOCR_MASK;
	   db[player].flags2 |=  LFTOCR_CRLF;
	}
	if(d) {
           d->flags &= ~TERMINATOR_MASK;
           d->flags |=  LFTOCR_CRLF;
	}
        if(!in_command) output(d,player,0,1,0,ANSI_LGREEN"Lines of text output to your terminal will be terminated with "ANSI_LWHITE"CR"ANSI_LGREEN" and "ANSI_LWHITE"LF"ANSI_LGREEN".");
     } else {
        output(d,player,0,1,0,ANSI_LGREEN"Please specify either "ANSI_LWHITE"on"ANSI_LGREEN", "ANSI_LWHITE"lf"ANSI_LGREEN", "ANSI_LWHITE"lfcr"ANSI_LGREEN", "ANSI_LWHITE"crlf"ANSI_LGREEN", "ANSI_LWHITE"cr"ANSI_LGREEN" or "ANSI_LWHITE"off"ANSI_LGREEN".");
        return;
     } 
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Echo text typed by user back to their terminal  <---- */
void prefs_localecho(dbref player,struct descriptor_data *d,const char *status)
{
     setreturn(ERROR,COMMAND_FAIL);
     if(d && (!(d->flags & CONNECTED) && IsHtml(d))) return;

     if(Blank(status)) {
        if(!in_command && d) {
           int echo;
 
           if(Validchar(player)) echo = (db[player].flags2 & LOCAL_ECHO);
              else echo = d->flags & LOCAL_ECHO;
           output(d,player,0,1,0,(echo) ? ANSI_LGREEN"Echo is "ANSI_LWHITE"on"ANSI_LGREEN".":ANSI_LGREEN"Echo is "ANSI_LWHITE"off"ANSI_LGREEN".");
	}
     } else if(string_prefix("on",status)) {
        if(Validchar(player)) db[player].flags2 |= LOCAL_ECHO;
        if(d) d->flags |= LOCAL_ECHO;
        if(!in_command) output(d,player,0,1,0,ANSI_LGREEN"Echo is now "ANSI_LWHITE"on"ANSI_LGREEN".");
     } else if(string_prefix("off",status)) {
        if(Validchar(player)) db[player].flags2 &= ~LOCAL_ECHO;
        if(d) d->flags &= ~LOCAL_ECHO;
        if(!in_command) output(d,player,0,1,0,ANSI_LGREEN"Echo is now "ANSI_LWHITE"off"ANSI_LGREEN".");
     } else {
        output(d,player,0,1,0,ANSI_LGREEN"Please specify either "ANSI_LWHITE"on"ANSI_LGREEN" or "ANSI_LWHITE"off"ANSI_LGREEN".");
        return;
     }
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Set whether terminal beeps or not when someone pages you  <---- */
void prefs_pagebell(dbref player,struct descriptor_data *d,const char *status)
{
     setreturn(ERROR,COMMAND_FAIL);
     if(d && (!(d->flags & CONNECTED) && IsHtml(d))) return;
     if(!d) {
        if(!Blank(status)) {
           if(string_prefix("on",status)) {
              db[player].flags2 |= PAGEBELL;
              if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Page bell is now "ANSI_LWHITE"on"ANSI_LGREEN".");
	   } else if(string_prefix("off",status)) {
              db[player].flags2 &= ~PAGEBELL;
              if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Page bell is now "ANSI_LWHITE"off"ANSI_LGREEN".");
	   } else {
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify either "ANSI_LWHITE"on"ANSI_LGREEN" or "ANSI_LWHITE"off"ANSI_LGREEN".");
              return;
	   }
           setreturn(OK,COMMAND_SUCC);
	} else if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Page bell is "ANSI_LWHITE"%s"ANSI_LGREEN".",Pagebell(player) ? "on":"off");
     } else output(d,player,0,1,0,ANSI_LGREEN"Sorry, you must be connected to set your page bell preference.");
}

/* ---->  Set user prompt  <---- */
void prefs_prompt(dbref player,struct descriptor_data *d,const char *params)
{
     setreturn(ERROR,COMMAND_FAIL);
     if(!d) d = getdsc(player);
     if(!d || (!(d->flags & CONNECTED) && IsHtml(d))) return;

     if(d->flags & CONNECTED) {
        if(!d->edit) {
           if(strlen(params) <= 80) {
              if(!Blank(params)) {
                 FREENULL(d->user_prompt);
                 sprintf(scratch_return_string,ANSI_LWHITE"%s%s ",punctuate((char *) params,2,'\0'),ANSI_DWHITE);
                 if(Validchar(d->player) && !Validchar(player)) player = d->player;
                 d->user_prompt = (char *) alloc_string(scratch_return_string);
                 if(!in_command) {
                    substitute(player,scratch_buffer,scratch_return_string,0,ANSI_LWHITE,NULL,0);
                    output(d,player,0,1,0,ANSI_LGREEN"Your prompt is now '%s"ANSI_LGREEN"'.",scratch_buffer);
		 }
                 setreturn(OK,COMMAND_SUCC);
	      } else {
                 FREENULL(d->user_prompt);
                 d->user_prompt = (char *) alloc_string(ANSI_DWHITE);
                 if(!in_command) output(d,player,0,1,0,ANSI_LGREEN"Your prompt has been removed.");
                 setreturn(OK,COMMAND_SUCC);
	      }
	   } else output(d,player,0,1,0,ANSI_LGREEN"Sorry, the maximum length of your prompt is 80 characters.");
	} else output(d,player,0,1,0,ANSI_LGREEN"Sorry, you can't set your prompt while you're using the editor.");
     } else output(d,player,0,1,0,ANSI_LGREEN"Sorry, you must be connected to set your prompt.");
}

/* ---->  Set screen height  <---- */
void prefs_screenheight(dbref player,struct descriptor_data *d,const char *params)
{
     setreturn(ERROR,COMMAND_FAIL);
     if(!d) {
        if(!Blank(params)) {
           int height = atol(params);

           if(height >= 0) {
              if(height >= 15) {
                 db[player].data->player.scrheight = MIN(MAX(height,15),255);
                 if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Your screen height is now "ANSI_LWHITE"%d"ANSI_LGREEN" line%s.",height,Plural(height));
                 setreturn(OK,COMMAND_SUCC);
	      } else if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the minimum screen height supported is "ANSI_LWHITE"15"ANSI_LGREEN" lines.");
	   } else if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't set your screen height to a negative value.");
	} else if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Your screen height is "ANSI_LWHITE"%d"ANSI_LGREEN" line%s.",db[player].data->player.scrheight,Plural(db[player].data->player.scrheight));
     } else output(d,player,0,1,0,ANSI_LGREEN"Sorry, you must be connected to set your screen height.");
}

/* ---->  Set terminal type  <---- */
void prefs_termtype(dbref player,struct descriptor_data *d,const char *termtype)
{
     setreturn(ERROR,COMMAND_FAIL);
     if(!d) d = getdsc(player);
     if(!d) return;

     if(!Blank(termtype)) {
	if(!(!strcasecmp(termtype,"none") || !strcasecmp(termtype,"unknown") || !strcasecmp(termtype,"dumb"))) {
	   if(set_terminal_type(d,(char *) termtype,0)) {
              if(!in_command) output(d,player,0,1,0,ANSI_LGREEN"Your terminal type is now set to '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",String(d->terminal_type));
	   } else if(!in_command) output(d,player,0,1,0,ANSI_LGREEN"Sorry, the terminal type '"ANSI_LWHITE"%s"ANSI_LGREEN"' is unknown.",termtype);
	} else {
	   set_terminal_type(d,"none",0);
           if(!in_command) output(d,player,0,1,0,ANSI_LGREEN"Your terminal type is no-longer set.");
	}
     } else if(!in_command) {
        if(d->terminal_type) output(d,player,0,1,0,ANSI_LGREEN"Your terminal type is currently set to '"ANSI_LWHITE"%s"ANSI_LGREEN"'.",String(d->terminal_type));
           else output(d,player,0,1,0,ANSI_LGREEN"Your terminal type isn't set.");
     }
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Set time difference  <---- */
void prefs_timediff(dbref player,struct descriptor_data *d,const char *params)
{
     setreturn(ERROR,COMMAND_FAIL);
     if(!d) {
        for(; !Blank(params) && (*params == ' '); params++);
        if(!Blank(params)) {
           const char *ptr = params;

           if(*params == '+') params++;
           if(*ptr    == '-') ptr++;
           if(isdigit(*ptr)) {
              int diff = atol(params);

              if(abs(diff) <= 24) {
                 db[player].data->player.timediff = diff;
                 if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Your time difference is now "ANSI_LWHITE"%+d"ANSI_LGREEN" hour%s.",diff,Plural(diff));
                 setreturn(OK,COMMAND_SUCC);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, your time difference can only be set to +/- 24 hours.");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, your time difference can only be set to +/- 24 hours.");
	} else if(!in_command) {
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"Your time difference is "ANSI_LWHITE"%+d"ANSI_LGREEN" hour%s.",db[player].data->player.timediff,Plural(db[player].data->player.timediff));
           setreturn(OK,COMMAND_SUCC);
	}
     } else output(d,player,0,1,0,ANSI_LGREEN"Sorry, you must be connected to set your time difference.");
}

/* ---->  Set whether proper underlines or underscores (~~~~) will be used to underline titles  <---- */
void prefs_underlining(dbref player,struct descriptor_data *d,const char *status)
{
     setreturn(ERROR,COMMAND_FAIL);
     if(!d) d = getdsc(player);

     if(Blank(status)) {
        if(!in_command && d) {
           int underline;

           if(Validchar(player)) underline = (db[player].flags2 & UNDERLINE);
              else underline = (d->flags & UNDERLINE);
           output(d,player,0,1,0,(underline) ? ANSI_LGREEN"Underlining is "ANSI_LWHITE"on"ANSI_LGREEN".":ANSI_LGREEN"Underlining is "ANSI_LWHITE"off"ANSI_LGREEN".");
	}
     } else if(string_prefix("on",status)) {
        if(Validchar(player)) db[player].flags2 |= UNDERLINE;
        if(d) d->flags                          |= UNDERLINE;
        if(!in_command) output(d,player,0,1,0,ANSI_LGREEN"Underlining is now "ANSI_LWHITE"on"ANSI_LGREEN".");
     } else if(string_prefix("off",status)) {
        if(Validchar(player)) db[player].flags2 &= ~UNDERLINE;
        if(d) d->flags                          &= ~UNDERLINE;
        if(!in_command) output(d,player,0,1,0,ANSI_LGREEN"Underlining is now "ANSI_LWHITE"off"ANSI_LGREEN".");
     } else output(d,player,0,1,0,ANSI_LGREEN"Please specify either "ANSI_LWHITE"on"ANSI_LGREEN" or "ANSI_LWHITE"off"ANSI_LGREEN".");
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Set word wrap  <---- */
void prefs_wrap(dbref player,struct descriptor_data *d,char *width)
{
     int i;

     setreturn(ERROR,COMMAND_FAIL);
     if(!d) d = getdsc(player);
     if(!d) return;

     if(!Blank(width)) {
        if((i = atol(width)) >= 0) {
           if(!((i > 0) && (i < 44))) {
              if(!string_prefix("on",width)) {
                 if((i == 0) && !Blank(width) && !string_prefix("off",width)) {
                    output(d,player,0,1,0,ANSI_LGREEN"Please specify the width of your screen.");
                    return;
		 }
	      } else i = 80;

              if(i) i = MIN(MAX(i,44),256);
              if(i > 0) d->terminal_width = i - 1;
 	         else d->terminal_width = 0;

              if(!in_command) {
                 if(d->terminal_width != 0) output(d,player,0,1,0,ANSI_LGREEN"Screen width (Word wrap) set to "ANSI_LWHITE"%d"ANSI_LGREEN" character%s.",i,Plural(i));
		    else output(d,player,0,1,0,ANSI_LGREEN"Word wrap turned "ANSI_LWHITE"off"ANSI_LGREEN".");
	      }
              setreturn(OK,COMMAND_SUCC);
	   } else if(!in_command) output(d,player,0,1,0,ANSI_LGREEN"Sorry, the minimum screen width supported is "ANSI_LWHITE"44"ANSI_LGREEN" characters.");
	} else if(!in_command) output(d,player,0,1,0,ANSI_LGREEN"Sorry, you can't set your screen width to a negative value.");
     } else {
        if(!in_command) {
           if(d->terminal_width != 0) output(d,player,0,1,0,ANSI_LGREEN"Word wrap is "ANSI_LWHITE"on"ANSI_LGREEN".\nYour screen width is "ANSI_LWHITE"%d"ANSI_LGREEN" characters.",d->terminal_width + 1);
   	      else output(d,player,0,1,0,ANSI_LGREEN"Word wrap is "ANSI_LWHITE"off"ANSI_LGREEN".");
	}
        setreturn(OK,COMMAND_SUCC);
     }
}

/* ---->  Set word-wrap, ANSI colour, terminal, LFTOCR and page bell preferences  <---- */
void prefs_set(dbref player,struct descriptor_data *d,char *arg1,char *arg2)
{
     char *command,*ptr = NULL,c;

     setreturn(ERROR,COMMAND_FAIL);
     if(Validchar(player)) d = getdsc(player);
     if(!d) return;

     /* ---->  Grab new command and 1st param  <---- */
     while(*arg1 && (*arg1 == ' ')) arg1++;
     command = arg1;
     while(*arg1 && (*arg1 != ' ')) arg1++;
     if(*arg1) ptr = arg1, c = *arg1, *arg1++ = '\0';
     if(!Blank(arg2)) arg1 = arg2;
        else while(*arg1 && (*arg1 == ' ')) arg1++;

     if(!Blank(command) && (string_prefix("wrapping",command) || string_prefix("width",command) || string_prefix("screenwidth",command) || string_prefix("wordwrapping",command) || string_prefix("scrwidth",command))) {

        /* ---->  Set word wrap (Screen width)  <---- */
        prefs_wrap(NOTHING,d,arg1);
     } else if(!Blank(command) && (string_prefix("height",command) || string_prefix("screenheight",command) || string_prefix("scrheight",command))) {

        /* ---->  Set screen height  <---- */
        prefs_screenheight(!Validchar(player) ? d->player:player,!Validchar(player) ? d:NULL,arg1);
     } else if(!Blank(command) && (string_prefix("ansicolours",command) || string_prefix("ansicolors",command))) {

        /* ---->  Set whether to use ANSI colour or not  <---- */
        prefs_ansi(player,d,arg1);
     } else if(!Blank(command) && (string_prefix("termtype",command) || string_prefix("terminaltype",command))) {

        /* ---->  Set preferred terminal type  <---- */
        prefs_termtype(NOTHING,d,arg1);
     } else if(!Blank(command) && (string_prefix("echo",command) || string_prefix("localecho",command) || string_prefix("remoteecho",command))) {

        /* ---->  Set whether TCZ should echo text typed by user to their terminal  <---- */
        prefs_localecho(player,d,arg1);
     } else if(!Blank(command) && string_prefix("lftocr",command)) {

        /* ---->  Set preferred line termination sequence  <---- */
        prefs_lftocr(player,d,arg1);
     } else if(!Blank(command) && (string_prefix("underlines",command) || string_prefix("underlining",command) || string_prefix("underscoring",command) || string_prefix("underscores",command))) {

        /* ---->  Set underlining preference  <---- */
        prefs_underlining(!Validchar(player) ? d->player:player,!Validchar(player) ? d:NULL,arg1);
     } else if(!Blank(command) && (string_prefix("pagebell",command) || string_prefix("pagebeeping",command) || string_prefix("bell",command) || string_prefix("beep",command))) {

        /* ---->  Set page bell preference  <---- */
        prefs_pagebell(!Validchar(player) ? d->player:player,!Validchar(player) ? d:NULL,arg1);
     } else if(!Blank(command) && string_prefix("prompt",command)) {

        /* ---->  Set preferred prompt  <---- */
        prefs_prompt(NOTHING,d,arg1);
     } else if(!Blank(command) && (string_prefix("timedifference",command) || string_prefix("difference",command) || string_prefix("jetlag",command))) {

        /* ---->  Set time difference  <---- */
        prefs_timediff(!Validchar(player) ? d->player:player,!Validchar(player) ? d:NULL,arg1);
     } else if(!Blank(command) && (string_prefix("morepager",command) || string_prefix("morepaging",command))) {

        /* ---->  'More' paging facility  <---- */
        pager_more(player,arg1,NULL,NULL,NULL,1,0);
     } else if(!Blank(command) && (string_prefix("afkauto",command) || string_prefix("autoafk",command))) {

        /* ---->  Auto-AFK facility  <---- */
        set_afk(player,arg1,NULL,NULL,NULL,1,0);
     } else if(d) {

        /* ---->  Display user's current preferences  <---- */
        if(!in_command) {
           if(IsHtml(d)) {
              html_anti_reverse(d,1);
              output(d,player,1,2,0,"<BR><TABLE BORDER WIDTH=100%% CELLPADDING=4>");
	      output(d,player,2,1,0,"\016<TR><TH ALIGN=CENTER BGCOLOR="HTML_TABLE_BLUE"><FONT SIZE=4><I>\016"ANSI_LCYAN"Your current preferences are...\016</I></FONT></TH></TR>\016");
              output(d,player,1,2,0,"<TR><TD BGCOLOR="HTML_TABLE_BLACK">");
	   } else if(!Validchar(d->player)) output(d,player,0,1,0,"\nYour current preferences are...\n"ANSI_DCYAN"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
	        else tilde_string(d->player,"Your current preferences are...",ANSI_LCYAN,ANSI_DCYAN,0,1,5);

           prefs_wrap(NOTHING,d,"");
           if(Validchar(player)) prefs_screenheight(player,NULL,"");
           output(d,player,0,1,0,"");
           prefs_lftocr(player,d,"");
           prefs_termtype(NOTHING,d,"");
           if(Validchar(player)) {
              set_afk(player,"",NULL,NULL,NULL,1,0);
              pager_more(player,"",NULL,NULL,NULL,1,0);
	   }
           prefs_localecho(NOTHING,d,"");
           output(d,player,0,1,0,"");
           prefs_ansi(player,d,"");
           if(Validchar(player)) {
              prefs_underlining(player,NULL,"");
              prefs_pagebell(player,NULL,"");
              output(d,player,0,1,0,"");
              prefs_timediff(player,NULL,"");
	   } else {
              output(d,player,0,1,0,"");
              output(d,player,0,1,14,ANSI_LRED"PLEASE NOTE: \016&nbsp;\016 "ANSI_LWHITE"Most of the above settings will be set automatically when you connect your character.  Further settings will be available once you have connected or created your %s character.",tcz_short_name);
	   }

           if(IsHtml(d)) {
              html_anti_reverse(d,0);
	      output(d,NOTHING,1,0,0,"</TD></TR><TR><TD ALIGN=LEFT BGCOLOR="HTML_TABLE_GREY"><FORM METHOD=POST ACTION=\"%s\">",html_server_url(d,0,0,NULL));
              html_preferences_form(d,0,1);
              output(d,NOTHING,1,0,0,"<INPUT NAME=PREFERENCES TYPE=HIDDEN VALUE=SET><INPUT NAME=DATA TYPE=HIDDEN VALUE=OUTPUT><INPUT NAME=CODE TYPE=HIDDEN VALUE=%08X%08X><P><CENTER><INPUT TYPE=SUBMIT VALUE=\"Save and use preferences...\"></CENTER></FORM></TD></TR></TABLE><BR>",d->html->id1,d->html->id2);
	   } else output(d,player,0,1,0,"");
	}
        if(ptr) *ptr = c;
        setreturn(OK,COMMAND_SUCC);
        return;
     }
     if(!Validchar(player)) output(d,player,0,1,0,"");
     if(ptr) *ptr = c;
}
