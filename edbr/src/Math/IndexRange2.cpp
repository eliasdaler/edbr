#include <edbr/Math/IndexRange2.h>

namespace math
{
IndexRange2::IndexRange2() = default;

IndexRange2::IndexRange2(const glm::ivec2& startIndex, const glm::ivec2& numberOfIndices) :
    startIndex(startIndex), numberOfIndices(numberOfIndices)
{}

IndexRange2::Iterator IndexRange2::begin() const
{
    return Iterator(startIndex, getMaxX());
}

IndexRange2::Iterator IndexRange2::end() const
{
    return Iterator(glm::ivec2(startIndex.x, startIndex.y + numberOfIndices.y), getMaxX());
}

glm::ivec2 IndexRange2::getRelativeIndex(const glm::ivec2& index) const
{
    return index - startIndex;
}

IndexRange2::Iterator::Iterator(const glm::ivec2& v, int maxX) : v(v), minX(v.x), maxX(maxX)
{}

IndexRange2::Iterator& IndexRange2::Iterator::operator++()
{
    if (v.x < maxX) {
        ++v.x;
    } else { // go to next row
        v.x = minX;
        ++v.y;
    }

    return *this;
}

bool IndexRange2::Iterator::operator!=(const Iterator& other) const
{
    return v != other.v;
}
}
