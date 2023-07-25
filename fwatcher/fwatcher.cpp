#include "fwatcher.h"
#include <boost/filesystem.hpp>
#include <fstream>
#include <map>
#include <spdlog/spdlog.h>
#include <thread>
#include <unordered_map>

namespace fs = boost ::filesystem;

struct PathHash {
  std::size_t operator()(const fs::path &p) const {
#if defined(__arm64__) || defined(__aarch64__)
    // Use standard string hash on ARM architectures
    // Was getting errors for normal hash
    // Could be optimised more stringview etc..
    return std::hash<std::string>{}(p.string());
#else
    // Use Boost's hash on other architectures
    return boost::filesystem::hash_value(p);
#endif
  }
};

std::unordered_map<fs::path, std::time_t, PathHash> lastWriteMap;

void processFile(const fs::path &path) {
  std::string ext = path.extension().string();
  spdlog::debug("Processing file {}\n", path.string());
  spdlog::debug("File ext: {}\n", ext);

  fs::path spvPath = path;
  spvPath.replace_extension(".spv");

  std::string command = fmt::format("glslangValidator -V {} -o {}",
                                    path.string(), spvPath.string());

  int result = system(command.c_str());
  spdlog::debug("Result: {}\n", result);
}

bool checkChanges(const fs::path &path) {
  std::string ext = path.extension().string();

  if (!(ext == ".frag" || ext == ".vert" || ext == ".geom")) {
    return false;
  }
  std::time_t lastWriteTime = fs::last_write_time(path);
  spdlog::debug("Last Write Time: {} for file: {}\n", lastWriteTime,
                path.string());

  auto it = lastWriteMap.find(path);
  if (it == lastWriteMap.end() || it->second != lastWriteTime) {
    spdlog::debug("File {} changed\n", path.string());
    processFile(path);
    lastWriteMap[path] = lastWriteTime;
    return true;
  }
  return false;
}

bool watchChanges(const fs::path &path) {
  bool changesDetected = false;
  if (fs::is_directory(path)) {
    for (const auto &entry : fs::directory_iterator(path)) {
      changesDetected |= checkChanges(entry.path());
    }
  }
  return changesDetected;
}

FWatcher::FWatcher(std::string pathToWatch,
                   std::chrono::duration<int, std::milli> interval,
                   std::function<void()> callback)
    : pathToWatch{pathToWatch}, interval{interval}, callback{callback} {}

void FWatcher::start() {
  spdlog::debug("Watching files in {}", pathToWatch);

  std::thread([this]() {
    while (true) {
      std::this_thread::sleep_for(interval);
      if (watchChanges(fs::path{pathToWatch}))
        callback();
    }
  }).detach();
}
