#include <cstdint>
#include <string>
#include <variant>
#include "ast.h"

using std::make_shared;
using namespace mcc;

namespace mcc {
    namespace id {
        std::string gentmp(context & ctx, const type::type_t & t) {
            char label;
            if (std::get_if<sptr<type::unit>>(&t)) {
                label = 'u';
            } else if (std::get_if<sptr<type::boolean>>(&t)) {
                label = 'b';
            } else if (std::get_if<sptr<type::integer>>(&t)) {
                label = 'i';
            } else if (std::get_if<sptr<type::floating_point>>(&t)) {
                label = 'd';
            } else if (std::get_if<sptr<type::function>>(&t)) {
                label = 'f';
            } else if (std::get_if<sptr<type::tuple>>(&t)) {
                label = 't';
            } else if (std::get_if<sptr<type::array>>(&t)) {
                label = 'a';
            } else if (std::get_if<sptr<type::variable>>(&t)) {
                return gentmp(ctx, **(std::get<sptr<type::variable>>(t)->value));
            } else {
                fprintf(stderr, "invalib type\n");
                assert(false);
            }
            return "`" + std::string(1, label) + std::to_string(ctx.counter++);
        }
        std::string genid(context & ctx, const std::string & s) {
            return s + "." + std::to_string(ctx.counter++);
        }
    }
}

std::shared_ptr<value::unit> mcc::value::get_const_unit(context & ctx) {
    if (!ctx.c_unit) {
        ctx.c_unit = make_shared<value::unit>();
    }
    return ctx.c_unit;
}

std::shared_ptr<value::boolean> mcc::value::get_const_true(context & ctx) {
    if (!ctx.c_true) {
        ctx.c_true = make_shared<value::boolean>(true);
    }
    return ctx.c_true;
}

std::shared_ptr<value::boolean> mcc::value::get_const_false(context & ctx) {
    if (!ctx.c_false) {
        ctx.c_false = make_shared<value::boolean>(false);
    }
    return ctx.c_false;
}

std::shared_ptr<value::boolean> mcc::value::get_const_boolean(context & ctx, bool value) {
    return value ? mcc::value::get_const_true(ctx) : mcc::value::get_const_false(ctx);
}

std::shared_ptr<value::integer> mcc::value::get_const_integer(context & ctx, int value) {
    if (ctx.c_int.find(value) == ctx.c_int.end()) {
        ctx.c_int[value] = make_shared<value::integer>(value);
    }
    return ctx.c_int[value];
}

std::shared_ptr<value::floating_point> mcc::value::get_const_floating_point(context & ctx, double value) {
    if (ctx.c_float.find(value) == ctx.c_float.end()) {
        ctx.c_float[value] = make_shared<value::floating_point>(value);
    }
    return ctx.c_float[value];
}
