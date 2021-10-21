/**
 * discovery::CircularBuffer.cpp
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Noah Bennett
 * Date: Oct 18, 2021
 * Description: CircularBuffer for audio
 */

#include "CircularBuffer.h"

template <typename T>
CircularBuffer<T>::CircularBuffer(size_t size):
_size(size),
data(new T[size])
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
    return this->data[this->front_];
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
    return this->data[this->_rear];
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
    this->increment_head();
  } 
  this->data[this->_rear] = el;
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
  return this->data[index];
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


