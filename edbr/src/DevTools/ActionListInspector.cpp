#include <edbr/DevTools/ActionListInspector.h>

#include <edbr/ActionList/ActionList.h>

#include <edbr/ActionList/Actions/ActionListFinishAction.h>
#include <edbr/ActionList/Actions/DelayAction.h>
#include <edbr/ActionList/Actions/DoAction.h>
#include <edbr/ActionList/Actions/MakeAction.h>
#include <edbr/ActionList/Actions/TweenAction.h>
#include <edbr/ActionList/Actions/WaitWhileAction.h>

#include <edbr/DevTools/ImGuiPropertyTable.h>
#include <edbr/Graphics/Color.h>
#include <edbr/Util/MetaUtil.h>

#include <edbr/Util/ImGuiUtil.h>
#include <imgui.h>

std::string ActionListInspector::getActionTypeName(const Action& action)
{
    if (auto alfa = dynamic_cast<const ActionListFinishAction*>(&action); alfa) {
        return "List finish";
    }

    if (auto da = dynamic_cast<const DoAction*>(&action); da) {
        if (!da->getName().empty()) {
            return da->getName();
        }
    }

    if (auto wwa = dynamic_cast<const WaitWhileAction*>(&action); wwa) {
        if (!wwa->getConditionName().empty()) {
            return wwa->getConditionName();
        }
    }

    if (auto ma = dynamic_cast<const MakeAction*>(&action); ma) {
        if (!ma->getMakeFuncName().empty()) {
            return "Make: " + ma->getMakeFuncName();
        }
    }

    if (auto ta = dynamic_cast<const TweenAction*>(&action); ta) {
        if (!ta->name.empty()) {
            return "Tween: " + ta->name;
        }
    }

    return util::getDemangledTypename(typeid(action).name());
}

void ActionListInspector::showActionListInfo(const ActionList& actionList, bool rootList)
{
    using namespace devtools;

    const auto& name = actionList.getName();

    static const auto actionListNameColor = RGBColor{142, 203, 229};
    util::ImGuiPushTextStyleColor(actionListNameColor);
    if (ImGui::TreeNodeEx(name.c_str(), rootList ? 0 : ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PopStyleColor();
        const auto& actions = actionList.getActions();
        for (std::size_t i = 0; i < actions.size(); ++i) {
            auto& action = *actions[i];
            bool currentAction =
                (actionList.isPlaying() && i == actionList.getCurrentActionIndex());
            showAction(action, currentAction);
        }
        ImGui::TreePop();
    } else {
        ImGui::PopStyleColor();
    }
}

void ActionListInspector::showAction(const Action& action, bool current)
{
    using namespace devtools;

    static const auto currentActionNameColor = RGBColor{255, 255, 0};
    static const auto actionNameColor = RGBColor{142, 229, 195};

    util::ImGuiTextColored(
        current ? currentActionNameColor : actionNameColor,
        current ? "=> %s" : "* %s",
        getActionTypeName(action).c_str());

    if (auto alfa = dynamic_cast<const ActionListFinishAction*>(&action); alfa) {
        showActionListInfo(alfa->getActionList());
    }

    if (auto da = dynamic_cast<const DelayAction*>(&action); da) {
        BeginPropertyTable();
        DisplayProperty("Delay", da->getDelay());
        DisplayProperty("Current", da->getCurrentTime());
        EndPropertyTable();
    }

    if (auto ta = dynamic_cast<const TweenAction*>(&action); ta) {
        BeginPropertyTable();
        DisplayProperty("Start value", ta->startValue);
        DisplayProperty("End value", ta->endValue);
        DisplayProperty("Duration", ta->duration);
        DisplayProperty("Current time", ta->currentTime);
        DisplayProperty("Current value", ta->currentValue);
        EndPropertyTable();
    }

    if (auto ma = dynamic_cast<const MakeAction*>(&action); ma) {
        if (ma->actionMade()) {
            showAction(ma->getAction(), current);
        }
    }
}
