#ifndef DO_NOT_SLEEP_DO_NOT_SLEEP_UTIL_H_
#define DO_NOT_SLEEP_DO_NOT_SLEEP_UTIL_H_

#include <chrono>
#include <cstdint>
#include <ctime>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

#define DS_LOGERR ds::log_error(__FILE__, __LINE__, true)
#define DS_LOG ds::log_error(__FILE__, __LINE__, false)

namespace ds {

std::int64_t current_time_ms();
std::int64_t current_time_ms(const std::chrono::system_clock::time_point& t);

const std::tm& localtime_safe(const std::time_t& t);
std::optional<std::string> getenv_safe(std::string_view key);

std::ostream& log_error(const std::string_view& file, const std::uint_fast32_t& line, const bool& err);

} // namespace ds

#endif // DO_NOT_SLEEP_DO_NOT_SLEEP_UTIL_H_
