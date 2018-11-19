/**
	@file
*/

#ifndef CROW_PUBSUB_H
#define CROW_PUBSUB_H

#include <crow/tower.h>
#include <crow/host.h>

#include <assert.h>

struct crow_theme;

#define SUBSCRIBE 	0
#define PUBLISH 	1
#define MESSAGE 	2

typedef struct crow_subheader_pubsub {
	uint8_t type;
	uint8_t thmsz;
} G1_PACKED crow_subheader_pubsub_t;

typedef struct crow_subheader_pubsub_data {
	uint8_t datsz;
} G1_PACKED crow_subheader_pubsub_data_t;

typedef struct crow_subheader_pubsub_control {
	uint8_t qos;
	uint16_t ackquant;
} G1_PACKED crow_subheader_pubsub_control_t;

__BEGIN_DECLS

static inline crow_subheader_pubsub_t* get_subheader_pubsub(crow::packet* pack) {
	return (crow_subheader_pubsub_t*) pack->dataptr();
}

static inline crow_subheader_pubsub_data_t* get_subheader_pubsub_data(crow::packet* pack) {
	return (crow_subheader_pubsub_data_t*) (pack->dataptr() + sizeof(crow_subheader_pubsub_t));
}

static inline crow_subheader_pubsub_control_t* get_subheader_pubsub_control(crow::packet* pack) {
	return (crow_subheader_pubsub_control_t*) (pack->dataptr() + sizeof(crow_subheader_pubsub_t));
}

static inline char* crow::packet_pubsub_thmptr(struct crow::packet* pack) 
{
	return pack->dataptr() + sizeof(crow_subheader_pubsub_t) + sizeof(crow_subheader_pubsub_data_t);
}

static inline char* crow::packet_pubsub_datptr(struct crow::packet* pack) 
{
	return crow::packet_pubsub_thmptr(pack) + get_subheader_pubsub(pack)->thmsz;
}
//gxx::buffer pubsub_message_datasect(crow::packet* pack);

/*static inline gxx::buffer pubsub_theme(crow::packet* pack) {
	crow_subheader_pubsub_t* shps = crow::get_subheader_pubsub(pack);
	crow_subheader_pubsub_t* shps_d = crow::get_subheader_pubsub_data(pack);
	return gxx::buffer(pack->dataptr() + sizeof(crow_subheader_pubsub_t) + sizeof(crow_subheader_pubsub_data_t), shps->thmsz);
}

static inline gxx::buffer pubsub_data(crow::packet* pack) {
	crow_subheader_pubsub_t* shps = crow::get_subheader_pubsub(pack);
	crow_subheader_pubsub_t* shps_d = crow::get_subheader_pubsub_data(pack);
	return gxx::buffer(pack->dataptr() + sizeof(crow_subheader_pubsub_t) + sizeof(crow_subheader_pubsub_data_t) + shps->thmsz, shps_d->datsz);		
}*/

void crow_publish_buffer(const char* theme, const void* data, size_t datsz, uint8_t qos, uint16_t acktime);
void crow_publish(const char* theme, const char* data, uint8_t qos, uint16_t acktime);
//void crow_subscribe(const void* theme, size_t thmsz, uint8_t qos);
void crow_subscribe(const char* theme, uint8_t qos, uint16_t acktime);

void crow_set_publish_host(const uint8_t* hhost, size_t hsize);
//void crow_set_publish_qos(crow::QoS qos);

__END_DECLS

#ifdef __cplusplus

#include <gxx/buffer.h>

namespace crow 
{
	static inline void publish(const char* theme, const char* data, uint8_t qos=0, uint16_t acktime=DEFAULT_ACKQUANT) {
		crow_publish(theme, data, qos, acktime);
	}
	
	static inline void subscribe(const char* theme, uint8_t qos=0, uint16_t acktime=DEFAULT_ACKQUANT) {
		crow_subscribe(theme, qos, acktime);
	}

	namespace pubsub 
	{
		static inline gxx::buffer get_theme(crow::packet* pack) 
		{
			assert(pack->header.f.type == G1_G3TYPE);

			struct crow_subheader_pubsub * shps = get_subheader_pubsub(pack);
			//struct crow_subheader_pubsub_data * shps_d = get_subheader_pubsub_data(pack);

			return gxx::buffer(crow::packet_pubsub_thmptr(pack), shps->thmsz);
		}

		static inline gxx::buffer get_data(crow::packet* pack) 
		{
			assert(pack->header.f.type == G1_G3TYPE);

			struct crow_subheader_pubsub * shps = get_subheader_pubsub(pack);
			struct crow_subheader_pubsub_data * shps_d = get_subheader_pubsub_data(pack);

			return gxx::buffer(crow::packet_pubsub_datptr(pack), shps_d->datsz);
		}
	}
}

#endif //__cplusplus

#endif