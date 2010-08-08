/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| EDIT_CMDTABLE.H  -  Table of editor commands.                               |
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
| Module originally designed and written by:  J.P.Boggis 09/10/1994.          |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: edit_cmdtable.h,v 1.1.1.1 2004/12/02 17:43:18 jpboggis Exp $

*/


/* ---->  Editor command table  <---- */
struct edit_cmd_table edit_cmds[] = {
       {EDIT_COMMAND, "-",                      (void *) edit_up_or_down,            0},
       {EDIT_COMMAND, "+",                      (void *) edit_up_or_down,            1},
       {EDIT_COMMAND, "a",                      (void *) edit_append,                0},
       {EDIT_COMMAND, "abandon",                (void *) edit_abort_or_save,         0},
       {EDIT_COMMAND, "abandoneditor",          (void *) edit_abort_or_save,         0},
       {EDIT_COMMAND, "abort",                  (void *) edit_abort_or_save,         0},
       {EDIT_COMMAND, "aborteditor",            (void *) edit_abort_or_save,         0},
       {EDIT_COMMAND, "append",                 (void *) edit_append,                0},
       {EDIT_COMMAND, "appendline",             (void *) edit_append,                0},
       {EDIT_COMMAND, "appendtext",             (void *) edit_append,                0},
       {EDIT_COMMAND, "b",                      (void *) edit_top_or_bottom,         1},
       {EDIT_COMMAND, "bottom",                 (void *) edit_top_or_bottom,         1},
       {EDIT_COMMAND, "c",                      (void *) edit_copy,                  0},
       {EDIT_COMMAND, "cancel",                 (void *) edit_abort_or_save,         0},
       {EDIT_COMMAND, "canceleditor",           (void *) edit_abort_or_save,         0},
       {EDIT_COMMAND, "copy",                   (void *) edit_copy,                  0},
       {EDIT_COMMAND, "copyline",               (void *) edit_copy,                  0},
       {EDIT_COMMAND, "copylines",              (void *) edit_copy,                  0},
       {EDIT_COMMAND, "copytext",               (void *) edit_copy,                  0},
       {EDIT_COMMAND, "d",                      (void *) edit_up_or_down,            1},
       {EDIT_COMMAND, "del",                    (void *) edit_delete,                0},
       {EDIT_COMMAND, "delete",                 (void *) edit_delete,                0},
       {EDIT_COMMAND, "deleteline",             (void *) edit_delete,                0},
       {EDIT_COMMAND, "deletelines",            (void *) edit_delete,                0},
       {EDIT_COMMAND, "deletetext",             (void *) edit_delete,                0},
       {EDIT_COMMAND, "delline",                (void *) edit_delete,                0},
       {EDIT_COMMAND, "dellines",               (void *) edit_delete,                0},
       {EDIT_COMMAND, "deltext",                (void *) edit_delete,                0},
       {EDIT_COMMAND, "display",                (void *) edit_view,                  0},
       {EDIT_COMMAND, "displayline",            (void *) edit_view,                  0},
       {EDIT_COMMAND, "displaylines",           (void *) edit_view,                  0},
       {EDIT_COMMAND, "displaytext",            (void *) edit_view,                  0},
       {EDIT_COMMAND, "down",                   (void *) edit_up_or_down,            1},
       {EDIT_COMMAND, "e",                      (void *) edit_top_or_bottom,         1},
       {EDIT_COMMAND, "editfail",               (void *) edit_succ_or_fail,          1},
       {EDIT_COMMAND, "editfailure",            (void *) edit_succ_or_fail,          1},
       {EDIT_COMMAND, "editsucc",               (void *) edit_succ_or_fail,          0},
       {EDIT_COMMAND, "editsuccess",            (void *) edit_succ_or_fail,          0},
       {EDIT_COMMAND, "element",                (void *) edit_field,                 0},
       {EDIT_COMMAND, "elementno",              (void *) edit_field,                 0},
       {EDIT_COMMAND, "elementnumber",          (void *) edit_field,                 0},
       {EDIT_COMMAND, "end",                    (void *) edit_top_or_bottom,         1},
       {EDIT_COMMAND, "erase",                  (void *) edit_delete,                0},
       {EDIT_COMMAND, "eraseline",              (void *) edit_delete,                0},
       {EDIT_COMMAND, "eraselines",             (void *) edit_delete,                0},
       {EDIT_COMMAND, "erasetext",              (void *) edit_delete,                0},
       {EDIT_COMMAND, "eval",                   (void *) edit_evaluate,              0},
       {EDIT_COMMAND, "evaluate",               (void *) edit_evaluate,              0},
       {EDIT_COMMAND, "exec",                   (void *) edit_execute,               0},
       {EDIT_COMMAND, "execute",                (void *) edit_execute,               0},
       {EDIT_COMMAND, "executecommand",         (void *) edit_execute,               0},
       {EDIT_COMMAND, "exit",                   (void *) edit_abort_or_save,         1},
       {EDIT_COMMAND, "exiteditor",             (void *) edit_abort_or_save,         1},
       {EDIT_COMMAND, "f",                      (void *) edit_field,                 0},
       {EDIT_COMMAND, "fail",                   (void *) edit_succ_or_fail,          1},
       {EDIT_COMMAND, "failure",                (void *) edit_succ_or_fail,          1},
       {EDIT_COMMAND, "field",                  (void *) edit_field,                 0},
       {EDIT_COMMAND, "first",                  (void *) edit_top_or_bottom,         0},
       {EDIT_COMMAND, "firstline",              (void *) edit_top_or_bottom,         0},
       {EDIT_COMMAND, "g",                      (void *) edit_position,              0},
       {EDIT_COMMAND, "go",                     (void *) edit_position,              0},
       {EDIT_COMMAND, "goto",                   (void *) edit_position,              0},
       {EDIT_COMMAND, "gotoline",               (void *) edit_position,              0},
       {EDIT_COMMAND, "gotolineno",             (void *) edit_position,              0},
       {EDIT_COMMAND, "gotolinenumber",         (void *) edit_position,              0},
       {EDIT_COMMAND, "h",                      (void *) edit_top_or_bottom,         0},
       {EDIT_COMMAND, "help",                   (void *) edit_help,                  0},
       {EDIT_COMMAND, "home",                   (void *) edit_top_or_bottom,         0},
       {EDIT_COMMAND, "i",                      (void *) edit_insert,                0},
       {EDIT_COMMAND, "ins",                    (void *) edit_insert,                0},
       {EDIT_COMMAND, "insert",                 (void *) edit_insert,                0},
       {EDIT_COMMAND, "insertline",             (void *) edit_insert,                0},
       {EDIT_COMMAND, "insertlines",            (void *) edit_insert,                0},
       {EDIT_COMMAND, "inserttext",             (void *) edit_insert,                0},
       {EDIT_COMMAND, "l",                      (void *) edit_position,              0},
       {EDIT_COMMAND, "last",                   (void *) edit_top_or_bottom,         2},
       {EDIT_COMMAND, "lastline",               (void *) edit_top_or_bottom,         2},
       {EDIT_COMMAND, "leave",                  (void *) edit_abort_or_save,         1},
       {EDIT_COMMAND, "leaveeditor",            (void *) edit_abort_or_save,         1},
       {EDIT_COMMAND, "line",                   (void *) edit_position,              0},
       {EDIT_COMMAND, "lineno",                 (void *) edit_position,              0},
       {EDIT_COMMAND, "linenumber",             (void *) edit_position,              0},
       {EDIT_COMMAND, "linenumbering",          (void *) edit_numbering,             0},
       {EDIT_COMMAND, "linenumbers",            (void *) edit_numbering,             0},
       {EDIT_COMMAND, "list",                   (void *) edit_view,                  0},
       {EDIT_COMMAND, "listline",               (void *) edit_view,                  0},
       {EDIT_COMMAND, "listlines",              (void *) edit_view,                  0},
       {EDIT_COMMAND, "listtext",               (void *) edit_view,                  0},
       {EDIT_COMMAND, "m",                      (void *) edit_move,                  0},
       {EDIT_COMMAND, "man",                    (void *) edit_help,                  0},
       {EDIT_COMMAND, "manual",                 (void *) edit_help,                  0},
       {EDIT_COMMAND, "move",                   (void *) edit_move,                  0},
       {EDIT_COMMAND, "moveline",               (void *) edit_move,                  0},
       {EDIT_COMMAND, "movelines",              (void *) edit_move,                  0},
       {EDIT_COMMAND, "movetext",               (void *) edit_move,                  0},
       {EDIT_COMMAND, "n",                      (void *) edit_up_or_down,            1},
       {EDIT_COMMAND, "next",                   (void *) edit_up_or_down,            1},
       {EDIT_COMMAND, "nextline",               (void *) edit_up_or_down,            1},
       {EDIT_COMMAND, "numbers",                (void *) edit_numbering,             0},
       {EDIT_COMMAND, "numbering",              (void *) edit_numbering,             0},
       {EDIT_COMMAND, "overwrite",              (void *) edit_overwrite,             0},
       {EDIT_COMMAND, "overwriting",            (void *) edit_overwrite,             0},
       {EDIT_COMMAND, "p",                      (void *) edit_position,              0},
       {EDIT_COMMAND, "permanent",              (void *) edit_permanent,             0},
       {EDIT_COMMAND, "pos",                    (void *) edit_position,              0},
       {EDIT_COMMAND, "position",               (void *) edit_position,              0},
       {EDIT_COMMAND, "prev",                   (void *) edit_up_or_down,            0},
       {EDIT_COMMAND, "previous",               (void *) edit_up_or_down,            0},
       {EDIT_COMMAND, "previousline",           (void *) edit_up_or_down,            0},
       {EDIT_COMMAND, "q",                      (void *) edit_abort_or_save,         1},
       {EDIT_COMMAND, "quit",                   (void *) edit_abort_or_save,         1},
       {EDIT_COMMAND, "quiteditor",             (void *) edit_abort_or_save,         1},
       {EDIT_COMMAND, "r",                      (void *) edit_replace,               0},
       {EDIT_COMMAND, "rem",                    (void *) edit_delete,                0},
       {EDIT_COMMAND, "remline",                (void *) edit_delete,                0},
       {EDIT_COMMAND, "remlines",               (void *) edit_delete,                0},
       {EDIT_COMMAND, "remtext",                (void *) edit_delete,                0},
       {EDIT_COMMAND, "remove",                 (void *) edit_delete,                0},
       {EDIT_COMMAND, "removeline",             (void *) edit_delete,                0},
       {EDIT_COMMAND, "removelines",            (void *) edit_delete,                0},
       {EDIT_COMMAND, "removetext",             (void *) edit_delete,                0},
       {EDIT_COMMAND, "rep",                    (void *) edit_replace,               0},
       {EDIT_COMMAND, "replace",                (void *) edit_replace,               0},
       {EDIT_COMMAND, "replaceline",            (void *) edit_replace,               0},
       {EDIT_COMMAND, "replacelines",           (void *) edit_replace,               0},
       {EDIT_COMMAND, "replacetext",            (void *) edit_replace,               0},
       {EDIT_COMMAND, "repline",                (void *) edit_replace,               0},
       {EDIT_COMMAND, "replines",               (void *) edit_replace,               0},
       {EDIT_COMMAND, "s",                      (void *) edit_set,                   0},
       {EDIT_COMMAND, "save",                   (void *) edit_abort_or_save,         1},
       {EDIT_COMMAND, "savetext",               (void *) edit_abort_or_save,         1},
       {EDIT_COMMAND, "saveeditor",             (void *) edit_abort_or_save,         1},
       {EDIT_COMMAND, "set",                    (void *) edit_set,                   0},
       {EDIT_COMMAND, "setline",                (void *) edit_set,                   0},
       {EDIT_COMMAND, "setlines",               (void *) edit_set,                   0},
       {EDIT_COMMAND, "settext",                (void *) edit_set,                   0},
       {EDIT_COMMAND, "succ",                   (void *) edit_succ_or_fail,          0},
       {EDIT_COMMAND, "success",                (void *) edit_succ_or_fail,          0},
       {EDIT_COMMAND, "t",                      (void *) edit_top_or_bottom,         0},
       {EDIT_COMMAND, "top",                    (void *) edit_top_or_bottom,         0},
       {EDIT_COMMAND, "tutor",                  (void *) edit_help,                  1},
       {EDIT_COMMAND, "tutorial",               (void *) edit_help,                  1},
       {EDIT_COMMAND, "u",                      (void *) edit_up_or_down,            0},
       {EDIT_COMMAND, "up",                     (void *) edit_up_or_down,            0},
       {EDIT_COMMAND, "v",                      (void *) edit_view,                  0},
       {EDIT_COMMAND, "value",                  (void *) edit_value,                 0},
       {EDIT_COMMAND, "view",                   (void *) edit_view,                  0},
       {EDIT_COMMAND, "viewer",                 (void *) edit_view,                  0},
       {EDIT_COMMAND, "viewline",               (void *) edit_view,                  0},
       {EDIT_COMMAND, "viewlines",              (void *) edit_view,                  0},
       {EDIT_COMMAND, "viewtext",               (void *) edit_view,                  0},
       {EDIT_COMMAND, "x",                      (void *) edit_execute,               0},
       {EDIT_COMMAND, NULL,                     NULL,                                0},
};
