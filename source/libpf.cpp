#include "libpf-internal.h"
#include "libpf.h"

#include "elapsed-repos.h"


namespace libpf {

bool init() {
    return ElapsedRepos::instance().init(60, 1);
}

bool init(uint32_t duration, uint32_t sample) {
    return ElapsedRepos::instance().init(duration, sample);
}

void terminate() {
    ElapsedRepos::instance().terminate();
}

void submit(const std::string& metric, int32_t val) {
    ElapsedRepos::instance().submit(metric, val);
}

bool message(std::string& msg) {
    return ElapsedRepos::instance().message(msg);
}

} // end namespace libpf