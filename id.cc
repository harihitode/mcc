#include <cstdint>
#include <string>
#include <variant>
#include "ast.h"

using std::make_shared;

namespace mcc {
    namespace id {
        static uint32_t counter = 0;
        std::string gentmp(const type::type_t & t) {
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
                return gentmp(**(std::get<sptr<type::variable>>(t)->value));
            } else {
                fprintf(stderr, "invalib type\n");
                assert(false);
            }
            return "`" + std::string(1, label) + std::to_string(counter++);
        }
        std::string genid(const std::string & s) {
            return s + "." + std::to_string(counter++);
        }
    }
}

namespace {
    using namespace mcc::value;
    std::unordered_map<int, std::shared_ptr<integer>> c_int;
    std::unordered_map<double, std::shared_ptr<floating_point>> c_float;
    std::shared_ptr<boolean> c_true;
    std::shared_ptr<boolean> c_false;
    std::shared_ptr<unit> c_unit;
}

std::shared_ptr<unit> mcc::value::get_const_unit() {
    if (!c_unit) {
        c_unit = make_shared<value::unit>();
    }
    return c_unit;
}


std::shared_ptr<boolean> mcc::value::get_const_true() {
    if (!c_true) {
        c_true = make_shared<value::boolean>(true);
    }
    return c_true;
}

std::shared_ptr<boolean> mcc::value::get_const_false() {
    if (!c_false) {
        c_false = make_shared<value::boolean>(false);
    }
    return c_false;
}

std::shared_ptr<boolean> mcc::value::get_const_boolean(bool value) {
    return value ? mcc::value::get_const_true() : mcc::value::get_const_false();
}

std::shared_ptr<integer> mcc::value::get_const_integer(int value) {
    if (c_int.find(value) == c_int.end()) {
        c_int[value] = make_shared<value::integer>(value);
    }
    return c_int[value];
}

std::shared_ptr<floating_point> mcc::value::get_const_floating_point(double value) {
    if (c_float.find(value) == c_float.end()) {
        c_float[value] = make_shared<value::floating_point>(value);
    }
    return c_float[value];
}
