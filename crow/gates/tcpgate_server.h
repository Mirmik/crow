#ifndef CROW_GATES_TCPGATE_SERVER_H
#define CROW_GATES_TCPGATE_SERVER_H

#include <crow/gateway.h>
#include <set>
#include <vector>

namespace crow
{
	class tcpgate_server : public crow::gateway
	{
		std::set<int> clients;
		int sock_acceptor = 0;
		crow::packet *block = nullptr;

	public:
		tcpgate_server(){}
		tcpgate_server(uint16_t port) { open(port); }

		void send(crow::packet *) override;
		void nblock_onestep() override;

		int open(uint16_t port = 0);
		void close();
		void finish() override;

		int bind(int gate_no) 
		{
			return gateway::bind(gate_no);
		}

#if CROW_ENABLE_WITHOUT_FDS
#else
		int get_fd() override { return sock_acceptor; }
		
		int fds_total() { return 1 + clients.size(); }
		void set_fds_to_array(int * it) 
		{
			*it++ = sock_acceptor;
			for (auto i : clients) 
				*it++ = i;
		}
#endif

		~tcpgate_server() override 
		{
			finish();
		} 
	};
}

#endif