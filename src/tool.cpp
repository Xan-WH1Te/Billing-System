#include <tool.h>

#include <iomanip>
#include <sstream>

bool stringToTime(const std::string& timeText, std::time_t& outTime)
{
    std::tm tmTime{};
    std::istringstream in(timeText);
    in >> std::get_time(&tmTime, "%Y-%m-%d %H:%M:%S");
    if (in.fail())
    {
        return false;
    }

    outTime = std::mktime(&tmTime);
    return outTime != static_cast<std::time_t>(-1);
}
