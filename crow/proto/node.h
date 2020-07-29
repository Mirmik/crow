#ifndef CROW_NODE_H
#define CROW_NODE_H

#include <crow/packet.h>
#include <crow/address.h>
#include <crow/proto/protocol.h>

#include <igris/container/dlist.h>
#include <igris/binreader.h>
#include <igris/sync/syslock.h>

#define CROW_NTYPE_COMMON 0
#define CROW_NTYPE_TRANSPORT_ERROR 1

#define CROW_ERRNO_UNREGISTRED_RID 33

namespace crow
{
	crow::packet_ptr node_send(uint16_t sid,
	                           uint16_t rid,
	                           const crow::hostaddr & addr,
	                           const igris::buffer data,
	                           uint8_t qos,
	                           uint16_t ackquant, uint8_t flags=CROW_NTYPE_COMMON);

	crow::packet_ptr node_send_v(uint16_t sid,
	                             uint16_t rid,
	                             const crow::hostaddr & addr,
	                             const igris::buffer * vec,
	                             size_t veclen,
	                             uint8_t qos,
	                             uint16_t ackquant, uint8_t flags=CROW_NTYPE_COMMON);

	// TODO: replace with annotation
	struct node_subheader
	{
		uint16_t sid;
		uint16_t rid;
		int8_t flags = 0;
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
	void link_node(node *srvs, uint16_t id);
	void bind_node_dynamic(node *srvs);

	static auto node_data(crow::packet *pack)
	{
		return igris::buffer(
		           pack->dataptr() + sizeof(node_subheader),
		           pack->datasize() - sizeof(node_subheader));
	}

	class node_packet_ptr : public packet_ptr
	{
	public:
		node_packet_ptr(crow::packet *pack_) : packet_ptr(pack_) {}
		node_packet_ptr(const crow::packet_ptr &oth) : packet_ptr(oth) {}
		node_packet_ptr(crow::packet_ptr &&oth) : packet_ptr(std::move(oth)) {}

		igris::buffer message()
		{
			return node_data(pack);
		}
	};

	struct node
	{
		struct dlist_head lnk = DLIST_HEAD_INIT(lnk); // Список нодов.
		struct dlist_head waitlnk = DLIST_HEAD_INIT(waitlnk); // Список ожидающих прихода сообщения.
		uint16_t id = 0;
		const char* mnem = NULL;

		virtual void incoming_packet(crow::packet *pack) = 0;
		virtual void undelivered_packet(crow::packet *pack) = 0;
		int waitevent();
		void notify_one(int future);

		virtual const char* typestr()
		{
			return "node";
		}

		node& bind(int addr)
		{
			link_node(this, addr); return *this;
		};

		node& bind()
		{
			bind_node_dynamic(this); return *this;
		};

		crow::packet_ptr send(uint16_t rid,
		                      const crow::hostaddr& raddr,
		                      const igris::buffer data,
		                      uint8_t qos,
		                      uint16_t ackquant, 
		                      uint8_t flags=CROW_NTYPE_COMMON)
		{
			return crow::node_send(id, rid, raddr, data, qos, ackquant, flags);
		}

		crow::node_packet_ptr send_v(uint16_t rid,
		                             const crow::hostaddr& raddr,
		                             const igris::buffer * vdat,
		                             size_t vlen,
		                             uint8_t qos,
		                             uint16_t ackquant, 
		                             uint8_t flags=CROW_NTYPE_COMMON)
		{
			return crow::node_send_v(id, rid, raddr, vdat, vlen, qos, ackquant, flags);
		}

		static
		igris::buffer message(crow::packet * pack)
		{
			return node_data(pack);
		}

		static
		node_subheader* subheader(crow::packet *pack)
		{
			return (crow::node_subheader*) pack->dataptr();
		}

		static
		node_subheader_annotation annotation(crow::packet *pack)
		{
			node_subheader_annotation annot;
			annot.parse(pack->dataptr());
			return annot;
		}

		~node()
		{
			igris::syslock_guard lock;
			dlist_del(&waitlnk);
			dlist_del(&lnk);
		}
	};

	class system_node_cls : public node
	{
		void incoming_packet(crow::packet *pack) override;

		void undelivered_packet(crow::packet *pack) override
		{
			crow::release(pack);
		}
	};

	class node_protocol_cls : public crow::protocol
	{
	private:
		//void send_node_error(crow::packet *pack, int errcode);

	public:
		system_node_cls system_node;

	public:
		void incoming(crow::packet *pack) override;
		void undelivered(crow::packet *pack) override;

		node_protocol_cls() : protocol(CROW_NODE_PROTOCOL)
		{
			link_node(&system_node, 0);
			system_node.mnem = "twrinfo";
		}

		static auto sid(crow::packet *pack) { return ((node_subheader*)(pack->dataptr()))->sid; }
		static auto rid(crow::packet *pack) { return ((node_subheader*)(pack->dataptr()))->rid; }

		static auto get_error_code(crow::packet *pack)
		{
			return *(int*)(node_data(pack).data());
		}
	};
	extern node_protocol_cls node_protocol;
	extern igris::dlist<node, &node::lnk> nodes;
}

#endif