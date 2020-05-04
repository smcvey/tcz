/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| EDIT.C  -  Implements a powerful (But easy to use) line editor for editing  |
|            compound commands, descriptions, mail, elements of dynamic       |
|            arrays, etc.                                                     |
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
| Module originally designed and written by:  J.P.Boggis 09/10/1994.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: edit.c,v 1.2 2005/06/29 20:56:17 tcz_monster Exp $

*/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "descriptor_flags.h"
#include "edit_cmdtable.h"
#include "object_types.h"
#include "objectlists.h"
#include "flagset.h"
#include "search.h"
#include "fields.h"
#include "match.h"


#define  EDITOR_VERSION    "1.4"    /*  Editor version       */

static   int   result;  /*  Used by fieldtype()  */
unsigned short edit_table_size = 0;

const    char  *field_name  (int field);
const    char  *object_name (int object);
         char  *get_field   (dbref player,int field,dbref object,int element);
         int   update_field (struct descriptor_data *d);


/* ---->  Sort editor command table into strict alphabetical order  <---- */
/*        (search_edit_cmdtable() relies on entries in the editor         */
/*        command table being in strict alphabetical order (A             */
/*        binary search is used for maximum efficiency.))                 */
short sort_edit_cmdtable()
{
      unsigned short loop,top,highest;
      struct   edit_cmd_table temp;

      for(loop = 0; edit_cmds[loop].name; edit_cmds[loop].len = strlen(edit_cmds[loop].name), loop++);
      for(top = (edit_table_size = loop) - 1; top > 0; top--) {

          /* ---->  Find highest entry in unsorted part of list  <---- */
          for(loop = 1, highest = 0; loop <= top; loop++)
              if(strcasecmp(edit_cmds[loop].name,edit_cmds[highest].name) > 0)
                 highest = loop;

          /* ---->  Swap highest entry in unsorted part of list with bottom entry of sorted part of list  <---- */
          if(highest < top) {
             temp.flags = edit_cmds[top].flags, temp.name = edit_cmds[top].name, temp.func = edit_cmds[top].func, temp.val1 = edit_cmds[top].val1, temp.len = edit_cmds[top].len;
             edit_cmds[top].flags = edit_cmds[highest].flags, edit_cmds[top].name = edit_cmds[highest].name, edit_cmds[top].func = edit_cmds[highest].func, edit_cmds[top].val1 = edit_cmds[highest].val1, edit_cmds[top].len = edit_cmds[highest].len;
             edit_cmds[highest].flags = temp.flags, edit_cmds[highest].name = temp.name, edit_cmds[highest].func = temp.func, edit_cmds[highest].val1 = temp.val1, edit_cmds[highest].len = temp.len;
	  }
      }
      return(edit_table_size);
}

/* ---->  Search for COMMAND in editor command table, returning  <---- */
/*        EDIT_CMD_TABLE entry if found, otherwise NULL.               */
struct edit_cmd_table *search_edit_cmdtable(const char *command)
{
       int      top = edit_table_size - 1,middle = 0,bottom = 0,nearest = NOTHING,result;
       unsigned short len,nlen = 0xFFFF;

       if(Blank(command)) return(NULL);
       len = strlen(command);
       while(bottom <= top) {
             middle = (top + bottom) / 2;
             if((result = strcasecmp(edit_cmds[middle].name,command)) != 0) {
                if((edit_cmds[middle].len < nlen) && (len <= edit_cmds[middle].len) && !strncasecmp(edit_cmds[middle].name,command,len))
                   nearest = middle, nlen = edit_cmds[middle].len;

                if(result < 0) bottom = middle + 1;
                   else top = middle - 1;
	     } else return(&edit_cmds[middle]);
       }

       /* ---->  Nearest matching command  <---- */
       if(nearest != NOTHING) return(&edit_cmds[nearest]);
       return(NULL);
}

/* ---->  Returns 'of <OBJECT>' or 'element <N> of <ARRAY>'  <---- */
void of_object_or_array(dbref player,dbref object,unsigned char field,int element,const char *ansicode,const char *index)
{
     switch(element) {
            case END:
                 sprintf(scratch_return_string," of new element (Number "ANSI_LWHITE"%d%s)",array_element_count(db[object].data->array.start) + 1,ansicode);
                 if(Valid(object)) sprintf(scratch_return_string + strlen(scratch_return_string)," of "ANSI_LWHITE"%s%s",unparse_object(player,object,0),ansicode);
                 break;
            case LAST:
                 strcpy(scratch_return_string," of last element");
                 if(Valid(object)) sprintf(scratch_return_string + strlen(scratch_return_string)," of "ANSI_LWHITE"%s%s",unparse_object(player,object,0),ansicode);
                 break;
            case INDEXED:
                 if(!Blank(index)) sprintf(scratch_return_string," of element '"ANSI_LWHITE"%s%s'",index,ansicode);
                    else strcpy(scratch_return_string," of unknown indexed element");
                 if(Valid(object)) sprintf(scratch_return_string + strlen(scratch_return_string)," of "ANSI_LWHITE"%s%s",unparse_object(player,object,0),ansicode);
                 break;
            case NOTHING:
                 switch(field) {
                        case 100:
                             sprintf(scratch_return_string," of a new message on %s BBS",tcz_full_name);
                             break;
                        case 101:
                             sprintf(scratch_return_string," of a reply to a message on %s BBS",tcz_full_name);
                             break;
                        case 102:
                             sprintf(scratch_return_string," of a message on %s BBS",tcz_full_name);
                             break;
                        case 103:
                             sprintf(scratch_return_string," of an append to a message on %s BBS",tcz_full_name);
                             break;
                        case 104:
                             strcpy(scratch_return_string," of a mail message");
                             break;
                        case 105:
                             strcpy(scratch_return_string," of a reply to a mailed message");
                             break;
                        case 106:
                             sprintf(scratch_return_string," of a new anonymous message on %s BBS",tcz_full_name);
                             break;
                        case 107:
                             sprintf(scratch_return_string," of an anonymous reply to a message on %s BBS",tcz_full_name);
                             break;
                        case 108:
                             sprintf(scratch_return_string," of an anonymous append to a message on %s BBS",tcz_full_name);
                             break;
                        default:
                             if(Valid(object)) sprintf(scratch_return_string," of %s"ANSI_LWHITE"%s%s",Article(object,LOWER,DEFINITE),getcname(player,object,1,0),ansicode);
		 }
                 break;
            default:
                 if(element >= 0) sprintf(scratch_return_string," of element "ANSI_LWHITE"%d%s",element,ansicode);
                    else strcpy(scratch_return_string," of invalid element");
                 if(Valid(object)) sprintf(scratch_return_string + strlen(scratch_return_string)," of "ANSI_LWHITE"%s%s",unparse_object(player,object,0),ansicode);
     }
}

/* ---->  Returns pointer to beginning of line number LINENO of TEXT  <---- */
char *get_lineno(int lineno,char *text)
{
     if(lineno < 2) return(text);
     while(*text && (lineno > 1)) {
           while(*text && (*text != '\n')) text++;
           if(*text) text++;
           lineno--;
     }
     return(text);
}

/* ---->  Return pointer to the line range RFROM to RTO  <---- */
char *get_range(struct descriptor_data *d,int rfrom,int rto)
{
     int  line = 1;
     char *p1,*p2;

     p1 = d->edit->text;
     p2 = scratch_return_string;
     while(*p1) {
           if((line < rfrom) || (line > rto)) {

              /* ---->  Current line isn't within <LINE RANGE>  -  skip it  <---- */
              while(*p1 && (*p1 != '\n')) p1++;
	   } else {

              /* ---->  Current line is within <LINE RANGE>  -  add it  <---- */
              if(p2 != scratch_return_string) *p2++ = '\n';
              while(*p1 && (*p1 != '\n')) *p2++ = *p1, p1++;
	   }
           if(*p1) p1++;
           line++;
     }
     *p2 = '\0';

     p1 = (char *) malloc_string(scratch_return_string);
     return(p1);
}

/* ---->  Insert TEXT into text above line LINENO  <---- */
int insert(struct descriptor_data *d,char *text,int lineno,int overwrite,unsigned char jump)
{
    int  editline = 1,newline = 0;
    char *p1,*p2,*p3;

    /* ---->  Truncate TEXT if > MAX_LENGTH (3k)  <---- */
    if(strlen(text) > MAX_LENGTH) text[MAX_LENGTH] = '\0';

    p1 = d->edit->text;
    p2 = scratch_return_string;
    for(; *p1 && (editline < lineno); *p2++ = *p1, p1++)
        if(*p1 && (*p1 == '\n')) editline++, newline = 1;
           else newline = 0;
    if(!*p1 && *d->edit->text && (!newline || (editline < lineno)))
       if(overwrite || (editline < lineno)) *p2++ = '\n';
    if(!*d->edit->text && !*text) lineno = 0;

    /* ---->  Insert/overwrite text  <---- */
    if(overwrite) for(; *p1 && (*p1 != '\n'); p1++);
    for(p3 = text; *p3; *p2++ = *p3, p3++);

    /* ---->  Add remainder of original text  <---- */
    if(!overwrite && *p1) *p2++ = '\n';
    for(; *p1; *p2++ = *p1, p1++);
    *p2 = '\0';

    /* ---->  Ensure length of resulting text (From insertation) isn't longer than MAX_LENGTH chars  <---- */
    if(strlen(scratch_return_string) > MAX_LENGTH) return(0);

    FREENULL(d->edit->text);
    d->edit->text = (char *) malloc_string(scratch_return_string);
    if(jump) d->edit->line = lineno + 1;
    return(1);
}

/* ---->  Delete the line range RFROM to RTO from D->EDITTEXT  <---- */
void delete(struct descriptor_data *d,int rfrom,int rto)
{
     int  line = 1,cr = 0;
     char *p1,*p2;

     p1 = d->edit->text;
     p2 = scratch_return_string;

     while(*p1) {
           if((line >= rfrom) && (line <= rto)) {

              /* ---->  Current line is within <LINE RANGE>  -  skip it  <---- */
              while(*p1 && (*p1 != '\n')) p1++;	      
              if(*p1) p1++;
              if(*p1) line++;

	   } else {

              /* ---->  Current line isn't within <LINE RANGE>  -  add it  <---- */
              if(cr) *p2++ = '\n', cr = 0;
              for(; *p1 && (*p1 != '\n'); *p2++ = *p1, p1++);
              if(*p1) cr = 1, p1++;
              line++;
	   }
     }
     if(cr && !((line >= rfrom) && (line <= rto))) *p2++ = '\n';
     *p2 = '\0';

     FREENULL(d->edit->text);
     d->edit->text = (char *) malloc_string(scratch_return_string);
}

/* ---->  Display edit text from line LINENO  <---- */
void display_edittext(struct descriptor_data *d,int lineno,int lineto,int lines)
{
     int  linecount = 0,spaces;
     char *p1,*p2;

     if(IsHtml(d)) {
        html_anti_reverse(d,1);
        output(d,d->player,1,2,0,"<BR><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">");
     }

     if(d->edit->line == (lines + 1)) lines++;
     if(lineno < 1)     lineno = 1;
     if(lineto < 1)     lineto = 1;
     if(lineno > lines) lineno = lines;
     if(lineto > lines) lineto = lines;

     if(lineno == lineto) {
        if(lineno <= d->edit->line) {
           linecount = 8 - (d->edit->line - lineno);
           if(linecount < 1) linecount = 0;
        }
     } else linecount = 16 - (lineto - lineno);
     p1 = get_lineno(lineno,d->edit->text);

     /* ---->  Header  <---- */
     of_object_or_array(d->player,d->edit->object,d->edit->field,d->edit->element,ANSI_LCYAN,d->edit->index);
     output(d,d->player,2,1,0,"%sEditing %s "ANSI_LWHITE"%s"ANSI_LCYAN"%s...%s",IsHtml(d) ? "\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER COLSPAN=2><FONT SIZE=4><I>\016"ANSI_LCYAN:"\n ",(d->edit->object == d->player) ? "your":"the",field_name(d->edit->field),(d->edit->object == d->player) ? "":scratch_return_string,IsHtml(d) ? "\016</I></FONT></TH></TR>\016":"\n");
     if(!IsHtml(d)) output(d,d->player,0,1,0,separator(d->terminal_width,0,'-','='));

     /* ---->  Edit text  <---- */
     while((linecount < 17) && (lineno <= lines)) {

           /* ---->  Get one line of text  <---- */
           for(p2 = scratch_return_string; *p1 && (*p1 != '\n'); *p2++ = *p1++);
           if(*p1) p1++;
           *p2 = '\0';

           /* ---->  Display line of text  <---- */
           if(lineno > 0) {
              if(db[d->player].flags2 & EDIT_NUMBERING) {

                 /* ---->  Display line with line numbers  <---- */
                 if(Valid(d->edit->object) && (Typeof(d->edit->object) == TYPE_COMMAND)) {
                    for(p2 = scratch_return_string, spaces = 0; *p2 && (*p2 == ' '); p2++, spaces++);
                    if((spaces + 8) > 50) spaces = (50 - 8);
		 } else spaces = 0;
                 output(d,d->player,2,1,8 + spaces,"%s%s[%03d%s%s%s%s%s",IsHtml(d) ? "\016<TR ALIGN=LEFT><TH WIDTH=10% BGCOLOR="HTML_TABLE_GREEN"><TT>\016":" ",(lineno == d->edit->line) ? ANSI_LGREEN:ANSI_DGREEN,lineno,(lineno == d->edit->line) ? "->":"]",IsHtml(d) ? "\016</TT></TH><TD>\016":(lineno == d->edit->line) ? " ":"  ",(lineno == d->edit->line) ? ANSI_LWHITE:ANSI_DWHITE,(IsHtml(d) && BlankContent(scratch_return_string)) ? "\016&nbsp;\016":scratch_return_string,IsHtml(d) ? "\016</TD></TR>\016":"\n");
	      } else if(!IsHtml(d)) {

                 /* ---->  Display line without line numbers  <---- */
                 if(Valid(d->edit->object) && (Typeof(d->edit->object) == TYPE_COMMAND)) {
                    for(p2 = scratch_return_string, spaces = 0; *p2 && (*p2 == ' '); p2++, spaces++);
                    if((spaces + 1) > 50) spaces = (50 - 1);
		 } else spaces = 0;
                 output(d,d->player,0,1,1 + spaces,"%s%s",(lineno == d->edit->line) ? ANSI_LGREEN">"ANSI_LWHITE:ANSI_DGREEN"~"ANSI_DWHITE,scratch_return_string);
	      } else output(d,d->player,2,1,0,"\016<TR><TH WIDTH=4%% BGCOLOR="HTML_TABLE_GREEN">%s</TH><TD ALIGN=LEFT>\016%s%s\016</TD></TR>\016",(lineno == d->edit->line) ? (IsHtml(d) ? ANSI_LGREEN"->":ANSI_LGREEN">"):(IsHtml(d) ? "&nbsp;":ANSI_DGREEN"~"),(lineno == d->edit->line) ? ANSI_LWHITE:ANSI_DWHITE,BlankContent(scratch_return_string) ? "\016&nbsp;\016":scratch_return_string);
	   }
           linecount++, lineno++;
     }

     if(IsHtml(d)) {
        output(d,d->player,1,2,0,"</TABLE><BR>");
        html_anti_reverse(d,1);
     } else if(!in_command) output(d,d->player,0,1,0,separator(d->terminal_width,1,'-','='));
}

/* ---->  Exit editor, optionally aborting save  <---- */
void exit_editor(struct descriptor_data *d,int save)
{
     dbref    editsucc = d->edit->succ,editfail = d->edit->fail;
     unsigned char uf_ok = 0,anon = 0;
     
     switch(d->edit->field) {
            case 106:
                 anon = 1;
            case 100:

                 /* ---->  Add message to the BBS  <---- */
                 if(save) {
                    if(!Blank(d->edit->text)) {
                       struct bbs_topic_data *topic,*subtopic;
                       short  msgno;
                       time_t now;

                       if((topic = lookup_topic(d->player,NULL,&topic,&subtopic))) {
                          gettime(now);
                          bbs_cyclicdelete(topic->topic_id,(subtopic) ? subtopic->topic_id:0);
                          msgno = bbs_addmessage(d->player,topic->topic_id,(subtopic) ? subtopic->topic_id:0,d->edit->data1,d->edit->text,now,(anon) ? MESSAGE_ANON:0,0);
                          substitute(d->player,scratch_return_string,punctuate(d->edit->data1,2,'\0'),0,ANSI_LYELLOW,NULL,0);
                          if(d->flags & BBS_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                          if(subtopic) {
                             if(!Valid(editsucc)) output(d,d->player,0,1,0,ANSI_LGREEN"%sessage '"ANSI_LYELLOW"%s"ANSI_LGREEN"' added to the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)\n",(anon) ? "Anonymous m":"M",scratch_return_string,topic->name,subtopic->name,msgno);
                             if(anon) bbs_output_except(d->player,d->edit->temp,0,1,2,ANSI_LGREEN"An anonymous user adds a message with the subject '"ANSI_LYELLOW"%s"ANSI_LGREEN"' to the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)",scratch_return_string,topic->name,subtopic->name,msgno);
                                else bbs_output_except(d->player,d->edit->temp,0,1,2,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" adds a message with the subject '"ANSI_LYELLOW"%s"ANSI_LGREEN"' to the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)",Article(d->player,UPPER,DEFINITE),getcname(NOTHING,d->player,0,0),scratch_return_string,topic->name,subtopic->name,msgno);
			  } else {
                             if(!Valid(editsucc)) output(d,d->player,0,1,0,ANSI_LGREEN"%sessage '"ANSI_LYELLOW"%s"ANSI_LGREEN"' added to the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)\n",(anon) ? "Anonymous m":"M",scratch_return_string,topic->name,msgno);
                             if(anon) bbs_output_except(d->player,d->edit->temp,0,1,2,ANSI_LGREEN"An anonymous user adds a message with the subject '"ANSI_LYELLOW"%s"ANSI_LGREEN"' to the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)",scratch_return_string,topic->name,msgno);
                                else bbs_output_except(d->player,d->edit->temp,0,1,2,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" adds a message with the subject '"ANSI_LYELLOW"%s"ANSI_LGREEN"' to the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)",Article(d->player,UPPER,DEFINITE),getcname(NOTHING,d->player,0,0),scratch_return_string,topic->name,msgno);
			  }
                          uf_ok = 1;
		       } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, your current selected topic is no-longer valid  -  Unable to add message.");
		    } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you can't add a blank message to %s BBS.\n",tcz_full_name);
		 } else if(!Valid(editfail)) output(d,d->player,0,1,0,ANSI_LRED"Your message has been abandoned and has not been added to %s BBS.\n",tcz_full_name);
                 break;
            case 107:
                 anon = 1;
            case 101:

                 /* ---->  Reply to message on the BBS  <---- */
                 if(save) {
                    if(!Blank(d->edit->text)) {
                       struct bbs_topic_data *topic,*subtopic;
                       short  msgno;
                       time_t now;

                       if((topic = lookup_topic(d->player,NULL,&topic,&subtopic))) {
                          gettime(now);
                          bbs_cyclicdelete(topic->topic_id,(subtopic) ? subtopic->topic_id:0);
                          strcpy(scratch_buffer,decompress(d->edit->data->message.subject));
                          msgno = bbs_addmessage(d->player,topic->topic_id,(subtopic) ? subtopic->topic_id:0,scratch_buffer,d->edit->text,now,(anon) ? MESSAGE_REPLY|MESSAGE_ANON:MESSAGE_REPLY,d->edit->data->message.id);
                          substitute(d->player,scratch_return_string,scratch_buffer,0,ANSI_LYELLOW,NULL,0);
                          if(d->flags & BBS_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                          if(subtopic) {
                             if(!Valid(editsucc)) output(d,d->player,0,1,0,ANSI_LGREEN"%seply to the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' left in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)\n",(anon) ? "Anonymous r":"R",(d->edit->data->message.flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,topic->name,subtopic->name,msgno);
                             if(anon) bbs_output_except(d->player,d->edit->temp,0,1,2,ANSI_LGREEN"An anonymous user leaves a reply to the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)",(d->edit->data->message.flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,topic->name,subtopic->name,msgno);
                                else bbs_output_except(d->player,d->edit->temp,0,1,2,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" leaves a reply to the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)",Article(d->player,UPPER,DEFINITE),getcname(NOTHING,d->player,0,0),(d->edit->data->message.flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,topic->name,subtopic->name,msgno);
			  } else {
                             if(!Valid(editsucc)) output(d,d->player,0,1,0,ANSI_LGREEN"%seply to the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' left in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)\n",(anon) ? "Anonymous r":"R",(d->edit->data->message.flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,topic->name,msgno);
                             if(anon) bbs_output_except(d->player,d->edit->temp,0,1,2,ANSI_LGREEN"An anonymous user leaves a reply to the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)",(d->edit->data->message.flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,topic->name,msgno);
                                else bbs_output_except(d->player,d->edit->temp,0,1,2,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" leaves a reply to the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%d"ANSI_LGREEN".)",Article(d->player,UPPER,DEFINITE),getcname(NOTHING,d->player,0,0),(d->edit->data->message.flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,topic->name,msgno);
			  }
                          uf_ok = 1;
		       } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, your current selected topic is no-longer valid  -  Unable to leave your reply.");
		    } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you can't leave a blank reply to a message on %s BBS.\n",tcz_full_name);
		 } else if(!Valid(editfail)) output(d,d->player,0,1,0,ANSI_LRED"Your reply has been abandoned and has not been left on %s BBS.\n",tcz_full_name);
                 d->edit->data->message.flags &= ~MESSAGE_MODIFY;
                 break;
            case 102:

                 /* ---->  Modify (Edit) message on the BBS  <---- */
                 if(save) {
                    if(!Blank(d->edit->text)) {
                       struct bbs_topic_data *topic,*subtopic;
		       struct   bbs_reader_data *reader,*next,*last = NULL;

                       if((topic = lookup_topic(d->player,NULL,&topic,&subtopic))) {
                          update_field(d);
                          anon = ((d->edit->data->message.flags & MESSAGE_ANON) != 0);
                          substitute(d->player,scratch_return_string,decompress(d->edit->data->message.subject),0,ANSI_LYELLOW,NULL,0);
                          if(d->flags & BBS_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
			  for(reader = d->edit->data->message.readers; reader; reader = next) {
			      next = reader->next;
			      if(!(reader->flags & READER_VOTE_MASK)) {
				  if(last) last->next = reader->next;
				  else d->edit->data->message.readers =
					   reader->next;
				  FREENULL(reader);
			      } else {
				  reader->flags &= ~(READER_READ|READER_IGNORE);
				  last           =  reader;
			      }
			  }
                          if(subtopic) {
                             if(!Valid(editsucc)) output(d,d->player,0,1,0,ANSI_LGREEN"Changes to the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%s"ANSI_LGREEN") in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' saved.\n",(d->edit->data->message.flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,d->edit->data1,topic->name,subtopic->name);
                             if(anon) bbs_output_except(d->player,d->edit->temp,0,1,2,ANSI_LGREEN"An anonymous user makes some changes to the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%s"ANSI_LGREEN".)",(d->edit->data->message.flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,topic->name,subtopic->name,d->edit->data1);
                                else bbs_output_except(d->player,d->edit->temp,0,1,2,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" makes some changes to the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%s"ANSI_LGREEN".)",Article(d->player,UPPER,DEFINITE),getcname(NOTHING,d->player,0,0),(d->edit->data->message.flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,topic->name,subtopic->name,d->edit->data1);
			  } else {
                             if(!Valid(editsucc)) output(d,d->player,0,1,0,ANSI_LGREEN"Changes to the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%s"ANSI_LGREEN") in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' saved.\n",(d->edit->data->message.flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,d->edit->data1,topic->name);
                             if(anon) bbs_output_except(d->player,d->edit->temp,0,1,2,ANSI_LGREEN"An anonymous user makes some changes to the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%s"ANSI_LGREEN".)",(d->edit->data->message.flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,topic->name,d->edit->data1);
                                else bbs_output_except(d->player,d->edit->temp,0,1,2,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" makes some changes to the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%s"ANSI_LGREEN".)",Article(d->player,UPPER,DEFINITE),getcname(NOTHING,d->player,0,0),(d->edit->data->message.flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,topic->name,d->edit->data1);
			  }
                          if(Owner(d->player) != d->edit->data->message.owner)
                             writelog(BBS_LOG,1,"MODIFY","%s%s(#%d) modified the message.",bbs_logmsg(&(d->edit->data->message),topic,subtopic,atol(d->edit->data1),0),getname(d->player),d->player); 
                          uf_ok = 1;
		       } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, your current selected topic is no-longer valid  -  Unable to save your changes.");
		    } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you can't wipe the message text of a message  -  Please use the '"ANSI_LWHITE"delete"ANSI_LGREEN"' command to delete messages from the %s BBS.\n",tcz_full_name);
		 } else if(!Valid(editfail)) output(d,d->player,0,1,0,ANSI_LRED"Your changes have been abandoned.\n");
                 d->edit->data->message.flags &= ~MESSAGE_MODIFY;
                 break;
            case 108:
                 anon = 1;
            case 103:

                 /* ---->  Append to message on the BBS  <---- */
                 if(save) {
                    if(!Blank(d->edit->text)) {
                       struct bbs_topic_data *topic,*subtopic;

                       if((topic = lookup_topic(d->player,NULL,&topic,&subtopic))) {
                          bbs_appendmessage(d->player,&(d->edit->data->message),topic,subtopic,d->edit->text,anon);
                          substitute(d->player,scratch_return_string,decompress(d->edit->data->message.subject),0,ANSI_LYELLOW,NULL,0);
                          if(d->flags & BBS_CENSOR) bad_language_filter(scratch_return_string,scratch_return_string);
                          if(subtopic) {
                             if(!Valid(editsucc)) output(d,d->player,0,1,0,ANSI_LGREEN"Message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%s"ANSI_LGREEN") appended to%s in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.\n",(d->edit->data->message.flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,d->edit->data1,(anon) ? " anonymously":"",topic->name,subtopic->name);
                             if(anon) bbs_output_except(d->player,d->edit->temp,0,1,2,ANSI_LGREEN"An anonymous user appends to the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%s"ANSI_LGREEN".)",(d->edit->data->message.flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,topic->name,subtopic->name,d->edit->data1);
                                else bbs_output_except(d->player,d->edit->temp,0,1,2,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" appends to the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' in the sub-topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%s"ANSI_LGREEN".)",Article(d->player,UPPER,DEFINITE),getcname(NOTHING,d->player,0,0),(d->edit->data->message.flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,topic->name,subtopic->name,d->edit->data1);
			  } else {
                             if(!Valid(editsucc)) output(d,d->player,0,1,0,ANSI_LGREEN"Message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%s"ANSI_LGREEN") appended to%s in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"'.\n",(d->edit->data->message.flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,d->edit->data1,(anon) ? " anonymously":"",topic->name);
                             if(anon) bbs_output_except(d->player,d->edit->temp,0,1,2,ANSI_LGREEN"An anonymous user appends to the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%s"ANSI_LGREEN".)",(d->edit->data->message.flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,topic->name,d->edit->data1);
                                else bbs_output_except(d->player,d->edit->temp,0,1,2,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" appends to the message '%s"ANSI_LYELLOW"%s"ANSI_LGREEN"' in the topic '"ANSI_LWHITE"%s"ANSI_LGREEN"' (Message number "ANSI_LYELLOW"%s"ANSI_LGREEN".)",Article(d->player,UPPER,DEFINITE),getcname(NOTHING,d->player,0,0),(d->edit->data->message.flags & MESSAGE_REPLY) ? ANSI_LMAGENTA"Re:  ":"",scratch_return_string,topic->name,d->edit->data1);
			  }
                          if(Owner(d->player) != d->edit->data->message.owner)
                             writelog(BBS_LOG,1,"APPEND","%s%s(#%d) appended to the message.",bbs_logmsg(&(d->edit->data->message),topic,subtopic,atol(d->edit->data1),0),getname(d->player),d->player); 
                          uf_ok = 1;
		       } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, your current selected topic is no-longer valid  -  Unable to append to message.");
		    } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you can't append blank text to a message on %s BBS.\n",tcz_full_name);
		 } else if(!Valid(editfail)) output(d,d->player,0,1,0,ANSI_LRED"Your append has been abandoned.\n");
                 d->edit->data->message.flags &= ~MESSAGE_MODIFY;
                 break;
            case 104:

                 /* ---->  Send mail  <---- */
                 if(1) {
                    struct list_data *ptr,*next;

                    if(save) {
                       if(!Blank(d->edit->text)) {
                          struct mlist_data *message;
                          int ic_cache = in_command;

                          if(Valid(editsucc)) in_command = 1;
                          message = (struct mlist_data *) d->edit->data;
		          uf_ok = mail_send_message(d->player,&message,d->edit->text,d->edit->data1,d->edit->data2,d->edit->temp >> 4,d->edit->temp & 0xF);
                          in_command = ic_cache;
		       } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you can't send a blank mail to %s.\n",pagetell_construct_list(d->player,d->player,d->edit->data,d->edit->temp >> 4,scratch_return_string,ANSI_LWHITE,ANSI_LGREEN,0,d->edit->temp & 0xF,DEFINITE));
		    } else if(!Valid(editfail)) output(d,d->player,0,1,0,ANSI_LRED"Your message has been abandoned and has not been sent to %s.\n",pagetell_construct_list(d->player,d->player,d->edit->data,d->edit->temp >> 4,scratch_return_string,ANSI_LWHITE,ANSI_LRED,0,d->edit->temp & 0xF,DEFINITE));

                    /* ---->  Wipe list  <---- */
                    for(ptr = &(d->edit->data->list); ptr; ptr = next) {
                        next = ptr->next;
                        FREENULL(ptr);
		    }
		 }
                 break;
            case 105:

                 /* ---->  Reply to mail  <---- */
                 if(1) {
                    struct mail_data *ptr = &(d->edit->data->mail);

                    if(save) {
                       if(!Blank(d->edit->text)) {
                          int ic_cache = in_command;

                          if(Valid(editsucc)) in_command = 1;
		          uf_ok = mail_reply_message(d->player,d->edit->data->mail.lastread,d->edit->data->mail.id,ptr,d->edit->temp,d->edit->text);
                          in_command = ic_cache;
		       } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you can't mail a blank reply to %s"ANSI_LWHITE"%s"ANSI_LGREEN".\n",Article(d->edit->data->mail.who,LOWER,DEFINITE),getcname(NOTHING,d->edit->data->mail.who,0,0));
		    } else if(!Valid(editfail)) output(d,d->player,0,1,0,ANSI_LRED"Your reply has been abandoned and has not been sent to %s"ANSI_LWHITE"%s"ANSI_LRED".\n",Article(d->edit->data->mail.who,LOWER,DEFINITE),getcname(NOTHING,d->edit->data->mail.who,0,0));
                    FREENULL(ptr);
		 }
                 break;
            default:

                 /* ---->  Object  <---- */
                 if(!save) {
                    if(!Valid(editfail)) {
                       of_object_or_array(d->player,d->edit->object,d->edit->field,d->edit->element,ANSI_LGREEN,d->edit->index);
                       output(d,d->player,0,1,0,ANSI_LGREEN"Changes to %s "ANSI_LWHITE"%s"ANSI_LGREEN"%s abandoned  -  Editing session finished.\n",(d->edit->object == d->player) ? "your":"the",field_name(d->edit->field),(d->edit->object == d->player) ? "":scratch_return_string);
		    }

                    /* ---->  Destroy created object  <---- */
                    if((d->edit->objecttype != NOTHING) && Valid(d->edit->object)) {
                       int ic_cache = in_command;
                       in_command   = 1;

                       if(!Valid(editfail)) output(d,d->player,0,1,0,ANSI_LGREEN"%s %s"ANSI_LWHITE"%s"ANSI_LGREEN" destroyed (Created at the beginning of this editing session.)\n",object_name(d->edit->objecttype),Article(d->edit->object,LOWER,DEFINITE),unparse_object(d->player,d->edit->object,0));
                       destroy_object(d->player,d->edit->object,0,0,0,0);
                       in_command = ic_cache;
		    }
		 } else if((uf_ok = update_field(d))) {
                    if(!Valid(editsucc)) {
                       of_object_or_array(d->player,d->edit->object,d->edit->field,d->edit->element,ANSI_LGREEN,d->edit->index);
                       output(d,d->player,0,1,0,ANSI_LGREEN"Changes to %s "ANSI_LWHITE"%s"ANSI_LGREEN"%s saved  -  Editing session finished.\n",(d->edit->object == d->player) ? "your":"the",field_name(d->edit->field),(d->edit->object == d->player) ? "":scratch_return_string);
		    }
                    uf_ok = 1;
		 } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the object you were originally editing no-longer exists  -  Unable to save changes.\n");
     }
				
     /* ---->  Shutdown editor  <---- */
     FREENULL(d->user_prompt);
     d->user_prompt = (char *) malloc_string(d->edit->prompt);
     FREENULL(d->edit->prompt);
     FREENULL(d->edit->index);
     FREENULL(d->edit->data1);
     FREENULL(d->edit->data2);
     FREENULL(d->edit->text);
     FREENULL(d->edit);
     d->flags2 &= ~ABSOLUTE_OVERRIDE;
     d->flags  &= ~(NOLISTEN|BBS_CENSOR|ABSOLUTE);
     d->flags  |=  EVALUATE;

     /* ---->  Execute .editsucc/.editfail compound command  <---- */
     if(!uf_ok) editsucc = editfail;
     if(Valid(editsucc) && (Typeof(editsucc) == TYPE_COMMAND) && could_satisfy_lock(d->player,editsucc,0)) {
        command_clear_args();
        command_cache_execute(d->player,editsucc,1,0);
     }
}

/* ---->  Abort edit (Abandoning changes) or save changes (Leaving editor in both cases)  <---- */
/*        (val1:  0 = Abort, 1 = Save.)                                                         */
void edit_abort_or_save(EDIT_CONTEXT)
{
     if(!val1) {
        if(!(*text && string_prefix("yes",params))) {
           of_object_or_array(d->player,d->edit->object,d->edit->field,d->edit->element,ANSI_LGREEN,d->edit->index);
           output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you must type '"ANSI_LWHITE"%s = yes"ANSI_LGREEN"' to abandon the changes made to %s "ANSI_LWHITE"%s"ANSI_LGREEN"%s.\n",editcmd,(d->edit->object == d->player) ? "your":"the",field_name(d->edit->field),(d->edit->object == d->player) ? "":scratch_return_string);
        } else exit_editor(d,0);
     } else exit_editor(d,1);
}

/* ---->  Append text to end of line  <---- */
void edit_append(EDIT_CONTEXT)
{
     char *clipboard;

     clipboard = get_range(d,rfrom,rfrom);
     sprintf(scratch_buffer,"%s%s",String(clipboard),String(text));
     if(!insert(d,scratch_buffer,rfrom,1,0)) output(d,d->player,0,1,0,ANSI_LRED"Sorry, appending to that line will make your edit text too large  -  Nothing appended.\n");
     FREENULL(clipboard);
}

/* ---->  Copy line(s)  <---- */
void edit_copy(EDIT_CONTEXT)
{
     char *clipboard;

     if(option) {
        clipboard = get_range(d,rfrom,rto);
        if(!insert(d,clipboard,option,(db[d->player].flags2 & EDIT_OVERWRITE),0))
           output(d,d->player,0,1,0,ANSI_LRED"Sorry, making a copy of that text will make your edit text too large  -  Nothing copied.\n");
              else if(rfrom < d->edit->line) d->edit->line += ((rto - rfrom) + 1);
        FREENULL(clipboard);
     } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, destination line for copied text not specified  -  Text hasn't been copied.\n");
}

/* ---->  Delete line(s)  <---- */
void edit_delete(EDIT_CONTEXT)
{
     if(rfrom < d->edit->line) d->edit->line -= ((rto - rfrom) + 1);
     delete(d,rfrom,rto);
}

/* ---->  Toggle evaluate on/off  <---- */
void edit_evaluate(EDIT_CONTEXT)
{
     if(Blank(params)) {
        if(db[d->player].flags2 & EDIT_EVALUATE) {
           db[d->player].flags2 &= ~EDIT_EVALUATE;
           d->flags             &= ~EVALUATE;
	} else {
           db[d->player].flags2 |= EDIT_EVALUATE;
           d->flags             |= EVALUATE;
	}
     } else if(string_prefix("on",params) || string_prefix("yes",params)) {
        db[d->player].flags2 |=  EDIT_EVALUATE;
        d->flags             |=  EVALUATE;
     } else if(string_prefix("off",params) || string_prefix("no",params)) {
        db[d->player].flags2 &= ~EDIT_EVALUATE;
        d->flags             &= ~EVALUATE;
     } else {
        output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, evaluation of "ANSI_LYELLOW"{}"ANSI_LGREEN"'s, "ANSI_LYELLOW"$"ANSI_LGREEN"'s and "ANSI_LYELLOW"\\"ANSI_LGREEN"'s can only be set to '"ANSI_LWHITE"yes"ANSI_LGREEN"' or '"ANSI_LWHITE"no"ANSI_LGREEN"'.\n");
        return;
     }

     if(db[d->player].flags2 & EDIT_EVALUATE) output(d,d->player,0,1,0,ANSI_LYELLOW"{}"ANSI_LGREEN"'s, "ANSI_LYELLOW"$"ANSI_LGREEN"'s and "ANSI_LYELLOW"\\"ANSI_LGREEN"'s "ANSI_LWHITE"WILL"ANSI_LGREEN" be evaluated.\n");
        else output(d,d->player,0,1,0,ANSI_LYELLOW"{}"ANSI_LGREEN"'s, "ANSI_LYELLOW"$"ANSI_LGREEN"'s and "ANSI_LYELLOW"\\"ANSI_LGREEN"'s "ANSI_LWHITE"WILL NOT"ANSI_LGREEN" be evaluated.\n");
}

/* ---->  Execute given TCZ command  <---- */
void edit_execute(EDIT_CONTEXT)
{
     unsigned char abort = 0;

     if(!Blank(params)) {
        d->flags2 |= ABSOLUTE_OVERRIDE;
        abort     |= event_trigger_fuses(d->player,d->player,params,FUSE_ARGS);
        abort     |= event_trigger_fuses(d->player,Location(d->player),params,FUSE_ARGS);
        if(!abort) process_basic_command(d->player,params,0);
     } else output(d,d->player,0,1,0,ANSI_LGREEN"Which %s command would you like to execute?\n",tcz_short_name);
}

/* ---->  Change field being edited  <---- */
void edit_field(EDIT_CONTEXT)
{
     int uf_ok;

     switch(d->edit->field) {
            case 100:  /*  ---->  Add new message to the BBS  <---- */
                 output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you can't update the changes made to a new message on %s BBS (Either type '"ANSI_LWHITE".save"ANSI_LGREEN"' to add the new message to %s BBS or '"ANSI_LWHITE".abort = yes"ANSI_LGREEN"' to abandon it.)\n",tcz_full_name,tcz_full_name);
                 break;
            case 101:  /*  ---->  Reply to message on the BBS  <---- */
                 output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you can't update the changes made to a reply to a message on %s BBS (Either type '"ANSI_LWHITE".save"ANSI_LGREEN"' to add the reply to %s BBS or '"ANSI_LWHITE".abort = yes"ANSI_LGREEN"' to abandon it.)\n",tcz_full_name,tcz_full_name);
                 break;
            case 103:  /*  ---->  Append to message on the BBS  <---- */
                 output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you can't update the changes made to an append to a message on %s BBS (Either type '"ANSI_LWHITE".save"ANSI_LGREEN"' to append to the message on %s BBS or '"ANSI_LWHITE".abort = yes"ANSI_LGREEN"' to abandon it.)\n",tcz_full_name,tcz_full_name);
                 break;
            case 104:  /*  ---->  Send mail  <---- */
                 output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you can't update the changes made to a mailed message to another character (Either type '"ANSI_LWHITE".save"ANSI_LGREEN"' to mail the message or %s BBS or '"ANSI_LWHITE".abort = yes"ANSI_LGREEN"' to abandon it.)\n",tcz_full_name);
                 break;
            case 105:  /*  ---->  Reply to mail  <---- */
                 output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you can't update the changes made to a reply to a mailed message from another character (Either type '"ANSI_LWHITE".save"ANSI_LGREEN"' to mail the reply or %s BBS or '"ANSI_LWHITE".abort = yes"ANSI_LGREEN"' to abandon it.)\n",tcz_full_name);
                 break;
            case 106:  /*  ---->  Add anonymous new message to the BBS  <---- */
                 output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you can't update the changes made to a new anonymous message on %s BBS (Either type '"ANSI_LWHITE".save"ANSI_LGREEN"' to add the new message to %s BBS or '"ANSI_LWHITE".abort = yes"ANSI_LGREEN"' to abandon it.)\n",tcz_full_name,tcz_full_name);
                 break;
            case 107:  /*  ---->  Anonymous reply to message on the BBS  <---- */
                 output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you can't update the changes made to an anonymous reply to a message on %s BBS (Either type '"ANSI_LWHITE".save"ANSI_LGREEN"' to add the reply to %s BBS or '"ANSI_LWHITE".abort = yes"ANSI_LGREEN"' to abandon it.)\n",tcz_full_name,tcz_full_name);
                 break;
            default:

                 /* ---->  Save changes to current field being edited  <---- */
                 if((uf_ok = update_field(d))) {
                    of_object_or_array(d->player,d->edit->object,d->edit->field,d->edit->element,ANSI_LGREEN,d->edit->index);
                    output(d,d->player,0,1,0,ANSI_LGREEN"Changes to %s "ANSI_LWHITE"%s"ANSI_LGREEN"%s saved...",(d->edit->object == d->player) ? "your":"the",field_name(d->edit->field),(d->edit->object == d->player) ? "":scratch_return_string);

                    if(*params) {
                       int  newfield = 10,newelement = NOTHING;
                       char *textptr,*newindex = NULL;

                       if(!d->edit->permanent) {
                          if(Typeof(d->edit->object) == TYPE_ARRAY) {
                             if(!(string_prefix("FIRST",params) || string_prefix("ALL",params))) {
                                if(!string_prefix("LAST",params)) {
                                   if(!string_prefix("END",params)) {
                                      if(!string_prefix("previous",params)) {
		   		         if(!string_prefix("next",params)) {
                                            int  ascii = 0;
                                            char *ptr;

                                            for(; *params && (*params == ' '); params++);
                                            if(strlen(params) > 128) params[128] = '\0';
                                            for(ptr = params; *ptr; ptr++) {
                                                if(!isdigit(*ptr)) ascii++;
                                                if((*ptr == '[') || (*ptr == ']') || (*ptr == '-') || (*ptr == '.')) {
                                                   output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the index name of a dynamic array element mustn't contain the characters '"ANSI_LYELLOW"["ANSI_LGREEN"', '"ANSI_LYELLOW"]"ANSI_LGREEN"', '"ANSI_LYELLOW"-"ANSI_LGREEN"' or '"ANSI_LYELLOW"."ANSI_LGREEN"'.");
                                                   return;
						}
					    }
                                            for(ptr--; (ptr > params) && (*ptr == ' '); *ptr = '\0', ptr--);

                                            if(ascii > 0) newelement = INDEXED, newindex = params;
	   			               else if((newelement = atol(params)) <= 0) newelement = 1;
					 } else {
                                            newelement = d->edit->element;
                                            array_nextprev_element(d->player,d->edit->object,&newelement,d->edit->index,&newindex,1);
					 }
				      } else {
                                         newelement = d->edit->element;
                                         array_nextprev_element(d->player,d->edit->object,&newelement,d->edit->index,&newindex,0);
				      }
				   } else newelement = END;
				} else newelement = LAST;
			     } else newelement = 1;
			  } else newfield = fieldtype(params);

                          if(((newfield != d->edit->field) || ((newelement != d->edit->element) || (newelement == END)) || (newelement == INDEXED)) && can_edit(d,newfield,d->edit->object,1)) {

                             /* ---->  Edit new field/element  <---- */
                             if(!Blank(newindex) && (newelement = INDEXED)) {
                                strcpy(indexfrom,newindex);
                                *indexto = '\0';
			     }
                             textptr = get_field(d->player,newfield,d->edit->object,newelement);

                             FREENULL(d->edit->text);
                             if(textptr != (char *) NULL) d->edit->text = (char *) malloc_string(textptr);
                                else d->edit->text = (char *) malloc_string("");

                             FREENULL(d->edit->index);
                             d->edit->element = newelement;
                             d->edit->field   = newfield;
                             d->edit->index   = (char *) alloc_string(newindex);
                             d->edit->line    = strcnt(d->edit->text,'\n') + 1;

                             /* ---->  Write changes made to current field, or start editing a different field (Attached to the same object)  <---- */
                             of_object_or_array(d->player,d->edit->object,d->edit->field,d->edit->element,ANSI_LGREEN,d->edit->index);
                             output(d,d->player,0,1,0,ANSI_LGREEN"Now editing %s "ANSI_LWHITE"%s"ANSI_LGREEN"%s...\n",(d->edit->object == d->player) ? "your":"the",field_name(d->edit->field),(d->edit->object == d->player) ? "":scratch_return_string);
			  } else output(d,d->player,0,1,0,"");
		       } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you can't edit another field of %s"ANSI_LWHITE"%s"ANSI_LGREEN" (The '"ANSI_LYELLOW".permanent"ANSI_LGREEN"' editor command has been used to prevent you from editing other fields for the rest of this edit session.)\n");
		    } else output(d,d->player,0,1,0,"");
		 } else output(d,d->player,0,1,0,ANSI_LRED"Sorry, the object you were originally editing no-longer exists  -  Unable to save changes.\n");
     }
}

/* ---->  Call On-Line Help System from within editor  <---- */
void edit_help(EDIT_CONTEXT)
{
     if(*params) help_main(d->player,params,NULL,NULL,NULL,val1,0);  /*  Help/tutorial on given subject?        */
        else help_main(d->player,"editor",NULL,NULL,NULL,val1,0);    /*  Otherwise help/tutorial on the editor  */
}

/* ---->  Insert text above given line  <---- */
void edit_insert(EDIT_CONTEXT)
{
     option = d->edit->line;
     if(!insert(d,text,rfrom,0,jump)) output(d,d->player,0,1,0,ANSI_LRED"Sorry, your edit text is too large  -  Can't insert text.\n");
        else if(rfrom < option) d->edit->line += ((rto - rfrom) + 1);
}

/* ---->  Move line(s)  <---- */
void edit_move(EDIT_CONTEXT)
{
     char *clipboard;

     if(option) {
        if(rfrom != option) {
           clipboard = get_range(d,rfrom,rto);
           delete(d,rfrom,rto);
           if(rfrom < option) option -= ((rto - rfrom) + 1);
           if(!insert(d,clipboard,option,0,0)) {
              output(d,d->player,0,1,0,ANSI_LRED"Sorry, moving that text will make your edit text too large  -  Nothing moved.\n");
              insert(d,clipboard,rfrom,0,0);
	   } else if(rfrom < d->edit->line) d->edit->line += ((rto - rfrom) + 1);
           FREENULL(clipboard);
	}
     } else output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, destination line for moved text not specified  -  Text hasn't been moved.\n");
}

/* ---->  Toggle line numbers on/off  <---- */
void edit_numbering(EDIT_CONTEXT)
{
     if(Blank(params)) {
        if(db[d->player].flags2 & EDIT_NUMBERING) db[d->player].flags2 &= ~EDIT_NUMBERING;
           else db[d->player].flags2 |= EDIT_NUMBERING;
     } else if(string_prefix("on",params) || string_prefix("yes",params)) db[d->player].flags2 |= EDIT_NUMBERING;
         else if(!(string_prefix("off",params) || string_prefix("no",params))) {
            output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, line numbering can only be turned '"ANSI_LWHITE"on"ANSI_LGREEN"' or '"ANSI_LWHITE"off"ANSI_LGREEN"'.\n");
            return;
	 } else db[d->player].flags2 &= ~EDIT_NUMBERING;

     if(db[d->player].flags2 & EDIT_NUMBERING) output(d,d->player,0,1,0,ANSI_LGREEN"Line numbers "ANSI_LYELLOW"will"ANSI_LGREEN" now be displayed.\n");
        else output(d,d->player,0,1,0,ANSI_LGREEN"Line numbers "ANSI_LYELLOW"will no-longer"ANSI_LGREEN" be displayed.\n");
}

/* ---->  Make settings of '.editfail' and '.editsucc' permanent  <---- */
void edit_permanent(EDIT_CONTEXT)
{
     if(!d->edit->permanent) {
        d->edit->permanent = 1;
        if(!in_command) output(d,d->player,0,1,0,ANSI_LGREEN"The settings of '"ANSI_LWHITE".editsucc"ANSI_LGREEN"' and '"ANSI_LWHITE".editfail"ANSI_LGREEN"' are now permanent.\n");
        setreturn(OK,COMMAND_SUCC);
     } else {
        if(!in_command) output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, the settings of '"ANSI_LWHITE".editsucc"ANSI_LGREEN"' and '"ANSI_LWHITE".editfail"ANSI_LGREEN"' are already permanent.\n");
        setreturn(ERROR,COMMAND_FAIL);
     }
}

/* ---->  Toggle overwrite mode on/off  <---- */
void edit_overwrite(EDIT_CONTEXT)
{
     if(Blank(params)) {
        if(db[d->player].flags2 & EDIT_OVERWRITE) db[d->player].flags2 &= ~EDIT_OVERWRITE;
           else db[d->player].flags2 |= EDIT_OVERWRITE;
     } else if(string_prefix("on",params) || string_prefix("yes",params)) db[d->player].flags2 |= EDIT_OVERWRITE;
         else if(!(string_prefix("off",params) || string_prefix("no",params))) {
            output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, overwrite mode can only be turned '"ANSI_LWHITE"on"ANSI_LGREEN"' or '"ANSI_LWHITE"off"ANSI_LGREEN"'.\n");
            return;
	 } else db[d->player].flags2 &= ~EDIT_OVERWRITE;

     if(db[d->player].flags2 & EDIT_OVERWRITE) output(d,d->player,0,1,0,ANSI_LGREEN"Overwrite mode is now "ANSI_LYELLOW"on"ANSI_LGREEN".\n");
        else output(d,d->player,0,1,0,ANSI_LGREEN"Overwrite mode is now "ANSI_LYELLOW"off"ANSI_LGREEN".\n");
}

/* ---->  Set current position (Line) in edit text  <---- */
void edit_position(EDIT_CONTEXT)
{
     lines++;
     if(!option && rfrom) option = rfrom;
     if(option == d->edit->line) {

        /* ---->  Give user useful info. about text being edited  <---- */
        of_object_or_array(d->player,d->edit->object,d->edit->field,d->edit->element,ANSI_LGREEN,d->edit->index);
        output(d,d->player,0,1,0,ANSI_LGREEN"\nYou're currently editing line "ANSI_LWHITE"%d"ANSI_LGREEN" of %s "ANSI_LWHITE"%s"ANSI_LGREEN"%s.\n",d->edit->line,(d->edit->object == d->player) ? "your":"the",field_name(d->edit->field),(d->edit->object == d->player) ? "":scratch_return_string);
        output(d,d->player,0,1,0,(db[d->player].flags2 & EDIT_OVERWRITE) ? ANSI_LGREEN"Overwrite mode is "ANSI_LWHITE"on"ANSI_LGREEN".":ANSI_LGREEN"Overwrite mode is "ANSI_LWHITE"off"ANSI_LGREEN".");
        output(d,d->player,0,1,0,(db[d->player].flags2 & EDIT_NUMBERING) ? ANSI_LGREEN"Line numbers "ANSI_LWHITE"will"ANSI_LGREEN" be displayed.":ANSI_LGREEN"Line numbers "ANSI_LWHITE"will not"ANSI_LGREEN" be displayed.");
        output(d,d->player,0,1,0,(db[d->player].flags2 & EDIT_EVALUATE)  ? ANSI_LYELLOW"{}"ANSI_LGREEN"'s, "ANSI_LYELLOW"$"ANSI_LGREEN"'s and "ANSI_LYELLOW"\\"ANSI_LGREEN"'s "ANSI_LWHITE"WILL"ANSI_LGREEN" be evaluated.\n":ANSI_LYELLOW"{}"ANSI_LGREEN"'s, "ANSI_LYELLOW"$"ANSI_LGREEN"'s and "ANSI_LYELLOW"\\"ANSI_LGREEN"'s "ANSI_LWHITE"WILL NOT"ANSI_LGREEN" be evaluated.\n");
     } else {
        d->edit->line                           = option;
        if(d->edit->line < 1) d->edit->line     = 1;
        if(d->edit->line > lines) d->edit->line = lines;
     }
}

/* ---->  Replace text  <---- */
void edit_replace(EDIT_CONTEXT)
{
     char *clipboard;
     int  result = 0;

     clipboard = get_range(d,rfrom,rto);
     delete(d,rfrom,rto);
     if((strlen(text) + strlen(clipboard) + 3) <= MAX_LENGTH) {
        sprintf(editcmd,"%s \x02%s\x02",text,clipboard);
        result = query_replace(d->player,editcmd,NULL,NULL,NULL,1,0);
        output(d,d->player,0,1,0,"");
        if(result <= 0) {
           if(!result) output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, invalid parameters to '"ANSI_LWHITE".replace"ANSI_LGREEN"'  -  See '"ANSI_LYELLOW".help .replace"ANSI_LGREEN"' for help on using '"ANSI_LWHITE".replace"ANSI_LGREEN"'.\n");
              else output(d,d->player,0,1,0,ANSI_LRED"Sorry, replace has made your edit text too large  -  Effects of replace have been discarded.\n");
           insert(d,clipboard,rfrom,0,0);
	} else if(!insert(d,(char *) command_result,rfrom,0,0)) {
           output(d,d->player,0,1,0,ANSI_LRED"Sorry, replace has made your edit text too large  -  Effects of replace have been discarded.\n");
           insert(d,clipboard,rfrom,0,0);
	}
     } else {
        output(d,d->player,0,1,0,ANSI_LRED"Sorry, replace has made your edit text too large  -  Effects of replace have been discarded.\n");
        insert(d,clipboard,rfrom,0,0);
     }
     FREENULL(clipboard);
}

/* ---->  Set given line (Or range of lines) to specified text  <---- */
void edit_set(EDIT_CONTEXT)
{
     if(!insert(d,text,rfrom,1,0))
        output(d,d->player,0,1,0,ANSI_LRED"Sorry, your edit text is too large  -  Can't overwrite text.\n");
}

/* ---->  Set compound command to execute on saving changes ('.editsucc') or command to execute on aborting them ('.editfail')  <---- */
/*        (val1:  0 = Succ, 1 = Fail.)                                                                                                */
void edit_succ_or_fail(EDIT_CONTEXT)
{
     dbref command;

     setreturn(ERROR,COMMAND_FAIL);
     if(d->edit->permanent) {
        output(d,d->player,0,1,0,ANSI_LGREEN"Sorry, you can't set the compound command that will be executed when you leave the editor and %s your changes during this edit session.\n",(val1) ? "abandon":"save");
        return;
     }

     if(!Blank(params)) {
        command = parse_link_command(d->player,NOTHING,params,255);
        if(!Valid(command)) return;
     } else command = NOTHING;

     if(!val1) d->edit->succ = command;
        else d->edit->fail   = command;

     if(!in_command) {
        if(Valid(command)) output(d,d->player,0,1,0,ANSI_LGREEN"The compound command '"ANSI_LWHITE"%s"ANSI_LGREEN"' will be executed on exiting the editor and %s changes.\n",unparse_object(d->player,command,0),(val1) ? "abandoning":"saving");
	   else output(d,d->player,0,1,0,ANSI_LGREEN"No compound command will be executed on exiting the editor and %s changes.\n",(val1) ? "abandoning":"saving");
     }
     setreturn(OK,COMMAND_SUCC);
}

/* ---->       Move to top or bottom of edit text       <---- */
/*        (val1:  0 = Top, 1 = Bottom, 2 = Last line.)        */
void edit_top_or_bottom(EDIT_CONTEXT)
{
     switch(val1) {
            case 0:
                 d->edit->line = 1;
                 break;
            case 1:
                 d->edit->line = lines + 1;
                 break;
            case 2:
                 d->edit->line = lines;
                 break;
     }
}

/* ---->  Move up or down edit text by given number of lines  <---- */
/*        (val1:  0 = Up, 1 = Down)                                 */
void edit_up_or_down(EDIT_CONTEXT)
{
     if(!option && numeric) option = numeric;
     if(!val1) {
        if(option) d->edit->line -= option;
           else d->edit->line--;
        if(d->edit->line < 1) d->edit->line = 1;
     } else {
        if(option) d->edit->line += option;
           else d->edit->line++;
        if(d->edit->line > (lines + 1)) d->edit->line = lines + 1;
     }
}

/* ---->  Execute given TCZ command, substituting return value into edit text  <---- */
void edit_value(EDIT_CONTEXT)
{
     for(; *text && (*text == ' '); text++);
     if(!Blank(text)) {
        if(*text && (*text != '{')) {
           if(!(db[d->player].flags2 & EDIT_EVALUATE)) {
              alias_substitute((in_command) ? db[current_command].owner:d->player,text,editcmd);
              substitute_command(d->player,editcmd,buffer);
              substitute_variable(d->player,buffer,editcmd);
	   }
           process_basic_command(d->player,editcmd,0);
           strcpy(buffer,(Blank(command_result)) ? "":command_result);
	} else if(!(db[d->player].flags2 & EDIT_EVALUATE)) {
           alias_substitute((in_command) ? db[current_command].owner:d->player,text,editcmd);
           substitute_command(d->player,editcmd,buffer);
           substitute_variable(d->player,buffer,editcmd);
           strcpy(buffer,editcmd);
	}
        option = d->edit->line;
        if(!insert(d,buffer,rfrom,(db[d->player].flags2 & EDIT_OVERWRITE),jump))
           output(d,d->player,0,1,0,ANSI_LRED"Sorry, your edit text is too large  -  Can't substitute return value of %s command into your edit text.\n",tcz_short_name);
              else if(rfrom < option) d->edit->line += ((rto - rfrom) + 1);
     } else output(d,d->player,0,1,0,ANSI_LGREEN"Which %s command would you like to insert the return value of?\n",tcz_short_name);
}

/* ---->  View text at around current position, or specified position/range  <---- */
void edit_view(EDIT_CONTEXT)
{
     if(rfrom == rto) {
        if(option) rfrom = option;
        if(!rfrom) rfrom = d->edit->line - 8;
           else if(rfrom == d->edit->line) rfrom = d->edit->line - 8;
        rto = rfrom;
     }
     display_edittext(d,rfrom,rto,lines);
}

/* ---->  Process editor command entered by user (Whilst editing something)  <---- */
void edit_process_command(struct descriptor_data *d,char *original_editcmd)
{
     int      rfrom = 0,rto = 0,option = 0,numeric = 0,lines;
     char     editcmd[TEXT_SIZE],buffer[TEXT_SIZE];
     char     *params,*text,*p1,*p2,*p3;
     struct   edit_cmd_table *cmd;
     unsigned char jump = 0;

     /* ---->  Validate character  <---- */
     if(original_editcmd == NULL) return;
     if(!Validchar(d->player)) {
        writelog(BUG_LOG,1,"BUG","(Process_editor_command() in edit.c)  Invalid character %s.",unparse_object(ROOT,d->player,0));
        return;
     }
     d->flags2 &= ~ABSOLUTE_OVERRIDE;

     editor_commands++;
     if(!(command_type & OTHER_COMMAND)) return;
     if(db[d->player].flags2 & EDIT_EVALUATE) {
        substitute_command(d->player,original_editcmd,editcmd);
        substitute_variable(d->player,editcmd,buffer);
     } else strcpy(buffer,original_editcmd);

     if(Censor(d->player) || Censor(db[d->player].location)) bad_language_filter(buffer,buffer);

     lines = strcnt(d->edit->text,'\n');
     if(d->edit->line > (lines + 1)) d->edit->line = lines + 1;
       else if(d->edit->line < 1) d->edit->line = 1;

     p1 = p2 = buffer;
     p3 = editcmd;

     /* ---->  Grab editor command  <---- */
     for(; *p2 && (*p2 == ' '); p2++);
     if(*p2 && (*p2 == '.')) p1 = p2;
     for(; *p1 && !((*p1 == ' ') || (*p1 == '=') || isdigit(*p1)); *p3++ = *p1, p1++);
     for(*p3 = '\0'; *p1 && (*p1 == ' '); p1++);
     params = p1;

     /* ---->  Grab RFROM and RTO (Range from and to values)  <---- */
     if(*p1 && (*p1 == '=')) {

        /* ---->  No RFROM/RTO specified  -  Assume user wants to do editor command on current line  <---- */
        rfrom = d->edit->line, rto = d->edit->line, jump = 1;
        for(params++; *params && (*params == ' '); params++);
     } else {

        /* ---->  Grab RFROM...  <---- */
        if(string_matched_prefix("first",p1)) rfrom = 1, p1 += 5;
	   else if(string_matched_prefix("last",p1)) rfrom = lines, p1 += 4;
	      else if(string_matched_prefix("end",p1)) rfrom = lines + 1, p1 += 3;
	         else if(string_matched_prefix("all",p1)) rfrom = -1, p1 += 3;
	            else if(string_matched_prefix("current",p1)) rfrom = d->edit->line, p1 += 7;
                       else {
                          for(p2 = scratch_return_string; *p1 && isdigit(*p1); *p2++ = *p1, p1++);
                          *p2 = '\0';
                          rfrom = atoi(scratch_return_string);
                          if(rfrom) numeric = rfrom;
		       }

        /* ---->  Skip over range separator ('-', 'to', etc.)  <---- */
        while(*p1 && (*p1 == ' ')) p1++;
        p3 = p1;
        if(*p3 && (*p3 == '-')) {
           for(p3++; *p3 && (*p3 == ' '); p3++);
           if(*p3 && isdigit(*p3)) p1 = p3;
	} else if(*p3 && (*p3 == '.')) {
           for(p3++; *p3 && (*p3 == '.'); p3++);
           while(*p3 && (*p3 == ' ')) p3++;
           if(*p3 && isdigit(*p3)) p1 = p3;
	}

        /* ---->  Grab RTO...  <---- */
        if(string_matched_prefix("first",p3)) rto = 1, p1 = p3 + 5;
           else if(string_matched_prefix("last",p3)) rto = lines, p1 = p3 + 4;
  	      else if(string_matched_prefix("end",p3)) rto = lines, p1 = p3 + 3;
                 else if(string_matched_prefix("all",p3)) rfrom = 1, rto = lines, p1 = p3 + 3;
	            else if(string_matched_prefix("current",p3)) rto = d->edit->line, p1 = p3 + 7;
	               else if(isdigit(*p1)) {
                          for(p2 = scratch_return_string; *p1 && isdigit(*p1); *p2++ = *p1, p1++);
                          *p2 = '\0';
                          rto = atoi(scratch_return_string);
		       } else rto = -1;

        if(rto > 0) {
           while(*p1 && (*p1 != ' ') && (*p1 != '=')) p1++;
           while(*p1 && (*p1 == ' ')) p1++;
	} else rto = rfrom;
     }
     if(*p1 && (*p1 == '=')) p1++;
     while(*p1 && (*p1 == ' ')) p1++;
     text = p1;

     /* ---->  Look for value after '=' (Or in place of text)  <---- */
     if(string_matched_prefix("first",p1)) option = 1, p1 += 5;
        else if(string_matched_prefix("last",p1)) option = lines, p1 += 4;
           else if(string_matched_prefix("end",p1)) option = lines + 1, p1 += 3;
              else if(string_matched_prefix("all",p1)) option = -1, p1 += 3;
                 else if(string_matched_prefix("current",p1)) option = d->edit->line, p1 += 7;
                    else {
                       for(p2 = scratch_return_string; *p1 && isdigit(*p1); *p2++ = *p1, p1++);
                       *p2 = '\0';
                       option = atoi(scratch_return_string);
                       if(option) numeric = option;
		    }

     /* ---->  Validate <LINE RANGE>  <---- */
     if(rfrom == -1) rfrom = 1, rto = lines;
     if(rfrom) {
        if((rfrom > rto )) {
           int temp = rfrom;
           rfrom    = rto;
           rto      = temp;
	}
     } else rfrom = d->edit->line, rto = d->edit->line, jump = 1;
     p1 = editcmd;

     /* ---->  Execute editor command  <---- */
     if(*p1 && (*p1 == '.')) {
        if(!(cmd = search_edit_cmdtable(++p1))) {
           output(d,d->player,0,1,0,ANSI_LGREEN"Unknown editing command '"ANSI_LWHITE"%s"ANSI_LGREEN"', %s"ANSI_LYELLOW"%s"ANSI_LGREEN".  "ANSI_LBLUE"(Type "ANSI_LCYAN""ANSI_UNDERLINE".HELP EDITCMDS"ANSI_LBLUE" for help!)\n",editcmd,Article(d->player,LOWER,INDEFINITE),getcname(NOTHING,d->player,0,0));
           return;
	} else cmd->func(d,rfrom,rto,option,numeric,lines,jump,text,params,cmd->val1,editcmd,buffer);
     } else if(!insert(d,buffer,d->edit->line,(db[d->player].flags2 & EDIT_OVERWRITE),1))
        output(d,d->player,0,1,0,ANSI_LRED"Sorry, your edit text is too large  -  Can't insert another line of text.\n");

     /* ---->  Change prompt to reflect current line number and total lines of edit text  <---- */
     if(d->edit) {
        lines = strcnt(d->edit->text,'\n');
        if(d->edit->line > (lines + 1)) d->edit->line = lines + 1;
           else if(d->edit->line < 1) d->edit->line = 1;

        sprintf(scratch_return_string,ANSI_DCYAN"["ANSI_LCYAN"%d"ANSI_DCYAN":"ANSI_LCYAN"%d"ANSI_DCYAN"]-->"ANSI_DWHITE" ",d->edit->line,lines);
        FREENULL(d->user_prompt);
        d->user_prompt = (char *) malloc_string(scratch_return_string);
     }
}


/*  <======================================================================>  */
/*               ---->  EDITOR INITIALISATION ROUTINES  <----                 */
/*  <======================================================================>  */


/* ---->  Return name of FIELD  <---- */
const char *field_name(int field)
{
      switch(field) {
             case 10:
             case 0:
                  return("description");
                  break;
             case 1:
                  return("name");
                  break;
             case 2:
                  return("failure message");
                  break;
             case 3:
                  return("success message");
                  break;
             case 4:
                  return("drop message");
                  break;
             case 5:
                  return("others failure message");
                  break;
             case 6:
                  return("others success message");
                  break;
             case 7:
                  return("others drop message");
                  break;
             case 8:
                  return("outside description");
                  break;
             case 9:
                  return("race");
                  break;
             case 100:
             case 101:
             case 102:
             case 103:
             case 104:
             case 105:
             case 106:
             case 107:
             case 108:
                  return("message text");
                  break;
             default:
                  return("<UNKNOWN FIELD>");
      }
}

/* ---->  Return type of field (Given as a string) as an integer  <---- */
int fieldtype(const char *field)
{
    int fieldtype;

    result = 1;
    if(*field && (*field == '@')) field++;
    if(string_prefix("description",field)) {
       fieldtype = 0;
    } else if(string_prefix("name",field)) {
       fieldtype = 1;
    } else if(string_prefix("failure",field)) {
       fieldtype = 2;
    } else if(string_prefix("success",field)) {
       fieldtype = 3;
    } else if(string_prefix("drop",field)) {
       fieldtype = 4;
    } else if(string_prefix("ofailure",field)) {
       fieldtype = 5;
    } else if(string_prefix("osuccess",field)) {
       fieldtype = 6;
    } else if(string_prefix("odrop",field)) {
       fieldtype = 7;
    } else if(string_prefix("odescription",field)) {
       fieldtype = 8;
    } else if(string_prefix("race",field)) {
       fieldtype = 9;
    } else {
       fieldtype = 0;
       result    = 0;
    }
    return(fieldtype);
}

/* ---->  Can user edit specified field of specified object?  <---- */
int can_edit(struct descriptor_data *d,int field,dbref object,int change)
{
    int result = 1;

    if((Typeof(object) == TYPE_FUSE) || (Typeof(object) == TYPE_ALARM)) result = 0;
    if(result) switch(field) {
       case 0:  /* ---->  Description  <---- */
            if(!HasField(object,DESC) || !CanSetField(object,DESC)) result = 0;
            if(Readonlydesc(object)) result = -1;
            break;
       case 1:  /* ---->  Name  <---- */
            result = 0;
            break;
       case 2:  /* ---->  Failure message  <---- */
            if(!HasField(object,FAIL) || !CanSetField(object,FAIL)) result = 0;
            break;
       case 3:  /* ---->  Success message  <---- */
            if(!HasField(object,SUCC) || !CanSetField(object,SUCC)) result = 0;
            break;
       case 4:  /* ---->  Drop message  <---- */
            if(!HasField(object,DROP) || !CanSetField(object,DROP)) result = 0;
            break;
       case 5:  /* ---->  Others failure message  <---- */
            if(!HasField(object,OFAIL) || !CanSetField(object,OFAIL)) result = 0;
            break;
       case 6:  /* ---->  Others success message  <---- */
            if(!HasField(object,OSUCC) || !CanSetField(object,OSUCC)) result = 0;
            break;
       case 7:  /* ---->  Others drop message  <---- */
            if(!HasField(object,ODROP) || !CanSetField(object,ODROP)) result = 0;
            break;
       case 8:  /* ---->  Outside description  <---- */
            if(!HasField(object,ODESC) || !CanSetField(object,ODESC)) result = 0;
            if(Readonlydesc(object)) result = -1;
            break;
       case 9:  /* ---->  Race  <---- */
            if(!Level4(db[d->player].owner) || !HasField(object,RACE) || !CanSetField(object,RACE)) result = 0;
            break;
       case 10:  /* ---->  Dynamic array element  <---- */
            break;
       default:
            result = 0;
    }

    if(result < 1) {
       if(change) {
          sprintf(scratch_return_string," of %s"ANSI_LWHITE"%s"ANSI_LRED,Article(d->edit->object,LOWER,DEFINITE),unparse_object(d->player,d->edit->object,0));
          output(d,d->player,0,1,0,"\n"ANSI_LRED"Sorry, you can't edit %s "ANSI_LWHITE"%s"ANSI_LRED"%s%s  -  Editing %s "ANSI_LWHITE"%s"ANSI_LRED".",(d->player == object) ? "your":"the",field_name(field),(d->player == object) ? "":scratch_return_string,(result == -1) ? " (It's Read-Only.)":"",(d->player == object) ? "your":"its",field_name(d->edit->field));
       } else {
          sprintf(scratch_return_string," of %s%s",(result == -1) ? "that ":"",object_type(object,(result == -1) ? 0:1));
          output(d,d->player,0,1,0,ANSI_LRED"Sorry, you can't edit %s "ANSI_LWHITE"%s"ANSI_LRED"%s%s  -  Edit aborted.\n",(d->player == object) ? "your":"the",field_name(field),(d->player == object) ? "":scratch_return_string,(result == -1) ? " (It's Read-Only.)":"");
       }
    }
    return(result > 0);
}

/* ---->  Return pointer to given FIELD of OBJECT <---- */
char *get_field(dbref player,int field,dbref object,int element)
{
     char *textptr;

     switch(field) {
            case 1:
                 textptr = (char *) getfield(object,NAME);
                 break;
            case 2:
                 textptr = (char *) getfield(object,FAIL);
                 break;
            case 3:
                 textptr = (char *) getfield(object,SUCC);
                 break;
            case 4:
                 textptr = (char *) getfield(object,DROP);
                 break;
            case 5:
                 textptr = (char *) getfield(object,OFAIL);
                 break;
            case 6:
                 textptr = (char *) getfield(object,OSUCC);
                 break;
            case 7:
                 textptr = (char *) getfield(object,ODROP);
                 break;
            case 8:
                 textptr = (char *) getfield(object,ODESC);
                 break;
            case 9:
                 textptr = (char *) getfield(object,RACE);
                 break;
            case 10:
                 array_subquery_elements(player,object,element,element,scratch_return_string);
                 return(scratch_return_string);
            default:
                 textptr = (char *) getfield(object,DESC);
     }
     return(textptr);
}

/* ---->  Update field of object being current being edited  <---- */
int update_field(struct descriptor_data *d)
{
    int dummy;

    if((d->edit->field < 100) && !((Typeof(d->edit->object) != TYPE_FREE) && (d->edit->checksum == db[d->edit->object].checksum))) return(0);
    switch(d->edit->field) {
           case 1:
                setfield(d->edit->object,NAME,d->edit->text,1);
                break;
           case 2:
                setfield(d->edit->object,FAIL,d->edit->text,0);
                break;
           case 3:
                setfield(d->edit->object,SUCC,d->edit->text,0);
                break;
           case 4:
                setfield(d->edit->object,DROP,d->edit->text,0);
                break;
           case 5:
                setfield(d->edit->object,OFAIL,d->edit->text,0);
                break;
           case 6:
                setfield(d->edit->object,OSUCC,d->edit->text,0);
                break;
           case 7:
                setfield(d->edit->object,ODROP,d->edit->text,0);
                break;
           case 8:
                setfield(d->edit->object,ODESC,d->edit->text,0);
                break;
           case 9:
                setfield(d->edit->object,RACE,d->edit->text,0);
                break;
           case 10:
                if(d->edit->element == INDEXED) {
                   strcpy(indexfrom,String(d->edit->index));
                   *indexto = '\0';
		}
                switch(array_set_elements(d->player,d->edit->object,d->edit->element,d->edit->element,d->edit->text,0,&dummy,&dummy)) {
                       case ARRAY_INSUFFICIENT_QUOTA:
                            output(d,d->player,0,1,0,"\n"ANSI_LRED"Sorry, adding a new element to "ANSI_LWHITE"%s"ANSI_LRED" will exceed your Building Quota limit.\n",unparse_object(d->player,d->edit->object,0));
                            return(1);
                       case ARRAY_TOO_MANY_ELEMENTS:
                            output(d,d->player,0,1,0,"\n"ANSI_LRED"Sorry, no more than "ANSI_LWHITE"%d"ANSI_LRED" new elements may be added to a dynamic array in one go.\n",ARRAY_LIMIT);
                            return(1);
                       case ARRAY_TOO_MANY_BLANKS:
                            output(d,d->player,0,1,0,"\n"ANSI_LRED"Sorry, no more than "ANSI_LWHITE"%d"ANSI_LRED" new elements may be created in a dynamic array in one go.\n",ARRAY_LIMIT);
                            return(1);
		}
                break;
           case 100:
           case 101:
           case 103:
           case 104:
           case 105:
           case 106:
           case 107:
                break;
           case 102:
                FREENULL(d->edit->data->message.message);
                d->edit->data->message.message = (char *) alloc_string(compress(d->edit->text,1));
                break;
           default:
                if((Typeof(d->edit->object) == TYPE_COMMAND) && Apprentice(d->edit->object) && (!Validchar(d->player) || !Level4(d->player))) {
                   if(!in_command)
                      writelog(HACK_LOG,1,"HACK","%s(#%d) changed the description of compound command %s(#%d), which has its APPRENTICE, WIZARD or ELDER flag set.",getname(d->player),d->player,getname(d->edit->object),d->edit->object);
                         else writelog(HACK_LOG,1,"HACK","%s(#%d) changed the description of compound command %s(#%d) within compound command %s(#%d), which has its APPRENTICE, WIZARD or ELDER flag set.",getname(d->player),d->player,getname(d->edit->object),d->edit->object,getname(current_command),current_command);
		}
                setfield(d->edit->object,DESC,d->edit->text,0);
    }
    return(1);
}

/* ---->  Return name of OBJECT  <---- */
const char *object_name(int object)
{
      switch(object) {
             case 0:
                  return("Alarm");
                  break;
             case 1:
                  return("Compound command");
                  break;
             case 2:
                  return("Exit");
                  break;
             case 3:
                  return("Character");
                  break;
             case 4:
                  return("Fuse");
                  break;
             case 5:
                  return("Room");
                  break;
             case 6:
                  return("Thing");
                  break;
             case 7:
                  return("Variable");
                  break;
             case 8:
                  return("Property");
                  break;
             case 9:
                  return("Dynamic array");
                  break;
            default:
                  return("<UNKNOWN OBJECT TYPE>");
      }
}

/* ---->  Return type of object (Given as a string) as an integer  <---- */
int get_objecttype(const char *object)
{
    if(!*object) return(NOTHING);
    if(*object == '@') object++;
    if(string_prefix("alarm",object)) {
       return(0);
    } else if(string_prefix("command",object) || string_prefix("compoundcommand",object)) {
       return(1);
    } else if(string_prefix("exit",object)) {
       return(2);
    } else if(string_prefix("character",object) || string_prefix("player",object)) {
       return(3);
    } else if(string_prefix("fuse",object)) {
       return(4);
    } else if(string_prefix("room",object)) {
       return(5);
    } else if(string_prefix("thing",object)) {
       return(6);
    } else if(string_prefix("variable",object)) {
       return(7);
    } else if(string_prefix("property",object) || string_prefix("properties",object)) {
       return(8);
    } else if(string_prefix("array",object) || string_prefix("dynamicarray",object)) {
       return(9);
    } else return(NOTHING);
}

/* ---->  Create object of type OBJECT and return DBref of it  <---- */
dbref create_object(dbref player,int object,const char *arg1,const char *arg2)
{
      dbref newobject;
      char  *p1;

      int cucache     = current_command;
      int iccache     = in_command;
      current_command = player;
      in_command      = 1;

      switch(object) {
             case 0:  /* ---->  Alarm  <---- */
                  output(getdsc(player),player,0,1,0,ANSI_LRED"Sorry, you can't create and edit a new "ANSI_LWHITE"alarm"ANSI_LRED".");
                  break;
             case 1:  /* ---->  Compound Command  <---- */
                  create_command(player,NULL,NULL,(char *) arg1,(char *) arg2,0,0);
                  break;
             case 2:  /* ---->  Exit  <---- */
                  create_exit(player,NULL,NULL,(char *) arg1,(char *) arg2,0,0);
                  break;
             case 3:  /* ---->  Character  <---- */
                  output(getdsc(player),player,0,1,0,ANSI_LRED"Sorry, you can't create and edit a new "ANSI_LWHITE"character"ANSI_LRED".");
                  break;
             case 4:  /* ---->  Fuse  <---- */
                  output(getdsc(player),player,0,1,0,ANSI_LRED"Sorry, you can't create and edit a new "ANSI_LWHITE"fuse"ANSI_LRED".");
                  break;
             case 5:  /* ---->  Room  <---- */
                  create_room(player,NULL,NULL,(char *) arg1,(char *) arg2,0,0);
                  break;
             case 6:  /* ---->  Thing  <---- */
                  create_thing(player,NULL,NULL,(char *) arg1,(char *) arg2,0,0);
                  break;
             case 7:  /* ---->  Variable  <---- */
                  create_variable_property(player,NULL,NULL,(char *) arg1,(char *) arg2,0,0);
                  break;
             case 8:  /* ---->  Property  <---- */
                  create_variable_property(player,NULL,NULL,(char *) arg1,(char *) arg2,1,0);
                  break;
             case 9:  /* ---->  Dynamic Array  <---- */
                  create_array(player,NULL,NULL,(char *) arg1,(char *) arg2,0,0);
                  break;
             default:
                  setreturn(ERROR,COMMAND_FAIL);
      }
      current_command = cucache;
      in_command      = iccache;

      p1 = (char *) command_result;
      while(*p1 && (*p1 == '#')) p1++;
      if(*p1) {
         newobject = atoi(p1);
         if(newobject) return(newobject);
            else return(NOTHING);
      } else return(NOTHING);
}

/* ---->  Already using editor?  <---- */
unsigned char editing(dbref player)
{
	 struct descriptor_data *p = getdsc(player);

	 if(!p) return(1);
	 if(p->edit) {
	    output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can't use the editor from within the editor!");
	    return(1);
	 } else if(p->prompt) {
	    output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can't use the editor from within an interactive '"ANSI_LWHITE"@prompt"ANSI_LGREEN"' session.");
	    return(1);
	 } else return(0);
}

/* ---->  Initialise edit data  <---- */
void edit_initialise(dbref player,unsigned char field,const char *text,union group_data *data,const char *data1,const char *data2,unsigned char permanent,int temp)
{
     struct descriptor_data *p = getdsc(player);

     if(!p) return;
     if(p->edit) {
        FREENULL(p->user_prompt);
        p->user_prompt = (char *) malloc_string(p->edit->prompt);
        FREENULL(p->edit->prompt);
        FREENULL(p->edit->index);
        FREENULL(p->edit->data1);
        FREENULL(p->edit->data2);
        FREENULL(p->edit->text);
        FREENULL(p->edit);
        p->flags2 &= ~ABSOLUTE_OVERRIDE;
        p->flags  &= ~(NOLISTEN|BBS_CENSOR|ABSOLUTE);
        p->flags  |=  EVALUATE;
     } 

     MALLOC(p->edit,struct edit_data);
     p->edit->objecttype     = NOTHING;
     p->edit->permanent      = (permanent & 0x1);
     p->edit->checksum       = 0;
     p->edit->element        = NOTHING;
     p->edit->object         = NOTHING;
     p->edit->index          = NULL;

     p->edit->field          = field;
     p->edit->data1          = (char *) data1;
     p->edit->data2          = (char *) data2;
     p->edit->data           = data;
     p->edit->text           = (char *) malloc_string((text) ? text:"");
     p->edit->succ           = NOTHING;
     p->edit->fail           = NOTHING;
     p->edit->line           = strcnt(p->edit->text,'\n') + 1;
     p->edit->temp           = temp;

     if(!Level4(player) && !Builder(player)) p->flags |= NOLISTEN;
     if(permanent & TOPIC_CENSOR)            p->flags |= BBS_CENSOR;
     if(permanent & EDIT_LAST_CENSOR)        p->flags |= ABSOLUTE;

     /* ---->  Setup user prompt  <---- */
     sprintf(scratch_return_string,ANSI_DCYAN"["ANSI_LCYAN"%d"ANSI_DCYAN":"ANSI_LCYAN"%d"ANSI_DCYAN"]-->"ANSI_DWHITE" ",p->edit->line,p->edit->line - 1);
     p->edit->prompt = (char *) malloc_string(p->user_prompt);
     FREENULL(p->user_prompt);
     p->user_prompt  = (char *) malloc_string(scratch_return_string);
}

/* ---->  Display TCZ editor header  <---- */
void edit_header(dbref player,dbref object,unsigned char field)
{
     char buffer[TEXT_SIZE];

     sprintf(buffer,"%s Editor v"EDITOR_VERSION".",tcz_full_name);
     tilde_string(player,buffer,ANSI_LYELLOW,ANSI_DYELLOW,0,1,5);
     of_object_or_array(player,object,field,NOTHING,ANSI_LGREEN,NULL);
     output(getdsc(player),player,0,1,0,ANSI_LGREEN"Editing %s "ANSI_LWHITE"%s"ANSI_LGREEN"%s...\n",(object == player) ? "your":"the",field_name(field),(object == player) ? "":scratch_return_string);
     output(getdsc(player),player,0,1,0,(db[player].flags2 & EDIT_OVERWRITE) ? ANSI_LGREEN"Overwrite mode is "ANSI_LWHITE"on"ANSI_LGREEN".":ANSI_LGREEN"Overwrite mode is "ANSI_LWHITE"off"ANSI_LGREEN".");
     output(getdsc(player),player,0,1,0,(db[player].flags2 & EDIT_NUMBERING) ? ANSI_LGREEN"Line numbers "ANSI_LWHITE"will"ANSI_LGREEN" be displayed.":ANSI_LGREEN"Line numbers "ANSI_LWHITE"will not"ANSI_LGREEN" be displayed.");
     output(getdsc(player),player,0,1,0,(db[player].flags2 & EDIT_EVALUATE)  ? ANSI_LYELLOW"{}"ANSI_LGREEN"'s, "ANSI_LYELLOW"$"ANSI_LGREEN"'s and "ANSI_LYELLOW"\\"ANSI_LGREEN"'s "ANSI_LWHITE"WILL"ANSI_LGREEN" be evaluated.\n":ANSI_LYELLOW"{}"ANSI_LGREEN"'s, "ANSI_LYELLOW"$"ANSI_LGREEN"'s and "ANSI_LYELLOW"\\"ANSI_LGREEN"'s "ANSI_LWHITE"WILL NOT"ANSI_LGREEN" be evaluated.\n");
}

/* ---->  Start the editor  <---- */
void edit_start(CONTEXT)
{
     struct descriptor_data *p = getdsc(player);
     char   *objecttype,*fieldname,*objectname;
     int    field,element = NOTHING;
     char   buffer[TEXT_SIZE];
     int    editobjecttype;
     char   *textptr;
     char   *p1,*p2;
     dbref  object;
     char   c;

     setreturn(ERROR,COMMAND_FAIL);
     if(!p) return;

     if(p->edit) {
        output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can't use the editor from within the editor!");
        return;
     } else if(p->prompt) {
        output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can't use the editor from within an interactive '"ANSI_LWHITE"@prompt"ANSI_LGREEN"' session.");
        return;
     }

     strcpy(buffer,arg1);
     p1 = buffer, p2 = buffer, c = *p2;

     /* ---->  Grab <OBJECT TYPE>  <---- */
     while(*p1 && (*p1 == ' ')) p1++;
     objecttype = p1;
     while(*p1 && (*p1 != ' ')) p1++;
     p2 = p1, c = *p1;
     if(*p1) *p1++ = '\0';

     editobjecttype = get_objecttype(objecttype);
     if(editobjecttype != NOTHING) {

        /* ---->  Grab <FIELD>  <---- */
        while(*p1 && (*p1 == ' ')) p1++;
        fieldname = p1;
        while(*p1 && (*p1 != ' ')) p1++;
        p2 = p1, c = *p1;
        if(*p1) *p1++ = '\0';

        /* ---->  No <OBJECT TYPE> specified  -  Assume this is the <FIELD>  <---- */
     } else fieldname = objecttype;

     field = fieldtype(fieldname);
     if(!result) *p2 = c, p1 = fieldname;

     /* ---->  Grab <OBJECT>/<OBJECT NAME>  <---- */
     while(*p1 && (*p1 == ' ')) p1++;
     objectname = p1;

     /* ---->  Find <OBJECT>  <---- */
     if(editobjecttype == NOTHING) {

        /* ---->  If no object specified, assume character starting editor  <---- */
        if(!*objectname) strcpy(objectname,"me");
        object = match_preferred(player,player,objectname,MATCH_PHASE_KEYWORD,MATCH_PHASE_GLOBAL,SEARCH_ALL,SEARCH_PREFERRED,MATCH_OPTION_DEFAULT);
        if(!Valid(object)) return;

        if(Readonly(object)) {
           output(p,player,0,1,0,ANSI_LGREEN"Sorry, that %s is Read-Only  -  You can't edit %s "ANSI_LWHITE"%s"ANSI_LGREEN".",object_type(object,0),(Typeof(object) == TYPE_CHARACTER) ? "their":"its",field_name(field));
           return;
	}

        /* ---->  Security constraints  <---- */
        if(!can_write_to(player,object,0)) {
           if(Level3(db[player].owner)) output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only edit something you own or something owned by someone of a lower level than yourself.");
              else output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can only edit something you own.");
           return;
	}
     }

     if(editobjecttype != NOTHING) {
        object = create_object(p->player,editobjecttype,objectname,arg2);
        field  = fieldtype(fieldname);
        if(!Valid(object)) {
           output(p,player,0,1,0,ANSI_LGREEN"Sorry, unable to create new object  -  Edit aborted.");
           return;
	}
     }

     /* ---->  Handle array element to edit  <---- */
     if(Typeof(object) == TYPE_ARRAY) {
        element = elementfrom;
        if((element == NOTHING) || (element == ALL)) element = 1;
        if((element == UNSET) || (element == INVALID)) {
           output(p,player,0,1,0,ANSI_LGREEN"Sorry, that element of the dynamic array "ANSI_LWHITE"%s"ANSI_LGREEN" is invalid  -  Edit aborted.",unparse_object(player,object,0));
           return;
	}
        if(!field) field = 10;
     } else element = NOTHING;

     /* ---->  Start the editor  <---- */
     sprintf(buffer,"%s Editor v"EDITOR_VERSION".",tcz_full_name);
     tilde_string(player,buffer,ANSI_LYELLOW,ANSI_DYELLOW,0,1,5);
     if(!in_command) {
        if(editobjecttype == NOTHING) {
           of_object_or_array(player,object,field,element,ANSI_LGREEN,indexfrom);
           output(p,player,0,1,0,ANSI_LGREEN"Editing %s "ANSI_LWHITE"%s"ANSI_LGREEN"%s...\n",(object == player) ? "your":"the",field_name(field),(object == player) ? "":scratch_return_string);
        } else if(element != NOTHING) {
           of_object_or_array(player,NOTHING,field,element,ANSI_LGREEN,indexfrom);
           output(p,player,0,1,0,ANSI_LGREEN"%s "ANSI_LWHITE"%s"ANSI_LGREEN" created  -  Editing description%s...\n",object_name(editobjecttype),unparse_object(player,object,0),scratch_return_string);
	} else output(p,player,0,1,0,ANSI_LGREEN"%s "ANSI_LWHITE"%s"ANSI_LGREEN" created  -  Editing %s "ANSI_LWHITE"%s"ANSI_LGREEN"...\n",object_name(editobjecttype),unparse_object(player,object,0),(Typeof(object) == TYPE_CHARACTER) ? "their":"its",field_name(field));
     }
     if(!can_edit(p,field,object,0)) return;

     output(p,player,0,1,0,(db[p->player].flags2 & EDIT_OVERWRITE) ? ANSI_LGREEN"Overwrite mode is "ANSI_LWHITE"on"ANSI_LGREEN".":ANSI_LGREEN"Overwrite mode is "ANSI_LWHITE"off"ANSI_LGREEN".");
     output(p,player,0,1,0,(db[p->player].flags2 & EDIT_NUMBERING) ? ANSI_LGREEN"Line numbers "ANSI_LWHITE"will"ANSI_LGREEN" be displayed.":ANSI_LGREEN"Line numbers "ANSI_LWHITE"will not"ANSI_LGREEN" be displayed.");
     output(p,player,0,1,0,(db[p->player].flags2 & EDIT_EVALUATE)  ? ANSI_LYELLOW"{}"ANSI_LGREEN"'s, "ANSI_LYELLOW"$"ANSI_LGREEN"'s and "ANSI_LYELLOW"\\"ANSI_LGREEN"'s "ANSI_LWHITE"WILL"ANSI_LGREEN" be evaluated.\n":ANSI_LYELLOW"{}"ANSI_LGREEN"'s, "ANSI_LYELLOW"$"ANSI_LGREEN"'s and "ANSI_LYELLOW"\\"ANSI_LGREEN"'s "ANSI_LWHITE"WILL NOT"ANSI_LGREEN" be evaluated.\n");
     output(p,player,0,1,0,ANSI_LGREEN"Type '"ANSI_LWHITE".help editor"ANSI_LGREEN"' for help on using the editor.\n");
     textptr = get_field(p->player,field,object,element);

     /* ---->  Setup editor  <---- */
     MALLOC(p->edit,struct edit_data);
     if(textptr != (char *) NULL) p->edit->text = (char *) malloc_string(textptr);
        else p->edit->text = (char *) malloc_string("");

     p->edit->objecttype     = editobjecttype;
     p->edit->permanent      = 0;
     p->edit->checksum       = db[object].checksum;
     p->edit->element        = (element != END) ? element:array_element_count(db[object].data->array.start) + 1;
     p->edit->object         = object;
     p->edit->field          = field;
     p->edit->index          = (char *) alloc_string(indexfrom);
     p->edit->data1          = NULL;
     p->edit->data2          = NULL;
     p->edit->data           = NULL;
     p->edit->fail           = NOTHING;
     p->edit->line           = strcnt(p->edit->text,'\n') + 1;
     p->edit->succ           = NOTHING;

     if(!Level4(player) && !Builder(player)) p->flags |= NOLISTEN;
     if(db[player].flags2 & EDIT_EVALUATE)   p->flags |= EVALUATE;
        else p->flags &= ~EVALUATE;

     /* ---->  Setup user prompt  <---- */
     sprintf(scratch_return_string,ANSI_DCYAN"["ANSI_LCYAN"%d"ANSI_DCYAN":"ANSI_LCYAN"%d"ANSI_DCYAN"]-->"ANSI_DWHITE" ",p->edit->line,p->edit->line - 1);
     p->edit->prompt = (char *) malloc_string(p->user_prompt);
     FREENULL(p->user_prompt);
     p->user_prompt  = (char *) malloc_string(scratch_return_string);
     setreturn(OK,COMMAND_SUCC);
}
