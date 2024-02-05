#include "do_not_sleep/block_info.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

#include "do_not_sleep/util.h"

namespace ds {

const std::filesystem::path BlockInfo::MOUNT_INFO_PATH{"/proc/self/mountinfo"};
const std::filesystem::path BlockInfo::SYS_BLOCK_PATH{"/sys/block"};
const std::filesystem::path BlockInfo::BLOCK_STAT_NAME{"stat"};

/* NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables) */
std::unordered_map<std::filesystem::path, std::filesystem::path> BlockInfo::mount_list;

BlockInfo BlockInfo::from_mount_path(const std::filesystem::path& mount_path) {
  if (!std::filesystem::is_directory(mount_path)) {
    throw std::runtime_error{"mount path `" + mount_path.string() + "` is not a directory"};
  }
  update_mount_list();
  std::unordered_map<std::filesystem::path, std::filesystem::path>::const_iterator mount_info
    = mount_list.find(mount_path);
  if (mount_info == mount_list.end()) {
    // not found
    throw std::runtime_error{"Could not find mount source of `" + mount_path.string() + "`"};
  }
  return BlockInfo{mount_info};
}

BlockInfo BlockInfo::from_block_path(const std::filesystem::path& block_path) {
  if (!std::filesystem::is_directory(block_path)) {
    throw std::runtime_error{"block path `" + block_path.string() + "` is not a directory"};
  }
  update_mount_list();
  std::unordered_map<std::filesystem::path, std::filesystem::path>::const_iterator mount_info = mount_list.begin();
  for (; mount_info != mount_list.end(); mount_info++) {
    if (mount_info->second == block_path) {
      return BlockInfo{mount_info};
    }
  }
  throw std::runtime_error{"could not find info about `" + block_path.string() + "` in " + MOUNT_INFO_PATH.string()};
}

BlockInfo BlockInfo::from_block_name(const std::filesystem::path& block_name) {
  update_mount_list();
  std::unordered_map<std::filesystem::path, std::filesystem::path>::const_iterator mount_info = mount_list.begin();
  for (; mount_info != mount_list.end(); mount_info++) {
    if (mount_info->first.filename() == block_name) {
      return BlockInfo{mount_info};
    }
  }
  throw std::runtime_error{"could not find info about `" + block_name.string() + "` in " + MOUNT_INFO_PATH.string()};
}

[[nodiscard]] std::uint64_t BlockInfo::total_reads() const {
  return get_io_statistics().first;
}

[[nodiscard]] std::uint64_t BlockInfo::total_writes() const {
  return get_io_statistics().second;
}

std::uint64_t BlockInfo::reads_taken() {
  std::uint64_t reads = get_io_statistics().first;
  if (reads == last_io.first) {
    return 0;
  }
  std::uint64_t result = reads - last_io.first;
  last_io.first = reads;
  return result;
}

std::uint64_t BlockInfo::writes_taken() {
  std::uint64_t writes = get_io_statistics().second;
  if (writes == last_io.second) {
    return 0;
  }
  std::uint64_t result = writes - last_io.second;
  last_io.second = writes;
  return result;
}

std::pair<std::uint64_t, std::uint64_t> BlockInfo::io_taken() {
  std::pair<std::uint64_t, std::uint64_t> io = get_io_statistics();
  std::pair<std::uint64_t, std::uint64_t> result;
  if (io.first == last_io.first) {
    result.first = 0;
  } else {
    result.first = io.first - last_io.first;
    last_io.first = io.first;
  }
  if (io.second == last_io.second) {
    result.second = 0;
  } else {
    result.second = io.second - last_io.second;
    last_io.second = io.second;
  }
  return result;
}

void BlockInfo::update_mount_list(const bool& force) {
  if (!mount_list.empty() && !force) {
    return;
  }
  std::ifstream mount_info_file(MOUNT_INFO_PATH);
  std::string mount_point;
  std::string mount_source;
  while (!mount_info_file.eof()) {
    for (int i = 0; i < 5; i++) {
      mount_info_file >> mount_point;
    }
    for (int i = 0; i < 5; i++) {
      mount_info_file >> mount_source;
    }
    mount_list[mount_point] = mount_source;
    mount_info_file >> mount_source;
    mount_info_file >> std::ws;
  }
  mount_info_file.close();
}

std::filesystem::path BlockInfo::find_block_stat(const std::filesystem::path& block) {
  const std::string block_name = block.has_parent_path() ? block.filename().string() : block.string();
  std::filesystem::path block_path = SYS_BLOCK_PATH / block_name;
  if (std::filesystem::exists(block_path)) {
    // it is a disk
    return /* disk */ block_path;
  }
  // it is a device of a disk
  const std::string& device_name = block_name;
  // try to find the disk to which this device belongs
  //   e.g. sda1 -> sda ==> /sys/block/sda
  bool found = false;
  std::string_view disk_name{device_name};
  std::filesystem::path disk_path;
  while (disk_name.length() > 1) {
    disk_name.remove_suffix(1);
    disk_path = SYS_BLOCK_PATH / disk_name;
    if (std::filesystem::exists(disk_path)) {
      found = true;
      break;
    }
  }
  if (!found) {
    throw std::runtime_error{"could not find `" + block.string() + "` in " + SYS_BLOCK_PATH.string()};
  }
  std::filesystem::path device_stat = disk_path / device_name / BLOCK_STAT_NAME;
  if (std::filesystem::exists(device_stat)) {
    // found the stat file of this device
    return device_stat;
  }
  std::filesystem::path disk_stat = disk_path / BLOCK_STAT_NAME;
  if (std::filesystem::exists(disk_stat)) {
    // found the stat file path of the disk
    return disk_stat;
  }
  // fuck
  throw std::runtime_error{"could not find stat file of `" + disk_path.string() + "`"};
}

std::pair<std::uint64_t, std::uint64_t> BlockInfo::get_io_statistics(const std::filesystem::path& stat_file) {
  std::ifstream stat(stat_file);
  if (!stat.good()) {
    DS_LOGERR << "failed to read stat file " << stat_file << '\n';
    return {-1, -1};
  }
  std::uint64_t reads{0};
  std::uint64_t writes{0};
  for (int i = 0; i < 3; i++) {
    stat >> reads;
  }
  for (int i = 0; i < 4; i++) {
    stat >> writes;
  }
  stat.close();
  return {reads, writes};
}

BlockInfo::BlockInfo(std::unordered_map<std::filesystem::path, std::filesystem::path>::const_iterator mount_info)
  : mount_info_iter(mount_info) {
  stat_file = find_block_stat(mount_info_iter->second);
  last_io = get_io_statistics();
}

[[nodiscard]] std::pair<std::uint64_t, std::uint64_t> BlockInfo::get_io_statistics() const {
  return get_io_statistics(stat_file);
}

} // namespace ds
