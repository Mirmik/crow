#include <crow/tower.h>
#include <crow/hexer.h>
#include <crow/gates/udpgate.h>
#include <crow/gates/serial_gstuff.h>

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
uint8_t type = 0;
uint8_t ackquant = 200;

bool api = false;
bool noconsole = false;
bool noend = false;
bool echo = false;

bool debug = false;
bool vdebug = false;
bool gdebug = false;

bool pulse_mode = false;
char* pulse_buffer = NULL;

void incoming_handler(struct crow_packet *pack)
{
	if (echo)
	{
		crow_send(crowket_addrptr(pack), pack->header.alen, crowket_dataptr(pack),
				   crowket_datasize(pack), pack->header.f.type, pack->header.qos,
				   pack->header.ackquant);
	}

	if (api)
	{
		char *dp = pack->dataptr();
		size_t ds = pack->datasize();

		if (strncmp(dp, "exit\n", ds) == 0)
		{
			raise(SIGINT);
		}
	}

	char buf[10000];
	bytes_to_dstring(buf, pack->dataptr(), pack->datasize());
	printf("%s\n", buf);
	fflush(stdout);

	crow_release(pack);
}

void transit_handler(crowket_t* pack)
{
	if (sniffer)
	{
		dpr("transit: ");
		crow_println(pack);
	}
}

void* console_listener(void* arg)
{
	(void)arg;

	char *input;
	size_t len;

	rl_catch_signals = 0;

	while (1)
	{
		input = readline("");

		if (!input)
			break;

		add_history(input);

		len = strlen(input);

		if (!noend)
		{
			input[len] = '\n';
			len++;
		}

		crow_send(addr, (uint8_t)addrsize, input, (uint16_t)len, type, qos,
		          ackquant);
	}

	exit(0);
}

uint16_t udpport = 0;
char* serial_port = NULL;

int main(int argc, char* argv[])
{
	pthread_t console_thread;

	const struct option long_options[] =
	{
#ifdef GATES_UDPGATE
		{"udp", required_argument, NULL, 'u'}, // udp порт для 12-ого гейта.
#endif

#ifdef GATES_SERIAL_GSTUFF_GATE
		{"serial", required_argument, NULL, 'S'}, // serial...
#endif

		{"qos", required_argument, NULL, 'q'}, // qos отправляемых сообщений. 0 по умолчанию
		{"type", required_argument, NULL, 't'}, // qos отправляемых сообщений. 0 по умолчанию

		{"echo", no_argument, NULL, 'E'}, //Активирует функцию эха входящих пакетов.
		{"noend", no_argument, NULL, 'e'}, //Активирует функцию эха входящих пакетов.

		{"info", no_argument, NULL, 'i'}, //Активирует информацию о вратах.
		{"api", no_argument, NULL, 'a'}, //Активирует информацию о вратах.

#ifdef CONSOLE_INTERFACE
		{"noconsole", no_argument, NULL, 'n'}, //Активирует информацию о вратах.
#endif

		{"pulse", required_argument, NULL, 'p'}, //Выйти из программы сразу после подтверждения получения пакета (или после отсылки для QoS 0).

		{"debug", no_argument, NULL, 'd'},  //Включить диагностику башни.
		{"vdebug", no_argument, NULL, 'v'}, //Включить диагностику времени жизни пакетов.
		{"gdebug", no_argument, NULL, 'g'}, //Включить диагностику врат.
		{NULL, 0, NULL, 0}
	};

	int long_index = 0;
	int opt = 0;

	while ((opt = getopt_long(argc, argv, "uqSEeidvgt", long_options,
	                          &long_index)) != -1)
	{
		switch (opt)
		{
			case 'q':
				qos = (uint8_t)atoi(optarg);
				break;

			case 't':
				type = (uint8_t)atoi(optarg);
				break;

			case 'u':
				udpport = (uint16_t)atoi(optarg);
				break;

			case 'S':
				serial_port = (char *)malloc(strlen(optarg) + 1);
				strcpy(serial_port, optarg);
				break;

			case 'E':
				echo = true;
				break;

			case 'e':
				noend = true;
				break;

			case 'i':
				info = true;
				break;

			case 'n':
				noconsole = true;
				break;

			case 'g':
				gdebug = true;
				break;

			case 'p':
				pulse_mode = true;
				pulse_buffer = optarg;
				break;

			case 'a':
				api = true;
				break;

			case 'd':
				crow_enable_diagnostic();
				break;

			case 'v':
				crow_enable_live_diagnostic();
				break;

			case 0:
				break;
		}
	}

	if (debug)
		crow_enable_diagnostic();

#ifdef GATES_UDPGATE

	if (crow_create_udpgate(udpport, G1_UDPGATE) == NULL)
	{
		perror("udpgate open");
		exit(-1);
	}

#endif

#ifdef GATES_SERIAL_GSTUFF_GATE

	if (serial_port != NULL)
	{
		if (crow_create_serial_gstuff(serial_port, 115200, 42, gdebug) == NULL)
		{
			perror("serialgate open");
			exit(-1);
		}
	}

#endif

	if (gdebug)
	{
		printf("gates:\n");

#ifdef GATES_UDPGATE
		printf("\tgate %d: udp port %d\n", G1_UDPGATE, udpport);

#endif

#ifdef GATES_SERIAL_GSTUFF_GATE

		if (serial_port != 0)
			printf("\tgate %d: serial port %s\n", 42, serial_port);

#endif
	}

	crow_user_incoming_handler = incoming_handler;
	crow_transit_handler = transit_handler;

	if (optind < argc)
	{
		addrsize = hexer(addr, 128, argv[optind], strlen(argv[optind]));

		if (addrsize < 0)
		{
			dprln("Wrong address format");
			exit(-1);
		}
	}
	else
	{
		addrsize = 0;
	}

	if (info)
	{
		printf("gates:\n");
		printf("\tgate %d: udp port %d\n", G1_UDPGATE, udpport);

		if (serial_port != 0)
			printf("\tgate %d: serial port %s\n", 42, serial_port);
	}

	if (pulse_mode)
	{
		//if (!noend)
		//	pulse = pulse + '\n';
		//crow::send(addr, (uint8_t)addrsize, pulse.data(),
		//		   (uint16_t)pulse.size(), type, qos, ackquant);

		dprln("pulse_mode::TODO");

		crow_onestep();
		exit(0);
	}

#ifdef CONSOLE_INTERFACE

	if (pthread_create(&console_thread, NULL, console_listener, NULL))
	{
		fprintf(stderr, "Error creating thread\n");
		return 1;
	}

#endif

	crow_spin();
}

