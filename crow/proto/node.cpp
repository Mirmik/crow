#include <crow/proto/node.h>
#include <crow/defs.h>
#include <crow/tower.h>

#include <igris/util/numconvert.h>
#include <igris/sync/syslock.h>
#include <crow/print.h>

igris::dlist<crow::node, &crow::node::lnk> crow::nodes;
crow::node_protocol_cls crow::node_protocol;

crow::packet_ptr crow::node_send(uint16_t sid, uint16_t rid, const void *raddr,
                     size_t rsize, const void *data, size_t size, uint8_t qos,
                     uint16_t ackquant)
{
	crow::node_subheader sh;
	sh.sid = sid;
	sh.rid = rid;
	sh.type = CROW_NODEPACK_COMMON;
	sh.namerid = false;

	const igris::buffer iov[2] = {{(void *)&sh, sizeof(sh)}, {(void *)data, size}};

	return crow::send_v(raddr, rsize, iov, 2, CROW_NODE_PROTOCOL, qos, ackquant);
}

crow::packet_ptr crow::node_send(uint16_t sid, const char* rid, const void *raddr,
                     size_t rsize, const void *data, size_t size, uint8_t qos,
                     uint16_t ackquant)
{
	crow::node_subheader sh;
	sh.sid = sid;
	sh.rid = strlen(rid);
	sh.type = CROW_NODEPACK_COMMON;
	sh.namerid = true;

	const igris::buffer iov[3] =
	{
		{(void *)&sh, sizeof(sh)},
		{(void *)rid, sh.rid},
		{(void *)data, size}
	};

	return crow::send_v(raddr, rsize, iov, 3, CROW_NODE_PROTOCOL, qos, ackquant);
}

crow::packet_ptr crow::node_send_v(uint16_t sid, uint16_t rid, const igris::buffer addr,
    	const igris::buffer * vec, size_t veclen, uint8_t qos,
	    uint16_t ackquant) 
{
	crow::node_subheader sh;
	sh.sid = sid;
	sh.rid = rid;
	sh.type = CROW_NODEPACK_COMMON;
	sh.namerid = false;

	const igris::buffer iov[1] = {
		{(void *)&sh, sizeof(sh)}
	};

	return crow::send_vv(addr, iov, 1, vec, veclen, CROW_NODE_PROTOCOL, qos, ackquant);
}

crow::packet_ptr crow::node_send_v(uint16_t sid, const char * rid, const igris::buffer addr,
	   const igris::buffer * vec, size_t veclen, uint8_t qos,
	   uint16_t ackquant) 
{
	crow::node_subheader sh;
	sh.sid = sid;
	sh.rid = strlen(rid);
	sh.type = CROW_NODEPACK_COMMON;
	sh.namerid = true;

	const igris::buffer iov[2] =
	{
		{(void *)&sh, sizeof(sh)},
		{(void *)rid, sh.rid},
	};

	return crow::send_vv(addr, iov, 2, vec, veclen, CROW_NODE_PROTOCOL, qos, ackquant);
}

void crow::node_protocol_cls::send_node_error(
    crow::packet *pack, int errcode)
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

	crow::send_v(pack->addrptr(), pack->addrsize(),
	             iov, 2, CROW_NODE_PROTOCOL, 0, pack->ackquant());
}

void crow::node_protocol_cls::incoming(crow::packet *pack)
{
	crow::node_subheader *sh = (crow::node_subheader *) pack->dataptr();
	crow::node * srv = nullptr;

	if (sh->namerid == 0)
	{
		for (crow::node &srvs : crow::nodes)
		{
			if (srvs.id == sh->rid)
			{
				srv = &srvs;
				break;
			}
		}
	}

	else // named node request
	{
		igris::buffer name = node_protocol_cls::get_name(pack);

		for (crow::node &srvs : crow::nodes)
		{
			if (name == srvs.mnem)
			{
				srv = &srvs;
				break;
			}
		}	
	}

	if (srv == nullptr)
	{
		//send_node_error(pack, CROW_ERRNO_UNREGISTRED_RID);
		crow::release(pack);
		return;
	}

	switch (sh->type)
	{
		case CROW_NODEPACK_COMMON:
			srv->incoming_packet(pack);
			break;

		case CROW_NODEPACK_ERROR:
			srv->notify_one(get_error_code(pack));
			crow::release(pack);
			break;
	}
	return;
}

void crow::node_protocol_cls::undelivered(crow::packet *pack)
{
	crow::node_subheader *sh = (crow::node_subheader *) pack->dataptr();

	for (crow::node &srvs : crow::nodes)
	{
		if (srvs.id == sh->sid)
		{
			srvs.undelivered_packet(pack);
			return;
		}
	}

	crow::release(pack);
}

void crow::link_node(crow::node *srv, uint16_t id)
{
	srv->id = id;
	system_lock();
	nodes.add_last(*srv);
	system_unlock();
}




void crow::system_node_cls::incoming_packet(crow::packet *pack)
{
	crow::node_subheader *sh = (crow::node_subheader *) pack->dataptr();
	auto data = node_protocol_cls::node_data(pack);

	char buf[32];

	if (data == "nodes" || data == "nodes\n")
	{
		for (crow::node &srvs : crow::nodes)
		{
			//int len = i32toa(srvs.id, buf, 10) - buf;
			//buf[len] = '\n';
			//buf[len+1] = 0;
			sprintf(buf, "%d %s %s\n", srvs.id, srvs.typestr(), srvs.mnem);

			node_send(0, sh->sid, pack->addrptr(), pack->addrsize(),
			          buf, strlen(buf), 0, 200);
		}
	}

	else 
	{
		sprintf(buf, "Unrecognized command\n");
		node_send(0, sh->sid, pack->addrptr(), pack->addrsize(),
	          buf, strlen(buf), 0, 200);
			
	}
}