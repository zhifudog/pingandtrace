#include"trace.h"

struct proto proto_v4 = 
{NULL,recv_v4,NULL,NULL,NULL,NULL,0,
IPPROTO_ICMP,IPPROTO_IP,IP_TTL};

int datalen = sizeof(rec);
int max_ttl = 30;
int nprobes = 3;
int dport = 37268+666;

int main(int argc,char **argv)
{
  int c;
  struct addrinfo *ai;

  opterr = 0;
  while((c = getopt(argc,argv,"m:v")) != -1){
    switch(c){
      case 'm':
        if((max_ttl = atoi(optarg)) < 1)
	return 0;
        break;
     case 'v':
       verbose++;
       break;
     case '?':
       return 0;
    }
  }

  if(optind != argc -1){
    printf("usage:[<-m<maxttl>-v>]<hostname>\n");
  }
  host = argv[optind];
  signal(SIGALRM,sig_alrm);
  if((ai = Host_serv(host,NULL,0,0)) == 0){
    printf("ping: Unknown host %s\n",host);
  }
  printf("traceroute to %s(%s): %d hopes_max,%d data bytes\n",
          ai->ai_canonname,sock_ntop(ai->ai_addr,ai->ai_addrlen),max_ttl,datalen);
  if(ai->ai_family == AF_INET)
    pr = &proto_v4;
#ifdef IPV6
  else if(ai->ai_family == AF_INET6)
   pr = &proto_v6;
#endif

   pr->sasend = ai->ai_addr;
   pr->sarecv = (struct sockaddr *)calloc(1,sizeof(ai->ai_addrlen));
   pr->salast = (struct sockaddr *)calloc(1,sizeof(ai->ai_addrlen));
   pr->sabind = (struct sockaddr *)calloc(1,sizeof(ai->ai_addrlen));
   traceloop();
   return 0;
}

void traceloop(void)
{
  int seq,code,done;
  double rtt;
  struct rec *rec;
  struct timeval tvrecv;
  
  recvfd = socket(pr->sasend->sa_family,SOCK_RAW,pr->icmpproto);
  sendfd = socket(pr->sasend->sa_family,SOCK_DGRAM,pr->icmpproto);
  setuid(getuid());
  pr->sabind->sa_family = pr->sasend->sa_family;
  sport = (getpid() & 0xffff)| 0x8000;/* UDP source port*/
  sock_set_port(pr->sabind,pr->salen,htons(sport));
  bind(sendfd,pr->sabind,pr->salen);

  sig_alrm(SIGALRM);
  seq = 0;
  done = 0;
  for(ttl = 1;ttl <= max_ttl&&done == 0;ttl++){
    setsockopt(sendfd,pr->ttllevel,pr->ttloptname,&ttl,sizeof(int));
    bzero(pr->salast,pr->salen);
    printf("%2d ",rtt);
    fflush(stdout);

    for(probe = 0;probe < nprobes;probe++){
      rec = (struct rec*)sendbuf;
      rec->rec_seq = ++seq;
      rec->rec_ttl = ttl;
      gettimeofday(&rec->rec_tv,NULL);
      sock_set_port(pr->sasend,pr->salen,htons(dport+seq));
      sendto(sendfd,sendbuf,datalen,0,pr->sasend,pr->salen);

     if((code = (*pr->frecv)(seq,&tvrecv)) == -3)
       printf("* ");
     else{
       char str[NI_MAXHOST];
       if(sock_cmp_addr(pr->sarecv,pr->salast,pr->salen) != 0){
         if(getnameinfo(pr->sarecv,pr->salen,str,sizeof(str),
	 NULL,0,0) == 0){
	   printf("%s(%s)",str,sock_ntop(pr->sarecv,pr->salen));
	 }else
	   printf("%s",sock_ntop(pr->sarecv,pr->salen));
	   memcpy(pr->salast,pr->sarecv,pr->salen);
       }
       tv_sub(&tvrecv,&rec->rec_tv);
       rtt = tvrecv.tv_sec * 1000.0 + tvrecv.tv_usec/1000.0;
       printf("%.3f ms",rtt);

       if(code == -1)
         done++;
       else if(code >= 0)
         printf("(ICMP %s)",(*pr->icmpcode)(code));
     }
     fflush(stdout);
    }
    printf("\n");
  }
}

int recv_v4(int seq,struct timeval *tv)
{
  int hlen1,hlen2,icmplen;
  socklen_t len;
  ssize_t n;
  struct ip *ip,*hip;
  struct icmp *icmp;
  struct udphdr *udp;
  alarm(3);
  for(;;){
    len = pr->salen;
    n = recvfrom(recvfd,recvbuf,sizeof(recvbuf),0,pr->sarecv,&len);
    if(n < 0){
      if(errno == EINTR)
        return(-3);
    }else{
      printf("recvfrom error\n");
      exit(0);
    }
    gettimeofday(tv,NULL);
    /*get ip header*/
    ip = (struct ip *)recvbuf;
    /*get icmp header*/
    hlen1 = ip->ip_hl << 2;
    /*start with icmp header*/
    icmp = (struct icmp*)(recvbuf + hlen1);
    
    if((icmplen = n - hlen1) < 8){
      printf("Icmplen < 8 \n");
      exit(0);
    }
    if(icmp->icmp_type == ICMP_TIMXCEED && 
       icmp->icmp_code == ICMP_TIMXCEED_INTRANS){
       if(icmplen < 20+8+8){
         printf("icmplen(%d)<8+20+8",icmplen);
	 exit(0);
       }
    hip = (struct ip*)(recvbuf + hlen1 + 8);
    hlen2 = hip->ip_hl << 2;
    udp = (struct udphdr *)(recvbuf+hlen1+8+hlen2);

    if(hip->ip_p == IPPROTO_UDP&&
       udp->source == htons(sport)&&
       udp->dest == htons(dport+seq)){
       return(-2);
    }
  }else if(icmp->icmp_type == ICMP_UNREACH){
     if(icmplen < 20+8+8){
       printf("icmplen(%d)<8+20+8",icmplen);
       exit(0);
     }
     hip = (struct ip*)(recvbuf + hlen1 + 8);
     hlen2 = hip->ip_hl << 2;
     udp = (struct udphdr*)(recvbuf + hlen1 + 8 + hlen2);
     if(hip->ip_p == IPPROTO_UDP&&
        udp->source == htons(sport)&&
	udp->dest == htons(dport+seq)){
	if(icmp->icmp_code == ICMP_UNREACH_PORT)
	  return (-1);
     }else
       return(icmp->icmp_code);

  }else if(verbose){
    printf("(from:%s:type = %d,code = %d)\n",
           sock_ntop(pr->sarecv,pr->salen),
	   icmp->icmp_type,icmp->icmp_code);
    
  }
  }
}

void sig_alrm(int signo)
{
  return;
}



















