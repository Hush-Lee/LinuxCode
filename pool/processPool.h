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
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>

class process{
public:
    process():m_pid(-1){}
    pid_t m_pid;
    int m_pipefd[2];
};

template<typename T>
class processPool{
    processPool(int listenfd,int process_number=8);
public:
    static processPool<T>* create(int listenfd,int process_number=8){
        if(!m_instance){
            m_instance = new processPool<T>(listenfd,process_number);
        }
        return  m_instance;
    }
    ~processPool(){
        delete [] m_sub_process;
    }
    void run();
private:
    void setup_sig_pipe();
    void run_parent();
    void run_child();
    //static function can only use static members data;
    static const int MAX_PROCESS_NUMBER = 16;

    static const int USER_PER_PROCESS=65535;

    static const int MAX_EVENT_NUMBER=10000;

    int m_process_number;

    int m_idx;

    int m_epollfd;

    int m_listenfd;

    int m_stop;

    process* m_sub_process;

    static processPool <T> * m_instance;
};

template<typename T> 
processPool<T>*processPool<T>::m_instance = nullptr;

static int sig_pipefd[2];
static int setNonblocking(int fd){
    int old_options = fcntl(fd, F_GETFL, 0);
    int new_options = old_options | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_options);
    return old_options;
}

static void addfd(int epollfd,int fd){
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD,fd,&event);
    setNonblocking(fd);
}

static void removefd(int epollfd,int fd){
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,0);
    close(fd);
}
static void sig_handler(int sig){
    int save_errno = errno;
    int msg = sig;
    send(sig_pipefd[1],(char*)&msg,1,0);
    errno = save_errno;
}

 static void addsig(int sig,void(handler)(int),bool restart=true){
    struct sigaction sa;
    memset(&sa,0,sizeof(sa));
    sa.sa_handler=handler;
    if(restart){
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig,&sa,nullptr)!=-1);
 }

 template<typename T>
 processPool<T>::processPool(int listenfd,int process_number):m_listenfd(listenfd),
                                m_process_number(process_number),m_idx(-1),m_stop(false){
    assert((process_number>0)&&(process_number<MAX_PROCESS_NUMBER));
    m_sub_process=new process[process_number];
    assert(m_sub_process);
    for(int i=0; i<MAX_PROCESS_NUMBER;++i){
        int ret=socketpair(PF_UNIX,SOCK_STREAM,0,m_sub_process[i].m_pipefd);
        assert(ret==0);
        m_sub_process[i].m_pid=fork();
        assert(m_sub_process[i].m_pid>=0);
        if(m_sub_process[i].m_pid>0){
            close(m_sub_process[i].m_pipefd[1]);
            continue;
        }else{
            close(m_sub_process[i].m_pipefd[0]);
            m_idx = i;
            break;
        }
    }
 }

 template<typename T>
 void processPool<T>::setup_sig_pipe(){
    m_epollfd=epoll_create(5);
    assert(m_epollfd!=-1);
    int ret =socketpair(PF_UNIX,SOCK_STREAM,0,sig_pipefd);
    assert(ret!=-1);
    setNonblocking(sig_pipefd[1]);
    addfd(m_epollfd,sig_pipefd[0]);

    addsig(SIGCHLD,sig_handler);
    addsig(SIGINT,sig_handler);
    addsig(SIGTERM,sig_handler);
    addsig(SIGFPE,SIG_IGN);

 }

 template <typename T>
 void processPool<T>::run() {
    if(m_idx!=-1){
        run_child();
        return ;
    }
    run_parent();
 }


template <typename T>
void processPool<T>::run_child(){
    setup_sig_pipe();
    //每个子进程都通过其在进程池中的序号值m_idx找到与父进程通信的管道
    int pipefd=m_sub_process[m_idx].m_pipefd[1];
    //子进程需要监听管道文件描述符pipefd,因为父进程将通过它来通知子进程accept新连接
    addfd(m_epollfd,pipefd);
    epoll_event events[MAX_EVENT_NUMBER];
    T*users=new T[USER_PER_PROCESS];
    assert(users);
    int number=0;
    int ret=-1;
    while(!m_stop){
        number=epoll_wait(m_epollfd,events,MAX_EVENT_NUMBER,-1);
        if((number<0)&&(errno!=EINTR)){
            printf("epoll failure\n");
            break;
        }
        for(int i=0;i<number;i++){
            int sockfd=events[i].data.fd;
            if((sockfd==pipefd)&&(events[i].events&EPOLLIN)){
                int client=0;
                ret=recv(sockfd,(char *)&client,sizeof(client),0);
                if(((ret<0)&&(errno!=EAGAIN))||ret==0){
                    continue;
                }else{
                    struct sockaddr_in client_address;
                    socklen_t client_address_len = sizeof(client_address);
                    int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_address_len);
                    if(connfd<0){
                        printf("errno is: %d\n", errno);
                        continue;
                    }
                    addfd(m_epollfd, connfd);
                    users[connfd].init(m_epollfd, connfd,client_address);
                }
            }else if((sockfd==sig_pipefd[0])&&(events[i].events&EPOLLIN)){
                int sig;
                char signals[1024];
                ret=recv(sig_pipefd[0], signals, sizeof(signals),0);
                if(ret<=0){
                    continue;
                }else{
                    for(int i=0; i<ret;++i){
                        switch(signals[i]){
                            case SIGCHLD:{
                                pid_t pid ;
                                int stat;
                                while((pid=waitpid(-1,&stat,WNOHANG))>0){
                                    continue;
                                }
                                break;
                            }
                            case SIGTERM:
                            case SIGINT:{
                                m_stop=true;
                                break;
                            }
                            default:
                                break;
                        }
                    }
                }
            }else if(events[i].events&EPOLLIN){
                users[sockfd].process();
            }else{
                continue;
            }
        }
    }
    delete[] users;
    users=nullptr;
    close(pipefd);
    close(m_epollfd);
}

template<typename T>
void processPool<T>::run_parent(){
    setup_sig_pipe();
    addfd(m_epollfd,m_listenfd);
    epoll_event events[MAX_EVENT_NUMBER];
    int sub_process_counter =0;
    int new_conn=1;
    int number=0;
    int ret=1;
    while(!m_stop) {
        number=epoll_wait(m_epollfd,events,MAX_EVENT_NUMBER,-1);
        if((number<0)&&(errno!=EINTR)){
            printf("epoll failure\n");
            break;
        }
        for(int i=0;i<number;++i){
            int sockfd = events[i].data.fd;
            if(sockfd==m_listenfd){
                int i = sub_process_counter;
                do{
                    if(m_sub_process[i].m_pid!=-1){
                        break;
                    }
                    i=(i+1)%m_process_number;
                }while(i!=sub_process_counter);
                if(m_sub_process[i].m_pid==-1){
                    m_stop=true;
                    break;
                }
                sub_process_counter=(i+1)%m_process_number;
                send(m_sub_process[i].m_pipefd[0],(char*)&new_conn,sizeof(new_conn),0);
                printf("send request to child%d\n",i);
            }else if((sockfd==sig_pipefd[0])&&(events[i].events&EPOLLIN)){
                int sig;
                char signals[1024];
                ret = recv(sig_pipefd[0],signals,sizeof(signals),0);
                if(ret<=0){
                    continue;
                }else{
                    for(int i=0;i<ret;++i){
                        switch(signals[i]){
                            case SIGCHLD:{
                                pid_t pid;
                                int stat;
                                while ((pid=waitpid(-1,&stat,WNOHANG))>0){
                                    for(int i=0;i<m_process_number;++i){
                                        if(m_sub_process[i].m_pid == pid){
                                            printf("child%d join\n",i);
                                            close(m_sub_process[i].m_pid=-1);
                                            m_sub_process[i].m_pid=-1;
                                        }
                                    }
                                }
                                m_stop = true;
                                for(int i=0;i<m_process_number;++i){
                                    if(m_sub_process[i].m_pid !=-1){
                                        m_stop = false;
                                    }
                                }
                                break;
                            }
                            case SIGTERM:
                            case SIGINT:{
                                printf("kill all the child now\n");
                                for(int i=0;i<m_process_number;++i){
                                    int pid=m_sub_process[i].m_pid;
                                    if(pid!=-1){
                                        kill(pid, SIGTERM);
                                    }
                                }
                                break;
                            }
                            default:{
                                break;
                            }
                        }
                    }
                }
            }else{
                continue;
            }
        }
    }
    close(m_epollfd);
}