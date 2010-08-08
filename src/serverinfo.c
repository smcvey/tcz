/*

.-----------------------------------------------------------------------------.
| The Chatting Zone (TCZ)                            (C) J.P.Boggis 1993-2004 |
| ~~~~~~~~~~~~~~~~~~~~~~~                            ~~~~~~~~~~~~~~~~~~~~~~~~ |
|-----------------------------------------------------------------------------|
| SERVERINFO.C  -  Code for automagically figuring out various                |
|                  network addresses and reading your kernel network          |
|                  configuration will be incorporated, so that you no-longer  |
|                  have to specify these at start up.                         |
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
| Module originally designed and written by:  Simon A. Boggis 27/02/2000.     |
|-------------------------[ The Chatting Zone (TCZ) ]-------------------------|
|                For more information about TCZ, please visit:                |
|                   http://www.sourceforge.net/projects/tcz                   |
`-----------------------------------------------------------------------------'

  $Id: serverinfo.c,v 1.2 2005/01/25 19:11:42 tcz_monster Exp $

*/


#include <stdio.h>
#include <stdlib.h>

/* START - getserverinfo() only */
#include <string.h>
#include <sys/utsname.h>
#include <netdb.h>
/* END - getserverinfo() only */

/* START - for get_interfaces() */
#include <unistd.h> /* for close */
#include <string.h> /* for strncpy, memset, memcpy */
#include <errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h> /* for inet_ntoa */
#include <arpa/inet.h> /* ditto */
#ifdef SUNOS
#include <memory.h>
/* These don't seem to be needed */
/*
#include <sys/uio.h>
#include <sys/mbuf.h>
*/
#endif /* SUNOS */
#ifdef SOLARIS
#include <sys/sockio.h>
#endif /* SOLARIS */

/* TCZ include files */
#include "logfiles.h"
#include "externs.h"
#include "config.h"
#include "db.h"


#ifdef SERVERINFO

/* Struct returned by get_interfaces() */
struct if_list {
  char name[IFNAMSIZ];
  int flags;            /* complete flags value */
  int family;           /* addr,mask,dest,bcast are ONLY valid for AF_INET */
                        /* for other families these values will be set to 0 */
  struct in_addr addr;
  struct in_addr mask;
  struct in_addr dest;  /* 0 if not Point-to-Point */
  struct in_addr bcast; /* 0 if broadcast not available */
};
/* END - for get_interfaces() */

/* Function Prototypes - internal to this modules */
const char      *s_addrtoa(unsigned long address); /* used for printing addresses */
char            *sab_ntoa(struct in_addr addr); /* used in getserverinfo () only */
int             get_interfaces(struct if_list **list);

void serverinfo(void)
{
  /*
   * Author:
   * -------
   * Simon A. Boggis, Feb 2000
   *
   * Synopsis:
   * ---------
   * Sets TCZ server parameters or EXITS on failure, closing the logs. If
   * the parameters differ from the compiled-in defaults the function assumes
   * you set them manually and leaves them alone. 
   *
   * Arguments
   * ---------
   * none
   *
   * Return values:
   * --------------
   * none
   *
   * Functions called by this:
   * -------------------------
   *   s_addrtoa()                      
   *   getserverinfo()
   */

  char          buf[TEXT_SIZE], domainname[SYS_NMLN+1];
  unsigned long ipaddr, network, netmask;
  const    char *returnedname; 
  char          *ptr;
  
  /* has the server name been set by user? */
  if (strcasecmp(tcz_server_name,TCZ_SERVER_NAME) != 0) {
     /* try calling with user supplied name */
     if ((returnedname = getserverinfo(tcz_server_name,&ipaddr,&network,&netmask)) != NULL) {
	sprintf(buf,"User supplied server name OK:  %s  ->  %s/",returnedname,s_addrtoa(ipaddr));
	sprintf(buf + strlen(buf),"%s ",s_addrtoa(netmask));
	sprintf(buf + strlen(buf),"network %s",s_addrtoa(network));
	writelog(SERVER_LOG,0,"SET SERVER INFO",buf);
     } else {
	writelog(SERVER_LOG,0,"SET SERVER INFO","FATAL:  User supplied server name '%s' is bad (Host lookup failure.)",tcz_server_name);
        logfile_close();
	exit(1);
     }
  } else {
     /* else try calling with automagic detection switched on */
     if ((returnedname = getserverinfo(NULL,&ipaddr,&network,&netmask)) != NULL) {
	sprintf(buf,"Automagic detection OK:  '%s'  ->  %s/",returnedname,s_addrtoa(ipaddr));
	sprintf(buf + strlen(buf),"%s ",s_addrtoa(netmask));
	sprintf(buf + strlen(buf),"network %s",s_addrtoa(network));
	writelog(SERVER_LOG,0,"SET SERVER INFO",buf);
     } else {
	writelog(SERVER_LOG,0,"SET SERVER INFO","FATAL:  Automagic detection failed  -  You must set this information manually with the '-s' option.");
	logfile_close();
	exit(1);
     }
  }

  if (strcasecmp(tcz_server_name,TCZ_SERVER_NAME) == 0) {
     FREENULL(tcz_server_name);
     tcz_server_name = alloc_string(returnedname);
     writelog(SERVER_LOG,0,"SET SERVER INFO","Setting %s server name to '%s'.",tcz_short_name,tcz_server_name);
  }

  /* work out domainname from returnedname */
  if (strncasecmp(returnedname,"localhost",9) == 0) {
      /* we are running as a local-only copy */
      strncpy(domainname,"localhost",SYS_NMLN);
  } else {
      strncpy(buf,returnedname,TEXT_SIZE);
      memset(domainname,'\0',SYS_NMLN+1);
      if ((ptr=strchr(buf,'.')) != NULL) {
         strncpy(domainname,ptr+1,SYS_NMLN);
      } else {
         writelog(SERVER_LOG,0,"SET SERVER INFO","WARNING:  Can't get DNS domain name from '%s'.",returnedname);
      }
  }

  if (!Blank(email_forward_name) && (strcasecmp(email_forward_name,EMAIL_FORWARD_NAME) == 0)) {
     FREENULL(email_forward_name);
     email_forward_name = alloc_string(domainname);
     writelog(SERVER_LOG,0,"SET SERVER INFO","Setting E-mail forwarding domain name to '%s' (user.name@%s)",email_forward_name,email_forward_name);
  }

  if (strcasecmp(html_home_url,HTML_HOME_URL) == 0) {
     FREENULL(html_home_url);
     snprintf(buf,TEXT_SIZE,"http://%s/",tcz_server_name);
     html_home_url = alloc_string(buf);
     writelog(SERVER_LOG,0,"SET SERVER INFO","Setting %s Web Site URL to '%s'.",tcz_short_name,html_home_url);
  }

  if (strcasecmp(html_data_url,HTML_DATA_URL) == 0) {
     FREENULL(html_data_url);
     snprintf(buf,TEXT_SIZE,"http://%s/tczhtml/",tcz_server_name);
     html_data_url = alloc_string(buf);
     writelog(SERVER_LOG,0,"SET SERVER INFO","Setting HTML Interface data URL to '%s'.",html_data_url);
  }

  if (tcz_server_ip == TCZ_SERVER_IP) {
     tcz_server_ip = ipaddr;
     writelog(SERVER_LOG,0,"SET SERVER INFO","Setting %s server IP address to %s",tcz_short_name,s_addrtoa(tcz_server_ip));
  } else {
     if (tcz_server_ip != ipaddr) {
	writelog(SERVER_LOG,0,"SET SERVER INFO","WARNING:  Manually specified %s server IP address %s does NOT MATCH server name '%s' (%s).",tcz_short_name,s_addrtoa(tcz_server_ip),returnedname,s_addrtoa(ipaddr));
     }
  }

  if (tcz_server_network == TCZ_SERVER_NETWORK) {
     tcz_server_network = network;
     writelog(SERVER_LOG,0,"SET SERVER INFO","Setting %s server network address to %s",tcz_short_name,s_addrtoa(tcz_server_network));
  }

  if (tcz_server_netmask == TCZ_SERVER_NETMASK) {
     tcz_server_netmask = netmask;
     writelog(SERVER_LOG,0,"SET SERVER INFO","Setting %s server netmask to %s",tcz_short_name,s_addrtoa(tcz_server_netmask));
  }

  return;
}

const char *getserverinfo(const char *suppliedname, unsigned long *ipaddr, unsigned long *network, unsigned long *netmask)
{
  /*
   * Author:
   * -------
   *
   * Simon A. Boggis, Feb 2000
   *
   * Synopsis:
   * ---------
   *
   * Obtains FQDN, ipaddress, network and netmask (in HOST BYTE ORDER)
   * depending on supplied argument 'suppliedname'.
   *
   * Arguments
   * ---------
   *
   * If suppliedname is NULL, it autodetects using calls to uname(2)
   *
   * If suppliedname is not NULL, it uses supplied name. It can be either a
   * bare hostname, a FQDN or an IP address in dotted-quad form.
   *
   * the other arguments are references to values to be filled in on success.
   *
   * WARNING!
   * --------
   * The char * gets overwritten on each call so you need to strncpy it 
   * if you want to keep it.
   *
   * Return values:
   * --------------
   *
   * On success, the FQDN is returned and the ipaddress, network and mask
   * are all filled in with appropriate HOST BYTE ORDER info.
   *
   * On failure, NULL is returned and the values passed by reference are no 
   * longer valid or meaningful.
   *
   * Notes on workings:
   * ------------------
   * The function attempts to resolve the IP address for the 
   * name. If this fails, it returns NULL (error).
   *
   * If all is OK so far, it attempts to figure out which interface (e.g. 
   * eth0) corresponds to that address in order to figure out the network and 
   * netmask. If this fails, it warns but returns OK (the network and mask
   * will be 0.0.0.0 and 255.255.255.255 respectively). If the interface is
   * Point-to-Point a warning is also generated, and the network and mask
   * are proabably useless.
   *
   * Functions called by this:
   * -------------------------
   *   sab_ntoa()                      (replacement inet_ntoa)
   *   get_interfaces()                (obtain interface list from kernel)
   *
   */

  /* for herror() in gethostbyname */
  extern int h_errno;

  /* for the get_interfaces() bit */
  int i, n=0;
  struct if_list *mylist=NULL, *ifptr;

  static char fqdn[2*SYS_NMLN+1];
  char hostname[2*SYS_NMLN+1], foundif, buf[TEXT_SIZE];

  struct hostent *he=NULL;
  struct utsname utsname;
  struct in_addr in;

  /* set to INADDR_ANY for sane default value */
  *ipaddr = 0x00000000;
  /* in case we can't find an interface with the address (could happen) */
  *netmask = 0xFFFFFFFF;
  *network = 0x00000000;
  
  writelog(SERVER_LOG,0,"GET SERVER INFO","Automagical configuration enabled.");
  memset(hostname,'\0',sizeof(hostname));  
  if (suppliedname == NULL) {
    if (uname(&utsname)) {
      writelog(SERVER_LOG,0,"GET SERVER INFO","ERROR:  can't get hostname - uname() failed (%s.).",strerror(errno));
      return(NULL); 
    }
    strncpy(hostname,utsname.nodename,sizeof(hostname));
    writelog(SERVER_LOG,0,"GET SERVER INFO","Hostname is '%s'.",hostname);
  } else {
    strncpy(hostname,suppliedname,sizeof(hostname));
    writelog(SERVER_LOG,0,"GET SERVER INFO","Using supplied hostname '%s'.",hostname);
  }

  writelog(SERVER_LOG,0,"GET SERVER INFO","Looking up address for '%s', please wait ...",hostname);
  if ((he=gethostbyname(hostname)) == NULL) {  /* get the host info for name */
    writelog(SERVER_LOG,0,"GET SERVER INFO","ERROR:  lookup on '%s' failed (%s.).",hostname,hstrerror(h_errno));
    return(NULL);
  }
  memset(fqdn,'\0',sizeof(fqdn));  
  strncpy(fqdn,he->h_name,sizeof(fqdn));
  writelog(SERVER_LOG,0,"GET SERVER INFO","FQDN of '%s' is '%s'.",hostname,fqdn);

  in.s_addr=(*((struct in_addr *) he->h_addr_list[0])).s_addr;
  *ipaddr=ntohl(in.s_addr); /* BAD and NON-PORTABLE - SAB */
  writelog(SERVER_LOG,0,"GET SERVER INFO","IP address of '%s' is '%s'.",hostname,sab_ntoa(in));

  if ((n=get_interfaces(&mylist)) < 0) {
    writelog(SERVER_LOG,0,"GET SERVER INFO","WARNING:  failed to extract kernel network interface list.");
  } else {
    ifptr = mylist;
    
    foundif=0; /* not found yet */
    
    /* alternative way to access mylist as an array: */
    /*  if (strlen(mylist[n].name) == 0 && (mylist[n].name)[0] == '\0') { */
    for (i=0;i<n;i++) {
      char buf[TEXT_SIZE];
      sprintf(buf,"Interface [%d]:",i+1);
      sprintf(buf + strlen(buf)," '%s'",(*ifptr).name);
      if((*ifptr).family == AF_INET) {
	sprintf(buf + strlen(buf)," (AF_INET) %s",sab_ntoa((*ifptr).addr));
	writelog(SERVER_LOG,0,"GET SERVER INFO","%s/%s",
		 buf,sab_ntoa((*ifptr).mask));
	if ((*ifptr).addr.s_addr == in.s_addr) {
	  foundif=1; /* found it! */
	  *netmask=htonl((*ifptr).mask.s_addr); /* BAD and NON-PORTABLE - SAB */
	  *network=*ipaddr & *netmask; /* BAD and NON-PORTABLE - SAB */
	  writelog(SERVER_LOG,0,"GET SERVER INFO","Interface '%s' has address of '%s' (%s)",(*ifptr).name,fqdn,sab_ntoa(in));
	  if ((*ifptr).flags & IFF_POINTOPOINT) { /* P-to-P ? */
	    writelog(SERVER_LOG,0,"GET SERVER INFO","WARNING:  interface '%s' appears to be Point-to-Point (Destination address '%s').",(*ifptr).name,sab_ntoa((*ifptr).dest));
	  }
	  break;
	}
      } else {
	writelog(SERVER_LOG,0,"GET SERVER INFO","%s (*NOT* AF_INET)",buf);
      }
      ifptr = (struct if_list *) ((caddr_t) ifptr + sizeof(struct if_list));
    }
  }

  if (!foundif) {
    /*
     * Attempt to guess the missing info (network,mask) from the address.
     * This is based on normal ipv4 class addresses and won't cope with
     * subnetworking (a la CIDR), but it's probably better than nothing
     */
    writelog(SERVER_LOG,0,"GET SERVER INFO","WARNING:  No interface corresponding to '%s' (%s) found  -  network and mask will now be guessed and may be incorrect.",fqdn,sab_ntoa(in));
    *network=htonl(inet_netof(in));
    /* WARNING - BAD and NON-PORTABLE + IPV4 ONLY */
    for (i=0; i < sizeof(unsigned long); i++) {
      if (((unsigned char *)network)[i] > 0) {
	((unsigned char *)netmask)[i] = 255;
      } else {
	((unsigned char *)netmask)[i] = 0;
      }
    }
    sprintf(buf,"WARNING:  GUESSED network %s",s_addrtoa(*network));
    sprintf(buf + strlen(buf)," mask %s",s_addrtoa(*netmask));
    sprintf(buf + strlen(buf)," from address %s",s_addrtoa(*ipaddr));
    writelog(SERVER_LOG,0,"GET_SERVER_INFO","%s",buf);
  }

  free(mylist);
  return(fqdn);
}

const char *s_addrtoa(unsigned long address) {
  /*
   * Author:
   * -------
   * Simon A. Boggis, July 1999
   *
   * Synopsis:
   * ---------
   * Avoid annoying conversion for printing s_addr values (unsigned long)
   * with this tiny utility function.
   *
   * WARNING!
   * --------
   * The char * gets overwritten on each call so you need to strncpy it 
   * if you want to keep it.
   *
   * WARNING II!
   * -----------
   * DUE TO TCZ HAVING HOST BYTE ORDER ADDRESS VALUES THIS IS THE WRONG
   * WAY ROUND FOR USE ON NORMAL S_ADDR VALUES.
   *
   * Functions called by this:
   * ------------------------
   *    sab_ntoa()
   */
  struct in_addr in;

  in.s_addr = htonl(address); /* BAD and NON-PORTABLE */

  return(sab_ntoa(in)); 
}

char *sab_ntoa(struct in_addr addr) {
  /*
   * Author:
   * -------
   * Simon A. Boggis, July 1999
   *
   * Synopsis:
   * ---------
   * Takes an address as a struct in_addr (member s_addr in Network Byte 
   * Order of course!) and returns a char * containing the address printed
   * in ascii in the popular 'dotted-quad' notation.
   *
   * WARNING!
   * --------
   * The char * gets overwritten on each call so you need to strncpy it 
   * if you want to keep it.
   *
   * Basically does the same as inet_ntoa -  I needed to write my
   * own because this function is completely cocked-up in irix  
   * and returns rubbish all the time ):
   *
   * Functions called by this:
   * ------------------------
   * None
   */
  static char address[16] = ""; /* 'vvv.xxx.yyy.zzz\0' */

  memset(address,'\0',16);
  
  snprintf(address,16,"%d.%d.%d.%d",
	  ((unsigned char *)&(addr.s_addr))[0],
	  ((unsigned char *)&(addr.s_addr))[1],
	  ((unsigned char *)&(addr.s_addr))[2],
	  ((unsigned char *)&(addr.s_addr))[3]
	  );

  return(address);
}

int get_interfaces(struct if_list *list[])
{
  /* 
   * Author:
   * -------
   * Simon A. Boggis, July 1999
   *
   * Synopsis:
   * ---------
   * Retrives (using ioctl() system calls) the system interface table from the
   * kernel and stores it in an array of struct if_list pointed to by the
   * argument (struct if_list *) list; the number of interfaces found on the 
   * system are returned. The function will realloc or malloc the pointer to 
   * provide sufficient storage for the array of if_list[] as appropriate, 
   * therefore any previous data stored there *will* be destroyed.
   * After the last interface stored in the array of struct if_list there
   * is a terminating entry which has all values set to 0. This is NOT
   * included in the number of interfaces returned (of course!).
   * Simply testing for the length of the name being zero (or name[0] being
   * equal to '\0') should suffice to detect this if you need to recount the
   * interfaces without another call.
   * 
   * Definition of struct if_list:
   * -----------------------------
   * struct if_list {
   *   char name[IFNAMSIZ];
   *   int flags;            (complete flags value)
   *   int family;           (addr,mask,dest,bcast are ONLY valid for AF_INET
   *                          for other families these values will be set to 0)
   *   struct in_addr addr;
   *   struct in_addr mask;
   *   struct in_addr dest;  (0 if not Point-to-Point)
   *   struct in_addr bcast; (0 if broadcast not available)
   * };
   *
   * Return Values:
   * --------------
   * returns -1 on error, and free's and sets the pointer to the array of
   * if_list to NULL.
   *
   * Values for the various addresses are only filled in where appropriate
   * (i.e. no broadcast address for non-broadcast capable interfaces, and
   * no destination address for non Point-to-Point interfaces) and
   * no _addresses_ will be returned for:
   *   (1) Interfaces that are not up (IFF_UP)
   *   (2) Interfaces that are not in the AF_INET family (i.e. non tcp/ip).
   * In either of these cases, only the name, flags and family are valid whilst
   * all other values (the addresses) will all be set to zero.
   *
   * Compilation:
   * ------------
   * Systems tested:
   * Linux 2.2.x, Irix 6.5 SunOS 4.4, Solaris 2.7, FreeBSD 2.2.x
   *
   * To compile for FreeBSD you MUST define FREEBSD (it defines its struct 
   * ifreq very differently to all the others) e.g.:
   *   #define FREEBSD or -DFREEBSD on the command line
   * You should also define LINUX, IRIX, SUNOS or SOLARIS as appropriate to
   * get the proper includes and the dynamic sizing for the retrieved 
   * interface table that only Linux seems able to do.
   * 
   * To see how the pointer to the current struct ifreq is advanced:
   *   #define SHOWIFREQTRAVERSE or -DSHOWIFREQTRAVERSE
   *
   * Example compilation flags:
   * --------------------------
   * Linux:
   *   gcc -Wall -DLINUX -o test_get_interfaces test_get_interfaces.c
   *
   * Irix:
   *   gcc -Wall -DIRIX -o test_get_interfaces test_get_interfaces.c
   *
   * SunOS:
   *   gcc -Wall -DSUNOS -o test_get_interfaces test_get_interfaces.c
   *
   * Solaris:
   *   gcc -Wall -DSOLARIS -o test_get_interfaces test_get_interfaces.c \
   *          -lsocket -lnsl
   *
   * FreeBSD
   *   gcc -Wall -DFREEBSD -o test_get_interfaces test_get_interfaces.c
   *
   * Notes: 
   * -----
   * FreeBSD will return an interface (at least) twice if it is
   * configured -  SunOS, Solaris, Irix and Linux don't do this. This means 
   * that (for example) you'll get a single ethernet interface returned as
   * at least two interfaces, one of which does not have an address. If the
   * interface has more than one address (due to aliasing or other
   * address families like ipx) you'll see it more times still.
   *
   * Functions called by this:
   * ------------------------
   * None
   *
   * Examples:
   * ---------
   * example function call:
   *
   * struct if_list *mylist=NULL;
   * int n;
   * if ((n=get_interfaces(&mylist)) < 0) {
   *   fprintf(stderr,"Error retrieving interface table\n");
   *   exit(1);
   * }
   * printf("\nNumber of interfaces %d\n",n);
   *
   * example test for terminating (null) entry:
   *
   * i=0;
   * while (1) {
   *   if (strlen(mylist[i].name) == 0 || (mylist[i].name)[0] == '\0') {
   *     printf("Finished\n");
   *     break;
   *   }
   *   printf("Interface %d is %s\n",i,mylist[i].name);
   *   i++;
   * }
   *
   *
   * example of testing flags on an interface:
   *
   * if (mylist[i].flags & IFF_PROMISC) 
   *   printf("Naughty promiscous interface[%d] %s!\n",i,mylist[i].name);
   *
   * Possible (unix-flavour independent) flags you can test for include:
   *
   * IFF_UP
   * IFF_BROADCAST
   * IFF_DEBUG
   * IFF_POINTOPOINT
   * IFF_LOOPBACK
   * IFF_NOARP
   * IFF_PROMISC
   * IFF_ALLMULTI
   * IFF_MULTICAST
   *
   */
 
#ifndef LINUX /* Linux can get the IFREQ size dynamically */
#define STATIC_IFREQ
#define MAXINTERFACES 32
#endif

  int fd, n_if;
  struct ifreq *buf, *ifr, *stop; /* struct for ioctl data */
  struct ifconf ifc; /* struct for interface table */
  struct if_list *listptr;  /* pointer to current if_list struct */

   /* we use a SOCK_DGRAM to retrieve our interface info */
   if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
     perror("get_interfaces: socket");
     return(-1);
   }

#ifdef STATIC_IFREQ
   ifc.ifc_len = sizeof(struct ifreq) * MAXINTERFACES;
#else
   ifc.ifc_len = 0;
   ifc.ifc_buf = NULL;

   /* Find out how much space we need to retrieve interface table */
   if (ioctl(fd, SIOCGIFCONF, (char *)&ifc) < 0) {
     perror("get_interfaces: ioctl (SIOCGIFCONF)");
     return(-1);
   }
#endif /* STATIC_IFREQ */

   /* allocate space required */
   if ((buf = malloc(ifc.ifc_len)) == NULL) {
     perror("get_interfaces: malloc");
     free(buf);
     return(-1);
   }

   /* point ifc_buf to our newly malloc'd buffer */
   ifc.ifc_buf = (caddr_t)buf;

   /* retrieve interface table data */
   if (ioctl(fd, SIOCGIFCONF, (char *)&ifc) < 0) {
     perror("get_interfaces: ioctl (SIOCGIFCONF)");
     free(buf);
     return(-1);
   }

   /* set initial pointer to first struct ifreq */
   ifr = ifc.ifc_req;

   /* calculate address of end of interface table */
   stop = (struct ifreq *) (ifc.ifc_buf + ifc.ifc_len);

   /* calculate number of interfaces */
   n_if = ifc.ifc_len / sizeof(struct ifreq);

   if (!n_if) {
     fprintf(stderr,"get_interfaces: No interfaces found: weirdness!\n");
     free(buf);
     return(0);
   }

   /* allocate our if_list structure to be returned */
   if (*list == NULL) { /* not yet used */
     if ((*list = malloc((n_if + 1) * sizeof(struct if_list))) == NULL) {
       perror("get_interfaces: malloc if_list");
       free(buf);
       return(-1);
     }
   } else {
     if ((*list = realloc(*list,(n_if + 1) * sizeof(struct if_list))) == NULL) {
       perror("get_interfaces: realloc if_list");
       free(buf);
       return(-1);
     }
   }

   /*
    * Pointer hell: 
    * (*listptr) is equiv to (*list)[n] is equiv to *(list[n])
    * but I just use listptr->... because it looks nicer. 
    */
   listptr = *list;

   while (ifr < stop) {
     struct ifreq ifreq;
     int flags = 0;

     ifreq = *ifr;

     if (ioctl(fd, SIOCGIFFLAGS, (char *)&ifreq) < 0) {
       perror("get_interfaces: ioctl (SIOCGIFFLAGS)");
       free(buf);
       free(*list);
       *list=NULL;
       return(-1);
     }

     /* zero out interface name - avoid trouble later */
     memset(listptr->name,'\0',IFNAMSIZ);

     /* INTERFACE NAME */
     strncpy(listptr->name,ifr->ifr_name,IFNAMSIZ);

     /* FLAGS on interface */
     listptr->flags = flags = ifreq.ifr_flags;

     /* FAMILY - we're only interested in AF_INET */
     listptr->family = ifr->ifr_addr.sa_family;

     /* Get address and co IF interface is up and family AF_INET */
     if ((flags & IFF_UP) && ifr->ifr_addr.sa_family == AF_INET) {
       /* these are for convenience and clarity, nothing more */
       struct sockaddr_in *addr, *brdcst, *dstadd, *netmsk;

       /* ADDRESS - don't need an ioctl as SIOCGIFFLAGS already done */
       addr = (struct sockaddr_in *) &ifreq.ifr_addr;
       listptr->addr = addr->sin_addr; 
 
       /* NETMASK */
       ifreq = *ifr;
       if (ioctl(fd, SIOCGIFNETMASK, (char *)&ifreq) < 0) {
	 perror("get_interfaces: ioctl (SIOCGIFNETMASK)");
	 free(buf);
	 free(*list);
	 *list=NULL;
	 return(-1);
       } else {	   
	 netmsk = (struct sockaddr_in *) &ifreq.ifr_addr;
	 listptr->mask = netmsk->sin_addr;
       }
       
       /* DESTINATION ADDRESS (if POINTOPOINT) */
       if (flags & IFF_POINTOPOINT) {
	 ifreq = *ifr;
	 if (ioctl(fd, SIOCGIFDSTADDR, (char *)&ifreq) < 0) {
	   perror("get_interfaces: ioctl (SIOCGIFDSTADDR)");
	   free(buf);
	   free(*list);
	   *list=NULL;
	   return(-1);
	 } else {	   
	   dstadd = (struct sockaddr_in *) &ifreq.ifr_dstaddr;
	   listptr->dest = dstadd->sin_addr; 
	 }
       } else {
	 listptr->dest.s_addr = 0;
       }
       
       /* BROADCAST ADDRESS (if broadcast capable) */
       if (flags & IFF_BROADCAST) {
	 ifreq = *ifr;
	 if (ioctl(fd, SIOCGIFBRDADDR, (char *)&ifreq) < 0) {
	   perror("get_interfaces: ioctl (SIOCGIFBRDADDR)");
	   free(buf);
	   free(*list);
	   *list=NULL;
	   return(-1);
	 } else {	   
	   brdcst = (struct sockaddr_in *) &ifreq.ifr_broadaddr;
	   listptr->bcast = brdcst->sin_addr;
	 }
       } else {
	 listptr->bcast.s_addr = 0;
       }
     } else {
       /* Not family AF_INET - set them all to zero */
       listptr->addr.s_addr = 0;
       listptr->mask.s_addr = 0;
       listptr->dest.s_addr = 0;
       listptr->bcast.s_addr = 0;
     }

     /* march ever onwards - advance the ifr pointer */
     
     /* This is the OS dependant bit - FreeBSD handles its struct ifreq */
     /* differently to the rest it seems */
#ifdef FREEBSD
#ifdef SHOWIFREQTRAVERSE
     fprintf(stderr,"sizeof(struct ifreq) = %d; ifr->ifr_addr.sa_len = %d minus (struct sockaddr) = %d equals %d\n", 
	    sizeof(struct ifreq),
	    ifr->ifr_addr.sa_len,
	    sizeof(struct sockaddr),
	    (ifr->ifr_addr.sa_len - sizeof(struct sockaddr)));
     fprintf(stderr,"ifr = %#lx, step %d, new ifr = %#lx (stop = %#lx)\n",
	    (unsigned long) ifr,
	    (ifr->ifr_addr.sa_len) ? (ifr->ifr_addr.sa_len - sizeof(struct sockaddr)) + 1 : 1,
	    (unsigned long) ((ifr->ifr_addr.sa_len) ? ((caddr_t) ifr + ifr->ifr_addr.sa_len - sizeof(struct sockaddr)) + 1 : 1),
	    (unsigned long) stop);
#endif /* SHOWIFREQTRAVERSE */
     if (ifr->ifr_addr.sa_len)
       ifr = (struct ifreq *) ((caddr_t) ifr +
			       ifr->ifr_addr.sa_len -
			       sizeof(struct sockaddr));
     ifr++;
#else /* Linux, Irix, SunOS and Solaris */
#ifdef SHOWIFREQTRAVERSE
     fprintf(stderr,"ifr = %#lx, step %d, new ifr = %#lx (stop = %#lx)\n",
	    (unsigned long) ifr,
	    sizeof(struct ifreq),
	    (unsigned long) ((caddr_t) ifr + sizeof(struct ifreq)),
	    (unsigned long) stop);
#endif /* SHOWIFREQTRAVERSE */
     ifr = (struct ifreq *) ((caddr_t) ifr +
			     sizeof(struct ifreq));
#endif /* FREEBSD */

     /* increment the list pointer to the next struct if_list */
     listptr = (struct if_list *) ((caddr_t) listptr +
				   sizeof(struct if_list));

   }

   close(fd);

   /* set name of last element of list to null */
   memset(listptr->name,'\0',IFNAMSIZ);
   /* and set all other values to 0 */
   listptr->flags = 0;
   listptr->family = 0;
   listptr->addr.s_addr = 0;
   listptr->mask.s_addr = 0;
   listptr->dest.s_addr = 0;
   listptr->bcast.s_addr = 0;

   free(buf);
   return(n_if);
}

#endif  /* SERVERINFO */
