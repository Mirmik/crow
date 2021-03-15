#ifndef CROW_NODE_H
#define CROW_NODE_H

#include <crow/types.h>
#include <crow/packet.h>
#include <crow/proto/protocol.h>

#include <igris/container/dlist.h>
#include <igris/binreader.h>
#include <igris/sync/syslock.h>

#define CROW_NODEPACK_COMMON 0
#define CROW_NODEPACK_ERROR 1

#define CROW_NODE_SPECIAL_BUS_ERROR -2

#define CROW_ERRNO_UNREGISTRED_RID 33

namespace crow
{
	crow::packet_ptr node_send(uint16_t sid,
	                           uint16_t rid,
	                           const crow::hostaddr_view & addr,
	                           const igris::buffer data,
	                           uint8_t qos,
	                           uint16_t ackquant,
	                           bool fastsend = false);

	crow::packet_ptr node_send_special(uint16_t sid,
	                                   uint16_t rid,
	                                   const crow::hostaddr_view & addr,
	                                   uint8_t type,
	                                   const igris::buffer data,
	                                   uint8_t qos,
	                                   uint16_t ackquant,
	                                   bool fastsend = false);

	crow::packet_ptr node_send_v(uint16_t sid,
	                             uint16_t rid,
	                             const crow::hostaddr_view & addr,
	                             const igris::buffer * vec,
	                             size_t veclen,
	                             uint8_t qos,
	                             uint16_t ackquant,
	                             bool fastsend = false);

	// TODO: replace with annotation
	struct node_subheader
	{
		uint16_t sid;
		uint16_t rid;
		union
		{
			uint8_t flags = 0;

			struct
			{
				uint8_t reserved : 4;
				uint8_t type : 4;
			};
		};
	} __attribute__((packed));

	struct node_subheader_annotation
	{
		uint16_t sid;
		uint16_t rid;
		union
		{
			uint8_t flags = 0;

			struct
			{
				uint8_t reserved : 4;
				uint8_t type : 4;
			};
		};

		int parse(igris::buffer data)
		{
			igris::binreader reader(data.data());

			if (data.size() < sizeof(node_subheader))
				return -1;

			reader.read_binary(sid);
			reader.read_binary(rid);
			reader.read_binary(flags);

			return 0;
		}
	};

	struct node;
	crow::node * find_node(int id);
	void __link_node(node *srvs, uint16_t id);
	void bind_node_dynamic(node *srvs);

	static auto node_data(crow::packet_ptr pack)
	{
		return igris::buffer(
		           pack->dataptr() + sizeof(node_subheader),
		           pack->datasize() - sizeof(node_subheader));
	}

	class node
	{
	public:
		struct dlist_head lnk = DLIST_HEAD_INIT(lnk); // Список нодов.
		struct dlist_head waitlnk = DLIST_HEAD_INIT(waitlnk); // Список ожидающих прихода сообщения.
		uint16_t id = 0;

		virtual void incoming_packet(crow::packet_ptr pack) = 0;
		virtual void undelivered_packet(crow::packet_ptr pack) 
		{ 
			(void) pack;
			notify_all(-1);
		}
		int waitevent();
		void notify_one(int future);
		void notify_all(int future);

		virtual const char* typestr()
		{
			return "node";
		}

		node& bind(int addr)
		{
			system_lock();
			__link_node(this, addr);
			system_unlock(); 
			return *this;
		};

		node& bind()
		{
			bind_node_dynamic(this); 
			return *this;
		};

		crow::packet_ptr send(uint16_t rid,
		                      const crow::hostaddr_view& raddr,
		                      const igris::buffer data,
		                      uint8_t qos,
		                      uint16_t ackquant,
		                      bool fastsend = false)
		{
			return crow::node_send(id, rid, raddr, data, qos, ackquant, fastsend);
		}

		crow::packet_ptr send_special(uint16_t rid,
		                              const crow::hostaddr_view& raddr,
		                              uint8_t type,
		                              const igris::buffer data,
		                              uint8_t qos,
		                              uint16_t ackquant,
		                              bool fastsend = false)
		{
			return crow::node_send_special(id, rid, raddr, type, data, qos, ackquant, fastsend);
		}

		crow::packet_ptr send_v(uint16_t rid,
		                        const crow::hostaddr_view& raddr,
		                        const igris::buffer * vdat,
		                        size_t vlen,
		                        uint8_t qos,
		                        uint16_t ackquant,
		                        bool fastsend = false)
		{
			return crow::node_send_v(id, rid, raddr, vdat, vlen, qos, ackquant, fastsend);
		}

		static
		igris::buffer message(crow::packet_ptr pack)
		{
			return node_data(pack);
		}

		static
		node_subheader* subheader(crow::packet_ptr pack)
		{
			return (crow::node_subheader*) pack->dataptr();
		}

		static
		nid_t sid(crow::packet *pack)
		{
			return subheader(pack)->sid;
		}

		static
		node_subheader_annotation annotation(crow::packet_ptr pack)
		{
			node_subheader_annotation annot;
			annot.parse(pack->dataptr());
			return annot;
		}

		virtual ~node();
	};

	class node_protocol_cls : public crow::protocol
	{
	private:
		void send_node_error(crow::packet_ptr pack, int errcode);

	public:
		void incoming(crow::packet_ptr pack) override;
		void undelivered(crow::packet_ptr pack) override;

		node_protocol_cls() : protocol(CROW_NODE_PROTOCOL)
		{}

		static auto sid(crow::packet_ptr pack) { return ((node_subheader*)(pack->dataptr()))->sid; }
		static auto rid(crow::packet_ptr pack) { return ((node_subheader*)(pack->dataptr()))->rid; }

		static auto get_error_code(crow::packet_ptr pack)
		{
			return *(int*)(node_data(pack).data());
		}
	};
	extern node_protocol_cls node_protocol;
	extern igris::dlist<node, &node::lnk> nodes_list;

	class node_packet_ptr : public packet_ptr
	{
	public:
		node_packet_ptr(crow::packet *pack_) : packet_ptr(pack_) {}
		node_packet_ptr(const crow::packet_ptr &oth) : packet_ptr(oth) {}
		node_packet_ptr(crow::packet_ptr &&oth) : packet_ptr(std::move(oth)) {}

		int rid() 
		{
			auto * h = node::subheader(pack);
			return h->rid;
		}

		igris::buffer message()
		{
			return node_data(pack);
		}
	};
}

#endif
