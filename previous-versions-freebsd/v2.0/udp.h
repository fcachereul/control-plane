
#ifndef __HAVE_UDP_H
#define __HAVE_UDP_H

#include "lib.h"
#include        <stdio.h>
#include        <unistd.h>
#include        <stdlib.h>
#include        <errno.h>
#include        <ctype.h>
#include        <netdb.h>
#include        <ifaddrs.h>
#include        <strings.h>
#include        <sys/types.h>
#include        <sys/param.h>
#include        <sys/socket.h>
#include        <netinet/in_systm.h>
#include        <netinet/in.h>
#include        <netinet/udp.h>
#include        <netinet/ip.h>
#include        <netinet/ip6.h>
#include        <arpa/inet.h>
#include        <net/if.h>
#include        <sys/ioctl.h>
#include		<assert.h>
#include	"hmac/hmac_sha.h"

#define	OUTPUT_STREAM stdout	

#define LISP_AFI_IP	1                                               
#define LISP_AFI_IPV6	2
#define PORT		4342

#define BUFLEN 512

#define	LISP_TYPE_RESERVED	0x0 
#define LISP_TYPE_MAP_REQUEST	0x1
#define LISP_TYPE_MAP_REPLY	0x2
#define LISP_TYPE_MAP_REGISTER	0x3
#define LISP_TYPE_MAP_NOTIFY	0x4
#define	LISP_TYPE_MAP_REFERRAL	0x6
#define LISP_TYPE_ENCAPSULATED_CONTROL_MESSAGE	0x8


#define TRUE    1
#define FALSE   0

/*
 *      CO --
 *
 *      Calculate Offset
 *
 *      Try not to make dumb mistakes with
 *      pointer arithmetic
 *
 */
#define CO(addr,len) (((char *) addr + len))

/* <AFI, Address> tuple IPv4 */
struct afi_address {
	uint16_t	afi;
	struct in_addr	address;
} __attribute__ ((__packed__));

/* <AFI, Address> tuple IPv6 */
struct afi_address6 {
	uint16_t	afi;
	struct in6_addr	address;
} __attribute__ ((__packed__));

/* <AFI, Address> tuple IPv4 and IPv6*/
union afi_address_generic {
	struct afi_address ip;
	struct afi_address ip6;
};

/*
     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |Type=6 |                Reserved               | Record Count  |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                         Nonce . . .                           |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                         . . . Nonce                           |
+-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   |                          Record  TTL                          |
|   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
R   | Referral Count| EID mask-len  | ACT |A|I|     Reserved        |
e   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
c   |SigCnt |   Map Version Number  |            EID-AFI            |
o   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
r   |                          EID-prefix ...                       |
d   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  /|    Priority   |    Weight     |  M Priority   |   M Weight    |
| L +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| o |        Unused Flags         |R|         Loc/LCAF-AFI          |
| c +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  \|                             Locator ...                       |
+-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

struct map_referral_hdr {
#ifdef LITTLE_ENDIAN
	uint8_t		rsvd:4;
	uint8_t		lisp_type:4;
#else
	uint8_t		lisp_type:4;
	uint8_t		rsvd:4;
#endif
	uint16_t	reserved;
	uint8_t		record_count;
	uint32_t	lisp_nonce0;
	uint32_t	lisp_nonce1;
}  __attribute__ ((__packed__));

struct map_referral_record {
	uint32_t	ttl;
	uint8_t		referral_count;
	uint8_t		eid_mask_len;
#ifdef LITTLE_ENDIAN
	uint8_t		reserved:3;
	uint8_t		i:1;
	uint8_t		a:1;
	uint8_t		act:3;
#else
	uint8_t		act:3;
	uint8_t		a:1;
	uint8_t		i:1;
	uint8_t		reserved:3;
#endif
	uint8_t		reserved1;
#ifdef LITTLE_ENDIAN
	uint16_t	version:12;
	uint16_t	sig_cnt:4;
#else
	uint16_t	sigcnt:4;
	uint16_t	version:12;
#endif
	uint8_t		unknown[12];
	uint16_t	eid_prefix_afi;
	struct in_addr	eid_prefix;
} __attribute__ ((__packed__));

struct map_referral_record6 {
	uint32_t	ttl;
	uint8_t		referral_count;
	uint8_t		eid_mask_len;
#ifdef LITTLE_ENDIAN
	uint8_t		reserved:3;
	uint8_t		i:1;
	uint8_t		a:1;
	uint8_t		act:3;
#else
	uint8_t		act:3;
	uint8_t		a:1;
	uint8_t		i:1;
	uint8_t		reserved:3;
#endif
	uint8_t		reserved1;
#ifdef LITTLE_ENDIAN
	uint16_t	version:12;
	uint16_t	sig_cnt:4;
#else
	uint16_t	sigcnt:4;
	uint16_t	version:12;
#endif
	uint16_t	eid_prefix_afi;
	struct in6_addr	eid_prefix;
} __attribute__ ((__packed__));

union map_referral_record_generic {
	struct map_referral_record	record;
	struct map_referral_record6	record6;
};

struct map_referral_locator {
	uint8_t		priority;
	uint8_t		weight;
	uint8_t		m_priority;
	uint8_t		m_weight;
	uint8_t		unused_flags;
#ifdef LITTLE_ENDIAN
	uint8_t		R:1;
	uint8_t		unused_flags1:7;
#else
	uint8_t		unused_flags1:7;
	uint8_t		R:1;
#endif
	uint16_t	rloc_afi;
	struct in_addr	rloc;
} __attribute__ ((__packed__));

struct map_referral_locator6 {
	uint8_t		priority;
	uint8_t		weight;
	uint8_t		m_priority;
	uint8_t		m_weight;
	uint8_t		unused_flags;
#ifdef LITTLE_ENDIAN
	uint8_t		R:1;
	uint8_t		unused_flags1:7;
#else
	uint8_t		unused_flags1:7;
	uint8_t		R:1;
#endif
	uint16_t	rloc_afi;
	struct in6_addr  rloc;
} __attribute__ ((__packed__));

union map_referral_locator_generic {
	struct map_referral_locator rloc;
	struct map_referral_locator6 rloc6;
};



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
c   | Rsvd  |  Map-Version Number   |            EID-AFI            |
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

struct map_reply_hdr {
#ifdef LITTLE_ENDIAN
	uint8_t		rsvd:1;
	uint8_t		security_bit:1;
	uint8_t		echo_nonce_capable:1;
	uint8_t		rloc_probe:1;
	uint8_t		lisp_type:4;
#else
	uint8_t		lisp_type:4;
	uint8_t		rloc_probe:1;
	uint8_t		echo_nonce_capable:1;
	uint8_t		security_bit:1;
	uint8_t		rsvd:1;
#endif
	uint16_t	reserved;
	uint8_t		record_count;
	uint32_t	lisp_nonce0;
	uint32_t	lisp_nonce1;
}  __attribute__ ((__packed__));

struct map_reply_record {
	uint32_t	ttl;
	uint8_t		locator_count;
	uint8_t		eid_mask_len;
#ifdef LITTLE_ENDIAN
	uint8_t		reserved:4;
	uint8_t		a:1;
	uint8_t		act:3;
#else
	uint8_t		act:3;
	uint8_t		a:1;
	uint8_t		reserved:4;
#endif
	uint8_t		reserved1;
#ifdef LITTLE_ENDIAN
	uint16_t	version:12;
	uint16_t	rsvd:4;
#else
	uint16_t	rsvd:4;
	uint16_t	version:12;
#endif
	uint16_t	eid_prefix_afi;
	struct in_addr	eid_prefix;
} __attribute__ ((__packed__));

struct map_reply_record6 {
	uint32_t	ttl;
	uint8_t		locator_count;
	uint8_t		eid_mask_len;
#ifdef LITTLE_ENDIAN
	uint8_t		reserved:4;
	uint8_t		a:1;
	uint8_t		act:3;
#else
	uint8_t		act:3;
	uint8_t		a:1;
	uint8_t		reserved:4;
#endif
	uint8_t		reserved1;
#ifdef LITTLE_ENDIAN
	uint16_t	version:12;
	uint16_t	rsvd:4;
#else
	uint16_t	rsvd:4;
	uint16_t	version:12;
#endif
	uint16_t	eid_prefix_afi;
	struct in6_addr	eid_prefix;
} __attribute__ ((__packed__));

union map_reply_record_generic {
	struct map_reply_record		record;
	struct map_reply_record6	record6;
};

struct map_reply_locator {
	uint8_t		priority;
	uint8_t		weight;
	uint8_t		m_priority;
	uint8_t		m_weight;
	uint8_t		unused_flags;
#ifdef LITTLE_ENDIAN
	uint8_t		R:1;
	uint8_t		p:1;
	uint8_t		L:1;
	uint8_t		unused_flags1:5;
#else
	uint8_t		unused_flags1:5;
	uint8_t		L:1;
	uint8_t		p:1;
	uint8_t		R:1;
#endif
	uint16_t	rloc_afi;
	struct in_addr	rloc;
} __attribute__ ((__packed__));

struct map_reply_locator6 {
	uint8_t		priority;
	uint8_t		weight;
	uint8_t		m_priority;
	uint8_t		m_weight;
	uint8_t		unused_flags;
#ifdef LITTLE_ENDIAN
	uint8_t		R:1;
	uint8_t		p:1;
	uint8_t		L:1;
	uint8_t		unused_flags1:5;
#else
	uint8_t		unused_flags1:5;
	uint8_t		L:1;
	uint8_t		p:1;
	uint8_t		R:1;
#endif
	uint16_t	rloc_afi;
	struct in6_addr  rloc;
} __attribute__ ((__packed__));

union map_reply_locator_generic {
	struct map_reply_locator rloc;
	struct map_reply_locator6 rloc6;
};

/* Map-Referral handling code */
int udp_referral_add(uint32_t request_id);

int udp_referral_add_record(uint32_t id, struct prefix * p, uint32_t ttl, uint8_t lcount, uint32_t version, uint8_t A, uint8_t act, uint8_t i, uint8_t sigcnt);

int udp_referral_add_locator(uint32_t id, struct map_entry * e);

int udp_referral_error(uint32_t id);

int udp_referral_terminate(uint32_t id);

/* ! Map-Referral handling code */



/* Map-Reply handling code */
int udp_reply_add(uint32_t request_id);

int udp_reply_add_record(uint32_t id, struct prefix * p, uint32_t ttl, uint8_t lcount, uint32_t version, uint8_t A, uint8_t act);

int udp_reply_add_locator(uint32_t id, struct map_entry * e);

int udp_reply_error(uint32_t id);

int udp_reply_terminate(uint32_t id);

/* ! Map-Reply handling code */

/* Map-Request handling code */
struct map_request{
	char * eid;
	uint64_t nonce;
};

int udp_request_terminate(uint32_t id);

int udp_request_get_eid(uint32_t id, struct prefix * p);

int udp_request_get_nonce(uint32_t id, uint64_t * nonce);

int udp_request_is_ddt(uint32_t id, int * is_ddt);

int udp_request_get_itr(uint32_t id, union sockunion * itr);

int udp_request_get_port(uint32_t id, uint16_t * port);

int udp_request_add(uint32_t id, uint8_t security, uint8_t ddt,\
		uint8_t A, uint8_t M, uint8_t P, uint8_t S,\
		uint8_t p, uint8_t s,\
		uint32_t nonce0, uint32_t nonce1,\
		const union sockunion * src, \
		const union sockunion * dst, \
		uint16_t source_port,\
		const struct prefix * eid );

int udp_request_ddt_terminate(uint32_t id, const union sockunion * server, char terminal);

uint32_t _forward_to_etr(uint32_t request_id, struct db_node * rn);

/* ! Map-Request handling code */

/* Communication handling code */
void * udp_start_communication(void * context);

void * udp_stop_communication(void * context);

/* ! Communication handling code */

/* UDP function binding */
 struct communication_fct udp_fct = {\
	.start_communication	= udp_start_communication, \
	.stop_communication	= udp_stop_communication, \
	/* Map-Reply */
	.reply_add 		= udp_reply_add,\
	.reply_add_record	= udp_reply_add_record, \
	.reply_add_locator	= udp_reply_add_locator,\
	.reply_error		= udp_reply_error, \
	.reply_terminate	= udp_reply_terminate, \
	/* Map-Referral */
	.referral_add 		= udp_referral_add,\
	.referral_add_record	= udp_referral_add_record, \
	.referral_add_locator	= udp_referral_add_locator,\
	.referral_error		= udp_referral_error, \
	.referral_terminate	= udp_referral_terminate, \
	/* Map-Request */
	.request_add		= udp_request_add, \
	.request_terminate	= udp_request_terminate, \
	.request_get_eid	= udp_request_get_eid , \
	.request_get_nonce	= udp_request_get_nonce, \
	.request_is_ddt		= udp_request_is_ddt, \
	.request_get_itr	= udp_request_get_itr, \
	.request_get_port	= udp_request_get_port,\
	.request_ddt_terminate	= udp_request_ddt_terminate,\
};


/*
 * Map-Request draft-ietf-lisp-22 structures definition
 * Map encapsulated control message draft-fuller-lisp-ddt-00
 *
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
LH  |Type=8 |S|D|               Reserved                            |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  / |                       IPv4 or IPv6 Header                     |
IH  |                  (uses RLOC or EID addresses)                 | 
  \ |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  / |       Source Port = xxxx      |       Dest Port = yyyy        |
UDP +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  \ |           UDP Length          |        UDP Checksum           |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
LCM |Type=1 |A|M|P|S|p|s|   Reserved      |   IRC   | Record Count  |
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

struct lisp_control_hdr {
#ifdef LITTLE_ENDIAN
	uint8_t rsvd:2;
	uint8_t ddt_originated:1;
	uint8_t	security_bit:1;
	uint8_t type:4;
#else
	uint8_t type:4;
	uint8_t security_bit:1;
	uint8_t ddt_originated:1;
	uint8_t rsvd:2;
#endif
	uint8_t reserved[3];
} __attribute__ ((__packed__));


struct map_request_hdr {
#ifdef LITTLE_ENDIAN
	uint8_t		smr_bit:1;
	uint8_t		rloc_probe:1;
	uint8_t		map_data_present:1;
	uint8_t		auth_bit:1;
	uint8_t		lisp_type:4;
#else
	uint8_t		lisp_type:4;
	uint8_t		auth_bit:1;
	uint8_t		map_data_present:1;
	uint8_t		rloc_probe:1;
	uint8_t		smr_bit:1;
#endif
#ifdef LITTLE_ENDIAN
	uint8_t         reserved1:6;
	uint8_t         smr_invoke_bit:1;
	uint8_t         pitr_bit:1;
#else
	uint8_t		pitr_bit:1;
	uint8_t		smr_invoke_bit:1;
	uint8_t		reserved1:6;
#endif
#ifdef LITTLE_ENDIAN
	uint8_t		irc:5;
	uint8_t		reserved2:3;
#else
	uint8_t		reserved2:3;
	uint8_t		irc:5;
#endif
	uint8_t		record_count;
	uint32_t	lisp_nonce0;
	uint32_t	lisp_nonce1;
}  __attribute__ ((__packed__));



/* Map-Request record tuple IPv4 */
struct map_request_record {
	uint8_t		reserved;
	uint8_t		eid_mask_len;
	uint16_t	eid_prefix_afi;
	struct in_addr	eid_prefix;
} __attribute__ ((__packed__));


/* Map-Request record tuple IPv6 */
struct map_request_record6 {
	uint8_t		reserved;
	uint8_t		eid_mask_len;
	uint16_t	eid_prefix_afi;
	struct in6_addr	eid_prefix;
} __attribute__ ((__packed__));

/* Map-Request record tuple IPv4 and IPv6 */
union map_request_record_generic {
	struct map_request_record	record;
	struct map_request_record6	record6;
};
/* =========================================================== */

/*
     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |Type=3 |P|            Reserved                 | Record Count  |
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
c   | Rsvd  |  Map-Version Number   |            EID-AFI            |
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

struct map_register_hdr {
#ifdef LITTLE_ENDIAN
	uint8_t		rsvd:3;
	uint8_t		proxy_map_reply:1;
	uint8_t		lisp_type:4;
#else
	uint8_t		lisp_type:4;
	uint8_t		proxy_map_reply:1;
	uint8_t		rsvd:3;
#endif
	uint16_t        reserved;
	uint8_t		record_count;
	uint32_t        lisp_nonce0;
	uint32_t        lisp_nonce1;
	uint16_t	key_id;
	uint16_t	auth_data_length;
	uint8_t		auth_data[0];
}  __attribute__ ((__packed__));


#endif
