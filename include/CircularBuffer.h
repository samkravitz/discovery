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

  // get front element
  T front(void);

  // get front index
  s16 fronti(void);

  // get rear element
  T rear(void);

  // get rear index
  s16 reari(void);

  // get element at cursor
  T cursor(void);

  // get cursor index
  s16 cursori(void);

  // get size
  size_t size(void);

  // place element at rear
  void enqueue(T);

  // remove element at front
  T dequeue(void);

  // fill with data
  void fill(T);

  // clear data
  void clear(void);

  // is empty
  bool is_empty(void);

  // is full
  bool is_full(void);

  // element at index
  T at(s16);

  // access buffer data
  T *data(void);

  void __set_rear_unsafe(s16);

  void __set_cursor_unsafe(s16);

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
