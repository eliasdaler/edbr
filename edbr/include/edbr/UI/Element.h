#pragma once

#include <memory>
#include <vector>

#include <glm/vec2.hpp>

namespace ui
{

class Element {
public:
    enum class AutomaticSizing {
        None,
        X, // auto-resize weight only
        Y, // auto-resize height only
        XY, // auto -resize both weight and height
    };

public:
    virtual ~Element() = default;

    void addChild(std::unique_ptr<Element> element);

    const Element* getParent() const { return parent; }
    const std::vector<std::unique_ptr<Element>>& getChildren() const { return children; }

    virtual void setPosition(const glm::vec2& pos) { position = pos; }
    virtual const glm::vec2 getPosition() const { return position; }

    glm::vec2 getSize() const;
    virtual glm::vec2 getContentSize() const = 0;

    void setAutomaticSizing(AutomaticSizing s) { autoSizing = s; }

protected:
    Element* parent{nullptr};
    std::vector<std::unique_ptr<Element>> children;

    glm::vec2 position;
    AutomaticSizing autoSizing{AutomaticSizing::None};
};

} // end of namespace ui
