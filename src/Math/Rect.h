#pragma once

#include <glm/vec2.hpp>

namespace math
{
template<typename T>
struct Rect {
    Rect(); // zero-initialized
    Rect(T left, T top, T with, T height);

    template<typename U>
    explicit Rect(const Rect<U>& r);

    Rect(const glm::vec<2, T>& position, const glm::vec<2, T>& size);

    bool contains(const glm::vec<2, T>& point) const;

    bool intersects(const Rect<T>& rect) const;

    glm::vec<2, T> getPosition() const;

    void setPosition(const glm::vec<2, T>& position);

    glm::vec<2, T> getSize() const;
    void setSize(const glm::vec<2, T>& size);

    glm::vec<2, T> getTopLeftCorner() const;
    glm::vec<2, T> getTopRightCorner() const;
    glm::vec<2, T> getBottomLeftCorner() const;
    glm::vec<2, T> getBottomRightCorner() const;

    glm::vec<2, T> getCenter() const;

    void move(const glm::vec<2, T>& offset);

    // data members
    T left, top, width, height;
};

// useful type aliases
using IntRect = Rect<int>;
using FloatRect = Rect<float>;

/////////////////
// free functions
/////////////////

// equality operator
template<typename T>
bool operator==(const Rect<T>& a, const Rect<T>& b);

// inequality operator
template<typename T>
bool operator!=(const Rect<T>& a, const Rect<T>& b);

template<typename T>
glm::vec<2, T> getIntersectionDepth(const Rect<T>& rectA, const Rect<T>& rectB);

// if rect has negative width or height, left-top corner is changed
// and w, h become positive in returned rect
template<typename T>
Rect<T> getAbsoluteRect(const Rect<T>& rect);

} // end of namespace math

#include "Rect.inl"
