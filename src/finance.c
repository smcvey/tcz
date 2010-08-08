/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| FINANCE.C  -  Implement virtual financial aspects of TCZ, such as credit,   |
|               bank accounts and payment of credit between users.            |
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
| Module originally designed and written by:  J.P.Boggis 03/01/1997.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: finance.c,v 1.3 2005/06/29 21:04:50 tcz_monster Exp $

*/


#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "object_types.h"
#include "flagset.h"


/* ---->  Convert currency_data to double  <---- */
double currency_to_double(struct currency_data *currency)
{
       double decimal,fraction,value = 0;

       if(currency) {
          decimal = (double) currency->decimal;
          fraction = (double) currency->fraction;
          value = decimal + (fraction / 100.0);
       }
       return(value);
}

/* ---->  Convert double to currency_data  <---- */
struct currency_data *double_to_currency(struct currency_data *currency,double value)
{
       static struct currency_data temp;
       double decimal,fraction;

       fraction = modf(value,&decimal);
       if(currency) {
          currency->decimal  = (int)           decimal;
          currency->fraction = (unsigned char) floor(fraction * 100);
          return(currency);
       } else {
          temp.decimal  = (int)           decimal;
          temp.fraction = (unsigned char) floor(fraction * 100);
          return(&temp);
       }
}

/* ---->  Add to/subtract from currency_data  <---- */
void currency_add(struct currency_data *currency,double value)
{
     if(currency) {
        value += currency_to_double(currency);
        double_to_currency(currency,value);
     }
}

/* ---->  Compare currency (Returns difference)  <---- */
double currency_compare(struct currency_data *currency1,struct currency_data *currency2)
{
       double value1,value2;

       value1 = currency_to_double(currency1);
       value2 = currency_to_double(currency2);
       return(value1 - value2);
}

/* ---->  Go to bank room  <---- */
void finance_bank(CONTEXT)
{
     setreturn(ERROR,COMMAND_FAIL);
     if(Valid(bankroom)) {
	if(db[player].location != bankroom) {
           if(!Invisible(db[player].location)) output_except(db[player].location,player,NOTHING,0,1,2,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" heads towards "ANSI_LYELLOW"The Bank of %s"ANSI_LGREEN".",Article(player,UPPER,INDEFINITE),getcname(NOTHING,player,0,0),tcz_short_name);
           move_enter(player,bankroom,1);
           output(getdsc(player),player,0,1,0,ANSI_LGREEN"For full details on how to use "ANSI_LYELLOW"The Bank of %s"ANSI_LGREEN", please type '"ANSI_LWHITE"help bank"ANSI_LGREEN"'.\n",tcz_short_name);
           setreturn(OK,COMMAND_SUCC);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you're already in "ANSI_LYELLOW"The Bank of %s"ANSI_LGREEN"  -  For full details on how to use the bank, type '"ANSI_LWHITE"help bank"ANSI_LGREEN"'.",tcz_short_name);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, "ANSI_LYELLOW"The Bank of %s"ANSI_LGREEN" is unavailable at the moment.",tcz_short_name);
}

/* ---->  Credit/debit account or pocket of user  <---- */
void finance_credit(CONTEXT)
{
     unsigned char account = (in_command) ? 1:0;
     char     *amnt,*dest,*reason,*destination;
     double   amount,credit,other;
     dbref    who;

     setreturn(ERROR,COMMAND_FAIL);
     if(Level4(player)) {
        if(!in_command) {
           if(!Blank(params)) {
              if(!Blank(arg1)) {
                 if((who = lookup_character(player,arg1,1)) == NOTHING) {
                    output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
                    return;
		 }
              } else who = Owner(player);

              if((who != player) || Root(player)) {
                 if(!Blank(arg2)) {
                    split_params((char *) arg2,&amnt,&destination);
                    if(!Blank(destination)) {
                       split_params(destination,&dest,&reason);
                       if(!Blank(dest)) {
                          if(string_prefix("account",dest) || string_prefix("bankaccount",dest) || string_prefix("balance",dest) || string_prefix("bank account",dest) || string_prefix("bank balance",dest)) account = 1;
         	             else if(string_prefix("pockets",dest) || string_prefix("credits",dest)) account = 0;
                                else {
                                   output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LYELLOW"pocket"ANSI_LGREEN"' (User's pocket) or '"ANSI_LYELLOW"account"ANSI_LGREEN"' (User's bank account.)");
                                   return;
				}

       	                  if(!Blank(amnt)) {
                             if(!Blank(reason)) {
                                credit = currency_to_double((account) ? &(db[who].data->player.balance):&(db[who].data->player.credit));
                                other  = currency_to_double((account) ? &(db[who].data->player.credit):&(db[who].data->player.balance));
                                if(((amount = tofloat(amnt,NULL)) >= credit) || Level3(player)) {
                                   if(((credit + other) <= RAISE_RESTRICTION) || (amount < credit) || Level2(player)) {
                                      if(((amount + other) <= RAISE_MAXIMUM) || Level2(player)) {
                                         if(amount >= 0) {
                                            if(account) {
                                               if(amount > currency_to_double(&(db[who].data->player.balance)))
                                                  currency_add(&(db[who].data->player.income),(amount - currency_to_double(&(db[who].data->player.balance))));
                                                     else currency_add(&(db[who].data->player.expenditure),(currency_to_double(&(db[who].data->player.balance)) - amount));
                                               double_to_currency(&(db[who].data->player.balance),amount);
					    } else {
                                               if(amount > currency_to_double(&(db[who].data->player.credit)))
                                                  currency_add(&(db[who].data->player.income),(amount - currency_to_double(&(db[who].data->player.credit))));
                                                     else currency_add(&(db[who].data->player.expenditure),(currency_to_double(&(db[who].data->player.credit)) - amount));
                                               double_to_currency(&(db[who].data->player.credit),amount);
					    }
                                            
					    output(getdsc(player),player,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" now has "ANSI_LYELLOW"%.2f"ANSI_LGREEN" credits in %s %s.",Article(who,UPPER,DEFINITE),getcname(NOTHING,who,0,0),amount,Possessive(who,0),(account) ? "bank account":"pocket");
                                            output(getdsc(who),who,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"%s"ANSI_LYELLOW"%s"ANSI_LWHITE" has changed the credit in your %s to "ANSI_LYELLOW"%.2f"ANSI_LWHITE" credits.",Article(player,UPPER,(Location(player) == Location(who)) ? DEFINITE:INDEFINITE),getcname(NOTHING,player,0,0),(account) ? "bank account":"pocket",amount);
                                            writelog(ADMIN_LOG,0,"CREDIT","%s(#%d) changed %s(#%d)'s %s to %.2f credits.",getname(player),player,getname(who),who,(account) ? "bank balance":"credit in pocket",amount);
                                            writelog(TRANSACTION_LOG,0,"CREDIT","%s(#%d) changed %s(#%d)'s %s to %.2f credits.",getname(player),player,getname(who),who,(account) ? "bank balance":"credit in pocket",amount);
                                            setreturn(OK,COMMAND_SUCC);
					 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" can't have a negative amount of credit in %s %s.",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0),Possessive(who,0),(account) ? "bank account":"pocket");
				      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only change the credit in %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s %s to a maximum of "ANSI_LYELLOW"%.2f"ANSI_LGREEN" credits.",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0),(account) ? "bank account":"pocket",RAISE_MAXIMUM - other);
				   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" currently has a total of "ANSI_LYELLOW"%.2f"ANSI_LGREEN" credits.  %s must have less than "ANSI_LYELLOW"%.2f"ANSI_LGREEN" before you can raise the credit in %s %s.",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0),credit + other,Subjective(who,1),RAISE_RESTRICTION,Possessive(who,0),(account) ? "bank account":"pocket");
				} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Wizards/Druids and above can reduce the credit in %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s %s.",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0),(account) ? "bank account":"pocket");
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the reason for adjusting the credit in %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s %s.",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0),(account) ? "bank account":"pocket");
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the new value for %s"ANSI_LYELLOW"%s"ANSI_LGREEN" credit in %s %s.",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0),Possessive(who,0),(account) ? "bank account":"pocket");
		       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify whether the adjustment will be made to %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s "ANSI_LYELLOW"pocket"ANSI_LGREEN" or bank "ANSI_LYELLOW"account"ANSI_LGREEN".",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0));
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify whether the adjustment will be made to %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s "ANSI_LYELLOW"pocket"ANSI_LGREEN" or bank "ANSI_LYELLOW"account"ANSI_LGREEN".",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0));
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the new value in credits, and whether the adjustment will be made to %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s "ANSI_LYELLOW"pocket"ANSI_LGREEN" or bank "ANSI_LYELLOW"account"ANSI_LGREEN".",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0));
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't adjust your own credit.");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify which character's credit you'd like to adjust.");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't adjust a character's credit from within a compound command.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, only Apprentice Wizards/Druids and above may adjust the credit of a character.");
}

/* ---->  Deposit into bank account  <---- */
void finance_deposit(CONTEXT)
{
     double amount;

     setreturn(ERROR,COMMAND_FAIL);
     if(!(Blank(params) || !(((amount = tofloat(params,NULL)) >= 0.01) || (amount < 0)))) {
        if(amount >= 0) {
           if((currency_to_double(&(db[player].data->player.credit)) - amount) >= 0) {
              currency_add(&(db[player].data->player.credit),0 - amount);
              currency_add(&(db[player].data->player.balance),amount);
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"You insert your bank card and deposit "ANSI_LYELLOW"%.2f"ANSI_LGREEN" credits into your bank account.\n\nYour balance now stands at %s%.2f"ANSI_LGREEN" credits.\n",amount,(currency_to_double(&(db[player].data->player.balance)) < 0) ? ANSI_LRED:ANSI_LWHITE,currency_to_double(&(db[player].data->player.balance)));
              if(!Invisible(db[player].location)) output_except(db[player].location,player,NOTHING,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" inserts %s bank card and deposits some credit into %s bank account.",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0),Possessive(player,0),Possessive(player,0));
              if(!in_command || !Wizard(current_command))
                 writelog(TRANSACTION_LOG,0,"DEPOSIT","%s(#%d) deposited %.2f credits (Balance:  %.2f credits.)",getname(player),player,amount,currency_to_double(&(db[player].data->player.balance)));
              setreturn(OK,COMMAND_SUCC);
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have insufficient credit to deposit "ANSI_LYELLOW"%.2f"ANSI_LGREEN" credits into your bank account.",amount);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't deposit a negative amount of credit into your bank account.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify how much credit you would like to deposit into your bank account.");
}

/* ---->  Update hourly payment counter of user  <---- */
void finance_payment(struct descriptor_data *p)
{
     int    diff,multiplier = 1;
     struct descriptor_data *d;
     time_t now;

     /* ---->  Update counter, if idle time is less than IDLE_PAYMENT minutes  <---- */
     if(!p || !Validchar(p->player)) return;
     gettime(now);
     diff = (now - p->last_time);

     if((now - p->last_time) >= (IDLE_TIME * MINUTE))
        db[p->player].data->player.idletime += (now - p->last_time);

     p->warning_level = 0;
     p->last_time     = now;
     if(Puppet(p->player) || (diff > (IDLE_PAYMENT * MINUTE))) return;

     /* ---->  If user has more than one connection below IDLE_PAYMENT idle time, don't update counter  <---- */
     for(d = descriptor_list; d; d = d->next)
         if((d->player == p->player) && (d != p) && ((now - d->last_time) < (IDLE_PAYMENT * MINUTE)))
	    return;
     db[p->player].data->player.payment += diff;

     /* ---->  Pay user, if counter has reached one hour  <---- */
     while(db[p->player].data->player.payment > HOUR) {
           if((now - p->start_time) > (DOUBLE_PAYMENT * HOUR)) multiplier *= 2;
           if(Level4(p->player)) multiplier *= ADMIN_PAYMENT;
           currency_add(&(db[p->player].data->player.balance),(HOURLY_PAYMENT * multiplier));
           currency_add(&(db[p->player].data->player.income),(HOURLY_PAYMENT * multiplier));
           db[p->player].data->player.payment -= HOUR;
     }
}

/* ---->  Initialise new financial quarter  <---- */
void finance_quarter()
{
     dbref ptr;

     for(ptr = 0; ptr < db_top; ptr++)
         if(Typeof(ptr) == TYPE_CHARACTER) {
            double_to_currency(&(db[ptr].data->player.income),0);
            double_to_currency(&(db[ptr].data->player.expenditure),0);
	 }
     quarter += QUARTER;
}

/* ---->  Restrict maximum credit payable from within a compound command  <---- */
void finance_restrict(CONTEXT)
{
     int restriction;

     setreturn(OK,COMMAND_FAIL);
     if(!in_command || (Valid(current_command) && Wizard(current_command))) {
        if(!Blank(params)) {
           for(; *params && (*params == ' '); params++);
           restriction = atol(params);
           if(restriction || isdigit(*params)) {
              if(restriction >= 0) {
                 db[player].data->player.restriction = (restriction > 0xFFFF) ? 0xFFFF:restriction;
                 if(!in_command) output(getdsc(player),player,0,1,0,ANSI_LGREEN"Your payment restriction is now "ANSI_LYELLOW"%d"ANSI_LGREEN" credit%s.  You will not be able to pay out more than this amount from within compound commands owned by other characters (With the exception of those authorised by administrators.)",restriction,Plural(restriction));
                 setreturn(OK,COMMAND_SUCC);
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, your payment restriction cannot be negative.");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify your new payment restriction in credits, e.g:  '"ANSI_LWHITE"restrict %d"ANSI_LGREEN"'.",DEFAULT_RESTRICTION);
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify your new payment restriction in credits, e.g:  '"ANSI_LWHITE"restrict %d"ANSI_LGREEN"'.",DEFAULT_RESTRICTION);
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't change your payment restriction from within a compound command.");
}

/* ---->  View statement of account  <---- */
void finance_statement(CONTEXT)
{
     struct   descriptor_data *w,*p = getdsc(player);
     unsigned char twidth = output_terminal_width(player);
     double   credit,balance,income,expenditure;
     double   avgday,avgweek,avgmonth;
     int      daysleft;
     double   profit;
     time_t   now;
     dbref    who;

     if(!Blank(params)) {
        if((who = lookup_character(player,params,1)) == NOTHING) {
           output(p,player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",params);
           return;
	}
     } else who = player;

     credit      = currency_to_double(&(db[who].data->player.credit));
     balance     = currency_to_double(&(db[who].data->player.balance));
     income      = currency_to_double(&(db[who].data->player.income));
     expenditure = currency_to_double(&(db[who].data->player.expenditure));

     html_anti_reverse(p,1);
     if(IsHtml(p)) output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
     if(!in_command) {
        output(p,player,2,1,1,"%sStatement of account for %s"ANSI_LWHITE"%s"ANSI_LCYAN"...%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER><FONT SIZE=4><I>\016"ANSI_LCYAN:"\n ",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0),IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
        if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
     }

     if(IsHtml(p)) output(p,player,1,2,0,"<TR><TD>");
     output(p,player,2,1,1,ANSI_LMAGENTA" Balance: \016&nbsp;\016 "ANSI_LWHITE"%.2f "ANSI_LMAGENTA"credits.\n\n",balance);
     if(!credit) {
        if(who == player) output(p,player,2,1,1,ANSI_LYELLOW" You currently have no credit in your pocket.\n\n");
           else output(p,player,2,1,1,ANSI_LYELLOW" %s currently has no credit in %s pocket.\n\n",Subjective(who,1),Possessive(who,0));
     } else if(who == player) output(p,player,2,1,1,ANSI_LYELLOW" You currently have "ANSI_LWHITE"%.2f"ANSI_LYELLOW" credits in your pocket.\n\n",credit);
        else output(p,player,2,1,1,ANSI_LYELLOW" %s currently has "ANSI_LWHITE"%.2f"ANSI_LYELLOW" credits in %s pocket.\n\n",Subjective(who,1),credit,Possessive(who,0));
     output(p,player,2,1,23,ANSI_LRED" Payment restriction: \016&nbsp;\016 "ANSI_LWHITE"%d"ANSI_LRED" credit%s%s%s%s%s%s",db[who].data->player.restriction,Plural(db[who].data->player.restriction),(who == player) ? " ":"",((who == player) && IsHtml(p)) ? "\016<I>\016":"",(who == player) ? "(Type '"ANSI_LYELLOW"restrict <AMOUNT>"ANSI_LRED"' to change.)":"",((who == player) && IsHtml(p)) ? "\016</I>\016":"",IsHtml(p) ? "":"\n");

     if(IsHtml(p)) output(p,player,1,2,0,"</TD></TR><TR><TD BGCOLOR="HTML_TABLE_DGREY">");
        else output(p,player,0,1,0,separator(twidth,0,'-','-'));
     output(p,player,0,1,1,ANSI_LGREEN" Income/expenditure for the period "ANSI_LWHITE"%s"ANSI_LGREEN" to "ANSI_LWHITE"%s"ANSI_LGREEN"...\n",date_to_string(quarter - QUARTER,UNSET_DATE,player,SHORTDATEFMT),date_to_string(quarter,UNSET_DATE,player,SHORTDATEFMT));
     if(!IsHtml(p)) output(p,player,0,1,0,ANSI_LCYAN"               This quarter:   Average/day:   Average/week:   Average/month:\n"ANSI_DCYAN"               ~~~~~~~~~~~~~   ~~~~~~~~~~~~   ~~~~~~~~~~~~~   ~~~~~~~~~~~~~~");

     gettime(now);
     w            = getdsc(who);
     if((avgday   = (double) (now - (quarter - QUARTER)) / DAY)   < 1) avgday   = 1;
     if((avgweek  = (double) (now - (quarter - QUARTER)) / WEEK)  < 1) avgweek  = 1;
     if((avgmonth = (double) (now - (quarter - QUARTER)) / MONTH) < 1) avgmonth = 1;
     profit       = income - expenditure;
     daysleft     = (quarter - now) / DAY;

     if(IsHtml(p)) {
        output(p,player,1,2,0,"<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">");
        output(p,player,2,1,0,"%s","\016<TR ALIGN=CENTER BGCOLOR="HTML_TABLE_CYAN"><TH><FONT COLOR="HTML_LCYAN"><I>&nbsp;</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN"><I>This quarter:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN"><I>Average/day:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN"><I>Average/week:</I></FONT></TH><TH><FONT COLOR="HTML_LCYAN"><I>Average/month:</I></FONT></TH></TR>\016");
     }
     output(p,player,2,1,0,"%s"ANSI_LYELLOW"Income:%s"ANSI_LWHITE"%-16.2f%s%-15.2f%s%-16.2f%s%-.2f%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=20% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"      ",IsHtml(p) ? "\016</I></TH><TD WIDTH=20%>\016":"  ",income,IsHtml(p) ? "\016</TD><TD WIDTH=20%>\016":"",income / avgday,IsHtml(p) ? "\016</TD><TD WIDTH=20%>\016":"",income / avgweek,IsHtml(p) ? "\016</TD><TD WIDTH=20%>\016":"",income / avgmonth,IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,0,"%s"ANSI_LYELLOW"Expenditure:%s"ANSI_LWHITE"%-16.2f%s%-15.2f%s%-16.2f%s%-.2f%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=20% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":" ",IsHtml(p) ? "\016</I></TH><TD WIDTH=20%>\016":"  ",expenditure,IsHtml(p) ? "\016</TD><TD WIDTH=20%>\016":"",expenditure / avgday,IsHtml(p) ? "\016</TD><TD WIDTH=20%>\016":"",expenditure / avgweek,IsHtml(p) ? "\016</TD><TD WIDTH=20%>\016":"",expenditure / avgmonth,IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,0,"%s"ANSI_LYELLOW"Profit:%s"ANSI_LWHITE"% -16.2f%s% -15.2f%s% -16.2f%s% -.2f%s",IsHtml(p) ? "\016<TR ALIGN=CENTER><TH ALIGN=RIGHT WIDTH=20% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"      ",IsHtml(p) ? "\016</I></TH><TD WIDTH=20%>\016":" ",profit,IsHtml(p) ? "\016</TD><TD WIDTH=20%>\016":"",profit / avgday,IsHtml(p) ? "\016</TD><TD WIDTH=20%>\016":"",profit / avgweek,IsHtml(p) ? "\016</TD><TD WIDTH=20%>\016":"",profit / avgmonth,IsHtml(p) ? "\016</TD></TR>\016":"\n");
     if(IsHtml(p)) output(p,player,1,2,0,"</TABLE>");

     output(p,player,0,1,0,"");
     output(p,player,0,1,18,ANSI_LMAGENTA" End of quarter: \016&nbsp;\016 "ANSI_LWHITE"%s \016&nbsp;\016"ANSI_LMAGENTA"("ANSI_LYELLOW"%d"ANSI_LMAGENTA" day%s remaining.)",date_to_string(quarter,UNSET_DATE,player,FULLDATEFMT),daysleft,Plural(daysleft));
     if(IsHtml(p)) output(p,player,1,2,0,"</TR></TD>");

     if(w && Connected(who) && (player == who) && !Puppet(who)) {
        unsigned char dbltime;

        if(IsHtml(p)) output(p,player,1,2,0,"<TR><TD>");
           else output(p,player,0,1,0,separator(twidth,0,'-','-'));
        dbltime = (w && ((now - w->start_time) / (DOUBLE_PAYMENT * HOUR))) ? 2:1;
        if(Level4(p->player)) dbltime *= 2;
        output(p,player,0,1,1,ANSI_LRED" You have "ANSI_LWHITE"%s"ANSI_LRED" remaining until your next payment of "ANSI_LYELLOW"%d"ANSI_LRED" credit%s.",interval(HOUR - db[who].data->player.payment,0,ENTITIES,0),HOURLY_PAYMENT * dbltime,Plural(HOURLY_PAYMENT * dbltime));
        if(IsHtml(p)) output(p,player,1,2,0,"</TR></TD>");
     }

     if(!IsHtml(p) && !in_command) output(p,player,0,1,0,separator(twidth,1,'-','='));
     if(IsHtml(p)) output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
     html_anti_reverse(p,0);
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Perform transaction with another user (I.e:  Pay credit to them or their bank account)  <---- */
/*        (val1:  0 = Payment from pocket ('pay'), 1 = Payment from bank account ('transaction'.))  */
void finance_transaction(CONTEXT)
{
     int      restriction = (val1) ? (WITHDRAWAL_RESTRICTION * MINUTE):(PAYMENT_RESTRICTION * MINUTE);
     unsigned char account = (in_command) ? 1:0;
     static   dbref pay_victim = NOTHING;
     static   dbref pay_who = NOTHING;
     static   long pay_time = 0;
     char     *amnt,*dest;
     double   amount;
     time_t   now;
     dbref    who;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(params)) {
        if(!Blank(arg1)) {
           if((who = lookup_character(player,arg1,1)) == NOTHING) {
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
              return;
	   }
        } else who = db[player].owner;

        if(in_command || Connected(player)) {
           if(!(command_type & FUSE_CMD) || (in_command && Valid(current_command) && Apprentice(current_command))) {
              if(!(command_type & AREA_CMD) || (in_command && Valid(current_command) && Apprentice(current_command))) {
                 if(!val1 || !in_command || (Valid(current_command) && ((player == db[current_command].owner) || Apprentice(current_command)))) {
                    if(who != player) {
                       time_t total;

                       gettime(now);
                       total = db[player].data->player.totaltime;
                       if(Connected(player)) total += (now - db[player].data->player.lasttime);
                       if(total >= restriction) {
                          if(!Blank(arg2)) {
                             split_params((char *) arg2,&amnt,&dest);
                             if(!Blank(amnt)) {
		                if(!Blank(dest)) {
                                   if(string_prefix("account",dest) || string_prefix("bankaccount",dest) || string_prefix("balance",dest) || string_prefix("bank account",dest) || string_prefix("bank balance",dest)) account = 1;
		                      else if(string_prefix("pockets",dest) || string_prefix("credits",dest)) account = 0;
                                         else {
                                            output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify either '"ANSI_LYELLOW"pocket"ANSI_LGREEN"' (User's pocket) or '"ANSI_LYELLOW"account"ANSI_LGREEN"' (User's bank account.)");
                                            return;
					 }
				}

                                if(((amount = tofloat(amnt,NULL)) >= 0.01) || (amount < 0)) {
                                   if(amount >= 0) {
                                      if(!in_command || (Valid(current_command) && ((player == db[current_command].owner) || Apprentice(current_command) || (amount <= db[player].data->player.restriction)))) {
                                         if((!val1 && ((currency_to_double(&(db[player].data->player.credit)) - amount) >= 0)) ||
                                            (val1  && ((currency_to_double(&(db[player].data->player.balance)) - amount) >= 0))) {
                                               currency_add(&(db[player].data->player.expenditure),amount);
                                               if(val1) currency_add(&(db[player].data->player.balance),0 - amount);
                                                  else currency_add(&(db[player].data->player.credit),0 - amount);
                                               currency_add(&(db[who].data->player.income),amount);
                                               if(account) currency_add(&(db[who].data->player.balance),amount);
                                                  else currency_add(&(db[who].data->player.credit),amount);

                                               if(!in_command) {
                                                  output(getdsc(player),player,0,1,0,ANSI_LGREEN"You pay "ANSI_LYELLOW"%.2f"ANSI_LGREEN" credits%s %s %s"ANSI_LWHITE"%s"ANSI_LGREEN"%s.",amount,(val1) ? " from your bank account":"",(account) ? "into":"to",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0),(account) ? "'s bank account":"");
                                                  if(!(!Level4(player) && (pay_who == player) && (pay_victim == who) && (now < pay_time))) {
                                                     output(getdsc(who),who,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" pays%s "ANSI_LYELLOW"%.2f"ANSI_LGREEN" credits%s.",Article(who,UPPER,(db[player].location == db[who].location) ? DEFINITE:INDEFINITE),getcname(NOTHING,player,0,0),(!account) ? " you":"",amount,(account) ? " into your bank account":"");
                                                     if((db[player].location == db[who].location) && !Invisible(db[player].location)) {
                                                        sprintf(scratch_return_string,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN,Article(player,UPPER,(db[player].location == db[who].location) ? DEFINITE:INDEFINITE),getcname(NOTHING,player,0,0));
                                                        output_except(db[player].location,player,who,0,1,0,"%s pays some credits to %s"ANSI_LWHITE"%s"ANSI_LGREEN".",scratch_return_string,Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0));
						     }
						  }
					       }

                                               if(!in_command || !Apprentice(current_command))
                                                  writelog(TRANSACTION_LOG,1,"PAYMENT","%s(#%d) payed %.2f credits into %s(#%d)'s %s.",getname(player),player,amount,getname(who),who,(account) ? "bank account":"pocket");
                                               pay_victim = who, pay_time = now + TRANSACTION_TIME, pay_who = player;
                                               setreturn(OK,COMMAND_SUCC);
					 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have insufficient credit%s to pay "ANSI_LYELLOW"%.2f"ANSI_LGREEN" credits %s %s"ANSI_LWHITE"%s"ANSI_LGREEN"%s.",(val1) ? " in your bank account":"",amount,(account) ? "into":"to",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0),(account) ? "'s bank account":"");
				      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, your payment restriction is currently "ANSI_LYELLOW"%d"ANSI_LGREEN" credit%s  -  You can't pay "ANSI_LYELLOW"%.2f"ANSI_LGREEN" credits %s %s"ANSI_LWHITE"%s"ANSI_LGREEN"%s (See '"ANSI_LWHITE"help bank"ANSI_LGREEN"' for details on how to change your payment restriction.)'",db[player].data->player.restriction,Plural(db[player].data->player.restriction),amount,(account) ? "into":"to",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0),(account) ? "'s bank account":"");
				   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't pay a negative amount of credit %s %s"ANSI_LWHITE"%s"ANSI_LGREEN"%s.",(account) ? "into":"to",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0),(account) ? "'s bank account":"");
				} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify how much credit%s you would like to pay %s %s"ANSI_LWHITE"%s"ANSI_LGREEN"%s.",(val1) ? " from your bank account":"",(account) ? "into":"to",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0),(account) ? "'s bank account":"");
			     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify how much credit%s you would like to pay %s %s"ANSI_LWHITE"%s"ANSI_LGREEN"%s.",(val1) ? " from your bank account":"",(account) ? "into":"to",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0),(account) ? "'s bank account":"");
			  } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify how much credit%s you would like to pay %s %s"ANSI_LWHITE"%s"ANSI_LGREEN"%s.",(val1) ? " from your bank account":"",(account) ? "into":"to",Article(who,LOWER,DEFINITE),getcname(NOTHING,who,0,0),(account) ? "'s bank account":"");
		       } else {
                          strcpy(scratch_return_string,interval(restriction - total,0,ENTITIES,0));
                          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't make payments%s to other characters until you have been connected for a total of "ANSI_LYELLOW"%s"ANSI_LGREEN" (You'll be able to make payments%s in "ANSI_LWHITE"%s"ANSI_LGREEN" time.)",(val1) ? " from your bank account":"",interval(restriction,0,ENTITIES,0),(val1) ? " from your bank account":"",scratch_return_string);
		       }
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't make a payment to yourself%s%s%s",(val1) ? " from your bank account (Please use the '"ANSI_LWHITE"withdraw"ANSI_LGREEN"' command in "ANSI_LYELLOW"The Bank of ":".",(val1) ? tcz_short_name:"",(val1) ? ANSI_LGREEN" to withdraw credit from your bank account.)":"");
		 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can only make a payment from your bank balance to another character from within a compound command which you own.");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't make payments%s from within an area compound command (I.e:  '"ANSI_LWHITE".enter"ANSI_LGREEN"', '"ANSI_LWHITE".leave"ANSI_LGREEN"', '"ANSI_LWHITE".login"ANSI_LGREEN"', etc.)",(val1) ? " from your bank account":"");
	   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't make payments%s from within a compound command executed by a fuse or alarm.",(val1) ? " from your bank account":"");
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you must be connected to make a payment%s to another character.",(val1) ? " from your bank account":"");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify who you would like to make a payment%s to.",(val1) ? " from your bank account":"");
}

/* ---->  Withdraw from bank account  <---- */
void finance_withdraw(CONTEXT)
{
     double amount;

     setreturn(ERROR,COMMAND_FAIL);
     if(!in_command || Wizard(current_command)) {
        if(Connected(player) || (in_command && Wizard(current_command))) {
           time_t total,now;

           gettime(now);
           total = db[player].data->player.totaltime;
           if(Connected(player)) total += (now - db[player].data->player.lasttime);
           if(Level4(player) || (total >= (WITHDRAWAL_RESTRICTION * MINUTE))) {
              if(!(Blank(params) || !(((amount = tofloat(params,NULL)) >= 0.01) || (amount < 0)))) {
                 if(amount >= 0) {
                    if((currency_to_double(&(db[player].data->player.balance)) - amount) >= 0) {
                       currency_add(&(db[player].data->player.credit),amount);
                       currency_add(&(db[player].data->player.balance),0 - amount);
                       output(getdsc(player),player,0,1,0,ANSI_LGREEN"You insert your bank card and withdraw "ANSI_LYELLOW"%.2f"ANSI_LGREEN" credits from your bank account.\n\nYour balance now stands at %s%.2f"ANSI_LGREEN" credits.\n",amount,(currency_to_double(&(db[player].data->player.balance)) < 0) ? ANSI_LRED:ANSI_LWHITE,currency_to_double(&(db[player].data->player.balance)));
                       if(!Invisible(db[player].location)) output_except(db[player].location,player,NOTHING,0,1,0,ANSI_LGREEN"%s"ANSI_LWHITE"%s"ANSI_LGREEN" inserts %s bank card and withdraws some credit from %s bank account.",Article(player,UPPER,DEFINITE),getcname(NOTHING,player,0,0),Possessive(player,0),Possessive(player,0));
                       if(!in_command || !Wizard(current_command))
                          writelog(TRANSACTION_LOG,0,"WITHDRAW","%s(#%d) withdrew %.2f credits (Balance:  %.2f credits.)",getname(player),player,amount,currency_to_double(&(db[player].data->player.balance)));
                       setreturn(OK,COMMAND_SUCC);
		    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you have insufficient credit in your bank account to withdraw "ANSI_LYELLOW"%.2f"ANSI_LGREEN" credits.",amount);
	         } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't withdraw a negative amount of credit from your bank account.");
	      } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify how much credit you would like to withdraw from your bank account.");
   	   } else {
              strcpy(scratch_return_string,interval((WITHDRAWAL_RESTRICTION * MINUTE) - total,0,ENTITIES,0));
              output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't make withdrawals from your bank balance until you have been connected for a total of "ANSI_LYELLOW"%s"ANSI_LGREEN" (You'll be able to make withdrawals in "ANSI_LWHITE"%s"ANSI_LGREEN" time.)",interval(WITHDRAWAL_RESTRICTION * MINUTE,0,ENTITIES,0),scratch_return_string);
	   }
	} else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you must be connected to make withdrawals from your bank account.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't make withdrawals from your bank account from within a compound command.");
}
