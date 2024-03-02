#include "DeletionQueue.h"

void DeletionQueue::pushFunction(std::function<void(VkDevice)>&& f)
{
    deletors.push_back(std::move(f));
}

void DeletionQueue::flush(VkDevice device)
{
    for (auto it = deletors.rbegin(); it != deletors.rend(); ++it) {
        (*it)(device);
    }
    deletors.clear();
}
