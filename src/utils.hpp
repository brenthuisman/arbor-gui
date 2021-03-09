#pragma once

#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>

#include <glm/glm.hpp>
#include <imgui.h>

#include <spdlog/spdlog.h>

// Logging
void log_init();

template <typename... T> void log_debug(T... t) { spdlog::debug(t...); }
template <typename... T> void log_info(T... t)  { spdlog::info(t...); }
template <typename... T> void log_warn(T... t)  { spdlog::warn(t...); }
template <typename... T> void log_error(T... t) { spdlog::error(t...); throw std::runtime_error(fmt::format(t...)); }
template <typename... T> void log_fatal(T... t) { spdlog::error(t...); abort(); }

ImVec4 to_imvec(const glm::vec4& v);

glm::vec2 to_glmvec(const ImVec2& v);

template <typename T> std::string to_string(const T &r) {
  std::stringstream ss;
  ss << r;
  return ss.str();
}

std::string slurp(const std::filesystem::path& fn);

constexpr float PI = 3.141f;

// trim from start (in place)
inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

// trim from start (copying)
inline std::string ltrim_copy(std::string s) {
    ltrim(s);
    return s;
}

// trim from end (copying)
inline std::string rtrim_copy(std::string s) {
    rtrim(s);
    return s;
}

// trim from both ends (copying)
inline std::string trim_copy(std::string s) {
    trim(s);
    return s;
}
