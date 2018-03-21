#include <iostream>
#include <fstream>
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/include/at_c.hpp>

#include "ast.h"
#include "parser.h"

namespace {

    class mcc_context_tag;

    namespace x3 = boost::spirit::x3;
    using namespace mcc;
    using namespace mcc::parser;

    using boost::fusion::at_c;
    using std::make_shared;
    using std::make_tuple;

    using x3::lit;
    using x3::lexeme;
    using x3::omit;
    using x3::int_;
    using x3::uint_;
    using x3::double_;
    using x3::char_;
    using x3::bool_;
    using x3::string;
    using x3::_attr;
    using x3::_val;
    using x3::eps;
    using x3::alpha;
    using x3::alnum;
    using x3::space;
    using x3::confix;
    using x3::raw;

    // ex) .1 (=0.1) is NOT accepted, 1. (=1.0) is accepted
    // floating point format for mincaml
    template <typename Type>
        struct mincaml_real_policies : x3::strict_real_policies<Type> {
        static bool const allow_leading_dot = false;
        static bool const allow_trailing_dot = true;
    };
    x3::real_parser<double, mincaml_real_policies<double>> const real_ = { };

    // keywords below are not to be used as identifiers
    struct mincaml_keywords : x3::symbols<x3::unused_type> {
        mincaml_keywords() {
            add ("if", x3::unused)
                ("then", x3::unused)
                ("else", x3::unused)
                ("true", x3::bool_)
                ("false", x3::bool_)
                ("let", x3::unused)
                ("rec", x3::unused)
                ("in", x3::unused)
                ("Array.create", x3::unused)
                ("create_array", x3::unused)
                ("extern", x3::unused);
        }
    } const keywords;

    x3::rule<class toplevel_class, std::vector<toplevel_t>> const toplevel = "toplevel";
    x3::rule<class exp_class, toplevel_t> const exp = "exp";
    x3::rule<class prec_let_class, ast> const prec_let = "prec_exp";
    x3::rule<class simple_exp_class, ast> const simple_exp = "simple_exp";
    x3::rule<class gram_integer_class, sptr<integer>> const gram_integer = "integer";
    x3::rule<class gram_floating_point_class, sptr<floating_point>> const gram_floating_point = "floating_point";
    x3::rule<class gram_boolean_class, sptr<boolean>> const gram_boolean = "boolean";
    x3::rule<class gram_unit_class, sptr<unit>> const gram_unit = "unit";
    x3::rule<class gram_identifier_class, sptr<identifier>> const gram_identifier = "identifier";
    x3::rule<class gram_branch_class, ast> const gram_branch = "branch";
    x3::rule<class gram_let_binding_class, ast> const gram_let_binding = "let_binding";
    x3::rule<class gram_let_class, sptr<global>> const gram_let = "let";
    x3::rule<class gram_let_rec_class, sptr<global_rec>> const gram_let_rec = "let_rec";
    x3::rule<class gram_let_tuple_class, sptr<global_tuple>> const gram_let_tuple = "let_tuple";
    x3::rule<class comment_class, ast> const comment = "comment";
    x3::rule<class prec_semicolon_class, ast> const prec_semicolon = "prec_semicolon";
    x3::rule<class prec_if_class, ast> const prec_if = "prec_if";
    x3::rule<class then_else_class, ast> const then_else = "then_else";
    x3::rule<class gram_get_class, std::vector<ast>> const gram_get = "get";
    x3::rule<class prec_put_class, ast> const prec_put = "prec_put";
    x3::rule<class gram_put_class, ast> const gram_put = "put";
    x3::rule<class prec_dot_class, ast> const prec_dot = "prec_dot";
    x3::rule<class prec_comma_class, ast> const prec_comma = "prec_comma";
    x3::rule<class prec_comp_class, ast> const prec_comp = "prec_comp";
    x3::rule<class comp_loop_class, std::function<ast (ast const &)>> const comp_loop = "comp_loop";
    x3::rule<class prec_additive_class, ast> const prec_additive = "prec_additive";
    x3::rule<class additive_loop_class, std::function<ast (ast const &)>> const additive_loop = "additive_loop";
    x3::rule<class prec_multiplicative_class, ast> const prec_multiplicative = "prec_multiplicative";
    x3::rule<class multiplicative_loop_class, std::function<ast (ast const &)>> const multiplicative_loop = "multiplicative_loop";
    x3::rule<class prec_unary_minus_class, ast> const prec_unary_minus = "prec_unary_minus";
    x3::rule<class prec_app_class, ast> const prec_app = "prec_app";
    x3::rule<class gram_external_class, sptr<external>> const gram_external = "gram_external";
    x3::rule<class gram_type_class, type::type_t> const gram_type = "gram_type";
    x3::rule<class gram_simple_type_class, type::type_t> const gram_simple_type = "gram_simple_type";
    x3::rule<class gram_constr_type_class, type::type_t> const gram_constr_type = "gram_constr_type";
    x3::rule<class gram_tuple_type_class, type::type_t> const gram_tuple_type = "gram_tuple_type";
    x3::rule<class gram_func_type_class, type::type_t> const gram_func_type = "gram_func_type";

    // comment skip parser
    auto const comment_def = confix("(*", "*)")[*(comment | char_ - "*)")];
    auto const toplevel_def = (exp % (&lit("external") | &lit("let") | lit(";;")));
    auto const exp_def = gram_external | prec_let | gram_let_rec | gram_let_tuple | gram_let;
    auto const prec_let_def = gram_let_binding | prec_semicolon;
    auto const gram_let_binding_def = (gram_let_rec >> lit("in") >> prec_let) [([] (auto && ctx) {
                auto && def_val = std::move(at_c<0>(_attr(ctx))->value);
                std::get<0>(def_val)->is_global = false;
                _val(ctx) = make_shared<let_rec>(make_tuple(std::move(std::get<0>(def_val)),
                                                            std::move(std::get<1>(def_val)),
                                                            std::move(std::get<2>(def_val)),
                                                            std::move(at_c<1>(_attr(ctx)))));
            })] | (gram_let_tuple >> lit("in") >> prec_let) [([] (auto && ctx) {
                auto && def_val = std::move(at_c<0>(_attr(ctx))->value);
                for (auto && i : std::get<0>(def_val)) i->is_global = false;
                _val(ctx) = make_shared<let_tuple>(make_tuple(std::move(std::get<0>(def_val)),
                                                              std::move(std::get<1>(def_val)),
                                                              std::move(at_c<1>(_attr(ctx)))));
            })] | (gram_let >> lit("in") >> prec_let) [([](auto && ctx) {
                auto && def_val = std::move(at_c<0>(_attr(ctx))->value);
                std::get<0>(def_val)->is_global = false;
                _val(ctx) = make_shared<let>(make_tuple(std::move(std::get<0>(def_val)),
                                                        std::move(std::get<1>(def_val)),
                                                        std::move(at_c<1>(_attr(ctx)))));
            })];
    auto const gram_let_def = (lit("let") >> gram_identifier >> '=' >> prec_let) [([] (auto && ctx) {
                at_c<0>(_attr(ctx))->is_global = true;
                _val(ctx) = make_shared<global>(make_tuple(std::move(at_c<0>(_attr(ctx))),
                                                           std::move(at_c<1>(_attr(ctx)))));
            })];
    auto const gram_let_tuple_def = (lit("let") >> confix('(', ')')[gram_identifier % ','] >> '=' >> prec_let) [([] (auto && ctx) {
                for (auto && i : at_c<0>(_attr(ctx))) i->is_global = true;
                _val(ctx) = make_shared<global_tuple>(make_tuple(std::move(at_c<0>(_attr(ctx))),
                                                                 std::move(at_c<1>(_attr(ctx)))));
            })];
    // wrong parse: let rectside = intsec_rectside.(0) in
    auto const gram_let_rec_def = (lit("let") >> lit("rec") >> gram_identifier >> +gram_identifier >> '=' >> prec_let) [([] (auto && ctx) {
                at_c<0>(_attr(ctx))->is_global = true;
                _val(ctx) = make_shared<global_rec>(make_tuple(std::move(at_c<0>(_attr(ctx))),
                                                               std::move(at_c<1>(_attr(ctx))),
                                                               std::move(at_c<2>(_attr(ctx)))));
            })];
    auto const gram_external_def = (lit("external") >> gram_identifier >> ':' >> gram_type >> '=' >> confix("\"", "\"")[*(char_ - "\"")]) [([] (auto && ctx) {
                auto && ident = at_c<0>(_attr(ctx));
                auto && t = at_c<1>(_attr(ctx));
                std::get<1>(ident->value) = t;
                ident->is_external = true;
                _val(ctx) = make_shared<external>(make_tuple(ident, at_c<2>(_attr(ctx))));
            })];
    auto const gram_type_def = gram_func_type;
    auto const gram_func_type_def = (gram_tuple_type % lit("->")) [([] (auto && ctx) {
                if (_attr(ctx).size() > 1) {
                    auto ret_t = _attr(ctx).back(); // copy for pop_back below
                    _attr(ctx).pop_back();
                    _val(ctx) = make_shared<type::function>(make_tuple(ret_t, std::move(_attr(ctx))), false);
                } else {
                    _val(ctx) = std::move(*std::begin(_attr(ctx)));
                }
            })];
    auto const gram_tuple_type_def = (gram_constr_type % char_('*')) [([] (auto && ctx) {
                if (_attr(ctx).size() > 1) {
                    _val(ctx) = make_shared<type::tuple>(std::move(_attr(ctx)));
                } else {
                    _val(ctx) = std::move(*std::begin(_attr(ctx)));
                }
            })];
    auto const gram_constr_type_def = (gram_simple_type >> *(string("array"))) [([] (auto && ctx) {
                auto && ret = std::move(at_c<0>(_attr(ctx)));
                std::for_each(std::begin(at_c<1>(_attr(ctx))), std::end(at_c<1>(_attr(ctx))), [&ret] (auto && i) {
                        ret = make_shared<type::array>(std::move(ret));
                    });
                _val(ctx) = std::move(ret);
            })];
    auto const gram_simple_type_def = lit("unit") [([] (auto && ctx) {
                _val(ctx) = type::get_unit();
            })] | lit("int") [([] (auto && ctx) {
                _val(ctx) = type::get_integer();
            })] | lit("float") [([] (auto && ctx) {
                _val(ctx) = type::get_floating_point();
            })] | lit("bool") [([] (auto && ctx) {
                _val(ctx) = type::get_boolean();
            })] | confix('(', ')')[gram_type] [([] (auto && ctx){
                _val(ctx) = std::move(_attr(ctx));
            })];
    auto const prec_dot_def = (simple_exp >> gram_get) [([] (auto && ctx) {
                if (at_c<1>(_attr(ctx)).size() > 0) {
                    auto && head = std::rbegin(at_c<1>(_attr(ctx)));
                    auto && tail = std::rend(at_c<1>(_attr(ctx)));
                    auto && base = std::move(at_c<0>(_attr(ctx)));
                    auto && ret = make_shared<get>(make_tuple(std::move(base), std::move(*head)));
                    head++;
                    for (auto idx = head; idx != tail; idx++) {
                        ret = make_shared<get>(make_tuple(std::move(ret), std::move(*idx)));
                    }
                    _val(ctx) = std::move(ret);
                } else {
                    _val(ctx) = std::move(at_c<0>(_attr(ctx)));
                }
            })];
    auto const gram_get_def = (lit('.') >> confix('(', ')')[prec_let] >> !lit("<-") >> gram_get) [([] (auto && ctx) {
                _val(ctx) = std::move(at_c<1>(_attr(ctx)));
                _val(ctx).emplace_back(std::move(at_c<0>(_attr(ctx))));
            })] | eps;
    auto const simple_exp_def = (gram_identifier |
                                 gram_floating_point |
                                 gram_integer |
                                 gram_boolean |
                                 gram_unit |
                                 confix('(', ')')[prec_let]
                                 );
    auto const gram_integer_def = uint_ [([] (auto && ctx) {
                _val(ctx) = value::get_const_integer(x3::get<mcc_context_tag>(ctx), _attr(ctx));
            })];
    auto const gram_floating_point_def = real_ [([] (auto && ctx) {
                _val(ctx) = value::get_const_floating_point(x3::get<mcc_context_tag>(ctx), _attr(ctx));
            })];
    auto const gram_boolean_def = bool_ [([] (auto && ctx) {
                _val(ctx) = value::get_const_boolean(x3::get<mcc_context_tag>(ctx), _attr(ctx));
            })];
    auto const gram_unit_def = lexeme[lit("()")] [([] (auto && ctx) {
                _val(ctx) = value::get_const_unit(x3::get<mcc_context_tag>(ctx));
            })];
    // want to use distinct directive of spirit x3 (currently unavailable)...
    auto const distinct_keywords = lexeme[keywords >> !(alnum | char_('_'))];
    auto const gram_identifier_def = raw[(lexeme[alpha >> *(alnum | char_('_'))] - distinct_keywords)] [([] (auto && ctx) {
                std::string value(std::begin(_attr(ctx)), std::end(_attr(ctx)));
                _val(ctx) = make_shared<identifier>(value);
            })] | (lit('_')) [([] (auto && ctx) {
                _val(ctx) = make_shared<identifier>(id::gentmp(x3::get<mcc_context_tag>(ctx), type::get_unit()), type::get_unit());
            })];
    auto const gram_branch_def = (lit("if") >> prec_if >> lit("then") >> then_else >> lit("else") >> then_else) [([] (auto && ctx) {
                _val(ctx) = make_shared<branch>(make_tuple(std::move(at_c<0>(_attr(ctx))),
                                                           std::move(at_c<1>(_attr(ctx))),
                                                           std::move(at_c<2>(_attr(ctx)))));
            })];
    auto const then_else_def = (&lit("let") >> prec_let) [([] (auto && ctx) {
                _val(ctx) = std::move(_attr(ctx));
            })] | prec_if [([] (auto && ctx) {
                _val(ctx) = std::move(_attr(ctx));
            })];
    auto const prec_semicolon_def = (prec_if >> *(lit(";") >> !lit(";") >> prec_let) >> -(lit(";") >> !lit(";"))) [([] (auto && ctx) {
                if (at_c<1>(_attr(ctx)).size() > 0) {
                    // if unit is not set
                    // print_int is typed as (ret<int>, arg<int>) not (ret<unit>, arg<int>)
                    std::shared_ptr<let> ret;
                    std::function<ast (ast &)> f = [] (ast & e) { return e; };
                    for (auto && i : at_c<1>(_attr(ctx))) {
                        f = [f, i, &ctx] (ast & e) {
                            return make_shared<let>(make_tuple(make_shared<identifier>(id::gentmp(x3::get<mcc_context_tag>(ctx), type::get_unit()), type::get_unit()),
                                                               f(e),
                                                               std::move(i)));
                        };
                    }
                    _val(ctx) = f(at_c<0>(_attr(ctx)));
                } else {
                    _val(ctx) = std::move(at_c<0>(_attr(ctx)));
                }
            })];
    auto const prec_if_def = gram_branch | prec_put;
    auto const prec_put_def = gram_put | prec_comma;
    auto const gram_put_def = (prec_dot >> lit('.') >> confix('(', ')')[prec_let] >> lit("<-") >> prec_if) [([] (auto && ctx) {
                _val(ctx) = make_shared<put>(make_tuple(std::move(at_c<0>(_attr(ctx))),
                                                        std::move(at_c<1>(_attr(ctx))),
                                                        std::move(at_c<2>(_attr(ctx)))));
            })];
    auto const prec_comma_def = (prec_comp >> !lit(",")) [([] (auto && ctx) {
                _val(ctx) = std::move(_attr(ctx));
            })] | (prec_comp % ',') [([] (auto && ctx) {
                _val(ctx) = make_shared<tuple>(std::move(_attr(ctx)));
            })];
    template <typename Op, typename T>
        std::function<ast (ast)> binary_recursion(T & ctx) {
        auto && f = at_c<1>(_attr(ctx));
        auto && rhs = at_c<0>(_attr(ctx));
        return ([f, rhs] (ast const & e) { // copy
                return f(make_shared<binary<Op>>(make_tuple(e, rhs)));
            });
    };
    auto const prec_comp_def = (prec_additive >> comp_loop) [([] (auto && ctx) {
                _val(ctx) = at_c<1>(_attr(ctx))(at_c<0>(_attr(ctx)));
            })];
    auto const comp_loop_def = (lit("<=") >> prec_additive >> comp_loop) [([] (auto && ctx) {
                _val(ctx) = binary_recursion<op_le>(ctx);
            })] | (lit(">=") >> prec_additive >> comp_loop) [([] (auto && ctx) {
                auto && f = at_c<1>(_attr(ctx));
                auto && rhs = at_c<0>(_attr(ctx));
                _val(ctx) = ([f, rhs] (ast const & e) { // copy
                        return f(make_shared<binary<op_le>>(make_tuple(rhs, e)));
                    });
            })] | (lit("<>") >> prec_additive >> comp_loop) [([] (auto && ctx) {
                auto && f = at_c<1>(_attr(ctx));
                auto && rhs = at_c<0>(_attr(ctx));
                _val(ctx) = ([f, rhs] (ast const & e) { // copy
                        return f(make_shared<unary<op_not>>(make_shared<binary<op_eq>>(make_tuple(e, rhs))));
                    });
            })] | (lit("<") >> prec_additive >> comp_loop) [([] (auto && ctx) {
                auto && f = at_c<1>(_attr(ctx));
                auto && rhs = at_c<0>(_attr(ctx));
                _val(ctx) = ([f, rhs] (ast const & e) { // copy
                        return f(make_shared<unary<op_not>>(make_shared<binary<op_le>>(make_tuple(rhs, e))));
                    });
            })] | (lit(">") >> prec_additive >> comp_loop) [([] (auto && ctx) {
                auto && f = at_c<1>(_attr(ctx));
                auto && rhs = at_c<0>(_attr(ctx));
                _val(ctx) = ([f, rhs] (ast const & e) { // copy
                        return f(make_shared<unary<op_not>>(make_shared<binary<op_le>>(make_tuple(e, rhs))));
                    });
            })] | (lit("=") >> prec_additive >> comp_loop) [([] (auto && ctx) {
                _val(ctx) = binary_recursion<op_eq>(ctx);
            })] | eps [([] (auto && ctx) {
                _val(ctx) = ([] (ast const & e) { return e; });
            })];
    auto const prec_additive_def = (prec_multiplicative >> additive_loop) [([] (auto && ctx) {
                _val(ctx) = at_c<1>(_attr(ctx))(at_c<0>(_attr(ctx)));
            })];
    auto const additive_loop_def = (lit(op_fadd::c_str) >> prec_multiplicative >> additive_loop) [([] (auto && ctx) {
                _val(ctx) = binary_recursion<op_fadd>(ctx);
            })] | (lit(op_fsub::c_str) >> prec_multiplicative >> additive_loop) [([](auto && ctx) {
                _val(ctx) = binary_recursion<op_fsub>(ctx);
            })] | (lit(op_add::c_str) >> prec_multiplicative >> additive_loop) [([](auto && ctx) {
                _val(ctx) = binary_recursion<op_add>(ctx);
            })] | (lit(op_sub::c_str) >> prec_multiplicative >> additive_loop) [([](auto && ctx) {
                _val(ctx) = binary_recursion<op_sub>(ctx);
            })] | eps [([] (auto && ctx) {
                _val(ctx) = ([] (ast const & e) { return e; });
            })];
    auto const prec_multiplicative_def = (prec_unary_minus >> multiplicative_loop) [([] (auto && ctx) {
                _val(ctx) = at_c<1>(_attr(ctx))(at_c<0>(_attr(ctx)));
            })];
    auto const multiplicative_loop_def = (lit(op_fmul::c_str) >> prec_unary_minus >> multiplicative_loop) [([] (auto && ctx) {
                _val(ctx) = binary_recursion<op_fmul>(ctx);
            })] | (lit(op_fdiv::c_str) >> prec_unary_minus >> multiplicative_loop) [([] (auto && ctx) {
                _val(ctx) = binary_recursion<op_fdiv>(ctx);
            })] | (lit(op_mul::c_str) >> prec_unary_minus >> multiplicative_loop) [([] (auto && ctx) {
                _val(ctx) = binary_recursion<op_mul>(ctx);
            })] | (lit(op_div::c_str) >> prec_unary_minus >> multiplicative_loop) [([] (auto && ctx) {
                _val(ctx) = binary_recursion<op_div>(ctx);
            })] | eps [([] (auto && ctx) {
                _val(ctx) = ([] (ast const & e) { return e; });
            })];
    auto const prec_unary_minus_def = (lit("-.") >> prec_unary_minus) [([] (auto && ctx) {
                _val(ctx) = make_shared<unary<op_fneg>>(std::move(_attr(ctx)));
            })] | (lit('-') >> prec_unary_minus)[([] (auto && ctx) {
                if (sptr<floating_point> * f = std::get_if<sptr<floating_point>>(&_attr(ctx))) {
                    _val(ctx) = make_shared<floating_point>(-((*f)->value));
                } else {
                    _val(ctx) = make_shared<unary<op_neg>>(std::move(_attr(ctx)));
                }
            })] | prec_app [([] (auto && ctx) {
                _val(ctx) = std::move(_attr(ctx));
            })];
    auto const prec_app_def = (lit("not") >> prec_app) [([] (auto && ctx) {
                _val(ctx) = make_shared<unary<op_not>>(std::move(_attr(ctx)));
            })] | (lit("Array.create") >> prec_dot >> prec_dot) [([] (auto && ctx) {
                _val(ctx) = make_shared<array>(make_tuple(std::move(at_c<0>(_attr(ctx))), std::move(at_c<1>(_attr(ctx)))));
            })] | (lit("create_array") >> prec_dot >> prec_dot) [([] (auto && ctx) {
                _val(ctx) = make_shared<array>(make_tuple(std::move(at_c<0>(_attr(ctx))), std::move(at_c<1>(_attr(ctx)))));
            })] | (prec_dot >> (+prec_dot)) [([] (auto && ctx) {
                _val(ctx) = make_shared<app>(make_tuple(std::move(at_c<0>(_attr(ctx))), std::move(at_c<1>(_attr(ctx)))));
            })] | prec_dot [([] (auto && ctx) {
                _val(ctx) = std::move(_attr(ctx));
            })];

    BOOST_SPIRIT_DEFINE(simple_exp,
                        comment,
                        toplevel,
                        gram_external,
                        gram_type,
                        gram_func_type,
                        gram_tuple_type,
                        gram_constr_type,
                        gram_simple_type,
                        gram_integer,
                        gram_floating_point,
                        gram_boolean,
                        gram_unit,
                        gram_identifier,
                        exp,
                        gram_branch,
                        prec_let,
                        gram_let,
                        gram_let_binding,
                        prec_semicolon,
                        prec_if,
                        then_else,
                        gram_get,
                        prec_put,
                        gram_put,
                        prec_dot,
                        prec_comma,
                        prec_comp,
                        comp_loop,
                        gram_let_tuple,
                        prec_additive,
                        additive_loop,
                        prec_multiplicative,
                        multiplicative_loop,
                        prec_unary_minus,
                        prec_app,
                        gram_let_rec
                        );

}

namespace mcc {
    namespace parser {

        bool parse(context & ctx,
                   std::string::const_iterator & bgn,
                   std::string::const_iterator & end,
                   std::vector<toplevel_t> & asts) {

            using boost::spirit::x3::with;
            auto const prs = with<mcc_context_tag>(std::ref(ctx))[toplevel];

            bool r = phrase_parse(bgn, end, prs, (comment | space), asts);
            if (bgn != end) {
                int i = 0;
                for (i = 0; i < 20 && bgn != end; i++, bgn++) {
                    std::cerr << *bgn;
                }
                std::cerr << '\n';
            }
            return (r && bgn == end);
        };

        module f(context & ctx, const std::string & filepath) {
            module ret;
            std::ifstream in(filepath, std::ios_base::in);
            ret.module_name = filepath;
            if (!in) {
                std::cerr << "Error: Could not open input file: " << filepath << std::endl;
            } else {
                std::string storage;
                in.unsetf(std::ios::skipws);
                std::copy(std::istream_iterator<char>(in),
                          std::istream_iterator<char>(),
                          std::back_inserter(storage));

                std::string::const_iterator bgn = storage.begin();
                std::string::const_iterator end = storage.end();

                bool r = mcc::parser::parse(ctx, bgn, end, ret.value);
                if (!r) {
                    std::cerr << "Error: Could not parse!" << std::endl;
                }
            }
            return ret;
        }

    }
}
