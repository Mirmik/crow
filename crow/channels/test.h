#ifndef G2_CHANNEL_TEST_H
#define G2_CHANNEL_TEST_H

#include <crow/channel.h>
#include <igris/print/stdprint.h>
#include <igris/util/string.h>

namespace crow {
	struct test_channel : public channel {
		void incoming_data_packet(crow::packet* pack) override {
			auto data = crow::get_datasect(pack);
			igris::println("test_incoming:", igris::dstring(data));
		} 
	};

	test_channel* create_test_channel(uint16_t id) {
		auto ptr = new crow::test_channel;
		crow::link_channel(ptr, id);
		return ptr;
	}
}

#endif