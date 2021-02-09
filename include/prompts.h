/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| PROMPTS.H  -  Standard prompts displayed to users.                          |
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
| Module originally designed and written by:  J.P.Boggis 18/06/2001.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                        https://github.com/smcvey/tcz                        |
`-----------------------------------------------------------------------------'

*/


/* ---->  Standard default TCZ prompt (%s replaced with TCZ MUD short name)  <---- */
#define TELNET_TCZ_PROMPT                  "%s>"

/* ---->  Prompt used when user is in BBS room (%s replaced with TCZ MUD short name)  <---- */
#define TELNET_BBS_PROMPT                  "%s BBS>"

/* ---->  Prompt used when user is in mail room (%s replaced with TCZ MUD short name)  <---- */
#define TELNET_MAIL_PROMPT                 "%s Mail>"

/* ---->  Prompt used when user is in bank room (%s replaced with TCZ MUD short name)  <---- */
#define TELNET_BANK_PROMPT                 "%s Bank>"

/* ---->  Login prompt (Without newbie instructions), displayed when user initiates connection to TCZ  <---- */
#define LOGIN_PROMPT                       ANSI_LGREEN"Please enter your name (Or "ANSI_LYELLOW"NEW"ANSI_LGREEN"):"

/* ---->  Login prompt additional newbie instructions  <---- */
#define NEWBIE_PROMPT                      ANSI_LWHITE"If you already have an existing character, please enter their name at the prompt below.  If you are a new user, and do not have a character yet, you will need to create one.  To do this, simply type "ANSI_LGREEN"NEW"ANSI_LWHITE" at the prompt below.\n\n"

/* ---->  Enter preferred name prompt (Create)  <---- */
#define PREFERRED_NAME_PROMPT              ANSI_LWHITE"\nYou need to think up a suitable name for your new character.  This "ANSI_LCYAN"does not"ANSI_LWHITE" need to be your real life name, and can be up to 20 letters in length and may contain spaces.\n\n"ANSI_LGREEN"Please enter your preferred character name:"

/* ---->  Create character confirmation prompt (%s replaced with desired character name)  <---- */
#define CREATE_PROMPT                      "\n"ANSI_LGREEN"Would you like to create a new character with the name '"ANSI_LWHITE"%s"ANSI_LGREEN"' ("ANSI_LYELLOW"Y"ANSI_LGREEN"/"ANSI_LYELLOW"N"ANSI_LGREEN")?"

/* ---->  Enter password prompt (Connect)  <---- */
#define CONNECT_PASSWORD_PROMPT            "\n"ANSI_LGREEN"Please enter your password:"

/* ---->  Enter password prompt (Create)  <---- */
#define CREATE_PASSWORD_PROMPT             "\n"ANSI_LWHITE"You now need to think of a suitable password for your character  -  This should consist of 6 or more letters/numbers, be easy to remember and "ANSI_LRED"MUST NOT"ANSI_LWHITE" be a password used for your Internet Service Provider, E-mail, etc. account(s).\n\n"ANSI_LGREEN"Please enter a password for your character:"

/* ---->  Verify password prompt  <---- */
#define VERIFY_PASSWORD_PROMPT             "\n"ANSI_LGREEN"Please re-type your password for verification:"

/* ---->  Verify character creation  <---- */
#define VERIFY_CREATE_PROMPT               ANSI_LGREEN"Would you still like to create this character ("ANSI_LYELLOW"Y"ANSI_LGREEN"/"ANSI_LYELLOW"N"ANSI_LGREEN")?"

/* ---->  Simple 'more' pager prompt (Used by paged disclaimer/title screens)  <---- */
#define SIMPLE_PAGER_PROMPT                ANSI_DCYAN"["ANSI_LCYAN"Press "ANSI_LWHITE"RETURN"ANSI_LCYAN"/"ANSI_LWHITE"ENTER"ANSI_LCYAN" to continue..."ANSI_DCYAN"]" 

/* ---->  Re-accept disclaimer prompt (1st %s = TCZ MUD full name, 2nd/3rd %s = TCZ MUD short name)  <---- */
#define REACCEPT_DISCLAIMER_PROMPT         "\n"ANSI_LYELLOW"Your continued use of "ANSI_LWHITE"%s"ANSI_LYELLOW" ("ANSI_LWHITE"%s"ANSI_LYELLOW") is subject to accepting the terms and conditions of the "ANSI_LWHITE""ANSI_UNDERLINE"disclaimer"ANSI_LYELLOW" and abiding by the "ANSI_LWHITE""ANSI_UNDERLINE"Official Rules of %s"ANSI_LYELLOW" (See '"ANSI_LGREEN"help rules"ANSI_LYELLOW"'.)  Please read the disclaimer "ANSI_LRED"CAREFULLY"ANSI_LYELLOW" and then type "ANSI_LGREEN"ACCEPT"ANSI_LYELLOW" if you fully agree with its terms and conditions."

/* ---->  Accept disclaimer terms and conditions prompt  <---- */
#define TELNET_ACCEPT_DISCLAIMER_PROMPT    "\n"ANSI_LGREEN"Do you accept the above terms and conditions (Type "ANSI_LYELLOW"ACCEPT"ANSI_LGREEN"/"ANSI_LYELLOW"REJECT"ANSI_LGREEN" in full)?"

/* ---->  Choose character gender prompt (Create)  <---- */
#define TELNET_GENDER_PROMPT               "\n"ANSI_LGREEN"Your new character needs a gender  -  Please enter ("ANSI_LYELLOW"M"ANSI_LGREEN")ale, ("ANSI_LYELLOW"F"ANSI_LGREEN")emale or ("ANSI_LYELLOW"N"ANSI_LGREEN")euter:"

/* ---->  Enter race prompt (Create)  <---- */
#define TELNET_RACE_PROMPT                 "\n"ANSI_LGREEN"Please enter your character's race (E.g:  "ANSI_LYELLOW"Human"ANSI_LGREEN", "ANSI_LYELLOW"Alien"ANSI_LGREEN", etc.):"

/* ---->  Enter E-mail address prompt (Create) (%s replaced with TCZ Admin. E-mail address)  <---- */
#define TELNET_EMAIL_PROMPT                "\n"ANSI_LWHITE"Please enter your full E-mail address  -  This should be in the format "ANSI_LYELLOW"USERNAME"ANSI_DYELLOW"@"ANSI_LYELLOW"YOUR_SITE"ANSI_LWHITE", e.g:  "ANSI_LGREEN"%s"ANSI_LWHITE" (Example only, "ANSI_LRED"DO NOT"ANSI_LWHITE" use!)  Your E-mail address will remain "ANSI_LRED"STRICTLY PRIVATE AND CONFIDENTIAL"ANSI_LWHITE".\n\n"ANSI_LGREEN"Please enter your full E-mail address:"

/* ---->  Enter old password prompt ('@password')  <---- */
#define TELNET_OLD_PASSWORD_PROMPT         ANSI_LGREEN"Please enter your old password:"

/* ---->  Enter new password prompt ('@password')  <---- */
#define TELNET_NEW_PASSWORD_PROMPT         ANSI_LGREEN"Please enter a new password for your character:"

/* ---->  Verify new password ('@password')  <---- */
#define TELNET_VERIFY_NEW_PASSWORD_PROMPT  ANSI_LGREEN"Please re-type your new password for verification:"

/* ---->  Enter password to leave AFK prompt ('afk')  <---- */
#define AFK_PASSWORD_PROMPT                ANSI_LRED"[AFK]  "ANSI_LWHITE"Please enter your password:"

/* ---->  Take over existing connection prompt (Connect) (%s replaced with TCZ MUD full name)  <---- */
#define TAKE_OVER_CONNECTION_PROMPT        "\n"ANSI_LWHITE"You're already currently connected to %s.  If your previous connection has locked up and is no-longer responding (And you have disconnected and then reconnected again because of this), you can attempt to resume this connection (Continuing with what you were last doing)  -  If you do this, and the connection still doesn't respond, simply disconnect, reconnect again and type "ANSI_LGREEN"N"ANSI_LWHITE" at the prompt below and then type "ANSI_LGREEN"@BOOTDEAD"ANSI_LWHITE" once connected to get rid of your 'dead' connections.\n\n"ANSI_LGREEN"Would you like to try and resume using your previous connection ("ANSI_LYELLOW"Y"ANSI_LGREEN"/"ANSI_LYELLOW"N"ANSI_LGREEN")?"

/* ---->  Local echo prompt (Create)  <---- */
#define LOCAL_ECHO_PROMPT                  "\n"ANSI_LWHITE"When you type, can you see what you are typing on your screen? (If you're unsure, try typing something now!)  If not, first try looking in the configuration/options screen/menu of your Telnet/communications software for a "ANSI_LCYAN"LOCAL ECHO"ANSI_LWHITE" option and turn it "ANSI_LYELLOW"ON"ANSI_LWHITE".  If you can't find this option or can't get it to work, answer "ANSI_LGREEN"NO"ANSI_LWHITE" to the question below, otherwise answer "ANSI_LGREEN"YES"ANSI_LWHITE".\n\n"ANSI_LGREEN"Can you see what you're typing on your screen ("ANSI_LYELLOW"Y"ANSI_LGREEN"/"ANSI_LYELLOW"N"ANSI_LGREEN")?"

/* ---->  Time difference prompt (Create) (1st %s = TCZ MUD full name, 2nd %s = Current time/date, 3rd %s = System time zone, 4th %s = Standard TCZ prompt)  <---- */
#define TELNET_TIME_DIFFERENCE_PROMPT      "\n"ANSI_LWHITE"The local server time on %s is currently "ANSI_LMAGENTA"%s"ANSI_LWHITE" ("ANSI_LYELLOW"%s"ANSI_LWHITE".)  If you're an overseas user and your local time is later or earlier than this time by an hour or more, you need to enter your time difference (In hours) at the prompt below.  For example, if you're "ANSI_LCYAN"two hours"ANSI_LWHITE" ahead, enter "ANSI_LGREEN"2"ANSI_LWHITE" and if you're "ANSI_LCYAN"five hours"ANSI_LWHITE" behind, enter "ANSI_LGREEN"-5"ANSI_LWHITE", etc.  If your time is the same or close to the one above, enter "ANSI_LGREEN"0"ANSI_LWHITE".\n\nYou can use the "ANSI_LGREEN"SET TIMEDIFF"ANSI_LWHITE" command at the '%s' prompt to set/change your time difference at a later date.\n\n"ANSI_LGREEN"Please enter your time difference (In hours):"

/* ---->  Set ANSI colour prompt help (Create/'screenconfig') (1st/2nd %s = TCZ MUD full name,  3rd %s = Standard TCZ prompt)  <---- */
#define SET_ANSI_COLOUR_TABLE_HELP         "\n"ANSI_DWHITE"The following questions will help you configure %s to suit the terminal/display you're currently using  -  If you connect to %s from a different type of computer/terminal in the future, you may need to run through these questions again by typing SCREENCONFIG at the '%s' prompt.\n\n"

/* ---->  Set ANSI colour prompt table of colours (Create/'screenconfig')  <--- */
#define SET_ANSI_COLOUR_TABLE              ANSI_DWHITE"  NORMAL:  "ANSI_DRED"Red "ANSI_DGREEN"Green "ANSI_DYELLOW"Yellow "ANSI_DBLUE"Blue "ANSI_DMAGENTA"Magenta "ANSI_DCYAN"Cyan "ANSI_DWHITE"White\n"ANSI_LWHITE"HI-LIGHT:  "ANSI_LRED"Red "ANSI_LGREEN"Green "ANSI_LYELLOW"Yellow "ANSI_LBLUE"Blue "ANSI_LMAGENTA"Magenta "ANSI_LCYAN"Cyan "ANSI_LWHITE"White"

/* ---->  ANSI colour prompt (Create/'screenconfig')  <---- */
#define SET_ANSI_COLOUR_PROMPT             "\n\n"ANSI_DWHITE"Above are some colour names  -  Please look at them carefully and then choose either A, B or C (Whichever is the closest to how the above colour names look on your screen)...\n\n\x05\x05(A)  All colour names are not in colour, are in a single colour only or there are lots of strange codes between them.\n\x05\x05(B)  The colour names appear in the correct colours, but there is no difference between them on the two lines, or one line appears in the correct colours and the other one is either not in colour or in a single colour only.\n\x05\x05(C)  Both lines appear in the correct colours and the colours in the 'HI-LIGHT:' line appear brighter or emboldened.\n\nPlease answer A, B or C:"

/* ---->  Underline prompt (Create/'screenconfig')  <---- */
#define UNDERLINE_PROMPT                   "\n"ANSI_LCYAN""ANSI_UNDERLINE"Some underlined text"ANSI_LGREEN"\n\nDoes the above text appear underlined ("ANSI_LYELLOW"Y"ANSI_LGREEN"/"ANSI_LYELLOW"N"ANSI_LGREEN")?"

/* ---->  Enter E-mail address prompt (Request for new character) (%s replaced with TCZ Admin. E-mail)  <---- */
#define TELNET_REQUEST_EMAIL_PROMPT        ANSI_LWHITE"If you do not currently have a character and would like one created for you, please enter your E-mail address at the prompt below.  Your request will be placed on a queue and we will be in touch with you shortly via E-mail.  If you experience any difficulties, please send E-mail to "ANSI_LYELLOW"%s"ANSI_LWHITE".\n\n"ANSI_LRED"If you do not wish to make a request for a new character, please type "ANSI_LYELLOW"NO"ANSI_LRED" at the prompt below.\n\n"ANSI_LGREEN"Please enter your E-mail address:"
