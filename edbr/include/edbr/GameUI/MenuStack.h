#pragma once

#include <glm/vec2.hpp>

#include <string>
#include <vector>

namespace ui
{
struct Element;
}

struct Cursor;
class SpriteRenderer;

// MenuStack handles multiple menus either being shown at the same time
// or shown one at a time, but with menu history preserved so that you can navigate
// backwards to the previous menu
class MenuStack {
public:
    MenuStack(Cursor& cursor);

    void calculateLayout(const glm::vec2& screenSize) const;
    void draw(SpriteRenderer& spriteRenderer) const;

    void replaceTopMenu(ui::Element& menu);
    void menuGoBack();

    // Add menu to stack - the menu will be displayed on top
    void pushMenu(ui::Element& menu);
    // Remove item from the stack - mostly used when entering new menus
    void popMenu(bool addToMenuHistory = false);

    bool hasMenus() const;
    const ui::Element& getTopMenu() const;
    // Returns top menu tag, if no menus in stack - empty string is returned
    const std::string& getTopMenuTag() const;

    const std::vector<ui::Element*>& getMenuHistory() const { return menuHistory; }

    bool isInsideMenu(const std::string& tag) const;
    bool isMenuOnTop(const std::string& tag) const;
    bool hasMenuInStack(const std::string& tag) const;
    bool hasMenuInHistory(const std::string& tag) const;

private:
    // What happens when menu is displayed
    void enterMenu(ui::Element& menu);
    // What happens when menu is exited
    void exitMenu(ui::Element& menu);

    // Currently displayed menus - from bottom to top
    // The one on the top takes input focus
    std::vector<ui::Element*> menuStack;

    // Menu history - when you press "back", the last item in the list is pushed
    // on top of the menu stack
    std::vector<ui::Element*> menuHistory;

    Cursor& cursor;
};
