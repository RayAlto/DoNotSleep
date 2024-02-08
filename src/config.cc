#include "do_not_sleep/config.h"

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "pwd.h"
#include "unistd.h"

#include "json/json.h"

#include "do_not_sleep/util.h"

namespace ds {

Config::Policy policy_from_string(std::string_view str) {
  static const std::unordered_map<std::string_view, Config::Policy> str2policy{
    {"time_range", Config::Policy::TIME_RANGE},
    {"monitor_io", Config::Policy::MONITOR_IO}
  };
  std::unordered_map<std::string_view, Config::Policy>::const_iterator policy_iter = str2policy.find(str);
  if (policy_iter == str2policy.end()) {
    std::string policies{};
    for (const auto& [policy, _] : str2policy) {
      policies += '`';
      policies += policy;
      policies += "` ";
    }
    DS_LOGERR << '`' << str << "` is not a valid policy ( " << policies << ")\n";
    return Config::Policy::INVALID;
  }
  return policy_iter->second;
}

bool jsoncpp_load_json(const std::filesystem::path& json_dir, Json::Value& out_json) {
  if (!std::filesystem::is_regular_file(json_dir)) {
    DS_LOGERR << json_dir << " is not a regular file.\n";
    return false;
  }
  std::ifstream conf_file{json_dir, std::ios::binary};
  if (conf_file.bad()) {
    DS_LOGERR << "failed to open file " << json_dir << ".\n";
    return false;
  }
  Json::CharReaderBuilder crb;
  std::string errs;
  if (!Json::parseFromStream(crb, conf_file, &out_json, &errs)) {
    DS_LOGERR << "failed to parse json from " << json_dir << "!\n" << errs << ".\n";
    return false;
  }
  return true;
}

constexpr std::string_view jsoncpp_valuetype_str(const Json::ValueType& value_type) {
  switch (value_type) {
    case Json::ValueType::nullValue: return "null"; break;
    case Json::ValueType::intValue: return "signed integer"; break;
    case Json::ValueType::uintValue: return "unsigned integer"; break;
    case Json::ValueType::realValue: return "float point"; break;
    case Json::ValueType::stringValue: return "string"; break;
    case Json::ValueType::booleanValue: return "boolean"; break;
    case Json::ValueType::arrayValue: return "array"; break;
    case Json::ValueType::objectValue: return "object"; break;
    default: return "unknown"; break;
  }
}

static const std::filesystem::path CONFIG_FILE = std::filesystem::path{".config"} / "do_not_sleep" / "conf";

/* NOLINTNEXTLINE(readability-function-cognitive-complexity) */
Config Config::from_json(const std::filesystem::path& config_dir) {
  Config conf;
  Json::Value conf_json;

  if (!jsoncpp_load_json(config_dir, conf_json)) {
    DS_LOGERR << "failed to load config.\n";
    return UNSET;
  }

  Json::Value dirs_json = conf_json["dirs"];
  if (dirs_json == Json::Value::null) {
    DS_LOGERR << "failed to read key `dirs` from " << config_dir << ".\n";
    return UNSET;
  }

  for (const Json::Value& dir_json : dirs_json) {
    if (!dir_json.isString()) {
      DS_LOGERR << "`dirs.*` should be string, got `" << dir_json << "` which is "
                << jsoncpp_valuetype_str(dir_json.type()) << ", from " << config_dir << ".\n";
      return UNSET;
    }
    conf.dirs.emplace(dir_json.asString());
  }

  Json::Value interval_json = conf_json["interval"];
  if (interval_json == Json::Value::null) {
    DS_LOGERR << "failed to read key `interval` from " << config_dir << ".\n";
    return UNSET;
  }
  if (!interval_json.isUInt()) {
    DS_LOGERR << "`interval` should be unsigned integer, got `" << interval_json << "` which is "
              << jsoncpp_valuetype_str(interval_json.type()) << ", from " << config_dir << ".\n";
    return UNSET;
  }
  conf.interval = std::chrono::seconds{interval_json.asUInt()};
  if (conf.interval.count() == 0) {
    DS_LOGERR << "`interval` should not be zero, got `" << interval_json << "`, from " << config_dir << ".\n";
    return UNSET;
  }

  Json::Value policy_json = conf_json["policy"];
  if (policy_json == Json::Value::null) {
    DS_LOGERR << "failed to read key `policy` from " << config_dir << ".\n";
    return UNSET;
  }
  if (!policy_json.isString()) {
    DS_LOGERR << "`policy` should be string, got `" << policy_json << "` which is "
              << jsoncpp_valuetype_str(policy_json.type()) << ", from " << config_dir << ".\n";
    return UNSET;
  }
  conf.policy = policy_from_string(policy_json.asString());
  if (conf.policy == Policy::INVALID) {
    DS_LOGERR << "`policy` is not valic, got `" << policy_json.asString() << "` from " << config_dir << ".\n";
    return UNSET;
  }

  if (conf.policy == Policy::TIME_RANGE) {
    Json::Value time_range_json = conf_json["time_range"];
    if (time_range_json == Json::Value::null) {
      DS_LOGERR << "failed to read key `time_range` from " << config_dir << ".\n";
      return UNSET;
    }

    Json::Value time_range_start_json = time_range_json["start"];
    if (time_range_start_json == Json::Value::null) {
      DS_LOGERR << "failed to read key `time_range.start` from " << config_dir << ".\n";
      return UNSET;
    }
    if (time_range_start_json.size() != 3) {
      DS_LOGERR << "`time_range.start` should contain 3 items, got " << time_range_start_json.size() << " from "
                << config_dir << ".\n";
      return UNSET;
    }

    Json::Value time_range_start_0_json = time_range_start_json[0];
    if (!time_range_start_0_json.isUInt()) {
      DS_LOGERR << "`time_range.start.0` should be unsigned integer, got `" << time_range_start_0_json << "` which is "
                << jsoncpp_valuetype_str(time_range_start_0_json.type()) << ", from " << config_dir << ".\n";
      return UNSET;
    }
    conf.time_range.first.hours = time_range_start_0_json.asUInt();
    if (conf.time_range.first.hours > 24) {
      DS_LOGERR << "`time_range.start.0` represents the hour (0-24), got " << time_range_start_0_json << " from "
                << config_dir << ".\n";
      return UNSET;
    }

    Json::Value time_range_start_1_json = time_range_start_json[1];
    if (!time_range_start_1_json.isUInt()) {
      DS_LOGERR << "`time_range.start.1` should be unsigned integer, got `" << time_range_start_1_json << "` which is "
                << jsoncpp_valuetype_str(time_range_start_1_json.type()) << ", from " << config_dir << ".\n";
      return UNSET;
    }
    conf.time_range.first.minutes = time_range_start_1_json.asUInt();
    if (conf.time_range.first.minutes > 60) {
      DS_LOGERR << "`time_range.start.1` represents the minute (0-60), got " << time_range_start_1_json << " from "
                << config_dir << ".\n";
      return UNSET;
    }

    Json::Value time_range_start_2_json = time_range_start_json[2];
    if (!time_range_start_2_json.isUInt()) {
      DS_LOGERR << "`time_range.start.2` should be unsigned integer, got `" << time_range_start_2_json << "` which is "
                << jsoncpp_valuetype_str(time_range_start_2_json.type()) << ", from " << config_dir << ".\n";
      return UNSET;
    }
    conf.time_range.first.seconds = time_range_start_2_json.asUInt();
    if (conf.time_range.first.seconds > 60) {
      DS_LOGERR << "`time_range.start.2` represents the second (0-60), got " << time_range_start_2_json << " from "
                << config_dir << ".\n";
    }

    Json::Value time_range_end_json = time_range_json["end"];
    if (time_range_end_json == Json::Value::null) {
      DS_LOGERR << "failed to read key `time_range.end` from " << config_dir << ".\n";
      return UNSET;
    }
    if (time_range_end_json.size() != 3) {
      DS_LOGERR << "`time_range.end` should contain 3 items, got " << time_range_end_json.size() << " from "
                << config_dir << ".\n";
      return UNSET;
    }

    Json::Value time_range_end_0_json = time_range_end_json[0];
    if (!time_range_end_0_json.isUInt()) {
      DS_LOGERR << "`time_range.end.0` should be unsigned integer, got `" << time_range_end_0_json << "` which is "
                << jsoncpp_valuetype_str(time_range_end_0_json.type()) << ", from " << config_dir << ".\n";
      return UNSET;
    }
    conf.time_range.second.hours = time_range_end_0_json.asUInt();
    if (conf.time_range.second.hours > 24) {
      DS_LOGERR << "`time_range.end.0` represents the hour (0-24), got " << time_range_end_0_json << " from "
                << config_dir << ".\n";
      return UNSET;
    }

    Json::Value time_range_end_1_json = time_range_end_json[1];
    if (!time_range_end_1_json.isUInt()) {
      DS_LOGERR << "`time_range.end.1` should be unsigned integer, got `" << time_range_end_1_json << "` which is "
                << jsoncpp_valuetype_str(time_range_end_1_json.type()) << ", from " << config_dir << ".\n";
      return UNSET;
    }
    conf.time_range.second.minutes = time_range_end_1_json.asUInt();
    if (conf.time_range.second.minutes > 24) {
      DS_LOGERR << "`time_range.end.1` represents the minute (0-60), got " << time_range_end_1_json << " from "
                << config_dir << ".\n";
      return UNSET;
    }

    Json::Value time_range_end_2_json = time_range_end_json[2];
    if (!time_range_end_2_json.isUInt()) {
      DS_LOGERR << "`time_range.end.1` should be unsigned integer, got `" << time_range_end_2_json << "` which is "
                << jsoncpp_valuetype_str(time_range_end_2_json.type()) << ", from " << config_dir << ".\n";
      return UNSET;
    }
    conf.time_range.second.seconds = time_range_end_2_json.asUInt();
    if (conf.time_range.second.seconds > 24) {
      DS_LOGERR << "`time_range.end.2` represents the second (0-60), got " << time_range_end_2_json << " from "
                << config_dir << ".\n";
      return UNSET;
    }
  } else if (conf.policy == Policy::MONITOR_IO) {
    // ============================================================
    // TODO(rayalto): fix this shit
    DS_LOGERR << "monitor_io policy is currently unavailable.\n";
    return UNSET;
    // ============================================================

    Json::Value monitor_io_json = conf_json["monitor_io"];
    if (monitor_io_json == Json::Value::null) {
      DS_LOGERR << "failed to read key `monitor_io` from " << config_dir << ".\n";
      return UNSET;
    }

    Json::Value scan_frequency_json = monitor_io_json["scan_frequency"];
    if (scan_frequency_json == Json::Value::null) {
      DS_LOGERR << "failed to read key `monitor_io.scan_frequency` from " << config_dir << ".\n";
      return UNSET;
    }
    if (!scan_frequency_json.isUInt()) {
      DS_LOGERR << "`monitor_io.scan_frequency` should be unsigned integer, got `" << scan_frequency_json
                << "` which is " << jsoncpp_valuetype_str(scan_frequency_json.type()) << ", from " << config_dir
                << ".\n";
      return UNSET;
    }
    conf.scan_frequency = std::chrono::seconds{scan_frequency_json.asUInt()};
    if (conf.scan_frequency > conf.interval) {
      DS_LOGERR << "scan_frequency should not be greater than interval (" << conf.scan_frequency.count() << " > "
                << conf.interval.count() << ").\n";
      return UNSET;
    }

    Json::Value keep_awake_json = monitor_io_json["keep_awake"];
    if (keep_awake_json == Json::Value::null) {
      DS_LOGERR << "failed to read key `monitor_io.keep_awake` from " << config_dir << ".\n";
      return UNSET;
    }
    if (!keep_awake_json.isUInt()) {
      DS_LOGERR << "`monitor_io.keep_awake` should be unsigned integer, got `" << keep_awake_json << "` which is "
                << jsoncpp_valuetype_str(keep_awake_json.type()) << ", from " << config_dir << ".\n";
      return UNSET;
    }
    conf.keep_awake = std::chrono::seconds{keep_awake_json.asUInt()};
  }
  return conf;
}

bool operator==(const Config& l, const Config& r) {
  if (&l == &r) {
    return true;
  }
  return std::tie(l.dirs, l.time_range, l.interval) == std::tie(r.dirs, r.time_range, r.interval);
}

bool operator!=(const Config& l, const Config& r) {
  return !(l == r);
}

const Config Config::UNSET{};

const std::filesystem::path Config::CONFIG_DIR{[]() -> std::filesystem::path {
  std::size_t pwd_buf_len = sysconf(_SC_GETPW_R_SIZE_MAX);
  if (pwd_buf_len == -1) {
    pwd_buf_len = 4096;
  }
  char pwd_buf[pwd_buf_len];
  passwd pwd_result{};
  passwd* pwd_resultp = nullptr;
  int e = getpwuid_r(getuid(), &pwd_result, pwd_buf, pwd_buf_len, &pwd_resultp);
  if (e == 0) {
    return std::filesystem::path{pwd_result.pw_dir} / CONFIG_FILE;
  }
  std::optional<std::string> env_home = getenv_safe("HOME");
  if (env_home.has_value()) {
    return std::filesystem::path{env_home.value()} / CONFIG_FILE;
  }
  DS_LOGERR << "failed to derive user directory, `ds::Config::CONFIG_DIR` need to be initialized manually!\n";
  return {};
}()};

} // namespace ds
