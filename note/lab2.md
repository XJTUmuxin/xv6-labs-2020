# xv6 labs
## **lab2**:system calls
### **Exercise1** System call tracing
任务：

实现一个系统调用trace，trace系统调用接受一个int参数mask，mask对应位置位则对应的系统调用在调用时要打印一行信息，包括pid，系统调用的名称和返回值。trace应该跟踪调用它的进程和它的所有子进程

代码：

首先需要在 /kernel/proc.h 中的proc结构体中添加trace_mask变量
```c
struct proc {
   struct file *ofile[NOFILE];  // Open files
   struct inode *cwd;           // Current directory
   char name[16];               // Process name (debugging)
+  int trace_mask;              // Trace mask
 };
```
之后修改 /kernel/proc.c 中的fork函数，使得fork时，子进程复制父进程的trace_mask

```c
int
fork(void)
   }
   np->sz = p->sz;

+  np->trace_mask = p->trace_mask;   //copy the trace_mask to child process
+
   np->parent = p;
```

在/kernel/syscall.h 中定义trace对应的系统调用编号

```c
 #define SYS_link   19
 #define SYS_mkdir  20
 #define SYS_close  21
+#define SYS_trace  22
+#define SYS_num    23
```

在/kernel/sysproc.c中定义sys_trace函数，是真正实现trace系统调用的函数，将trace的参数赋给当前进程的trace_mask变量

```c
+uint64
+sys_trace(void){
+  int n;
+  if(argint(0,&n)<0){
+    return -1;
+  }
+  myproc()->trace_mask = n;
+  return 0;
+}
```

之后在 /kernel/syscall.c 中声明sys_trace函数，并在syscalls函数指针数组中添加sys_trace，并创建syscall_names字符串数组，记录不同系统调用编号对应的名称，同时修改syscall函数，使得调用完系统调用后打印trace信息

```c
 extern uint64 sys_uptime(void);
+extern uint64 sys_trace(void);
```
```c
 static uint64 (*syscalls[])(void) = {
 [SYS_link]    sys_link,
 [SYS_mkdir]   sys_mkdir,
 [SYS_close]   sys_close,
+[SYS_trace]   sys_trace,
 };
```

```c
+ static char* syscall_names[SYS_num] = {" ","fork","exit","wait","pipe","read","kill","exec","fstat","chdir","dup","getpid","sbrk","sleep","uptime","open","write","mknod","unlink","link","mkdir","close","trace"};
```

```c
void
syscall(void){
   num = p->trapframe->a7;
   if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
     p->trapframe->a0 = syscalls[num]();
+    int trace_mask = p->trace_mask;
+    if((1<<num) & trace_mask){
+      printf("%d: syscall %s -> %d\n",p->pid,syscall_names[num],p->trapframe->a0);
+    }
   } 
```

之后在/user/user.h中声明trace()函数，给用户空间添加系统调用接口

```c
 char* sbrk(int);
 int sleep(int);
 int uptime(void);
+int trace(int);
```

同时需要在/user/usys.pl脚本文件中添加trace，它生成实际的系统调用stub

```pl
 entry("sbrk");
 entry("sleep");
 entry("uptime");
+entry("trace");
```

grade 结果：

![](./image/trace.png)