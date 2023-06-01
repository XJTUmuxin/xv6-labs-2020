#include "kernel/types.h"
#include "user/user.h"
#include "kernel/stat.h"
#include "kernel/param.h"
int main(int argc, char *argv[]){
    for(int i=0;i<3;++i){
        printf("%d\n",argv[i]);
    }
    char *argvbuf[3];
    argvbuf[0] = "echo";
    argvbuf[1] = "qwerq";
    if(fork()==0){
        exec(argvbuf[0],argvbuf);
        exit(0);
    }
    else{
        wait(0);
    }
    exit(0);
}