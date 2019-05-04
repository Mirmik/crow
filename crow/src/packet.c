/**
@file packet.cpp
*/

#include <string.h>
#include <stdio.h>

#include <crow/packet.h>
#include <crow/gateway.h>
#include <igris/sync/syslock.h>

crowket_t* crow_create_packet(crow_gw_t* ingate, size_t addrsize, size_t datasize) { 
	system_lock();
	crowket_t* pack = crow_allocate_packet(addrsize + datasize);
	system_unlock();
	
	pack -> header.flen = sizeof(crow_header_t) + addrsize + datasize;
	pack -> header.alen = addrsize;
	pack -> header.ackquant = 200;
	pack -> header.pflag = 0;
	pack -> header.qos = 0;
	pack -> header.stg = 0;

	dlist_init(&pack->lnk);
	dlist_init(&pack->ulnk);
	pack -> ingate = ingate;
	pack -> ackcount = 0; 
	pack -> flags = 0;
	
	return pack;
}

void crowket_initialization(crowket_t* pack, crow_gw_t* ingate) { 
	dlist_init(&pack->lnk);
	dlist_init(&pack->ulnk);
	pack -> ingate = ingate;
	pack -> ackcount = 0; 
	pack -> flags = 0;
}

void crow_utilize(crowket_t* pack) {
	system_lock();
	dlist_del(&pack->lnk); // Очищается в tower_release((см. tower.c))
	dlist_del(&pack->ulnk);
	crow_deallocate_packet(pack);
	system_unlock();
}

void crowket_revert_g(crowket_t* pack, uint8_t gateindex) {
	*crowket_stageptr(pack) = gateindex;
	++pack->header.stg;
}

void crowket_revert(crowket_t* pack, struct iovec* vec, size_t veclen) {
	struct iovec* it = vec + veclen - 1;
	struct iovec* eit = vec - 1;

	size_t sz = 0;
	uint8_t* tgt = crowket_stageptr(pack);

	for (; it != eit; --it) {
		sz += it->iov_len;
		char* ptr = (char*)it->iov_base + it->iov_len;
		char* eptr = it->iov_base;
		while(ptr != eptr) 
			*tgt++ = *--ptr;
	}
	pack->header.stg += sz;
}