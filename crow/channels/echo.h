#ifndef G2_CHANNEL_ECHO_H
#define G2_CHANNEL_ECHO_H

#include <crow/channel.h>
#include <igris/print/stdprint.h>

namespace crow {
	struct echo_channel : public channel {
		void incoming_data_packet(crow::packet* pack) override {
			auto data = crow::get_datasect(pack);
			crow::__channel_send(this, "TEST", 4);
			crow::release(pack);
		} 
	};

	echo_channel* create_echo_channel(uint16_t id) {
		auto ptr = new crow::echo_channel;
		crow::link_channel(ptr, id);
		return ptr;
	}
}

#endif