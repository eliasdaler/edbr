#pragma once

#include <memory>
#include <vector>

#include <glm/vec2.hpp>

namespace ui
{

class Element {
public:
    virtual ~Element() = default;

    void addChild(std::unique_ptr<Element> element);

    const Element* getParent() const { return parent; }
    const std::vector<std::unique_ptr<Element>>& getChildren() const { return children; }

    void setPosition(const glm::vec2& pos) { position = pos; }
    const glm::vec2& getPosition() const { return position; }

private:
    Element* parent{nullptr};
    std::vector<std::unique_ptr<Element>> children;

    glm::vec2 position;
};

} // end of namespace ui
