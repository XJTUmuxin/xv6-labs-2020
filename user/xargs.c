#include "kernel/types.h"
#include "user/user.h"
#include "kernel/stat.h"
#include "kernel/param.h"

int main(int argc, char *argv[]){
    char buf[512] = {0};
    char *argbuf[5];
    char *p = buf;
    for(int i=1;i<argc;++i){
        argbuf[i-1] = argv[i];
    }
    argbuf[argc-1] = buf;
    while(read(0,p,1)==1){
        if(*p == '\n'){
            *p-- = 0;
            if(fork() == 0){
                //exec(argv[1],argv+2);
                exec(argbuf[0],argbuf);
                exit(0);
            }
            else{
                memset(buf,0,p-buf);
                p = buf;
            }
        }
        else{
            p++;
        }
    }
    while(wait(0)!=-1){}
    exit(0);
}