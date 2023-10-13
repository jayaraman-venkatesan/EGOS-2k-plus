/*
 * Description: the service register/discover server
 */

#include "app.h"
#include <string.h>

#define NUM_SERVICES 16
typedef struct {
    /* TODO: */

} serv_info;

static serv_info services[NUM_SERVICES];


int main() {
    SUCCESS("Enter kernel process GPID_SERV");

    /* Send a notification to GPID_PROCESS */
    char buf[SYSCALL_MSG_LEN];
    strcpy(buf, "Finish GPID_SERV initialization");
    grass->sys_send(GPID_PROCESS, buf, 32);

    /* Wait for directory requests */
    while (1) {
        int sender;
        struct serv_request *req = (void*)buf;
        struct serv_reply *reply = (void*)buf;
        sender = 0;
        grass->sys_recv(&sender, buf, SYSCALL_MSG_LEN);

        char *serv_name = req->name;
        int pid = sender;

        switch (req->type) {
        case SERV_REGISTER: {
            /* TODO:
             * implement service register
             */
        }
        case SERV_DISCOVER: {
            /* TODO:
             * implement service discover
             */
        }
        case SERV_DESTROY: {
            /* TODO:
             * implement service destory
             */
        }
        default:
            FATAL("sys_serv: request%d not implemented", req->type);
        }
    }
}
