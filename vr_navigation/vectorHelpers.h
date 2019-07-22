#pragma once

#include <vector>

template <typename Container, typename Predicate>
bool some(const Container & container, Predicate predicate) {
    return std::any_of(container.begin(), container.end(), predicate);
}

template <typename Container, typename Predicate>
bool every(const Container & container, Predicate predicate) {
    return std::all_of(container.begin(), container.end(), predicate);
}

template <typename Container, typename Predicate>
Container filter(const Container & container, Predicate predicate) {
    Container result;
    std::copy_if(
        container.begin(),
        container.end(),
        std::back_inserter(result),
        predicate
    );
    return result;
}

// TODO: more generic types
template <typename Content, typename Predicate>
Content * find(std::vector<Content> & container, Predicate predicate) {
    const auto it = std::find_if(container.begin(), container.end(), predicate);
    return (
        (it == container.end())
            ? nullptr
            // TODO: understand this
            : &(*it)
    );
}
