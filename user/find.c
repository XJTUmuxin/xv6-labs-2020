#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/stat.h"

char path[512] = {0} , *p;
char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), 0 , DIRSIZ-strlen(p));
  return buf;
}
void find(char *path,char *file){
    int fd;
    struct dirent de;
    struct stat st;
    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }
    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }
    switch (st.type)
    {
    case T_FILE:
        if(strcmp(fmtname(path),file) ==0 ){
            printf("%s\n",path);
        }
        break;
    
    case T_DIR:
        if(strlen(path) + 1 + DIRSIZ + 1 > 512){
            printf("ls: path too long\n");
            break;
        }
        *p = '/';
        p++;
        while(read(fd, &de, sizeof(de)) == sizeof(de)){
            if(de.inum == 0)
                continue;
            if(strcmp(de.name,".") == 0 || strcmp(de.name,"..") == 0){
                continue;
            }
            memmove(p, de.name, DIRSIZ);
            int shift = 0;
            while (p[shift] != 0)
            {
                shift++;
            }
            p += shift;
            find(path,file);
            p -= shift;
            memset(p,0,DIRSIZ);

        }
        p--;
        *p = 0;
        break;
    }
    close(fd);
}

int main(int argc, char *argv[]){
    if(argc != 3){
        fprintf(2,"The argument number is invaild\n");
        exit(1);
    }
    strcpy(path,argv[1]);
    p = path+strlen(argv[1]);
    *p = 0;
    find(path,argv[2]);
    exit(0);
}