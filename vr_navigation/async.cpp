#include "stdafx.h"
#include "async.h"

template <class F, class... Args>
void setInterval(std::atomic_bool& cancelToken, size_t interval, F&& f, Args&&... args) {
	cancelToken.store(true);
	auto cb = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
	std::async(std::launch::async, [=, &cancelToken]()mutable {
		while (cancelToken.load()) {
			cb();
			std::this_thread::sleep_for(std::chrono::milliseconds(interval));
		}
	});
}
