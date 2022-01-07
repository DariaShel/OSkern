//
// Created by Илья Русецкий on 06.01.2022.
//
#include <inc/lib.h>

int flag;
int flag_c;

void handler_for_child(int signo){
    cprintf("C. receive signal (%d)\n", signo);
    flag_c--;
}

void handler_for_parent(int signo){
    cprintf("P. receive signal (%d)\n", signo);
    flag = 0;
}


void
umain(int argc, char **argv) {
    cprintf("SIGNAL'S TEST BEGINS:\n");

    int r;
    if ((r = fork()) == 0){

        struct sigaction SigAct;
        SigAct.sa_handler = &handler_for_child;
        sigaction(1, &SigAct, 0);

        cprintf("C. send signal\n");
        sigqueue(sys_get_parent_envid(), 2, (const union sigval)0);

        flag_c = 5;
        while (flag_c > 0){
            cprintf("C. waits signal\n");
        }
        cprintf("C. ends\n");
    } else{

        struct sigaction SigAct;
        SigAct.sa_handler = &handler_for_parent;
        sigaction(2, &SigAct, 0);

        flag = 1;
        while (flag){
            cprintf("P. waits signal\n");
        }

        cprintf("P. send signal 1\n");
        sigqueue(r, 1, (const union sigval)0);
        cprintf("P. send signal 2\n");
        sigqueue(r, 1, (const union sigval)0);
        cprintf("P. send signal 3\n");
        sigqueue(r, 1, (const union sigval)0);
        cprintf("P. send signal 4\n");
        sigqueue(r, 1, (const union sigval)0);
        cprintf("P. send signal 5\n");
        sigqueue(r, 1, (const union sigval)0);

        cprintf("P. ends\n");
    }
}
