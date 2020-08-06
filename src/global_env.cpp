#include "natalie.hpp"

#include <stdarg.h>
#include <string>
#include <unordered_map>

namespace Natalie {

struct GlobalEnv::value_map {
    std::unordered_map<std::string, Value *> m_map {};
};

GlobalEnv::GlobalEnv()
    : m_globals { new value_map {} }
    , m_symbols { new value_map {} } {
}

GlobalEnv::~GlobalEnv() { }

Value *GlobalEnv::get_symbol(const char *name) {
    auto result = m_symbols->m_map.find(name);
    if (result == m_symbols->m_map.end()) {
        return nullptr;
    }
    return result->second;
}

void GlobalEnv::add_symbol(const char *name, Value *value) {
    m_symbols->m_map[name] = value;
}

Value *GlobalEnv::global_get(const char *name) {
    auto result = m_globals->m_map.find(name);
    if (result == m_globals->m_map.end()) {
        return nullptr;
    }
    return result->second;
}

void GlobalEnv::global_set(const char *name, Value *value) {
    m_globals->m_map[name] = value;
}

}
