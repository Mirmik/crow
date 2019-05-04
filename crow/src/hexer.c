#include <crow/hexer.h>
#include <ctype.h>
#include <igris/util/hexascii.h>
#include <stdbool.h>
#include <string.h>

int hexer(uint8_t *dst, size_t maxsz, const char *src, size_t srcsz)
{
	const char *it = src;
	const char *const eit = src + srcsz;
	uint16_t sz = 0;

	while (it != eit)
	{
		switch (*it)
		{
		case '.':
		{
			uint8_t byte = 0;
			while (isdigit(*++it))
			{
				byte *= 10;
				byte += hex2half(*it);
			}
			++sz;
			*dst++ = byte;
		}
		break;
		case ':':
		{
			uint16_t twobyte = 0;
			while (isdigit(*++it))
			{
				twobyte *= 10;
				twobyte += hex2half(*it);
			}
			*dst++ = (twobyte & 0xFF00) >> 8;
			*dst++ = twobyte & 0x00FF;
			sz += 2;
		}
		break;
		case '#':
		{
			uint8_t high;
			bool phase = 0;
			while (isxdigit(*++it))
			{
				if (phase)
				{
					*dst++ = (high << 4) | hex2half(*it);
					++sz;
				}
				else
				{
					high = hex2half(*it);
				}
				phase = !phase;
			}
			if (phase)
			{
				*dst++ = high << 4;
				++sz;
			}
		}
		break;
		case '_':
			++it;
			break;
		default:
			return -1;
		}
	}

	return sz;
}

int hexer_s(uint8_t *dst, size_t maxsz, const char *src)
{
	return hexer(dst, maxsz, src, strlen(src));
}