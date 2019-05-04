#ifndef CROW_BROCKER_H
#define CROW_BROCKER_H

#include <stdio.h>

#include <unordered_map>
#include <unordered_set>

#include <igris/print.h>
#include <igris/print/stdprint.h>

extern bool brocker_info;

void brocker_publish(const std::string& theme, const std::string& data);
void g3_brocker_subscribe(uint8_t* raddr, size_t rlen, const std::string& theme, uint8_t qos, uint16_t ackquant);

namespace crow
{
	struct g3_subscriber
	{
		uint8_t* host;
		size_t hlen;
		uint8_t qos;
		uint16_t ackquant;

		g3_subscriber() = default;
		g3_subscriber(const uint8_t* host, size_t hlen, uint8_t qos, uint16_t ackquant) 
			: qos(qos), ackquant(ackquant), hlen(hlen)
		{
			this->host = (uint8_t*) malloc(hlen);
			memcpy(this->host, host, hlen);
		}
		
		~g3_subscriber() 
		{
			free(this->host);
		}

		bool operator==(const g3_subscriber& oth) const
		{
			return hlen == oth.hlen && (memcmp(host, oth.host, hlen) == 0);
		}

		bool operator!=(const g3_subscriber& oth) const
		{
			return hlen != oth.hlen || (memcmp(host, oth.host, hlen) != 0);
		}
	};
}


namespace std
{
	template<>
	struct hash<crow::g3_subscriber>
	{
		size_t operator()(const crow::g3_subscriber & x) const noexcept
		{
			return std::hash<std::string>()(std::string((char*)x.host, x.hlen));
		}
	};
}

namespace crow
{
	struct theme
	{
		theme() = default;
		theme(const std::string& name) : name(name) {}
		theme(const theme& oth) : name(oth.name) {}
		std::string name;
		std::unordered_set<crow::g3_subscriber> subs;
		void publish(const std::string& data);
	};
}

extern std::unordered_map<std::string, crow::theme> themes;

#endif