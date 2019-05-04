#include <crow/tower.h>

#include <nos/print.h>
#include <nos/fprint.h>

#include <igris/util/dstring.h>
#include <igris/util/hexascii.h>
#include <igris/util/string.h>

void crow_print_to(nos::ostream& out, crowket_t* pack) {
	nos::fprint_to(out, "("
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
	crow_print_to(*nos::current_ostream, pack);
}

void crow_println(crowket_t* pack) {
	crow_print_to(*nos::current_ostream, pack);
	nos::print_to(*nos::current_ostream, "\n");
}
