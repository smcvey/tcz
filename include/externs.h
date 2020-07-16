/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|---------------------------[ Module Description ]----------------------------|
| EXTERNS.H  -  Global function declarations.                                 |
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
| Module originally modified for TCZ by:  J.P.Boggis 21/12/1993.              |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                        https://github.com/smcvey/tcz                        |
`-----------------------------------------------------------------------------'

*/


#ifndef __EXTERNS_H
#define __EXTERNS_H


#include <netinet/in.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdarg.h>

#include "config.h"
#include "db.h"
#include "flagset.h"


/* ---->  Misc. #define's  <---- */
#define ABS(n)               (((n) > 0) ? (n):(-(n)))
#define can_link_or_home_to(player,loc) ((can_link_to(player,(loc),0)) || Abode(loc))


/* ---->  Define MIN/MAX functions (If not defined already.)  <---- */
#ifndef MIN
   #define MIN(a,b)          ((a) < (b) ? (a):(b))
#endif

#ifndef MAX
   #define MAX(a,b)          ((a) > (b) ? (a):(b))
#endif


/* ---->  Used for the AUTOACTION parameter of construct_message()  <---- */ 
#define NONE                 0
#define PLAYER               1
#define OTHERS               2


/* ---->  Used by grouping/range operator code and array code  <---- */
#define  DEFAULT  0
#define  FIRST   -2
#define  LAST    -3
#define  ALL     -4
#define  UNSET   -5
#define  END     -6
#define  INVALID -7
#define  INDEXED -8


/* ---->  Dynamic array error codes  <---- */
#define ARRAY_INSUFFICIENT_QUOTA -1
#define ARRAY_TOO_MANY_ELEMENTS  -2
#define ARRAY_TOO_MANY_BLANKS    -3
#define ARRAY_INVALID_RANGE      -4

extern struct flag_data flag_list[];
extern struct flag_data flag_list2[];


/* ---->  From admin.c  <---- */
extern  char            *shutdown_reason;
extern  int             shutdown_counter;
extern  long            shutdown_timing;
extern  dbref           shutdown_who;

extern  unsigned char   admin_can_assist        (void);
extern  void            admin_notify_assist     (const char *message,const char *altmsg,dbref exclude);
#ifdef NOTIFY_WELCOME
extern  void            admin_welcome_message   (struct descriptor_data *d,unsigned char guest);
#endif
extern  void            admin_time_adjust       (int adjustment);
extern  void            admin_options           (CONTEXT);
extern  void            admin_list_all          (struct descriptor_data *p,dbref player);
extern  void            admin_list              (CONTEXT);
extern  void            admin_assist            (CONTEXT);
extern  void            admin_ban               (CONTEXT);
extern  unsigned char   admin_boot_charaacter   (dbref player,unsigned char bootdead);
extern  void            admin_boot              (CONTEXT);
extern  void            admin_bootdead          (CONTEXT);
extern  void            admin_controller        (CONTEXT);
extern  void            admin_dump              (CONTEXT);
extern  void            admin_escape            (CONTEXT);
extern  void            admin_force             (CONTEXT);
extern  unsigned char   admin_character_maintenance(void);
extern  unsigned char   admin_object_maintenance(void);
extern  void            admin_maintenance       (CONTEXT);
extern  void            admin_monitor           (CONTEXT);
extern  void            admin_chat              (CONTEXT);
extern  void            admin_newpassword       (CONTEXT);
extern  void            admin_marriage_prefix   (dbref player,unsigned char set);
extern  void            admin_partner           (CONTEXT);
extern  void            admin_quotalimit        (CONTEXT);
extern  void            admin_shout             (CONTEXT);
extern  void            admin_shutdown          (CONTEXT);
extern  void            admin_summon            (CONTEXT);
extern  void            admin_warn              (CONTEXT);
extern  void            admin_welcome           (CONTEXT);


/* ---->  From alias.c  <---- */
extern  void            alias_substitute        (dbref player,char *src,char *dest);
extern  struct alias_data *alias_lookup         (dbref player,const char *command,unsigned char inherit,unsigned char global);
extern  void            alias_readd             (dbref player,struct alias_data *ptr);
extern  unsigned char   alias_remove            (dbref player,struct alias_data *current);
extern  void            alias_query             (CONTEXT);
extern  void            alias_aliases           (CONTEXT);
extern  void            alias_alias             (CONTEXT);
extern  void            alias_unalias           (CONTEXT);


/* ---->  From array.c  <---- */
extern  int             array_element_count     (struct array_element *array);
extern  const char      *array_unparse_element_range(int from,int to,const char *ansi);
extern  int             array_set_elements      (dbref player,dbref array,int from,int to,const char *text,unsigned char insert,int *count,int *newcount);
extern  int             array_set_index         (dbref player,dbref array,int element,const char *indexname);
extern  int             array_nextprev_element  (dbref player,dbref array,int *element,char *in_index,char **out_index,unsigned char next);
extern  int             array_destroy_elements  (dbref player,dbref array,int from,int to,int *count);
extern  int             array_subquery_elements (dbref player,dbref array,int from,int to,char *buffer);
extern  int             array_subquery_index    (dbref player,dbref array,int from,int to,char *buffer);
extern  int             array_subquery_indexno  (dbref player,dbref array,int element);
extern  int             array_display_elements  (dbref player,int from,int to,dbref array,int cr);
extern  void            array_traverse_elements (struct array_sort_data *current);
extern  unsigned char   array_secondary_key     (const char *ptr,const char **sortptr,int *ofs,int *len,unsigned char alpha,short offset,unsigned char elements,const char *separator);
extern  int             array_secondary_match   (const char *sortkey1,unsigned short sortofs1,unsigned short sortlen1,const char *sortkey2,unsigned short sortofs2,unsigned short sortlen2,unsigned char alpha,unsigned char elements,const char *separator);
extern  void            array_sort              (CONTEXT);
extern  void            array_query_elementno   (CONTEXT);
extern  void            array_query_index       (CONTEXT);
extern  void            array_query_indexno     (CONTEXT);
extern  void            array_query_noelements  (CONTEXT);
extern  void            array_index             (CONTEXT);
extern  void            array_insert            (CONTEXT);


/* ---->  From banish.c  <---- */
extern  unsigned char   banish_add              (const char *name,struct banish_data *node);
extern  struct banish_data *banish_lookup       (const char *name,struct banish_data **last,unsigned char exact);
extern  unsigned char   banish_remove           (const char *name);
extern  void            banish_check            (dbref player,const char *name,unsigned char errmsg);
extern  void            banish_traverse         (struct banish_data *current);
extern  void            banish_list             (dbref player,struct descriptor_data *d,const char *name);
extern  void            banish_main             (CONTEXT);


/* ---->  From bbs.c  <---- */
extern  struct          bbs_topic_data          *bbs;

extern  const char      *bbs_logmsg             (struct bbs_message_data *message,struct bbs_topic_data *topic,struct bbs_topic_data *subtopic,int msgno,unsigned char anon);
extern  const char      *bbs_logtopic           (struct bbs_topic_data *topic,struct bbs_topic_data *subtopic);
extern  unsigned char   bbs_unread_message      (struct bbs_message_data *message,dbref who,unsigned char *ignored);
extern  void            bbs_update_vote_expiry  ();
extern  struct          bbs_topic_data *lookup_topic(dbref player,char *name,struct bbs_topic_data **last,struct bbs_topic_data **subtopic);
extern  unsigned char   can_access_topic        (dbref player,struct bbs_topic_data *topic,struct bbs_topic_data *subtopic,unsigned char log);
extern  void            bbs_output_except       (dbref exception,char accesslevel,unsigned char raw,unsigned char redirect,unsigned char wrap,char *fmt, ...);
extern  unsigned char   bbs_addmessage          (dbref player,unsigned short topic_id,unsigned short subtopic_id,char *subject,char *text,long date,unsigned char flags,unsigned short msgid);
extern  void            bbs_appendmessage       (dbref player,struct bbs_message_data *message,struct bbs_topic_data *topic,struct bbs_topic_data *subtopic,const char *append,unsigned char anon);
extern  void            bbs_cyclicdelete        (unsigned short topic_id,unsigned short subtopic_id);
extern  void            bbs_delete_outofdate    (void);


/* ---->  Standard BBS commands  <----- */
extern  void            bbs_add                 (CONTEXT);
extern  void            bbs_anonymous           (CONTEXT);
extern  void            bbs_bbs                 (CONTEXT);
extern  void            bbs_ignore              (CONTEXT);
extern  void            bbs_latest              (CONTEXT);
extern  void            bbs_news                (CONTEXT);
extern  void            bbs_query_latest        (CONTEXT);
extern  void            bbs_query_newmessages   (CONTEXT);
extern  void            bbs_query_readertimediff(CONTEXT);
extern  void            bbs_reply               (CONTEXT);
extern  void            bbs_settings            (CONTEXT);
extern  void            bbs_summary             (CONTEXT);
extern  void            bbs_topic               (CONTEXT);
extern  void            bbs_topics              (CONTEXT);
extern  void            bbs_unread              (CONTEXT);
extern  void            bbs_view                (CONTEXT);
extern  void            bbs_vote                (CONTEXT);


/* ---->  Message owner BBS commands  <---- */
extern  void            bbs_append              (CONTEXT);
extern  void            bbs_delete              (CONTEXT);
extern  void            bbs_modify              (CONTEXT);
extern  void            bbs_move                (CONTEXT);
extern  void            bbs_subject             (CONTEXT);


/* ---->  Admin BBS commands  <---- */
extern  void            bbs_accesslevel         (CONTEXT);
extern  void            bbs_addtopic            (CONTEXT);
extern  void            bbs_copy                (CONTEXT);
extern  void            bbs_desctopic           (CONTEXT);
extern  void            bbs_messagelimit        (CONTEXT);
extern  void            bbs_owner               (CONTEXT);
extern  void            bbs_ownertopic          (CONTEXT);
extern  void            bbs_readers             (CONTEXT);
extern  void            bbs_removetopic         (CONTEXT);
extern  void            bbs_renametopic         (CONTEXT);
extern  void            bbs_setparameter        (CONTEXT);
extern  void            bbs_subtopiclimit       (CONTEXT);
extern  void            bbs_timelimit           (CONTEXT);


/* ---->  From boolexp.c  <---- */
extern  unsigned char   could_satisfy_lock      (dbref player,dbref thing,short level);
extern  unsigned char   can_satisfy_lock        (dbref player,dbref thing,const char *default_fail_msg,unsigned char cr);
extern  struct boolexp  *sanitise_boolexp       (struct boolexp *ptr);
extern  int             eval_boolexp            (dbref player,struct boolexp *b,int level);
extern  struct boolexp  *parse_boolexp          (dbref player,char *buf);


/* ---->  From calculate.c  <---- */
extern  void            calculate_substitute    (dbref player,struct str_ops *str_data,int brackets);
extern  void            calculate_bracket_substitute(dbref player,char *src,char *dest,int level);
extern  struct calc_ops calculate_numeric_op    (dbref player,struct calc_ops result,struct calc_ops value,int calc_op);
extern  struct calc_ops calculate_string_op     (dbref player,struct calc_ops result,char *str,int calc_op);
extern  void            calculate_evaluate      (CONTEXT);


/* ---->  From channels.c  <---- */
extern  char            *channel_restrictions   [];
extern  struct          channel_data            *permanent;

extern  short           channel_sort_cmdtable   (void);
extern  void            channel_main            (CONTEXT);


/* ---->  Channel communication  <---- */
void                    channel_speak           (CHANNEL_CONTEXT);

/* ---->  Channel manipulation  <---- */
void                    channel_join            (CHANNEL_CONTEXT);
void                    channel_leave           (CHANNEL_CONTEXT);
void                    channel_list            (CHANNEL_CONTEXT);
void                    channel_on              (CHANNEL_CONTEXT);
void                    channel_off             (CHANNEL_CONTEXT);

/* ---->  Channel customisation  <---- */
void                    channel_banner          (CHANNEL_CONTEXT);
void                    channel_create          (CHANNEL_CONTEXT);
void                    channel_censor          (CHANNEL_CONTEXT);
void                    channel_destroy         (CHANNEL_CONTEXT);
void                    channel_motd            (CHANNEL_CONTEXT);
void                    channel_rename          (CHANNEL_CONTEXT);
void                    channel_title           (CHANNEL_CONTEXT);
void                    channel                 (CONTEXT);


/* ---->  From character.c  <---- */
extern  struct          feeling_data            feelinglist[];

extern  unsigned char   guestcount;
extern  int             init_feelings           ();
extern  dbref           lookup_character        (dbref player,const char *name,unsigned char connected);
extern  dbref           lookup_nccharacter      (dbref player,const char *name,int create);
extern  dbref           connect_character       (const char *name,const char *password,const char *hostname);
extern  dbref           create_new_character    (const char *name,const char *password,unsigned char checknp);
extern  dbref           connect_guest           (unsigned char *created);
extern  void            destroy_guest           (dbref guestchar);
extern  const char      *check_duplicates       (dbref player,const char *email,unsigned char warn,unsigned char newchar);


/* ---->  From combat.c  <---- */
extern  void            update_health           (dbref player);
extern  const char      *combat_percent         (double numb1,double numb2);
extern  void            combat_damage           (CONTEXT);
extern  void            combat_delay            (CONTEXT);
extern  void            combat_heal             (CONTEXT);
extern  void            combat_statistics       (CONTEXT);
extern  void            combat_query_delay      (CONTEXT);
extern  void            combat_query_health     (CONTEXT);
extern  void            combat_query_statistics (CONTEXT);


/* ---->  From command.c  <---- */
extern  void            setreturn               (const char *result,int value);
extern  int             usec_difference         (struct timeval time1,struct timeval time2);
extern  void            command_clear_args      (void);
extern  void            command_reset           (dbref player);
extern  const char      *command_sub_execute  (dbref player,char *command,unsigned char cachebool,unsigned char cachegrp);
extern  void            command_cache_execute   (dbref player,dbref command,unsigned char temps,unsigned char restartable);
extern  int             command_can_execute     (dbref player,char *arg0,char *arg1,char *arg2,char *arg3);
extern  void            command_execute         (dbref player,dbref command,const char *commands,unsigned char restartable);
extern  unsigned char   command_execute_action  (dbref player,dbref object,const char *name,const char *arg0,const char *arg1,const char *arg2,const char *arg3,int secure);
extern  void            command_chpid           (CONTEXT);
extern  void            command_chuid           (CONTEXT);
extern  void            command_csucc_or_cfail  (CONTEXT);
extern  void            command_executionlimit  (CONTEXT);
extern  void            command_output          (CONTEXT);
extern  void            command_rem             (CONTEXT);
extern  void            command_returnvalue     (CONTEXT);
extern  void            command_true_or_false   (CONTEXT);
extern  void            command_unchpid         (CONTEXT);
extern  void            command_use             (CONTEXT);
extern  void            command_query_cmdname   (CONTEXT);
extern  void            command_query_csucc_or_cfail(CONTEXT);
extern  void            command_query_execution (CONTEXT);
extern  void            command_query_internal  (CONTEXT);
extern  void            command_query_uid       (CONTEXT);


/* ---->  From communication.c  <---- */
extern  void            comms_spoken            (dbref player,int absolute);
extern  void            comms_afk               (CONTEXT);
extern  void            comms_areawrite         (CONTEXT);
extern  void            comms_ask               (CONTEXT);
extern  void            comms_beep              (CONTEXT);
extern  void            comms_censor            (CONTEXT);
extern  dbref           comms_chat_operator     (int channel);
extern  void            comms_chat_check        (struct descriptor_data *op,const char *nameptr);
extern  const char      *comms_chat_channelname (int channel);
extern  void            comms_chat              (CONTEXT);
extern  void            comms_comment           (CONTEXT);
extern  void            comms_converse          (CONTEXT);
extern  void            comms_echo              (CONTEXT);
extern  void            comms_echolist          (CONTEXT);
extern  void            comms_emergency         (CONTEXT);
extern  void            comms_notify            (CONTEXT);
extern  void            comms_oecho             (CONTEXT);
extern  void            comms_oemote            (CONTEXT);
extern  void            comms_pose              (CONTEXT);
extern  void            comms_say               (CONTEXT);
extern  void            comms_session           (CONTEXT);
extern  void            comms_think             (CONTEXT);
extern  void            comms_wake              (CONTEXT);
extern  void            comms_whisper           (CONTEXT);
extern  void            comms_write             (CONTEXT);
extern  void            comms_yell              (CONTEXT);


/* ---->  From compression.c  <---- */
extern  char            *compress               (const char *str,unsigned char capitalise);
extern  char            *decompress             (const char *str);
extern  void            compression_ratio       (void);
extern  unsigned char   initialise_compression_table(const char *filename,unsigned char compress);


/* ---->  From container.c  <---- */
extern  void            container_close         (CONTEXT);
extern  void            container_enter         (CONTEXT);
extern  void            container_leave         (CONTEXT);
extern  void            container_lock          (CONTEXT);
extern  void            container_open          (CONTEXT);
extern  void            container_unlock        (CONTEXT);


/* ---->  From create.c  <---- */
extern  dbref           create_alarm            (CONTEXT);
extern  dbref           create_array            (CONTEXT);
extern  dbref           create_command          (CONTEXT);
extern  dbref           create_character        (CONTEXT);
extern  struct boolexp  *create_duplicate_lock  (struct boolexp *bool);
extern  dbref           create_duplicate        (CONTEXT);
extern  dbref           create_exit             (CONTEXT);
extern  dbref           create_fuse             (CONTEXT);
extern  dbref           create_homeroom         (dbref player,unsigned char warn,unsigned char sethome,unsigned char tohome);
extern  dbref           create_room             (CONTEXT);
extern  dbref           create_thing            (CONTEXT);
extern  dbref           create_variable_property(CONTEXT);


/* ---->  From destroy.c  <---- */
extern  unsigned char   destroy_object          (dbref player,dbref object,unsigned char char_ok,unsigned char log,unsigned char queue,unsigned char nested);
extern  void            destroy_destroy         (CONTEXT);
extern  void            destroy_destroyall      (CONTEXT);
extern  void            destroy_undestroy       (CONTEXT);


/* ---->  From edit.c  <---- */
extern  struct          edit_cmd_table edit_cmds[];
extern  unsigned short  edit_table_size;

extern  char 		*get_lineno		(int lineno,char *text);
extern  const   	char *field_name  	(int field);
extern  int 		fieldtype		(const char *field);
extern  int 		can_edit		(struct descriptor_data *d,int field,dbref object,int change);
extern  const   	char *object_name 	(int object);
extern  char    	*get_field   		(dbref player,int field,dbref object,int element);
extern  int     	update_field 		(struct descriptor_data *d);
extern  short           sort_edit_cmdtable      (void);
extern  struct edit_cmd_table *search_edit_cmdtable(const char *command);
extern  void            exit_editor             (struct descriptor_data *d,int save);
extern  const char      *field_name             (int field);
extern  void            edit_abort_or_save      (EDIT_CONTEXT);
extern  void            edit_append             (EDIT_CONTEXT);
extern  void            edit_copy               (EDIT_CONTEXT);
extern  void            edit_delete             (EDIT_CONTEXT);
extern  void            edit_evaluate           (EDIT_CONTEXT);
extern  void            edit_execute            (EDIT_CONTEXT);
extern  void            edit_field              (EDIT_CONTEXT);
extern  void            edit_help               (EDIT_CONTEXT);
extern  void            edit_insert             (EDIT_CONTEXT);
extern  void            edit_move               (EDIT_CONTEXT);
extern  void            edit_numbering          (EDIT_CONTEXT);
extern  void            edit_permanent          (EDIT_CONTEXT);
extern  void            edit_overwrite          (EDIT_CONTEXT);
extern  void            edit_position           (EDIT_CONTEXT);
extern  void            edit_replace            (EDIT_CONTEXT);
extern  void            edit_set                (EDIT_CONTEXT);
extern  void            edit_succ_or_fail       (EDIT_CONTEXT);
extern  void            edit_top_or_bottom      (EDIT_CONTEXT);
extern  void            edit_up_or_down         (EDIT_CONTEXT);
extern  void            edit_value              (EDIT_CONTEXT);
extern  void            edit_view               (EDIT_CONTEXT);
extern  void            edit_process_command    (struct descriptor_data *d,char *original_editcmd);
extern  unsigned char   editing                 (dbref player);
extern  void            edit_initialise         (dbref player,unsigned char field,const char *text,union group_data *data,const char *data1,const char *data2,unsigned char permanent,int temp);
extern  void            edit_header             (dbref player,dbref object,unsigned char field);
extern  void            edit_start              (CONTEXT);


/* ---->  From event.c  <---- */
extern  void            event_initialise        (void);
extern  void            event_remove            (dbref object);
extern  void            event_add               (dbref player,dbref object,dbref command,dbref data,long exectime,const char *str);
extern  unsigned char   event_pending_at        (long exectime,dbref *player,dbref *object,dbref *command,dbref *data,char **str);
extern  void            event_set_fuse_args     (const char *args,char **arg0,char **arg1,char **arg2,char **arg3,char *buffer,char *token,unsigned char flags);
extern  unsigned char   event_trigger_fuses     (dbref player,dbref fuse,const char *args,unsigned char flags);
extern  long            event_next_cron         (const char *cron_format);
extern  void            event_pending           (CONTEXT);


/* ---->  From finance.c  <---- */
extern  double          currency_to_double      (struct currency_data *currency);
extern  struct          currency_data           *double_to_currency(struct currency_data *currency,double value);
extern  void            currency_add            (struct currency_data *currency,double value);
extern  double          currency_compare        (struct currency_data *currency1,struct currency_data *currency2);

extern  void            finance_bank            (CONTEXT);
extern  void            finance_credit          (CONTEXT);
extern  void            finance_deposit         (CONTEXT);
extern  void            finance_payment         (struct descriptor_data *p);
extern  void            finance_quarter         ();
extern  void            finance_restrict        (CONTEXT);
extern  void            finance_statement       (CONTEXT);
extern  void            finance_transaction     (CONTEXT);
extern  void            finance_withdraw        (CONTEXT);


/* ---->  From friends.c  <---- */
extern  unsigned char   friend                  (dbref player,dbref friend);
extern  int             friend_flags            (dbref player,dbref friend);
extern  int             friendflags_set         (dbref player,dbref friend,dbref object,int flags);
extern  void            friendflags_privs       (dbref player);
extern  unsigned char   friendflags_check       (dbref player,dbref subject,dbref user,int flag,const char *msg);
extern  void            friends_add             (CONTEXT);
extern  void            friends_remove          (CONTEXT);
extern  void            friends_set             (CONTEXT);
extern  unsigned char   parse_friendflagtype    (const char *str,int *flags_inc,int *flags_exc);
extern  void            friends_list            (CONTEXT);
extern  void            friends_chat            (CONTEXT);
extern  void            friends_cmd             (CONTEXT);


/* ---->  From global.c  <---- */
extern  dbref           effective_location      (dbref command);
extern  void            global_add              (dbref command);
extern  void            global_readd            (struct global_data *ptr);
extern  void            global_delete           (dbref command);
extern  int             global_initialise       (dbref location,unsigned char init);
extern  dbref           global_lookup           (const char *name,int occurence);


/* ---->  From group.c  <---- */
extern  struct grp_data *grouprange_initialise  (struct grp_data *grpdata);
extern  const char      *grouprange_value       (char *buffer,int value);
extern  const char      *listed_items           (char *buffer,u_char fullstop);
extern  void            sanitise_grproot        (void);
extern  const char      *parse_grouprange       (dbref player,const char *str,char autopage,unsigned char defaults);
extern  int             match_object_type       (dbref object,int type,int object_type);
extern  void            set_conditions_ps       (dbref player,int flags,int mask,int flags2,int mask2,int exc,int object_type,int object_who,const char *object_name,int condition);
extern  void            set_conditions          (dbref player,int object_flags,int object_mask,int object_type,int object_who,const char *object_name,int condition);
extern  void            sanitise_grouprange     (void);
extern  void           	db_initgrouprange       (dbref object,dbref currentobj);
extern  void           	entiredb_initgrouprange (void);
extern  void           	union_initgrouprange    (union group_data *object);
extern  int            	db_grouprange           (void);
extern  int             entiredb_grouprange     (void);
extern  int             union_grouprange        (void);
extern  void            group_query_display     (CONTEXT);
extern  void            group_query_groupno     (CONTEXT);
extern  void            group_query_grouprange  (CONTEXT);
extern  void            group_query_groupsize   (CONTEXT);
extern  void            group_query_rangefrom   (CONTEXT);
extern  void            group_query_rangeto     (CONTEXT);


/* ---->  From help.c  <---- */
extern  struct          helptopic               *generictutorials;
extern  struct          helptopic               *localtutorials;
extern  struct          helptopic               *generichelp;
extern  int                                     titlescreens;
extern  struct          helptopic               *localhelp;
extern  char                                    *disclaimer;

extern  char            *help_topic_filter      (char *topicname);
extern  void            help_search             (struct descriptor_data *d,char *topic,int page,int help);
extern  struct helptopic *help_add_topic        (struct helptopic **topics,const char *name,int reloading,int help,int local);
extern  void            help_register_topic     (struct helptopic **topics,char *topicnames,struct helppage *page,const char *title,unsigned char tutorial,int reloading,int help,int local);
extern  int             help_register_topics    (struct helptopic **topics,char *filename,const char *wildspec,int reloading,int help,int local);
extern  void            help_status             (void);
extern  const char      *help_buttons           (struct descriptor_data *d,struct helptopic *topic,unsigned char page,unsigned char connected,int help,char *buffer);
extern  int             help_display_topic      (struct descriptor_data *d,struct helptopic *topic,unsigned char page,const char *topicname,int help);
extern  struct helptopic *help_match_topic      (const char *topic,int help);
extern  const char      *help_get_titlescreen   (int titlescreenno);
extern  char            *help_reload_text       (const char *filename,unsigned char limit,unsigned char compression,u_char addcr);
extern  unsigned char   help_reload_titles      (void);
extern  void            help_reload             (CONTEXT);
extern  void            help_main               (CONTEXT);


/* ---->  From html.c  <---- */
extern  const char      *http_content_type      [];
extern  int             html_internal_images;

extern  const char      *http_header            (struct descriptor_data *d,int output,int code,const char *codestr,const char *contenttype,time_t modified,time_t expires,int cache,int contentlen);
extern  const char      *html_header            (struct descriptor_data *d,int output,const char *title,int refresh,const char *background,const char *fg,const char *bg,const char *link,const char *alink,const char *vlink,const char *body,int java,int logo);
extern  const char      *html_error             (struct descriptor_data *d,int output,const char *message,const char *title,const char *back,const char *backurl,int flags);
extern  const char      *html_server_url        (struct descriptor_data *d,int code,int params,const char *resource);
extern  const char      *html_image_url         (const char *image);
extern  void            html_init_smileys       (void);
extern  short           html_sort_tags          (void);
extern  short           html_sort_entities      (void);
extern  void            html_free_images        (void);
extern  void            html_init_images        (int count);
extern  struct html_image_data *html_search_images(const char *filename);
extern  int             html_load_images        (void);
extern  void            html_free               (void);
extern  unsigned char   html_init               (char *src,int len);
extern  const char      *html_lookup            (const char *name,unsigned char leading,unsigned char trailing,unsigned char excess,unsigned char invalid,unsigned char truncate);
extern  const char      *text_to_html           (struct descriptor_data *d,const char *text,char *buffer,int *length,int limit);
extern  const char      *text_to_html_close_tags(struct descriptor_data *d,char *buffer,int *length,int limit);
extern  const char      *text_to_html_reset     (struct descriptor_data *d,char *buffer,int *length);
extern  const char      *html_encode            (const char *text,char *buffer,int *copied,int limit);
extern  const char      *html_encode_basic      (const char *text,char *buffer,int *copied,int limit);
extern  const char      *html_get_preferences   (struct descriptor_data *d);
extern  unsigned char   html_set_preferences    (struct descriptor_data *d,const char *prefs);
extern  void            html_preferences_form   (struct descriptor_data *d,unsigned char saved,unsigned char set);
extern  const char      *html_to_text           (struct descriptor_data *d,char *html,char *buffer,int limit);
extern  void            html_process_input      (struct descriptor_data *d,unsigned char *buffer,int length);
extern  unsigned char   html_process_data       (struct descriptor_data *d);
extern  void            html_anti_reverse       (struct descriptor_data *d,unsigned char reverse);
extern  void            html_query_link         (CONTEXT);
extern  void            html_query_tczlink      (CONTEXT);
extern  void            html_query_image        (CONTEXT);


/* ---->  From interface.c  <---- */
extern  struct cmd_table general_cmds[];
extern  struct cmd_table query_cmds[];
extern  struct cmd_table short_cmds[];
extern  struct cmd_table bank_cmds[];
extern  struct cmd_table bbs_cmds[];
extern  struct cmd_table at_cmds[];

extern  const char      *tcz_command_string;
extern  unsigned short  general_table_size;
extern  unsigned short  query_table_size;
extern  unsigned short  short_table_size;
extern  unsigned short  bank_table_size;
extern  unsigned short  bbs_table_size;
extern  unsigned short  at_table_size;
extern  int             nesting_level;

extern  short           sort_cmdtable           (struct cmd_table *table,unsigned short *count);
extern  struct cmd_table *search_cmdtable       (const char *command,struct cmd_table *table,unsigned short entries);
extern  int             init_tcz                (unsigned char load_db,unsigned char compressed);
extern  void            process_basic_command   (dbref player,char *original_command,unsigned char converse);
extern  unsigned char   sanitise_character      (dbref player);
extern  void            tcz_command             (struct descriptor_data *d,dbref player,const char *command);
extern  void            update_lasttotal        (dbref player,int update);
extern  void            update_all_lasttotal    (void);
extern  void            tcz_connect_character   (struct descriptor_data *d,dbref player,int create);
extern  void            tcz_disconnect_character(struct descriptor_data *d);
extern  void            birthday_notify         (time_t now,dbref birthday);
extern  void            notify_shutdown         (const char *msg);
extern  void            tcz_time_sync           (unsigned char init);


/* ---->  From lists.c  <---- */
extern  int             lists_free              (struct userlist_data **userlist,int incuser,int excuser,int inccustom,int exccustom,unsigned char all);
extern  struct          lists_data              *userlist_parse(dbref player,const char *userlist,const char *action,int userflags,unsigned char errors);
extern  int             lists_count             (struct userlist_data *userlist,int incuser,int excuser,int inccustom,int exccustom,unsigned char all);
extern  int             lists_userflags         (struct userlist_data *userlist,int set,int reset,int incuser,int excuser,int inccustom,int exccustom,unsigned char all);
extern  int             lists_customflags       (struct userlist_data *userlist,int set,int reset,int incuser,int excuser,int inccustom,int exccustom,unsigned char all);
extern  struct          lists_data              *userlist_duplicate(struct userlist_data *userlist,int incuser,int excuser,int inccustom,int exccustom,unsigned char all);
extern  const char      *lists_construct        (dbref source,dbref target,struct userlist_data *userlist,const char *ansi1,const char *ansi2,char *buffer,int incuser,int excuser,int inccustom,int exccustom,unsigned char all,unsigned char single);
extern  void            lists_test              (CONTEXT);


/* ---->  From logfiles.c  <---- */
extern  void            logfile_open            (int restart);
extern  void            logfile_close           (void);
extern  const char      *get_timedate           (void);
extern  void            writelog                (int logfile,int logdate,const char *title,const char *fmt, ...);
extern  void            logfile_log             (CONTEXT);
extern  void            logfile_logentry        (CONTEXT);


/* ---->  From look.c  <---- */
extern  const char      *look_name_status       (dbref player,dbref object,char *buffer,unsigned char spaces);
extern  unsigned char   look_description        (dbref player,const char *desc,unsigned char subst,unsigned char censor,unsigned char addcr);
extern  void            look_container          (dbref player,dbref object,const char *title,int level,unsigned char examine);
extern  void            look_contents           (dbref player,dbref loc,const char *title);
extern  void            look_simple             (dbref player,dbref object);
extern  void            look_exits              (dbref player,dbref location);
extern  void            look_room               (dbref player,dbref location);
extern  void            look_examine_list       (dbref player,dbref object,dbref objecttype,const char *title,const char *lansi,const char *dansi);
extern  void            look_cls                (CONTEXT);
extern  void            look_date               (CONTEXT);
extern  void            look_disclaimer         (CONTEXT);
extern  void            look_examine            (CONTEXT);
extern  void            look_idle               (CONTEXT);
extern  void            look_inventory          (CONTEXT);
extern  void            look_at                 (CONTEXT);
extern  void            look_motd               (CONTEXT);
extern  void            look_notice             (CONTEXT);
extern  void            look_privileges         (CONTEXT);
extern  void            look_profile            (CONTEXT);
extern  void            look_scan               (CONTEXT);
extern  void            look_score              (CONTEXT);
extern  void            look_titles             (CONTEXT);
extern  void            look_uptime             (CONTEXT);
extern  void            look_version            (CONTEXT);


/* ---->  From mail.c  <---- */
extern  void            mail_update             (dbref player,const char *name,const char *arg2);
extern  unsigned char 	mail_send_message	(dbref player,struct mlist_data **list,const char *text,const char *subject,const char *grouplist,int listsize,unsigned char targetgroup);
extern  unsigned char 	mail_reply_message	(dbref player,dbref who,dbref redirect,struct mail_data *rmail,short msgno,const char *message);
extern  void            mail_main               (CONTEXT);



/* ---->  From map.c  <---- */
extern  int             map_reload              (dbref player,int reload);
extern  void            map_html                (struct descriptor_data *p);
extern  void            map_query_colourmap     (CONTEXT);
extern  void            map_main                (CONTEXT);


/* ---->  From match.c  <---- */
extern  char            indexfrom               [];  /*  From index name  */
extern  char            indexto                 [];  /*  To index name    */
extern  int             elementfrom;                 /*  From element     */
extern  int             elementto;                   /*  To element       */

extern  int             type_to_search          [];  /*  Convert TYPE_... to SEARCH_...  */
extern  short           sort_name_to_type       (void);
extern  int             search_name_to_type     (const char *name);
extern  u_char          match_recursed;

extern  dbref           match_simple            (dbref object,const char *name,int list,unsigned char recurse,unsigned char multiple);
extern  dbref           match_object            (dbref owner,dbref start,dbref end,const char *searchstr,unsigned char startphase,unsigned char endphase,int types,int options,int subtypse,struct match_data *caller,unsigned char level);
extern  dbref           match_preferred         (dbref owner,dbref start,const char *searchstr,unsigned char startphase,unsigned char endphase,int types,int preferred,int options);
extern  dbref           match_continue          (void);
extern  void            match_done              (void);
extern  void            match_query             (CONTEXT);
extern  void            match_query_my          (CONTEXT);
extern  void            match_query_myself      (CONTEXT);


/* ---->  From modules.c  <---- */
extern  int             modules_type            (const char *name);
extern  int             modules_sort_modules    (void);
extern  int             modules_sort_authors    (void);
extern  void            modules_modules_view    (dbref player,const char *module);
extern  void            modules_modules_list    (dbref player);
extern  void            modules_authors_view    (dbref player,const char *author);
extern  void            modules_authors_list    (dbref player);
extern  void            modules_modules         (CONTEXT);
extern  void            modules_authors         (CONTEXT);


/* ---->  From move.c  <---- */
extern  unsigned char   move_to                 (dbref source,dbref destination);
extern  void            move_home               (dbref object,int homeroom);
extern  void            move_contents           (dbref loc,dbref dest,dbref override);
extern  void            move_dropto_delay       (dbref object,dbref dropto);
extern  unsigned char   move_enter              (dbref player,dbref destination,unsigned char autolook);
extern  void            move_sendhome           (void);
extern  unsigned char   move_character          (CONTEXT);
extern  void            move_vehicle            (dbref player,dbref location,dbref vehicle,char *dest,unsigned char drive);
extern  void            move_drive              (CONTEXT);
extern  void            move_getdrop            (CONTEXT);
extern  void            move_give               (CONTEXT);
extern  void            move_homeroom           (CONTEXT);
extern  void            move_kick               (CONTEXT);
extern  void            move_remote             (CONTEXT);
extern  void            move_ride               (CONTEXT);
extern  void            move_teleport           (CONTEXT);
extern  void            move_location           (CONTEXT);
extern  void            move_visit              (CONTEXT);
extern  void            move_warp               (CONTEXT);


/* ---->  From options.c  <---- */
extern  int             option_boolean          (const char *optarg,int defaultvalue);
extern  const char      *option_adminemail      (OPTCONTEXT);
extern  const char      *option_backdoor        (OPTCONTEXT);
extern  int             option_check            (OPTCONTEXT);
extern  int             option_compress_disk    (OPTCONTEXT);
extern  int             option_compress_memory  (OPTCONTEXT);
extern  int             option_console          (OPTCONTEXT);
extern  int             option_coredump         (OPTCONTEXT);
extern  const char      *option_database        (OPTCONTEXT);
extern  const char      *option_dataurl         (OPTCONTEXT);
extern  int             option_debug            (OPTCONTEXT);
extern  int             option_dns              (OPTCONTEXT);
extern  int             option_dumping          (OPTCONTEXT);
extern  const char      *option_emailforward    (OPTCONTEXT);
extern  int             option_emergency        (OPTCONTEXT);
extern  int             option_forkdump         (OPTCONTEXT);
extern  const char      *option_fullname        (OPTCONTEXT);
extern  int	        option_guardian         (OPTCONTEXT);
extern  const char      *option_generate        (OPTCONTEXT);
extern  int             option_htmlport         (OPTCONTEXT);
extern  int             option_images           (OPTCONTEXT);
extern  int             option_local            (OPTCONTEXT);
extern  const char      *option_location        (OPTCONTEXT);
extern  int             option_logins           (OPTCONTEXT);
extern  int             option_loglevel         (OPTCONTEXT);
extern  const char      *option_motd            (OPTCONTEXT);
extern  int             option_nice             (OPTCONTEXT);
extern  const char      *option_path            (OPTCONTEXT);
extern  int             option_quit             (OPTCONTEXT);
extern  const char      *option_server          (OPTCONTEXT);
extern  int             option_serverinfo       (OPTCONTEXT);
extern  const char      *option_shortname       (OPTCONTEXT);
extern  int             option_shutdown         (OPTCONTEXT);
extern  int             option_ssl              (OPTCONTEXT);
extern  int             option_telnetport       (OPTCONTEXT);
extern  int             option_userlogs         (OPTCONTEXT);
extern  const char      *option_website         (OPTCONTEXT);
extern  void            option_options          (struct descriptor_data *d,unsigned char options);
extern  void            option_get_list         (int argc,char **argv);
extern  void            option_free_list        (void);
extern  int             option_option           (dbref player,struct option_list_data *opt,const char *title,const char *pname,int *error,int *critical,int instance);
extern  int             option_match            (dbref player,const char *title,const char *option,const char *value,const char *pname,int handler,int instance);
extern  int             option_config           (const char *filename,const char *pname,int defaultconfig);
extern  int             option_configfile       (const char *pname);
extern  int             option_changepath       (void);
extern  int             option_helpoptions      (const char *pname);
extern  int             option_main             (const char *pname);


/* ---->  From output.c  <---- */
extern  int             wrap_leading;
extern  int             termflags;
extern  unsigned char   add_cr;

extern  void            output_terminate        (char **dest,int terminator,int *length,unsigned char *overflow);
extern  int             output_text_remaining   (const char *str);
extern  int             output_ansi_filter      (char **dest,char **src,struct descriptor_data *d,const char **cur_bg,const char **cur_fg,int *ansiflags,int *length,unsigned char *overflow);
extern  void            output_terminal_style   (char **dest,struct descriptor_data *d,int *length,unsigned char *overflow);
extern  void            output_filter_nonstandard_ansi(char *src);
extern  int             output_queue_nonwrapped (struct descriptor_data *d,char *src);
extern  int             output_queue_wrapped    (struct descriptor_data *d,char *src);
extern  int             output_queue_string     (struct descriptor_data *d,const char *str,unsigned char redirect);
extern  struct descriptor_data *getdsc          (dbref player);
extern  void            output                  (struct descriptor_data *d,dbref player,unsigned char raw,unsigned char redirect,unsigned char wrap,char *fmt, ...);
extern  void            output_except           (dbref area,dbref name1,dbref name2,unsigned char raw,unsigned char redirect,unsigned char wrap,char *fmt, ...);
extern  void            output_chat             (int channel,dbref exception,unsigned char raw,unsigned char redirect,char *fmt, ...);
extern  void            output_admin            (unsigned char quiet,unsigned char raw,unsigned char redirect,unsigned char wrap,char *fmt, ...);
extern  unsigned char   output_trace            (dbref player,dbref command,unsigned char raw,unsigned char redirect,unsigned char wrap,char *fmt, ...);
extern  void            output_all              (unsigned char users,unsigned char admin,unsigned char raw,unsigned char wrap,char *fmt, ...);
extern  int             output_terminal_width   (dbref player);
extern  void            output_columns          (struct descriptor_data *p,dbref player,const char *text,const char *colour,int setwidth,unsigned char setleading,unsigned char setitemlen,unsigned char setpadding,unsigned char setalignleft,unsigned char settable,char option,int setlines,const char *setnonefound,char *buffer);
extern  void            output_listen           (struct descriptor_data *d,int action);


/* ---->  From pager.c  <---- */
extern  void            pager_init              (struct descriptor_data *d);
extern  void            pager_add               (struct descriptor_data *d,unsigned char *text,int len);
extern  void            pager_free              (struct descriptor_data *d);
extern  void            pager_prompt            (struct descriptor_data *d);
extern  void            pager_display           (struct descriptor_data *d);
extern  void            pager_process           (struct descriptor_data *d,const char *command);
extern  void            pager_more              (CONTEXT);


/* ---->  From pagetell.c  <---- */
extern  const char      *pagetell_construct_list(dbref pager,dbref player,union group_data *list,int listsize,char *buffer,const char *ansi1,const char *ansi2,unsigned char sameroomonly,unsigned char targetgroup,unsigned char article_setting);
extern  void            pagetell_send           (CONTEXT);
extern  void            pagetell_friends        (CONTEXT);
extern  void            pagetell_recall         (CONTEXT);
extern  void            pagetell_reply          (CONTEXT);


/* ---->  From predicates.c  <---- */
extern  unsigned char   can_see                 (dbref player,dbref thing,unsigned char can_see_location);
extern  void            warnquota               (dbref player,dbref subject,const char *message);
extern  unsigned char   adjustquota             (dbref player,dbref subject,int quota);
extern  unsigned char   ok_name                 (const char *password);
extern  unsigned char   ok_character_name       (dbref player,dbref who,const char *password);
extern  unsigned char   ok_password             (const char *password);
extern  unsigned char   ok_email                (dbref player,char *email);
extern  unsigned char   ok_presuffix            (dbref player,const char *presuffix,unsigned char prefix);
extern  unsigned char   can_reach               (dbref who,dbref what);
extern  int             getweight               (dbref thing);
extern  unsigned char   will_fit                (dbref victim,dbref destination);
extern  int             find_volume_of_contents (dbref thing,unsigned char level);
extern  int             find_mass_of_contents   (dbref thing,unsigned char level);
extern  unsigned char   can_link_to             (dbref player,dbref destination,unsigned char override);
extern  unsigned char   privilege               (dbref player,unsigned char cutoff);
extern  unsigned char   level                   (dbref player);
extern  unsigned char   level_app               (dbref player);
extern  unsigned char   can_read_from           (dbref player,dbref object);
extern  unsigned char   can_write_to            (dbref player,dbref object,unsigned char app_priv);
extern  dbref           parse_link_command      (dbref player,dbref object,const char *name,unsigned char prompt);
extern  dbref           parse_link_destination  (dbref player,dbref object,const char *name,int flags);


/* ---->  From preferences.c  <---- */
extern  void            prefs_ansi              (dbref player,struct descriptor_data *d,const char *status);
extern  void            prefs_lftocr            (dbref player,struct descriptor_data *d,const char *status);
extern  void            prefs_localecho         (dbref player,struct descriptor_data *d,const char *status);
extern  void            prefs_pagebell          (dbref player,struct descriptor_data *d,const char *status);
extern  void            prefs_prompt            (dbref player,struct descriptor_data *d,const char *params);
extern  void            prefs_screenheight      (dbref player,struct descriptor_data *d,const char *params);
extern  void            prefs_termtype          (dbref player,struct descriptor_data *d,const char *termtype);
extern  void            prefs_timediff          (dbref player,struct descriptor_data *d,const char *params);
extern  void            prefs_underlining       (dbref player,struct descriptor_data *d,const char *status);
extern  void            prefs_wrap              (dbref player,struct descriptor_data *d,char *width);
extern  void            prefs_set               (dbref player,struct descriptor_data *d,char *arg1,char *arg2);


/* ---->  From prompt.c  <---- */
extern  void            prompt_display          (struct descriptor_data *d);
extern  void            prompt_interactive_input(struct descriptor_data *d,char *input);
extern  void            prompt_interactive      (CONTEXT);
extern  void            prompt_user_free        (struct descriptor_data *d);
extern  void            prompt_user_input       (struct descriptor_data *d,char *input);
extern  int             prompt_user             (dbref player,const char *prompt,const char *promptmsg,const char *defaultval,const char *blankmsg,const char *incmdmsg,const char *command,int setarg,const char *params,const char *arg1,const char *arg2,const char *arg3);


/* ---->  From query.c  <---- */
extern  char            querybuf                [BUFFER_LEN];
extern  dbref           query_find_object       (dbref player,const char *name,int preferred,unsigned char need_control,unsigned char private);
extern  dbref           query_find_character    (dbref player,const char *name,unsigned char need_control);

extern  void            query_address           (CONTEXT);
extern  void            query_area              (CONTEXT);
extern  void            query_areaname          (CONTEXT);
extern  void            query_areanameid        (CONTEXT);
extern  void            query_averageactive     (CONTEXT);
extern  void            query_averageidle       (CONTEXT);
extern  void            query_averagelogins     (CONTEXT);
extern  void            query_averagetime       (CONTEXT);
extern  void            query_boolean           (CONTEXT);
extern  void            query_balance           (CONTEXT);
extern  void            query_censor            (CONTEXT);
extern  void            query_cestring          (CONTEXT);
extern  void            query_charclass         (CONTEXT);
extern  void            query_connected         (CONTEXT);
extern  void            query_controller        (CONTEXT);
extern  void            query_created_or_lastused(CONTEXT);
extern  void            query_credit            (CONTEXT);
extern  void            query_datetime          (CONTEXT);
extern  void            query_datetimeformat    (CONTEXT);
extern  void            query_delete            (CONTEXT);
extern  void            query_description       (CONTEXT);
extern  void            query_destination       (CONTEXT);
extern  void            query_drop              (CONTEXT);
extern  void            query_email             (CONTEXT);
extern  void            query_evallock          (CONTEXT);
extern  void            query_exists            (CONTEXT);
extern  void            query_expiry            (CONTEXT);
extern  void            query_fail              (CONTEXT);
extern  void            query_feeling           (CONTEXT);
extern  void            query_filter            (CONTEXT);
extern  void            query_finance           (CONTEXT);
extern  void            query_first_name        (CONTEXT);
extern  void            query_flags             (CONTEXT);
extern  void            query_format            (CONTEXT);
extern  void		      query_format_number	   (CONTEXT);
extern  void            query_friend            (CONTEXT);
extern  void            query_fullname          (CONTEXT);
extern  void            query_head              (CONTEXT);
extern  void            query_id                (CONTEXT);
extern  void            query_idletime          (CONTEXT);
extern  void            query_insert            (CONTEXT);
extern  void            query_interval          (CONTEXT);
extern  void            query_item              (CONTEXT);
extern  void            query_itemno            (CONTEXT);
extern  void            query_key               (CONTEXT);
extern  void            query_lastcommand       (CONTEXT);
extern  void            query_lastconnected     (CONTEXT);
extern  void            query_lastsite          (CONTEXT);
extern  void            query_leftstr           (CONTEXT);
extern  void            query_line              (CONTEXT);
extern  void            query_location          (CONTEXT);
extern  void            query_lock              (CONTEXT);
extern  void            query_longestdate       (CONTEXT);
extern  void            query_longesttime       (CONTEXT);
extern  void            query_mass_or_volume    (CONTEXT);
extern  void            query_midstr            (CONTEXT);
extern  void            query_modify            (CONTEXT);
extern  void            query_name              (CONTEXT);
extern  void            query_namec             (CONTEXT);
extern  void            query_nested            (CONTEXT);
extern  void            query_newline           (CONTEXT);
extern  void            query_next              (CONTEXT);
extern  void            query_noitems           (CONTEXT);
extern  void            query_nowords           (CONTEXT);
extern  void            query_number            (CONTEXT);
extern  void            query_object            (CONTEXT);
extern  void            query_odesc             (CONTEXT);
extern  void            query_odrop             (CONTEXT);
extern  void            query_ofail             (CONTEXT);
extern  void            query_osucc             (CONTEXT);
extern  void            query_owner             (CONTEXT);
extern  void            query_pad               (CONTEXT);
extern  void            query_parent            (CONTEXT);
extern  void            query_partner           (CONTEXT);
extern  void            query_peak              (CONTEXT);
extern  void            query_pending           (CONTEXT);
extern  void            query_prefix            (CONTEXT);
extern  void            query_privileges        (CONTEXT);
extern  void            query_profile           (CONTEXT);
extern  void            query_prompt            (CONTEXT);
extern  void            query_pronoun           (CONTEXT);
extern  void            query_protect           (CONTEXT);
extern  void            query_quota             (CONTEXT);
extern  void            query_quotalimit        (CONTEXT);
extern  void            query_race              (CONTEXT);
extern  void            query_rand              (CONTEXT);
extern  void            query_rank              (CONTEXT);
extern  void            query_realtime          (CONTEXT);
extern  char            query_replace           (CONTEXT);
extern  void            query_result            (CONTEXT);
extern  void            query_rightstr          (CONTEXT);
extern  void            query_score             (CONTEXT);
extern  void            query_screenheight      (CONTEXT);
extern  void            query_screenwidth       (CONTEXT);
extern  void            query_set               (CONTEXT);
extern  void            query_separator         (CONTEXT);
extern  void            query_size              (CONTEXT);
extern  void            query_sort              (CONTEXT);
extern  void            query_specialroom       (CONTEXT);
extern  void            query_status            (CONTEXT);
extern  void            query_strcase           (CONTEXT);
extern  void            query_strlen            (CONTEXT);
extern  void            query_strpad            (CONTEXT);
extern  void            query_strpos            (CONTEXT);
extern  void            query_strprefix         (CONTEXT);
extern  void            query_succ              (CONTEXT);
extern  void            query_suffix            (CONTEXT);
extern  void            query_tail              (CONTEXT);
extern  void            query_terminaltype      (CONTEXT);
extern  void            query_time              (CONTEXT);
extern  void            query_timediff          (CONTEXT);
extern  void            query_total             (CONTEXT);
extern  void            query_totalactive       (CONTEXT);
extern  void            query_totalidle         (CONTEXT);
extern  void            query_totallogins       (CONTEXT);
extern  void            query_totaltime         (CONTEXT);
extern  void            query_true_or_false     (CONTEXT);
extern  void            query_typeof            (CONTEXT);
extern  void            query_uptime            (CONTEXT);
extern  void            query_version           (CONTEXT);
extern  void            query_weight            (CONTEXT);
extern  void            query_wildcard          (CONTEXT);
extern  void            query_word              (CONTEXT);
extern  void            query_wordno            (CONTEXT);
extern  void            query_www               (CONTEXT);


/* ---->  From request.c  <---- */
extern  time_t          request_add             (char *email,char *name,unsigned long address,const char *host,time_t date,dbref user,unsigned short ref);
extern  void            request_expired         (void);
extern  struct request_data *request_lookup     (const char *reference,unsigned short refno,struct request_data **last);
extern  void            request_view            (dbref player,const char *reference,unsigned short refno);
extern  void            request_list            (dbref player,const char *email,const char *name);
extern  void            request_new             (dbref player);
extern  void            request_accept          (dbref player,const char *reference,const char *name);
extern  void            request_refuse          (dbref player,const char *reference,const char *reason);
extern  void            request_cancel          (dbref player,const char *reference);
extern  void            request_process         (CONTEXT);


/* ---->  From sanity.c  <---- */
extern  dbref           sanity_marklist         (dbref first,dbref object);
extern  void            sanity_general          (dbref player,unsigned char log);
extern  int             sanity_checklists       (dbref player);
extern  int             sanity_fixlists         (dbref player,unsigned char log);
extern  void            sanity_main             (CONTEXT);


/* ---->  From search.c  <---- */
extern  int             parse_objecttype        (const char *str);
extern  int             parse_fieldtype         (const char *str,int *object_type);
extern  int             parse_flagtype          (const char *str,int *object_type,int *mask);
extern  int             parse_flagtype2         (const char *str,int *object_type,int *mask);
extern  const char      *percent                (double numb1,double numb2);
extern  dbref           search_room_by_owner    (dbref owner,dbref location,int level);
extern  void            search_entrances        (CONTEXT);
extern  void            search_find             (CONTEXT);
extern  void            search_list             (CONTEXT);


/* ---->  From selection.c  <---- */
extern  void            selection_log_execlimit (dbref player,char *buffer);
extern  unsigned char   selection_do_keyword    (const char *str);
extern  unsigned char   selection_else_keyword  (const char *str);
extern  unsigned char   selection_skip_do_keyword(char *str);
extern  char            *selection_seek_end     (char *str,int *lineno,unsigned char elsepart,unsigned char casepart);
extern  void            selection_begin         (CONTEXT);
extern  void            selection_breakloop     (CONTEXT);
extern  void            selection_case          (CONTEXT);
extern  void            selection_continue      (CONTEXT);
extern  void            selection_else          (CONTEXT);
extern  void            selection_end           (CONTEXT);
extern  void            selection_if            (CONTEXT);
extern  void            selection_for           (CONTEXT);
extern  void            selection_foreach       (CONTEXT);
extern  void            selection_goto          (CONTEXT);
extern  void            selection_return_break  (CONTEXT);
extern  void            selection_skip          (CONTEXT);
extern  void            selection_test          (CONTEXT);
extern  void            selection_while         (CONTEXT);
extern  void            selection_with          (CONTEXT);


/* ---->  From server.c  <---- */
extern  const char      *emergency_shutdown_message;
extern  struct descriptor_data *descriptor_list;
extern  const char      *shutdown_message;
extern  dbref           current_character;
extern  const char      *current_cmdptr;
extern  char            **output_fmt;
extern  va_list         output_ap;

extern  int             powerstate;
extern  int             powercount;
extern  time_t          powertime;

extern  unsigned char   auto_time_adjust;

extern  time_t          server_gettime          (time_t *tm,int check_tz);
extern  void            server_log_commands     (void);
extern  void 		server_connect_peaktotal(void);
extern  int             server_count_connections(dbref player,unsigned char html);
extern  void            server_sort_descriptor  (struct descriptor_data *d);
#ifdef GUARDIAN_ALARM
extern  void            server_SIGALRM_handler  (int sig);
#endif
extern  void            server_SIGFPE_handler   (int sig);
extern  void            server_SIGCONT_handler  (int sig);
extern  void            server_SIGUSR1_handler  (int sig);
extern  void            server_SIGUSR2_handler  (int sig);
extern  void            server_SIGPWR_handler   (int sig);
extern  void            server_SIGCHLD_handler  (int sig);
extern  int             server_process_output   (struct descriptor_data *d);
extern  int             server_process_input    (struct descriptor_data *d,unsigned char html);
extern  void            server_close_sockets    (void);
extern  void            server_emergency_exception(int sig);
extern  void            server_emergency_dump   (const char *message,int sig);
extern  void            server_signal_handler   (int sig);
extern  void            server_set_signals      (void);
extern  void            server_initialise_telnet(int descriptor);
extern  int             server_open_socket      (int port,unsigned char restart,unsigned char refresh,unsigned char html,unsigned char logtime);
extern  void            server_process_commands (void);
extern  void            server_mainloop         (void);
extern  void            server_set_echo         (struct descriptor_data *d,int echo);
extern  int             server_unconditionally_banned(struct site_data *site,int sock,char *buffer,unsigned char ctype);
extern  struct descriptor_data *server_initialise_sock(int new,struct sockaddr_in *a,struct site_data *site,unsigned char ctype);
extern  struct descriptor_data *server_new_connection(int sock,unsigned char ctype);
extern  void            server_clear_textqueue  (struct text_queue_data *queue);
extern  void            server_clear_strings    (struct descriptor_data *d);
extern  unsigned char   server_queue_input      (struct descriptor_data *d,const char *str,int len);
extern  int             server_queue_output     (struct descriptor_data *d,const char *str,int len);
extern  void            server_shutdown_sock    (struct descriptor_data *d,unsigned char boot,unsigned char keepalive);
extern  int             server_connection_allowed(struct descriptor_data *d,char *buffer,dbref user);
extern  unsigned char   server_site_banned      (dbref user,const char *name,const char *email,struct descriptor_data *d,int connect,char *buffer);
extern  int             server_connect_user     (struct descriptor_data *d,const char *input);
extern  void            server_current_last     (struct descriptor_data *d,char *command);
extern  int             server_command          (struct descriptor_data *d,char *command);


/* ---->  From serverinfo.c  <---- */
#ifdef SERVERINFO

/* ---->  Function prototypes  <----- */
extern 	void 		serverinfo		(void);
extern	const char 	*getserverinfo		(const char *suppliedname, unsigned long *ipaddr, unsigned long *network, unsigned long *netmask);
#endif /* SERVERINFO */


/* ---->  From set.c  <---- */
extern  struct alias_data *alias_lookup         (dbref player,const char *command,unsigned char inherit,unsigned char global);
extern  void            set_afk                 (CONTEXT);
extern  void            set_areaname            (CONTEXT);
extern  void            set_cstring             (CONTEXT);
extern  void            set_datetimeformat      (CONTEXT);
extern  void            set_description         (CONTEXT);
extern  void            set_drop                (CONTEXT);
extern  void            set_email               (CONTEXT);
extern  void            set_email_publicprivate (CONTEXT);
extern  void            set_estring             (CONTEXT);
extern  void            set_expiry              (CONTEXT);
extern  void            set_fail                (CONTEXT);
extern  void            set_feeling             (CONTEXT);
extern  int             set_flag_by_name        (dbref player,dbref thing,const char *flagname,int reset,const char *reason,unsigned char write,int fflags);
extern  void            set_flag                (CONTEXT);
extern  void            set_key                 (CONTEXT);
extern  void            set_link                (CONTEXT);
extern  void            set_lock                (CONTEXT);
extern  void            set_mass                (CONTEXT);
extern  void            set_name                (CONTEXT);
extern  void            set_odesc               (CONTEXT);
extern  void            set_odrop               (CONTEXT);
extern  void            set_ofail               (CONTEXT);
extern  void            set_osucc               (CONTEXT);
extern  void            set_owner               (CONTEXT);
extern  void            set_parent              (CONTEXT);
extern  void            set_password            (CONTEXT);
extern  void            set_preferences         (CONTEXT);
extern  void            set_prefix              (CONTEXT);
extern  void 		set_profile_init	(dbref player);
extern  void            set_profile             (CONTEXT);
extern  void            set_race                (CONTEXT);
extern  void            set_score               (CONTEXT);
extern  void            set_screenconfig        (CONTEXT);
extern  void            set_succ                (CONTEXT);
extern  void            set_suffix              (CONTEXT);
extern  void            set_unlink              (CONTEXT);
extern  void            set_unlock              (CONTEXT);
extern  void            set_volume              (CONTEXT);
extern  void            set_www                 (CONTEXT);


/* ---->  From sites.c  <---- */
extern  struct          site_data               *sitelist;

extern  unsigned long    text_to_ip             (const char *address,unsigned long *mask);
extern  const char       *ip_to_text            (unsigned long address,unsigned long mask,char *buffer);
extern  void             site_process           (CONTEXT);
extern  unsigned char    register_sites         (void);
extern  struct site_data *lookup_site           (struct in_addr *a,char *buffer);


/* ---->  From statistics.c  <---- */
extern  const char      *stats_percent          (double numb1,double numb2);
extern  void            stats_tcz_update_record (int peak,int charconnected,int charcreated,int objcreated,int objdestroyed,int shutdowns,time_t now);
extern  void            stats_tcz_update        (void);
extern  void            stats_tcz               (dbref player);
extern  void            stats_character         (dbref player);
extern  void            stats_others            (dbref player,const char *name);
extern  void            stats_bandwidth         (dbref player);
extern  void            stats_command           (dbref player);
extern  void            stats_resource          (dbref player);
extern  void            stats_connections       (dbref player);
extern  void            stats_statistics        (CONTEXT);
extern  void            stats_quota             (CONTEXT);
extern  void            stats_size              (CONTEXT);
extern  void            stats_contents          (CONTEXT);
extern  void            stats_rank_traverse     (struct rank_data *current);
extern  void            stats_rank              (CONTEXT);


/* ---->  From stringutils.c  <---- */
extern  void            tilde_string            (dbref player,const char *str,const char *ansi1,const char *ansi2,unsigned char spaces,unsigned char cr,unsigned char fsize);
extern  const char      *construct_message      (dbref player,const char *ansi1,const char *ansi2,const char *defaction,char defpunct,char pose,char autoaction,const char *message,unsigned char tell,unsigned char article_setting);
extern  const char      *privilege_countcolour  (dbref player,short *deities,short *elders,short *delders,short *wizards,short *druids,short *apprentices,short *dapprentices,short *retired,short *dretired,short *experienced,short *assistants,short *builders,short *mortals,short *beings,short *puppets,short *morons);
extern  const char      *privilege_colour       (dbref player);
extern  void            progress_meter          (unsigned long total,int divisor,unsigned char units,unsigned char init);
extern  const char      *gettextfield           (int number,char separator,const char *list,int skip,char *buffer);
extern  const char      *settextfield           (const char *text,int number,char separator,const char *list,char *buffer);
extern  int             strlen_ansi             (const char *str);
extern  const char      *filter_ansi            (const char *str,char *buffer);
extern  void            init_strops             (struct str_ops *str_data,char *src,char *dest);
extern  unsigned char   strins_limits           (struct str_ops *str_data,const char *str);
extern  unsigned char   strcat_limits           (struct str_ops *str_data,const char *str);
extern  unsigned char   strcat_limits_char      (struct str_ops *str_data,const char chr);
extern  unsigned char   strcat_limits_exact     (struct str_ops *str_data,const char *str);
extern  int             strlen_ignansi          (const char *src);
extern  char            *strcpy_ignansi         (char *dest,const char *src,int *length,int *rlength,int *copied,int maxlen);
extern  const char      *skip_article           (const char *str,unsigned char article_setting,unsigned char definite);
extern  char            *punctuate              (char *message,int no_quotes,char punctuation);
extern  char	         *format_commas	   	   (char *message);
extern  int		         isnumber	               (char *message);
extern  char            *separator              (int scrwidth,int trailing_cr,const char ch1,const char ch2);
extern  const char      *bad_language_filter    (char *dest,const char *src);
extern  const char      *truncatestr            (char *dest,const char *str,unsigned char nonansi,int length);
extern  const char      *ansi_code_filter       (char *dest,const char *src,unsigned char ansi);
extern  const char      *rank                   (int value);
extern  int             strcnt                  (const char *str,char c);
extern  int             strpos                  (const char *str,const char c);
extern  const char      *strpad                 (char pad,int len,char *buffer);
extern  void            split_params            (char *str,char **arg1,char **arg2);
extern  const char      *filter_spaces          (char *dest,const char *src,int capitalise);
extern  unsigned char   keyword                 (const char *kword,char *text,char **trailing);
extern  int             match_range             (const char *str,const char *rangefrom,const char *rangeto,int prefix);
extern  unsigned char   match_wildcard          (const char *wildcard,const char *str);
extern  const char      *match_wildcard_list    (const char *wildcard,char separator,const char *list,char *buffer);
extern  int             string_compare          (const char *string1,const char *string2,int minimum);
extern  int             string_matched_list     (const char *str,const char *list,const char separator,int word);
extern  int             string_matched_prefix   (const char *match,const char *string);
extern  int             string_prefix           (const char *string,const char *prefix);
extern  const char      *string_match           (const char *src,const char *sub);
extern  int             instring                (const char *substr,const char *string);
extern  const char      *pose_string            (char **str,const char *punct);
extern  const char      *binary_to_ascii        (unsigned char *str,int len,char *buffer);
extern  int             digit_wrap              (int offset,int value);
extern  double          tofloat                 (const char *str,char *dps);
extern  int             tohex                   (const char *str);
extern  long            parse_time              (char *str);
extern  const char      *date_to_string         (time_t datetime,unsigned long longdate,dbref player,const char *defaultformat);
extern  const char      *interval               (unsigned long interval,int negative,unsigned char entities,unsigned char shortened);
extern  unsigned char   leapyear                (time_t date);
extern  time_t          string_to_date          (dbref player,const char *str,unsigned char epoch,unsigned char timedate,unsigned char *invalid);
extern  time_t          longdate_to_epoch       (unsigned long longdate);
extern  unsigned long   epoch_to_longdate       (time_t epoch);
extern  unsigned long   longdate_difference     (unsigned long longdate1,unsigned long longdate2);


/* ---->  From substitute.c  <---- */
extern  unsigned char   substitute_allowable_command(char *command_string);
extern  void            substitute_sub_command  (dbref player,struct str_ops *str_data);
extern  void            substitute_command      (dbref player,char *src,char *dest);
extern  void            substitute_sub_variable (dbref player,struct str_ops *str_data);
extern  void            substitute_variable     (dbref player,char *src,char *dest);
extern  void            substitute_query        (dbref player,struct str_ops *str_data);
extern  const char      *substitute             (dbref player,char *dest,char *src,unsigned char addname,const char *def_ansi,struct substitution_data *subst,dbref sender);
extern  void            substitute_large        (dbref player,dbref who,const char *str,const char *def_ansi,char *buffer,unsigned char censor);


/* ---->  From tcz.c  <---- */
extern  void            tcz_version             (struct descriptor_data *d,int console);
extern  unsigned char   tcz_db_exists           (const char *filename,unsigned char panic_db,unsigned char compressed_db);
extern  void            tcz_db_copy             (const char *srcfile,const char *destfile,unsigned char site);
extern  void            tcz_exec_copy           (const char *execfile);
extern  void            tcz_check_directories   (void);
extern  void            tcz_check_lockfile      (void);
extern  int             tcz_get_timezone        (int log,int restart);
extern  void            tcz_startup_shutdown    (const char *name,const char *logfile);
extern  void            tcz_get_operatingsystem (void);
extern  int             main                    (int argc,char **argv);


/* ---->  From temp.c  <---- */
extern  struct temp_data *temp_lookup           (const char *name);
extern  void            temp_describe           (CONTEXT);
extern  void            temp_readd              (struct temp_data *ptr);
extern  void            temp_destroy            (CONTEXT);
extern  void            temp_clear              (struct temp_data **tmp,struct temp_data *newtmp);


/* ---->  From termcap.c  <---- */
extern  unsigned char   set_terminal_type       (struct descriptor_data *d,char *termtype,unsigned char log);
extern  void            clear_termcap           (void);
extern  unsigned char   reload_termcap          (unsigned char reload);


/* ---->  From unparse.c  <---- */
extern  char            boolexp_buf[];
extern  struct str_ops  exp_data;

extern  const char      *unparse_flaglist       (dbref object,unsigned char permission,char *buffer);
extern  char            *unparse_flags          (dbref object);
extern  const char      *unparse_object         (dbref player,dbref object,unsigned char article_setting);
extern  const char      *getnameid              (dbref player,dbref object,char *buffer);
extern  char            *unparse_boolexp        (dbref player,struct boolexp *b,int for_return);
extern  void            unparse_parameters      (char *str,unsigned char pcount,struct arg_data *arg,unsigned char keywords);
extern  const char      *getname                (dbref object);
extern  const char      *getname_prefix         (dbref player,int limit,char *buffer);
extern  const char      *getcname               (dbref player,dbref object,unsigned char unparse,unsigned char article_setting);
extern  const char      *forwarding_address     (dbref player,unsigned char tail,char *buffer);
extern  const char      *getexit_firstname      (dbref player,dbref exit,unsigned char unparse);
extern  const char      *getexitname            (dbref player,dbref exit);


/* ---->  From userlist.c  <---- */
extern  const char      *userlist_users         (dbref player,const char *listed,short deities,short elders,short delders,short wizards,short druids,short apprentices,short dapprentices,short retired,short dretired,short experienced,short assistants,short builders,short mortals,short beings,short puppets,short morons,short idle,unsigned char space,unsigned char html);
extern  const char      *userlist_peak          (unsigned char space,unsigned char html);
extern  const char      *userlist_uptime        (long total,unsigned char space,unsigned char html);
extern  const char      *userlist_shorttime     (time_t time,time_t now,char *buffer,unsigned char spod);
extern  void            userlist_who            (struct descriptor_data *d);
extern  void 		userlist_swho		(struct descriptor_data *d);
extern  void            userlist_hosts          (struct descriptor_data *d);
extern  void            userlist_where          (struct descriptor_data *d,int lwho);
extern  void            userlist_email          (struct descriptor_data *d,int number);
extern  void            userlist_last           (struct descriptor_data *d);
extern  void            userlist_channels       (struct descriptor_data *d);
extern  void            userlist_session        (struct descriptor_data *d);
extern  void            userlist_assist         (struct descriptor_data *d);
extern  void            userlist_admin          (struct descriptor_data *d,unsigned char dsc);
extern  void            userlist_query_title    (CONTEXT);
extern  void            userlist_view           (CONTEXT);
extern  void            userlist_set_title      (CONTEXT);
extern  void            userlist_title          (CONTEXT);


/* ---->  From yearlyevents.c  <---- */
extern  struct          yearly_event_data       *yearly_event_start;

extern  int		yearly_event_sort	(dbref player);
extern  void            yearly_event_show       (dbref player,unsigned char timed);
extern  void            yearly_event_list 	(CONTEXT);

#endif /* __EXTERNS_H */
