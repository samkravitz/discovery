/**
 * discovery::CircularBuffer.cpp
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Noah Bennett
 * Date: Oct 18, 2021
 * Description: CircularBuffer for audio
 */

#include "CircularBuffer.h"
#include <iostream>

template <typename T>
CircularBuffer<T>::CircularBuffer(size_t size):
_size(size),
_data(new T[size])
{
  this->_front = -1;
  this->_rear = -1;
}

template <typename T>
CircularBuffer<T>::~CircularBuffer()
{
}

template <typename T>
T CircularBuffer<T>::front() 
{
  try 
  {
    return this->_data[this->_front];
  }
  catch(...) 
  {
    std::cout << "Error accessing front" << std::endl;
    exit(1);
  }
}

template <typename T>
s16 CircularBuffer<T>::fronti()
{
  return this->_front;
}


template <typename T>
T CircularBuffer<T>::rear()
{
  try
  {
    return this->_data[this->_rear];
  }
  catch(...)
  {
    std::cout << "Error accessing front" << std::endl;
    exit(1);
  }
}

template <typename T>
s16 CircularBuffer<T>::reari() 
{
  return this->_rear;
}

template <typename T>
T CircularBuffer<T>::cursor() 
{
  try
  {
    return this->_data[this->_cursor];
  }
  catch(...)
  {
    std::cout << "Error accessing cursor" << std::endl;
    exit(1);
  }
}

template <typename T>
s16 CircularBuffer<T>::cursori() 
{
  return this->_cursor;
}

template <typename T>
size_t CircularBuffer<T>::size()
{
  return this->_size;
}

template <typename T>
void CircularBuffer<T>::enqueue(T el) 
{
  this->increment_rear(); 
  if(this->_cursor > this->_size)
  {
    this->increment_front();
  } 
  this->_data[this->_rear] = el;
}

template <typename T>
T CircularBuffer<T>::dequeue() 
{
  
}

template <typename T>
void CircularBuffer<T>::fill(T)
{
  
}

template <typename T>
void CircularBuffer<T>::clear() 
{
  this->_front = 1;
  this->_rear = this->_cursor = 0;
}

template <typename T>
bool CircularBuffer<T>::is_empty()
{
  return (this->_cursor == 0);
}

template <typename T>
bool CircularBuffer<T>::is_full()
{
  return (this->_cursor == this->_size);
}

template <typename T>
T CircularBuffer<T>::at(s16 index)
{
  return this->_data[index];
}

template <typename T>
void CircularBuffer<T>::increment_front() 
{
  if(this->_cursor > 0) 
  {
    this->_front += 1;
    this->_cursor -= 1;
  }
  if(this->_front == this->_size) this->_front = 0;
}

template <typename T>
void CircularBuffer<T>::increment_rear() 
{
  this->_rear += 1;
  this->_cursor += 1;
  if(this->_rear == this->_size) this->_rear = 0;
}

template <typename T>
T *CircularBuffer<T>::data() 
{
  return this->_data;
}

template <typename T>
void CircularBuffer<T>::__set_rear_unsafe(s16 i)
{
  if(i >= this->_size) 
  {
    this->_rear = i % this->_size;
  }
  else 
  {
    this->_rear = i;
  }
}

template <typename T>
void CircularBuffer<T>::__set_cursor_unsafe(s16 i)
{
  if(i >= this->_size) 
  {
    this->_cursor = i % this->_size;
  }
  else 
  {
    this->_cursor = i;
  }
}
