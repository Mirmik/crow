#include <assert.h>
#include <string.h>
#include <stdbool.h>

#include <igris/sync/syslock.h>
#include <igris/math/defs.h>

#include <crow/tower.h>
#include <crow/print.h>
#include <crow/gateway.h>
#include <crow/proto/protocol.h>
#include <crow/warn.h>


bool crow::diagnostic_noack = false;
uint16_t crow::debug_data_size = 28;

struct dlist_head crow::protocols = DLIST_HEAD_INIT(crow::protocols);
DLIST_HEAD(crow_travelled);
DLIST_HEAD(crow_incoming);
DLIST_HEAD(crow_outters);

void (*crow::unsleep_handler)() = nullptr;
void (*crow::user_type_handler)(crow::packet_ptr pack) = nullptr;
void (*crow::user_incoming_handler)(crow::packet_ptr pack) = nullptr;
void (*crow::undelivered_handler)(crow::packet_ptr pack) = nullptr;

static bool __diagnostic_enabled = false;
bool __live_diagnostic_enabled = false;

bool _in_incoming_handler = false;
bool _in_undelivered_handler = false;

void crow::enable_diagnostic()
{
	__diagnostic_enabled = true;
}

void crow::enable_live_diagnostic()
{
	__live_diagnostic_enabled = true;
}

void crow::diagnostic_setup(bool en, bool len)
{
	__diagnostic_enabled = en;
	__live_diagnostic_enabled = len;
}


static void __crow_utilize(crow::packet_ptr pack)
{
	if (__live_diagnostic_enabled)
		crow::diagnostic("utilz", pack);

	pack->unlink_from_system_list(); // Очищается в tower_release((см. tower.c))
	pack->unlink_from_user_list();
	crow::deallocate_packet(pack);
}

static crow::gateway *crow_find_target_gateway(crow::packet_ptr pack)
{
	uint8_t gidx = *pack->stageptr();

	for (auto& gate : crow::gateway_list)
	{
		if (gate.id == gidx)
			return &gate;
	}

	return NULL;
}

void crow::utilize(crow::packet * pack)
{
	system_lock();
	__crow_utilize(pack);	
	system_unlock();
}

static void crow_tower_release(crow::packet_ptr pack)
{
	//Скорее всего это поле должно освобождаться без инициализации.
	//Инициализируем для последуещего освобождения в utilize
	pack->unlink_from_system_list();
}

static void confirmed_utilize_from_outers(crow::packet_ptr pack)
{
	crow::packet_ptr it;
	dlist_for_each_entry(it, &crow_outters, lnk)
	{
		if (it->header.seqid == pack->header.seqid &&
		        pack->header.alen == it->header.alen &&
		        !memcmp(it->addrptr(), pack->addrptr(), pack->header.alen))
		{
			it->f.confirmed = 1;
			crow_tower_release(it);
			return;
		}
	}
}

static void qos_release_from_incoming(crow::packet_ptr pack)
{
	crow::packet_ptr it;
	dlist_for_each_entry(it, &crow_incoming, lnk)
	{
		if (it->header.seqid == pack->header.seqid &&
		        pack->header.alen == it->header.alen &&
		        !memcmp(it->addrptr(), pack->addrptr(), pack->header.alen))
		{
			crow_tower_release(it);
			return;
		}
	}
}

bool crow_time_comparator(crow::packet* a, crow::packet* b)
{
	uint64_t a_timer = a->last_request_time + a->header.ackquant;
	uint64_t b_timer = b->last_request_time + b->header.ackquant;

	return a_timer < b_timer;
}

static void add_to_incoming_list(crow::packet_ptr pack)
{
	pack->last_request_time = crow::millis();
	dlist_move_sorted(pack, &crow_incoming, lnk, crow_time_comparator);
}

static void add_to_outters_list(crow::packet_ptr pack)
{
	pack->last_request_time = crow::millis();
	dlist_move_sorted(pack, &crow_outters, lnk, crow_time_comparator);
}

crow::packet_ptr crow::travel(crow::packet_ptr pack)
{
	system_lock();
	dlist_add_tail(&pack->lnk, &crow_travelled);
	system_unlock();

	if (unsleep_handler)
		unsleep_handler();

	return crow::packet_ptr(pack);
}

void crow::nocontrol_travel(crow::packet_ptr pack)
{
	system_lock();
	dlist_add_tail(&pack->lnk, &crow_travelled);
	system_unlock();
}

static void crow_travel_error(crow::packet_ptr pack)
{
	system_lock();
	__crow_utilize(pack);
	system_unlock();
}

static void crow_incoming_handler(crow::packet_ptr pack)
{
	crow::protocol * it;

	if (crow::user_incoming_handler)
	{
		_in_incoming_handler = true;
		crow::user_incoming_handler(pack);
		_in_incoming_handler = false;
	}

	dlist_for_each_entry(it, &crow::protocols, lnk)
	{
		if (it->id == pack->header.f.type)
		{
			_in_incoming_handler = true;
			it->incoming(pack);
			_in_incoming_handler = false;
			return;
		}
	}

	if (crow::user_type_handler)
	{
		_in_incoming_handler = true;
		crow::user_type_handler(pack);
		_in_incoming_handler = false;
	}
	else
	{
		if (__diagnostic_enabled)
		{
			crow::diagnostic("wproto", pack);
		}
		crow::release(pack);
	}

}

static void crow_send_ack(crow::packet_ptr pack)
{
	crow::packet_ptr ack = crow::create_packet(NULL, pack->header.alen, 0);

	assert(pack);
	assert(ack);

	ack->header.f.type =
	    pack->header.qos == CROW_BINARY_ACK ? G1_ACK21_TYPE : G1_ACK_TYPE;
	ack->header.f.ack = 1;
	ack->header.qos = CROW_WITHOUT_ACK;
	ack->header.ackquant = pack->header.ackquant;
	ack->header.seqid = pack->header.seqid;
	memcpy(ack->addrptr(), pack->addrptr(), pack->header.alen);
	ack->f.released_by_world = true;
	crow::travel(ack);
}

static void crow_send_ack2(crow::packet_ptr pack)
{
	crow::packet_ptr ack = crow::create_packet(NULL, pack->header.alen, 0);

	assert(pack);
	assert(ack);

	ack->header.f.type = G1_ACK22_TYPE;
	ack->header.f.ack = 1;
	ack->header.qos = CROW_WITHOUT_ACK;
	ack->header.ackquant = pack->header.ackquant;
	ack->header.seqid = pack->header.seqid;
	memcpy(ack->addrptr(), pack->addrptr(), pack->header.alen);
	crow::travel(ack);
}

static void crow_revert_address(crow::packet_ptr pack)
{
	uint8_t *first = pack->addrptr();
	uint8_t *last = pack->addrptr() + pack->header.alen;

	while ((first != last) && (first != --last))
	{
		char tmp = *last;
		*last = *first;
		*first++ = tmp;
	}
}

static void crow_do_travel(crow::packet_ptr pack)
{
	if (pack->header.stg == pack->header.alen)
	{
		//Ветка доставленного пакета.
		crow_revert_address(pack);

		if (pack->header.f.ack)
		{
			//Перехватываем ack пакеты.
			if (__diagnostic_enabled && crow::diagnostic_noack == false)
				crow::diagnostic("inack", pack);

			switch (pack->header.f.type)
			{
				case G1_ACK_TYPE:
					confirmed_utilize_from_outers(pack);
					break;

				case G1_ACK21_TYPE:
					confirmed_utilize_from_outers(pack);
					crow_send_ack2(pack);
					break;

			case G1_ACK22_TYPE:
				qos_release_from_incoming(pack);
				break;

			default:
				break;
			}

			system_lock();
			__crow_utilize(pack);
			system_unlock();

			return;
		}

		if (__diagnostic_enabled)
			crow::diagnostic("incom", pack);

		if (pack->ingate)
		{
			//Если пакет пришел извне, используем логику обеспечения качества.

			//Для пакетов с подтверждение посылаем ack первого или второго
			//типов.
			if (pack->header.qos == CROW_TARGET_ACK ||
			        pack->header.qos == CROW_BINARY_ACK)
				crow_send_ack(pack);

			if (pack->header.qos == CROW_BINARY_ACK)
			{
				//Перед тем как добавить пакет в обработку, проверяем,
				//нет ли его в списке принятых.
				crow::packet_ptr inc;
				dlist_for_each_entry(inc, &crow_incoming, lnk)
				{
					if (inc->header.seqid == pack->header.seqid &&
					        inc->header.alen == pack->header.alen &&
					        memcmp(inc->addrptr(), pack->addrptr(),
					               inc->header.alen) == 0)
					{
						system_lock();
						__crow_utilize(pack);
						system_unlock();

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
		else
		{
			//Если пакет отправлен из данного нода, обслуживание не требуется
			crow_tower_release(pack);
		}

		//Решаем, что делать с пришедшим пакетом.
		if (!pack->header.f.noexec)
			crow_incoming_handler(pack);
		else 
		{
			//crow::release(pack);
		}

		return;
	}
	else
	{

		//Ветка транзитного пакета. Логика поиска врат и пересылки.
		crow::gateway *gate = crow_find_target_gateway(pack);

		if (gate == NULL)
		{
			if (__diagnostic_enabled)
				crow::diagnostic("wgate", pack);

			crow_travel_error(pack);
		}
		else
		{
			if (pack->header.f.ack)
			{
				if (__diagnostic_enabled && crow::diagnostic_noack == false)
					crow::diagnostic("track", pack);
			}
			else
			{
				if (__diagnostic_enabled)
					crow::diagnostic("trans", pack);
			}
			//Здесь пакет штампуется временем отправки и пересылается во врата.
			//Врата должны после пересылки отправить его назад в башню
			//с помощью return_to_tower для контроля качества.
			assert(pack->f.sended_to_gate == 0);
			pack->f.sended_to_gate = 1;
			gate->send(pack);
		}
	}
}

uint16_t __seqcounter = 0;
crow::packet_ptr crow_transport(crow::packet_ptr pack, bool fastsend = false);
crow::packet_ptr crow_transport(crow::packet_ptr pack, bool fastsend)
{
	pack->header.stg = 0;
	pack->header.f.ack = 0;
	system_lock();
	pack->header.seqid = __seqcounter++;
	system_unlock();

	if (fastsend == false)
		return crow::travel(pack);
	else
	{
		crow::packet_ptr ptr(pack);
		crow_do_travel(pack);
		return ptr;
	}
}

crow::packet_ptr crow::send(const crow::hostaddr_view & addr,
                            const igris::buffer data,
                            uint8_t type,
                            uint8_t qos,
                            uint16_t ackquant,
                            bool fastsend)
{
	crow::packet_ptr pack = crow::create_packet(NULL, addr.size(), data.size());
	if (pack == nullptr) 
	{
		crow::warn("cannot create packet");
		return nullptr;
	}

	pack->header.f.type = type & 0x1F;
	pack->header.qos = qos;
	pack->header.ackquant = ackquant;

	memcpy(pack->addrptr(), addr.data(), addr.size());
	memcpy(pack->dataptr(), data.data(), data.size());

	return crow_transport(pack, fastsend);
}

crow::packet_ptr crow::send_v(const crow::hostaddr_view & addr, const igris::buffer *vec,
                              size_t veclen, uint8_t type, uint8_t qos, uint16_t ackquant,
                              bool fastsend)
{
	size_t dsize = 0;
	const igris::buffer *it = vec;
	const igris::buffer *const eit = vec + veclen;

	for (; it != eit; ++it)
	{
		dsize += it->size();
	}

	crow::packet_ptr pack = crow::create_packet(NULL, addr.size(), dsize);
	if (pack == nullptr)
		return nullptr;

	pack->header.f.type = type & 0x1F;
	pack->header.qos = qos;
	pack->header.ackquant = ackquant;

	memcpy(pack->addrptr(), addr.data(), addr.size());

	it = vec;
	char *dst = pack->dataptr();

	for (; it != eit; ++it)
	{
		memcpy(dst, it->data(), it->size());
		dst += it->size();
	}

	return crow_transport(pack, fastsend);
}

crow::packet_ptr crow::send_vv(const crow::hostaddr_view & addr,
                               const igris::buffer* vec, size_t veclen,
                               const igris::buffer* vec2, size_t veclen2,
                               uint8_t type, uint8_t qos, uint16_t ackquant,
                               bool fastsend)
{
	size_t dsize = 0;
	const igris::buffer *it;
	const igris::buffer *eit;

	it = vec;
	eit = vec + veclen;
	for (; it != eit; ++it)
	{
		dsize += it->size();
	}

	it = vec2;
	eit = vec2 + veclen2;
	for (; it != eit; ++it)
	{
		dsize += it->size();
	}

	crow::packet_ptr pack = crow::create_packet(NULL, addr.size(), dsize);
	if (pack == nullptr)
		return nullptr;

	pack->header.f.type = type & 0x1F;
	pack->header.qos = qos;
	pack->header.ackquant = ackquant;

	memcpy(pack->addrptr(), addr.data(), addr.size());
	char *dst = pack->dataptr();

	it = vec;
	eit = vec + veclen;
	for (; it != eit; ++it)
	{
		memcpy(dst, it->data(), it->size());
		dst += it->size();
	}

	it = vec2;
	eit = vec2 + veclen2;
	for (; it != eit; ++it)
	{
		memcpy(dst, it->data(), it->size());
		dst += it->size();
	}

	return crow_transport(pack, fastsend);
}

void crow::return_to_tower(crow::packet_ptr pack, uint8_t sts)
{
	pack->f.sended_to_gate = 0;
	assert(pack);

	system_lock();

	if (pack->ingate != NULL)
	{
		//Пакет был отправлен, и он нездешний. Уничтожить.
		__crow_utilize(pack);
	}
	else
	{
		//Пакет здешний.
		if (sts != CROW_SENDED || pack->header.qos == CROW_WITHOUT_ACK)
			__crow_utilize(pack);
		else
			add_to_outters_list(pack);
	}

	system_unlock();
}

void crow::onestep_travel_only()
{
	system_lock();

	while (1)
	{
		bool empty = dlist_empty(&crow_travelled);

		if (empty)
			break;

		crow::packet_ptr pack =
		    dlist_first_entry(&crow_travelled, crow::packet, lnk);
		dlist_del_init(&pack->lnk);

		crow_do_travel(pack);
	}

	system_unlock();
}

void crow_undelivered(crow::packet* pack)
{
	pack->f.undelivered = 1;
	crow::protocol * it;
	dlist_for_each_entry(it, &crow::protocols, lnk)
	{
		if (it->id == pack->header.f.type)
		{
			_in_undelivered_handler = true; 
			it->undelivered(pack);
			_in_undelivered_handler = false;
			return;
		}
	}

	if (crow::undelivered_handler) 
	{
		_in_undelivered_handler = true;
		crow::undelivered_handler(pack);
		_in_undelivered_handler = false;
	}
	else
		crow_tower_release(pack);
}

static inline void crow_onestep_send_stage()
{
	crow::packet_ptr pack;

	system_lock();

	while (1)
	{

		if (dlist_empty(&crow_travelled))
		{
			break;
		}

		pack = dlist_first_entry(&crow_travelled, crow::packet, lnk);
		dlist_del_init(&pack->lnk);

		system_unlock();
		crow_do_travel(pack);

		system_lock();
	}
	system_unlock();
}

static inline void crow_onestep_outers_stage()
{
	crow::packet_ptr pack;
	crow::packet_ptr n;

	uint16_t curtime = crow::millis();

	system_lock();
	if (dlist_empty(&crow_outters))
	{
		system_unlock();
		return;
	}

	dlist_for_each_entry_safe(pack, n, &crow_outters, lnk)
	{
		assert(pack->f.released_by_tower == 0);

		if (curtime - pack->last_request_time >= pack->header.ackquant)
		{
			dlist_del_init(&pack->lnk);

			if (pack->_ackcount != 0xFFFF)
				--pack->_ackcount;

			if (pack->_ackcount == 0)
			{
				crow_tower_release(pack);
				crow_undelivered(pack);
			}
			else
			{
				system_unlock();
				crow::travel(pack);
				system_lock();
			}
		}
	}
	system_unlock();
}

static inline void crow_onestep_incoming_stage()
{
	crow::packet_ptr pack;
	crow::packet_ptr n;

	uint16_t curtime = crow::millis();

	system_lock();
	if (dlist_empty(&crow_incoming))
	{
		system_unlock();
		return;
	}

	dlist_for_each_entry_safe(pack, n, &crow_incoming, lnk)
	{
		assert(pack->f.released_by_tower == 0);

		if (curtime - pack->last_request_time >= pack->header.ackquant)
		{
			dlist_del_init(&pack->lnk);

			if (pack->_ackcount != 0xFFFF)
				--pack->_ackcount;

			if (pack->_ackcount == 0)
			{
				pack->f.undelivered = 1;
				__crow_utilize(pack);
			}
			else
			{
				dlist_move_sorted(pack, &crow_incoming, lnk, crow_time_comparator);
				pack->last_request_time = curtime;
				system_unlock();
				crow_send_ack(pack);
				system_lock();
			}
		}
	}
	system_unlock();
}

void crow::onestep()
{
	for (crow::gateway &gate : crow::gateway_list)
	{
		gate.nblock_onestep();
	}

	crow::protocol * it;
	dlist_for_each_entry(it, &crow::protocols, lnk)
	{
		it->onestep();
	}

	crow_onestep_send_stage();
	crow_onestep_outers_stage();
	crow_onestep_incoming_stage();
}

void crow::spin()
{
	while (1)
	{
		crow::onestep();
	}
}

bool crow::has_untravelled()
{
	igris::syslock_guard lock;

	return ! (
	           dlist_empty(&crow_travelled) &&
	           dlist_empty(&crow_outters) &&
	           dlist_empty(&crow_incoming)
	       );
}

bool crow::has_untravelled_now()
{
	igris::syslock_guard lock;
	return !dlist_empty(&crow_travelled);
}

void crow::finish()
{
	for (crow::gateway &gate : crow::gateway_list)
	{
		gate.finish();
	}
}

bool crow::fully_empty()
{
	system_lock();
	int ret =
		dlist_empty(&crow_travelled) &&
		dlist_empty(&crow_incoming) &&
		dlist_empty(&crow_outters);
	system_unlock();
	return ret;
}

int64_t crow::get_minimal_timeout()
{
	int64_t result;
	int64_t i_finish;
	int64_t o_finish;
	int64_t curtime = crow::millis();

	if (!dlist_empty(&crow_incoming))
	{
		crow::packet_ptr  i = dlist_first_entry(&crow_incoming, crow::packet, lnk);
		i_finish = i->last_request_time + i->header.ackquant;
	}
	else
	{
		i_finish = -1;
	}

	if (!dlist_empty(&crow_outters))
	{
		crow::packet_ptr  o = dlist_first_entry(&crow_outters, crow::packet, lnk);
		o_finish = o->last_request_time + o->header.ackquant;
	}
	else
	{
		o_finish = -1;
	}

	if (i_finish > 0 && o_finish > 0)
		result = __MIN__(i_finish, o_finish) - curtime;

	else if (o_finish > 0)
		result = o_finish - curtime;

	else if (i_finish > 0)
		result = i_finish - curtime;

	else
		return -1;

	if (result < 0)
		return 0;

	return result;
}
