#pragma once

#include <thread>
#include <atomic>
#include <future>

template <class F, class... Args>
void setInterval(std::atomic_bool& cancelToken, size_t interval, F&& f, Args&&... args);
