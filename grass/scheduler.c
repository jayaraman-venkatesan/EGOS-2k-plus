/*
 * (C) 2022, Cornell University
 * All rights reserved.
 */

/* Author: Yunhao Zhang
 * Description: scheduler
 *
 * updated by CS6640 23fall staff
 */

#include "egos.h"
#include "process.h"
#include <string.h>
#include "queue2.h"

/* processes that have pid < USER_PID_START is system processes */
#define USER_PID_START 5
#define USER_IDX_START pid2idx(USER_PID_START)

/* translating between pid and idx (in proc_set) */
#define pid2idx(pid)  ((pid>=1 && pid<=MAX_NPROCESS) ? (pid-1) \
                            : FATAL("pid2idx: invalid pid"))
#define idx2pid(idx)  ((idx>=0 && idx<MAX_NPROCESS) ? (idx+1) \
                            : FATAL("idx2pid: invalid idx"))


enum {
   HIGH_PRIORITY,
   MEDIUM_PRIORITY,
   LOW_PRIORITY,
};
/* [lab3-ex1]
 * TODO: plan how to use "schd_attr"
 *  - You need to design you own
 *  - Below are two examples.
 *  - With these macros, one can get the attribute by, for example,
 *       proc_set[pid2idx(pid)].arrive_time = earth->gettime();
 */
// #define arrive_time schd_attr.longlongs[0]
// #define schded_count schd_attr.ints[15]

#define arrive_time schd_attr.longlongs[0]

#define cpu_time schd_attr.longlongs[1]

#define yeilds schd_attr.longlongs[2]

#define response_time schd_attr.longlongs[3]

#define turn_around_time schd_attr.longlongs[4]

#define cpu_start_time schd_attr.longlongs[5]

#define sleep_start_time schd_attr.longlongs[6]

#define sleep_quantum schd_attr.longlongs[7]




unsigned long long ULLONG_MAX = 0xffffffffffffffff;

static int mlfq();


static queue_t high_priority_queue;

static queue_t low_priority_queue;

static queue_t medium_priority_queue;

static queue_t sleeping_queue;


/* [lab-ex1]
 * TODO: return the actual CPU time the pid used
 * - the returned value is a float number (how many QUANTUM)
 */
static float cpu_runtime(int pid) {
  return (float)proc_set[pid2idx(pid)].cpu_time / QUANTUM;
}



void update_cpu_time(int idx){

   

    unsigned long long cur_time = earth->gettime();

    if (cur_time >= proc_set[idx].cpu_start_time) {
        proc_set[idx].cpu_time += (cur_time - proc_set[idx].cpu_start_time) ;
    } else {
    
        // Time overflow occurred, handle it
        proc_set[idx].cpu_time += ULLONG_MAX - proc_set[idx].cpu_start_time +  cur_time + 1;
    }

}


void set_turnaround_time(int pid){
   int idx = pid2idx(pid);

   unsigned long long cur_time = earth->gettime();

    if (cur_time >= proc_set[idx].arrive_time) {
        // No time overflow occurred
        proc_set[idx].turn_around_time = (cur_time - proc_set[idx].arrive_time) ;
    } else {
        // Time overflow occurred, handle it
        proc_set[idx].turn_around_time = ULLONG_MAX - proc_set[idx].arrive_time +  cur_time + 1;
    }


}
void set_cpu_time(){

    proc_set[proc_curr_idx].cpu_time = earth->gettime();

}

void set_response_time(){

    unsigned long long cur_time = earth->gettime();

    if (cur_time >= proc_set[proc_curr_idx].arrive_time) {
        // No time overflow occurred
        proc_set[proc_curr_idx].response_time = (cur_time - proc_set[proc_curr_idx].arrive_time) ;
    } else {
        // Time overflow occurred, handle it
        proc_set[proc_curr_idx].response_time = ULLONG_MAX - proc_set[proc_curr_idx].arrive_time +  cur_time + 1;
    }

}


/* schedule next process */
void proc_yield() {
    /* [lab3-ex1]
     * TODO: update "schd_attr" in "proc_set" for the current process
     * hints:
     *   - the current process will likely to be scheduled "out"
     *   - you can use earth->gettime() to get the current time
     *     -- the returned time is in "long long" (i.e., "m_uint64")
     *   - you may want to calculate how long it runs here
     *     -- a challenge is that the returned time may overflow
     * */

    update_cpu_time(proc_curr_idx);
   

    /* Find the next runnable process */
    int next_idx = -1;

#ifndef MLFQ // if not defined MLFQ in Makefile
    /* default scheduler policy: find the next runnable process */
    for (int i = 1; i <= MAX_NPROCESS; i++) {
        int s = proc_set[(proc_curr_idx + i) % MAX_NPROCESS].status;
        if (s == PROC_READY || s == PROC_RUNNING || s == PROC_RUNNABLE) {
            next_idx = (proc_curr_idx + i) % MAX_NPROCESS;
            break;
        }
    }
#else
    int next_pid = mlfq();
    next_idx = (next_pid > 0) ? pid2idx(next_pid) : -1;
#endif

    if (next_idx == -1) FATAL("proc_yield: no runnable process");
    if (curr_status == PROC_RUNNING) proc_set_runnable(curr_pid);

    /* Switch to the next runnable process and reset timer */
    int test_idx = proc_curr_idx;
    proc_curr_idx = next_idx;
    ASSERT(curr_pid > 0 && curr_pid < pid2idx(MAX_NPROCESS), \
            "proc_yield: invalid pid to switch");
    earth->mmu_switch(curr_pid);
    earth->timer_reset();



    /* [lab3-ex1]
     * TODO: update "schd_attr" for the next process (new process)
     * */

    if(proc_set[proc_curr_idx].yeilds == 0){
        set_response_time(proc_curr_idx);
        //set_cpu_time(proc_curr_idx);
    }


     proc_set[proc_curr_idx].cpu_start_time = earth->gettime();
     
     proc_set[proc_curr_idx].yeilds += 1;


     




    /* Call the entry point for newly created process */
    if (curr_status == PROC_READY) {
        proc_set_running(curr_pid);

        /* [lab4-ex3]
         * TODO: the kernel will switch privilege level here:
         * - if the curr_pid is a system process, set the privilege level to S-Mode
         * - if the curr_pid is a user application, set the privilege level to U-Mode
         */

        /* TODO: your code here */

        int mstatus;
        asm("csrr %0, mstatus" : "=r"(mstatus));
        if(curr_pid >= 5){
            asm("csrw mstatus, %0" ::"r"((mstatus & ~(3 << 11))));   /* clear MPP */
                                        
        } else {
         
        /* Enter supervisor mode after mret */
        
        asm("csrw mstatus, %0" ::"r"((mstatus & ~(3 << 11))   /* clear MPP */
                                            | (1 << 11) )); /* set MPP to S */
        }

        /* Prepare argc and argv */
        asm("mv a0, %0" ::"r"(APPS_ARG));
        asm("mv a1, %0" ::"r"(APPS_ARG + 4));
        /* Enter application code entry using mret */
        asm("csrw mepc, %0" ::"r"(APPS_ENTRY));
        asm("mret");
    }

    proc_set_running(curr_pid);
}

/* [lab3-ex2]
 * initialize MLFQ data structures */
void mlfq_init() {
    /* TODO: your code here*/


    queue_init(&high_priority_queue);
    queue_init(&low_priority_queue);
    queue_init(&medium_priority_queue);
    queue_init(&sleeping_queue);

    //FATAL("mlfq_init not implemented");



}


int get_from_queue(queue_t q){
    int r_pid = -1;
    node_t *current = q.head;
    while (current != NULL) {
        struct process *p = (struct process *) current->item;
        if(p->status == PROC_RUNNABLE || p->status == PROC_READY){
            r_pid = p->pid;
            break;
        }
        current = current->next;
    }

   
    printf("returning [%d]\n",r_pid);
    return r_pid;
}


int find_in_queue(queue_t *q){


    int r_pid = -1;
    int first_id = -1;
    struct process *p = (struct process *) dequeue(q);
    //printf("IMP P IS  [%d]\n",p->pid);
    while( p  != NULL ){

        
        if(first_id != p->pid){
            if(p->status == PROC_READY || p->status == PROC_RUNNABLE){
                r_pid = p->pid;
                break;
            } else {
                //printf("ENQUEUED [%d]\n",p->pid);
                enqueue(q,p);
            }
        } else if(first_id == p->pid) {
            enqueue(q,p);
            break;
        }

        if(first_id == -1){
            first_id = p->pid;
        }  

        p = (struct process *) dequeue(q);

        

    }

    return r_pid;

}

int is_present(){
    if(sleeping_queue.tail == NULL && sleeping_queue.tail == NULL){
        return 0;
    } 

    return 1;
}

int update_sleeping(){

    int r_pid = -1;
    int first_id = -1;

    static int loop = 1;
    //printf("came for [%d]...\n",loop);
    loop += 1;

    struct process *p = (struct process *) dequeue(&sleeping_queue);
    while(p != NULL){
        

        if(first_id != p->pid){

            unsigned long long cur_time = earth->gettime();
            int wake_flag = 0;
        
            if(cur_time > p->sleep_start_time){
                unsigned long long sleeping_time = cur_time - p->sleep_start_time;
                if((float) sleeping_time /QUANTUM > p->sleep_quantum){
                    r_pid = p->pid;
                    break;
                } else {
                    enqueue(&sleeping_queue,p);
                } 
            }   

        } else if(first_id == p->pid){
            enqueue(&sleeping_queue,p);
            break;
        }

       

        if(first_id == -1){
            
            first_id = p->pid;
        }
        
         


        p = dequeue(&sleeping_queue);
    }

    return r_pid;

}


/* [lab3-ex2]
 * implement MLFQ scheduler
 *   - return the **pid** (NOT index) of the next runnable process.
 *   - if there is no other runnable processes and the current process is PROC_RUNNING,
 *     you should let the current process keep running.
 *   - if no process is runnable, return 0.
 *   - you should always put system processes (pid<USER_PID_START) in the highest priority queue
 *   - you should not enqueue PROC_UNUSED
 *
 * hints:
 *   - you need to check the status of the process before running it;
 *     a process might not be runnable when dequeue (for example, in a status of waiting)
 *   - some invariants you want to check (for robust MLFQ implementation):
 *     -- the current running process should not be in the queue
 *     -- all processes except the current running one are in some queue
 *     -- there are no duplicated pids between queues
 *   - use pid2idx() and idx2pid() to translate pid and index in "proc_set"
 * */
static int mlfq() {
    /* TODO: your code here */

    //printf("PROC YEILDING IS [%d]\n",curr_pid);
    //update_sleeping();


    int return_pid = -1;

    int flag = 0;

    return_pid = update_sleeping();
    if(return_pid != -1){
        flag = 1;
       
    }

    if(!flag){
        return_pid = find_in_queue(&high_priority_queue);
        if(return_pid != -1){
         flag = 1;
       
        }
    }

   
    

   

    if(!flag){
        return_pid = find_in_queue(&medium_priority_queue);
        if(return_pid != -1){
            flag = 1;
           
        }
    }

     if(!flag){
        return_pid = find_in_queue(&low_priority_queue);
        if(return_pid != -1){
            flag = 1;
        }
    }




    if(return_pid == -1){
        if(proc_set[proc_curr_idx].status == PROC_RUNNING){
            return curr_pid;
        } else {
            if(is_present()){
               while(return_pid == -1){
                 return_pid = update_sleeping();
               }
               return return_pid;
            }
            return 0;
        }
    } 

    if(return_pid != -1){

        if(proc_set[proc_curr_idx].status != PROC_UNUSED){
            if(proc_set[proc_curr_idx].pid < USER_PID_START || (cpu_runtime(proc_set[proc_curr_idx].pid) < 1)){
                //printf("ENQUEUEING [%d] IN HIGH\n",proc_set[proc_curr_idx].pid);
                enqueue(&high_priority_queue,&proc_set[proc_curr_idx]);
            } else if(cpu_runtime(proc_set[proc_curr_idx].pid) > 1 && cpu_runtime(proc_set[proc_curr_idx].pid) < 2){
                //printf("ENQUEUES [%d] in MED queue CPU TIME [%f]\n",proc_set[proc_curr_idx].pid,cpu_runtime(proc_set[proc_curr_idx].pid));
                enqueue(&medium_priority_queue,&proc_set[proc_curr_idx]);
            } else if(cpu_runtime(proc_set[proc_curr_idx].pid) > 2){
                //printf("ENQUEUES [%d] in LOW queue CPU TIME [%f]\n",proc_set[proc_curr_idx].pid,cpu_runtime(proc_set[proc_curr_idx].pid));
                enqueue(&low_priority_queue,&proc_set[proc_curr_idx]);
            }
        }
       
    }

    return return_pid;

    
    

   
}


/* process scheduling metrics */

/* [lab-ex1]
 * TODO: return turn-around time for pid
 * - the returned value is a float number (how many QUANTUM)
 */
static float tar_time(int pid) {
    return (float)proc_set[pid2idx(pid)].turn_around_time / QUANTUM;
}

/* [lab-ex1]
 * TODO: return response time for pid
 * - the returned value is a float number (how many QUANTUM)
 */
static float resp_time(int pid) {

    return (float)proc_set[pid2idx(pid)].response_time / QUANTUM;
}

/* [lab-ex1]
 * TODO: return how many times the process pid has been scheduled
 * - the returned value is an integer
 */
static int yield_num(int pid) {
  return proc_set[pid2idx(pid)].yeilds;
}



/* process life-cycle functions */

/* proc_on_arrive will be called when the process is created */
void proc_on_arrive(int pid) {
    /* [lab-ex1]
     * TODO: collect pid's scheduling information
     * hint: remember to init/clear the pid's "schd_attr"
     */


    proc_set[pid2idx(pid)].arrive_time = earth->gettime();
    proc_set[pid2idx(pid)].response_time = 0;
    proc_set[pid2idx(pid)].yeilds=0;
    proc_set[pid2idx(pid)].cpu_time=0;

#ifdef MLFQ
  static int first_time = 1;
  if (first_time) {
      mlfq_init();
      first_time = 0;
      /* return without adding proc to queue for the first time
       * because this process is sys_proc, which will directly
       * run without go via proc_yield().*/
      return;
  }

  /* [lab3-ex2]
   * TODO: add the process to MLFQ
   * Note that pid is not the curr_pid
   */
  
    enqueue(&high_priority_queue , &proc_set[pid2idx(pid)]);

#endif
}

/* proc_on_stop() will be called when the process exits */
void proc_on_stop(int pid) {
    ASSERTX(proc_set[pid2idx(pid)].status == PROC_UNUSED); // pid must be exited

    /* [lab3-ex1]
     * TODO: collect pid's scheduling information */


    update_cpu_time(pid2idx(pid));
    set_turnaround_time(pid);

    

    INFO("proc %d died after %d yields, turnaround time: %.2f, response time: %.2f, cputime: %.2f",
            pid, yield_num(pid),
            tar_time(pid), resp_time(pid),
            cpu_runtime(pid));

#ifdef MLFQ
    /* [lab3-ex2]
     * TODO: remove process from queues */

    
    
    if(in_queue(&high_priority_queue,&proc_set[pid2idx(pid)])){
        rm_item(&high_priority_queue,&proc_set[pid2idx(pid)]);
    } 

    else if(in_queue(&medium_priority_queue,&proc_set[pid2idx(pid)])){
        rm_item(&medium_priority_queue,&proc_set[pid2idx(pid)]);
    } 

    else if(in_queue(&low_priority_queue,&proc_set[pid2idx(pid)])){
        rm_item(&low_priority_queue,&proc_set[pid2idx(pid)]);
    } 




#endif
}

/* [lab3-ex3]
 * let pid sleep for the given time units (i.e.., time_units of QUANTUM):
 *   - set pid's status to PROC_SLEEPING
 *   - record some information in "schd_attr" to later decide when the time is up
 *   - later (whenever scheduling is happening), check if the process should be waked up
 *   - in your scheduler (mlfq), if no process can run but there exists sleeping processes,
 *     the scheduler should not return; it should busy loop until one of the
 *     sleeping processes times up.
 *
 * hints:
 *   - you will likely modify multiple other places in your MLFQ implementation
 *   - you probably want a new sleeping queue
 *     -- if so, remember to remove this process from its original queue
 * */
void proc_on_sleep(int pid, int time_units) {
    ASSERTX(proc_set[pid2idx(pid)].status != PROC_SLEEPING);



    proc_set[pid2idx(pid)].status = PROC_SLEEPING;

    proc_set[pid2idx(pid)].sleep_quantum = time_units;

    proc_set[pid2idx(pid)].sleep_start_time = earth->gettime();
    /* TODO: your code here */

    if(in_queue(&high_priority_queue , &proc_set[pid2idx(pid)]) == 1){    
        
        rm_item(&high_priority_queue,&proc_set[pid2idx(pid)]);

       
        enqueue(&sleeping_queue,&proc_set[pid2idx(pid)]);

    }


    else if(in_queue(&medium_priority_queue , &proc_set[pid2idx(pid)]) == 1){

       
        
        rm_item(&medium_priority_queue,&proc_set[pid2idx(pid)]);

       
        enqueue(&sleeping_queue,&proc_set[pid2idx(pid)]);

     

    }



    else if(in_queue(&low_priority_queue , &proc_set[pid2idx(pid)]) == 1){

      
        
        rm_item(&low_priority_queue,&proc_set[pid2idx(pid)]);

      
        enqueue(&sleeping_queue,&proc_set[pid2idx(pid)]);

      

    }





}