#ifndef DO_NOT_SLEEP_DO_NOT_SLEEP_CONFIG_H_
#define DO_NOT_SLEEP_DO_NOT_SLEEP_CONFIG_H_

#include <chrono>
#include <filesystem>
#include <set>
#include <utility>

#include "hms.h"

namespace ds {

struct Config {
  std::set<std::filesystem::path> dirs;
  std::pair<HMS, HMS> time_range;
  std::chrono::seconds interval;

  static Config from_json(const std::filesystem::path& config_dir = CONFIG_DIR);

  friend bool operator==(const Config& l, const Config& r);
  friend bool operator!=(const Config& l, const Config& r);

  static const Config UNSET;

  static const std::filesystem::path CONFIG_DIR;
};

} // namespace ds

#endif // DO_NOT_SLEEP_DO_NOT_SLEEP_CONFIG_H_
