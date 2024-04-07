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

    Element& addChild(std::unique_ptr<Element> element);

    const Element* getParent() const { return parent; }
    const std::vector<std::unique_ptr<Element>>& getChildren() const { return children; }

    virtual void setPosition(const glm::vec2& pos) { position = pos; }
    virtual const glm::vec2 getPosition() const { return position; }

    glm::vec2 getSize() const;

    void setAutomaticSizing(AutomaticSizing s) { autoSizing = s; }
    AutomaticSizing getAutomaticSizing() const { return autoSizing; }

    void setTag(std::string t) { tag = std::move(t); }
    const std::string& getTag() const { return tag; }

    void setAutomaticSizingElementIndex(std::size_t i);

protected:
    virtual glm::vec2 getSizeImpl() const = 0;

    Element* parent{nullptr};
    std::vector<std::unique_ptr<Element>> children;

    glm::vec2 position;
    AutomaticSizing autoSizing{AutomaticSizing::None};
    std::string tag;
    std::size_t autoSizeElementIndex{0};
};

} // end of namespace ui
