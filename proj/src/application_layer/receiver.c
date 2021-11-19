#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>

#include "../data_link_layer/include/link_layer.h"
#include "../data_link_layer/include/errnos.h"
#include "include/packet.h"


int main(int argc, char **argv) {
    if (argc < 2 || argc > 2) {
        fprintf(stderr, RED"Usage:\treceiver SerialPort\n\tex: receiver /dev/ttyS1\n"RESET);
        exit(1);
    }

    printf("[receiver]: started: using serial port: %s\n"RESET, argv[1]);

    int fd = ll_open(argv[1], false);
    if (fd < 0) {
        exit(-1);
    }


    ssize_t r;
    control_packet_t start_packet, end_packet;
    memset(&start_packet, 0, sizeof(start_packet));
    memset(&end_packet, 0, sizeof(end_packet));

    unsigned char packet[PACKET_SIZE];
    r = ll_read(fd, packet, sizeof(packet));


    if (r == EOF_DISCONNECT) {
    } else if (r < 0) {
    } else if (r >= 0) {
        if (process_control_packet(packet, r, &start_packet, true) < 0) {
        }
    } else {
        // Does it make sense for any packet to arrive before this one?
    }

    int fd2 = open(start_packet.file_name, O_WRONLY | O_CREAT, 0644);
    if (fd2 < 0) {}

    size_t no_packets = start_packet.file_size / PACKET_SIZE + 1;

    size_t i = 0;
    size_t n;

    unsigned char buf[BUFFER_SIZE];
    while (true) {
        r = ll_read(fd, packet, sizeof(packet));
        if (r == EOF_DISCONNECT) {
            break;
        } else if (r < 0) {
            // Raise an error
        } else if (r >= 0) {
            if (packet[0] == C_DATA) {
                n = process_data_packet(packet, r, buf, i % 256, sizeof(buf));
                if (n < 0) {
                    continue;
                } else {
                    printf("WRITING: %s\n", buf);
                    if (write(fd2, buf, n) < 0) {}
                }

            } else {
                if (process_control_packet(packet, r, &end_packet, false) < 0) {
                    // Raise an error
                } else {
                    break;
                }
                // From this point on, does it make sense for other packets to arrive?
            }
            ++i;
        }
    }

    if (start_packet.file_size != end_packet.file_size || strcmp(start_packet.file_name, end_packet.file_name) != 0) {
    }

    free(start_packet.file_name);
    free(end_packet.file_name);


    if (ll_close(fd, false) < 0) exit(-1);
    return SUCCESS;
}
