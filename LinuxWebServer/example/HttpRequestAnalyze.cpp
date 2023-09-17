/*
 * HttpRequestAnalyze.cpp
 *
 *  Created on: Sep 14, 2023
 *      Author: hush-lee
 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
const int BUFFER_SIZE=4096;
//主状态机的两种可能状态：正在分析请求行，正在分析头部字段
enum CHECK_STATE{CHECK_STATE_REQUESTLINE=0,CHECK_STATE_HEADER};
//从状态机：读取到一个完整的行，行出错和行数据尚且不完整
enum LINE_STATUS{LINE_OK=0,LINE_BAD,LINE_OPEN};
/*
 *服务处理结果：NO_REQUEST表示请求不完整，继续读取
 *GET_REQUEST:获取完整请求
 *BAD_REQUEST:客户请求有语法错误
 *FORBIDDEN_REQUEST:无访问权限
 *INTERNAL_ERROR:服务群内部错误
 *CLOSED_CONNECTION:客户端连接关闭
 */
 enum HTTP_CODE{NO_REQUEST,GET_REQUEST,BAD_REQUEST,FORBIDDEN_REQUEST,INTERNAL_ERROR,CLODE_CONNECTION};
 
 static const char * szret[]={"I got a correct result \n","Something went wrong \n"};
 
 LINE_STATUS parse_line(char * buffer,int &checked_index, int & read_index){
	 char temp;
	 for(;checked_index<read_index;++checked_index){
		temp=buffer[checked_index];
		if(temp=='\r'){
			if((checked_index+1)==read_index){
				return LINE_OPEN;
			}else if(buffer[checked_index+1]=='\n'){
				buffer[checked_index++]='\0';
				buffer[checked_index++]='\0';
				return LINE_OK;
			}
			return LINE_BAD;
		}else if(temp=='\n'){
			if((checked_index > 1)&& buffer[checked_index-1]=='\r'){
				buffer[checked_index++]='\0';
				buffer[checked_index++]='\0';
				return LINE_OK;
			}
			return LINE_BAD;
		}
	 }
	 return LINE_OPEN;
 }

HTTP_CODE parse_requestline(char*temp,CHECK_STATE&checkstate){
	char *url=strpbrk(temp,"\t");
	if(!url){
		return BAD_REQUEST;
	}
	*url++='\0';
	char * method = temp;
	if(strcasecmp(method,"GET")==0){
		printf("The request method is GET\n");
	}else{
		return BAD_REQUEST;
	}
	url+=strspn(url,"\t");
	char * version =strpbrk(url,"\t");
	if(!version){
		return BAD_REQUEST;
	}
	*version++ = '\0';
	version+=strspn(version,"\t");
	if(strcasecmp(version,"HTTP/1.1")!=0){
		return BAD_REQUEST;
	}
	if(strncasecmp(url,"http://",7)==0){
		url+=7;
		url=strchr(url,'/');
	}
	if(!url || url[0]!='/'){
		return BAD_REQUEST;
	}
	printf("The Request URL is:%s\n",url);
	checkstate=CHECK_STATE_HEADER;
	return NO_REQUEST;
}

HTTP_CODE parse_headers(char*temp){
	if(temp[0]=='\0'){
		return GET_REQUEST;
	}else if(strncasecmp(temp,"Host:",5)==0){
		temp+=5;
		temp+=strspn(temp,"\t");
		printf("the request host is %s\n",temp);
	}else{
		printf("I can not handle this header\n");
	}
	return NO_REQUEST;
}

HTTP_CODE parse_content (char * buffer,int & checked_index,CHECK_STATE&checkstate,int &read_index,int & start_line){
	LINE_STATUS linestatus =LINE_OK;
	HTTP_CODE retcode = NO_REQUEST;
	return NO_REQUEST;
}
 
 



