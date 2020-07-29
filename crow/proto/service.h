/**
	Протокол для брокера на основе механизма нодов и почтового ящика.
	Позволяет зарегистрировать сервис в брокере, передавать команды и получать ответы.
*/

#include <crow/proto/msgbox.h>

#include <igris/event/delegate.h>
#include <igris/binreader.h>

#define CROW_SERVICE_REGISTER 1
#define CROW_BROCKER_SERVICE_NODE 5

#define CROW_SERVICE_SERVICE_TO_BROCKER_REGISTER 0
#define CROW_SERVICE_FROM_USER_TO_BROCKER_QUESTION 1
#define CROW_SERVICE_FROM_BROCKER_TO_SERVICE_QUESTION 2
#define CROW_SERVICE_FROM_SERVICE_TO_BROCKER_ANSWER 3
#define CROW_SERVICE_FROM_USER_TO_BROCKER_ANSWER 4

namespace crow
{
	void service_registration(const crow::hostaddr & addr, const crow::node & node, const char * mnemo, uint8_t qos, uint16_t ackquant);

	/*class service;

	struct service_package_annotation
	{
		uint8_t type;
		uint16_t datalen = 0;
		uint16_t namelen = 0;
		const uint8_t* data = nullptr;
		const char* name = nullptr;

		int parse(igris::buffer message)
		{
			igris::binreader reader;

			if (message.size() < 7)
				return -1;

			reader.init(message.data());

			reader.read_binary(type);
			reader.read_binary(datalen);
			reader.read_binary(namelen);

			reader.bind_buffer(data, datalen);
			reader.bind_buffer(name, namelen);

			return 0;
		}

		void to_buffers(igris::buffer bufs[5])
		{
			bufs[0] = {&type, 1};
			bufs[1] = {&datalen, sizeof(datalen)};
			bufs[2] = {&namelen, sizeof(namelen)};
			bufs[3] = {data, datalen};
			bufs[4] = {name, namelen};
		}
	};

	class service_request_ptr : public node_packet_ptr
	{
	public:
		crow::service * srv;
		service_package_annotation & annot;

		service_request_ptr(crow::packet *pack_,
		                   crow::service * srv,
		                   service_package_annotation & annotation)
			: node_packet_ptr(pack_), srv(srv), annot(annotation)
		{}

		crow::service * service() { return srv; }
		igris::buffer   message() { return {annot.data, annot.datalen}; }

		packet_ptr answer(igris::buffer data);
	};

	class service_answer_ptr : public node_packet_ptr
	{
	public:
		service_package_annotation annot;

		service_answer_ptr(const node_packet_ptr & ptr) : node_packet_ptr(ptr)
		{
			annot.parse(node_packet_ptr::message());			
		}

		igris::buffer   message() { return {annot.data, annot.datalen}; }
};

	class service : public crow::node
	{
		using dlg_t = igris::delegate <void, crow::service_request_ptr>;

	private:
		crow::hostaddr brocker_addr;

		const char *   _name;
		dlg_t          _handler;

	public:
		service(const char * name, dlg_t dlg) : _name(name), _handler(dlg)
		{}

		const char * name() { return _name; }

		void incoming_packet(crow::packet *pack) override
		{
			int sts;
			service_package_annotation annot;
			igris::buffer message;

			message = node::message(pack);

			if ((sts = annot.parse(message)))
			{
				crow::release(pack);
				return;
			}

			_handler({ pack, this, annot });
		};

		void undelivered_packet(crow::packet *pack) override
		{
			crow::release(pack);
		};

		void dial(const crow::hostaddr & addr,
		          int qos,
		          uint16_t ackquant)
		{
			brocker_addr = addr;

			service_package_annotation annot;
			igris::buffer buffers[5];

			annot.type = CROW_SERVICE_REGISTER;
			annot.name = _name;
			annot.namelen = strlen(_name);
			annot.to_buffers(buffers);

			node::send_v(
			    CROW_BROCKER_SERVICE_NODE,
			    addr,
			    buffers,
			    std::size(buffers),
			    qos,
			    ackquant);
		}
	};*/

	class requester_sync : public crow::node
	{
		crow::packet * _answer;

	public:
		crow::node_packet_ptr query(const crow::hostaddr & addr,
		                               const char * theme,
		                               igris::buffer data,
		                               uint8_t qos,
		                               uint16_t ackquant)
		{
			uint8_t type = CROW_SERVICE_ONESHOOT;
			uint8_t namelen = strlen(theme);
			uint16_t datalen = data.size();

			igris::buffer buffers[5] = 
			{
				{ &type, sizeof(type) },
				{ &namelen, sizeof(namelen) },
				{ &datalen, sizeof(datalen) },
				{ theme, namelen },
				{ data.data(), data.size() }
			}

			node::send_v(
			           CROW_BROCKER_SERVICE_NODE,
			           addr,
			           buffers,
			           std::size(buffers),
			           qos,
			           ackquant);

			waitevent();

			return _answer;
		}

	protected:
		void incoming_packet(crow::packet * pack) 
		{
			_answer = pack;
			notify_one(0);
		}

		void undelivered_packet(crow::packet * pack) 
		{
			notify_one(-1);
		}
	};
}