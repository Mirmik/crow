/**
@file tower.cpp
*/

#include <stdbool.h>
#include <crow/tower.h>
#include <crow/host.h>
//#include <igris/util/hexascii.h>
//#include <igris/algorithm.h>
//#include <igris/util/string.h>
#include <igris/sync/syslock.h>
//#include <igris/print/stdprint.h>

#include <igris/dprint.h>

/*igris::dlist<crow_gw_t, &crow_gw_t::lnk> crow_gw_ts;
igris::dlist<crowket_t, &crowket_t::lnk> crow_travelled;
igris::dlist<crowket_t, &crowket_t::lnk> crow_incoming;
igris::dlist<crowket_t, &crowket_t::lnk> crow_outters;*/

DLIST_HEAD(crow_gateways);
DLIST_HEAD(crow_travelled);
DLIST_HEAD(crow_incoming);
DLIST_HEAD(crow_outters);

void(*crow_user_type_handler)(crowket_t* pack) = NULL;
void(*crow_user_incoming_handler)(crowket_t* pack) = NULL;
void(*crow_undelivered_handler)(crowket_t* pack) = NULL;
void(*crow_traveling_handler)(crowket_t* pack) = NULL;
void(*crow_transit_handler)(crowket_t* pack) = NULL;

static bool __diagnostic_enabled = false;

static void __diagnostic_traveling_handler(crowket_t* pack)
{
	debug_print("travel: ");
	crow_println(pack);
}

void crow_enable_diagnostic()
{
	__diagnostic_enabled = true;
	crow_traveling_handler = __diagnostic_traveling_handler;
}

crow_gw_t* crow_find_target_gateway(crowket_t* pack)
{
	uint8_t gidx = *crowket_stageptr(pack);
	crow_gw_t* g;
	dlist_for_each_entry(g, &crow_gateways, lnk)
	{
		if (g->id == gidx) return g;
	}
	return NULL;
}

void crow_release(crowket_t* pack)
{
	system_lock();

	if (pack->f.released_by_tower) crow_utilize(pack);
	else pack->f.released_by_world = true;

	system_unlock();
}

void crow_tower_release(crowket_t* pack)
{
	system_lock();

	//Скорее всего это поле должно освобождаться без инициализации.
	//Инициализируем для последуещего освобождения в utilize
	dlist_del_init(&pack->lnk);

	if (pack->f.released_by_world) crow_utilize(pack);
	else pack->f.released_by_tower = true;
	system_unlock();
}

void utilize_from_outers(crowket_t* pack)
{
	crowket_t* it;
	dlist_for_each_entry(it, &crow_outters, lnk)
	{
		if (
		    it->header.seqid == pack->header.seqid &&
		    pack->header.alen == it->header.alen &&
		    !memcmp(crowket_addrptr(it), crowket_addrptr(pack), pack->header.alen)
		)
		{
			crow_utilize(it);
			return;
		}
	}
}

void qos_release_from_incoming(crowket_t* pack)
{
	crowket_t* it;
	dlist_for_each_entry(it, &crow_incoming, lnk)
	{
		if (it->header.seqid == pack->header.seqid &&
		        pack->header.alen == it->header.alen &&
		        !memcmp(crowket_addrptr(it), crowket_addrptr(pack), pack->header.alen)
		   )
		{
			crow_tower_release(it);
			return;
		}
	}
}

void add_to_incoming_list(crowket_t* pack)
{
	pack->last_request_time = crow_millis();
	dlist_move_tail(&pack->lnk, &crow_incoming);
}

void add_to_outters_list(crowket_t* pack)
{
	pack->last_request_time = crow_millis();
	dlist_move_tail(&pack->lnk, &crow_outters);
}

void crow_travel(crowket_t* pack)
{
	system_lock();
	dlist_add_tail(&pack->lnk, &crow_travelled);
	system_unlock();
}

void crow_travel_error(crowket_t* pack)
{
	crow_utilize(pack);
}

void crow_incoming_handler(crowket_t* pack)
{
//	if (__diagnostic_enabled)
//	{
//		debug_print_line("incoming packet");
//	}

	switch (pack->header.f.type)
	{
		case G1_G0TYPE: crow_incoming_node_packet(pack); break;
		case G1_G3TYPE:
			if (crow_pubsub_handler) crow_pubsub_handler(pack);
			else crow_release(pack);
			break;
		default:
			if (crow_user_type_handler) crow_user_type_handler(pack);
			else crow_release(pack);
	}
}

void crow_do_travel(crowket_t* pack)
{
	if (crow_traveling_handler)
		crow_traveling_handler(pack);

	if (pack->header.stg == pack->header.alen)
	{
		//Ветка доставленного пакета.
		crow_revert_address(pack);

		if (pack->header.f.ack)
		{
			switch (pack->header.f.type)
			{
				case G1_ACK_TYPE:
					utilize_from_outers(pack);
					break;
				case G1_ACK21_TYPE:
					utilize_from_outers(pack);
					crow_send_ack2(pack);
					break;
				case G1_ACK22_TYPE:
					qos_release_from_incoming(pack);
					break;
				default:
					break;
			}
			crow_utilize(pack);
			return;
		}

		if (pack->ingate)
		{
			if (pack->header.qos == CROW_TARGET_ACK || pack->header.qos == CROW_BINARY_ACK)
				crow_send_ack(pack);

			if (pack->header.qos == CROW_BINARY_ACK)
			{
				crowket_t* inc;
				dlist_for_each_entry(inc, &crow_incoming, lnk)
				{
					if (inc->header.seqid == pack->header.seqid &&
					        inc->header.alen == pack->header.alen &&
					        memcmp(crowket_addrptr(inc), crowket_addrptr(pack), inc->header.alen) == 0)
					{
						crow_utilize(pack);
						return;
					}
				}
				system_lock();
				add_to_incoming_list(pack);
				system_unlock();
			}
			else
			{
				crow_tower_release(pack);
			}
		}
		//Если пакет отправлен из данного нода, обслуживание не требуется
		else
		{
			crow_tower_release(pack);
		}

		if (!pack->header.f.noexec)
		{
			if (crow_user_incoming_handler) crow_user_incoming_handler(pack);
			else crow_incoming_handler(pack);
		}
		else crow_release(pack);

		return;
	}
	else
	{
		if (crow_transit_handler) crow_transit_handler(pack);
		//Ветка транзитного пакета. Логика поиска врат и пересылки.
		crow_gw_t* gate = crow_find_target_gateway(pack);
		if (gate == NULL)
		{
			crow_travel_error(pack);
		}
		else
		{
			//Здесь пакет штампуется временем отправки и пересылается во врата.
			//Врата должны после пересылки отправить его назад в башню
			//с помощью return_to_tower для контроля качества.
			gate->ops->send(gate, pack);
		}
	}
}

uint16_t __seqcounter = 0;
void crow_transport(crowket_t* pack)
{
	pack->header.stg = 0;
	pack->header.f.ack = 0;
	system_lock();
	pack->header.seqid = __seqcounter++;
	system_unlock();
	crow_travel(pack);
}

//void crow_send(crow_address& addr, const char* data, size_t len, uint8_t type, uint8_t qos, uint16_t ackquant) {
void crow_send(const void* addr, uint8_t asize, const char* data, uint16_t dsize, uint8_t type, uint8_t qos, uint16_t ackquant)
{
	crowket_t* pack = crow_create_packet(NULL, asize, dsize);
	pack->header.f.type = type;
	pack->header.qos = qos;
	pack->header.ackquant = ackquant;
	//if (addr)
	memcpy(crowket_addrptr(pack), addr, asize);
	memcpy(crowket_dataptr(pack), data, dsize);
	crow_transport(pack);
}

void crow_send_v(const void* addr, uint8_t asize, const struct iovec* vec, size_t veclen, uint8_t type, uint8_t qos, uint16_t ackquant)
{
	size_t dsize = 0;
	const struct iovec* it = vec;
	const struct iovec* const eit = vec + veclen;

	for (; it != eit; ++it)
	{
		dsize += it->iov_len;
	}

	crowket_t* pack = crow_create_packet(NULL, asize, dsize);
	pack->header.f.type = type;
	pack->header.qos = qos;
	pack->header.ackquant = ackquant;
	//if (addr)
	memcpy(crowket_addrptr(pack), addr, asize);

	it = vec;
	char* dst = crowket_dataptr(pack);
	for (; it != eit; ++it)
	{
		memcpy(dst, it->iov_base, it->iov_len);
		dst += it->iov_len;
	}

	crow_transport(pack);
}

void crow_return_to_tower(crowket_t* pack, uint8_t sts)
{
	system_lock();
	if (pack->ingate != NULL)
	{
		//Пакет был отправлен, и он нездешний. Уничтожить.
		crow_utilize(pack);
	}
	else
	{
		//Пакет здешний.
		if (sts != CROW_SENDED || pack->header.qos == CROW_WITHOUT_ACK)
			crow_utilize(pack);
		else add_to_outters_list(pack);
	}
	system_unlock();
}

void crow_revert_address(crowket_t* pack)
{
	uint8_t* first = crowket_addrptr(pack);
	uint8_t* last = crowket_addrptr(pack) + pack->header.alen;
	while ((first != last) && (first != --last))
	{
		char tmp = *last;
		*last = *first;
		*first++ = tmp;
	}
}

void crow_send_ack(crowket_t* pack)
{
	crowket_t* ack = crow_create_packet(NULL, pack->header.alen, 0);
	ack->header.f.type = pack->header.qos == CROW_BINARY_ACK ? G1_ACK21_TYPE : G1_ACK_TYPE;
	ack->header.f.ack = 1;
	ack->header.qos = CROW_WITHOUT_ACK;
	ack->header.seqid = pack->header.seqid;
	memcpy(crowket_addrptr(ack), crowket_addrptr(pack), pack->header.alen);
	ack->f.released_by_world = true;
	crow_travel(ack);
}

void crow_send_ack2(crowket_t* pack)
{
	crowket_t* ack = crow_create_packet(NULL, pack->header.alen, 0);
	ack->header.f.type = G1_ACK22_TYPE;
	ack->header.f.ack = 1;
	ack->header.qos = CROW_WITHOUT_ACK;
	ack->header.seqid = pack->header.seqid;
	memcpy(crowket_addrptr(ack), crowket_addrptr(pack), pack->header.alen);
	crow_travel(ack);
}

void crow_onestep_travel_only()
{
	system_lock();
	while (1)
	{
		system_lock();
		bool empty = dlist_empty(&crow_travelled);
		if (empty) break;
		crowket_t* pack = dlist_first_entry(&crow_travelled, crowket_t, lnk);
		dlist_del_init(&pack->lnk);
		system_unlock();
		crow_do_travel(pack);
	}
	system_unlock();
}

/**
	@todo Переделать очередь пакетов, выстроив их в порядке работы таймеров.
*/
void crow_onestep()
{
	crowket_t* pack;
	crowket_t* n;

	while (1)
	{
		system_lock();
		if (dlist_empty(&crow_travelled))
		{
			break;
		}
		pack = dlist_first_entry(&crow_travelled, crowket_t, lnk);
		dlist_del_init(&pack->lnk);
		system_unlock();
		crow_do_travel(pack);
	}

	uint16_t curtime = crow_millis();
	//igris::for_each_safe(crow_outters.begin(), crow_outters.end(), [&](crowket_t& pack) {
	dlist_for_each_entry_safe(pack, n, &crow_outters, lnk)
	{
		if (curtime - pack->last_request_time > pack->header.ackquant)
		{
			dlist_del_init(&pack->lnk);
			if (++pack->ackcount == 5)
			{
				if (crow_undelivered_handler) crow_undelivered_handler(pack);
				else crow_utilize(pack);
			}
			else
			{
				crow_travel(pack);
			}
		}
	}

	dlist_for_each_entry_safe(pack, n, &crow_incoming, lnk)
	{
		if (curtime - pack->last_request_time > pack->header.ackquant)
		{
			if (++pack->ackcount == 5)
			{
				crow_utilize(pack);
			}
			else
			{
				pack->last_request_time = curtime;
				dprln("crow_resend_ack");
				crow_send_ack(pack);
			}
		}
	}
	system_unlock();
}

void crow_spin()
{
	while (1)
	{
		crow_gw_t* gate;
		dlist_for_each_entry(gate, &crow_gateways, lnk)
		{
			gate->ops->nblock_onestep(gate);
		}
		crow_onestep();
	}
}

/*crow_host::host(const char* addr) {
	int prelen = strlen(addr);
	data = (uint8_t*)malloc(prelen);
	int len = hexer(data, prelen, addr, prelen);
	size = len;
	data = (uint8_t*)realloc(data, len);
}

crow_host::host(const uint8_t* addr, size_t size) {
	this->data = (uint8_t*)malloc(size);
	this->size = size;
	memcpy(data, addr, size);
}

crow_host::host(const host& oth) {
	this->data = (uint8_t*)malloc(oth.size);
	this->size = oth.size;
	memcpy(data, oth.data, size);
}

crow_host::~host() {
	free(data);
}*/

/*void crow_print_dump(crowket_t* pack) {
	igris::println(igris::dstring(&pack->header, pack->header.flen));
} */