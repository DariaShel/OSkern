//
// Created by Илья Русецкий on 24.12.2021.
//

#ifndef IZ_SIG_H
#define IZ_SIG_H

typedef int sigset_t;
typedef int pid_t;
typedef int uid_t;

enum sig_nums {
    SIG0 = 0,
    SIG1,
    SIG2,
    NUM_SIG
};

enum sig_handling {
    ENV_NOT_HANDLING_SIG = 0,
    ENV_HANDLING_SIG
};

void sig_ign(int);
void sig_dfl(int);

#define SIG_DFL &sig_dfl
#define SIG_IGN &sig_ign

#define MAX_QUEUE_LEN 20

union sigval{
    int sival_int;
    void *sival_ptr;
};

struct siginfo_t{
    int si_signo;
    int si_code;
    int si_errno;
    pid_t si_pid;
    uid_t si_uid;
    void *si_addr;
    int si_status;
    long si_band;
    union sigval si_value;
};

struct sigaction{
    void (*sa_handler)(int);
    sigset_t sa_mask;
    int sa_flags;
    void (*sa_sigaction)(int, struct siginfo_t *, void *);
};


int sigqueue(int eid, int signo, const union sigval value);
int sigwait(const sigset_t* set, int* sig);
int sigaction(int sig, const struct sigaction* act, struct sigaction* oact);

#endif // IZ_SIG_H
