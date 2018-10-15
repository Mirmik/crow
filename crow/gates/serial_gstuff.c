#include <crow/gates/serial_gstuff.h>

#include <crow/tower.h>

#include <gxx/gstuff/autorecv.h>
#include <gxx/gstuff/gstuff.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include <termios.h>

struct crow_serial_gstuff {
	int fd;

	struct crow_gw gw;
	struct crowket * rpack;

	struct gstuff_autorecv recver;
	//struct gstuff_sender sender;

} crow_serial_gstuff;

struct crow_gw* crow_create_serial_gstuff(const char* path, uint32_t baudrate, uint8_t id) 
{
	int ret;

	struct crow_serial_gstuff* g = (struct crow_serial_gstuff*) 
		malloc(sizeof(struct crow_serial_gstuff));
	
	g->fd = open(path, O_RDWR);
	if (g->fd < 0) {
		perror("serial::open");
		exit(0);
	}

	struct termios tattr;
	ret = tcgetattr(g->fd, &tattr);
	if (ret) {
		printf("%s\n", strerror(ret));
	}

	//crow_serialgate_open(g, port); // TODO: should return NULL on error
	g->rpack = NULL;
	//gstuff_autorecv_init(&g->recver, buf);

	crow_link_gate(&g->gw, id);
	
	return &g->gw;
}

void callback_handler(void* priv, int sts, char * buf, int len) {
	/*struct crowket * block = rpack;
	init_recv();

	block->revert_stage(id);

	crowket_initialization(block, this);
	crow_travel(block);*/
}

void crow_serial_gstuff_send(struct crow_gw* gw, struct crowket* pack) 
{
	struct crow_serial_gstuff* g = mcast_out(gw, struct crow_serial_gstuff, gw);
	/*std::string str;
	gxx::io::std_string_writer strm(str);
	gxx::gstuff::sender sender(strm);

	sender.start_message();
	sender.write((char*)&pack->header, pack->header.flen);
	sender.end_message();

	mtx.lock();
	ser->write((uint8_t*)str.data(), str.size());
	mtx.unlock();
*/
	crow_return_to_tower(pack, CROW_SENDED);
}

/*void crow_serial_gstuff_send(struct crow_gw* gw, struct crowket* pack) 
{

}*/

static inline void 
init_recv(struct crow_serial_gstuff* g) {
	g->rpack = (struct crowket*) malloc(128 + sizeof(struct crowket) - sizeof(struct crow_header));
//	gstuff_automat_init(&g->recver, (char*)&g->rpack->header, 128);
	//recver.init(gxx::buffer((char*)&rpack->header, 128));
}

void crow_serial_gstuff_nblock_onestep(struct crow_gw* gw) 
{
/*	if (rpack == nullptr) {
		init_recv();
	}

	char c;
	//int len = read(ser->fd(), (uint8_t*)&c, 1);
	int len = ser->read((uint8_t*)&c, 1);
	if (len == 1) {
		//dprhex(c); dpr("\t"); gxx::println(gxx::dstring(&c, 1));
		recver.newchar(c);
	}*/
}

const struct crow_gw_operations crow_serial_gstuff_ops = {
	.send = crow_serial_gstuff_send,
	.nblock_onestep = crow_serial_gstuff_nblock_onestep
};