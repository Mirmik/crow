/**
	@file
*/

#ifndef G1_UTIL_H
#define G1_UTIL_H

#include <string>
#include <igris/datastruct/iovec.h>

#include <g1/packet.h>
#include <g1/tower.h>
#include <g1/indexes.h>

#include <igris/inet/dgramm.h>

namespace g1 {
	namespace util {
		static inline std::string udphost_from_enviroment(std::string envvar) { 
			auto ret = getenv(envvar.c_str());
			if (ret == nullptr) return std::string();
			return ret; 
		}		

		static inline uint16_t udpport_from_enviroment(std::string envvar) { 
			auto ret = getenv(envvar.c_str());
			if (ret == nullptr) return 0;
			return atoi(ret);	
		}

		
	}
}

#endif