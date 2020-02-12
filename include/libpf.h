#ifndef __LIBPF_H__
#define __LIBPF_H__

#include <string>

namespace libpf {

// 默认设置是采样时长60s，保留最新统计结果1个
bool init();
bool init(uint32_t duration, uint32_t sample);
// 退出内部的线程
void terminate();


void submit(const std::string& metric, int32_t val);

// 返回最新统计结果的字符串显示
bool message(std::string* msg_out);


} // end namespace libpf

#endif // __LIBPF_H__