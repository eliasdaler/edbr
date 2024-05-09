#pragma once

class DialogueBox;

class IGameUI {
public:
    virtual DialogueBox& getDialogueBox() = 0;
    virtual void openDialogueBox() = 0;
    virtual void closeDialogueBox() = 0;
    virtual bool isDialogueBoxOpen() const = 0;
};
