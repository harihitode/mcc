#ifndef MCC_TYPING_H
#define MCC_TYPING_H

#include <iostream>
#include <boost/fusion/include/at_c.hpp>
#include <boost/fusion/include/transform.hpp>
#include <unordered_map>
#include "ast.h"

namespace mcc {
    namespace type {

        type_t deref_typ(type_t & t);
        type_t unwrap(type_t const & t);
        void deref_id_typ(std::shared_ptr<parser::identifier> & i);
        void deref_term(parser::toplevel_t & ast);

        bool occur(const std::shared_ptr<variable> & a, const type_t & b);
        bool unify(const type_t & a, const type_t & b);

        parser::module f(context & ctx, parser::module && mod);

    }
}

#endif
