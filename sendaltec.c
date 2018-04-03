#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>



#define USAGESTRING "sendaltec [opts] <device> <ADR> <param> [value]\n\r -b<baudrate> = Baudrate ex :-b4800 (9600 is default)\n\r -t<delay> = Response timeout in usec ex: -t100000\n\r -n = Numerical output only\n\r -i = Ignore checksums\n\n\r"



#define DEFAULT9600TO  30000

#define BUFSIZE         5000
#define ALTEC_STX       0x02
#define ALTEC_ETX       0x03
#define ALTEC_EOT       0x04
#define ALTEC_ENQ       0x05
#define ALTEC_ACK       0x06
#define ALTEC_NAK 0x15



//------------------------------------------------------
int set_interface_attribs (int fd, int speed, int parity);
void set_blocking (int fd, int should_block);



int main(int argc, char *argv[])
{
    uint8_t firstarg;
    uint8_t csum;
    uint8_t altecadr, adrh, adrl;
    int i, j, n;
    int cspos;
    int sendtoreceivetimeout = DEFAULT9600TO;
    uint8_t numericaloutput = 0;
    uint8_t ignorechecksum = 0;
	int value = 0;
	int baudrate = B9600;
	char buf1[BUFSIZE];
	char buf2[BUFSIZE];

	altecadr = 0;
    firstarg = 1;
    
    if((argc < 4) || ((argv[1][0] == '-')&&(argc < 5)))
		printf(USAGESTRING);
   
    else {
	while((argv[firstarg][0] == '-')&&(argc > (firstarg + 2))) 
	        	firstarg++;
        if(argc <= (firstarg + 2)) {
		printf(USAGESTRING);
		exit(0);
	}
        
	//Options
	for(i=1;i<firstarg; i++)
		switch(argv[i][1]) {
		case 'b' : 	value = atoi(&argv[i][2]);
				switch(value) {
				case 2400 : baudrate = B2400; sendtoreceivetimeout = DEFAULT9600TO * 4; break;
				case 4800 : baudrate = B4800; sendtoreceivetimeout = DEFAULT9600TO * 2; break;
				case 9600 : baudrate = B9600; sendtoreceivetimeout = DEFAULT9600TO; break;
				case 19200 : baudrate = B19200; sendtoreceivetimeout = DEFAULT9600TO / 2; break;
				}
				break;
		case 't' : 	sendtoreceivetimeout = atoi(&argv[i][2]);
				break;
		case 'n' :	numericaloutput = 1;
				break;
		case 'i' :	ignorechecksum = 1;
				break;
		}
	
	int fd = open (argv[firstarg], O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0) {
		printf("error %d opening %s: %s\n\r", errno, argv[firstarg], strerror (errno));
		return 0;
	}
		
	set_interface_attribs(fd, baudrate, PARENB);  // set speed to 9600 bps, 7e1 even parity
	set_blocking(fd, 0);		//non blocking

	//Flush serial buffer
	n = read(fd, buf1, BUFSIZE);

		
        altecadr = 0x00FF & atoi(argv[firstarg + 1]);
        adrh = altecadr / 10;
        adrl = altecadr % 10;

	if(!altecadr) {
		printf("error : invalid Altec device address (ADR) : 0 \n\r");
                return 0;
	}
        
        i = 0;
        buf1[i++] = ALTEC_EOT;
        buf1[i++] = '0' + adrh;
        buf1[i++] = '0' + adrh;
        buf1[i++] = '0' + adrl;
        buf1[i++] = '0' + adrl;
        
	if(argc > (firstarg + 3)) {

		buf1[i++] = ALTEC_STX;

		//param
		buf1[i++] = argv[firstarg + 2][0];
		buf1[i++] = argv[firstarg + 2][1];

		//Copy value
		j = 0;
		while((i<BUFSIZE-3)&&(argv[firstarg + 3][j]))
			buf1[i++] = argv[firstarg + 3][j++];

		buf1[i++] = ALTEC_ETX;

		//Calculate checksum
		csum = 0;
		for(j=6;j<i;j++)
			csum ^= buf1[j];
	
		buf1[i++] = csum;
	}
	else {
		//Read param
		buf1[i++] = argv[firstarg + 2][0];
		buf1[i++] = argv[firstarg + 2][1];

		buf1[i++] = ALTEC_ENQ;
	}
                 
	buf1[i] = 0;

//	for(j=0;j<i;j++)
//              printf("%02X '%c'\n", buf1[j], buf1[j]);

	write(fd, buf1, strlen(buf1));
	usleep(sendtoreceivetimeout);
		
        n = read(fd, buf1, BUFSIZE);

//	for(i=0;i<n;i++)
//		printf("%02X '%c'\n", buf1[i], buf1[i]);
        
        if(!n) {
            printf("<No response>\n\r", errno, argv[1], strerror (errno));
			return 0;
        }

	//ACK or NAK
	if(n == 1) {
		if(buf1[0] == ALTEC_ACK) {
			if(numericaloutput)	printf("1");
			else			printf("<ACK>\n\r");
			return 1;
		}
		if(buf1[0] == ALTEC_NAK) {
			if(numericaloutput)	printf("0");
			else			printf("<NAK>\n\r");
			return 0;
		}
	}
        
        i = 0;
        j = -1;
        cspos = 0;
        
        //Search for start character
        while((i<n)&&(i<BUFSIZE)) {
            switch(buf1[i]) {
                case ALTEC_STX:     j = 0;
                                    csum = 0;
                                    break;
                case ALTEC_ETX:     if(((i+1)<n)&&(j >= 0))     cspos = j;
                                    csum ^= buf1[i];
                                    break;
                default :           if(j >= 0) {
                                        buf2[j++] = buf1[i];
                                        if(!cspos)              csum ^= buf1[i];
                                    }
                                    
            }
            i++;
        }
        
        if(((uint8_t)buf2[cspos] == csum)||(ignorechecksum)) {
	    buf2[cspos] = 0;
		
	    if(numericaloutput) {
		value = atoi(&buf2[2]);
		printf("%d", value);
	    }
	    else
	        printf("%s\n\r", buf2);
        }
        else
            printf("<Bad checksum>\n\r");
        
//        for(i=0;i<j;i++)
//            printf("%02X '%c'\n", buf2[i], buf2[i]);
        
//        printf("Received CS = %02X\n", buf2[cspos]);
//        printf("Calculated CS = %02X\n", csum);
		
		close(fd);
	}

	return 1;
}


int set_interface_attribs (int fd, int speed, int parity)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
//                printf("Erreur get attr...\n\r");
                return -1;
        }

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        //tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS7;     // 7-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
        {
//                printf("Erreur set attr...\n\r");
                return -1;
        }
        return 0;
}

void set_blocking (int fd, int should_block)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0) {
                printf("Error get attr...\n\r");
        }
	else {
	        tty.c_cc[VMIN]  = should_block ? 1 : 0;
        	tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout
	}
}


