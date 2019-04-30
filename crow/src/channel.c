#include <crow/channel.h>
#include <crow/tower.h>
#include <sys/uio.h>

/*
void crow::link_channel(crow::channel* ch, uint16_t id) {
	ch->id = id;
	crow::nodes.move_back(*ch);
}

void crow::channel::incoming_packet(crow::packet* pack) {
	crow::subheader* sh0 = crow::get_subheader(pack);
	crow::subheader_channel* sh2 = crow::get_subheader_channel(pack);

	switch(sh2->ftype) {
		case crow::Frame::HANDSHAKE:
			igris::println("HANDSHAKE");
			if (state == crow::State::INIT) {
				crow::subheader_handshake* shh = crow::get_subheader_handshake(pack);
				rid = sh0->sid;
				qos = shh->qos;
				ackquant = shh->ackquant;
				raddr_ptr = malloc(pack->header.alen);
				memcpy(raddr_ptr, pack->addrptr(), pack->header.alen);
				raddr_len = pack->header.alen;
				state = crow::State::CONNECTED;

			}
			else {
				igris::panic("no INIT state");
				//unknown_port(pack);
			}
			break;
		case crow::Frame::DATA:
			igris::println("DATA");
			incoming_data_packet(pack);
			return;
		case crow::Frame::REFUSE:
			igris::println("REFUSE");
			state = crow::State::DISCONNECTED;
			break;
		default: break;
	}
	crow::release(pack);
}

void crow::acceptor::incoming_packet(crow::packet* pack) {
	crow::subheader* sh0 = crow::get_subheader(pack);
	crow::subheader_channel* sh2 = crow::get_subheader_channel(pack);
	crow::subheader_handshake* shh = crow::get_subheader_handshake(pack);

	auto ch = init_channel();

	crow::handshake(ch, sh0->sid, pack->addrptr(), pack->addrsize(), shh->qos, shh->ackquant);
	crow::release(pack);
}

void crow::handshake(crow::channel* ch, uint16_t rid, const void* raddr_ptr, size_t raddr_len, crow::QoS qos, uint16_t ackquant) {
	crow::subheader sh0;
	crow::subheader_channel sh2;
	crow::subheader_handshake shh;

	sh0.sid = ch->id;
	sh0.rid = ch->rid = rid;

	sh2.frame_id = 0;
	sh2.ftype = crow::Frame::HANDSHAKE;

	ch->raddr_ptr = malloc(raddr_len);
	memcpy(ch->raddr_ptr, raddr_ptr, raddr_len);
	ch->raddr_len = raddr_len;

	ch->qos = shh.qos = qos;
	ch->ackquant = shh.ackquant = ackquant;

	igris::iovec vec[] = {
		{&sh0, sizeof(sh0)},
		{&sh2, sizeof(sh2)},
		{&shh, sizeof(shh)},
	};

	ch->state = crow::State::CONNECTED;
	crow::send(raddr_ptr, raddr_len, vec, sizeof(vec) / sizeof(igris::iovec), G1_G0TYPE, crow::QoS(2), ackquant);
}

void crow::__channel_send(crow::channel* ch, const char* data, size_t size) {
	crow::subheader sh0;
	crow::subheader_channel sh2;
	sh0.sid = ch->id;
	sh0.rid = ch->rid;
	sh2.frame_id = ch->fid++;
	sh2.ftype = crow::Frame::DATA;	

	igris::iovec vec[] = {
		{&sh0, sizeof(sh0)},
		{&sh2, sizeof(sh2)},
		{data, size},
	};
	crow::send(ch->raddr_ptr, ch->raddr_len, vec, sizeof(vec) / sizeof(igris::iovec), G1_G0TYPE, ch->qos, ch->ackquant);
}

uint16_t crow::dynport() {
	return 512;
}

void crow::channel::handshake(const crow::host& host, uint16_t rid, crow::QoS qos, uint16_t ackquant) {
	crow::handshake(this, rid, host.data, host.size, qos, ackquant);
}

int crow::channel::send(const char* data, size_t size) {
	crow::__channel_send(this, data, size);
}
*/