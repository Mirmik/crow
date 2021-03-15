#include <crow/gates/serial_gstuff.h>
#include <crow/gates/serial_gstuff_v1.h>
#include <crow/gates/udpgate.h>

#include <crow/tower.h>
#include <crow/pubsub/pubsub.h>
#include <crow/proto/channel.h>
#include <crow/proto/acceptor.h>

#include <crow/address.h>
#include <crow/select.h>

#include <igris/util/string.h>
#include <igris/util/dstring.h>
#include <igris/util/bug.h>

#ifdef WITH_MSGTYPE
#include <igris/protocols/msgtype.h>
#endif

#include <nos/fprint.h>

#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <string>
#include <map>
#include <thread>

#include "binin.h"
#include "binout.h"

#include <iostream>

#include <unistd.h>

bool infinite = false;
bool debug_mode = false;

const uint8_t* addr = nullptr;
int addrsize;
crow::hostaddr address;

bool userqos = false;
uint8_t qos = 0;
uint8_t type = 0;
uint16_t ackquant = 200;

bool api = false;
bool noconsole = false;
bool nlout = false;
bool noend = false;
bool echo = false;
bool gdebug = false;
bool info = false;
bool subscribe_mode = false;

bool bin_output_mode = false;
bool bin_input_mode = false;

int acceptorno = -1;
int channelno = -1;
int nodeno = -1;
std::string nodename = "";

crow::channel channel;
crow::channel * reverse_channel;
crow::acceptor acceptor;

std::string binout_fmt;
std::string binin_fmt;

#ifdef WITH_MSGTYPE
std::string msgtype;
igris::msgtype_struct msgreader;
#endif

std::string pulse;
std::string theme;
std::string pipelinecmd;

int DATAOUTPUT_FILENO = STDOUT_FILENO;
int DATAINPUT_FILENO = STDIN_FILENO;

//int unselect_pipe_fd[2];

enum class protoopt_e
{
	PROTOOPT_BASIC,
	PROTOOPT_CHANNEL,
	PROTOOPT_NODE,
	PROTOOPT_REVERSE_CHANNEL,
	PROTOOPT_PUBLISH
};

enum class input_format
{
	INPUT_RAW,
	INPUT_RAW_ENDLINE,
	INPUT_BINARY,
#ifdef WITH_MSGTYPE
	INPUT_MSGTYPE
#endif
};

enum class output_format
{
	OUTPUT_RAW,
	OUTPUT_DSTRING,
	OUTPUT_BINARY,
#ifdef WITH_MSGTYPE
	OUTPUT_MSGTYPE
#endif
};

protoopt_e protoopt = protoopt_e::PROTOOPT_BASIC;
input_format informat = input_format::INPUT_RAW;
output_format outformat = output_format::OUTPUT_RAW;

std::string informat_tostr()
{
	switch (informat)
	{
		case input_format::INPUT_RAW: return "INPUT_RAW";

		case input_format::INPUT_RAW_ENDLINE: return "INPUT_RAW_ENDLINE";

		case input_format::INPUT_BINARY: return "INPUT_BINARY";

#ifdef WITH_MSGTYPE
		case input_format::INPUT_MSGTYPE: return "INPUT_MSGTYPE";
#endif

		default: 
			BUG();
	}

	return std::string();
}

std::string outformat_tostr()
{
	switch (outformat)
	{
		case output_format::OUTPUT_RAW: return "OUTPUT_RAW";

		case output_format::OUTPUT_DSTRING: return "OUTPUT_DSTRING";

		case output_format::OUTPUT_BINARY: return "OUTPUT_BINARY";

#ifdef WITH_MSGTYPE
		case output_format::OUTPUT_MSGTYPE: return "OUTPUT_MSGTYPE";
#endif

		default: 
			BUG();
	}

	return std::string();
}

void output_do(igris::buffer data, crow::packet_ptr pack)
{
	if (api)
	{
		if (data == "exit\n")
			exit(0);
	}

	switch (outformat)
	{
		case output_format::OUTPUT_RAW:
			write(DATAOUTPUT_FILENO, data.data(), data.size());
			break;

		case output_format::OUTPUT_DSTRING:
			// Вывод в stdout информацию пакета.
			char buf[10000];
			bytes_to_dstring(buf, data.data(), data.size());
			write(DATAOUTPUT_FILENO, buf, strlen(buf));
			break;

		case output_format::OUTPUT_BINARY:
			output_binary(data, pack);
			break;

#ifdef WITH_MSGTYPE

		case output_format::OUTPUT_MSGTYPE:
			{
				auto ret = msgreader.tostring(data);
				//for (unsigned int i = 0; i < ret.size(); ++i)
				//{
				//	nos::fprintln("{}: {}", ret[i].first, ret[i].second);
				//}
				nos::println(ret);
			}
			break;
#endif

		default:
			BUG();
	}

	if (nlout)
	{
		write(DATAOUTPUT_FILENO, "\n", 1);
	}
}

std::pair<std::string, bool> input_do(const std::string& data)
{
	std::string message;

	switch (informat)
	{
		case input_format::INPUT_RAW_ENDLINE:
			message = data;
			message += '\n';
			return std::make_pair(message, true);

		case input_format::INPUT_RAW:
			message = data;
			return std::make_pair(message, true);

		case input_format::INPUT_BINARY:
			message = input_binary(std::string(data.data(), data.size()));
			return std::make_pair(message, true);

#ifdef WITH_MSGTYPE

		case input_format::INPUT_MSGTYPE:
			message = msgreader.fromstring_as_string(data);
			return std::make_pair(message, message == "" ? false : true);
#endif

		default:
			BUG();
	}
}

void pipeline(const std::string& cmd)
{
	nos::println("pipeline", cmd);

	int ans;

	int child_pid;
	int pipe_to_child[2];
	int pipe_from_child[2];

	char *child_args [] = { (char*)cmd.c_str(), (char*)0 };

	ans = pipe(pipe_to_child);

	if (ans)
	{
		perror("pipe");
		exit(-1);
	}

	ans = pipe(pipe_from_child);

	if (ans)
	{
		perror("pipe");
		exit(-1);
	}

	if ((child_pid = fork()) == 0)
	{
		// child branch
		dprln("child:", getpid());

		dup2(pipe_from_child[1], STDOUT_FILENO);
		dup2(pipe_to_child[0], STDIN_FILENO);
		close(pipe_from_child[0]);
		close(pipe_to_child[1]);

		ans = execvp(cmd.c_str(), child_args);

		if (ans)
		{
			perror("execve");
			exit(-1);
		}
	}

	// parent branch
	dprln("parent:", getpid());

	DATAINPUT_FILENO = pipe_from_child[0];
	DATAOUTPUT_FILENO = pipe_to_child[1];
	close(pipe_from_child[1]);
	close(pipe_to_child[0]);
}

void send_do(const std::string message)
{
	switch (protoopt)
	{
		case protoopt_e::PROTOOPT_BASIC:
			crow::send(
				{addr, (uint8_t)addrsize},
				{message.data(), message.size()}, 
				type, qos, ackquant);
			break;

		case protoopt_e::PROTOOPT_PUBLISH:
			{
				/*crow::packet * pack = crow::make_publish_packet(
				                          addr, (uint8_t)addrsize,
				                          theme.c_str(),
				                          message.data(), message.size());

				pack->qos(qos);
				pack->ackquant(ackquant);
				if (infinite)
					pack->set_infinite_ack();

				crow::travel(pack);*/

				crow::publish(
					{addr, (uint8_t)addrsize}, 
					theme.c_str(),
			        message,
			        qos, ackquant);
			}

			break;

		case protoopt_e::PROTOOPT_CHANNEL:
			{
				int ret = channel.send(message.data(), message.size());

				if (ret == CROW_CHANNEL_ERR_NOCONNECT)
				{
					nos::println("Channel is not connected");
				}
			}
			break;

		case protoopt_e::PROTOOPT_NODE:
			{
				/*if (nodename != "")
					crow::node_send(1, nodename.c_str(), addr, (uint8_t)addrsize,
					                message.data(), message.size(),
					                qos, ackquant);*/
					crow::node_send(1, nodeno, 
									{addr, (uint8_t)addrsize},
					                {message.data(), message.size()},
					                qos, ackquant);
			}
			break;

		case protoopt_e::PROTOOPT_REVERSE_CHANNEL:
			{
				int ret = reverse_channel->send(message.data(), message.size());

				if (ret == CROW_CHANNEL_ERR_NOCONNECT)
				{
					nos::println("Channel is not connected");
				}
			}
			break;
	}
}

void incoming_handler(crow::packet_ptr pack)
{
	if (echo)
	{
		// Переотослать пакет точно повторяющий входящий.
		crow::send({pack->addrptr(), pack->header.alen}, 
				   {pack->dataptr(), pack->datasize()}, 
				   pack->header.f.type, 
				   pack->header.qos,
		           pack->header.ackquant);
	}

	if (api)
	{
		// Запуск встроенных функций.
		char *dp = pack->dataptr();
		size_t ds = pack->datasize();

		if (strncmp(dp, "exit\n", ds) == 0)
		{
			raise(SIGINT);
		}
	}

	switch (pack->header.f.type)
	{
		case CROW_PUBSUB_PROTOCOL:
			output_do(crow::pubsub::get_data(pack), pack);
			break;

		case CROW_NODE_PROTOCOL:
			if (((crow::node_subheader *) pack->dataptr())->rid == 1)
			{
				output_do(crow::node::message(pack), pack);
			}

			else
			{
				crow::node_protocol.incoming(pack);
			}
			return;

		default:
			output_do(pack->rawdata(), pack);
	}
}

void print_channel_message(crow::channel* ch, crow::packet_ptr pack)
{
	(void) ch;
	(void) pack;
	output_do(crow::channel::getdata(pack), pack);
}

crow::channel* acceptor_create_channel()
{
	crow::channel * ch = new crow::channel(print_channel_message);

	// TODO: Утечка памяти. Удалять буффер на закрытии канала.
	ch->set_addr_buffer((char*)malloc(128), 128);

	reverse_channel = ch;
	return ch;
}

void *console_listener(void *arg)
{
	(void)arg;

	std::string input;
	char readbuf[1024];

	while (1)
	{
		//if (std::cin.peek() == std::char_traits<char>::eof())
		//{
		//	exit(0);
		//}
		//std::getline(std::cin, input);
		int len = read(DATAINPUT_FILENO, readbuf, 1024);

		input = std::string(readbuf, len);
		auto msgpair = input_do(input);

		if (msgpair.second)
			send_do(msgpair.first);

		//send unselect signal
		//write(crow::unselect_pipe[1], "a", 1);
	}

	exit(0);
}

uint16_t udpport = 0;
char *serial_port = NULL;
char *serial_port_v1 = NULL;

void print_help() 
{
	printf(
		"Usage: ctrans [OPTION]... ADDRESS\n"
		"\n"
		"Common option list:\n"
		"  -h, --help            print this page\n"
		"\n"
		"Gate`s option list:\n"
		"  -u, --udp             set udp address (gate 12)\n"
		"  -S, --serial          make gate on serial device\n"
		"\n"
		"Package settings option list:\n"
		"  -q, --qos             set QOS policy mode\n"            
		"  -A, --ackquant        set time quant (for QOS:1 and QOS:2)\n"
		"  -t, --type            set package type (if protocol isn't choosen)\n"
		"\n"
		"Protocol option list:\n"
		"      --chlisten        (channel) enable acceptor mode\n"
		"      --channel         (channel) connect to channel on nid\n"
		"      --node            (node)    send message to specified node\n"
		"      --subscribe       (pubsub)  subscribe to crowker theme\n"
		"      --publish         (pubsub)  publish to crowker theme\n"		
		"\n"
		"Info option list:\n"
		"      --info\n"
		"      --debug\n"
		"      --debug-data-size\n"
		"      --vdebug\n"
		"      --gdebug\n"
		"\n"
		"Control option list:\n"
		"      --noconsole       disable console input\n"
		"      --pulse           oneshoot mode. leave after first message\n"
		"      --echo            echo input packages to sender\n"
		"      --api             enable incoming console (cmds: 'exit')"
		"\n"
		"Crow address reference:\n"
		"      man crow-protocol\n"
	);
}

int main(int argc, char *argv[])
{
	pthread_t console_thread;

	const struct option long_options[] =
	{
		{"help", no_argument, NULL, 'h'},

		{"udp", required_argument, NULL, 'u'}, // udp порт для 12-ого гейта.
		{"serial", required_argument, NULL, 'S'}, // serial...
		{"serial_v1", required_argument, NULL, 'C'}, // serial...

		{"qos", required_argument, NULL, 'q'}, // qos отправляемых сообщений. 0 по умолчанию
		{"type", required_argument, NULL, 't'}, // метка типа отправляемых сообщений
		{"ackquant", required_argument, NULL, 'A'}, // установка кванта ack
		{"infinite", no_argument, NULL, 'I'},

		{"noend", no_argument, NULL, 'x'}, // Блокирует добавление символа конца строки.
		{"nlout", no_argument, NULL, 'N'}, // Блокирует добавление символа конца строки.
		{"echo", no_argument, NULL, 'E'}, // Активирует функцию эха входящих пакетов.
		{"api", no_argument, NULL, 'a'}, // Активирует удалённое управление.
		{"noconsole", no_argument, NULL, 'n'}, // Отключает создание консоли.
		{"pulse", required_argument, NULL, 'p'}, // Отключает программу по первой транзакции.

		{"binin", required_argument, NULL, 'b'}, // Форматирует вход согласно бинарного шаблона.
		{"binout", required_argument, NULL, 'B'}, // Форматирует вывод согласно бинарного шаблона.
		{"rawout", no_argument, NULL, 'r'},
		{"dbgout", no_argument, NULL, 'j'},

		{"chlisten", required_argument, NULL, 'w'},
		{"channel", required_argument, NULL, 'c'},
		{"node", required_argument, NULL, 'M'},
		{"pipeline", required_argument, NULL, 'e'},

		{"subscribe", required_argument, NULL, 'l'},
		{"publish", required_argument, NULL, 'P'},
		{"msgtype", required_argument, NULL, 'm'},

		{"info", no_argument, NULL, 'i'}, // Выводит информацию о имеющихся гейтах и режимах.
		{"debug", no_argument, NULL, 'd'}, // Включает информацию о событиях башни.
		{"debug-data-size", required_argument, NULL, 's'}, // Включает информацию о событиях башни.
		{"vdebug", no_argument, NULL, 'v'}, // Активирует информацию о времени жизни пакетов.
		{"gdebug", no_argument, NULL, 'g'}, // Активирует информацию о вратах.
		{NULL, 0, NULL, 0}
	};

	int long_index = 0;
	int opt = 0;

	while ((opt = getopt_long(argc, argv, "", long_options,
	                          &long_index)) != -1)
	{
		switch (opt)
		{
			case 'h':
				print_help();
				exit(0);

			case 'q':
				qos = (uint8_t)atoi(optarg);
				userqos = true;
				break;

			case 'A':
				ackquant = (uint16_t)atoi(optarg);
				break;

			case 's':
				crow::debug_data_size = (uint16_t)atoi(optarg);
				break;

			case 't':
				type = (uint8_t)atoi(optarg);
				break;

			case 'I':
				infinite = true;
				break;

			case 'u':
				udpport = (uint16_t)atoi(optarg);
				break;

			case 'S':
				serial_port = (char *)malloc(strlen(optarg) + 1);
				strcpy(serial_port, optarg);
				break;

			case 'C':
				serial_port_v1 = (char *)malloc(strlen(optarg) + 1);
				strcpy(serial_port_v1, optarg);
				break;

			case 'E':
				echo = true;
				break;

			case 'x':
				noend = true;
				break;

			case 'N':
				nlout = true;
				break;

			case 'i':
				info = true;
				break;

			case 'r':
				outformat = output_format::OUTPUT_RAW;
				break;

			case 'j':
				outformat = output_format::OUTPUT_DSTRING;
				break;

			case 'n':
				noconsole = true;
				break;

			case 'g':
				gdebug = true;
				break;

			case 'p':
				pulse = optarg;
				break;

			case 'a':
				api = true;
				break;

			case 'd':
				debug_mode = true;
				crow::enable_diagnostic();
				break;

			case 'v':
				crow::enable_live_diagnostic();
				break;

			case 'b':
				binin_fmt = optarg;
				bin_input_mode = true;
				break;

			case 'B':
				//binary_mode_prepare(optarg);
				binout_fmt = optarg;
				bin_output_mode = true;
				break;

			case 'P':
				theme = optarg;
				protoopt = protoopt_e::PROTOOPT_PUBLISH;
				break;
#ifdef WITH_MSGTYPE

			case 'm':
				msgtype = optarg;
				outformat = output_format::OUTPUT_MSGTYPE;
				informat = input_format::INPUT_MSGTYPE;
				break;
#endif

			case 'l':
				theme = optarg;
				subscribe_mode = 1;
				break;

			case 'w':
				acceptorno = atoi(optarg);
				protoopt = protoopt_e::PROTOOPT_REVERSE_CHANNEL;
				break;

			case 'c':
				channelno = atoi(optarg);
				protoopt = protoopt_e::PROTOOPT_CHANNEL;
				break;

			case 'M':
				if (isalpha(*optarg))
					nodename = optarg;
				else
					nodeno = atoi(optarg);

				protoopt = protoopt_e::PROTOOPT_NODE;
				break;

			case 'e':
				pipelinecmd = optarg;
				break;

			case '?':
				exit(-1);
				break;

			case 0:
				BUG();
				break;
		}
	}

	if (crow::create_udpgate(CROW_UDPGATE_NO, udpport))
	{
		perror("udpgate open");
		exit(-1);
	}

	if (serial_port != NULL)
	{
		if (crow::create_serial_gstuff(serial_port, 115200, 42, gdebug) == NULL)
		{
			perror("serialgate open");
			exit(-1);
		}
	}
	
	if (serial_port_v1 != NULL)
	{
		if (crow::create_serial_gstuff_v1(serial_port_v1, 115200, 42, gdebug) == NULL)
		{
			perror("serialgate open");
			exit(-1);
		}
	}
// Переопределение стандартного обработчика (Для возможности перехвата и api)
	crow::user_incoming_handler = incoming_handler;

	if (bin_output_mode == true)
	{
		binout_mode_prepare(binout_fmt);
		outformat = output_format::OUTPUT_BINARY;
	}

	if (bin_input_mode == true)
	{
		binin_mode_prepare(binin_fmt);
		informat = input_format::INPUT_BINARY;
	}

	if (noend)
	{
		informat = input_format::INPUT_RAW;
	}

	// Определение целевого адреса
	if (optind < argc)
	{
		address = crow::address_warned(argv[optind]);
		if (address.size() == 0)
		{
			nos::println("address error");
			exit(0);
		}

		addr = address.data();
		addrsize = address.size();

		if (addrsize < 0)
		{
			printf("Wrong address format\n");
			exit(-1);
		}
	}
	else
	{
		addrsize = 0;
	}

// Вывод информации о созданных вратах.
	if (info)
	{
		printf("gates:\n");
		printf("\tgate %d: udp port %d\n", CROW_UDPGATE_NO, udpport);

		if (serial_port != NULL)
			nos::println("gate is here (TODO)");
			//printf("\tgate %d: serial port %s\n", 42, serial_port);

		nos::println("informat:", informat_tostr());
		nos::println("outformat:", outformat_tostr());
	}

#ifdef WITH_MSGTYPE

	if (outformat == output_format::OUTPUT_MSGTYPE)
	{
		msgreader = igris::msgtype_read_type(msgtype,
		                                     "/home/mirmik/project/crow/apps/ctrans/test.msg");
	}

#endif

	if (pipelinecmd != "")
	{
		pipeline(pipelinecmd);
	}

	//START CROW
	crow::unselect_init();
	crow::select_collect_fds();
	crow::start_spin_with_select();

	if (channelno >= 0)
	{
		if (userqos == false)
			qos = 2;

		channel.init(33, print_channel_message);
		channel.set_addr_buffer((char*)malloc(128), 128);

		int ret = channel.connect(addr, addrsize, channelno, qos, ackquant);

		if (ret)
		{
			switch (ret)
			{

				case CROW_ERRNO_UNREGISTRED_RID:
					nos::println("Unregistred remote rid");
					break;
				default:
					nos::println("Handshake failure");
					break;
			}

			crow::stop_spin(false);
			crow::join_spin();
			exit(0);
		}
	}

	if (acceptorno >= 0)
	{
		acceptor.init(acceptorno, acceptor_create_channel);
	}

	// Ветка обработки pulse мода.
	if (pulse != "")
	{
		auto msgpair = input_do(pulse);

		if (msgpair.second)
			send_do(msgpair.first);

		while (crow::has_untravelled() || crow::has_allocated())
		{
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}

		crow::stop_spin(false);
		crow::join_spin();
		exit(0);
	}

	// Создание консольного ввода, если необходимо.
	if (!noconsole)
	{
		if (pthread_create(&console_thread, NULL, console_listener, NULL))
		{
			fprintf(stderr, "Error creating thread\n");
			return 1;
		}
	}

	if (subscribe_mode)
	{
		std::thread([]()
		{
			while (1)
			{
				crow::subscribe(
					{addr, (size_t)addrsize}, 
					theme.c_str(), 
					qos, ackquant, qos, ackquant);
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}
		}).detach();
	}

	crow::join_spin();
}
