#include "DeletionQueue.h"

void DeletionQueue::pushFunction(std::function<void()>&& f)
{
    deletors.push_back(std::move(f));
}

void DeletionQueue::flush()
{
    for (auto it = deletors.rbegin(); it != deletors.rend(); ++it) {
        (*it)();
    }
    deletors.clear();
}
