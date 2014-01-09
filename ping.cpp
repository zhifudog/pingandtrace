#include"ping.h"
struct proto proto_v4 = 
{proc_v4,send_v4,NULL,NULL,0,IPPROTO_ICMP};

#ifdef IPV6
struct proto proto_v6 = 
{proc_v6,send_v6,NULL,NULL,0,IPPROTO_ICMP6};
#endif

int main(int argc,char **argv)
{
  int c;
  struct addrinfo *ai;
  opterr = 0;
  while((c = getopt(argc,argv,"v")) != -1){
    switch(c){
      case 'v':
        verbose++;
	break;
      case '?':
        printf("Unrecognized option:%c\n",c);
	return 0;
        
    }
  }
  if(optind != argc -1){
    printf("ping [-v] Useage:Hostname\n");
    return 0;
  }
  host = argv[optind];
  pid = getpid();
  /*same as a signal cycle*/
  signal(SIGALRM,sig_alrm);
  if((ai = Host_serv(host,NULL,0,0)) == 0){
    printf("ping:Unknown host %s\n",host);
    exit(0);
  }
  printf("ping :(%s) %s:%d databytes\n",ai->ai_canonname,
          sock_ntop(ai->ai_addr,ai->ai_addrlen),datalen);
  if(ai->ai_family == AF_INET){
    pr = &proto_v4;
#ifdef IPV6
  }else if(ai->ai_family == AF_INET6){
    pr = &proto_v6;
    if(IN6_IS_ADDR_V4MAPPED(&(((struct sockaddr_in6 *)ai->ai_addr)->sin6_addr))){
      printf("Can,t ping IP_V4 mapped IP_V6 address\n");
      return 0;
    }
#endif
 }else
   printf("Unknown address family %d",ai->ai_family);

 pr->sasend = ai->ai_addr;
 pr->sarecv = (struct sockaddr *)calloc(1,ai->ai_addrlen);
 pr->salen = ai->ai_addrlen; 
 /*read retuen message*/
 readloop();
 exit(0);
}


void readloop(void)
{
  int size;
  char recvbuf[BUFSIZE];
  socklen_t len;
  ssize_t n;
  struct timeval tval;
  if((sockfd = socket(pr->sasend->sa_family,SOCK_RAW,pr->icmpproto)) == -1){
    printf("user not root\n");
    return;
  }
  /*Don,t need special permission any more*/
  setuid(getuid());
  size = 60 * 1024;
  setsockopt(sockfd,SOL_SOCKET,SO_RCVBUF,&size,sizeof(size));
  /* send fir packet*/
  sig_alrm(SIGALRM);
  for(;;){
    len = pr->salen;
    n = recvfrom(sockfd,recvbuf,sizeof(recvbuf),0,pr->sarecv,&len);
    if(n < 0){
      if(errno = EINTR){
        continue;
      }else{
        printf("recv error\n");
	exit(0);
      }
    }
    gettimeofday(&tval,NULL);
    (*pr->fproc)(recvbuf,n,&tval);
  }
}


void proc_v4(char *ptr,ssize_t len,struct timeval *tv_recv)
{
  int hlen1,icmplen;
  double rtt;
  struct ip *ip;
  struct icmp *icmp;
  struct timeval *tv_send;

  ip = (struct ip*)ptr;
  hlen1 = ip->ip_hl << 2;
  icmp = (struct icmp*)(ptr + hlen1);
  if((icmplen = len - hlen1 )< 8){
    printf("icmplen(%d) < 8\n",icmplen);
    exit(0);
  }
  if(icmp->icmp_type == ICMP_ECHOREPLY){
    if(icmp->icmp_id != pid){
      return;
    }
    if(icmplen < 16){
       printf("icmplen(%d) < 16\n",icmplen);
       exit(0);
    }
    tv_send = (struct timeval*)icmp->icmp_data;
    tv_sub(tv_recv,tv_send);
    rtt = tv_recv->tv_sec*1000.0 + tv_recv->tv_usec/1000.0;
    printf("%d bytes from %s:seq = %u,ttl = %d,rtt = %.3fms\n",
           icmplen,sock_ntop(pr->sarecv,pr->salen),icmp->icmp_seq,ip->ip_ttl,rtt);

  }else if(verbose){
     printf("%d bytes from %s:type = %d,code = %d\n",
                icmplen,sock_ntop(pr->sarecv,pr->salen),icmp->icmp_type,icmp->icmp_code);
  }

}

void proc_v6(char *ptr,ssize_t len,struct timeval *tvrecv)
{
  printf("sorry ,i don,t write process for ipv6");
  return;
}

void send_v4(void)
{
  int len;
  struct icmp *icmp;
  icmp = (struct icmp *)sendbuf;

  icmp->icmp_type = ICMP_ECHO;
  icmp->icmp_code = 0;
  icmp->icmp_id = pid;
  icmp->icmp_seq = nsent++;
  gettimeofday((struct timeval *)icmp->icmp_data,NULL);
  len = 8 + datalen;
  icmp->icmp_cksum = 0;
  icmp->icmp_cksum = in_cksum((unsigned short *)icmp,len);
  sendto(sockfd,sendbuf,len,0,pr->sasend,pr->salen);
}

unsigned short in_cksum(unsigned short *addr,int len)
{
  int nleft = len;
  int sum = 0;
  unsigned short *w =addr;
  unsigned short answer = 0;

  while(nleft > 1){
    sum += *w++;
    nleft -= 2;
  }
  if(nleft == 1){
    *(unsigned char *)(&answer) = *(unsigned char *)w;
    sum += answer;
  }

  sum = (sum >> 16) + (sum &0xffff);
  sum += (sum >> 16);
  answer =~ sum;
  return (answer);
}

void sig_alrm(int signo)
{
  (*pr->fsend)();
  alarm(1);
  return;
}



















