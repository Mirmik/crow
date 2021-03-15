/**
@file packet.cpp
*/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <crow/gateway.h>
#include <crow/packet.h>
#include <igris/sync/syslock.h>

#include <igris/compiler.h>

void crow::packet_initialization(crow::packet *pack, crow::gateway *ingate)
{
	pack->init(ingate);
}

crow::packet *crow::create_packet(crow::gateway *ingate, uint8_t addrsize,
                                  size_t datasize)
{
	crow::packet *pack = crow::allocate_packet(addrsize + datasize);

	if (pack == nullptr)
		return nullptr;

	pack->header.flen = (uint16_t)(sizeof(crow::header) + addrsize + datasize);
	pack->header.alen = addrsize;
	pack->header.ackquant = 200;
	pack->header.pflag = 0;
	pack->header.qos = 0;
	pack->header.stg = 0;

	packet_initialization(pack, ingate);

	return pack;
}

void crow::packet::revert_gate(uint8_t gateindex)
{
	*stageptr() = gateindex;
	++header.stg;
}

void crow::packet::revert(igris::buffer *vec, size_t veclen)
{
	igris::buffer *it = vec + veclen - 1;
	igris::buffer *eit = vec - 1;

	size_t sz = 0;
	uint8_t *tgt = stageptr();

	for (; it != eit; --it)
	{
		sz += it->size();
		char *ptr = (char *)it->data() + it->size();
		char *eptr = (char *)it->data();
		while (ptr != eptr)
			*tgt++ = *--ptr;
	}
	header.stg = (uint8_t)(header.stg + sz);
}


bool crow::has_allocated()
{
	return !!allocated_count;
}

crow::packet_ptr crow::packet::first_user_list_entry(dlist_head * head)
{
	igris::syslock_guard lock;
	return dlist_first_entry(head, crow::packet, ulnk);
}

// change to packet_ptr
crow::packet_ptr crow::packet::first_system_list_entry(dlist_head * head)
{
	igris::syslock_guard lock;
	return dlist_first_entry(head, crow::packet, lnk);
}
