/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| MODULES_C.H  -  Module author information definitions for '*.c' source      |
|                 files.                                                      |
|                                                                             |
|                 Format of author information:                               |
|                 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~                               |
|                  Initials:  Author:   Date From:   Date To:                 |
|                 "INITIALS | Y/N     | DD/MM/YYYY | DD/MM/YYYY >>>"          |
|                                                                             |
|                 *  NULL for Date To: indicates author is still actively     |
|                    working on module.                                       |
|                                                                             |
|                 *  Ensure that the initials of the author exist in the      |
|                    table in include/modules_authors.h                       |
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
| Module originally designed and written by:  J.P.Boggis 21/05/2000.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: modules_c.h,v 1.3 2004/12/15 20:41:49 jpboggis Exp $

*/


/* ---->  From admin.c  <---- */
#define module_admin_c_date "21/12/1993"
#define module_admin_c_desc "Admin commands and utilities"
#define module_admin_c_authors \
        "BY  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DP  | N | 01/01/1990 | 21/12/1993 >>>" \
        "GDW | N | 01/01/1990 | 21/12/1993 >>>" \
        "JA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "JPB | N | 21/12/1993 | NULL >>>" \
        "MJH | N | 01/01/1990 | 21/12/1993 >>>" \
        "NH  | N | 01/01/1990 | 21/12/1993 >>>" \
        "PC  | N | 01/01/1990 | 21/12/1993 >>>" \
        "RH  | N | 01/01/1990 | 21/12/1993 >>>"


/* ---->  From alias.c  <---- */
#define module_alias_c_date "21/02/1995"
#define module_alias_c_desc "Custom command aliases"
#define module_alias_c_authors \
        "JPB | Y | 21/02/1995 | NULL >>>"


/* ---->  From array.c  <---- */
#define module_array_c_date "16/05/1995"
#define module_array_c_desc "Dynamic arrays"
#define module_array_c_authors \
        "JPB | Y | 16/05/1995 | NULL >>>"

/* ---->  From banish.c  <---- */
#define module_banish_c_date "12/04/1997"
#define module_banish_c_desc "Banished character names"
#define module_banish_c_authors \
        "JPB | Y | 12/04/1997 | NULL >>>"


/* ---->  From bbs.c  <---- */
#define module_bbs_c_date "16/07/1995"
#define module_bbs_c_desc "Topic-based Bulletin Board System"
#define module_bbs_c_authors \
        "JPB | Y | 16/07/1995 | NULL >>>"


/* ---->  From boolexp.c  <---- */
#define module_boolexp_c_date "21/12/1993"
#define module_boolexp_c_desc "Boolean expressions"
#define module_boolexp_c_authors \
        "BY  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DP  | N | 01/01/1990 | 21/12/1993 >>>" \
        "GDW | N | 01/01/1990 | 21/12/1993 >>>" \
        "JA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "JPB | N | 21/12/1993 | NULL >>>" \
        "MJH | N | 01/01/1990 | 21/12/1993 >>>" \
        "NH  | N | 01/01/1990 | 21/12/1993 >>>" \
        "PC  | N | 01/01/1990 | 21/12/1993 >>>" \
        "RH  | N | 01/01/1990 | 21/12/1993 >>>"


/* ---->  From calculate.c  <---- */
#define module_calculate_c_date "18/10/1994"
#define module_calculate_c_desc "Integer, floating point and string-based calculations ('@eval'/'@calc')"
#define module_calculate_c_authors \
        "JPB | Y | 18/10/1994 | NULL >>>"


/* ---->  From channels.c  <---- */
#define module_channels_c_date "18/04/1997"
#define module_channels_c_desc "Channel communication"
#define module_channels_c_authors \
        "JPB | Y | 18/04/1997 | NULL >>>"


/* ---->  From character.c  <---- */
#define module_character_c_date "21/12/1993"
#define module_character_c_desc "Character creation, lookup, etc."
#define module_character_c_authors \
        "BY  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DP  | N | 01/01/1990 | 21/12/1993 >>>" \
        "GDW | N | 01/01/1990 | 21/12/1993 >>>" \
        "JA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "JPB | N | 21/12/1993 | NULL >>>" \
        "MJH | N | 01/01/1990 | 21/12/1993 >>>" \
        "NH  | N | 01/01/1990 | 21/12/1993 >>>" \
        "PC  | N | 01/01/1990 | 21/12/1993 >>>" \
        "RH  | N | 01/01/1990 | 21/12/1993 >>>"


/* ---->  From combat.c  <---- */
#define module_combat_c_date "01/06/1995"
#define module_combat_c_desc "Combat commands"
#define module_combat_c_authors \
        "JPB | Y | 01/06/1995 | NULL >>>"


/* ---->  From command.c  <---- */
#define module_command_c_date "21/12/1993"
#define module_command_c_desc "Compound command processing"
#define module_command_c_authors \
        "DP  | Y | 01/01/1990 | 21/12/1993 >>>" \
        "GDW | Y | 01/01/1990 | 21/12/1993 >>>" \
        "JPB | N | 21/12/1993 | NULL >>>" \
        "MJH | Y | 01/01/1990 | 21/12/1993 >>>" \
        "NH  | Y | 01/01/1990 | 21/12/1993 >>>" \
        "PC  | Y | 01/01/1990 | 21/12/1993 >>>" \
        "RH  | Y | 01/01/1990 | 21/12/1993 >>>"


/* ---->  From communication.c  <---- */
#define module_communication_c_date "21/12/1993"
#define module_communication_c_desc "Communication commands"
#define module_communication_c_authors \
        "BY  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DP  | N | 01/01/1990 | 21/12/1993 >>>" \
        "GDW | N | 01/01/1990 | 21/12/1993 >>>" \
        "JA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "JPB | N | 21/12/1993 | NULL >>>" \
        "MJH | N | 01/01/1990 | 21/12/1993 >>>" \
        "NH  | N | 01/01/1990 | 21/12/1993 >>>" \
        "PC  | N | 01/01/1990 | 21/12/1993 >>>" \
        "RH  | N | 01/01/1990 | 21/12/1993 >>>"


/* ---->  From compression.c  <---- */
#define module_compression_c_date "19/02/1996"
#define module_compression_c_desc "Compression of text fields"
#define module_compression_c_authors \
        "JPB | Y | 19/02/1996 | NULL >>>"


/* ---->  From container.c  <---- */
#define module_container_c_date "21/12/1993"
#define module_container_c_desc "Container manipulation"
#define module_container_c_authors \
        "DP  | Y | 01/01/1990 | 21/12/1993 >>>" \
        "GDW | Y | 01/01/1990 | 21/12/1993 >>>" \
        "JPB | N | 21/12/1993 | NULL >>>" \
        "MJH | Y | 01/01/1990 | 21/12/1993 >>>" \
        "NH  | Y | 01/01/1990 | 21/12/1993 >>>" \
        "PC  | Y | 01/01/1990 | 21/12/1993 >>>" \
        "RH  | Y | 01/01/1990 | 21/12/1993 >>>"


/* ---->  From create.c  <---- */
#define module_create_c_date "21/12/1993"
#define module_create_c_desc "Object creation"
#define module_create_c_authors \
        "BY  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DP  | N | 01/01/1990 | 21/12/1993 >>>" \
        "GDW | N | 01/01/1990 | 21/12/1993 >>>" \
        "JA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "JPB | N | 21/12/1993 | NULL >>>" \
        "MJH | N | 01/01/1990 | 21/12/1993 >>>" \
        "NH  | N | 01/01/1990 | 21/12/1993 >>>" \
        "PC  | N | 01/01/1990 | 21/12/1993 >>>" \
        "RH  | N | 01/01/1990 | 21/12/1993 >>>"


/* ---->  From db.c  <---- */
#define module_db_c_date "21/12/1993"
#define module_db_c_desc "Database manipulation/loading/saving"
#define module_db_c_authors \
        "BY  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DP  | N | 01/01/1990 | 21/12/1993 >>>" \
        "GDW | N | 01/01/1990 | 21/12/1993 >>>" \
        "JA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "JPB | N | 21/12/1993 | NULL >>>" \
        "MJH | N | 01/01/1990 | 21/12/1993 >>>" \
        "NH  | N | 01/01/1990 | 21/12/1993 >>>" \
        "PC  | N | 01/01/1990 | 21/12/1993 >>>" \
        "RH  | N | 01/01/1990 | 21/12/1993 >>>"


/* ---->  From destroy.c  <---- */
#define module_destroy_c_date "24/03/1995"
#define module_destroy_c_desc "Object destruction/recovery"
#define module_destroy_c_authors \
        "JPB | Y | 24/03/1995 | NULL >>>"


/* ---->  From edit.c  <---- */
#define module_edit_c_date "09/10/1994"
#define module_edit_c_desc "Line-based text editor"
#define module_edit_c_authors \
        "JPB | Y | 09/10/1994 | NULL >>>"


/* ---->  From event.c  <---- */
#define module_event_c_date "21/12/1993"
#define module_event_c_desc "Queued/timed events (Alarms/Fuses)"
#define module_event_c_authors \
        "DP  | Y | 01/01/1990 | 21/12/1993 >>>" \
        "GDW | Y | 01/01/1990 | 21/12/1993 >>>" \
        "JPB | N | 21/12/1993 | NULL >>>" \
        "MJH | Y | 01/01/1990 | 21/12/1993 >>>" \
        "NH  | Y | 01/01/1990 | 21/12/1993 >>>" \
        "PC  | Y | 01/01/1990 | 21/12/1993 >>>" \
        "RH  | Y | 01/01/1990 | 21/12/1993 >>>"


/* ---->  From feelings.c  <---- */
#define module_feelings_c_date "07/07/2000"
#define module_feelings_c_desc "Feeling commands"
#define module_feelings_c_authors \
        "JPB | Y | 07/07/2000 | NULL >>>"


/* ---->  From finance.c  <---- */
#define module_finance_c_date "03/01/1997"
#define module_finance_c_desc "Monetry system (Credits)"
#define module_finance_c_authors \
        "JPB | Y | 03/01/1997 | NULL >>>"


/* ---->  From friends.c  <---- */
#define module_friends_c_date "20/03/1994"
#define module_friends_c_desc "Friends lists & chatting channel"
#define module_friends_c_authors \
        "JPB | Y | 20/03/1994 | NULL >>>"


/* ---->  From global.c  <---- */
#define module_global_c_date "14/05/1996"
#define module_global_c_desc "Global compound command lookup"
#define module_global_c_authors \
        "JPB | Y | 14/05/1996 | NULL >>>"


/* ---->  From group.c  <---- */
#define module_group_c_date "17/11/1994"
#define module_group_c_desc "Grouping/range operators"
#define module_group_c_authors \
        "JPB | Y | 17/11/1994 | NULL >>>"


/* ---->  From help.c  <---- */
#define module_help_c_date "06/07/1994"
#define module_help_c_desc "On-line help & tutorials"
#define module_help_c_authors \
        "JPB | Y | 06/07/1994 | NULL >>>"


/* ---->  From html.c  <---- */
#define module_html_c_date "21/06/1996"
#define module_html_c_desc "World Wide Web Interface & HTML support"
#define module_html_c_authors \
        "JPB | Y | 21/06/1996 | NULL >>>"


/* ---->  From interface.c  <---- */
#define module_interface_c_date "21/12/1993"
#define module_interface_c_desc "Command pre-processing"
#define module_interface_c_authors \
        "BY  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DP  | N | 01/01/1990 | 21/12/1993 >>>" \
        "GDW | N | 01/01/1990 | 21/12/1993 >>>" \
        "JA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "JPB | N | 21/12/1993 | NULL >>>" \
        "MJH | N | 01/01/1990 | 21/12/1993 >>>" \
        "NH  | N | 01/01/1990 | 21/12/1993 >>>" \
        "PC  | N | 01/01/1990 | 21/12/1993 >>>" \
        "RH  | N | 01/01/1990 | 21/12/1993 >>>"


/* ---->  From lists.c  <---- */
#define module_lists_c_date "31/08/1998"
#define module_lists_c_desc "Character name list processing"
#define module_lists_c_authors \
        "JPB | Y | 31/08/1998 | NULL >>>"


/* ---->  From logfiles.c  <---- */
#define module_logfiles_c_date "11/03/2000"
#define module_logfiles_c_desc "System and user log files"
#define module_logfiles_c_authors \
        "JPB | Y | 11/03/2000 | NULL >>>"


/* ---->  From look.c  <---- */
#define module_look_c_date "21/12/1993"
#define module_look_c_desc "Examining rooms/objects/characters"
#define module_look_c_authors \
        "BY  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DP  | N | 01/01/1990 | 21/12/1993 >>>" \
        "GDW | N | 01/01/1990 | 21/12/1993 >>>" \
        "JA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "JPB | N | 21/12/1993 | NULL >>>" \
        "MJH | N | 01/01/1990 | 21/12/1993 >>>" \
        "NH  | N | 01/01/1990 | 21/12/1993 >>>" \
        "PC  | N | 01/01/1990 | 21/12/1993 >>>" \
        "RH  | N | 01/01/1990 | 21/12/1993 >>>"


/* ---->  From mail.c  <---- */
#define module_mail_c_date "01/11/1995"
#define module_mail_c_desc "Internal mail system"
#define module_mail_c_authors \
        "JPB | Y | 01/11/1995 | NULL >>>"


/* ---->  From map.c  <---- */
#define module_map_c_date "27/01/2000"
#define module_map_c_desc "Map of TCZ"
#define module_map_c_authors \
        "JPB | Y | 27/01/2000 | NULL >>>"


/* ---->  From match.c  <---- */
#define module_match_c_date "12/12/1997"
#define module_match_c_desc "Hierarchical object matching"
#define module_match_c_authors \
        "JPB | Y | 12/12/1997 | NULL >>>"


/* ---->  From modules.c  <---- */
#define module_modules_c_date "20/05/2000"
#define module_modules_c_desc "Description and author information for all TCZ source code modules"
#define module_modules_c_authors \
        "JPB | Y | 20/05/2000 | NULL >>>"

/* ---->  From move.c  <---- */
#define module_move_c_date "21/12/1993"
#define module_move_c_desc "Object movement"
#define module_move_c_authors \
        "BY  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DP  | N | 01/01/1990 | 21/12/1993 >>>" \
        "GDW | N | 01/01/1990 | 21/12/1993 >>>" \
        "JA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "JPB | N | 21/12/1993 | NULL >>>" \
        "MJH | N | 01/01/1990 | 21/12/1993 >>>" \
        "NH  | N | 01/01/1990 | 21/12/1993 >>>" \
        "PC  | N | 01/01/1990 | 21/12/1993 >>>" \
        "RH  | N | 01/01/1990 | 21/12/1993 >>>"


/* ---->  From options.c  <---- */
#define module_options_c_date "24/12/1999"
#define module_options_c_desc "Command-line arguments"
#define module_options_c_authors \
        "JPB | Y | 24/12/1999 | NULL >>>"


/* ---->  From output.c  <---- */
#define module_output_c_date "28/12/1994"
#define module_output_c_desc "Output to users/descriptors"
#define module_output_c_authors \
        "JPB | Y | 28/12/1994 | NULL >>>"


/* ---->  From pager.c  <---- */
#define module_pager_c_date "02/05/1996"
#define module_pager_c_desc "'more' pager for output"
#define module_pager_c_authors \
        "JPB | Y | 02/05/1996 | NULL >>>"


/* ---->  From pagetell.c  <---- */
#define module_pagetell_c_date "17/04/1995"
#define module_pagetell_c_desc "'page'/'tell' commands"
#define module_pagetell_c_authors \
        "JPB | Y | 17/04/1995 | NULL >>>"


/* ---->  From predicates.c  <---- */
#define module_predicates_c_date "21/12/1993"
#define module_predicates_c_desc "Utility functions & conditional tests"
#define module_predicates_c_authors \
        "BY  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DP  | N | 01/01/1990 | 21/12/1993 >>>" \
        "GDW | N | 01/01/1990 | 21/12/1993 >>>" \
        "JA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "JPB | N | 21/12/1993 | NULL >>>" \
        "MJH | N | 01/01/1990 | 21/12/1993 >>>" \
        "NH  | N | 01/01/1990 | 21/12/1993 >>>" \
        "PC  | N | 01/01/1990 | 21/12/1993 >>>" \
        "RH  | N | 01/01/1990 | 21/12/1993 >>>"


/* ---->  From preferences.c  <---- */
#define module_preferences_c_date "24/09/2000"
#define module_preferences_c_desc "'set' preferences command"
#define module_preferences_c_authors \
        "JPB | Y | 24/09/2000 | NULL >>>"


/* ---->  From prompt.c  <---- */
#define module_prompt_c_date "02/10/2000"
#define module_prompt_c_desc "Interactive user prompts"
#define module_prompt_c_authors \
        "JPB | Y | 02/10/2000 | NULL >>>"


/* ---->  From qmwlogsocket.c  <---- */
#define module_qmwlogsocket_c_date "20/08/2001"
#define module_qmwlogsocket_c_desc "Log to a unix domain socket (QMW research)"
#define module_qmwlogsocket_c_authors \
        "SAB | Y | 20/08/2001 | NULL >>>"


/* ---->  From query.c  <---- */
#define module_query_c_date "21/12/1993"
#define module_query_c_desc "'{@?query}' commands"
#define module_query_c_authors \
        "DP  | Y | 01/01/1990 | 21/12/1993 >>>" \
        "GDW | Y | 01/01/1990 | 21/12/1993 >>>" \
        "JPB | N | 21/12/1993 | NULL >>>" \
        "MJH | Y | 01/01/1990 | 21/12/1993 >>>" \
        "NH  | Y | 01/01/1990 | 21/12/1993 >>>" \
        "PC  | Y | 01/01/1990 | 21/12/1993 >>>" \
        "RH  | Y | 01/01/1990 | 21/12/1993 >>>"


/* ---->  From request.c  <---- */
#define module_request_c_date "18/11/1996"
#define module_request_c_desc "Request queue for new characters"
#define module_request_c_authors \
        "JPB | Y | 18/11/1996 | NULL >>>"


/* ---->  From sanity.c  <---- */
#define module_sanity_c_date "23/03/1995"
#define module_sanity_c_desc "Database integrity checks"
#define module_sanity_c_authors \
        "JPB | Y | 23/03/1995 | NULL >>>"


/* ---->  From search.c  <---- */
#define module_search_c_date "29/12/1994"
#define module_search_c_desc "Object searching and listing"
#define module_search_c_authors \
        "JPB | Y | 29/12/1994 | NULL >>>"


/* ---->  From selection.c  <---- */
#define module_selection_c_date "15/06/1994"
#define module_selection_c_desc "Selection & iteration commands"
#define module_selection_c_authors \
        "JPB | Y | 15/06/1994 | NULL >>>"


/* ---->  From server.c  <---- */
#define module_server_c_date "21/12/1993"
#define module_server_c_desc "Handling of Telnet/HTML sockets"
#define module_server_c_authors \
        "BY  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DP  | N | 01/01/1990 | 21/12/1993 >>>" \
        "GDW | N | 01/01/1990 | 21/12/1993 >>>" \
        "JA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "JPB | N | 21/12/1993 | NULL >>>" \
        "MJH | N | 01/01/1990 | 21/12/1993 >>>" \
        "NH  | N | 01/01/1990 | 21/12/1993 >>>" \
        "PC  | N | 01/01/1990 | 21/12/1993 >>>" \
        "RH  | N | 01/01/1990 | 21/12/1993 >>>"


/* ---->  From serverinfo.c  <---- */
#define module_serverinfo_c_date "27/02/2000"
#define module_serverinfo_c_desc "Automatic lookup of server information"
#define module_serverinfo_c_authors \
        "SAB | Y | 27/02/2000 | NULL >>>"


/* ---->  From set.c  <---- */
#define module_set_c_date "21/12/1993"
#define module_set_c_desc "Setting object fields, flags, etc."
#define module_set_c_authors \
        "BY  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DP  | N | 01/01/1990 | 21/12/1993 >>>" \
        "GDW | N | 01/01/1990 | 21/12/1993 >>>" \
        "JA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "JPB | N | 21/12/1993 | NULL >>>" \
        "MJH | N | 01/01/1990 | 21/12/1993 >>>" \
        "NH  | N | 01/01/1990 | 21/12/1993 >>>" \
        "PC  | N | 01/01/1990 | 21/12/1993 >>>" \
        "RH  | N | 01/01/1990 | 21/12/1993 >>>"


/* ---->  From sites.c  <---- */
#define module_sites_c_date "28/12/1994"
#define module_sites_c_desc "Database of registered Internet sites"
#define module_sites_c_authors \
        "JPB | Y | 28/12/1994 | NULL >>>"


/* ---->  From statistics.c  <---- */
#define module_statistics_c_date "29/12/1994"
#define module_statistics_c_desc "Database statistics"
#define module_statistics_c_authors \
        "JPB | Y | 29/12/1994 | NULL >>>"


/* ---->  From stringutils.c  <---- */
#define module_stringutils_c_date "21/12/1993"
#define module_stringutils_c_desc "String matching/formatting"
#define module_stringutils_c_authors \
        "BY  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DP  | N | 01/01/1990 | 21/12/1993 >>>" \
        "GDW | N | 01/01/1990 | 21/12/1993 >>>" \
        "JA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "JPB | N | 21/12/1993 | NULL >>>" \
        "MJH | N | 01/01/1990 | 21/12/1993 >>>" \
        "NH  | N | 01/01/1990 | 21/12/1993 >>>" \
        "PC  | N | 01/01/1990 | 21/12/1993 >>>" \
        "RH  | N | 01/01/1990 | 21/12/1993 >>>"


/* ---->  From substitute.c  <---- */
#define module_substitute_c_date "13/10/1994"
#define module_substitute_c_desc "Command/variable/query/%-type substitutions"
#define module_substitute_c_authors \
        "JPB | Y | 13/10/1994 | NULL >>>"


/* ---->  From tcz.c  <---- */
#define module_tcz_c_date "21/12/1993"
#define module_tcz_c_desc "TCZ server (Start-up/shutdown)"
#define module_tcz_c_authors \
        "BY  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DP  | N | 01/01/1990 | 21/12/1993 >>>" \
        "GDW | N | 01/01/1990 | 21/12/1993 >>>" \
        "JA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "JPB | N | 21/12/1993 | NULL >>>" \
        "MJH | N | 01/01/1990 | 21/12/1993 >>>" \
        "NH  | N | 01/01/1990 | 21/12/1993 >>>" \
        "PC  | N | 01/01/1990 | 21/12/1993 >>>" \
        "RH  | N | 01/01/1990 | 21/12/1993 >>>"


/* ---->  From temp.c  <---- */
#define module_temp_c_date "18/10/1994"
#define module_temp_c_desc "Temporary variables"
#define module_temp_c_authors \
        "JPB | Y | 18/10/1994 | NULL >>>"


/* ---->  From termcap.c  <---- */
#define module_termcap_c_date "26/11/1995"
#define module_termcap_c_desc "Database of terminal definitions"
#define module_termcap_c_authors \
        "JPB | Y | 26/11/1995 | NULL >>>"


/* ---->  From unparse.c  <---- */
#define module_unparse_c_date "21/12/1993"
#define module_unparse_c_desc "Unparsing of object names, etc."
#define module_unparse_c_authors \
        "BY  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "DP  | N | 01/01/1990 | 21/12/1993 >>>" \
        "GDW | N | 01/01/1990 | 21/12/1993 >>>" \
        "JA  | Y | 01/01/1989 | 21/12/1993 >>>" \
        "JPB | N | 21/12/1993 | NULL >>>" \
        "MJH | N | 01/01/1990 | 21/12/1993 >>>" \
        "NH  | N | 01/01/1990 | 21/12/1993 >>>" \
        "PC  | N | 01/01/1990 | 21/12/1993 >>>" \
        "RH  | N | 01/01/1990 | 21/12/1993 >>>"


/* ---->  From userlist.c  <---- */
#define module_userlist_c_date "10/03/1994"
#define module_userlist_c_desc "User lists in various formats"
#define module_userlist_c_authors \
        "JPB | Y | 10/03/1994 | NULL >>>"


/* ---->  From yearlyevents.c  <---- */
#define module_yearlyevents_c_date "24/11/1997"
#define module_yearlyevents_c_desc "Yearly event notification"
#define module_yearlyevents_c_authors \
        "JPB | Y | 24/11/1997 | NULL >>>"
