#ifndef CROW_ADDRESS_H
#define CROW_ADDRESS_H

#include <stdint.h>
#include <stdlib.h>
#include <sys/cdefs.h>

#include <igris/dprint.h>

#ifdef __cplusplus
#include <string>
#include <vector>
#include <igris/buffer.h>
#endif

#define CROW_HEXER_UNDEFINED_SYMBOL -1
#define CROW_HEXER_MORE3_DOT -4
#define CROW_HEXER_ODD_GRID -5

__BEGIN_DECLS

/// Утилита для удобной записи хекс адреса.
/// Пример: .12.127.0.0.1#2714  -> 0C7F0000012714
/// Пример: .12.127.0.0.1:10004 -> 0C7F0000012714
/// Пример: .12.127:.1:10004 -> 0C7F0000012714
/// Пример: .12#7F000001:10004 -> 0C7F0000012714
/// Пример: #0C#7F000001#2714 -> 0C7F0000012714
/// Пример: #0C7F0000012714 -> 0C7F0000012714
int hexer(uint8_t *dst, size_t maxsz, const char *src, size_t srcsz);
int hexer_s(uint8_t *dst, size_t maxsz, const char *src);

__END_DECLS


#ifdef __cplusplus
namespace crow
{
	std::vector<uint8_t> address(const std::string& in);
	std::vector<uint8_t> address_warned(const std::string& in);

	class hostaddr
	{
	public:
		const void * addr;
		size_t       alen;

		hostaddr() = default;
		hostaddr(const std::vector<uint8_t> & v) : addr(v.data()), alen(v.size()) {}
		hostaddr(const igris::buffer & v) : addr(v.data()), alen(v.size()) {}
		hostaddr(const void * addr, size_t alen) : addr(addr), alen(alen) {}

		const void * data() const { return addr; }
		const size_t size() const { return alen; }
	};
}
#endif

#endif