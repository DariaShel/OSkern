//
// Created by Илья Русецкий on 07.01.2022.
//
#include <inc/lib.h>

void handler_for_child(int signo){
    cprintf("C. receive signal (%d)\n", signo);
}

void
umain(int argc, char **argv) {
    cprintf("SIGNAL'S TEST BEGINS:\n");

    int r;
    if ((r = fork()) == 0){

        struct sigaction SigAct;
        SigAct.sa_handler = &handler_for_child;
        sigaction(1, &SigAct, 0);
        int i = 0;
        while (i < 20){
            cprintf("C. before waiting.\n");
            i++;
        }
        sys_sigwait(0, 1);
        cprintf("C. received signal and stops waiting!\n");
        cprintf("C. ends\n");
    } else{
        int i = 0;
        while (i < 150){
            cprintf("P. waiting.\n");
            i++;
        }
        cprintf("P. sends signal.\n");
        sigqueue(r, 1, (const union sigval)0);

        wait(r);
        cprintf("P. ends\n");
    }
}
