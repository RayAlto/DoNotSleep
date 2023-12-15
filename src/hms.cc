#include "do_not_sleep/hms.h"

#include <chrono>
#include <ctime>
#include <tuple>

#include "do_not_sleep/util.h"

namespace ds {

[[nodiscard]] bool HMS::between(const HMS& l, const HMS& r) const {
  if ((l == UNSET) || (r == UNSET)) {
    return true;
  }
  if (l == r) {
    return *this == l;
  }
  if (l < r) {
    return ((l < *this) && (*this < r));
  }
  return ((l < *this) || (*this < r));
}

[[nodiscard]] bool HMS::between(const std::pair<HMS, HMS>& range) const {
  return between(range.first, range.second);
}

bool operator<(const HMS& l, const HMS& r) {
  return std::tie(l.hours, l.minutes, l.seconds) < std::tie(r.hours, r.minutes, r.seconds);
}

bool operator>(const HMS& l, const HMS& r) {
  return r < l;
}

bool operator<=(const HMS& l, const HMS& r) {
  return !(l > r);
}

bool operator>=(const HMS& l, const HMS& r) {
  return !(l < r);
}

bool operator==(const HMS& l, const HMS& r) {
  if (&l == &r) {
    return true;
  }
  return std::tie(l.hours, l.minutes, l.seconds) == std::tie(r.hours, r.minutes, r.seconds);
}

bool operator!=(const HMS& l, const HMS& r) {
  return !(l == r);
}

const HMS HMS::UNSET = {.hours = 0, .minutes = 0, .seconds = 0};

HMS HMS::now() {
  const std::chrono::system_clock::time_point now_time_point = std::chrono::system_clock::now();
  const std::time_t now_time_t = std::chrono::system_clock::to_time_t(now_time_point);
  const std::tm& now_tm = localtime_safe(now_time_t);
  return HMS{.hours = static_cast<uint_fast8_t>(now_tm.tm_hour),
             .minutes = static_cast<uint_fast8_t>(now_tm.tm_min),
             .seconds = static_cast<uint_fast8_t>(now_tm.tm_sec)};
}

} // namespace ds
