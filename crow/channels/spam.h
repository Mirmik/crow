#ifndef G2_CHANNEL_SPAM_H
#define G2_CHANNEL_SPAM_H

#include <g2/core.h>
#include <igris/panic.h>

namespace g2 {
	struct spam_channel : public channel {
		void incoming_packet(g1::packet* pack) override {
			igris::panic("spam incoming!!!");
		} 
	};

	spam_channel* create_spam_channel(uint16_t id) {
		auto ptr = new g2::spam_channel;
		g2::link_channel(ptr, id);
		return ptr;
	}
}

#endif