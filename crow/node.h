/**
	@file
*/

#ifndef G0_CORE_H
#define G0_CORE_H

#include <crow/packet.h>
#include <sys/uio.h>

typedef struct crow_node {
	struct dlist_head lnk;
	uint16_t id;
	void (*incoming_packet)(struct crow_node* node, crowket_t* pack);
} crow_node_t;

typedef struct crow_subheader {
	uint16_t sid;
	uint16_t rid;		
} G1_PACKED crow_subheader_t;

__BEGIN_DECLS

static inline void crow_node_init(crow_node_t* node) { 
	dlist_init(&node->lnk); 
}

static inline crow_subheader_t* crow_get_subheader(crowket_t* pack) {
	return (crow_subheader_t*) crowket_dataptr(pack);
}

/*static inline igris::buffer crow_get_datasect(crow::packet* pack) {
	return igris::buffer(pack->dataptr() + sizeof(subheader), pack->datasize() - sizeof(subheader));
}*/	

//extern igris::dlist<crow::node, &crow::node::lnk> nodes;
extern struct dlist_head crow_nodes;

/// Добавить сервис к ядру.
void crow_link_node(crow_node_t* srvs, uint16_t id);
void crow_node_send(uint16_t sid, uint16_t rid, 
	const void* raddr, size_t rlen, 
	const void* data, size_t size, 
	uint8_t qos, uint16_t ackquant);

__END_DECLS

#endif