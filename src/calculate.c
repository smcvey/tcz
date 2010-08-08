/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| CALCULATE.C  -  Implements '@eval'/'@calc', which allows integer, floating  |
|                 point and string-based calculations to be performed.        |
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
| Module originally designed and written by:  J.P.Boggis 18/10/1994.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: calculate.c,v 1.1.1.1 2004/12/02 17:40:45 jpboggis Exp $

*/


#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "command.h"
#include "externs.h"
#include "config.h"
#include "db.h"


static  char                calcbuf[BUFFER_LEN];

#define CALC_SEPARATOR(x)   ((x == ' ') || (x == '+') || (x == '-') || (x == '*') || (x == '/') || (x == '%') || (x == '&') || (x == '|') || (x == '!') || (x == '~') || (x == '<') || (x == '>') || (x == '=') || (x == '^') || (x == '#'))
#define CALC_COMPCHAR(x)    ((x == '=') || (x == '<') || (x == '>'))


/* ---->  Comparison  <---- */
#define CALC_EQUAL          1
#define CALC_NOTEQUAL       2
#define CALC_LT             3
#define CALC_LTE            4
#define CALC_GT             5
#define CALC_GTE            6


/* ---->  Arithmetic operation  <---- */
#define CALC_INIT           18  /*  Initial CALC_OP:  add  */
#define CALC_DEFAULT        19  /*  Default CALC_OP:  add  */
#define CALC_ADD            20
#define CALC_SUB            21
#define CALC_DIV            22
#define CALC_MUL            23
#define CALC_MOD            24
#define CALC_PWR            25
#define CALC_SQRT           26
#define CALC_COS            27
#define CALC_SIN            28
#define CALC_TAN            29
#define CALC_ACOS           30
#define CALC_ASIN           31
#define CALC_ATAN           32
#define CALC_ABS            33
#define CALC_CONVINT        38
#define CALC_CONVFLOAT      39


/* ---->  Logical operation  <---- */
#define CALC_AND            40
#define CALC_OR             41
#define CALC_NOT            42
#define CALC_XOR            43


/* ---->  Bitwise operation  <---- */
#define CALC_BITAND         60
#define CALC_BITOR          61
#define CALC_BITNOT         62
#define CALC_BITSHL         63
#define CALC_BITSHR         64  


/* ---->  Calculation type (Interger, floating point or string-based?)  <---- */
#define CALC_INT            100
#define CALC_FLOAT          101
#define CALC_STRING         102
#define CALC_ERROR          255  /*  Error in calculation  */

#define CALC_PI             "3.1415926536"

#define CALC_COMPARISON(x)  ((x == CALC_EQUAL) || (x == CALC_NOTEQUAL) || (x == CALC_LT) || (x == CALC_GT) || (x == CALC_LTE) || (x == CALC_GTE))
#define CALC_SCIENTIFIC(x)  ((x == CALC_SQRT) || (x == CALC_SIN) || (x == CALC_COS) || (x == CALC_TAN) || (x == CALC_ASIN) || (x == CALC_ACOS) || (x == CALC_ATAN))


/* ---->  Substitute result of calculation  <---- */
void calculate_substitute(dbref player,struct str_ops *str_data,int brackets)
{
     char closingchar;
     char *expression;

     /* ---->  Seek closing ')'  <---- */
     expression = ++str_data->src;
     while(*(str_data->src) && brackets) {
           switch(*(str_data->src)) {
                  case '"':
	   
                       /* ---->  Skip over a string (Something in "'s)  <---- */
                       str_data->src++;
                       while(*(str_data->src) && (*(str_data->src) != '\"')) str_data->src++;
                       while(*(str_data->src) && !(CALC_SEPARATOR(*(str_data->src)) || (*(str_data->src) == '(') || (*(str_data->src) == ')'))) str_data->src++;
                       break;
		
                  case '(':
                       str_data->src++;
                       brackets++;
                       break;
			 
                  case ')':
                       brackets--;
                       if(brackets) str_data->src++;
                       break;

                  default:
                       str_data->src++;
	   }
     }
     closingchar      = *(str_data->src);
     *(str_data->src) = '\0';

     /* ---->  Calculate expression within the ()'s and substitute result  <---- */
     calculate_evaluate(player,expression,NULL,NULL,NULL,str_data->backslash,0);
     strcat_limits(str_data,command_result);

     *(str_data->src) = closingchar;
     if(*(str_data->src)) {
        str_data->src++;
        if(*(str_data->src) && (isdigit(*(str_data->src)) || (*(str_data->src) == '('))) strcat_limits_char(str_data,'*');
     }
}

/* ---->  Evaluate ()'s in given string and return resulting string  <---- */
void calculate_bracket_substitute(dbref player,char *src,char *dest,int level)
{
     struct str_ops str_data;
     char   *start = src;

     str_data.dest      = dest;
     str_data.src       = src;
     str_data.backslash = level;
     str_data.length    = 0;

     while(*str_data.src && (str_data.length < MAX_LENGTH)) {
           switch(*str_data.src) {
		  case '"':
		  
		       /* ---->  Skip over a string (Something in "'s)  <---- */
		       *str_data.dest++ = *str_data.src;
		       str_data.length++;
		       str_data.src++;
		       if(str_data.length < MAX_LENGTH) {
			  while(*str_data.src && (*str_data.src != '\"') && (str_data.length < MAX_LENGTH)) {
				*str_data.dest++ = *str_data.src;
				str_data.length++;			 
				str_data.src++;
			  }

			  while(*str_data.src && !(CALC_SEPARATOR(*str_data.src) || (*str_data.src == '(') || (*str_data.src == ')')) && (str_data.length < MAX_LENGTH)) {
				*str_data.dest++ = *str_data.src;
				str_data.length++;
				str_data.src++;
			  }
		       }
		       break;
		  case '(':

		       /* ---->  Substitute return value of command  <---- */
		       if(((str_data.src - 1) >= start) && isdigit(*(str_data.src - 1))) strcat_limits_char(&str_data,'*');
		       calculate_substitute(player,&str_data,1);
		       break;
		  default:
		       *str_data.dest++ = *str_data.src;
		       str_data.length++;
		       str_data.src++;
	   }
     }
     *str_data.dest = '\0';
}

/* ---->  Perform numeric operation on current RESULT  <---- */
struct calc_ops calculate_numeric_op(dbref player,struct calc_ops result,struct calc_ops value,int calc_op)
{
       double temp;

       if(!result.calctype) result.calctype = value.calctype;
          else if((result.calctype == CALC_INT) && (value.calctype == CALC_FLOAT)) {
             result.calctype  = CALC_FLOAT;
             result.calcfloat = result.calcint;
	  }

       if(!((result.calctype == CALC_INT) || (result.calctype == CALC_FLOAT))) {
          if(!in_command) output(getdsc(player),player,0,1,12,ANSI_LGREEN"[EVALUATE] \016&nbsp;\016 "ANSI_LWHITE"Error  -  Can't do numeric operation on a string.");
          result.calctype = CALC_ERROR;
          return(result);
       }

       if((result.calctype == CALC_FLOAT) && ((calc_op == CALC_BITSHL) || (calc_op == CALC_BITSHR))) {
          if(!in_command) output(getdsc(player),player,0,1,12,ANSI_LGREEN"[EVALUATE] \016&nbsp;\016 "ANSI_LWHITE"Error  -  A floating point number can't be shifted left/right.");
          result.calctype = CALC_ERROR;
          return(result);
       }

       switch(calc_op) {
              case CALC_ADD:
              case CALC_DEFAULT:
              case CALC_INIT:
                   if(result.calctype == CALC_INT) result.calcint += value.calcint;
                      else result.calcfloat += value.calcfloat;
                   break;
              case CALC_SUB:
                   if(result.calctype == CALC_INT) result.calcint -= value.calcint;
                      else result.calcfloat -= value.calcfloat;
                   break;
              case CALC_DIV:
                   if(((result.calctype == CALC_INT)   && (value.calcint   == 0)) ||
                      ((result.calctype == CALC_FLOAT) && (value.calcfloat == 0))) {
                        if(!in_command) output(getdsc(player),player,0,1,12,ANSI_LGREEN"[EVALUATE] \016&nbsp;\016 "ANSI_LWHITE"Error  -  Division by zero.");
                        result.calctype = CALC_ERROR;
                        return(result);
		   }
                   if(result.calctype == CALC_INT) result.calcint /= value.calcint;             
                      else result.calcfloat /= value.calcfloat;
                   break;
              case CALC_MUL:
                   if(result.calctype == CALC_INT) result.calcint *= value.calcint;
                      else result.calcfloat *= value.calcfloat;
                   break;
              case CALC_MOD:
                   if(result.calctype == CALC_INT) {
                      if(!value.calcint) {
                         if(!in_command) output(getdsc(player),player,0,1,12,ANSI_LGREEN"[EVALUATE] \016&nbsp;\016 "ANSI_LWHITE"Error  -  Modulus of zero.");
                         result.calctype = CALC_ERROR;
                         return(result);
		      } else result.calcint %= value.calcint;
		   } else {
                      if(!in_command) output(getdsc(player),player,0,1,12,ANSI_LGREEN"[EVALUATE] \016&nbsp;\016 "ANSI_LWHITE"Error  -  The modulus of a floating point number isn't known.");
                      result.calctype = CALC_ERROR;
                      return(result);
		   }
                   break;
              case CALC_PWR:
                   if(result.calctype == CALC_INT) result.calcfloat = result.calcint;
                   if((result.calcfloat < 0) && ((value.calctype == CALC_FLOAT) && (modf(value.calcfloat,&temp) != 0))) {
                      if(!in_command) output(getdsc(player),player,0,1,12,ANSI_LGREEN"[EVALUATE] \016&nbsp;\016 "ANSI_LWHITE"Error  -  Raising a negative value to a non-integral power would result in a complex number.");
                      result.calctype  = CALC_ERROR;
		   } else if(!((result.calcfloat == 0) && (value.calcfloat < 0))) {
                      result.calcfloat = pow(result.calcfloat,value.calcfloat);
                      result.calcint   = result.calcfloat;
		   } else result.calcint = result.calcfloat = 0;
                   break;
              case CALC_AND:
                   if(result.calctype == CALC_INT) result.calcint = (result.calcint && value.calcint);
                      else result.calcfloat = (result.calcfloat && value.calcfloat);
                   break;
              case CALC_OR:
                   if(result.calctype == CALC_INT) result.calcint = (result.calcint || value.calcint);
                      else result.calcfloat = (result.calcfloat || value.calcfloat);
                   break;
              case CALC_XOR:
                   if(result.calctype == CALC_INT) result.calcint = ((result.calcint || value.calcint) && !(result.calcint && value.calcint));
                      else result.calcfloat = ((result.calcfloat || value.calcfloat) && !(result.calcfloat && value.calcfloat));
                   break;
              case CALC_BITAND:
                   if(result.calctype == CALC_INT) result.calcint = (result.calcint & value.calcint);
                      else result.calcfloat = (result.calcfloat && value.calcfloat);
                   break;
              case CALC_BITOR:
                   if(result.calctype == CALC_INT) result.calcint = (result.calcint | value.calcint);
                      else result.calcfloat = (result.calcfloat || value.calcfloat);
                   break;
              case CALC_BITSHL:
                   result.calcint = (result.calcint << value.calcint);
                   break;
              case CALC_BITSHR:
                   result.calcint = (result.calcint >> value.calcint);
                   break;
              default:
                   if(!in_command) output(getdsc(player),player,0,1,12,ANSI_LGREEN"[EVALUATE] \016&nbsp;\016 "ANSI_LWHITE"Error  -  Missing/invalid operator(s) in expression.");
                   result.calctype = CALC_ERROR;
                   return(result);
       }

       /* ---->  Arithmetic exception raised by numeric operation?  <---- */
       if(command_type & ARITHMETIC_EXCEPTION) {
          if(!in_command) output(getdsc(player),player,0,1,12,ANSI_LGREEN"[EVALUATE] \016&nbsp;\016 "ANSI_LWHITE"Error  -  Arithmetic exception.");
          result.calctype = CALC_ERROR;
       }
       return(result);
}

/* ---->  Perform string operation on current RESULT  <---- */
struct calc_ops calculate_string_op(dbref player,struct calc_ops result,char *str,int calc_op)
{
       if(!result.calctype) result.calctype = CALC_STRING;

       if(result.calctype != CALC_STRING) {
          if(!in_command) output(getdsc(player),player,0,1,12,ANSI_LGREEN"[EVALUATE] \016&nbsp;\016 "ANSI_LWHITE"Error  -  Can't do numeric operation on a string.");
          result.calctype = CALC_ERROR;
          return(result);
       }

       if((calc_op != CALC_DEFAULT) && (calc_op != CALC_INIT) && (calc_op != CALC_ADD)) {
          if(!in_command) output(getdsc(player),player,0,1,12,ANSI_LGREEN"[EVALUATE] \016&nbsp;\016 "ANSI_LWHITE"Error  -  Missing/invalid operator(s) in expression (Strings can only be concatenated ('"ANSI_LYELLOW"##"ANSI_LWHITE"' or '"ANSI_LYELLOW"+"ANSI_LWHITE"'.))");
          result.calctype = CALC_ERROR;
          return(result);
       }

       if(*str) {
          strcpy(cmpbuf,result.calcstring);
          strcat(cmpbuf,str);
          FREENULL(result.calcstring);
          result.calcstring = (char *) malloc_string(cmpbuf);
       }
       return(result);
}

/* ---->  Perform calculation  <---- */
void calculate_evaluate(CONTEXT)
{
     int    negate  = 0,fallthru = 0,numeric,floating,colon;
     int    calc_op = CALC_INIT,comp_op;
     static char prototype[16];
     char   dp = NOTHING,dps;
     struct calc_ops compare;
     struct calc_ops result;
     char   exp[TEXT_SIZE];
     struct calc_ops temp;
     int    bool_op = 0;
     char   *p1,*p2;

     /* ---->  Check no. of brackets hasn't exceeded max. limit  <---- */
     if(val1 > MAX_BRACKETS) {
        output(getdsc(player),player,0,1,11,ANSI_LRED"["ANSI_UNDERLINE"WARNING"ANSI_LRED"] \016&nbsp;\016 "ANSI_LWHITE"You've exceeded the maximum number of brackets allowed in a calculation ("ANSI_LYELLOW"%d"ANSI_LWHITE".)",MAX_BRACKETS);
        setreturn(ERROR,COMMAND_FAIL);
        return;
     } else if(!val1) command_type &= ~ARITHMETIC_EXCEPTION;

     /* ---->  Initialise  <---- */
     compare.comparison = 0;
     compare.calctype   = 0;
     result.calcfloat   = 0;
     result.calctype    = 0;
     result.calcint     = 0;

     result.calcstring  = (char *) malloc_string("");
     compare.calcstring = (char *) malloc_string("");

     /* ---->  Evaluate brackets in expression  <---- */
     calculate_bracket_substitute(player,params,exp,val1 + 1);

     /* ---->  Evaluate expression (EXP)  <---- */
     for(p1 = exp; *p1 && (*p1 == ' '); p1++);
     while(*p1 && (result.calctype != CALC_ERROR)) {
           if(*p1) switch(*p1) {
              case '"':

                   /* ---->  String op.  <---- */
                   p1++, p2 = calcbuf;
                   while(*p1 && (*p1 != '\"')) *p2++ = *p1++;
                   while(*p1 && !CALC_SEPARATOR(*p1)) *p2++ = *p1++;
                   if(p2 != calcbuf) {
                      p2--;
                      if(*p2 && (*p2 != '\"')) p2++;
                      *p2 = '\0';
		   } else *p2 = '\0';

                   result = calculate_string_op(player,result,calcbuf,calc_op);

                   calc_op = CALC_DEFAULT;
                   bool_op = 0;
                   negate  = 0;
                   break;
              case '+':
                   calc_op = CALC_ADD;
                   p1++;
                   break;
              case '-':
                   if(negate == 1) calc_op = CALC_SUB;
                      else if(calc_op == CALC_SUB) {
                         calc_op = CALC_ADD;
                         negate  = 0;
		      } else if(calc_op != CALC_ADD) negate = 1;
                         else calc_op = CALC_SUB;
                   p1++;
                   break;
              case '*':
                   p1++;
                   if(*p1 && (*p1 == '*')) {
                      calc_op = CALC_PWR;
                      if(*p1) p1++;
                   } else calc_op = CALC_MUL;
                   break;
              case '/':
                   calc_op = CALC_DIV;
                   p1++;
                   break;
              case '%':
                   calc_op = CALC_MOD;
                   p1++;
                   break;
              case '|':
                   p1++;
                   if(*p1 && (*p1 == '|')) {
                      calc_op = CALC_OR;
                      if(*p1) p1++;
                   } else calc_op = CALC_BITOR;
                   break;
              case '&':
                   p1++;
                   if(*p1 && (*p1 == '&')) {
                      calc_op = CALC_AND;
                      if(*p1) p1++;
                   } else calc_op = CALC_BITAND;
                   break;
              case '!':
                   p1++;
                   if(*p1 && (*p1 == '=')) {
                      calc_op = CALC_NOTEQUAL;
                      if(*p1) p1++;
                   } else bool_op = CALC_NOT;		       
                   break;
              case '~':
                   bool_op = CALC_BITNOT;
                   p1++;
                   break;
              case '<':
                   p1++;
                   if(*p1 && (*p1 == '=')) {
                      calc_op = CALC_LTE;
                      if(*p1) p1++;
		   } else if(*p1 && (*p1 == '>')) {
                      calc_op = CALC_NOTEQUAL;
                      if(*p1) p1++;
                   } else if(*p1 && (*p1 == '<')) {
                      calc_op = CALC_BITSHL;
                      if(*p1) p1++;
                   } else calc_op = CALC_LT;
                   break;
              case '>':
                   p1++;
                   if(*p1 && (*p1 == '=')) {
                      calc_op = CALC_GTE;
                      if(*p1) p1++;
	           } else if(*p1 && (*p1 == '>')) {
                      calc_op = CALC_BITSHR;
                      if(*p1) p1++;
	           } else calc_op = CALC_GT;
                   break;
              case '=':
                   p1++;
                   if(*p1 && (*p1 == '<')) {
                      calc_op = CALC_LTE;
                      if(*p1) p1++;
                   } else if(*p1 && (*p1 == '>')) {
                      calc_op = CALC_GTE;
                      if(*p1) p1++;
		   } else if(*p1 && (*p1 == '=')) {
                      calc_op = CALC_EQUAL;
                      if(*p1) p1++;
                   } else calc_op = CALC_EQUAL;
                   break;
              case '^':
                   calc_op = CALC_PWR;
                   p1++;
                   break;
              case '#':
                   if(!strncmp(p1,"##",2)) {
                      calc_op = CALC_ADD;
                      p1 += 2;
                      break;
	           } else fallthru = 1;  /*  Fall-through to allow comparisons between ID numbers that aren't in ""'s  */
              default:
                   colon = 0, numeric = 1, floating = 0, p2 = calcbuf;
                   if(fallthru) fallthru = 0, p1++;
                   while(*p1 && ((colon && (*p1 == '-')) || !CALC_SEPARATOR(*p1))) {
                         if(!(isdigit(*p1) || (*p1 == '.') || (*p1 == '-') || (*p1 == ':'))) numeric = 0;
                         if(*p1 == ':') colon = 1, floating = 1;
			    else if(*p1 == '.') floating = 1; 
                         *p2++ = *p1++;
		   }
                   *p2 = '\0';

                   /* ---->  Substitute the value of PI  <---- */
                   if(!numeric && (!strcasecmp("pi",calcbuf))) {
                      strcpy(calcbuf,CALC_PI);
                      floating = 1;
                      numeric  = 1;
		   }

                   if(numeric) {

                      /* ---->  Numeric op.  <---- */
                      if(floating && ((bool_op != CALC_CONVINT) || (bool_op == CALC_CONVFLOAT) || CALC_SCIENTIFIC(bool_op))) {
                         temp.calcfloat = tofloat(calcbuf,&dps);
                         if((dps != NOTHING) && ((dp == NOTHING) || (dps > dp))) dp = dps;
                         if(negate) temp.calcfloat = 0 - temp.calcfloat;
                         temp.calctype  = CALC_FLOAT;
                         switch(bool_op) {
                                case CALC_NOT:
                                case CALC_BITNOT:
                                     temp.calcfloat = !(temp.calcfloat);
                                     break;
                                case CALC_ABS:
                                     if(temp.calcfloat < 0) temp.calcfloat = 0 - temp.calcfloat;
                                     break;
                                case CALC_SQRT:
                                     if(temp.calcfloat < 0) {
                                        if(!in_command) output(getdsc(player),player,0,1,12,ANSI_LGREEN"[EVALUATE] \016&nbsp;\016 "ANSI_LWHITE"Error  -  Value is outside square domain.");
                                        temp.calctype = CALC_ERROR;
				     } else {
                                        temp.calcfloat = sqrt(temp.calcfloat);
                                        if(calc_op == CALC_DEFAULT) calc_op = CALC_MUL;
				     }
                                     break;
                                case CALC_SIN:
                                     temp.calcfloat = sin(temp.calcfloat);
                                     if(calc_op == CALC_DEFAULT) calc_op = CALC_MUL;
                                     break;
                                case CALC_COS:
                                     temp.calcfloat = cos(temp.calcfloat);
                                     if(calc_op == CALC_DEFAULT) calc_op = CALC_MUL;
                                     break;
                                case CALC_TAN:
                                     temp.calcfloat = tan(temp.calcfloat);
                                     if(calc_op == CALC_DEFAULT) calc_op = CALC_MUL;
                                     break;
                                case CALC_ASIN:
                                     if((temp.calcfloat < -1) || (temp.calcfloat > 1)) {
                                        if(!in_command) output(getdsc(player),player,0,1,12,ANSI_LGREEN"[EVALUATE] \016&nbsp;\016 "ANSI_LWHITE"Error  -  Value must be in the range of "ANSI_LYELLOW"-1"ANSI_LWHITE" to "ANSI_LYELLOW"1"ANSI_LWHITE".");
                                        temp.calctype = CALC_ERROR;
				     } else {
                                        temp.calcfloat = asin(temp.calcfloat);
                                        if(calc_op == CALC_DEFAULT) calc_op = CALC_MUL;
				     }
                                     break;
                                case CALC_ACOS:
                                     if((temp.calcfloat < -1) || (temp.calcfloat > 1)) {
                                        if(!in_command) output(getdsc(player),player,0,1,12,ANSI_LGREEN"[EVALUATE] \016&nbsp;\016 "ANSI_LWHITE"Error  -  Value must be in the range of "ANSI_LYELLOW"-1"ANSI_LWHITE" to "ANSI_LYELLOW"1"ANSI_LWHITE".");
                                        temp.calctype = CALC_ERROR;
				     } else {
                                        temp.calcfloat = acos(temp.calcfloat);
                                        if(calc_op == CALC_DEFAULT) calc_op = CALC_MUL;
				     }
                                     break;
                                case CALC_ATAN:
                                     temp.calcfloat = atan(temp.calcfloat);
                                     if(calc_op == CALC_DEFAULT) calc_op = CALC_MUL;
                                     break;
                                default:
				     break;
		         }
                         temp.calcint = temp.calcfloat;
		      } else {
                         if(bool_op == CALC_CONVINT) {
                            p2 = calcbuf;
                            while(*p2 && (*p2 != '.')) p2++;
                            if(*p2) p2 = '\0';
			 }
                         temp.calcint   = atol(calcbuf);
                         if(negate) temp.calcint = 0 - temp.calcint;
                         temp.calctype  = CALC_INT;
                         switch(bool_op) {
                                case CALC_NOT:
                                     temp.calcint = !(temp.calcint);
                                     break;
                                case CALC_BITNOT:
                                     temp.calcint = ~(temp.calcint);
                                     break;
                                case CALC_ABS:
                                     if(temp.calcint < 0) temp.calcint = 0 - temp.calcint;
                                     break;
                                default:
				     break;
		         }
                         temp.calcfloat = temp.calcint;
		      }

                      /* ---->  Do the numeric op.  <---- */
                      if(temp.calctype != CALC_ERROR)
                         result = calculate_numeric_op(player,result,temp,calc_op);
                            else result.calctype = CALC_ERROR;

                      calc_op = CALC_DEFAULT;
                      bool_op = 0;
                      negate  = 0;
		   } else {

                      /* ---->  Is it another operator, or is it a string?  <---- */
                      if(!strcmp(calcbuf,OK)) {
                         if(!result.calctype) result.calctype = CALC_INT;
                         if(result.calctype != CALC_STRING) {
                            temp.calcint    = ((bool_op == CALC_NOT) || (bool_op == CALC_BITNOT)) ? 0:1;
                            temp.calcfloat  = ((bool_op == CALC_NOT) || (bool_op == CALC_BITNOT)) ? 0:1;
                            temp.calctype   = result.calctype;
                            result = calculate_numeric_op(player,result,temp,calc_op);
			 } else result = calculate_string_op(player,result,OK,calc_op);
                         calc_op = CALC_DEFAULT;
                         bool_op = 0;
                         negate  = 0;
		      } else if(!strcmp(calcbuf,ERROR)) {
                         if(!result.calctype) result.calctype = CALC_INT;
                         if(result.calctype != CALC_STRING) {
                            temp.calcint    = ((bool_op == CALC_NOT) || (bool_op == CALC_BITNOT)) ? 1:0;
                            temp.calcfloat  = ((bool_op == CALC_NOT) || (bool_op == CALC_BITNOT)) ? 1:0;
                            temp.calctype   = result.calctype;
                            result = calculate_numeric_op(player,result,temp,calc_op);
			 } else result = calculate_string_op(player,result,ERROR,calc_op);
                         calc_op = CALC_DEFAULT;
                         bool_op = 0;
                         negate  = 0;
		      } else if(!strcasecmp(calcbuf,"int")) {
                         bool_op = CALC_CONVINT;
		      } else if(!strcasecmp(calcbuf,"float") || !strcasecmp(calcbuf,"real")) {
                         bool_op = CALC_CONVFLOAT;
		      } else if(!strcasecmp(calcbuf,"div")) {
                         calc_op = CALC_DIV;
		      } else if(!strcasecmp(calcbuf,"mod")) {
                         calc_op = CALC_MOD;
		      } else if(!strcasecmp(calcbuf,"and")) {
                         calc_op = CALC_AND;
		      } else if(!strcasecmp(calcbuf,"or")) {
                         calc_op = CALC_OR;
		      } else if(!strcasecmp(calcbuf,"not")) {
                         bool_op = CALC_NOT;
		      } else if(!strcasecmp(calcbuf,"xor")) {
                         calc_op = CALC_XOR;
		      } else if(!strcasecmp(calcbuf,"shl")) {
                         calc_op = CALC_BITSHL;
		      } else if(!strcasecmp(calcbuf,"shr")) {
                         calc_op = CALC_BITSHR;
		      } else if(!strcasecmp(calcbuf,"sqrt")) {
                         bool_op = CALC_SQRT;
		      } else if(!strcasecmp(calcbuf,"cos")) {
                         bool_op = CALC_COS;
		      } else if(!strcasecmp(calcbuf,"sin")) {
                         bool_op = CALC_SIN;
		      } else if(!strcasecmp(calcbuf,"tan")) {
                         bool_op = CALC_TAN;
		      } else if(!strcasecmp(calcbuf,"acos")) {
                         bool_op = CALC_ACOS;
		      } else if(!strcasecmp(calcbuf,"asin")) {
                         bool_op = CALC_ASIN;
		      } else if(!strcasecmp(calcbuf,"atan")) {
                         bool_op = CALC_ATAN;
		      } else if(!strcasecmp(calcbuf,"abs")) {
                         bool_op = CALC_ABS;
		      } else if(!strcmp(calcbuf,LIMIT_EXCEEDED) || !strcmp(calcbuf,UNKNOWN_VARIABLE) || !strcmp(calcbuf,UNSET_VALUE)) {
                         if(!result.calctype) result.calctype = CALC_INT;
                         if(result.calctype != CALC_STRING) {
                            temp.calcint    = 0;
                            temp.calcfloat  = 0;
                            temp.calctype   = result.calctype;
                            result = calculate_numeric_op(player,result,temp,calc_op);
			 } else result = calculate_string_op(player,result,ERROR,calc_op);
                         calc_op = CALC_DEFAULT;
                         bool_op = 0;
                         negate  = 0;
		      } else {
                         result  = calculate_string_op(player,result,calcbuf,calc_op);
                         calc_op = CALC_DEFAULT;
                         bool_op = 0;
                         negate  = 0;
		      }
		   }
	   }

           /* ---->  Skip over leading blanks  <---- */
           while(*p1 && (*p1 == ' ')) p1++;

           /* ---->  Check right-hand side of comparison  <---- */
           if((!*p1 && compare.comparison) || CALC_COMPARISON(calc_op)) {
               if(!compare.comparison && (result.calctype != 0) && *p1) {

                  /* ---->  Initialise comparison  <---- */
                  FREENULL(compare.calcstring);
                  compare.comparison = calc_op;
                  compare.calcstring = (char *) malloc_string(result.calcstring);
                  compare.calcfloat  = result.calcfloat;
                  compare.calctype   = result.calctype;
                  compare.calcint    = result.calcint;

                  FREENULL(result.calcstring);
                  result.calcstring = (char *) malloc_string("");
                  result.calcfloat  = 0;
                  result.calctype   = 0;
                  result.calcint    = 0;
 
                  calc_op = CALC_INIT;
                  bool_op = 0;
                  negate  = 0;
	       } else if(compare.comparison || (result.calctype == 0) || !*p1) {

                  /* ---->  Check LHS/RHS exist...  <---- */
                  if(!*p1 && CALC_COMPARISON(calc_op)) {
                     if(!in_command) output(getdsc(player),player,0,1,12,ANSI_LGREEN"[EVALUATE] \016&nbsp;\016 "ANSI_LWHITE"Error  -  Comparison has no right-hand side expression.");
                     compare.calctype = CALC_ERROR;
		  } else if(result.calctype == 0) {
                     if(!in_command) output(getdsc(player),player,0,1,12,ANSI_LGREEN"[EVALUATE] \016&nbsp;\016 "ANSI_LWHITE"Error  -  Comparison has no left-hand side expression.");
                     compare.calctype = CALC_ERROR;
		  }

                  /* ---->  Pre-processing  <---- */
                  switch(compare.calctype) {
                         case CALC_INT:
                              compare.calcfloat = compare.calcint;
                         case CALC_FLOAT:
                              switch(result.calctype) {
                                     case CALC_INT:
                                          result.calcfloat = result.calcint;
                                          break;
                                     case CALC_STRING:
                                          if(!in_command) output(getdsc(player),player,0,1,12,ANSI_LGREEN"[EVALUATE] \016&nbsp;\016 "ANSI_LWHITE"Error  -  Comparison of integer with string.");
                                          result.calctype = CALC_ERROR;
                                          break;
			      }
		              break;
                         case CALC_STRING:
                              if((result.calctype == CALC_FLOAT) || (result.calctype == CALC_INT)) {
                                 if(!in_command) output(getdsc(player),player,0,1,12,ANSI_LGREEN"[EVALUATE] \016&nbsp;\016 "ANSI_LWHITE"Error  -  Comparison of integer with string.");
                                 compare.calctype = CALC_ERROR;
			      }
                              break;
		  }

                  /* ---->  Check for errors, then compare...  <---- */
		  if((compare.calctype == CALC_ERROR) || (result.calctype == CALC_ERROR)) {
                      FREENULL(compare.calcstring);
                      compare.calcstring = (char *) malloc_string("0");
                      compare.calctype   = CALC_INT;
                      compare.calcfloat  = 0;
                      compare.calcint    = 0;                     
		  } else {

                      /* ---->  Comparison  <---- */
                      comp_op = 0;
                      switch(compare.comparison) {
                             case CALC_NOTEQUAL:
                                  if(compare.calctype == CALC_STRING) {
                                     if(strcasecmp(compare.calcstring,result.calcstring)) comp_op = 1;
				  } else if(compare.calcfloat != result.calcfloat) comp_op = 1;
                                  break;
                             case CALC_LT:
                                  if(compare.calctype == CALC_STRING) {
                                     if(strcasecmp(compare.calcstring,result.calcstring) < 0) comp_op = 1;
				  } else if(compare.calcfloat < result.calcfloat) comp_op = 1;
                                  break;
                             case CALC_GT:
                                  if(compare.calctype == CALC_STRING) {
                                     if(strcasecmp(compare.calcstring,result.calcstring) > 0) comp_op = 1;
				  } else if(compare.calcfloat > result.calcfloat) comp_op = 1;
                                  break;
                             case CALC_LTE:
                                  if(compare.calctype == CALC_STRING) {
                                     if(strcasecmp(compare.calcstring,result.calcstring) <= 0) comp_op = 1;
				  } else if(compare.calcfloat <= result.calcfloat) comp_op = 1;
                                  break;
                             case CALC_GTE:
                                  if(compare.calctype == CALC_STRING) {
                                     if(strcasecmp(compare.calcstring,result.calcstring) >= 0) comp_op = 1;
				  } else if(compare.calcfloat >= result.calcfloat) comp_op = 1;
                                  break;
                             case CALC_EQUAL:
                             default:
                                  if(compare.calctype == CALC_STRING) {
                                     if(!strcasecmp(compare.calcstring,result.calcstring)) comp_op = 1;
				  } else if(compare.calcfloat == result.calcfloat) comp_op = 1;
		      }
                      FREENULL(compare.calcstring);
                      if(calc_op) compare.comparison = calc_op;
                      compare.calctype = CALC_INT;

                      if(comp_op) {
                         compare.calcstring = (char *) malloc_string("1");
                         compare.calcfloat  = 1;
                         compare.calcint    = 1;
		      } else {
                         compare.calcstring = (char *) malloc_string("0");
                         compare.calcfloat  = 0;
                         compare.calcint    = 0;
		      }
		  }

                  FREENULL(result.calcstring);
                  result.calcstring  = (char *) malloc_string("");

                  result.calcfloat   = 0;
                  result.calctype    = 0;
                  result.calcint     = 0;
 
                  calc_op = CALC_INIT;
                  bool_op = 0;
                  negate  = 0;
	       }
	   }    
     }

     /* ---->  If comparison done, set RESULT to its result  <---- */
     if(compare.comparison) {
        result.calctype    = CALC_INT;
        result.calcint     = compare.calcint;
     }

     /* ---->  Return result of calculation  <---- */
     switch(result.calctype) {
            case CALC_INT:
                 sprintf(calcbuf,"%ld",result.calcint);
                 setreturn(calcbuf,COMMAND_SUCC);
                 break;
            case CALC_FLOAT:
                 sprintf(prototype,"%%.%df",(dp == NOTHING) ? 10:dp);
                 sprintf(calcbuf,prototype,result.calcfloat);
                 if(*(p2 = calcbuf)) {
                    p2 += strlen(calcbuf) - 1;
                    while(*p2 && (*p2 == '0')) *p2-- = '\0';
                    if(*p2 && (*p2 == '.')) {
                       *(++p2) = '0';
                       *(++p2) = '\0';
		    }
		 }
                 setreturn(calcbuf,COMMAND_SUCC);
                 break;
            case CALC_STRING:
                 strcpy(calcbuf,result.calcstring);
                 setreturn(calcbuf,COMMAND_SUCC);
                 break;
            case CALC_ERROR:
            default:
                 setreturn(ERROR,COMMAND_FAIL);
     }
     FREENULL(compare.calcstring);
     FREENULL(result.calcstring);
}
