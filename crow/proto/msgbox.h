#ifndef CROW_MSGBOX_H
#define CROW_MSGBOX_H

#include <crow/proto/node.h>
#include <igris/datastruct/dlist.h>
#include <igris/sync/syslock.h>
#include <igris/sync/semaphore.h>

#include <vector>

#define CROW_MSGBOX_STATE_NONE 0
#define CROW_MSGBOX_STATE_SEND 1
#define CROW_MSGBOX_STATE_RECEIVE 2

namespace crow
{
	class msgbox : public crow::node
	{
		struct semaphore message_lock = SEMAPHORE_INIT(message_lock, 1);
		struct dlist_head messages = DLIST_HEAD_INIT(messages);

	public:
		crow::node_packet_ptr query(uint16_t rid,
		                            const crow::hostaddr_view & addr,
		                            const igris::buffer data,
		                            uint8_t qos, uint16_t ackquant);

		crow::node_packet_ptr receive();

		crow::packet_ptr reply(crow::node_packet_ptr msg, 
			igris::buffer data,
			uint8_t qos,
			uint16_t ackquant) ;

		void incoming_packet(crow::packet_ptr pack) override;

		void undelivered_packet(crow::packet_ptr pack) override;

		~msgbox() override;
	};
}

#endif
