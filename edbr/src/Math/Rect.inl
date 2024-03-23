#include <cmath>

namespace math
{
template<typename T>
Rect<T>::Rect() : left(0), top(0), width(0), height(0)
{}

template<typename T>
Rect<T>::Rect(T left, T top, T width, T height) : left(left), top(top), width(width), height(height)
{}

template<typename T>
template<typename U>
Rect<T>::Rect(const Rect<U>& r) :
    Rect(static_cast<T>(r.left), static_cast<T>(r.top), static_cast<T>(r.width),
         static_cast<T>(r.height))
{}

template<typename T>
Rect<T>::Rect(const glm::vec<2, T>& position, const glm::vec<2, T>& size) :
    Rect(position.x, position.y, size.x, size.y)
{}

template<typename T>
bool Rect<T>::contains(const glm::vec<2, T>& point) const
{
    return (point.x >= left) && (point.x <= left + width) && (point.y >= top) &&
           (point.y <= top + height);
}

template<typename T>
bool Rect<T>::intersects(const Rect<T>& rect) const
{
    return getIntersectionDepth(*this, rect) != glm::vec<2, T>{T(0)};
}

template<typename T>
glm::vec<2, T> Rect<T>::getPosition() const
{
    return {left, top};
}

template<typename T>
void Rect<T>::setPosition(const glm::vec<2, T>& position)
{
    left = position.x;
    top = position.y;
}

template<typename T>
glm::vec<2, T> Rect<T>::getSize() const
{
    return {width, height};
}

template<typename T>
void Rect<T>::setSize(const glm::vec<2, T>& size)
{
    width = size.x;
    height = size.y;
}

template<typename T>
glm::vec<2, T> Rect<T>::getTopLeftCorner() const
{
    return {left, top};
}

template<typename T>
glm::vec<2, T> Rect<T>::getTopRightCorner() const
{
    return {left + width, top};
}

template<typename T>
glm::vec<2, T> Rect<T>::getBottomLeftCorner() const
{
    return {left, top + height};
}

template<typename T>
glm::vec<2, T> Rect<T>::getBottomRightCorner() const
{
    return {left + width, top + height};
}

template<typename T>
glm::vec<2, T> Rect<T>::getCenter() const
{
    return {left + width / T(2), top + height / T(2)};
}

template<typename T>
void Rect<T>::move(const glm::vec<2, T>& offset)
{
    setPosition(getPosition() + offset);
}

/////////////////
// free functions
/////////////////

template<typename T>
bool operator==(const Rect<T>& a, const Rect<T>& b)
{
    return a.left == b.left && a.top == b.top && a.width == b.width && a.height == b.height;
}

template<typename T>
bool operator!=(const Rect<T>& a, const Rect<T>& b)
{
    return !(a == b);
}

template<typename T>
glm::vec<2, T> getIntersectionDepth(const Rect<T>& rectA, const Rect<T>& rectB)
{
    if (rectA.width == T(0) || rectA.height == T(0) || rectB.width == T(0) ||
        rectB.height == T(0)) { // rectA or rectB are zero-sized
        return glm::vec<2, T>{T(0)};
    }

    T rectARight = rectA.left + rectA.width;
    T rectADown = rectA.top + rectA.height;

    T rectBRight = rectB.left + rectB.width;
    T rectBDown = rectB.top + rectB.height;

    // check if rects intersect
    if (rectA.left >= rectBRight || rectARight <= rectB.left || rectA.top >= rectBDown ||
        rectADown <= rectB.top) {
        return glm::vec<2, T>{T(0)};
    }

    glm::vec<2, T> depth;

    // x
    if (rectA.left < rectB.left) {
        depth.x = rectARight - rectB.left;
    } else {
        depth.x = rectA.left - rectBRight;
    }

    // y
    if (rectA.top < rectB.top) {
        depth.y = rectADown - rectB.top;
    } else {
        depth.y = rectA.top - rectBDown;
    }

    return depth;
}

template<typename T>
Rect<T> getAbsoluteRect(const Rect<T>& rect)
{
    return Rect<T>(rect.width >= T(0) ? rect.left : rect.left + rect.width,
                   rect.height >= T(0) ? rect.top : rect.top + rect.height, std::abs(rect.width),
                   std::abs(rect.height));
}

} // // end of namespace math


