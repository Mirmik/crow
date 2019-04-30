#include <crow/node.h>
#include <crow/nodes/test.h>
#include <crow/nodes/action.h>

#include <crow/tower.h>
#include <crow/indexes.h>
#include <crow/gates/udpgate.h>

void incoming(crow::packet* pack) {
	igris::println("incoming");

	switch(pack->header.type) {
		case G1_G0TYPE:
			crow::incoming_node_packet(pack);
			break;
		default:
			crow::release(pack);
	}
}

void hello(crow::packet* pack) {
	igris::println("HelloWorld!!!!!");
}

int main() {
	crow::create_test_node(10);
	crow::create_action_node(11, hello);
	
	crow::incoming_handler = incoming;
	crow::__send(0,11,nullptr,0,"HelloWorld",10,crow::QoS(0));
	
	crow::spin();
}