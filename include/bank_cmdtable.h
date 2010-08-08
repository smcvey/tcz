/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| BANK_CMDTABLE.H  -  Table of bank commands.                                 |
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
| Module originally designed and written by:  J.P.Boggis 03/06/1999           |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: bank_cmdtable.h,v 1.1.1.1 2004/12/02 17:43:11 jpboggis Exp $

*/


/* ---->  Bank command table  <---- */
struct cmd_table bank_cmds[] = {
       {BANK_COMMAND,  "bank",                   (void *) finance_bank,              0, 0, 0},
       {BANK_COMMAND,  "bankroom",               (void *) finance_bank,              0, 0, 0},
       {BANK_COMMAND,  "cashpoint",              (void *) finance_bank,              0, 0, 0},
       {BANK_COMMAND,  "deposit",                (void *) finance_deposit,           0, 0, 0},
       {OTHER_COMMAND, "p",                      (void *) pagetell_send,             0, 0, 0},
       {OTHER_COMMAND, "pa",                     (void *) pagetell_send,             0, 0, 0},
       {BANK_COMMAND,  "pay",                    (void *) finance_transaction,       0, 0, 0},
       {BANK_COMMAND,  "restrict",               (void *) finance_restrict,          0, 0, 0},
       {BANK_COMMAND,  "restriction",            (void *) finance_restrict,          0, 0, 0},
       {BANK_COMMAND,  "statement",              (void *) finance_statement,         0, 0, 0},
       {BANK_COMMAND,  "statistics",             (void *) finance_statement,         0, 0, 0},
       {BANK_COMMAND,  "stats",                  (void *) finance_statement,         0, 0, 0},
       {BANK_COMMAND,  "statement",              (void *) finance_statement,         0, 0, 0},
       {BANK_COMMAND,  "summary",                (void *) finance_statement,         0, 0, 0},
       {OTHER_COMMAND, "t",                      (void *) pagetell_send,             1, 0, 0},
       {BANK_COMMAND,  "transaction",            (void *) finance_transaction,       1, 0, 0},
       {BANK_COMMAND,  "transfer",               (void *) finance_transaction,       1, 0, 0},
       {BANK_COMMAND,  "update",                 (void *) finance_statement,         0, 0, 0},
       {OTHER_COMMAND, "w",                      (void *) userlist_view,             1, 0, 0},
       {BANK_COMMAND,  "withdrawal",             (void *) finance_withdraw,          0, 0, 0},
       {BANK_COMMAND,  NULL,                     NULL,                               0, 0, 0},
};
