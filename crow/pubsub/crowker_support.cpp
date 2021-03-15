#include <crow/brocker/crowker.h>
#include "pubsub.h"
#include "subscriber.h"

void incoming_crowker_handler(crow::packet_ptr pack)
{

	struct crow_subheader_pubsub *shps = crow::get_subheader_pubsub(pack);

	switch (shps->type)
	{
		case PUBLISH:
			{
				auto theme = std::string(crow::pubsub::get_theme(pack));
				auto data = std::string(crow::pubsub::get_data(pack));

				crow::crowker::instance()->publish(theme, data);
			}
			break;

		case SUBSCRIBE:
			{
				auto shps_c = get_subheader_pubsub_control(pack);
				std::string theme(pack->dataptr() + sizeof(crow_subheader_pubsub) +
				                  sizeof(crow_subheader_pubsub_control),
				                  shps->thmsz);

				crow::crowker::instance()->crow_subscribe(
				{ pack->addrptr(), pack->addrsize() },
				theme,
				shps_c->qos, shps_c->ackquant);
			}
			break;

		default:
			{
				printf("unresolved pubsub frame type %d", (uint8_t)shps->type);
			}
			break;

		case MESSAGE:
			{
				crow::subscriber * sub;
				igris::buffer theme = crow::pubsub::get_theme(pack);
				
				dlist_for_each_entry(
				    sub,
				    &crow::pubsub_protocol.subscribers,
				    lnk)
				{
					if (theme == sub->theme)
					{
						sub->newpack_handler(pack);
						return;
					}
				}
			}
	}
}

void undelivered_crowker_handler(crow::packet_ptr pack)
{
	auto shps = crow::get_subheader_pubsub(pack);

	if (shps->type == PUBLISH)
	{
		std::string theme(pack->dataptr() + sizeof(crow_subheader_pubsub) +
		                  sizeof(crow_subheader_pubsub_data),
		                  shps->thmsz);

		crow::crowker::instance()->erase_crow_subscriber(
		    std::string((char *)pack->addrptr(), pack->header.alen));
	}
}

void crow::pubsub_protocol_cls::enable_crowker_subsystem()
{
	instance().incoming_handler = incoming_crowker_handler;
	instance().undelivered_handler = undelivered_crowker_handler;
}
