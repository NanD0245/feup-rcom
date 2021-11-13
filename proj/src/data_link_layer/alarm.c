#include "include/alarm.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

extern int interrupt;

int setupAlarm(void) {
    struct sigaction action;
    action.sa_handler = sigalrm_hadler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGALRM, &action, NULL);
}

void sigalrm_hadler(int _) {
    interrupt = 1;
}