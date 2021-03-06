#include "../../include/data_link_layer/link_layer.h"

#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "../../include/data_link_layer/connection.h"
#include "../../include/data_link_layer/port.h"
#include "../../include/data_link_layer/alarm.h"
#include "../../include/errors/error_nos.h"
#include "../../include/gui/gui.h"

extern bool n;

int ll_open(const char *path, bool is_emitter) {
    init_ll_logs();
    init_driver_logs();
    if (path == NULL) return NULL_POINTER_ERROR;

    const char *source, *destination;
    if (is_emitter) {
        destination = "receiver";
        source = "emitter";
    } else {
        source = "receiver";
        destination = "emitter";
    }

    int fd = open_serial_port(path);
    if (fd < 0) {
        LOG_LL_ERROR("[%s]: opening serial port: error: %s\n", source, strerror(errno))
        return fd;
    } else {
        LOG_LL_EVENT("[%s]: opening serial port: success\n", source)
    }

    LOG_LL_EVENT("[%s]: configuring alarm\n", source)
    setup_alarm();
    LOG_LL_EVENT("[%s]: configuring alarm: success\n", source)

    LOG_LL_EVENT("[%s]: connecting to %s\n", source, destination)
    int r = is_emitter ? connect_to_receiver(fd) : connect_to_emitter(fd);
    if (r < 0) {
        LOG_LL_ERROR("[%s]: connecting to %s: error\n", source, destination)
        //COMBACK: Is this LL?
        if (close_serial_port(fd) < 0) {
            LOG_LL_ERROR("[%s]: closing serial port: error: %s\n", source, strerror(errno))
        } else {
            LOG_LL_EVENT("[%s]: closing serial port: success\n", source)
        }
        return -1;
    } else {
        LOG_LL_EVENT("[%s]: connecting to %s: success\n", source, destination)
    }
    return fd;
}

int ll_close(int fd, bool is_emitter) {
    const char *source;
    int r;

    if (is_emitter) {
        source = "emitter";
        r = disconnect_from_receiver(fd);
    } else {
        source = "receiver";
        r = disconnect_from_emitter(fd);
    }
    if (r < 0) {
        LOG_LL_ERROR("[%s]: disconnecting: error: %s\n", source, strerror(errno))
    } else {
        LOG_LL_EVENT("[%s]: disconnecting: success\n", source)
    }

    r = close_serial_port(fd);
    if (r < 0) {
        LOG_LL_ERROR("[%s]: closing serial port: error: %s\n", source, strerror(errno))
        return r;
    } else {
        LOG_LL_EVENT("[%s]: closing serial port: success\n", source)
    }

    return SUCCESS;
}

ssize_t ll_read(int fd, void *data, size_t nb) {
    if (data == NULL) return NULL_POINTER_ERROR;
    bool out_of_order = true;
    ssize_t r;

    unsigned char bytes[LL_SIZE_MAX];

    while (out_of_order) {
        LOG_LL_EVENT("[receiver]: reading message (R = %d)\n", n)
        if ((r = read_frame(fd, bytes, sizeof(bytes))) < 0) { return r; }
        r = check_i_frame(bytes, r, data, nb);
        if (r == EOF_DISCONNECT) {
            LOG_LL_EVENT("[receiver]: received disconnect\n")
            return EOF_DISCONNECT;
        } else if (r == OUT_OF_ORDER) {
            n = !n;
        } else if (r == WRONG_HEADER) {
            continue;
        } else if (r < 0) {
            LOG_LL_ERROR("[receiver]: reading message: error %zd\n", r)
            ssize_t r2 = send_supervision_frame(fd, ADDRESS_RECEIVER_EMITTER, REJ(n)) < 0;
            if (r2) {
                LOG_LL_ERROR("[receiver]: sending confirmation: error %zd\n", r2)
            } else {
                LOG_LL_EVENT("[receiver]: sending reject: success\n")
                continue;
            }
            return r;
        } else {
            LOG_LL_EVENT("[receiver]: read message: %s\n", (char *) data)
            out_of_order = false;
        }

        if (send_supervision_frame(fd, ADDRESS_RECEIVER_EMITTER, RR(!n)) < 0) {
            LOG_LL_ERROR("[receiver]: sending confirmation: error\n")
        } else {
            LOG_LL_EVENT("[receiver]: sending confirmation: success\n")
            n = !n;
        }
    }
    return r;
}

ssize_t ll_write(int fd, const void *data, size_t nb) {
    if (data == NULL) return NULL_POINTER_ERROR;
    unsigned char bytes[LL_SIZE_MAX];

    int tries = 1;
    ssize_t s;
    while (tries <= MAX_ATTEMPTS) {
        for (int i = 0; i < NUMBER_OF_DUPLICATE_MESSAGES; ++i) {

            alarm(0);
            LOG_LL_EVENT("TRY: %d/%d\n", tries, MAX_ATTEMPTS);
            LOG_LL_EVENT("[emitter]: sending message (R = %d): message: %s\n", n, (char *) data);

            s = send_i_frame(fd, data, nb + 1, n);
            if (s < 0) {
                LOG_LL_ERROR("[emitter]: sending message: error %zd\n", s)
                continue;
            } else {
                LOG_LL_EVENT("[emitter]: sending message: success\n")
            }
        }

        ++tries;
        alarm(TIMEOUT);

        unsigned char a, c;
        ssize_t r;
        if (read_frame(fd, bytes, sizeof(bytes)) < 0) continue;
        if ((r = check_supervision_frame(bytes, &a, &c, sizeof(bytes))) < 0 || a != ADDRESS_RECEIVER_EMITTER) {
            LOG_LL_ERROR("[emitter]: reading confirmation: error %zd\n", r)
        } else {
            alarm(0);
            if (c == RR(!n)) {
                LOG_LL_EVENT("[emitter]: reading confirmation: success\n")
                n = !n;
                return s;
            } else if (c == REJ(n)) {
                LOG_LL_ERROR("[emitter]: reading confirmation: error: message rejected\n")
            } else {
                --tries;
                continue;
            }
        }
    }
    return TOO_MANY_ATTEMPTS;
}
