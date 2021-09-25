
#pragma once

#include <vector>
#include <functional>
#include <tuple>
#include "mmio.h"

class Watcher {
  public:
  
  Watcher(void);

  ~Watcher(void);

  // add register to watcher
  void add(u32, std::function<void(u32, u32)>);

  // check if a register is being watched
  bool isWatching(u32);

  // call the function paired with the regester passed as a parameter
  void checkRegister(u32, u32);

  private:

  // registers being watched, with respective callback
  std::vector<std::tuple<u32, std::function<void(u32, u32)>>> watching;

};


// watching: vector of tuples
//  - (register, function())
