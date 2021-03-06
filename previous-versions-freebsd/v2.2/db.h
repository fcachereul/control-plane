
#ifndef __HAVE_MSDB_H
	#define __HAVE_MSDB_H

#define	_MAPP		1
#define	_MAPP_XTR	2
#define _EID		4
#define _GEID		8
#define _GREID		16
#define _ROOT		0

#define	_ACTIVE 	1
#define _NOACTIVE 	0


/*db include 2 radix tree, one for ipv4 and other for ipv6 */
struct lisp_db {
		struct db_table *lisp_db4;
		struct db_table *lisp_db6;
};

/*second db: db of sites */
struct site_info{
		char * name;
		char * key;
		char * contact;
		u_char active;
		char * hashing;
		struct list_t  * eid;		 		
};

union sockunion {
	struct  sockaddr sa;
	struct  sockaddr_in sin;
	struct  sockaddr_in6 sin6;
	struct  sockaddr_storage ss;
};

struct mapping_flags{
	uint32_t iid;	/*InstanceID */
	uint16_t act:3,			/* Action bit */
		 A:1,			/* Authoritative bit */
		 version:12,		/* Version */
		 proxy:1;			/*Proxy-map-reply*/
	uint32_t ttl;			/* TTL */
	uint8_t referral:4;		/* Is referral */
	uint8_t incomplete:1;		/* incomplete DDT entry */
	uint8_t range;	/*range of EID: an mapping, a global EID-range */
	uint8_t active:1;
	void * rsvd;
};

struct map_entry{
	union sockunion rloc;		/* RLOC address */
	uint8_t priority;		/* priority */
	uint8_t weight;			/* weight */
	uint8_t m_priority;		/* multicast priority */
	uint8_t m_weight;		/* multicast weight */
	uint8_t L:1,			/* Local locator */
		p:1,			/* RLOC-probing locator */
		r:1;			/* reachability bit */
};

/* struct lisp_data{
	struct prefix eid_prefix;
	union sockunion itr_address;	
	uint64_t next_nonce; //nonce for next map-request
	uint8_t step; //steps of recur
	//struct stack_t hops; //stack of next node to send map-request	
}; */

struct pk_req_entry{
	uint8_t emc:1;		/*Encapsulate Message Control*/
	uint8_t ddt:1;		/*Encapsulate Message Control*/
	uint8_t type;  	/*type of lisp message*/
	uint32_t nonce0; /*nonce in map-request with ddt */
	uint32_t nonce1; /*nonce in map-request with ddt */
	union sockunion  si; /* source address OH */
	void * lh;	/*EMC lisp header */
	union sockunion  ih_si;
	void * ih;	/*IH ip header */
	void * udp;	/*IH udp header */
	void * lcm;	/*Lisp control message */
	void * buf; /*package content */
	uint32_t * nonce_0; /*nonce0*/
	uint32_t * nonce_1;
	struct list_t * itr;
	struct list_t * eid;
	uint16_t buf_len; /*package len */
	uint8_t ttl; /* how long exist in queue, ttl = n (n second) */
	uint8_t hop; /* number of recue - use for map-request */
//	struct lisp_data ldata;
};

struct pk_rpl_entry{
	void * buf; /*package content */
	uint16_t buf_len;
	void * curs;
	uint32_t request_id;
};

struct ms_entry {
	union sockunion addr;
	char * key;
};

struct mr_entry{
	union sockunion addr;
};

extern struct lisp_db * ms_db;
extern struct list_t * site_db;
extern struct list_t * etr_db;
extern struct list_t * xtr_ms;
extern struct list_t * xtr_mr;
extern char * listening_address[];

/* third db: struct of map-server for xTR send map-register
struct list_t xtr_mr;
struct list_t xtr_ms;
*/

/*make new node with pre-set type */
struct mapping_flags * ms_new_node_ex(u_char n_type);

/*add type to node. Note that: one node can have many roles (types) */
void ms_node_update_type(struct db_node * node, u_char n_type);

/*get a real parent of node (real parent = node with type not null) */
struct db_node * ms_get_target(struct db_node * node);

/*delete a node*/
void ms_free_node(void * node);

/*create a new site info*/
struct site_info * ms_new_site_info();

/*create a new site in list of site*/
struct list_entry_t * ms_new_site();

/*check type of node */
u_char ms_node_is_type(struct db_node * node, u_char n_type);

/*check if a node has referrall flag or not*/
u_char ms_node_is_referral(struct db_node * node);

/*check if a mapping is proxy-map-reply or not */
u_char ms_node_is_proxy_reply(struct db_node * node);

/*load configure to database */
void ms_load_config();

/*init database */
struct lisp_db * ms_init_db();

/*finish database */
void ms_finish_db(struct lisp_db *db);

/*select db_table base on AF */
struct db_table * ms_get_db_table(const struct lisp_db * db, struct prefix * pf);

/*parser configure file */
int ms_parser_config();

/*get paramater from map-register --> reused existing functions */
//void ms_get_register(void *mr_id);

/*get list of mapping from map-register package */
//void * ms_get_register_mapping(void *mr_id);

/*get hashing from map-register package */
//int ms_get_register_hashing(void *mr_id);

/*Check if map-register is new or not */
//int ms_validate_register(void *mr_id);

/*check if mr package is authentecated or not */
//int ms_auth_register(void * mr_id);

/*get site which mr belong to */
//struct list_entry_t * ms_get_register_site(void * mr_id);

/*compare hashing to see if mr package is new or not */
//int ms_exist_hashing(void * mr_id);

/*compare if all mapping in the mr package are the same site or not */
//int ms_same_site(void * mr_id);

/*update dabase with new mapping */
//void ms_accept_register(void * mr_id);

/*empty mapping of a site */
//void ms_empty_mapping(struct list_entry_t * site_entry);

//insert mapping for a site
//void ms_insert_mapping(struct list_entry_t * site_entry, void * mr_id);

/*show database */
void list_db(struct db_table * db);

/*show site's database */
void list_site(struct list_t *list);

/*run over a list */
void explore_list(struct list_t * list, int (* data_process)(void *));

/*show node info */
int show_eid_info(void *data);

/*show site info */
int show_site_info(void * data);
#endif
