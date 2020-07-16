/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| QUERY_CMDTABLE.H  -  Table of '@?' query commands.                          |
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
| code entirely at your OWN RISK and deny the author(s) all responsiblity     |
| for any consequences of its use or misuse.                                  |
|-----------------------[ Credits & Acknowledgements ]------------------------|
| For full details of authors and contributers to TCZ, please see the files   |
| MODULES and CONTRIBUTERS.  For copyright and license information, please    |
| see LICENSE and COPYRIGHT.                                                  |
|                                                                             |
| Module originally designed and written by:  J.P.Boggis 03/06/1999           |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                        https://github.com/smcvey/tcz                        |
`-----------------------------------------------------------------------------'

*/


/* ---->  Query ('@?') command table  <---- */
struct cmd_table query_cmds[] = {
       {QUERY_COMMAND, "@?",                     (void *) look_notice,               11, 0, 0},
       {QUERY_COMMAND, "@?a",                    (void *) query_pronoun,             4, 0, 0},
       {QUERY_COMMAND, "@?abs",                  (void *) query_pronoun,             4, 0, 0},
       {QUERY_COMMAND, "@?absolute",             (void *) query_pronoun,             4, 0, 0},
       {QUERY_COMMAND, "@?account",              (void *) query_credit,              1, 0, 0},
       {QUERY_COMMAND, "@?addr",                 (void *) query_address,             0, 0, 0},
       {QUERY_COMMAND, "@?address",              (void *) query_address,             0, 0, 0},
       {QUERY_COMMAND, "@?alarms",               (void *) query_object,              TYPE_ALARM, 0, 0},
       {QUERY_COMMAND, "@?alias",                (void *) alias_query,               0, 0, 0},
       {QUERY_COMMAND, "@?aliases",              (void *) alias_query,               0, 0, 0},
       {QUERY_COMMAND, "@?alpha",                (void *) query_charclass,           0, 0, 0},
       {QUERY_COMMAND, "@?alphabetical",         (void *) query_charclass,           0, 0, 0},
       {QUERY_COMMAND, "@?alphanumeric",         (void *) query_charclass,           2, 0, 0},
       {QUERY_COMMAND, "@?alnum",                (void *) query_charclass,           2, 0, 0},
       {QUERY_COMMAND, "@?alnumeric",            (void *) query_charclass,           2, 0, 0},
       {QUERY_COMMAND, "@?area",                 (void *) query_area,                0, 0, 0},
       {QUERY_COMMAND, "@?areaname",             (void *) query_areaname,            0, 0, 0},
       {QUERY_COMMAND, "@?areanameid",           (void *) query_areanameid,          0, 0, 0},
       {QUERY_COMMAND, "@?arrays",               (void *) query_object,              TYPE_ARRAY, 0, 0},
       {QUERY_COMMAND, "@?articles",             (void *) query_name,                2, 0, 0},
       {QUERY_COMMAND, "@?average",              (void *) query_averagetime,         0, 0, 0},
       {QUERY_COMMAND, "@?averageactive",        (void *) query_averageactive,       0, 0, 0},
       {QUERY_COMMAND, "@?averageidle",          (void *) query_averageidle,         0, 0, 0},
       {QUERY_COMMAND, "@?averagelogins",        (void *) query_averagelogins,       0, 0, 0},
       {QUERY_COMMAND, "@?averagetime",          (void *) query_averagetime,         0, 0, 0},
       {QUERY_COMMAND, "@?balance",              (void *) query_credit,              1, 0, 0},
       {QUERY_COMMAND, "@?bank",                 (void *) query_specialroom,         2, 0, 0},
       {QUERY_COMMAND, "@?bankroom",             (void *) query_specialroom,         2, 0, 0},
       {QUERY_COMMAND, "@?bbs",                  (void *) query_specialroom,         0, 0, 0},
       {QUERY_COMMAND, "@?bbsrtd",               (void *) bbs_query_readertimediff,  0, 0, 0},
       {QUERY_COMMAND, "@?bbsroom",              (void *) query_specialroom,         0, 0, 0},
       {QUERY_COMMAND, "@?bool",                 (void *) query_boolean,             0, 0, 0},
       {QUERY_COMMAND, "@?boolean",              (void *) query_boolean,             0, 0, 0},
       {QUERY_COMMAND, "@?calc",                 (void *) calculate_evaluate,        0, 0, 0},
       {QUERY_COMMAND, "@?calculate",            (void *) calculate_evaluate,        0, 0, 0},
       {QUERY_COMMAND, "@?capital",              (void *) query_strcase,             2, 0, 0},
       {QUERY_COMMAND, "@?capitalise",           (void *) query_strcase,             2, 0, 0},
       {QUERY_COMMAND, "@?caps",                 (void *) query_strcase,             2, 0, 0},
       {QUERY_COMMAND, "@?cash",                 (void *) query_credit,              0, 0, 0},
       {QUERY_COMMAND, "@?censor",               (void *) query_censor,              0, 0, 0},
       {QUERY_COMMAND, "@?censored",             (void *) query_censor,              0, 0, 0},
       {QUERY_COMMAND, "@?censorship",           (void *) query_censor,              0, 0, 0},
       {QUERY_COMMAND, "@?cfail",                (void *) command_query_csucc_or_cfail, 1, 0, 0},
       {QUERY_COMMAND, "@?cfailure",             (void *) command_query_csucc_or_cfail, 1, 0, 0},
       {QUERY_COMMAND, "@?chars",                (void *) query_object,              TYPE_CHARACTER, 0, 0},
       {QUERY_COMMAND, "@?characters",           (void *) query_object,              TYPE_CHARACTER, 0, 0},
       {QUERY_COMMAND, "@?child",                (void *) match_query,               0, 1, 0},
       {QUERY_COMMAND, "@?childerror",           (void *) match_query,               1, 1, 0},
       {QUERY_COMMAND, "@?cmdname",              (void *) command_query_cmdname,     0, 0, 0},
       {QUERY_COMMAND, "@?colormap",             (void *) map_query_colourmap,       0, 0, 0},
       {QUERY_COMMAND, "@?colourmap",            (void *) map_query_colourmap,       0, 0, 0},
       {QUERY_COMMAND, "@?coms",                 (void *) query_object,              TYPE_COMMAND, 0, 0},
       {QUERY_COMMAND, "@?comms",                (void *) query_object,              TYPE_COMMAND, 0, 0},
       {QUERY_COMMAND, "@?commandname",          (void *) command_query_cmdname,     0, 0, 0},
       {QUERY_COMMAND, "@?commands",             (void *) query_object,              TYPE_COMMAND, 0, 0},
       {QUERY_COMMAND, "@?con",                  (void *) query_connected,           0, 0, 0},
       {QUERY_COMMAND, "@?conn",                 (void *) query_connected,           0, 0, 0},
       {QUERY_COMMAND, "@?connected",            (void *) query_connected,           0, 0, 0},
       {QUERY_COMMAND, "@?cont",                 (void *) query_object,              SEARCH_ALL, 0, 0},
       {QUERY_COMMAND, "@?contents",             (void *) query_object,              SEARCH_ALL, 0, 0},
       {QUERY_COMMAND, "@?control",              (void *) query_controller,          0, 0, 0},
       {QUERY_COMMAND, "@?controller",           (void *) query_controller,          0, 0, 0},
       {QUERY_COMMAND, "@?created",              (void *) query_created_or_lastused, 0, 0, 0},
       {QUERY_COMMAND, "@?creationdate",         (void *) query_created_or_lastused, 0, 0, 0},
       {QUERY_COMMAND, "@?credits",              (void *) query_credit,              0, 0, 0},
       {QUERY_COMMAND, "@?csize",                (void *) query_size,                0, 0, 0},
       {QUERY_COMMAND, "@?cstring",              (void *) query_cestring,            0, 0, 0},
       {QUERY_COMMAND, "@?csucc",                (void *) command_query_csucc_or_cfail, 0, 0, 0},
       {QUERY_COMMAND, "@?csuccess",             (void *) command_query_csucc_or_cfail, 0, 0, 0},
   /*  NOTE:  '@?date_to_string' added to resolve bug in early v4.3.0 code - Should be removed at some point in the future  */
       {QUERY_COMMAND, "@?date_to_string",       (void *) query_realtime,            0, 0, 0},
       {QUERY_COMMAND, "@?datetime",             (void *) query_datetime,            0, 0, 0},
       {QUERY_COMMAND, "@?del",                  (void *) query_delete,              0, 0, 0},
       {QUERY_COMMAND, "@?delay",                (void *) combat_query_delay,        0, 0, 0},
       {QUERY_COMMAND, "@?delete",               (void *) query_delete,              0, 0, 0},
       {QUERY_COMMAND, "@?deleteitem",           (void *) query_delete,              0, 0, 0},
       {QUERY_COMMAND, "@?desc",                 (void *) query_description,         0, 0, 0},
       {QUERY_COMMAND, "@?description",          (void *) query_description,         0, 0, 0},
       {QUERY_COMMAND, "@?dest",                 (void *) query_destination,         0, 0, 0},
       {QUERY_COMMAND, "@?destination",          (void *) query_destination,         0, 0, 0},
       {QUERY_COMMAND, "@?digit",                (void *) query_charclass,           1, 0, 0},
       {QUERY_COMMAND, "@?display",              (void *) group_query_display,       0, 0, 0},
       {QUERY_COMMAND, "@?displayed",            (void *) group_query_display,       0, 0, 0},
       {QUERY_COMMAND, "@?displayeditems",       (void *) group_query_display,       0, 0, 0},
       {QUERY_COMMAND, "@?drop",                 (void *) query_drop,                0, 0, 0},
       {QUERY_COMMAND, "@?element",              (void *) array_query_elementno,     0, 0, 0},
       {QUERY_COMMAND, "@?elementno",            (void *) array_query_elementno,     0, 0, 0},
       {QUERY_COMMAND, "@?elementnumber",        (void *) array_query_elementno,     0, 0, 0},
       {QUERY_COMMAND, "@?elements",             (void *) array_query_noelements,    0, 0, 0},
       {QUERY_COMMAND, "@?email",                (void *) query_email,               0, 0, 0},
       {QUERY_COMMAND, "@?emailaddress",         (void *) query_email,               0, 0, 0},
       {QUERY_COMMAND, "@?enemy",                (void *) query_friend,              1, 0, 0},
       {QUERY_COMMAND, "@?estring",              (void *) query_cestring,            1, 0, 0},
       {QUERY_COMMAND, "@?eval",                 (void *) calculate_evaluate,        0, 0, 0},
       {AT_COMMAND,    "@?evallock",             (void *) query_evallock,            0, 0, 0},
       {QUERY_COMMAND, "@?evaluate",             (void *) calculate_evaluate,        0, 0, 0},
       {AT_COMMAND,    "@?evaluatelock",         (void *) query_evallock,            0, 0, 0},
       {QUERY_COMMAND, "@?execlimit",            (void *) command_query_execution,   1, 0, 0},
       {QUERY_COMMAND, "@?executed",             (void *) command_query_execution,   0, 0, 0},
       {QUERY_COMMAND, "@?execution",            (void *) command_query_execution,   1, 0, 0},
       {QUERY_COMMAND, "@?executionlimit",       (void *) command_query_execution,   1, 0, 0},
       {QUERY_COMMAND, "@?exists",               (void *) query_exists,              0, 0, 0},
       {QUERY_COMMAND, "@?exits",                (void *) query_object,              TYPE_EXIT, 0, 0},
       {QUERY_COMMAND, "@?expenses",             (void *) query_finance,             0, 0, 0},
       {QUERY_COMMAND, "@?expenditure",          (void *) query_finance,             0, 0, 0},
       {QUERY_COMMAND, "@?expiry",               (void *) query_expiry,              0, 0, 0},
       {QUERY_COMMAND, "@?fail",                 (void *) query_fail,                0, 0, 0},
       {QUERY_COMMAND, "@?failure",              (void *) query_fail,                0, 0, 0},
       {QUERY_COMMAND, "@?false",                (void *) query_true_or_false,       1, 0, 0},
       {QUERY_COMMAND, "@?feeling",              (void *) query_feeling,             0, 0, 0},
       {QUERY_COMMAND, "@?filter",               (void *) query_filter,              0, 0, 0},
       {QUERY_COMMAND, "@?filteransicodes",      (void *) query_filter,              0, 0, 0},
       {QUERY_COMMAND, "@?filtersubstitutions",  (void *) query_filter,              0, 0, 0},
       {QUERY_COMMAND, "@?first",                (void *) query_first_name,          0, 0, 0},
       {QUERY_COMMAND, "@?firstname",            (void *) query_first_name,          0, 0, 0},
       {QUERY_COMMAND, "@?flags",                (void *) query_flags,               0, 0, 0},
       {QUERY_COMMAND, "@?format",               (void *) query_format,              0, 0, 0},
       {QUERY_COMMAND, "@?formattext",           (void *) query_format,              0, 0, 0},
       {QUERY_COMMAND, "@?formatnumber",	     (void *) query_format_number,       0, 0, 0},
       {QUERY_COMMAND, "@?friend",               (void *) query_friend,              0, 0, 0},
       {QUERY_COMMAND, "@?fullname",             (void *) query_fullname,            0, 0, 0},
       {QUERY_COMMAND, "@?fuses",                (void *) query_object,              TYPE_FUSE, 0, 0},
       {QUERY_COMMAND, "@?groupitems",           (void *) selection_with,            3, 0, 0},
       {QUERY_COMMAND, "@?groupno",              (void *) group_query_groupno,       0, 0, 0},
       {QUERY_COMMAND, "@?groupnumber",          (void *) group_query_groupno,       0, 0, 0},
       {QUERY_COMMAND, "@?grouprange",           (void *) group_query_grouprange,    0, 0, 0},
       {QUERY_COMMAND, "@?groupsize",            (void *) group_query_groupsize,     0, 0, 0},
       {QUERY_COMMAND, "@?head",                 (void *) query_head,                0, 0, 0},
       {QUERY_COMMAND, "@?health",               (void *) combat_query_health,       0, 0, 0},
       {QUERY_COMMAND, "@?href",                 (void *) html_query_link,           0, 0, 0},
       {QUERY_COMMAND, "@?hreference",           (void *) html_query_link,           0, 0, 0},
       {QUERY_COMMAND, "@?id",                   (void *) query_id,                  0, 0, 0},
       {QUERY_COMMAND, "@?idle",                 (void *) query_idletime,            0, 0, 0},
       {QUERY_COMMAND, "@?idletime",             (void *) query_idletime,            0, 0, 0},
       {QUERY_COMMAND, "@?image",                (void *) html_query_image,          0, 0, 0},
       {QUERY_COMMAND, "@?img",                  (void *) html_query_image,          0, 0, 0},
       {QUERY_COMMAND, "@?internal",             (void *) command_query_internal,    0, 0, 0},
       {QUERY_COMMAND, "@?income",               (void *) query_finance,             1, 0, 0},
       {QUERY_COMMAND, "@?incomings",            (void *) query_finance,             1, 0, 0},
       {QUERY_COMMAND, "@?index",                (void *) array_query_index,         0, 0, 0},
       {QUERY_COMMAND, "@?indexname",            (void *) array_query_index,         0, 0, 0},
       {QUERY_COMMAND, "@?indexno",              (void *) array_query_indexno,       0, 0, 0},
       {QUERY_COMMAND, "@?indexnumber",          (void *) array_query_indexno,       0, 0, 0},
       {QUERY_COMMAND, "@?ins",                  (void *) query_insert,              0, 0, 0},
       {QUERY_COMMAND, "@?insert",               (void *) query_insert,              0, 0, 0},
       {QUERY_COMMAND, "@?int",                  (void *) query_interval,            0, 0, 0},
       {QUERY_COMMAND, "@?interval",             (void *) query_interval,            0, 0, 0},
       {QUERY_COMMAND, "@?isalpha",              (void *) query_charclass,           0, 0, 0},
       {QUERY_COMMAND, "@?isalphabetical",       (void *) query_charclass,           0, 0, 0},
       {QUERY_COMMAND, "@?isalphanumeric",       (void *) query_charclass,           2, 0, 0},
       {QUERY_COMMAND, "@?isalnum",              (void *) query_charclass,           2, 0, 0},
       {QUERY_COMMAND, "@?isalnumeric",          (void *) query_charclass,           2, 0, 0},
       {QUERY_COMMAND, "@?isboolean",            (void *) query_boolean,             0, 0, 0},
       {QUERY_COMMAND, "@?isdigit",              (void *) query_charclass,           1, 0, 0},
       {QUERY_COMMAND, "@?islower",              (void *) query_charclass,           4, 0, 0},
       {QUERY_COMMAND, "@?islowercase",          (void *) query_charclass,           4, 0, 0},
       {QUERY_COMMAND, "@?isnumber",             (void *) query_number,              0, 0, 0},
       {QUERY_COMMAND, "@?ispunct",              (void *) query_charclass,           3, 0, 0},
       {QUERY_COMMAND, "@?ispunctuation",        (void *) query_charclass,           3, 0, 0},
       {QUERY_COMMAND, "@?isupper",              (void *) query_charclass,           5, 0, 0},
       {QUERY_COMMAND, "@?isuppercase",          (void *) query_charclass,           5, 0, 0},
       {QUERY_COMMAND, "@?item",                 (void *) query_item,                0, 0, 0},
       {QUERY_COMMAND, "@?itemexact",            (void *) query_item,                1, 0, 0},
       {QUERY_COMMAND, "@?itemno",               (void *) query_itemno,              0, 0, 0},
       {QUERY_COMMAND, "@?itemnumber",           (void *) query_itemno,              0, 0, 0},
       {QUERY_COMMAND, "@?key",                  (void *) query_key,                 0, 0, 0},
       {QUERY_COMMAND, "@?last",                 (void *) query_lastcommand,         0, 0, 0},
       {QUERY_COMMAND, "@?lastcom",              (void *) query_lastcommand,         0, 0, 0},
       {QUERY_COMMAND, "@?lastcomm",             (void *) query_lastcommand,         0, 0, 0},
       {QUERY_COMMAND, "@?lastcommands",         (void *) query_lastcommand,         0, 0, 0},
       {QUERY_COMMAND, "@?lastcon",              (void *) query_lastconnected,       0, 0, 0},
       {QUERY_COMMAND, "@?lastconn",             (void *) query_lastconnected,       0, 0, 0},
       {QUERY_COMMAND, "@?lastconnected",        (void *) query_lastconnected,       0, 0, 0},
       {QUERY_COMMAND, "@?lastsite",             (void *) query_lastsite,            0, 0, 0},
       {QUERY_COMMAND, "@?lastused",             (void *) query_created_or_lastused, 1, 0, 0},
       {QUERY_COMMAND, "@?lastusagedate",        (void *) query_created_or_lastused, 1, 0, 0},
       {QUERY_COMMAND, "@?latest",               (void *) bbs_query_latest,          0, 0, 0},
       {QUERY_COMMAND, "@?latestmessages",       (void *) bbs_query_latest,          0, 0, 0},
       {QUERY_COMMAND, "@?left",                 (void *) query_leftstr,             0, 0, 0},
       {QUERY_COMMAND, "@?leftstr",              (void *) query_leftstr,             0, 0, 0},
       {QUERY_COMMAND, "@?leftstring",           (void *) query_leftstr,             0, 0, 0},
       {QUERY_COMMAND, "@?len",                  (void *) query_strlen,              0, 0, 0},
       {QUERY_COMMAND, "@?length",               (void *) query_strlen,              0, 0, 0},
       {QUERY_COMMAND, "@?level",                (void *) query_status,              0, 0, 0},
       {QUERY_COMMAND, "@?line",                 (void *) query_line,                0, 0, 0},
       {QUERY_COMMAND, "@?lineno",               (void *) query_line,                0, 0, 0},
       {QUERY_COMMAND, "@?linenumber",           (void *) query_line,                0, 0, 0},
       {QUERY_COMMAND, "@?link",                 (void *) html_query_link,           0, 0, 0},
       {QUERY_COMMAND, "@?loc",                  (void *) query_location,            0, 0, 0},
       {QUERY_COMMAND, "@?location",             (void *) query_location,            0, 0, 0},
       {QUERY_COMMAND, "@?lock",                 (void *) query_lock,                0, 0, 0},
       {QUERY_COMMAND, "@?logins",               (void *) query_totallogins,         0, 0, 0},
       {QUERY_COMMAND, "@?long",                 (void *) query_longesttime,         0, 0, 0},
       {QUERY_COMMAND, "@?longest",              (void *) query_longesttime,         0, 0, 0},
       {QUERY_COMMAND, "@?longestdate",          (void *) query_longestdate,         0, 0, 0},
       {QUERY_COMMAND, "@?longesttime",          (void *) query_longesttime,         0, 0, 0},
       {QUERY_COMMAND, "@?lost",                 (void *) combat_query_statistics,   1, 0, 0},
       {QUERY_COMMAND, "@?lower",                (void *) query_strcase,             1, 0, 0},
       {QUERY_COMMAND, "@?lowercase",            (void *) query_strcase,             1, 0, 0},
       {QUERY_COMMAND, "@?mass",                 (void *) query_mass_or_volume,      0, 0, 0},
       {QUERY_COMMAND, "@?match",                (void *) match_query,               0, 0, 0},
       {QUERY_COMMAND, "@?matcherror",           (void *) match_query,               1, 0, 0},
       {QUERY_COMMAND, "@?messages",             (void *) bbs_query_newmessages,     1, 0, 0},
       {QUERY_COMMAND, "@?mid",                  (void *) query_midstr,              0, 0, 0},
       {QUERY_COMMAND, "@?middlestring",         (void *) query_midstr,              0, 0, 0},
       {QUERY_COMMAND, "@?midstr",               (void *) query_midstr,              0, 0, 0},
       {QUERY_COMMAND, "@?midstring",            (void *) query_midstr,              0, 0, 0},
       {QUERY_COMMAND, "@?mod",                  (void *) query_modify,              0, 0, 0},
       {QUERY_COMMAND, "@?modify",               (void *) query_modify,              0, 0, 0},
       {QUERY_COMMAND, "@?modifyitem",           (void *) query_modify,              0, 0, 0},
       {QUERY_COMMAND, "@?money",                (void *) query_credit,              0, 0, 0},
       {QUERY_COMMAND, "@?my",                   (void *) match_query_my,            0, 0, 0},
       {QUERY_COMMAND, "@?myleaf",               (void *) match_query,               0, 1, 0},
       {QUERY_COMMAND, "@?myleaferror",          (void *) match_query,               1, 1, 0},
       {QUERY_COMMAND, "@?myself",               (void *) match_query_myself,        0, 0, 0},
       {QUERY_COMMAND, "@?name",                 (void *) query_name,                0, 0, 0},
       {QUERY_COMMAND, "@?namec",                (void *) query_namec,               0, 0, 0},
       {QUERY_COMMAND, "@?namecon",              (void *) query_namec,               0, 0, 0},
       {QUERY_COMMAND, "@?nameconn",             (void *) query_namec,               0, 0, 0},
       {QUERY_COMMAND, "@?nameconnected",        (void *) query_namec,               0, 0, 0},
       {QUERY_COMMAND, "@?nested",               (void *) query_nested,              0, 0, 0},
       {QUERY_COMMAND, "@?newline",              (void *) query_newline,             0, 0, 0},
       {QUERY_COMMAND, "@?newmessages",          (void *) bbs_query_newmessages,     0, 0, 0},
       {QUERY_COMMAND, "@?next",                 (void *) query_next,                0, 0, 0},
       {QUERY_COMMAND, "@?noelements",           (void *) array_query_noelements,    0, 0, 0},
       {QUERY_COMMAND, "@?nogroups",             (void *) selection_with,            1, 0, 0},
       {QUERY_COMMAND, "@?noitems",              (void *) query_noitems,             0, 0, 0},
       {QUERY_COMMAND, "@?nowords",              (void *) query_nowords,             0, 0, 0},
       {QUERY_COMMAND, "@?no",                   (void *) query_number,              0, 0, 0},
       {QUERY_COMMAND, "@?number",               (void *) query_number,              0, 0, 0},
       {QUERY_COMMAND, "@?o",                    (void *) query_pronoun,             1, 0, 0},
       {QUERY_COMMAND, "@?obj",                  (void *) query_pronoun,             1, 0, 0},
       {QUERY_COMMAND, "@?object",               (void *) query_object,              TYPE_THING, 0, 0},
       {QUERY_COMMAND, "@?objects",              (void *) query_object,              TYPE_THING, 0, 0},
       {QUERY_COMMAND, "@?objective",            (void *) query_pronoun,             1, 0, 0},
       {QUERY_COMMAND, "@?odesc",                (void *) query_odesc,               0, 0, 0},
       {QUERY_COMMAND, "@?odescription",         (void *) query_odesc,               0, 0, 0},
       {QUERY_COMMAND, "@?odrop",                (void *) query_odrop,               0, 0, 0},
       {QUERY_COMMAND, "@?ofail",                (void *) query_ofail,               0, 0, 0},
       {QUERY_COMMAND, "@?ofailure",             (void *) query_ofail,               0, 0, 0},
       {QUERY_COMMAND, "@?osucc",                (void *) query_osucc,               0, 0, 0},
       {QUERY_COMMAND, "@?osuccess",             (void *) query_osucc,               0, 0, 0},
       {QUERY_COMMAND, "@?owner",                (void *) query_owner,               0, 0, 0},
       {QUERY_COMMAND, "@?ownership",            (void *) query_owner,               0, 0, 0},
       {QUERY_COMMAND, "@?pad",                  (void *) query_pad,                 1, 0, 0},
       {QUERY_COMMAND, "@?padcent",              (void *) query_pad,                 0, 0, 0},
       {QUERY_COMMAND, "@?padcenter",            (void *) query_pad,                 0, 0, 0},
       {QUERY_COMMAND, "@?padcenterjustify",     (void *) query_pad,                 0, 0, 0},
       {QUERY_COMMAND, "@?padcentre",            (void *) query_pad,                 0, 0, 0},
       {QUERY_COMMAND, "@?padcentrejustify",     (void *) query_pad,                 0, 0, 0},
       {QUERY_COMMAND, "@?padleft",              (void *) query_pad,                 1, 0, 0},
       {QUERY_COMMAND, "@?padleftjustify",       (void *) query_pad,                 1, 0, 0},
       {QUERY_COMMAND, "@?padright",             (void *) query_pad,                 2, 0, 0},
       {QUERY_COMMAND, "@?padrightjustify",      (void *) query_pad,                 2, 0, 0},
       {QUERY_COMMAND, "@?par",                  (void *) query_parent,              0, 0, 0},
       {QUERY_COMMAND, "@?parent",               (void *) query_parent,              0, 0, 0},
       {QUERY_COMMAND, "@?partner",              (void *) query_partner,             0, 0, 0},
       {QUERY_COMMAND, "@?peak",                 (void *) query_peak,                0, 0, 0},
       {QUERY_COMMAND, "@?pending",              (void *) query_pending,             0, 0, 0},
       {QUERY_COMMAND, "@?perf",                 (void *) combat_query_statistics,   3, 0, 0},
       {QUERY_COMMAND, "@?perform",              (void *) combat_query_statistics,   3, 0, 0},
       {QUERY_COMMAND, "@?performance",          (void *) combat_query_statistics,   3, 0, 0},
       {QUERY_COMMAND, "@?players",              (void *) query_object,              TYPE_CHARACTER, 0, 0},
       {QUERY_COMMAND, "@?p",                    (void *) query_pronoun,             2, 0, 0},
       {QUERY_COMMAND, "@?pos",                  (void *) query_pronoun,             2, 0, 0},
       {QUERY_COMMAND, "@?poss",                 (void *) query_pronoun,             2, 0, 0},
       {QUERY_COMMAND, "@?possessive",           (void *) query_pronoun,             2, 0, 0},
       {QUERY_COMMAND, "@?prefix",               (void *) query_prefix,              0, 0, 0},
       {QUERY_COMMAND, "@?privs",                (void *) query_privileges,          0, 0, 0},
       {QUERY_COMMAND, "@?privileges",           (void *) query_privileges,          0, 0, 0},
       {QUERY_COMMAND, "@?profile",              (void *) query_profile,             0, 0, 0},
       {QUERY_COMMAND, "@?profit",               (void *) query_finance,             2, 0, 0},
       {QUERY_COMMAND, "@?prompt",               (void *) query_prompt,              0, 0, 0},
       {QUERY_COMMAND, "@?prop",                 (void *) query_object,              TYPE_PROPERTY, 0, 0},
       {QUERY_COMMAND, "@?properties",           (void *) query_object,              TYPE_PROPERTY, 0, 0},
       {QUERY_COMMAND, "@?property",             (void *) query_object,              TYPE_PROPERTY, 0, 0},
       {QUERY_COMMAND, "@?protect",              (void *) query_protect,             0, 0, 0},
       {QUERY_COMMAND, "@?punct",                (void *) query_charclass,           3, 0, 0},
       {QUERY_COMMAND, "@?punctuation",          (void *) query_charclass,           3, 0, 0},
       {QUERY_COMMAND, "@?quarter",              (void *) query_time,                1, 0, 0},
       {QUERY_COMMAND, "@?quota",                (void *) query_quota,               0, 0, 0},
       {QUERY_COMMAND, "@?quotalimit",           (void *) query_quotalimit,          0, 0, 0},
       {QUERY_COMMAND, "@?race",                 (void *) query_race,                0, 0, 0},
       {QUERY_COMMAND, "@?rand",                 (void *) query_rand,                0, 0, 0},
       {QUERY_COMMAND, "@?random",               (void *) query_rand,                0, 0, 0},
       {QUERY_COMMAND, "@?randomise",            (void *) query_rand,                0, 0, 0},
       {QUERY_COMMAND, "@?randomize",            (void *) query_rand,                0, 0, 0},
       {QUERY_COMMAND, "@?range",                (void *) selection_with,            2, 0, 0},
       {QUERY_COMMAND, "@?rangeitems",           (void *) selection_with,            2, 0, 0},
       {QUERY_COMMAND, "@?rangefrom",            (void *) group_query_rangefrom,     0, 0, 0},
       {QUERY_COMMAND, "@?rangeto",              (void *) group_query_rangeto,       0, 0, 0},
       {QUERY_COMMAND, "@?rank",                 (void *) query_rank,                0, 0, 0},
       {QUERY_COMMAND, "@?real",                 (void *) query_name,                1, 0, 0},
       {QUERY_COMMAND, "@?realname",             (void *) query_name,                1, 0, 0},
       {QUERY_COMMAND, "@?realtime",             (void *) query_realtime,            0, 0, 0},
       {QUERY_COMMAND, "@?recursion",            (void *) command_query_execution,   1, 0, 0},
       {QUERY_COMMAND, "@?recursionlimit",       (void *) command_query_execution,   1, 0, 0},
       {QUERY_COMMAND, "@?r",                    (void *) query_pronoun,             3, 0, 0},
       {QUERY_COMMAND, "@?ref",                  (void *) query_pronoun,             3, 0, 0},
       {QUERY_COMMAND, "@?reflexive",            (void *) query_pronoun,             3, 0, 0},
       {QUERY_COMMAND, "@?rep",                  (void *) query_replace,             0, 0, 0},
       {QUERY_COMMAND, "@?replace",              (void *) query_replace,             0, 0, 0},
       {QUERY_COMMAND, "@?result",               (void *) query_result,              0, 0, 0},
       {QUERY_COMMAND, "@?restriction",          (void *) query_credit,              2, 0, 0},
       {QUERY_COMMAND, "@?right",                (void *) query_rightstr,            0, 0, 0},
       {QUERY_COMMAND, "@?rightstr",             (void *) query_rightstr,            0, 0, 0},
       {QUERY_COMMAND, "@?rightstring",          (void *) query_rightstr,            0, 0, 0},
       {QUERY_COMMAND, "@?rooms",                (void *) query_object,              TYPE_ROOM, 0, 0},
       {QUERY_COMMAND, "@?score",                (void *) query_score,               0, 0, 0},
       {QUERY_COMMAND, "@?screenheight",         (void *) query_screenheight,        0, 0, 0},
       {QUERY_COMMAND, "@?screenwidth",          (void *) query_screenwidth,         0, 0, 0},
       {QUERY_COMMAND, "@?scrheight",            (void *) query_screenheight,        0, 0, 0},
       {QUERY_COMMAND, "@?scrwidth",             (void *) query_screenwidth,         0, 0, 0},
       {QUERY_COMMAND, "@?separator",            (void *) query_separator,           0, 0, 0},
       {QUERY_COMMAND, "@?set",                  (void *) query_set,                 0, 0, 0},
       {QUERY_COMMAND, "@?size",                 (void *) query_size,                1, 0, 0},
       {QUERY_COMMAND, "@?sort",                 (void *) query_sort,                0, 0, 0},
       {QUERY_COMMAND, "@?sortalpha",            (void *) query_sort,                0, 0, 0},
       {QUERY_COMMAND, "@?sortalphabetically",   (void *) query_sort,                0, 0, 0},
       {QUERY_COMMAND, "@?sortitems",            (void *) query_sort,                0, 0, 0},
       {QUERY_COMMAND, "@?sortnum",              (void *) query_sort,                1, 0, 0},
       {QUERY_COMMAND, "@?sortnumber",           (void *) query_sort,                1, 0, 0},
       {QUERY_COMMAND, "@?sortnumeric",          (void *) query_sort,                1, 0, 0},
       {QUERY_COMMAND, "@?sortnumerically",      (void *) query_sort,                1, 0, 0},
       {QUERY_COMMAND, "@?status",               (void *) query_status,              0, 0, 0},
       {QUERY_COMMAND, "@?stringlength",         (void *) query_strlen,              0, 0, 0},
       {QUERY_COMMAND, "@?stringposition",       (void *) query_strpos,              0, 0, 0},
       {QUERY_COMMAND, "@?stringprefix",         (void *) query_strprefix,           0, 0, 0},
       {QUERY_COMMAND, "@?strlen",               (void *) query_strlen,              0, 0, 0},
       {QUERY_COMMAND, "@?strlength",            (void *) query_strlen,              0, 0, 0},
       {QUERY_COMMAND, "@?strpos",               (void *) query_strpos,              0, 0, 0},
       {QUERY_COMMAND, "@?strposition",          (void *) query_strpos,              0, 0, 0},
       {QUERY_COMMAND, "@?strprefix",            (void *) query_strprefix,           0, 0, 0},
       {QUERY_COMMAND, "@?s",                    (void *) query_pronoun,             0, 0, 0},
       {QUERY_COMMAND, "@?sub",                  (void *) query_pronoun,             0, 0, 0},
       {QUERY_COMMAND, "@?subj",                 (void *) query_pronoun,             0, 0, 0},
       {QUERY_COMMAND, "@?subject",              (void *) query_pronoun,             0, 0, 0},
       {QUERY_COMMAND, "@?subjective",           (void *) query_pronoun,             0, 0, 0},
       {QUERY_COMMAND, "@?succ",                 (void *) query_succ,                0, 0, 0},
       {QUERY_COMMAND, "@?success",              (void *) query_succ,                0, 0, 0},
       {QUERY_COMMAND, "@?suffix",               (void *) query_suffix,              0, 0, 0},
       {QUERY_COMMAND, "@?tail",                 (void *) query_tail,                0, 0, 0},
       {QUERY_COMMAND, "@?tczlink",              (void *) html_query_tczlink,        0, 0, 0},
       {QUERY_COMMAND, "@?terminaltype",         (void *) query_terminaltype,        0, 0, 0},
       {QUERY_COMMAND, "@?termtype",             (void *) query_terminaltype,        0, 0, 0},
       {QUERY_COMMAND, "@?things",               (void *) query_object,              TYPE_THING, 0, 0},
       {QUERY_COMMAND, "@?time",                 (void *) query_time,                0, 0, 0},
       {QUERY_COMMAND, "@?timediff",             (void *) query_timediff,            0, 0, 0},
       {QUERY_COMMAND, "@?timedifference",       (void *) query_timediff,            0, 0, 0},
       {QUERY_COMMAND, "@?title",                (void *) userlist_query_title,      0, 0, 0},
       {QUERY_COMMAND, "@?tolower",              (void *) query_strcase,             1, 0, 0},
       {QUERY_COMMAND, "@?tolowercase",          (void *) query_strcase,             1, 0, 0},
       {QUERY_COMMAND, "@?total",                (void *) combat_query_statistics,   2, 0, 0},
       {QUERY_COMMAND, "@?totalactive",          (void *) query_totalactive,         0, 0, 0},
       {QUERY_COMMAND, "@?totalidle",            (void *) query_totalidle,           0, 0, 0},
       {QUERY_COMMAND, "@?totalitems",           (void *) selection_with,            4, 0, 0},
       {QUERY_COMMAND, "@?totallogins",          (void *) query_totallogins,         0, 0, 0},
       {QUERY_COMMAND, "@?totaltime",            (void *) query_totaltime,           0, 0, 0},
       {QUERY_COMMAND, "@?toupper",              (void *) query_strcase,             0, 0, 0},
       {QUERY_COMMAND, "@?touppercase",          (void *) query_strcase,             0, 0, 0},
       {QUERY_COMMAND, "@?true",                 (void *) query_true_or_false,       0, 0, 0},
       {QUERY_COMMAND, "@?type",                 (void *) query_typeof,              0, 0, 0},
       {QUERY_COMMAND, "@?typeof",               (void *) query_typeof,              0, 0, 0},
       {QUERY_COMMAND, "@?upper",                (void *) query_strcase,             0, 0, 0},
       {QUERY_COMMAND, "@?uppercase",            (void *) query_strcase,             0, 0, 0},
       {QUERY_COMMAND, "@?uptime",               (void *) query_uptime,              0, 0, 0},
       {QUERY_COMMAND, "@?uid",                  (void *) command_query_uid,         0, 0, 0},
       {QUERY_COMMAND, "@?unreadmessages",       (void *) bbs_query_newmessages,     0, 0, 0},
       {QUERY_COMMAND, "@?v",                    (void *) query_pronoun,             3, 0, 0},
       {QUERY_COMMAND, "@?variables",            (void *) query_object,              TYPE_VARIABLE, 0, 0},
       {QUERY_COMMAND, "@?vars",                 (void *) query_object,              TYPE_VARIABLE, 0, 0},
       {QUERY_COMMAND, "@?version",              (void *) query_version,             1, 0, 0},
       {QUERY_COMMAND, "@?vol",                  (void *) query_mass_or_volume,      1, 0, 0},
       {QUERY_COMMAND, "@?volume",               (void *) query_mass_or_volume,      1, 0, 0},
       {QUERY_COMMAND, "@?weight",               (void *) query_weight,              0, 0, 0},
       {QUERY_COMMAND, "@?wild",                 (void *) query_wildcard,            0, 0, 0},
       {QUERY_COMMAND, "@?wildcard",             (void *) query_wildcard,            0, 0, 0},
       {QUERY_COMMAND, "@?who",                  (void *) userlist_query_title,      0, 0, 0},
       {QUERY_COMMAND, "@?whomessage",           (void *) userlist_query_title,      0, 0, 0},
       {QUERY_COMMAND, "@?whostring",            (void *) userlist_query_title,      0, 0, 0},
       {QUERY_COMMAND, "@?won",                  (void *) combat_query_statistics,   0, 0, 0},
       {QUERY_COMMAND, "@?word",                 (void *) query_word,                0, 0, 0},
       {QUERY_COMMAND, "@?wordexact",            (void *) query_word,                1, 0, 0},
       {QUERY_COMMAND, "@?wordno",               (void *) query_wordno,              0, 0, 0},
       {QUERY_COMMAND, "@?wordnumber",           (void *) query_wordno,              0, 0, 0},
       {QUERY_COMMAND, "@?wordwrap",             (void *) query_screenwidth,         0, 0, 0},
       {QUERY_COMMAND, "@?wrap",                 (void *) query_screenwidth,         0, 0, 0},
       {QUERY_COMMAND, "@?www",                  (void *) query_www,                 0, 0, 0},
       {QUERY_COMMAND, "@?wwwaddress",           (void *) query_www,                 0, 0, 0},
       {QUERY_COMMAND, "@?wwwhomepage",          (void *) query_www,                 0, 0, 0},
       {QUERY_COMMAND, NULL,                     NULL,                               0, 0, 0},
};
