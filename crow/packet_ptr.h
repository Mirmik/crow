#ifndef CROW_PACKET_PTR_H
#define CROW_PACKET_PTR_H

#include <crow/packet.h>

namespace crow
{
	class packet_ptr
	{
	protected:
		crow::packet *pack;

	public:
		packet_ptr() : pack(nullptr) {}

		packet_ptr(crow::packet *pack_) : pack(pack_)
		{
			if (pack == nullptr) return;

			pack->up_ref();
		}

		packet_ptr(const crow::packet_ptr &oth) : pack(oth.pack)
		{
			pack->up_ref();
		}

		packet_ptr(crow::packet_ptr &&oth) : pack(oth.pack)
		{
			oth.pack = nullptr;
		}

		packet_ptr & operator = (const crow::packet_ptr &oth) 
		{
			if (pack) { pack->down_ref(); }

			pack = oth.pack;
			pack->up_ref();		
			return *this;
		}

		packet_ptr & operator = (crow::packet_ptr &&oth) 
		{
			if (pack) { pack->down_ref(); }

			pack = oth.pack;
			oth.pack = nullptr;
			return *this;
		}

		crow::packet* get()
		{
			return pack;
		}

		crow::packet* get() const
		{
			return pack;
		}

		crow::packet* operator ->() 
		{
			return pack;
		}

		crow::packet& operator *() 
		{
			return *pack;
		}

		~packet_ptr() 
		{
			if (pack)
				pack->down_ref();
		}

		operator bool() { return pack != nullptr; }

		bool operator == (std::nullptr_t) 
		{
			return pack == nullptr;
		}
	};
}

#endif
