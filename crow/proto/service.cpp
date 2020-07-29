#include <crow/proto/service.h>

/*
crow::packet_ptr crow::service_request_ptr::answer(igris::buffer data)
{
	auto subheader = node::subheader(this->pack);
	return srv->send(subheader->sid, pack->addr(), data, pack->qos(), pack->ackquant());
}*/

crow::packet_ptr crow::crowker_service_registration(const crow::hostaddr & crowker,
                                const crow::node & node,
                                const char * mnemo,
                                uint8_t qos,
                                uint16_t ackquant)
{
	uint8_t type = CROWKER_SERVICE_REGISTER;

	igris::buffer bufs[] =
	{
		{ &type, 1 },
		{ mnemo, strlen(mnemo) }
	};

	return node.send_v(CROWKER_SYSTEM_NODE, crowker, bufs, std::size(bufs), qos, ackquant);
}