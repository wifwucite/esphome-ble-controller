#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

namespace esphome {
namespace esp32_ble_controller {

template <typename T>
class ThreadSafeBoundedQueue {
public:
  ThreadSafeBoundedQueue(unsigned int size);

  void push(T&& object);

  boolean take(T& object);

private:
  const char* getTag() { return "thread_safe_bounded_queue"; }

private:
  QueueHandle_t queue;
};

template <typename T>
ThreadSafeBoundedQueue<T>::ThreadSafeBoundedQueue(unsigned int size) {
  queue = xQueueCreate( size, sizeof( void* ) );
  if (queue == NULL) {
    ESP_LOGE(getTag(), "Could not create RTOS queue");
  }
}

template <typename T>
void ThreadSafeBoundedQueue<T>::push(T&& object) {
  T* pointer_to_copy = new T();
  *pointer_to_copy = std::move(object);

  // add the pointer to the queue, not the object itself
  auto result = xQueueSend(queue, &pointer_to_copy, 20L / portTICK_PERIOD_MS);
  if (result != pdPASS) {
    ESP_LOGW(getTag(), "Bounded queue is full");
  }
}

template <typename T>
boolean ThreadSafeBoundedQueue<T>::take(T& object) {
  T* pointer_to_object; 
  auto result = xQueueReceive(queue, &pointer_to_object, 0);
  if (result != pdPASS) {
    return false;
  }

  object = std::move(*pointer_to_object);
  delete pointer_to_object;
  return true;
}

} // namespace esp32_ble_controller
} // namespace esphome
