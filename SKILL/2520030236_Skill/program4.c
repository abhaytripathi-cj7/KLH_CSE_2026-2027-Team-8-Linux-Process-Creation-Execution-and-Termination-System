#include<stdio.h>
#include<unistd.h>
int main() {
pid_t pid;
pid = fork();
if(pid==0)
{
printf("child PID = %d\n" , getpid());
printf("Parent PID = %d\n",getppid());
}
else {
printf("parent PID =%d\n", getpid());
printf("child PID=%d\n", pid);
}
return 0;
}
