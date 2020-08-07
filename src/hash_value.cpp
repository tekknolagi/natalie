#include "natalie.hpp"

#include <string>
#include <unordered_map>

namespace Natalie {

auto hash = [](HashValue::Key *key) {
    return key->hash;
};

auto equal = [](HashValue::Key *a, HashValue::Key *b) {
    return a->hash == b->hash;
};

struct HashValue::value_map : public gc {
    Val *find(Key *key) {
        auto result = m_map.find(key);
        if (result == m_map.end()) {
            return nullptr;
        }
        return result->second;
    }

    void set(Key *key, Val *val) {
        m_map[key] = val;
    }

    size_t erase(Key *key) {
        return m_map.erase(key);
    }

    std::unordered_map<Key *, Val *, decltype(hash), decltype(equal)> m_map { 8, hash, equal };
};

HashValue::HashValue(Env *env, ClassValue *klass)
    : Value { Value::Type::Hash, klass }
    , m_hashmap { new value_map {} }
    , m_default_value { env->nil_obj() } {
}

Value *HashValue::get(Env *env, Value *key) {
    Key key_container;
    key_container.key = key;
    key_container.hash = key->send(env, "hash")->as_integer()->to_int64_t();
    Val *container = m_hashmap->find(&key_container);
    Value *val = container ? container->val : nullptr;
    return val;
}

Value *HashValue::get_default(Env *env, Value *key) {
    if (m_default_block) {
        Value *args[2] = { this, key };
        return NAT_RUN_BLOCK_WITHOUT_BREAK(env, m_default_block, 2, args, nullptr);
    } else {
        return m_default_value;
    }
}

void HashValue::put(Env *env, Value *key, Value *val) {
    NAT_ASSERT_NOT_FROZEN(this);
    Key key_container;
    key_container.key = key;
    key_container.hash = key->send(env, "hash")->as_integer()->to_int64_t();
    Val *container = m_hashmap->find(&key_container);
    if (container) {
        container->key->val = val;
        container->val = val;
    } else {
        if (m_is_iterating) {
            NAT_RAISE(env, "RuntimeError", "can't add a new key into hash during iteration");
        }
        container = static_cast<Val *>(malloc(sizeof(Val)));
        container->key = key_list_append(env, key, val);
        container->val = val;
        m_hashmap->set(container->key, container);
    }
}

Value *HashValue::remove(Env *env, Value *key) {
    Key key_container;
    key_container.key = key;
    key_container.hash = key->send(env, "hash")->as_integer()->to_int64_t();
    Val *container = m_hashmap->find(&key_container);
    m_hashmap->erase(&key_container);
    if (container) {
        key_list_remove_node(container->key);
        Value *val = container->val;
        free(container);
        return val;
    } else {
        return nullptr;
    }
}

HashValue::Key *HashValue::key_list_append(Env *env, Value *key, Value *val) {
    if (m_key_list) {
        Key *first = m_key_list;
        Key *last = m_key_list->prev;
        Key *new_last = static_cast<Key *>(malloc(sizeof(Key)));
        new_last->key = key;
        new_last->val = val;
        // <first> ... <last> <new_last> -|
        // ^______________________________|
        new_last->prev = last;
        new_last->next = first;
        new_last->hash = key->send(env, "hash")->as_integer()->to_int64_t();
        new_last->removed = false;
        first->prev = new_last;
        last->next = new_last;
        return new_last;
    } else {
        Key *node = static_cast<Key *>(malloc(sizeof(Key)));
        node->key = key;
        node->val = val;
        node->prev = node;
        node->next = node;
        node->hash = key->send(env, "hash")->as_integer()->to_int64_t();
        node->removed = false;
        m_key_list = node;
        return node;
    }
}

void HashValue::key_list_remove_node(Key *node) {
    Key *prev = node->prev;
    Key *next = node->next;
    // <prev> <-> <node> <-> <next>
    if (node == next) {
        // <node> -|
        // ^_______|
        node->prev = nullptr;
        node->next = nullptr;
        node->removed = true;
        m_key_list = nullptr;
        return;
    } else if (m_key_list == node) {
        // starting point is the node to be removed, so shift them forward by one
        m_key_list = next;
    }
    // remove the node
    node->removed = true;
    prev->next = next;
    next->prev = prev;
}

Value *HashValue::initialize(Env *env, Value *default_value, Block *block) {
    if (block) {
        if (default_value) {
            NAT_RAISE(env, "ArgumentError", "wrong number of arguments (given 1, expected 0)");
        }
        set_default_block(block);
    } else if (default_value) {
        set_default_value(default_value);
    }
    return this;
}

// Hash[]
Value *HashValue::square_new(Env *env, ssize_t argc, Value **args) {
    if (argc == 0) {
        return new HashValue { env };
    } else if (argc == 1) {
        Value *value = args[0];
        if (value->type() == Value::Type::Hash) {
            return value;
        } else if (value->type() == Value::Type::Array) {
            HashValue *hash = new HashValue { env };
            for (auto &pair : *value->as_array()) {
                if (pair->type() != Value::Type::Array) {
                    NAT_RAISE(env, "ArgumentError", "wrong element in array to Hash[]");
                }
                ssize_t size = pair->as_array()->size();
                if (size < 1 || size > 2) {
                    NAT_RAISE(env, "ArgumentError", "invalid number of elements (%d for 1..2)", size);
                }
                Value *key = (*pair->as_array())[0];
                Value *value = size == 1 ? env->nil_obj() : (*pair->as_array())[1];
                hash->put(env, key, value);
            }
            return hash;
        }
    }
    if (argc % 2 != 0) {
        NAT_RAISE(env, "ArgumentError", "odd number of arguments for Hash");
    }
    HashValue *hash = new HashValue { env };
    for (ssize_t i = 0; i < argc; i += 2) {
        Value *key = args[i];
        Value *value = args[i + 1];
        hash->put(env, key, value);
    }
    return hash;
}

Value *HashValue::inspect(Env *env) {
    StringValue *out = new StringValue { env, "{" };
    ssize_t last_index = size() - 1;
    ssize_t index = 0;
    for (HashValue::Key &node : *this) {
        StringValue *key_repr = node.key->send(env, "inspect")->as_string();
        out->append_string(env, key_repr);
        out->append(env, "=>");
        StringValue *val_repr = node.val->send(env, "inspect")->as_string();
        out->append_string(env, val_repr);
        if (index < last_index) {
            out->append(env, ", ");
        }
        index++;
    }
    out->append_char(env, '}');
    return out;
}

Value *HashValue::ref(Env *env, Value *key) {
    Value *val = get(env, key);
    if (val) {
        return val;
    } else {
        return get_default(env, key);
    }
}

Value *HashValue::refeq(Env *env, Value *key, Value *val) {
    put(env, key, val);
    return val;
}

Value *HashValue::delete_key(Env *env, Value *key) {
    NAT_ASSERT_NOT_FROZEN(this);
    Value *val = remove(env, key);
    if (val) {
        return val;
    } else {
        return env->nil_obj();
    }
}

ssize_t HashValue::size() {
    return m_hashmap->m_map.size();
}

Value *HashValue::size(Env *env) {
    assert(size() <= NAT_MAX_INT);
    return new IntegerValue { env, static_cast<int64_t>(size()) };
}

Value *HashValue::eq(Env *env, Value *other_value) {
    if (!other_value->is_hash()) {
        return env->false_obj();
    }
    HashValue *other = other_value->as_hash();
    if (size() != other->size()) {
        return env->false_obj();
    }
    Value *other_val;
    for (HashValue::Key &node : *this) {
        other_val = other->get(env, node.key);
        if (!other_val) {
            return env->false_obj();
        }
        if (!node.val->send(env, "==", 1, &other_val, nullptr)->is_truthy()) {
            return env->false_obj();
        }
    }
    return env->true_obj();
}

#define NAT_RUN_BLOCK_AND_POSSIBLY_BREAK_WHILE_ITERATING_HASH(env, the_block, argc, args, block, hash) ({ \
    Value *_result = the_block->_run(env, argc, args, block);                                             \
    if (_result->has_break_flag()) {                                                                      \
        _result->remove_break_flag();                                                                     \
        hash->set_is_iterating(false);                                                                    \
        return _result;                                                                                   \
    }                                                                                                     \
    _result;                                                                                              \
})

Value *HashValue::each(Env *env, Block *block) {
    NAT_ASSERT_BLOCK(); // TODO: return Enumerator when no block given
    Value *block_args[2];
    for (HashValue::Key &node : *this) {
        block_args[0] = node.key;
        block_args[1] = node.val;
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK_WHILE_ITERATING_HASH(env, block, 2, block_args, nullptr, this);
    }
    return this;
}

Value *HashValue::keys(Env *env) {
    ArrayValue *array = new ArrayValue { env };
    for (HashValue::Key &node : *this) {
        array->push(node.key);
    }
    return array;
}

Value *HashValue::values(Env *env) {
    ArrayValue *array = new ArrayValue { env };
    for (HashValue::Key &node : *this) {
        array->push(node.val);
    }
    return array;
}

Value *HashValue::sort(Env *env) {
    ArrayValue *ary = new ArrayValue { env };
    for (HashValue::Key &node : *this) {
        ArrayValue *pair = new ArrayValue { env };
        pair->push(node.key);
        pair->push(node.val);
        ary->push(pair);
    }
    return ary->sort(env);
}

Value *HashValue::has_key(Env *env, Value *key) {
    Value *val = get(env, key);
    if (val) {
        return env->true_obj();
    } else {
        return env->false_obj();
    }
}

}
