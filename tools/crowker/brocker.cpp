#include "brocker.h"
#include <crow/pubsub.h>
#include <sys/uio.h>
#include <gxx/print.h>

std::unordered_map<std::string, crow::theme> themes;

void brocker_publish(const std::string& theme, const std::string& data)
{
	//gxx::println("brocker_publish");
	//gxx::println("theme: ", theme);
	//gxx::println("data: ", data);

	try
	{
		auto& thm = themes.at(theme);
		thm.publish(data);
	}
	catch (std::exception ex)
	{
		//gxx::println("unres theme");
	}
}


void g3_brocker_subscribe(uint8_t* raddr, size_t rlen, const std::string& theme, uint8_t qos, uint16_t ackquant)
{
	//gxx::println("add subscribe");

	if (themes.count(theme) == 0)
	{
		themes[theme] = crow::theme(theme);
	}

	GXX_PRINT(theme);

	auto& thm = themes[theme];
	thm.subs.emplace(raddr, rlen, qos, ackquant);
}

void crow::theme::publish(const std::string& data)
{
	struct crow_subheader_pubsub subps;
	struct crow_subheader_pubsub_data subps_d;

	subps.type = PUBLISH;
	subps.thmsz = name.size();
	subps_d.datsz = data.size();

	iovec vec[4] =
	{
		{ &subps, sizeof(subps) },
		{ &subps_d, sizeof(subps_d) },
		{(void*)name.data(), name.size()},
		{(void*)data.data(), data.size()}
	};

	for (auto& sub : subs)
	{
		crow_send_v(sub.host, sub.hlen, vec, 4, G1_G3TYPE, sub.qos, sub.ackquant);
	}
}
