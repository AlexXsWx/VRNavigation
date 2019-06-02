#pragma once

#include <string>

void logError(const char* const msg);
void log(const std::string& msg);
void log(const char * const msg);

template <class... Args>
void log(const char * const msg, Args&&... args) {
	printf(msg, args...);
	printf("\n");
}
