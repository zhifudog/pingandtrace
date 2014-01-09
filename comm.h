#ifndef _COMM_H_
#define _COMM_H_
#include<unistd.h>
#include<stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include<string.h>

struct addrinfo * Host_serv(const char * host,const char *serv,int family,int socktype)
{
  int n;
  struct addrinfo hints,*res;
  bzero(&hints,sizeof(struct addrinfo));
  hints.ai_flags = AI_CANONNAME;
  hints.ai_family = family;
  hints.ai_socktype = socktype;

  if((n = getaddrinfo(host,serv,&hints,&res)) != 0)
    return 0;
    return (res);
}

char *sock_ntop(const struct sockaddr *sa,socklen_t addrlen)
{
  char portstr[7];
  static char str[128];
  switch(sa->sa_family){
    case AF_INET:{
      struct sockaddr_in *sin = (sockaddr_in *)sa;
      if(inet_ntop(AF_INET,&sin->sin_addr,str,sizeof(str)) == NULL){
        return (NULL);
      }
      if(ntohs(sin->sin_port) != 0){
        snprintf(portstr,sizeof(portstr),".%d",ntohs(sin->sin_port));
	strcat(str,portstr);
      }
    } 
  }
  return (str);
}

void sock_set_port(sockaddr *sa,socklen_t len,short port)
{
  struct sockaddr_in *timp;
  timp = (sockaddr_in *)sa;
  timp->sin_port = port;
  return;
}

int sock_cmp_addr(struct sockaddr *dest,struct sockaddr *src,socklen_t len)
{
  struct sockaddr_in *sa1,*sa2;
  sa1 = (struct sockaddr_in *)dest;
  sa2 = (struct sockaddr_in *)src;

  if((sa1->sin_family == sa2->sin_family) &&
      (sa1->sin_port == sa2->sin_port) &&
      (sa1->sin_addr.s_addr == sa2->sin_addr.s_addr)){
        return 0;
   }
   else
    return -1;
}

void tv_sub(struct timeval *out,struct timeval *in)
{
  if((out->tv_usec -= in->tv_usec) < 0){
       --out->tv_sec;
       out->tv_usec += 1000000;
  }
  out->tv_sec -= in->tv_sec;
}

#endif
