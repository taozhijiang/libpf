#ifndef __LIBPF_INTERNAL_H__
#define __LIBPF_INTERNAL_H__

#include <cstdint>

#define __noncopyable__(class_name) \
private: \
class_name(const class_name&) = delete; \
class_name& operator=(const class_name&) = delete;

namespace libpf {
    class ElapsedRepos;
}

#endif // __LIBPF_INTERNAL_H__