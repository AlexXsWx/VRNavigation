/*

MIT License

Copyright (c) 2018 Shalitha Suranga

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

// https://github.com/shalithasuranga/timercpp

#pragma once

#include <iostream>
#include <thread>
#include <chrono>

class Timer {
	bool clear = false;

public:
	template <class F>
	void setTimeout(F&& function, int delay);

	template <class F>
	void setInterval(F&& function, int interval);

	void stop();

};

template <class F>
void Timer::setTimeout(F&& function, int delay) {
	this->clear = false;
	std::thread t([=]() mutable {
		if (this->clear) return;
		std::this_thread::sleep_for(std::chrono::milliseconds(delay));
		if (this->clear) return;
		function();
	});
	t.detach();
}

template <class F>
void Timer::setInterval(F&& function, int interval) {
	this->clear = false;
	std::thread t([=]() mutable {
		while (true) {
			if (this->clear) return;
			std::this_thread::sleep_for(std::chrono::milliseconds(interval));
			if (this->clear) return;
			function();
		}
	});
	t.detach();
}
