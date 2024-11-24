#pragma once

#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace util {

std::optional<std::string_view> GetCommand(std::string_view line) {
  std::cout << line << std::endl;
  size_t first_bar = line.find('|') + 1;
  if (first_bar >= line.size()) {
    return std::nullopt;
  }
  size_t second_bar = line.substr(first_bar).find('|');
  std::cout << first_bar << " " << second_bar << std::endl;
  if (second_bar >= line.size()) {
    return std::nullopt;
  }
  std::cout << line.substr(first_bar, second_bar) << std::endl;
  return line.substr(first_bar, second_bar);
}

// Split a line by the '|' delimiter.
std::vector<std::string_view> SplitLine(std::string_view line,
                                        char delim = '|') {
  std::vector<std::string_view> result;
  size_t delim_idx = line.find(delim);
  size_t prev_idx = 0;
  while (prev_idx < line.size()) {
    std::string_view substring = line.substr(prev_idx, delim_idx - prev_idx);
    result.push_back(substring);
    // If the delim_idx is the end of the string, set it to npos.
    prev_idx = (delim_idx == std::string_view::npos) ? std::string_view::npos
                                                     : delim_idx + 1;
    delim_idx = line.find(delim, delim_idx + 1);
  }
  return result;
}

}  // namespace util
