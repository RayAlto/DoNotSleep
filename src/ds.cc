#include "do_not_sleep/ds.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <set>
#include <thread>
#include <unordered_map>
#include <utility>

#include "do_not_sleep/block_info.h"
#include "do_not_sleep/config.h"
#include "do_not_sleep/hms.h"
#include "do_not_sleep/util.h"

namespace ds {

struct MonitorCtx {
  BlockInfo block_info;
  std::chrono::seconds awake_time_remaining;
  std::chrono::seconds time_until_next_ticktock;
};

DoNotSleep::DoNotSleep()
  : config{Config::from_json()}
  , rand_engine(current_time_ms() & std::numeric_limits<std::uint8_t>::max()) {
}

DoNotSleep::DoNotSleep(std::initializer_list<std::filesystem::path> dirs,
                       std::chrono::seconds interval,
                       std::pair<HMS, HMS> time_range)
  : config{.dirs = dirs, .interval = interval, .policy = Config::Policy::TIME_RANGE, .time_range = time_range}
  , rand_engine(current_time_ms() & std::numeric_limits<std::uint8_t>::max()) {
}

DoNotSleep::DoNotSleep(std::initializer_list<std::filesystem::path> dirs,
                       std::chrono::seconds interval,
                       std::chrono::seconds scan_frequency,
                       std::chrono::seconds keep_awake)
  : config{.dirs = dirs,
           .interval = interval,
           .policy = Config::Policy::MONITOR_IO,
           .scan_frequency = scan_frequency,
           .keep_awake = keep_awake}
  , rand_engine(current_time_ms() & std::numeric_limits<std::uint8_t>::max()) {
}

void DoNotSleep::start() {
  if (config == Config::UNSET) {
    DS_LOGERR << "something wrong with the config, stopped.\n";
    return;
  }
  sanitize_config();
  if (config.dirs.empty()) {
    DS_LOGERR << "no dirs to proceed, stopped.\n";
    return;
  }
  switch (config.policy) {
    case Config::Policy::TIME_RANGE: start_time_range(); break;
    case Config::Policy::MONITOR_IO: start_monitor_io(); break;
    case Config::Policy::SERVICE_AVAILABLE: start_service_available(); break;
    default: DS_LOGERR << "invalid policy, stopped.\n"; return;
  }
}

void DoNotSleep::start_time_range() {
  while (true) {
    if (!HMS::now().between(config.time_range)) {
      DS_LOG << "zzz\n" << std::flush;
      std::this_thread::sleep_for(config.interval);
      continue;
    }
    for (const std::filesystem::path& dir : config.dirs) {
      tick_tock(dir);
    }
    std::this_thread::sleep_for(config.interval);
  }
}

/* NOLINTNEXTLINE(readability-function-cognitive-complexity) */
void DoNotSleep::start_monitor_io() {
  // TODO(rayalto): try to make these shit work
  std::unordered_map<std::filesystem::path, MonitorCtx> blocks;
  for (const std::filesystem::path& block : config.dirs) {
    blocks.emplace(block,
                   MonitorCtx{.block_info{BlockInfo::from_mount_path(block)},
                              .awake_time_remaining{std::chrono::seconds::zero()},
                              .time_until_next_ticktock{std::chrono::seconds::zero()}});
  }
  while (true) {
    for (std::pair<const std::filesystem::path, ds::MonitorCtx>& block : blocks) {
      bool io_detected = (block.second.block_info.io_taken() != BlockInfo::NO_IO);

      if (block.second.awake_time_remaining > std::chrono::seconds::zero()) {
        // decrease the remaining time to keep awake by scan frequency
        block.second.awake_time_remaining -= config.scan_frequency;
        if (block.second.awake_time_remaining < std::chrono::seconds::zero()) {
          // make sure it is not negative
          block.second.awake_time_remaining = std::chrono::seconds::zero();
        }
      }

      if (block.second.time_until_next_ticktock > std::chrono::seconds::zero()) {
        // decrease the time untile next ticktock by scan frequency
        block.second.time_until_next_ticktock -= config.scan_frequency;
        if (block.second.time_until_next_ticktock <= std::chrono::seconds::zero()) {
          // time to ticktock, flush I/O statistics at first
          io_detected = (io_detected || (block.second.block_info.io_taken() != BlockInfo::NO_IO));
          tick_tock(block.first);
          // ignore I/O from our tichtock
          block.second.block_info.io_taken();
          if (block.second.awake_time_remaining > std::chrono::seconds::zero()) {
            // still need to keep awake, prepare for the next ticktock
            block.second.time_until_next_ticktock = config.interval;
          } else {
            // no need to keep awake anymore
            block.second.time_until_next_ticktock = std::chrono::seconds::zero();
          }
        }
      }

      if (!io_detected) {
        continue;
      }

      // I/O operation detected
      DS_LOG << block.first << ": I/O detected.\n";
      block.second.awake_time_remaining = config.keep_awake;
      if (block.second.time_until_next_ticktock == std::chrono::seconds::zero()) {
        block.second.time_until_next_ticktock = config.interval;
      }
    }
    std::this_thread::sleep_for(config.scan_frequency);
  }
}

void DoNotSleep::start_service_available() {
  while (true) {
    if (service_available(config.service)) {
      for (const std::filesystem::path& dir : config.dirs) {
        tick_tock(dir);
      }
    } else {
      DS_LOG << "zzz\n" << std::flush;
    }
    std::this_thread::sleep_for(config.interval);
  }
}

bool DoNotSleep::sanitize_config() {
  // dirs
  for (std::set<std::filesystem::path>::const_iterator i_dir = config.dirs.cbegin(); i_dir != config.dirs.end();) {
    if (!std::filesystem::is_directory(*i_dir)) {
      DS_LOGERR << *i_dir << " is not a directory, ignored.\n";
      i_dir = config.dirs.erase(i_dir);
      continue;
    }
    std::ofstream test_file{*i_dir / DS_FILENAME, std::ios::trunc};
    if (test_file.bad()) {
      DS_LOGERR << "could not create " << DS_FILENAME << " in " << *i_dir << ", ignored.\n";
      i_dir = config.dirs.erase(i_dir);
      continue;
    }
    test_file.close();
    i_dir++;
  }

  return true;
}

void DoNotSleep::tick_tock(const std::filesystem::path& dir) {
  const std::filesystem::path ds_filedir = dir / DS_FILENAME;
  bool tick{false};
  if (std::filesystem::is_empty(ds_filedir)) {
    tick = true;
  }
  std::ofstream ds_file{ds_filedir, std::ios::trunc | std::ios::binary};
  if (tick) {
    std::uint8_t rand_byte_buf[DS_RAND_BYTE_COUNT + 1];
    std::generate_n(rand_byte_buf, DS_RAND_BYTE_COUNT, std::ref(rand_engine));
    DS_LOG << dir << " tick.\n" << std::flush;
    ds_file.write(reinterpret_cast<const char*>(rand_byte_buf), DS_RAND_BYTE_COUNT);
  } else {
    DS_LOG << dir << " tock.\n" << std::flush;
  }
  ds_file.close();
}

const std::filesystem::path DoNotSleep::DS_FILENAME{".do_not_sleep"};
const std::size_t DoNotSleep::DS_RAND_BYTE_COUNT{4};

} // namespace ds
