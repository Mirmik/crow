#include <crow/tower.h>
#include <crow/pubsub.h>
#include <crow/gates/udpgate.h>
//#include <thread>
#include <getopt.h>
#include <stdbool.h>

#include <stdio.h>
#include <string>

#include "brocker.h"

int udpport = -1;
bool sniffer_option = false;

void sniffer_travel_handler(struct crowket* pack)
{
//	igris::print("travel: ");
//	crow::println(pack);
}

void incoming_pubsub_packet(struct crowket* pack)
{
	struct crow_subheader_pubsub * shps = get_subheader_pubsub(pack);

	switch (shps->type)
	{
		case PUBLISH:
		{
			struct crow_subheader_pubsub_data * shps_d =
			    get_subheader_pubsub_data(pack);

			auto theme = std::string(crowket_pubsub_thmptr(pack), shps->thmsz);
			auto data = std::string(crowket_pubsub_datptr(pack), shps_d->datsz);

			brocker_publish(theme, data);
		}
		break;
		case SUBSCRIBE:
		{
			auto shps_c = get_subheader_pubsub_control(pack);
			std::string theme(crowket_dataptr(pack) + sizeof(crow_subheader_pubsub) + sizeof(crow_subheader_pubsub_control), shps->thmsz);
			g3_brocker_subscribe(crowket_addrptr(pack), crowket_addrsize(pack), theme, shps_c->qos, shps_c->ackquant);
		}
		break;
		default:
		{
			printf("unresolved pubsub frame type %d", (uint8_t)shps->type);
		} break;
	}
	crow_release(pack);
}

void undelivered_handler(struct crowket* pack)
{
	if (pack->header.f.type == G1_G3TYPE) {
		auto shps = get_subheader_pubsub(pack);

		if (shps->type == PUBLISH) {
			std::string theme(crowket_dataptr(pack) + sizeof(crow_subheader_pubsub) 
				+ sizeof(crow_subheader_pubsub_data), shps->thmsz);
			auto& thm = themes[theme];
			thm.subs.erase(crow::g3_subscriber(crowket_addrptr(pack), pack->header.alen, pack->header.qos, pack->header.ackquant));
			if (brocker_info)
				igris::fprintln("g3_refuse: t:{}, r:{}", theme, igris::hexascii_encode(crowket_addrptr(pack), pack->header.alen));
		}
	}

	crow_utilize(pack);
}

/*void crow::theme::publish(const std::string& data) {
	crow::subheader_pubsub subps;
	crow::subheader_pubsub_data subps_d;

	subps.type = crow::frame_type::PUBLISH;
	subps.thmsz = name.size();
	subps_d.datsz = data.size();

	iovec vec[4] = {
		{ &subps, sizeof(subps) },
		{ &subps_d, sizeof(subps_d) },
		{(void*)name.data(), name.size()},
		{(void*)data.data(), data.size()}
	};

	for (auto& sub : subs) {
		crow::send(sub.host.data, sub.host.size, vec, 4, G1_G3TYPE, sub.qos, sub.ackquant);
	}
}*/

int main(int argc, char* argv[])
{
	crow_pubsub_handler = incoming_pubsub_packet;
	crow_undelivered_handler = undelivered_handler;

	const struct option long_options[] =
	{
		{"udp", required_argument, NULL, 'u'},
		{"debug", no_argument, NULL, 'd'},
		{"sniffer", no_argument, NULL, 's'},
		{NULL, 0, NULL, 0}
	};

	int long_index = 0;
	int opt = 0;
	while ((opt = getopt_long(argc, argv, "usdi", long_options, &long_index)) != -1)
	{
		switch (opt)
		{
			case 'u': udpport = atoi(optarg); break;
			case 'd': crow_enable_diagnostic(); break;
			case 's': sniffer_option = true; break;
			case 'i': brocker_info = true; break;
			case 0: break;
		}
	}

	if (udpport == -1)
	{
		printf("You should specify udp port.\n");
		exit(-1);
	}

	if (crow_create_udpgate(udpport, G1_UDPGATE) == NULL)
	{
		perror("udpgate open");
		exit(-1);
	}

	if (sniffer_option) crow_traveling_handler = sniffer_travel_handler;

	crow_spin();
}

