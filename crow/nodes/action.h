#ifndef G0_ACTION_H
#define G0_ACTION_H

#include <gxx/event/delegate.h>
#include <crow/tower.h>

namespace crow {
	struct action_node : public node {
		gxx::delegate<void, crow::packet*> dlg;

		action_node(gxx::delegate<void, crow::packet*> dlg) : dlg(dlg) {}

		void incoming_packet(crow::packet* pack) override {
			dlg(pack);
		}
	};

	crow::action_node* create_action_node(int i, gxx::delegate<void, crow::packet*> dlg);
}

#endif