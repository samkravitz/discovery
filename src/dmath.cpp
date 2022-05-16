#include <iostream>
#include "dmath.h"

/**
 * fourier transform
 */
std::vector<std::complex<double>> dmath::fourier(std::vector<std::complex<double>> x) {
  size_t size = x.size();
  if(size <= 1) return x;

  // divide by even / odd index values
  std::vector<std::complex<double>> evens, odds;
  for(u16 i = 0; i < size; i++) {
    if(i % 2) {
      odds.push_back(x[i]);
    } else {
      evens.push_back(x[i]);
    }
  }

  // solve recursively
  evens = dmath::fourier(evens);
  odds = dmath::fourier(odds);

  // merge
  std::vector<std::complex<double>> merged(x);
  for(size_t i = 0; i < size / 2; ++i) {
    std::complex<double> t = std::polar(1., -2 * M_PI * ( i / size )) * odds[i];
    merged[i] = evens[i] + t;
    merged[i+(size/2)] = evens[i] - t;
  }
  return merged;
}

