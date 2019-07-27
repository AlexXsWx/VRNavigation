#pragma once

#include <list>
#include <functional>

template<typename T>
class Stream {
    private:
        std::function<bool(const T & value)> filter;
        std::function<bool(const T & value, const std::list<T> & all)> isTooOld;

        std::list<T> list;

    public:
        Stream(
            std::function<bool(const T & value)> filter,
            std::function<bool(const T & value, const std::list<T> & all)> isTooOld
        )
        : filter(filter)
        , isTooOld(isTooOld)
        {}

        inline auto empty()  { return this->list.empty();  }
        inline auto back()   { return this->list.back();   }
        inline auto rbegin() { return this->list.rbegin(); }
        inline auto rend()   { return this->list.rend();   }

        void feed(T && value) {
			if (this->filter(value)) {
				this->list.push_back(value);
			}
			this->removeOld();
		}

        void removeOld() {
			while (
                !this->list.empty() &&
                this->isTooOld(this->list.front(), this->list)
            ) {
				this->list.pop_front();
			}
		}
};
