/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| DESCRIPTOR_FLAGS.H  -  Flags used by descriptor of character's connection   |
|                        (Telnet/HTML.)  Most of these are temporary and are  |
| 			 not saved when the user disconnects.		      |
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
| Module originally designed and written by:  J.P.Boggis 04/12/2003.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: descriptor_flags.h,v 1.1.1.1 2004/12/02 17:43:17 jpboggis Exp $

*/


/* ---->  Primary descriptor list flags  <---- */
#define SUPPRESS_ECHO      0x00000001  /*  Suppresses local echo (I.e:  When entering passwords, etc.)  */
#define DESTROY            0x00000002  /*  Destroy character when they disconnect (Guest characters)  */
/*      LFTOCR_LFCR        0x00000004      Terminal's end-of-line terminator (Already defined elsewhere)  */
/*      LFTOCR_CRLF        0x00000008      Terminal's end-of-line terminator (Already defined elsewhere)  */
#define INITIALISE         0x00000020  /*  Connection is in initialise state:  First input will be checked for external database dumping process/port concentrator identification code  */
/*      CONNECTED          0x00000040      Character is currently connected to TCZ (Already defined elsewhere)  */
/*      ANSI8              0x00000080      Character's ANSI colour preference (8 colour) (Already defined elsewhere)  */
#define SSL                0x00000100  /*  SSL (Secure Socket Layer) connection  */
#define DISCONNECTED       0x00000200  /*  User's connection lost  -  Connection kept open for 5 mins to allow for reconnection  */
#define PROMPT             0x00000400  /*  User prompt previously displayed  */
/*      UNDERLINE          0x00000800      Character's underline preference (Already defined elsewhere)  */
#define TELNET_SUBNEG      0x00001000  /*  'Split' Telnet sub-negotiation in progress  */
#define TELNET_EOR         0x00002000  /*  IAC EOR should be added to the end of user prompts (Support for TinyFugue)  */
#define CLOSING            0x00004000  /*  Descriptor in process of closing  -  Close after output has been delivered  */
#define LASTMSG_TELL       0x00008000  /*  Last message sent was via 'tell' (Rather than 'page')  */
/*      LOCAL_ECHO         0x00010000      TCZ should echo text typed by user back to their terminal? (Already defined elsewhere)  */
#define EVALUATE           0x00020000  /*  Backslashes in input text will be evaluated  */
#define BACKSLASH          0x00040000  /*  Last (text) character processed was an unprotected backslash  */
#define NOLISTEN           0x00080000  /*  Temporarily disable '[<NAME> has connected]' style messages.  */
#define SPOKEN_TEXT        0x00100000  /*  Last command typed is spoken text (And should be censored on 'last')?  */
#define BBS_CENSOR         0x00200000  /*  Filter bad language from subject of message (Via editor)  */
#define ABSOLUTE           0x00400000  /*  Absolute censoring of last command (Unless ABSOLUTE_OVERRIDE in secondary descriptor flags is set.)  */
#define CONVERSE           0x00800000  /*  User is in 'converse' mode  */
#define MONITOR_OUTPUT     0x01000000  /*  User's output (To their screen) being monitored by Admin.  */
#define MONITOR_CMDS       0x02000000  /*  User's commands being monitored by Admin.  */
#define PROCESSED          0x04000000  /*  Descriptor processed?  */
/*      ANSI               0x08000000      Character's ANSI colour preference (16 colour) (Already defined elsewhere)  */
#define WELCOME            0x10000000  /*  New user who hasn't been welcomed yet  */
#define ASSIST             0x20000000  /*  User who has asked for assistance and hasn't received it yet  */
#define BIRTHDAY           0x40000000  /*  Character is currently celebrating their birthday  */
#define DELIVERED          0x80000000  /*  Close descriptor once output has been delivered, keeping descriptor data intact  */
#define TERMINATOR_MASK    0x0000000C  /*  Terminal end-of-line terminator mask  */
#define ANSI_MASK          0x08000080  /*  ANSI colour mask                      */
#define RECONNECT_MASK     (ABSOLUTE|ASSIST|BBS_CENSOR|BIRTHDAY|CONVERSE|DESTROY|EVALUATE|LASTMSG_TELL|MONITOR_CMDS|MONITOR_OUTPUT|NOLISTEN|SPOKEN_TEXT|SUPPRESS_ECHO|WELCOME)


/* ---->  Secondary descriptor list flags  <---- */
#define OUTPUT_PENDING     0x00000001  /*  Output pending on descriptor  */
#define OUTPUT_SUPPRESS    0x00000002  /*  Suppress output during compound command ('@output off'.)  */
#define ABSOLUTE_OVERRIDE  0x00000100  /*  Override absolute censoring of last command typed  */
#define WARN_LOGIN_FAILED  0x00000200  /*  Warn user of failed login     */
#define SENT_AUTO_AFK      0x00001000  /*  User has been sent AFK automatically (After idling for AFK_TIME minutes.)  */
#define NOTIFIED           0x00002000  /*  User notified with reminder (Assist/Welcome)  */
/*      HTML_TAG           0x04000000      Open HTML tag (Use in html_to_text() conversion) (Already defined elsewhere)  */
#define RECONNECT_MASK2    (ABSOLUTE_OVERRIDE)
