#include <crow/gates/udpgate.h>
#include <crow/pubsub/pubsub.h>
#include <crow/tower.h>
#include <crow/address.h>
#include <crow/select.h>
//#include <crow/brocker/service.h>
#include <crow/nodes/cli.h>
#include <crow/netkeep.h>

#include <getopt.h>
#include <stdbool.h>

#include <stdio.h>

#include <map>
#include <string>
#include <thread>

#include <crow/brocker/crowker.h>
#include "control_node.h"

#include <nos/print.h>
#include <nos/fprint.h>
#include <igris/util/dstring.h>

#include <nos/inet/tcp_server.h>
#include <nos/inet/tcp_socket.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

bool brocker_info = false;
int udpport = -1;
int tcpport = -1;
bool quite = false;

void tcp_client_listener(nos::inet::tcp_socket client)
{
	char buf[1024];
	nos::inet::netaddr addr = client.getaddr();

	if (brocker_info)
		nos::println("new tcp client from", addr);

	while (1)
	{
		int ret;
		char cmd;
		uint8_t datasize;
		uint16_t thmsize;

		std::string theme;
		std::string data;

		ret = client.recv(buf, 3, MSG_WAITALL);

		if (ret <= 0)
			break;

		cmd = buf[0];
		if (cmd != 's' && cmd != 'p')
			goto clean;

		buf[3] = 0;
		thmsize = atoi32(buf + 1, 10, nullptr);

		if (thmsize == 0)
			goto clean;

		ret = client.recv(buf, thmsize, MSG_WAITALL);

		if (ret <= 0)
			break;

		theme = std::string(buf, thmsize);

		if ( cmd == 's' )
		{
			crow::crowker::instance()->tcp_subscribe(theme, &client);
			continue;
		}
		else if (cmd == 'p')
		{
			ret = client.recv(buf, 6, MSG_WAITALL);
			if (ret <= 0)
				break;

			buf[6] = 0;
			datasize = atoi32(buf, 10, nullptr);
			if (datasize == 0) goto clean;

			ret = client.recv(buf, datasize, MSG_WAITALL);
			if (ret <= 0)
				break;

			data = std::string(buf, datasize);

			crow::crowker::instance()->publish(theme, data);
			continue;
		}

clean:
		{
			if (brocker_info)
				nos::println("unresolved tcp command from", addr, cmd);

			continue;
		}
	}

	if (brocker_info)
		nos::println("tcp connection was clossed", addr);
}

void tcp_listener(int port)
{
	nos::inet::tcp_server srv;
	srv.init();
	srv.reusing(true);
	srv.bind("0.0.0.0", port);
	srv.listen(10);

	while (1)
	{
		auto client = srv.accept();

		std::thread thr(tcp_client_listener, std::move(client));
		thr.detach();
	}
}

void print_help()
{
	printf(
	    "Usage: crowker [OPTION]...\n"
	    "\n"
		"Common option list:\n"
		"  -h, --help            print this page\n"
		"\n"
		"Gate`s option list:\n"
		"  -u, --udp             set udp address (gate 12)\n"
		"  -S, --serial          make gate on serial device\n"
		"\n"
	);
}

int main(int argc, char *argv[])
{
	//crow::pubsub_protocol.incoming_handler = incoming_pubsub_packet;
	//crow::undelivered_handler = undelivered_handler;
	crow::pubsub_protocol.enable_crowker_subsystem();

	const struct option long_options[] =
	{
		{"help", no_argument, NULL, 'h'},
		{"udp", required_argument, NULL, 'u'}, // crow udpgate port
		{"tcp", required_argument, NULL, 't'},
		{"debug", no_argument, NULL, 'd'}, // crow transport log
		{"binfo", no_argument, NULL, 'b'}, // browker log
		{"vdebug", no_argument, NULL, 'v'}, // vital packet log
		{"netname", required_argument, NULL, 'n'},
		{NULL, 0, NULL, 0}
	};

	int long_index = 0;
	int opt = 0;

	while ((opt = getopt_long(argc, argv, "usvdibtn", long_options,
	                          &long_index)) != -1)
	{
		switch (opt)
		{
			case 'h':
				print_help();
				exit(0);

			case 'u':
				udpport = atoi(optarg);
				break;

			case 't':
				tcpport = atoi(optarg);
				break;

			case 'd':
				crow::enable_diagnostic();
				break;

			case 'v':
				crow::enable_live_diagnostic();
				break;

			case 'b':
				brocker_info = true;
				crow::crowker::instance()->brocker_info = true;
				break;

			case 0:
				break;
		}
	}

	if (udpport == -1)
	{
		printf("You should specify udp port.\n");
		exit(-1);
	}

	if (crow::create_udpgate(CROW_UDPGATE_NO, udpport))
	{
		perror("udpgate open");
		exit(-1);
	}


	if (tcpport != -1)
	{
		std::thread thr(tcp_listener, tcpport);
		thr.detach();
	}

	//control_node.bind(100);
	init_control_node();

	//auto * service_control_node = crow::crowker_service_control_node();
	//service_control_node->bind(CROWKER_CONTROL_SERVICE_NID);

	crow::spin_with_select();
}
