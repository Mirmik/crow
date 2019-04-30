/**
@file
@brief tower file
*/

#ifndef CROW_TOWER_H
#define CROW_TOWER_H

#include <crow/gateway.h>
#include <sys/uio.h>

#include <igris/dprint.h>

#define CROW_SENDED 0
#define CROW_WRONG_ADDRESS -1

///Список врат.
extern struct dlist_head crow_gateways;

__BEGIN_DECLS

///Переместить пакет дальше по конвееру врат.
void crow_travel_error(crowket_t* pack); 
void crow_travel(crowket_t* pack); 
void crow_do_travel(crowket_t* pack); 
	
void crow_transport(crowket_t* pack); 
void crow_send(const void* addr, uint8_t asize, const char* data, uint16_t dsize, uint8_t type, uint8_t qos, uint16_t ackquant);
void crow_send_v(const void* addr, uint8_t asize, const struct iovec* vec, size_t veclen, uint8_t type, uint8_t qos, uint16_t ackquant);
	
///Вызывается на только что отправленный пакет. Башня или уничтожает его, или кеширует для контроля качества.
void crow_return_to_tower(crowket_t* pack, uint8_t sts);

///Подключить врата к башне.
static inline void crow_link_gate(struct crow_gw* gate, uint8_t id) { 
	gate->id = id; 
	dlist_add_tail(&gate->lnk, &crow_gateways); 
} 


struct crow_gw* crow_find_target_gateway(crowket_t* pack);

void crow_release(crowket_t* pack);
void crow_tower_release(crowket_t* pack);

void crow_revert_address(crowket_t* pack);
void crow_send_ack(crowket_t* pack);
void crow_send_ack2(crowket_t* pack);

extern void (*crow_user_type_handler)(crowket_t* pack);
extern void (*crow_user_incoming_handler)(crowket_t* pack);
extern void (*crow_traveling_handler)(crowket_t* pack);
extern void (*crow_transit_handler)(crowket_t* pack);

/// Обработчик недоставленного пакета. Определяется локальным софтом.
/// Освобождение должно производиться функцией tower_release.
extern void(*crow_undelivered_handler)(crowket_t* pack);
extern void(*crow_pubsub_handler)(crowket_t* pack);

uint16_t crow_millis();

void crow_onestep();
void crow_onestep_travel_only();
void crow_spin();

void crow_incoming_handler(crowket_t* pack);
void crow_incoming_node_packet(crowket_t* pack);
void crow_incoming_pubsub_packet(crowket_t* pack);
//void incoming_channel_packet(crowket_t* pack);

void crow_enable_diagnostic();

void crow_print(crowket_t* pack);
void crow_println(crowket_t* pack);

__END_DECLS

#endif