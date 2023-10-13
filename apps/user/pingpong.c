#include "app.h"
#include <string.h>

int main(int argc, char** argv) {
    char *msg;
    int listening = 0;
    int pid = service_discover("pingpong");
    if (pid == -1) {
        int ret = service_register("pingpong");
        ASSERT(ret == 0, "service registration failed");
        listening = 1;
        msg = "ping";
    } else {
        listening = 0;
        msg = "pong";
    }

    for (int i=0; i<10; i++) {
        if (listening) {
            char buf[128];
            int sender = 0;  // receive msg from any pid
            grass->sys_recv(&sender, buf, sizeof(buf));
            printf("%s from pid=%d\n", buf, sender);
            listening = 0;
            pid = sender; // the other process will be listening
        } else {
            grass->sys_send(pid, msg, 5);
            listening = 1;
        }
    }

    if (strcmp(msg, "ping") == 0) { // proc who registered the service
        printf("Destroying the service pingpong\n");
        int ret = service_destroy("pingpong");
        ASSERT(ret == 0, "destroy service failed");
    } else {
        printf("Try to destroy the service pingpong (should fail)\n");
        int ret = service_destroy("pingpong");
        ASSERT(ret == -1, "destroy a service that is not registered by me");
    }
}
