#include <crow/proto/rpc.h>

void crow::rpc_node::incoming_packet(crow::packet_ptr pack)
{
	int8_t format;
	int8_t status;

	igris::buffer data;
	igris::buffer outdata;
	igris::buffer function_name;
	crow::node_subheader * nsh = crow::node::subheader(pack);

	data = crow::node_data(pack);

	igris::archive::binreader reader(data.data(), data.size());

	reader.load(format);
	reader.load_set_buffer(function_name);

	data = igris::buffer { reader.pointer(), (size_t)((char*)reader.end() - (char*)reader.pointer()) };

	remote_function_basic * procedure = rfuncmap[
	                                        std::string(function_name.data(), function_name.size())];
	if (procedure == nullptr)
	{
		status = CROW_RPC_ERROR_FUNCTION_NOT_FOUNDED;

		igris::buffer vdata[] =
		{
			{ &status, 1 },
		};

		crow::node_send_v(
		    nsh->rid,
		    nsh->sid,
		    pack->addr(),
		    vdata,
		    1,
		    0,
		    10
		);

		return;
	}

	if (format == CROW_RPC_BINARY_FORMAT)
	{
		size_t outsize = procedure->outsize();
		void * outbuf = alloca(outsize);
		outdata = igris::buffer{outbuf, outsize};
		status = procedure->invoke(data, outdata);

		igris::buffer vdata[] =
		{
			{ &status, 1 },
			outdata
		};

		crow::node_send_v(
		    nsh->rid,
		    nsh->sid,
		    pack->addr(),
		    vdata,
		    2,
		    0,
		    10
		);
	}

	else if (format == CROW_RPC_TEXT_FORMAT) 
	{
		std::string outdata;
		std::string senddata;
		igris::archive::binary_string_writer writer(senddata);

		status = procedure->invoke_text_format(data, outdata);

		writer.dump(status);
		igris::serialize(writer, outdata);

		crow::node_send(
		    nsh->rid,
		    nsh->sid,
		    pack->addr(),
		    senddata,
		    0,
		    0
		);
	}

	else 
	{
		status = CROW_RPC_ERROR_UNRESOLVED_FORMAT;

		igris::buffer vdata[] =
		{
			{ &status, 1 },
		};

		crow::node_send_v(
		    nsh->rid,
		    nsh->sid,
		    pack->addr(),
		    vdata,
		    1,
		    0,
		    10
		);
	}
}
