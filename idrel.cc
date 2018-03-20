#include <string>
#include <vector>
#include <unordered_map>
#include "idrel.h"

using namespace mcc;

namespace {
    using env_t = map_stack<std::string, std::shared_ptr<parser::identifier>>;

    struct pass {
        using result_type = parser::ast;
        env_t & env;
        env_t & extenv;
        pass(env_t & e, env_t & ex) : env(e), extenv(ex) { }

        template <typename T>
        result_type operator() (T & ast) {
            return ast;
        }

        result_type operator() (parser::ast & ast) {
            return std::visit(pass(env, extenv), ast);
        }

        result_type operator() (std::shared_ptr<parser::identifier> & ast) {
            auto t = env.find(std::get<0>(ast->value));
            if (t != env.end()) {
                return t->second;
            } else {
                auto t = extenv.find(std::get<0>(ast->value));
                if (t != extenv.end()) {
                    return t->second;
                }
                ast->is_external = true;
                extenv.insert(std::make_pair(std::get<0>(ast->value), ast));
                return ast;
            }
        }

        template <typename Op>
        result_type operator() (std::shared_ptr<parser::unary<Op>> & ast) {
            ast->value = pass(env, extenv)(ast->value);
            return ast;
        }

        template <typename Op>
        result_type operator() (std::shared_ptr<parser::binary<Op>> & ast) {
            for_each_tuple(ast->value, [this] (auto && a) { a = pass(env, extenv)(a); });
            return ast;
        }

        result_type operator() (std::shared_ptr<parser::branch> & ast) {
            for_each_tuple(ast->value, [this] (auto && a) { a = pass(env, extenv)(a); });
            return ast;
        }

        result_type operator() (std::shared_ptr<parser::let> & ast) {
            std::get<1>(ast->value) = pass(env, extenv)(std::get<1>(ast->value));
            env.push();
            auto && ident = std::get<0>(ast->value);
            env.insert(make_pair(std::get<0>(ident->value), ident));
            std::get<2>(ast->value) = pass(env, extenv)(std::get<2>(ast->value));
            env.pop();
            return ast;
        }

        result_type operator() (std::shared_ptr<parser::let_tuple> & ast) {
            std::get<1>(ast->value) = pass(env, extenv)(std::get<1>(ast->value));
            env.push();
            for (auto && ident : std::get<0>(ast->value)) {
                env.insert(make_pair(std::get<0>(ident->value), ident));
            }
            std::get<2>(ast->value) = pass(env, extenv)(std::get<2>(ast->value));
            env.pop();
            return ast;
        }

        result_type operator() (std::shared_ptr<parser::let_rec> & ast) {
            env.push();
            auto && fun_name = std::get<0>(ast->value); // fun_name
            env.insert(make_pair(std::get<0>(fun_name->value), fun_name));
            env.push();
            for (auto && fun_arg : std::get<1>(ast->value)) {
                env.insert(make_pair(std::get<0>(fun_arg->value), fun_arg));
            }
            std::get<2>(ast->value) = pass(env, extenv)(std::get<2>(ast->value));
            env.pop();
            std::get<3>(ast->value) = pass(env, extenv)(std::get<3>(ast->value));
            env.pop();
            return ast;
        }

        result_type operator() (std::shared_ptr<parser::app> & ast) {
            std::get<0>(ast->value) = pass(env, extenv)(std::get<0>(ast->value));
            for (auto && app_arg : std::get<1>(ast->value)) {
                app_arg = pass(env, extenv)(app_arg);
            }
            return ast;
        }

        result_type operator() (std::shared_ptr<parser::tuple> & ast) {
            for (auto && elem : ast->value) {
                elem = pass(env, extenv)(elem);
            }
            return ast;
        }

        result_type operator() (std::shared_ptr<parser::array> & ast) {
            for_each_tuple(ast->value, [this] (auto && a) { a = pass(env, extenv)(a); });
            return ast;
        }

        result_type operator() (std::shared_ptr<parser::get> & ast) {
            for_each_tuple(ast->value, [this] (auto && a) { a = pass(env, extenv)(a); });
            return ast;
        }

        result_type operator() (std::shared_ptr<parser::put> & ast) {
            for_each_tuple(ast->value, [this] (auto && a) { a = pass(env, extenv)(a); });
            return ast;
        }

    };

    struct global_pass {
        using result_type = parser::toplevel_t;
        env_t & env;
        env_t & extenv;
        global_pass(env_t & e, env_t & ex) : env(e), extenv(ex) { }

        result_type operator() (std::shared_ptr<parser::external> & t) {
            auto && ident = std::get<0>(t->value);
            env.insert(std::make_pair(std::get<0>(ident->value), ident));
            return t;
        }

        result_type operator() (std::shared_ptr<parser::global> & t) {
            std::get<1>(t->value) = pass(env, extenv)(std::get<1>(t->value));
            auto && ident = std::get<0>(t->value);
            env.insert(make_pair(std::get<0>(ident->value), ident));
            return t;
        }

        result_type operator() (std::shared_ptr<parser::global_tuple> & t) {
            std::get<1>(t->value) = pass(env, extenv)(std::get<1>(t->value));
            for (auto && ident : std::get<0>(t->value)) {
                env.insert(make_pair(std::get<0>(ident->value), ident));
            }
            return t;
        }

        result_type operator() (std::shared_ptr<parser::global_rec> & t) {
            auto && fun_name = std::get<0>(t->value); // fun_name
            env.insert(make_pair(std::get<0>(fun_name->value), fun_name));
            env.push();
            for (auto && fun_arg : std::get<1>(t->value)) {
                env.insert(make_pair(std::get<0>(fun_arg->value), fun_arg));
            }
            std::get<2>(t->value) = pass(env, extenv)(std::get<2>(t->value));
            env.pop();
            return t;
        }

        result_type operator() (parser::ast & t) {
            return std::visit(pass(env, extenv), t);
        }

    };

}

parser::module idrel::f(parser::module && mod) {
    env_t env;
    env_t extenv;
    std::vector<mcc::parser::toplevel_t> ret;
    for (auto && t : mod.value) {
        t = std::visit(global_pass(env, extenv), t);
    }
    for (auto i = extenv.begin(); i != extenv.end(); i++) {
        auto external_decl = std::make_shared<parser::external>(std::make_tuple(i->second, "min_caml_" + i->first));
        ret.push_back(external_decl);
    }
    for (auto && t : mod.value) {
        ret.push_back(t);
    }
    mod.value = std::move(ret);
    return std::move(mod);
}
