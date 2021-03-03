#ifndef CROW_TOWER_H
#define CROW_TOWER_H

#include <crow/gateway.h>
#include <crow/packet_ptr.h>
#include <igris/container/dlist.h>

#define CROW_SENDED 0
#define CROW_WRONG_ADDRESS -1

namespace crow
{
	extern uint16_t debug_data_size;

	extern dlist_head protocols;
	extern bool diagnostic_noack;

	extern void (*user_incoming_handler)(crow::packet *pack);
	//extern void (*pubsub_handler)(crow::packet *pack);
	//extern void (*node_handler)(crow::packet *pack);
	extern void (*user_type_handler)(crow::packet *pack);
//	extern void (*query_tower_handler)(crow::packet *pack);
//	extern void (*netkeep_tower_handler)(crow::packet *pack);
	extern void (*undelivered_handler)(crow::packet *pack);

	extern void (*unsleep_handler)();

	// Передать пакет в обработку.
	packet_ptr travel(crow::packet *pack);
	void nocontrol_travel(crow::packet *pack);

	// Включить трассировку пакетов.
	void diagnostic_enable();
	void enable_diagnostic();

	// Включить трассировку аллокации.
	void live_diagnostic_enable();
	void enable_live_diagnostic();

	void diagnostic_setup(bool en, bool len = false);
	
	// Отправить пакет.
	crow::packet_ptr send(const crow::hostaddr & addr,
	                      igris::buffer data,
	                      uint8_t type,
	                      uint8_t qos,
	                      uint16_t ackquant,
	                      bool fastsend=false);

	crow::packet_ptr send_v(const crow::hostaddr & addr,
	                        const igris::buffer* vec,
	                        size_t veclen,
	                        uint8_t type,
	                        uint8_t qos,
	                        uint16_t ackquant,
	                        bool fastsend=false);

	crow::packet_ptr send_vv(const crow::hostaddr & addr,
	                         const igris::buffer* vec, size_t veclen,
	                         const igris::buffer* vec2, size_t veclen2,
	                         uint8_t type,
	                         uint8_t qos,
	                         uint16_t ackquant,
	                         bool fastsend=false);

	// Эта функция вызывается вратами после обработки отсылаемого пакета.
	void return_to_tower(crow::packet *pack, uint8_t sts);

	// Используется пользовательским кодом для освобождения пакета.
	void release(crow::packet *pack);

	// Взятие отметки времени.
	uint16_t millis();

	void onestep();
	void onestep_travel_only();
	
	void spin();
	void spin_with_select();
	void spin_with_select_realtime();

	int stop_spin();
	int start_spin_with_select();
	int start_spin_with_select_realtime();
	int start_spin_without_select();

	int start_spin();
	int start_spin_realtime();

	[[deprecated]]
	void spin_join();
	void join_spin();

	bool has_untravelled();
	bool has_untravelled_now();
	void print_list_counts();

	bool fully_empty();

	int64_t get_minimal_timeout();

	// Завершить гейты.
	void finish();
} // namespace crow

#endif
