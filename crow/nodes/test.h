#ifndef G0_TEST_H
#define G0_TEST_H

#include <gxx/util/string.h>
#include <gxx/print.h>
#include <gxx/print/stdprint.h>
#include <crow/tower.h>

namespace crow {
	struct test_node : public node {
		void incoming_packet(crow::packet* pack) override {
			auto sh = crow::get_subheader(pack);
			auto data = crow::get_datasect(pack);
			crow::println(pack);
			gxx::fprintln("subheader: sid={}, rid={}", (uint16_t)sh->sid, (uint16_t)sh->rid);
			gxx::fprintln("datasect: {}", gxx::dstring(data));
			crow::release(pack);
		}
	};

	crow::test_node* create_test_node(int i);
}

#endif