
#include "do_not_sleep/ds.h"

// NOLINTNEXTLINE(misc-unused-parameters)
int main(int argc, const char* argv[]) {
  ds::DoNotSleep ds{};
  ds.start();
  return 0;
}
