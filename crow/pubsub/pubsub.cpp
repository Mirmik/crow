#include "pubsub.h"
#include "subscriber.h"

#include <crow/hexer.h>
#include <igris/sync/syslock.h>

crow::pubsub_protocol_cls crow::pubsub_protocol;

crow::pubsub_protocol_cls& crow::pubsub_protocol_cls::instance() 
{
	return crow::pubsub_protocol;
}

void crow::pubsub_protocol_cls::incoming(crow::packet_ptr pack)
{
	if (incoming_handler)
	{
		incoming_handler(pack);
	}
	else
	{
		crow::subscriber * sub;
		igris::buffer theme = crow::pubsub::get_theme(pack);
		
		dlist_for_each_entry(sub, &subscribers, lnk)
		{
			if (theme == sub->theme)
			{
				sub->newpack_handler(pack);
				return;
			}
		}
	}
}

void crow::pubsub_protocol_cls::undelivered(crow::packet_ptr pack)
{
	if (undelivered_handler) 
	{
		undelivered_handler(pack);
	}
}

crow::packet_ptr crow::publish(
    const crow::hostaddr_view & addr, 
    const std::string_view theme, 
    const igris::buffer data,
    uint8_t qos, uint16_t acktime, uint8_t type)
{
	struct crow_subheader_pubsub subps;
	struct crow_subheader_pubsub_data subps_d;

	subps.type = type;
	subps.thmsz = theme.size();
	subps_d.datsz = data.size();

	const igris::buffer iov[4] =
	{
		{&subps, sizeof(subps)},
		{&subps_d, sizeof(subps_d)},
		{theme.data(), subps.thmsz},
		data,
	};

	return crow::send_v(addr, iov, 4, CROW_PUBSUB_PROTOCOL,
	             qos, acktime, false);
}

crow::packet_ptr crow::publish_v(
    const crow::hostaddr_view & addr, 
    const std::string_view theme, 
    const igris::buffer * vec,
    int vecsz,
    uint8_t qos, uint16_t acktime)
{
	struct crow_subheader_pubsub subps;
	struct crow_subheader_pubsub_data subps_d;

	subps.type = PUBLISH;
	subps.thmsz = theme.size();
	subps_d.datsz = 0;

	for (int i = 0; i < vecsz; i++) 
	{
		subps_d.datsz += vec[i].size();		
	}

	const igris::buffer iov[4] =
	{
		{&subps, sizeof(subps)},
		{&subps_d, sizeof(subps_d)},
		{theme.data(), subps.thmsz},
	};

	return crow::send_vv(addr, iov, 4, vec, vecsz, CROW_PUBSUB_PROTOCOL,
	             qos, acktime, false);
}

void crow::subscribe(
    const crow::hostaddr_view & addr, 
    const std::string_view theme, 
    uint8_t qos, uint16_t acktime,
    uint8_t rqos, uint16_t racktime)
{
	size_t thmsz = theme.size();

	struct crow_subheader_pubsub subps;
	struct crow_subheader_pubsub_control subps_c;

	subps.type = SUBSCRIBE;
	subps.thmsz = (uint8_t)thmsz;
	subps_c.qos = rqos;
	subps_c.ackquant = racktime;

	const igris::buffer iov[3] =
	{
		{&subps, sizeof(subps)},
		{&subps_c, sizeof(subps_c)},
		{theme.data(), theme.size()},
	};

	crow::send_v(addr, iov, 3, CROW_PUBSUB_PROTOCOL, qos,
	             acktime);
}

void crow::pubsub_protocol_cls::resubscribe_all()
{
	crow::subscriber * sub;

	system_lock();
	dlist_for_each_entry(sub, &subscribers, lnk)
	{
		system_unlock();
		sub->resubscribe();
		system_lock();
	}
	system_unlock();
}
