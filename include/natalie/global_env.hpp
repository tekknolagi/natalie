#pragma once

#include <stdlib.h>

#include "natalie/forward.hpp"

namespace Natalie {

extern "C" {
#include "onigmo.h"
}

struct GlobalEnv {
    GlobalEnv();
    ~GlobalEnv();

    Value *get_symbol(const char *);
    void add_symbol(const char *, Value *);

    Value *global_get(const char *);
    void global_set(const char *, Value *);

    ClassValue *Object() { return m_Object; }
    void set_Object(ClassValue *Object) { m_Object = Object; }

    NilValue *nil_obj() { return m_nil_obj; }
    void set_nil_obj(NilValue *nil_obj) { m_nil_obj = nil_obj; }

    TrueValue *true_obj() { return m_true_obj; }
    void set_true_obj(TrueValue *true_obj) { m_true_obj = true_obj; }

    FalseValue *false_obj() { return m_false_obj; }
    void set_false_obj(FalseValue *false_obj) { m_false_obj = false_obj; }

private:
    struct value_map;
    value_map *m_globals { nullptr };
    value_map *m_symbols { nullptr };
    ClassValue *m_Object { nullptr };
    NilValue *m_nil_obj { nullptr };
    TrueValue *m_true_obj { nullptr };
    FalseValue *m_false_obj { nullptr };
};

}
