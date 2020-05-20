/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| PROMPT.C  -  Interactive '@prompt' command and internal prompt_user()       |
|              function, both used for interactively prompting user for       |
|              arguments.                                                     |
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
| Module originally designed and written by:  J.P.Boggis 02/10/2000.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                        https://github.com/smcvey/tcz                        |
`-----------------------------------------------------------------------------'

*/

#include <string.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "descriptor_flags.h"
#include "object_types.h"
#include "flagset.h"
#include "prompts.h"


/* ---->  Display user prompt (Highest is 31 (Simple 'more' paged disclaimer at login screen))  <---- */
void prompt_display(struct descriptor_data *d)
{
     unsigned char cached_addcr = add_cr;
     char          buffer[BUFFER_LEN];
     time_t        now;

     if((d->clevel == 127) && Validchar(d->player)) {
        db[d->player].flags2 |=  FRIENDS_CHAT;
        db[d->player].flags  &= ~(HAVEN|QUIET);
        if(!Level4(d->player) && !Experienced(d->player) && !Assistant(d->player))
           d->flags |= WELCOME;
        d->clevel =  0;
     }
     if(d->terminal_xpos > 0) output(d,d->player,0,1,0,"");

     gettime(now);
     if((d->next_time - now) > 5) {
        output(d,d->player,0,1,0,"");
        output(d,d->player,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"The last command which you executed took %s some time to complete, possibly causing 'lag' and inconveniencing other users.  You will not be able to execute further commands for another "ANSI_LYELLOW"%s"ANSI_LWHITE".\n",tcz_short_name,interval(d->next_time - now,0,ENTITIES,0));
     }

     d->flags &= ~PROMPT;
     if(d->flags & TELNET_EOR) add_cr = 255;
     if(d->clevel == 0) {

        /* ---->  'more' pager prompt  <---- */
        if(d->pager && !d->pager->prompt) pager_display(d);
        if(d->pager) {
           pager_prompt(d);
           output(d,d->player,2,0,0,"%s",d->pager->prompt);
	} else if(Validchar(d->player)) {
           unsigned char override = (d->edit || d->prompt);

           /* ---->  Normal user prompt ('TCZ>')  <---- */
           if(!override && (Location(d->player) == bbsroom)) {
              output(d,d->player,2,0,0,ANSI_LWHITE""TELNET_BBS_PROMPT""ANSI_DWHITE" ",tcz_short_name);
	   } else if(!override && (Location(d->player) == mailroom)) {
              output(d,d->player,2,0,0,ANSI_LWHITE""TELNET_MAIL_PROMPT""ANSI_DWHITE" ",tcz_short_name);
	   } else if(!override && (Location(d->player) == bankroom)) {
              output(d,d->player,2,0,0,ANSI_LWHITE""TELNET_BANK_PROMPT""ANSI_DWHITE" ",tcz_short_name);
	   } else {
              substitute(d->player,buffer,(d->prompt) ? !Blank(d->prompt->prompt) ? d->prompt->prompt:ANSI_LWHITE"INPUT> "ANSI_DWHITE:d->user_prompt,0,ANSI_LWHITE,NULL,0);
              output(d,d->player,2,0,0,"%s",buffer);
	   }
	} else output(d,d->player,2,0,0,ANSI_LWHITE"%s"ANSI_DWHITE" ",tcz_prompt);
     } else { 
       if((d->clevel < 8) || (d->clevel > 13)) termflags = TXT_NORMAL;
       switch(d->clevel) {
              case -1:

                   /* ---->  Enter name (Without newbie instructions)  <---- */
                   output(d,d->player,2,0,0,LOGIN_PROMPT""ANSI_DWHITE" ");
                   d->clevel = 1;
                   break;
	      case 1:

                   /* ---->  Enter name (With newbie instructions)  <---- */
                   d->page = 0;
                   output(d,d->player,2,0,0,NEWBIE_PROMPT""LOGIN_PROMPT""ANSI_DWHITE" ");
                   break;
              case 2:

                   /* ---->  Would user like to create character with given name?  <---- */
                   output(d,d->player,2,0,0,CREATE_PROMPT""ANSI_DWHITE" ",StringDefault(d->name,"Unknown"));
                   break;
	      case 3:

                   /* ---->  Enter password (Connect)  <---- */
                   server_set_echo(d,0);
                   output(d,d->player,2,0,0,CONNECT_PASSWORD_PROMPT""ANSI_DWHITE" ");
                   break;
	      case 4:

                   /* ---->  Enter password (Create)  <---- */
                   server_set_echo(d,0);
                   output(d,d->player,2,0,0,CREATE_PASSWORD_PROMPT""ANSI_DWHITE" ");
                   break;
	      case 5:

                   /* ---->  Re-type password  <---- */
                   server_set_echo(d,0); 
                   output(d,d->player,2,0,0,VERIFY_PASSWORD_PROMPT""ANSI_DWHITE" ");
                   break;
              case 6:

                   /* ---->  Does user still want to create character?  <---- */
                   output(d,d->player,2,0,0,VERIFY_CREATE_PROMPT""ANSI_DWHITE" ");
                   break;
	      case 7:
              case 23:
              case 24:
              case 29:

                   /* ---->  Disclaimer  <---- */
                   if(d->page != NOTHING) {
                      termflags = 0, add_cr = cached_addcr;
                      if(!Blank(disclaimer)) {
   	 	         const char *ptr = decompress(disclaimer);

#ifdef PAGE_DISCLAIMER
                         if(!d->html) {
                            d->page_clevel = d->clevel;
                            d->clevel = 30;
                            d->page = 1;
			 } else {
#endif
   		            if(d->clevel == 29) for(; *ptr && (*ptr == '\n'); ptr++);
		            output(d,d->player,2,1,0,"%s%s%s",IsHtml(d) ? "\016<TABLE BORDER=5 CELLPADDING=4 BGCOLOR=#FFFFDD><TR><TD><FONT COLOR="HTML_DRED"><TT><B>\016":ANSI_LRED,ptr,IsHtml(d) ? "\016</B></TT></TD></TR></TABLE>\016":"");
#ifdef PAGE_DISCLAIMER
			 }
#endif
		      } else output(d,d->player,0,1,0,ANSI_LRED"\nSorry, the disclaimer isn't available at the moment.");

                      if(d->clevel != 30) termflags = TXT_NORMAL;

                      if(((d->clevel == 30) && (d->page_clevel == 29)) || (d->clevel == 29)) {
                         output(d,d->player,0,1,0,REACCEPT_DISCLAIMER_PROMPT,tcz_full_name,tcz_short_name,tcz_short_name);
                         if(d->clevel == 30) {
                            output(d,d->player,2,0,0,"\n"SIMPLE_PAGER_PROMPT""ANSI_DWHITE" ");
                            d->page = TCZ_INFINITY;
			 }
		      }
		   } else d->page = 0;

                   if(d->clevel != 30) {
                      if(!d->pager) {
                         if(d->flags & TELNET_EOR) add_cr = 255;
                         output(d,d->player,2,0,0,TELNET_ACCEPT_DISCLAIMER_PROMPT""ANSI_DWHITE" ");
		      }
		   }
                   break;
	      case 8:

                   /* ---->  Choose gender  <---- */
                   output(d,d->player,2,0,0,TELNET_GENDER_PROMPT""ANSI_DWHITE" ");
                   break;
	      case 9:

                   /* ---->  Enter race  <---- */
                   output(d,d->player,2,0,0,TELNET_RACE_PROMPT""ANSI_DWHITE" ");
                   break;
	      case 10:
              case 25:

                   /* ---->  Enter E-mail address  <---- */
                   output(d,d->player,2,0,0,TELNET_EMAIL_PROMPT""ANSI_DWHITE" ",tcz_admin_email);
                   break;
              case 11:

                   /* ---->  Enter old password ('@password')  <---- */
                   server_set_echo(d,0);
                   output(d,d->player,2,0,0,TELNET_OLD_PASSWORD_PROMPT""ANSI_DWHITE" ");
                   break;
              case 12:

                   /* ---->  Enter new password ('@password')  <---- */
                   server_set_echo(d,0); 
                   output(d,d->player,2,0,0,TELNET_NEW_PASSWORD_PROMPT""ANSI_DWHITE" ");
                   break;
              case 13:

                   /* ---->  Re-type new password ('@password')  <---- */
                   server_set_echo(d,0); 
                   output(d,d->player,2,0,0,TELNET_VERIFY_NEW_PASSWORD_PROMPT""ANSI_DWHITE" ");
                   break;
	      case 14:

                   /* ---->  Enter password to leave AFK  <---- */
                   server_set_echo(d,0);
                   output(d,d->player,2,0,0,AFK_PASSWORD_PROMPT""ANSI_DWHITE" ");
                   break;
              case 15:

                   /* ---->  Take over existing connection  <---- */
                   output(d,d->player,2,0,0,TAKE_OVER_CONNECTION_PROMPT""ANSI_DWHITE" ",tcz_full_name);
                   break;
              case 16:
              case 18:

                   /* ---->  Local echo  <---- */
                   output(d,d->player,2,0,0,LOCAL_ECHO_PROMPT""ANSI_DWHITE" ");
                   break;
              case 17:

                   /* ---->  Time difference  <---- */
                   if(1) {
                      time_t now;

                      gettime(now);
                      output(d,d->player,2,0,0,TELNET_TIME_DIFFERENCE_PROMPT""ANSI_DWHITE" ",tcz_full_name,date_to_string(now,UNSET_DATE,d->player,FULLDATEFMT),tcz_timezone,tcz_prompt);
		   }
                   break;
              case 19:
              case 21:

                   /* ---->  ANSI colour ('screenconfig')  <---- */
                   if(1) {
                      int cached_flags = d->flags;

                      add_cr = cached_addcr;
                      d->flags &= ~(ANSI|ANSI8);
                      output(d,d->player,2,0,0,SET_ANSI_COLOUR_TABLE_HELP,tcz_full_name,tcz_full_name,tcz_prompt);

                      d->flags |= ANSI;
                      output(d,d->player,2,0,0,SET_ANSI_COLOUR_TABLE);
                      d->flags &= ~ANSI;

                      if(d->flags & TELNET_EOR) add_cr = 255;
                      output(d,d->player,2,0,0,SET_ANSI_COLOUR_PROMPT" ");
                      d->flags = cached_flags;
		   }
                   break;
              case 20:
              case 22:

                   /* ---->  Underlining ('screenconfig')  <---- */
                   if(1) {
                      int cached_flags = d->flags;

                      termflags = 0, d->flags |= UNDERLINE;
                      output(d,d->player,2,0,0,UNDERLINE_PROMPT""ANSI_DWHITE" ");
                      termflags = TXT_NORMAL, d->flags = cached_flags;
		   }
                   break;
              case 26:

                   /* ---->  Enter E-mail address (Request for new character via HTML Interface)  <---- */
                   output(d,d->player,2,0,0,HTML_REQUEST_EMAIL_PROMPT""ANSI_DWHITE" ",tcz_admin_email);
                   break;
              case 27:

                   /* ---->  Enter E-mail address (Request for new character via Telnet)  <---- */
                   output(d,d->player,2,0,0,TELNET_REQUEST_EMAIL_PROMPT""ANSI_DWHITE" ",tcz_admin_email);
                   break;
              case 28:

                   /* ---->  Enter preferred name (Create new character)  <---- */
                   output(d,d->player,2,0,0,PREFERRED_NAME_PROMPT""ANSI_DWHITE" ");
                   break;
	      case 30:

                   /* ---->  {J.P.Boggis 17/06/2001}  Simple 'more' pager (Optionally used by disclaimer and/or title screens at login)  <---- */
                   break;
              case 31:

                   /* ---->  {J.P.Boggis 19/06/2001}  Simple 'more' paged disclaimer at login screen  <---- */
                   d->clevel = 1;
                   d->page   = 0;
                   output(d,d->player,2,0,0,"\n");
		   prompt_display(d);
                   break;
              default:
                   break;
       }
       termflags = 0;
     }

     if(d->clevel == 30) {
        if(d->page != TCZ_INFINITY) {  /*  Required for re-accept disclaimer prompt  */

           /* ---->  {J.P.Boggis 17/06/2001}  Simple 'more' pager (Optionally used by disclaimer and/or title screens at login)  <---- */
	   int   line,page,pos,firstpage = 0,pageadjust = 0;
	   const char *ptr = NULL,*userprompt = NULL;
	   char  buffer[BUFFER_LEN];
	   char  *dest;

           /* ---->  Get text to page using simple 'more' pager  <---- */
	   switch(d->page_clevel) {
	          case 1:
		       ptr = decompress(help_get_titlescreen(d->page_title));
	   	       break;
	          case 0:
	          case 7:
	          case 23:
	          case 24:
	          case 29:
                  case 31:
	               ptr = decompress(disclaimer);
	               break;
	   }

 	   /* ---->  Set page adjustment to compensate for user prompt, shown after simple 'more' pager  <---- */
	   switch(d->page_clevel) {
	          case 1:
                  case 31:
                       pageadjust = strcnt(userprompt = NEWBIE_PROMPT,'\n');
	               break;
	          case 29:
                       pageadjust = strcnt(userprompt = REACCEPT_DISCLAIMER_PROMPT,'\n');
		       break;
                  default:
		       pageadjust = 0;
	   }

	   if(ptr && (d->page != NOTHING) && !d->html) {

	      /* ---->  Seek start of page  <---- */
	      dest = buffer;
	      for(pos = 0, page = 1, line = 1; *ptr && (page < d->page); ptr++) {
	          if(*ptr == '\n') {
  	             if(pos > d->terminal_width)
		        line += (pos / (d->terminal_width + 1));
		     line++;
                     pos = 0;

		     if(line >= (d->terminal_height - 1)) {
	                line = 1;
		        page++;
		     }
		  } else pos++;
	      }

	      if(*ptr) {

   	         /* ---->  Copy page  <---- */
	         for(pos = 0, dest = buffer, line = 1; *ptr && (line < d->terminal_height); *dest++ = *ptr++) {
		     if(*ptr == '\n') {
		        if(pos > d->terminal_width)
		   	   line += (pos / (d->terminal_width + 1));
		        line++;
		        pos = 0;
		     } else pos++;
		 } 
	         *dest-- = '\0';

                 if(d->page == 1) firstpage = 1;

	         if(!*ptr) {

                    /* ---->  No further pages, end pager  <---- */
                    if(d->page_clevel != 1) for(; (dest > buffer) && (*dest == '\n'); *dest-- = '\0');
		    if(d->page > 1) {
	               for(ptr = buffer; *ptr && (*ptr != '\n'); ptr++);
		       if(*ptr && (*ptr == '\n')) ptr++;
		    } else ptr = buffer;

                    /* ---->  Last page:  Room to display with user prompt?  <---- */
                    if((pageadjust > 0) && (d->page != (TCZ_INFINITY - 1)))
                       if((line + pageadjust) >= d->terminal_height)
                          d->page = (TCZ_INFINITY - 1);

                    /* ---->  No further pages, end pager  <---- */
		    if(d->page != (TCZ_INFINITY - 1)) {
		       d->clevel = d->page_clevel;
		       d->page = NOTHING, d->page_clevel = 0;
		    }
		 } else {
                    dest = buffer + strlen(buffer) - 1;
		    ptr  = buffer;
		    d->page++;
		 }

	         /* ---->  Output page  <---- */
	         termflags = 0, add_cr = cached_addcr;
	         output(d,d->player,2,1,0,"%s"ANSI_LRED"%s%s%s",(firstpage) ? "\n\n":"\n",ptr,((dest >= buffer) && (*dest == '\n')) ? "":"\n",(d->clevel == 0) ? "\n":"");
	         termflags = TXT_NORMAL;

	         if(d->clevel == 30) {
	            if(d->flags & TELNET_EOR) add_cr = 255;
	            output(d,d->player,2,0,0,SIMPLE_PAGER_PROMPT""ANSI_DWHITE" ");
		 } else {
                    if((d->clevel != 31) && userprompt && (*userprompt != '\n'))
		       output(d,d->player,0,0,0,"");
                    prompt_display(d);
                    d->page = 0;
		 }
	      } else {

	         /* ---->  No further pages, end pager  <---- */
 	         d->clevel = d->page_clevel;
                 if(((d->clevel != 31) || (d->page == (TCZ_INFINITY - 1))) && userprompt && (*userprompt != '\n'))
		    output(d,d->player,0,0,0,"");
	         d->page = NOTHING, d->page_clevel = 0;
                 prompt_display(d);
	      }
	   } else {

	      /* ---->  No further pages, end pager  <---- */
	      d->clevel = d->page_clevel;
	      d->page = NOTHING, d->page_clevel = 0;
              if((d->clevel != 31) && userprompt && (*userprompt != '\n'))
                 output(d,d->player,0,0,0,"");
              prompt_display(d);
	   }
	} else d->page = 1;  /*  Required for re-accept disclaimer prompt  */
     }

     if(!(d->flags & TELNET_EOR)) d->flags |= PROMPT;
     add_cr = cached_addcr;
}

/* ---->  Process interactive '@prompt' session input  <---- */
void prompt_interactive_input(struct descriptor_data *d,char *input)
{
     const char *cached_curcmdptr;
     char  buffer[TEXT_SIZE];
     dbref cached_curchar;
     char  *p1,*p2;

     /* ---->  Validate character  <---- */
     if(!Validchar(d->player)) {
        writelog(BUG_LOG,1,"BUG","(prompt_interactive_input() in prompt.c)  Invalid character %s.",unparse_object(ROOT,d->player,0));
        return;
     }

     /* ---->  Strip leading/trailing blanks  <---- */
     setreturn(ERROR,COMMAND_FAIL);
     for(; *input && (*input == ' '); input++);
     for(p1 = input + strlen(input) - 1; (input >= p1) && (*p1 == ' '); *p1-- = '\0');
     cached_curcmdptr = current_cmdptr;
     cached_curchar   = current_character;
     current_cmdptr   = input;

     if(!strcasecmp("ABORT",input)) {

        /* ---->  Abort interactive '@prompt' session?  <---- */
        if(Valid(d->prompt->fail))
           command_cache_execute(d->player,d->prompt->fail,1,0);
              else output(d,d->player,0,1,0,ANSI_LGREEN"Interactive '"ANSI_LWHITE"@prompt"ANSI_LGREEN"' session aborted.");
        if(d->prompt->temp) temp_clear(&(d->prompt->temp),NULL);
        FREENULL(d->prompt->prompt);
        FREENULL(d->prompt);
     } else {
        for(p1 = input, p2 = buffer; *p1 && (*p1 != ' '); *p2++ = *p1, p1++);
        for(; *p1 && (*p1 == ' '); p1++);
        *p2 = '\0';
        if(!strcasecmp("help",buffer) || !strcasecmp("man",buffer) || !strcasecmp("manual",buffer)) {

           /* ---->  Call On-Line Help System  <---- */
           help_main(d->player,p1,NULL,NULL,NULL,0,0);
	} else if(*buffer == PROMPT_CMD_TOKEN) {
           unsigned char abort = 0;
           const    char *ptr;

           /* ---->  Execute TCZ command  <---- */
           ptr = (!strcasecmp("x",buffer + 1) || string_prefix("execute",buffer + 1)) ? p1:input + 1;
           abort |= event_trigger_fuses(d->player,d->player,ptr,FUSE_ARGS);
           abort |= event_trigger_fuses(d->player,Location(d->player),ptr,FUSE_ARGS);
           if(!abort) process_basic_command(d->player,(char *) ptr,0);
	} else {

           /* ---->  Interactive '@prompt' session input  <---- */
           if(Valid(d->prompt->process)) {
              strcpy(buffer,input);
              p1 = strchr(input,'=');
              if(!Blank(p1)) {
                 for(*p1 = '\0', p2 = p1 - 1; (p2 >= input) && (*p2 == ' '); *p2-- = '\0');
                 for(p1++; *p1 && (*p1 == ' '); p1++);
                 command_arg1 = input;
                 command_arg2 = p1;
	      } else {
                 command_arg1 = input;
                 command_arg2 = "";
	      }
              command_arg0 = "@prompt";
              command_arg3 = buffer;
              command_cache_execute(d->player,d->prompt->process,1,0);
              if(command_boolean == COMMAND_SUCC) {
                 struct temp_data *cached_temp = tempptr;
                 dbref  succ = d->prompt->succ;

                 /* ---->  Interactive '@prompt' session finished?  <---- */
                 tempptr = d->prompt->temp;
                 FREENULL(d->prompt->prompt);
                 FREENULL(d->prompt);
                 if(Valid(succ)) command_cache_execute(d->player,succ,0,0);
                 temp_clear(&tempptr,cached_temp);
	      }
	   } else {
              output(d,d->player,0,1,0,ANSI_LGREEN"The compound command which processes the input from this interactive '"ANSI_LWHITE"@prompt"ANSI_LGREEN"' session is no-longer valid  -  Interactive '"ANSI_LWHITE"@prompt"ANSI_LGREEN"' session aborted.");
              if(d->prompt->temp) temp_clear(&(d->prompt->temp),NULL);
              FREENULL(d->prompt->prompt);
              FREENULL(d->prompt);
	   }
	}
     }
     current_character = cached_curchar;
     current_cmdptr    = cached_curcmdptr;
}

/* ---->  Start interactive prompt session for user input ('@prompt')  <---- */
void prompt_interactive(CONTEXT)
{
     dbref                    process = NOTHING,succ = NOTHING,fail = NOTHING;
     struct   descriptor_data *p = getdsc(player);
     char                     *p2,*processcmd,*succcmd,*failcmd;
     char                     buffer2[BUFFER_LEN];
     char                     buffer[BUFFER_LEN];
     unsigned char            check = 1;
     const    char            *p1;

     setreturn(ERROR,COMMAND_FAIL);
     if(!p) return;

     if(arg2) split_params((char *) arg2,&processcmd,&succcmd);
     if(succcmd) split_params(succcmd,&succcmd,&failcmd);
     if(!Blank(arg1)) {
        if(!Blank(processcmd)) {
           if(!Blank(succcmd)) {
              process = parse_link_command(player,NOTHING,processcmd,1);
              if(!Valid(process)) return;
              succ = parse_link_command(player,NOTHING,succcmd,2);
              if(!Valid(succ)) return;
              if(!Blank(failcmd)) {
                 fail = parse_link_command(player,NOTHING,failcmd,3);
                 if(!Valid(fail)) return;
	      }

              if(!p->edit) {
                 if(!p->prompt) {
                    sprintf(buffer2,"%s%s ",punctuate((char *) arg1,2,'\0'),ANSI_DWHITE);
                    if(check && !Level4(Owner(player))) {
                    p1 = buffer2, p2 = buffer;
                    while(*p1)
                          if(*p1 == '%') {
                             if(*(++p1)) p1++;
			  } else *p2++ = *p1, p1++;
                          *p2 = '\0';
                          if(!strncasecmp(buffer,tcz_prompt,strlen(tcz_prompt))) {
                             output(p,player,0,1,0,ANSI_LGREEN"Sorry, you can't use '"ANSI_LYELLOW"%s"ANSI_LGREEN"' (The standard prompt) as the prompt for '"ANSI_LWHITE"@prompt"ANSI_LGREEN"'.",tcz_prompt);
                             return;
			  }
                          check = 0;
		    }

                    MALLOC(p->prompt,struct prompt_data);
                    p->prompt->process = process;
                    p->prompt->prompt  = (char *) alloc_string(buffer2);
                    p->prompt->temp    = NULL;
                    p->prompt->succ    = succ;
                    p->prompt->fail    = fail;
                    setreturn(OK,COMMAND_SUCC);
		 } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@prompt"ANSI_LGREEN"' can't be used from within another interactive '"ANSI_LWHITE"@prompt"ANSI_LGREEN"' session.");
	      } else output(p,player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@prompt"ANSI_LGREEN"' can't be used from within the editor.");
  	   } else output(p,player,0,1,0,ANSI_LGREEN"Please specify the compound command to execute when "ANSI_LYELLOW"<PROCESS CMD>"ANSI_LGREEN" succeeds and ends the interactive '"ANSI_LWHITE"@prompt"ANSI_LGREEN"' session.");
	} else output(p,player,0,1,0,ANSI_LGREEN"Please specify the compound command to execute to process user input from the interactive '"ANSI_LWHITE"@prompt"ANSI_LGREEN"' session.");
     } else output(p,player,0,1,0,ANSI_LGREEN"Please specify the user prompt (E.g:  '"ANSI_LWHITE"@prompt Please input something: = <PROCESS CMD> = <SUCC CMD> [= <ABORT CMD>]"ANSI_LGREEN"'.");
}

/* ---->  {J.P.Boggis 02/10/2000}  Free allocated fields of prompt for arguments to internal command  <---- */
void prompt_user_free(struct descriptor_data *d)
{
     if(d->cmdprompt) {
        FREENULL(d->cmdprompt->defaultval);
        FREENULL(d->cmdprompt->blankmsg);
        FREENULL(d->cmdprompt->command);
        /* FREENULL(d->cmdprompt->setarg); - Don't free an integer */
        FREENULL(d->cmdprompt->params);
        FREENULL(d->cmdprompt->arg1);
        FREENULL(d->cmdprompt->arg2);
        FREENULL(d->cmdprompt->arg3);
        FREENULL(d->cmdprompt->prompt);
     }
}

/* ---->  {J.P.Boggis 02/10/2000}  Process input of prompt for arguments to internal command  <---- */
void prompt_user_input(struct descriptor_data *d,char *input)
{
     char *ptr;

     /* ---->  Validate character  <---- */
     if(!Validchar(d->player)) {
        writelog(BUG_LOG,1,"BUG","(prompt_user_input() in prompt.c)  Invalid character %s.",unparse_object(ROOT,d->player,0));
        return;
     }

     /* ---->  Strip leading/trailing blanks  <---- */
     setreturn(ERROR,COMMAND_FAIL);
     for(; *input && (*input == ' '); input++);
     for(ptr = input + strlen(input) - 1; (input >= ptr) && (*ptr == ' '); *ptr-- = '\0');

     /* ---->  Interactive prompt session  <---- */
     if(strcasecmp("ABORT",input)) {
        if(!(Blank(input) && Blank(d->cmdprompt->defaultval) && !Blank(d->cmdprompt->blankmsg))) {
           const char *params = d->cmdprompt->params;
	   const char *arg1   = d->cmdprompt->arg1;
	   const char *arg2   = d->cmdprompt->arg2;
	   const char *arg3   = d->cmdprompt->arg3;
           char       buffer[BUFFER_LEN];

	   /* ---->  Interactive prompt session input  <---- */
	   if(Blank(input)) input = (char *) d->cmdprompt->defaultval;

           /* ---->  Set appropriate argument with user input  <---- */
           switch(d->cmdprompt->setarg) {
                  case 0:

                       /* ---->  Set PARAMS  <---- */
                       params = input;
                       break;
                  case 1:

                       /* ---->  Set ARG1  <---- */
                       arg1 = input;
                       break;
                  case 2:

                       /* ---->  Set ARG2  <---- */
                       arg2 = input;
                       break;
                  case 3:

                       /* ---->  Set ARG3  <---- */
                       arg3 = input;
                       break;
                  default:
                       break;
	   }

           /* ---->  Create command execution string using new arguments  <---- */
           if(d->cmdprompt->setarg)
              sprintf(buffer,"%s%s %s%s%s%s%s",(d->cmdprompt->command && (*(d->cmdprompt->command) == '|')) ? "":"|",String(d->cmdprompt->command),String(arg1),!Blank(arg2) ? "":" = ",String(arg2),!Blank(arg3) ? "":" = ",String(arg3));
	         else sprintf(buffer,"%s%s %s",(d->cmdprompt->command && (*(d->cmdprompt->command) == '|')) ? "":"|",String(d->cmdprompt->command),String(params));

           /* ---->  Execute command and free interactive prompt session  <---- */
           process_basic_command(d->player,buffer,0);
           prompt_user_free(d);
	} else output(d,d->player,0,1,0,"%s",d->cmdprompt->blankmsg);

        /* ---->  Abort interactive prompt session  <---- */
     } else prompt_user_free(d);
}

/* ---->  {J.P.Boggis 02/10/2000}  Prompt user for arguments to internal command  <---- */
/*        PLAYER     = DBref of character  */
/*        PROMPT     = User prompt (E.g:  'Enter description: ')  */
/*        PROMPTMSG  = Optional message to display to user, above first instance of prompt  */
/*        DEFAULTVAL = Default value, set if user enters nothing at prompt  */
/*        BLANKMSG   = Optional error message, displayed if user enters nothing at prompt  */
/*        INCMDMSG   = Message to display to user if called from within a compound command or interactive prompting is disabled  */
/*        COMMAND    = Internal command to execute (E.g:  '@describe'.)  */
/*        SETARG     = Argument number to set with value entered at user prompt (See below)  */
/*        PARAMS     = (0)  Complete command arguments  */
/*        ARG1       = (1)  First command argument  (ARG1 = arg2 [= arg3])  */
/*        ARG2       = (2)  Second command argument (arg1 = ARG2 [= arg3])  */
/*        ARG3       = (3)  Third command argument  (arg1 = arg2 [= ARG3])  */
int prompt_user(dbref player,const char *prompt,const char *promptmsg,const char *defaultval,const char *blankmsg,const char *incmdmsg,const char *command,int setarg,const char *params,const char *arg1,const char *arg2,const char *arg3)
{
    struct descriptor_data *p = getdsc(player);

    if(Validchar(player) && p && !Blank(prompt) && !Blank(command) && (setarg >= 0) && (setarg <= 3)) {
       if(!in_command && PrefsPrompt(player)) {
          char buffer[BUFFER_LEN];

          if(!Blank(promptmsg)) output(p,player,0,1,0,"%s\n",promptmsg);
          MALLOC(p->cmdprompt,struct cmdprompt_data);

          snprintf(buffer,BUFFER_LEN,"%s%s",(*prompt == '\x1B') ? "":ANSI_LWHITE,prompt);
          p->cmdprompt->prompt     = (char *) alloc_string(buffer);

          p->cmdprompt->defaultval = (char *) alloc_string(defaultval);
          p->cmdprompt->blankmsg   = (char *) alloc_string(blankmsg);
          p->cmdprompt->command    = (char *) alloc_string(command);
          p->cmdprompt->setarg     = setarg;
          p->cmdprompt->params     = (char *) alloc_string(params);
          p->cmdprompt->arg1       = (char *) alloc_string(arg1);
          p->cmdprompt->arg2       = (char *) alloc_string(arg2);
          p->cmdprompt->arg3       = (char *) alloc_string(arg3);
          return(1);
       } else if(!Blank(incmdmsg)) output(p,player,0,1,0,"%s%s",(*incmdmsg == '\x1B') ? "":ANSI_LGREEN,incmdmsg); 
    }
    return(0);
}
