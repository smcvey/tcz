/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| COMBAT.C  -  Implements simple combat commands for construction of basic    |
|              combat areas.                                                  |
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
| Module originally designed and written by:  J.P.Boggis 01/06/1995.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: combat.c,v 1.2 2005/01/25 18:54:51 tcz_monster Exp $

*/

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>

#include "logfiles.h"
#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"

#include "object_types.h"
#include "friend_flags.h"
#include "flagset.h"
#include "fields.h"


/* ---->  Update character's health  <---- */
void update_health(dbref player)
{
     time_t now,count;
     double health;

     gettime(now);
     if(db[player].data->player.healthtime < now) {
        count = (now - db[player].data->player.healthtime) / HEALTH_REPLENISH_INTERVAL;
        db[player].data->player.healthtime += count * HEALTH_REPLENISH_INTERVAL;
        health = currency_to_double(&(db[player].data->player.health));
       
        if(health < 100) {
           health += count * HEALTH_REPLENISH_AMOUNT;
           if(health > 100) health = 100;
        } else if(health > 100) {
           health -= count * HEALTH_REPLENISH_AMOUNT * 5;
           if(health < 100) health = 100;
	}
        double_to_currency(&(db[player].data->player.health),health);
     } else db[player].data->player.healthtime = now;
}

/* ---->  Return percentage as string (Handles division by zero correctly)  <---- */
const char *combat_percent(double numb1,double numb2)
{
      static char buffer[32];

      if(!((numb1 == 0) || (numb2 == 0))) {
         sprintf(buffer,"%.1f%%",(100 * (numb1 / numb2)));
         return(buffer);
      } else return("0.0%");
}

/* ---->  Warn character if value given for parameter is too high or not specified  <---- */
double combat_warn(dbref player,const char *param,double limit,double applimit,double wizlimit,double elderlimit,const char *paramname)
{
       double value;

       /* ---->  Parameter blank (Not specified)?  <---- */
       if(Blank(param)) {
          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, "ANSI_LWHITE"%s"ANSI_LGREEN" not specified.",paramname);
          return(NOTHING);
       } else value = tofloat(param,NULL);

       /* ---->  Parameter negative?  <---- */
       if(value < 0) {
          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, "ANSI_LWHITE"%s"ANSI_LGREEN" can't be negative.",paramname);
          return(NOTHING);
       }

       if(Level2(current_command)) {

          /* ---->  Parameter too high for ELDER set combat command?  <---- */
          if((elderlimit != NOTHING) && (value > elderlimit)) {
             output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a maximum of "ANSI_LYELLOW"%.0f"ANSI_LGREEN" for "ANSI_LWHITE"%s"ANSI_LGREEN" can be specified from within an "ANSI_LYELLOW"ELDER"ANSI_LGREEN" set combat command.",elderlimit,paramname);
             return(NOTHING);
	  }
       } else if(Level3(current_command)) {

          /* ---->  Parameter too high for WIZARD set combat command?  <---- */
          if((wizlimit != NOTHING) && (value > wizlimit)) {
             output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a maximum of "ANSI_LYELLOW"%.0f"ANSI_LGREEN" for "ANSI_LWHITE"%s"ANSI_LGREEN" can be specified from within a "ANSI_LYELLOW"WIZARD"ANSI_LGREEN" set combat command.",wizlimit,paramname);
             return(NOTHING);
	  }
       } else if(Level4(current_command)) {

          /* ---->  Parameter too high for APPRENTICE set combat command?  <---- */
          if((applimit != NOTHING) && (value > applimit)) {
             output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a maximum of "ANSI_LYELLOW"%.0f"ANSI_LGREEN" for "ANSI_LWHITE"%s"ANSI_LGREEN" can be specified from within an "ANSI_LYELLOW"APPRENTICE"ANSI_LGREEN" set combat command.",applimit,paramname);
             return(NOTHING);
	  }
       } else if((limit != NOTHING) && (value > limit)) {

          /* ---->  Parameter too high for ordinary combat command?  <---- */
          output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, a maximum of "ANSI_LYELLOW"%.0f"ANSI_LGREEN" for "ANSI_LWHITE"%s"ANSI_LGREEN" can be specified from within a combat command.",limit,paramname);
          return(NOTHING);
       }
       return(value);
}

/* ---->  Return random floating point number between 0 and RANGE  <---- */
double floatrand(int range)
{
       double value = (lrand48() % (range + 1));

       if(value < range) value += (double) (lrand48() % 100) / 100;
       return(value);
}

/* ---->  Damage a character  <---- */
void combat_damage(CONTEXT)
{
     double rnd,accuracy,mindamage,maxdamage;
     int    delay,score;
     dbref  victim;
     char   *ptr;
     time_t now;

     setreturn(ERROR,COMMAND_FAIL);
     gettime(now);
     if(in_command && Valid(current_command) && Combat(current_command)) {
        if(Combat(Location(player))) {
           if((Owner(current_command) == Owner(Location(player))) || friendflags_set(Owner(Location(player)),Owner(current_command),NOTHING,FRIEND_COMBAT)) {
              if(!Haven(Location(player))) {
                 if(!Blank(arg1)) {
                    if((victim = lookup_character(player,arg1,1)) != NOTHING) {
                       if(Connected(victim)) {
                          if((Location(player) == Location(victim)) || Level2(current_command)) {
                             if(!((player == victim) && (player == Owner(current_command)) && !Level3(current_command))) {
                                if(!((player == victim) && (player == Owner(Location(player))) && !Level3(current_command)) && !Level3(Location(player))) {
                                   if(now >= db[player].data->player.damagetime) {

                                      /* ---->  Accuracy  <---- */
                                      for(; *arg2 && (*arg2 == ' '); arg2++);
                                      for(ptr = scratch_buffer; *arg2 && (*arg2 != ' '); *ptr++ = *arg2, arg2++);
                                      *ptr = '\0';
                                      if((accuracy = combat_warn(player,scratch_buffer,60,80,100,100,"<ACCURACY>")) < 0) return;
        
                                      /* ---->  Minimum damage  <---- */
                                      for(; *arg2 && (*arg2 == ' '); arg2++);
                                      for(ptr = scratch_buffer; *arg2 && (*arg2 != ' '); *ptr++ = *arg2, arg2++);
                                      *ptr = '\0';
                                      if((mindamage = combat_warn(player,scratch_buffer,50,75,100,1000,"<MIN DAMAGE>")) < 0) return;

                                      /* ---->  Maximum damage  <---- */
                                      for(; *arg2 && (*arg2 == ' '); arg2++);
                                      for(ptr = scratch_buffer; *arg2 && (*arg2 != ' '); *ptr++ = *arg2, arg2++);
                                      *ptr = '\0';
                                      if((maxdamage = combat_warn(player,scratch_buffer,50,75,100,1000,"<MAX DAMAGE>")) < 0) return;

                                      /* ---->  Combat delay  <---- */
                                      for(; *arg2 && (*arg2 == ' '); arg2++);
                                      for(ptr = scratch_buffer; *arg2 && (*arg2 != ' '); *ptr++ = *arg2, arg2++);
                                      *ptr = '\0';
                                      if((delay = combat_warn(player,scratch_buffer,30,45,60,(5 * MINUTE),"<DELAY>")) < 0) return;

                                      /* ---->  Score  <---- */
                                      for(; *arg2 && (*arg2 == ' '); arg2++);
                                      for(ptr = scratch_buffer; *arg2 && (*arg2 != ' '); *ptr++ = *arg2, arg2++);
                                      *ptr = '\0';
                                      if((score = combat_warn(player,scratch_buffer,100,500,1000,10000,"<SCORE>")) < 0) return;

                                      /* ---->  Combat delay:  Increase character's combat delay as specified  <---- */
                                      db[player].data->player.damagetime = now + delay;
                  
                                      /* ---->  Accuracy:  See if attacker 'hits' or 'misses'  <---- */
                                      if(floatrand(100) < accuracy) {

                                         /* ---->  Attacker hits:  Determine amount of damage done  <---- */
                                         if(mindamage > maxdamage) {
                                            rnd       = mindamage;
                                            mindamage = maxdamage;
                                            maxdamage = rnd;
                                         }
                                         rnd = mindamage + floatrand(maxdamage - mindamage);
                                         update_health(victim);
                                         currency_add(&(db[victim].data->player.health),0 - rnd);
                                         if(currency_to_double(&(db[victim].data->player.health)) <= 0) {

                                            /* ---->  Victim killed  <---- */
                                            double_to_currency(&(db[victim].data->player.health),100);
                                            if(Level4(current_command)) {
                                               db[victim].data->player.score -= (score / 3) * 2;
                                               db[player].data->player.score += score;
                                            }
                                            db[victim].data->player.lost++;
                                            db[player].data->player.won++;
                                            setreturn("KILL",COMMAND_SUCC);

                                            /* ---->  Victim damaged  <---- */
                                         } else setreturn("HIT",COMMAND_SUCC);

                                            /* ---->  Attacker misses  <---- */
                                      } else setreturn("MISS",COMMAND_SUCC);
                                   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you can't fight again for another "ANSI_LWHITE"%s"ANSI_LGREEN".",interval(db[player].data->player.damagetime - now,db[player].data->player.damagetime - now,ENTITIES,0));
                                } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you own your current location ("ANSI_LWHITE"%s"ANSI_LGREEN".)  You cannot use the '"ANSI_LYELLOW"@damage"ANSI_LGREEN"' command on yourself in your own combat areas unless they have their "ANSI_LYELLOW"WIZARD"ANSI_LGREEN" or "ANSI_LYELLOW"ELDER"ANSI_LGREEN" flag set.",unparse_object(player,current_command,0));
                             } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you own the compound command "ANSI_LWHITE"%s"ANSI_LGREEN".  You cannot use the '"ANSI_LYELLOW"@damage"ANSI_LGREEN"' command on yourself from within your own combat commands unless they have their "ANSI_LYELLOW"WIZARD"ANSI_LGREEN" or "ANSI_LYELLOW"ELDER"ANSI_LGREEN" flag set.",unparse_object(player,current_command,0));
                          } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is not in the same location as you.",Article(victim,LOWER,DEFINITE),getcname(NOTHING,victim,0,0));
                       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" isn't currently connected.",Article(victim,LOWER,DEFINITE),getcname(NOTHING,victim,0,0));
                    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
                 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify who you would like to damage.");
              } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@damage"ANSI_LGREEN"' cannot be used in a "ANSI_LYELLOW"HAVEN"ANSI_LGREEN" location.");
           } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@damage"ANSI_LGREEN"' can only be used from within a compound command and location which has the "ANSI_LYELLOW"COMBAT"ANSI_LGREEN" flag set, where the owner of the location and compound command are the same.");
        } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@damage"ANSI_LGREEN"' can only be used within a location which has its "ANSI_LYELLOW"COMBAT"ANSI_LGREEN" flag set.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@damage"ANSI_LGREEN"' can only be used from within a compound command with its "ANSI_LYELLOW"COMBAT"ANSI_LGREEN" flag set.");
}

/* ---->  Increase character's combat delay  <---- */
void combat_delay(CONTEXT)
{
     dbref  victim;
     int    amount;
     time_t now;

     setreturn(ERROR,COMMAND_FAIL);
     if(in_command && Valid(current_command) && Combat(current_command)) {
        if(Combat(Location(player))) {
           if((Owner(current_command) == Owner(Location(player))) || friendflags_set(Owner(Location(player)),Owner(current_command),NOTHING,FRIEND_COMBAT)) {
              if(!Haven(Location(player))) {
                 if(!Blank(arg1)) {
                    if((victim = lookup_character(player,arg1,1)) != NOTHING) {
                       if(Connected(victim)) {
                          if((Location(player) == Location(victim)) || Level2(current_command)) {
                             if(!((player == victim) && (player == Owner(current_command)) && !Level3(current_command))) {
                                if(!((player == victim) && (player == Owner(Location(player))) && !Level3(current_command)) && !Level3(Location(player))) {
                                   if(!Blank(arg2)) {
                                      if(*arg2 == '+') arg2++;
                                      gettime(now);
                                      amount = atoi(arg2);
                                      if(amount > MINUTE) amount = MINUTE;
                                      if(now < db[victim].data->player.damagetime)
                                         amount += (db[victim].data->player.damagetime - now);
                                      db[victim].data->player.damagetime = now + amount;
                                      if(db[victim].data->player.damagetime < now)
                                         db[victim].data->player.damagetime = now;
                                      setreturn(OK,COMMAND_SUCC);
                                   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the "ANSI_LYELLOW"<AMOUNT>"ANSI_LGREEN" to increase/decrease %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s combat delay by.",Article(victim,LOWER,DEFINITE),getcname(NOTHING,victim,0,0));
                                } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you own your current location ("ANSI_LWHITE"%s"ANSI_LGREEN".)  You cannot use the '"ANSI_LYELLOW"@delay"ANSI_LGREEN"' command on yourself in your own combat areas unless they have their "ANSI_LYELLOW"WIZARD"ANSI_LGREEN" or "ANSI_LYELLOW"ELDER"ANSI_LGREEN" flag set.",unparse_object(player,current_command,0));
                             } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you own the compound command "ANSI_LWHITE"%s"ANSI_LGREEN".  You cannot use the '"ANSI_LYELLOW"@delay"ANSI_LGREEN"' command on yourself from within your own combat commands unless they have their "ANSI_LYELLOW"WIZARD"ANSI_LGREEN" or "ANSI_LYELLOW"ELDER"ANSI_LGREEN" flag set.",unparse_object(player,current_command,0));
                          } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is not in the same location as you.",Article(victim,LOWER,DEFINITE),getcname(NOTHING,victim,0,0));
                       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" isn't currently connected.",Article(victim,LOWER,DEFINITE),getcname(NOTHING,victim,0,0));
                    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
                 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify who's combat delay you would like to increase/decrease.");
              } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@delay"ANSI_LGREEN"' cannot be used in a "ANSI_LYELLOW"HAVEN"ANSI_LGREEN" location.");
           } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@delay"ANSI_LGREEN"' can only be used from within a compound command and location which has the "ANSI_LYELLOW"COMBAT"ANSI_LGREEN" flag set, where the owner of the location and compound command are the same.");
        } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@delay"ANSI_LGREEN"' can only be used within a location which has its "ANSI_LYELLOW"COMBAT"ANSI_LGREEN" flag set.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@delay"ANSI_LGREEN"' can only be used from within a compound command with its "ANSI_LYELLOW"COMBAT"ANSI_LGREEN" flag set.");
}

/* ---->  Heal a character (Increase their health)  <---- */
void combat_heal(CONTEXT)
{
     double   old,health,amount;
     unsigned char exceed = 0;
     dbref    victim;

     setreturn(ERROR,COMMAND_FAIL);
     if(in_command && Valid(current_command) && Combat(current_command)) {
        if(Combat(Location(player))) {
           if((Owner(current_command) == Owner(Location(player))) || friendflags_set(Owner(Location(player)),Owner(current_command),NOTHING,FRIEND_COMBAT)) {
              if(!Haven(Location(player))) {
                 if(!Blank(arg1)) {
                    if((victim = lookup_character(player,arg1,1)) != NOTHING) {
                       if(Connected(victim)) {
                          if((Location(player) == Location(victim)) || Level2(current_command)) {
                             if(!((player == victim) && (player == Owner(current_command)) && !Level3(current_command))) {
                                if(!((player == victim) && (player == Owner(Location(player))) && !Level3(current_command)) && !Level3(Location(player))) {
                                   if(!Blank(arg2)) {
                                      if(*arg2 == '+') {
                                         if(!Level3(current_command)) {
                                            output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"+"ANSI_LGREEN"' can only be used in a "ANSI_LYELLOW"WIZARD"ANSI_LGREEN" or "ANSI_LYELLOW"ELDER"ANSI_LGREEN" set compound command to increase health above "ANSI_LWHITE"100%%"ANSI_LGREEN".");
                                            return;
                                         } else exceed = 1, arg2++;
				      }
                                      amount = tofloat(arg2,NULL);
                       
                                      update_health(victim);
                                      old = health = currency_to_double(&(db[victim].data->player.health));
                                      if(!exceed && (amount > 0) && (old >= 100)) amount = 0;
                                      health += amount;
                                      if(health < 0) health = 0;
                                      if(!exceed && (amount >= 0) && (old <= 100) && (health > 100)) health = 100;
                                      if(health >= 1000) health = 999.99;
                                      writelog(COMBAT_LOG,1,"HEAL","%s(#%d)'s health %screased from %.2f%% to %.2f%% (%+.2f%%) from within compound command %s(#%d).",getname(victim),victim,(amount < 0) ? "de":"in",old,health,amount,getname(current_command),current_command);
                                      double_to_currency(&(db[victim].data->player.health),health);
                                      setreturn(OK,COMMAND_SUCC);
                                   } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify the "ANSI_LYELLOW"<AMOUNT>"ANSI_LGREEN" to %screase %s"ANSI_LWHITE"%s"ANSI_LGREEN"'s health by.",(amount < 0) ? "de":"in",Article(victim,LOWER,DEFINITE),getcname(NOTHING,victim,0,0));
                                } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you own your current location ("ANSI_LWHITE"%s"ANSI_LGREEN".)  You cannot use the '"ANSI_LYELLOW"@heal"ANSI_LGREEN"' command on yourself in your own combat areas unless they have their "ANSI_LYELLOW"WIZARD"ANSI_LGREEN" or "ANSI_LYELLOW"ELDER"ANSI_LGREEN" flag set.",unparse_object(player,current_command,0));
                             } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, you own the compound command "ANSI_LWHITE"%s"ANSI_LGREEN".  You cannot use the '"ANSI_LYELLOW"@heal"ANSI_LGREEN"' command on yourself from within your own combat commands unless they have their "ANSI_LYELLOW"WIZARD"ANSI_LGREEN" or "ANSI_LYELLOW"ELDER"ANSI_LGREEN" flag set.",unparse_object(player,current_command,0));
                          } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" is not in the same location as you.",Article(victim,LOWER,DEFINITE),getcname(NOTHING,victim,0,0));
                       } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, %s"ANSI_LWHITE"%s"ANSI_LGREEN" isn't currently connected.",Article(victim,LOWER,DEFINITE),getcname(NOTHING,victim,0,0));
                    } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",arg1);
                 } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Please specify who you would like to heal.");
              } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@heal"ANSI_LGREEN"' cannot be used in a "ANSI_LYELLOW"HAVEN"ANSI_LGREEN" location.");
           } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@heal"ANSI_LGREEN"' can only be used from within a compound command and location which has the "ANSI_LYELLOW"COMBAT"ANSI_LGREEN" flag set, where the owner of the location and compound command are the same.");
        } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@heal"ANSI_LGREEN"' can only be used within a location which has its "ANSI_LYELLOW"COMBAT"ANSI_LGREEN" flag set.");
     } else output(getdsc(player),player,0,1,0,ANSI_LGREEN"Sorry, '"ANSI_LWHITE"@heal"ANSI_LGREEN"' can only be used from within a compound command with its "ANSI_LYELLOW"COMBAT"ANSI_LGREEN" flag set.");
}

/* ---->  Display a character's combat statistics  <---- */
void combat_statistics(CONTEXT)
{
     struct descriptor_data *p = getdsc(player);
     int    twidth = output_terminal_width(player);
     dbref  victim;

     setreturn(ERROR,COMMAND_FAIL);
     if(!Blank(params)) {
        if((victim = lookup_character(player,params,1)) == NOTHING) {
           output(p,player,0,1,0,ANSI_LGREEN"Sorry, the character '"ANSI_LWHITE"%s"ANSI_LGREEN"' doesn't exist.",params);
           return;
	}
     } else victim = player;

     html_anti_reverse(p,1);
     if(IsHtml(p)) output(p,player,1,2,0,"%s<TABLE BORDER WIDTH=100%% CELLPADDING=4 BGCOLOR="HTML_TABLE_BLACK">",(in_command) ? "":"<BR>");
     if(!in_command) {
        if(victim != player) output(p,player,2,1,0,"%s%s"ANSI_LWHITE"%s"ANSI_LCYAN"'s combat statistics...%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER COLSPAN=4><FONT SIZE=4><I>\016"ANSI_LCYAN:"\n ",Article(victim,UPPER,DEFINITE),getcname(NOTHING,victim,0,0),IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
           else output(p,player,2,1,0,"%sYour combat statistics...%s",IsHtml(p) ? "\016<TR BGCOLOR="HTML_TABLE_CYAN"><TH ALIGN=CENTER COLSPAN=4><FONT SIZE=4><I>\016"ANSI_LCYAN:"\n ",IsHtml(p) ? "\016</I></FONT></TH></TR>\016":"\n");
        if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','='));
     }

     if(Combat(Location(victim))) {
        if(!Secret(victim) && !Secret(Location(victim))) {
           dbref area = get_areaname_loc(Location(victim));

           if(Valid(area) && !Blank(getfield(area,AREANAME))) {
              strcpy(scratch_return_string,getfield(area,AREANAME));
              bad_language_filter(scratch_return_string,scratch_return_string);
           } else strcpy(scratch_return_string,getname(Location(victim)));
           output(p,player,2,1,18,"%sCombat area:%s"ANSI_LCYAN"%s"ANSI_LWHITE" owned by %s"ANSI_LYELLOW"%s"ANSI_LWHITE".%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_MAGENTA"><I>\016"ANSI_LMAGENTA:ANSI_LMAGENTA"    ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT WIDTH=25% COLSPAN=3>\016":"  ",scratch_return_string,Article(victim,LOWER,DEFINITE),getcname(NOTHING,victim,0,0),Haven(Location(victim)) ? ANSI_LRED"  (HAVEN)":"",IsHtml(p) ? "\016</TD></TR>\016":"\n");
        } else output(p,player,2,1,18,"%sCombat area:%s"ANSI_LWHITE"Secret location.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_MAGENTA"><I>\016"ANSI_LMAGENTA:ANSI_LMAGENTA"    ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT WIDTH=25% COLSPAN=3>\016":"  ",IsHtml(p) ? "\016</TD></TR>\016":"\n");
     } else output(p,player,2,1,18,"%sCombat area:%s"ANSI_LWHITE"No.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_MAGENTA"><I>\016"ANSI_LMAGENTA:ANSI_LMAGENTA"    ",IsHtml(p) ? "\016</I></TH><TD ALIGN=LEFT WIDTH=25% COLSPAN=3>\016":"  ",IsHtml(p) ? "\016</TD></TR>\016":"\n");
     if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','-'));

     update_health(victim);
     output(p,player,2,1,0,"%sCurrent health:%s"ANSI_LWHITE"%-25s%s"ANSI_LGREEN"Current score:%s"ANSI_LWHITE"%d point%s.%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_GREEN"><I>\016"ANSI_LGREEN:ANSI_LGREEN" ",IsHtml(p) ? "\016</I></TH><TD WIDTH=25%>\016":"  ",combat_percent(currency_to_double(&(db[victim].data->player.health)),100),IsHtml(p) ? "\016</TD><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_GREEN"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD WIDTH=25%>\016":"  ",db[victim].data->player.score,Plural(db[victim].data->player.score),IsHtml(p) ? "\016</TD></TR>\016":"\n");
     if(!IsHtml(p)) output(p,player,0,1,0,separator(twidth,0,'-','-'));

     output(p,player,2,1,0,"%sBattles won:%s"ANSI_LWHITE"%-18d%s"ANSI_LYELLOW"Total battles fought:%s"ANSI_LWHITE"%-10d%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"    ",IsHtml(p) ? "\016</I></TH><TD WIDTH=25%>\016":"  ",db[victim].data->player.won,IsHtml(p) ? "\016</TD><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD WIDTH=25%>\016":"  ",(db[victim].data->player.won + db[victim].data->player.lost),IsHtml(p) ? "\016</TD></TR>\016":"\n");
     output(p,player,2,1,0,"%sBattles lost:%s"ANSI_LWHITE"%-20d%s"ANSI_LYELLOW"Combat performance:%s"ANSI_LWHITE"%s%s",IsHtml(p) ? "\016<TR><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016"ANSI_LYELLOW:ANSI_LYELLOW"   ",IsHtml(p) ? "\016</I></TH><TD WIDTH=25%>\016":"  ",db[victim].data->player.lost,IsHtml(p) ? "\016</TD><TH ALIGN=RIGHT WIDTH=25% BGCOLOR="HTML_TABLE_YELLOW"><I>\016":"",IsHtml(p) ? "\016</I></TH><TD WIDTH=25%>\016":"  ",combat_percent(db[victim].data->player.won,(db[victim].data->player.won + db[victim].data->player.lost)),IsHtml(p) ? "\016</TD></TR>\016":"\n");
     if(!IsHtml(p) && !in_command) output(p,player,0,1,0,separator(twidth,1,'-','='));
     if(IsHtml(p)) output(p,player,1,2,0,"</TABLE>%s",(!in_command) ? "<BR>":"");
     html_anti_reverse(p,0);
     setreturn(OK,COMMAND_SUCC);
}

/* ---->  Return character's combat delay (In seconds)  <---- */
void combat_query_delay(CONTEXT)
{
     dbref victim;
     time_t now;

     victim = query_find_character(player,params,1);
     if(!Validchar(victim)) return;
     gettime(now);
     now = db[victim].data->player.damagetime - now;
     if(now < 0) now = 0;

     sprintf(querybuf,"%d",(int) now);
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Return character's health  <---- */
void combat_query_health(CONTEXT)
{
     dbref victim;
     char  *ptr;

     victim = query_find_character(player,params,0);
     if(!Validchar(victim)) return;
     update_health(victim);
     sprintf(querybuf,"%s",combat_percent(currency_to_double(&(db[victim].data->player.health)),100));
     if((ptr = (char *) strchr(querybuf,'%')) && *ptr) *ptr = '\0';
     setreturn(querybuf,COMMAND_SUCC);
}

/* ---->  Return combat statistics of character              <---- */
/*        (Val1:  0 = Won, 1 = Lost, 2 = Total, 3 = Performance.)  */
void combat_query_statistics(CONTEXT)
{
     dbref victim;

     victim = query_find_character(player,params,0);
     if(!Validchar(victim)) return;
     switch(val1) {
            case 0:
                 sprintf(querybuf,"%d",db[victim].data->player.won);
                 break;
            case 1:
                 sprintf(querybuf,"%d",db[victim].data->player.lost);
                 break;
            case 2:
                 sprintf(querybuf,"%d",db[victim].data->player.won + db[victim].data->player.lost);
                 break;
            case 3:
                 strcpy(querybuf,combat_percent(db[victim].data->player.won,(db[victim].data->player.won + db[victim].data->player.lost)));
                 break;
            default:
                 strcpy(querybuf,INVALID_STRING);
                 break;
     }
     setreturn(querybuf,COMMAND_SUCC);
}


