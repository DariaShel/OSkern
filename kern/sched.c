#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/env.h>
#include <kern/monitor.h>


struct Taskstate cpu_ts;
_Noreturn void sched_halt(void);

int check_signal_in_queue(int cur_id){
    int i = envs[cur_id].que_start_position;
    int num = 0;
    while (i < MAX_QUEUE_LEN && i < envs[cur_id].que_start_position + envs[cur_id].que_members_num){
        if (envs[cur_id].queue[i].signo == envs[cur_id].waiting_signal){
            envs[cur_id].env_status = ENV_RUNNABLE;
            envs[cur_id].que_start_position = (envs[cur_id].que_start_position + num) % MAX_QUEUE_LEN;
            envs[cur_id].que_members_num -= num;
            return i;
        }
        i++;
        num++;
    }
    if (i == MAX_QUEUE_LEN - 1 && envs[cur_id].que_start_position + envs[cur_id].que_members_num > MAX_QUEUE_LEN - 1){
        i = 0;
        while (i < (envs[cur_id].que_start_position + envs[cur_id].que_members_num) % MAX_QUEUE_LEN){
            if (envs[cur_id].queue[i].signo == envs[cur_id].waiting_signal){
                envs[cur_id].env_status = ENV_RUNNABLE;
                envs[cur_id].que_start_position = (envs[cur_id].que_start_position + num) % MAX_QUEUE_LEN;
                envs[cur_id].que_members_num -= num;
                return i;
            }
            i++;
            num++;
        }
    }
    return -1;
}

/* Choose a user environment to run and run it */
_Noreturn void
sched_yield(void) {
    /* Implement simple round-robin scheduling.
     *
     * Search through 'envs' for an ENV_RUNNABLE environment in
     * circular fashion starting just after the env was
     * last running.  Switch to the first such environment found.
     *
     * If no envs are runnable, but the environment previously
     * running is still ENV_RUNNING, it's okay to
     * choose that environment.
     *
     * If there are no runnable environments,
     * simply drop through to the code
     * below to halt the cpu */

    // LAB 3: Your code here:
    int cur_id, parent_id;
	if (curenv) {
		cur_id = ENVX(curenv->env_id);
	} else {
		cur_id = 0;
	}
	parent_id = cur_id;
	while (1) {
		cur_id = (cur_id + 1) % NENV;
        if (envs[cur_id].env_status == ENV_WAITING_SIGNAL){
            check_signal_in_queue(cur_id);
        }

		if (envs[cur_id].env_status == ENV_RUNNABLE) {
			env_run(&envs[cur_id]);
		}
		if (parent_id == cur_id) {
			if (envs[cur_id].env_status == ENV_RUNNING) {
				env_run(&envs[cur_id]);
			}
			break;
		}
	}
    cprintf("Halt\n");
    /* No runnable environments,
     * so just halt the cpu */
    sched_halt();
}

/* Halt this CPU when there is nothing to do. Wait until the
 * timer interrupt wakes it up. This function never returns */
_Noreturn void
sched_halt(void) {

    /* For debugging and testing purposes, if there are no runnable
     * environments in the system, then drop into the kernel monitor */
    int i;
    for (i = 0; i < NENV; i++)
        if (envs[i].env_status == ENV_RUNNABLE ||
            envs[i].env_status == ENV_RUNNING) break;
    if (i == NENV) {
        cprintf("No runnable environments in the system!\n");
        for (;;) monitor(NULL);
    }

    /* Mark that no environment is running on CPU */
    curenv = NULL;

    /* Reset stack pointer, enable interrupts and then halt */
    asm volatile(
            "movq $0, %%rbp\n"
            "movq %0, %%rsp\n"
            "pushq $0\n"
            "pushq $0\n"
            "sti\n"
            "hlt\n" ::"a"(cpu_ts.ts_rsp0));

    /* Unreachable */
    for (;;)
        ;
}
