#include "do_not_sleep/util.h"

#include <bits/chrono.h>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <ostream>

namespace ds {

std::int64_t current_time_ms() {
  return current_time_ms(std::chrono::system_clock::now());
}

std::int64_t current_time_ms(const std::chrono::system_clock::time_point& t) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(t.time_since_epoch()).count();
}

const std::tm& localtime_safe(const std::time_t& t) {
  static std::mutex localtime_mutex;
  std::unique_lock<std::mutex> localtime_lock(localtime_mutex);
  /* NOLINTNEXTLINE(concurrency-mt-unsafe) */
  return *std::localtime(&t);
}

std::optional<std::string> getenv_safe(std::string_view key) {
  static std::mutex getenv_mutex;
  std::unique_lock<std::mutex> getenv_lock(getenv_mutex);
  /* NOLINTNEXTLINE(concurrency-mt-unsafe) */
  char* value = std::getenv(key.data());
  if (value == nullptr) {
    return std::nullopt;
  }
  return value;
}

std::ostream& log_error(const std::string_view& file, const std::uint_fast32_t& line, const bool& err) {
  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
  const std::tm& now_time_tm = localtime_safe(std::chrono::system_clock::to_time_t(now));
  std::int_fast64_t now_ms
    = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
  std::ostream& out = (err ? std::cerr : std::cout);
  const char fill_prev = out.fill();
  /* [YYYY-MM-DDThh:mm:ss.sss](__FILE__:__LINE__): MSG */
  return out << '[' << std::put_time(&now_time_tm, "%FT%T") << '.' << std::setfill('0') << std::setw(3) << now_ms
             << std::setfill(fill_prev) << "](" << file << ':' << line << "): ";
}

} // namespace ds
