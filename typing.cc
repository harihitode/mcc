#include "ast.h"
#include "typing.h"
#include "printer.h"

using std::make_shared;
using namespace mcc::type;
using namespace mcc::operand;

namespace {
    std::shared_ptr<unit> unit_type;
    std::shared_ptr<integer> integer_type;
    std::shared_ptr<floating_point> floating_point_type;
    std::shared_ptr<boolean> boolean_type;
}

type_t mcc::type::unwrap(type::type_t const & t) {
    if (auto td = std::get_if<sptr<type::variable>>(&t)) {
        if (*(*td)->value != nullptr)
            return unwrap(**((*td)->value));
        else
            assert(false);
    } else {
        return t;
    }
}

type_t mcc::type::get_unit() {
    if (unit_type) {
        return unit_type;
    } else {
        unit_type = make_shared<unit>();
        return unit_type;
    }
}

type_t mcc::type::get_boolean() {
    if (boolean_type) {
        return boolean_type;
    } else {
        boolean_type = make_shared<boolean>();
        return boolean_type;
    }
}

type_t mcc::type::get_floating_point() {
    if (floating_point_type) {
        return floating_point_type;
    } else {
        floating_point_type = make_shared<floating_point>();
        return floating_point_type;
    }
}

type_t mcc::type::get_integer() {
    if (integer_type) {
        return integer_type;
    } else {
        integer_type = make_shared<integer>();
        return integer_type;
    }
}

// for deref_term
namespace mcc {
    template<class T>
    struct remove_cref {
        typedef std::remove_const_t<std::remove_reference_t<T>> type;
    };

    template<class T>
    using remove_cref_t = typename remove_cref<T>::type;
}

using namespace mcc;
using namespace mcc::type;

using std::remove_reference;
using std::make_tuple;

bool mcc::type::occur(const sptr<variable> & a, const type_t & b) {
    if (std::get_if<sptr<function>>(&b)) {
        bool ret = false;
        for (auto & i : std::get<1>(std::get<sptr<function>>(b)->value)) {
            ret |= occur(a, i);
        }
        ret |= occur(a, std::get<0>(std::get<sptr<function>>(b)->value));
        return ret;
    } else if (std::get_if<sptr<tuple>>(&b)) {
        bool ret = false;
        for (auto & i : std::get<sptr<tuple>>(b)->value) {
            ret |= occur(a, i);
        }
        return ret;
    } else if (std::get_if<sptr<array>>(&b)) {
        return occur(a, std::get<sptr<array>>(b)->value);
    } else if (std::get_if<sptr<variable>>(&b)) {
        if (a->value.get() == (std::get<sptr<variable>>(b)->value).get()) {
            assert(false);
            return true;
        } else if (*(std::get<sptr<variable>>(b)->value) == nullptr) {
            return false;
        } else {
            return occur(a, **(std::get<sptr<variable>>(b)->value));
        }
    } else {
        return false;
    }
}

bool mcc::type::unify(const type_t & a, const type_t & b) {
#ifdef DEBUG
    printf("a: ");
    printer()(a);
    putchar('\n');
    printf("b: ");
    printer()(b);
    putchar('\n');
#endif
    if (std::get_if<sptr<unit>>(&a) && std::get_if<sptr<unit>>(&b)) {
        return true;
    } else if (std::get_if<sptr<integer>>(&a) && std::get_if<sptr<integer>>(&b)) {
        return true;
    } else if (std::get_if<sptr<floating_point>>(&a) && std::get_if<sptr<floating_point>>(&b)) {
        return true;
    } else if (std::get_if<sptr<boolean>>(&a) && std::get_if<sptr<boolean>>(&b)) {
        return true;
    } else if (std::get_if<sptr<function>>(&a) && std::get_if<sptr<function>>(&b)) {
        bool ret = true;
        const sptr<function> & a_p = std::get<sptr<function>>(a);
        const sptr<function> & b_p = std::get<sptr<function>>(b);
        if (std::get<1>(a_p->value).size() != std::get<1>(b_p->value).size()) {
            return false;
        }
        auto i = std::get<1>(a_p->value).begin();
        auto j = std::get<1>(b_p->value).begin();
        for (; (i != std::get<1>(a_p->value).end() && j != std::get<1>(b_p->value).end()); ++i, ++j) {
            ret &= unify(*i, *j);
        }
        ret &= unify(std::get<0>(a_p->value), std::get<0>(b_p->value));
        return ret;
    } else if (std::get_if<sptr<tuple>>(&a) && std::get_if<sptr<tuple>>(&b)) {
        bool ret = true;
        const sptr<tuple> & a_p = std::get<sptr<tuple>>(a);
        const sptr<tuple> & b_p = std::get<sptr<tuple>>(b);
        if (a_p->value.size() != b_p->value.size()) {
            return false;
        }
        auto i = a_p->value.begin();
        auto j = b_p->value.begin();
        for (; (i != a_p->value.end() && j != b_p->value.end()); ++i, ++j) {
            ret &= unify(*i, *j);
        }
        return ret;
    } else if (std::get_if<sptr<array>>(&a) && std::get_if<sptr<array>>(&b)) {
        bool ret = unify(std::get<sptr<array>>(a)->value, std::get<sptr<array>>(b)->value);
        return ret;
    } else if (std::get_if<sptr<variable>>(&a) && std::get_if<sptr<variable>>(&b) &&
               (std::get<sptr<variable>>(a)->value.get() ==
                std::get<sptr<variable>>(b)->value.get())) {
        return true;
    } else if (std::get_if<sptr<variable>>(&a)) {
        sptr<variable> & a_p = const_cast<sptr<variable> &>(std::get<sptr<variable>>(a));
        if ((*a_p->value).get() == nullptr) {
            if (std::get_if<sptr<variable>>(&b) &&
                (*std::get<sptr<variable>>(b)->value != nullptr)) {
                sptr<variable> & b_p = const_cast<sptr<variable> &>(std::get<sptr<variable>>(b));
                return unify(a, **(b_p->value));
            } else if (occur(a_p, b)) {
                return false;
            } else {
                *a_p->value = std::make_shared<type_t>(b);
                return true;
            }
        } else {
            bool ret = unify(**(a_p->value), b);
            return ret;
        }
    } else if (std::get_if<sptr<variable>>(&b)) {
        sptr<variable> & b_p = const_cast<sptr<variable> &>(std::get<sptr<variable>>(b));
        if ((*b_p->value).get() == nullptr) {
            if (std::get_if<sptr<variable>>(&a) &&
                (*std::get<sptr<variable>>(a)->value != nullptr)) {
                sptr<variable> & a_p = const_cast<sptr<variable> &>(std::get<sptr<variable>>(a));
                return unify(b, **(a_p->value));
            } else if (occur(b_p, a)) {
                return false;
            } else {
                *b_p->value = std::make_shared<type_t>(a);
                return true;
            }
        } else {
            bool ret = unify(**(b_p->value), a);
            return ret;
        }
    } else {
        return false;
    }
}

type_t mcc::type::deref_typ(type_t & t) {
    if (std::get_if<sptr<function>>(&t)) {
        sptr<function> & t_p = std::get<sptr<function>>(t);
        std::vector<type_t> deref_args;
        type_t deref_ret = deref_typ(std::get<0>(t_p->value));
        std::transform(std::get<1>(t_p->value).begin(), std::get<1>(t_p->value).end(), std::back_inserter(deref_args), deref_typ);
        std::get<0>(t_p->value) = std::move(deref_ret);
        std::get<1>(t_p->value) = std::move(deref_args);
        return t_p;
    } else if (std::get_if<sptr<tuple>>(&t)) {
        sptr<tuple> & t_p = std::get<sptr<tuple>>(t);
        std::vector<type_t> deref_elms;
        std::transform(t_p->value.begin(), t_p->value.end(), std::back_inserter(deref_elms), deref_typ);
        t_p->value = std::move(deref_elms);
        return t_p;
    } else if (std::get_if<sptr<array>>(&t)) {
        sptr<array> & t_p = std::get<sptr<array>>(t);
        t_p->value = deref_typ(t_p->value);
        return t_p;
    } else if (std::get_if<sptr<variable>>(&t)) {
        sptr<variable> & t_p = std::get<sptr<variable>>(t);
        if (*(t_p->value) == nullptr) {
            std::cerr << "uninstantiated type variable detected; assuming int@." << std::endl;
            //assert(false);
            auto ret = get_integer();
            *(t_p->value) = std::make_shared<type_t>(ret);
            return ret;
        } else {
            type_t ret = deref_typ(**(t_p->value));
            *t_p->value = std::make_shared<type_t>(ret);
            return ret;
        }
    } else {
        return t;
    }
}

void type::deref_id_typ(sptr<parser::identifier> & i) {
    std::get<1>(i->value) = deref_typ(std::get<1>(i->value));
}

struct deref_term_i {

    using result_type = void;

    template <typename T>
    result_type operator() (T && ast) const { }

    result_type operator() (parser::ast & ast) const {
        std::visit(deref_term_i(), ast);
    }

    result_type operator() (sptr<parser::identifier> & ast) const {
        deref_id_typ(ast);
    }

    template <typename Op>
    result_type operator() (sptr<parser::unary<Op>> & ast) const {
        std::visit(deref_term_i(), ast->value);
    }

    template <typename Op>
    result_type operator() (sptr<parser::binary<Op>> & ast) const {
        for_each_tuple(ast->value, [] (auto && a) { std::visit(deref_term_i(), a); });
    }

    result_type operator() (sptr<parser::branch> & ast) const {
        for_each_tuple(ast->value, [] (auto && a) { std::visit(deref_term_i(), a); });
    }

    result_type operator() (sptr<parser::let> & ast) const {
        deref_id_typ(std::get<0>(ast->value));
        std::visit(deref_term_i(), std::get<1>(ast->value));
        std::visit(deref_term_i(), std::get<2>(ast->value));
    }

    result_type operator() (sptr<parser::external> & ast) const {
        deref_id_typ(std::get<0>(ast->value));
    }

    result_type operator() (sptr<parser::global> & ast) const {
        deref_id_typ(std::get<0>(ast->value));
        std::visit(deref_term_i(), std::get<1>(ast->value));
    }

    result_type operator() (sptr<parser::let_tuple> & ast) const {
        std::for_each(std::get<0>(ast->value).begin(), std::get<0>(ast->value).end(), deref_id_typ);
        std::visit(deref_term_i(), std::get<1>(ast->value));
        std::visit(deref_term_i(), std::get<2>(ast->value));
    }

    result_type operator() (sptr<parser::global_tuple> & ast) const {
        std::for_each(std::get<0>(ast->value).begin(), std::get<0>(ast->value).end(), deref_id_typ);
        std::visit(deref_term_i(), std::get<1>(ast->value));
    }

    result_type operator() (sptr<parser::let_rec> & ast) const {
        deref_id_typ(std::get<0>(ast->value));
        std::for_each(std::get<1>(ast->value).begin(), std::get<1>(ast->value).end(), deref_id_typ);
        std::visit(deref_term_i(), std::get<2>(ast->value));
        std::visit(deref_term_i(), std::get<3>(ast->value));
    }

    result_type operator() (sptr<parser::global_rec> & ast) const {
        deref_id_typ(std::get<0>(ast->value));
        std::for_each(std::get<1>(ast->value).begin(), std::get<1>(ast->value).end(), deref_id_typ);
        std::visit(deref_term_i(), std::get<2>(ast->value));
    }

    result_type operator() (sptr<parser::app> & ast) const {
        std::visit(deref_term_i(), std::get<0>(ast->value));
        std::for_each(std::get<1>(ast->value).begin(), std::get<1>(ast->value).end(), [] (auto && a) { std::visit(deref_term_i(), a); });
    }

    result_type operator() (sptr<parser::tuple> & ast) const {
        std::for_each(ast->value.begin(), ast->value.end(), [] (auto && a) { std::visit(deref_term_i(), a); });
    }

    result_type operator() (sptr<parser::array> & ast) const {
        for_each_tuple(ast->value, [] (auto && a) { std::visit(deref_term_i(), a); });
    }

    result_type operator() (sptr<parser::get> & ast) const {
        for_each_tuple(ast->value, [] (auto && a) { std::visit(deref_term_i(), a); });
    }

    result_type operator() (sptr<parser::put> & ast) const {
        for_each_tuple(ast->value, [] (auto && a) { std::visit(deref_term_i(), a); });
    }

};

void type::deref_term(parser::toplevel_t & ast) {
    return std::visit(deref_term_i(), ast);
}

namespace {
    using env_t = map_stack<std::string, type::type_t>;
    struct pass {

        using result_type = type_t;
        env_t & env;

        pass(env_t & e) : env(e) { }

        result_type operator() (const parser::toplevel_t & ast) const {
            return std::visit(pass(env), ast);
        }

        result_type operator() (const parser::ast & ast) const {
            return std::visit(pass(env), ast);
        }

        result_type operator() (const sptr<parser::unit> & ast) const {
            return get_unit();
        }

        result_type operator() (const sptr<parser::integer> & ast) const {
            return get_integer();
        }

        result_type operator() (const sptr<parser::floating_point> & ast) const {
            return get_floating_point();
        }

        result_type operator() (const sptr<parser::boolean> & ast) const {
            return get_boolean();
        }

        template <typename Op>
        result_type operator() (const sptr<parser::unary<Op>> & ast) const {
            assert(unify(std::visit(pass(env), ast->value), Op::result_type()));
            return Op::result_type();
        }

        result_type operator() (const sptr<parser::branch> & ast) const {
            type_t condition = pass(env)(std::get<0>(ast->value));
            type_t t_branch = pass(env)(std::get<1>(ast->value));
            type_t f_branch = pass(env)(std::get<2>(ast->value));
            assert(unify(condition, get_boolean()));
            assert(unify(t_branch, f_branch));
            return t_branch;
        }

        result_type operator() (const sptr<parser::let> & ast) {
            env.push();
            const sptr<parser::identifier> & ident = std::get<0>(ast->value);
            assert(unify(std::get<1>(ident->value),
                         std::visit(pass(env), std::get<1>(ast->value))));
            env.insert(make_pair(std::get<0>(ident->value), std::get<1>(ident->value)));
            auto && ret = pass(env)(std::get<2>(ast->value));
            env.pop();
            return std::move(ret);
        }

        result_type operator() (const sptr<parser::external> & ast) {
            auto ident = std::get<0>(ast->value);
            env.insert(make_pair(std::get<0>(ident->value), std::get<1>(ident->value)));
            return get_unit();
        }

        result_type operator() (const sptr<parser::global> & ast) {
            const sptr<parser::identifier> & ident = std::get<0>(ast->value);
            assert(unify(std::get<1>(ident->value),
                         std::visit(pass(env), std::get<1>(ast->value))));
            env.insert(make_pair(std::get<0>(ident->value), std::get<1>(ident->value)));
            return get_unit();
        }

        result_type operator() (const sptr<parser::let_tuple> & ast) {
            env.push();
            type_t t1 = pass(env)(std::get<1>(ast->value));
            std::vector<type_t> t2;
            for (auto && i : std::get<0>(ast->value)) {
                t2.push_back(std::get<1>(i->value)); // type
                env.insert(make_pair(std::get<0>(i->value), std::get<1>(i->value))); // key pair
            }
            assert(unify(t1, make_shared<tuple>(std::move(t2))));
            auto && ret = pass(env)(std::get<2>(ast->value));
            env.pop();
            return std::move(ret);
        }

        result_type operator() (const sptr<parser::global_tuple> & ast) {
            type_t t1 = pass(env)(std::get<1>(ast->value));
            std::vector<type_t> t2;
            for (auto && i : std::get<0>(ast->value)) {
                t2.push_back(std::get<1>(i->value)); // type
                env.insert(make_pair(std::get<0>(i->value), std::get<1>(i->value))); // key pair
            }
            assert(unify(t1, make_shared<tuple>(std::move(t2))));
            return get_unit();
        }

        result_type operator() (const sptr<parser::let_rec> & ast) {
            env.push();
            const sptr<parser::identifier> & ident = std::get<0>(ast->value);
            env.insert(make_pair(std::get<0>(ident->value), std::get<1>(ident->value))); // key pair
            auto && ret = pass(env)(std::get<3>(ast->value)); // exp
            std::vector<type_t> vec_tmp;
            for (auto && i : std::get<1>(ast->value)) {
                vec_tmp.push_back(std::get<1>(i->value));
                env.insert(make_pair(std::get<0>(i->value), std::get<1>(i->value)));
            }
            auto && type_tmp = make_shared<function>(make_tuple(pass(env)(std::get<2>(ast->value)), vec_tmp));
            assert(unify(std::get<1>(ident->value), type_tmp));
            env.pop();
            return std::move(ret);
        }

        result_type operator() (const sptr<parser::global_rec> & ast) {
            const sptr<parser::identifier> & ident = std::get<0>(ast->value);
            env.insert(make_pair(std::get<0>(ident->value), std::get<1>(ident->value))); // key pair
            std::vector<type_t> vec_tmp;
            env.push();
            for (auto && i : std::get<1>(ast->value)) {
                vec_tmp.push_back(std::get<1>(i->value));
                env.insert(make_pair(std::get<0>(i->value), std::get<1>(i->value)));
            }
            auto && type_tmp = make_shared<function>(make_tuple(pass(env)(std::get<2>(ast->value)), vec_tmp));
            assert(unify(std::get<1>(ident->value), type_tmp));
            env.pop();
            return get_unit();
        }

        result_type operator() (const sptr<parser::identifier> & ast) const {
            std::string ident = std::get<0>(ast->value);
            auto internal_idx = env.find(ident);
            if (internal_idx != env.end()) {
                return internal_idx->second;
            } else {
                assert(false);
            }
        }

        template <typename Op, std::enable_if_t<std::disjunction_v<std::is_same<Op, op_add>,
                                                                   std::is_same<Op, op_sub>,
                                                                   std::is_same<Op, op_mul>,
                                                                   std::is_same<Op, op_div>>,
                                                std::nullptr_t> = nullptr>
        result_type operator() (const sptr<parser::binary<Op>> & ast) const {
            auto test_type = get_integer();
            assert(unify(pass(env)(std::get<0>(ast->value)), test_type));
            assert(unify(pass(env)(std::get<1>(ast->value)), test_type));
            return test_type;
        }

        template <typename Op, std::enable_if_t<std::disjunction_v<std::is_same<Op, op_fadd>,
                                                                   std::is_same<Op, op_fsub>,
                                                                   std::is_same<Op, op_fmul>,
                                                                   std::is_same<Op, op_fdiv>>,
                                                std::nullptr_t> = nullptr>
        result_type operator() (const sptr<parser::binary<Op>> & ast) const {
            auto test_type = get_floating_point();
            assert(unify(pass(env)(std::get<0>(ast->value)), test_type));
            assert(unify(pass(env)(std::get<1>(ast->value)), test_type));
            return test_type;
        }

        template <typename Op, std::enable_if_t<std::disjunction_v<std::is_same<Op, op_eq>,
                                                                   std::is_same<Op, op_le>>,
                                                std::nullptr_t> = nullptr>
        result_type operator() (const sptr<parser::binary<Op>> & ast) const {
            assert(unify(pass(env)(std::get<0>(ast->value)),
                         pass(env)(std::get<1>(ast->value))));
            return get_boolean();
        }

        result_type operator() (const sptr<parser::array> & ast) const {
            type_t array_size = pass(env)(std::get<0>(ast->value));
            type_t array_type = pass(env)(std::get<1>(ast->value));
            assert(unify(array_size, get_integer()));
            return make_shared<array>(std::move(array_type));
        }

        result_type operator() (const sptr<parser::put> & ast) const {
            type_t t0 = pass(env)(std::get<0>(ast->value));
            type_t t1 = pass(env)(std::get<1>(ast->value));
            type_t t2 = pass(env)(std::get<2>(ast->value));
            assert(unify(t1, get_integer()));
            assert(unify(t0, make_shared<array>(std::move(t2))));
            return get_unit();
        }

        result_type operator() (const sptr<parser::tuple> & ast) const {
            std::vector<type_t> t;
            std::transform(ast->value.begin(), ast->value.end(), std::back_inserter(t), pass(env));
            return make_shared<tuple>(std::move(t));
        }

        result_type operator() (const sptr<parser::get> & ast) const {
            type_t t_i = make_shared<variable>();
            auto t_a = make_shared<array>(std::move(t_i));
            assert(unify(t_a, pass(env)(std::get<0>(ast->value))));
            assert(unify(pass(env)(std::get<1>(ast->value)), get_integer()));
            return t_a->value;
        }

        result_type operator() (const sptr<parser::app> & ast) const {
            auto t_i = make_shared<variable>();
            std::vector<type_t> vec_tmp;
            std::transform(std::get<1>(ast->value).begin(), std::get<1>(ast->value).end(), std::back_inserter(vec_tmp), pass(env));
            auto type_tmp = pass(env)(std::get<0>(ast->value));
            assert(unify(type_tmp, make_shared<function>(make_tuple(t_i, vec_tmp))));
            return t_i;
        }

    };

}

parser::module type::f(parser::module && mod) {
    env_t env;
    type_t module_type;
    for (auto && top : mod.value) {
        pass p(env);
        module_type = p(top);
    };
    for (auto && top : mod.value) deref_term(top);

    mod.module_type = deref_typ(module_type);

    return std::move(mod);
}
