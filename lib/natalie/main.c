#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashmap.h"

typedef struct NatObject NatObject;

enum NatValueType {
    NAT_VALUE_CLASS,
    NAT_VALUE_ARRAY,
    NAT_VALUE_STRING,
    NAT_VALUE_NUMBER,
    NAT_VALUE_NIL,
    NAT_VALUE_OTHER
};

struct NatObject {
    enum NatValueType type;
    NatObject *class;

    union {
        long long number;
        struct hashmap hashmap;

        // NAT_VALUE_CLASS
        struct {
            char *name;
            NatObject *superclass;
            struct hashmap methods;
        };

        // NAT_VALUE_ARRAY
        struct {
            size_t ary_len;
            size_t ary_cap;
            NatObject **ary;
        };

        // NAT_VALUE_STRING
        struct {
            size_t str_len;
            size_t str_cap;
            char *str;
        };

        // NAT_VALUE_REGEX
        struct {
            size_t regex_len;
            char *regex;
        };

        // NAT_LAMBDA_TYPE, NAT_CONTINUATION_TYPE
        //struct {
        //  NatObject* (*fn)(NatEnv *env, size_t argc, NatObject **args);
        //  char *function_name;
        //  NatEnv *env;
        //  size_t argc;
        //  NatObject **args;
        //};
    };
};

typedef struct NatEnv NatEnv;

struct NatEnv {
    struct hashmap data;
    NatEnv *outer;
};

NatEnv *env_find(NatEnv *env, char *key) {
    if (hashmap_get(&env->data, key)) {
        return env;
    } else if (env->outer) {
        return env_find(env->outer, key);
    } else {
        return NULL;
    }
}

NatObject *env_get(NatEnv *env, char *key) {
    env = env_find(env, key);
    /*
    if (!env) {
        return nat_error(nat_sprintf("'%s' not found", key));
    }
    */
    NatObject *val = hashmap_get(&env->data, key);
    if (val) {
        return val;
    } else {
        //return nat_error(nat_sprintf("'%s' not found", key));
    }
}

NatObject *env_set(NatEnv *env, char *key, NatObject *val) {
    //if (is_blank_line(val)) return val;
    hashmap_remove(&env->data, key);
    hashmap_put(&env->data, key, val);
    return val;
}

void env_delete(NatEnv *env, char *key) {
    hashmap_remove(&env->data, key);
}

#define TRUE 1
#define FALSE 0

char *heap_string(char *str) {
    size_t len = strlen(str);
    char *copy = malloc(len + 1);
    memcpy(copy, str, len + 1);
    return copy;
}

NatObject *nat_alloc() {
    NatObject *val = malloc(sizeof(NatObject));
    val->type = NAT_VALUE_OTHER;
    return val;
}

NatObject *nat_subclass(NatObject *superclass, char *name) {
    NatObject *val = nat_alloc();
    val->type = NAT_VALUE_CLASS;
    val->name = heap_string(name);
    val->superclass = superclass;
    hashmap_init(&val->methods, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&val->methods, hashmap_alloc_key_string, NULL);
    return val;
}

NatObject *nat_new(NatObject *class) {
    NatObject *val = nat_alloc();
    val->class = class;
    return val;
}

NatObject *nat_number(NatEnv *env, long long num) {
    NatObject *val = nat_new(env_get(env, "Numeric"));
    val->type = NAT_VALUE_NUMBER;
    val->number = num;
    return val;
}

NatObject *nat_string(NatEnv *env, char *str) {
    NatObject *val = nat_new(env_get(env, "String"));
    val->type = NAT_VALUE_STRING;
    size_t len = strlen(str);
    val->str = heap_string(str);
    val->str_len = len;
    val->str_cap = len;
    return val;
}

// note: there is a formula using log10 to calculate a number length, but this works for now
size_t num_char_len(long long num) {
    if (num < 0) {
        return 1 + num_char_len(llabs(num));
    } else if (num < 10) {
        return 1;
    } else if (num < 100) {
        return 2;
    } else if (num < 1000) {
        return 3;
    } else if (num < 10000) {
        return 4;
    } else if (num < 100000) {
        return 5;
    } else if (num < 1000000) {
        return 6;
    } else if (num < 1000000000) {
        return 9;
    } else if (num < 1000000000000) {
        return 12;
    } else if (num < 1000000000000000) {
        return 15;
    } else if (num < 1000000000000000000) {
        return 18;
    } else { // up to 128 bits
        return 40;
    }
}

char* long_long_to_string(long long num) {
  char* str;
  size_t len;
  if (num == 0) {
    return heap_string("0");
  } else {
    len = num_char_len(num);
    str = malloc(len + 1);
    snprintf(str, len + 1, "%lli", num);
    return str;
  }
}

NatObject *nat_send(NatEnv *env, NatObject *receiver, char *sym, size_t argc, NatObject **args) {
    // TODO: look up the class inheritance for the method
    NatObject* (*method)(NatEnv*, NatObject*, size_t, NatObject**) = hashmap_get(&receiver->class->methods, sym);
    if (method == NULL) {
        printf("Error: object has no %s method, cannot call send.\n", sym);
        abort();
    }
    return method(env, receiver, argc, args);
}

NatObject *NilClass_to_s(NatEnv *env, NatObject *self, size_t argc, NatObject **args) {
    assert(self->type == NAT_VALUE_NIL);
    NatObject *out = nat_string(env, "");
    return out;
}

NatObject *NilClass_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args) {
    assert(self->type == NAT_VALUE_NIL);
    NatObject *out = nat_string(env, "nil");
    return out;
}

NatObject *Object_puts(NatEnv *env, NatObject *self, size_t argc, NatObject **args) {
    NatObject *str;
    for (size_t i=0; i<argc; i++) {
        str = nat_send(env, args[i], "to_s", 0, NULL);
        assert(str->type == NAT_VALUE_STRING);
        printf("%s\n", str->str);
    }
    return env_get(env, "nil");
}

NatObject *Numeric_to_s(NatEnv *env, NatObject *self, size_t argc, NatObject **args) {
    assert(self->type == NAT_VALUE_NUMBER);
    char *str = long_long_to_string(self->number);
    return nat_string(env, str);
}

NatObject *String_to_s(NatEnv *env, NatObject *self, size_t argc, NatObject **args) {
    assert(self->type == NAT_VALUE_STRING);
    return self;
}

#define STRING_GROW_FACTOR 2

void nat_grow_string(NatObject *obj, size_t capacity) {
    size_t len = strlen(obj->str);
    assert(capacity >= len);
    obj->str = realloc(obj->str, capacity + 1);
    obj->str_cap = capacity;
}

void nat_grow_string_at_least(NatObject *obj, size_t min_capacity) {
    size_t capacity = obj->str_cap;
    if (capacity >= min_capacity)
        return;
    if (capacity > 0 && min_capacity <= capacity * STRING_GROW_FACTOR) {
        nat_grow_string(obj, capacity * STRING_GROW_FACTOR);
    } else {
        nat_grow_string(obj, min_capacity);
    }
}

void nat_string_append(NatObject *str, char *s) {
    size_t new_len = strlen(s);
    if (new_len == 0) return;
    size_t total_len = str->str_len + new_len;
    nat_grow_string_at_least(str, total_len);
    strcat(str->str, s);
    str->str_len = total_len;
}

void nat_string_append_char(NatObject *str, char c) {
    size_t total_len = str->str_len + 1;
    nat_grow_string_at_least(str, total_len);
    str->str[total_len - 1] = c;
    str->str[total_len] = 0;
    str->str_len = total_len;
}

NatObject *String_ltlt(NatEnv *env, NatObject *self, size_t argc, NatObject **args) {
    assert(self->type == NAT_VALUE_STRING);
    assert(argc == 1);
    NatObject *arg = args[0];
    char *str;
    if (arg->type == NAT_VALUE_STRING) {
        str = arg->str;
    } else {
        NatObject *str_obj = nat_send(env, arg, "to_s", 0, NULL);
        assert(str_obj->type == NAT_VALUE_STRING);
        str = str_obj->str;
    }
    nat_string_append(self, str);
    return self;
}

NatObject *String_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args) {
    assert(self->type == NAT_VALUE_STRING);
    NatObject *out = nat_string(env, "\"");
    for (size_t i=0; i<self->str_len; i++) {
        // FIXME: iterate over multibyte chars
        char c = self->str[i];
        if (c == '"' || c == '\\') {
            nat_string_append_char(out, '\\');
            nat_string_append_char(out, c);
        } else {
            nat_string_append_char(out, c);
        }
    }
    nat_string_append_char(out, '"');
    return out;
}

NatEnv *build_env(NatEnv *outer) {
    NatEnv *env = malloc(sizeof(NatEnv));
    env->outer = outer;
    hashmap_init(&env->data, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&env->data, hashmap_alloc_key_string, NULL);
    return env;
}

NatEnv *build_top_env() {
    NatEnv *env = build_env(NULL);

    NatObject *Class = nat_alloc();
    Class->type = NAT_VALUE_CLASS;
    Class->name = heap_string("Class");
    Class->superclass = Class;
    hashmap_init(&Class->methods, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&Class->methods, hashmap_alloc_key_string, NULL);
    env_set(env, "Class", Class);

    NatObject *Object = nat_subclass(Class, "Object");
    hashmap_put(&Object->methods, "puts", Object_puts);
    env_set(env, "Object", Object);

    NatObject *main_obj = nat_new(Object);
    env_set(env, "self", main_obj);

    NatObject *NilClass = nat_subclass(Object, "NilClass");
    hashmap_put(&NilClass->methods, "to_s", NilClass_to_s);
    hashmap_put(&NilClass->methods, "inspect", NilClass_inspect);
    env_set(env, "NilClass", NilClass);

    NatObject *nil = nat_new(NilClass);
    nil->type = NAT_VALUE_NIL;
    env_set(env, "nil", nil);

    NatObject *Numeric = nat_subclass(Object, "Numeric");
    hashmap_put(&Numeric->methods, "to_s", Numeric_to_s);
    hashmap_put(&Numeric->methods, "inspect", Numeric_to_s);
    env_set(env, "Numeric", Numeric);

    NatObject *String = nat_subclass(Object, "String");
    hashmap_put(&String->methods, "to_s", String_to_s);
    hashmap_put(&String->methods, "inspect", String_inspect);
    hashmap_put(&String->methods, "<<", String_ltlt);
    env_set(env, "String", String);

    return env;
}

/*TOP*/

NatObject *EVAL(NatEnv *env) {
    /*DECL*/
    /*BODY*/
}

int main(void) {
    NatEnv *env = build_top_env();
    EVAL(env);
    return 0;
}