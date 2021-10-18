/**
 * discovery::lib::CircularBuffer.inl
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Noah Bennett
 * Date: Oct 18, 2021
 * Description: CircularBuffer for audio
 */

<template typename T>
class CircularBuffer {
  public:
  typedef T type;
  
  // construct with size
  CircularBuffer(size_t);  

  // construct with size and value
  CircularBuffer(size_t, T);
  
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

  private:

  size_t size;

  s16 front, rear;
  
  std::unique_ptr<T> data;
  
};

