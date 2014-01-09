#ifndef _TRACE_H_
#define _TRACE_H_
#include "public.h"
#include "comm.h"
#include<netinet/in_systm.h>
#include<netinet/ip.h>
#include<netinet/ip_icmp.h>
#include<netinet/udp.h>

#define BUFSIZE 1500

struct rec{
  u_short rec_seq;
  u_short rec_ttl;
  struct timeval rec_tv;
};

char recvbuf[BUFSIZE];
char sendbuf[BUFSIZE];

char *host;
u_short sport;
int nsent;
pid_t pid;
int probe;
int sendfd,recvfd;
int ttl;
int verbose;
char *icmpcode_v4(int);
char *icmpcode_v6(int);

void sig_alrm(int);
void traceloop(void);
void tv_sub(struct timeval *,struct timeval *);

int recv_v4(int ,struct timeval *);
int recv_v6(int ,struct timeval *);


struct proto{
  char *(* icmpcode)(int);
  int (* frecv)(int,struct timeval*);
  struct sockaddr* sasend;
  struct sockaddr* sarecv;
  struct sockaddr* salast;
  struct sockaddr* sabind;
  socklen_t salen;

  int icmpproto;
  int ttllevel;
  int ttloptname;

} * pr;

#ifdef IPV6
#include"ip6.h"
#include"icmp6.h"
#endif





#endif
