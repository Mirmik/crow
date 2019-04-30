#include <crow/tower.h>
#include <igris/print.h>
#include <igris/print/stdprint.h>
#include <igris/util/hexascii.h>
#include <igris/util/string.h>

void crow_print_to(igris::io::ostream& out, crowket_t* pack) {
	igris::fprint_to(out, "("
		"qos:{}, "
		"ack:{}, "
		"alen:{}, "
		"type:{}, "
		"addr:{}, "
		"stg:{}, "
		"data:{}, "
		"released:{}"
		")", 
		pack->header.qos,
		(uint8_t)pack->header.f.ack, 
		pack->header.alen, 
		(uint8_t)pack->header.f.type, 
		igris::hexascii_encode((const uint8_t*)crowket_addrptr(pack), pack->header.alen), 
		pack->header.stg, 
		igris::dstring(crowket_dataptr(pack), crowket_datasize(pack)), 
		pack->flags
	);
}

void crow_print(crowket_t* pack) {
	crow_print_to(*igris::standart_output, pack);
}

void crow_println(crowket_t* pack) {
	crow_print_to(*igris::standart_output, pack);
	igris::print_to(*igris::standart_output, "\n");
}
