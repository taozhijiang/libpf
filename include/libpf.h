#ifndef __LIBPF_H__
#define __LIBPF_H__

#include <string>

namespace libpf {

bool init();
bool init(uint32_t duration, uint32_t sample);
void terminate();


void submit(const std::string& metric, int32_t val);

bool message(std::string& msg);


} // end namespace libpf

#endif // __LIBPF_H__