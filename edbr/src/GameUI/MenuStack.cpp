#include <edbr/GameUI/MenuStack.h>

#include <edbr/GameUI/Cursor.h>
#include <edbr/UI/Element.h>
#include <edbr/UI/UIRenderer.h>

MenuStack::MenuStack(Cursor& cursor) : cursor(cursor)
{}

void MenuStack::calculateLayout(const glm::vec2& screenSize) const
{
    for (auto& menu : menuStack) {
        menu->calculateLayout(screenSize);
    }
}

void MenuStack::draw(SpriteRenderer& spriteRenderer) const
{
    for (const auto& menu : menuStack) {
        ui::drawElement(spriteRenderer, *menu);
    }
}

void MenuStack::replaceTopMenu(ui::Element& menu)
{
    if (!menuStack.empty()) {
        popMenu(true);
    }
    pushMenu(menu);
}

void MenuStack::pushMenu(ui::Element& menu)
{
    menu.visible = true;
    menuStack.push_back(&menu);
    if (!menuStack.empty()) {
        exitMenu(*menuStack.back());
    }
    enterMenu(menu);
}

void MenuStack::popMenu(bool addToMenuHistory)
{
    if (addToMenuHistory) {
        menuHistory.push_back(menuStack.back());
    }

    exitMenu(*menuStack.back());
    menuStack.back()->visible = false;
    menuStack.pop_back();
    if (!menuStack.empty()) {
        enterMenu(*menuStack.back());
    } else {
        cursor.visible = false;
        cursor.selection = nullptr;
    }
}

void MenuStack::enterMenu(ui::Element& menu)
{
    if (menu.cursorSelectionStartElement) {
        cursor.selection = menu.cursorSelectionStartElement;
        if (menu.cursorSelectionStartElement->visible) {
            cursor.visible = true;
            cursor.selection->setSelected(true);
        } else {
            cursor.visible = false;
        }
    } else {
        cursor.visible = false;
    }
}

void MenuStack::exitMenu(ui::Element& menu)
{
    if (cursor.selection) {
        cursor.selection->setSelected(false);
    }
}

void MenuStack::menuGoBack()
{
    popMenu();
    if (!menuHistory.empty()) {
        pushMenu(*menuHistory.back());
        menuHistory.pop_back();
    }
}

const std::string& MenuStack::getTopMenuTag() const
{
    if (!hasMenus()) {
        static const std::string emptyString{};
        return emptyString;
    }
    return getTopMenu().tag;
}

bool MenuStack::hasMenus() const
{
    return !menuStack.empty();
}

const ui::Element& MenuStack::getTopMenu() const
{
    assert(!menuStack.empty());
    return *menuStack.back();
}

bool MenuStack::isInsideMenu(const std::string& tag) const
{
    return hasMenuInStack(tag) || hasMenuInHistory(tag);
}

bool MenuStack::isMenuOnTop(const std::string& tag) const
{
    if (hasMenus()) {
        return menuStack.back()->tag == tag;
    }
    return false;
}

bool MenuStack::hasMenuInStack(const std::string& tag) const
{
    for (const auto& m : menuStack) {
        if (m->tag == tag) {
            return true;
        }
    }
    return false;
}

bool MenuStack::hasMenuInHistory(const std::string& tag) const
{
    for (const auto& m : menuHistory) {
        if (m->tag == tag) {
            return true;
        }
    }
    return false;
}
