#include <crow/address.h>
#include <string>

namespace crow
{
    hostaddr address(const igris::buffer &in)
    {
        std::vector<uint8_t> out;
        out.resize(in.size());

        int len = hexer_s((uint8_t *)out.data(), in.size(), in.data());

        if (len < 0)
            return {};

        out.resize(len);
        return out;
    }

    hostaddr address_warned(const igris::buffer &in)
    {
        std::vector<uint8_t> out;
        out.resize(in.size());

        int len = hexer_s((uint8_t *)out.data(), in.size(), in.data());

        /*if (len == CROW_HEXER_MORE3_DOT) 
            dprln("crow::hexer: more then three symbols after dot.");

        if (len == CROW_HEXER_ODD_GRID)
            dprln("crow::hexer: odd symbols after #");

        if (len == CROW_HEXER_UNDEFINED_SYMBOL)
            dprln("crow::hexer: undefined symbol");*/

        if (len < 0)
            return {};

        out.resize(len);
        return out;
    }
} // namespace crow


crow::hostaddr crow::crowker_address() 
{
    return crow::address(getenv("CROWKER"));
}