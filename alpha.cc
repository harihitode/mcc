#include "alpha.h"

using namespace mcc;
using namespace mcc::alpha;
using namespace mcc::knormal;

namespace {
    using std::make_tuple;
    using std::make_pair;
    using std::make_shared;
    using knormal::identifier;

    struct pass {
        using result_type = void;

        explicit pass() { }

        result_type operator() (ast & e) {
            std::visit(pass(), e);
        }

        template <typename T>
        result_type operator() (T & e) { }

        result_type operator() (sptr<branch> & e) {
            std::visit(pass(), std::get<1>(e->value));
            std::visit(pass(), std::get<2>(e->value));
        }

        result_type operator() (sptr<let> & e) {
            auto && ident = std::get<0>(e->value);
            auto && new_x = id::genid(std::get<0>(ident->value));
            std::get<0>(ident->value) = new_x;
            std::visit(pass(), std::get<1>(e->value));
            std::visit(pass(), std::get<2>(e->value));
        }

        result_type operator() (sptr<let_tuple> & e) {
            // env_ update to eval result
            std::for_each(std::get<0>(e->value).begin(),
                          std::get<0>(e->value).end(),
                          [&] (auto & x) {
                              std::get<0>(x->value) = id::genid(std::get<0>(x->value));
                          });
            std::visit(pass(), std::get<2>(e->value));
        }

        result_type operator() (sptr<let_rec> & e) {
            auto && name = std::get<0>(e->value);
            auto && new_name = id::genid(std::get<0>(name->value));
            std::get<0>(name->value) = new_name;
            // env update to eval function body
            std::for_each(std::get<1>(e->value).begin(),
                          std::get<1>(e->value).end(),
                          [&] (auto & arg) {
                              std::get<0>(arg->value) = id::genid(std::get<0>(arg->value));
                          });
            std::visit(pass(), std::get<2>(e->value));
            std::visit(pass(), std::get<3>(e->value));
        }

    };

    struct global_pass {
        using result_type = void;

        global_pass() { }

        result_type operator() (sptr<knormal::external> & e) {
        }

        result_type operator() (sptr<knormal::global> & e) {
            auto && ident = std::get<0>(e->value);
            auto && new_x = id::genid(std::get<0>(ident->value));
            std::visit(pass(), std::get<1>(e->value));
            std::get<0>(ident->value) = new_x;
        }

        result_type operator() (sptr<knormal::global_tuple> & e) {
            std::for_each(std::get<0>(e->value).begin(),
                          std::get<0>(e->value).end(),
                          [&] (auto & x) {
                              std::get<0>(x->value) = id::genid(std::get<0>(x->value));
                          });
            std::visit(pass(), std::get<1>(e->value));
        }

        result_type operator() (sptr<knormal::global_rec> & e) {
            auto && name = std::get<0>(e->value);
            auto && new_name = id::genid(std::get<0>(name->value));
            std::get<0>(name->value) = new_name;
            std::for_each(std::get<1>(e->value).begin(),
                          std::get<1>(e->value).end(),
                          [&] (auto & arg) {
                              std::get<0>(arg->value) = id::genid(std::get<0>(arg->value));
                          });
            std::for_each(std::get<2>(e->value).begin(),
                          std::get<2>(e->value).end(),
                          [&] (auto & arg) {
                              std::get<0>(arg->value) = id::genid(std::get<0>(arg->value));
                          });
            std::visit(pass(), std::get<3>(e->value));
        }

        template <typename T>
        result_type operator() (T & ast) {
            std::visit(pass(), ast);
        }

    };

}

mcc::knormal::module mcc::alpha::f(mcc::knormal::module && mod) {
    for (auto && t : mod.value) { std::visit(global_pass(), t); }
    return std::move(mod);
}
