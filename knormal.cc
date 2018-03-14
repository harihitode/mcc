#include <iostream>
#include "knormal.h"
#include "typing.h"

namespace {
    using namespace mcc;
    using namespace mcc::knormal;
    using std::make_shared;
    using std::make_tuple;
    using type::unwrap;

    template <typename C>
    std::tuple<ast, type::type_t>
    insert_let(std::tuple<ast, type::type_t> && e, C f) {
        if (std::get_if<sptr<identifier>>(&std::get<0>(e))) {
            auto && x = std::get<sptr<identifier>>(std::get<0>(e));
            return f(x);
        } else {
            auto && x = make_shared<identifier>(id::gentmp(std::get<1>(e)), std::get<1>(e), /* is_global = */ false);
            auto && new_body = f(x);
            auto && new_let = make_shared<let>(make_tuple(x,
                                                          std::move(std::get<0>(e)),
                                                          std::move(std::get<0>(new_body))));
            return make_tuple(std::move(new_let),
                              std::get<1>(new_body));
        }
    }

    struct pass {

        using result_type = std::tuple<ast, type::type_t>;
        pass() { }

        result_type operator() (const parser::ast & ast) const {
            return std::visit(pass(), ast);
        }

        result_type operator() (const sptr<parser::unit> & ast) const {
            return make_tuple(ast, type::get_unit());
        }

        result_type operator() (const sptr<parser::integer> & ast) const {
            return make_tuple(ast, type::get_integer());
        }

        result_type operator() (const sptr<parser::floating_point> & ast) const {
            return make_tuple(ast, type::get_floating_point());
        }

        result_type operator() (const sptr<parser::boolean> & ast) const {
            return make_tuple(ast, type::get_boolean());
          }

        template <typename Op>
        result_type operator() (const sptr<parser::unary<Op>> & ast) const {
            return insert_let(pass()(ast->value), [] (auto & x) {
                    return make_tuple(make_shared<unary<Op>>(x), Op::result_type());
                });
        }

        result_type operator() (const sptr<parser::unary<op_not>> & ast) const {
            auto && t = make_shared<parser::branch>(make_tuple(ast->value,
                                                               value::get_const_false(),
                                                               value::get_const_true()));
            return pass()(std::move(t));
        }

        // template <typename Op, std::enable_if_t<std::negation_v<std::disjunction<std::is_same<Op, op_eq>,
        //                                                                          std::is_same<Op, op_le>>>,
        //                                         std::nullptr_t> = nullptr>
        template <typename Op>
        result_type operator() (const sptr<parser::binary<Op>> & ast) const {
            auto && lhs = pass()(std::get<0>(ast->value));
            auto && rhs = pass()(std::get<1>(ast->value));
            return insert_let(std::move(lhs), [&] (auto & x) {
                    return insert_let(std::move(rhs), [&] (auto & y) {
                            return make_tuple(make_shared<binary<Op>>(make_tuple(x, y)), Op::result_type());
                        });
                });
        }

        // template <typename Op, std::enable_if_t<std::disjunction_v<std::is_same<Op, op_eq>,
        //                                                            std::is_same<Op, op_le>>,
        //                                         std::nullptr_t> = nullptr>
        // result_type operator() (sptr<parser::binary<Op>> && ast) const {
        //     auto t = make_shared<parser::branch>(make_tuple(std::move(ast),
        //                                                     value::get_const_true(),
        //                                                     value::get_const_false()));
        //     return pass(env)(std::move(t));
        // }

        result_type operator() (const sptr<parser::branch> & ast) const {
            // remove not
            if (auto not_exp = std::get_if<sptr<parser::unary<op_not>>>(&std::get<0>(ast->value))) {
                // swap
                auto t = make_shared<parser::branch>(make_tuple((*not_exp)->value,
                                                                std::get<2>(ast->value),
                                                                std::get<1>(ast->value)));
                return pass()(t);
            } else {
                return insert_let(pass()(std::get<0>(ast->value)), [&] (auto & condition) {
                        auto && t_branch = pass()(std::get<1>(ast->value));
                        auto && f_branch = pass()(std::get<2>(ast->value));
                        return make_tuple(make_shared<branch>(make_tuple(condition,
                                                                         std::move(std::get<0>(t_branch)),
                                                                         std::move(std::get<0>(f_branch)))),
                                          std::get<1>(t_branch));
                    });
            }
        }

        result_type operator() (const sptr<parser::identifier> & ast) const {
            std::string ident = std::get<0>(ast->value);
            return make_tuple(ast, std::get<1>(ast->value));
        }

        result_type operator() (const sptr<parser::let> & ast) {
            auto && var = std::get<0>(ast->value);
            auto && val = pass()(std::move(std::get<1>(ast->value)));
            auto && cnt = pass()(std::move(std::get<2>(ast->value)));
            auto && new_let = make_shared<let>(make_tuple(std::move(var),
                                                          std::move(std::get<0>(val)),
                                                          std::move(std::get<0>(cnt))));
            return make_tuple(std::move(new_let), std::get<1>(cnt));
        }

        result_type operator() (const sptr<parser::let_tuple> & ast) {
            return insert_let(pass()(std::move(std::get<1>(ast->value))), [&] (auto & x) {
                    std::vector<sptr<identifier>> xts;
                    for (auto && i : std::get<0>(ast->value)) {
                        xts.emplace_back(i);
                    }
                    auto && cnt = pass()(std::move(std::get<2>(ast->value)));
                    auto && new_let_tuple = make_shared<let_tuple>(make_tuple(std::move(xts),
                                                                              x,
                                                                              std::move(std::get<0>(cnt))));
                    return make_tuple(std::move(new_let_tuple),
                                      std::get<1>(cnt));
                });
        }

        result_type operator() (const sptr<parser::let_rec> & ast) {
            auto && name = std::get<0>(ast->value);
            auto && cnt = pass()(std::move(std::get<3>(ast->value)));
            std::vector<sptr<identifier>> yts;
            // env_update
            for (auto & arg : std::get<1>(ast->value)) {
                yts.emplace_back(std::move(arg));
            }
            auto && body = pass()(std::move(std::get<2>(ast->value)));
            auto && new_let_rec = make_shared<let_rec>(make_tuple(name,
                                                                  std::move(yts),
                                                                  std::move(std::get<0>(body)),
                                                                  std::move(std::get<0>(cnt))));
            return make_tuple(std::move(new_let_rec),
                              std::get<1>(cnt));
        }

        result_type operator() (const sptr<parser::put> & ast) const {
            return insert_let(pass()(std::get<0>(ast->value)), [&] (auto & x) {
                    return insert_let(pass()(std::get<1>(ast->value)), [&] (auto & y) {
                            return insert_let(pass()(std::get<2>(ast->value)), [&] (auto & z) {
                                    return make_tuple(make_shared<put>(make_tuple(x, y, z)), type::get_unit());
                                });
                        });
                });
        }

        result_type operator() (const sptr<parser::get> & ast) const {
            auto && pass_e1 = pass()(std::get<0>(ast->value));
            auto pass_e1_t = std::get<sptr<type::array>>(unwrap(std::get<1>(pass_e1)))->value;
            return insert_let(std::move(pass_e1), [&] (auto & x) {
                    return insert_let(pass()(std::get<1>(ast->value)), [&] (auto & y) {
                            return make_tuple(make_shared<get>(make_tuple(x, y)), pass_e1_t);
                        });
                });
        }

        result_type operator() (const sptr<parser::array> & ast) const {
            return insert_let(pass()(std::get<0>(ast->value)), [&] (auto & x) {
                    auto && g_e2 = pass()(std::get<1>(ast->value));
                    auto t2 = std::get<1>(g_e2);
                    return insert_let(std::move(g_e2), [&] (auto & y) {
                            return make_tuple(make_shared<array>(make_tuple(x, y)),
                                              make_shared<type::array>(std::move(t2)));
                        });
                });
        }

        result_type operator() (const sptr<parser::tuple> & ast) const {
            auto tuple_end = ast->value.end();
            std::vector<sptr<identifier>> xs;
            std::vector<type::type_t> ts;
            auto bind = recursive([&] (auto bind, decltype(xs) && xs, decltype(ts) && ts, decltype(tuple_end) t) {
                    if (t == tuple_end) {
                        knormal::ast && val = make_shared<tuple>(std::move(xs));
                        type::type_t && typ = make_shared<type::tuple>(std::move(ts));
                        return make_tuple(std::move(val), std::move(typ));
                    } else {
                        auto && pass_tmp = pass()(std::move(*t));
                        auto pass_tmp_t = std::get<1>(pass_tmp);
                        return insert_let(std::move(pass_tmp), [&] (auto & x) {
                                xs.push_back(x);
                                ts.push_back(pass_tmp_t);
                                return bind(bind, std::move(xs), std::move(ts), ++t);
                            });
                    }
                });
            return bind(std::move(xs), std::move(ts), ast->value.begin());
        }

        result_type operator() (const sptr<parser::app> & ast) const {
            std::vector<sptr<identifier>> xs;
            auto args_end = std::get<1>(ast->value).end();
            auto && g_e1 = pass()(std::get<0>(ast->value));
            auto g_e1_t = std::get<0>(std::get<sptr<type::function>>(unwrap(std::get<1>(g_e1)))->value);
            return insert_let(std::move(g_e1), [&] (auto & f) {
                    auto bind = recursive([&] (auto bind, decltype(xs) && xs, decltype(args_end) t) {
                            if (t == args_end) {
                                knormal::ast && val = make_shared<app>(make_tuple(f, std::move(xs)));
                                type::type_t && typ = std::move(g_e1_t);
                                return make_tuple(std::move(val), std::move(typ));
                            } else {
                                auto && pass_tmp = pass()(std::move(*t));
                                return insert_let(std::move(pass_tmp), [&] (auto & x) {
                                        xs.push_back(x);
                                        return bind(bind, std::move(xs), ++t);
                                    });
                            }
                        });
                    return bind(std::move(xs), std::get<1>(ast->value).begin());
                });
        }

    };

    struct global_pass {
        using result_type = toplevel_t;

        global_pass() { }

        result_type operator() (const sptr<parser::external> & ast) {
            return ast;
        }

        result_type operator() (const sptr<parser::global> & ast) {
            auto && var = std::get<0>(ast->value);
            auto && val = pass()(std::move(std::get<1>(ast->value)));
            return make_shared<global>(make_tuple(std::move(var),
                                                  std::move(std::get<0>(val))));
        }

        result_type operator() (const sptr<parser::global_tuple> & ast) {
            std::vector<sptr<identifier>> xts;
            auto && val = pass()(std::move(std::get<1>(ast->value)));
            for (auto && i : std::get<0>(ast->value)) {
                xts.emplace_back(i);
            }
            return make_shared<global_tuple>(make_tuple(std::move(xts),
                                                        std::move(std::get<0>(val))));
        }

        result_type operator() (const sptr<parser::global_rec> & ast) {
            auto && name = std::get<0>(ast->value);
            std::vector<sptr<identifier>> yts;
            for (auto & arg : std::get<1>(ast->value)) {
                yts.emplace_back(std::move(arg));
            }
            auto && body = pass()(std::move(std::get<2>(ast->value)));
            return make_shared<global_rec>(make_tuple(name,
                                                      std::move(yts),
                                                      std::move(std::get<0>(body))));
        }

        template <typename T>
        result_type operator() (const T & ast) {
            return std::get<0>(std::visit(pass(), ast));
        }

    };

}

std::vector<mcc::knormal::toplevel_t> mcc::knormal::f(std::vector<mcc::parser::toplevel_t> && ast) {
    std::vector<mcc::knormal::toplevel_t> ret;
    for (auto && t : ast) {
        ret.emplace_back(std::visit(global_pass(), t));
    }
    return ret;
}
