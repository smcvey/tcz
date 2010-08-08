/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| BOOLEAN_FLAGS.H  -  Boolean (Object lock) flags.                            |
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

  $Id: boolean_flags.h,v 1.1.1.1 2004/12/02 17:43:12 jpboggis Exp $

*/


/* ---->  Lock boolean operators  <---- */
#define BOOLEXP_AND                 0
#define BOOLEXP_OR                  1
#define BOOLEXP_NOT                 2
#define BOOLEXP_CONST               3
#define BOOLEXP_FLAG                4


/* ---->  Gender boolean flags  <---- */
#define BOOLFLAG_MALE               4
#define BOOLFLAG_FEMALE             5
#define BOOLFLAG_NEUTER             3
#define BOOLFLAG_UNSET              35


/* ---->  Privilege boolean flags  <---- */
#define BOOLFLAG_MORON              15
#define BOOLFLAG_BEING              6
#define BOOLFLAG_BUILDER            18
#define BOOLFLAG_EXPERIENCED        20
#define BOOLFLAG_DRUID              2
#define BOOLFLAG_APPRENTICE         13
#define BOOLFLAG_WIZARD             1
#define BOOLFLAG_ELDER              12
#define BOOLFLAG_DEITY              25


/* ---->  Miscellaneous boolean flags  <---- */
/* NOTE:  Highest value is presently 55 (BOOLFLAG_CHAT_PRIVATE)  */
#define BOOLFLAG_ANSI               16
#define BOOLFLAG_ASHCAN             17
#define BOOLFLAG_ASSISTANT          48
#define BOOLFLAG_BBS                46
#define BOOLFLAG_BBS_INFORM         51
#define BOOLFLAG_CENSOR             41
#define BOOLFLAG_CHAT_PRIVATE       55
#define BOOLFLAG_CHAT_OPERATOR      33
#define BOOLFLAG_CONNECTED          34
#define BOOLFLAG_ECHO               24
#define BOOLFLAG_EDIT_EVALUATE      30
#define BOOLFLAG_EDIT_NUMBERING     19
#define BOOLFLAG_EDIT_OVERWRITE     29
#define BOOLFLAG_ENGAGED            50
#define BOOLFLAG_FRIENDS_INFORM     52
#define BOOLFLAG_HAVEN              7
#define BOOLFLAG_HTML               49
#define BOOLFLAG_INHERITABLE        43
#define BOOLFLAG_LFTOCR_CR          36
#define BOOLFLAG_LFTOCR_CRLF        32
#define BOOLFLAG_LFTOCR_LFCR        31
#define BOOLFLAG_LISTEN             11
#define BOOLFLAG_PAGEBELL           54
#define BOOLFLAG_PERMANENT          42
#define BOOLFLAG_PRIVATE            45
#define BOOLFLAG_MAIL               47
#define BOOLFLAG_MARRIED            8
#define BOOLFLAG_MORE               37
#define BOOLFLAG_NUMBER             22
#define BOOLFLAG_QUIET              10
#define BOOLFLAG_READONLY           39
#define BOOLFLAG_READONLYDESC       44
#define BOOLFLAG_RETIRED            53
#define BOOLFLAG_SECRET             38
#define BOOLFLAG_TRACING            28
#define BOOLFLAG_UNDERLINE          40
#define BOOLFLAG_VISIBLE            21
#define BOOLFLAG_YELL               14

#define BOOLFLAG_ARTICLE_CONSONANT  23
#define BOOLFLAG_ARTICLE_VOWEL      26
#define BOOLFLAG_ARTICLE_PLURAL     27

#define TRUE_BOOLEXP ((struct boolexp *) NULL)
