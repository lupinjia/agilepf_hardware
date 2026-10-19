#pragma once

#include "monitor_types.hpp"

#include <array>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>

namespace agilepf::monitor {
namespace detail {

inline std::string number(double value, int precision = 3) {
  if (!std::isfinite(value)) return "--";
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << std::fixed << std::setprecision(precision) << value;
  return out.str();
}

inline std::string safe(std::string value) {
  for (char& c : value) {
    const unsigned char u = static_cast<unsigned char>(c);
    if (u < 0x20 || u == 0x7f) c = ' ';
  }
  return value;
}

inline std::string jsonString(const std::string& value) {
  std::ostringstream out;
  out << '"';
  static constexpr char hex[] = "0123456789abcdef";
  for (unsigned char c : value) {
    switch (c) {
      case '"': out << "\\\""; break;
      case '\\': out << "\\\\"; break;
      case '\b': out << "\\b"; break;
      case '\f': out << "\\f"; break;
      case '\n': out << "\\n"; break;
      case '\r': out << "\\r"; break;
      case '\t': out << "\\t"; break;
      default:
        if (c < 0x20) out << "\\u00" << hex[c >> 4] << hex[c & 15];
        else out << static_cast<char>(c);
    }
  }
  out << '"';
  return out.str();
}

inline std::string jsonNumber(double value) {
  if (!std::isfinite(value)) return "null";
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << std::setprecision(15) << value;
  return out.str();
}

inline void jsonArray(std::ostringstream& out, const double* values,
                      std::size_t size, bool available) {
  out << '[';
  for (std::size_t i = 0; i < size; ++i) {
    if (i) out << ',';
    out << (available ? jsonNumber(values[i]) : "null");
  }
  out << ']';
}

inline std::string vectorText(const double* values, std::size_t count,
                              bool available, int precision = 3) {
  std::ostringstream out;
  out << '[';
  for (std::size_t i = 0; i < count; ++i) {
    if (i) out << ' ';
    out << (available ? number(values[i], precision) : "--");
  }
  out << ']';
  return out.str();
}

}  // namespace detail

inline std::string renderText(const MonitorSnapshot& s, const ViewConfig& c,
                              bool ansi) {
  const bool available = s.received;
  const bool compact = c.columns < 64;
  const std::size_t width = c.columns > 40 ? c.columns - 1 : 39;
  const std::string reset = ansi ? "\033[0m" : "";
  const std::string accent = ansi ? "\033[1;36m" : "";
  std::ostringstream out;
  if (ansi) out << "\033[H\033[J";
  auto title = [&](const char* text) { out << accent << std::string(text).substr(0, width) << reset << '\n'; };
  auto line = [&](const std::string& text) {
    if (text.size() <= width) out << text << '\n';
    else for (std::size_t p = 0; p < text.size(); p += width) out << text.substr(p, width) << '\n';
  };
  title("AGILEPF  /  LOW-STATE MONITOR");
  line("READ ONLY  |  DDS " + std::string(!s.received ? "WAITING" : s.stale ? "STALE" : "RECEIVING"));
  line("domain " + std::to_string(c.domain) + "  interface " + detail::safe(c.interface_name));
  line("topic " + detail::safe(c.topic));
  if (!compact) {
    line("messages " + std::to_string(s.message_count) + "   rate " + (available ? detail::number(s.rx_hz, 1) : "--") +
         " Hz   age " + (available ? detail::number(s.age_ms, 1) : "--") + " ms");
    line("last interval " + (available ? detail::number(s.last_interval_ms, 1) : "--") + " ms");
  } else {
    line("msgs " + std::to_string(s.message_count) + " rate " + (available ? detail::number(s.rx_hz, 1) : "--") +
         "Hz age " + (available ? detail::number(s.age_ms, 1) : "--") + "ms");
  }
  title("MOTORS / q,dq driver units (verify firmware units)");
  if (!compact) {
    for (int i = 0; i < 3; ++i) {
      auto motor = [&](int n) { return "M" + std::to_string(n) + " " +
        (available ? detail::number(s.q[n]) : "--") + "/" +
        (available ? detail::number(s.dq[n]) : "--"); };
      line("L " + motor(i) + "       R " + motor(i + 3));
    }
  } else {
    for (int i = 0; i < 6; ++i)
      line(std::string(i < 3 ? "LEFT  " : "RIGHT ") + "M" + std::to_string(i) + " " +
           (available ? detail::number(s.q[i]) : "--") + " / " +
           (available ? detail::number(s.dq[i]) : "--"));
  }
  title("IMU / feedback freshness UNKNOWN (not published)");
  line("quat WXYZ " + detail::vectorText(s.quaternion.data(), 4, available));
  line("RPY rad " + detail::vectorText(s.rpy.data(), 3, available));
  line("gyro rad/s " + detail::vectorText(s.gyroscope.data(), 3, available));
  line("accel m/s^2 " + detail::vectorText(s.accelerometer.data(), 3, available));
  title("GAMEPAD");
  if (!compact) {
    line("axes LX " + (available ? detail::number(s.axes[0]) : "--") + " LY " + (available ? detail::number(s.axes[1]) : "--") +
         " RX " + (available ? detail::number(s.axes[2]) : "--") + " RY " + (available ? detail::number(s.axes[3]) : "--"));
  } else {
    line("axes LX " + (available ? detail::number(s.axes[0]) : "--") + " LY " + (available ? detail::number(s.axes[1]) : "--"));
    line("axes RX " + (available ? detail::number(s.axes[2]) : "--") + " RY " + (available ? detail::number(s.axes[3]) : "--"));
  }
  static constexpr const char* names[] = {"start","select","left","right","up","down","a","b","x","y","l1","r1"};
  const std::size_t group_count = compact ? 3 : 2;
  const std::size_t group_size = compact ? 4 : 6;
  for (std::size_t group = 0; group < group_count; ++group) {
    std::string buttons = "buttons ";
    for (std::size_t j = group * group_size; j < (group + 1) * group_size; ++j) {
      if (j != group * group_size) buttons += ' ';
      buttons += names[j]; buttons += ':';
      buttons += available ? (s.buttons[j] ? '1' : '0') : '-';
    }
    line(buttons);
  }
  line("Sensor feedback freshness: UNKNOWN");
  line("DDS arrival does not confirm fresh sensor feedback.");
  line("Ctrl+C to quit.");
  return out.str();
}

inline std::string renderJson(const MonitorSnapshot& s, const ViewConfig& c) {
  const bool available = s.received;
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << "{\"status\":" << detail::jsonString(!s.received ? "WAITING" : s.stale ? "STALE" : "RECEIVING")
      << ",\"read_only\":true,\"config\":{\"domain\":" << c.domain
      << ",\"interface\":" << detail::jsonString(c.interface_name)
      << ",\"topic\":" << detail::jsonString(c.topic)
      << ",\"refresh_hz\":" << detail::jsonNumber(c.refresh_hz)
      << ",\"columns\":" << c.columns << "},\"message_count\":" << s.message_count
      << ",\"rx_hz\":" << (available ? detail::jsonNumber(s.rx_hz) : "null")
      << ",\"age_ms\":" << (available ? detail::jsonNumber(s.age_ms) : "null")
      << ",\"last_interval_ms\":" << (available ? detail::jsonNumber(s.last_interval_ms) : "null")
      << ",\"motors\":{\"units\":\"driver units; firmware verification pending\",\"left\":[";
  for (int i = 0; i < 3; ++i) {
    if (i) out << ',';
    out << "{\"q\":" << (available ? detail::jsonNumber(s.q[i]) : "null") << ",\"dq\":" << (available ? detail::jsonNumber(s.dq[i]) : "null") << '}';
  }
  out << "],\"right\":[";
  for (int i = 3; i < 6; ++i) {
    if (i != 3) out << ',';
    out << "{\"q\":" << (available ? detail::jsonNumber(s.q[i]) : "null") << ",\"dq\":" << (available ? detail::jsonNumber(s.dq[i]) : "null") << '}';
  }
  out << "]},\"imu\":{\"quaternion_wxyz\":"; detail::jsonArray(out, s.quaternion.data(), 4, available);
  out << ",\"rpy_rad\":"; detail::jsonArray(out, s.rpy.data(), 3, available);
  out << ",\"gyroscope_rad_s\":"; detail::jsonArray(out, s.gyroscope.data(), 3, available);
  out << ",\"accelerometer_m_s2\":"; detail::jsonArray(out, s.accelerometer.data(), 3, available);
  out << ",\"feedback_freshness\":\"unknown\"},\"gamepad\":{\"axes\":{\"left_x\":"
      << (available ? detail::jsonNumber(s.axes[0]) : "null") << ",\"left_y\":" << (available ? detail::jsonNumber(s.axes[1]) : "null")
      << ",\"right_x\":" << (available ? detail::jsonNumber(s.axes[2]) : "null") << ",\"right_y\":" << (available ? detail::jsonNumber(s.axes[3]) : "null") << "},\"buttons\":{";
  static constexpr const char* names[] = {"start","select","left","right","up","down","a","b","x","y","l1","r1"};
  for (std::size_t i = 0; i < 12; ++i) {
    if (i) out << ',';
    out << detail::jsonString(names[i]) << ':';
    if (available) out << (s.buttons[i] ? "true" : "false"); else out << "null";
  }
  out << "}}}";
  return out.str();
}

}  // namespace agilepf::monitor
