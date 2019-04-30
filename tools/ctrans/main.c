#include <crow/tower.h>
#include <crow/gates/udpgate.h>
#include <crow/gates/serial_gstuff.h>
#include <igris/util/hexer.h>
#include <igris/util/dstring.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>

#include <readline/readline.h>
#include <readline/history.h>

uint8_t addr[128];
int addrsize;

uint8_t qos = 0;
bool raw;
bool packmon;
bool sniffer;
bool echo;
bool info;
bool debug;

void incoming_handler(crowket_t* pack) {
	dpr("incoming: "); 
	
	if (echo) {
		crow_send(crowket_addrptr(pack), pack->header.alen, crowket_dataptr(pack), crowket_datasize(pack), 0, 0, 300);
	}
	
	if (packmon) {	
		crow_println(pack); 
	} else {
		char buf[10000];
		bytes_to_dstring(buf, crowket_dataptr(pack), crowket_datasize(pack));		
		printf("%s\n", buf);
	}
	
	crow_release(pack);
}

void traveling_handler(crowket_t* pack) {}

void transit_handler(crowket_t* pack) {
	if (sniffer) {
		dpr("transit: ");
		crow_println(pack); 
	}
}

void* console_listener(void* arg) {
	(void) arg;

  	char* input;
  	int len;

	rl_catch_signals = 0;
  	while(1) {	
 	   input = readline("");
    	
    	if (!input)
    	  break;
		add_history(input);
	
		len = strlen(input);
		input[len] = '\n';
		input[len + 1] = '\0';
		crow_send(addr, addrsize, input, len+1, 0, qos, 200);
	}

	exit(0);
}

uint16_t udpport = 0;
char* serial_port = NULL;
//std::string serial_port;
//int serialfd;

//void signal_handler(int sig) {
//	printf("dadas\n");
//	exit(sig);
//}

int main(int argc, char* argv[]) {
	pthread_t console_thread;

//	signal(SIGINT, signal_handler);

	const struct option long_options[] = {
		{"udp", required_argument, NULL, 'u'}, //udp порт для 12-ого гейта.
		{"qos", required_argument, NULL, 'q'}, //qos отправляемых сообщений. 0 по умолчанию
		{"serial", required_argument, NULL, 'S'}, //serial...
		{"sniffer", no_argument, NULL, 's'}, //Вывод проходных пакетов.
		{"pack", no_argument, NULL, 'p'}, //Подробная информация о входящих пакетах.
		{"echo", no_argument, NULL, 'e'}, //Активирует функцию эха входящих пакетов.
		{"info", no_argument, NULL, 'i'}, //Активирует информацию о вратах.
		{"debug", no_argument, NULL, 'd'}, //Активирует информацию о вратах.
		{NULL,0,NULL,0}
	};

    int long_index =0;
	int opt= 0;
	while ((opt = getopt_long(argc, argv, "uvSsedqip", long_options, &long_index)) != -1) {
		switch (opt) {
			case 'q': qos = atoi(optarg); break;
			case 'u': udpport = atoi(optarg); break;
			case 'S':
				serial_port = (char*) malloc(strlen(optarg) + 1); 
				strcpy(serial_port, optarg); 
				break;
			case 's': sniffer = true; break;
			case 'p': packmon = true; break;
			case 'e': echo = true; break;
			case 'i': info = true; break;
			case 'd': debug = true; break;
			case 0: break;
		}
	}

	if (crow_create_udpgate(udpport, G1_UDPGATE) == NULL) {
		perror("udpgate open");
		exit(-1);
	}
		
	if (debug) crow_enable_diagnostic();
	crow_user_incoming_handler = incoming_handler;
	crow_transit_handler = transit_handler;
	
	/*if (!serial_port.empty()) {
		if (crow_create_serialgate(serial_port.c_str(), 115200, 42) == NULL) {
			perror("serialgate open");
			exit(-1);
		}				
		//auto ser = new serial::Serial(serial_port, 115200);
		//auto* serial = new igris::io::file(ser->fd());
		//auto* serialgate = new crow::serial_gstuff_gate(ser);
		//crow_link_gate(serialgate, 42);
	}*/

	if (serial_port != NULL) {
		if (crow_create_serial_gstuff(serial_port, 115200, 42) == NULL) {
			perror("serialgate open");
			exit(-1);
		}				
	}

	if (optind < argc) {
		addrsize = hexer(addr, 128, argv[optind], strlen(argv[optind]));
		if (addrsize < 0) {
			dprln("Wrong address format");
			exit(-1);
		}
	} else {
		addrsize = 0;
	}	

	if (info) 
	{
		printf("gates:\n");
		printf("\tgate %d: udp port %d\n", G1_UDPGATE, udpport);
		if (serial_port != 0) 
			printf("\tgate %d: serial port %s\n", 42, serial_port);
		//printf("modes:\n");

	}

	if (pthread_create(&console_thread, NULL, console_listener, NULL)) {
		fprintf(stderr, "Error creating thread\n");
		return 1;
	}

	crow_spin();
}

