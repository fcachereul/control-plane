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
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "map_register_reply.h"

int	r_debug = 0;
char * sep_t =  "\t ";
int str2token(char * str, char * res[], char * sep);
int ip2sockaddr(char * , struct addrinfo **, char * port);
void print_params(struct map_db *);

//get register parameters: EID, RLOCs v.v
void getparameters_reg(struct map_db *params, int afi){
  
	FILE *file = fopen(filename, "r"); /* try to open the file */
	struct addrinfo *res;
	struct rloc_db *rloc_db = NULL, * rloc_lst = NULL;
	struct eid_rloc_db *eid_rloc_db = NULL, * eid_rloc_lst = NULL;
	struct map_server_db * ms_db = NULL, *mr_db = NULL, *mr_lst = NULL, *ms_lst = NULL;
	char  port_str[NI_MAXSERV];

	if (afi == LISP_AFI_IP) {
		afi = AF_INET;
	}
	else if (afi == LISP_AFI_IPV6) {
		afi = AF_INET6;
	}

	if ( file != NULL ){
		char line[BUFSIZ]; /* space to read a line into */

		int nx; //{0=NOTHING,1=SOURCEIP,2=MAPSERVER,3=EID,4=RLOC};
        nx = 0;
		char opk[5][20] = {"EMPTY","@MAPRESOLVER","@MAPSERVER","@EID", "@SOURCEIP"};
		int opk_c = 5;

		
		while ( fgets(line, sizeof line, file) != NULL ) /* read each line */
		{
			char data[50][255];
			char *token = line;			
			char * tk;
			char * ptr;
			int	i = 0; //counter
			
			if ((token[1] == '\0') || (token[0] == '#')) {//skip empty and comment line
				continue;
			}
						
			if (token[0] == '@') {//@control
				for ( i = 0; i < opk_c; i++) {
					if ( strncmp(token,opk[i],strlen(opk[i])) == 0 ){
						//printf("token = %s\n",token);
						nx = i;
						break;
					}
				}
				continue;
			}
			
			i = 0;
			//puts(line);
			for (tk = strtok_r(line, sep_t, &ptr); tk ; tk = strtok_r(NULL, sep_t, &ptr)){
				strcpy(data[i++], tk);
			}
			token = data[i-1];
			token[strlen(data[i-1])-1] = '\0';
			
			switch (nx) {
				case 1://map resolver				
					if (mr_db == NULL){
						mr_db = malloc(sizeof(struct map_server_db));
						mr_lst = mr_db;
					}
					else{
						mr_lst->ms_next = malloc(sizeof(struct map_server_db));
						mr_lst = mr_lst->ms_next;
					}
					mr_lst->ms_next = NULL;
					sprintf(port_str, "%d", LISP_CONTROL_PORT);
					ip2sockaddr(data[0], &res, port_str);
					memcpy(&mr_lst->ms_ip, res->ai_addr, SA_LEN(res->ai_family));
					
					freeaddrinfo(res);
					break;

				case 2://mapserver
					if (ms_db == NULL){
						ms_db = malloc(sizeof(struct map_server_db));
						ms_lst = ms_db;
					}
					else{
						ms_lst->ms_next = malloc(sizeof(struct map_server_db));
						ms_lst = ms_lst->ms_next;
					}
					ms_lst->ms_next = NULL;
					ms_lst->ms_key = malloc(strlen(data[1])* sizeof(char));
					strcpy((char *)ms_lst->ms_key, data[1]);
					sprintf(port_str, "%d", LISP_CONTROL_PORT);
					ip2sockaddr(data[0], &res, port_str);
					memcpy(&ms_lst->ms_ip, res->ai_addr, SA_LEN(res->ai_family));
					
					freeaddrinfo(res);
					break;
				
				case 3://eid
					ip2sockaddr(data[0], &res,NULL);

					if ((res->ai_family != afi) && (afi != 0)) {
						nx = 0;
						break;
					}

					if (eid_rloc_db == NULL){
						eid_rloc_db = malloc(sizeof(struct eid_rloc_db));
						eid_rloc_lst = eid_rloc_db;
					}
					else{
						eid_rloc_lst->ed_next = malloc(sizeof(struct eid_rloc_db));
						eid_rloc_lst = eid_rloc_lst->ed_next;
					}
					eid_rloc_lst->ed_next = NULL;
					eid_rloc_lst->eidlen = atoi(data[1]);
					eid_rloc_lst->record_ttl = atoi(data[2]);
					eid_rloc_lst->flag = atoi(data[3]);
					eid_rloc_lst->locator = 0;

					ip2sockaddr(data[0], &res,NULL);
					memcpy(&eid_rloc_lst->ed_ip, res->ai_addr, SA_LEN(res->ai_family));
					nx = 4;
					rloc_db = NULL;
					freeaddrinfo(res);				
					break;
				
				case 4://rloc
					if (rloc_db == NULL){
						rloc_db = malloc(sizeof(struct rloc_db));
						eid_rloc_lst->rloc = rloc_lst = rloc_db;
					}
					else{
						rloc_lst->rl_next = malloc(sizeof(struct rloc_db));
						rloc_lst = rloc_lst->rl_next;
					}
					rloc_lst->rl_next = NULL;
					rloc_lst->priority = atoi(data[1]);
					rloc_lst->weight = atoi(data[2]);
					rloc_lst->local = atoi(data[3]);
					eid_rloc_lst->locator = eid_rloc_lst->locator + 1;

					ip2sockaddr(data[0], &res,NULL);
					memcpy(&rloc_lst->rl_ip, res->ai_addr,SA_LEN(res->ai_family));					
					freeaddrinfo(res);				

					break;
			
			}//end switch

		}//end while
		fclose(file);
		params->ms = ms_db;
		params->mr = mr_db;
		params->data = eid_rloc_db;
		//print_params(params);
	}
	else {
		perror(filename); /* why didn't the file open? */
	}
	
	return ;
}

//make configuration file
void write_parametes_2_file(struct map_db *params){
  
	FILE *file = fopen(filename, "w"); /* try to open the file */
	struct eid_rloc_db *eid_rloc_db = NULL;
	struct rloc_db *rloc;

	struct map_server_db *ms;
	char ip_str[INET6_ADDRSTRLEN];

	//map server section
	fprintf(file,"%s\n","@MAPSERVER");
	fprintf(file,"%s\t\t\t%s\n","#mapserver","Key");
	ms = params->ms;
	while (ms != NULL){
		sockaddr2ip(&(ms->ms_ip), ip_str);
		fprintf(file,"%s\t\t\t%s\n",ip_str,ms->ms_key);
		ms = ms->ms_next;
	}

	//map resolver section
	fprintf(file,"\n%s\n","@MAPRESOLVER");
	ms = params->mr;
	while (ms != NULL){
		sockaddr2ip(&(ms->ms_ip), ip_str);
		fprintf(file,"%s\n",ip_str);
		ms = ms->ms_next;
	}
	
	//EID section
	eid_rloc_db = params->data;
	while (eid_rloc_db != NULL) {
		fprintf(file,"\n%s\n","@EID");
		fprintf(file,"%s\t\t%s\t%s\t\t%s\n","#Eid-Prefix","Subnet-mask","TTL","Flag");
		sockaddr2ip(&(eid_rloc_db->ed_ip), ip_str);
		fprintf(file,"%s\t\t%d\t\t%d\t\t%d\n",ip_str,
			eid_rloc_db->eidlen,eid_rloc_db->record_ttl,eid_rloc_db->flag);
		//Rloc section
		rloc = eid_rloc_db->rloc;
		fprintf(file,"\n%s\t\t\t%s\t%s\t\t%s\n","#Rloc","Priority","Weight","Local");		
		while (rloc != NULL) {
			sockaddr2ip(&(rloc->rl_ip), ip_str);
			fprintf(file,"%s\t\t%d\t\t%d\t\t%d\n",ip_str,
			rloc->priority,rloc->weight,rloc->local);
			rloc = rloc->rl_next;
		}
		eid_rloc_db = eid_rloc_db->ed_next;
	}

	fclose(file);
	return ;
}

//Function convert from string to token
int str2token(char * str, char * res[], char * sep){

	char * token;
	char * ptr;
	int	i = 0; //counter

	if (str[1] == '\0')
		return 1;

	for (token = strtok_r(str, sep, &ptr); token ; token = strtok_r(NULL, sep, &ptr))
		strcpy(res[i++], token);
	return 1;
}

//Function to make sockadd struct 
int ip2sockaddr(char * ip, struct addrinfo **res, char * port){
	
	struct addrinfo hints;
	int e; 
    struct protoent	    *proto;
    if ((proto = getprotobyname("UDP")) == NULL) {
		perror ("getprotobyname");
		exit(BAD);
    }

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;	//Allow IPv4 or IPv6 
	hints.ai_socktype  = SOCK_DGRAM;	// Datagram socket 
	hints.ai_flags  = AI_PASSIVE;	
    hints.ai_protocol  = proto->p_proto;
    hints.ai_canonname = NULL;
    hints.ai_addr      = NULL;
    hints.ai_next      = NULL;

	
	if ((e = getaddrinfo(ip, port, &hints, res)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(e));
			exit(BAD);
	}
	
	return 1;	
}

//Function to get ip string from sockaddr struct
void sockaddr2ip(struct sockaddr_storage *sk, char *res){

	struct sockaddr_in *tmp;
	struct sockaddr_in6 *tmp6;
	char ip[INET6_ADDRSTRLEN];
	tmp = (struct sockaddr_in *)sk;
	if (tmp->sin_family == AF_INET){
		tmp = (struct sockaddr_in *)sk;
		inet_ntop(AF_INET, &(tmp->sin_addr), ip, INET6_ADDRSTRLEN);
	}
	else{
		tmp6 = (struct sockaddr_in6 *)sk;
		inet_ntop(AF_INET6, &(tmp6->sin6_addr), ip, INET6_ADDRSTRLEN);
	}
	
	memcpy(res,&ip, INET6_ADDRSTRLEN);
}

//Function to get sockddr from an inet format of ip address
void inet2sockaddr(int afi, uchar *ip, struct addrinfo **res, int port){
	
	struct addrinfo hints;
	int e; 
    struct protoent	    *proto;
	char ip_str[INET6_ADDRSTRLEN];
	char port_str[NI_MAXSERV];

	inet_ntop(afi, ip, ip_str, INET6_ADDRSTRLEN);

    if ((proto = getprotobyname("UDP")) == NULL) {
		perror ("getprotobyname");
		exit(BAD);
    }

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;	//Allow IPv4 or IPv6 
	hints.ai_socktype  = SOCK_DGRAM;	// Datagram socket 
	hints.ai_flags  = AI_PASSIVE;	
    hints.ai_protocol  = proto->p_proto;
    hints.ai_canonname = NULL;
    hints.ai_addr      = NULL;
    hints.ai_next      = NULL;
	sprintf(port_str, "%d", port);
	if ((e = getaddrinfo(ip_str, port_str, &hints, res)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(e));
			exit(BAD);
	}
	
	return 1;	
}

//Print all parameters
void print_params(struct map_db * map_db){
	
	int i, j;
	struct map_server_db *ms, *mr;
	struct eid_rloc_db *eid;
	struct rloc_db *rloc;
	char ip[INET6_ADDRSTRLEN];

	printf("Parameters from configuration file\n");
    printf("==========================\n");
	
	//sockaddr2ip( (struct sockaddr_storage *)(&map_db->sourceip), ip);
	//printf("SOURCE IP= %s\n",ip); 
	//sockaddr2ip( (struct sockaddr_storage *)(&map_db->sourceip6), ip);
	//printf("SOURCE IP= %s\n",ip); 
	
	//printf("\n");

	//show list of map-server and map-resolver
	ms = map_db->ms;
	mr = map_db->mr;
	i = 0;

	while(ms != NULL){
		sockaddr2ip(&(ms->ms_ip), ip);
		printf("Map server[%i] = %s\t\t, Key = %s\n",
					i++, 
					ip,
					ms->ms_key);		
		ms = ms->ms_next;
	}

	i = 0;
	while(mr != NULL){
		sockaddr2ip(&(mr->ms_ip), ip);
		printf("Map resolver[%i] = %s\n",
					i++, 
					ip);		
		mr = mr->ms_next;
	}

	//Show mapping eid-rloc
	eid = map_db->data;
	i = 0;

	while (eid != NULL) {
		j = 0;
		sockaddr2ip(&(eid->ed_ip), ip);
		printf("\nEID[%i] =  %s \t\t , Mask-len = %d\t, TTL = %d\t, Locator = %d\t, Flag = %d\n",
					i++, 
					ip,
					eid->eidlen,
					eid->record_ttl,
					eid->locator,
					eid->flag
				);
		rloc = eid->rloc;
		
		//rloc
		while (rloc != NULL) {
			sockaddr2ip(&(rloc->rl_ip), ip);
			printf("RLOC[%i] = %s \t\t, Priority = %d\t, Weight = %d\t, Local = %d\n",
						j++, 
						ip,
						rloc->priority,
						rloc->weight,
						rloc->local);
			rloc = rloc->rl_next;
		}
		eid = eid->ed_next;
	}

    printf("\n==========================\n");
}
