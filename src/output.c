/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| OUTPUT.C  -  Functions to send formatted or unformatted output to           |
|              users/descriptors, supporting word wrapping and output         |
|              redirection.                                                   |
|                                                                             |
|              Used extensively through TCZ for sending output to users.      |
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
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: output.c,v 1.2 2005/06/29 21:36:20 tcz_monster Exp $

*/


#include <arpa/telnet.h>
#include <string.h>
#include <ctype.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "descriptor_flags.h"
#include "friend_flags.h"
#include "object_types.h"
#include "flagset.h"
#include "fields.h"
#include "html.h"


/* ---->  Buffers used by word wrapping code  <---- */
static   char textbuf[BUFFER_LEN * 2];
static   char htmlbuf[BUFFER_LEN];

int           wrap_leading = 0;  /*  Number of lead-in blanks in wrapped text  */
int           termflags    = 0;  /*  Terminal display flags  */
unsigned char add_cr       = 0;  /*  Add CR to output  */


/* ---->  Add correct line termination sequence for character's terminal to string pointed to by DEST  <---- */
void output_terminate(char **dest,int terminator,int *length,unsigned char *overflow)
{
     switch(terminator) {
	    case LFTOCR_LFCR:
		 if(!length || ((*length - 2) > 0)) {
		    *(*dest)++ = '\n';
		    *(*dest)++ = '\r';
		    if(length) *length -= 2;
		 } else *overflow = 1;
		 break;
	    case LFTOCR_CRLF:
		 if(!length || ((*length - 2) > 0)) {
		    *(*dest)++ = '\r';
		    *(*dest)++ = '\n';
		    if(length) *length -= 2;
		 } else *overflow = 1;
		 break;
	    case LFTOCR_CR:
		 if(!length || ((*length - 1) > 0)) {
		    *(*dest)++ = '\r';
		    if(length) *length -= 1;
		 } else *overflow = 1;
		 break;
	    default:
		 if(!length || ((*length - 1) > 0)) {
		    *(*dest)++ = '\n';
		    if(length) *length -= 1;
		 } else *overflow = 1;
     }
}

/* ---->  Text remaining in specified string (Apart from ANSI codes and blank space)?  <---- */
int output_text_remaining(const char *str)
{
    if(!str) return(0);
    while(*str) {
          if(*str == '\x1B') {
             while(*str && (*str != 'm')) str++;
             if(*str && (*str == 'm')) str++;
	  } else if((*str == '\n') || (*str == ' ')) {
             if(wrap_leading) return(1);
                else return(0);
	  } else return(1);
    }
    return(0);
}

/* ---->  Insert ANSI code, appropriate terminal code or filter it out altogether  <---- */
int output_ansi_filter(char **dest,char **src,struct descriptor_data *d,const char **cur_bg,const char **cur_fg,int *ansiflags,int *length,unsigned char *overflow)
{
    static int  counter,ansi;
    static char *ptr,*start;

    counter = 0;
    ansi    = d->flags & ANSI_MASK;
    ptr     = start = (*src);
    while(*ptr && !isdigit(*ptr) && (*ptr != 'm')) ptr++;
          if((ansi & ANSI8) && *ptr && (*ptr == '1')) ansi = 0;
             else if(!(d->flags & UNDERLINE) && *ptr && (*ptr == '4') && (*(ptr + 1) == 'm')) ansi = 0;

    while(*(*src) && (*(*src) != 'm')) {
          if(ansi && (--(*length) > 0)) *(*dest)++ = *(*src);
          counter++, (*src)++;
    }
    if(*(*src) && (*(*src) == 'm')) {
       if(*ptr) switch(*ptr) {
          case '5':
               (*ansiflags) |= TXT_BLINK;
               break;
          case '4':
               if(*(ptr + 1) == '0') (*ansiflags) &= ~TXT_INVERSE, (*cur_bg) = start; 
                  else if(isdigit(*(ptr + 1))) (*ansiflags) |= TXT_INVERSE, (*cur_bg) = start;
                     else if(d->flags & UNDERLINE) (*ansiflags) |= TXT_UNDERLINE;
               break;
          case '1':
               (*ansiflags) |= TXT_BOLD;
               break;
          case '0':
               if(*(ptr + 1) == ';') {
                  (*ansiflags) = 0;
                  (*cur_fg)    = start;
	       }
       }
       if(ansi && (--(*length) > 0)) *(*dest)++ = *(*src);
       counter++, (*src)++;
    }

    if(!(d->flags & ANSI_MASK) && !termflags && *ptr) {
       switch(*ptr) {
              case '5':
                   ptr = (d->termcap) ? d->termcap->blink:NULL;
                   break;
              case '4':
                   switch(*(ptr + 1)) {
                          case '7':
                          case '6':
                          case '5':
                          case '4':
                          case '3':
                          case '2':
                          case '1':
                               ptr = (d->termcap) ? d->termcap->inverse:NULL;
                               break;
                          case '0':
                               ptr = (d->termcap) ? d->termcap->normal:NULL;
                               break;
                          default:
                               ptr = (d->termcap && (d->flags & UNDERLINE)) ? d->termcap->underline:NULL;
		   }
                   break;
              case '1':
                   ptr = (d->termcap) ? d->termcap->bold:NULL;
                   break;
              case '0':
                   ptr = (d->termcap) ? d->termcap->normal:NULL;
                   break;
              default:
                   ptr = NULL;
       }
       if(ptr) while(*ptr && (--(*length) > 0)) *(*dest)++ = *ptr, ptr++;
    }
    if(*length <= 0) *length = 0, *overflow = 1;
    return(counter);
}

/* ---->  When not using ANSI colour, puts terminal into appropriate style (E.g:  Bold, etc.)  <---- */
void output_terminal_style(char **dest,struct descriptor_data *d,int *length,unsigned char *overflow)
{
     static char *ptr;

     /* ---->  Normal text  <---- */
     if(termflags & TXT_NORMAL)
        for(ptr = (d->termcap) ? d->termcap->normal:NULL; !Blank(ptr) && (--(*length) > 0); *(*dest)++ = *ptr, ptr++);

     /* ---->  Bold text  <---- */
     if(termflags & TXT_BOLD) {
        for(ptr = (d->termcap) ? d->termcap->normal:NULL; !Blank(ptr) && (--(*length) > 0); *(*dest)++ = *ptr, ptr++);
        for(ptr = (d->termcap) ? d->termcap->bold:NULL;   !Blank(ptr) && (--(*length) > 0); *(*dest)++ = *ptr, ptr++);
     }

     /* ---->  Blinking text  <---- */
     if(termflags & TXT_BLINK)
        for(ptr = (d->termcap) ? d->termcap->blink:NULL; !Blank(ptr) && (--(*length) > 0); *(*dest)++ = *ptr, ptr++);

     /* ---->  Underlined text  <---- */
     if(termflags & TXT_UNDERLINE)
        for(ptr = (d->termcap) ? d->termcap->underline:NULL; !Blank(ptr) && (--(*length) > 0); *(*dest)++ = *ptr, ptr++);

     /* ---->  Inverse text  <---- */
     if(termflags & TXT_INVERSE)
        for(ptr = (d->termcap) ? d->termcap->inverse:NULL; !Blank(ptr) && (--(*length) > 0); *(*dest)++ = *ptr, ptr++);
     if(*length <= 0) *length = 0, *overflow = 1;
}

/* ---->  Filter non-standard ANSI codes (Those which cannot be produced using %-substitutions in TCZ)  <---- */
void output_filter_nonstandard_ansi(char *src)
{
     char *ptr;

     if(Blank(src)) return;
     for(; *src; src++)
         if(*src == '\x1B') {
            if(*(ptr = src + 1) && (*ptr == '[')) ptr++;
            for(; *ptr && (isdigit(*ptr) || (*ptr == ';')); ptr++);
            if(!*ptr || (*ptr != 'm')) *src = '^';
	 }
}

/* ---->  Queue text without word wrapping, but with ANSI filtering  <---- */
int output_queue_nonwrapped(struct descriptor_data *d,char *src)
{
    static int           counter,ansiflags,length;
    static const    char *cur_bg,*cur_fg;
    static char          *dest,*ptr;
    static unsigned char overflow;

    output_filter_nonstandard_ansi(src);

    /* ---->  If user has a prompt (Indicated by non-zero TERMINAL_XPOS), do a CR to skip to the line below it  <---- */
    dest   = textbuf, length = sizeof(textbuf) - 2, overflow = 0;
    cur_bg = ANSI_IBLACK, cur_fg = ANSI_DWHITE, ansiflags = 0;
    if((d->flags & PROMPT) && (d->terminal_xpos > 0)) {
       output_terminate(&dest,d->flags & TERMINATOR_MASK,&length,&overflow);
       d->flags &= ~PROMPT, d->terminal_xpos = 0;
    }

    /* ---->  Convert HTML tags to plain text  <---- */
    if(!html_to_text(d,src,htmlbuf,sizeof(htmlbuf))) return(0);
    src = htmlbuf;

    /* ---->  Standard ANSI colour (ANSI_LCYAN) or current terminal style  <---- */
    if(!(!(d->flags & ANSI_MASK) && termflags)) {
       if(!(*src && (*src == '\x1B'))) {
          ptr = ANSI_LCYAN;
          while(*ptr)
                output_ansi_filter(&dest,&ptr,d,&cur_bg,&cur_fg,&ansiflags,&length,&overflow);
       }
    } else output_terminal_style(&dest,d,&length,&overflow);

    /* ---->  Filter text  <---- */
    while(*src && !overflow)
          switch(*src) {
                 case '\x05':

                      /* ---->  Hanging indent control  <---- */
                      if(*(++src)) src++;
                      break;
                 case '\x06':

                      /* ---->  Skip rest of line  <---- */
                      for(; *src && (*src != '\n'); src++);
                      if(*src && (*src == '\n')) src++;
                      break;
                 case '\x1B':

                      /* ---->  ANSI code  <---- */
                      output_ansi_filter(&dest,&src,d,&cur_bg,&cur_fg,&ansiflags,&length,&overflow);
                      break;
                 case '\n':

                      /* ---->  New line  <---- */
	              output_terminate(&dest,d->flags & TERMINATOR_MASK,&length,&overflow);
                      if(!overflow) d->terminal_xpos = 0;
                      src++;
                      break;
	         default:
                      if((length - 1) > 0) {
                         d->terminal_xpos++;
                         *dest++ = *src;
                         src++, length--;
		      } else overflow = 1;
	  }

    /* ---->  Return to normal text colour (White text on black background)  <---- */
    if(add_cr == 1) {
       output_terminate(&dest,d->flags & TERMINATOR_MASK,&length,&overflow);
       if(!overflow) d->terminal_xpos = 0;
    }

    if(dest > textbuf) {
       if((*(dest - 1) == '\n') || (*(dest - 1) == '\r')) {
          for(counter = 0, dest--, length++, overflow = 0; (dest >= textbuf) && ((*dest == '\n') || (*dest == '\r')); dest--, counter++);
          if(((d->flags & TERMINATOR_MASK) == LFTOCR_LFCR) || ((d->flags & TERMINATOR_MASK) == LFTOCR_CRLF)) counter /= 2;
          ptr = ANSI_NORMAL, dest++, length--;
          while(*ptr && !overflow)
                output_ansi_filter(&dest,&ptr,d,&cur_bg,&cur_fg,&ansiflags,&length,&overflow);
          for(; (counter > 0) && !overflow; counter--)
              output_terminate(&dest,d->flags & TERMINATOR_MASK,&length,&overflow);
       } else {
          ptr = ANSI_NORMAL;
          while(*ptr && !overflow)
                output_ansi_filter(&dest,&ptr,d,&cur_bg,&cur_fg,&ansiflags,&length,&overflow);
       }
    }

    if((add_cr == 255) && ((length - 2) > 0)) *dest++ = IAC, *dest++ = EOR, length -= 2;
    *dest = '\0';

    return(server_queue_output(d,textbuf,strlen(textbuf)));
}

/* ---->  Word wrapping routine (Handles ANSI colour/formatting codes correctly and features user-adjustable hanging indent)  <---- */
int output_queue_wrapped(struct descriptor_data *d,char *src)
{
    static int           cached_wrapleading,ansiflags,leading,dummy2,length,blank,line,loop,ls,sc,wc,rc;
    static char          *sp,*wp,*ansi,*dest,*ptr,*colour;
    static const    char *cur_bg,*cur_fg,*dummy;
    static char          add_bg,start;
    static char          ansibuf[64];
    static unsigned char overflow;

    output_filter_nonstandard_ansi(src);
    cached_wrapleading = wrap_leading, start = (command_type & LEADING_BACKGROUND) ? 1:0;
    cur_bg = ANSI_IBLACK, cur_fg = ANSI_DWHITE, ansiflags = 0, add_bg = 0;
    leading = 0, blank = 1, dest = textbuf, line = 1, ls = 1;
    length = sizeof(textbuf) - 2, overflow = 0;

    if((d->flags & PROMPT) && (d->terminal_xpos > 0)) {
       output_terminate(&dest,d->flags & TERMINATOR_MASK,&length,&overflow);
       d->flags &= ~PROMPT, d->terminal_xpos = 0;
    }

    /* ---->  Convert HTML tags to plain text  <---- */
    if(!html_to_text(d,src,htmlbuf,sizeof(htmlbuf))) return(0);
    src = htmlbuf;

    /* ---->  Standard ANSI colour (ANSI_LCYAN) or current terminal style  <---- */
    if(!(!(d->flags & ANSI_MASK) && termflags)) {
       if(!(*src && (*src == '\x1B'))) {
          ptr = ANSI_LCYAN;
          while(*ptr)
                output_ansi_filter(&dest,&ptr,d,&cur_bg,&cur_fg,&ansiflags,&length,&overflow);
       }
    } else output_terminal_style(&dest,d,&length,&overflow);

    while(*src && !overflow) {

          /* ---->  New line in text  <---- */
          if(*src == '\n') {
             line++, src++;
             if((ansiflags & TXT_BLINK) || (ansiflags & TXT_UNDERLINE)) {
                colour = ANSI_DWHITE;
                output_ansi_filter(&dest,&colour,d,&dummy,&dummy,&dummy2,&length,&overflow);
                add_bg = 1;
	     }
             if(ansiflags & TXT_INVERSE) {
                colour = ANSI_IBLACK;
                output_ansi_filter(&dest,&colour,d,&dummy,&dummy,&dummy2,&length,&overflow);
                add_bg = 1;
	     }
             output_terminate(&dest,d->flags & TERMINATOR_MASK,&length,&overflow);
             d->terminal_xpos = 0, wrap_leading = cached_wrapleading, leading = wrap_leading;
             if(!wrap_leading) leading = 0;
		else if(*src && (*src == ' ')) ls = 0;
             if(!wrap_leading && (*src == ' ')) blank = 1;
                      
             /* ---->  Leading spaces on next line down  <---- */
             if(*src && output_text_remaining(src)) {
                for(loop = 0; (loop < leading) && (--length > 0); loop++)
                    *dest++ = ' ', d->terminal_xpos++;
                if(length <= 0) length = 0, overflow = 1;
	     }
	  }
          sc = 0, wc = 0, rc = 0, wp = src, sp = src, ansi = NULL;

          /* ---->  Count leading spaces  <---- */
          while(*src && !overflow && ((*src == ' ') || (*src == '\x1B') || (*src == '\x05') || (*src == '\x06')))
                switch(*src) {
                       case '\x05':  /* ---->  Hanging indent control  <---- */
                            src++, wp++;
                            if(*src) {
                               if(*src != '\x0B') {
                                  wrap_leading += *src;
                                  if(wrap_leading > 70) wrap_leading = 70;
			       } else wrap_leading = 0;
                               leading = wrap_leading;
                               src++, wp++;
			    }
                            break;
                       case '\x06':  /*  ---->  Skip rest of line  <---- */
                            for(; *src && (*src != '\n'); src++, wp++);
                            sc = 0;
                            break;
                       case '\x1B':  /* ---->  ANSI code  <---- */
                            if(!ansi) ansi = src;
                            for(; *src && (*src != 'm'); src++, wp++);
                            if(*src && (*src == 'm')) src++, wp++;
                            break;
                       default:
                            src++, wp++, sc++;
		}

          /* ---->  Count length of word  <---- */
          while(*src && !overflow && !((*src == ' ') || (*src == '\n')))
                switch(*src) {
                       case '\x05':  /* ---->  Hanging indent control  <---- */
                            if(*(++src)) {
                               if(*src != '\x0B') {
                                  wrap_leading += *src;
                                  if(wrap_leading > 70) wrap_leading = 70;
			       } else wrap_leading = 0;
                               leading = wrap_leading;
                               src++;
			    }
                            break;
                       case '\x06':  /*  ---->  Skip rest of line  <---- */
                            for(; *src && (*src != '\n'); src++);
                            break;
                       case '\x1B':  /* ---->  ANSI code  <---- */
                            while(*src && (*src != 'm')) src++;
                            if(*src && (*src == 'm')) src++;
                            break;
		       default:
                            wc++, src++;
		}
          if(!rc) rc = wc;

          /* ---->  Hanging indent  <---- */
          if(blank && !wrap_leading) {
             blank = 0, leading = sc;
             if(leading > ((d->terminal_width / 3) * 2)) leading = (d->terminal_width / 2);
	  } else if(blank) {
             blank = 0, leading = wrap_leading;
             if(leading > ((d->terminal_width / 3) * 2)) leading = (d->terminal_width / 2);
	  }

          /* ---->  Add spaces to returned text  <---- */
          loop = 0, ptr = dest;
          if(ls && !overflow) {
             if(ansi && start) {
                do {
                   while(*ansi && (*ansi == ' ')) ansi++;
                   if(*ansi) switch(*ansi) {
                      case '\x05':

                           /* ---->  Hanging indent control  <---- */
                           if(*(++ansi)) ansi++;
                           break;
                      case '\x06':

                           /* ---->  Skip rest of line  <---- */
                           for(; *ansi && (*ansi != '\n'); ansi++);
                           break;
                      case '\x1B':

                           /* ---->  ANSI code  <---- */
                           output_ansi_filter(&dest,&ansi,d,&cur_bg,&cur_fg,&ansiflags,&length,&overflow);
                           break;
		   }
		} while(*ansi && !overflow && ((*ansi == '\x1B') || (*ansi == ' ') || (*ansi == '\x05') || (*ansi == '\x06')));
                ansi = NULL;
	     }

             while((loop < sc) && !overflow)
                   if(d->terminal_xpos >= d->terminal_width) {
                      line++, dest = ptr;
                      if((ansiflags & TXT_BLINK) || (ansiflags & TXT_UNDERLINE)) {
                         colour = ANSI_DWHITE;
                         output_ansi_filter(&dest,&colour,d,&dummy,&dummy,&dummy2,&length,&overflow);
                         add_bg = 1;
		      }
                      if(ansiflags & TXT_INVERSE) {
                         colour = ANSI_IBLACK;
                         output_ansi_filter(&dest,&colour,d,&dummy,&dummy,&dummy2,&length,&overflow);
                         add_bg = 1;
		      }
                      output_terminate(&dest,d->flags & TERMINATOR_MASK,&length,&overflow);
                      d->terminal_xpos = 0;

                      /* ---->  Leading spaces on next line down  <---- */
                      for(loop = 0; (loop < leading) && (--length > 0); loop++)
                          *dest++ = ' ', d->terminal_xpos++;
                      if(length <= 0) length = 0, overflow = 1;
                      loop = sc;
		   } else if((length - 1) > 0) *dest++ = ' ', d->terminal_xpos++, loop++, length--;
                      else overflow = 1;
	  }
          start = 0, ls = 1;

          /* ---->  Will word fit on current line, or will it have to be wrapped?  <---- */
          if(!((line == 1) && (rc >= d->terminal_width)) && (*wp != '\n') && ((d->terminal_xpos + rc) > d->terminal_width)) {
             line++, dest = ptr;
             if((ansiflags & TXT_BLINK) || (ansiflags & TXT_UNDERLINE)) {
                colour = ANSI_DWHITE;
                output_ansi_filter(&dest,&colour,d,&dummy,&dummy,&dummy2,&length,&overflow);
                add_bg = 1;
	     }
             if(ansiflags & TXT_INVERSE) {
                colour = ANSI_IBLACK;
                output_ansi_filter(&dest,&colour,d,&dummy,&dummy,&dummy2,&length,&overflow);
                add_bg = 1;
	     }
	     output_terminate(&dest,d->flags & TERMINATOR_MASK,&length,&overflow);
             d->terminal_xpos = 0;

	     /* ---->  Leading spaces on next line down  <---- */
             for(loop = 0; (loop < leading) && (--length > 0); loop++)
                 *dest++ = ' ', d->terminal_xpos++;
             if(length <= 0) length = 0, overflow = 1;
	  }

          /* ---->  Add word to returned text  <---- */
          if(ansi) do {
             while(*ansi && (*ansi == ' ')) ansi++;
             if(*ansi) switch(*ansi) {
                case '\x05':

                     /* ---->  Hanging indent control  <---- */
                     if(*(++ansi)) ansi++;
                     break;
                case '\x06':

                     /*  ---->  Skip rest of line  <---- */
                     for(; *ansi && (*ansi != '\n'); ansi++);
                     break;
                case '\x1B':

                     /* ---->  ANSI code  <---- */
                     output_ansi_filter(&dest,&ansi,d,&cur_bg,&cur_fg,&ansiflags,&length,&overflow);
                     break;
	     }
	  } while(*ansi && !overflow && ((*ansi == '\x1B') || (*ansi == ' ') || (*ansi == '\x05') || (*ansi == '\x06')));

          if(*wp && !((*wp == ' ') || (*wp == '\n'))) {
             while(*wp && !overflow && !((*wp == ' ') || (*wp == '\n'))) {
                   if((d->terminal_xpos >= d->terminal_width) && (*wp != '\x1B') && (*wp != '\x05') && (*wp != '\x06')) {
                      line++;
                      if((ansiflags & TXT_BLINK) || (ansiflags & TXT_UNDERLINE)) {
                         colour = ANSI_DWHITE;
                         output_ansi_filter(&dest,&colour,d,&dummy,&dummy,&dummy2,&length,&overflow);
                         add_bg = 1;
		      }
                      if(ansiflags & TXT_INVERSE) {
                         colour = ANSI_IBLACK;
                         output_ansi_filter(&dest,&colour,d,&dummy,&dummy,&dummy2,&length,&overflow);
                         add_bg = 1;
		      }
                      output_terminate(&dest,d->flags & TERMINATOR_MASK,&length,&overflow);
                      d->terminal_xpos = 0;
                
                      /* ---->  Leading spaces on next line down  <---- */
                      if(*wp && output_text_remaining(wp)) {
                         for(loop = 0; (loop < leading) && (--length > 0); loop++)
                             *dest++ = ' ', d->terminal_xpos++;
                         if(length <= 0) length = 0, overflow = 1;
		      }
		   } else switch(*wp) {
 	   	      case '\x05':

                           /* ---->  Hanging indent control (Already dealt with earlier)  <---- */
                           if(*(++wp)) wp++;
                           break;
	              case '\x06':

                           /* ---->  Skip rest of line  <---- */
                           for(; *wp && (*wp != '\n'); wp++);
                           if(*wp && (*wp == '\n')) wp++;
                           break;
                      case '\x1B':

                           /* ---->  ANSI code  <---- */
                           output_ansi_filter(&dest,&wp,d,&cur_bg,&cur_fg,&ansiflags,&length,&overflow);
                           break;
                      default:
                           if(add_bg) {
                              ptr = ansibuf;
                              if((ansiflags & TXT_BLINK) || (ansiflags & TXT_UNDERLINE) || (!(d->flags & ANSI_MASK) && (ansiflags & TXT_INVERSE)))
                                 ptr[0] = cur_fg[0], ptr[1] = cur_fg[1], ptr[2] = cur_fg[2], ptr[3] = cur_fg[3], ptr[4] = cur_fg[4], ptr[5] = cur_fg[5], ptr[6] = 'm', ptr += 7;
                              if(cur_bg[3] != '0') ptr[0] = cur_bg[0], ptr[1] = cur_bg[1], ptr[2] = cur_bg[2], ptr[3] = cur_bg[3], ptr[4] = 'm', ptr += 5;
                              *ptr = '\0', ptr = ansibuf;
                              while(*ptr && !overflow)
                                    output_ansi_filter(&dest,&ptr,d,&dummy,&dummy,&dummy2,&length,&overflow);
                              if((ansiflags & TXT_BLINK) || (ansiflags & TXT_UNDERLINE) || (!(d->flags & ANSI_MASK) && (ansiflags & TXT_INVERSE))) {
                                 if(ansiflags & TXT_BOLD) {
                                    colour = ANSI_LIGHT;
                                    output_ansi_filter(&dest,&colour,d,&dummy,&dummy,&dummy2,&length,&overflow);
				 }
                                 if(ansiflags & TXT_BLINK) {
                                    colour = ANSI_BLINK;
                                    output_ansi_filter(&dest,&colour,d,&dummy,&dummy,&dummy2,&length,&overflow);
				 }
                                 if(ansiflags & TXT_UNDERLINE) {
                                    colour = ANSI_UNDERLINE;
                                    output_ansi_filter(&dest,&colour,d,&dummy,&dummy,&dummy2,&length,&overflow);
				 }
			      }
                              add_bg = 0;
			   }
  		           if((length - 1) > 0) *dest++ = *wp, d->terminal_xpos++, wp++;
                              else overflow = 1;
		   }
	     }

             /* ---->  Skip trailing spaces  <---- */
             if(*src && (*src == ' ')) {
                for(sp = src; *sp && (*sp == ' '); sp++);
                if(*sp && (*sp == '\n'))
                   for(; *src && (*src == ' '); src++);
	     }
	  }
    }

    /* ---->  Return to normal text colour (White text on black background)  <---- */
    if((add_cr == 1) && !overflow) {
       output_terminate(&dest,d->flags & TERMINATOR_MASK,&length,&overflow);
       if(!overflow) d->terminal_xpos = 0;
    }

    if(dest > textbuf) {
       if((*(dest - 1) == '\n') || (*(dest - 1) == '\r')) {
          for(loop = 0, dest--; (dest >= textbuf) && ((*dest == '\n') || (*dest == '\r')); dest--, loop++);
          if(((d->flags & TERMINATOR_MASK) == LFTOCR_LFCR) || ((d->flags & TERMINATOR_MASK) == LFTOCR_CRLF)) loop /= 2;
          ptr = ANSI_NORMAL, dest++;
          while(*ptr && !overflow)
                output_ansi_filter(&dest,&ptr,d,&cur_bg,&cur_fg,&ansiflags,&length,&overflow);
          for(; (loop > 0) && !overflow; loop--)
              output_terminate(&dest,d->flags & TERMINATOR_MASK,&length,&overflow);
       } else {
          ptr = ANSI_NORMAL;
          while(*ptr && !overflow)
                output_ansi_filter(&dest,&ptr,d,&cur_bg,&cur_fg,&ansiflags,&length,&overflow);
       }
    }

    if((add_cr == 255) && ((length - 2) > 0)) *dest++ = IAC, *dest++ = EOR, length -= 2;
    *dest = '\0';

    wrap_leading = cached_wrapleading;
    return(server_queue_output(d,textbuf,strlen(textbuf)));
}

/* ---->  Add string to output queue  <---- */
int output_queue_string(struct descriptor_data *d,const char *str,unsigned char redirect)
{
    static int length;

    if(!d) {
       writelog(BUG_LOG,1,"BUG","(output_queue_string() in output.c)  String '%s' queued with NULL descriptor_data.",str);
       return(0);
    }

    if(redirect && d->monitor && (d->flags & MONITOR_OUTPUT))
       output_queue_string(d->monitor,str,0);

    if(IsHtml(d)) {
       if(*str) {
          text_to_html(d,str,textbuf,&length,sizeof(textbuf) - 1);
          if(length > 0) server_queue_output(d,textbuf,length);
       }
       if(!(command_type & LARGE_SUBSTITUTION)) {
          while(d->html->tag) {
                text_to_html_close_tags(d,textbuf,&length,sizeof(textbuf) - 1);
                if(length > 0) server_queue_output(d,textbuf,length);
	  }
          text_to_html_reset(d,textbuf,&length);
          if(length > 0) server_queue_output(d,textbuf,length);
       }
       return(1);
    } else {
       if(*str || add_cr) {
          if(d->terminal_width) return(output_queue_wrapped(d,(char *) str));
             else return(output_queue_nonwrapped(d,(char *) str));
       }
       if(!(command_type & LARGE_SUBSTITUTION))
          d->flags2 &= ~HTML_TAG;
    }
    return(0);
}

/* ---->  Return most recently used descriptor which is in use by PLAYER  <---- */
struct descriptor_data *getdsc(dbref player)
{
       struct descriptor_data *d,*recent = NULL;

       if(!Validchar(player)) return(NULL);
       for(d = descriptor_list; d; d = d->next)
           if((d->player == player) && (!IsHtml(d) || (d->html->flags & HTML_OUTPUT)) && (!recent || (d->last_time >= recent->last_time)))
              recent = d;
       return(recent);
}

/* ---->  Format and queue string for output to descriptor  <---- */
/*        (RAW:       0 = output_queue_string() + ADD_CR, 1 = server_queue_output(), 2 = output_queue_string() only)  */
/*        (REDIRECT:  0 = No redirection, 1 = Redirect, 2 = Redirect HTML only.)  */
/*        (WRAP:      > 0 = Adjust WRAP_LEADING to given value.)  */
void output(struct descriptor_data *d,dbref player,unsigned char raw,unsigned char redirect,unsigned char wrap,char *fmt, ...)
{
     static  char buffer[2 * BUFFER_LEN];
     int     count,va_alloc = 0;

     if(d && (d->flags2 & OUTPUT_SUPPRESS)) return;
     if(fmt) {
        va_start(output_ap,fmt);
        va_alloc = 1;
     }

     if(!d && fmt) {
        if(Validchar(player) && redirect && (player == redirect_src) && (redirect_src != redirect_dest)) {
           struct descriptor_data *r = getdsc(redirect_dest);

           if(r && ((redirect != 2) || IsHtml(r))) {
              if(fmt) output_fmt = &fmt;
              output(r,NOTHING,raw,0,wrap,NULL);
	   }
	}
        if(va_alloc) va_end(output_ap);
        return;
     } else if(!fmt && !output_fmt) {
        if(va_alloc) va_end(output_ap);
        return;
     }

     if(fmt) count = vsnprintf(buffer,sizeof(buffer),fmt,output_ap);
        else count = vsnprintf(buffer,sizeof(buffer),*output_fmt,output_ap);
     if(count < 0) {
        buffer[sizeof(buffer) - 1] = '\0';
     } else if(!raw) {
        buffer[count]     = '\n';
        buffer[count + 1] = '\0';
     }

     if(raw != 1) {
        if(wrap > 0) wrap_leading = wrap;
        output_queue_string(d,buffer,0);
        if(wrap > 0) wrap_leading = 0;
     } else server_queue_output(d,buffer,strlen(buffer));

     /* ---->  Output redirection (Via '@force' and '@monitor')  <---- */
     if(redirect) {
        if((d->player == redirect_src) && (redirect_src != redirect_dest) && !(d->monitor && (d->flags & MONITOR_OUTPUT) && (redirect_dest == d->monitor->player))) {
           struct descriptor_data *r = getdsc(redirect_dest);

           if(r && ((redirect != 2) || IsHtml(r))) {
              if(fmt) output_fmt = &fmt;
              output(r,NOTHING,raw,0,wrap,NULL);
	   }
	}

        if(d->monitor && (d->flags & MONITOR_OUTPUT) && ((redirect != 2) || d->monitor->html)) {
           if(fmt) output_fmt = &fmt;
           output(d->monitor,NOTHING,raw,0,wrap,NULL);
	}
     }
     if(va_alloc) va_end(output_ap);
}

/* ---->  Output to everyone in AREA, except characters NAME1 and NAME2  <---- */
void output_except(dbref area,dbref name1,dbref name2,unsigned char raw,unsigned char redirect,unsigned char wrap,char *fmt, ...)
{
     struct descriptor_data *d;

     va_start(output_ap,fmt);
     output_fmt = &fmt;
     for(d = descriptor_list; d; d = d->next)
         if((d->flags & CONNECTED) && Validchar(d->player) && (db[d->player].location == area) && (d->player != name1) && (d->player != name2))
            output(d,d->player,raw,redirect,wrap,NULL);
     va_end(output_ap);
}

/* ---->  Output to users of given chatting channel  <---- */
void output_chat(int channel,dbref exception,unsigned char raw,unsigned char redirect,char *fmt, ...)
{
     struct descriptor_data *d;

     va_start(output_ap,fmt);
     output_fmt = &fmt;
     for(d = descriptor_list; d; d = d->next)
         if((d->flags & CONNECTED) && Validchar(d->player) && (d->player != exception) && (d->channel == channel))
            output(d,d->player,raw,redirect,8,NULL);
     va_end(output_ap);
}

/* ---->  Output to Admin (Apprentice Wizards/Druids and above) only  <---- */
void output_admin(unsigned char quiet,unsigned char raw,unsigned char redirect,unsigned char wrap,char *fmt, ...)
{
     struct descriptor_data *d;

     va_start(output_ap,fmt);
     output_fmt = &fmt;
     for(d = descriptor_list; d; d = d->next)
         if((d->flags & CONNECTED) && Validchar(d->player) && Level4(d->player) && (!quiet || !Quiet(d->player)))
            output(d,d->player,raw,redirect,wrap,NULL);
     va_end(output_ap);
}

/* ---->  Output trace sequence of compound command  <---- */
unsigned char output_trace(dbref player,dbref command,unsigned char raw,unsigned char redirect,unsigned char wrap,char *fmt, ...)
{
	 int                      cached_command_type = command_type;
	 unsigned char            delivered = 0,suppress;
	 struct   descriptor_data *d;

	 va_start(output_ap,fmt);
	 output_fmt = &fmt;
	 command_type |= NO_AUTO_FORMAT;

	 if(!(in_command && Valid(command) && (Typeof(command) == TYPE_COMMAND) && Validchar(db[command].owner))) command = player;
	 for(d = descriptor_list; d; d = d->next)
	     if((d->flags & CONNECTED) && Validchar(d->player) && (((Typeof(command) == TYPE_CHARACTER) && (d->player == command) && Tracing(d->player)) || ((Typeof(command) != TYPE_CHARACTER) && (Tracing(command) || Tracing(d->player)) && ((d->player == db[command].owner) || (Uid(d->player) == db[command].owner))))) {
		if(!fmt) {
		   command_type = cached_command_type;
		   va_end(output_ap);
		   return(1);
		}
		suppress = ((d->flags2 & OUTPUT_SUPPRESS) != 0);
		d->flags2 &= ~OUTPUT_SUPPRESS;
		if(!d->pager && !IsHtml(d) && More(d->player)) pager_init(d);
		output(d,d->player,raw,redirect,wrap,NULL);
		if(suppress) d->flags2 |= OUTPUT_SUPPRESS;
		delivered = 1;
	     }

	 command_type = cached_command_type;
	 va_end(output_ap);
	 return(delivered);
}

/* ---->  Output to every connected character  <---- */
void output_all(unsigned char users,unsigned char admin,unsigned char raw,unsigned char wrap,char *fmt, ...)
{
     struct descriptor_data *d;

     va_start(output_ap,fmt);
     output_fmt = &fmt;
     for(d = descriptor_list; d; d = d->next)
         if((d->flags & CONNECTED) && Validchar(d->player))
            if((users && !Level4(d->player)) || (admin && Level4(d->player)))
               output(d,d->player,raw,0,wrap,NULL);
     va_end(output_ap);
}

/* ---->  Return terminal width of specified character  <---- */
int output_terminal_width(dbref player)
{
    struct descriptor_data *p = getdsc(player);

    if(p && (p->terminal_width > 1)) {
       return(p->terminal_width);
    }
    return(79);
}

/* ---->  Output list of items in column format  <---- */
void output_columns(struct descriptor_data *p,dbref player,const char *text,const char *colour,int setwidth,unsigned char setleading,unsigned char setitemlen,unsigned char setpadding,unsigned char setalignleft,unsigned char settable,char option,int setlines,const char *setnonefound,char *buffer)
{
     static   unsigned char leading,itemlen,padding,alignleft,table,percent,scrheight;
     static   int           width,column,columns,itemcount,lines;
     static   const    char *nonefound = NULL;
     unsigned          char loop;
     char                   *ptr;

     html_anti_reverse(p,1);
     switch(option) {
            case FIRST:
                 if(IsHtml(p)) leading = 0, padding = 0;
                 lines     = setlines;
                 width     = setwidth;
                 table     = settable;
                 column    = 0;
                 itemlen   = setitemlen;
                 leading   = setleading;
                 padding   = setpadding;
                 columns   = ((width - leading + padding) / (itemlen + padding));
                 percent   = ((double) 1 / columns) * 100;
                 alignleft = setalignleft;
                 itemcount = 0;
                 nonefound = setnonefound;
                 if(lines) {
                    scrheight = db[player].data->player.scrheight;
                    db[player].data->player.scrheight = ((db[player].data->player.scrheight - lines) * columns) * 2;
		 } else scrheight = 0;
                 if(!IsHtml(p)) {
                    for(loop = 0, ptr = buffer; loop < leading; *ptr++ = ' ', loop++);
                    *ptr = '\0';
		 } else {
                    output(p,player,1,0,2,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(table) ? "<TR><TD ALIGN=CENTER VALIGN=CENTER>":"");
		    sprintf(buffer,"\016<TR ALIGN=%s>\016",(alignleft) ? "LEFT":"CENTER");
		 }
                 break;
            case LAST:
                 if(column != 0) {
                   if(IsHtml(p)) {
                      while(++column <= columns) sprintf(buffer + strlen(buffer),"\016<TD WIDTH=%d%%>&nbsp;</TD>\016",percent);
                      sprintf(buffer + strlen(buffer),"\016</TR></TABLE>%s\016",(table) ? "</TD></TR>":"");
		   } else strcat(buffer,"\n");
                   output(p,player,2,1,0,"%s",buffer);
		 } else if(!itemcount && nonefound) {
                   if(IsHtml(p)) {
                      output(p,player,2,1,0,"\016<TR><TH ALIGN=CENTER BGCOLOR="HTML_TABLE_BLACK">"ANSI_LRED"<FONT SIZE=+1><B><I>\016%s\016</I></B></FONT></TH></TR></TABLE>%s\016",nonefound,(table) ? "</TD></TR>":"");
		   } else output(p,player,0,leading,0,"%s"ANSI_LRED"%s",strpad(' ',leading,buffer),nonefound);
		 } else if(IsHtml(p)) output(p,player,2,1,0,"\016<TD>&nbsp;</TD></TR></TABLE>%s",(table) ? "</TD></TR>":"");
                 if(scrheight) db[player].data->player.scrheight = scrheight;
                 break;
            default:

                 /* ---->  Output row, if columns/row reached  <---- */
                 itemcount++;
                 if(++column > columns) {
                    strcat(buffer,IsHtml(p) ? "\016</TR>\016":"\n");
		    output(p,player,2,1,0,"%s",buffer);
                    if(!IsHtml(p)) {
                       for(loop = 0, ptr = buffer; loop < leading; *ptr++ = ' ', loop++);
                       *ptr = '\0';
		    } else sprintf(buffer,"\016<TR ALIGN=%s>\016",(alignleft) ? "LEFT":"CENTER");
                    column = 1;
		 }

                 /* ---->  Copy item text (Truncate, if too large)  <---- */
                 if(IsHtml(p)) sprintf(buffer + strlen(buffer),"\016<TD WIDTH=%d%%>\016%s",percent,!Blank(colour) ? colour:"");
                    else if(!Blank(colour)) strcat(buffer,colour);
                 loop = 0, ptr = buffer + strlen(buffer);

                 if(!IsHtml(p)) {

                    /* ---->  Copy item (Truncate if neccessary)  <---- */
                    if(!Blank(text)) {
                       while(*text && (loop < itemlen)) {
                             if(*text == '\x1B') {
                                while(*text && (*text != 'm')) *ptr++ = *text++;
                                if(*text && (*text == 'm')) *ptr++ = *text++;
			     } else {
                                *ptr++ = *text++;
                                loop++;
			     }
		       }
		    }
 
                    /* ---->  Pad text with spaces  <---- */
                    if(!IsHtml(p) && (column < columns))
                       for(; loop < (itemlen + padding); *ptr++ = ' ', loop++);
                    *ptr = '\0';
		 } else sprintf(buffer + strlen(buffer),"%s\016</TD>\016",Blank(text) ? "\016&nbsp;\016":text);
     }
     html_anti_reverse(p,0);
}

/* ---->  Output connect/disconnect messages to connected characters set LISTEN  <---- */
/*        (ACTION:  0 = Disconnect, 1 = Connect,   2 = Create,         */
/*                  3 = No message, 4 = Reconnect, 5 = Disconnected.)  */
void output_listen(struct descriptor_data *d,int action)
{
     struct descriptor_data *ptr;
     short  flags,flags2;
     const  char *colour;

     /* ---->  Update last site character connected from  <---- */
     if((action != 0) && (action != 5) && (action != 3) && d->hostname)
	if(!getfield(d->player,LASTSITE) || strcasecmp(d->hostname,getfield(d->player,LASTSITE)))
           setfield(d->player,LASTSITE,d->hostname,1);
     if(action == 3) return;

     for(ptr = descriptor_list; ptr; ptr = ptr->next) {
         flags  = friend_flags(ptr->player,d->player);
         flags2 = friend_flags(d->player,ptr->player)|flags;
         if((ptr->flags & CONNECTED) && (((Level4(ptr->player) || ((Help(ptr->player) || Assistant(ptr->player)) && !Quiet(ptr->player))) && (action == 2)) || (Listen(ptr->player) && !(ptr->flags & NOLISTEN)) || ((flags & FRIEND_INFORM) && ((flags & FRIEND_BEEP) || !Quiet(ptr->player)))) && (ptr->player != d->player)) {
            if(flags & FRIEND_INFORM) colour = (flags2 & FRIEND_ENEMY) ? ANSI_LRED:ANSI_LGREEN;
               else colour = ANSI_LYELLOW;
            if(Level4(ptr->player)) {
               switch(action) {
                      case 1:
                      case 4:  /*  Connect/reconnect  */
                           output(ptr,ptr->player,0,0,0,"%s%s[%s"ANSI_LWHITE"%s%s%s%s has %sconnected from "ANSI_LWHITE"%s%s.]",(flags & FRIEND_BEEP) ? "\007":"",colour,Article(d->player,UPPER,INDEFINITE),getcname(NOTHING,d->player,0,0),colour,(flags2) ? (flags2 & FRIEND_ENEMY) ? " (Enemy)":" (Friend)":"",Level4(d->player) ? " (Admin)":Retired(d->player) ? (RetiredDruid(d->player) ? " (Retired Druid)":" (Retired Wizard)"):Experienced(d->player) ? " (Experienced Builder)":Assistant(d->player) ? " (Assistant)":"",(action == 4) ? "re":"",d->hostname,colour);
                           break;
                      case 2:  /*  Create  */
                           output(ptr,ptr->player,0,0,0,ANSI_LYELLOW"[%s"ANSI_LWHITE"%s"ANSI_LYELLOW" has been created from "ANSI_LWHITE"%s"ANSI_LYELLOW".]",Article(d->player,UPPER,INDEFINITE),getcname(NOTHING,d->player,0,0),d->hostname);
                           break;
                      case 5:  /*  Disconnected (Connection lost)  */
                           output(ptr,ptr->player,0,0,0,"%s[%s"ANSI_LWHITE"%s%s%s%s has lost %s connection.]",colour,Article(d->player,UPPER,INDEFINITE),getcname(NOTHING,d->player,0,0),colour,(flags2) ? (flags2 & FRIEND_ENEMY) ? " (Enemy)":" (Friend)":"",Level4(d->player) ? " (Admin)":Retired(d->player) ? (RetiredDruid(d->player) ? " (Retired Druid)":" (Retired Wizard)"):Experienced(d->player) ? " (Experienced Builder)":Assistant(d->player) ? " (Assistant)":"",Possessive(d->player,0));
                           break;
                      default:  /*  Disconnect (QUIT/timed out)  */
                           output(ptr,ptr->player,0,0,0,"%s[%s"ANSI_LWHITE"%s%s%s%s has disconnected.]",colour,Article(d->player,UPPER,INDEFINITE),getcname(NOTHING,d->player,0,0),colour,(flags2) ? (flags2 & FRIEND_ENEMY) ? " (Enemy)":" (Friend)":"",Level4(d->player) ? " (Admin)":Retired(d->player) ? (RetiredDruid(d->player) ? " (Retired Druid)":" (Retired Wizard)"):Experienced(d->player) ? " (Experienced Builder)":Assistant(d->player) ? " (Assistant)":"");
	       }
	    } else switch(action) {
               case 1:
               case 4:  /*  Connect/reconnect  */
                    output(ptr,ptr->player,0,0,0,"%s%s[%s"ANSI_LWHITE"%s%s%s%s has %sconnected.]",(flags & FRIEND_BEEP) ? "\007":"",colour,Article(d->player,UPPER,INDEFINITE),getcname(NOTHING,d->player,0,0),colour,(flags2) ? (flags2 & FRIEND_ENEMY) ? " (Enemy)":" (Friend)":"",Level4(d->player) ? " (Admin)":Retired(d->player) ? (RetiredDruid(d->player) ? " (Retired Druid)":" (Retired Wizard)"):Experienced(d->player) ? " (Experienced Builder)":Assistant(d->player) ? " (Assistant)":"",(action == 4) ? "re":"");
                    break;
               case 2:  /*  Create  */
                    output(ptr,ptr->player,0,0,0,ANSI_LYELLOW"[%s"ANSI_LWHITE"%s"ANSI_LYELLOW" has %s.]",Article(d->player,UPPER,INDEFINITE),getcname(NOTHING,d->player,0,0),(Help(ptr->player) || Assistant(ptr->player)) ? "been created":"connected");
                    break;
               case 5:  /*  Disconnected (Connection lost)  */
                    output(ptr,ptr->player,0,0,0,"%s[%s"ANSI_LWHITE"%s%s%s%s has lost %s connection.]",colour,Article(d->player,UPPER,INDEFINITE),getcname(NOTHING,d->player,0,0),colour,(flags2) ? (flags2 & FRIEND_ENEMY) ? " (Enemy)":" (Friend)":"",Level4(d->player) ? " (Admin)":Retired(d->player) ? (RetiredDruid(d->player) ? " (Retired Druid)":" (Retired Wizard)"):Experienced(d->player) ? " (Experienced Builder)":Assistant(d->player) ? " (Assistant)":"",Possessive(d->player,0));
                    break;
               default:  /*  Disconnect (QUIT/timed out)  */
                    output(ptr,ptr->player,0,0,0,"%s[%s"ANSI_LWHITE"%s%s%s%s has disconnected.]",colour,Article(d->player,UPPER,INDEFINITE),getcname(NOTHING,d->player,0,0),colour,(flags2) ? (flags2 & FRIEND_ENEMY) ? " (Enemy)":" (Friend)":"",Level4(d->player) ? " (Admin)":Retired(d->player) ? (RetiredDruid(d->player) ? " (Retired Druid)":" (Retired Wizard)"):Experienced(d->player) ? " (Experienced Builder)":Assistant(d->player) ? " (Assistant)":"");
	    }
	 }
     }
}

