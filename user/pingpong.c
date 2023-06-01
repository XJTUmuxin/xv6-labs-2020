#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int main(int argc, char *argv[]){
    int p1[2],p2[2];
    pipe(p1);   //parent to child
    pipe(p2);   //child to parent
    
    if(fork() != 0){    //parent
        char buf;
        write(p1[1],"a",1);
        read(p2[0],&buf,1);
        printf("%d: received pong\n",getpid());
        wait(0);
    }
    else{               //child
        char buf;
        read(p1[0],&buf,1);
        printf("%d: received ping\n",getpid());
        write(p2[1],&buf,1);
    }
    exit(0);
}