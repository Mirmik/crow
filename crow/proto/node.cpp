#include <crow/proto/node.h>
#include <crow/defs.h>
#include <crow/tower.h>

#include <igris/util/numconvert.h>
#include <igris/sync/syslock.h>
#include <crow/print.h>

igris::dlist<crow::node, &crow::node::lnk> crow::nodes_list;
crow::node_protocol_cls crow::node_protocol;

crow::packet_ptr crow::node_send(uint16_t sid,
                                 uint16_t rid,
                                 const crow::hostaddr_view & addr,
                                 const igris::buffer data,
                                 uint8_t qos,
                                 uint16_t ackquant,
                                 bool fastsend)
{
	crow::node_subheader sh;
	sh.sid = sid;
	sh.rid = rid;
	sh.type = CROW_NODEPACK_COMMON;

	const igris::buffer iov[2] = {{(void *)&sh, sizeof(sh)}, {(void *)data.data(), data.size()}};

	return crow::send_v(addr, iov, 2, CROW_NODE_PROTOCOL, qos, ackquant, fastsend);
}

crow::packet_ptr crow::node_send_special(uint16_t sid,
        uint16_t rid,
        const crow::hostaddr_view & addr,
        uint8_t type,
        const igris::buffer data,
        uint8_t qos,
        uint16_t ackquant,
        bool fastsend)
{
	crow::node_subheader sh;
	sh.sid = sid;
	sh.rid = rid;
	sh.type = type;

	const igris::buffer iov[2] = {{(void *)&sh, sizeof(sh)}, {(void *)data.data(), data.size()}};

	return crow::send_v(addr, iov, 2, CROW_NODE_PROTOCOL, qos, ackquant, fastsend);
}

crow::packet_ptr crow::node_send_v(uint16_t sid,
                                   uint16_t rid,
                                   const crow::hostaddr_view & addr,
                                   const igris::buffer * vec,
                                   size_t veclen,
                                   uint8_t qos,
                                   uint16_t ackquant,
                                   bool fastsend)
{
	crow::node_subheader sh;
	sh.sid = sid;
	sh.rid = rid;
	sh.type = CROW_NODEPACK_COMMON;

	const igris::buffer iov[1] =
	{
		{(void *)&sh, sizeof(sh)}
	};

	return crow::send_vv(addr, iov, 1, vec, veclen, CROW_NODE_PROTOCOL, qos, ackquant, fastsend);
}

void crow::node_protocol_cls::send_node_error(
    crow::packet_ptr pack, int errcode)
{
	crow::node_subheader sh;

	sh.sid = crow::node_protocol.rid(pack);
	sh.rid = crow::node_protocol.sid(pack);
	sh.type = CROW_NODEPACK_ERROR;

	const igris::buffer iov[2] =
	{
		{(void *)&sh, sizeof(sh)},
		{(void *)&errcode, sizeof(errcode)}
	};

	crow::send_v(pack->addr(), iov, 2, CROW_NODE_PROTOCOL, 0, pack->ackquant());
}

void crow::node_protocol_cls::incoming(crow::packet_ptr pack)
{
	crow::node_subheader *sh = (crow::node_subheader *) pack->dataptr();
	crow::node * srv = nullptr;

	for (crow::node &srvs : crow::nodes_list)
	{
		if (srvs.id == sh->rid)
		{
			srv = &srvs;
			break;
		}
	}

	if (srv == nullptr)
	{
		send_node_error(pack, CROW_ERRNO_UNREGISTRED_RID);
		return;
	}

	switch (sh->type)
	{
		case CROW_NODEPACK_COMMON:
			srv->incoming_packet(pack);
			break;

		case CROW_NODEPACK_ERROR:
			srv->notify_one(get_error_code(pack));
			break;
	}
	return;
}

void crow::node_protocol_cls::undelivered(crow::packet_ptr pack)
{
	crow::node_subheader *sh = (crow::node_subheader *) pack->dataptr();

	for (crow::node &srvs : crow::nodes_list)
	{
		if (srvs.id == sh->sid)
		{
			srvs.undelivered_packet(pack);
			return;
		}
	}
}

void crow::__link_node(crow::node *srv, uint16_t id)
{
	srv->id = id;
	//system_lock();
	nodes_list.add_last(*srv);
	//system_unlock();
}

crow::node * crow::find_node(int id)
{
	int protector = 0;

	//TODO: переделать на хештаблицу

	for (crow::node& node : nodes_list)
	{
		protector++;

		if (node.id == id)
			return &node;

		assert(protector < 512);
	}

	return nullptr;
}

void crow::bind_node_dynamic(crow::node *srv)
{
	// Динамические порты располагаются в верхнем полупространстве.
	static uint16_t counter = (1 << 15);

	system_lock();
	do
	{
		counter++;
		if (counter == 0) counter = (1 << 15);
	}
	while (crow::find_node(counter) != nullptr);

	__link_node(srv, counter);
	system_unlock();
}



crow::node::~node()
{
	system_lock();

	if (!dlist_empty(&waitlnk))
		notify_all(-1);

	dlist_del(&lnk);

	system_unlock();
}
