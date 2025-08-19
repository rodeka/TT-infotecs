#pragma once
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace logging
{

    // ISO8601 без исключений
    inline std::string now_iso_local()
    {
        using namespace std::chrono;
        auto tp = system_clock::now();
        std::time_t t = system_clock::to_time_t(tp);
        std::tm tm{};
#if defined(_POSIX_VERSION)
        localtime_r(&t, &tm);
#else
        tm = *std::localtime(&t);
#endif

        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
        auto ms = duration_cast<milliseconds>(tp.time_since_epoch()) % 1000;
        char out[40];
        std::snprintf(out, sizeof(out), "%s.%03lld", buf, static_cast<long long>(ms.count()));
        return std::string(out);
    }

    // Преобразование числового значения в Severity метку
    inline const char *to_str_severity(int s)
    {
        switch (s)
        {
        case 0:
            return "INFO";
        case 1:
            return "WARN";
        case 2:
            return "ERROR";
        default:
            return "UNK";
        }
    }

} // namespce logging