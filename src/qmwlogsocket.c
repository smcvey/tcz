/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| QMWLOGSOCKET.C -  Log to a unix domain socket (QMW Research)                |
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
| Module originally designed and written by:  Simon A. Boggis 20/08/2001.     |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

$Id: qmwlogsocket.c,v 1.1.1.1 2004/12/02 17:42:14 jpboggis Exp $

*/

/* TCZ config include file (Required at this point for QMW_RESEARCH #define) */
#include "config.h"

#ifdef QMW_RESEARCH

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

/* socket stuff */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <fcntl.h> /* fcntl */

#include <sys/timeb.h> /* for ftime(3) */

#include <time.h> /* for strftime(3) */

#include <limits.h> /* for ULONG_MAX */

#include <stdarg.h> /* for send_logmsg() */

/* TCZ include files */
#include "logfiles.h"
#include "externs.h"
#include "db.h"

/* Path to log unix socket */
#define LOGSOCKETPATH "/tmp/udsvrd.sock"

/* Default facility for send_logmsg() */
#define LOGSOCKETFACILITY 0

/* buffersizes */
#define PAGESIZE (getpagesize())
#define BUFSIZE (4 * PAGESIZE)
#define MAXBUFSIZE (256 * PAGESIZE)
#define CONST_PAGESIZE 4096
#define CONST_BUFSIZE (4* CONST_PAGESIZE) /* 65536 */
#define CONST_MAXBUFSIZE (256 * CONST_PAGESIZE) /* max size bufs can grow to */

/* max atomic size on Linux 2.4.5 seems to be 65536 - 33 = 65503 */
#define MAXATOMICSIZE 65504

/* logmsg stuff */
#define LMFATAL   0
/* #define LMERROR   1 */
#define LMERROR   4 /* don't want lots of msgs when receiver is down */
#define LMWARNING 2
#define LMNORMAL  3
#define LMMEDIUM  4
#define LMMINOR   5
int loglvl=LMNORMAL;

/* string to describe the module in tcz writelog() */
#define MODDESC "QMW LOG SOCKET UNIX"

/* BOOL type */
typedef int BOOL;
#define TRUE 1
#define FALSE 0

/* private to this file */ 
/* for stdarg in mkmsg */
static char *mkmsg_fmt = NULL;
static va_list mkmsg_ap;

/* private to this file */ 
/* loglevels */
static const char *loglevels[] = { "FATAL  ",
				   "ERROR  ",
				   "WARNING",
				   "NORMAL ",
				   "MEDIUM ",
				   "MINOR  " };

/* use gcc attribute to check varargs fns */
#ifdef __GNUC__
#define VARARGS_ATTRIBUTE(a1, a2) __attribute__ ((format (__printf__, a1, a2)))
#else
#define VARARGS_ATTRIBUTE(a1, a2)
#endif

/* function prototypes */

/* supposed to be private to this file */
char *mkmsg(const char *fmt, ...) VARARGS_ATTRIBUTE(1,2);
int logmsg(int lvl, const char *fmt, ...) VARARGS_ATTRIBUTE(2,3);
int unix_socket_send(const char *p_socketname, 
		     const char *p_message, size_t size);
int send_logmsg(const char *p_socketname,
		unsigned long p_invoc_serial,
		unsigned long p_log_serial,
		unsigned long p_facility,
		const char *fmt,...) VARARGS_ATTRIBUTE(5,6);

/* public */
int qmwlogsocket(const char *fmt,...) VARARGS_ATTRIBUTE(1,2);

/* private to this file */
char *mkmsg(const char *fmt, ...) {
  /*
   * Function to dynamically allocate a buffer and vsnprintf() a message into
   * it, returning the buffer:
   *
   * Returns a char * that YOU must free() when you are done with it. We
   * don't declare this as const char * because its your memory
   * now and you can do as you wish with it.
   *
   * There is a problem recursively calling functions with variadic arguments
   * (see man stdarg), i.e. calling a variadic function between the
   * va_start() and va_end() macros, and this often occurs where a function
   * with variadic args wants to call mkmsg(). We therefore we adopt this 
   * aproach:
   *
   * (1) If the argument char *fmt is NOT NULL, we process normally, calling
   *     va_start etc ourselves.
   *
   * (2) If the argument char *fmt is NULL, we look at the global (in mkmsg.h)
   *     mkmsg_fmt and mkmsg_ap and pass these to our vsnprintf call. YOU must
   *     call va_start(mkmsg_ap,YOUR_fmt_arg) and set mkmsg_fmt = YOUR_fmt_arg
   *     before calling mkmsg(). After the call you must call va_end(mkmsg_ap).
   *
   * NOTE: don't call this with less than the fmt + one arg (see man stdarg)
   * -  use (fmt,NULL) if necessary, for example. 
   * Returns NULL on error.
   *
   * You MUST free the memory for this function.
   */
  char fname[]="mkmsg";
  char *p = NULL;
  /* Use long instead of size_t to allow return of -1 from vsnprintf() */
  static long current_size = 0;
  
  /* allocate */
  if ((p = (char *)malloc (BUFSIZE)) == NULL) {
    fprintf(stderr,"%s: ERROR: cannot malloc buffer size %d: %d: %s",
	    fname, BUFSIZE, errno, strerror(errno));
    return NULL;
  }
  current_size = BUFSIZE;

  while (1) {
    int n;
    /* Use long instead of size_t to allow return of -1 from vsnprintf() */
    size_t new_size;

    /* Try to print in the allocated space. */
    if (fmt != NULL) { /* using our arguments */
      va_list ap;

      va_start(ap, fmt);
      n = vsnprintf (p, current_size, fmt, ap);
      va_end(ap);
    } else { /* using global args from mkmsg.h */
      if (mkmsg_fmt == NULL) {
        fprintf(stderr,
		"%s: ERROR: called with fmt=NULL, but global fmt is also NULL",
		fname);
        FREENULL(p);
        return(NULL);
      }
      n = vsnprintf (p, current_size, mkmsg_fmt, mkmsg_ap);
    }

    /* If that worked, return the string. */
    if (n > -1 && n < current_size)
      return p;

    new_size = current_size;
    /* Else try again with more space. */
    if (n > -1) {     /* glibc 2.1 */
      new_size = n+1; /* precisely what is needed */
    } else {          /* glibc 2.0 */
      new_size *= 2;  /* twice the old size */
    }

    if (new_size > (size_t)MAXBUFSIZE) {
      new_size = MAXBUFSIZE; /* limit our expansion */
      fprintf(stderr,
	      "%s: WARNING: (fmt arg='%s') cannot expand buffer (size %ld) beyond hard limit (MAXBUFSIZE) of %d; truncating message to maximum size %d",
	      fname,
	      (fmt == NULL)? mkmsg_fmt : fmt,
	      current_size, MAXBUFSIZE, new_size);
    }

    if ((p=(char *)realloc(p,new_size)) == NULL) {
      fprintf(stderr,"%s: ERROR: cannot realloc buffer from %ld to %d: %s",
	      fname, current_size, new_size, strerror(errno));
      FREENULL(p);
      return(NULL);
    } else {
      current_size = new_size;
      fprintf(stderr,"%s: buffer increased from %ld to %d",
	      fname, current_size, new_size);
    }
  }
  return p;
}

/* private to this file */
int logmsg(int lvl, const char *fmt, ...) {
  /* 
   * function to log using tcz writelog 
   */
  char fname[]="logmsg";

  if (lvl <= loglvl) {
    char *outfmt = NULL;

    /*
     * assemble the error message
     *
     * This is not a real branch instruction - just an error handler
     *
     * we add a '\n' to make sure we have enough room for one
     * in case user didn't put one there - see below.
     */
    if ((outfmt=mkmsg("%s(%d): %s\n", loglevels[lvl], lvl, fmt)) == NULL) {
      fprintf(stderr,"%s: ERROR: assembling output format with mkmsg(): %d: %s ",
	      fname, errno, strerror(errno));
      fprintf(stderr,"%s: ERROR: message would have been %s(%d): %s (other args lost)",
	      fname, loglevels[lvl], lvl, fmt);
    } else {
      char *msg = NULL;
      char *p = NULL;

      /* sanitise end of message - we want end to be exactly one "\n" */
      p = outfmt + strlen(outfmt) -1;      
      while (*p == '\r' || *p == '\n' || *p == ' ' || *p == '\t') {
	*p = '\0';
	p--;
      }
      *(p+1)='\n';
      
      /* prepare message */
      /* use global mkmsg vars to pass args */
      mkmsg_fmt = outfmt;
      va_start(mkmsg_ap, fmt);
      msg = mkmsg(NULL);
      va_end(mkmsg_ap);
      FREENULL(outfmt);
      /* reset global var for next call */
      mkmsg_fmt = NULL;
      
      if (msg == NULL) {
	writelog(RESEARCH_LOG,0,MODDESC,
		 "logmsg: ERROR: preparing output with mkmsg() failed");
	return(-1);
      }

      writelog(RESEARCH_LOG,0,MODDESC,"%s",msg);
      FREENULL(msg);
    }
  }

  return(0);
}

/* private to this file */
int unix_socket_send(const char *p_socketname, 
		     const char *p_message, size_t size){
  /*
   * send a formatted message to a unix socket
   *
   * returns 0 on success, -1 on error (check errno for details)
   */
  char fname[]="unix_socket_send";
  int count;
  int sock;
  struct sockaddr_un sun;
  socklen_t addrlen;

  if (size > MAXATOMICSIZE) {
    logmsg(LMWARNING,"%s: message length %d > %d (MAXATOMICSIZE)",
	   fname, size, MAXATOMICSIZE);
  }

  logmsg(LMMINOR,"%s: sending message (%d bytes) '%s'\n",
	 fname, size, p_message);

  sock = socket(PF_UNIX, SOCK_DGRAM, 0);
  if (sock < 0) {
    logmsg(LMERROR,"%s: ERROR: socket() PF_UNIX SOCK_DGRAM failed: %d: %s\n",
           fname, errno, strerror(errno));
    return(-1);
  }
  logmsg(LMMINOR,"%s: fd=%d socket PF_UNIX SOCK_DGRAM open\n",
         fname, sock);

  /* check path against UNIX_PATH_MAX = 108 */
#define UNIX_PATH_MAX 108
  sun.sun_family = AF_UNIX;
  strncpy(sun.sun_path, p_socketname, UNIX_PATH_MAX);
  addrlen = sizeof(struct sockaddr_un);

  do {
    count = sendto(sock, p_message, size, MSG_DONTWAIT,
		   (struct sockaddr *) &sun, addrlen);
  } while (count < 0 && (errno == EINTR));
  if (count == -1) {
    switch (errno) {
    case EAGAIN:
      /* send would block (we're using nonblocking IO) */
      logmsg(LMERROR,"%s: ERROR: fd=%d: send(): would block: %d: %s\n",
	     fname,sock,errno,strerror(errno));
      break;
    case EMSGSIZE:
      /*  The  socket  requires  that  message be sent atomi­ */
      /*  cally, and the size of the message to be sent  made */
      /* this impossible. */
      logmsg(LMERROR,"%s: ERROR: fd=%d: send(): msg too big for atomic send: %d: %s\n",
	     fname,sock,errno,strerror(errno));	
      break;
    case ENOBUFS:
      /* The output queue for a network interface was  full. */
      /* This  generally  indicates  that  the interface has */
      /* stopped sending, but may  be  caused  by  transient */
      /* congestion.   (This  cannot occur in Linux, packets */
      /* are just silently dropped when a device queue over­ */
      /* flows.) */
      logmsg(LMERROR,"%s: ERROR: fd=%d: send(): msg queue full: %d: %s\n",
	     fname,sock,errno,strerror(errno));	
      break;
    case ENOMEM:
      logmsg(LMERROR,"%s: ERROR: fd=%d: send(): no memory available: %d: %s\n",
	     fname,sock,errno,strerror(errno));	
      break;
    case ECONNREFUSED:
      logmsg(LMERROR,"%s: ERROR: fd=%d: send(): connection refused: %d: %s\n",
	     fname,sock,errno,strerror(errno));
      break;
    case EPIPE:
      /* The  local  end  has been shut down on a connection */
      /* oriented socket.  In this  case  the  process  will */
      /* also  receive a SIGPIPE unless MSG_NOSIGNAL is set. */
      logmsg(LMERROR,"%s: ERROR: fd=%d: send(): peer has closed connection: %d: %s\n",
	     fname,sock,errno,strerror(errno));
      break;
    default:
      logmsg(LMERROR,"%s: ERROR: fd=%d: send(): %d: %s\n",
	     fname,sock,errno,strerror(errno));
    }
    close(sock);
    return(-1);
  }
  logmsg(LMMINOR,"%s: fd=%d sent %d bytes to AF_UNIX socket '%s'\n",
	 fname, sock, count, p_socketname);
  
  close(sock);
  
  return 0;
}

/* private to this file */
int send_logmsg(const char *p_socketname,
		unsigned long p_invoc_serial,
		unsigned long p_log_serial,
		unsigned long p_facility,
		const char *fmt,...) {
  /*
   * format a message and send it using unix_socket_send()
   *
   * returns 0 on success, -1 on error 
   */
  char fname[]="send_logmsg";
  struct timeb tmb;
  size_t size, tm_sz, mt_sz, is_sz, ls_sz, f_sz, m_sz;
  char *buf=NULL; /* buffer for packing message into */
  char date822[CONST_BUFSIZE]; /* buffer for holding RFC822 fmt date */
  char *ptr = NULL;
  char *msg = NULL;
  static unsigned long lastlogserial=0;   /* keep track of last log serial */
  static unsigned long lastinvocserial=0; /* keep track of last invoc serial */

  /* have we missed any messages? */
  if (lastinvocserial != 0
      && p_invoc_serial == lastinvocserial
      && lastlogserial != 0
      && (p_log_serial-lastlogserial) > 1) {
    logmsg(LMWARNING,
	   "%s: missed %lu messages (last msg %lu:%lu curr msg %lu:%lu)",
	   fname, (p_log_serial-lastlogserial-1),
	   lastinvocserial, lastlogserial,
	   p_invoc_serial, p_log_serial);
  }
  /* save last invoc and log serial numbers for check */
  lastinvocserial = p_invoc_serial;
  lastlogserial = p_log_serial;

  /* prepare message */
  /* use global mkmsg vars to pass args */
  mkmsg_fmt = (char *)fmt;
  va_start(mkmsg_ap, fmt);
  msg = mkmsg(NULL);
  va_end(mkmsg_ap);
  /* reset global var for next call */
  mkmsg_fmt = NULL;

  if (msg == NULL) {
    logmsg(LMERROR,"%s: preparing output with mkmsg() failed", fname);
    return(-1);
  }

  tm_sz = sizeof(time_t);
  mt_sz = sizeof(unsigned short);
  is_sz = ls_sz = f_sz = sizeof(unsigned long);
  m_sz = (strlen(msg) + 1) * sizeof(char); /* +1 for '\0' */

  size = tm_sz + mt_sz + is_sz + ls_sz + f_sz + m_sz;

  buf=(char *)malloc(size);
  if (buf == NULL) {
    logmsg(LMERROR,"%s: ERROR: malloc message send buffer failed: %d: %s\n",
           fname, errno, strerror(errno));
    FREENULL(msg);
    return(-1);
  }
  memset(buf,'\0',size);

  /* get the current time */
  ftime(&tmb); /* always succeeds */

  /* set up message send buffer */
  ptr = buf;
  memcpy(ptr,&(tmb.time), tm_sz); ptr+=tm_sz;
  memcpy(ptr,&(tmb.millitm), mt_sz); ptr+=mt_sz;
  memcpy(ptr,&(p_invoc_serial), is_sz); ptr+=is_sz;
  memcpy(ptr,&(p_log_serial), ls_sz); ptr+=ls_sz;
  memcpy(ptr,&(p_facility), f_sz); ptr+=f_sz;
  memcpy(ptr,msg, m_sz); ptr+=m_sz;

  /* convert date to string for logging */
  strftime(date822,CONST_BUFSIZE,"%a, %d %b %Y %H:%M:%S %z",
	   localtime(&(tmb.time)));

  /* now send the message */
  if (unix_socket_send(p_socketname, buf, size) < 0) {
    logmsg(LMERROR,"%s: ERROR: send message failed: to '%s' at %s invoc=%lu serial=%lu facility=%lu: '%s'\n",
	   fname,
	   p_socketname, date822,
	   p_invoc_serial, p_log_serial, p_facility, 
	   msg); 
    FREENULL(buf);
    FREENULL(msg);
    return(-1);
  } else { 
    logmsg(LMMINOR,"%s: sent message OK: to '%s' at %s invoc=%lu serial=%lu facility=%lu: '%s'\n",
	   fname,
	   p_socketname, date822,
	   p_invoc_serial, p_log_serial, p_facility, 
	   msg);
    FREENULL(buf);
    FREENULL(msg);
    return(0);
  }
}

int qmwlogsocket(const char *fmt,...) {
  /*
   * format a message and send it using unix_socket_send()
   *
   * Same code as send_logmsg() but with parameters hard-wired to TCZ's
   *
   * returns 0 on success, -1 on error 
   */
  char fname[]="send_logmsg";
  struct timeb tmb;
  size_t size, tm_sz, mt_sz, is_sz, ls_sz, f_sz, m_sz;
  char *buf=NULL; /* buffer for packing message into */
  char date822[CONST_BUFSIZE]; /* buffer for holding RFC822 fmt date */
  char *ptr = NULL;
  char *msg = NULL;

  /* hard-wired parameters */
  char *p_socketname = LOGSOCKETPATH;
  unsigned long p_invoc_serial = restart_serial_no;
  unsigned long p_log_serial = log_serial_no;
  unsigned long p_facility = LOGSOCKETFACILITY;

  static unsigned long lastlogserial=0;   /* keep track of last log serial */
  static unsigned long lastinvocserial=0; /* keep track of last invoc serial */

  /* we've got current log serial no., so can now increment TCZ's */
  log_serial_no++;

  /* have we missed any messages? */
  if (lastinvocserial != 0
      && p_invoc_serial == lastinvocserial
      && lastlogserial != 0
      && (p_log_serial-lastlogserial) > 1) {
    logmsg(LMWARNING,
	   "%s: missed %lu messages (last msg %lu:%lu curr msg %lu:%lu)",
	   fname, (p_log_serial-lastlogserial-1),
	   lastinvocserial, lastlogserial,
	   p_invoc_serial, p_log_serial);
  }
  /* save last invoc and log serial numbers for check */
  lastinvocserial = p_invoc_serial;
  lastlogserial = p_log_serial;
  
  /* prepare message */
  /* use global mkmsg vars to pass args */
  mkmsg_fmt = (char *)fmt;
  va_start(mkmsg_ap, fmt);
  msg = mkmsg(NULL);
  va_end(mkmsg_ap);
  /* reset global var for next call */
  mkmsg_fmt = NULL;

  if (msg == NULL) {
    logmsg(LMERROR,"%s: preparing output with mkmsg() failed", fname);
    return(-1);
  }

  tm_sz = sizeof(time_t);
  mt_sz = sizeof(unsigned short);
  is_sz = ls_sz = f_sz = sizeof(unsigned long);
  m_sz = (strlen(msg) + 1) * sizeof(char); /* +1 for '\0' */

  size = tm_sz + mt_sz + is_sz + ls_sz + f_sz + m_sz;

  buf=(char *)malloc(size);
  if (buf == NULL) {
    logmsg(LMERROR,"%s: ERROR: malloc message send buffer failed: %d: %s\n",
           fname, errno, strerror(errno));
    FREENULL(msg);
    return(-1);
  }
  memset(buf,'\0',size);

  /* get the current time */
  ftime(&tmb); /* always succeeds */

  /* set up message send buffer */
  ptr = buf;
  memcpy(ptr,&(tmb.time), tm_sz); ptr+=tm_sz;
  memcpy(ptr,&(tmb.millitm), mt_sz); ptr+=mt_sz;
  memcpy(ptr,&(p_invoc_serial), is_sz); ptr+=is_sz;
  memcpy(ptr,&(p_log_serial), ls_sz); ptr+=ls_sz;
  memcpy(ptr,&(p_facility), f_sz); ptr+=f_sz;
  memcpy(ptr,msg, m_sz); ptr+=m_sz;

  /* convert date to string for logging */
  strftime(date822,CONST_BUFSIZE,"%a, %d %b %Y %H:%M:%S %z",
	   localtime(&(tmb.time)));

  /* now send the message */
  if (unix_socket_send(p_socketname, buf, size) < 0) {
    logmsg(LMERROR,"%s: ERROR: send message failed: to '%s' at %s invoc=%lu serial=%lu facility=%lu: '%s'\n",
	   fname,
	   p_socketname, date822,
	   p_invoc_serial, p_log_serial, p_facility, 
	   msg); 
    FREENULL(buf);
    FREENULL(msg);
    return(-1);
  } else { 
    logmsg(LMMINOR,"%s: sent message OK: to '%s' at %s invoc=%lu serial=%lu facility=%lu: '%s'\n",
	   fname,
	   p_socketname, date822,
	   p_invoc_serial, p_log_serial, p_facility, 
	   msg);
    FREENULL(buf);
    FREENULL(msg);
    return(0);
  }
}

#endif /* #ifdef QMW_RESEARCH */
