#ifndef G0_DGRAMM_SPAMER_H
#define G0_DGRAMM_SPAMER_H

#include <igris/io/ostream.h>
#include <crow/node.h>
#include <igris/util/hexer.h>

namespace crow {
	struct dgramm_spammer : public crow::basic_service, public igris::io::ostream {
		int rid;
		uint8_t* addr;
		size_t size;

		dgramm_spammer(int rid, uint8_t* addr, size_t size) : rid(rid), addr(addr), size(size) {}
		dgramm_spammer(int rid, const char* hexaddr, uint8_t* addr, size_t size) : rid(rid), addr(addr), size(size) {
			hexer(addr, size, hexaddr, strlen(hexaddr));
		}

		int writeData(const char* dat, size_t sz) {
			crow::send(addr, size, dat, sz, 0, g1::QoS(0));
		}

		void incoming_message(crow::message*) override {}
	};
}

#endif