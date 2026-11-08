#include "check.hpp"
#include "fake_api.hpp"

int main() {
  for(auto &c : check::cases()) {
    fake::reset();
    int before = check::failures();
    c.fn();
    std::printf("%s %s\n", check::failures() == before ? "ok  " : "FAIL", c.name);
  }
  return check::failures() ? 1 : 0;
}
