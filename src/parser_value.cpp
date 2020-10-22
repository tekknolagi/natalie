#include "natalie.hpp"
#include "natalie/array_value.hpp"
#include "natalie/env.hpp"
#include "natalie/symbol_value.hpp"

#include <peg_parser/generator.h>

namespace Natalie {

using std::string;

Value *ParserValue::parse(Env *env, Value *code) {
    peg_parser::ParserGenerator<Value *, Env *&> g;

    g.setSeparator(g["Whitespace"] << "[\t ]");
    g["Nl"] << "'\n'*";
    g["Nl"]->hidden = true;

    g["EndOfExpression"] << "[;\n]";
    g["EndOfExpression"]->hidden = true;

    g["Program"] << "ValidProgram Nl Garbage?" >> [](auto e, Env *env) {
        if (e.size() > 1) e[1].evaluate(env);
        return e[0].evaluate(env);
    };
    g.setStart(g["Program"]);

    g["ValidProgram"] << "Expression? (EndOfExpression Expression)*" >> [](auto e, Env *env) {
        ArrayValue *array = new ArrayValue { env, { SymbolValue::intern(env, "block") } };
        for (auto item : e) {
            array->push(item.evaluate(env));
        }
        return array;
    };

    g["Garbage"] << ".+" >> [](auto e, Env *env) {
        auto position = e.position();
        size_t line = 1;
        size_t index = 0;
        auto full_string = std::string(e.syntax()->fullString);
        for (char c : full_string) {
            if (index > position) break;
            if (c == '\n') line++;
            index++;
        }
        NAT_RAISE(env, "SyntaxError", "(string):%d :: parse error on value \"%s\"", line, e.string().c_str());
        return env->nil_obj();
    };

    g["Expression"] << "Sum | CallWithParens | CallWithoutParens | DqString | SqString | Numeric";

    g["Sum"] << "SumOp | Product";
    g["SumOp"] << "Product SumOperator Nl Product" >> [](auto e, Env *env) { return new ArrayValue { env, { SymbolValue::intern(env, "call"), e[0].evaluate(env), e[1].evaluate(env), e[2].evaluate(env) } }; };
    g["SumOperator"] << "[+\\-]" >> [](auto e, Env *env) { return SymbolValue::intern(env, e.string().c_str()); };

    g["Product"] << "ProductOp | Atomic";
    g["ProductOp"] << "Product ProductOperator Nl Atomic" >> [](auto e, Env *env) { return new ArrayValue { env, { SymbolValue::intern(env, "call"), e[0].evaluate(env), e[1].evaluate(env), e[2].evaluate(env) } }; };
    g["ProductOperator"] << "[\\*/]" >> [](auto e, Env *env) { return SymbolValue::intern(env, e.string().c_str()); };

    g["Atomic"] << "Numeric | '(' Nl Sum Nl ')' | Expression";
    g["Numeric"] << "Float | Integer";

    g["CallWithoutParens"] << "Identifier Args?" >> [](auto e, Env *env) {
        ArrayValue *result = new ArrayValue { env, { SymbolValue::intern(env, "call"), env->nil_obj(), e[0].evaluate(env) } };
        if (e.size() > 1) {
            for (auto item : *e[1].evaluate(env)->as_array()) {
                result->push(item);
            }
        }
        return result;
    };

    g["CallWithParens"] << "Identifier '(' Nl Args? Nl ')'" >> [](auto e, Env *env) {
        ArrayValue *result = new ArrayValue { env, { SymbolValue::intern(env, "call"), env->nil_obj(), e[0].evaluate(env) } };
        if (e.size() > 1) {
            for (auto item : *e[1].evaluate(env)->as_array()) {
                result->push(item);
            }
        }
        return result;
    };

    g["Args"] << "Expression (',' Nl Expression)*" >> [](auto e, Env *env) {
        ArrayValue *array = new ArrayValue { env };
        for (auto item : e) {
            array->push(item.evaluate(env));
        }
        return array;
    };

    g["Identifier"] << "[a-z_] [a-zA-Z0-9_]*" >> [](auto e, Env *env) { return SymbolValue::intern(env, e.string().c_str()); };

    g["Float"] << "'-'? [0-9]+ '.' [0-9]+" >> [](auto e, Env *env) { return new ArrayValue { env, { SymbolValue::intern(env, "lit"), new FloatValue { env, stof(e.string()) } } }; };
    g["Integer"] << "'-'? [0-9]+" >> [](auto e, Env *env) { return new ArrayValue { env, { SymbolValue::intern(env, "lit"), new IntegerValue { env, stoll(e.string()) } } }; };

    g["EscapedChar"] << "'\\\\' .";

    g["DqString"] << "'\"' (EscapedChar | DqChar)* '\"'" >> [](auto e, Env *env) {
        string result;
        string s = e.string();
        for (size_t i = 1; i < s.length() - 1; i++) {
            char c = s[i];
            if (c == '\\') {
                i++;
                c = s[i];
                switch (c) {
                case 'n':
                    result += '\n';
                    break;
                case 't':
                    result += '\t';
                    break;
                default:
                    result += c;
                }
                continue;
            }
            result += c;
        }
        return new ArrayValue { env, { SymbolValue::intern(env, "str"), new StringValue { env, result.c_str() } } };
    };
    g["DqChar"] << "!'\"' .";

    g["SqString"] << "'\\'' (EscapedChar | SqChar)* '\\''" >> [](auto e, Env *env) {
        string result;
        string s = e.string();
        for (size_t i = 1; i < s.length() - 1; i++) {
            char c = s[i];
            if (c == '\\') {
                i++;
                c = s[i];
                switch (c) {
                case '\\':
                case '\'':
                    result += c;
                    break;
                default:
                    result += '\\';
                    result += c;
                }
                continue;
            }
            result += c;
        }
        return new ArrayValue { env, { SymbolValue::intern(env, "str"), new StringValue { env, result.c_str() } } };
    };
    g["SqChar"] << "!'\\'' .";

    const char *input = code->as_string()->c_str();
    Value *result;
    try {
        result = g.run(input, env);
    } catch (peg_parser::SyntaxError &error) {
        // Note: bad syntax should be caught by the "Garbage" rule above.
        NAT_UNREACHABLE();
    }
    return result;
}
}
