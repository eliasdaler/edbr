#pragma once

#include <deque>
#include <functional>

class DeletionQueue {
public:
    void pushFunction(std::function<void()>&& f);
    void flush();

private:
    std::deque<std::function<void()>> deletors;
};
