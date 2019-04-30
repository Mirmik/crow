/**
	@file
*/

#ifndef G1_GATEWAY_H
#define G1_GATEWAY_H

#include <crow/packet.h>
#include <igris/datastruct/dlist.h>

struct crow_gw_operations;

/**
	@brief Абстрактный класс врат. Врата отвечают за пересылку пакетов между мирами.
	@details Может содержать поле списка и некоторое время хранить отправляемые пакеты.
*/
typedef struct crow_gw {
	struct dlist_head lnk; ///< встроенное поле списка.
	uint16_t id; ///< номер врат.
	const struct crow_gw_operations* ops;
} crow_gw_t;
	
struct crow_gw_operations {
	void(*send)(struct crow_gw*, crowket_t*);
	void(*nblock_onestep)(struct crow_gw*);
};

/*static inline void crow_gw_init(crow_gw_t* gw) {
	dlist_init(&gw->lnk);
}*/

#endif