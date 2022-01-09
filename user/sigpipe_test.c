//
// Created by Илья Русецкий on 08.01.2022.
//
#include <inc/lib.h>

#include <inc/lib.h>

int flag_p;
int flag_c;

void handler_for_sigpipe(int signo){
    cprintf("C. received SIGPIPE\n");
    flag_c = 0;
}

void handler_for_parent(int signo){
    cprintf("P. received signal (%d)\n", signo);
    flag_p = 0;
}


void
umain(int argc, char **argv) {
    cprintf("SIGPIPE'S TEST BEGINS:\n");

    char buf[128];
    char path_fifo[] = "my_fifo";
    char test_string[] = "This is a message for testing FIFO file";
    int test_len = strlen(test_string);
    int fifo_read_fd, fifo_write_fd;

    cprintf("mkfifo result = %d\n", mkfifo(path_fifo));

    struct sigaction SigAct1;
    SigAct1.sa_handler = &handler_for_parent;
    sigaction(1, &SigAct1, 0);

    int child;
    if ((child = fork()) == 0){

        struct sigaction SigAct2;
        SigAct2.sa_handler = &handler_for_sigpipe;
        sigaction(SIGPIPE, &SigAct2, 0);

        fifo_write_fd = open(path_fifo, O_WRONLY);
        cprintf("fifo_write_fd = %d\n", fifo_write_fd);

        cprintf("C. writes to channel, result: %ld\n", write(fifo_write_fd, test_string, test_len));

        cprintf("C. is ready for SIGPIPE\n");
        sigqueue(sys_get_parent_envid(), 1, (const union sigval)0);

        wait(sys_get_parent_envid());

        cprintf("C. writes to channel, result: %ld\n",  write(fifo_write_fd, test_string, test_len));

        close(fifo_write_fd);
        cprintf("C. ends\n");
    } else{
        fifo_read_fd = open(path_fifo, O_RDONLY);
        cprintf("fifo_read_fd = %d\n", fifo_read_fd);

        flag_p = 1;
        while (flag_p){
            cprintf("P. is waiting\n");
        }

        cprintf("P. read buf from fifo, result %ld:", read(fifo_read_fd, buf, 8));
        cprintf("   %s\n", buf);

        cprintf("P. is closing fifo\n");
        close(fifo_read_fd);

        cprintf("P. ends\n");
    }
}

