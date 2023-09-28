/*
 * (C) 2022, Cornell University
 * All rights reserved.
 */

/* Author: Yunhao Zhang
 * Description: the system call interface to user applications
 *
 * Updated by CS6640 23fall staff
 */

/* syscall workflow:

sys_send/recv (syscall.c)
  |                                                         ^
  +-> sys_invoke (syscall.c)                              [ret]
        |                              USER SPACE           |
-----[trap]-------------------------------------------------|----
        |                               KERNEL              |
        +-> trap_entry (cpu_intr.c)                       [mret]
            |                                               |
            +-> trap_handler (kernel.c)                   [ret]
                [switch to kernel stack]          [switch to user stack]
                ...                                         |
                |                                           |
                +-> proc_syscall (syscall.c)              [ret]
                    |                                       |
                    +-> proc_send/recv (syscall.c)        [ret]
                        |                                   |
                        +-> sys_yield (scheduler.c)       [ret]
                            |                               |
                            +->[context switch]       [switch back]
                                  |                         |
                                  +-->[other proc running]--+
 */


#include "egos.h"
#include "syscall.h"
#include "process.h"
#include <string.h>


/* Syscall first half running in user-space */
static void sys_invoke() {
    struct syscall *sc = (struct syscall*)SYSCALL_ARG;
    /* set machine-mode software interrupt:
     * MSIP bit of mip to 1 */
    *((int*)0x2000000) = 1;
    while (sc->type != SYS_UNUSED);
}

void sys_yield() {
    struct syscall *sc = (struct syscall*)SYSCALL_ARG;
    sc->type = SYS_YIELD;
    sys_invoke();
}

int sys_send(int receiver, char* msg, int size) {
    if (size > SYSCALL_MSG_LEN) return -1;

    struct syscall *sc = (struct syscall*)SYSCALL_ARG;
    sc->type = SYS_SEND;
    sc->msg.receiver = receiver;
    memcpy(sc->msg.content, msg, size);
    sys_invoke();
    return sc->retval;
}

int sys_recv(int* sender, char* buf, int size) {
    if (size > SYSCALL_MSG_LEN) return -1;

    struct syscall *sc = (struct syscall*)SYSCALL_ARG;
    sc->type = SYS_RECV;
    sys_invoke();
    memcpy(buf, sc->msg.content, size);
    if (sender) *sender = sc->msg.sender;
    return sc->retval;
}

void sys_exit(int status) {
    struct proc_request req;
    req.type = PROC_EXIT;
    sys_send(GPID_PROCESS, (void*)&req, sizeof(req));
}

void sys_sleep(int time_units) {
    struct proc_request req;
    req.type = PROC_SLEEP;
    req.argc = 1;
    *(int*)req.argv = time_units;
    sys_send(GPID_PROCESS, (void*)&req, sizeof(req));
    // FIXME: a hacky-way of implementing sleep
    // ASSUMPTION: the GPID_PROC runs at least once before the sys_yield() returns
    sys_yield();
}

/* Syscall second half running in kernel-space */
static void proc_send(struct syscall *sc);
static void proc_recv(struct syscall *sc);

void proc_syscall() {
    struct syscall *sc = (struct syscall*)SYSCALL_ARG;

    int type = sc->type;
    sc->retval = 0;
    sc->type = SYS_UNUSED;
    /* set machine-mode software interrupt:
     * MSIP bit of mip to 0 */
    *((int*)0x2000000) = 0;

    switch (type) {
    case SYS_RECV:
        proc_recv(sc);
        break;
    case SYS_SEND:
        proc_send(sc);
        break;
    case SYS_YIELD:
        proc_yield();
        break;
    default:
        FATAL("proc_syscall: got unknown syscall type=%d", type);
    }
}

static void proc_send(struct syscall *sc) {
    sc->msg.sender = curr_pid;
    int receiver = sc->msg.receiver;

    for (int i = 0; i < MAX_NPROCESS; i++) {
        if (proc_set[i].pid == receiver) {
            /* Find the receiver */
            if (proc_set[i].status != PROC_WAIT_TO_RECV) {
                curr_status = PROC_WAIT_TO_SEND;
                proc_set[proc_curr_idx].receiver_pid = receiver;
            } else {
                /* Copy message from sender to kernel stack */
                struct sys_msg tmp;
                earth->mmu_switch(curr_pid);
                memcpy(&tmp, &sc->msg, sizeof(tmp));

                /* Copy message from kernel stack to receiver */
                earth->mmu_switch(receiver);
                memcpy(&sc->msg, &tmp, sizeof(tmp));

                /* Set receiver process as runnable */
                proc_set_runnable(receiver);
            }
            proc_yield();
            return;
        }
    }

    sc->retval = -1;
}

static void proc_recv(struct syscall *sc) {
    int sender = -1;
    for (int i = 0; i < MAX_NPROCESS; i++)
        if (proc_set[i].status == PROC_WAIT_TO_SEND &&
            proc_set[i].receiver_pid == curr_pid)
            sender = proc_set[i].pid;

    if (sender == -1) {
        curr_status = PROC_WAIT_TO_RECV;
    } else {
        /* Copy message from sender to kernel stack */
        struct sys_msg tmp;
        earth->mmu_switch(sender);
        memcpy(&tmp, &sc->msg, sizeof(tmp));

        /* Copy message from kernel stack to receiver */
        earth->mmu_switch(curr_pid);
        memcpy(&sc->msg, &tmp, sizeof(tmp));

        /* Set sender process as runnable */
        proc_set_runnable(sender);
    }

    proc_yield();
}

