#include "do_not_sleep/ds.h"

#include <chrono>
#include <cstddef>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <set>
#include <thread>
#include <utility>

#include "do_not_sleep/config.h"
#include "openssl/rand.h"

#include "do_not_sleep/hms.h"
#include "do_not_sleep/util.h"

namespace ds {

DoNotSleep::DoNotSleep() : config_{Config::from_json()} {
}

DoNotSleep::DoNotSleep(std::initializer_list<std::filesystem::path> dirs,
                       std::pair<HMS, HMS> time_range,
                       std::chrono::seconds interval)
  : config_{.dirs = dirs, .time_range = time_range, .interval = interval} {
  auto min = std::chrono::system_clock::time_point::min();
}

void DoNotSleep::start() {
  sanitize_config_();
  if (config_.dirs.empty()) {
    DS_LOGERR << "no dirs to proceed, stopped.\n";
    return;
  }
  while (true) {
    if (!HMS::now().between(config_.time_range)) {
      DS_LOG << "zzz\n" << std::flush;
      std::this_thread::sleep_for(config_.interval);
      continue;
    }
    for (const std::filesystem::path& dir : config_.dirs) {
      tick_tock_(dir);
    }
    std::this_thread::sleep_for(config_.interval);
  }
}

bool DoNotSleep::sanitize_config_() {
  if (config_ == Config::UNSET) {
  }
  // dirs
  for (std::set<std::filesystem::path>::const_iterator i_dir = config_.dirs.cbegin(); i_dir != config_.dirs.end();) {
    if (!std::filesystem::is_directory(*i_dir)) {
      DS_LOGERR << *i_dir << " is not a directory, ignored.\n";
      i_dir = config_.dirs.erase(i_dir);
      continue;
    }
    std::ofstream test_file{*i_dir / DS_FILENAME_, std::ios::trunc};
    if (test_file.bad()) {
      DS_LOGERR << "could not create " << DS_FILENAME_ << " in " << *i_dir << ", ignored.\n";
      i_dir = config_.dirs.erase(i_dir);
      continue;
    }
    test_file.close();
    i_dir++;
  }

  return true;
}

void DoNotSleep::tick_tock_(const std::filesystem::path& dir) {
  const std::filesystem::path ds_filedir = dir / DS_FILENAME_;
  bool tick{false};
  if (std::filesystem::is_empty(ds_filedir)) {
    tick = true;
  }
  std::ofstream ds_file{ds_filedir, std::ios::trunc | std::ios::binary};
  if (tick) {
    unsigned char rand_byte_buf[DS_RAND_BYTE_COUNT_ + 1];
    if (RAND_bytes(rand_byte_buf, DS_RAND_BYTE_COUNT_) <= 0) {
      DS_LOGERR << "OpenSSL RAND_bytes call failed!\n";
      ds_file.close();
      return;
    }
    DS_LOG << dir << " tick.\n" << std::flush;
    ds_file.write(reinterpret_cast<const char*>(rand_byte_buf), DS_RAND_BYTE_COUNT_);
  } else {
    DS_LOG << dir << " tock.\n" << std::flush;
  }
  ds_file.close();
}

const std::filesystem::path DoNotSleep::DS_FILENAME_{".do_not_sleep"};
const std::size_t DoNotSleep::DS_RAND_BYTE_COUNT_{4};

} // namespace ds
