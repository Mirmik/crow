#include <crow/tower.h>
#include <crow/pubsub.h>
#include <igris/print/stdprint.h>
#include <crow/gates/udpgate.h>
#include <thread>
#include <getopt.h>

#include <sstream>

#include <igris/trent/trent.h>
#include <igris/trent/gbson.h>
#include <igris/util/hexer.h>

uint8_t crowker_addr[256];
size_t crowker_len;

uint8_t qos = 0;
uint8_t acktime = 200;
bool gbson_flag = false;

void subscribe_handler(crowket* pack)
{
	struct crow_subheader_pubsub * shps = get_subheader_pubsub(pack);
	struct crow_subheader_pubsub_data * shps_d =
	    get_subheader_pubsub_data(pack);

	auto theme = std::string(crowket_pubsub_thmptr(pack), shps->thmsz);
	auto data = std::string(crowket_pubsub_datptr(pack), shps_d->datsz);

	if (!gbson_flag)
	{
		igris::println(igris::dstring(data));
	}
	else
	{
		//std::string str(dat.data(), dat.size());
		//std::stringstream strm(str);
		igris::trent tr;
		igris::gbson::load(tr, data.data(), data.size());
		igris::println(tr);
	}
}

void undelivered_handler(crowket* pack)
{
	dprln("Crowker access error");
	exit(-1);
}

int main(int argc, char* argv[])
{
	const char* crowker = getenv("CROWKER");

	const struct option long_options[] =
	{
		{"crowker", required_argument, NULL, 'c'},
		{"debug", no_argument, NULL, 'd'},
		{"gbson", no_argument, NULL, 'g'},
		{"qos", required_argument, NULL, 'q'},
		{NULL, 0, NULL, 0}
	};

	int long_index = 0;
	int opt = 0;
	while ((opt = getopt_long(argc, argv, "cdgq", long_options, &long_index)) != -1)
	{
		switch (opt)
		{
			case 'c': crowker = optarg; break;
			case 'd': crow_enable_diagnostic(); break;
			case 'g': gbson_flag = true;
			case 'q': qos = atoi(optarg);
			case 0: break;
		}
	}

	if (crow_create_udpgate(0, G1_UDPGATE) == NULL)
	{
		perror("udpgate open");
		exit(-1);
	}

	if (argc - optind != 1)
	{
		igris::println("Usage: crow_publish theme");
		exit(-1);
	}

	if (crowker == nullptr)
	{
		igris::println("Enviroment variable CROWKER doesn't setted");
		exit(-1);
	}

	crowker_len = hexer(crowker_addr, 128, crowker, strlen(crowker));
	crow_set_publish_host(crowker_addr, crowker_len);

	crow_undelivered_handler = undelivered_handler;
	crow_pubsub_handler = subscribe_handler;

	std::string theme = argv[optind];

	crow_subscribe(theme.data(), qos, acktime);
	crow_spin();
}

