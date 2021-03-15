#pragma once

#include <string>
#include <igris/buffer.h>

#include <crow/packet.h>

std::string flt32_output(void *tgt);
std::string int32_output(void *tgt);

void binout_mode_prepare(const std::string& fmt);
void output_binary(igris::buffer data, crow::packet_ptr pack);