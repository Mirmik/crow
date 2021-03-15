#include <crow/proto/channel.h>
#include <crow/proto/acceptor.h>
#include <crow/tower.h>

#include <igris/util/bug.h>
#include <nos/print.h>

void crow::channel::incoming_packet(crow::packet_ptr pack)
{
	crow::node_subheader *sh_node = (node_subheader *) pack->dataptr();
	crow::subheader_channel *sh_channel = crow::get_subheader_channel(pack);

	switch (sh_channel->ftype)
	{
		case crow::Frame::HANDSHAKE_REQUEST:
			// Кто-то пытается установить с нами связь.
			if (naive_listener || _state == CROW_CHANNEL_WAIT_HANDSHAKE_REQUEST)
			{
				crow::subheader_handshake *sh_handshake =
				    crow::get_subheader_handshake(pack);

				// TODO: перенести аллокацию под адрес в другое место
				//raddr_ptr = malloc(pack->header.alen);
				if (pack->header.alen > raddr_cap)
					return;

				memcpy(raddr_ptr, pack->addrptr(), pack->header.alen);
				raddr_len = pack->header.alen;
				rid = sh_node->sid;

				this->qos = sh_handshake->qos;
				this->ackquant = sh_handshake->ackquant;

				send_handshake_answer();

				_state = CROW_CHANNEL_CONNECTED;
			}
			else
			{
				dprln("WARN: HANDSHAKE_REQUEST on another state");
			}

			break;

		case crow::Frame::HANDSHAKE_ANSWER:
			// Кто-то ответил на зов.
			if (_state == CROW_CHANNEL_WAIT_HANDSHAKE_ANSWER)
			{
				// Если мы еще ни с кем и никогда.
				crow::subheader_handshake *sh_handshake =
				    crow::get_subheader_handshake(pack);

				// TODO: перенести аллокацию под адрес в другое место
				//raddr_ptr = malloc(pack->header.alen);
				if (pack->header.alen > raddr_cap)
					return;

				memcpy(raddr_ptr, pack->addrptr(), pack->header.alen);
				raddr_len = pack->header.alen;
				rid = sh_node->sid;
				qos = sh_handshake->qos;
				ackquant = sh_handshake->ackquant;

				_state = CROW_CHANNEL_CONNECTED;
				notify_one(0);
			}
			else
			{
				dprln("WARN: HANDSHAKE_ANSWER on another state");
			}

			break;

		case crow::Frame::DATA:
			incoming_data_packet(pack);
			return;

		case crow::Frame::REFUSE:
			_state = CROW_CHANNEL_DISCONNECTED;
			break;

		default:
			BUG();
			break;
	}
}

void crow::channel::incoming_data_packet(crow::packet_ptr pack)
{
	this->incoming_handler(this, pack);
}

void crow::channel::undelivered_packet(crow::packet_ptr pack)
{
	notify_one(-1);
	_state = CROW_CHANNEL_DISCONNECTED;
}

void crow::channel::handshake(const uint8_t *raddr_ptr,
                              uint16_t raddr_len,
                              uint16_t rid,
                              uint8_t qos,
                              uint16_t ackquant)
{
	crow::node_subheader sh_node;
	crow::subheader_channel sh_channel;
	crow::subheader_handshake sh_handshake;

	sh_node.sid = id;
	sh_node.rid = this->rid = rid;
	sh_node.type = CROW_NODEPACK_COMMON;

	sh_channel.frame_id = 0;
	sh_channel.ftype = crow::Frame::HANDSHAKE_REQUEST;

	sh_handshake.qos = this->qos = qos;
	sh_handshake.ackquant = this->ackquant = ackquant;

	igris::buffer vec[] =
	{
		{&sh_node, sizeof(sh_node)},
		{&sh_channel, sizeof(sh_channel)},
		{&sh_handshake, sizeof(sh_handshake)}
	};

	_state = CROW_CHANNEL_WAIT_HANDSHAKE_ANSWER;

	//_state = crow::State::CONNECTED;
	crow::send_v({raddr_ptr, raddr_len}, vec, sizeof(vec) / sizeof(igris::buffer),
	             CROW_NODE_PROTOCOL, 2, ackquant);
}

void crow::channel::send_handshake_answer()
{
	crow::node_subheader sh_node;
	crow::subheader_channel sh_channel;
	crow::subheader_handshake sh_handshake;

	sh_node.sid = id;
	sh_node.rid = rid;
	sh_node.type = CROW_NODEPACK_COMMON;

	sh_channel.frame_id = 0;
	sh_channel.ftype = crow::Frame::HANDSHAKE_ANSWER;

	sh_handshake.qos = this->qos;
	sh_handshake.ackquant = this->ackquant;

	igris::buffer vec[] =
	{
		{&sh_node, sizeof(sh_node)},
		{&sh_channel, sizeof(sh_channel)},
		{&sh_handshake, sizeof(sh_handshake)}
	};

	//_state = crow::State::CONNECTED;
	crow::send_v({raddr_ptr, raddr_len}, vec, sizeof(vec) / sizeof(igris::buffer),
	             CROW_NODE_PROTOCOL, 2, ackquant);
}

int crow::channel::send(const char *data, size_t size)
{
	if (_state != CROW_CHANNEL_CONNECTED)
	{
		return CROW_CHANNEL_ERR_NOCONNECT;
	}

	crow::node_subheader sh_node;
	crow::subheader_channel sh_channel;

	sh_node.sid = this->id;
	sh_node.rid = this->rid;
	sh_node.type = CROW_NODEPACK_COMMON;

	sh_channel.frame_id = this->fid++;
	sh_channel.ftype = crow::Frame::DATA;

	igris::buffer vec[] =
	{
		{&sh_node, sizeof(sh_node)},
		{&sh_channel, sizeof(sh_channel)},
		{(void *)data, size},
	};

	crow::send_v({this->raddr_ptr, this->raddr_len}, vec, sizeof(vec) / sizeof(igris::buffer),
	             CROW_NODE_PROTOCOL, this->qos, this->ackquant);

	return 0;
}


igris::buffer crow::channel::getdata(crow::packet_ptr pack)
{
	return igris::buffer(pack->dataptr() + sizeof(crow::node_subheader) +
	                     sizeof(crow::subheader_channel),
	                     pack->datasize() - sizeof(crow::node_subheader) -
	                     sizeof(crow::subheader_channel));
}
