/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| STRINGUTILS.C  -  Implements string matching, comparison, parsing and       |
|                   formatting functions.                                     |
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
#include <time.h>

#include "command.h"
#include "externs.h"
#include "config.h"

#include "object_types.h"
#include "flagset.h"


/* ---->  Underline a string with tildes ('~') or proper underline if UNDERLINE flag is set  <---- */
void tilde_string(dbref player,const char *str,const char *ansi1,const char *ansi2,unsigned char spaces,unsigned char cr,unsigned char fsize)
{
     struct descriptor_data *p = getdsc(player);
     int    count,cached_wl,cached_tf,twidth;
     const  char *p3;
     char   *p1,*p2;

     cached_tf    = termflags;
     cached_wl    = wrap_leading;
     termflags    = (Underline(player)) ? TXT_BOLD|TXT_UNDERLINE:TXT_BOLD;
     wrap_leading = spaces;
     p1           = scratch_return_string;

     /* ---->  HTML font size  <---- */
     if(IsHtml(p) || (!p && Html(player))) {
        sprintf(p1,"\016<FONT SIZE=%d>\016",fsize);
        p1 += 15;
     }

     /* ---->  Leading spaces  <---- */
     if(cr & 0x1) *p1++ = '\n';
     for(count = 0; count < spaces; count++) *p1++ = ' ';
     for(; *ansi1; ansi1++) *p1++ = *ansi1;
     count = 0;

     /* ---->  Single line title (And character set to UNDERLINE)?  <---- */
     if(Underline(player)) {
        for(p3 = str; *p3 && (*p3 != '\n'); p3++);
        if(!*p3) for(p2 = ANSI_UNDERLINE; *p2; p2++) *p1++ = *p2;
     }

     while(*str) {
           switch(*str) {
                  case '\x1B':
                       for(; *str && (*str != 'm'); str++) *p1++ = *str; 
                       if(*str && (*str == 'm')) *p1++ = *str;
                       if(*(str + 1) && (*str != '\x1B') && Underline(player))
                          for(p2 = ANSI_UNDERLINE; *p2; p2++) *p1++ = *p2;
		       break;
                  case '\n':
                       *p1++ = *str;
                       if(Underline(player)) {
                          for(p3 = (str + 1); *p3 && (*p3 != '\n'); p3++);
                          if(!*p3) for(p2 = ANSI_UNDERLINE; *p2; p2++) *p1++ = *p2;
		       }
                       count = 0;
                       break;
	          default:
                       *p1++ = *str;
                       count++;
	   }
           str++;
     }
     
     /* ---->  Underscores (~~~)  <---- */
     if(!Underline(player)) {
        *p1++ = '\n';
        twidth = output_terminal_width(player);
        if(count > twidth) count = twidth;
        for(; *ansi2; ansi2++) *p1++ = *ansi2;
        for(; count > 0; count--) *p1++ = '~';
        *p1 = '\0';
     } else *p1 = '\0';

     /* ---->  HTML font size  <---- */
     if(IsHtml(p) || (!p && Html(player))) {
        strcpy(p1,"\016</FONT>\016");
        p1 += 9;
     }

     output(p,player,0,1,0,"%s",scratch_return_string);
     wrap_leading = cached_wl, termflags = cached_tf;
}

/* ---->  Construct output for spoken message  <---- */
const char *construct_message(dbref player,const char *ansi1,const char *ansi2,const char *defaction,char defpunct,char pose,char autoaction,const char *message,unsigned char tell,unsigned char article_setting)
{
      unsigned char multi = 0;
      char          punct;
      const    char *ptr;

      /* ---->  Negotiate pose/think?  <---- */
      if((pose < 0) && ((*message == *POSE_TOKEN) || (*message == *THINK_TOKEN) || (*message == *ALT_POSE_TOKEN) || (*message == *ALT_THINK_TOKEN))) {
          if((*message == *POSE_TOKEN) || (*message == *ALT_POSE_TOKEN)) pose = 1;
             else pose = 2;
         message++;
      } else if(pose < 0) pose = 0;

      /* ---->  Punctuate and format message  <---- */
      if(pose != 1) {
         substitute(player,scratch_return_string,punctuate((char *) message,1,defpunct),0,ansi1,NULL,0);
         if((autoaction == PLAYER) || (autoaction == -1)) sprintf(scratch_buffer,"%sYou",ansi2);
            else sprintf(scratch_buffer,ANSI_LWHITE"%s%s",Article(player,UPPER,article_setting),getcname(NOTHING,player,0,0));
      } else substitute(player,scratch_buffer,punctuate((char *) message,0,defpunct),article_setting,ansi2,NULL,0);

      if(pose == 2) {
         if((autoaction == PLAYER) || (autoaction == -1)) sprintf(scratch_buffer + strlen(scratch_buffer),"%s think ",ansi2);
             else sprintf(scratch_buffer + strlen(scratch_buffer),"%s thinks ",ansi2);
         if(*message) sprintf(scratch_buffer + strlen(scratch_buffer),ANSI_LYELLOW" . o O ( %s"ANSI_LYELLOW" )",scratch_return_string);
	    else strcat(scratch_buffer,"about something that isn't important.");
      } else if(!pose) {

         /* ---->  Determine terminating punctuation  <---- */
         ptr = scratch_return_string;
         if(*ptr && (autoaction > 0)) {
            while(*ptr) ptr++;
            punct = *(--ptr);
            if((tell != 2) && (ptr > scratch_return_string) && (*(ptr - 1) == punct)) multi = 1;
	 } else punct = '.';
         sprintf(scratch_buffer + strlen(scratch_buffer),"%s ",ansi2);

         switch(punct) {
                case '?':
                     if((autoaction == PLAYER) || (autoaction == -1)) strcat(scratch_buffer,"ask \"");
                        else strcat(scratch_buffer,"asks \"");
                     break;
                case '!':
                     if((autoaction == PLAYER) || (autoaction == -1)) strcat(scratch_buffer,(multi) ? "shout \"":"exclaim \"");
                        else strcat(scratch_buffer,(multi) ? "shouts \"":"exclaims \"");
                     break;
                default:
                     if(Blank(defaction)) {
                        if((autoaction == PLAYER) || (autoaction == -1)) strcat(scratch_buffer,"say \"");
                           else strcat(scratch_buffer,"says \"");
		     } else sprintf(scratch_buffer + strlen(scratch_buffer),"%s \"",defaction);
	 }

         sprintf(scratch_buffer + strlen(scratch_buffer),"%s%s\"",scratch_return_string,ansi2);
         if(tell == 1) {
            if(punct == '?') strcat(scratch_buffer," of ");
               else if(multi && (punct == '!')) strcat(scratch_buffer," at ");
	          else strcat(scratch_buffer," to ");
	 }
      }
      return(scratch_buffer);
}

/* ---->  Return correct colour for character, and increment count for appropriate rank  <---- */
const char *privilege_countcolour(dbref player,short *deities,short *elders,short *delders,short *wizards,short *druids,short *apprentices,short *dapprentices,short *retired,short *dretired,short *experienced,short *assistants,short *builders,short *mortals,short *beings,short *puppets,short *morons)
{
      if(!Validchar(player)) return(ANSI_DWHITE);
      if(Moron(player)) {
         if(morons) (*morons)++;
         return(MORON_COLOUR);
      } else if(Level1(player)) {
         if(deities) (*deities)++;
         return(DEITY_COLOUR);
      } else if(Level2(player)) {
         if(Druid(player)) {
            if(delders) (*delders)++;
            return(ELDER_DRUID_COLOUR);
	 } else {
            if(elders) (*elders)++;
            return(ELDER_COLOUR);
	 }
      } else if(Level3(player)) {
         if(Druid(player)) {
            if(druids) (*druids)++;
            return(DRUID_COLOUR);
	 } else {
            if(wizards) (*wizards)++;
            return(WIZARD_COLOUR);
	 }
      } else if(Level4(player)) {
         if(Druid(player)) {
            if(dapprentices) (*dapprentices)++;
            return(APPRENTICE_DRUID_COLOUR);
	 } else {
            if(apprentices) (*apprentices)++;
            return(APPRENTICE_COLOUR);
	 }
      } else if(Retired(player)) {
         if(RetiredDruid(player)) {
            if(dretired) (*dretired)++;
            return(RETIRED_DRUID_COLOUR);
	 } else {
            if(retired) (*retired)++;
            return(RETIRED_COLOUR);
	 }
      } else if(Experienced(player)) {
	 if(builders)    (*builders)++;
         if(experienced) (*experienced)++;
         return(EXPERIENCED_COLOUR);
      } else if(Assistant(player)) {
         if(assistants) (*assistants)++;
         return(ASSISTANT_COLOUR);
      } else if(Builder(player)) {
	 if(builders) (*builders)++;
	 return(BUILDER_COLOUR);
      } else if(Being(player)) {
         if(beings) (*beings)++;
         return(MORTAL_COLOUR);
      } else if(Puppet(player)) {
         if(puppets) (*puppets)++;
         return(MORTAL_COLOUR);
      } else {
         if(mortals) (*mortals)++;
         return(MORTAL_COLOUR);
      }
}

/* ---->  Return correct colour for character (No rank counting)  <---- */
const char *privilege_colour(dbref player)
{
      return(privilege_countcolour(player,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL));
}

/* ---->  Progress meter  <---- */
/*        (INIT:  0 = Initialise, 1 = Progress, 2 = Finalise.)  */
void progress_meter(unsigned long total,int divisor,unsigned char units,unsigned char init)
{
     static unsigned long lastcount,unitcount;
     static char          buffer[16];
     static time_t        start,end;
     static unsigned char width;

     if(!log_stderr) return;
     switch(init) {
            case 0:
                 width = 14;
                 lastcount = 0;
                 gettime(start);
                 fputs("PROGRESS:  {0}",stderr);
                 break;
            case 1:
                 unitcount = total / (divisor / units);
                 if(unitcount > lastcount) {
                    if(unitcount % units) {
                       if((width + 1) > 79) {
                          fputs("\n           .",stderr);
                          width = 11;
		       } else fputc('.',stderr);
		    } else {
                       sprintf(buffer,"{%ld}",unitcount / units);
                       if((width + strlen(buffer)) > 79) {
                          fprintf(stderr,"\n           %s",buffer);
                          width = 11;
		       } else fputs(buffer,stderr);
		    }
                    lastcount = unitcount;
		 }
	         break;
            case 2:
                 gettime(end);
                 if(end <= start) end = start + 1;
                 fprintf(stderr,"%s[%.2fMb @ %ldKb/sec.]\n",(width > 40) ? "\n           ":"  ",(double) total / MB,(total / KB) / (end - start));
                 break;
     }
}

/* ---->  Get field in internal separated list (If NUMBER is 0, first non-blank field is return)  <---- */
const char *gettextfield(int number,char separator,const char *list,int skip,char *buffer)
{
      int  counter = 0;
      char *tmp;

      if(!list) return(NULL);
      while(*list) {
            for(tmp = buffer; *list && (*list != separator); *tmp++ = *list++);
            *tmp = '\0', counter++;
            if(*list) list++;
            if((number && (counter == number)) || (!number && (counter != skip) && !Blank(buffer))) return(buffer);
      }
      return(NULL);
}

/* ---->  Set field in internal separated list  <---- */
const char *settextfield(const char *text,int number,char separator,const char *list,char *buffer)
{
      int  counter = 1;
      char *tmp;

      if(!text) return(NULL);
      tmp = buffer;
      while((counter <= number) || !Blank(list)) {
            if(counter == number) {
               for(; !Blank(list) && (*list != separator); list++);
               for(; *text; *tmp++ = *text++);
	    } else for(; !Blank(list) && (*list != separator); *tmp++ = *list++);
            if((counter < number) || !Blank(list)) {
               if(!Blank(list)) list++;
               *tmp++ = separator;
	    }
            counter++;
      }
      *tmp = '\0';
      return(buffer);
}

/* ---->  Return length of string, excluding ANSI codes  <---- */
int strlen_ansi(const char *str)
{
    int count = 0;

    while(*str) {
          if(*str == '\x1B') while(*str && (*str != 'm')) str++;
             else count++;
          if(*str) str++;
    }
    return(count);
}

/* ---->  Filter out ANSI codes in given string  <---- */
const char *filter_ansi(const char *str,char *buffer)
{
      char *ptr = buffer;

      while(*str) {
            if(*str == '\x1B') while(*str && (*str != 'm')) str++;
               else *ptr++ = *str;
            if(*str) str++;
      }
      *ptr = '\0';
      return(buffer);
}

/* ---->  Initialise given STR_OPS struct  <---- */
void init_strops(struct str_ops *str_data,char *src,char *dest)
{
     (str_data->backslash) = 0;
     (str_data->length)    = 0;
     (str_data->dest)      = dest;
     (str_data->src)       = src;
}

/* ---->  Insert STR into DEST, making sure total length of DEST doesn't exceed MAX_LENGTH  <---- */
unsigned char strins_limits(struct str_ops *str_data,const char *str)
{
	 int  count,loop,length = (str_data->length);
	 char *start = (str_data->dest);

	 if(!str) return(0);
	 count = strlen(str);
	 start -= length;

	 /* ---->  Shift existing text right by appropriate number of characters  <---- */
	 for(loop = length - 1; loop >= 0; loop--)
	     if((loop + count) < MAX_LENGTH)
		start[loop + count] = start[loop];

	 /* ---->  Insert STR  <---- */
	 for(loop = 0; (loop < count) && (loop < MAX_LENGTH); loop++)
	     start[loop] = str[loop];

	 /* ---->  Sort out length of new string  <---- */
	 if((length += count) > MAX_LENGTH) length = MAX_LENGTH;
	 (str_data->length) = length;
	 (str_data->dest)   = start + length;
	 return(1);
}

/* ---->  Concat string STR to DEST, making sure length of DEST doesn't exceed MAX_LENGTH  <---- */
unsigned char strcat_limits(struct str_ops *str_data,const char *str)
{
	 if(!str) return(0);
	 while(*str && (str_data->length < MAX_LENGTH)) {
	       *(str_data->dest)++ = *str;
	       str_data->length++;
	       str++;

	       if(str_data->length >= MAX_LENGTH)
		  return(0);
	 }
	 return(1);
}

/* ---->  Concat character CHR to string DEST, making sure length of DEST doesn't exceed MAX_LENGTH  <---- */
unsigned char strcat_limits_char(struct str_ops *str_data,const char chr)
{
	 if(!chr) return(0);
	 if(str_data->length < MAX_LENGTH) {
	    *(str_data->dest)++ = chr;
	    str_data->length++;
	    if(str_data->length >= MAX_LENGTH) return(0);
	 }
	 return(1);
}

/* ---->  Concat string STR to DEST, making sure length of DEST doesn't exceed MAX_LENGTH (If it does, concat WILL NOT take place)  <---- */
unsigned char strcat_limits_exact(struct str_ops *str_data,const char *str)
{
	 const char *ptr;
	 int   length;

	 if(!str) return(0);
	 for(ptr = str, length = 0; *ptr; length++, ptr++);
	 if(((str_data->length) + length) <= MAX_LENGTH) {
	    while(*str && (str_data->length < MAX_LENGTH)) {
		  *(str_data->dest)++ = *str;
		  str_data->length++;
		  str++;

		  if(str_data->length >= MAX_LENGTH)
		     return(0);
	    }
	 } else return(0);
	 return(1);
}

/* ---->  Return length of STR (Excluding 'hard' ANSI codes)  <---- */
int strlen_ignansi(const char *src)
{
    int length = 0;

    if(!src) return(0);
    while(*src)
          if(*src == '\x1B') {
             while(*src && (*src != 'm')) src++;
             if(*src && (*src == 'm')) src++;
	  } else length++, src++;
    return(length);
}

/* ---->  Copy COUNT characters of STR (Excluding 'hard' ANSI codes)   <---- */
char *strcpy_ignansi(char *dest,const char *src,int *length,int *rlength,int *copied,int maxlen)
{
     *copied = 0, *dest = '\0';
     if(!src) return(dest);
     while(*src && (*length < maxlen) && (*rlength < MAX_LENGTH))
           if(*src == '\x1B') {
              while(*src && (*src != 'm') && (*rlength < MAX_LENGTH)) *dest++ = *src++, (*rlength)++, (*copied)++;
              if(*src && (*src == 'm') && (*rlength < MAX_LENGTH)) *dest++ = *src++, (*rlength)++, (*copied)++;
	   } else {
              (*length)++, (*rlength)++, (*copied)++;
              *dest++ = *src++;
	   }
     *dest = '\0';
     return(dest);
}

/* ---->  Skip over article prefix in given string (If ARTICLE_SETTING is an article)  <---- */
const char *skip_article(const char *str,unsigned char article_setting,unsigned char definite)
{
      unsigned char hasarticle = 0;

      if(Blank(str)) return(str);
      for(; *str && (*str == ' '); str++);

      /* ---->  Match for article in STR  <---- */
      if((definite || article_setting) && !strncasecmp(str,"The ",4)) hasarticle = 1;
      switch(article_setting) {
             case ARTICLE_CONSONANT:
                  if(!strncasecmp(str,"A ",2)) hasarticle = 1;
                  break;
             case ARTICLE_PLURAL:
                  if(!strncasecmp(str,"Some ",5)) hasarticle = 1;
                  break;
             case ARTICLE_VOWEL:
                  if(!strncasecmp(str,"An ",3)) hasarticle = 1;
                  break;
      }

      /* ---->  Skip article (If found)  <---- */
      if(hasarticle) {
         for(; *str && (*str != ' '); str++);
         for(; *str && (*str == ' '); str++);
      }
      return(str);
}

/* ---->  Automatically punctuate and format given string  <---- */
char *punctuate(char *message,int no_quotes,char punctuation)
{
     int    length = 0,lf_count = 0,html = 0,capitalise = no_quotes;
     static char buffer[BUFFER_LEN];
     char   *p1,*p2,*p3;

     #define istext(x) (isalnum(x) || ((x) == '%'))
     #define abbr_match(str,fchar,len) ((p1 - (len) - 1) >= message) && (toupper(*(p1 - (len) - 1)) == (fchar)) && !strncasecmp(p1 - (len) - 1,(str),(len)) && (((p1 - (len) - 2) < message) || !isalnum(*(p1 - (len) - 2)))

     if(!message) {
        *buffer = '\0';
        return(buffer);
     }
     p1 = message, p2 = buffer;
     if((no_quotes == 1) && (*message == '\"')) while(*p1 && ((*p1 == '\"') || (*p1 == ' '))) p1++;
     while(*p1 && (*p1 == ' ')) p1++;
     if(no_quotes && (*p1 && (isalnum(*p1) || (*p1 == ' ')))) capitalise = 1;
     *p2 = '\0';

     if(*p1) {

        /* ---->  Auto-capitalisation (After punctuation, etc)  <---- */
        while(*p1 && (length < MAX_LENGTH)) {
              if(capitalise && isalnum(*p1)) {
                 *p2++ = toupper(*p1);
                 capitalise = 0;
                 length++;
                 p1++;
	      } else switch(*p1) {
                 case ' ':

                      /* ---->  Space:  Filter out excess blank space  <---- */
                      for(p3 = p1; *p3 && (*p3 == ' '); p3++);  /*  Scan ahead to find out what next character is  */
                      if(*p3) {
                         if(isalnum(*p3) && isalnum(*(p1 - 1))) {
                            while(*p1 && (*p1 == ' ')) p1++;
                            if(*p1 && (*p1 != '\n')) {
                               *p2++ = ' ';
                               length++;
			    }
			 } else while(*p1 && (*p1 == ' ') && (length < MAX_LENGTH))
                            *p2++ = *p1++, length++;

	              } else p1 = p3;
                      break;
                 case '\n':

                      /* ---->  Linefeed:  Filter trailing/leading spaces  <---- */
                      *p2++ = *p1++, length++;
                      if(no_quotes != 3) while(*p1 && (*p1 == ' ')) p1++;
                      break;
                 case '\x0E':

                      /* ---->  Toggle evaluation of HTML tags  <---- */
                      html = !html, *p2++ = *p1++, length++;
                      break;
                 case '\x1B':  

                      /* ---->  'Hard' ANSI code  <---- */
                      while(*p1 && (*p1 != 'm') && (length < MAX_LENGTH)) *p2++ = *p1++, length++;
                      if(*p1 && (length < MAX_LENGTH)) *p2++ = *p1++, length++;
                      break;
                 case ',':

                      /* ---->  Comma:  Add space  <---- */
                      *p2++ = *p1++, length++;
                      if(*p1 && isdigit(*p1)) break;
                      for(p3 = p1; *p3 && (*p3 == ' '); p3++);  /*  Scan ahead to find out what next character is  */
                      if(*p3 && isalnum(*p3)) {
                         if(length < MAX_LENGTH) *p2++ = ' ', length++;
                         while(*p1 && (*p1 == ' ')) p1++;
		      } else while(*p1 && (*p1 == ' ') && (length < MAX_LENGTH))
                         *p2++ = *p1++, length++;
                      break;
                 case '.':
                 case ':':
                 case '!':
                 case '?':
                 case ';':

                      /* ---->  Punctuation:  Handle appropriately (Spaces + capitalisation  -  Handles common abbreviations correctly)  <---- */
                      *p2++ = *p1, length++, p1++;
                      if(istext(*p1) || (((p1 - 2) >= message) && !istext(*(p1 - 2)))) break;
                      for(p3 = p1; *p3 && (*p3 == ' '); p3++);  /*  Scan ahead to find out what next character is  */
                      if(*p3 && istext(*p3)) {

                         /* ---->  Handle common abbreviations (Such as 'Etc.', 'Ltd.', etc.)  <---- */
                         if(((p1 - 2) >= message) && (*(p1 - 1) == '.')) {
                            p3 = NULL; 
                            switch(*(p1 - 2)) {
                                   case 'c':
                                   case 'C':
                                        if(abbr_match("Etc",'E',3)) p3 = p1;
                                           else if(abbr_match("Inc",'I',3)) p3 = p1;
                                              else if(abbr_match("Spec",'S',4)) p3 = p1;
                                        break;
                                   case 'd':
                                   case 'D':
                                        if(abbr_match("Ltd",'L',3)) p3 = p1;
                                        break;
                                   case 'n':
                                   case 'N':
                                        if(abbr_match("Min",'M',3)) p3 = p1;
                                           else if(abbr_match("Admin",'A',5)) p3 = p1;
                                        break;
                                   case 'o':
                                   case 'O':
                                        if(abbr_match("Co",'C',2)) p3 = p1;
                                        break;
                                   case 'p':
                                   case 'P':
                                        if(abbr_match("Esp",'E',3)) p3 = p1;
                                           else if(abbr_match("Exp",'E',3)) p3 = p1;
                                              else if(abbr_match("Corp",'C',4)) p3 = p1;
                                        break;
                                   case 'r':
                                   case 'R':
                                        if(abbr_match("Dr",'D',2)) p3 = p1;
                                        break;
                                   case 't':
                                   case 'T':
                                        if(abbr_match("St",'S',2)) p3 = p1;
                                        break;
                                   case 'x':
                                   case 'X':
                                        if(abbr_match("Max",'M',3)) p3 = p1;
                                        break;
			    }
                            if(p3) break;
			 }
                         if(length < MAX_LENGTH) {
                            *p2++ = ' ', length++;
                            if(length < MAX_LENGTH) *p2++ = ' ', length++;
			 }
                         while(*p1 && (*p1 == ' ')) p1++;
                         capitalise = 1;
		      } else switch(*p1) {
                         case '.':
                         case '!':
                         case '?':
                              capitalise = 1;
                              break;
                         case ' ':
                              *p2++ = *p1++, length++;
                              break;
		      }
                      break;
                 case '%':

                      /* ---->  Substitution  <---- */
                      if(*(p1 + 1) && (*(p1 + 1) == '#') && !strncasecmp(p1 + 1,"#NF#",4)) {

                         /* ---->  Disable automatic formatting/punctuation  <---- */
                         for(p1 += 5; *p1; *p2++ = *p1++);
                         *p2 = '\0';
                         return(buffer);
		      } else if(*(p1 + 1) && ((*(p1 + 1) == 'h') || (*(p1 + 1) == 'H'))) html = !html;
                      *p2++ = *p1++, length++;
                      if(*p1 && (length < MAX_LENGTH)) *p2++ = *p1++, length++;
                      break;
                 case '<':

                      /* ---->  HTML tag  <---- */
                      if(html) {
                         for(; *p1 && (*p1 != '>'); *p2++ = *p1++, length++);
                         if(*p1 && (*p1 == '>')) *p2++ = *p1++, length++;
		      } else *p2++ = *p1++, length++;
                      break;
                 case '&':

                      /* ---->  HTML character entity  <---- */
                      if(html) {
                         for(; *p1 && (*p1 != ';'); *p2++ = *p1++, length++);
                         if(*p1 && (*p1 == ';')) *p2++ = *p1++, length++;
		      } else *p2++ = *p1++, length++;
                      break;
                 case '(':

                      /* ---->  Open bracket:  Capitalise first word  <---- */
                      *p2++ = *p1++, length++;
                      if(*p1 && ((*p1 == ' ') || isalnum(*p1)) && !(*(p1 + 1) && (*(p1 + 1) == ')'))) capitalise = 1;
                      break;
                 case 'i':

                      /* ---->  The word 'I':  Capitalise  <---- */
                      if((*(p1 - 1) == ' ') && (!*(p1 + 1) || !isalnum(*(p1 + 1)))) *p1 = 'I';
                 default:

                      /* ---->  Ordinary text  <---- */
                      *p2++ = *p1++, length++;
	      }
	}
        *p2-- = '\0';

        /* ---->  Move backwards over spaces and linefeeds (Counting LF's) to last character in message  <---- */
        while((p2 != buffer) && ((*p2 == ' ') || (*p2 == '\n'))) {
              if(*p2 == '\n') lf_count++;
              length--, p2--;
	}

        /* ---->  Skip backwards over double-quotes if terminating double-quotes aren't wanted (E.g:  'say')  <---- */ 
        if(no_quotes == 1) while((p2 != buffer) && (*p2 == '\"')) length--, p2--;

        /* ---->  Add terminating punctuation (If necessary)  <---- */
        if(*p2 && (punctuation != '\0') && isalnum(*p2) && (length < MAX_LENGTH))
           p2++, *p2++ = punctuation, length++;
	      else p2++;

        /* ---->  Add trailing linefeeds (If any)  <---- */
        while(lf_count && (length < MAX_LENGTH))
              *p2++ = '\n', lf_count--, length++, p1++;
        *p2 = '\0';
     }
     return(buffer);
}

/* ---->  Create separator ('-=-=-=-') or ('-------'), etc.  <---- */
char *separator(int scrwidth,int trailing_cr,const char ch1,const char ch2)
{
     static char buf[512];
     int    length = 1;
     char   *p1;

     if(scrwidth < 1) scrwidth = 79;
     strcpy(buf,ANSI_DCYAN);
     p1    = buf;
     while(*p1 && (*p1 != '\0')) p1++;
     *p1++ = ch1;

     while((length + 2) <= scrwidth) {
            length = length + 2;
            *p1++  = ch2;
            *p1++  = ch1;
     } 
     for(; trailing_cr > 0; *p1++ = '\n', trailing_cr--);
     *p1 = '\0';

     return(buf);
}

/* ---->  Is given swereword in given text?  <---- */
int bad_matched(const char *word,const char *text,const char last,const char *leading,int exact,int allow_spaces)
{
    if(!(word && *word) || !(text && *text)) return(0);
    if(exact && isalpha(last))               return(0);

    /* ---->  LAST must be one of the characters in LEADING  <---- */
    if(leading) {
       while(*leading && (*leading != toupper(last))) leading++;
       if(*leading) return(0); 
    }

    while(*word && *text) {
          if(*word == toupper(*text)) {
             word++, text++;
             if(*word) {
                while(*text && !((isalpha(*text) && (*(text - 1) != '%')) || ((*text == ' ') && allow_spaces))) {
                      if(*text == '\x1B') {
                         for(; *text && (*text != 'm'); text++);
                         if(*text && (*text == 'm')) text++;
		      } else text++;
		}
	     }
	  } else return(0);  
    }

    if(!*word && (!exact || (exact && (!*text || (*text && !(isalpha(*text) || (*text == ' '))))))) return(1);
    return(0);
}

/* ---->  Filter out dirty word  <---- */
void bad_filter(int count,const char **src,char **dest,char *last,int allow_spaces)
{
    while(*(*src) && count) {
          *last      = *(*src);
          *(*dest)++ = '*';
          (*src)++;
          while(*(*src) && !((isalpha(*(*src)) && (*(*src - 1) != '%')) || ((*(*src) == ' ') && allow_spaces))) {
                if(*(*src) == '\x1B') {
                   while(*(*src) && (*(*src) != 'm')) {
                       *last      = *(*src);
                       *(*dest)++ = *(*src)++;
		   }
                   if(*(*src) && (*(*src) == 'm')) {
                      *last      = *(*src);
                      *(*dest)++ = *(*src)++;
		   }
		} else {
                   *last      = *(*src);
                   *(*dest)++ = *(*src)++;
		}
	  }
          count--;
    }
}

/* ---->  Filters out swere words/bad language from given string  <---- */ 
const char *bad_language_filter(char *dest,const char *src)
{
      char *start = dest;
      char last = ' ';
      int  as;

      if(Blank(src)) {
         if(dest) *dest = '\0';
         return(start);
      }

      while(*src) {
            while(*src && !isalpha(*src))
                  switch(*src) {
                         case '%':

                              /* ---->  Skip substitution  <---- */
                              *dest++ = *src++;
                              if(*src) *dest++ = *src++;
                              break;
                         case '\x1B':

                              /* ---->  'Hard' ANSI code  <---- */
                              for(; *src && (*src != 'm'); *dest++ = *src++);
                              if(*src && (*src == 'm')) *dest++ = *src++;
                              break;
                         default:
                              last    = *src;
                              *dest++ = *src++;
		  }

            as = isalpha(last);
            if(*src) {
             if(bad_matched("SHIT",src,last,NULL,0,as)) bad_filter(4,&src,&dest,&last,as);
              else if(bad_matched("FUCK",src,last,NULL,0,as)) bad_filter(4,&src,&dest,&last,as);
               else if(bad_matched("CUNT",src,last,"S",0,as)) bad_filter(4,&src,&dest,&last,as);
                else if(bad_matched("PISS",src,last,NULL,0,as)) bad_filter(4,&src,&dest,&last,as);
                 else if(bad_matched("ARSE",src,last,"PE",0,as)) bad_filter(4,&src,&dest,&last,as);
                  else if(bad_matched("WANK",src,last,NULL,0,as)) bad_filter(4,&src,&dest,&last,as);
                   else if(bad_matched("SHAG",src,last,NULL,0,as)) bad_filter(4,&src,&dest,&last,as);
                    else if(bad_matched("DICK",src,last,NULL,0,as)) bad_filter(4,&src,&dest,&last,as);
                     else if(bad_matched("COCK",src,last,NULL,1,as)) bad_filter(4,&src,&dest,&last,as);
                      else if(bad_matched("KUNT",src,last,NULL,0,as)) bad_filter(4,&src,&dest,&last,as);
                       else if(bad_matched("FVCK",src,last,NULL,0,as)) bad_filter(4,&src,&dest,&last,as);
                        else if(bad_matched("CVNT",src,last,NULL,0,as)) bad_filter(4,&src,&dest,&last,as);
                         else if(bad_matched("SLUT",src,last,NULL,0,as)) bad_filter(4,&src,&dest,&last,as);
                          else if(bad_matched("BITCH",src,last,NULL,0,as)) bad_filter(5,&src,&dest,&last,as);
                           else if(bad_matched("PUSSY",src,last,NULL,0,as)) bad_filter(5,&src,&dest,&last,as);
                            else if(bad_matched("WHORE",src,last,NULL,0,as)) bad_filter(5,&src,&dest,&last,as);
                             else if(bad_matched("BUGGER",src,last,"E",0,as)) bad_filter(6,&src,&dest,&last,as);
                              else if(bad_matched("NIGGER",src,last,"S",0,as)) bad_filter(6,&src,&dest,&last,as);
                               else if(bad_matched("ASSHOLE",src,last,NULL,0,as)) bad_filter(7,&src,&dest,&last,as);
                                else if(bad_matched("BOLLOCK",src,last,NULL,0,as)) bad_filter(7,&src,&dest,&last,as);
                                 else if(bad_matched("BASTARD",src,last,NULL,0,as)) bad_filter(7,&src,&dest,&last,as);
                                  else if(bad_matched("BARSTARD",src,last,NULL,0,as)) bad_filter(8,&src,&dest,&last,as);
                                   else if(bad_matched("COCKSUCKER",src,last,NULL,0,as)) bad_filter(8,&src,&dest,&last,as);
                                    else last = *src, *dest++ = *src++;
	    }
      }
      *dest = '\0';
      return(start);
}

/* ---->  Truncate string to given length (Taking ANSI/formatting codes into account)  <---- */
const char *truncatestr(char *dest,const char *str,unsigned char nonansi,int length)
{
      char *start = dest;
      int  count = 0;

      if(!dest) return(NULL);
      if(!str) {
         *dest = '\0';
         return(start);
      }

      while(*str && (count < length))
            switch(*str) {
                   case '\x05':

                        /* ---->  Hanging indent control  <---- */
                        if(nonansi) {
                           *dest++ = *str++;
                           if(*str) *dest++ = *str++;
			} else if(*(++str)) str++;
                        break;
                   case '\x06':
                   case '\x0E':

                        /* ---->  Skip rest of line/Toggle evaluation of HTML tags  <---- */
                        if(nonansi) *dest++ = *str++;
                           else str++;
                        break;
                   case '\x1B':

                        /* ---->  'Hard' ANSI code  <---- */
                        for(; *str && (*str != 'm'); *dest++ = *str++);
                        if(*str && (*str == 'm')) *dest++ = *str++;
                        break;
                   default:
                        *dest++ = *str++;
                        count++;
	    }
      *dest = '\0';
      return(start);
}

/* ---->  Filter 'hard' ANSI and formatting codes from given string  <---- */
const char *ansi_code_filter(char *dest,const char *src,unsigned char ansi)
{
      char *start = dest;

      if(!dest) return(NULL);
      if(!src) {
         *dest = '\0';
         return(dest);
      }

      while(*src)
            switch(*src) {
                   case '\x05':

                        /* ---->  Hanging indent control  <---- */
                        if(*(++src)) src++;
                        break;
                   case '\x06':
                   case '\x0E':

                        /* ---->  Skip rest of line/Toggle evaluation of HTML tags  <---- */
                        src++;
                        break;
                   case '\x1B':

                        /* ---->  'Hard' ANSI code  <---- */
                        if(ansi) {
                           for(; *src && (*src != 'm'); src++);
                           if(*src && (*src == 'm')) src++;
                           break;
			}
                   default:
                        *dest++ = *src++;
	    }
      *dest = '\0';
      return(start);
}

/* ---->  Return value as rank (1st, 2nd, etc.)  <---- */
const char *rank(int value)
{
      static char buffer[RETURN_BUFFERS * 32];
      static int  bufptr = 0;

      if(++bufptr >= RETURN_BUFFERS) bufptr = 0;

      switch(value % 10) {
             case 1:snprintf(buffer + (bufptr * 32),32,"%d%s",value,((value % 100) == 11) ? "th":"st");
		    break;
	     case 2:snprintf(buffer + (bufptr * 32),32,"%d%s",value,((value % 100) == 12) ? "th":"nd");
		    break;
	     case 3:snprintf(buffer + (bufptr * 32),32,"%d%s",value,((value % 100) == 13) ? "th":"rd");
		    break;
            default:snprintf(buffer + (bufptr * 32),32,"%dth",value);
      }

      return(buffer + (bufptr * 32));
}

/* ---->  Count occurences of a character within a string  <---- */
int strcnt(const char *str,const char c)
{
    int count = 0;

    if(!str) return(0);
    if(*str) count++;
    while(*str) {
          if(*str == c) count++;
          str++;
    }
    return(count);
}

/* ---->  Return position of character C in string STR  <---- */
int strpos(const char *str,const char c)
{
    int pos;
    for(pos = 0; (*str && (*str != c)); pos++) str++;
    return(pos);
}

/* ---->  Return string consisting of character PAD of LEN length  <---- */
const char *strpad(char pad,int len,char *buffer)
{
      buffer[len] = '\0';
      for(len--; len >= 0; buffer[len] = pad, len--);
      return(buffer);
}

/* ---->  Split given string into two separate parameter strings (Separated by '=')  <---- */
void split_params(char *str,char **arg1,char **arg2)
{
     if(Blank(str)) {
        *arg1 = str;
        *arg2 = NULL;
        return;
     }
     if(!(*arg2 = (char *) strchr(str,'='))) {
        *arg1 = str;
        return;
     }

     /* ---->  Terminate ARG1 and find beginning of ARG2  <---- */
     if((*arg1 = *arg2) > str) (*arg1)--;
     for((*arg2)++; *(*arg2) && (*(*arg2) == ' '); (*arg2)++);
     for(; (*arg1 > str) && (*(*arg1) == ' '); (*arg1)--);
     *(*arg1 + 1) = '\0';
     *arg1 = str;
}

/* ---->  Strips unneccessary blanks out of string and capitalises first letter (Optionally)  <---- */
const char *filter_spaces(char *dest,const char *src,int capitalise)
{    
      int  length = 0;
      char *p1 = dest;

      if(src) {
         for(; *src && (*src == ' '); src++);
         while(*src && (length < MAX_LENGTH)) {
               if(*src && capitalise && (*src != ' ')) {
                  *p1++ = toupper(*src);
                  length++;
                  src++;
	       }

               while(*src && (*src != ' ')) {
                     *p1++ = *src++;
                     length++;
	       }

               while(*src && (*src == ' ')) src++;
               if(*src && (*src != ' ')) {
                  *p1++ = ' ';
                  length++;
	       }
	 }
      }
      *p1 = '\0';

      return(dest);
}

/* ---->  Determine whether specified keyword is present in TEXT  <---- */
unsigned char keyword(const char *kword,char *text,char **trailing) {
	 char          *ptr,*tmp;
	 unsigned char length;

	 if(Blank(text) || Blank(kword)) return(0);
	 length = strlen(kword);

	 for(ptr = text; *ptr; ptr++)
	     if(((ptr == text) || (*(ptr - 1) == ' ')) && !strncasecmp(ptr,kword,length))
		if(!(*(ptr + length)) || (*(ptr + length) == ' ')) {
		   if(trailing) {
		      for(tmp = (ptr - 1); (tmp >= text) && (*tmp == ' '); *tmp-- = '\0');
		      for(ptr += length; *ptr && (*ptr == ' '); ptr++);
		      *trailing = ptr;
		   }
		   return(1);
		}
	 return(0);
}

/* ---->  {J.P.Boggis 16/07/2000}  Match range  <---- */
/*           STR:  String to match against range.     */
/*     RANGEFROM:  Lower range delimiting string.     */
/*       RANGETO:  Upper range delimiting string.     */
/*        PREFIX:  0 = STR must exactly match range.  */
/*                 1 = Range must prefix STR.         */
/*                                                    */
/*  RETURN VALUE:  0 = Not matched within range.      */
/*                 1 = Matched within range.          */
int match_range(const char *str,const char *rangefrom,const char *rangeto,int prefix)
{
    int slen,rfromlen,rtolen,minlen,maxlen;

    if(Blank(str) || Blank(rangefrom) || Blank(rangeto)) return(0);

    /* ---->  Work out minimum and maximum range lengths  <---- */
    if((rfromlen = strlen(rangefrom)) >= (rtolen = strlen(rangeto))) {
       maxlen = rfromlen;
       minlen = rtolen;
    } else {
       maxlen = rtolen;
       minlen = rfromlen;
    }

    /* ---->  Range from alphabetically greater than range to  -  Swap round  <---- */
    if(strcasecmp(rangeto,rangefrom) < 0) {
       const char *swap = rangeto;

       rangeto   = rangefrom;
       rangefrom = swap;
    }

    /* ---->  Less than minimum range length, or greater than maximum range length  <---- */
    if(((slen = strlen(str)) < minlen) || ((slen > maxlen) && !prefix)) return(0);

    /* ---->  Perform range match  <---- */
    for(; *str; str++) {

	/* ---->  End of RANGEFROM and RANGETO, but not end of STR (Non-prefix range match)  <---- */
	if(!*rangefrom && !*rangeto && !prefix)
	   return(0);

	/* ---->  STR character greater than RANGEFROM character  <---- */
	if(*rangefrom && (tolower(*str) < tolower(*rangefrom)))
	   return(0);

	/* ---->  STR character less than RANGETO character  <---- */
	if(*rangeto && (tolower(*str) > tolower(*rangeto)))
	   return(0);

        /* ---->  Advance range pointers  <---- */
        if(*rangefrom) rangefrom++;
        if(*rangeto)   rangeto++;
    }
    return(1);
}

/* ---->  Returns 1 if string STR matches wildcard specification WILDCARD, otherwise 0  <---- */
unsigned char match_wildcard(const char *wildcard,const char *str)
{
	 const    char *wptr,*sptr;
	 unsigned char matched;

	 if(!wildcard || !str) return(0);
	 if(!*wildcard || !*str) return(!*wildcard && !*str);
	 while(*str && *wildcard)
	       switch(*wildcard) {
		      case '?':
			   wildcard++, str++;
			   break;
		      case '*':
			   matched = 2;
			   wildcard++;
			   if(*wildcard && (*wildcard != '*')) {
			      while(*str && (matched == 2)) {
				    wptr = wildcard, sptr = str;
				    while(*sptr && *wptr && (*wptr != '*') && matched)
					  switch(*wptr) {
						 case '?':
						      wptr++, sptr++;
						      break;
						 default:
						      if(tolower(*wptr) != tolower(*sptr)) matched = 0;
						      wptr++, sptr++;
					  }
				    if((!*wptr || (*wptr == '*')) && matched) {
				       wildcard = wptr;
				       matched  = 1;
				       str      = sptr;
				    } else {
				       matched = 2;
				       str++;
				    }
			      }
			   } else if(!*wildcard) return(1);
			   break;
		      default:
			   if(tolower(*wildcard) != tolower(*str)) return(0);
			   wildcard++, str++;
	       }
	 for(; *wildcard && (*wildcard == '*'); wildcard++);
	 return(!*wildcard && !*str);
}

/* ---->  Match wildcard specification in given list  <---- */
const char *match_wildcard_list(const char *wildcard,char separator,const char *list,char *buffer)
{
      char *tmp;

      if(!wildcard || !list) return(NULL);
      while(*list) {
            for(tmp = buffer; *list && (*list != separator); *tmp++ = *list++);
            *tmp = '\0';
            if(!Blank(buffer))
               if(match_wildcard(wildcard,buffer))
                  return(buffer);
            if(*list) list++;
      }
      return(NULL);
}

/* ---->  Compare two strings, returning count of matching characters from start of string  <---- */
int string_compare(const char *string1,const char *string2,int minimum)
{
    int matched = 0;

    if(!string1 || !string2) return(0);
    while(*string1 && *string2 && (tolower(*string1) == tolower(*string2)))
          string1++, string2++, matched++;
    return((matched >= minimum) ? matched:0);
}

/* ---->  Matches for STR in LIST (Items separated by SEPARATOR)  <---- */
int string_matched_list(const char *str,const char *list,const char separator,int word)
{
    const char *ptr;

    if(!str || !list) return(0);
    while(*list) {
          for(; *list && ((*list == separator) || (*list == ' ')); list++);
          for(ptr = str; *ptr && *list && (toupper(*ptr) == toupper(*list)) && !((*list == separator) || (word && (*list == ' '))); ptr++, list++);
          if(!*ptr && (!*list || (*list == separator) || (word && (*list == ' ')))) return(1);
          while(*list && (*list != separator)) list++;
    }
    return(0);
}

/* ---->  Returns 1 if string <MATCH> prefixes string <STRING>, otherwise 0  <---- */
int string_matched_prefix(const char *match,const char *string)
{
    if((match == NULL) || (string == NULL)) return(0);
    if(!*match && !*string) return(1);

    while(*match) {
          if(*match && !*string) return(0);
          if(tolower(*match) != tolower(*string)) return(0);
	  string++, match++;
    }
    return(1);
}

/* ---->  Does string PREFIX prefix the string STRING  <---- */
int string_prefix(const char *string,const char *prefix)
{
    if(!prefix) return(1);
    if(!string) return(0);
    while(*string && *prefix && (tolower(*string) == tolower(*prefix)))
          string++, prefix++;
    return(!*prefix);
}

/* ---->  Accepts only non-empty matches starting at the beginning of a word  <---- */
const char *string_match(const char *src,const char *sub)
{
      if(!sub) return(src);
      if(!src) return(NULL);

      if(*sub) while(*src) {
         if(string_prefix(src,sub)) return(src);
     
         while(*src && (isalnum(*src)  || (*src == '@'))) src++;
         while(*src && !(isalnum(*src) || (*src == '@'))) src++;
      }
      return(NULL);
}

/* ---->  Is SUBSTR present in the string STRING  <---- */
int instring(const char *substr,const char *string)
{
    const char *p1,*p2,*p3;
    int   pos = 1;

    if(!substr || !string) return(0);

    p1 = string;
    while(*p1) {          
          p2 = substr, p3 = p1;
          while(*p2 && *p3 && (tolower(*p2) == tolower(*p3))) p2++, p3++;
          if(*p2 == '\0') return(pos);
          pos++, p1++;
    }
    return(0);
}

/* ---->  Format string correctly for emote/pose  <---- */
const char *pose_string(char **str,const char *punct)
{
      static char buffer[32];
      char        *ptr;

      ptr = buffer;

      if((*str) && *(*str)) {
         if((*(*str) == ',') || (*(*str) == ';') || (*(*str) == ':') || (*(*str) == '?') || (*(*str) == '!') || (*(*str) == '.') || (*(*str) == '>') || (*(*str) == ']') || (*(*str) == ')')) {
             *ptr++ = *(*str);
             (*str)++;
             if(*(*str) && (*(*str) != '\'')) {
                *ptr++ = ' ';
                if((*(*str) == '.') || (*(*str) == '?') || (*(*str) == '!') || (*(*str) == ';') || (*(*str) == ':')) *ptr++ = ' ';
	     }
	 } else if((*punct != '@') && *(*str) && (*(*str) != '\'')) *ptr++ = ' ';
         *ptr++ = '\0';
         while(*(*str) && (*(*str) == ' ')) (*str)++;
      } else {
         *buffer = '\0';
         if((*punct != '*') && (*punct != '@')) sprintf(buffer + strlen(buffer)," does nothing%s",punct);
      }
      return(buffer);
}

/* ---->  Convert binary string to format suitable for display (Control codes and unprintable characters displayed as '{nnn}')  <---- */
const char *binary_to_ascii(unsigned char *str,int len,char *buffer)
{
      int  pos,newlen = 0;
      char *ptr = buffer;

      for(pos = 0; (pos < len); pos++)
          if((str[pos] < 32) || (str[pos] > 126)) {
             if((newlen + 5) < MAX_LENGTH) {
                sprintf(ptr,"{%03d}",str[pos]);
                ptr += 5, newlen += 5;
	     } else {
                *ptr = '\0';
                return(buffer);
	     }
	  } else if(newlen < MAX_LENGTH) {
             *ptr++ = str[pos];
             newlen++;
	  } else {
             *ptr = '\0';
             return(buffer);
	  }
     *ptr = '\0';
     return(buffer);
}

/* ---->  Returns correct WRAP_LEADING setting for text string which contains a value (VALUE)  <---- */
int digit_wrap(int offset,int value)
{
    int digits = 1;

    while(value > 9) value /= 10, digits++;
    return(offset + digits);
}

/* ---->  Convert given string to floating point number (Now supports rounding to given number of D.P's by using <DP>:<NUMBER>)  <---- */
double tofloat(const char *str,char *dps)
{
       int           count,negate = 0,dp = TCZ_INFINITY;
       double        temp,result  = 0;
       unsigned char dp_spec      = 0;
       static   char buffer[17];

       /* ---->  Blank or NULL string?  <---- */
       result = 0;
       if(dps) *dps = NOTHING;
       if(Blank(str)) return(result);

       /* ---->  Skip over leading blanks  <---- */
       while(*str && (*str == ' ')) str++;

       /* ---->  Grab first number  <---- */
       if(*str && (*str == '-')) str++, negate = 1;
       for(count = 0; (count < 16) && (*str && (isdigit(*str) && (*str != '.') && (*str != ':'))); count++)
           buffer[count] = *str++;
       buffer[count] = '\0';
       for(; *str && isdigit(*str); str++);

       /* ---->  Work out if number is D.P. specification or Mantissa part  <---- */
       if(*str && (*str == ':')) {
          dp = atoi(buffer), str++, dp_spec = 1;
          if(*str && (*str == '-')) str++, negate = 1;
	     else negate = 0;
          for(count = 0; (count < 16) && (*str && isdigit(*str)); count++)
              buffer[count] = *str++;
          buffer[count] = '\0';
          for(; *str && isdigit(*str); str++);
       }
       temp = atol(buffer);
       result += temp;

       /* ---->  Work out exponent part of number  <---- */
       if(*str && (*str == '.')) {
          double divisor = 10;

          if(dp_spec && dps) *dps = 0;
          if(dp > 0) {
             for(str++; *str && isdigit(*str) && (dp > 0); str++) {
                 if(result >= 0) result += (*str - 48) / divisor;
                    else result -= (*str - 48) / divisor;

                 if((dp == 1) && *(str + 1) && isdigit(*(str + 1)) && ((*(str + 1) - 48) >= 5)) {
                    if(result >= 0) result += 1 / divisor;
                       else result -= 1 / divisor;
		 }
                 divisor *= 10, dp--;
                 if(dp_spec && dps) (*dps)++;
	     }
	  } else if(*(str + 1) && isdigit(*(str + 1)) && ((*(str + 1) - 48) >= 5)) {
             if(result >= 0) result++;
                else result--;
	  }
       }
       if(negate) result = 0 - result;
       return(result);
}

/* ---->  Convert hexadecimal string to integer  <---- */
int tohex(const char *str)
{
    int           value  = 0;

    if(Blank(str)) return(0);
    if(*str && (*str == '-')) str++;
    if(Blank(str)) return(0);

    while(*str && isxdigit(*str)) {
          value *= 16;
          if(!((*str >= '0') && (*str <= '9'))) {
             switch(tolower(*str)) {
                    case 'a':
                         value += 10;
                         break;
                    case 'b':
                         value += 11;
                         break;
                    case 'c':
                         value += 12;
                         break;
                    case 'd':
                         value += 13;
                         break;
                    case 'e':
                         value += 14;
                         break;
                    case 'f':
                         value += 15;
                         break;
	     }
	  } else value += (*str - 48);
          str++;
    }
    return(value);
}

/* ---->  Convert time given as a string into correct value (In secs)  <---- */
long parse_time(char *str)
{
     long time = 0,value;
     char *p1;
     char c;

     if(Blank(str)) return(0);
     while(*str) {
           while(*str && !isdigit(*str)) str++;
           if(*str) {
              for(p1 = str; *str && isdigit(*str); str++);
	      c = *str, *str = '\0', value = atol(p1), *str = c;
              while(*str && (*str == ' ')) str++;
              for(p1 = str; *str && isalnum(*str); str++);
              c = *str, *str  = '\0';

              if(string_prefix("seconds",p1) || string_prefix("secs",p1)) time += value;
	         else if(string_prefix("minutes",p1) || string_prefix("mins",p1)) time += (value * MINUTE);
	            else if(string_prefix("hours",p1)) time += (value * HOUR);
	               else if(string_prefix("weeks",p1)) time += (value * WEEK);
	                  else if(string_prefix("months",p1)) time += (value * MONTH);
	                     else if(string_prefix("years",p1)) time += (value * YEAR);
	                        else if(!*p1 || string_prefix("days",p1)) time += (value * DAY);
	                           else return(-1);
              *str = c;
	   }
     }
     return(time);
}

/* ---->  {J.P.Boggis 27/08/2000}  Return EPOCH date/time in specified format  <---- */
/*        DATETIME  -  EPOCH date/time (Seconds since midnight 1st January 1970.)    */
/*        LONGDATE  -  LONGDATE date (UNSET_DATE if EPOCH date/time used.)           */
/*          PLAYER  -  Character who's preferred date/time format will be used (NOTHING = Use default format.)  */
/*   DEFAULTFORMAT  -  Default date/time format (Used if PLAYER == NOTHING or is invalid.)  */
const char *date_to_string(time_t datetime,unsigned long longdate,dbref player,const char *defaultformat)
{
      int    longday = 0, longmonth = 0, longyear = 0, longdoy = 0;
      int    length = 0, count = 0, upper = 0;
      static char buffer[RETURN_BUFFERS * KB];
      int    hour = 0, hour24 = 1, pm = 0;
      struct tm *rtime = NULL;
      char   formatbuffer[KB];
      static int bufptr = 0;
      const  char *format;
      char   *dest,*ptr;

      if(longdate == UNSET_DATE) {

         /* ---->  EPOCH  <---- */
         rtime = localtime(&datetime);
      } else {

         if((datetime = longdate_to_epoch(longdate)) == NOTHING) {

	    /* ---->  LONGDATE  <---- */
	    longday   = (longdate  & 0xFF);
	    longmonth = ((longdate & 0xFF00) >> 8);
	    longyear  = ((longdate & 0xFFFF0000) >> 16);

	    /* ---->  Day of year  <---- */
	    for(count = 1; count < longmonth; count++)
		if(!(!(longyear % 4) && ((longyear % 100) || !(longyear % 400)))) {
		   longdoy += mdays[count - 1];
		} else longdoy += leapmdays[count - 1];
	    longdoy += longday;

	    /* ---->  Day  <---- */
	    if(longday < 1) {
	       longday = 1;
	    } else if(longday > 31) {
	       longday = 31;
	    }

	    /* ---->  Month  <---- */
	    if(longmonth < 1) {
	       longmonth = 1;
	    } else if(longmonth > 12) {
	       longmonth = 12;
	    }

	    /* ---->  Year  <---- */
	    if(longyear < 0) {
	       longyear = 1;
	    } else if(longyear > 9999) {
	       longyear = 9999;
	    }
	 } else rtime = localtime(&datetime);
      }

      /* ---->  Date/time format  <---- */
      if(++bufptr >= RETURN_BUFFERS) bufptr = 0;
      if(Blank(defaultformat)) defaultformat = FULLDATEFMT;

      if(player != NOTHING) {
	 if(!strcasecmp(defaultformat,LONGDATEFMT)) {

	    /* ---->  Long date format  <---- */
	    if(Validchar(player) && !Blank(db[player].data->player.longdateformat))
	       snprintf(formatbuffer,KB,"%s",db[player].data->player.longdateformat);
		  else snprintf(formatbuffer,KB,"%s",LONGDATEFMT);
	 } else if(!strcasecmp(defaultformat,SHORTDATEFMT)) {

	    /* ---->  Short date format  <---- */
	    if(Validchar(player) && !Blank(db[player].data->player.shortdateformat))
	       snprintf(formatbuffer,KB,"%s",db[player].data->player.shortdateformat);
		  else snprintf(formatbuffer,KB,"%s",SHORTDATEFMT);
	 } else if(!strcasecmp(defaultformat,TIMEFMT)) {

	    /* ---->  Time format  <---- */
	    if(Validchar(player) && !Blank(db[player].data->player.timeformat))
	       snprintf(formatbuffer,KB,"%s",db[player].data->player.timeformat);
		  else snprintf(formatbuffer,KB,"%s",TIMEFMT);
	 } else {

	    /* ---->  Full time/date format  <---- */
	    if(Validchar(player) && !Blank(db[player].data->player.longdateformat) && !Blank(db[player].data->player.timeformat)) {
	       if(!db[player].data->player.dateorder)
		  snprintf(formatbuffer,KB,"%s%s%s",db[player].data->player.longdateformat,!Blank(db[player].data->player.dateseparator) ? db[player].data->player.dateseparator:", ",db[player].data->player.timeformat);
		     else snprintf(formatbuffer,KB,"%s%s%s",db[player].data->player.timeformat,!Blank(db[player].data->player.dateseparator) ? db[player].data->player.dateseparator:", ",db[player].data->player.longdateformat);
	    } else snprintf(formatbuffer,KB,"%s",FULLDATEFMT);
	 }

         /* ---->  Custom date/time format (player == NOTHING)  <---- */
      } else snprintf(formatbuffer,KB,"%s",defaultformat);

      /* ---->  Substitute date/time format  <---- */
      format = formatbuffer;
      dest   = buffer + (bufptr * KB);
      *dest  = '\0', length = 0;
      while(*format && (length < KB)) {
            switch(*format) {
                   case 'd':

                        /* ---->  Day of month  <---- */
                        for(count = 0; *format && (*format == 'd'); format++, count++);
                        if(count > 3) count = 3;
                        switch(count) {
                               case 1:

                                    /* ---->  Day of month without leading zero  <---- */
                                    snprintf(dest,KB - length,"%d",(rtime ? rtime->tm_mday:longday));
                                    break;
                               case 3:

                                    /* ---->  Day of month with leading space  <---- */
                                    snprintf(dest,KB - length,"%2d",(rtime ? rtime->tm_mday:longday));
                                    break;
                               case 2:
                              default:

                                    /* ---->  Day of month with leading zero  <---- */
                                    snprintf(dest,KB - length,"%02d",(rtime ? rtime->tm_mday:longday));
                                    break;
			}

                        while(*dest && (length < KB)) dest++, length++;
                        format--;
                        break;
                   case 'D':

                        /* ---->  Ranked day of month  <---- */
                        for(count = 0; *format && (*format == 'D'); format++, count++);
                        if(count > 3) count = 3;
                        switch(count) {
                               case 2:

                                    /* ---->  Ranked day of month with leading zero  <---- */
                                    snprintf(dest,KB - length,"%4s",rank(rtime ? rtime->tm_mday:longday));
                                    for(ptr = dest; *ptr && (*ptr == ' '); *ptr++ = '0');
                                    break;
                               case 3:

                                    /* ---->  Ranked day of month with leading space  <---- */
                                    snprintf(dest,KB - length,"%4s",rank((rtime ? rtime->tm_mday:longday)));
                                    break;
                               case 1:
                              default:

                                    /* ---->  Ranked day of month without leading zero  <---- */
                                    snprintf(dest,KB - length,"%s",rank((rtime ? rtime->tm_mday:longday)));
                                    break;
			}

                        while(*dest && (length < KB)) dest++, length++;
                        format--;
                        break;
                   case 'e':
                   case 'E':

                        /* ---->  Number of seconds since midnight, 1st January 1970 (EPOCH)  <---- */
                        if(rtime) {
                           snprintf(dest,KB - length,"%d",(int) datetime);
                           while(*dest && (length < KB)) dest++, length++;
			}
                        break;
                   case 'h':

                        /* ---->  12-hour hour  <---- */
                        hour = (rtime ? rtime->tm_hour:0);
                        hour24 = 0;
                        if(hour > 0) {
                           if(hour > 12) hour -= 12;
			} else hour = 12;
                   case 'H':

                        /* ---->  24-hour hour  <---- */
                        if(hour24) hour = (rtime ? rtime->tm_hour:0);
                        for(count = 0; *format && ((*format == 'h') || (*format == 'H')); format++, count++);
                        if(count > 3) count = 3;
                        switch(count) {
                               case 2:

                                    /* ---->  Hour with leading zero  <---- */
                                    snprintf(dest,KB - length,"%02d",hour);
                                    break;
                               case 3:

                                    /* ---->  Hour with leading space  <---- */
                                    snprintf(dest,KB - length,"%2d",hour);
                                    break;
                               case 1:
                              default:

                                    /* ---->  Hour without leading zero  <---- */
                                    snprintf(dest,KB - length,"%d",hour);
                                    break;
			}

                        while(*dest && (length < KB)) dest++, length++;
                        format--;
                        break;
                   case 'j':
                   case 'J':

                        /* ---->  Day of year  <---- */
                        for(count = 0; *format && ((*format == 'j') || (*format == 'J')); format++, count++);
                        if(count > 3) count = 3;
                        switch(count) {
                               case 2:

                                    /* ---->  Day of year with leading zero  <---- */
                                    snprintf(dest,KB - length,"%03d",(rtime ? (rtime->tm_yday + 1):longdoy));
                                    break;
                               case 3:

                                    /* ---->  Day of year with leading space  <---- */
                                    snprintf(dest,KB - length,"%3d",(rtime ? (rtime->tm_yday + 1):longdoy));
                                    break;
                               case 1:
                              default:

                                    /* ---->  Day of year without leading zero  <---- */
                                    snprintf(dest,KB - length,"%d",(rtime ? (rtime->tm_yday + 1):longdoy));
                                    break;
			}

                        while(*dest && (length < KB)) dest++, length++;
                        format--;
                        break;
                   case 'm':

                        /* ---->  Month  <---- */
                        for(count = 0; *format && (*format == 'm'); format++, count++);
                        if(count > 3) count = 3;
                        switch(count) {
                               case 1:

                                    /* ---->  Month without leading zero  <---- */
                                    snprintf(dest,KB - length,"%d",(rtime ? (rtime->tm_mon + 1):longmonth));
                                    break;
                               case 3:

                                    /* ---->  Month with leading space  <---- */
                                    snprintf(dest,KB - length,"%2d",(rtime ? (rtime->tm_mon + 1):longmonth));
                                    break;
                               case 2:
                              default:

                                    /* ---->  Month with leading zero  <---- */
                                    snprintf(dest,KB - length,"%02d",(rtime ? (rtime->tm_mon + 1):longmonth));
                                    break;
			}

                        while(*dest && (length < KB)) dest++, length++;
                        format--;
                        break;
                   case 'M':

                        /* ---->  Month name  <---- */
                        for(count = 0; *format && (*format == 'M'); format++, count++);
                        if(count > 2) count = 2;
                        switch(count) {
                               case 1:

                                    /* ---->  Abbreviated name of month  <---- */
                                    snprintf(dest,KB - length,"%s",monthabbr[(rtime ? rtime->tm_mon:(longmonth - 1))]);
                                    break;
                               case 2:
                              default:

                                    /* ---->  Full name of month  <---- */
                                    snprintf(dest,KB - length,"%s",month[(rtime ? rtime->tm_mon:(longmonth - 1))]);
                                    break;
			}

                        while(*dest && (length < KB)) dest++, length++;
                        format--;
                        break;
                   case 'n':
                   case 'N':

                        /* ---->  Minute  <---- */
                        for(count = 0; *format && ((*format == 'n') || (*format == 'N')); format++, count++);
                        if(count > 3) count = 3;
                        switch(count) {
                               case 1:

                                    /* ---->  Minute without leading zero  <---- */
                                    snprintf(dest,KB - length,"%d",(rtime ? rtime->tm_min:0));
                                    break;
                               case 3:

                                    /* ---->  Minute with leading space  <---- */
                                    snprintf(dest,KB - length,"%2d",(rtime ? rtime->tm_min:0));
                                    break;
                               case 2:
                              default:

                                    /* ---->  Minute with leading zero  <---- */
                                    snprintf(dest,KB - length,"%02d",(rtime ? rtime->tm_min:0));
                                    break;
			}

                        while(*dest && (length < KB)) dest++, length++;
                        format--;
                        break;
                   case 'A':
                   case 'P':
                        upper = 1;
                   case 'a':
                   case 'p':

                        /* ---->  AM/PM  <---- */
                        hour = (rtime ? rtime->tm_hour:0);
                        if(hour > 0) {
                           if(hour >= 12) {
                              if(hour > 12) hour -= 12;
                              pm = 1;
			   } else pm = 0;
			} else {
                           hour = 12;
                           pm   = 0;
			}

                        if((*format == 'a') || (*format == 'A')) {
                           for(count = 0; *format && ((*format == 'a') || (*format == 'A')); format++, count++);
			} else if((*format == 'p') || (*format == 'P')) {
                           for(count = 0; *format && ((*format == 'p') || (*format == 'P')); format++, count++);
			}

                        if(count > 2) count = 2;
                        switch(count) {
                               case 1:

                                    /* ---->  a/p A/P  <---- */
                                    snprintf(dest,KB - length,"%s",(upper) ? (pm ? "P":"A"):(pm ? "p":"a"));
                                    break;
                               case 2:
                              default:

                                    /* ---->  am/pm AM/PM  <---- */
                                    snprintf(dest,KB - length,"%s",(upper) ? (pm ? "PM":"AM"):(pm ? "pm":"am"));
                                    break;
			}

                        while(*dest && (length < KB)) dest++, length++;
                        format--;
                        break;
                   case 's':
                   case 'S':

                        /* ---->  Second  <---- */
                        for(count = 0; *format && ((*format == 's') || (*format == 'S')); format++, count++);
                        if(count > 3) count = 3;
                        switch(count) {
                               case 1:

                                    /* ---->  Second without leading zero  <---- */
                                    snprintf(dest,KB - length,"%d",(rtime ? rtime->tm_sec:0));
                                    break;
                               case 3:

                                    /* ---->  Second with leading space  <---- */
                                    snprintf(dest,KB - length,"%2d",(rtime ? rtime->tm_sec:0));
                                    break;
                               case 2:
                              default:

                                    /* ---->  Second with leading zero  <---- */
                                    snprintf(dest,KB - length,"%02d",(rtime ? rtime->tm_sec:0));
                                    break;
			}

                        while(*dest && (length < KB)) dest++, length++;
                        format--;
                        break;
                   case 'u':
                   case 'U':

                        /* ---->  Day of week  <---- */
                        if(rtime) {
                           snprintf(dest,KB - length,"%d",rtime->tm_wday + (isupper(*format) ? 1:0));
                           while(*dest && (length < KB)) dest++, length++;
			}
                        break;
                   case 'w':

                        /* ---->  Week of year  <---- */
                        for(count = 0; *format && (*format == 'w'); format++, count++);
                        if(count > 3) count = 3;
                        switch(count) {
                               case 2:

                                    /* ---->  Week of year with leading zero  <---- */
                                    snprintf(dest,KB - length,"%02d",(rtime ? ((rtime->tm_yday / 7) + 1):(((longdoy - 1) / 7) + 1)));
                                    break;
                               case 3:

                                    /* ---->  Week of year with leading space  <---- */
                                    snprintf(dest,KB - length,"%2d",(rtime ? ((rtime->tm_yday / 7) + 1):(((longdoy - 1) / 7) + 1)));
                                    break;
                               case 1:
                              default:

                                    /* ---->  Week of year without leading zero  <---- */
                                    snprintf(dest,KB - length,"%d",(rtime ? ((rtime->tm_yday / 7) + 1):(((longdoy - 1) / 7) + 1)));
                                    break;
			}

                        while(*dest && (length < KB)) dest++, length++;
                        format--;
                        break;
                   case 'W':

                        /* ---->  Weekday name  <---- */
                        for(count = 0; *format && (*format == 'W'); format++, count++);
                        if(rtime) {
			   if(count > 2) count = 2;
			   switch(count) {
				  case 1:

				       /* ---->  Abbreviated name of weekday  <---- */
				       snprintf(dest,KB - length,"%s",dayabbr[rtime->tm_wday]);
				       break;
				  case 2:
				 default:

				       /* ---->  Full name of weekday  <---- */
				       snprintf(dest,KB - length,"%s",day[rtime->tm_wday]);
				       break;
			   }

			   while(*dest && (length < KB)) dest++, length++;
			   format--;
			}
                        break;
                   case 'y':
                   case 'Y':

                        /* ---->  Year  <---- */
                        for(count = 0; *format && ((*format == 'y') || (*format == 'Y')); format++, count++);
                        if(count > 4) count = 4;
                        switch(count) {
                               case 1:

                                    /* ---->  1-digit year  <---- */
                                    snprintf(dest,KB - length,"%01d",(rtime ? (rtime->tm_year + 1900):longyear) % 10);
                                    break;
                               case 2:

                                    /* ---->  2-digit year  <---- */
                                    snprintf(dest,KB - length,"%02d",(rtime ? (rtime->tm_year + 1900):longyear) % 100);
                                    break;
                               case 3:

                                    /* ---->  3-digit year  <---- */
                                    snprintf(dest,KB - length,"%03d",(rtime ? (rtime->tm_year + 1900):longyear) % 1000);
                                    break;
                               default:

                                    /* ---->  4-digit year  <---- */
                                    snprintf(dest,KB - length,"%04d",(rtime ? (rtime->tm_year + 1900):longyear));
                                    break;
			}

                        while(*dest && (length < KB)) dest++, length++;
                        format--;
                        break;
                   case 'z':
                   case 'Z':

                        /* ---->  Server time zone  <---- */
                        snprintf(dest,KB - length,"%s",tcz_timezone);
                        while(*dest && (length < KB)) dest++, length++;
                        break;
                   default:

                        /* ---->  Punctuation/spaces  <---- */
                        if(((*format == ' ') || ispunct(*format)) && (length < (KB - 1))) {
                           *dest++ = *format;
                           length++;
			} else {

                           /* ---->  Unknown format code  <---- */
                           if(length < (KB - 1)) {
                              *dest++ = '?';
                              length++;
			   }
			}
                        break;
	    }

            if(*format) format++;
      }

      if(length < KB) *dest = '\0';
      *(buffer + (bufptr * KB) + KB - 1) = '\0';
      return(buffer + (bufptr * KB));
}

/* ---->  Return time interval in format:  1 year, 1 month, 1 week, 1 day, 1 minute and 1 second  <---- */
const char *interval(unsigned long interval,int negative,unsigned char entities,unsigned char shortened)
{
      unsigned char  and,loop,count = 0,negate = 0;
      static   char  buffer[256];
      static   short times[7];

      *buffer = '\0';
      if(negative < 0) interval = 0 - negative, negate = 1;

      for(loop = 0; loop < 7; times[loop] = 0, loop++);
      if(interval >= YEAR)   times[0] = interval / YEAR,   interval %= YEAR;
      if(interval >= MONTH)  times[1] = interval / MONTH,  interval %= MONTH;
      if(interval >= WEEK)   times[2] = interval / WEEK,   interval %= WEEK;
      if(interval >= DAY)    times[3] = interval / DAY,    interval %= DAY;
      if(interval >= HOUR)   times[4] = interval / HOUR,   interval %= HOUR;
      if(interval >= MINUTE) times[5] = interval / MINUTE, interval %= MINUTE;
      times[6] = interval;

      /* ---->  Years  <---- */
      if((count < entities) && times[0]) {
	 sprintf(buffer + strlen(buffer),"%d year%s",times[0],Plural(times[0]));
         count++;
      }

      /* ---->  Months  <---- */
      if((count < entities) && times[1]) {
         for(loop = 2; (loop < 6) && !times[loop]; loop++);
         and = (((count + 1) >= entities) || !times[loop]); 
	 sprintf(buffer + strlen(buffer),"%s%d month%s",(!count) ? "":(and) ? " and ":", ",times[1],Plural(times[1]));
         count++;
      }

      /* ---->  Weeks  <---- */
      if((count < entities) && times[2]) {
         for(loop = 3; (loop < 6) && !times[loop]; loop++);
         and = (((count + 1) >= entities) || !times[loop]); 
	 sprintf(buffer + strlen(buffer),"%s%d week%s",(!count) ? "":(and) ? " and ":", ",times[2],Plural(times[2]));
         count++;
      }

      /* ---->  Days  <---- */
      if((count < entities) && times[3]) {
         for(loop = 4; (loop < 6) && !times[loop]; loop++);
         and = (((count + 1) >= entities) || !times[loop]); 
	 sprintf(buffer + strlen(buffer),"%s%d day%s",(!count) ? "":(and) ? " and ":", ",times[3],Plural(times[3]));
         count++;
      }

      /* ---->  Hours  <---- */
      if((count < entities) && times[4]) {
         for(loop = 5; (loop < 6) && !times[loop]; loop++);
         and = (((count + 1) >= entities) || !times[loop]); 
	 sprintf(buffer + strlen(buffer),"%s%d hour%s",(!count) ? "":(and) ? " and ":", ",times[4],Plural(times[4]));
         count++;
      }

      /* ---->  Minutes  <---- */
      if((count < entities) && times[5]) {
         and = (((count + 1) >= entities) || !times[6]); 
	 sprintf(buffer + strlen(buffer),"%s%d min%s%s",(!count) ? "":(and) ? " and ":", ",times[5],(shortened) ? "":"ute",Plural(times[5]));
         count++;
      }

      /* ---->  Seconds  <---- */
      if((count < entities) && (!count || times[6]))
	 sprintf(buffer + strlen(buffer),"%s%d sec%s%s",(!count) ? "":" and ",times[6],(shortened) ? "":"ond",Plural(times[6]));
      if(negate) strcat(buffer," ago");
      return(buffer);
}

/* ---->  {J.P.Boggis 16/05/1999}  Determines whether given epoch date is a leap year or not.  <---- */
unsigned char leapyear(time_t date)
{
         struct tm *rtime;
         int       year;

         rtime = localtime(&date);
         year  = rtime->tm_year + 1900;
         return(!((year % 4) || (year % 100) || (year % 400)));
}

/* ---->     Convert string to EPOCH/LONGDATE date/time     <---- */
/*        (TIMEDATE:  0 = Date + Time, 1 = Date, 2 = Time.        */
time_t string_to_date(dbref player,const char *str,unsigned char epoch,unsigned char timedate,unsigned char *invalid)
{
       char          m_day = -1,m_month = -1,t_hour = -1,t_minute = -1,t_second = -1;
       char          pm = -1,*tmp,shortdate = 0;
       short         m_year = -1;
       char          buffer[16];
       struct   tm   *rtime;
       unsigned long temp;
       time_t        now;

       /* ---->  Day of month  <---- */
       *invalid = 1;
       if(Blank(str)) return(UNSET_DATE);
       if(!timedate || (timedate == 1)) {
          while(*str && !isdigit(*str)) str++;
          if(*str) {
             if((temp = ABS(atol(str))) > 0x7F) temp = 0x7F;
             m_day = temp;
	  }
          while(*str && isdigit(*str)) str++;
          if(!strncasecmp(str,"th ",3)) str += 3;
             else if(!strncasecmp(str,"st ",3)) str += 3;
                else if(!strncasecmp(str,"nd ",3)) str += 3;
                   else if(!strncasecmp(str,"rd ",3)) str += 3;

          /* ---->  Month  <---- */
          while(*str && !isalnum(*str)) str++;
          for(tmp = buffer, temp = 0; *str && isalpha(*str) && (temp < 16); *tmp++ = *str++, temp++);
          *tmp = '\0';
          if(*str && isdigit(*str)) {

             /* ---->  Numeric month  <---- */
             if((temp = ABS(atol(str))) > 0x7F) temp = 0x7F;
             m_month = temp;
	  } else if(!BlankContent(buffer)) {

             /* ---->  Alphabetic month  <---- */
             for(temp = 0; (temp < 12) && !string_prefix(month[temp],buffer); temp++);
             if(temp >= 12) return(UNSET_DATE);
	        else m_month = temp + 1;
	  }
          while(*str && isdigit(*str)) str++;

          /* ---->  Year  <---- */
          while(*str && !isdigit(*str)) str++;
          if(*str) {
             const char *ptr;
             int   count = 0;

             if((temp = ABS(atol(str))) > 0x7FFF) temp = 0x7FFF;
             if(temp < 100) {
                for(ptr = str; *ptr && isdigit(*ptr); ptr++, count++);
                if(count <= 2) shortdate = 1;
	     }
             m_year = temp;
	  }
          while(*str && isdigit(*str)) str++;
       }

       if(epoch && (!timedate || (timedate == 2))) {

          /* ---->  Hour  <---- */
          while(*str && !isdigit(*str)) str++;
          if(*str) {
             if((temp = ABS(atol(str))) > 0x7F) temp = 0x7F;
             t_hour = temp;
	  }
          while(*str && isdigit(*str)) str++;

          /* ---->  Minute  <---- */
          while(*str && !isdigit(*str)) str++;
          if(*str) {
             if((temp = ABS(atol(str))) > 0x7F) temp = 0x7F;
             t_minute = temp;
	  }
          while(*str && isdigit(*str)) str++;

          /* ---->  Second  <---- */
          while(*str && !isdigit(*str)) str++;
          if(*str) {
             if((temp = ABS(atol(str))) > 0x7F) temp = 0x7F;
             t_second = temp;
	  }
          while(*str && isdigit(*str)) str++;

          /* ---->  AM/PM  <---- */
          while(*str && (*str == ' ')) str++;
          if(!*str) {
             for(str--; isalpha(*str); str--);
             str++;
	  }

          if(!Blank(str)) {
             if(string_prefix("AM",str)) pm = 0;
                else if(string_prefix("PM",str)) pm = 1;
	  }
       }

       /* ---->  Fill-in blank fields using current date/time  <---- */
       gettime(now);
       if(Validchar(player)) now += (db[player].data->player.timediff * HOUR);
       rtime = localtime(&now);

       if(m_day   == -1) m_day   = rtime->tm_mday;
       if(m_month == -1) m_month = rtime->tm_mon  + 1;
       if(m_year  != -1) {
          if(shortdate) m_year += ((m_year > ((epoch) ? 37:69)) ? 1900:2000);
       } else m_year = rtime->tm_year + 1900;

       if(epoch) {
          if(t_hour   == -1) t_hour   = 0;
          if(t_minute == -1) t_minute = 0;
          if(t_second == -1) t_second = 0;
       }

       /* ---->  Validation  <---- */
       if((m_day   <  1) || (m_day   > 31))   return(UNSET_DATE);  /*  Day of month <= Days in month checked further down  */
       if((m_month <  1) || (m_month > 12))   return(UNSET_DATE);
       if((m_year  <= 0) || (m_year  > 9999)) return(UNSET_DATE);
       if(epoch) {
          if(pm >= 0) {
             if((t_hour < 1) || (t_hour > 12)) return(UNSET_DATE);      /*  12-hour clock  */
             if(!pm) {
                if(t_hour == 12) t_hour = 0;
	     } else if(t_hour != 12) t_hour += 12;
	  } else if((t_hour < 0) || (t_hour > 23)) return(UNSET_DATE);  /*  24-hour clock  */
          if((t_minute < 0) || (t_minute > 59)) return(UNSET_DATE);
          if((t_second < 0) || (t_second > 59)) return(UNSET_DATE);
       }

       /* ---->  Correct number of days in month  <---- */
       if(!(!(m_year % 4) && ((m_year % 100) || !(m_year % 400)))) {
          if(m_day > mdays[m_month - 1]) return(UNSET_DATE);
       } else if(m_day > leapmdays[m_month - 1]) return(UNSET_DATE);

       /* ---->  Return date/time in either EPOCH or LONGDATE format  <---- */
       if(epoch) {
          if(m_year < 1900) return(UNSET_DATE);
          rtime->tm_mday = m_day;  rtime->tm_mon = m_month - 1, rtime->tm_year = m_year - 1900;
          rtime->tm_hour = t_hour; rtime->tm_min = t_minute,    rtime->tm_sec  = t_second, rtime->tm_isdst = 0;
          rtime->tm_wday = rtime->tm_yday = -1;
          now = mktime(rtime);
          if((rtime->tm_wday == -1) && (rtime->tm_yday == -1)) return(UNSET_DATE);
          *invalid = 0;
          return(now);
       } else {
          *invalid = 0;
          return(m_day | (m_month << 8) | (m_year << 16));
       }
}

/* ---->  Convert LONGDATE to EPOCH  <---- */
time_t longdate_to_epoch(unsigned long longdate)
{
       struct tm rtime;
       time_t now;

       if(longdate == UNSET_DATE) return(NOTHING);
       rtime.tm_mday  = (longdate  & 0xFF);
       rtime.tm_mon   = ((longdate & 0xFF00) >> 8) - 1;
       if(((longdate & 0xFFFF0000) >> 16) < 1900) return(NOTHING);
       rtime.tm_year  = ((longdate & 0xFFFF0000) >> 16) - 1900;
       rtime.tm_hour  = rtime.tm_min  = rtime.tm_sec = rtime.tm_isdst = 0;
       rtime.tm_wday  = rtime.tm_yday = -1;
       now = mktime(&rtime);
       if((rtime.tm_wday == -1) && (rtime.tm_yday == -1)) return(NOTHING);
       return(now);
}

/* ---->  Convert EPOCH to LONGDATE  <---- */
unsigned long epoch_to_longdate(time_t epoch)
{
	 struct tm *rtime;

	 rtime = localtime(&epoch);
	 if((rtime->tm_mday < 1) || (rtime->tm_mon < 0) || (rtime->tm_year < 0)) return(UNSET_DATE);
	 return(rtime->tm_mday | ((rtime->tm_mon + 1) << 8) | ((rtime->tm_year + 1900) << 16));
}

/* ---->  Return difference (In months) between two dates stored in LONGDATE format  <---- */
unsigned long longdate_difference(unsigned long longdate1,unsigned long longdate2)
{
	 short         m_year1  = ((longdate1 & 0xFFFF0000) >> 16);
	 short         m_year2  = ((longdate2 & 0xFFFF0000) >> 16);
	 unsigned char m_month1 = ((longdate1 & 0xFF00)     >> 8);
	 unsigned char m_month2 = ((longdate2 & 0xFF00)     >> 8);
	 unsigned char m_day1   = (longdate1  & 0xFF);
	 unsigned char m_day2   = (longdate2  & 0xFF);
	 unsigned long interval = 0;

	 if(longdate1 > longdate2) return(0);
	 while(m_year1 < m_year2) {
	       interval += 12;
	       m_year1++;
	 }
	 if(m_month1 == m_month2) {
	    if(m_day1 > m_day2) interval--;
	 } else if(m_month1 > m_month2) {
	    while(m_month1 > m_month2)
		  interval--, m_month1--;
	 } else while(m_month1 < m_month2)
	    interval++, m_month1++;
	 return(interval);
}
