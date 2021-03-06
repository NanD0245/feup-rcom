#include "setup.h"

int openSerialPort (int argc, char** argv, struct termios * oldtio) {
    if (argc < 2 || argc > 2) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }
    
    struct termios newtio;
    
    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd, oldtio) == -1) {
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   
    newtio.c_cc[VMIN]     = 1; 

    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

    return 0;
}

int closeSerialPort (struct termios * oldtio) {
    sleep(1);
    if (tcsetattr(fd, TCSANOW, oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    close(fd);
}


int connectToReader() {
    int done = 1, interrupt_count = 0;

    while (interrupt_count < MAX_ATTEMPTS && done != 0) {
		send_set();
		printf("Attempt - %d\n",interrupt_count);
		alarm(TIMEOUT);
		if ((done = read_ua()) != 0) interrupt_count++; else break;
	}
    
    if (interrupt_count == MAX_ATTEMPTS) { 
		puts("INTERRUPTED - REACHED MAX TRIES");
		return 1;
	}

    return 0;
}

int connectToWriter() {
    read_set();
    send_SU(UA);
}