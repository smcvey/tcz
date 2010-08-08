/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| MODULES_H.H  -  Module author information definitions for '*.h' header      |
|                 files.                                                      |
|                                                                             |
|                 Format of author information:                               |
|                 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~                               |
|                  Initials:  Author: | Date From:   Date To:                 |
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

  $Id: modules_h.h,v 1.3 2004/12/15 20:41:50 jpboggis Exp $

*/


/* ---->  From at_cmdtable.h  <---- */
#define module_at_cmdtable_h_date "03/06/1999"
#define module_at_cmdtable_h_desc "'@' command table"
#define module_at_cmdtable_h_authors \
        "JPB | Y | 03/06/1999 | NULL >>>"


/* ---->  From bank_cmdtable.h  <---- */
#define module_bank_cmdtable_h_date "03/06/1999"
#define module_bank_cmdtable_h_desc "Bank command table"
#define module_bank_cmdtable_h_authors \
        "JPB | Y | 03/06/1999 | NULL >>>"


/* ---->  From bbs_cmdtable.h  <---- */
#define module_bbs_cmdtable_h_date "03/06/1999"
#define module_bbs_cmdtable_h_desc "BBS command table"
#define module_bbs_cmdtable_h_authors \
        "JPB | Y | 03/06/1999 | NULL >>>"


/* ---->  From boolean_flags.h  <---- */
#define module_boolean_flags_h_date "04/12/2003"
#define module_boolean_flags_h_desc "Boolean (Object lock) flags."
#define module_boolean_flags_h_authors \
        "JPB | Y | 04/12/2003 | NULL >>>"


/* ---->  From colourmap.h  <---- */
#define module_colourmap_h_date "04/02/2000"
#define module_colourmap_h_desc "Colour map table"
#define module_colourmap_h_authors \
        "JPB | Y | 04/02/2000 | NULL >>>"


/* ---->  From command.h  <---- */
#define module_command_h_date "21/12/1993"
#define module_command_h_desc "Compound command specific definitions"
#define module_command_h_authors \
        "DP  | Y | 01/01/1990 | 21/12/1993 >>>" \
        "GDW | Y | 01/01/1990 | 21/12/1993 >>>" \
        "JPB | N | 21/12/1993 | NULL >>>" \
        "MJH | Y | 01/01/1990 | 21/12/1993 >>>" \
        "NH  | Y | 01/01/1990 | 21/12/1993 >>>" \
        "PC  | Y | 01/01/1990 | 21/12/1993 >>>" \
        "RH  | Y | 01/01/1990 | 21/12/1993 >>>"


/* ---->  From config.h  <---- */
#define module_config_h_date "21/12/1993"
#define module_config_h_desc "Configuration and parameters"
#define module_config_h_authors \
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


/* ---->  From db.h  <---- */
#define module_db_h_date "21/12/1993"
#define module_db_h_desc "Global, database and flag definitions"
#define module_db_h_authors \
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


/* ---->  From descriptor_flags.h  <---- */
#define module_descriptor_flags_h_date "04/12/2003"
#define module_descriptor_flags_h_desc "Flags used by descriptor of character's connection (Telnet/HTML.)"
#define module_descriptor_flags_h_authors \
        "JPB | Y | 04/12/2003 | NULL >>>"
	
	
/* ---->  From edit_cmdtable.h  <---- */
#define module_edit_cmdtable_h_date "09/10/1994"
#define module_edit_cmdtable_h_desc "Editor command table"
#define module_edit_cmdtable_h_authors \
        "JPB | Y | 09/10/1994 | NULL >>>"


/* ---->  From emailforward.h  <---- */
#define module_emailforward_h_date "03/06/1999"
#define module_emailforward_h_desc "Invalid E-mail forwarding names"
#define module_emailforward_h_authors \
        "JPB | Y | 03/06/1999 | NULL >>>"


/* ---->  From externs.h  <---- */
#define module_externs_h_date "21/12/1993"
#define module_externs_h_desc "External declarations"
#define module_externs_h_authors \
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


/* ---->  From fields.h  <---- */
#define module_fields_h_date "03/06/1999"
#define module_fields_h_desc "Definitions of fields each object has"
#define module_fields_h_authors \
        "JPB | Y | 03/06/1999 | NULL >>>"


/* ---->  From flagset.h  <---- */
#define module_flagset_h_date "03/06/1999"
#define module_flagset_h_desc "Table of which flags can be set on which objects"
#define module_flagset_h_authors \
        "JPB | Y | 03/06/1999 | NULL >>>"


/* ---->  From friend_flags.h  <---- */
#define module_friend_flags_h_date "20/05/2000"
#define module_friend_flags_h_desc "Table of friend flags"
#define module_friend_flags_h_authors \
        "JPB | Y | 20/05/2000 | NULL >>>"


/* ---->  From general_cmdtable.h  <---- */
#define module_general_cmdtable_h_date "03/06/1999"
#define module_general_cmdtable_h_desc "General command table"
#define module_general_cmdtable_h_authors \
        "JPB | Y | 03/06/1999 | NULL >>>"


/* ---->  From html.h  <---- */
#define module_html_h_date "09/12/2003"
#define module_html_h_desc "HTML Interface definitions and data structures"
#define module_html_h_authors \
        "JPB | Y | 09/12/2003 | NULL >>>"


/* ---->  From html_entities.h  <---- */
#define module_html_entities_h_date "21/06/1996"
#define module_html_entities_h_desc "Table of HTML entities recognised by the HTML Interface"
#define module_html_entities_h_authors \
        "JPB | Y | 21/06/1996 | NULL >>>"


/* ---->  From html_tags.h  <---- */
#define module_html_tags_h_date "21/06/1996"
#define module_html_tags_h_desc "Table of HTML tags recognised by the HTML Interface"
#define module_html_tags_h_authors \
        "JPB | Y | 21/06/1996 | NULL >>>"


/* ---->  From logfile_table.h  <---- */
#define module_logfile_table_h_date "11/03/2000"
#define module_logfile_table_h_desc "Table of system log files"
#define module_logfile_table_h_authors \
        "JPB | Y | 11/03/2000 | NULL >>>"


/* ---->  From logfiles.h  <---- */
#define module_logfiles_h_date "03/06/1999"
#define module_logfiles_h_desc "Constants for TCZ system log files"
#define module_logfiles_h_authors \
        "JPB | Y | 03/06/1999 | NULL >>>"


/* ---->  From match.h  <---- */
#define module_match_h_date "03/06/1999"
#define module_match_h_desc "Definitions for hierarchical object matching"
#define module_match_h_authors \
        "JPB | Y | 03/06/1999 | NULL >>>"


/* ---->  From modules.h  <---- */
#define module_modules_h_date "21/05/2000"
#define module_modules_h_desc "Table of source code modules and pointers to author information"
#define module_modules_h_authors \
        "JPB | Y | 21/05/2000 | NULL >>>"


/* ---->  From modules_authors.h  <---- */
#define module_modules_authors_h_date "21/05/2000"
#define module_modules_authors_h_desc "Table of source code module authors"
#define module_modules_authors_h_authors \
        "JPB | Y | 21/05/2000 | NULL >>>"


/* ---->  From modules_c.h  <---- */
#define module_modules_c_h_date "21/05/2000"
#define module_modules_c_h_desc "Module author information definitions for '*.c' source files"
#define module_modules_c_h_authors \
        "JPB | Y | 21/05/2000 | NULL >>>"


/* ---->  From modules_h.h  <---- */
#define module_modules_h_h_date "21/05/2000"
#define module_modules_h_h_desc "Table of source code modules and pointers to author information"
#define module_modules_h_h_authors \
        "JPB | Y | 21/05/2000 | NULL >>>"


/* ---->  From object_types.h  <---- */
#define module_object_types_h_date "04/12/2003"
#define module_object_types_h_desc "Definitions of object type flags and related macros."
#define module_object_types_h_authors \
        "JPB | Y | 04/12/2003 | NULL >>>"


/* ---->  From objectlists.h  <---- */
#define module_objectlists_h_date "03/06/1999"
#define module_objectlists_h_desc "Definitions of lists each object has, and which objects can be moved into them"
#define module_objectlists_h_authors \
        "JPB | Y | 03/06/1999 | NULL >>>"


/* ---->  From primary_flags.h  <---- */
#define module_primary_flags_h_date "15/03/2000"
#define module_primary_flags_h_desc "Definitions of primary object/character flags"
#define module_primary_flags_h_authors \
        "JPB | Y | 15/03/2000 | NULL >>>"


/* ---->  From prompts.h  <---- */
#define module_prompts_h_date "18/06/2001"
#define module_prompts_h_desc "Standard prompts displayed to users"
#define module_prompts_h_authors \
        "JPB | Y | 18/06/2001 | NULL >>>"


/* ---->  From query_cmdtable.c  <---- */
#define module_query_cmdtable_h_date "03/06/1999"
#define module_query_cmdtable_h_desc "'@?' query command table"
#define module_query_cmdtable_h_authors \
        "JPB | Y | 03/06/1999 | NULL >>>"


/* ---->  From quota.h  <---- */
#define module_quota_h_date "03/06/1999"
#define module_quota_h_desc "Cost in Building Quota per object"
#define module_quota_h_authors \
        "JPB | Y | 03/06/1999 | NULL >>>"


/* ---->  From search.h  <---- */
#define module_search_h_date "03/06/1999"
#define module_search_h_desc "Definitions for object searching/listing commands"
#define module_search_h_authors \
        "JPB | Y | 03/06/1999 | NULL >>>"


/* ---->  From secondary_flags.h  <---- */
#define module_secondary_flags_h_date "15/03/2000"
#define module_secondary_flags_h_desc "Definitions of secondary object/character flags"
#define module_secondary_flags_h_authors \
        "JPB | Y | 15/03/2000 | NULL >>>"


/* ---->  From short_cmdtable.h  <---- */
#define module_short_cmdtable_h_date "03/06/1999"
#define module_short_cmdtable_h_desc "Short command table (\" for 'say', etc.)"
#define module_short_cmdtable_h_authors \
        "JPB | Y | 03/06/1999 | NULL >>>"


/* ---->  From signallist.h  <---- */
#define module_signallist_h_date "26/08/2000"
#define module_signallist_h_desc "List of signal names/descriptions"
#define module_signallist_h_authors \
        "JPB | Y | 26/08/2000 | NULL >>>"


/* ---->  From smileys.h  <---- */
#define module_smileys_h_date "03/03/2000"
#define module_smileys_h_desc "Table of graphical smileys for use in HTML Interface"
#define module_smileys_h_authors \
        "JPB | Y | 03/03/2000 | NULL >>>"


/* ---->  From structures.h  <---- */
#define module_structures_h_date "30/07/2000"
#define module_structures_h_desc "Data structures used throughout TCZ"
#define module_structures_h_authors \
        "JPB | Y | 30/07/2000 | NULL >>>"


/* ---->  From teleport.h  <---- */
#define module_teleport_h_date "03/06/1999"
#define module_teleport_h_desc "Object teleportation restrictions"
#define module_teleport_h_authors \
        "JPB | Y | 03/06/1999 | NULL >>>"


/* ---->  From yearlyevents.h  <---- */
#define module_yearlyevents_h_date "03/06/2000"
#define module_yearlyevents_h_desc "Table of yearly events"
#define module_yearlyevents_h_authors \
        "JPB | Y | 03/06/2000 | NULL >>>"
