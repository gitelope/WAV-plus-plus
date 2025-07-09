
#include "Formats.hpp"
#include <format>
#include <sstream>

namespace wav
{

std::ostream & operator<< (std::ostream & os, GUID const & guid)
{
    os << std::format ("{:08X}-{:04X}-{:04X}-{:02X}{:02X}-{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}",
                       guid.data1,
                       guid.data2,
                       guid.data3,
                       guid.data4[0], guid.data4[1],
                       guid.data4[2], guid.data4[3], guid.data4[4], guid.data4[5], guid.data4[6], guid.data4[7]);
    return os;
}

std::string GUID::str() const
{
    std::ostringstream oss;
    oss << *this;
    return oss.str();
}

} // namespace wav
