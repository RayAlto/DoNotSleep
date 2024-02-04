#ifndef DONOTSLEEP_DO_NOT_SLEEP_BLOCK_INFO_H_
#define DONOTSLEEP_DO_NOT_SLEEP_BLOCK_INFO_H_

#include <filesystem>
#include <optional>
#include <unordered_map>

namespace ds {

class BlockInfo {
public:
  BlockInfo() = delete;
  BlockInfo(const BlockInfo&) = default;
  BlockInfo(BlockInfo&&) noexcept = default;
  BlockInfo& operator=(const BlockInfo&) = default;
  BlockInfo& operator=(BlockInfo&&) noexcept = default;

  virtual ~BlockInfo() = default;

  // e.g. `/mnt/usb_disk`
  static std::optional<BlockInfo> from_mount_path(const std::filesystem::path& mount_path);
  // e.g. `/dev/sda1`
  static std::optional<BlockInfo> from_block_path(const std::filesystem::path& block_path);
  // e.g. `sda1`
  static std::optional<BlockInfo> from_block_name(const std::filesystem::path& block_name);

protected:
  static const std::filesystem::path MOUNT_INFO_PATH;
  static const std::filesystem::path SYS_BLOCK_PATH;
  static const std::filesystem::path BLOCK_STAT_NAME;

  /* NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables) */
  static std::unordered_map<std::filesystem::path, std::filesystem::path> mount_list;

  static void update_mount_list(const bool& force = false);
  static std::optional<std::filesystem::path> find_block_stat(const std::filesystem::path& block_device);

  std::unordered_map<std::filesystem::path, std::filesystem::path>::const_iterator mount_info_iter;
  std::optional<std::filesystem::path> stat_file;

  explicit BlockInfo(std::unordered_map<std::filesystem::path, std::filesystem::path>::const_iterator mount_info);
};

} // namespace ds

#endif // DONOTSLEEP_DO_NOT_SLEEP_BLOCK_INFO_H_
