#include "do_not_sleep/util.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

#include "netdb.h"
#include "netinet/in.h"
#include "sys/socket.h"
#include "sys/time.h"
#include "unistd.h"

namespace ds {

std::int64_t current_time_ms() {
  return current_time_ms(std::chrono::system_clock::now());
}

std::int64_t current_time_ms(const std::chrono::system_clock::time_point& t) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(t.time_since_epoch()).count();
}

const std::tm& localtime_safe(const std::time_t& t) {
  static std::mutex localtime_mutex;
  std::unique_lock<std::mutex> localtime_lock(localtime_mutex);
  /* NOLINTNEXTLINE(concurrency-mt-unsafe) */
  return *std::localtime(&t);
}

std::optional<std::string> getenv_safe(std::string_view key) {
  static std::mutex getenv_mutex;
  std::unique_lock<std::mutex> getenv_lock(getenv_mutex);
  /* NOLINTNEXTLINE(concurrency-mt-unsafe) */
  char* value = std::getenv(key.data());
  if (value == nullptr) {
    return std::nullopt;
  }
  return value;
}

bool service_available(const std::string_view& service, const std::int64_t& time_out) {
  std::size_t colon_pos = service.find(':');
  if (colon_pos == std::string::npos) {
    DS_LOGERR << "failed to parse service `" << service << "`, it should be `<IP>:<PORT>`\n";
    return false;
  }
  std::string ip{service.substr(0, colon_pos)};
  std::string port{service.substr(colon_pos + 1)};
  int ret{0};
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;
  hints.ai_protocol = IPPROTO_TCP;
  addrinfo* result{nullptr};
  ret = getaddrinfo(ip.c_str(), port.c_str(), &hints, &result);
  if (ret != 0) {
    DS_LOGERR << "failed to resolve service: " << gai_strerror(ret) << '\n';
    return false;
  }

  timeval timeout{};
  timeout.tv_sec = time_out / 1000;
  timeout.tv_usec = time_out % 1000;
  int socket_fd{0};
  addrinfo* result_ptr{nullptr};
  for (result_ptr = result; result_ptr != nullptr; result_ptr = result_ptr->ai_next) {
    socket_fd = socket(result_ptr->ai_family, result_ptr->ai_socktype, result_ptr->ai_protocol);
    if (socket_fd == -1) {
      continue;
    }

    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
      DS_LOGERR << "failed to set timeout\n";
    }
    if (setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == -1) {
      DS_LOGERR << "failed to set timeout\n";
    }

    if (connect(socket_fd, result_ptr->ai_addr, result_ptr->ai_addrlen) != -1) {
      // connected
      break;
    }

    // failed
    close(socket_fd);
  }

  freeaddrinfo(result);
  return result_ptr != nullptr;
}

std::ostream& log_error(const std::string_view& file, const std::uint_fast32_t& line, const bool& err) {
  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
  const std::tm& now_time_tm = localtime_safe(std::chrono::system_clock::to_time_t(now));
  std::int_fast64_t now_ms
    = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
  std::ostream& out = (err ? std::cerr : std::cout);
  const char fill_prev = out.fill();
  /* [YYYY-MM-DDThh:mm:ss.sss](__FILE__:__LINE__): MSG */
  return out << '[' << std::put_time(&now_time_tm, "%FT%T") << '.' << std::setfill('0') << std::setw(3) << now_ms
             << std::setfill(fill_prev) << "](" << file << ':' << line << "): ";
}

} // namespace ds
