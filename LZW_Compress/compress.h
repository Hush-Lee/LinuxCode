#include <string.h>
#include <iostream>
#include <fstream>

const int DIVISOR =4099,
          MAX_CODES=4096,
          BYTE_SIZE=8,
          EXCESS=4,
          ALPHA = 256,
          MASK1=255,
          MASK2 =15;
typedef std::pair<const long,int> pairType;
int leftOver;
bool bitsLeftOver=false;
std::ifstream in;
std::ofstream out;

void setFiles(int argc, char *argv[]){
    char outputFile[50],inputFile[54];
    if(argc!= 2){
        std::cout<<"Enter the name of the input file: "<<std::endl;
        std::cin>>inputFile;
    }else{
        strcpy(inputFile,argv[1]);
    }
    in.open(inputFile);
    if(in.fail()){
        std::cerr<<"Cannot open input file"<<std::endl;
        exit(1);
    }
    strcpy(outputFile,inputFile);
    strcat(outputFile,".zzz");
    out.open(outputFile,std::ios::binary);
}
void output(long pcode){
    int c,d;
    if(bitsLeftOver){
        d=int(pcode&MASK1);
        c=int((leftOver<<EXCESS)|(pcode>>BYTE_SIZE));
        out.put(c);
        out.put(d);
        bitsLeftOver=false;
    }else{
        leftOver=pcode&MASK2;
        c=int(pcode>>BYTE_SIZE);
        out.put(c);
        bitsLeftOver=true;
    }
    
}
void compress(){
    hashChains<long,int> h(DIVISOR);
}

int main(int argc, char *argv[]){
    setFiles(argc,argv);
    compress();
}