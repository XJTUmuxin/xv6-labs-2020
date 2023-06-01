#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void fun(int *pl){
    int num;
    read(pl[0],&num,sizeof(num));
    if(num == -1){
        exit(0);
    }
    printf("prime %d\n",num);
    int pr[2];
    pipe(pr);
    if(fork()!=0){  //parent
        int temp;
        while (read(pl[0],&temp,sizeof(temp)))
        {
            if(temp%num!=0){
                write(pr[1],&temp,sizeof(temp));
            }   
        }
        close(pl[0]);
        close(pr[1]);
        wait(0);
    }
    else{       //child
        close(pr[1]);
        fun(pr);
    }
}
int main(int argc,char *argv[]){
    if(argc!=1){
        fprintf(2,"argument number invaild!\n");
        exit(1);
    }
    int p1[2];
    pipe(p1);
    if(fork()!=0){  //parent
        int nums[35-2+2];
        for(int i=2;i<=35;++i){
            nums[i-2] = i;
        }
        nums[35-2+1] = -1;
        write(p1[1],(char*)nums,sizeof(nums));
        close(p1[0]);
        close(p1[1]);
        wait(0);
    }
    else{   //child
        close(p1[1]);
        fun(p1);
    }
    exit(0);
}