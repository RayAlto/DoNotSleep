#ifndef DONOTSLEEP_DO_NOT_SLEEP_BLOCK_INFO_H_
#define DONOTSLEEP_DO_NOT_SLEEP_BLOCK_INFO_H_

#include <cstdint>
#include <filesystem>
#include <unordered_map>
#include <utility>

namespace ds {

class BlockInfo {
public:
  BlockInfo() = delete;
  BlockInfo(const BlockInfo&) = default;
  BlockInfo(BlockInfo&&) noexcept = default;
  BlockInfo& operator=(const BlockInfo&) = default;
  BlockInfo& operator=(BlockInfo&&) noexcept = default;

  virtual ~BlockInfo() = default;

  static const std::pair<std::uint64_t, std::uint64_t> NO_IO;

  // e.g. `/mnt/usb_disk`
  static BlockInfo from_mount_path(const std::filesystem::path& mount_path);
  // e.g. `/dev/sda1`
  static BlockInfo from_block_path(const std::filesystem::path& block_path);
  // e.g. `sda1`
  static BlockInfo from_block_name(const std::filesystem::path& block_name);

  static std::pair<std::uint64_t, std::uint64_t> self_io_taken();

  // from the stat file
  [[nodiscard]] std::uint64_t total_reads() const;
  // from the stat file
  [[nodiscard]] std::uint64_t total_writes() const;
  // diff from last call or nullopt if unchanged
  std::uint64_t reads_taken();
  // diff from last call or nullopt if unchanged
  std::uint64_t writes_taken();
  // diff from last call or nullopt if unchanged
  std::pair<std::uint64_t, std::uint64_t> io_taken();

protected:
  static const std::filesystem::path MOUNT_INFO_PATH;
  static const std::filesystem::path SYS_BLOCK_PATH;
  static const std::filesystem::path BLOCK_STAT_NAME;
  static const std::filesystem::path SELF_IO;

  /* NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables) */
  static std::unordered_map<std::filesystem::path, std::filesystem::path> mount_list;
  /* NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables) */
  static std::pair<std::uint64_t, std::uint64_t> self_last_io;

  static void update_mount_list(const bool& force = false);
  static std::filesystem::path find_block_stat(const std::filesystem::path& block_device);
  // get read sectors and write sectors from stat file, return {-1, -1} on error
  static std::pair<std::uint64_t, std::uint64_t> get_io_statistics(const std::filesystem::path& stat_file);

  std::unordered_map<std::filesystem::path, std::filesystem::path>::const_iterator mount_info_iter;
  std::filesystem::path stat_file;
  std::pair<std::uint64_t, std::uint64_t> last_io;

  explicit BlockInfo(std::unordered_map<std::filesystem::path, std::filesystem::path>::const_iterator mount_info);

  // get read sectors and write sectors from stat file, return {-1, -1} on error
  [[nodiscard]] std::pair<std::uint64_t, std::uint64_t> get_io_statistics() const;
};

} // namespace ds

#endif // DONOTSLEEP_DO_NOT_SLEEP_BLOCK_INFO_H_
