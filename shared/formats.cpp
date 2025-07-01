
#include "formats.hpp"
#include <format>
#include <sstream>

namespace wav
{

std::ostream & operator<< (std::ostream & os, GUID const & guid)
{
    os << std::format("{:08X}", guid.data1) << "-";

    os << std::format("{:04X}", guid.data2) << "-";

    os << std::format("{:04X}", guid.data3) << "-";

    os << std::format("{:02X}", guid.data4[0]) << std::format("{:02X}", guid.data4[1]) << "-";

    os << std::format("{:02X}", guid.data4[2]) << std::format("{:02X}", guid.data4[3])
    << std::format("{:02X}", guid.data4[4]) << std::format("{:02X}", guid.data4[5])
    << std::format("{:02X}", guid.data4[6]) << std::format("{:02X}", guid.data4[7]);

    return os;
}

std::string GUID::str() const
{
    std::ostringstream oss;
    oss << *this;
    return oss.str();
}

} // namespace wav
