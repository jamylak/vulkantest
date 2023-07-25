/**
 * Used to watch and live recompile shaders
 * Very simple file watching implementation
 **/
#include <string>

class FWatcher {
private:
  std::string pathToWatch;
  std::chrono::duration<int, std::milli> interval;
  std::function<void()> callback;

public:
  FWatcher(std::string pathToWatch,
           std::chrono::duration<int, std::milli> interval,
           std::function<void()> callback);
  void start();
};
