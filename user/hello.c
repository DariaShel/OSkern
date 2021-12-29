/* hello, world */
#include <inc/lib.h>

void handler(int signo){
    cprintf("I am child and I receive signal (%d)\n", signo);
}

void
umain(int argc, char **argv) {
    cprintf("hello, world\n");
    cprintf("i am environment %08x\n", thisenv->env_id);

    cprintf("SIGNAL'S TEST BEGINS:\n");
    int r;
    if ((r = fork()) == 0){
        struct sigaction SigAct;
        SigAct.sa_handler = SIG_IGN;
        sigaction(1, &SigAct, 0);
        int i = 0;
        while(i < 300){
            printf("Child: %d\n", i++);
        }
    } else{
        int i = 0;
        while(i < 300){
            printf("Parent: %d\n", i++);
        }
        int s = sigqueue(r, 1, (const union sigval)0);
        printf("Sigqueue(son_id, 1, 0) returned %d\n", s);
    }
}
