#ifndef G2_CHANNEL_ECHO_H
#define G2_CHANNEL_ECHO_H

#include <crow/channel.h>
#include <gxx/print/stdprint.h>

namespace crow {
	struct echo_channel : public channel {
		void incoming_data_packet(crow::packet* pack) override {
			gxx::println("incoming_packet");
			auto data = crow::get_datasect(pack);
			gxx::println(data);

			gxx::println("pretest");
			dprhexln(this);
			crow::__channel_send(this, "TEST", 4);
			gxx::println("aftertest");

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