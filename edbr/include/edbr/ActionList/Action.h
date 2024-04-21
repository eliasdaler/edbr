#pragma once

class Action {
public:
    virtual ~Action(){};

    // return true from "enter" to signify that action is finished
    virtual bool enter() { return true; }
    // return true from "update" to signify that action is finished
    virtual bool update(float dt) { return true; }
};
