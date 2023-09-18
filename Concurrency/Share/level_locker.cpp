#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <pthread.h>
const int BUFFER_SIZE=10;
const int MAX_EVENT_NUMBER=1024;

int setnonblocking(int fd){
	int old_option = fcntl(fd,F_GETFL);
	int new_option = old_option|O_NONBLOCK;
	fcntl(fd,F_SETFL,new_option);
	return old_option;
}

void addfd(int epollfd,int fd,bool enable_et){
	epoll_event event;
	event.data.fd=fd;
	event.events=EPOLLIN;
	if(enable_et){
		event.events|=EPOLLET;
	}
	epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
	setnonblocking(fd);
}

void lt(epoll_event*events,int number,int epollfd,int listenfd){
	char buf[BUFFER_SIZE];
	for(int i=0;i<number;i++){
		int sockfd=events[i].data.fd;
		if(sockfd==listenfd){
			struct sockaddr_in client_address;
			socklen_t client_addrlength=sizeof(client_address);
			int connfd=accept(listenfd,(struct sockaddr*)&client_address,&client_addrlength);
			addfd(epollfd,connfd,false);
		}else if(events[i].events&EPOLLIN){
			printf("event trigger once\n");
			memset(buf,'\0',BUFFER_SIZE);
			int ret = recv(sockfd,buf,BUFFER_SIZE-1,0);
			if(ret<=0){
				close(sockfd);
				continue;
			}
			printf("get %d bytes of content:%s\n",ret,buf);
		}else{
			printf("something else happend\n");
		}
	}
}

void et(epoll_event*events,int number,int epollfd,int listenfd){
	char buf[BUFFER_SIZE];
	for(int i=0;i<number;i++){
		int sockfd=events[i].data.fd;
		if(sockfd==listenfd){
			struct sockaddr_in client_address;
			socklen_t client_addrlength = sizeof(client_address);
			int connfd= accept(listenfd,(struct sockaddr*)&client_address,&client_addrlength);
			addfd(epollfd,connfd,true);
		}else if(events[i].events&EPOLLIN){
			printf("event trigger once\n");
			while(1){
				memset(buf,'\0',BUFFER_SIZE);
				int ret = recv(sockfd,buf,BUFFER_SIZE-1,0);
				if(ret<0){
					if((errno==EAGAIN)||(error==EWOULDBLOCK)){
						printf("read later\n");
						break;
					}
				}
			}
		}
		
	}
}