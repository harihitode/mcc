#ifndef MCC_PRINTER_H
#define MCC_PRINTER_H

#include "ast.h"
#include "typing.h"

namespace mcc {
    namespace printer {

        struct printer {
            using result_type = void;

            printer() {}

            result_type operator() (const std::string & e) const {
                std::cerr << "(" << e << ")";
            }

            template <typename T>
            result_type operator() (const std::vector<T> & e) {
                std::for_each(e.begin(), e.end(), printer());
            }

            template <typename ...TS>
            result_type operator() (const std::tuple<TS...> & e) {
                for_each_tuple(e, printer());
            }


            // type
            result_type operator() (const type::type_t & t) const {
                std::visit(printer(), t);
            }

            result_type operator() (const sptr<type::unit> & t) const {
                std::cerr << "<unit>";
            }

            result_type operator() (const sptr<type::integer> & t) const {
                std::cerr << "<int>";
            }

            result_type operator() (const sptr<type::floating_point> & t) const {
                std::cerr << "<float>";
            }

            result_type operator() (const sptr<type::boolean> & t) const {
                std::cerr << "<boolean>";
            }

            result_type operator() (const sptr<type::function> & t) const {
                if (t->is_closure) {
                    std::cerr << "<closure[ret]";
                } else {
                    std::cerr << "<function[ret]";
                }
                printer()(std::get<0>(t->value));
                std::cerr << "[arg]";
                std::for_each(std::get<1>(t->value).begin(), std::get<1>(t->value).end(), printer());
                std::cerr << ">";
            }

            result_type operator() (const sptr<type::tuple> & t) const {
                std::cerr << "<tuple";
                std::for_each(t->value.begin(), t->value.end(), printer());
                std::cerr << ">";
            }

            result_type operator() (const sptr<type::array> & t) const {
                std::cerr << "<array";
                printer()(t->value);
                std::cerr << ">";
            }

            result_type operator() (const sptr<type::variable> & t) const {
                std::cerr << "<variable";
                if (*t->value != nullptr) {
                    printer()(*(*t->value));
                } else {
                    std::cerr << "<none>";
                }
                std::cerr << ">";
            }

            // value
            result_type operator() (const sptr<value::unit> & ast) const {
                std::cerr << "[unit]";
            }

            result_type operator() (const sptr<value::integer> & ast) const {
                std::cerr << "[int " << ast->value << "]";
            }

            result_type operator() (const sptr<value::floating_point> & ast) const {
                std::cerr << "[float " << ast->value << "]";
            }

            result_type operator() (const sptr<value::boolean> & ast) const {
                std::cerr << "[bool " << ast->value << "]";
            }

            // parser

            result_type operator() (const parser::toplevel_t & ast) const {
                std::visit(printer(), ast);
            }

            result_type operator() (const parser::ast & ast) const {
                std::visit(printer(), ast);
            }

            result_type operator() (const sptr<parser::external> & ast) const {
                std::cerr << "[external ";
                for_each_tuple(ast->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<parser::global> & ast) const {
                std::cerr << "[let(global) ";
                for_each_tuple(ast->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<parser::global_tuple> & ast) const {
                std::cerr << "[let_tuple(global) ";
                for_each_tuple(ast->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<parser::global_rec> & ast) const {
                std::cerr << "[let_rec(global) ";
                for_each_tuple(ast->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<parser::identifier> & ast) const {
                std::cerr << "[id ";
                std::cerr << std::get<0>(ast->value);
                if (ast->is_external) std::cerr << "#e";
                if (ast->is_global) std::cerr << "#g";
                printer()(std::get<1>(ast->value));
                std::cerr << "]";
            }

            template <typename Op>
            result_type operator() (const sptr<parser::unary<Op>> & ast) const {
                std::cerr << "[" << Op::c_str;
                std::visit(printer(), ast->value);
                std::cerr << "]";
            }

            template <typename Op>
            result_type operator() (const sptr<parser::binary<Op>> & ast) const {
                std::cerr << "[" << Op::c_str;
                for_each_tuple(ast->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<parser::branch> & ast) const {
                std::cerr << "[if ";
                for_each_tuple(ast->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<parser::let> & ast) const {
                std::cerr << "[let ";
                for_each_tuple(ast->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<parser::let_tuple> & ast) const {
                std::cerr << "[let_tuple ";
                for_each_tuple(ast->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<parser::let_rec> & ast) const {
                std::cerr << "[let_rec ";
                for_each_tuple(ast->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<parser::get> & ast) const {
                std::cerr << "[get ";
                for_each_tuple(ast->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<parser::put> & ast) const {
                std::cerr << "[put ";
                for_each_tuple(ast->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<parser::tuple> & ast) const {
                std::cerr << "[tuple ";
                printer()(ast->value);
                std::cerr << "]";
            }

            result_type operator() (const sptr<parser::array> & ast) const {
                std::cerr << "[array ";
                for_each_tuple(ast->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<parser::app> & ast) const {
                std::cerr << "[app ";
                for_each_tuple(ast->value, printer());
                std::cerr << "]";

            }

            // knormal
            result_type operator() (const knormal::toplevel_t & ast) const {
                std::visit(printer(), ast);
            }

            result_type operator() (const knormal::ast & e) const {
                std::visit(printer(), e);
            }

            template <typename Op>
            result_type operator() (const sptr<knormal::unary<Op>> & ast) const {
                std::cerr << "[" << Op::c_str;
                std::cerr << ast->value;
                std::cerr << "]";
            }

            template <typename Op>
            result_type operator() (const sptr<knormal::binary<Op>> & ast) const {
                std::cerr << "[" << Op::c_str;
                for_each_tuple(ast->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<knormal::branch> & e) const {
                std::cerr << "[branch ";
                for_each_tuple(e->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<knormal::let> & e) const {
                std::cerr << "[let ";
                for_each_tuple(e->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<knormal::let_rec> & e) const {
                std::cerr << "[let_rec ";
                for_each_tuple(e->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<knormal::let_tuple> & e) const {
                std::cerr << "[let_tuple ";
                for_each_tuple(e->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<knormal::global> & ast) const {
                std::cerr << "[let(global) ";
                for_each_tuple(ast->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<knormal::global_tuple> & ast) const {
                std::cerr << "[let_tuple(global) ";
                for_each_tuple(ast->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<knormal::global_rec> & ast) const {
                std::cerr << "[let_rec(global) ";
                for_each_tuple(ast->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<knormal::app> & e) const {
                std::cerr << "[app ";
                for_each_tuple(e->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<knormal::tuple> & e) const {
                std::cerr << "[tuple ";
                printer()(e->value);
                std::cerr << "]";
            }

            result_type operator() (const sptr<knormal::array> & e) const {
                std::cerr << "[array ";
                printer()(e->value);
                std::cerr << "]";
            }

            result_type operator() (const sptr<knormal::get> & e) const {
                std::cerr << "[get ";
                for_each_tuple(e->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<knormal::put> & e) const {
                std::cerr << "[put ";
                for_each_tuple(e->value, printer());
                std::cerr << "]";
            }

            // closure
            // knormal
            result_type operator() (const closure::toplevel_t & ast) const {
                std::visit(printer(), ast);
            }

            result_type operator() (const closure::ast & e) const {
                std::visit(printer(), e);
            }

            result_type operator() (const closure::closure_t & e) const {
                std::cerr << "[cls ";
                for_each_tuple(e, printer());
                std::cerr << ']';
            }

            result_type operator() (const sptr<closure::make_cls> & e) const {
                std::cerr << "[make_cls ";
                for_each_tuple(e->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<closure::branch> & e) const {
                std::cerr << "[branch ";
                for_each_tuple(e->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<closure::let_tuple> & e) const {
                std::cerr << "[let_tuple ";
                for_each_tuple(e->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<closure::let> & e) const {
                std::cerr << "[let ";
                for_each_tuple(e->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<closure::global> & ast) const {
                std::cerr << "[let(global) ";
                for_each_tuple(ast->value, printer());
                std::cerr << "]";
            }

            result_type operator() (const sptr<closure::global_tuple> & ast) const {
                std::cerr << "[let_tuple(global) ";
                for_each_tuple(ast->value, printer());
                std::cerr << "]";
            }

        };

    }

}

#endif
