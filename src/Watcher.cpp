#include <iostream>
#include <tuple>
#include "Watcher.h"

Watcher::Watcher() {
  this->watching = std::vector<std::tuple<u32, std::function<void(u32, u32)>>>();
}

Watcher::~Watcher() {

}

void Watcher::add(u32 reg, std::function<void(u32, u32)> callback) {
  std::tuple<u32, std::function<void(u32, u32)>> watch = std::make_tuple(reg, callback);
  this->watching.push_back(watch);

  // std::sort(this->watching.begin(), this->watching.end(), [](const std::tuple<T, T, std::function<void(T, T)>> &a, const std::tuple<T, T, std::function<void(T, T)>> &b) -> bool {
  //   return std::get<1>(a) > std::get<1>(b);
  // });
}

bool Watcher::isWatching(u32 reg) {
  for(auto &watchTup : this->watching) {
    if(std::get<0>(watchTup) == reg) {
      return true;
    }
  }
  return false;
}

void Watcher::checkRegister(u32 reg, u32 val) {
  for(auto &watchTup : this->watching) {
    if(std::get<0>(watchTup) == reg) {
      std::function<void (u32, u32)> callback = std::get<1>(watchTup);
      callback(reg, val);
    }
  }
}
