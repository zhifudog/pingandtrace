#include<netinet/in_systm.h>
#include<netinet/ip.h>
#include<netinet/ip_icmp.h>
#include<unistd.h>
#include<stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <strings.h>
#include <stdlib.h>
#include<error.h>
#include<errno.h>
#include"comm.h"

#define BUFSIZE 1500

char recvbuf[BUFSIZE];
char sendbuf[BUFSIZE];

int datalen;
char *host;
int nsent;
pid_t pid;
int sockfd;
int verbose;

void proc_v4(char *,ssize_t,struct timeval *);
void proc_v6(char *,ssize_t,struct timeval *);

void send_v4(void);
void send_v6(void);
void readloop(void);

void sig_alrm(int);
unsigned short in_cksum(unsigned short *addr,int len);

struct proto{
  void (* fproc)(char *,ssize_t,struct timeval *);
  void (* fsend)(void);
  struct sockaddr *sasend;
  struct sockaddr *sarecv;
  socklen_t salen;
  int icmpproto;
} *pr;
#ifdef IPV6
#include "ip6.h"
#include "icmp6.h"
#endif
