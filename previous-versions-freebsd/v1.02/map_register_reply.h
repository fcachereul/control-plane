/* 
 *	get_parameters_reg.c --
 * 
 *	get_parameters_reg -- OpenLISP control-plane 
 *
 *	Copyright (c) 2012 LIP6 <http://www.lisp.ipv6.lip6.fr>
 *	Base on <Lig code> copyright by David Meyer <dmm@1-4-5.net>
 *	All rights reserved.
 *
 *	LIP6
 *	http://www.lisp.ipv6.lip6.fr
 *
 *Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     o Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     o Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     o Neither the name of the University nor the names of its contributors
 *       may be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 *
 *
 *	Definitions data struct for map_register_reply
 *
 *
 */

#include	<stdio.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<ctype.h>
#include    <netdb.h>
#include	<ifaddrs.h>
#include	<strings.h>
#include	<sys/types.h>
#include	<sys/param.h>

#include	<sys/socket.h>
#include	<netinet/in_systm.h>
#include	<netinet/in.h>
#include	<netinet/udp.h>
#include	<netinet/ip.h>
#include	<netinet/ip6.h>
#include	<arpa/inet.h>
#include	<net/if.h>
#include	<sys/ioctl.h>
#include    <pthread.h>
#include	<net/lisp/lisp.h>
#include	<net/lisp/maptables.h>
#include	<poll.h>
#include	<time.h>


typedef enum			{FALSE,TRUE} boolean;
#define	uchar			u_char

#define GOOD				0
#define BAD					-1
#define	MAX_IP_PACKET		4096
#define	SOURCEIP		"SOURCEIP"
#define	LOOPBACK		"127.0.0.1"
#define	LOOPBACK6		"::1"
#define	LINK_LOCAL		"fe80"
#define	LINK_LOCAL_LEN		4
#define	MIN_EPHEMERAL_PORT	32768
#define	MAX_EPHEMERAL_PORT	65535

#define LISP_AFI_IP		1
#define LISP_AFI_IPV6		2
#define	LISP_IP_MASK_LEN	32
#define	LISP_IPV6_MASK_LEN	128
	
/*
 *	VERSION 
 *
 *	XXYYZZ, where
 * 
 *	XX is the draft-ietf-lisp-XX.txt version
 *	YY is the draft-ietf-lisp-ms-YY.txt version
 *      ZZ is the lisp version
 */

#define	VERSION "%s version 22.16.02\n"


/*
 *	CO --
 *
 *	Calculate Offset
 *
 *	Try not to make dumb mistakes with
 *	pointer arithmetic
 *
 */

#define	CO(addr,len) (((char *) addr + len))

/*
 *      SA_LEN --
 *
 *      sockaddr length
 *
 */

#define SA_LEN(a) ((a == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6))

/*
 *      IA_LEN --
 *
 *      inet sockaddr length
 *
 */

#define IA_LEN(a) ((a == AF_INET) ? sizeof(struct in_addr) : sizeof(struct in6_addr))

/*
 *	names for where the udp checksum goes
 */

#define UHTONL(n) (((((n) & 0xFF)) << 24) | \
                  ((((n) & 0xFF00)) << 8) | \
                  ((((n) & 0xFF0000)) >> 8) | \
                  ((((n) & 0xFF000000)) >> 24))

                  
#define UHTONS(n) (((((n) & 0xFF)) << 8) | (((n) & 0xFF00) >> 8))                   
#ifdef BSD
#define udpsum(x) x->uh_sum
#else
#define udpsum(x) x->check
#endif

/*
 * LISP Types
 */

#define	LISP_MAP_REQUEST			1
#define	LISP_MAP_REPLY				2
#define	LISP_MAP_REGISTER			3
#define LISP_CHANGE_PRIORITY		    5
#define LISP_ENCAP_CONTROL_TYPE		8
#define	LISP_CONTROL_PORT			4342
#define	LISP_CONTROL_PORT_STR		"4342"

/*
 *	Map Reply action codes
 */

#define LISP_ACTION_NO_ACTION			0
#define LISP_ACTION_FORWARD				1
#define LISP_ACTION_DROP				2
#define LISP_ACTION_SEND_MAP_REQUEST	3


#define LISP_AFI_IP			1
#define LISP_AFI_IPV6		2

#define MAPM_VERSION	1	/* Up the ante and ignore older versions */

#define MAPM_ADD	   0x01	 /* Add Map */
#define MAPM_DELETE	   0x02	 /* Delete Map */
#define MAPM_CHANGE	   0x03	 /* Change Mapping (not yet implemented) */
#define MAPM_GET 	   0x04	 /* Get matching mapping */
#define MAPM_MISS          0x05  /* Lookup Failed  (general case) */
#define MAPM_MISS_EID      0x06  /* Lookup Failed  and EID returned */
#define MAPM_MISS_HEADER   0x07  /* Lookup Failed  and IP header returned */
#define MAPM_MISS_PACKET   0x08  /* Lookup Failed  and Packet returned */
#define MAPM_LSBITS        0x09  /* Locator Status Bits Changed */
#define MAPM_LOCALSTALE    0x0A  /* Local Map Version is stale */
#define MAPM_REMOTESTALE   0x0B  /* Remote Map Version is stale */
#define MAPM_NONCEMISMATCH 0x0C  /* Rceived a mismatching nonce */

/* Sysctl missmsg state definition
 */
#define LISP_MISSMSG_EID           1
#define LISP_MISSMSG_HEADER        2
#define LISP_MISSMSG_PACKET        3

/* Sysctl ETR state definition
 */
#define LISP_ETR_STANDARD          1
#define LISP_ETR_NOTIFY            2
#define LISP_ETR_SECURE            3

/*
 * Bitmask values for map_addrs.
 */
#define MAPA_EID	0x01	 /* EID sockaddr present */
#define MAPA_EIDMASK	0x02	 /* netmask sockaddr present */
#define MAPA_RLOC	0x04	 /* Locator present */

/*
 * Index offsets for sockaddr array for alternate internal encoding.
 */
#define MAPX_EID	0	 /* EID sockaddr present */
#define MAPX_EIDMASK	1	 /* EIDmask sockaddr present */
#define MAPX_RLOC	2	 /* RLOC sockaddr present */
#define MAPX_MAX	3	 /* size of array to allocate */


#define COUNT		3
#define MIN_COUNT		1
#define	MAX_COUNT		3
#define MAX_LOOKUPS     100     /* Maximum allowed concurrent lookups */
#define MAP_REPLY_TIMEOUT	2	/* seconds */


/*Encapsulated Control Message Format

        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     / |                       IPv4 or IPv6 Header                     |
   OH  |                      (uses RLOC addresses)                    |
     \ |                                                               |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     / |       Source Port = xxxx      |       Dest Port = 4342        |
   UDP +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     \ |           UDP Length          |        UDP Checksum           |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ \
   LH  |Type=8 |S|                  Reserved                           |  | ----> lisp_control_pkt
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ /
     / |                       IPv4 or IPv6 Header                     |
   IH  |                  (uses RLOC or EID addresses)                 |
     \ |                                                               |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     / |       Source Port = xxxx      |       Dest Port = yyyy        |
   UDP +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     \ |           UDP Length          |        UDP Checksum           |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   LCM |                      LISP Control Message                     |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


*/

struct lisp_control_pkt {
 #ifdef LITTLE_ENDIAN
    int rsvd:4;
    unsigned int type:4;
 #else
    unsigned int type:4;
    int rsvd:4;
 #endif
    uchar reserved[3];
} __attribute__ ((__packed__));

/*Map-request Message Format

		0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |Type=1 |A|M|P|S|p|s|    Reserved     |   IRC   | Record Count  |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                         Nonce . . .                           |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                         . . . Nonce                           |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |         Source-EID-AFI        |   Source EID Address  ...     |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |         ITR-RLOC-AFI 1        |    ITR-RLOC Address 1  ...    |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                              ...                              |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |         ITR-RLOC-AFI n        |    ITR-RLOC Address n  ...    |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     / |   Reserved    | EID mask-len  |        EID-prefix-AFI         |
   Rec +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     \ |                       EID-prefix  ...                         |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                   Map-Reply Record  ...                       |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/

struct map_request_pkt {
#ifdef LITTLE_ENDIAN
	uchar           smr_bit:1;
	uchar           rloc_probe:1;
	uchar           map_data_present:1;
	uchar           auth_bit:1;
	uchar           lisp_type:4;
#else
	uchar           lisp_type:4;
	uchar           auth_bit:1;
	uchar           map_data_present:1;
	uchar           rloc_probe:1;
	uchar           smr_bit:1;
#endif

#ifdef LITTLE_ENDIAN
	uchar           reserved1:6;
	uchar           smr_invoked:1;
	uchar           pitr_bit:1;
#else
	uchar           pitr_bit:1;
	uchar           smr_invoked:1;
	uchar           reserved1:6;
#endif

#ifdef LITTLE_ENDIAN
	ushort          irc:5;
    uchar           reserved2:3;
#else
    uchar           reserved2:3;
	ushort          irc:5;
#endif
	uchar           record_count;
	unsigned int    lisp_nonce0;
	unsigned int    lisp_nonce1;
	ushort          source_eid_afi;
	ushort          itr_afi;
    uchar           originating_itr_rloc[0];
} __attribute__ ((__packed__));

struct map_request_eid {
	uchar 		reserved;
	uchar 		eid_mask_len;
	ushort		eid_prefix_afi;
    uchar           eid_prefix[0];
} __attribute__ ((__packed__));


//Map-Reply Message format
/*

        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |Type=2 |P|E|S|          Reserved               | Record Count  |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                         Nonce . . .                           |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                         . . . Nonce                           |
   +-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |   |                          Record  TTL                          |
   |   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   R   | Locator Count | EID mask-len  | ACT |A|      Reserved         |
   e   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   c   | Rsvd  |  Map-Version Number   |       EID-prefix-AFI          |
   o   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   r   |                          EID-prefix                           |
   d   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  /|    Priority   |    Weight     |  M Priority   |   M Weight    |
   | L +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | o |        Unused Flags     |L|p|R|           Loc-AFI             |
   | c +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  \|                             Locator                           |
   +-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
struct map_reply_pkt {
#ifdef LITTLE_ENDIAN
     int            rsvd:1;
     int            security_bit:1;
	 int            echo_nonce_capable:1;
     int            rloc_probe:1;
     int            lisp_type:4;
#else
     int            lisp_type:4;
     int            rloc_probe:1;
     int            echo_nonce_capable:1;
     int            security_bit:1;
     int            rsvd:1;
#endif
     ushort         reserved;
     uchar          record_count;
     unsigned int   lisp_nonce0;
     unsigned int   lisp_nonce1;
     uchar          data[0];
}  __attribute__ ((__packed__));


struct map_reply_eidtype {
    unsigned int	record_ttl;
    uchar 		loc_count;
    uchar 		eid_mask_len;
#ifdef LITTLE_ENDIAN
    int			reserved:4;
    int			auth_bit:1;
    int			action:3;
#else
    int			action:3;
    int			auth_bit:1;
    int			reserved:4;
#endif
    uchar 		reserved2;
#ifdef LITTLE_ENDIAN
	int			lisp_map_version1:4;
	int			reserved3:4;	
#else
	int			reserved3:4;
	int			lisp_map_version1:4;
#endif
	uchar		lisp_map_version2;
    ushort		eid_afi;
    uchar 		eid_prefix[0];         /* ITR-locator than EID-prefix */
} __attribute__ ((__packed__));


struct map_reply_loctype {
    uchar   priority;
    uchar   weight;
    uchar   mpriority;
    uchar   mweight;
    uchar   unused_flags1;
#ifdef LITTLE_ENDIAN
    uchar   reach_bit:1;
    uchar   rloc_prob:1;
    uchar   rloc_local:1;
    uchar   unused_flags2:5;
#else
    uchar   unused_flags2:5;
    uchar   rloc_local:1;
    uchar   rloc_prob:1;
    uchar   reach_bit:1;
#endif
    ushort loc_afi;
    uchar   locator[0];
} __attribute__ ((__packed__));


/*Map-Regiester Message format

        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |Type=3 |P|            Reserved               |M| Record Count  |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                         Nonce . . .                           |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                         . . . Nonce                           |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |            Key ID             |  Authentication Data Length   |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       ~                     Authentication Data                       ~
   +-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |   |                          Record  TTL                          |
   |   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   R   | Locator Count | EID mask-len  | ACT |A|      Reserved         |
   e   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   c   | Rsvd  |  Map-Version Number   |        EID-prefix-AFI         |
   o   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   r   |                          EID-prefix                           |
   d   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  /|    Priority   |    Weight     |  M Priority   |   M Weight    |
   | L +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | o |        Unused Flags     |L|p|R|           Loc-AFI             |
   | c +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  \|                             Locator                           |
   +-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/

//lisp-register struct
struct map_register_pkt {
#ifdef LITTLE_ENDIAN
     int            reserved:3;
     int            rloc_probe:1;
     int            lisp_type:4;
#else
     int            lisp_type:4;
     int            rloc_probe:1;
     int            reserved:3;
#endif
     uchar         reserved1;
#ifdef LITTLE_ENDIAN
	 int            want_map_notify:1;
     int            reserved2:7;
#else
     int            reserved2:7;
	 int            want_map_notify:1;
#endif
     uchar          record_count;
     unsigned int   lisp_nonce0;
     unsigned int   lisp_nonce1;
	 ushort         key_id;
     ushort         key_len;
	 unsigned char  auth_data[20];
     uchar          data[0];
}  __attribute__ ((__packed__));

//eid record struct
struct lisp_map_register_eidtype {
    unsigned int	record_ttl;
    uchar 		loc_count;
    uchar 		eid_mask_len;
#ifdef LITTLE_ENDIAN
    int			reserved:4;
    int			auth_bit:1;
    int			action:3;
#else
    int			action:3;
    int			auth_bit:1;
    int			reserved:4;
#endif
    uchar 		reserved1;
#ifdef LITTLE_ENDIAN
	int			lisp_map_version1:4;
    int 		rsvd:4;
#else
    int 		rsvd:4;
	int			lisp_map_version1:4;	
#endif
	uchar		lisp_map_version2;
    ushort		eid_afi;
    uchar 		eid_prefix[0];     
} __attribute__ ((__packed__));

//rloc record struct
struct lisp_map_register_loctype {
    uchar   priority;
    uchar   weight;
    uchar   mpriority;
    uchar   mweight;
    uchar   unused_flags1;
#ifdef LITTLE_ENDIAN
    uchar   reach_bit:1;
    uchar   rloc_probe:1;
	uchar   rloc_local:1;
	uchar   unused_flags2:5;
#else
    uchar   unused_flags2:5;
    uchar   rloc_local:1;
    uchar   rloc_probe:1;
    uchar   reach_bit:1;
#endif
    ushort	loc_afi;
    uchar   locator[0];
} __attribute__ ((__packed__));

//change priority struct


/* 		0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |Type=5 |H|S|D|        Reserved                 | Record Count  |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                         Nonce . . .                           |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                         . . . Nonce                           |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |            Key ID             |  Authentication Data Length   |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       ~                     Authentication Data                       ~
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |    EID-mask-len		       |        EID-prefix-AFI         |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                          EID-prefix                           |
      /+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 R |           List-IP-AFI           |      List-IP	. . .	       ~ 
     e +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
     c ~                       . . .  List-IP                          ~
      \+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
*/
	  
	  
struct lisp_change_priority_pkt {
#ifdef LITTLE_ENDIAN
	int reserved:1;
	unsigned int dest_bit:1;
	unsigned int src_bit:1;
	unsigned int hyper_bit:1;
	int type:4;
#else
	int type:4;
	unsigned int hyper_bit:1;
	unsigned int src_bit:1;
	unsigned int dest_bit:1;
	int reserved:1;
#endif
	uint16_t reserved1;
	uchar record_count;
	unsigned int lisp_nonce0;
	unsigned int lisp_nonce1;
	ushort key_id;
	ushort auth_data_len;
	unsigned char auth_data[20];
	uchar eid_mask_len;
	ushort eid_prefix_afi;
	uchar eid_prefix[0];
	uchar data[0];
} __attribute__ ((__packed__));

//Rec struct
struct lisp_change_priority_rec {
	ushort l_ip_afi;
	uchar l_ip[0];
} __attribute__ ((__packed__));


/*
 *  emr_inner_src_port --
 * 
 *  source port in the EMR's inner UDP header. Listen on
 *  this port in the returned map-reply (the source port on
 *  the map-reply is 4342).
 *
 */

ushort emr_inner_src_port;

struct map_register_data{
	int register_socket;
	int register_socket6;
	unsigned int		nonce0;
	unsigned int		nonce1;
};


struct map_reply_data{
	int l_sk;
	int sk;
	int sk6;
};

struct map_rloc_params {
      unsigned char *key;
      char ** rloc;
      int *weight;
      int *local;
      int *priority;
      int locator;
	  unsigned int	record_ttl;
	  int mapservercount; 
      char **eid;
	  unsigned int eidlen;
      struct sockaddr_storage * map_server_addr;	  
	  int flag;
};

//Struct to keep map-server information
struct map_server_db {
	unsigned char * ms_key;//key to authenticate with map-server
	struct sockaddr_storage ms_ip;//Ip address
	struct map_server_db *ms_next;//point to next map_server
};

//Struct to keep RLOC information
struct rloc_db {
	struct sockaddr_storage rl_ip;
    int  priority;
	int  weight;
    int  local;
	struct rloc_db *rl_next;
};

//Struct to keep EID-RLOC information
struct eid_rloc_db {
	struct sockaddr_storage  ed_ip;
	int eidlen;
	unsigned int record_ttl;
	int locator;
	int flag;
	struct rloc_db * rloc;
	struct eid_rloc_db * ed_next;
};
//Struct to keep all database mapping
struct map_db {
	struct sockaddr_in sourceip;
	struct sockaddr_in6 sourceip6;
	struct map_server_db *ms;
	struct map_server_db *mr;
	struct eid_rloc_db *data;
};

struct openlisp_mapmsg{
	struct	map_msghdr m_map;
	char	m_space[8192];
};

struct eid_lookup {
    struct sockaddr_storage eid;/* Destination EID */
    int rx;                     /* Receiving socket */
    uint32_t nonce0[MAX_COUNT]; /* First half of the nonce */
    uint32_t nonce1[MAX_COUNT]; /* Second half of the nonce */
    uint16_t sport;             /* EMR inner header source port */
    struct timespec start;      /* Start time of lookup */
    int count;                  /* Current count of retries */
    uint64_t active;            /* Unique lookup identifier, 0 if inactive */
	struct sockaddr * ms;					/* Point to mapserver  */
};

struct thread_params {
	int opl_socket;
	int mr_socket;
	struct sockaddr_storage map_resolver_addr;
	struct sockaddr_storage my_addr;	
};

extern  unsigned int	debug;
extern	unsigned int	machinereadable;
extern  unsigned int	disallow_eid;
extern  unsigned int	udp_checksum_disabled;
extern	ushort			emr_inner_src_port;

#ifdef BSD
#include <string.h>
#else
extern void		*memcpy();
extern void		*memset();
extern char		*strdup();
#endif

static const char filename[] = "/etc/register_parameters.txt";

struct map_request_pkt2 {
#ifdef LITTLE_ENDIAN
	uchar           smr_bit:1;
	uchar           rloc_probe:1;
	uchar           map_data_present:1;
	uchar           auth_bit:1;
	uchar           lisp_type:4;
#else
	uchar           lisp_type:4;
	uchar           auth_bit:1;
	uchar           map_data_present:1;
	uchar           rloc_probe:1;
	uchar           smr_bit:1;
#endif

#ifdef LITTLE_ENDIAN
	uchar           reserved1:6;
	uchar           smr_invoked:1;
	uchar           pitr_bit:1;
#else
	uchar           pitr_bit:1;
	uchar           smr_invoked:1;
	uchar           reserved1:6;
#endif

#ifdef LITTLE_ENDIAN
	ushort          irc:5;
    uchar           reserved2:3;
#else
    uchar           reserved2:3;
	ushort          irc:5;
#endif
	uchar           record_count;
	unsigned int    lisp_nonce0;
	unsigned int    lisp_nonce1;
	ushort          source_eid_afi;
	uchar		source_eid[4];
	ushort          itr_afi;
    uchar           originating_itr_rloc[4];
} __attribute__ ((__packed__));

struct map_request_eid2 {
	uchar 		reserved;
	uchar 		eid_mask_len;
	ushort		eid_prefix_afi;
    uchar           eid_prefix[4];
} __attribute__ ((__packed__));
