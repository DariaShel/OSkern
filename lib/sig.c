//
// Created by Илья Русецкий on 26.12.2021.
//
#include <inc/lib.h>

void sig_ign(int arg){
    ;
}

void sig_dfl(int arg){
    ;
}

int
sigqueue(int eid, int signo, const union sigval value){
    return sys_sigqueue(eid, signo, value);
}

int
sigwait(const sigset_t* set, int sig){
    return sys_sigwait(set, sig);
}

int
sigaction(int sig, const struct sigaction* act, struct sigaction* oact){
    return sys_sigaction(sig, act, oact);
}