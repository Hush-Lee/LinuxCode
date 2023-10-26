#include <fcntl.h>
#include <stdio.h>
#include <curl/curl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
struct fileInfo{
    char *fileptr;
    int offset;
};
size_t writeFunc(void *ptr, size_t size, size_t nmemb, void *userdata) {
    struct fileInfo * info = (struct fileInfo *)userdata;
    memcpy(info->fileptr+info->offset, ptr, size*nmemb);
    info->offset += size*nmemb;
    printf("Download\n");
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
        curl_easy_getinfo(curl,CURLINFO_CONTENT_LENGTH_DOWNLOAD_T,&downloadFileLength);
    }else{
        downloadFileLength=-1;
    }
    curl_easy_cleanup(curl);
    return downloadFileLength;
}

int download(const char* url,const char* filename) {
    long downloadFileLength = getDownloadFileLength(url);
    printf("DownloadFileLength: %ld\n",downloadFileLength);
    int fd = open(filename,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR);
    if(fd == -1){
        return -1;
    }
    if(lseek(fd, downloadFileLength-1, SEEK_SET)==-1){
        perror("lseek");
        close(fd);
        return -1;
    }
    if(write(fd,"",1)!=1){
        perror(" wrote");
        close(fd);
        return -1;
    }
    char* ptr = (char*)mmap(nullptr, downloadFileLength,PROT_READ|PROT_WRITE,MAP_SHARED, fd, 0);
    if(ptr ==MAP_FAILED){
        perror("mmap");
        close(fd);
        return -1;
    }
    struct fileInfo *info=new fileInfo();
    if(info==nullptr){
        
        munmap(ptr, downloadFileLength);
        close(fd);
        return -1;
    }
    info->fileptr=ptr;
    info->offset=0;
    printf("ready to download\n");
    CURL *curl=curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,info);
    CURLcode res=curl_easy_perform(curl);
    if(res != CURLE_OK){
        printf("res: %d\n", res);
    }
    printf("finished downloading\n");
    curl_easy_cleanup(curl);
    delete info;
    munmap(ptr, downloadFileLength);
    return 0;
}

int main(int argc,const char* argv[]){
    if(argc!=3){
        return -1;
    }
    return download(argv[1],argv[2]);
}
