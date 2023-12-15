#ifndef DO_NOT_SLEEP_DO_NOT_SLEEP_HMS_H_
#define DO_NOT_SLEEP_DO_NOT_SLEEP_HMS_H_

#include <cstdint>
#include <utility>

namespace ds {

// Hours Minutes Seconds
struct HMS {
  std::uint_fast8_t hours;
  std::uint_fast8_t minutes;
  std::uint_fast8_t seconds;

  [[nodiscard]] bool between(const HMS& l, const HMS& r) const;
  [[nodiscard]] bool between(const std::pair<HMS, HMS>& range) const;

  friend bool operator<(const HMS& l, const HMS& r);
  friend bool operator>(const HMS& l, const HMS& r);
  friend bool operator<=(const HMS& l, const HMS& r);
  friend bool operator>=(const HMS& l, const HMS& r);
  friend bool operator==(const HMS& l, const HMS& r);
  friend bool operator!=(const HMS& l, const HMS& r);

  static const HMS UNSET;

  static HMS now();
};

} // namespace ds

#endif // DO_NOT_SLEEP_DO_NOT_SLEEP_HMS_H_
