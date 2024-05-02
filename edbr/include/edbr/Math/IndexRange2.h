#pragma once

#include <glm/vec2.hpp>

namespace math
{
class IndexRange2 {
public:
    IndexRange2();
    IndexRange2(const glm::ivec2& startIndex, const glm::ivec2& numberOfIndices);

    class Iterator {
    public:
        Iterator() = default;
        Iterator(const glm::ivec2& v, int maxX);
        Iterator& operator++();
        bool operator!=(const Iterator& other) const;
        const glm::ivec2& operator*() const { return v; }

    private:
        glm::ivec2 v;
        int minX, maxX; // v.x go in range [minX, maxX]
    };

    Iterator begin() const;
    Iterator end() const;

    const glm::ivec2& getStartIndex() const { return startIndex; }
    glm::ivec2 getEndIndex() const { return startIndex + numberOfIndices; } // inclusive

    const glm::ivec2& getNumberOfIndices() const { return numberOfIndices; }

    int getMaxX() const { return startIndex.x + numberOfIndices.x - 1; }
    int getNumberOfRows() const { return numberOfIndices.y; }
    int getNumberOfColumns() const { return numberOfIndices.x; }

    glm::ivec2 getRelativeIndex(const glm::ivec2& index) const;

private:
    glm::ivec2 startIndex;
    glm::ivec2 numberOfIndices;
};

}
