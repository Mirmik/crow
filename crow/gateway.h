#ifndef CROW_GATEWAY_H
#define CROW_GATEWAY_H

#include <crow/packet.h>
#include <igris/container/dlist.h>
#include <igris/sync/syslock.h>

#include <nos/print.h>

/**
	@brief Абстрактный класс врат. Врата отвечают за пересылку пакетов 
	между башнями.
	@details Может некоторое время хранить отправляемые пакеты.
*/
namespace crow
{
	class gateway
	{
	public:
		struct dlist_head lnk; ///< встроенное поле списка.
		uint8_t id;			   ///< номер врат.

		virtual void send(crow::packet *) = 0;
		virtual void nblock_onestep() = 0;
		virtual void finish() = 0;

		int bind(int gateno);
#if !CROW_ENABLE_WITHOUT_FDS
		virtual int get_fd() { return -1; }
		virtual int fds_total() { return 0; }
		virtual void set_fds_to_array(int *) {}
#endif

		gateway() { dlist_init(&lnk); }

		virtual ~gateway() 
		{
			system_lock();
			dlist_del(&lnk);
			system_unlock();
		}
	};

	gateway * get_gateway(int no);

	extern igris::dlist<crow::gateway, &crow::gateway::lnk> gateway_list;
}

#endif
