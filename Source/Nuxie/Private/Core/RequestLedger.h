#pragma once
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace nuxie {
// Exact decimal counters never traverse a JSON double or signed engine pin.
inline std::optional<std::uint64_t> counter(const std::string& value) {
  if (value.empty() || (value.size() > 1 && value.front() == '0')) return {};
  std::uint64_t result = 0;
  for (char c : value) {
    if (c < '0' || c > '9') return {};
    auto digit = static_cast<std::uint64_t>(c - '0');
    if (result > (UINT64_MAX - digit) / 10) return {};
    result = result * 10 + digit;
  }
  return result;
}
inline bool quantity(std::int64_t value) { return value > 0 && value <= 9007199254740991LL; }
struct Request { std::uint64_t identity; bool durable; double deadline; };
class RequestLedger {
public:
  bool admit(std::string id, Request request) { return pending.emplace(std::move(id), request).second; }
  std::optional<Request> take(const std::string& id) {
    auto found = pending.find(id); if (found == pending.end()) return {};
    auto result = found->second; pending.erase(found); return result;
  }
  std::vector<std::string> expired(double now) const {
    std::vector<std::string> ids;
    for (const auto& pair : pending) if (pair.second.deadline <= now) ids.push_back(pair.first);
    return ids;
  }
  std::vector<std::string> all() const {
    std::vector<std::string> ids; for (const auto& pair : pending) ids.push_back(pair.first); return ids;
  }
  static bool deliverable(const Request& request, std::uint64_t identity) {
    return request.durable || request.identity == identity;
  }
private:
  std::map<std::string, Request> pending;
};
}
