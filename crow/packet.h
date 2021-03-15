/**
@file
@brief Всё, что касается работы с пакетом.
*/

#ifndef CROW_PACKAGE_H
#define CROW_PACKAGE_H

#include <stdint.h>
#include <stdlib.h>

#include <crow/hostaddr.h>
#include <crow/defs.h>

#include <igris/datastruct/dlist.h>
#include <igris/container/pool.h>
#include <igris/buffer.h>
#include <igris/sync/syslock.h>

/// Качество обслуживания.
#define CROW_WITHOUT_ACK 0
#define CROW_TARGET_ACK 1
#define CROW_BINARY_ACK 2

namespace crow
{
	struct packet;
	void __crow_utilize(packet * pack);

	struct gateway;
	class packet_ptr;

	/**
		@brief Структура заголовок пакета.
		@details Заголовок пакета располагается в первых байтах пакета.
		за заголовком следует поле адреса переменной длины, а за ним данные.
	*/
	struct header
	{
		union
		{
			uint8_t pflag; ///< Флаги пакета
			struct
			{
				uint8_t ack : 1; ///< Идентифицирует ack пакеты. Доп.инф.
				///< передается в типе.
				uint8_t RESERVED : 1;
				uint8_t noexec : 1; ///< Флаг предотвращает исполнение пакета.
				///< Используется для запросов существования
				uint8_t type : 5; ///< Доп. инф. зависит от ситуации.
			} f;
		};
		uint16_t flen; ///< Полная длина пакета
		uint8_t alen;  ///< Длина поля адреса.
		uint8_t stg; ///< Поле стадии. Используется для того, чтобы цепочка врат
		///< знала, какую часть адреса обрабатывать.
		uint16_t ackquant; ///< Таймаут для пересылки пакета.
		uint16_t seqid; ///< Порядковый номер пакета. Присваивается отправителем.
		uint8_t qos; ///< Поле качества обслуживания.
	} __attribute__((packed));

	struct packet
	{
	private:
		struct dlist_head lnk = DLIST_HEAD_INIT(lnk);; ///< Для подключения в списки башни crow.
		struct dlist_head ulnk = DLIST_HEAD_INIT(ulnk); ///< Для подключения в список пользователя и
		int8_t refs = 0;
		///< зависимых протоколов.
	public:
		struct crow::gateway *ingate; ///< gate, которым пакет прибыл в систему.
		uint16_t last_request_time; ///< время последней отправки
		uint16_t _ackcount;			///< счетчик количества попыток отправки
		union
		{
			uint8_t flags; ///< Местные флаги
			struct
			{
				uint8_t confirmed : 1;
				uint8_t undelivered : 1;
				uint8_t sended_to_gate : 1;
			} f;
		};
		struct crow::header header;

		void link_to_system_list(dlist_head * lst) { dlist_add_tail(&lnk, lst); up_ref(); }
		void link_to_user_list(dlist_head * ulst) { dlist_add_tail(&ulnk, ulst); up_ref(); }

		void unlink_from_system_list()
		{
			igris::syslock_guard lock;
			if (dlist_is_linked(&lnk))
			{
				dlist_del_init(&lnk);
				down_ref();
			}
		}

		void unlink_from_user_list()
		{
			igris::syslock_guard lock;
			if (dlist_is_linked(&ulnk))
			{
				dlist_del_init(&ulnk);
				down_ref();
			}
		}

		void up_ref()
		{
			igris::syslock_guard lock;

			refs++;
		}

		void down_ref()
		{
			igris::syslock_guard lock;

			refs--;
			assert( refs >= 0 );

			if (refs == 0)
				self_destruct();
		}

		void init()
		{
			dlist_init(&pack->lnk);
			dlist_init(&pack->ulnk);
			pack->ingate = ingate;
			pack->ackcount(5);
			pack->flags = 0;
			pack->refs = 0;
		}

		// change to packet_ptr
		static packet_ptr first_user_list_entry(dlist_head * head);

		// change to packet_ptr
		static packet_ptr first_system_list_entry(dlist_head * head);

		void self_destruct()
		{
			igris::syslock_guard lock;
			__crow_utilize(this);
		}

		uint8_t *stageptr() { return (uint8_t *)(&header + 1) + header.stg; }

		char *endptr() { return (char *)(&header) + header.flen; }
		uint16_t blocksize() { return header.flen; }

		igris::buffer rawdata() { return igris::buffer(dataptr(), datasize()); }
		igris::buffer addr() { return igris::buffer(addrptr(), addrsize()); }

		void revert_gate(uint8_t gateindex);
		void revert(igris::buffer * vec, size_t veclen);

		uint8_t *addrptr() { return (uint8_t *)(&header + 1); }
		uint8_t addrsize() { return header.alen; }

		char *dataptr() { return (char*)(addrptr() + addrsize()); }
		uint16_t datasize()
		{
			return (uint16_t)(header.flen - header.alen - sizeof(crow::header));
		}

		size_t fullsize()
		{
			return header.flen;
		};

		void type(uint8_t t) { header.f.type = t; }
		void qos(uint8_t q) { header.qos = q; }
		void ackquant(uint16_t a) { header.ackquant = a; }

		uint8_t type() { return header.f.type; }
		uint8_t qos() { return header.qos; }
		uint16_t ackquant() { return header.ackquant; }

		void ackcount(uint16_t c) { _ackcount = c; }
		uint16_t ackcount() { return _ackcount; }
	}; //На самом деле, он не должен быть packed.
	//__attribute__((packed));

	/**
	 * Выделить память для пакета.
	 *
	 * Выделяет adlen + sizeof(crow::packet) байт
	 * @param adlen Суммарная длина адреса и данных в выделяемом пакете.
	 */
	crow::packet *allocate_packet(size_t adlen);

	///Вернуть память выделенную для пакета pack
	void deallocate_packet(crow::packet *pack);

	packet *create_packet(struct crow::gateway *ingate, uint8_t addrsize,
	                      size_t datasize);

	void packet_initialization(struct crow::packet *pack,
	                           struct crow::gateway *ingate);

	// Только для аллокации через pool.
	void engage_packet_pool(void *zone, size_t zonesize, size_t elsize);
	igris::pool * get_package_pool();

	extern int allocated_count;
	bool has_allocated();

	void diagnostic(const char* label, crow::packet_ptr pack);
}

#endif
