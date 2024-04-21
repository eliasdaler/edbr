#include <edbr/Util/MetaUtil.h>

#ifdef __GNUG__
#include <cxxabi.h>
#include <memory>
#endif

namespace util
{
std::string getDemangledTypename(const char* typeName)
{
    auto name = typeName;
#ifdef __GNUG__
    int status;
    std::unique_ptr<char, void (*)(void*)>
        res{abi::__cxa_demangle(name, NULL, NULL, &status), std::free};
    name = res.get();
#endif
    return name;
}

}
