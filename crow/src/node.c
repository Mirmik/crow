#include <crow/node.h>
//#include <crow/nodes/test.h>
//#include <crow/nodes/action.h>
#include <crow/tower.h>
#include <igris/sync/syslock.h>

DLIST_HEAD(crow_nodes);

void crow_node_send(uint16_t sid, uint16_t rid, const void* raddr, size_t rsize, const void* data, size_t size, uint8_t qos, uint16_t ackquant) {		
	crow_subheader_t sh;
	sh.sid = sid;
	sh.rid = rid;

	struct iovec iov[2] = {
		{ (void*)&sh, sizeof(sh) },
		{ (void*)data, size }
	};

	crow_send_v(raddr, rsize, iov, 2, G1_G0TYPE, qos, ackquant);
}


void crow_incoming_node_packet(crowket_t* pack) {
	crow_subheader_t* sh = crow_get_subheader(pack);
	crow_node_t* srvs;
	dlist_for_each_entry(srvs, &crow_nodes, lnk) {
		if (srvs->id == sh->rid) {
			srvs->incoming_packet(srvs, pack);
			return;
		}
	}
	crow_utilize(pack);	
}

void crow_link_node(crow_node_t* srv, uint16_t id) {
	srv->id = id;
	dlist_move_tail(&srv->lnk, &crow_nodes);
}

/*crow::test_node* crow::create_test_node(int port) {
	auto tsrv = new crow::test_node();
	crow::link_node(tsrv, port);
	return tsrv;
}

crow::action_node* crow::create_action_node(int port, igris::delegate<void, crow::packet*> dlg) {
	auto asrv = new crow::action_node(dlg);
	crow::link_node(asrv, port);
	return asrv;
}*/