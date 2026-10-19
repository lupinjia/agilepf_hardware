#include "monitor/terminal_view.hpp"

#include <cassert>
#include <algorithm>
#include <clocale>
#include <limits>
#include <sstream>

static void check_width(const std::string& text, std::size_t width) {
  std::istringstream in(text);
  std::string line;
  while (std::getline(in, line)) assert(line.size() <= width - 1);
}

int main() {
  using namespace agilepf::monitor;
  MonitorSnapshot s;
  ViewConfig c;
  const auto waiting = renderText(s, c, false);
  assert(waiting.find("WAITING") != std::string::npos);
  assert(waiting.find("READ ONLY") != std::string::npos);
  assert(waiting.find("\033") == std::string::npos);
  assert(renderJson(s, c).find("\"status\":\"WAITING\"") != std::string::npos);
  s.received = true;
  s.message_count = 12;
  s.q = {1, 2, 3, 4, 5, 6};
  s.dq = {6, 5, 4, 3, 2, 1};
  s.quaternion = {1, .1, .2, .3};
  s.rpy = {.4, .5, .6};
  s.gyroscope = {7, 8, 9};
  s.accelerometer = {10, 11, 12};
  s.axes = {.11, .22, .33, .44};
  s.buttons[11] = true;
  auto text = renderText(s, c, false);
  assert(text.find("M0 1.000/6.000") != std::string::npos);
  assert(text.find("M3 4.000/3.000") != std::string::npos);
  assert(text.find("[1.000 0.100 0.200 0.300]") != std::string::npos);
  assert(text.find("RX 0.330") != std::string::npos);
  assert(text.find("r1:1") != std::string::npos);
  for (std::size_t w : {40u, 60u, 100u}) {
    c.columns = w;
    const auto view = renderText(s, c, false);
    check_width(view, w);
    assert(view.find(w >= 64 ? "M5 6.000/1.000" : "M5 6.000 / 1.000") != std::string::npos);
    assert(view.find("r1:1") != std::string::npos);
    assert(view.find("0.330") != std::string::npos && view.find("0.440") != std::string::npos);
    if (w >= 80) {
      assert(static_cast<std::size_t>(std::count(view.begin(), view.end(), '\n')) <= 23);
      assert(view.find("Sensor feedback freshness: UNKNOWN") != std::string::npos);
    }
  }
  s.stale = true;
  assert(renderText(s, c, false).find("DDS STALE") != std::string::npos);
  c.columns = 40;
  assert(renderText(s, c, false).find("M5 6.000 / 1.000") != std::string::npos);
  c.interface_name = "x\"\n\\y";
  s.q[0] = std::numeric_limits<double>::infinity();
  s.axes[0] = std::numeric_limits<double>::quiet_NaN();
  const auto json = renderJson(s, c);
  assert(json.find("x\\\"\\n\\\\y") != std::string::npos);
  assert(json.find("\"q\":null") != std::string::npos);
  assert(json.find("\"left_x\":null") != std::string::npos);
  std::setlocale(LC_NUMERIC, "C");
  assert(renderJson(s, c).find("0.22") != std::string::npos);
}
