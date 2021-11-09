/**
 * discovery::CircularBuffer.h
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Noah Bennett
 * Date: Oct 18, 2021
 * Description: CircularBuffer for audio
 */

#pragma once

#include "common.h"
#include <string>

template <typename T>
class CircularBuffer {
  public:
  typedef T type;
  
  // construct with size
  CircularBuffer(size_t);  

  // destructor
  ~CircularBuffer(void);

  // place item in buffer
  void emplace(T);

  private:

  size_t _size;

  s16 _front, _rear, _cursor;
  
  T *_data;

  void increment_front(void);

  void increment_rear(void);
  
};

// compile a template for AUDIO_S16 sound buffers
// eventually this might need to be changed
// to accomodate other driver types
// see https://wiki.libsdl.org/SDL_AudioFormat
// for all types
template class CircularBuffer<s16>;
