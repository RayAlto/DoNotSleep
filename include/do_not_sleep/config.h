#ifndef DO_NOT_SLEEP_DO_NOT_SLEEP_CONFIG_H_
#define DO_NOT_SLEEP_DO_NOT_SLEEP_CONFIG_H_

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <set>
#include <string>
#include <utility>

#include "hms.h"

namespace ds {

struct Config {
  enum class Policy : std::uint8_t { INVALID, TIME_RANGE, MONITOR_IO, SERVICE_AVAILABLE };
  std::set<std::filesystem::path> dirs;
  std::chrono::seconds interval;
  Policy policy;
  std::pair<HMS, HMS> time_range;
  std::chrono::seconds scan_frequency;
  std::chrono::seconds keep_awake;
  std::string service;

  static Config from_json(const std::filesystem::path& config_dir = CONFIG_DIR);

  friend bool operator==(const Config& l, const Config& r);
  friend bool operator!=(const Config& l, const Config& r);

  static const Config UNSET;

  static const std::filesystem::path CONFIG_DIR;
};

} // namespace ds

#endif // DO_NOT_SLEEP_DO_NOT_SLEEP_CONFIG_H_
