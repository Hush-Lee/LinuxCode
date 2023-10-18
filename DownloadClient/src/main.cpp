#include <cstdio>
#include <curl/curl.h>
#include <curl/easy.h>
#include <unistd.h>
#include <sys/fcntl.h>

size_t writeFunc(void *ptr, size_t size, size_t nmemb, void *userdata) {
    printf("write function \n");
    
    return size*nmemb;

}

long getDownloadFileLength(const char *url){
    long downloadFileLength = 0;
    CURL *curl=curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HEADER,1 );
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
    
    
    CURLcode res=curl_easy_perform(curl);
    if(res == CURLE_OK){
        curl_easy_getinfo(curl,CURLINFO_CONTENT_LENGTH_DOWNLOAD,&downloadFileLength);
    }else{
        downloadFileLength=-1;
    }
    curl_easy_cleanup(curl);
    return downloadFileLength;
}

int download(const char* url,const char* filename) {

    int fd = open(filename,O_RDWR|O_CREAT);    
    if(fd==-1){
         return -1;
    }
    long downloadFileLength = getDownloadFileLength(url);
    CURL *curl=curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunc);
    
    CURLcode res=curl_easy_perform(curl);
    if(res != CURLE_OK){
        printf("res: %d\n", res);
    }
    curl_easy_cleanup(curl);
    return 0;
}