/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| HTML.C  -  Implements TCZ's World Wide Web Interface and HTML support.      |
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
| Module originally designed and written by:  J.P.Boggis 21/06/1996.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: html.c,v 1.3 2005/06/29 21:20:46 tcz_monster Exp $

*/


#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "prompts.h"
#include "db.h"

#include "descriptor_flags.h"
#include "object_types.h"
#include "flagset.h"
#include "fields.h"
#include "search.h"
#include "match.h"
#include "html.h"


#define HTML_VERSION     "2.0"  /*  TCZ World Wide Web Interface version (NOTE:  This is NOT the version of HTML/HTTP used.)  */


/* ---->  Content types  <---- */
#define HTTP_TEXT_HTML   0  /*  text/html   */
#define HTTP_IMAGE_JPEG  1  /*  image/jpeg  */
#define HTTP_IMAGE_GIF   2  /*  image/gif   */

const char *http_content_type[] = {
      "text/html",
      "image/jpeg",
      "image/gif"
};


/* ---->  Combined linked list and binary tree of loaded images  <---- */
struct html_image_data {
       struct   html_image_data *next;   /*  Next image in linked list     */
       struct   html_image_data *left;   /*  Left pointer of binary tree   */
       struct   html_image_data *right;  /*  Right pointer of binary tree  */
       const    char *filename;          /*  File name of image            */
       unsigned char imgtype;            /*  Type of image                 */
       unsigned char *data;              /*  Raw data of image             */
       int           size;               /*  Size of image data            */
       unsigned char tree;               /*  Currently in binary tree?     */
};


struct html_image_data *html_image_tree = NULL;
struct character_sort_data *rootnode    = NULL;
struct html_image_data *html_images     = NULL;
struct character_sort_data *tail        = NULL;
int    html_internal_images             = 1;


/* ---->  Character entry in sorted binary tree  <---- */
struct character_sort_data {
       struct character_sort_data *next;
       struct character_sort_data *sort;
       struct character_sort_data *left;
       struct character_sort_data *right;
       dbref  player;
};


/* ---->  HTML GET/POST parameters data  <---- */
struct html_param_data {
       char *name;
       char *contents;
       char *fullcontents;
       struct html_param_data *next;
};


/* ---->  HTML tag table  <---- */
#include "html_tags.h"


/* ---->  HTML character entity table  <---- */
#include "html_entities.h"


/* ---->  Smileys (Emoticons) table  <---- */
#include "smileys.h"


static unsigned short html_entity_table_size = 0;
static struct   html_param_data *params = NULL;
static unsigned short html_tag_table_size = 0;
static unsigned short fonttag = 0;
static short    entities[256];


/* ---->  {J.P.Boggis 11/05/2000}  Return/output HTTP header  <---- */
/*            D:  Valid TCZ HTML Interface descriptor.                             */
/*       OUTPUT:  Output to descriptor                         (0 = Don't output)  */
/*         CODE:  HTTP result code (200 = OK, 400 = Bad Request, 404 = Not Found)  */
/*      CODESTR:  HTTP result code as string   ("OK", "Bad Request", "Not Found")  */
/*  CONTENTTYPE:  Page content type as string                       ("text/html")  */
/*     MODIFIED:  Time/date when page was last modified      (0 = Do not include)  */
/*      EXPIRES:  Time/date when page will expire            (0 = Do not include)  */
/*        CACHE:  Cache page contents                        (0 = Don't cache)     */
/*   CONTENTLEN:  Length of content in bytes                 (0 = Do not include)  */
const char *http_header(struct descriptor_data *d,int output,int code,const char *codestr,const char *contenttype,time_t modified,time_t expires,int cache,int contentlen)
{
      static char datebuf[KB + 1],modbuf[KB + 1],expbuf[KB + 1],contbuf[KB + 1];
      static char buffer[BUFFER_LEN];
      struct tm *rtime;
      time_t now;

      gettime(now);
      rtime = localtime(&now);
      strftime(datebuf,KB,"%a, %d %b %Y %H:%M:%S %Z",rtime);

      if(modified) {
         rtime = localtime(&modified);
         strftime(modbuf,KB,"%a, %d %b %Y %H:%M:%S %Z",rtime);
      }

      if(expires) {
         rtime = localtime(&expires);
         strftime(expbuf,KB,"%a, %d %b %Y %H:%M:%S %Z",rtime);
      }

      if(contentlen > 0)
         snprintf(contbuf,KB,"Content-Length: %d\r\n",contentlen);

      snprintf(buffer,sizeof(buffer),"HTTP/%s %d %s%s\r\nDate: %s\r\nServer: TCZ/"TCZ_VERSION".%d (%s) %s\r\n%s%s%s%s%s%sAccept: text/html\r\nAllow: GET POST%s\r\n%sContent-Type: %s\r\n\r\n",
               (IsHtml(d) && (d->html->protocol == HTML_PROTOCOL_HTTP_1_0)) ? "1.0":"1.1",
               code,codestr,(cache) ? "":(IsHtml(d) && (d->html->protocol == HTML_PROTOCOL_HTTP_1_0)) ? "\r\nPragma: no-cache":"\r\nCache-Control: no-cache",
               datebuf,TCZ_REVISION,operatingsystem,tcz_short_name,
               (modified) ? "Last-Modified: ":"",(modified) ? modbuf:"",(modified) ? "\r\n":"",
               (expires) ? "Expires: ":"",(expires) ? expbuf:"",(expires) ? "\r\n":"",
               (IsHtml(d) && (d->html->protocol == HTML_PROTOCOL_HTTP_1_0)) ? "":"\r\nConnection: close",
               (contentlen > 0) ? contbuf:"",contenttype);

      /* ---->  Output to descriptor?  <---- */
      if(d && output)
         server_queue_output(d,buffer,strlen(buffer));
      return(buffer);
}

/* ---->  {J.P.Boggis 11/06/2000}  Return/output HTML header  <---- */
/*           D:  Valid TCZ HTML Interface descriptor.                      */
/*      OUTPUT:  Output to descriptor                  (0 = Don't output)  */
/*       TITLE:  Page title                         (NULL = None)          */
/*     REFRESH:  Refresh rate in seconds               (0 = No refresh)    */
/*  BACKGROUND:  Background image URL  (NULL = Use descriptor preference)  */
/*          FG:  Foreground colour     (NULL = Use descriptor preference)  */
/*          BG:  Background colour     (NULL = Use descriptor preference)  */
/*        LINK:  Unvisited link colour (NULL = Use descriptor preference)  */
/*       ALINK:  Active link colour    (NULL = Use descriptor preference)  */
/*       VLINK:  Visited link colour   (NULL = Use descriptor preference)  */
/*        BODY:  Additional BODY section parameters         (NULL = None)  */
/*        JAVA:  JAVA enhancements (For output window only)    (0 = None)  */
/*        LOGO:  TCZ HTML Interface logo                       (0 = None)  */
const char *html_header(struct descriptor_data *d,int output,const char *title,int refresh,const char *background,const char *fg,const char *bg,const char *link,const char *alink,const char *vlink,const char *body,int java,int logo)
{
      static char buffer[BUFFER_LEN];

      *buffer = '\0';
      if(!IsHtml(d)) return(buffer);

      /* ---->  Head Section  <---- */
      strcpy(buffer,"<HTML><HEAD>");

      /* ---->  Title  <---- */
      if(!Blank(title)) {
         if(title == HTML_TITLE)
            sprintf(buffer + strlen(buffer),"<TITLE>"HTML_TITLE"</TITLE>",tcz_full_name,tcz_year);
               else sprintf(buffer + strlen(buffer),"<TITLE>%s</TITLE>",title);
      }

      if(refresh)
         sprintf(buffer + strlen(buffer),"<META http-equiv=\"Refresh\" CONTENT=\"%d;\">",refresh);

      strcat(buffer,"</HEAD>");

      /* ---->  Body Section  <---- */
      strcat(buffer,"<BODY");

      /* ---->  Background image  <---- */
      if(Blank(background)) {
	 if(d->html->flags & HTML_BGRND) {
	    if(!Blank(d->html->background))
	       sprintf(buffer + strlen(buffer)," BACKGROUND=\"%s\"",decompress(d->html->background));
	 } else sprintf(buffer + strlen(buffer)," BACKGROUND=\"%s\"",html_image_url("default.gif"));
      } else sprintf(buffer + strlen(buffer)," BACKGROUND=\"%s\"",background);

      /* ---->  Text foreground colour  <---- */
      if(Blank(fg)) {
         if(!(d->html->flags & HTML_WHITE_AS_BLACK))
            strcat(buffer," TEXT="HTML_LWHITE);
               else strcat(buffer," TEXT="HTML_DBLACK);
      } else sprintf(buffer + strlen(buffer)," TEXT=%s",fg);

      /* ---->  Page background colour  <---- */
      if(Blank(bg)) {
         if(!(d->html->flags & HTML_WHITE_AS_BLACK))
            sprintf(buffer + strlen(buffer)," BGCOLOR=%s",(d->html->flags & HTML_BGRND) ? HTML_TABLE_GREY:HTML_TABLE_BLACK);
               else sprintf(buffer + strlen(buffer)," BGCOLOR=%s",(d->html->flags & HTML_BGRND) ? HTML_TABLE_LGREY:HTML_TABLE_WHITE);
      } else sprintf(buffer + strlen(buffer)," BGCOLOR=%s",bg);

      /* ---->  Unvisited link  <---- */
      if(Blank(link))
         strcat(buffer," LINK="HTML_LINK);
            else sprintf(buffer + strlen(buffer)," LINK=%s",link);

      /* ---->  Active link  <---- */
      if(Blank(alink))
         strcat(buffer," ALINK="HTML_ALINK);
            else sprintf(buffer + strlen(buffer)," ALINK=%s",alink);

      /* ---->  Visited link  <---- */
      if(Blank(vlink))
         strcat(buffer," VLINK="HTML_VLINK);
            else sprintf(buffer + strlen(buffer)," VLINK=%s",vlink);

      /* ---->  Additional parameters  <---- */
      if(!Blank(body))
         sprintf(buffer + strlen(buffer)," %s>",body);
            else strcat(buffer,">");

      /* ---->  JAVA enhancements  <---- */
      if(java && (d->html->flags & HTML_JAVA) && (d->html->flags & HTML_OUTPUT)) {

         /* ---->  New automatic scrolling method (scrollBy(x,y);)  <---- */
         if(d->html->flags & HTML_SCROLLBY)
            sprintf(buffer + strlen(buffer),"<SCRIPT LANGUAGE=\"JavaScript\">\n<!--\nscrollon = 0;\nfunction ScrollOutput() { if(scrollon) { self.scrollBy(0,1000000); scrollon = 0; }}\nfunction scrl2() { scrollon = 1; ScrollOutput(); scrollon = 1; } self.setInterval(ScrollOutput,400);\n//-->\n</SCRIPT>");

         /* ---->  Old automatic scrolling method (scroll(x,y);)  <---- */
         if(d->html->flags & HTML_SCROLL)
            sprintf(buffer + strlen(buffer),"<SCRIPT LANGUAGE=\"JavaScript\">\n<!--\nfunction scrl1(){ self.scroll(-1,1000000); }\n//-->\n</SCRIPT>");
      }

      /* ---->  HTML Interface logo  <---- */
      if(logo)
         sprintf(buffer + strlen(buffer),"<CENTER><FONT SIZE=6><B>%s (TCZ v"TCZ_VERSION")</FONT><P><IMG SRC=\"%s\" ALT=\"%s Logo\"><P><FONT SIZE=5>World Wide Web Interface (v"HTML_VERSION")</FONT><BR><FONT SIZE=2><I>&copy; J.P.Boggis 1993 - %d</I></FONT></B><P><HR><P></CENTER>",tcz_full_name,html_image_url("logo.gif"),tcz_full_name,tcz_year);

      /* ---->  Output to descriptor?  <---- */
      if(d && output)
         server_queue_output(d,buffer,strlen(buffer));
      return(buffer);
}

/* ---->  Construct generic HTML error message  <---- */
/*        D:  Valid TCZ HTML Interface descriptor.               */
/*   OUTPUT:  Output to descriptor.          (0 = Don't output)  */
/*  MESSAGE:  Error message.                                     */
/*    TITLE:  Page title.                                        */
/*     BACK:  Back link text.             (NULL = No back link)  */
/*  BACKURL:  Back link destination URL.  (NULL = No back link)  */
/*    FLAGS:  HTML_CODE flags for inclusion/exclusion of specific parts of generated page.  */
const char *html_error(struct descriptor_data *d,int output,const char *message,const char *title,const char *back,const char *backurl,int flags)
{
      static char buffer[BUFFER_LEN];

      *buffer = '\0';
      if(!flags) flags = HTML_CODE_ERROR;
      if(flags & HTML_CODE_HEADER) {
         time_t now;

         gettime(now);
         strcat(buffer,http_header(d,0,400,"Bad Request","text/html",now,now,1,0));
      }

      if(flags & HTML_CODE_HTML)
         strcat(buffer,"<HTML>");

      if(flags & HTML_CODE_HEAD)
         strcat(buffer,"<HEAD>");

      if(flags & HTML_CODE_TITLE)
         sprintf(buffer + strlen(buffer),"<TITLE>"HTML_TITLE"</TITLE>",tcz_full_name,tcz_year);

      if(flags & HTML_CODE_HEAD)
         strcat(buffer,"</HEAD>");

      if(flags & HTML_CODE_BODY)
         sprintf(buffer + strlen(buffer),"<BODY BACKGROUND=\"%s\">",html_image_url("background.gif"));

      if(flags & HTML_CODE_LOGO)
         sprintf(buffer + strlen(buffer),"<CENTER><FONT SIZE=6><B>%s (TCZ v"TCZ_VERSION")</FONT><P><IMG SRC=\"%s\" ALT=\"%s Logo\"><P><FONT SIZE=5>World Wide Web Interface (v"HTML_VERSION")</FONT><BR><FONT SIZE=2><I>&copy; J.P.Boggis 1993 - %d</I></FONT></B><P><HR><P>",tcz_full_name,html_image_url("logo.gif"),tcz_full_name,tcz_year);

      sprintf(buffer + strlen(buffer),"<FONT SIZE=5><B><I>- &nbsp; %s &nbsp; -</I></B></FONT><P></CENTER><HR><P><FONT SIZE=4>%s",title,message);
      if((flags & HTML_CODE_BACK) && back && backurl)
         sprintf(buffer + strlen(buffer),"</FONT><P><HR><A HREF=\"%s\" TARGET=_parent><IMG SRC=\"%s\" BORDER=0 ALIGN=MIDDLE ALT=\"[BACK]\"></A> &nbsp; &nbsp; <B>%s</B><HR>",backurl,html_image_url("back.gif"),back);
            else strcat(buffer,"</FONT>");

      if(flags & HTML_CODE_BODY)
         strcat(buffer,"</BODY>");

      if(flags & HTML_CODE_HTML)
         strcat(buffer,"</HTML>");

      if(flags & HTML_CODE_HEADER)
         strcat(buffer,"\r\n");

      /* ---->  Output to descriptor?  <---- */
      if(d && output)
         server_queue_output(d,buffer,strlen(buffer));
      return(buffer);
}

/* ---->  {J.P.Boggis 07/05/2000}  Return HTML Interface server URL  <---- */
/*               D:  Valid descriptor from descriptor_list  */
/*            CODE:  0 = Omit security code, 1 = Add security code.  */
/*          PARAMS:  0 = No paramaters, 1 = Parameters (Resource), 2 = Relative reference (No http://server:port prefix)  */
/*        RESOURCE:  Resource (Parameters) added to end of URL  */
const char *html_server_url(struct descriptor_data *d,int code,int params,const char *resource)
{
      static char buffer[RETURN_BUFFERS * KB];
      static int  bufptr = 0;

      if(++bufptr >= RETURN_BUFFERS) bufptr = 0;
      if(params != 2) {
         if(code) snprintf(buffer + (bufptr * KB),KB,"http%s://%s:%ld/%s?CODE=%08X%08X&",(d && (d->flags & SSL)) ? "s":"",tcz_server_name,htmlport,!Blank(resource) ? resource:"",IsHtml(d) ? (int) d->html->id1:0,IsHtml(d) ? (int) d->html->id2:0);
            else snprintf(buffer + (bufptr * KB),KB,"http%s://%s:%ld%s%s%s",(d && (d->flags & SSL)) ? "s":"",tcz_server_name,htmlport,(params || !Blank(resource)) ? "/":"",!Blank(resource) ? resource:"",(params) ? "?":"");
      } else if(code) snprintf(buffer + (bufptr * KB),KB,"/%s?CODE=%08X%08X&",!Blank(resource) ? resource:"",IsHtml(d) ? (int) d->html->id1:0,IsHtml(d) ? (int) d->html->id2:0);
         else snprintf(buffer + (bufptr * KB),KB,"/%s?",!Blank(resource) ? resource:"");
      return(buffer + (bufptr * KB));
}

/* ---->  {J.P.Boggis 07/05/2000}  Return HTML Interface image URL  <---- */
const char *html_image_url(const char *image)
{
      static char buffer[RETURN_BUFFERS * KB];
      static int bufptr = 0;
      
      if(++bufptr >= RETURN_BUFFERS) bufptr = 0;
      if(html_internal_images)
         snprintf(buffer + (bufptr * KB),KB,"%s/images/%s",html_server_url(NULL,0,0,NULL),image);
            else snprintf(buffer + (bufptr * KB),KB,"%s%s",html_data_url,image);
      return(buffer + (bufptr * KB));
}

/* ---->  Initialise smiley table  <---- */
void html_init_smileys(void)
{
     char  buffer[TEXT_SIZE];
     const char *ptr;
     int   loop,count;

     for(loop = 0; smileys[loop].smiley; loop++) {

         /* ---->  Count length of text-based smiley  <---- */
         if(!Blank(smileys[loop].smiley)) {
            for(count = 0, ptr = smileys[loop].smiley; *ptr; ptr++, count++);
            smileys[loop].smileylength = count;
	 }

         /* ---->  Set HTML image tag for smiley icon and count length  <---- */
         if(!Blank(smileys[loop].icon)) {
            snprintf(buffer,sizeof(buffer),"<IMG SRC=\"%s\" ALT=\"%s\">",html_image_url(smileys[loop].icon),smileys[loop].smiley);
            for(count = 0, ptr = buffer; *ptr; ptr++, count++);
            FREENULL(smileys[loop].img);
            smileys[loop].img = (char *) alloc_string(buffer);
            smileys[loop].imglength    = count;
	 }
     }
}

/* ---->  Sort HTML tag table into alphabetical order  <---- */
short html_sort_tags()
{
      struct   html_tagtable_data temp;
      unsigned short loop,top,highest;

      for(loop = 0; tags[loop].tag; loop++)
          if(!Blank(tags[loop].tag))
             tags[loop].len = strlen(tags[loop].tag);
      html_tag_table_size = loop;

      for(top = html_tag_table_size - 1; top > 0; top--) {

          /* ---->  Find highest entry in unsorted part of list  <---- */
          for(loop = 1, highest = 0; loop <= top; loop++)
              if(strcasecmp(tags[loop].tag,tags[highest].tag) > 0)
                 highest = loop;

          /* ---->  Swap highest entry in unsorted part of list with bottom entry of sorted part of list  <---- */
          if(highest < top) {
             temp.tag = tags[top].tag, temp.closetag = tags[top].closetag, temp.exception = tags[top].exception, temp.tagtype = tags[top].tagtype, temp.len = tags[top].len;
             tags[top].tag = tags[highest].tag, tags[top].closetag = tags[highest].closetag, tags[top].exception = tags[highest].exception, tags[top].tagtype = tags[highest].tagtype, tags[top].len = tags[highest].len;
             tags[highest].tag = temp.tag, tags[highest].closetag = temp.closetag, tags[highest].exception = temp.exception, tags[highest].tagtype = temp.tagtype, tags[highest].len = temp.len;
	  }
      }
      for(loop = 0; !fonttag && (loop < html_tag_table_size); loop++)
          if(tags[loop].tagtype == HTML_TAG_FONT) fonttag = loop;
      return(html_tag_table_size);
}

/* ---->  Sort HTML character entity table into alphabetical order  <---- */
short html_sort_entities()
{
      unsigned short loop,top,highest;
      struct   html_entity_data temp;

      for(loop = 0; loop < 256; entities[loop] = -1, loop++);
      for(loop = 0; entity[loop].entity || entity[loop].text; loop++)
          if(entity[loop].text)
             entity[loop].len = strlen(entity[loop].text);
      html_entity_table_size = loop;

      for(top = html_entity_table_size - 1; top > 0; top--) {

          /* ---->  Find highest entry in unsorted part of list  <---- */
          for(loop = 1, highest = 0; loop <= top; loop++)
              if(entity[loop].entity && (!entity[highest].entity || strcmp(entity[loop].entity,entity[highest].entity) > 0))
                 highest = loop;

          /* ---->  Swap highest entry in unsorted part of list with bottom entry of sorted part of list  <---- */
          if(highest < top) {
             temp.entity = entity[top].entity, temp.text = entity[top].text, temp.number = entity[top].number, temp.len = entity[top].len;
             entity[top].entity = entity[highest].entity, entity[top].text = entity[highest].text, entity[top].number = entity[highest].number, entity[top].len = entity[highest].len;
             entity[highest].entity = temp.entity, entity[highest].text = temp.text, entity[highest].number = temp.number, entity[highest].len = temp.len;
	  }
      }

      for(loop = 0; entity[loop].entity || entity[loop].text; loop++) {
          if(entity[loop].number > 0) {
             entities[entity[loop].number] = loop;
	  }
      }
      
      return(html_entity_table_size);
}

/* ---->  Search for STR in HTML tag table, returning HTML_TAGTABLE_DATA  <---- */
/*        entry if found, otherwise NULL.                                       */
short html_search_tags(const char *str,unsigned char len,unsigned short *exception,unsigned short *tagtype)
{
      int  top = html_tag_table_size - 1,middle = 0,bottom = 0,result;
      char buffer[32];
      char *dest;

      *exception = 0, *tagtype = 0;
      if(Blank(str)) return(NOTHING);
      if(len > 31) len = 31;
      for(dest = buffer; *str && (len > 0); *dest++ = *str++, len--);
      *dest = '\0';

      while(bottom <= top) {
            middle = (top + bottom) / 2;
            if(tags[middle].tag) {
               if((result = strcasecmp(tags[middle].tag,buffer)) != 0) {
                  if(result < 0) bottom = middle + 1;
                     else top = middle - 1;
	       } else {
                  *exception = tags[middle].exception;
                  *tagtype   = tags[middle].tagtype;
                  return(middle);
	       }
	    } else bottom = middle + 1;
      }
      return(NOTHING);
}

/* ---->  Search for STR in HTML character entity table, returning  <---- */
/*        HTML_ENTITY_DATA entry if found, otherwise NULL.                */
struct html_entity_data *html_search_entities(const char *str,unsigned char len)
{
       int  top = html_entity_table_size - 1,middle = 0,bottom = 0,result;
       char buffer[16];
       char *dest;

       if(Blank(str)) return(NULL);
       if(len > 15) len = 15;
       for(dest = buffer; *str && (len > 0); *dest++ = *str++, len--);
       *dest = '\0';

       while(bottom <= top) {
             middle = (top + bottom) / 2;
             if(entity[middle].entity) {
                if((result = strcmp(entity[middle].entity,buffer)) != 0) {
                   if(result < 0) bottom = middle + 1;
                      else top = middle - 1;
		} else return(&entity[middle]);
	     } else bottom = middle + 1;
       }
       return(NULL);
}

/* ---->  Free images loaded into linked list/binary tree  <---- */
void html_free_images(void)
{
     struct html_image_data *next;

     for(; html_images; html_images = next) {
         next = html_images->next;
         FREENULL(html_images->filename);
         FREENULL(html_images->data);
         FREENULL(html_images);
     }
}

/* ---->  Initialise binary tree of image names  <---- */
void html_init_images(int count)
{
     struct html_image_data *ptr,*current,*last;
     int    sorted = 0,offset,right;

     while(sorted < count) {

           /* ---->  Find random position in linked list  <---- */
           for(ptr = html_images, offset = (lrand48() % count) + 1; offset; offset--)
               if(ptr->next) ptr = ptr->next;
                  else ptr = html_images;

           /* ---->  Seek first image that has not been added to binary tree (From offset in list)  <---- */
           while(ptr->tree)
               if(ptr->next) ptr = ptr->next;
                  else ptr = html_images;

           /* ---->  Find position in tree  <---- */
           if(html_image_tree) {
              current = html_image_tree, last = NULL;
              while(current)
                    if(strcasecmp(current->filename,ptr->filename) <= 0)
                       last = current, current = current->right, right = 1;
                          else last = current, current = current->left, right = 0;

              if(right) last->right = ptr;
	         else last->left = ptr;
	   } else html_image_tree = ptr;
           ptr->tree = 1;
           sorted++;
     }
}

/* ---->  Find image by name in binary tree  <---- */
struct html_image_data *html_search_images(const char *filename)
{
       struct html_image_data *current;
       int    value;

       if(html_image_tree) {
          current = html_image_tree;
          while(current)
                if((value = strcasecmp(current->filename,filename))) {
		   if(value < 0)
                      current = current->right;
                         else current = current->left;
		} else return(current);
       }
       return(NULL);
}

/* ---->  Load images into linked list/binary tree  <---- */
int html_load_images(void)
{
    int    error = 0,loaded = 0,size = 0,totalsize = 0,imgtype,img;
    struct html_image_data *tail = NULL;
    char   image[HTML_MAX_IMAGE_SIZE];
    struct html_image_data *new;
    struct dirent *direntry;
    char   name[TEXT_SIZE];
    char   *ptr;
    DIR    *dir;

    /* ---->  Open directory   <---- */
    writelog(SERVER_LOG,0,"RESTART","Loading HTML Interface images from the directory '%s'...",HTML_IMAGE_PATH);
    if((dir = opendir(HTML_IMAGE_PATH))) {

       /* ---->  Process files in directory  <---- */
       while(!error && (direntry = readdir(dir))) {

             /* ---->  Determine type of file (Must be '.jpeg', '.jpg' or '.gif'.)  <---- */
	     imgtype = 0;
	     snprintf(name,sizeof(name),HTML_IMAGE_PATH"%s",direntry->d_name);
	     for(ptr = (name + strlen(name)); (ptr > name) && (*ptr != '.'); ptr--);
	     if((ptr > name) && (*ptr == '.')) {
		ptr++;
		if(!strcasecmp(ptr,"jpeg")) imgtype = HTTP_IMAGE_JPEG;
		   else if(!strcasecmp(ptr,"jpg")) imgtype = HTTP_IMAGE_JPEG;
		      else if(!strcasecmp(ptr,"gif")) imgtype = HTTP_IMAGE_GIF;
	     }

	     /* ---->  Attempt to open image  <---- */
	     if(imgtype) {
		if((img = open(name,O_RDONLY))) {
		   if((size = read(img,image,HTML_MAX_IMAGE_SIZE)) > 0) {
		      if(size < HTML_MAX_IMAGE_SIZE) {
			 MALLOC(new,struct html_image_data);
			 new->next     = NULL;
			 new->left     = NULL;
			 new->right    = NULL;
			 new->filename = alloc_string((char *) direntry->d_name);
                         new->imgtype  = imgtype;
			 new->size     = size;
                         new->tree     = 0;
			 NMALLOC(new->data,unsigned char,size);
			 memcpy(new->data,image,size);

			 if(tail) {
			    tail->next = new;
			    tail       = new;
			 } else html_images = tail = new;
			 totalsize += size;
			 loaded++;
		      } else writelog(SERVER_LOG,0,"RESTART","  Image '%s' is too large (Maximum size allowed is %dKb.)",name,HTML_MAX_IMAGE_SIZE / KB);
		   } else writelog(SERVER_LOG,0,"RESTART","  Unable to read from the image file '%s' (%s).",name,strerror(errno));
		   close(img);
		} else writelog(SERVER_LOG,0,"RESTART","  Unable to open the image file '%s' for reading (%s).",name,strerror(errno));
	     }
       }

       closedir(dir);
       if(!error && !loaded) {
          writelog(SERVER_LOG,0,"RESTART","  Unable to read directory entries from directory '%s' (%s.)  HTML Interface images cannot be served internally  -  Using external HTML Interface data URL '%s'.",HTML_IMAGE_PATH,strerror(errno),html_data_url);
          error++;
       }
    } else {
       writelog(SERVER_LOG,0,"RESTART","  Unable to access the directory '%s' (%s.)  HTML Interface images cannot be served internally  -  Using external HTML Interface data URL '%s'.",HTML_IMAGE_PATH,strerror(errno),html_data_url);
       error++;
    }

    if(error) {
       html_free_images();
       html_internal_images = 0;
       return(0);
    } else {
       html_init_images(loaded);
       writelog(SERVER_LOG,0,"RESTART","  %d images loaded (%dKb)  -  HTML Interface images will be served internally.",loaded,totalsize / KB);
    }
    return(1);
}

/* ---->  Free parameter list  <---- */
void html_free()
{
     struct html_param_data *current;

     while(params) {
           current = params;
           params  = params->next;
           FREENULL(current->fullcontents);
           FREENULL(current->contents);
           FREENULL(current->name);
           FREENULL(current);
     }
     params = NULL;
}

/* ---->  Initialise parameter list  <---- */
unsigned char html_init(char *src,int len)
{
	 struct   html_param_data *new;
         int      namelen,contentslen;
         char     *name,*contents;
	 unsigned char check = 0;
	 char     hex[3];
	 char     *ptr;

	 if(params && !src) html_free();
	 if(!src) return(0);
	 hex[2] = '\0';
	 if(len > 0) check = 1;

	 while(*src && (*src != ' ') && (!check || (len > 0))) {

	       /* ---->  Name  <---- */
               namelen = 0;
	       ptr = name = src;
	       while(*src && (*src != '=') && (*src != ' ') && (!check || (len > 0)))
		     switch(*src) {
			    case '%':
				 if((!check && (strlen(src) > 2)) || (len > 2)) {
				    hex[0] = *(src + 1), hex[1] = *(src + 2);
				    *ptr++ = tohex(hex), src += 3, len -= 3;
                                    namelen++;
				 } else while(*src && (!check || (len > 0))) src++, len--;
				 break;
			    case '+':
				 *ptr++ = ' ';
				 src++, len--;
                                 namelen++;
				 break;
			    default:
				 *ptr++ = *src++;
                                 namelen++;
				 len--;
		     }

	       if(*src && (*src != ' ') && (!check || (len > 0))) src++, len--;

	       /* ---->  Contents (Value)  <---- */
               contentslen = 0;
	       ptr = contents = src;
	       while(*src && (*src != '&') && (*src != ' ') && (!check || (len > 0)))
		     switch(*src) {
			    case '%':
				 if((!check && (strlen(src) > 2)) || (len > 2)) {
				    hex[0] = *(src + 1), hex[1] = *(src + 2);
				    *ptr++ = tohex(hex), src += 3, len -= 3;
                                    contentslen++;
				 } else while(*src) src++;
				 break;
			    case '+':
				 *ptr++ = ' ';
				 src++, len--;
                                 contentslen++;
				 break;
			    default:
				 *ptr++ = *src++;
                                 contentslen++;
				 len--;
		     }

	       if(*src && (*src != ' ') && (!check || (len > 0))) src++, len--;

               /* ---->  Create new set of HTML parameters and add to linked list  <---- */
	       if(!Blank(name)) {
		  MALLOC(new,struct html_param_data);

                  /* ---->  Parameter name (Restricted to TEXT_SIZE)  <---- */
		  if(namelen > TEXT_SIZE) namelen = TEXT_SIZE;
                  NMALLOC(new->name,char,namelen + 1);
                  memcpy(new->name,name,namelen);
                  *(new->name + namelen) = '\0';

                  /* ---->  Parameter value (Unrestricted for future usage where length must not be truncated to TEXT_SIZE)  <---- */
                  NMALLOC(new->fullcontents,char,contentslen + 1);
                  memcpy(new->fullcontents,contents,contentslen);
                  *(new->fullcontents + contentslen) = '\0';

                  /* ---->  Parameter value (Restricted to TEXT_SIZE)  <---- */
                  if(contentslen > TEXT_SIZE) contentslen = TEXT_SIZE;
                  NMALLOC(new->contents,char,contentslen + 1);
                  memcpy(new->contents,contents,contentslen);
                  *(new->contents + contentslen) = '\0';

                  /* ---->  Add to linked list of parameters  <---- */
		  new->next = params;
		  params    = new;
	       }
	 }
	 return(params ? 1:0);
}

/* ---->  Lookup parameter in parameter list  <---- */
/*            NAME:  Parameter name to search for.                */
/*         LEADING:  Strip leading spaces from contents.          */
/*        TRAILING:  Strip trailing spaces from contents.         */
/*          EXCESS:  Filter excess spaces (Two or more together)  */
/*         INVALID:  Remove control characters (< 32.)            */
/*        TRUNCATE:  Truncate contents to TEXT_SIZE.              */
/*                                                                */
/*  NOTE:  The LEADING, TRAILING and EXCESS options permanently   */
/*         alter the parameter contents value.                    */
const char *html_lookup(const char *name,unsigned char leading,unsigned char trailing,unsigned char excess,unsigned char invalid,unsigned char truncate)
{
      struct html_param_data *current;
      char   *contents = NULL;
      char   *ptr,*ptr2;

      if(Blank(name)) return(NULL);
      for(current = params; current; current = current->next) {
          if(current->name && !strcasecmp(current->name,name)) {
             contents = (truncate) ? current->contents:current->fullcontents;

	     if(contents) {

   	        /* ---->  Strip leading spaces  <---- */
	        if(leading) {
                   ptr = ptr2 = contents;
                   for(; *ptr == ' '; ptr++);
                   for(; *ptr; *ptr2++ = *ptr++);
                   *ptr2 = '\0';
		}

                /* ---->  Strip trailing spaces  <---- */
                if(trailing) {
                   ptr = contents + strlen(contents) - 1;
	    	   for(; (ptr >= contents) && (*ptr == ' '); *ptr-- = '\0');
		}

                /* ---->  Filter excess spaces and/or strip control characters  <---- */
                if(excess || invalid) {
                   ptr = ptr2 = contents;
	   	   while(*ptr) {
                         for(; *ptr && (*ptr != ' '); ptr++)
                             if(*ptr >= 32) *ptr2++ = *ptr;

                         if(excess && (*ptr == ' ')) {
			    *ptr2++ = ' ';
                            for(; *ptr == ' '; ptr++);
			 } else for(; *ptr && (*ptr == ' '); *ptr2++ = *ptr++);
		   }
                   *ptr2 = '\0';
		}
	     }
                
	     /* ---->  Return value of parameter  <---- */
             return(contents);
	  }
      }
      return(NULL);
}

/* ---->  Convert standard text to HTML format  <---- */
const char *text_to_html(struct descriptor_data *d,const char *text,char *buffer,int *length,int limit)
{
      unsigned char          overflow = 0,repeatansi = 1,html = 0,embolden,darken,toggle;
      int                    count,autoformat = 0,inlink = 0;
      struct   html_tag_data *new,*last,*current;
      static   const char    *codes = "01234567";
      unsigned short         exception,tagtype;
      char                   *dest = buffer;
      unsigned char          defaults = 1;
      const    char          *ptr,*colour;
      short                  tag;

      *buffer = '\0', *length = 0;
      if(!text) return(buffer);

      if(IsHtml(d) && (d->html->txtflags & HTML_TAG)) {

         /* ---->  Part way through HTML tag  <---- */
         if(*text && (*text == '\016')) html = 1;
         while(*text && (*text != '>') && ((*length + 1) <= limit)) *dest++ = *text++, (*length)++;
         if(*text && (*text == '>')) {
            if((*length + 1) <= limit) *dest++ = *text++, (*length)++;
            d->html->txtflags &= ~HTML_TAG;
	 }
         if((*length + 1) > limit) overflow = 1;
         defaults = 0;
      } else if(IsHtml(d) && (d->html->txtflags & HTML_COMMENT)) {

         /* ---->  Part way through HTML comment  <---- */
         if(*text && (*text == '\016')) html = 1;
         while(*text && (*text != '>')) text++;
         if(*text && (*text == '>')) {
            d->html->txtflags &= ~HTML_COMMENT;
            text++;
	 }
      }

      for(ptr = text; *ptr && (*ptr == '\016'); ptr++); 
      if(*ptr && (*ptr != '\x1B')) {
         if(IsHtml(d) && (d->html->flags & HTML_SIMULATE_ANSI)) {
            if((*length + 20) <= limit) {
               strcpy(dest,"<FONT COLOR="HTML_LCYAN">");
               dest += 20, *length += 20, d->html->txtflags |= (TXT_BOLD|HTML_FONT);
               d->html->flags |= TXT_FG_CHANGE;
	    } else overflow = 1;
	 } else if((*length + 3) <= limit) {
            strcpy(dest,"<B>");
            dest += 3, *length += 3, d->html->txtflags |= TXT_BOLD;
	 } else overflow = 1;
      }

      if((d->flags & PROMPT) && !(d->html->txtflags & HTML_NEWLINE)) {
         if((*length + 4) <= limit) {
            strcpy(dest,"<BR>");
            dest += 4, *length += 4, d->html->txtflags &= ~HTML_NEWLINE;
	 }
         d->flags &= ~PROMPT;
      }

      while(*text && !overflow) {
            if(autoformat > 0) autoformat--;
            switch(*text) {
                   case '\x05':

                        /* ---->  Hanging indent control  <---- */
                        if(*(++text)) text++;
                        autoformat = 2;
                        break;
                   case '\x06':

                        /* ---->  Skip rest of line  <---- */
                        for(; *text && (*text != '\n'); text++);
                        if(*text && (*text == '\n')) text++;
                        autoformat = 2;
                        break;
                   case '\x07':

                        /* ---->  Beep  <---- */
                        text++;
                        break;
                   case '\x0E':

                        /* ---->  Toggle evaluation of HTML tags  <---- */
                        html = !html;
                        text++;
                        break;
                   case '\x0F':

                        /* ---->  Insert HTML Interface connection identity code  <---- */
                        if(html && (d->html->txtflags & HTML_TAG)) {
                           if((*length + 22) <= limit) {
                              sprintf(dest,"CODE=%08X%08X&",(int) d->html->id1,(int) d->html->id2);
                              dest += 22, *length += 22;
			   } else overflow = 1;
			}
                        text++;
                        break;
                   case '\x1B':

                        /* ---->  ANSI code  <---- */
                        autoformat = 2, embolden = 0, darken = 0, ptr = NULL;
                        if(*(++text) && (*text == '[') && *(++text))
                           switch(*text) {
                                  case '5':

                                       /* ---->  Flashing text  <---- */
                                       if(!(d->html->txtflags & TXT_BLINK)) {
                                          if((*length + 7) <= limit) {
                                             strcpy(dest,"<BLINK>");
                                             dest += 7, *length += 7;
                                             d->html->txtflags |= TXT_BLINK;
					  } else overflow = 1;
				       }
                                       break;
                                  case '4':
                                       if(*(++text) && (*text == 'm') && !(d->html->txtflags & TXT_UNDERLINE)) {

                                          /* ---->  Underlined text  <---- */
                                          if((*length + 3) <= limit) {
                                             strcpy(dest,(IsHtml(d) && (d->html->flags & HTML_UNDERLINE)) ? "<U>":"<I>");
                                             dest += 3, *length += 3;
                                             d->html->txtflags |= TXT_UNDERLINE;
					  } else overflow = 1;
				       } else if(isdigit(*text) && (*(text + 1) == 'm')) {

					  /* ---->  Coloured text background  <---- */
                                          if(!overflow)
					     if(IsHtml(d) && (d->html->flags & HTML_SIMULATE_ANSI) && (d->html->flags & HTML_STYLE)) {
						switch(*text) {
						       case '0':
							    if(repeatansi || (((d->html->txtflags & TXT_BG_MASK) >> TXT_BG_SHIFT) != TXT_BLACK) || !(d->html->flags & TXT_BG_CHANGE)) {
							       colour = (IsHtml(d) && (d->html->flags & HTML_WHITE_AS_BLACK)) ? HTML_DWHITE:HTML_DBLACK;
							       d->html->txtflags &= ~TXT_BG_MASK;
							       d->html->txtflags |= (TXT_BLACK << TXT_BG_SHIFT);
							    } else colour = NULL;
							    break;
						       case '1':
							    if(repeatansi || (((d->html->txtflags & TXT_BG_MASK) >> TXT_BG_SHIFT) != TXT_RED) || !(d->html->flags & TXT_BG_CHANGE)) {
							       colour = HTML_DRED;
							       d->html->txtflags &= ~TXT_BG_MASK;
							       d->html->txtflags |= (TXT_RED << TXT_BG_SHIFT);
							    } else colour = NULL;
							    break;
						       case '2':
							    if(repeatansi || (((d->html->txtflags & TXT_BG_MASK) >> TXT_BG_SHIFT) != TXT_GREEN) || !(d->html->flags & TXT_BG_CHANGE)) {
							       colour = HTML_DGREEN;
							       d->html->txtflags &= ~TXT_BG_MASK;
							       d->html->txtflags |= (TXT_GREEN << TXT_BG_SHIFT);
							    } else colour = NULL;
							    break;
						       case '3':
							    if(repeatansi || (((d->html->txtflags & TXT_BG_MASK) >> TXT_BG_SHIFT) != TXT_YELLOW) || !(d->html->flags & TXT_BG_CHANGE)) {
							       colour = HTML_DYELLOW;
							       d->html->txtflags &= ~TXT_BG_MASK;
							       d->html->txtflags |= (TXT_YELLOW << TXT_BG_SHIFT);
							    } else colour = NULL;
							    break;
						       case '4':
							    if(repeatansi || (((d->html->txtflags & TXT_BG_MASK) >> TXT_BG_SHIFT) != TXT_BLUE) || !(d->html->flags & TXT_BG_CHANGE)) {
							       colour = HTML_DBLUE;
							       d->html->txtflags &= ~TXT_BG_MASK;
							       d->html->txtflags |= (TXT_BLUE << TXT_BG_SHIFT);
							    } else colour = NULL;
							    break;
						       case '5':
							    if(repeatansi || (((d->html->txtflags & TXT_BG_MASK) >> TXT_BG_SHIFT) != TXT_MAGENTA) || !(d->html->flags & TXT_BG_CHANGE)) {
							       colour = HTML_DMAGENTA;
							       d->html->txtflags &= ~TXT_BG_MASK;
							       d->html->txtflags |= (TXT_MAGENTA << TXT_BG_SHIFT);
							    } else colour = NULL;
							    break;
						       case '6':
							    if(repeatansi || (((d->html->txtflags & TXT_BG_MASK) >> TXT_BG_SHIFT) != TXT_CYAN) || !(d->html->flags & TXT_BG_CHANGE)) {
							       colour = HTML_DCYAN;
							       d->html->txtflags &= ~TXT_BG_MASK;
							       d->html->txtflags |= (TXT_CYAN << TXT_BG_SHIFT);
							    } else colour = NULL;
							    break;
						       case '7':
							    if(repeatansi || (((d->html->txtflags & TXT_BG_MASK) >> TXT_BG_SHIFT) != TXT_WHITE) || !(d->html->flags & TXT_BG_CHANGE)) {
							       colour = (IsHtml(d) && (d->html->flags & HTML_WHITE_AS_BLACK)) ? HTML_DBLACK:HTML_DWHITE;
							       d->html->txtflags &= ~TXT_BG_MASK;
							       d->html->txtflags |= (TXT_WHITE << TXT_BG_SHIFT);
							    } else colour = NULL;
							    break;
						       default:
							    colour = NULL;
							    autoformat = 0;
						}
						repeatansi = 0;

						if(colour) {

                                                   /* ---->  Reset text background colour  <---- */
						   if(d->html->txtflags & HTML_SPAN_BACKGROUND) {
						      if((*length + 7) <= limit) {
						         strcpy(dest,"</SPAN>");
							 dest += 7, *length += 7;
                                                         d->html->txtflags &= ~HTML_SPAN_BACKGROUND;
						      } else overflow = 1;
						   }

                                                   /* ----->  Change text background colour  <---- */
						   if(!overflow && ((*length + 34) <= limit)) {
						      sprintf(dest,"<SPAN STYLE=\"background:%s;\">",colour);
						      d->html->txtflags |= (HTML_SPAN_BACKGROUND|TXT_BG_CHANGE);
						      dest += 34, *length += 34;
						   } else overflow = 1;
						}
					     }
				       }
                                       break;
                                  case '1':

                                       /* ---->  Bold text  <---- */
                                       if(!(d->html->txtflags & TXT_BOLD)) {
                                          ptr = codes + ((d->html->txtflags & TXT_FG_MASK) >> TXT_FG_SHIFT);
                                          embolden = 1;
				       } else break;
                                  case '0':

                                       /* ---->  Coloured text  <---- */
                                       if(!embolden && *(text + 1) && (*(text + 1) == 'm')) embolden = 1, darken = 1;
                                       if(embolden || (*(++text) && (*text == ';'))) {
                                          if(embolden || (*(++text) && (*text == '3'))) {
                                             if(embolden || *(++text)) {

                                                /* ---->  Reset text background colour  <---- */
                                                if(!overflow && (d->html->txtflags & HTML_SPAN_BACKGROUND)) {
                                                   if((*length + 7) <= limit) {
                                                      strcpy(dest,"</SPAN>");
                                                      dest += 7, *length += 7;
                                                      d->html->txtflags &= ~(HTML_SPAN_BACKGROUND|TXT_BG_MASK);
						   } else overflow = 1;
						}

                                                /* ---->  Reset underline  <---- */
                                                if(!embolden && !overflow && (d->html->txtflags & TXT_UNDERLINE)) {
                                                   if((*length + 4) <= limit) {
                                                      strcpy(dest,(IsHtml(d) && (d->html->flags & HTML_UNDERLINE)) ? "</U>":"</I>");
                                                      dest += 4, *length += 4;
                                                      d->html->txtflags &= ~TXT_UNDERLINE;
						   } else overflow = 1;
						}

                                                /* ---->  Reset flashing  <---- */
                                                if(!embolden && !overflow && (d->html->txtflags & TXT_BLINK)) {
                                                   if((*length + 8) <= limit) {
                                                      strcpy(dest,"</BLINK>");
                                                      dest += 8, *length += 8;
                                                      d->html->txtflags &= ~TXT_BLINK;
						   } else overflow = 1;
						}
                                                if(darken) embolden = 0;

                                                if(!ptr) {
                                                   for(ptr = text; *ptr && (*ptr != 'm'); ptr++);
                                                   if(*ptr && (*ptr == 'm')) ptr++;
                                                   if(*ptr && (*ptr == '\x1B')) {
                                                      if(*(++ptr) && (*ptr == '[')) {
                                                         if(*(++ptr) && (*ptr == '1') && *(ptr + 1) && (*(ptr + 1) == 'm'))
                                                            ptr = text, embolden = 1;
						      }
						   }
                                                   if(ptr != text) ptr = NULL;
						}

                                                if(!overflow) {
                                                   if(IsHtml(d) && (d->html->flags & HTML_SIMULATE_ANSI)) {
                                                      switch((ptr) ? *ptr:*text) {
                                                             case '0':
                                                                  if(repeatansi || (((d->html->txtflags & TXT_FG_MASK) >> TXT_FG_SHIFT) != TXT_BLACK) || (embolden && !(d->html->txtflags & TXT_BOLD)) || (!embolden && (d->html->txtflags & TXT_BOLD)) || !(d->html->flags & TXT_FG_CHANGE)) {
                                                                     colour = (IsHtml(d) && (d->html->flags & HTML_WHITE_AS_BLACK)) ? (embolden) ? HTML_LWHITE:HTML_DWHITE:(embolden) ? HTML_LBLACK:HTML_DBLACK;
                                                                     d->html->txtflags &= ~TXT_FG_MASK;
                                                                     d->html->txtflags |= (TXT_BLACK << TXT_FG_SHIFT);
								  } else colour = NULL;
                                                                  break;
                                                             case '1':
                                                                  if(repeatansi || (((d->html->txtflags & TXT_FG_MASK) >> TXT_FG_SHIFT) != TXT_RED) || (embolden && !(d->html->txtflags & TXT_BOLD)) || (!embolden && (d->html->txtflags & TXT_BOLD)) || !(d->html->flags & TXT_FG_CHANGE)) {
                                                                     colour = (embolden) ? HTML_LRED:HTML_DRED;
                                                                     d->html->txtflags &= ~TXT_FG_MASK;
                                                                     d->html->txtflags |= (TXT_RED << TXT_FG_SHIFT);
								  } else colour = NULL;
                                                                  break;
                                                             case '2':
                                                                  if(repeatansi || (((d->html->txtflags & TXT_FG_MASK) >> TXT_FG_SHIFT) != TXT_GREEN) || (embolden && !(d->html->txtflags & TXT_BOLD)) || (!embolden && (d->html->txtflags & TXT_BOLD)) || !(d->html->flags & TXT_FG_CHANGE)) {
                                                                     colour = (embolden) ? HTML_LGREEN:HTML_DGREEN;
                                                                     d->html->txtflags &= ~TXT_FG_MASK;
                                                                     d->html->txtflags |= (TXT_GREEN << TXT_FG_SHIFT);
								  } else colour = NULL;
                                                                  break;
                                                             case '3':
                                                                  if(repeatansi || (((d->html->txtflags & TXT_FG_MASK) >> TXT_FG_SHIFT) != TXT_YELLOW) || (embolden && !(d->html->txtflags & TXT_BOLD)) || (!embolden && (d->html->txtflags & TXT_BOLD)) || !(d->html->flags & TXT_FG_CHANGE)) {
                                                                     colour = (embolden) ? HTML_LYELLOW:HTML_DYELLOW;
                                                                     d->html->txtflags &= ~TXT_FG_MASK;
                                                                     d->html->txtflags |= (TXT_YELLOW << TXT_FG_SHIFT);
								  } else colour = NULL;
                                                                  break;
                                                             case '4':
                                                                  if(repeatansi || (((d->html->txtflags & TXT_FG_MASK) >> TXT_FG_SHIFT) != TXT_BLUE) || (embolden && !(d->html->txtflags & TXT_BOLD)) || (!embolden && (d->html->txtflags & TXT_BOLD)) || !(d->html->flags & TXT_FG_CHANGE)) {
                                                                     colour = (embolden) ? HTML_LBLUE:HTML_DBLUE;
                                                                     d->html->txtflags &= ~TXT_FG_MASK;
                                                                     d->html->txtflags |= (TXT_BLUE << TXT_FG_SHIFT);
								  } else colour = NULL;
                                                                  break;
                                                             case '5':
                                                                  if(repeatansi || (((d->html->txtflags & TXT_FG_MASK) >> TXT_FG_SHIFT) != TXT_MAGENTA) || (embolden && !(d->html->txtflags & TXT_BOLD)) || (!embolden && (d->html->txtflags & TXT_BOLD)) || !(d->html->flags & TXT_FG_CHANGE)) {
                                                                     colour = (embolden) ? HTML_LMAGENTA:HTML_DMAGENTA;
                                                                     d->html->txtflags &= ~TXT_FG_MASK;
                                                                     d->html->txtflags |= (TXT_MAGENTA << TXT_FG_SHIFT);
								  } else colour = NULL;
                                                                  break;
                                                             case '6':
                                                                  if(repeatansi || (((d->html->txtflags & TXT_FG_MASK) >> TXT_FG_SHIFT) != TXT_CYAN) || (embolden && !(d->html->txtflags & TXT_BOLD)) || (!embolden && (d->html->txtflags & TXT_BOLD)) || !(d->html->flags & TXT_FG_CHANGE)) {
                                                                     colour = (embolden) ? HTML_LCYAN:HTML_DCYAN;
                                                                     d->html->txtflags &= ~TXT_FG_MASK;
                                                                     d->html->txtflags |= (TXT_CYAN << TXT_FG_SHIFT);
								  } else colour = NULL;
                                                                  break;
                                                             case '7':
                                                                  if(repeatansi || (((d->html->txtflags & TXT_FG_MASK) >> TXT_FG_SHIFT) != TXT_WHITE) || (embolden && !(d->html->txtflags & TXT_BOLD)) || (!embolden && (d->html->txtflags & TXT_BOLD)) || !(d->html->flags & TXT_FG_CHANGE)) {
                                                                     colour = (IsHtml(d) && (d->html->flags & HTML_WHITE_AS_BLACK)) ? (embolden) ? HTML_LBLACK:HTML_DBLACK:(embolden) ? HTML_LWHITE:HTML_DWHITE;
                                                                     d->html->txtflags &= ~TXT_FG_MASK;
                                                                     d->html->txtflags |= (TXT_WHITE << TXT_FG_SHIFT);
								  } else colour = NULL;
                                                                  break;
                                                             default:
                                                                  colour = NULL;
                                                                  autoformat = 0;
						      }
                                                      repeatansi = 0;

                                                      if(colour) {
                                                         if(d->html->txtflags & HTML_FONT) {
                                                            if((*length + 7) <= limit) {
                                                               strcpy(dest,"</FONT>");
                                                               dest += 7, *length += 7, d->html->txtflags &= ~HTML_FONT;
							    } else overflow = 1;
							 }

                                                         if(!overflow && ((*length + 20) <= limit)) {
                                                            sprintf(dest,"<FONT COLOR=%s>",colour);
                                                            dest += 20, *length += 20, d->html->txtflags |= HTML_FONT;
                                                            if(embolden) d->html->txtflags |= TXT_BOLD;
                                                               else d->html->txtflags &= ~TXT_BOLD;
    						            d->html->flags |= TXT_FG_CHANGE;
							 } else overflow = 1;
						      }
						   } else if(embolden) {
                                                      if(!(d->html->txtflags & TXT_BOLD)) {
                                                         if((*length + 3) <= limit) {
                                                            strcpy(dest,"<B>");
                                                            dest += 3, *length += 3;
                                                            d->html->txtflags |= TXT_BOLD;
							 } else overflow = 1;
						      }
						   } else if(d->html->txtflags & TXT_BOLD) {
                                                      if((*length + 4) <= limit) {
                                                         strcpy(dest,"</B>");
                                                         dest += 4, *length += 4;
                                                         d->html->txtflags &= ~TXT_BOLD;
						      } else overflow = 1;
						   }
						}
					     }
					  }
				       }
                                       break;
                                  default:
                                       autoformat = 0;
			   }
                        for(; *text && (*text != 'm'); text++);
                        if(*text && (*text == 'm')) text++;
                        break;
                   case ' ':

                        /* ---->  Space (In preformatted text)  <---- */
                        autoformat = 2;
                        if(html && (d->html->txtflags & HTML_TAG)) {
                           d->html->txtflags &= ~HTML_NEWLINE;
                           *dest++ = *text++, (*length)++;
			} else if(d->html->txtflags & HTML_PREFORMAT) {
                           if((*length + 6) <= limit) {
                              strcpy(dest,"&nbsp;");
                              dest += 6, *length += 6, text++;
                              d->html->txtflags &= ~HTML_NEWLINE;
			   } else overflow = 1;
			} else if((*length + 1) <= limit) {
                           d->html->txtflags &= ~HTML_NEWLINE;
			   *dest++ = ' ', (*length)++, text++;
			   if(!html) {
			      if((*text == ' ') && (isalnum(*(text + 1)) || ispunct(*(text + 1)))) {
				 if((*length + 7) <= limit) {
				    strcpy(dest,"&nbsp; ");
				    dest += 7, *length += 7, text++;
				 } else overflow = 1;
			      } else while(*text && (*text == ' ')) text++;
			   } else while(*text && (*text == ' ')) text++;
			} else overflow = 1;
                        break;
                   case '\n':

                        /* ---->  Line Feed  <---- */
                        autoformat = 2;
                        if(!overflow) {
			   if(!html || ((d->html->txtflags & HTML_PREFORMAT) && !(d->html->txtflags & HTML_TAG))) {
			      if((*length + 4) <= limit) {
				 strcpy(dest,"<BR>");
				 dest += 4, *length += 4;
				 d->html->txtflags |= HTML_NEWLINE, text++;
			      } else overflow = 1;
			   } else if((*length + 1) <= limit)
			      *dest++ = ' ', (*length)++, text++;
				 else overflow = 1;
			}
                        break;
                   case '<':

                        /* ---->  Beginning of HTML tag  <---- */
                        if(!html) {
                           if((*length + 4) <= limit) {
                              strcpy(dest,"&lt;");
                              dest += 4, *length += 4;
                              d->html->txtflags &= ~HTML_NEWLINE, text++;
			   } else overflow = 1;
			} else if(*(text + 1) == '!') {

                           /* ---->  HTML comment  <---- */
                           d->html->txtflags |= HTML_COMMENT;
                           for(; *text && (*text != '>'); text++);
                           if(*text && (*text == '>')) {
                              d->html->txtflags &= ~HTML_COMMENT;
                              text++;
			   }
			} else if((*length + 1) <= limit) {

                           /* ---->  HTML tag  <---- */
                           d->html->txtflags &= ~HTML_NEWLINE;
                           d->html->txtflags |=  HTML_TAG;
                           *dest++ = *text++, (*length)++;

                           /* ---->  Add tag to tag table  <---- */
                           for(ptr = text, count = 0; *ptr && (*ptr != ' ') && (*ptr != '>') && (*ptr != '\n'); count++, ptr++);
                           if(*text == '/') {
                              if((--count > 0) && ((tag = html_search_tags(text + 1,count,&exception,&tagtype)) != NOTHING)) {
                                 if(exception != EXCEPTION_SKIP) {
                                    for(current = d->html->tag, last = NULL; current && (current->ptr != tag); last = current, current = current->next);
                                    if(current) {
                                       if(last) last->next = current->next;
                                          else d->html->tag = current->next;
                                       FREENULL(current);
				    }
                                    if(exception == EXCEPTION_FONT) d->html->txtflags &= ~HTML_FONT;
                                    if((tagtype == HTML_TAG_PRE) || (tagtype == HTML_TAG_TT))
                                       d->html->txtflags &= ~HTML_PREFORMAT;

                                    if(exception == EXCEPTION_PRE_TO_TT) {

                                       /* ---->  Replace </PRE> tag with </TT> tag  <---- */
                                       for(dest--, (*length)--; *text && (*text != '>'); text++);
                                       if(*text && (*text == '>')) text++;
                                       d->html->txtflags &= ~HTML_TAG;
                                       if((*length + 5) <= limit) {
                                          strcpy(dest,"</TT>");
                                          dest += 5, *length += 5;
				       } else overflow = 1;
				    }
				 } else {
                                    if((tagtype == HTML_TAG_A) && (inlink > 0)) inlink--;
                                    for(dest--, (*length)--; *text && (*text != '>'); text++);
                                    if(*text && (*text == '>')) text++;
                                    d->html->txtflags &= ~HTML_TAG;
				 }
			      }
			   } else if((count > 0) && ((tag = html_search_tags(text,count,&exception,&tagtype)) != NOTHING)) {
                              if((exception != EXCEPTION_SKIP) && (exception != EXCEPTION_IGNORE)) {
                                 MALLOC(new,struct html_tag_data);
                                 new->next    = d->html->tag;
                                 new->ptr     = tag;
                                 d->html->tag = new;
			      }

                              if(tagtype == HTML_TAG_A) inlink++;

                              if((tagtype == HTML_TAG_PRE) || (tagtype == HTML_TAG_TT)) {
                                 d->html->txtflags |= HTML_PREFORMAT;
                                 dest--, (*length)--, toggle = 0;
                                 if(dest > buffer) {
                                    dest--, (*length)--;
                                    if(dest >= buffer) {
                                       for(count = 0; (dest >= buffer) && *dest && (*dest == ' '); dest--, (*length)--, count++);
                                       dest++, (*length)++;
				    }
				 }

                                 if((*length + 1) <= limit)
                                    *dest++ = '<', (*length)++;
                                       else overflow = 1;
			      }

                              switch(exception) {
                                     case EXCEPTION_SKIP:
                                     case EXCEPTION_PRE_TO_TT:

                                          /* ---->  Skip tag  <---- */
                                          for(dest--, (*length)--; *text && (*text != '>'); text++);
                                          if(*text && (*text == '>')) text++;
                                          d->html->txtflags &= ~HTML_TAG;
                                          if(exception != EXCEPTION_PRE_TO_TT) break;

                                          /* ---->  Replace <PRE> tag with <TT> tag  <---- */
                                          if((*length + 4) <= limit) {
                                             strcpy(dest,"<TT>");
                                             dest += 4, *length += 4;
		  		          } else overflow = 1;
                                     case EXCEPTION_FONT:

                                          /* ---->  <FONT> tag  <---- */
                                          if(d->html->txtflags & HTML_FONT) {
                                             MALLOC(new,struct html_tag_data);
                                             new->next          = d->html->tag;
                                             new->ptr           = fonttag;
                                             d->html->tag       = new;
                                             d->html->txtflags &= ~HTML_FONT;
					  }
                                          break;
                                     case EXCEPTION_IGNORE:

                                          /* ---->  Ignore tag (Don't place on stack)  <---- */
                                          if((tagtype == HTML_TAG_TD) || (tagtype == HTML_TAG_TH)) {
                                             d->html->txtflags &= ~(HTML_FONT|HTML_SPAN_BACKGROUND|TXT_BOLD|TXT_UNDERLINE|TXT_ITALIC);
                                             repeatansi = 1;
					  }
                                          break;
			      }

                              if((tagtype == HTML_TAG_PRE) || (tagtype == HTML_TAG_TT)) {
                                 for(; count && !overflow; count--) {
                                     if((*length + 6) <= limit) {
                                        strcpy(dest,"&nbsp;");
                                        dest += 6, *length += 6;
				     } else overflow = 1;
				 }

                                 if(toggle && !overflow) {
                                    if((*length + 1) <= limit)
                                       *dest++ = '\x0E', (*length)++;
                                          else overflow = 1;
				 }
			      }
			   }
			} else overflow = 1;
                        break;
                   case '&':

                        /* ---->  Beginning of HTML entity?  <---- */
                        if(!html) {
                           if((*length + 5) <= limit) {
                              strcpy(dest,"&amp;");
                              dest += 5, *length += 5;
                              d->html->txtflags &= ~HTML_NEWLINE, text++;
		   	   } else overflow = 1;
			} else if((*length + 1) <= limit) {
                           d->html->txtflags &= ~HTML_NEWLINE;
                           *dest++ = *text++, (*length)++;
			} else overflow = 1;
                        break;
                   case '\"':

                        /* ---->  Quotation mark  <---- */
                        if(!html) {
                           if((*length + 6) <= limit) {
                              strcpy(dest,"&quot;");
                              dest += 6, *length += 6;
                              d->html->txtflags &= ~HTML_NEWLINE, text++;
			   } else overflow = 1;
			} else if((*length + 1) <= limit) {
                           d->html->txtflags &= ~HTML_NEWLINE;
                           *dest++ = *text++, (*length)++;
			} else overflow = 1;
                        break;
                   case '>':
                   case ';':
                   case ':':
                   case '|':
                   case '*':
                   case '8':

                        /* ---->  End of HTML tag/entity and graphical smileys (Emoticons)  <---- */
                        if(autoformat && !(command_type & NO_AUTO_FORMAT) && (d->html->flags & HTML_SMILEY) && !(d->html->flags & HTML_INPUT) && !html && !(d->html->txtflags & HTML_PREFORMAT)) {
                           int   loop,count;
                           const char *ptr;

                           /* ---->  Smiley (Emoticon)?  <---- */
                           for(loop = 0; smileys[loop].smiley && strncasecmp(text,smileys[loop].smiley,smileys[loop].smileylength); loop++);
                           if(smileys[loop].smiley) {
                              for(ptr = text, count = smileys[loop].smileylength; *ptr && (count > 0); ptr++, count--);
                              if(*ptr && !count && ((*ptr == ' ') || (*ptr == '\n') || (*ptr == '.') || !isprint(*ptr))) {
                                 if((*length + smileys[loop].imglength) <= limit) {

                                    /* ---->  Insert smiley icon  <---- */
                                    for(ptr = smileys[loop].img; *ptr; *dest++ = *ptr++, (*length)++);
                                    text += smileys[loop].smileylength;
                                    autoformat = 1;
				 } else overflow = 1;
			      } else autoformat = 0;
			   } else autoformat = 0;

                           if(!autoformat) {
                              if((*length + 1) <= limit) {
                                 d->html->txtflags &= ~HTML_NEWLINE;
                                 *dest++ = *text++, (*length)++;
	   		      } else overflow = 1;
			   }
			} else if(*text == '>') {

			   /* ---->  End of HTML tag  <---- */
			   if(!html) {
			      if((*length + 4) <= limit) {
				 strcpy(dest,"&gt;");
				 dest += 4, *length += 4;
				 d->html->txtflags &= ~HTML_NEWLINE, text++;
			      } else overflow = 1;
			   } else if((*length + 1) <= limit) {
			      d->html->txtflags &= ~(HTML_NEWLINE|HTML_TAG);
			      *dest++ = *text++, (*length)++;
			   } else overflow = 1;
			} else if(*text == ';') {

                           /* ---->  Closing ';' of HTML entity?  <---- */
			   if(!html) {
			      if((*length + 5) <= limit) {
				 strcpy(dest,"&#59;");
				 dest += 5, *length += 5;
				 d->html->txtflags &= ~HTML_NEWLINE, text++;
			      } else overflow = 1;
			   } else if((*length + 1) <= limit) {
			      d->html->txtflags &= ~HTML_NEWLINE;
			      *dest++ = *text++, (*length)++;
			   } else overflow = 1;
			} else if((*length + 1) <= limit) {
                           d->html->txtflags &= ~HTML_NEWLINE;
                           *dest++ = *text++, (*length)++;
			} else overflow = 1;
                        break;
#ifdef HTML_FTP_LINK
                   case 'f':
                   case 'F':

                        /* ---->  'ftp.domain' or 'ftp://ftp.domain' link  <---- */
                        if(autoformat && !(command_type & NO_AUTO_FORMAT) && (d->html->flags & HTML_LINKS) && !(d->html->flags & HTML_INPUT) && !html && (!strncasecmp(text,"ftp.",4) || !strncasecmp(text,"ftp:",4))) {
                           char  buffer[BUFFER_LEN];
                           char  link[TEXT_SIZE];
                           int   count = 0;
                           const char *ptr;
			   char  *linkdest;

			   for(ptr = (char *) text, linkdest = link; *ptr && (count < KB) && (*ptr != ' ') && (*ptr != '\n') && isprint(*ptr); *linkdest++ = (isupper(*ptr) ? tolower(*ptr):*ptr), ptr++, count++);
                           for(*(linkdest--) = '\0'; (linkdest >= link) && ispunct(*linkdest) && (*linkdest != '/'); *(linkdest--) = '\0', count--);

                           if(!Blank(link)) {
                              snprintf(buffer,sizeof(buffer),"<A HREF=\"%s%s\" TARGET=_blank>%s</A>",!strncasecmp(link,"ftp:",4) ? "":"ftp://",link,link);
                              if((*length + count) <= limit) {

                                 /* ---->  Insert formatted link  <---- */
                                 for(ptr = buffer; *ptr; *dest++ = *ptr++, (*length)++);
                                 text += count;
			      } else overflow = 1;
			   }
    			} else if((*length + 1) <= limit) {
                           d->html->txtflags &= ~HTML_NEWLINE;
                           *dest++ = *text++, (*length)++;
			} else overflow = 1;
                        break;
#endif
                   case 'h':
                   case 'H':

                        /* ---->  'http://www.domain' link  <---- */
                        if(autoformat && !(command_type & NO_AUTO_FORMAT) && (d->html->flags & HTML_LINKS) && !(d->html->flags & HTML_INPUT) && !html && !strncasecmp(text,"http:",5)) {
                           char  buffer[BUFFER_LEN];
                           char  link[TEXT_SIZE];
                           int   count = 0;
                           const char *ptr;
			   char  *linkdest;

			   for(ptr = (char *) text, linkdest = link; *ptr && (count < KB) && (*ptr != ' ') && (*ptr != '\n') && isprint(*ptr); *linkdest++ = (isupper(*ptr) ? tolower(*ptr):*ptr), ptr++, count++);
                           for(*(linkdest--) = '\0'; (linkdest >= link) && ispunct(*linkdest) && (*linkdest != '/') && (*linkdest != '&'); *(linkdest--) = '\0', count--);

                           if(!Blank(link)) {
                              snprintf(buffer,sizeof(buffer),"<A HREF=\"%s\" TARGET=_blank>%s</A>",link,link);
                              if((*length + count) <= limit) {

                                 /* ---->  Insert formatted link  <---- */
                                 for(ptr = buffer; *ptr; *dest++ = *ptr++, (*length)++);
                                 text += count;
			      } else overflow = 1;
			   }
    			} else if((*length + 1) <= limit) {
                           d->html->txtflags &= ~HTML_NEWLINE;
                           *dest++ = *text++, (*length)++;
			} else overflow = 1;
                        break;
#ifdef HTML_MAIL_LINK
                   case 'm':
                   case 'M':

                        /* ---->  'mailto:user@domain' link  <---- */
                        if(autoformat && !(command_type & NO_AUTO_FORMAT) && (d->html->flags & HTML_LINKS) && !(d->html->flags & HTML_INPUT) && !html && !strncasecmp(text,"mailto:",7)) {
                           char  buffer[BUFFER_LEN];
                           int   count = 0,at = 0;
                           char  link[TEXT_SIZE];
                           const char *ptr;
			   char  *linkdest;

			   for(ptr = (char *) text, linkdest = link; *ptr && (count < KB) && (*ptr != ' ') && (*ptr != '\n') && isprint(*ptr); *linkdest++ = (isupper(*ptr) ? tolower(*ptr):*ptr), ptr++, count++)
                               if(*ptr == '@') at = 1;
                           for(*(linkdest--) = '\0'; (linkdest >= link) && ispunct(*linkdest); *(linkdest--) = '\0', count--);

                           if(!Blank(link) && at) {
                              snprintf(buffer,sizeof(buffer),"<A HREF=\"%s\">%s</A>",link,link);
                              if((*length + count) <= limit) {

                                 /* ---->  Insert formatted link  <---- */
                                 for(ptr = buffer; *ptr; *dest++ = *ptr++, (*length)++);
                                 text += count;
			      } else overflow = 1;
			   }
    			} else if((*length + 1) <= limit) {
                           d->html->txtflags &= ~HTML_NEWLINE;
                           *dest++ = *text++, (*length)++;
			} else overflow = 1;
                        break;
#endif
#ifdef HTML_TELNET_LINK
                   case 't':
                   case 'T':

                        /* ---->  'telnet://domain' link  <---- */
                        if(autoformat && !(command_type & NO_AUTO_FORMAT) && (d->html->flags & HTML_LINKS) && !(d->html->flags & HTML_INPUT) && !html && !strncasecmp(text,"telnet:",7)) {
                           char  buffer[BUFFER_LEN];
                           char  link[TEXT_SIZE];
                           int   count = 0;
                           const char *ptr;
			   char  *linkdest;

			   for(ptr = (char *) text, linkdest = link; *ptr && (count < KB) && (*ptr != ' ') && (*ptr != '\n') && isprint(*ptr); *linkdest++ = (isupper(*ptr) ? tolower(*ptr):*ptr), ptr++, count++);
                           for(*(linkdest--) = '\0'; (linkdest >= link) && ispunct(*linkdest); *(linkdest--) = '\0', count--);

                           if(!Blank(link)) {
                              snprintf(buffer,sizeof(buffer),"<A HREF=\"%s\">%s</A>",link,link);
                              if((*length + count) <= limit) {

                                 /* ---->  Insert formatted link  <---- */
                                 for(ptr = buffer; *ptr; *dest++ = *ptr++, (*length)++);
                                 text += count;
			      } else overflow = 1;
			   }
    			} else if((*length + 1) <= limit) {
                           d->html->txtflags &= ~HTML_NEWLINE;
                           *dest++ = *text++, (*length)++;
			} else overflow = 1;
                        break;
#endif
                   case 'w':
                   case 'W':

                        /* ---->  'www.domain' link  <---- */
                        if(autoformat && !(command_type & NO_AUTO_FORMAT) && (d->html->flags & HTML_LINKS) && !(d->html->flags & HTML_INPUT) && !html && !strncasecmp(text,"www.",4)) {
                           char  buffer[BUFFER_LEN];
                           char  link[TEXT_SIZE];
                           int   count = 0;
                           const char *ptr;
			   char  *linkdest;

			   for(ptr = (char *) text, linkdest = link; *ptr && (count < KB) && (*ptr != ' ') && (*ptr != '\n') && isprint(*ptr); *linkdest++ = (isupper(*ptr) ? tolower(*ptr):*ptr), ptr++, count++);
                           for(*(linkdest--) = '\0'; (linkdest >= link) && ispunct(*linkdest) && (*linkdest != '/') && (*linkdest != '&'); *(linkdest--) = '\0', count--);

                           if(!Blank(link)) {
                              snprintf(buffer,sizeof(buffer),"<A HREF=\"http://%s\" TARGET=_blank>%s</A>",link,link);
                              if((*length + count) <= limit) {

                                 /* ---->  Insert formatted link  <---- */
                                 for(ptr = buffer; *ptr; *dest++ = *ptr++, (*length)++);
                                 text += count;
			      } else overflow = 1;
			   }
    			} else if((*length + 1) <= limit) {
                           d->html->txtflags &= ~HTML_NEWLINE;
                           *dest++ = *text++, (*length)++;
			} else overflow = 1;
                        break;
                   default:
                        if(isprint(*text)) {
                           if((*length + 1) <= limit) {
                              d->html->txtflags &= ~HTML_NEWLINE;
                              *dest++ = *text++, (*length)++;
	  		   } else overflow = 1;
			} else text++;
	    }
      }
      *dest = '\0';
      return(buffer);
}

/* ---->  Close unterminated HTML tags  <---- */
const char *text_to_html_close_tags(struct descriptor_data *d,char *buffer,int *length,int limit)
{
      struct   html_tag_data *next;
      unsigned char overflow = 0;
      char     *dest = buffer;

      *buffer = '\0', *length = 0;
      for(; !overflow && d->html->tag; d->html->tag = next) {
          next = d->html->tag->next;
          if(!overflow && ((*length + (tags[d->html->tag->ptr].len + 3)) <= limit)) {
             if(tags[d->html->tag->ptr].closetag && (tags[d->html->tag->ptr].len > 0)) {
                sprintf(dest,"</%s>",tags[d->html->tag->ptr].tag);
                *length += (tags[d->html->tag->ptr].len + 3);
                dest    += (tags[d->html->tag->ptr].len + 3);
	     }
             FREENULL(d->html->tag);
	  }
      }
      *dest = '\0';
      return(buffer);
}

/* ---->  Reset unterminated HTML tags generated by text_to_html()  <---- */
const char *text_to_html_reset(struct descriptor_data *d,char *buffer,int *length)
{
      unsigned short exception,tagtype;
      struct   html_tag_data *new;
      char     *dest = buffer;
      short    tag;

      /* ---->  Close open HTML tag  <---- */
      *buffer = '\0', *length = 0;
      if(d->html->txtflags & HTML_TAG)
	 *dest++ = '>', (*length)++, d->html->txtflags &= ~HTML_TAG;

      /* ---->  Reset underline  <---- */
      if(d->html->txtflags & TXT_UNDERLINE) {
         if((command_type & LARGE_SUBSTITUTION) && ((tag = html_search_tags("U",1,&exception,&tagtype)) != NOTHING)) {
            MALLOC(new,struct html_tag_data);
            new->next    = d->html->tag;
            new->ptr     = tag;
            d->html->tag = new;
	 } else {
            strcpy(dest,(IsHtml(d) && (d->html->flags & HTML_UNDERLINE)) ? "</U>":"</I>");
            dest += 4, *length += 4, d->html->txtflags &= ~TXT_UNDERLINE;
	 }
      }

      /* ---->  Reset flashing  <---- */
      if(d->html->txtflags & TXT_BLINK) {
         if((command_type & LARGE_SUBSTITUTION) && ((tag = html_search_tags("BLINK",5,&exception,&tagtype)) != NOTHING)) {
            MALLOC(new,struct html_tag_data);
            new->next    = d->html->tag;
            new->ptr     = tag;
            d->html->tag = new;
	 } else {
            strcpy(dest,"</BLINK>");
            dest += 8, *length += 8, d->html->txtflags &= ~TXT_BLINK;
	 }
      }

      /* ---->  Reset bold  <---- */
      if(!(d->html->flags & HTML_SIMULATE_ANSI) && (d->html->txtflags & TXT_BOLD)) {
         if((command_type & LARGE_SUBSTITUTION) && ((tag = html_search_tags("B",1,&exception,&tagtype)) != NOTHING)) {
            MALLOC(new,struct html_tag_data);
            new->next    = d->html->tag;
            new->ptr     = tag;
            d->html->tag = new;
	 } else {
            strcpy(dest,"</B>");
            dest += 4, *length += 4, d->html->txtflags &= ~TXT_BOLD;
	 }
      }

      /* ---->  Reset text colour  <---- */
      if(d->html->txtflags & HTML_FONT) {
         if((command_type & LARGE_SUBSTITUTION) && ((tag = html_search_tags("FONT",4,&exception,&tagtype)) != NOTHING)) {
            MALLOC(new,struct html_tag_data);
            new->next    = d->html->tag;
            new->ptr     = tag;
            d->html->tag = new;
	 } else {
            strcpy(dest,"</FONT>");
            dest += 7, *length += 7, d->html->txtflags &= ~(HTML_FONT|TXT_FG_MASK);
	 }
      }

      /* ---->  Reset text background colour  <---- */
      if(d->html->txtflags & HTML_SPAN_BACKGROUND) {
         strcpy(dest,"</SPAN>");
         dest += 7, *length += 7;
         d->html->txtflags &= ~(HTML_SPAN_BACKGROUND|TXT_BG_MASK);
      }

      /* ---->  New line  <---- */
      if(add_cr == 1) {
         strcpy(dest,"<BR>");
         dest += 4, *length += 4, d->html->txtflags |= HTML_NEWLINE;
      }

      if(!(command_type & LARGE_SUBSTITUTION))
         d->html->txtflags &= ~HTML_PREFORMAT;
      *dest = '\0';
      return(buffer);
}

/* ---->  Convert text to URL encoded format  <---- */
const char *html_encode(const char *text,char *buffer,int *copied,int limit)
{
      unsigned char overflow = 0;
      char     *dest = buffer;
      int      length = 0;

      *buffer = '\0';
      if(copied) *copied = 0;
      if(!text) return(buffer);
      while(*text && !overflow) {
            if(*text == ' ') {

               /* ---->  Blank space  <---- */
               *dest++ = '+';
               if(copied) (*copied)++;
	    } else if(isalnum(*text)) {

               /* ---->  Alphanumeric text  <---- */
               *dest++ = *text;
               if(copied) (*copied)++;
	    } else if((length + 3) <= limit) {

               /* ---->  Non-alphanumeric text (Encode)  <---- */
               sprintf(dest,"%%%02X",*text);
               dest += 3, length += 3;
               if(copied) (*copied) += 3;
	    } else overflow = 1;
            text++;
      }
      *dest = '\0';
      return(buffer);      
}

/* ---->  Convert text to basic encoded format ('<', '>', '&', ';' and '"' protected)  <---- */
const char *html_encode_basic(const char *text,char *buffer,int *copied,int limit)
{
      unsigned char overflow = 0;
      char     *dest = buffer;
      int      length = 0;

      *buffer = '\0';
      if(copied) *copied = 0;
      if(!text) return(buffer);
      while(*text && !overflow)
            switch(*text) {
                   case '\"':
                        if((length + 6) <= limit) {
                           strcpy(dest,"&quot;"), text++;
                           dest += 6, length += 6;
                           if(copied) (*copied) += 6;
			} else overflow = 1;
                        break;
                   case '<':
                        if((length + 4) <= limit) {
                           strcpy(dest,"&lt;"), text++;
                           dest += 4, length += 4;
                           if(copied) (*copied) += 4;
			} else overflow = 1;
                        break;
                   case '>':
                        if((length + 4) <= limit) {
                           strcpy(dest,"&gt;"), text++;
                           dest += 4, length += 4;
                           if(copied) (*copied) += 4;
			} else overflow = 1;
                        break;
                   case '&':
                        if((length + 5) <= limit) {
                           strcpy(dest,"&amp;"), text++;
                           dest += 5, length += 5;
                           if(copied) (*copied) += 5;
			} else overflow = 1;
                        break;
                   case ';':
                        if((length + 5) <= limit) {
                           strcpy(dest,"&#59;"), text++;
                           dest += 5, length += 5;
                           if(copied) (*copied) += 5;
			} else overflow = 1;
                        break;
                   default:
                        *dest++ = *text++, length++;
                        if(copied) (*copied)++;
	    }
      *dest = '\0';
      return(buffer);      
}

/* ---->  Produce copy of HTML parameters in encoded format  <---- */
const char *html_copy_params(char *buffer,int limit)
{
      struct html_param_data *current;
      int    length = 0,copied;
      char   *dest = buffer;

      for(current = params; current && (length < limit); current = current->next)
          if(!Blank(current->name) && strcasecmp(current->name,"DATA")) {
             html_encode(current->name,dest,&copied,limit - length), dest += copied;
             if(((length += copied) + 1) < limit) {
                *dest++ = '=', length++;
                html_encode(current->contents,dest,&copied,limit - length), dest += copied;
                if(((length += copied) + 1) < limit)
                   *dest++ = '&', length++;
	     }
	  }
      *dest = '\0';
      return(buffer);
}

/* ---->  Return character's preferences identifier code  <---- */
const char *html_get_preferences(struct descriptor_data *d)
{
      static char buffer[16];

      if(IsHtml(d)) sprintf(buffer,"ID=%lX&",d->html->identifier);
         else *buffer = '\0';
      return(buffer);
}

/* ---->  Set character's HTML preferences using preferences identifier code  <---- */
unsigned char html_set_preferences(struct descriptor_data *d,const char *prefs)
{
	 if(IsHtml(d) && !Blank(prefs)) {
	    unsigned long identifier = tohex(prefs);
	    struct   descriptor_data *p;

	    for(p = descriptor_list; p; p = p->next)
		if((p != d) && p->html && (p->html->identifier == identifier)) {
		   d->html->flags &= ~HTML_PREFERENCE_FLAGS;
		   d->html->flags |=  (p->html->flags & HTML_PREFERENCE_FLAGS);
		   FREENULL(d->html->background);
		   d->html->background = (char *) alloc_string(p->html->background);
		   d->html->identifier = p->html->identifier;
		   return(1);
		}
	 }
	 return(0);
}

/* ---->  Recall saved preferences  <---- */
void html_recall_preferences(dbref player,struct descriptor_data *d)
{
     if(!d || !d->html) return;
     if(Validchar(player)) {
        d->html->flags     &= ~HTML_PREFERENCE_FLAGS;
        d->html->flags     |= (db[player].data->player.htmlflags & HTML_PREFERENCE_FLAGS);
        d->html->cmdwidth   = db[player].data->player.htmlcmdwidth;
        d->html->background = (char *) alloc_string(db[player].data->player.htmlbackground);
     } else {
        d->html->cmdwidth = HTML_CMDWIDTH;
        d->html->flags   &= ~HTML_PREFERENCE_FLAGS;
        d->html->flags   |=  HTML_DEFAULT_FLAGS;
        FREENULL(d->html->background);
     }
}

/* ---->  Save current preferences  <---- */
void html_store_preferences(dbref player,struct descriptor_data *d)
{
     if(!d || !IsHtml(d) || !Validchar(player)) return;
     db[player].data->player.htmlcmdwidth = d->html->cmdwidth;
     db[player].data->player.htmlflags    = 0;
     db[player].data->player.htmlflags   |= (d->html->flags & HTML_PREFERENCE_FLAGS);
     FREENULL(db[player].data->player.htmlbackground);
     if(d->html->background && strcasecmp(d->html->background,html_image_url("default.gif")))
        db[player].data->player.htmlbackground = (char *) alloc_string(d->html->background);
}

/* ---->  {J.P.Boggis 12/02/2000}  Generate form to set preferences  <---- */
void html_preferences_form(struct descriptor_data *d,unsigned char saved,unsigned char set)
{
     const    char *data = NULL;
     unsigned char defaults = 1;
     int           checked;

     if(!d || !d->html) return;
     if(params && params->next)
        defaults = 0;

     output(d,NOTHING,1,0,0,"<TABLE BORDER WIDTH=100%% CELLPADDING=5>");
     output(d,NOTHING,1,0,0,"<TR><TH ALIGN=CENTER BGCOLOR="HTML_DGREEN"><FONT SIZE=+2><B><U><I>World Wide Web Interface Preferences</I></U></B></FONT></TH></TR>");
     if(set) output(d,NOTHING,1,0,0,"<TR><TD BACKGROUND=\"%s\"><TABLE WIDTH=100%% CELLPADDING=10>",html_image_url((d->html->flags & HTML_WHITE_AS_BLACK) ? "background.gif":"default.gif"));
        else output(d,NOTHING,1,0,0,"<TR><TD><TABLE WIDTH=100%% CELLPADDING=10>");

     if(saved) {
        if(!set) checked = (defaults || html_lookup("SAVED",0,0,0,0,1));
           else checked = 1;
        output(d,NOTHING,1,0,0,"<TR VALIGN=TOP><TD WIDTH=25><INPUT NAME=SAVED TYPE=CHECKBOX%s></TD>",(checked) ? " CHECKED":"");
        output(d,NOTHING,1,0,0,"<TD><B>Use your saved preferences.</B><P><I>If this box is checked, your previously saved preferences will be used when you connect.<BR><B>Uncheck this box if you wish to make any changes to the options below.</B></I></TD></TR>");
     }

#ifdef SSL_SOCKETS

     /* ---->  SSL (Secure Sockets Layer) connection  <---- */
     if(option_ssl(OPTSTATUS)) {
        if(!set) checked = (html_lookup("SSL",0,0,0,0,1) != NULL);
           else checked = ((d->html->flags & HTML_SSL) != 0);
        output(d,NOTHING,1,0,0,"<TR VALIGN=TOP><TD WIDTH=25><INPUT NAME=SSL TYPE=CHECKBOX%s></TD>",(checked) ? " CHECKED":"");
        output(d,NOTHING,1,0,0,"<TD><B>Enable SSL (Secure Sockets Layer) connections:</B><P>");
        output(d,NOTHING,1,0,0,"<I>Allows secure encrypted connections (Providing a suitable web browser is used), ensuring complete confidentiality and security of information sent to/received from %s. &nbsp; Ideal safeguard against 'packet sniffing' on your network.</I><P><FONT COLOR="HTML_DRED"><B><U>IMPORTANT:</U></B></FONT> &nbsp; <I><B>Use of encryption software may be illegal in some countries.</B></I><P></TD></TR>",tcz_short_name);
     }
#endif

     /* ---->  JAVA Script enhancements  <---- */
     if(!set) checked = (html_lookup("JAVA",0,0,0,0,1) != NULL);
        else checked = ((d->html->flags & HTML_JAVA) != 0);
     output(d,NOTHING,1,0,0,"<TR VALIGN=TOP><TD WIDTH=25><INPUT NAME=JAVA TYPE=CHECKBOX%s></TD>",(checked) ? " CHECKED":"");
     output(d,NOTHING,1,0,0,"<TD><B>Enable JAVA Script enhancements:</B><P><I>Requires up-to-date web browser with <B>JAVA Script</B> enabled.  Click on the link below to test whether your browser is JAVA-enabled or not (You will see a pop-up message if it is.)  JAVA Script improves user-friendliness of the World Wide Web Interface.</I><P>");
     if(!set) checked = (html_lookup("SCROLLBY",0,0,0,0,1) != NULL);
        else checked = ((d->html->flags & HTML_SCROLLBY) != 0);
     output(d,NOTHING,1,0,0,"<INPUT NAME=SCROLLBY TYPE=CHECKBOX%s> &nbsp; <B>Use new scrolling method <I>(Requires JAVA Script 1.2)</I><BR>",(checked) ? " CHECKED":"");
     if(!set) checked = (html_lookup("SCROLL",0,0,0,0,1) != NULL);
        else checked = ((d->html->flags & HTML_SCROLL) != 0);
     output(d,NOTHING,1,0,0,"<INPUT NAME=SCROLL TYPE=CHECKBOX%s> &nbsp; <B>Use old scrolling method <I>(Recommended for older browsers.)</I><BR>",(checked) ? " CHECKED":"");
     if(!set) checked = (html_lookup("FOCUS",0,0,0,0,1) != NULL);
        else checked = ((d->html->flags & HTML_FOCUS) != 0);
     output(d,NOTHING,1,0,0,"<INPUT NAME=FOCUS TYPE=CHECKBOX%s> &nbsp; <B>Automatically set focus to '"HTML_TCZ_PROMPT"' box.<P>",(checked) ? " CHECKED":"",tcz_short_name);
     output(d,NOTHING,1,0,0,"<SCRIPT LANGUAGE=\"JavaScript\">\n<!--\n function TestJava() { alert(\"Your browser supports JAVA Script.\"); }\n//-->\n</SCRIPT>");
     output(d,NOTHING,1,0,0,"<CENTER><A HREF=\"JavaScript:TestJava();\"><B><I>Click HERE to see if your browser supports JAVA Script...</I></B></A></CENTER></TD></TR>");

     /* ---->  Cascading Style-Sheets enhancements  <----- */
     if(!set) checked = (html_lookup("STYLE",0,0,0,0,1) != NULL);
        else checked = ((d->html->flags & HTML_STYLE) != 0);
     output(d,NOTHING,1,0,0,"<TR VALIGN=TOP><TD WIDTH=25><INPUT NAME=STYLE TYPE=CHECKBOX%s></TD>",(checked) ? " CHECKED":"");
     output(d,NOTHING,1,0,0,"<TD><B>Enable Cascading Style-Sheets enhancements:</B><P><I>Requires modern web browser with full support for <B>style sheets</B> (Latest versions of <A HREF=\"http://www.netscape.com\" TARGET=_blank>Netscape</A> and <A HREF=\"http://www.microsoft.com\" TARGET=_blank>Microsoft Internet Explorer</A>.)  Enhances output, table layout and enables text background colours.</I><P><FONT COLOR="HTML_LYELLOW" STYLE=\"background:"HTML_LRED";\"><B>This text will be yellow on a red background if your browser supports style sheets.</B></FONT></TD></TR>");

     /* ---->  Automatically formatted links  <----- */
     if(!set) checked = (defaults || html_lookup("LINKS",0,0,0,0,1));
        else checked = ((d->html->flags & HTML_LINKS) != 0);
     output(d,NOTHING,1,0,0,"<TR VALIGN=TOP><TD WIDTH=25><INPUT NAME=LINKS TYPE=CHECKBOX%s></TD>",(checked) ? " CHECKED":"");
     output(d,NOTHING,1,0,0,"<TD><B>Enable automatically formatted links:</B><P><I>Automatically turns links into proper HTML hyperlinks (I.e:  </I><A HREF=\"http://www.sourceforge.net/projects/tcz\" TARGET=_blank>www.sourceforge.net/projects/tcz</A></I> instead of </I><B>www.sourceforge.net/projects/tcz</B><I>)</I></TD></TR>");

     /* ---->  Graphical Smileys (Emoticons)  <----- */
     if(!set) checked = (defaults || html_lookup("SMILEY",0,0,0,0,1));
        else checked = ((d->html->flags & HTML_SMILEY) != 0);
     output(d,NOTHING,1,0,0,"<TR VALIGN=TOP><TD WIDTH=25><INPUT NAME=SMILEY TYPE=CHECKBOX%s></TD>",(checked) ? " CHECKED":"");
     output(d,NOTHING,1,0,0,"<TD><B>Enable graphical smileys (Emoticons):</B><P><I>Replaces text smileys (Emoticons) used to express emotions by users (E.g: &nbsp; </I><B><FONT COLOR="HTML_DMAGENTA">:-)</FONT></B> <I>) with graphical icon equivanlents (E.g: &nbsp; <IMG SRC=\"%s\" ALT=\":-)\">.)</I></TD></TR>",html_image_url("smiley.happy.gif"));

     /* ---->  Coloured text (Simulated ANSI colour)  <----- */
     if(!set) checked = (defaults || html_lookup("COLOUR",0,0,0,0,1));
        else checked = ((d->html->flags & HTML_SIMULATE_ANSI) != 0);
     output(d,NOTHING,1,0,0,"<TR VALIGN=TOP><TD WIDTH=25><INPUT NAME=COLOUR TYPE=CHECKBOX%s></TD><TD><B>Enable coloured text (<A HREF=\"%sTOPIC=ANSI&\" TARGET=_blank TITLE=\"Click to find out more about %s's ANSI colour support...\">ANSI colour</A>):</B><P>",(checked) ? " CHECKED":"",html_server_url(d,0,1,"help"),tcz_short_name);
     output(d,NOTHING,1,0,0,"<I>Requires a recent web browser with support for <B>font colour tags</B>.  Enhances readability of output by fully simulating ANSI colour (As used on text-only Telnet connections.)</I><P>");
     output(d,NOTHING,1,0,0,"<FONT STYLE=\"background:"HTML_DBLACK";\"><B><FONT COLOR="HTML_LRED">This</FONT> <FONT COLOR="HTML_LGREEN">text</FONT> <FONT COLOR="HTML_LYELLOW">will</FONT> <FONT COLOR="HTML_LBLUE">be</FONT> <FONT COLOR="HTML_LMAGENTA">multicoloured</FONT> <FONT COLOR="HTML_LCYAN">if</FONT> <FONT COLOR="HTML_LRED">your</FONT> <FONT COLOR="HTML_LGREEN">browser</FONT> <FONT COLOR="HTML_LYELLOW">supports</FONT> <FONT COLOR="HTML_LBLUE">font</FONT> <FONT COLOR="HTML_LMAGENTA">colours.</FONT></B></FONT></TD></TR>");

     /* ---->  Italic instead of underlined text  <----- */
     if(!set) checked = (html_lookup("ITALIC",0,0,0,0,1) != NULL);
        else checked = ((d->html->flags & HTML_UNDERLINE) == 0);
     output(d,NOTHING,1,0,0,"<TR VALIGN=TOP><TD WIDTH=25><INPUT NAME=ITALIC TYPE=CHECKBOX%s></TD>",(checked) ? " CHECKED":"");
     output(d,NOTHING,1,0,0,"<TD><B><I>Italic</I> text instead of <U>underlined</U> text:</B><P><I>Some browsers do not support <B><U>underlined</U></B> text.  If this is the case with your browser, enable this option.</I><P><FONT COLOR="HTML_DBLUE"><U><B>This text will be underlined if your browser supports underlined text.</B></U></TD></TR></FONT>");

     /* ---->  Background image  <----- */
     if(!set) checked = (html_lookup("BACKGROUND",0,0,0,0,1) != NULL);
        else checked = ((d->html->flags & HTML_BGRND) != 0);
     output(d,NOTHING,1,0,0,"<TR VALIGN=TOP><TD WIDTH=25><INPUT NAME=BACKGROUND TYPE=CHECKBOX%s></TD>",(checked) ? " CHECKED":"");
     output(d,NOTHING,1,0,0,"<TD><B>Use alternative background image:</B><P>");

     if(set) {
        if(!Blank(d->html->background)) {
           checked = 1;
           data    = decompress(d->html->background);
	} else checked = 0;
     } else checked = (!params || (data = html_lookup("IMAGE",0,0,0,0,1)));
     output(d,NOTHING,1,0,0,"<INPUT NAME=IMAGE TYPE=TEXT SIZE=45 MAXLENGTH=128 VALUE=\"%s\"><P>",(checked) ? String(data):"");

     /* ---->  Reverse foreground and background colours  <----- */
     if(!set) checked = (html_lookup("REVERSE",0,0,0,0,1) != NULL);
        else checked = ((d->html->flags & HTML_WHITE_AS_BLACK) != 0);
     output(d,NOTHING,1,0,0,"<INPUT NAME=REVERSE TYPE=CHECKBOX%s> &nbsp; <B>Reverse foreground and background text colours.</B><P>",(checked) ? " CHECKED":"");
     output(d,NOTHING,1,0,0,"<FONT COLOR="HTML_DBLACK" STYLE=\"background:"HTML_LWHITE";\"><B>Black text on white background</B></FONT> &nbsp; <I>instead of</I> &nbsp; <FONT COLOR="HTML_LWHITE" STYLE=\"background:"HTML_DBLACK";\"><B>White text on black background.</B></FONT></TD></TR>");

     /* ---->  Command input box width  <---- */
     if(!set) {
        const char *cmdwidth = html_lookup("CMDWIDTH",0,0,0,0,1);

        if(!Blank(cmdwidth)) checked = atoi(cmdwidth);
           else checked = HTML_CMDWIDTH;
     } else checked = d->html->cmdwidth;
     output(d,NOTHING,1,0,0,"<TR VALIGN=TOP><TD WIDTH=25><INPUT NAME=CMDWIDTH TYPE=TEXT SIZE=3 MAXLENGTH=3 VALUE=\"%d\"></TD>",checked);
     output(d,NOTHING,1,0,0,"<TD><B>Width of '"HTML_TCZ_PROMPT"' input box:</B><P><I>If you have a high-resolution display, you may like to increase the width of the '<B>"HTML_TCZ_PROMPT"</B>' input box from the default of %d characters to take full advantage of it.%s%s%s</I><P></TD></TR>",tcz_short_name,tcz_short_name,HTML_CMDWIDTH,(set) ? "<P><B></I>NOTE:<I> &nbsp; Change will not take place until you send your next command to ":"",(set) ? tcz_short_name:"",(set) ? ".</B>":"");

     /* ---->  Table closing tags  <---- */
     output(d,NOTHING,1,0,0,"</TABLE></TD></TR></TABLE>");
}

/* ---->  {J.P.Boggis 16/02/2000}  Generate JAVA Script to set default JAVA options on preferences form  <---- */
void html_preferences_form_java(struct descriptor_data *d)
{
     output(d,NOTHING,1,0,0,"<SCRIPT LANGUAGE=\"JavaScript\">\n<!--\n// Default options for early JAVA enabled browsers\ndocument.OPTIONS.JAVA.checked = true; document.OPTIONS.SCROLLBY.checked = false; document.OPTIONS.SCROLL.checked = false; document.OPTIONS.FOCUS.checked = true; document.OPTIONS.COLOUR.checked = true; document.OPTIONS.STYLE.checked = false; document.OPTIONS.ITALIC.checked = false;\n//-->\n</SCRIPT>");
     output(d,NOTHING,1,0,0,"<SCRIPT LANGUAGE=\"JavaScript1.1\">\n<!--\n// Default options for JavaScript 1.1\nif(navigator.javaEnabled()) { document.OPTIONS.JAVA.checked = true; document.OPTIONS.SCROLLBY.checked = false; document.OPTIONS.SCROLL.checked = true; document.OPTIONS.FOCUS.checked = true; document.OPTIONS.COLOUR.checked = true; document.OPTIONS.STYLE.checked = false; document.OPTIONS.ITALIC.checked = false; }\n//-->\n</SCRIPT>");
     output(d,NOTHING,1,0,0,"<SCRIPT LANGUAGE=\"JavaScript1.2\">\n<!--\n// Default options for JavaScript 1.2\nif(navigator.javaEnabled()) { document.OPTIONS.JAVA.checked = true; document.OPTIONS.SCROLLBY.checked = true; document.OPTIONS.SCROLL.checked = false; document.OPTIONS.FOCUS.checked = true; document.OPTIONS.COLOUR.checked = true; document.OPTIONS.STYLE.checked = true; document.OPTIONS.ITALIC.checked = false; }\n//-->\n</SCRIPT>");
}

/* ---->  Convert text in HTML format to standard text  <---- */
const char *html_to_text(struct descriptor_data *d,char *html,char *buffer,int limit)
{
      unsigned char overflow = 0,htmleval = 0,preformat = 0;
      struct   html_entity_data *ent;
      int      length = 0,count,temp;
      char     *dest = buffer;
      const    char *start;

      if(!html) {
         *buffer = '\0';
         return(buffer);
      }

      /* ---->  Skip open HTML tag  <---- */
      if(d->flags2 & HTML_TAG) {
         if(*html && (*html == '\016')) htmleval = 1;
         while(*html && (*html != '>')) html++;
         if(*html && (*html == '>')) {
            d->flags2 &= ~HTML_TAG;
            *html++ = '\0';
	 }
         if(!*html) return(NULL);
      }

      while(*html && !overflow)
            switch(*html) {
                   case '\x0E':

                        /* ---->  Toggle evaluation of HTML tags  <---- */
                        htmleval = !htmleval;
                        html++;
                        break;
                   case ' ':

                        /* ---->  Blank space  <---- */
                        if((length + 1) <= limit)
                           *dest++ = ' ', length++;
                              else overflow = 1;
                        if(htmleval && !preformat) {
                           while(*html && (*html == ' ')) html++;
			} else html++;
                        break;
                   case '\n':

                        /* ---->  NEWLINE  <---- */
                        if(htmleval && !preformat) {
                           if((length + 1) <= limit)
                              *dest++ = ' ', length++;
   	                         else overflow = 1;
                           while(*html && (*html == '\n')) html++;
			} else {
                           if((length + 1) <= limit)
                              *dest++ = '\n', length++;
   	                         else overflow = 1;
                           html++;
			}
                        break;
                   case '<':

                        /* ---->  HTML tag  <---- */
                        if(htmleval) {
                           start = html + 1;
                           d->flags2 |= HTML_TAG;
                           while(*html && (*html != '>')) html++;
                           if(*html && (*html == '>')) {
                              d->flags2 &= ~HTML_TAG;
                              *html++ = '\0';
			   }

                           if(*start)
                              switch(*start) {
                                     case 'b':
                                     case 'B':

                                          /* ---->  <BR> tag ('\n')  <---- */
                                          if(((*(start + 1) == 'r') || (*(start + 1) == 'R')) && !*(start + 2)) {
                                             if((length + 1) <= limit)
                                                *dest++ = '\n', length++;
   	                                 	   else overflow = 1;
					  }
                                          break;
                                     case 'i':
                                     case 'I':

                                          /* ---->  <IMG SRC="..." ALT="..."> tag (Get 'ALT' description)  <---- */
                                          if(((*(start + 1) == 'm') || (*(start + 1) == 'M')) && !strncasecmp(start + 1,"MG ",3)) {
					     const char *altstart = "";
  					     const char *slash = "";
					     char  *ptr;

					     start += 3;

                                             /* ---->  Find 'ALT'  <---- */
                                             for(ptr = (char *) start; *ptr && strncasecmp(ptr," ALT=",5); ptr++);
                                             if(*ptr && !strncasecmp(ptr," ALT=",5) && (strlen(ptr) > 5)) {
                                                ptr += 5;
                                                if(*ptr == '\"') {
						   for(altstart = (++ptr); *ptr && (*ptr != '\"'); ptr++);
						} else for(altstart = ptr; *ptr && (*ptr != ' '); ptr++);
					        if(*ptr) *ptr = '\0';
					     }

					     /* ---->  Find 'SRC' (No 'ALT')  <---- */
                                             if(Blank(altstart)) {
						for(ptr = (char *) start; *ptr && strncasecmp(ptr," SRC=",5); ptr++);
						if(*ptr && !strncasecmp(ptr," SRC=",5) && (strlen(ptr) > 5)) {
                                                   int quotes = 0;

						   ptr += 5;
						   if(*ptr == '\"') ptr++, quotes = 1;
						   for(slash = ptr; *ptr && !(!quotes && (*ptr == ' ')) && !(quotes && (*ptr == '\"')); ptr++)
						       if(*ptr == '/') slash = (ptr + 1);
						   if(*ptr) *ptr = '\0';
                                                   if(!Blank(slash)) altstart = slash;
						}
					     }

					     /* ---->  Insert text description of image  <---- */
                                             if(Blank(altstart)) altstart = "[Image]";
     				             if(!Blank(altstart)) {
                                                if(!Blank(slash)) {
                                                   if((length + 1) <= limit)
                                                      *dest++ = '[', length++;
     	                                 	         else overflow = 1;
						}

                                                if(!overflow) {
                                                   if((length + strlen(altstart)) <= limit) {
  					              for(; *altstart; *dest++ = *altstart++, length++);
						   } else overflow = 1;

						   if(!overflow) {
                                                      if(!Blank(slash)) {
                                                         if((length + 1) <= limit)
                                                            *dest++ = ']', length++;
   	                                 	               else overflow = 1;
						      }
						   }
						}
					     }
					  }
                                          break;
                                     case 'p':
                                     case 'P':
                                          if(!*(start + 1)) {

                                             /* ---->  <P> tag ('\n\n')  <---- */
                                             if((length + 2) <= limit)
                                                *dest++ = '\n', *dest++ = '\n', length += 2;
   	                                 	   else overflow = 1;
					  } else if(!strcasecmp("RE",start + 1)) {

                                             /* ---->  <PRE> tag  <---- */
                                             preformat = 1;
					  }
                                          break;
                                     case 't':
                                     case 'T':
                                          if(((*(start + 1) == 't') || (*(start + 1) == 'T')) && !*(start + 2)) {

                                             /* ---->  <TT> tag  <---- */
                                             preformat = 1;
					  }
                                          break;
                                     case '/':

                                          /* ---->  Closing tag  <---- */
                                          if(*(++start))
                                             switch(*start) {
                                                    case 'd':
                                                    case 'D':
                                                         if(((*(start + 1) == 'l') || (*(start + 1) == 'L')) && !*(start + 2)) {

                                                            /* ---->  </DL> ('\n')  <---- */
                                                            if((length + 1) <= limit)
                                                               *dest++ = '\n', length++;
   	                                             	          else overflow = 1;
							 }
                                                         break;
                                                    case 'l':
                                                    case 'L':
                                                         if(((*(start + 1) == 'i') || (*(start + 1) == 'I')) && !*(start + 2)) {

                                                            /* ---->  </LI> ('\n')  <---- */
                                                            if((length + 1) <= limit)
                                                               *dest++ = '\n', length++;
   	                                             	          else overflow = 1;
							 }
                                                         break;
                                                    case 'o':
                                                    case 'O':
                                                         if(((*(start + 1) == 'l') || (*(start + 1) == 'L')) && !*(start + 2)) {

                                                            /* ---->  </OL> ('\n')  <---- */
                                                            if((length + 1) <= limit)
                                                               *dest++ = '\n', length++;
   	                                             	          else overflow = 1;
							 }
                                                         break;
                                                    case 'p':
                                                    case 'P':
			                                 if(!strcasecmp("RE",start + 1)) {

                                                            /* ---->  </PRE> tag  <---- */
                                                            preformat = 0;
							 } 
                                                         break;
                                                    case 't':
                                                    case 'T':
                                                         if(((*(start + 1) == 'd') || (*(start + 1) == 'D') || (*(start + 1) == 'h') || (*(start + 1) == 'H')) && !*(start + 2)) {

                                                            /* ---->  </TD> and <TH> tags (' ')  <---- */
                                                            if((length + 1) <= limit)
                                                               *dest++ = ' ', length++;
   	                                             	          else overflow = 1;
							 } else if(((*(start + 1) == 'r') || (*(start + 1) == 'R')) && !*(start + 2)) {

                                                            /* ---->  </TR> tag ('\n')  <---- */
           	                                            if((length + 1) <= limit)
                                                               *dest++ = '\n', length++;
   	                                             	          else overflow = 1;
							 } else if(((*(start + 1) == 't') || (*(start + 1) == 'T')) && !*(start + 2)) {

                                                            /* ---->  </TT> tag  <---- */
                                                            preformat = 0;
							 }
                                                         break;
                                                    case 'u':
                                                    case 'U':
                                                         if(((*(start + 1) == 'l') || (*(start + 1) == 'L')) && !*(start + 2)) {

                                                            /* ---->  </UL> ('\n')  <---- */
                                                            if((length + 1) <= limit)
                                                               *dest++ = '\n', length++;
   	                                             	          else overflow = 1;
							 }
                                                         break;
					     }
                                          break;
			      }
			} else if((length + 1) <= limit)
                           *dest++ = *html++, length++;
   	          	      else overflow = 1;
                        break;
                   case '&':

                        /* ---->  HTML character entity sets  <---- */
                        if(htmleval) {
                           for(start = ++html, count = 0; *html && (*html != ';'); html++, count++);
                           if(*html && (*html == ';')) *html++ = '\0';
                           if(*start) {
                              if(*start == '#') {
                                 if(*(start + 1)) {
                                    if(((temp = atol(start + 1)) > 0) && (temp < 256)) {
                                       if((temp >= 32) && (temp <= 127)) {
                                          if((length + 1) <= limit) {
                                             *dest++ = temp;
                                             length++;
				          } else overflow = 1;
				       } else if((entities[temp] >= 0) && (entity[entities[temp]].len > 0)) {
                                          if((length + entity[entities[temp]].len) <= limit) {
                                             strcpy(dest,entity[entities[temp]].text);
                                             length += entity[entities[temp]].len;
                                             dest   += entity[entities[temp]].len;
					  } else overflow = 1;
				       }
				    }
				 }
			      } else if((ent = html_search_entities(start,count)) && (ent->len > 0)) {
                                 if((length + ent->len) <= limit) {
                                    strcpy(dest,ent->text);
                                    length += ent->len;
                                    dest   += ent->len;
				 } else overflow = 1;
			      }
			   }
			} else if((length + 1) <= limit)
                           *dest++ = *html++, length++;
   	          	      else overflow = 1;
                        break;
                   default:
                        if((length + 1) <= limit)
                           *dest++ = *html++, length++;
   	                      else overflow = 1;
	    }

      *dest = '\0';
      return(buffer);
}

/* ---->  Process input from HTML connections  <---- */
void html_process_input(struct descriptor_data *d,unsigned char *buffer,int length)
{
     unsigned char          *new,*ptr,*end,*tmp,*tmp2,*tmp3;
     static   unsigned char htmlbuffer[BUFFER_LEN];
     int                    contentlen = NOTHING;

     if(IsHtml(d) && (option_loglevel(OPTSTATUS) >= 6))
        writelog(COMMAND_LOG,1,"HTML","[%d] {RCV}  %s",d->descriptor,binary_to_ascii(buffer,length,htmlbuffer));

     if((new = (char *) realloc(d->negbuf,d->neglen + length))) {
        memcpy(new + d->neglen,buffer,length);
        d->neglen += length, d->negbuf = new;

        /* ---->  Parse information in header  <---- */
        end = new + d->neglen, ptr = new, tmp = htmlbuffer;
        do {
           if(ptr < end) {
              if(*(ptr++) == '\r') {
                 if((ptr < end) && (*ptr == '\n')) ptr++;
	      } else if(*(ptr++) == '\n') {
                 if((ptr < end) && (*ptr == '\r')) ptr++;
	      }
	   }
           for(; (ptr < end) && !((*ptr == '\r') || (*ptr == '\n')); *tmp++ = *ptr++);
           *tmp = '\0';

           if(!Blank(htmlbuffer)) {
              for(tmp = htmlbuffer; *tmp && (*tmp == ' '); tmp++);
              for(tmp2 = tmp; *tmp2 && (*tmp2 != ':'); tmp2++);
              if(*tmp2) *tmp2 = '\0';
              if(!Blank(tmp) && !strncasecmp(tmp,"Content-Length",14) && d->html) {
                 for(tmp3 = (tmp + 15); (tmp3 < end) && (*tmp3 == ' '); tmp3++);
                 if(isdigit(*tmp3)) contentlen = atol(tmp3);
                 d->html->flags |= HTML_START;
	      }
	   }
	} while((ptr < end) && !Blank(htmlbuffer));

        if((ptr < end) && Blank(htmlbuffer)) {
           if(IsHtml(d) && !(d->html->flags & HTML_START)) {
              d->html->flags &= ~HTML_INPUT_PENDING;
              return;
	   }

           /* ---->  Parse contents  <---- */
           while((ptr < end) && ((*ptr == '\r') || (*ptr == '\n'))) ptr++;
           if(contentlen >= 0) {

              /* ---->  Check contents length  <---- */
              for(; (ptr < end) && (contentlen > 0); ptr++, contentlen--);
              if(contentlen < 1) {
                 if(IsHtml(d)) d->html->flags &= ~HTML_INPUT_PENDING;
                 return;
	      }
	   } else {

              /* ---->  Search for terminator  <---- */
              for(; (ptr < end); ptr++) {
                  if((*ptr == '\r') || (*ptr == '\n')) {
                     if(IsHtml(d)) d->html->flags &= ~HTML_INPUT_PENDING;
                     return;
		  }
	      }
	   }
	}
     } else if(IsHtml(d)) d->html->flags &= ~HTML_INPUT_PENDING;
}

/* ---->  Recursively traverse binary tree to sort character names into alphabetical order  <---- */
void traverse_names(struct character_sort_data *current)
{
     if(current->left) traverse_names(current->left);
     if(rootnode) {
        current->sort = NULL;
        tail->sort    = current;
        tail          = current;
     } else {
        rootnode      = current;
        tail          = current;
        current->sort = NULL;
     }
     if(current->right) traverse_names(current->right);
}

/* ---->  Generate list of users     <---- */
/*        0 = Users with web sites         */
/*        1 = Users with pictures          */
/*        2 = Users with scan              */
/*        3 = Users with profile           */
void html_userlist(struct descriptor_data *d,const char *params,unsigned char listtype)
{
     const    char *listname = (listtype == 0) ? "homepages":(listtype == 1) ? "gallery":(listtype == 2) ? "scans":"profiles";
     struct        character_sort_data *new,*root = NULL,*current,*last;
     char          buffer[TEXT_SIZE],buffer2[TEXT_SIZE];
     int           copied,counter = 0;
     unsigned char sections[27];
     char          ptr,section;
     unsigned char right,loop;
     const    char *homepage;
     dbref         i;

     /* ---->  Determine which sections exist  <---- */
     for(loop = 0; loop < 27; sections[loop] = 0, loop++);
     for(i = 0; i < db_top; i++) {
         if((Typeof(i) == TYPE_CHARACTER) && (db[i].data->player.bantime != -1) && (Controller(i) == i)) {
            if(((listtype == 0) && !Blank(db[i].data->player.odesc)) ||
               ((listtype == 1) && db[i].data->player.profile && db[i].data->player.profile->picture) ||
                (listtype == 2) ||
               ((listtype == 3) && hasprofile(db[i].data->player.profile))) {
                  if(!isalpha(*(db[i].name))) sections[26] = 1;
                     else sections[toupper(*(db[i].name)) - 65] = 1;
	       }
	 }
     }

     /* ---->  If no section specified, automatically choose first section with entries  <---- */
     if(Blank(params) || !(isalpha(*params) || (*params == '*'))) {
        section = '\0';
        for(ptr = 'A', loop = 0; loop < 27; ptr++, loop++)
            if((section == '\0') && sections[ptr - 65])
               section = (section == 26) ? '*':ptr;
        if(section == '\0') section = 'A';
     } else section = toupper(*params);

     /* ---->  Title  <---- */
     sprintf(buffer2,(listtype == 0) ? "Home Pages of Users of %s...":(listtype == 1) ? "%s User Picture Gallery...":(listtype == 2) ? "Scans of %s Users...":"Profiles of %s Users...",tcz_full_name);
     html_header(d,1,buffer2,0,NULL,NULL,NULL,NULL,NULL,NULL,NULL,0,0);
     output(d,NOTHING,1,0,0,"<TABLE WIDTH=100%% BORDER BGCOLOR=%s><TR><TD ALIGN=CENTER><FONT SIZE=6 COLOR=%s><B><I>%s</I></B></FONT><BR><FONT SIZE=5 COLOR="HTML_LYELLOW"><I><B>Section '<FONT COLOR="HTML_LWHITE">%c</FONT>'...</B></I></FONT></TD></TR></TABLE>",
            (listtype == 0) ? HTML_TABLE_GREEN:(listtype == 1) ? HTML_TABLE_BLUE:(listtype == 2) ? HTML_TABLE_CYAN:(listtype == 3) ? HTML_TABLE_MAGENTA:HTML_TABLE_GREEN,
            (listtype == 0) ? HTML_LGREEN:(listtype == 1) ? HTML_LBLUE:(listtype == 2) ? HTML_LCYAN:(listtype == 3) ? HTML_LMAGENTA:HTML_LGREEN,buffer2,section);

     /* ---->  Find character names and add to binary tree  <---- */
     for(i = 0; i < db_top; i++)
         if((Typeof(i) == TYPE_CHARACTER) && (db[i].data->player.bantime != -1) && (Controller(i) == i) && (((section == '*') && !isalpha(*(db[i].name))) || (toupper(*(db[i].name)) == section)))
            if(((listtype == 0) && !Blank(db[i].data->player.odesc)) ||
               ((listtype == 1) && db[i].data->player.profile && db[i].data->player.profile->picture) ||
                (listtype == 2) ||
               ((listtype == 3) && hasprofile(db[i].data->player.profile))) {
                  if(!root) {
                     MALLOC(new,struct character_sort_data);
                     new->right  = new->left = new->next = NULL;
                     new->player = i;
                     root        = new;
		  } else {
                     current = root, last = NULL;
                     while(current) {
                           if(strcasecmp(db[i].name,db[current->player].name) < 0) {
                              last    = current;
                              current = current->left;
                              right   = 0;
			   } else last = current, current = current->right, right = 1;
		     }

                     MALLOC(new,struct character_sort_data);
                     new->right  = new->left = NULL;
                     new->player = i;
                     new->next   = root->next;
                     root->next  = new;

                     if(last) {
                        if(right) last->right = new;
                           else last->left = new;
		     }
		  }
	    }

     /* ---->  Upper section list  <---- */
     for(ptr = 'A', loop = 0; loop < 27; ptr++, loop++)
         if(!(((section == '*') && (loop == 26)) || (ptr == section))) {
            if(sections[loop]) output(d,NOTHING,1,0,0,"%s<A HREF=\"%sSECTION=%c&\">%c</A>%s",(loop == 0) ? "<BR><HR><FONT SIZE=4 COLOR="HTML_DWHITE"><B><CENTER>":"&nbsp;",html_server_url(d,0,1,listname),(loop == 26) ? '*':ptr,(loop == 26) ? '*':ptr,(loop == 26) ? "</CENTER></FONT></B><HR><BR>":"");
               else output(d,NOTHING,1,0,0,"%s%c%s",(loop == 0) ? "<BR><HR><FONT SIZE=4 COLOR="HTML_DWHITE"><B><CENTER>":"&nbsp;",(loop == 26) ? '*':ptr,(loop == 26) ? "</CENTER></FONT></B><HR><BR>":"");
	 } else output(d,NOTHING,1,0,0,"%s<FONT SIZE=5 COLOR="HTML_LYELLOW"><I><BLINK>&nbsp;%c&nbsp;</BLINK></I></FONT>%s",(loop == 0) ? "<BR><HR><FONT SIZE=4 COLOR="HTML_DWHITE"><B><CENTER>":"&nbsp;",(loop == 26) ? '*':ptr,(loop == 26) ? "</CENTER></FONT></B><HR><BR>":"");

     /* ---->  Traverse tree to list names in alphabetical order  <---- */
     if(root) {
        rootnode = NULL, tail = NULL;
        traverse_names(root);
        output(d,NOTHING,1,0,0,"<TABLE BORDER WIDTH=100%% CELLPADDING=0 BGCOLOR="HTML_TABLE_GREY"><TR><TD><TABLE BORDER=0 WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK"><TR ALIGN=CENTER>");
        for(current = rootnode; current; current = current->sort) {
           if(++counter > 4) {
              output(d,NOTHING,1,0,0,"</TR><TR ALIGN=CENTER>");
              counter = 1;
	   }
           switch(listtype) {
                  case 0:
                       homepage = getfield(current->player,WWW);
                       output(d,NOTHING,1,0,0,"<TD WIDTH=25%%><A HREF=\"%s%s\" TARGET=_blank>%s</A></TD>",!strncasecmp(homepage,"http://",7) ? "":"http://",html_encode_basic(homepage,buffer,&copied,512),db[current->player].name);
                       break;
                  case 1:
                       output(d,NOTHING,1,0,0,"<TD WIDTH=25%%><A HREF=\"%sNAME=*%s&SECTION=%c&\">%s</A></TD>",html_server_url(d,0,1,"picture"),html_encode(db[current->player].name,buffer,&copied,256),section,db[current->player].name);
                       break;
                  case 2:
                       output(d,NOTHING,1,0,0,"<TD WIDTH=25%%><A HREF=\"%sNAME=*%s&SECTION=%c&\">%s</A></TD>",html_server_url(d,0,1,"scan"),html_encode(db[current->player].name,buffer,&copied,256),section,db[current->player].name);
                       break;
                  case 3:
                       output(d,NOTHING,1,0,0,"<TD WIDTH=25%%><A HREF=\"%sNAME=*%s&SECTION=%c&\">%s</A></TD>",html_server_url(d,0,1,"profile"),html_encode(db[current->player].name,buffer,&copied,256),section,db[current->player].name);
                       break;
	   }
	}

        if(counter != 0) {
           while(++counter <= 4) output(d,NOTHING,1,0,0,"<TD WIDTH=25%%>&nbsp;</TD>");
           output(d,NOTHING,1,0,0,"</TR>");
	}
        output(d,NOTHING,1,0,0,"</TABLE></TD></TR></TABLE>");

        /* ---->  Free binary tree  <---- */
        for(current = root; current; current = root) {
            root = current->next;
            FREENULL(current);
	}
     } else output(d,NOTHING,1,0,0,"<CENTER><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_RED"><TR><TD ALIGN=CENTER><FONT COLOR="HTML_LRED"><I><FONT SIZE=5>There are no users in this section.</FONT><BR>Please choose another section by clicking on its letter.</I></FONT></TD></TR></TABLE></CENTER>");

     /* ---->  Upper section list  <---- */
     for(ptr = 'A', loop = 0; loop < 27; ptr++, loop++)
         if(!(((section == '*') && (loop == 26)) || (ptr == section))) {
            if(sections[loop]) output(d,NOTHING,1,0,0,"%s<A HREF=\"%sSECTION=%c&\">%c</A>%s",(loop == 0) ? "<BR><HR><FONT SIZE=4 COLOR="HTML_DWHITE"><B><CENTER>":"&nbsp;",html_server_url(d,0,1,listname),(loop == 26) ? '*':ptr,(loop == 26) ? '*':ptr,(loop == 26) ? "</CENTER></FONT></B><HR>":"");
               else output(d,NOTHING,1,0,0,"%s%c%s",(loop == 0) ? "<BR><HR><FONT SIZE=4 COLOR="HTML_DWHITE"><B><CENTER>":"&nbsp;",(loop == 26) ? '*':ptr,(loop == 26) ? "</CENTER></FONT></B><HR>":"");
	 } else output(d,NOTHING,1,0,0,"%s<FONT SIZE=5 COLOR="HTML_LYELLOW"><I><BLINK>&nbsp;%c&nbsp;</BLINK></I></FONT>%s",(loop == 0) ? "<BR><HR><FONT SIZE=4 COLOR="HTML_DWHITE"><B><CENTER>":"&nbsp;",(loop == 26) ? '*':ptr,(loop == 26) ? "</CENTER></FONT></B><HR>":"");

     if(!html_lookup("NOBACK",0,0,0,0,1))
        output(d,NOTHING,1,0,0,"<HR><A HREF=\"%s\" TARGET=_parent><IMG SRC=\"%s\" ALIGN=MIDDLE BORDER=0 ALT=\"[BACK]\"></A> &nbsp; &nbsp; <FONT SIZE=4><B>Back to %s web site...</B></FONT><HR></BODY></HTML>",html_home_url,html_image_url("back.gif"),tcz_full_name);
}

/* ---->  Generate HTML for TCZ user input form  <---- */
void html_input_form(struct descriptor_data *src,struct descriptor_data *dest,char *buffer,unsigned char recall)
{
     int dummy;

     *buffer = '\0';
     if((src && !src->html) || !(dest && dest->html)) return;

     sprintf(buffer + strlen(buffer),"<BODY BACKGROUND=\"%s\"%s>",html_image_url("background.gif"),((src->html->flags & HTML_JAVA) && (src->html->flags & HTML_FOCUS)) ? " onLoad=\"document.INPUTFORM.COMMAND.focus();\"":"");
     if(src && (src->clevel == 14)) {

        /* ---->  AFK (Away From Keyboard)  <---- */
        sprintf(buffer + strlen(buffer),"<FORM NAME=INPUTFORM METHOD=POST ACTION=\"%s\"><CENTER><TABLE CELLPADDING=0><TR><TD ALIGN=LEFT><B><I><FONT COLOR="HTML_DRED">(AFK)</FONT></I> &nbsp; Please enter your password:</B><BR><INPUT NAME=COMMAND TYPE=PASSWORD SIZE=%d MAXLENGTH=128 VALUE=\"\"></TD></TR></TABLE><INPUT NAME=SEND TYPE=IMAGE SRC=\"%s\" ALT=\"[SEND]\" BORDER=0> ",html_server_url(src,0,0,NULL),src->html->cmdwidth,html_image_url("send.gif"));
        sprintf(buffer + strlen(buffer),"<A HREF=\"%sCOMMAND=QUIT&\"><IMG SRC=\"%s\" ALT=\"[LOGOUT]\" BORDER=0></A> ",html_server_url(src,1,1,"input"),html_image_url("logout.gif"));
        sprintf(buffer + strlen(buffer),"</CENTER><INPUT NAME=CODE TYPE=HIDDEN VALUE=\"%08X%08X\"><INPUT NAME=DATA TYPE=HIDDEN VALUE=INPUT></FORM>",(int) src->html->id1,(int) src->html->id2);
     } else if(src && src->prompt) {
        int htmlflags = src->html->flags;

        /* ---->  @prompt  <---- */
        output(dest,NOTHING,1,0,0,"%s<FORM NAME=INPUTFORM METHOD=POST ACTION=\"%s\"><CENTER><TABLE CELLPADDING=0><TR><TD ALIGN=LEFT><B>",buffer,html_server_url(src,0,0,NULL));
        substitute(src->player,buffer,!Blank(src->prompt->prompt) ? src->prompt->prompt:ANSI_LWHITE"User input:",0,ANSI_LWHITE,NULL,0);
        dest->html->flags |= ((src->html->flags & (HTML_SIMULATE_ANSI|HTML_UNDERLINE))|HTML_WHITE_AS_BLACK);
        output(dest,NOTHING,2,0,0,"%s",buffer);
        dest->html->flags = htmlflags;

        output(dest,NOTHING,1,0,0,"</B><BR><INPUT NAME=COMMAND TYPE=%s SIZE=%d MAXLENGTH=%d VALUE=\"",(src->flags & SUPPRESS_ECHO) ? "PASSWORD":"TEXT",src->html->cmdwidth,TEXT_SIZE);
        if(recall && !Blank(src->last_command)) {
           html_encode_basic(decompress(src->last_command),buffer,&dummy,MAX_LENGTH);
           output(dest,NOTHING,1,0,0,"%s",buffer);
	}

        sprintf(buffer,"\"></TD></TR></TABLE><INPUT NAME=SEND TYPE=IMAGE SRC=\"%s\" ALT=\"[SEND]\" BORDER=0> ",html_image_url("send.gif"));
        sprintf(buffer + strlen(buffer),"<A HREF=\"%sRECALL=&\"><IMG SRC=\"%s\" ALT=\"[RECALL]\" BORDER=0></A> ",html_server_url(src,1,1,"input"),html_image_url("recall.gif"));
        sprintf(buffer + strlen(buffer),"<A HREF=\"%sCOMMAND=ABORT&\"><IMG SRC=\"%s\" ALT=\"[ABORT]\" BORDER=0></A> ",html_server_url(src,1,1,"input"),html_image_url("abort.gif"));
        sprintf(buffer + strlen(buffer),"<INPUT NAME=EXECUTE TYPE=IMAGE SRC=\"%s\" ALT=\"[EXECUTE COMMAND]\" VALUE=YES BORDER=0> ",html_image_url("execute.gif"));
        sprintf(buffer + strlen(buffer),"<A HREF=\"%sTOPIC=INDEX&%s\" TARGET=_blank><IMG SRC=\"%s\" ALT=\"[TUTORIALS]\" BORDER=0></A> <A HREF=\"%sTOPIC=INDEX&%s\" TARGET=_blank><IMG SRC=\"%s\" ALT=\"[HELP]\" BORDER=0></A>",html_server_url(src,0,1,"tutorial"),html_get_preferences(src),html_image_url("tutorials.gif"),html_server_url(src,0,1,"help"),html_get_preferences(src),html_image_url("help.gif"));
        sprintf(buffer + strlen(buffer),"</CENTER><INPUT NAME=CODE TYPE=HIDDEN VALUE=\"%08X%08X\"><INPUT NAME=DATA TYPE=HIDDEN VALUE=INPUT></FORM>",(int) src->html->id1,(int) src->html->id2);
     } else if(src && src->cmdprompt) {
        int htmlflags = src->html->flags;

        /* ---->  Command arguments prompt session  <---- */
        output(dest,NOTHING,1,0,0,"%s<FORM NAME=INPUTFORM METHOD=POST ACTION=\"%s\"><CENTER><TABLE CELLPADDING=0><TR><TD ALIGN=LEFT><B>",buffer,html_server_url(src,0,0,NULL));
        substitute(src->player,buffer,!Blank(src->cmdprompt->prompt) ? src->cmdprompt->prompt:ANSI_LWHITE"User input:",0,ANSI_LWHITE,NULL,0);
        dest->html->flags |= ((src->html->flags & (HTML_SIMULATE_ANSI|HTML_UNDERLINE))|HTML_WHITE_AS_BLACK);
        output(dest,NOTHING,2,0,0,"%s",buffer);
        dest->html->flags = htmlflags;

        output(dest,NOTHING,1,0,0,"</B><BR><INPUT NAME=COMMAND TYPE=%s SIZE=%d MAXLENGTH=%d VALUE=\"",(src->flags & SUPPRESS_ECHO) ? "PASSWORD":"TEXT",src->html->cmdwidth,TEXT_SIZE);
        if(recall && !Blank(src->last_command)) {
           html_encode_basic(decompress(src->last_command),buffer,&dummy,MAX_LENGTH);
           output(dest,NOTHING,1,0,0,"%s",buffer);
	}

        sprintf(buffer,"\"></TD></TR></TABLE><INPUT NAME=SEND TYPE=IMAGE SRC=\"%s\" ALT=\"[SEND]\" BORDER=0> ",html_image_url("send.gif"));
        sprintf(buffer + strlen(buffer),"<A HREF=\"%sRECALL=&\"><IMG SRC=\"%s\" ALT=\"[RECALL]\" BORDER=0></A> ",html_server_url(src,1,1,"input"),html_image_url("recall.gif"));
        sprintf(buffer + strlen(buffer),"<A HREF=\"%sCOMMAND=ABORT&\"><IMG SRC=\"%s\" ALT=\"[ABORT]\" BORDER=0></A> ",html_server_url(src,1,1,"input"),html_image_url("abort.gif"));
        sprintf(buffer + strlen(buffer),"<A HREF=\"%sTOPIC=INDEX&%s\" TARGET=_blank><IMG SRC=\"%s\" ALT=\"[TUTORIALS]\" BORDER=0></A> <A HREF=\"%sTOPIC=INDEX&%s\" TARGET=_blank><IMG SRC=\"%s\" ALT=\"[HELP]\" BORDER=0></A>",html_server_url(src,0,1,"tutorial"),html_get_preferences(src),html_image_url("tutorials.gif"),html_server_url(src,0,1,"help"),html_get_preferences(src),html_image_url("help.gif"));
        sprintf(buffer + strlen(buffer),"</CENTER><INPUT NAME=CODE TYPE=HIDDEN VALUE=\"%08X%08X\"><INPUT NAME=DATA TYPE=HIDDEN VALUE=INPUT></FORM>",(int) src->html->id1,(int) src->html->id2);
     } else if(src && src->edit) {

        /* ---->  Editor  <---- */
        output(dest,NOTHING,1,0,0,"%s<FORM NAME=INPUTFORM METHOD=POST ACTION=\"%s\"><CENTER><TABLE CELLPADDING=0><TR><TD ALIGN=LEFT><B>Editor command (Line %d of %d):</B><BR><INPUT NAME=COMMAND TYPE=%s SIZE=%d MAXLENGTH=%d VALUE=\"",buffer,html_server_url(src,0,0,NULL),src->edit->line,strcnt(src->edit->text,'\n'),(src->flags & SUPPRESS_ECHO) ? "PASSWORD":"TEXT",src->html->cmdwidth,TEXT_SIZE);
        if(recall && !Blank(src->last_command)) {
           html_encode_basic(decompress(src->last_command),buffer,&dummy,MAX_LENGTH);
           output(dest,NOTHING,1,0,0,"%s",buffer);
	}

        sprintf(buffer,"\"></TD></TR></TABLE><INPUT NAME=SEND TYPE=IMAGE SRC=\"%s\" ALT=\"[SEND]\" BORDER=0> ",html_image_url("send.gif"));
        sprintf(buffer + strlen(buffer),"<A HREF=\"%sRECALL=&\"><IMG SRC=\"%s\" ALT=\"[RECALL]\" BORDER=0></A> ",html_server_url(src,1,1,"input"),html_image_url("recall.gif"));
        sprintf(buffer + strlen(buffer),"<A HREF=\"%sCOMMAND=.save&\"><IMG SRC=\"%s\" ALT=\"[SAVE]\" BORDER=0></A> ",html_server_url(src,1,1,"input"),html_image_url("save.gif"));
        sprintf(buffer + strlen(buffer),"<A HREF=\"%sCOMMAND=.abort=yes&\"><IMG SRC=\"%s\" ALT=\"[ABORT]\" BORDER=0></A> ",html_server_url(src,1,1,"input"),html_image_url("abort.gif"));
        sprintf(buffer + strlen(buffer),"<A HREF=\"%sCOMMAND=.up&\"><IMG SRC=\"%s\" ALT=\"[UP]\" BORDER=0></A> ",html_server_url(src,1,1,"input"),html_image_url("up.gif"));
        sprintf(buffer + strlen(buffer),"<A HREF=\"%sCOMMAND=.down&\"><IMG SRC=\"%s\" ALT=\"[DOWN]\" BORDER=0></A> ",html_server_url(src,1,1,"input"),html_image_url("down.gif"));
        sprintf(buffer + strlen(buffer),"<A HREF=\"%sCOMMAND=.view&\"><IMG SRC=\"%s\" ALT=\"[VIEW]\" BORDER=0></A> ",html_server_url(src,1,1,"input"),html_image_url("view.gif"));
        sprintf(buffer + strlen(buffer),"<INPUT NAME=EXECUTE TYPE=IMAGE SRC=\"%s\" ALT=\"[EXECUTE COMMAND]\" VALUE=YES BORDER=0> ",html_image_url("execute.gif"));
        sprintf(buffer + strlen(buffer),"<A HREF=\"%sOVERWRITE=&\"><IMG SRC=\"%s\" ALT=\"[OVERWRITE (%s)]\" BORDER=0></A> ",html_server_url(src,1,1,"input"),html_image_url((Validchar(src->player) && (db[src->player].flags2 & EDIT_OVERWRITE)) ? "overwrite.on.gif":"overwrite.off.gif"),(Validchar(src->player) && (db[src->player].flags2 & EDIT_OVERWRITE)) ? "ON":"OFF");
        sprintf(buffer + strlen(buffer),"<A HREF=\"%sNUMBERING=&\"><IMG SRC=\"%s\" ALT=\"[LINE NUMBERS (%s)]\" BORDER=0></A> ",html_server_url(src,1,1,"input"),html_image_url((Validchar(src->player) && (db[src->player].flags2 & EDIT_NUMBERING)) ? "numbers.on.gif":"numbers.off.gif"),(Validchar(src->player) && (db[src->player].flags2 & EDIT_NUMBERING)) ? "ON":"OFF");
        sprintf(buffer + strlen(buffer),"<A HREF=\"%sTOPIC=EDITOR&%s\" TARGET=_blank><IMG SRC=\"%s\" ALT=\"[TUTORIAL]\" BORDER=0></A> <A HREF=\"%sTOPIC=EDITOR&%s\" TARGET=_blank><IMG SRC=\"%s\" ALT=\"[HELP]\" BORDER=0></A>",html_server_url(src,0,1,"tutorial"),html_get_preferences(src),html_image_url("tutorial.gif"),html_server_url(src,0,1,"help"),html_get_preferences(src),html_image_url("help.gif"));
        sprintf(buffer + strlen(buffer),"</CENTER><INPUT NAME=CODE TYPE=HIDDEN VALUE=\"%08X%08X\"><INPUT NAME=DATA TYPE=HIDDEN VALUE=INPUT></FORM>",(int) src->html->id1,(int) src->html->id2);
     } else {
        const char *ptr,*help = "INDEX";
        char buffer2[TEXT_SIZE];

        /* ---->  TCZ command  <---- */
        if(src) {
           switch(src->clevel) {
                  case 7:
                  case 23:
                  case 24:
	          case 29:  /*  Disclaimer  */
                       ptr = HTML_ACCEPT_DISCLAIMER_PROMPT;
                       break;
                  case 8:  /*  Gender  */
                       ptr = HTML_GENDER_PROMPT;
                       break;
                  case 9:  /*  Race  */
                       ptr = HTML_RACE_PROMPT;
                       break;
                  case 10:
                  case 25:  /*  E-mail address  */
                       ptr = HTML_EMAIL_PROMPT;
                       break;
                  case 11:  /*  '@password':  Enter old password  */
                       ptr = HTML_OLD_PASSWORD_PROMPT;
                       break;
                  case 12:  /*  '@password':  Enter new password  */
                       ptr = HTML_NEW_PASSWORD_PROMPT;
                       break;
                  case 13:  /*  '@password':  Verify new password  */
                       ptr = HTML_VERIFY_NEW_PASSWORD_PROMPT;
                       break;
                  case 17:  /*  Time difference  */
                       ptr = HTML_TIME_DIFFERENCE_PROMPT;
                       break;
                  default:
                       ptr = NULL;
	   }
	} else ptr = NULL;

        if(!ptr) {
           if(src && Validchar(src->player) && (src->flags & CONNECTED)) {
              if(Location(src->player) == bbsroom) {
                 sprintf(buffer2,HTML_BBS_PROMPT,tcz_short_name);
                 ptr = buffer2, help = "BBS";
	      } else if(Location(src->player) == mailroom) {
                 sprintf(buffer2,HTML_MAIL_PROMPT,tcz_short_name);
                 ptr = buffer2, help = "MAIL";
	      } else if(Location(src->player) == bankroom) {
                 sprintf(buffer2,HTML_BANK_PROMPT,tcz_short_name);
                 ptr = buffer2, help = "BANK";
	      } else {
		 sprintf(buffer2,HTML_TCZ_PROMPT,tcz_short_name);
                 ptr = buffer2;
	      }
	   } else {
              sprintf(buffer2,HTML_TCZ_PROMPT,tcz_short_name);
              ptr = buffer2;
	   }
	}

        output(dest,NOTHING,1,0,0,"%s<FORM NAME=INPUTFORM METHOD=POST ACTION=\"%s\"><CENTER><TABLE CELLPADDING=0><TR><TD ALIGN=LEFT><B>%s</B><BR><INPUT NAME=COMMAND TYPE=%s SIZE=%d MAXLENGTH=%d VALUE=\"",buffer,html_server_url(src,0,0,NULL),ptr,(src && (src->flags & SUPPRESS_ECHO)) ? "PASSWORD":"TEXT",src->html->cmdwidth,TEXT_SIZE);
        if(src && recall && !Blank(src->last_command)) {
           html_encode_basic(decompress(src->last_command),buffer,&dummy,MAX_LENGTH);
           output(dest,NOTHING,1,0,0,"%s",buffer);
	}

        sprintf(buffer,"\"></TD></TR></TABLE><INPUT NAME=SEND TYPE=IMAGE SRC=\"%s\" ALT=\"[SEND]\" BORDER=0> ",html_image_url("send.gif"));
        sprintf(buffer + strlen(buffer),"<A HREF=\"%sRECALL=&\"><IMG SRC=\"%s\" ALT=\"[RECALL]\" BORDER=0></A> ",html_server_url((src) ? src:dest,1,1,"input"),html_image_url("recall.gif"));
        /* sprintf(buffer + strlen(buffer),"<A HREF=\"%sCONVERSE=&\"><IMG SRC=\"%s\" ALT=\"[CONVERSE (%s)]\" BORDER=0></A> ",html_server_url((src) ? src:dest,1,1,"input"),html_image_url((src && (src->flags & CONVERSE)) ? "converse.on.gif":"converse.off.gif"),(src && (src->flags & CONVERSE)) ? "ON":"OFF"); */
        sprintf(buffer + strlen(buffer),"<A HREF=\"%s%s\" TARGET=_blank><IMG SRC=\"%s\" ALT=\"[USER LIST]\" BORDER=0></A> ",html_server_url((src) ? src:dest,0,1,"userlist"),html_get_preferences(src),html_image_url("userlist.gif"));
        sprintf(buffer + strlen(buffer),"<A HREF=\"%sCOMMAND=QUIT&\"><IMG SRC=\"%s\" ALT=\"[LOGOUT]\" BORDER=0></A> ",html_server_url((src) ? src:dest,1,1,"input"),html_image_url("logout.gif"));
        sprintf(buffer + strlen(buffer),"<A HREF=\"%sTOPIC=INDEX&%s\" TARGET=_blank><IMG SRC=\"%s\" ALT=\"[TUTORIALS]\" BORDER=0></A> <A HREF=\"%sTOPIC=%s&%s\" TARGET=_blank><IMG SRC=\"%s\" ALT=\"[HELP]\" BORDER=0></A>",html_server_url((src) ? src:dest,0,1,"tutorial"),html_get_preferences(src),html_image_url("tutorials.gif"),html_server_url((src) ? src:dest,0,1,"help"),help,html_get_preferences(src),html_image_url("help.gif"));
        sprintf(buffer + strlen(buffer),"</CENTER><INPUT NAME=CODE TYPE=HIDDEN VALUE=\"%08X%08X\"><INPUT NAME=DATA TYPE=HIDDEN VALUE=INPUT></FORM>",(src) ? (int) src->html->id1:(int) dest->html->id1,(src) ? (int) src->html->id2:(int) dest->html->id2);
     }

     strcat(buffer,"</BODY>");
     output(dest,NOTHING,1,0,0,"%s",buffer);
     return;
}

/* ---->  Connect/create character via TCZ World Wide Web Interface  <---- */
unsigned char html_connect_character(struct descriptor_data *d,const char *data,unsigned char store)
{
       unsigned char     create = !strcasecmp("CREATE",data);
       const    char     *back  = (create) ? "Please click on the button to the left to return to the character creation form...":"Please click on the button to the left to return to the connection form...";
       const    char     *title = (create) ? "CREATE":"CONNECT";
       char              buffer[TEXT_SIZE],backurl[BUFFER_LEN];
       unsigned char     created = 0,reconnect = 0,guest = 0;
       const    unsigned char *name,*password;
       dbref             user = NOTHING;
       const    char     *error = NULL;
       time_t            now;

       gettime(now);
       if(create) {
          sprintf(backurl,"%sPARAMS=&",html_server_url(d,0,1,"createform"));
          html_copy_params(backurl + strlen(backurl),MAX_LENGTH - strlen(backurl));
       } else sprintf(backurl,"%s",html_server_url(d,0,0,"connectform"));

       if(html_lookup("GUEST",0,0,0,0,1)) guest = 1;
       if(guest || !(!(name = html_lookup("NAME",1,1,1,1,1)) || !*name)) {
          if(!error) {
             if(guest || !strcasecmp("guest",name)) {
                if(!create) {
                   sprintf(buffer,"To connect as a Guest character, please go to the character creation form and check the box '<B>Connect as a Guest character</B>'.<P><CENTER>Click <A HREF=\"%s\"><B>HERE</B></A> to go to the character creation form.</CENTER>",html_server_url(d,0,0,"createform"));
	           error = html_error(d,0,buffer,title,back,backurl,HTML_CODE_ERROR);
		}
                guest = 1;
	     } else user = lookup_nccharacter(NOTHING,name,create);

             if(!error) {
                strcpy(buffer,(guest) ? "Guest":((char *) name));
                if(!server_connection_allowed(d,buffer,user)) return(0);

                /* ---->  Validate given character name  <---- */
                if((user == NOTHING) && create) {
                   if(!guest)
                      switch(ok_character_name(NOTHING,NOTHING,buffer)) {
                             case 0:
                                  break;
                             case 2:
	                          error = html_error(d,0,"Sorry, the maximum length of your preferred character name is 20 characters.",title,back,backurl,HTML_CODE_ERROR);
                                  break;
                             case 3:
	                          error = html_error(d,0,"Sorry, your preferred character name must be at least 4 characters in length.",title,back,backurl,HTML_CODE_ERROR);
                                  break;
                             case 6:
                                  sprintf(buffer,"Sorry, the character name '<B>%s</B>' is not allowed  -  Please choose a different name for your character.",name);
	                          error = html_error(d,0,buffer,title,back,backurl,HTML_CODE_ERROR);
                                  break;
                             default:
                                  sprintf(buffer,"Sorry, the character name '<B>%s</B>' is invalid  -  Please choose a different name for your character.",name);
	                          error = html_error(d,0,buffer,title,back,backurl,HTML_CODE_ERROR);
		      }

                   strcpy(buffer,(guest) ? "Guest":((char *) name));
                   if(!error && server_site_banned(NOTHING,buffer,html_lookup("EMAIL",1,1,1,1,1),d,0,buffer)) return(0);
	        } else if(user == NOTHING) {
                   sprintf(buffer,"Sorry, a character with the name '<B>%s</B>' doesn't exist  -  Please check that you entered your character's name correctly.<P>If you do not have a character, and would like to create one, please click <A HREF=\"%s\">HERE</A>...",name,html_server_url(d,0,0,"createform"));
                   error = html_error(d,0,buffer,title,back,backurl,HTML_CODE_ERROR);
   	        } else if(!create) {
                   if(!Level4(user) && (server_count_connections(user,1) >= 5)) {
                      sprintf(buffer,"Sorry, the character <B>%s</B> is currently connected 5 or more times simultaneously  -  If your other connections have stopped responding, please connect as our Guest character (Specify <B>GUEST</B> as your preferred character name) and ask an <I>Apprentice Wizard/Druid</I> or above to boot your 'dead' connections using the '<B>@bootdead</B>' command.<P><B>PLEASE NOTE:</B> &nbsp; Any connections which have stopped responding will idle out and disconnect after <I>30 minutes</I> of inactivity.",getname(user));
	              error = html_error(d,0,buffer,title,back,backurl,HTML_CODE_ERROR);
		   } else {

                      /* ---->  Check character connecting isn't banned/site banned  <---- */
                      if(server_site_banned(user,name,NULL,d,1,buffer)) return(0);
                      if(!error) {
                         time_t bantime = db[user].data->player.bantime;

                         if(!Level4(user) && !bantime && Puppet(user)) bantime = db[Controller(user)].data->player.bantime;
                         if(bantime && !Level4(user)) {
      	                    if((bantime - now) > 0) {
                               sprintf(buffer,"Sorry, you have been banned from using %s for <I>%s</I>  -  Please send E-mail to <A HREF=\"mailto:%s\">%s</A> to negotiate lifting this ban.",tcz_full_name,interval(bantime - now,bantime - now,ENTITIES,0),tcz_admin_email,tcz_admin_email);
    	                       error = html_error(d,0,buffer,title,back,backurl,HTML_CODE_ERROR);
			    } else if(bantime == -1) {
                               sprintf(buffer,"Sorry, you have been banned permanently from using %s  -  Please send E-mail to <A HREF=\"mailto:%s\">%s</A> to negotiate lifting this ban.",tcz_full_name,tcz_admin_email,tcz_admin_email);
                               error = html_error(d,0,buffer,title,back,backurl,HTML_CODE_ERROR);
			    } else if(bantime) {
                               db[user].data->player.bantime = 0;
                               if(Puppet(user)) db[Controller(user)].data->player.bantime = 0;
			    }
			 }
		      }
		   }
		} else {
                   sprintf(buffer,"Sorry, a character with the name '<B>%s</B>' already exists (Somebody else is currently using that name.)  -  Please choose a different name for your character.",name);
                   error = html_error(d,0,buffer,title,back,backurl,HTML_CODE_ERROR);
		}
	     }

             if(!error && create) {
                const unsigned char *timediff,*gender,*verify,*email,*race;
                int   difference;

                /* ---->  Validate given password  <---- */
                if(!error && !guest) {
                   if((password = html_lookup("PASSWORD",1,1,1,1,1)) && *password) {
                      switch(ok_password(password)) {
                             case 0:
                                  if(!strcasecmp(password,name))
                                     error = html_error(d,0,"Sorry, your preferred password can't be the same as your name.",title,back,backurl,HTML_CODE_ERROR);
                                  break;
                             case 1:
                             case 2:
                             case 4:
                                  error = html_error(d,0,"Sorry, the password you gave contains invalid characters.",title,back,backurl,HTML_CODE_ERROR);
                                  break;
                             case 3:
                                  error = html_error(d,0,"Sorry, your preferred password must be at least <I>6 characters</I> in length.",title,back,backurl,HTML_CODE_ERROR);
                                  break;
                             default:
                                  error = html_error(d,0,"Sorry, the password you gave is invalid.",title,back,backurl,HTML_CODE_ERROR);
		      }
		   } else error = html_error(d,0,"Please specify a password for your new character.",title,back,backurl,HTML_CODE_ERROR);
		}

                if(!error && !guest) {
                   if((verify = html_lookup("VERIFY",1,1,1,1,1)) && *verify) {
                      if(strcasecmp(password,verify))
                         error = html_error(d,0,"Sorry, the passwords you gave for your character don't match.",title,back,backurl,HTML_CODE_ERROR);
		   } else error = html_error(d,0,"Please specify your preferred password twice (I.e:  Type it into the <I>PASSWORD</I> and <I>VERIFY PASSWORD</I> boxes.)",title,back,backurl,HTML_CODE_ERROR);
		}

                /* ---->  Terms and conditions of disclaimer accepted?  <---- */
                if(!error && !html_lookup("DISCLAIMER",0,0,0,0,1)) {
                   char buffer2[TEXT_SIZE];

                   sprintf(buffer2,"Sorry, you must read and accept the terms and conditions of the disclaimer before you can use %s.",tcz_full_name);
                   error = html_error(d,0,buffer2,title,back,backurl,HTML_CODE_ERROR);
		}

                /* ---->  Validate race  <---- */
                if(!error && !guest) {
                   if((race = html_lookup("RACE",1,1,1,1,1)) && *race) {
                      bad_language_filter((char *) race,(char *) race);
                      if(!(ok_name(race) && (strlen(race) >= 3) && (strlen(race) <= 50) && !strchr(race,'\n')))
                         error = html_error(d,0,"Sorry, the race of your character is invalid.",title,back,backurl,HTML_CODE_ERROR);
		   } else error = html_error(d,0,"Please specify the race of your character (E.g:  <I>Human</I>, <I>Alien</I>, etc.)",title,back,backurl,HTML_CODE_ERROR);
		}

                /* ---->  Validate E-mail address  <---- */
                if(!error) {
                   if((email = html_lookup("EMAIL",1,1,1,1,1)) && *email) {
                      if(!ok_email(NOTHING,(char *) email)) {
                         sprintf(buffer,"Sorry, the E-mail address you gave is invalid  -  It should be in the form of <I>YOUR_USERNAME@YOUR_SITE</I>, e.g:  <I>%s</I> (Example only.)",tcz_admin_email);
                         error = html_error(d,0,buffer,title,back,backurl,HTML_CODE_ERROR);
		      }
		   } else {
                      sprintf(buffer,"Please specify your E-mail address (E.g:  <I>%s</I> (Example only.))",tcz_admin_email);
                      error = html_error(d,0,buffer,title,back,backurl,HTML_CODE_ERROR);
		   }
		}

                /* ---->  Time difference  <---- */
                if(!error && !guest) {
                   if((timediff = html_lookup("TIMEDIFF",1,1,1,1,1)) && *timediff) {
                      for(; *timediff && (*timediff == ' '); timediff++);
                      if(!Blank(timediff)) {
                         const unsigned char *tptr = timediff;

                         if(*tptr == '-') tptr++;
                         if(isdigit(*tptr)) {
                            difference = atol(timediff);
                            if(abs(difference) > 24)
                               error = html_error(d,0,"Sorry, your time difference (From GMT) must be in the range of <I>+/- 24</I> hours.",title,back,backurl,HTML_CODE_ERROR);
			 } else error = html_error(d,0,"Sorry, your time difference (From GMT) must be in the range of <I>+/- 24</I> hours.",title,back,backurl,HTML_CODE_ERROR);
		      } else error = html_error(d,0,"Please specify your time difference (In hours, from GMT.)",title,back,backurl,HTML_CODE_ERROR);
		   } else error = html_error(d,0,"Please specify your time difference (In hours, from GMT.)",title,back,backurl,HTML_CODE_ERROR);
		}

                /* ---->  Create new character  <---- */
                if(!error) {
                   if(guest) {
                      if((user = connect_guest(&created)) == NOTHING) {
                         sprintf(buffer,"Sorry, all %d Guest characters are currently in use  -  Please try again later.",guestcount);
                         error = html_error(d,0,buffer,title,back,backurl,HTML_CODE_ERROR);
		      } else d->flags |= DESTROY;
		   } else if((user = create_new_character(name,password,1)) == NOTHING) {
                      sprintf(buffer,"Sorry, unable to automatically create a new character for you  -  Please send E-mail to <A HREF=\"mailto:%s\">%s</A> and we will create one for you.",tcz_admin_email,tcz_admin_email);
                      error = html_error(d,0,buffer,title,back,backurl,HTML_CODE_ERROR);
                      writelog(CREATE_LOG,1,"FAILED CREATE","%s (%s descriptor %d) from %s (Unable to automatically create new character.)",name,(d->html) ? "HTML":"Telnet",d->descriptor,d->hostname);
		   } else {
#ifdef HOME_ROOMS
                      create_homeroom(user,0,1,0);
#endif
                      created = 1;
		   }

                   if(!error) {
                      if(!created) {
                         writelog(CONNECT_LOG,1,"CONNECTED","%s (%s descriptor %d) from %s.",unparse_object(ROOT,user,0),(d->html) ? "HTML":"Telnet",d->descriptor,d->hostname);
                         writelog(UserLog(user),1,"CONNECTED","%s (%s) from %s.",unparse_object(ROOT,user,0),(d->html) ? "HTML":"Telnet",d->hostname);
		      } else writelog(CREATE_LOG,1,"CREATED","%s (%s descriptor %d) from %s.",unparse_object(ROOT,user,0),(d->html) ? "HTML":"Telnet",d->descriptor,d->hostname);

                      d->player = user;
                      if(!guest) setfield(user,RACE,race,1);
                      setfield(user,EMAIL,settextfield(email,2,'\n',getfield(user,EMAIL),buffer),0);
                      if(!guest) db[user].data->player.timediff = difference;
                      db[user].flags2 |=  FRIENDS_CHAT;
                      db[user].flags  &= ~(HAVEN|QUIET);

                      db[user].flags &= ~GENDER_MASK;
                      if((gender = html_lookup("GENDER",1,1,1,1,1)) && *gender) {
                         if(!strcasecmp(gender,"MALE")) db[user].flags |= GENDER_MALE << GENDER_SHIFT;
                            else if(!strcasecmp(gender,"FEMALE")) db[user].flags |= GENDER_FEMALE << GENDER_SHIFT;
                               else if(!strcasecmp(gender,"NEUTER")) db[user].flags |= GENDER_NEUTER << GENDER_SHIFT;
		      }
		   }
		}
	     } else if(!error) {
		if(!(!(password = html_lookup("PASSWORD",1,1,1,1,1)) || !*password)) {
                   if(connect_character(name,password,d->hostname) == NOTHING) {
                      writelog(PASSWORD_LOG,1,"CONNECT","Failed login attempt as '%s' from %s (HTML descriptor %d.)",name,d->hostname,d->descriptor);
                      error = html_error(d,0,"Sorry, incorrect password.  To request a new password, <a href=\"createform\">connect as a Guest</a> and contact an Admin.",title,back,backurl,HTML_CODE_ERROR);
		   } else {
                      writelog(CONNECT_LOG,1,"CONNECTED","%s (%s descriptor %d) from %s.",unparse_object(ROOT,user,0),(d->html) ? "HTML":"Telnet",d->descriptor,d->hostname);
                      writelog(UserLog(user),1,"CONNECTED","%s (%s) from %s.",unparse_object(ROOT,user,0),(d->html) ? "HTML":"Telnet",d->hostname);
		   }
		} else error = html_error(d,0,"Please specify your character's password.",title,back,backurl,HTML_CODE_ERROR);
	     }

             if(!error) {
                struct   descriptor_data *ptr,*m;
                char     buffer2[TEXT_SIZE];
                unsigned char finished = 0;
                unsigned long id1,id2;

                /* ---->  Take over existing connection  <---- */
                for(ptr = descriptor_list; ptr && !finished; ptr = (finished) ? ptr:m) {
                    m = ptr->next;
                    if((ptr != d) && ptr->html && (user == ptr->player)) {
                       if(!(ptr->html->flags & HTML_OUTPUT) || !(ptr->flags & CONNECTED))
                          server_shutdown_sock(ptr,0,0);
		             else finished = 1;
		    }
		}

                /* ---->  Allocate unique ID code  <---- */
                finished = 0;
                if(!ptr) {
                   while(!finished) {
                         id1 = lrand48(), id2 = lrand48(), finished = 1;
                         if(id1 && id2) {
                            for(ptr = descriptor_list; ptr && finished; ptr = ptr->next)
                                if(ptr->html && ((id1 == ptr->html->id1) || (id2 == ptr->html->id2)))
                                   finished = 0;
			 } else finished = 0;
		   }
                   d->html->id1 = id1, d->html->id2 = id2;
		} else {
                   d->html->id1 = ptr->html->id1, d->html->id2 = ptr->html->id2;
                   d->html->flags |= HTML_INPUT;
                   reconnect = 1;
		}

                /* ---->  Initialise HTML interface  <---- */
                sprintf(buffer,"<TITLE>"HTML_TITLE"</TITLE>",tcz_full_name,tcz_year);
                output(d,NOTHING,1,0,0,"%s<FRAMESET ROWS=\"*,110\"><FRAME NAME=TCZOUTPUT SRC=\"%s%s\"><FRAME NAME=TCZINPUT SRC=\"%s\"></FRAMESET><NOFRAMES>",buffer,html_server_url(d,1,1,"output"),(create) ? "CREATED=YES&":"",html_server_url(d,1,1,"input"));

                sprintf(buffer,"Sorry, your web browser doesn't support frames.<P>In order to use <I>%s World Wide Web Interface</I>, you'll need to use a browser which supports <B>frames</B> and <B>tables</B>, such as an up-to-date version of either <A HREF=\"http://www.netscape.com\">Netscape Navigator</A> or <A HREF=\"http://www.microsoft.com\">Microsoft Internet Explorer</A>.  Please download an up-to-date copy of one of these browsers and then try again.<P>Alternatively, you can connect with (Text-only) <I>Telnet</I> software, using the following address: &nbsp; <A HREF=\"telnet://%s:%d\">%s</A>, port number <A HREF=\"telnet://%s:%d\">%d</A> (<A HREF=\"telnet://%s:%d\">%s</A>, port number <A HREF=\"telnet://%s:%d\">%d</A>.)",tcz_full_name,tcz_server_name,(int) telnetport,tcz_server_name,tcz_server_name,(int) telnetport,(int) telnetport,tcz_server_name,(int) telnetport,tcz_server_name,tcz_server_name,(int) telnetport,(int) telnetport);                
                sprintf(buffer2,"Return to %s web site...",tcz_full_name);
                html_error(d,1,buffer,"CONNECTION REFUSED",buffer2,html_home_url,HTML_CODE_ERROR & ~(HTML_CODE_HEADER|HTML_CODE_HTML|HTML_CODE_HEAD|HTML_CODE_TITLE));
                output(d,NOTHING,1,0,0,"</NOFRAMES></HTML>");

                /* ---->  (Re)connect character  <---- */
                if(!reconnect) {
		   d->name_time = (created && guest) ? 0:(now + NAME_TIME);
                   d->channel   = NOTHING;
		}
                d->player = user;
                d->clevel = 0;
                d->flags |= UNDERLINE;

                if(!reconnect) {

                   /* ---->  Monitor/emergency command logging  <---- */
                   for(ptr = descriptor_list; ptr; ptr = ptr->next)
                       if((ptr->flags & CONNECTED) && (d->player == ptr->player)) {
                          if(ptr->monitor) {
                             d->flags  |= (ptr->flags & (MONITOR_OUTPUT|MONITOR_CMDS));
                             d->monitor = ptr->monitor;
			  }
                          d->emergency_time = ptr->emergency_time;
		       }

                   for(ptr = descriptor_list; ptr; ptr = m) {
                       m = ptr->next;
                       if((ptr != d) && (ptr->flags & DELIVERED) && (ptr->descriptor == NOTHING) && (ptr->player == user))
                          server_shutdown_sock(ptr,0,0);
		   }

                   if(store || create || (db[user].data->player.htmlflags & HTML_START))
                      html_store_preferences(user,d);
                         else html_recall_preferences(user,d);
                   d->flags |= DELIVERED;
		} else {
                   if(store) {
                      html_store_preferences(user,d);
                      html_recall_preferences(user,ptr);
		   }
                   server_shutdown_sock(d,0,2);
		}
	     }
	  }
       } else {
          char buffer2[TEXT_SIZE];

          sprintf(buffer2,(create) ? "Please specify the name (Or alias) you would like to be known as on %s.":"Please specify your character name on %s.",tcz_full_name);
          error = html_error(d,0,buffer2,title,back,backurl,HTML_CODE_ERROR);
       }

       if(error) {
          output(d,NOTHING,1,0,0,"%s",error);
          return(0);
       }
       return(1);
}

/* ---->  Lookup and set HTML preferences from HTML parameters  <---- */
void html_preferences_lookup(struct descriptor_data *d)
{
     const char *data;

     if(!d || !IsHtml(d) || !params) return;
     if(html_lookup("BACKGROUND",0,0,0,0,1)) d->html->flags |= HTML_BGRND;
     if(!html_lookup("ITALIC",0,0,0,0,1))    d->html->flags |= HTML_UNDERLINE;
     if(html_lookup("SCROLLBY",0,0,0,0,1))   d->html->flags |= HTML_SCROLLBY;
     if(html_lookup("REVERSE",0,0,0,0,1))    d->html->flags |= HTML_WHITE_AS_BLACK;
     if(html_lookup("SMILEY",0,0,0,0,1))     d->html->flags |= HTML_SMILEY;
     if(html_lookup("COLOUR",0,0,0,0,1))     d->html->flags |= HTML_SIMULATE_ANSI;
     if(html_lookup("SCROLL",0,0,0,0,1))     d->html->flags |= HTML_SCROLL;
     if(html_lookup("FOCUS",0,0,0,0,1))      d->html->flags |= HTML_FOCUS;
     if(html_lookup("STYLE",0,0,0,0,1))      d->html->flags |= HTML_STYLE;
     if(html_lookup("LINKS",0,0,0,0,1))      d->html->flags |= HTML_LINKS;
     if(html_lookup("JAVA",0,0,0,0,1))       d->html->flags |= HTML_JAVA;
     if(html_lookup("SSL",0,0,0,0,1))        d->html->flags |= HTML_SSL;

     /* ---->  Command input box width  <---- */
     if((data = html_lookup("CMDWIDTH",0,0,0,0,1))) {
	int value;

	value = atoi(data);
	if(value <= 0)   value = HTML_CMDWIDTH;
	if(value <= 25)  value = 25;
	if(value >= 255) value = 255;
	d->html->cmdwidth = value;
     }
}

/* ---->  Process received HTML data  <---- */
unsigned char html_process_data(struct descriptor_data *d)
{
	 char           *resource = NULL,*resource_params = NULL;
	 unsigned char  *getstart = NULL,*poststart = NULL;
	 unsigned char  header = 1,error = 0,temp,post;
	 const    char  *contents,*data;
	 unsigned char  *ptr = d->negbuf;
	 unsigned short pos = 0,tpos;
	 int            postlen = 0;
	 char           *tmp;
	 time_t         now;

	 gettime(now);
	 if(d->negbuf && (d->neglen > 1)) {
	    for(; (pos < d->neglen) && (*ptr == ' '); ptr++, pos++);

	    /* ---->  HTTP protocol version  <---- */
	    for(tmp = (char *) ptr, tpos = pos; (tpos < (d->neglen - 8)) && !(((*tmp == 'h') || (*tmp == 'H')) && !strncasecmp(tmp + 1,"TTP/",4)); tmp++, tpos++);
	    if((tpos < (d->neglen - 8)) && !strncasecmp(tmp,"HTTP/",5)) {
	       int  major,minor,count = 0;
	       char buffer[32];
	       char *cpy;

	       for(cpy = buffer, tmp += 5; (tpos < d->neglen) && (count < 32) && (isdigit(*tmp) || (*tmp == '.')); *cpy++ = *tmp++, count++, tpos++);
	       *cpy = '\0';

	       if(!Blank(buffer)) {
		  sscanf(buffer,"%d.%d",&major,&minor);
		  if((major == 1) && (minor == 0)) {
		     d->html->protocol = HTML_PROTOCOL_HTTP_1_0;
		  } else d->html->protocol = HTML_PROTOCOL_HTTP_1_1;
	       } else d->html->protocol = HTML_PROTOCOL_HTTP_1_1;
	    } else d->html->protocol = HTML_PROTOCOL_HTTP_1_1;

	    /* ---->  Method (GET/POST)  <---- */
	    if((!(post = 0) && !strncasecmp(ptr,"GET ",4)) || ((post = 1) && !strncasecmp(ptr,"POST ",5))) {

	       /* ---->  GET method  <---- */
	       for(ptr += 4, pos += 4; (pos < d->neglen) && (*ptr == ' '); ptr++, pos++);
	       if(pos < d->neglen) {
		  int questionmark = 0;

		  /* ---->  Resource specified  <---- */
		  if(*ptr == '/') {
		     char *rptr;

		     for(ptr++, pos++, resource = ptr; (pos < d->neglen) && (*ptr != '/') && (*ptr != '?') && (*ptr != ' ') && (*ptr != '\r') && (*ptr != '\n'); ptr++, pos++);
		     if(pos < d->neglen) {
			if((*ptr == '/') || (*ptr == '?') || (*ptr == ' ') || (*ptr == '\r') || (*ptr == '\n')) {
			   if(*ptr == '/') {
			      for(*ptr++ = '\0', pos++, resource_params = ptr; (pos < d->neglen) && (*ptr != '?') && (*ptr != ' ') && (*ptr != '\r') && (*ptr != '\n'); ptr++, pos++);
			      if(pos < d->neglen) {
				 if((*ptr == '?') || (*ptr == ' ') || (*ptr == '\r') || (*ptr == '\n')) {
				    if(*ptr == '?') questionmark = 1;
				    *ptr++ = '\0', pos++;
				 } else resource_params = NULL;
			      } else resource_params = NULL;
			   } else {
			      if(*ptr == '?') questionmark = 1;
			      *ptr++ = '\0', pos++;
			   }
			} else resource = NULL;
		     } else resource = NULL;

		     if(!Blank(resource) && (rptr = strchr(resource,'.')))
			*rptr = '\0';
		  }
		  
		  /* ---->  GET parameters ('/?PARAM=VALUE&')  <---- */
		  if(!questionmark)
		     for(; (pos < d->neglen) && (*ptr != '?') && (*ptr != '\r') && (*ptr != '\n'); ptr++, pos++);
		  for(; (pos < d->neglen) && (*ptr == '?') && (*ptr != '\r') && (*ptr != '\n'); ptr++, pos++);
		  for(getstart = ptr; (pos < d->neglen) && (*ptr != ' ') && (*ptr != '\r') && (*ptr != '\n'); ptr++, pos++);
		  if(ptr > getstart) {
		     if((pos < d->neglen) && (*ptr == ' ')) *ptr = '\0';
			else d->negbuf[d->neglen - 1] = '\0';
		     for(pos--, ptr--; (ptr >= getstart) && (*ptr == '/'); *ptr-- = '\0', pos--);
		     if(ptr <= getstart) error = 1;
		  } else getstart = NULL;
	       } else error = 1;

	       if(!error && post) {

		  /* ---->  POST method  <---- */
		  for(; (pos < (d->neglen - 5)) && strncmp(ptr,"\r\n\r\n",4); ptr++, pos++);
		  if((pos < (d->neglen - 5)) && !strncmp(ptr,"\r\n\r\n",4)) {
		     ptr += 4, pos += 4, poststart = ptr;
		     for(; (pos < d->neglen) && (*ptr != '\r'); ptr++, pos++, postlen++);
		     if(pos < d->neglen) *ptr = '\0', postlen = NOTHING;
		     if(ptr <= poststart) error = 1;
		  }
	       }
	       html_init(NULL,NOTHING);

	       if(!error) {
		  if(!(!Blank(resource) && !strcasecmp(resource,"INDEX")) && (html_init(getstart,NOTHING) || html_init(poststart,postlen) || !Blank(resource))) {
		     if(((data = html_lookup("DATA",1,1,1,1,1)) && !Blank(data)) || ((data = resource) && !Blank(resource))) {
			if(!strcasecmp("IMAGE",data) || !strcasecmp("IMAGES",data)) {

			   /* ---->  HTML Interface image, served directly from TCZ  <---- */
			   if(((data = html_lookup("NAME",1,1,1,1,1)) && !Blank(data)) || ((data = resource_params) && !Blank(resource_params))) {
			      if(option_images(OPTSTATUS)) {
				 struct html_image_data *image;

				 if((image = html_search_images(data)) && (image->size > 0) && image->data) {
				    writelog(HTML_LOG,1,"IMAGE","Internal image '%s' served to descriptor %d (%s).",data,d->descriptor,d->hostname);
				    command_type |= NO_FLUSH_OUTPUT;
				    http_header(d,1,200,"OK",http_content_type[image->imgtype],now - MONTH,now + MONTH,1,image->size);
				    server_queue_output(d,image->data,image->size);
				    command_type &= ~NO_FLUSH_OUTPUT;
				 } else {
				    char buf[TEXT_SIZE];

				    sprintf(buf,"Sorry, the image '<B>%s</B>' cannot be found on this server.",data);
				    html_error(d,1,buf,"NOT FOUND","Please click on the <B>BACK</B> button of your browser or the button to the left...",html_home_url,(header) ? (HTML_CODE_ERROR|HTML_CODE_HEADER):HTML_CODE_ERROR);
				    writelog(HTML_LOG,1,"IMAGE","Unable to serve image '%s' to descriptor %d (%s)  -  Image does not exist.",resource_params,d->descriptor,d->hostname);
				 }
			      } else {
				 char buf[TEXT_SIZE];

				 sprintf(buf,"Sorry, images are not currently being served internally by %s.<P>The image '<B>%s</B>' can be found at '<A HREF=\"%s%s\">%s%s</A>'.",tcz_short_name,data,html_data_url,data,html_data_url,data);
				 html_error(d,1,buf,"PERMISSION DENIED","Please click on the <B>BACK</B> button of your browser or the button to the left...",html_home_url,(header) ? (HTML_CODE_ERROR|HTML_CODE_HEADER):HTML_CODE_ERROR);
				 writelog(HTML_LOG,1,"IMAGE","Unable to serve image '%s' to descriptor %d (%s)  -  Internal images disabled.",data,d->descriptor,d->hostname);
			      }
			   } else {
			      html_error(d,1,"Sorry, name of image not specified.","ERROR","Please click on the <B>BACK</B> button of your browser or the button to the left...",html_home_url,(header) ? (HTML_CODE_ERROR|HTML_CODE_HEADER):HTML_CODE_ERROR);
			      writelog(HTML_LOG,1,"IMAGE","Blank image name requested by descriptor %d (%s).",d->descriptor,d->hostname);
			   }
			   server_shutdown_sock(d,0,2);
			} else if(!strcasecmp("INPUT",data)) {
			   struct        descriptor_data *ptr;
			   unsigned long id1,id2,recall = 0;
			   char          buffer[BUFFER_LEN];
			   unsigned char closed = 0;
			   char          *code;

			   /* ---->  User input  <---- */
			   http_header(d,1,200,"OK","text/html",now,now,0,0), header = 0;
			   if((code = (char *) html_lookup("CODE",1,1,1,1,1)) && (strlen(code) == 16)) {
			      d->html->flags |= HTML_INPUT;
			      id2 = tohex(code + 8), code[8] = '\0', id1 = tohex(code);
			      for(ptr = descriptor_list; ptr && ((ptr == d) || !ptr->html || (ptr->html->flags & HTML_INPUT) || !((ptr->html->id1 == id1) && (ptr->html->id2 == id2))); ptr = ptr->next);
			      if(ptr) {
				 if(!(ptr->flags & CONNECTED) || !Validchar(ptr->player) || (d->address == ptr->address)) {
				    ptr->warning_level = 0;
				    if(html_lookup("CONVERSE",0,0,0,0,1)) {
				       if(ptr->flags & CONVERSE) ptr->flags &= ~CONVERSE;
					  else ptr->flags |= CONVERSE;
				    }

				    if(Validchar(ptr->player) && ptr->edit) {
				       if(html_lookup("OVERWRITE",0,0,0,0,1)) {
					  if(db[ptr->player].flags2 & EDIT_OVERWRITE) db[ptr->player].flags2 &= ~EDIT_OVERWRITE;
					     else db[ptr->player].flags2 |= EDIT_OVERWRITE;
				       }

				       if(html_lookup("NUMBERING",0,0,0,0,1)) {
					  if(db[ptr->player].flags2 & EDIT_NUMBERING) db[ptr->player].flags2 &= ~EDIT_NUMBERING;
					     else db[ptr->player].flags2 |= EDIT_NUMBERING;
				       }
				    }

				    /* ---->  Execute command  <---- */
				    if(ptr->edit) data = html_lookup("COMMAND",0,0,0,0,1);
				       else data = html_lookup("COMMAND",1,1,1,1,1);

				    if(data) {
				       const char *secure = html_lookup("LINK",1,1,1,1,1);

				       if(ptr->flags & PROMPT) {
					  command_type     |=  NO_AUTO_FORMAT;
					  ptr->flags       &= ~PROMPT;
					  output(ptr,NOTHING,0,0,0,ANSI_DWHITE"%s",(ptr->flags & SUPPRESS_ECHO) ? "":data);
					  command_type     &= ~NO_AUTO_FORMAT;
				       }

				       if(!secure && (ptr->edit || ptr->prompt || ptr->cmdprompt) && html_lookup("EXECUTE",0,0,0,0,1)) {
					  writelog(HTML_LOG,1,"INPUT","%s received from descriptor %d (%s) for %s(#%d.)",(ptr->edit) ? "Editor command":(ptr->prompt) ? "@prompt input":(ptr->cmdprompt) ? "Command arguments prompt input":"Unknown type of command",ptr->descriptor,ptr->hostname,getname(ptr->player),ptr->player);
					  sprintf(buffer,".x %s",data);
					  buffer[TEXT_SIZE] = '\0';
					  server_command(ptr,buffer);
				       } else {
					  const char *subst = NULL;

					  /* ---->  ('SUBST=OK') Protect '$' and '{' in executed command  <---- */
					  if((subst = html_lookup("SUBST",0,0,0,0,1))) {
					     const char *ptr = data;
					     char *dest = buffer;

					     for(; *ptr; *dest++ = *ptr++)
						 if((*ptr == '$') || (*ptr == '{'))
						    *dest++ = '\\';
					     *dest = '\0';
					     buffer[TEXT_SIZE] = '\0';
					     data = buffer;
					  }

					  /* ---->  Execute command  <---- */
					  if(secure) security = 1, compoundonly = 1;
					  writelog(HTML_LOG,1,"INPUT","Command received from descriptor %d (%s) for %s(#%d) (Substitutions are %s.)",ptr->descriptor,ptr->hostname,getname(ptr->player),ptr->player,Blank(subst) ? "unprotected":"protected");
					  if(!server_command(ptr,(char *) data)) {
					     output(ptr,ptr->player,1,0,0,"</BODY></HTML>");
					     server_shutdown_sock(ptr,0,2), closed = 1;
					  }
					  if(secure) security = 0, compoundonly = 0;
				       }
				    } else if(html_lookup("RECALL",0,0,0,0,1)) {
				       writelog(HTML_LOG,1,"INPUT","Last command recall received from descriptor %d (%s) for %s(#%d.)",ptr->descriptor,ptr->hostname,getname(ptr->player),ptr->player);
				       recall = 1;
				    }
				 } else {
				    char buffer2[TEXT_SIZE];

				    writelog(HTML_LOG,1,"INPUT","Access Denied:  Host address of descriptor %d (%s) does not match the host address of descriptor %d (%s) assigned to %s(#%d).",d->descriptor,d->hostname,ptr->descriptor,ptr->hostname,getname(ptr->player),ptr->player);
				    sprintf(buffer2,"Sorry, you can't send commands to a user's World Wide Web Interface connection from a different Internet address.<P>If you have lost your connection and are unable to reconnect because your character is still connected to %s, please connect as a <B>Guest Character</B> and ask an administrator to use the '<B>@bootdead</B>' command to disconnect your 'dead' connection (<B>NOTE:</B> &nbsp; Lost connections will usually time-out and disconnect after <B><I>5 minutes</I></B>.)",tcz_short_name);
				    html_error(d,1,buffer2,"CONNECTION REFUSED","Please click on the <B>BACK</B> button of your browser or the button to the left...",html_home_url,(header) ? (HTML_CODE_ERROR|HTML_CODE_HEADER):HTML_CODE_ERROR);
				    error = 2;
				 }

				 /* ---->  Regenerate input form  <---- */
				 if(!error) {
				    if(closed) {
				       html_header(d,1,HTML_TITLE,0,html_image_url("background.gif"),HTML_DBLACK,HTML_DWHITE,HTML_LBLUE,HTML_LCYAN,HTML_DBLUE,NULL,0,0);
				       output(d,NOTHING,1,0,0,"<CENTER><FONT SIZE=6><B><I>CONNECTION CLOSED.</I></B></FONT><BR><B>Thank you for visiting %s</B>  -  <FONT SIZE=2><I>Please remember to clear the disk cache and exit your web browser <B>BEFORE</B> leaving your computer (Especially if other people will be using it.)</I></FONT></CENTER><HR><A HREF=\"%s\" TARGET=_parent><IMG SRC=\"%s\" ALT=\"[BACK]\" BORDER=0 ALIGN=MIDDLE></A> &nbsp; &nbsp; <B>Back to %s web site...</B>",tcz_full_name,html_home_url,html_image_url("back.gif"),tcz_full_name);
				    } else html_input_form(ptr,d,buffer,recall);
				    output(d,d->player,1,0,0,"</BODY></HTML>");
				    server_shutdown_sock(d,0,2);
				 }
			      } else {
				 writelog(HTML_LOG,1,"INPUT","Access Denied:  Invalid authorisation code received from descriptor %d (%s) (CODE=%s&)  -  Code does not match a connected HTML descriptor.",d->descriptor,d->hostname,!Blank(data) ? data:"");
				 error = 1;
			      }
			   } else {
			      writelog(HTML_LOG,1,"INPUT","Access Denied:  Invalid authorisation code received from descriptor %d (%s) (CODE=%s&)  -  Blank code or incorrect length.",d->descriptor,d->hostname,!Blank(data) ? data:"");
			      error = 1;
			   }
			} else if((!strcasecmp("CONNECT",data) && (temp = 1)) || (!strcasecmp("CREATE",data) && !(temp = 0))) {
			   unsigned char store = 0;

			   /* ---->  Connect/create character  <---- */
			   writelog(HTML_LOG,1,(temp) ? "CONNECT":"CREATE","Character %s form parameters received from descriptor %d (%s) for '%s'.",(temp) ? "connection":"creation",d->descriptor,d->hostname,html_lookup("NAME",0,0,0,0,1));
			   http_header(d,1,200,"OK","text/html",now,now,0,0), header = 0;
			   if(!html_lookup("SAVED",0,0,0,0,1)) store = 1;
			   html_preferences_lookup(d);
			   if((d->html->flags & HTML_BGRND) && (contents = html_lookup("IMAGE",1,1,1,1,1)) && !Blank(contents) && strcasecmp(contents,html_image_url("default.gif")))
			      d->html->background = (char *) alloc_string(compress(contents,0));
			   if(!html_connect_character(d,data,store)) error = 2;
			} else if(!strcasecmp("OUTPUT",data)) {
			   struct        descriptor_data *ptr;
			   char          buffer[TEXT_SIZE];
			   unsigned char created = 0;
			   int           taken = 0;
			   unsigned long id1,id2;
			   char          *code;

			   /* ---->  User output  <---- */
			   if((code = (char *) html_lookup("CODE",1,1,1,1,1)) && (strlen(code) == 16)) {
			      d->html->flags |= HTML_OUTPUT;
			      id2 = tohex(code + 8), code[8] = '\0', id1 = tohex(code);
			      for(ptr = descriptor_list; ptr && !(ptr->html && !(ptr->html->flags & HTML_INPUT) && (ptr->html->id1 == id1) && (ptr->html->id2 == id2)); ptr = ptr->next);
			      if(ptr) {
				 if(!(ptr->flags & CONNECTED) || !Validchar(ptr->player) || (d->address == ptr->address)) {

				    /* ---->  Take over existing connection  <---- */
				    if(Validchar(ptr->player) && Connected(ptr->player)) taken = 1;
				    output(ptr,ptr->player,0,1,0,ANSI_LRED"\n[You have taken over this connection either because you clicked on the "ANSI_LWHITE"RELOAD"ANSI_LRED" button of your browser, or it stopped responding and you reconnected.]\n\016</BODY></HTML>\016");
				    server_process_output(ptr);
				    if(ptr->descriptor > NOTHING) {
				       shutdown(ptr->descriptor,2);
				       close(ptr->descriptor);
				    }

				    if(((now - ptr->last_time) >= (IDLE_TIME * MINUTE)) && Validchar(ptr->player))
				       db[ptr->player].data->player.idletime += (now - ptr->last_time);

				    ptr->warning_level =  0;
				    ptr->descriptor    =  d->descriptor, d->descriptor = -1;
				    ptr->last_time     =  now;
				    ptr->flags        &= ~(DELIVERED|DISCONNECTED);
				    ptr->flags        |=  CONNECTED;
				    server_shutdown_sock(d,0,2), d = ptr;
				    server_clear_textqueue(&(d->input));
				    server_clear_textqueue(&(d->output));

				    /* ---->  Leave AFK mode  <---- */
				    if(d->afk_message) {
				       FREENULL(d->afk_message);
				       d->flags2 &= ~(SENT_AUTO_AFK);
				       d->flags  &= ~(SUPPRESS_ECHO);
				       d->clevel = 0;
				    }

				    /* ---->  Change preferences?  <---- */
				    if(html_lookup("PREFERENCES",0,0,0,0,1)) {
				       const char *background;

				       d->html->flags &= ~HTML_PREFERENCE_FLAGS;
				       html_preferences_lookup(d);
				       if((d->html->flags & HTML_BGRND) && (background = html_lookup("IMAGE",1,1,1,1,1)) && !Blank(background) && strcasecmp(background,html_image_url("default.gif")))
					  d->html->background = (char *) alloc_string(compress(background,0));
				       html_store_preferences(d->player,d);
				    } else html_recall_preferences(d->player,d);

				    /* ---->  Initialise HTML interface  <---- */
				    d->html->flags |= HTML_OUTPUT;
				    http_header(d,1,200,"OK","text/html",now,now,0,0), header = 0;
				    html_header(d,1,HTML_TITLE,0,NULL,NULL,NULL,NULL,NULL,NULL,NULL,1,1);
				    d->flags |= PROCESSED;

				    /* ---->  Connect character to TCZ  <---- */
				    if(Validchar(d->player)) {
				       db[d->player].flags2 |= HTML;
				       if(!Connected(d->player)) {
					  if(html_lookup("CREATED",0,0,0,0,1)) created = 1;
					  if(created) {

					     /* ---->  Create new character  <---- */
					     writelog(HTML_LOG,1,"OUTPUT","Connection Accepted:  %s(#%d) on descriptor %d (%s) (New character created.)",getname(d->player),d->player,d->descriptor,d->hostname);
					     if(d->site) d->site->created++;
					     stats_tcz_update_record(0,0,1,0,0,0,now);
					     output_listen(d,2);
					     tcz_connect_character(d,d->player,1);
					     d->next_time = 0 - d->next_time;
					     look_room(d->player,db[d->player].location);
  #ifdef NOTIFY_WELCOME
					     if(!Level4(d->player) && !Experienced(d->player) && !Assistant(d->player))
						admin_welcome_message(d,instring("guest",getname(d->player)));
  #endif
  #ifdef WARN_DUPLICATES
					     check_duplicates(d->player,getfield(d->player,EMAIL),1,1);
  #endif
					     d->flags |= WELCOME;
					  } else {

					     /* ---->  Connect character, taking over existing connection  <---- */
					     writelog(HTML_LOG,1,"OUTPUT","Connection Accepted:  %s(#%d) on descriptor %d (%s) (Previous connection taken over.)",getname(d->player),d->player,d->descriptor,d->hostname);
					     d->clevel = (db[d->player].data->player.totaltime == 0) ? 24:0;
					     if(d->site) d->site->connected++;
					     stats_tcz_update_record(0,1,0,0,0,0,now);
					     output_listen(d,1);
					     look_room(d->player,db[d->player].location);
					     tcz_connect_character(d,d->player,0);
					     d->next_time = 0 - d->next_time;
					     birthday_notify(now,d->player);
					  }
					  server_connect_peaktotal();

					  /* ---->  User must re-accept terms and conditions of disclaimer  <---- */
					  /* if((d->clevel == 0) && Validchar(d->player) && !option_debug(OPTSTATUS) && (now > (db[d->player].data->player.disclaimertime + (DISCLAIMER_TIME * DAY)))) d->clevel = 29; */

					  prompt_display(d);
				       } else {

					  /* ---->  Connect character without taking over previous connection  <---- */
					  if(taken) writelog(HTML_LOG,1,"OUTPUT","Connection Accepted:  %s(#%d) on descriptor %d (%s) (Previous connection taken over.)",getname(d->player),d->player,d->descriptor,d->hostname);
					     else writelog(HTML_LOG,1,"OUTPUT","Connection Accepted:  %s(#%d) on descriptor %d (%s) (New connection.)",getname(d->player),d->player,d->descriptor,d->hostname);
					  d->flags |= CONNECTED;
					  if(d->pager && d->pager->prompt) {
                                             pager_display(d);
					  } else if(d->edit) {
				  	     strcpy(buffer,".position");
				    	     edit_process_command(d,buffer);
					  } else look_room(d->player,db[d->player].location);

					  /* ---->  User must re-accept terms and conditions of disclaimer  <---- */
					  /* if((d->clevel == 0) && Validchar(d->player) && !option_debug(OPTSTATUS) && (now > (db[d->player].data->player.disclaimertime + (DISCLAIMER_TIME * DAY)))) d->clevel = 29; */

					  prompt_display(d);
				       }
				    } else {
				       writelog(HTML_LOG,1,"OUTPUT","Connection Refused:  Character assigned to descriptor %d (%s) is invalid (#%d).",ptr->descriptor,ptr->hostname,ptr->player);
				       error = 1;
				    }
				 } else {
				    writelog(HTML_LOG,1,"OUTPUT","Access Denied:  Host address of descriptor %d (%s) does not match the host address of descriptor %d (%s) assigned to %s(#%d).",d->descriptor,d->hostname,ptr->descriptor,ptr->hostname,getname(ptr->player),ptr->player);
				    sprintf(buffer,"Sorry, you can't take over a user's World Wide Web Interface connection from a different Internet address.<P>If you have lost your connection and are unable to reconnect because your character is still connected to %s, please connect as a Guest Character and ask an administrator to use the '<B>@bootdead</B>' command to disconnect your 'dead' connection (<B>NOTE:</B> &nbsp; Lost connections will usually time-out and disconnect after <B><I>5 minutes</I></B>.)",tcz_short_name);
				    html_error(d,1,buffer,"CONNECTION REFUSED","Please click on the <B>BACK</B> button of your browser or the button to the left...",html_home_url,(header) ? (HTML_CODE_ERROR|HTML_CODE_HEADER):HTML_CODE_ERROR);
				    error = 2;
				 }
			      } else {
				 writelog(HTML_LOG,1,"OUTPUT","Access Denied:  Invalid authorisation code received from descriptor %d (%s) (CODE=%s&)  -  Code does not match a connected HTML descriptor.",d->descriptor,d->hostname,!Blank(data) ? data:"");
				 error = 1;
			      }
			   } else {
			      writelog(HTML_LOG,1,"OUTPUT","Access Denied:  Invalid authorisation code received from descriptor %d (%s) (CODE=%s&)  -  Blank code or incorrect length.",d->descriptor,d->hostname,!Blank(data) ? data:"");
			      error = 1;
			   }
			} else if(!strcasecmp("CONNECTFORM",data)) { 

			   /* ---->  Connect character form  <---- */
			   writelog(HTML_LOG,1,"CONNECT","Character connection form served to descriptor %d (%s).",d->descriptor,d->hostname);
			   http_header(d,1,200,"OK","text/html",now,now,0,0), header = 0;
			   if(!html_set_preferences(d,html_lookup("ID",1,1,1,1,1)))
			      d->html->flags |= HTML_DEFAULT_FLAGS;
			   html_header(d,1,HTML_TITLE,0,html_image_url("background.gif"),HTML_DBLACK,HTML_DWHITE,HTML_LBLUE,HTML_LCYAN,HTML_DBLUE,"onLoad=\"document.OPTIONS.NAME.focus();\"",0,1);

			   output(d,NOTHING,1,0,0,"Welcome to <I>%s World Wide Web Interface</I>.  If you already have a character, you can connect to %s by filling in the form below with their name and password and then clicking on the <B>CONNECT TO %s</B> button.  If you do not have a character, please click <A HREF=\"%s\" TARGET=_parent>HERE</A> to go to the character creation form, which will allow you to create a new character for yourself to use on %s.<P>",tcz_full_name,tcz_full_name,tcz_short_name,html_server_url(d,0,1,"createform"),tcz_full_name);
			   output(d,NOTHING,1,0,0,"<B><FONT COLOR="HTML_LRED"><BLINK>IMPORTANT:</BLINK></FONT>  If other people have access to the computer you are using (I.e:  You are using a computer in a university computer room), please make sure you clear the <I>disk cache</I> of your browser and then exit it altogether (Close it down) after you have finished using %s.  Please do not leave your computer unattended while you are logged into %s without first typing <FONT COLOR="HTML_DGREEN"><BLINK>AFK</BLINK></FONT> to put you into <I>Away From Keyboard</I> mode.  <I>Failure to do this may result in other people gaining unauthorised access to your character and misusing them.</I><P><FONT SIZE=4>Please click <A HREF=\"%sTOPIC=HTML&\" TARGET=_blank>HERE</A> for further information on <I>%s World Wide Web Interface</I> and how to use it.</B></FONT><P><HR><P>",tcz_full_name,tcz_full_name,html_server_url(d,0,1,"help"),tcz_full_name);
			   output(d,NOTHING,1,0,0,"<FORM NAME=OPTIONS METHOD=POST ACTION=\"%s\" TARGET=_parent><CENTER><B>Name:</B> &nbsp; <INPUT NAME=NAME TYPE=TEXT SIZE=20 MAXLENGTH=128> &nbsp; &nbsp; &nbsp; &nbsp; <B>Password:</B> &nbsp; <INPUT NAME=PASSWORD TYPE=PASSWORD TYPE=TEXT SIZE=20 MAXLENGTH=128><P><INPUT TYPE=SUBMIT VALUE=\"Connect to %s...\"><P><HR><P></CENTER>",html_server_url(d,0,0,NULL),tcz_short_name);
			   html_preferences_form(d,1,0);
			   output(d,NOTHING,1,0,0,"<INPUT NAME=DATA TYPE=HIDDEN VALUE=CONNECT></FORM>");
			   html_preferences_form_java(d);

			   if(!html_lookup("NOBACK",0,0,0,0,1))
                              output(d,NOTHING,1,0,0,"<HR><A HREF=\"%s\" TARGET=_parent><IMG SRC=\"%s\" BORDER=0 ALIGN=MIDDLE ALT=\"[BACK]\"></A> &nbsp; &nbsp; <B>Return to %s web site...</B><HR></BODY></HTML>",html_home_url,html_image_url("back.gif"),tcz_full_name);
			   server_shutdown_sock(d,0,2);
			} else if(!strcasecmp("CREATEFORM",data)) { 
			   unsigned char param,params = 0;
			   const    char *ptr;

			   /* ---->  Create new character form  <---- */
			   writelog(HTML_LOG,1,"CONNECT","Character creation form served to descriptor %d (%s).",d->descriptor,d->hostname);
			   if(html_lookup("PARAMS",0,0,0,0,1)) params = 1;

			   http_header(d,1,200,"OK","text/html",now,now,0,0), header = 0;
			   if(!html_set_preferences(d,html_lookup("ID",1,1,1,1,1)))
			      d->html->flags |= HTML_DEFAULT_FLAGS;
			   html_header(d,1,HTML_TITLE,0,html_image_url("background.gif"),HTML_DBLACK,HTML_DWHITE,HTML_LBLUE,HTML_LCYAN,HTML_DBLUE,"onLoad=\"document.OPTIONS.NAME.focus();\"",0,1);

			   output(d,NOTHING,1,0,0,"Welcome to <I>%s World Wide Web Interface</I>.  Before you can use %s, you will need to create a character for yourself by filling in the form below with their details and then clicking on the <B>CREATE NEW CHARACTER</B> button.  If you already have a character, please click <A HREF=\"%s\" TARGET=_parent>HERE</A> to go to the character connection form, which will allow you to connect your character to %s.<P>",tcz_full_name,tcz_full_name,html_server_url(d,0,1,"connectform"),tcz_full_name);
			   output(d,NOTHING,1,0,0,"<B><FONT COLOR="HTML_LRED"><BLINK>IMPORTANT:</BLINK></FONT>  If other people have access to the computer you are using (I.e:  You are using a computer in a university computer room), please make sure you clear the <I>disk cache</I> of your browser and then exit it altogether (Close it down) after you have finished using %s.  Please do not leave your computer unattended while you are logged into %s without first typing <FONT COLOR="HTML_DGREEN"><BLINK>AFK</BLINK></FONT> to put you into <I>Away From Keyboard</I> mode.  <I>Failure to do this may result in other people gaining unauthorised access to your character and misusing them.</I><P><FONT SIZE=4>Please click <A HREF=\"%sTOPIC=HTML&\" TARGET=_blank>HERE</A> for further information on <I>%s World Wide Web Interface</I> and how to use it.</B></FONT><P><HR><P>",tcz_full_name,tcz_full_name,html_server_url(d,0,1,"help"),tcz_full_name);
			   output(d,NOTHING,1,0,0,"<FORM NAME=OPTIONS METHOD=POST ACTION=\"%s\" TARGET=_parent><CENTER><INPUT NAME=GUEST TYPE=CHECKBOX%s> &nbsp; <B><I>Connect as a Guest character.</I></B><P>",html_server_url(d,0,0,NULL),(params && html_lookup("GUEST",0,0,0,0,1)) ? " CHECKED":"");
			   output(d,NOTHING,1,0,0,"<B>Your preferred name:</B> &nbsp; <INPUT NAME=NAME TYPE=TEXT SIZE=20 MAXLENGTH=20 VALUE=\"%s\"><P><B>Password:</B> &nbsp; <INPUT NAME=PASSWORD TYPE=PASSWORD TYPE=TEXT SIZE=20 MAXLENGTH=32 VALUE=\"%s\"> &nbsp; &nbsp; &nbsp; &nbsp; <B>Verify password:</B> &nbsp; <INPUT NAME=VERIFY TYPE=PASSWORD TYPE=TEXT SIZE=20 MAXLENGTH=32 VALUE=\"%s\"><BR><I>(Please type your preferred password in <B>BOTH</B> of the above boxes (For verification.))</I><P>",(params && (data = html_lookup("NAME",1,1,1,1,1))) ? data:"",(params && (data = html_lookup("PASSWORD",1,1,1,1,1))) ? data:"",(params && (data = html_lookup("VERIFY",1,1,1,1,1))) ? data:"");
			   if(params && (data = html_lookup("GENDER",1,1,1,1,1))) {
			      if(!strcasecmp(data,"MALE")) param = 1;
				 else if(!strcasecmp(data,"FEMALE")) param = 2;
				    else if(!strcasecmp(data,"NEUTER")) param = 3;
				       else param = 0;
			   } else param = 0;
			   output(d,NOTHING,1,0,0,"<B>Gender:</B> &nbsp; <INPUT NAME=GENDER TYPE=RADIO VALUE=MALE%s> &nbsp; Male, &nbsp; <INPUT NAME=GENDER TYPE=RADIO VALUE=FEMALE%s> &nbsp; Female, &nbsp; <INPUT NAME=GENDER TYPE=RADIO VALUE=NEUTER%s> &nbsp; Neuter.<P><HR><FONT SIZE=6><B><I>Disclaimer (Please read carefully)...</I></B></FONT><HR><P>",(!param || (params && (param == 1))) ? " CHECKED":"",(params && (param == 2)) ? " CHECKED":"",(params && (param == 3)) ? " CHECKED":"");
			   if(disclaimer) {
			      for(ptr = decompress(disclaimer); *ptr && (*ptr == '\n'); ptr++);
			   } else ptr = "Sorry, the disclaimer isn't available at the moment.\n";

			   output(d,NOTHING,2,0,0,"\016<TABLE BORDER=5 CELLPADDING=4 BGCOLOR=#FFFFDD><TR><TD><FONT COLOR="HTML_DRED"><TT><B>\016%s\016</B></TT></TD></TR></TABLE>\016",ptr);
			   output(d,NOTHING,1,0,0,"<INPUT NAME=DISCLAIMER TYPE=CHECKBOX%s> &nbsp; <B>Check this box if you accept the above terms and conditions.</B></CENTER><P><HR><P><B>Your character's race:</B> &nbsp; <INPUT NAME=RACE TYPE=TEXT SIZE=50 MAXLENGTH=50 VALUE=\"%s\"><BR><B>Your full E-mail address:</B> &nbsp; <INPUT NAME=EMAIL TYPE=TEXT SIZE=50 MAXLENGTH=128 VALUE=\"%s\"><BR><B>Time difference from <I>%s (%s)</I>:</B> &nbsp; <INPUT NAME=TIMEDIFF TYPE=TEXT SIZE=3 MAXLENGTH=3 VALUE=\"%s\"> hours.<P><HR><P><CENTER><INPUT TYPE=SUBMIT VALUE=\"Create new character...\"><P><HR><P></CENTER>",(params && (data = html_lookup("DISCLAIMER",0,0,0,0,1))) ? " CHECKED":"",(params && (data = html_lookup("RACE",1,1,1,1,1))) ? data:"Human",(params && (data = html_lookup("EMAIL",1,1,1,1,1))) ? data:"",date_to_string(now,UNSET_DATE,NOTHING,TIMEFMT),tcz_timezone,(params && (data = html_lookup("TIMEDIFF",1,1,1,1,1))) ? data:"0");
			   html_preferences_form(d,0,0);
			   output(d,NOTHING,1,0,0,"<INPUT NAME=DATA TYPE=HIDDEN VALUE=CREATE></FORM>");
			   html_preferences_form_java(d);

			   if(!html_lookup("NOBACK",0,0,0,0,1))
                              output(d,NOTHING,1,0,0,"<HR><A HREF=\"%s\" TARGET=_parent><IMG SRC=\"%s\" BORDER=0 ALIGN=MIDDLE ALT=\"[BACK]\"></A> &nbsp; &nbsp; <B>Return to %s web site...</B><HR></BODY></HTML>",html_home_url,html_image_url("back.gif"),tcz_full_name);
			   server_shutdown_sock(d,0,2);
			} else if((!(temp = 0) && !strcasecmp("HELP",data)) || ((temp = 1) && !strcasecmp("TUTORIAL",data))) {
			   const char *topic = html_lookup("TOPIC",1,1,1,1,1);
			   char  buffer[TEXT_SIZE],buffer2[TEXT_SIZE];

			   /* ---->  On-line Help & Tutorials  <---- */
			   command_type |= (HTML_ACCESS|NO_FLUSH_OUTPUT);
			   writelog(HTML_LOG,1,(temp) ? "TUTORIAL":"HELP","%s topic '%s' served to descriptor %d (%s).",(temp) ? "Tutorial":"Help",!Blank(topic) ? topic:"",d->descriptor,d->hostname);
			   http_header(d,1,200,"OK","text/html",now - DAY,now + DAY,1,0), header = 0;
			   if(!html_set_preferences(d,html_lookup("ID",1,1,1,1,1)))
			      d->html->flags |= HTML_DEFAULT_FLAGS;
			   sprintf(buffer2,(temp) ? "%s On-line Tutorials":"%s On-line Help System",tcz_full_name);
			   html_header(d,1,buffer2,0,NULL,NULL,NULL,NULL,NULL,NULL,NULL,0,0);

			   d->player     = NOBODY;
			   strncpy(buffer,!Blank(topic) ? topic:"",127);
			   help_main(NOBODY,buffer,NULL,NULL,NULL,temp,0);
			   output(d,NOTHING,1,0,0,"</BODY></HTML>");
			   d->player = NOTHING;
			   server_shutdown_sock(d,0,2);
			   command_type &= ~(HTML_ACCESS|NO_FLUSH_OUTPUT);
			} else if(!strcasecmp("SEARCH",data)) {
			   const    char *data,*topic = html_lookup("TOPIC",1,1,1,1,1);
			   char          buffer2[TEXT_SIZE];
			   unsigned char help = 1;
			   int           page = 0;

			   /* ---->  On-line Help/Tutorial Search  <---- */
			   command_type |= (HTML_ACCESS|NO_FLUSH_OUTPUT);
			   if((data  = html_lookup("PAGE",1,1,1,1,1))) page = atol(data);
			   if((data = html_lookup("SEARCHMODE",1,1,1,1,1)) && !strcasecmp("TUTORIAL",data)) help = 0;
			   if(Blank(topic)) writelog(HTML_LOG,1,"SEARCH","%s search form served to descriptor %d (%s).",(help) ? "Help":"Tutorial",d->descriptor,d->hostname);
			      else writelog(HTML_LOG,1,"SEARCH","%s search results for topic '%s' served to descriptor %d (%s).",(help) ? "Help":"Tutorial",topic,d->descriptor,d->hostname);

			   http_header(d,1,200,"OK","text/html",now - DAY,now + DAY,1,0), header = 0;
			   if(!html_set_preferences(d,html_lookup("ID",1,1,1,1,1)))
			      d->html->flags |= HTML_DEFAULT_FLAGS;
			   sprintf(buffer2,(temp) ? "%s On-line Tutorials":"%s On-line Help System",tcz_full_name);
			   html_header(d,1,buffer2,0,NULL,NULL,NULL,NULL,NULL,NULL,(topic) ? "onLoad=\"document.SEARCHFORM.TOPIC.focus(); document.SEARCHFORM.TOPIC.select();\"":"onLoad=\"document.SEARCHFORM.TOPIC.focus();\"",0,0);

			   help_search(d,(char *) topic,page,help);
			   server_shutdown_sock(d,0,2);
			   command_type &= ~(HTML_ACCESS|NO_FLUSH_OUTPUT);
			} else if(!strcasecmp("USERLIST",data) || !strcasecmp("WHOLIST",data)) {
			   struct descriptor_data *ptr;
			   char   buffer2[TEXT_SIZE];
			   short  count = 0;

			   /* ---->  Connected User List  <---- */
			   command_type |= NO_FLUSH_OUTPUT;
			   writelog(HTML_LOG,1,"USERLIST","Connected user list served to descriptor %d (%s).",d->descriptor,d->hostname);
			   http_header(d,1,200,"OK","text/html",now,now,1,0), header = 0;
			   if(!html_set_preferences(d,html_lookup("ID",1,1,1,1,1)))
			      d->html->flags |= HTML_DEFAULT_FLAGS;
			   sprintf(buffer2,"People currently connected to %s...",tcz_full_name);
			   html_header(d,1,buffer2,HTML_USERLIST_REFRESH,NULL,NULL,NULL,NULL,NULL,NULL,NULL,0,0);

			   output(d,NOTHING,1,0,0,"<CENTER><TABLE BORDER WIDTH=100%% BGCOLOR="HTML_TABLE_GREEN"><TR><TH><FONT SIZE=6 COLOR="HTML_LGREEN"><B><I>The following people are currently connected to %s...</I></B></FONT></TH></TR></TABLE></CENTER>",tcz_full_name);
			   parse_grouprange(NOTHING,"",ALL,1);
			   set_conditions(NOTHING,0,0,NOTHING,NOTHING,NULL,210);
			   for(ptr = descriptor_list; ptr; ptr = ptr->next)
			       if((ptr->flags & CONNECTED) && Validchar(ptr->player)) count++;
			   if(count > 50) userlist_swho(d);
			      else userlist_who(d);

			   if(!html_lookup("NOBACK",0,0,0,0,1))
                              output(d,NOTHING,1,0,0,"<HR><A HREF=\"%s\"><IMG SRC=\"%s\" ALIGN=MIDDLE BORDER=0 ALT=\"[BACK]\"></A> &nbsp; &nbsp; <FONT SIZE=4><B>Return to %s web site...</B></FONT><HR></BODY></HTML>",html_home_url,html_image_url("back.gif"),tcz_full_name);
			   server_shutdown_sock(d,0,2);
			   command_type &= ~NO_FLUSH_OUTPUT;
			} else if(!strcasecmp("REQUEST",data)) {
			   const char *email = html_lookup("EMAIL",1,1,1,1,1);
			   const char *name  = html_lookup("NAME",1,1,1,1,1);
			   char  buffer[TEXT_SIZE];

			   /* ---->  New character request  <---- */
			   if(!Blank(email)) {
			      if(strlen(email) <= 128) {
				 if(!Blank(name)) {
				    if(strlen(name) <= 20) {
				       char   buf[2048];
				       time_t rtime;

				       strcpy(buf,email);
				       if(!(rtime = request_add(buf,(char *) name,d->address,d->hostname,now,NOTHING,0))) {
					  writelog(HTML_LOG,1,"REQUEST","Request Accepted:  New character request for '%s' (%s) received from descriptor %d (%s)  -  Added to request queue.",!Blank(name) ? name:"Unknown",!Blank(email) ? email:"Unknown",d->descriptor,d->hostname);
					  sprintf(buf,"Your request for a new character ('<B>%s</B>') has been added to the queue of requests.<P>We will be in touch with you shortly via the E-mail address you gave us (<B>%s</B>) &nbsp; Please ensure that you submitted your correct and valid E-mail address, otherwise we will not be able to contact you.<P>If you do not hear from us within the next few days, please connect again and make a new request, ensuring that you enter your correct E-mail address.<P>If you experience any difficulties, please send E-mail to <A HREF=\"mailto:%s\">%s</A>.",name,email,tcz_admin_email,tcz_admin_email);
					  html_error(d,1,buf,"NEW CHARACTER REQUEST","Back to the character creation form...",html_server_url(d,0,1,"createform"),HTML_CODE_ERROR|HTML_CODE_HEADER);
				       } else {
					  writelog(HTML_LOG,1,"REQUEST","Request Denied:  New character request for '%s' (%s) received from descriptor %d (%s)  -  Character with same name already exists.",!Blank(name) ? name:"Unknown",!Blank(email) ? email:"Unknown",d->descriptor,d->hostname);
					  sprintf(buf,"Sorry, we already have a request with your E-mail address (<A>%s</A>) in the queue of requests for new characters.  This request was made on <I>%s</I>.<P>Please ensure that the E-mail address you gave is correct and valid, otherwise we will not be able to contact you.<P>Please click on the <B>BACK</B> button of your browser to return to the previous page and re-enter your preferred character name and your E-mail address.<P>If you experience any difficulties, please send E-mail to <A HREF=\"mailto:%s\">%s</A>.",email,date_to_string(rtime,UNSET_DATE,NOTHING,FULLDATEFMT),tcz_admin_email,tcz_admin_email);
					  html_error(d,1,buf,"NEW CHARACTER REQUEST","Back to the character creation form...",html_server_url(d,0,1,"createform"),HTML_CODE_ERROR|HTML_CODE_HEADER);
				       }
				    } else {
				       writelog(HTML_LOG,1,"REQUEST","Request Denied:  New character request received from descriptor %d (%s)  -  Preferred character name is too long.",d->descriptor,d->hostname);
				       sprintf(buffer,"Sorry, your preferred character name is too long  -  It must be 20 characters or less in length.<P>Please click on the <B>BACK</B> button of your browser to return to the previous page and re-enter your preferred character name and your E-mail address.<P>If you experience any difficulties, please send E-mail to <A HREF=\"mailto:%s\">%s</A>.",tcz_admin_email,tcz_admin_email);
				       html_error(d,1,buffer,"NEW CHARACTER REQUEST","Back to the character creation form...",html_server_url(d,0,1,"createform"),HTML_CODE_ERROR|HTML_CODE_HEADER);
				    }
				 } else {
				    writelog(HTML_LOG,1,"REQUEST","Request Denied:  New character request received from descriptor %d (%s)  -  Preferred character name not specified.",d->descriptor,d->hostname);
				    sprintf(buffer,"Sorry, you must enter your preferred character name to make a request for a new character.<P>Please click on the <B>BACK</B> button of your browser to return to the previous page and re-enter your preferred character name and your E-mail address.<P>If you experience any difficulties, please send E-mail to <A HREF=\"mailto:%s\">%s</A>.",tcz_admin_email,tcz_admin_email);
				    html_error(d,1,buffer,"NEW CHARACTER REQUEST","Back to the character creation form...",html_server_url(d,0,1,"createform"),HTML_CODE_ERROR|HTML_CODE_HEADER);
				 }
			      } else {
				 writelog(HTML_LOG,1,"REQUEST","Request Denied:  New character request for '%s' received from descriptor %d (%s)  -  E-mail address specified is too long.",!Blank(name) ? name:"Unknown",d->descriptor,d->hostname);
				 sprintf(buffer,"Sorry, that E-mail address is too long  -  It must be under 128 characters in length.<P>Please click on the <B>BACK</B> button of your browser to return to the previous page and re-enter your preferred character name and your E-mail address.<P>If you experience any difficulties, please send E-mail to <A HREF=\"mailto:%s\">%s</A>.",tcz_admin_email,tcz_admin_email);
				 html_error(d,1,buffer,"NEW CHARACTER REQUEST","Back to the character creation form...",html_server_url(d,0,1,"createform"),HTML_CODE_ERROR|HTML_CODE_HEADER);
			      }
			   } else {
			      writelog(HTML_LOG,1,"REQUEST","Request Denied:  New character request for '%s' received from descriptor %d (%s)  -  No E-mail address specified.",!Blank(name) ? name:"Unknown",d->descriptor,d->hostname);
			      sprintf(buffer,"Sorry, you must enter your full E-mail address to make a request for a new character.<P>Please click on the <B>BACK</B> button of your browser to return to the previous page and re-enter your preferred character name and your E-mail address.<P>If you experience any difficulties, please send E-mail to <A HREF=\"mailto:%s\">%s</A>.",tcz_admin_email,tcz_admin_email);
			      html_error(d,1,buffer,"NEW CHARACTER REQUEST","Back to the character creation form...",html_server_url(d,0,1,"createform"),HTML_CODE_ERROR|HTML_CODE_HEADER);
			   }
			   output(d,NOTHING,1,0,0,"%s",buffer);
			   server_shutdown_sock(d,0,2);
			} else if((!strcasecmp("HOMEPAGES",data) && !(temp = 0)) || (!strcasecmp("GALLERY",data) && (temp = 1)) || (!strcasecmp("SCANS",data) && (temp = 2)) || (!strcasecmp("PROFILES",data) && (temp = 3))) {

			   /* ---->  User Home Pages  <---- */
			   writelog(HTML_LOG,1,(temp == 3) ? "PROFILE":(temp == 2) ? "SCAN":(temp == 1) ? "GALLERY":"HOMEPAGE","List of users with %s served to descriptor %d (%s).",(temp == 3) ? "profiles":(temp == 2) ? "scans":(temp == 1) ? "pictures":"home pages",d->descriptor,d->hostname);
			   command_type   |= (NO_FLUSH_OUTPUT|NO_USAGE_UPDATE);
			   http_header(d,1,200,"OK","text/html",now,now,1,0), header = 0;
			   if(!html_set_preferences(d,html_lookup("ID",1,1,1,1,1)))
			      d->html->flags |= HTML_DEFAULT_FLAGS;

			   html_userlist(d,html_lookup("SECTION",1,1,1,1,1),temp);
			   command_type &= ~(NO_FLUSH_OUTPUT|NO_USAGE_UPDATE);
			   server_shutdown_sock(d,0,2);
			} else if(!strcasecmp("PICTURE",data)) {

			   /* ---->  User Pictures  <---- */
			   char  buffer[TEXT_SIZE + KB],buffer2[TEXT_SIZE + KB],backurl[TEXT_SIZE];
			   const char *section,*name,*url,*ptr;
			   int   copied;
			   dbref user;

			   section = html_lookup("SECTION",1,1,1,1,1);
			   sprintf(backurl,"%sSECTION=%s&",html_server_url(d,0,1,"gallery"),!Blank(section) ? section:"");

			   if((user = lookup_character(NOTHING,name = html_lookup("NAME",1,1,1,1,1),2)) != NOTHING) {
			      if(hasprofile(db[user].data->player.profile) && !Blank(db[user].data->player.profile->picture)) {
				 command_type |= NO_USAGE_UPDATE;
				 writelog(HTML_LOG,1,"PICTURE","Picture (URL) of %s(#%d) served to descriptor %d (%s).",getname(user),user,d->descriptor,d->hostname);
				 http_header(d,1,200,"OK","text/html",now,now,1,0), header = 0;
				 if(!html_set_preferences(d,html_lookup("ID",1,1,1,1,1)))
				    d->html->flags |= HTML_DEFAULT_FLAGS;
				 sprintf(buffer2,"%s%s's Picture...",Article(UPPER,DEFINITE,user),getcname(NOTHING,user,0,0));
				 html_header(d,1,buffer2,0,NULL,NULL,NULL,NULL,NULL,NULL,NULL,0,0);

				 output(d,NOTHING,1,0,0,"<HR><CENTER><FONT SIZE=6 COLOR="HTML_LGREEN"><B><I>%s<FONT COLOR="HTML_LWHITE">%s</FONT>'s Picture...</I></B></FONT></CENTER><HR>",Article(UPPER,DEFINITE,user),getcname(NOTHING,user,0,0));
				 output(d,NOTHING,1,0,0,"<CENTER><P><TABLE BORDER=7><TR><TD ALIGN=CENTER VALIGN=CENTER><A HREF=\"%s\"><IMG SRC=\"%s\" BORDER=0 ALT=\"%s User Picture\"></A></TD></TR></TABLE><P>",html_encode_basic(url = decompress(db[user].data->player.profile->picture),scratch_return_string,&copied,512),scratch_return_string,tcz_short_name);
				 output(d,NOTHING,1,0,0,"<A HREF=\"%s\"><FONT SIZE=2>%s</FONT></A><P>&nbsp;<P>",html_encode_basic(decompress(db[user].data->player.profile->picture),scratch_return_string,&copied,512),url);
				 output(d,NOTHING,1,0,0,"<TABLE BORDER CELLPADDING=4 BGCOLOR="HTML_LBLACK"><TR ALIGN=CENTER VALIGN=CENTER>");
				 if((ptr = getfield(user,WWW))) output(d,NOTHING,1,0,0,"<TD><A HREF=\"%s%s\"><IMG SRC=\"%s\" BORDER=0 ALT=\"[HOMEPAGE]\"></A></TD>",!strncasecmp(ptr,"http://",7) ? "":"http://",html_encode_basic(ptr,scratch_return_string,&copied,512),html_image_url("homepage.gif"));
				    else output(d,NOTHING,1,0,0,"<TD><IMG SRC=\"%s\" ALT=\"{HOMEPAGE}\"></TD>",html_image_url("nohomepage.gif"));
				 output(d,NOTHING,1,0,0,"<P><TD><IMG SRC=\"%s\" BORDER=0 ALT=\"{PICTURE}\"></TD>",html_image_url("nopicture.gif"));
				 output(d,NOTHING,1,0,0,"<TD><A HREF=\"%sNAME=*%s&\"><IMG SRC=\"%s\" BORDER=0 ALT=\"[PROFILE]\"></A></TD>",html_server_url(d,0,1,"profile"),html_encode(getname(user),buffer,&copied,256),html_image_url("profile.gif"));
				 output(d,NOTHING,1,0,0,"<TD><A HREF=\"%sNAME=*%s&\"><IMG SRC=\"%s\" BORDER=0 ALT=\"[SCAN]\"></A></TD>",html_server_url(d,0,1,"scan"),html_encode(getname(user),buffer,&copied,256),html_image_url("scan.gif"));
				 output(d,NOTHING,1,0,0,"</TR></TABLE></CENTER><P><HR><TABLE BORDER WIDTH=100%% BGCOLOR="HTML_TABLE_BLUE"><TR><TD><I>If you can't see the above picture, please check that the downloading of images in your web browser is enabled.  If it is, and there is still no picture above, the user has not set the URL to their picture correctly.</I></TD></TR></TABLE><HR>");

        			 if(!html_lookup("NOBACK",0,0,0,0,1))
				    output(d,NOTHING,1,0,0,"<A HREF=\"%s\"><IMG SRC=\"%s\" ALIGN=MIDDLE BORDER=0 ALT=\"[BACK]\"></A> &nbsp; &nbsp; <FONT SIZE=4><B>Back to the picture gallery page...</B></FONT><HR></BODY></HTML>",backurl,html_image_url("back.gif"));
				 command_type &= ~NO_USAGE_UPDATE;
				 server_shutdown_sock(d,0,2);
			      } else {
				 writelog(HTML_LOG,1,"PICTURE","Error:  Unable to serve picture of %s(#%d) to descriptor %d (%s)  -  Character does not have the URL of their picture set.",getname(user),user,d->descriptor,d->hostname);
				 sprintf(buffer,"Sorry, %s<B>%s</B> doesn't have a picture in the gallery.",Article(LOWER,DEFINITE,user),getcname(NOTHING,user,0,0));
				 html_error(d,1,buffer,"ERROR","Please click on the <B>BACK</B> button of your browser or the button to the left to return to the gallery...",backurl,(header) ? (HTML_CODE_ERROR|HTML_CODE_HEADER):HTML_CODE_ERROR);
				 error = 2;
			      }
			   } else {
			      writelog(HTML_LOG,1,"PICTURE","Error:  Unable to serve picture of '%s' to descriptor %d (%s)  -  Character does not exist.",!Blank(name) ? name:"Unknown",d->descriptor,d->hostname);
			      sprintf(buffer,"Sorry, the character '<B>%s</B>' doesn't exist.",!Blank(name) ? name:"Unknown");
			      html_error(d,1,buffer,"ERROR","Please click on the <B>BACK</B> button of your browser or the button to the left to return to the gallery...",backurl,(header) ? (HTML_CODE_ERROR|HTML_CODE_HEADER):HTML_CODE_ERROR);
			      error = 2;
			   }
			} else if((!strcasecmp("PROFILE",data) && !(temp = 0)) || (!strcasecmp("SCAN",data) && (temp = 1))) {

			   /* ---->  User Profiles/Scans  <---- */
			   char  buffer[TEXT_SIZE + KB],buffer2[TEXT_SIZE + KB],backurl[TEXT_SIZE];
			   const char *section,*name,*ptr;
			   int   copied;
			   dbref user;

			   section = html_lookup("SECTION",1,1,1,1,1);
			   sprintf(backurl,"%sSECTION=%s&",html_server_url(d,0,1,(!temp) ? "profiles":"scans"),!Blank(section) ? section:"");

			   if((user = lookup_character(NOTHING,name = html_lookup("NAME",1,1,1,1,1),2)) != NOTHING) {
			      if(temp || hasprofile(db[user].data->player.profile)) {
				 command_type |= NO_FLUSH_OUTPUT;
				 writelog(HTML_LOG,1,(temp) ? "SCAN":"PROFILE","%s of %s(#%d) served to descriptor %d (%s).",(temp) ? "Scan":"Profile",getname(user),user,d->descriptor,d->hostname);
				 http_header(d,1,200,"OK","text/html",now,now,1,0), header = 0;
				 if(!html_set_preferences(d,html_lookup("ID",1,1,1,1,1)))
				    d->html->flags |= HTML_DEFAULT_FLAGS;
				 sprintf(buffer2,"%s%s's %s...",Article(UPPER,DEFINITE,user),getcname(NOTHING,user,0,0),(!temp) ? "Profile":"Scan");
				 html_header(d,1,buffer2,0,NULL,NULL,NULL,NULL,NULL,NULL,NULL,0,0);

				 output(d,NOTHING,1,0,0,"<TABLE BORDER WIDTH=100%% CELLPADDING=5 BGCOLOR=%s><TR ALIGN=CENTER><TD><FONT SIZE=6 COLOR=%s><B><I>%s<FONT COLOR="HTML_LWHITE">%s</FONT>'s %s...</I></B></FONT></TD></TR></TABLE>",(!temp) ? HTML_TABLE_MAGENTA:HTML_TABLE_CYAN,(!temp) ? HTML_LMAGENTA:HTML_LCYAN,Article(UPPER,DEFINITE,user),getcname(NOTHING,user,0,0),(!temp) ? "Profile":"Scan");
				 d->player = NOBODY;
				 d->html->flags |= HTML_SIMULATE_ANSI;
				 if(!temp) look_profile(NOBODY,NULL,NULL,(char *) name,NULL,0,0);
				    else look_scan(NOBODY,NULL,NULL,(char *) name,NULL,2,0);
				 d->player = NOTHING;

				 output(d,NOTHING,1,0,0,"<P><CENTER><TABLE BORDER CELLPADDING=4 BGCOLOR="HTML_LBLACK"><TR ALIGN=CENTER VALIGN=CENTER>");
				 if((ptr = getfield(user,WWW))) output(d,NOTHING,1,0,0,"<TD><A HREF=\"%s%s\"><IMG SRC=\"%s\" BORDER=0 ALT=\"[HOMEPAGE]\"></A></TD>",!strncasecmp(ptr,"http://",7) ? "":"http://",html_encode_basic(ptr,scratch_return_string,&copied,512),html_image_url("homepage.gif"));
				    else output(d,NOTHING,1,0,0,"<TD><IMG SRC=\"%s\" ALT=\"{HOMEPAGE}\"></TD>",html_image_url("nohomepage.gif"));
				 if(hasprofile(db[user].data->player.profile) && !Blank(db[user].data->player.profile->picture))
				    output(d,NOTHING,1,0,0,"<TD><A HREF=\"%sNAME=*%s&\"><IMG SRC=\"%s\" BORDER=0 ALT=\"[PICTURE]\"></A></TD>",html_server_url(d,0,1,"picture"),html_encode(getname(user),buffer,&copied,256),html_image_url("picture.gif"));
				       else output(d,NOTHING,1,0,0,"<TD><IMG SRC=\"%s\" BORDER=0 ALT=\"{PICTURE}\"></TD>",html_image_url("nopicture.gif"));
				 if(!temp) output(d,NOTHING,1,0,0,"<TD><A HREF=\"%sNAME=*%s&\"><IMG SRC=\"%s\" BORDER=0 ALT=\"[SCAN]\"></A></TD>",html_server_url(d,0,1,"scan"),html_encode(getname(user),buffer,&copied,256),html_image_url("scan.gif"));
				    else output(d,NOTHING,1,0,0,"<TD><IMG SRC=\"%s\" BORDER=0 ALT=\"{SCAN}\"></TD>",html_image_url("noscan.gif"));
				 if(temp && hasprofile(db[user].data->player.profile))
				    output(d,NOTHING,1,0,0,"<TD><A HREF=\"%sNAME=*%s&\"><IMG SRC=\"%s\" BORDER=0 ALT=\"[PROFILE]\"></A></TD>",html_server_url(d,0,1,"profile"),html_encode(getname(user),buffer,&copied,256),html_image_url("profile.gif"));
				       else output(d,NOTHING,1,0,0,"<TD><IMG SRC=\"%s\" BORDER=0 ALT=\"{PROFILE}\"></TD>",html_image_url("noprofile.gif"));

      			         if(!html_lookup("NOBACK",0,0,0,0,1))
				    output(d,NOTHING,1,0,0,"</TR></TABLE></CENTER><P><HR><A HREF=\"%s\"><IMG SRC=\"%s\" ALIGN=MIDDLE BORDER=0 ALT=\"[BACK]\"></A> &nbsp; &nbsp; <FONT SIZE=4><B>Back to the %s page...</B></FONT><HR></BODY></HTML>",backurl,html_image_url("back.gif"),(!temp) ? "profiles":"scans");
				 command_type &= ~NO_FLUSH_OUTPUT;
				 server_shutdown_sock(d,0,2);
			      } else {
				 writelog(HTML_LOG,1,(temp) ? "SCAN":"PROFILE","Error:  Unable to serve %s of %s(#%d) to descriptor %d (%s)  -  Character does not have a %s.",(temp) ? "scan":"profile",getname(user),user,d->descriptor,d->hostname,(temp) ? "scan":"profile");
				 sprintf(buffer,"Sorry, %s<B>%s</B> doesn't have a %s.",Article(LOWER,DEFINITE,user),getcname(NOTHING,user,0,0),(temp) ? "scan":"profile");
				 html_error(d,1,buffer,"ERROR","Please click on the <B>BACK</B> button of your browser or the button to the left to return to the gallery...",backurl,(header) ? (HTML_CODE_ERROR|HTML_CODE_HEADER):HTML_CODE_ERROR);
				 error = 2;
			      }
			   } else {
			      writelog(HTML_LOG,1,(temp) ? "SCAN":"PROFILE","Error:  Unable to serve %s of '%s' to descriptor %d (%s)  -  Character does not exist.",(temp) ? "scan":"profile",!Blank(name) ? name:"Unknown",d->descriptor,d->hostname);
			      sprintf(buffer,"Sorry, the character '<B>%s</B>' doesn't exist.",!Blank(name) ? name:"Unknown");
			      html_error(d,1,buffer,"ERROR","Please click on the <B>BACK</B> button of your browser or the button to the left to return to the gallery...",backurl,(header) ? (HTML_CODE_ERROR|HTML_CODE_HEADER):HTML_CODE_ERROR);
			      error = 2;
			   }
			} else if(!strcasecmp("MAP",data)) {
			   char buffer2[TEXT_SIZE];

			   /* ---->  Map of TCZ  <---- */
			   writelog(HTML_LOG,1,"MAP","Map of %s served to descriptor %d (%s).",tcz_short_name,d->descriptor,d->hostname);
			   http_header(d,1,200,"OK","text/html",now - WEEK,now + WEEK,1,0), header = 0;
			   if(!html_set_preferences(d,html_lookup("ID",1,1,1,1,1)))
			      d->html->flags |= HTML_DEFAULT_FLAGS;
			   sprintf(buffer2,"%s Map",tcz_full_name);
			   html_header(d,1,buffer2,0,NULL,NULL,NULL,NULL,NULL,NULL,NULL,0,0);

			   map_html(d);
			   output(d,NOTHING,1,0,0,"</BODY></HTML>");
			   server_shutdown_sock(d,0,2);
			} else if(!strcasecmp("ADMINLIST",data)) {
			   char buffer2[TEXT_SIZE];

			   /* ---->  Current administrators list  <---- */
			   writelog(HTML_LOG,1,"ADMINLIST","List of current %s administrators served to descriptor %d (%s).",tcz_short_name,d->descriptor,d->hostname);
			   http_header(d,1,200,"OK","text/html",now,now,1,0), header = 0;
			   if(!html_set_preferences(d,html_lookup("ID",1,1,1,1,1)))
			      d->html->flags |= HTML_DEFAULT_FLAGS;
			   sprintf(buffer2,"Administrators, Experienced Builders and Assistants of %s...",tcz_full_name);
			   html_header(d,1,buffer2,0,NULL,NULL,NULL,NULL,NULL,NULL,NULL,0,0);

			   parse_grouprange(NOTHING,"",ALL,1);
			   set_conditions(NOTHING,0,0,NOTHING,NOTHING,NULL,210);
			   command_type |= NO_FLUSH_OUTPUT;
			   admin_list_all(d,NOTHING);

			   if(!html_lookup("NOBACK",0,0,0,0,1))
  			      output(d,NOTHING,1,0,0,"<HR><A HREF=\"%s\"><IMG SRC=\"%s\" ALIGN=MIDDLE BORDER=0 ALT=\"[BACK]\"></A> &nbsp; &nbsp; <FONT SIZE=4 COLOR="HTML_LWHITE"><B>Return to %s web site...</B></FONT><HR></BODY></HTML>",html_home_url,html_image_url("back.gif"),tcz_full_name);
			   server_shutdown_sock(d,0,2);
			   command_type &= ~NO_FLUSH_OUTPUT;
			} else if((!(temp = 0) && !strcasecmp("MODULES",data)) || ((temp = 1) && !strcasecmp("AUTHORS",data))) {
			   char buffer2[TEXT_SIZE];

			   /* ---->  Modules / Authors List  <---- */
			   command_type |= (HTML_ACCESS|NO_FLUSH_OUTPUT);
			   writelog(HTML_LOG,1,(temp) ? "AUTHORS":"MODULES","%s list served to descriptor %d (%s).",(temp) ? "Authors":"Modules",d->descriptor,d->hostname);
			   http_header(d,1,200,"OK","text/html",now,now,1,0), header = 0;
			   if(!html_set_preferences(d,html_lookup("ID",1,1,1,1,1)))
			      d->html->flags |= HTML_DEFAULT_FLAGS;
			   sprintf(buffer2,(temp) ? "%s Source Code Authors List":"%s Source Code Modules List",tcz_short_name);
			   html_header(d,1,buffer2,0,NULL,NULL,NULL,NULL,NULL,NULL,NULL,0,0);

			   d->player     = NOBODY;
			   output(d,NOTHING,1,0,0,"<CENTER><TABLE BORDER WIDTH=100%% BGCOLOR=%s><TR><TH><FONT SIZE=6 COLOR=%s><B><I>%s</I></B></FONT></TH></TR></TABLE></CENTER>",(temp) ? HTML_TABLE_RED:HTML_TABLE_MAGENTA,(temp) ? HTML_LRED:HTML_LMAGENTA,buffer2);
			   parse_grouprange(NOTHING,NULL,ALL,1);
			   if(temp) modules_authors_list(NOBODY);
			      else modules_modules_list(NOBODY);

			   if(!html_lookup("NOBACK",0,0,0,0,1))
			      output(d,NOTHING,1,0,0,"<HR><A HREF=\"%s\"><IMG SRC=\"%s\" ALIGN=MIDDLE BORDER=0 ALT=\"[BACK]\"></A> &nbsp; &nbsp; <FONT SIZE=4 COLOR="HTML_LWHITE"><B>Return to %s web site...</B></FONT><HR></BODY></HTML>",html_home_url,html_image_url("back.gif"),tcz_full_name);
			   d->player = NOTHING;
			   server_shutdown_sock(d,0,2);
			   command_type &= ~(HTML_ACCESS|NO_FLUSH_OUTPUT);
			} else if((!(temp = 0) && !strcasecmp("MODULE",data)) || ((temp = 1) && !strcasecmp("AUTHOR",data))) {
			   const char *name = html_lookup("NAME",1,1,1,1,1);
			   char  buffer[TEXT_SIZE],buffer2[TEXT_SIZE];

			   /* ---->  Modules/Authors Information  <---- */
			   http_header(d,1,200,"OK","text/html",now,now,1,0), header = 0;
			   if(!html_set_preferences(d,html_lookup("ID",1,1,1,1,1)))
			      d->html->flags |= HTML_DEFAULT_FLAGS;

			   if(!Blank(name)) {
			      command_type |= (HTML_ACCESS|NO_FLUSH_OUTPUT);
			      writelog(HTML_LOG,1,(temp) ? "AUTHOR":"MODULE","%s '%s' information served to descriptor %d (%s).",(temp) ? "Author":"Module",!Blank(name) ? name:"",d->descriptor,d->hostname);
			      sprintf(buffer2,"%s Source Code Module%s Information",tcz_short_name,(temp) ? " Author":"");
			      html_header(d,1,buffer2,0,NULL,NULL,NULL,NULL,NULL,NULL,NULL,0,0);

			      d->player = NOBODY;
			      output(d,NOTHING,1,0,0,"<CENTER><TABLE BORDER WIDTH=100%% BGCOLOR=%s><TR><TH><FONT SIZE=6 COLOR=%s><B><I>%s Source Code Module%s Information</I></B></FONT></TH></TR></TABLE></CENTER>",(temp) ? HTML_TABLE_RED:HTML_TABLE_MAGENTA,(temp) ? HTML_LRED:HTML_LMAGENTA,tcz_short_name,(temp) ? " Author":"");
			      strncpy(buffer,name,127);

			      parse_grouprange(NOTHING,NULL,ALL,1);
			      if(temp) modules_authors_view(NOBODY,buffer);
				 else modules_modules_view(NOBODY,buffer);

			      if(!html_lookup("NOBACK",0,0,0,0,1))
			         output(d,NOTHING,1,0,0,"<HR><A HREF=\"%s\"><IMG SRC=\"%s\" ALIGN=MIDDLE BORDER=0 ALT=\"[BACK]\"></A> &nbsp; &nbsp; <FONT SIZE=4 COLOR="HTML_LWHITE"><B>Return to %s web site...</B></FONT><HR></BODY></HTML>",html_home_url,html_image_url("back.gif"),tcz_full_name);
			      d->player = NOTHING;
			      server_shutdown_sock(d,0,2);
			      command_type &= ~(HTML_ACCESS|NO_FLUSH_OUTPUT);
			   } else {
			      writelog(HTML_LOG,1,(temp) ? "AUTHOR":"MODULE","Error:  Unable to serve source code module%s information to descriptor %d (%s)  -  Name of %s not specified.",(temp) ? " author":"",d->descriptor,d->hostname,(temp) ? "author":"module");
			      sprintf(buffer,"Please specify the name of the module%s.",(temp) ? " author":"");
			      html_error(d,1,buffer,"ERROR","Please click on the <B>BACK</B> button of your browser or the button to the left...",html_home_url,(header) ? (HTML_CODE_ERROR|HTML_CODE_HEADER):HTML_CODE_ERROR);
			      error = 2;
			   }
			} else if(!strcasecmp("VERSION",data) || !strcasecmp("VERSIONINFO",data)  || !strcasecmp("VERSIONINFORMATION",data)) {
			   char buffer[TEXT_SIZE];

			   /* ---->  Version Information  <---- */
			   http_header(d,1,200,"OK","text/html",now,now,1,0), header = 0;
			   if(!html_set_preferences(d,html_lookup("ID",1,1,1,1,1)))
			      d->html->flags |= HTML_DEFAULT_FLAGS;

			   command_type |= (HTML_ACCESS|NO_FLUSH_OUTPUT);
			   writelog(HTML_LOG,1,"VERSION","Version information served to descriptor %d (%s).",d->descriptor,d->hostname);
			   sprintf(buffer,"%s (%s) Version Information",tcz_full_name,tcz_short_name);
			   html_header(d,1,buffer,0,NULL,NULL,NULL,NULL,NULL,NULL,NULL,0,0);

 		           d->player = NOBODY;
 		           output(d,NOTHING,1,0,0,"<CENTER><TABLE BORDER WIDTH=100%% BGCOLOR="HTML_TABLE_GREEN"><TR><TH><FONT SIZE=6 COLOR="HTML_LGREEN"><B><I>Version Information</I></B></FONT></TH></TR></TABLE></CENTER>");
                           tcz_version(d,0);
   		           d->player = NOTHING;

			   if(!html_lookup("NOBACK",0,0,0,0,1))
			      output(d,NOTHING,1,0,0,"<HR><A HREF=\"%s\"><IMG SRC=\"%s\" ALIGN=MIDDLE BORDER=0 ALT=\"[BACK]\"></A> &nbsp; &nbsp; <FONT SIZE=4 COLOR="HTML_LWHITE"><B>Return to %s web site...</B></FONT><HR></BODY></HTML>",html_home_url,html_image_url("back.gif"),tcz_full_name);
			   server_shutdown_sock(d,0,2);
			   command_type &= ~(HTML_ACCESS|NO_FLUSH_OUTPUT);
			} else if(!strcasecmp("DISCLAIMER",data)) { 
  			   char  buffer[TEXT_SIZE];
			   const char *ptr;

			   /* ---->  Show TCZ disclaimer  <---- */
			   http_header(d,1,200,"OK","text/html",now,now,1,0), header = 0;
			   if(!html_set_preferences(d,html_lookup("ID",1,1,1,1,1)))
			      d->html->flags |= HTML_DEFAULT_FLAGS;

			   command_type |= (HTML_ACCESS|NO_FLUSH_OUTPUT);
			   writelog(HTML_LOG,1,"DISCLAIMER","Disclaimer served to descriptor %d (%s).",d->descriptor,d->hostname);
			   sprintf(buffer,"%s (%s) Disclaimer",tcz_full_name,tcz_short_name);
			   html_header(d,1,buffer,0,NULL,NULL,NULL,HTML_LBLUE,HTML_LCYAN,HTML_DBLUE,NULL,0,0);

			   if(!html_set_preferences(d,html_lookup("ID",1,1,1,1,1)))
			      d->html->flags |= HTML_DEFAULT_FLAGS;

 		           output(d,NOTHING,1,0,0,"<CENTER><TABLE BORDER WIDTH=100%% BGCOLOR="HTML_TABLE_RED"><TR><TH><FONT SIZE=6 COLOR="HTML_LRED"><B><I>%s (%s) Disclaimer</I></B></FONT></TH></TR></TABLE></CENTER>",tcz_full_name,tcz_short_name);

			   if(disclaimer) {
			      for(ptr = decompress(disclaimer); *ptr && (*ptr == '\n'); ptr++);
			   } else ptr = "Sorry, the disclaimer isn't available at the moment.\n";
			   output(d,NOTHING,2,0,0,"\016<P><CENTER><TABLE BORDER=5 CELLPADDING=4 BGCOLOR=#FFFFDD><TR><TD><FONT COLOR="HTML_DRED"><TT><B>\016%s\016</B></TT></TD></TR></TABLE></CENTER>\016",ptr);

			   if(!html_lookup("NOBACK",0,0,0,0,1))
			      output(d,NOTHING,1,0,0,"<P><HR><A HREF=\"%s\"><IMG SRC=\"%s\" ALIGN=MIDDLE BORDER=0 ALT=\"[BACK]\"></A> &nbsp; &nbsp; <FONT SIZE=4 COLOR="HTML_LWHITE"><B>Return to %s web site...</B></FONT><HR></BODY></HTML>",html_home_url,html_image_url("back.gif"),tcz_full_name);
			   server_shutdown_sock(d,0,2);
			   command_type &= ~(HTML_ACCESS|NO_FLUSH_OUTPUT);
			} else if(!strcasecmp("ROBOTS",data)) {
			    http_header(d,1,200,"OK","text/plain",now - WEEK,now + WEEK,1,0), header = 0;
			    output(d, NOTHING, 1, 0, 0, "user-agent: googlebot\nDisallow: /authors");
			    server_shutdown_sock(d,0,2);
			} else {
			   char buffer[TEXT_SIZE];

			   writelog(HTML_LOG,1,"HTML","Unknown resource '%s' requested by descriptor %d (%s).",!Blank(data) ? data:"Unknown",d->descriptor,d->hostname);
			   sprintf(buffer,"Sorry, your browser sent a query that cannot be understood by this server (TCZ v"TCZ_VERSION" (<I>%s</I>.))",tcz_full_name);
			   html_error(d,1,buffer,"CONNECTION REFUSED","Please click on the <B>BACK</B> button of your browser or the button to the left...",html_home_url,(header) ? (HTML_CODE_ERROR|HTML_CODE_HEADER):HTML_CODE_ERROR);
			   error = 2;
			}
		     } else {
			char buffer[TEXT_SIZE];

			writelog(HTML_LOG,1,"HTML","Non-specific resource requested by descriptor %d (%s).",d->descriptor,d->hostname);
			sprintf(buffer,"Sorry, your browser sent a query that cannot be understood by this server (TCZ v"TCZ_VERSION" (<I>%s</I>.))",tcz_full_name);
			html_error(d,1,buffer,"CONNECTION REFUSED","Please click on the <B>BACK</B> button of your browser or the button to the left...",html_home_url,(header) ? (HTML_CODE_ERROR|HTML_CODE_HEADER):HTML_CODE_ERROR);
			error = 2;
		     }
		     html_free();
		  } else {

		     /* ---->  No parameters given to server  -  Generate links page  <---- */
		     writelog(HTML_LOG,1,"HTML","Generic HTML Interface home page served to descriptor %d (%s)  -  No resource requested.",d->descriptor,d->hostname);
		     http_header(d,1,200,"OK","text/html",now,now,1,0), header = 0;
		     html_header(d,1,HTML_TITLE,0,html_image_url("background.gif"),HTML_DBLACK,HTML_DWHITE,HTML_LBLUE,HTML_LCYAN,HTML_DBLUE,NULL,0,1);
		     output(d,NOTHING,1,0,0,"<B><I>Please choose one of the following:</I></B><BR><UL>");

		     output(d,NOTHING,1,0,0,"<LI><A HREF=\"%s\" TARGET=_parent>%s's web site.</A><P></LI>",html_home_url,tcz_full_name);
		     output(d,NOTHING,1,0,0,"<LI><A HREF=\"%sTOPIC=HTML&\" TARGET=_parent>Information about the World Wide Web Interface.</A><P></LI>",html_server_url(d,0,1,"help"));
		     output(d,NOTHING,1,0,0,"<LI><A HREF=\"%s\" TARGET=_parent>See who's currently connected to %s.</A><P></LI>",html_server_url(d,0,0,"userlist"),tcz_short_name);
		     output(d,NOTHING,1,0,0,"<LI><A HREF=\"%s\" TARGET=_parent>%s Map.</A><P></LI>",html_server_url(d,0,0,"map"),tcz_full_name);

		     output(d,NOTHING,1,0,0,"</UL><P><HR WIDTH=90%%><P><UL>");
		     output(d,NOTHING,1,0,0,"<LI><A HREF=\"%s\" TARGET=_parent>%s (%s) Disclaimer.</A><P></LI>",html_server_url(d,0,0,"disclaimer"),tcz_full_name,tcz_short_name);
		     output(d,NOTHING,1,0,0,"<LI><A HREF=\"%s\" TARGET=_parent>Create a new %s character via the World Wide Web.</A><P></LI>",html_server_url(d,0,0,"createform"),tcz_short_name);
		     output(d,NOTHING,1,0,0,"<LI><A HREF=\"%s\" TARGET=_parent>Connect your existing %s character via the World Wide Web.</A><P></LI>",html_server_url(d,0,0,"connectform"),tcz_short_name);
		     output(d,NOTHING,1,0,0,"<LI><A HREF=\"telnet://%s:%d\">Connect to %s via Telnet (Text-only Telnet software required.)</A><P></LI>",tcz_server_name,(int) telnetport,tcz_short_name);

		     output(d,NOTHING,1,0,0,"</UL><P><HR WIDTH=90%%><P><UL>");
		     output(d,NOTHING,1,0,0,"<LI><A HREF=\"%sTOPIC=INDEX&\" TARGET=_parent>%s On-line Help System.</A><P></LI>",html_server_url(d,0,1,"help"),tcz_full_name);
		     output(d,NOTHING,1,0,0,"<LI><A HREF=\"%sTOPIC=INDEX&\" TARGET=_parent>%s On-line Tutorials.</A><P></LI>",html_server_url(d,0,1,"tutorial"),tcz_full_name);

		     output(d,NOTHING,1,0,0,"</UL><P><HR WIDTH=90%%><P><UL>");
		     output(d,NOTHING,1,0,0,"<LI><A HREF=\"%s\" TARGET=_parent>Home Pages of %s Users.</A><P></LI>",html_server_url(d,0,0,"homepages"),tcz_short_name);
		     output(d,NOTHING,1,0,0,"<LI><A HREF=\"%s\" TARGET=_parent>%s User Picture Gallery.</A><P></LI>",html_server_url(d,0,0,"gallery"),tcz_short_name);
		     output(d,NOTHING,1,0,0,"<LI><A HREF=\"%s\" TARGET=_parent>Profiles of %s Users.</A><P></LI>",html_server_url(d,0,0,"profiles"),tcz_short_name);
		     output(d,NOTHING,1,0,0,"<LI><A HREF=\"%s\" TARGET=_parent>Scans of %s Users.</A><P></LI>",html_server_url(d,0,0,"scans"),tcz_short_name);
		     output(d,NOTHING,1,0,0,"<LI><A HREF=\"%s\" TARGET=_parent>Current Administrators of %s.</A><P></LI>",html_server_url(d,0,0,"adminlist"),tcz_full_name);

		     output(d,NOTHING,1,0,0,"</UL><P><HR WIDTH=90%%><P><UL>");
		     output(d,NOTHING,1,0,0,"<LI><A HREF=\"%s\" TARGET=_parent>%s version information.</A><P></LI>",html_server_url(d,0,0,"version"),tcz_short_name);
		     output(d,NOTHING,1,0,0,"<LI><A HREF=\"%s\" TARGET=_parent>%s source code modules.</A><P></LI>",html_server_url(d,0,0,"modules"),tcz_short_name);
		     output(d,NOTHING,1,0,0,"<LI><A HREF=\"%s\" TARGET=_parent>%s source code module authors.</A><P></LI>",html_server_url(d,0,0,"authors"),tcz_short_name);
		     output(d,NOTHING,1,0,0,"</UL><BR>");
		     output(d,NOTHING,1,0,0,"<HR><TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_DRED"><TR><TH><FONT COLOR="HTML_LYELLOW">");
                     output(d,NOTHING,1,0,0,"TCZ is free software, which is distributed under <B><I>version 2</I></B> of the <B>GNU General Public License</B> (See '<A HREF=\"%sTOPIC=gpl&\" TARGET=_blank TITLE=\"Click to read the GNU General Public License...\"><FONT COLOR="HTML_LWHITE">help gpl</FONT></A>' in TCZ, or visit <A HREF=\"http://www.gnu.org\" TARGET=_blank TITLE=\"Visit the GNU web site...\"><FONT COLOR="HTML_LWHITE">http://www.gnu.org</FONT></A>) &nbsp; For more information about the TCZ, please visit: &nbsp; <A HREF=\"http://www.sourceforge.net/projects/tcz\" TARGET=_blank TITLE=\"Visit the TCZ project web site...\"><FONT COLOR="HTML_LWHITE">http://www.sourceforge.net/projects/tcz</FONT></A>",html_server_url(d,0,1,"help"));
                     output(d,NOTHING,1,0,0,"</FONT></TH></TR></TABLE><HR>");
                     output(d,NOTHING,1,0,0,"</BODY></HTML>");
		     server_shutdown_sock(d,0,2);
		  }
	       } else {
		  writelog(HTML_LOG,1,"HTML","Access Denied:  Invalid/unrecognised header received from descriptor %d (%s).",d->descriptor,d->hostname);
		  error = 1;
	       }
	    } else {
	       char *ptr2;

	       for(ptr2 = ptr; *ptr2 && (*ptr2 != ' '); ptr2++);
	       if(*ptr) *ptr = '\0';
	       writelog(HTML_LOG,1,"HTML","Access Denied:  Unsupported/unrecognised method '%s' received from descriptor %d (%s) (GET/POST methods are only supported.)",ptr,d->descriptor,d->hostname);
	       error = 1;
	    }
	 } else {
	    writelog(HTML_LOG,1,"HTML","Access Denied:  No header received from descriptor %d (%s).",d->descriptor,d->hostname);
	    error = 1;
	 }

	 if(error) {
	    if(error == 1)
	       html_error(d,1,"Sorry, corrupt HTML data received  -  Please try again.","CONNECTION REFUSED","Please click on the <B>BACK</B> button of your browser or the button to the left...",html_home_url,(header) ? (HTML_CODE_ERROR|HTML_CODE_HEADER):HTML_CODE_ERROR);
	    server_shutdown_sock(d,0,2);
	    return(0);
	 }
	 FREENULL(d->negbuf);
	 return(1);
}

/* ---->  {J.P.Boggis 15/03/2000}  Disable reverse foreground/background colours (For HTML tables with background colours)  <---- */
void html_anti_reverse(struct descriptor_data *d,unsigned char reverse)
{
     static int htmlflags = 0;
     static int stored    = 0;

     if(!IsHtml(d)) return;
     if(reverse) {
        if(!stored) htmlflags = d->html->flags;
        d->html->flags &= ~HTML_WHITE_AS_BLACK;
        stored++;
     } else if(stored) {
        if(stored == 1) d->html->flags = htmlflags;
        if(stored > 0) stored--;
     }
}

/* ---->  {J.P.Boggis 29/01/2000}  {@?link} query command  <---- */
/*        Insert link to web page:                               */
/*        {@?link "<NAME>" "<DESTINATION>" "<HELPTEXT>"}         */
void html_query_link(CONTEXT)
{
     struct arg_data arg;

     unparse_parameters(params,3,&arg,0);
     if(Blank(arg.text[1])) arg.text[1] = (char *) html_home_url;
     if(Blank(arg.text[0])) arg.text[0] = arg.text[1];

     if(!Blank(arg.text[3]))
        sprintf(querybuf,"\016<A HREF=\"%s\" TARGET=_blank TITLE=\"%s\">\016%s\016</A>\016",arg.text[1],arg.text[2],arg.text[0]);
           else sprintf(querybuf,"\016<A HREF=\"%s\" TARGET=_blank>\016%s\016</A>\016",arg.text[1],arg.text[0]);
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  {J.P.Boggis 29/01/2000}  {@?tczlink} query command  <---- */
/*        Insert link that executes TCZ command for HTML user:      */
/*        {@?tczlink "<NAME>" "<COMMAND>" ["<HELPTEXT>"]}           */
void html_query_tczlink(CONTEXT)
{
     struct descriptor_data *p = getdsc(player);
     char   buffer[TEXT_SIZE + 1];
     struct arg_data arg;
     int    copied;

     unparse_parameters(params,4,&arg,0);
     if(Blank(arg.text[0])) arg.text[0] = arg.text[1];

     if(!Blank(arg.text[1])) {
	if(Validchar(player) && Connected(player)) {
           if(Blank(arg.text[3])) {
   	      if(!Blank(arg.text[2]))
	         sprintf(querybuf,"\016<A HREF=\"%s\017SUBST=OK&COMMAND=%s&LINK=YES&\" TARGET=TCZINPUT TITLE=\"%s\">\016%s\016</A>\016",html_server_url(p,0,1,"input"),html_encode(arg.text[1],buffer,&copied,TEXT_SIZE),arg.text[2],arg.text[0]);
	  	    else sprintf(querybuf,"\016<A HREF=\"%s\017SUBST=OK&COMMAND=%s&LINK=YES&\" TARGET=TCZINPUT>\016%s\016</A>\016",html_server_url(p,0,1,"input"),html_encode(arg.text[1],buffer,&copied,TEXT_SIZE),arg.text[0]);
	   } else {
   	      if(!Blank(arg.text[2]))
	         sprintf(querybuf,"\016<A HREF=\"%s\017SUBST=OK&COMMAND=%s&LINK=YES&\" TARGET=TCZINPUT TITLE=\"%s\"><IMAGE SRC=\"%s\" BORDER=0 ALT=\"%s\"></A>\016",html_server_url(p,0,1,"input"),html_encode(arg.text[1],buffer,&copied,TEXT_SIZE),arg.text[2],arg.text[3],arg.text[2]);
	            else sprintf(querybuf,"\016<A HREF=\"%s\017SUBST=OK&COMMAND=%s&LINK=YES&\" TARGET=TCZINPUT><IMAGE SRC=\"%s\" BORDER=0></A>\016",html_server_url(p,0,1,"input"),html_encode(arg.text[1],buffer,&copied,TEXT_SIZE),arg.text[3]);
	   }
	} else sprintf(querybuf,"%s",arg.text[0]);
     } else {
        output(p,player,0,1,0,ANSI_LGREEN"Please specify the compound command to execute when the link is clicked on.");
        setreturn(ERROR,COMMAND_FAIL);
        return;
     }
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  {J.P.Boggis 29/01/2000}  {@?image} query command  <---- */
/*        Insert image with optional web page link:               */
/*        {@?image "<IMAGE URL>" "<DESCRIPTION>" ["<DESTINATION URL>"]}  */
void html_query_image(CONTEXT)
{
     struct arg_data arg;

     unparse_parameters(params,3,&arg,0);
     if(!Blank(arg.text[2])) {
        if(!Blank(arg.text[1]))
	   sprintf(querybuf,"\016<A HREF=\"%s\" TARGET=_blank TITLE=\"%s\"><IMG SRC=\"%s\" BORDER=0 ALT=\"%s\"></A>\016",arg.text[2],arg.text[1],arg.text[0],arg.text[1]);
	      else sprintf(querybuf,"\016<A HREF=\"%s\" TARGET=_blank><IMG SRC=\"%s\" BORDER=0></A>\016",arg.text[2],arg.text[0]);
     } else if(!Blank(arg.text[1]))
	sprintf(querybuf,"\016<IMG SRC=\"%s\" ALT=\"%s\">\016",arg.text[0],arg.text[1]);
	   else sprintf(querybuf,"\016<IMG SRC=\"%s\">\016",arg.text[0]);
     setreturn(querybuf,COMMAND_SUCC);
}

