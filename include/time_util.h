#ifndef __TIME_UTIL_H__
#define __TIME_UTIL_H__

#include <cstdint>
#include <sys/time.h>

namespace libpf {

class TimeUtil {
public:
    static uint64_t now_ms() {
        struct timeval t;
        ::gettimeofday(&t, NULL);
        return t.tv_sec * 1000 + t.tv_usec / 1000;
    }

    static uint64_t now_us() {
        struct timeval t;
        ::gettimeofday(&t, NULL);
        return t.tv_sec * 1000 * 1000 + t.tv_usec;
    }

    static uint64_t now_diff_ms(uint64_t start) {
        return now_ms() - start;
    }

    static uint64_t now_diff_us(uint64_t start) {
        return now_us() - start;
    }

    static std::string unixtime_str(time_t timep) {
        struct tm tm{};
        ::localtime_r(&timep, &tm);

        char buff[125] {};
        ::strftime(buff, sizeof(buff)-1, "%Y-%m-%d %H:%M:%S", &tm);
        return buff;
    }
};

} // end namespace libpf


#endif // __TIME_UTIL_H__