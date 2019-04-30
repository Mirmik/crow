/**
@file 
@brief Всё, что касается работы с пакетом.
*/

#ifndef CROW_PACKAGE_H
#define CROW_PACKAGE_H

#include <stdint.h>
#include <stdlib.h>
#include <sys/uio.h>

#include <igris/datastruct/dlist.h>
#include <crow/defs.h>

struct crow_gw;

/// Качество обслуживания.
#define CROW_WITHOUT_ACK 0
#define	CROW_TARGET_ACK 1
#define CROW_BINARY_ACK 2

/**
	@brief Структура заголовок пакета. 
	@details Заголовок пакета располагается в первых байтах пакета.
	за заголовком следует поле адреса переменной длины, а за ним данные.
*/
typedef struct crow_header {
	union {
		uint8_t pflag; ///< Флаги пакета
		struct {
			uint8_t ack : 1; ///< Идентифицирует ack пакеты. Доп.инф. передается в типе.
			uint8_t vaddr : 1; ///< Поле указатель виртуального адреса @todo
			uint8_t noexec : 1; ///< Флаг предотвращает исполнение пакета. Используется для запросов существования
			uint8_t type : 5; ///< Доп. инф. зависит от ситуации.
		} f;
	};
	uint16_t 	flen; ///< Полная длина пакета
	uint8_t 	alen; ///< Длина поля адреса.
	uint8_t 	stg; ///< Поля стадии. Используется для того, чтобы цепочка врат знала, какую часть адреса обрабатывать.
	uint16_t 	ackquant; ///< Таймаут для пересылки пакета.
	uint16_t 	seqid; ///< Порядковый номер пакета. Присваивается отправителем.
	uint8_t 	qos; ///< Поле качества обслуживания.
} G1_PACKED crow_header_t;

typedef struct crowket {
	struct dlist_head 	lnk; ///< Для подключения в списки башни crow.
	struct dlist_head 	ulnk; ///< Для подключения в список пользователя и зависимых протоколов.
	struct crow_gw* 	ingate; ///< gate, которым пакет прибыл в систему.
	uint16_t 			last_request_time; ///< @todo
	uint8_t 			ackcount; ///< @todo
	uint8_t 			status;
	union {
		uint8_t flags; ///< Местные флаги
		struct {
			uint8_t released_by_world : 1;
			uint8_t released_by_tower : 1;
		} f;
	};
	struct crow_header header;
} G1_PACKED crowket_t;


__BEGIN_DECLS

static inline uint8_t* crowket_addrptr(struct crowket* pack) { 
	return (uint8_t*)(&pack->header + 1); 
}

static inline char* crowket_dataptr(struct crowket* pack) { 
	return (char*)(&pack->header + 1) + pack->header.alen; 
}

static inline uint8_t* crowket_stageptr(struct crowket* pack) { 
	return (uint8_t*)(&pack->header + 1) + pack->header.stg; 
}

static inline char* crowket_endptr(struct crowket* pack) { 
	return (char*)(&pack->header) + pack->header.flen; 
}

static inline size_t crowket_addrsize(struct crowket* pack) { 
	return pack->header.alen; 
}

static inline size_t crowket_blocksize(struct crowket* pack) { 
	return pack->header.flen; 
}

static inline size_t crowket_datasize(struct crowket* pack) { 
	return pack->header.flen - pack->header.alen - sizeof(crow_header_t); 
}

void crowket_revert_g(struct crowket* pack, uint8_t gateindex);
void crowket_revert(struct crowket* pack, struct iovec* vec, size_t veclen);

///
void crowket_initialization(struct crowket* pack, struct crow_gw* ingate); 

/**
 * Выделить память для пакета.
 * 
 * Выделяет adlen + sizeof(crowket_t) байт
 * @param adlen Суммарная длина адреса и данных в выделяемом пакете. 
 */ 
crowket_t* crow_allocate_packet(size_t adlen); 

///Вернуть память выделенную для пакета pack
void crow_deallocate_packet(crowket_t* pack);

///
crowket_t* crow_create_packet(struct crow_gw* ingate, size_t addrsize, size_t datasize); 

///
void crow_utilize(crowket_t* pack);

__END_DECLS

#endif