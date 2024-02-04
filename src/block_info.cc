#include "do_not_sleep/block_info.h"

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <unordered_map>

namespace ds {

const std::filesystem::path BlockInfo::MOUNT_INFO_PATH{"/proc/self/mountinfo"};
const std::filesystem::path BlockInfo::SYS_BLOCK_PATH{"/sys/block"};
const std::filesystem::path BlockInfo::BLOCK_STAT_NAME{"stat"};

std::optional<BlockInfo> BlockInfo::from_mount_path(const std::filesystem::path& mount_path) {
  update_mount_list();
  std::unordered_map<std::filesystem::path, std::filesystem::path>::const_iterator mount_info
    = mount_list.find(mount_path);
  if (mount_info == mount_list.end()) {
    // not found
    return std::nullopt;
  }
  return BlockInfo{mount_info};
}

std::optional<BlockInfo> BlockInfo::from_block_path(const std::filesystem::path& block_path) {
  update_mount_list();
  std::unordered_map<std::filesystem::path, std::filesystem::path>::const_iterator mount_info = mount_list.begin();
  for (; mount_info != mount_list.end(); mount_info++) {
    if (mount_info->second == block_path) {
      return BlockInfo{mount_info};
    }
  }
  return std::nullopt;
}

std::optional<BlockInfo> BlockInfo::from_block_name(const std::filesystem::path& block_name) {
  update_mount_list();
  std::unordered_map<std::filesystem::path, std::filesystem::path>::const_iterator mount_info = mount_list.begin();
  for (; mount_info != mount_list.end(); mount_info++) {
    if (mount_info->first.filename() == block_name) {
      return BlockInfo{mount_info};
    }
  }
  return std::nullopt;
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

std::optional<std::filesystem::path> BlockInfo::find_block_stat(const std::filesystem::path& block) {
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
    return std::nullopt;
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
  return std::nullopt;
}

BlockInfo::BlockInfo(std::unordered_map<std::filesystem::path, std::filesystem::path>::const_iterator mount_info)
  : mount_info_iter(mount_info) {
  stat_file = find_block_stat(mount_info_iter->first);
}

} // namespace ds
