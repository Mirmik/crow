#ifndef CROW_CLI_NODE
#define CROW_CLI_NODE

#include <crow/proto/node.h>
#include <igris/shell/mshell.h>
#include <igris/datastruct/argvc.h>

#include <nos/print.h>

namespace crow
{
	template <class Dictionary>
	class cli_node : public crow::node
	{
		Dictionary dictionary;

	public:
		bool debug_mode = false;

		cli_node(Dictionary&& dictionary) : dictionary(std::move(dictionary)) {}
		cli_node(const Dictionary& dictionary) : dictionary(dictionary) {}

		void incoming_packet(crow::packet_ptr pack) override;

		void undelivered_packet(crow::packet_ptr pack) override
		{}
	};

	template <class Dictionary>
	void crow::cli_node<Dictionary>::incoming_packet(crow::packet_ptr pack)
	{
		auto sh = crow::node::subheader(pack);
		auto data = crow::node_data(pack);

		if (debug_mode)
		{
			dpr("clidata: "); debug_write(data.data(), data.size()); dprln();
		}

		char buf[32];
		memset(buf, 0, 32);
		typename Dictionary::iterator it;

		if (data.size() == 0)
			goto __end__;

		char * argv[10];
		memset(argv, 0, sizeof(argv));

		int argc;

		if (data.data()[data.size()-1] == '\n')
			data.data()[data.size()-1] = 0;
		else 
		{
			data.data()[data.size()] = 0;	
		}

		argc = argvc_internal_split_n(data.data(), data.size(), argv, 10);

		if (argc == 0) 
			goto __end__;
			
		it = dictionary.find(argv[0]);

		if (it == dictionary.end()) {
			sprintf(buf, "Unrecognized command: %s\n", argv[0]);
		}
		else 
		{
			int ret = it->second(argc, argv, buf, sizeof(buf));
			(void) ret;
		}
		
		if (strlen(buf))
			node_send(id, sh->sid, {pack->addrptr(), pack->addrsize()},
		    	{buf, strlen(buf)}, 2, 200);

	__end__:
		return;
	}
}

#endif
