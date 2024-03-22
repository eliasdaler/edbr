#include "JsonMath.h"

namespace glm
{
void to_json(nlohmann::json& j, const glm::quat& obj)
{
    j = {obj.x, obj.y, obj.z, obj.w};
}

void from_json(const nlohmann::json& j, glm::quat& obj)
{
    assert(j.size() == 4);
    obj.x = j[0];
    obj.y = j[1];
    obj.z = j[2];
    obj.w = j[3];
}

}
