#ifndef DO_NOT_SLEEP_DO_NOT_SLEEP_DS_H_
#define DO_NOT_SLEEP_DO_NOT_SLEEP_DS_H_

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <initializer_list>
#include <utility>

#include "do_not_sleep/config.h"
#include "do_not_sleep/hms.h"

namespace ds {

class DoNotSleep {
public:
  DoNotSleep();
  DoNotSleep(std::initializer_list<std::filesystem::path> dirs,
             std::pair<HMS, HMS> time_range = {HMS::UNSET, HMS::UNSET},
             std::chrono::seconds interval = std::chrono::seconds{30});
  DoNotSleep(const DoNotSleep&) = default;
  DoNotSleep(DoNotSleep&&) noexcept = default;
  DoNotSleep& operator=(const DoNotSleep&) = default;
  DoNotSleep& operator=(DoNotSleep&&) noexcept = default;

  virtual ~DoNotSleep() = default;

  void start();

protected:
  Config config_;

  bool sanitize_config_();

  static void tick_tock_(const std::filesystem::path& dir);

  static const std::filesystem::path DS_FILENAME_;
  static const std::size_t DS_RAND_BYTE_COUNT_;
};

} // namespace ds

#endif // DO_NOT_SLEEP_DO_NOT_SLEEP_DS_H_
