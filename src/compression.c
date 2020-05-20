/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| COMPRESSION.C  -  Implements an enhanced compression routine which features |
|                   run-length encoding and a compression table using the     |
|                   most common words in the database (Constructed            |
|                   automatically at run-time.)                               |
|                                                                             |
|                   To enable enhanced compression, EXTRA_COMPRESSION must be |
|                   #define'd in 'config.h' at compile-time.                  |
|                                                                             |
|                   Enhanced compression can drastically save on memory usage |
|                   (Typically obtaining a compression ratio of up to 70% or  |
|                   more on text fields in a reasonable sized database), but  |
|                   it requires slightly more CPU overhead than the basic     |
|                   compression routine, which will be used instead if        |
|                   EXTRA_COMPRESSION isn't #define'd (This gains a flat      |
|                   compression ratio of 12.5%)                               |
|                                                                             |
|                   NOTE:  Only the compression routine requires slightly     |
|                          more CPU overhead  -  The decompression routine    |
|                          is no more intensive than the basic one.           |
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
| Module originally designed and written by:  J.P.Boggis 19/02/1996.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                        https://github.com/smcvey/tcz                        |
`-----------------------------------------------------------------------------'

*/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "logfiles.h"
#include "externs.h"
#include "config.h"
#include "db.h"


extern char scratch_return_string[];

struct table_data
{
       short          prefix;
       unsigned short entry;
       char           *word;
       unsigned short len;
};

struct word_data
{
       struct  word_data *centre;
       struct  word_data *cright;
       struct  word_data *cleft;
       struct  word_data *right;
       struct  word_data *left;
       struct  word_data *next;
       int     count;
       char    *word;
};


static struct   table_data table[COMPRESSION_TABLE_SIZE];  /*  Compression table (Array of most common words in database)  */
static struct   word_data  *wordptr;
static char                wordbuf[TEXT_SIZE];
static unsigned char       statistics = 1;  /*  Generate compression statistics (Done on initial proper database load) to calculate estimated compression ratio from  */
static unsigned short      entries = 0;     /*  Total number of entries in compression table  */
static unsigned long       db_size = 0;     /*  Total size of database (In bytes)  */
static unsigned long       saved = 0;       /*  Bytes saved by compression routine  */
static int                 processed;


/* ---->  Check case of word is lowercase (Except first letter)  <---- */
unsigned char check_case(const char *word,short *length,short *capital)
{
	 static short len,upper,lower,capitalise;

	 if(Blank(word)) {
	    *length = 0, *capital = 0;
	    return(0);
	 }
	 len = 0, capitalise = isupper(*word);
	 for(upper = 0, lower = 0; *word && (len < COMPRESSION_MAX_WORDLEN); len++, word++)
	     if(isupper(*word)) upper++;
		else lower++;

	 if(len < 3) {
	    *length = len, *capital = 0;
	    return(0);
	 }
	 if(upper == len) {
	    *length = len, *capital = 0;
	    return(1);
	 } else if((lower == len) || (capitalise && (lower == (len - 1)))) {
	    *length = len, *capital = capitalise;
	    return(1);
	 }
	*length = len, *capital = 0;
	return(0);
}

/* ---->  Search for WORD in compression table, returning TABLE  <---- */
/*        entry if found, otherwise NULL.                              */
struct table_data *search_table(const char *word,unsigned short len,unsigned short *nearst)
{
       static int top,middle,bottom,nearest,current,result;
       static short nlen;

       *nearst = 0;
       if(Blank(word)) return(NULL);
       top = entries - 1, middle = 0, bottom = 0, nearest = NOTHING, nlen = 0;
       while(bottom <= top) {
             middle = (top + bottom) / 2;
             if((result = strcmp(table[middle].word,word)) != 0) {
                if(*word == table[middle].word[0])
                   for(current = table[middle].prefix; current != NOTHING; current = table[current].prefix)
                       if((table[current].len > nlen) && !strncmp(word,table[current].word,table[current].len))
                          nearest = current, nlen = table[current].len;

                if(result < 0) bottom = middle + 1;
                   else top = middle - 1;
	     } else return(&table[middle]);
       }

       /* ---->  Nearest matching word  <---- */
       if(nearest != NOTHING) {
          *nearst = 1;
          return(&table[nearest]);
       }
       return(NULL);
}

#ifdef EXTRA_COMPRESSION

/* ---->  Compress given string (Enhanced compression)  <---- */
char *compress(const char *str,unsigned char capitalise)
{
     static char *word,*cmp,*ptr,*cacheptr;
     static unsigned short count,nearest;
     static struct table_data *entry;
     static const char *ptr2,*start;
     static unsigned char subst;
     static short wlen,capital;
     static char cache;

     if(!str) return(NULL);
     cmp = cmpptr, start = str;
     while(*str) {
           if(isalpha(*str)) count = 0;
              else for(ptr2 = str + 1, count = 1; *ptr2 && (*ptr2 == *str) && (count < 130); ptr2++, count++);
           if(count > 2) {
              /* ---->  Run-length encoding  <---- */
              if(statistics) db_size += count, saved += (count - 2);
              count -= 3;
              count  = (count << 8);
              count |= ((*str & 0x7F) << 1);
              count |= 0x8000;
              *cmp++ = (count >> 8);
              *cmp++ = (count & 0xFF);
              str    = ptr2;
	   } else if(!(isalpha(*str) || (*str == '@') || ((*str == '%') && *(str + 1)))) {

              /* ---->  Non-alphabetical text (Excludes substituion strings)  <---- */
              if(capitalise && isdigit(*str)) capitalise = 0;
              *cmp++ = *(str++) & 0x7F;
              if(statistics) db_size++;
	   } else {

              /* ---->  Alphabetical text (Scan for word)  <---- */
              word = wordbuf;
              if(((str - 1) >= start) && (*(str - 1) == '&')) capitalise = 0;
              if(*str != '%') {
                 if(*str == '@') {
                    *word++ = *(str++) & 0x7F;
                    if(*str && (*str == '?')) *word++ = *(str++) & 0x7F;
                    if(capitalise) capitalise = 0;
		 } 
                 for(; *str && isalpha(*str); *word++ = *(str++) & 0x7F);
	      } else while((*str == '%') && *(str + 1))
                 *word++ = (*(str++) & 0x7F), *word++ = (*(str++) & 0x7F);
              *word = '\0';

              ptr = wordbuf, subst = 0;
              while(*ptr) {
                    subst = (*ptr == '%');
                    for(ptr2 = ptr + 1, count = 1; *ptr2 && (*ptr2 == *ptr) && (count < 130); ptr2++, count++);
                    if(count > 2) {

                       /* ---->  Run-length encoding  <---- */
                       if(statistics) db_size += count, saved += (count - 2);
                       count -= 3;
                       count  = (count << 8);
                       count |= ((*ptr & 0x7F) << 1);
                       count |= 0x8000;
                       *cmp++ = (count >> 8);
                       *cmp++ = (count & 0xFF);
                       ptr    = (char *) ptr2;
		    } else if(check_case(ptr,&wlen,&capital)) {
                       if(capital) *ptr = tolower(*ptr);
                       cacheptr = ptr + wlen, cache = *cacheptr, *cacheptr = '\0';
                       if((entry = search_table(ptr,wlen,&nearest))) {
                          if(!nearest) {

                             /* ---->  Exact match  <---- */
                             count  = ((entry->entry + 1) << 1);
                             count |= 0x8001;
                             if(capitalise && !subst) count |= 0x4000, capitalise = 0;
                                else if(capital) count |= 0x4000;
                             if((cache == '\0') && *str && (*str == ' ')) 
                                count |= 0x2000, db_size++, saved++, str++;
                             *cmp++ = count >> 8;
                             *cmp++ = count & 0xFF;
                             if(statistics) db_size += entry->len, saved += (entry->len - 2);
                             *wordbuf = '\0', ptr = wordbuf;
			  } else {

                             /* ---->  Prefix match  <---- */
                             count  = ((entry->entry + 1) << 1);
                             count |= 0x8001;
                             if(capitalise && !subst) count |= 0x4000, capitalise = 0;
                                else if(capital) count |= 0x4000;
                             *cmp++ = count >> 8;
                             *cmp++ = count & 0xFF;
                             ptr   += entry->len;
                             if(statistics) db_size += entry->len, saved += (entry->len - 2);
			  }
		       } else {

                          /* ---->  No match  <---- */
                          if(!subst) {
                             if(capital || capitalise) *cmp++ = toupper(*ptr++), capitalise = 0;
                                else *cmp++ = *ptr++;
			  } else {
                             *cmp++ = *ptr++;
                             if(*ptr) *cmp++ = *ptr++;
			  }
                          if(statistics) db_size++;
		       }
                       *cacheptr = cache;
		    } else {

                       /* ---->  No match  <---- */
                       if(!subst) {
                          if(capitalise) *cmp++ = toupper(*ptr++), capitalise = 0;
                             else *cmp++ = *ptr++;
		       } else {
                          *cmp++ = *ptr++;
                          if(*ptr) *cmp++ = *ptr++;
		       }
                       if(statistics) db_size++;
		    }
	      }
	   }
     }
     *cmp = '\0';
     return(cmpptr);
}

/* ---->  Decompress given string (Enhanced compression)  <---- */
char *decompress(const char *str)
{
     static unsigned char  capitalise,addspace;
     static char           *cmp,*start;
     static const          char *ptr;
     static unsigned short count;
     static char           tmp;

     if(!str) return(NULL);
     cmp = cmpptr;
     while(*str)
           if((*str & 0x80) && *(str + 1)) {
              if(*(str + 1) & 0x1) {

                 /* ---->  Word from compression table  <---- */
                 capitalise = (*str & 0x40);
                 addspace   = (*str & 0x20);
                 count  = ((*str & 0x1F) << 7);
                 count |= ((*(++str) & 0xFE) >> 1), count--;
                 for(ptr = table[count].word, start = cmp; *ptr; *cmp++ = *ptr++);
                 if(capitalise) if(islower(*start)) *start = toupper(*start);
                 if(addspace) *cmp++ = ' ';
                 str++;
	      } else {

                 /* ---->  Run-length encoding  <---- */
                 count = (*(str++) & 0x7F);
                 for(count += 3, tmp = ((*(str++) & 0xFE) >> 1); count > 0; *cmp++ = tmp, count--);
	      }
	   } else *cmp++ = *str++;
     *cmp = '\0';
     return(cmpptr);
}

#else

/* ---->  Compress given string (Alternative simple compression)  <---- */
char *compress(const char *str,unsigned char capitalise)
{
     static char *start,*ptr,*cptr;
     static int  subcount,count;
     static char current;

     if(!str) return(NULL);

     ptr = cmpptr, start = str, count = 0;
     while(*str && (count < MAX_LENGTH)) {
           for(cptr = ptr, subcount = 0; *str && (count < MAX_LENGTH) && (subcount < 8); *ptr++ = current, str++, count++, subcount++) {
               current = *str;
               if(capitalise && isalpha(current) && ((str <= start) || (*(str - 1) != '&'))) {
                  if(islower(current)) current = toupper(current);
                  capitalise = 0;
	       }
	   }

           if(*str) {
              current = *str;
              if(capitalise && isalpha(current) && ((str <= start) || (*(str - 1) != '&'))) {
                 if(islower(current)) current = toupper(current);
                 capitalise = 0;
	      }
              for(subcount = 0; subcount < 8; *cptr |= ((current & (0x80 >> subcount)) << subcount), cptr++, subcount++);
              str++;
	   }
     }
     *ptr = '\0';
     return(cmpptr);
}

/* ---->  Decompress given string (Alternative simple compression)  <---- */
char *decompress(const char *str)
{
     static int   subcount,count;
     static const char *cptr;
     static char  *ptr;

     if(!str) return(NULL);

     ptr   = cmpptr;
     count = 0;
     while(*str && (count < MAX_LENGTH)) {
           for(cptr = str, subcount = 0; *str && (count < MAX_LENGTH) && (subcount < 8); *ptr++ = (*str & 0x7F), str++, count++, subcount++);
           if((subcount == 8) && (count < MAX_LENGTH)) {
              for(subcount = 0, *ptr = '\0'; *cptr && (subcount < 8); *ptr |= ((*cptr & 0x80) >> subcount), cptr++, subcount++);
              ptr++, count++;
	   }
     }
     *ptr = '\0';
     return(cmpptr);
}

#endif

/* ---->  Log estimated compression ratio  <---- */
void compression_ratio()
{
     writelog(SERVER_LOG,0,"RESTART","Estimated compression ratio is %.2f%%  -  %.2fMb of memory saved.",((float) saved / db_size) * 100,(float) saved / MB);
     statistics = 0;
}

/* ---->  Recursively traverse tertiary tree to load most commonly used words into compression table  <---- */
void traverse_tree(struct word_data *word)
{
     if(!word) return;
     if(word->cright) traverse_tree(word->cright);
     processed = 0;
     if(entries < COMPRESSION_TABLE_SIZE) {
        table[entries].len = strlen(word->word);
        if(((table[entries].len - 2) * word->count) > (table[entries].len + 1)) {
           table[entries].entry  = entries;
           table[entries].word   = word->word;
           saved                += ((table[entries].len - 2) * word->count) - (table[entries].len + 1);
           word->word            = NULL;
           entries++, processed++;
	}
     }
     for(wordptr = word->centre; wordptr && (entries < COMPRESSION_TABLE_SIZE); wordptr = wordptr->centre) {
         table[entries].len = strlen(wordptr->word);
         if(((table[entries].len - 2) * word->count) > (table[entries].len + 1)) {
            table[entries].entry  = entries;
            table[entries].word   = wordptr->word;
            saved                += ((table[entries].len - 2) * wordptr->count) - (table[entries].len + 1);
            wordptr->word         = NULL;
            entries++, processed++;
	 }
     }
     if(word->cleft && processed && (entries < COMPRESSION_TABLE_SIZE)) traverse_tree(word->cleft);
}

/* ---->  Initialise compression table  <---- */
unsigned char initialise_compression_table(const char *filename,unsigned char compress)
{
	 struct   word_data *tail = NULL,*start = NULL;
	 struct   word_data *current,*last,*new,*cptr;
	 short    wlen,capital,highest,top;
	 int      words = 0,value,loop;
	 time_t   starttime,finishtime;
	 struct   table_data temp;
	 unsigned char found;
	 char     *ptr,*p1;
	 char     right;
	 FILE     *f;

	 gettime(starttime);
	 writelog(SERVER_LOG,0,"RESTART","Initialising compression table from %sdatabase '%s'...",(compress) ? "compressed ":"",filename);
	 if(compress) {
	    sprintf(scratch_return_string,DB_DECOMPRESS" < %s",filename);
	    if((f = popen(scratch_return_string,"r")) == NULL) return(0);
	 } else if((f = fopen(filename,"r")) == NULL) return(0);
	 for(loop = 0; loop < COMPRESSION_TABLE_SIZE; table[loop].word = NULL, table[loop].len = 0, table[loop].entry = 0, loop++);

	 bytesread = 0;
	 if(log_stderr) progress_meter(0,MB,PROGRESS_UNITS,0);
	 while((ptr = (char *) db_read_field(f))) {
	       if(log_stderr) progress_meter(bytesread,MB,PROGRESS_UNITS,1);
	       wlen = strlen(ptr);
	       if(wlen > MAX_LENGTH) ptr[MAX_LENGTH] = '\0';

	       /* ---->  Load unique words from database into alphabetically sorted binary tree  <---- */
	       while(*ptr) {

		     /* ---->  Scan for word  <---- */
		     for(; *ptr && !(isalpha(*ptr) || (*ptr == '@') || ((*ptr == '%') && *(ptr + 1))); ptr++);
		     p1 = cmpbuf;
		     if(*ptr != '%') {
			if(*ptr == '@') {
			   *p1++ = *ptr++;
			   if(*ptr && (*ptr == '?')) *p1++ = *ptr++;
			}
			for(; *ptr && isalpha(*ptr); *p1++ = *ptr, ptr++);
		     } else while((*ptr == '%') && *(ptr + 1))
			*p1++ = *ptr++, *p1++ = *ptr++;
		     *p1 = '\0';

		     /* ---->  Add word to binary tree (Providing it isn't already in binary tree)  <---- */
		     if(*cmpbuf)
			if(check_case(cmpbuf,&wlen,&capital)) {
			   current = start, last = NULL, found = 0, cmpbuf[wlen] = '\0';
			   if(capital) *cmpbuf = tolower(*cmpbuf);
			   if(!current) {

			      /* ---->  Create root node of binary tree  <---- */
			      MALLOC(new,struct word_data);
			      new->left  = new->right = new->cleft = new->cright = new->centre = new->next = NULL;
			      new->count = 1;
			      new->word  = (char *) alloc_string(cmpbuf);
			      start      = tail = new;
			      words++;
			   } else {
			      while(current && !found) {
				    if(!(value = strcmp(current->word,cmpbuf))) {

				       /* ---->  Word matches existing word in binary tree:  Increment usage count  <---- */
				       current->count++;
				       found   = 1;
				    } else if(value < 0) {
				       last    = current;
				       current = current->right;
				       right   = 1;
				    } else {
				       last    = current;
				       current = current->left;
				       right   = 0;
				    }
			      }

			      /* ---->  If not found, add to binary tree  <---- */
			      if(!found && (words < COMPRESSION_QUEUE_SIZE)) {
				 MALLOC(new,struct word_data);
				 new->left  = new->right = new->cleft = new->cright = new->centre = new->next = NULL;
				 new->count = 1;
				 new->word  = (char *) alloc_string(cmpbuf);
				 tail->next = new;
				 tail       = new;
				 if(last) {
				    if(right) last->right = new;
				       else last->left = new;
				 }
				 words++;
			      }
			   }
			}
	       }
	 }
	 if(log_stderr) progress_meter(bytesread,MB,PROGRESS_UNITS,2);

	 /* ---->  Create tertiary tree, sorted on word usage count  <---- */
	 for(cptr = (start) ? start->next:NULL; cptr; cptr = cptr->next) {
	     current = start;
	     last    = NULL;
	     found   = 0;

	     while(current && !found) {
		   if(current->count == cptr->count) {
		      cptr->centre    = current->centre;
		      current->centre = cptr;
		      found           = 1;
		   } else if(current->count < cptr->count) {
		      last    = current;
		      current = current->cright;
		      right   = 1;
		   } else {
		      last    = current;
		      current = current->cleft;
		      right   = -1;
		   }
	     }

	     /* ---->  Add to tertiary tree  <---- */
	     if(!found && last) {
		if(right > 0) last->cright = cptr;
		   else last->cleft = cptr;
	     }
	 }

	 /* ---->  Load most commonly used words (From tertiary tree) into compression table  <---- */
	 traverse_tree(start);

	 /* ---->  Free linked list of words  <---- */
	 for(; start; start = cptr) {
	     cptr = start->next;
	     FREENULL(start->word);
	     FREENULL(start);
	 }

	 /* ---->  Sort words in compression table into alphabetical order  <---- */
	 for(top = entries - 1; top > 0; top--) {

	     /* ---->  Find highest entry in unsorted part of list  <---- */
	     for(loop = 1, highest = 0; loop <= top; loop++)
		 if(strcmp(table[loop].word,table[highest].word) > 0)
		    highest = loop;

	     /* ---->  Swap highest entry in unsorted part of list with bottom entry of sorted part of list  <---- */
	     if(highest < top) {
		temp.word = table[top].word, temp.len = table[top].len;
		table[top].word = table[highest].word, table[top].len = table[highest].len;
		table[highest].word = temp.word, table[highest].len = temp.len;
	     }
	 }

	 /* ---->  Set prefix pointers in word table  <---- */
	 for(top = 0; top < entries; top++) {
	     highest = NOTHING, value = 0;
	     for(loop = 0; loop < entries; loop++)
		 if((loop != top) && (table[loop].len > value) && (table[loop].len < table[top].len) && string_prefix(table[top].word,table[loop].word))
		    highest = loop, value = table[loop].len;
	     table[top].prefix = (highest != NOTHING) ? highest:NOTHING;
	 }

	 /* ---->  Compression statistics  <---- */
	 gettime(finishtime);
	 writelog(SERVER_LOG,0,"RESTART","%d unique word%s processed (Most common %d/%d in compression table  -  Initialisation took %s.)",words,Plural(words),entries,COMPRESSION_TABLE_SIZE,interval(finishtime - starttime,0,ENTITIES,0));
	 if(compress) pclose(f);
	    else fclose(f);
	 return(1);
}
