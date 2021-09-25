#include <iostream>
#include <tuple>
#include "Watcher.h"

Watcher::Watcher() {
  std::cout<<"constructing watcher"<<std::endl;
  this->watching = std::vector<std::tuple<u32, std::function<void(u32, u32)>>>();
}

Watcher::~Watcher() {

}

void Watcher::add(u32 reg, std::function<void(u32, u32)> callback) {
  std::tuple<u32, std::function<void(u32, u32)>> watch = std::make_tuple(reg, callback);
  this->watching.push_back(watch);
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
