#include "include/read.h"

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

int read_ua(void) {
    int res;
    unsigned char a, c, m;
    res = read(fd, &m, 1);
    if (res <= 0) return 1;
    if (m != F) puts("ERROR FLAG");

    alarm(0);

    res = read(fd, &a, 1);
    if (a != ARE) puts("ERROR A");

    res = read(fd, &c, 1);;
    if (c != UA) puts("ERROR C");

    res = read(fd, &m, 1);
    if (m != (unsigned char) (a ^ c)) puts("ERROR BCC");

    res = read(fd, &m, 1);
    if (m != F) puts("ERROR FLAG");

    return 0;
}

int read_set(void) {
    int res;
    unsigned char m, a, c;
    res = read(fd, &m, 1);

    if (m != F) puts("ERROR FLAG");
    res = read(fd, &a, 1);

    if (a != AER) puts("ERROR A");
    res = read(fd, &c, 1);

    if (c != SET) puts("ERROR C");
    res = read(fd, &m, 1);

    if (m != (char) (a ^ c)) puts("ERROR BCC");
    res = read(fd, &m, 1);

    if (m != F) puts("ERROR FLAG");

    printf("everything okeie\n");
}