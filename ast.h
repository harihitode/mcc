#ifndef MCC_AST_H
#define MCC_AST_H

#include <string>
#include <vector>
#include <tuple>
#include <variant>
#include <unordered_map>
#include <unordered_set>

namespace mcc {
    template <typename ...TS>
    class variant_t {
        using Tuple = std::tuple<TS...>;
        using seq = std::make_index_sequence<std::tuple_size<Tuple>::value>;
        template <size_t ...N>
        static auto variant_unique_ptr(std::index_sequence<N...>)
            -> std::variant<typename std::unique_ptr<std::tuple_element_t<N, Tuple>>...>;
        template <size_t ...N>
        static auto variant_shared_ptr(std::index_sequence<N...>)
            -> std::variant<typename std::shared_ptr<std::tuple_element_t<N, Tuple>>...>;
        template <size_t ...N>
        static auto variant_weak_ptr(std::index_sequence<N...>)
            -> std::variant<typename std::weak_ptr<std::tuple_element_t<N, Tuple>>...>;
    public:
        using unique_ptr = decltype(variant_unique_ptr(seq()));
        using shared_ptr = decltype(variant_shared_ptr(seq()));
        using weak_ptr = decltype(variant_weak_ptr(seq()));
    };

    template <typename T>
    using uptr = std::unique_ptr<T>;

    template <typename T>
    using sptr = std::shared_ptr<T>;

    template <typename Key, typename Value>
    struct map_stack {
    protected:
        std::vector<std::unordered_map<Key, Value>> map;
    public:
        map_stack() { }

        template <typename T>
        void insert(T && t) noexcept {
            if (map.size() == 0) {
                map.push_back(std::unordered_map<Key, Value>({ }));
            }
            map.back().insert(std::forward<T>(t));
        }
        void push() noexcept { map.push_back(std::unordered_map<Key, Value>({ })); }
        void pop() noexcept { map.pop_back(); }
        using current_stack_t = decltype(map.rbegin());
        using current_elem_t = typename std::unordered_map<Key, Value>::iterator;
        // non sufficient
        struct iterator {
            const map_stack & c;
            current_stack_t s;
            current_elem_t e;

            iterator(const map_stack<Key, Value> & container,
                     current_stack_t stack_idx,
                     current_elem_t elem_idx = current_elem_t()) : c(container), s(stack_idx), e(elem_idx) { }

            bool operator==(const iterator & rhs) noexcept {
                if (s == rhs.s && s == c.map.rend()) { // stack_end
                    return true;
                } else if (s == rhs.s && e == rhs.e) {
                    return true;
                } else {
                    return false;
                }
            }

            bool operator!=(const iterator & rhs) noexcept {
                return !((*this) == rhs);
            }

            std::pair<const Key, Value> & operator*() const noexcept {
                return *e;
            }

            std::pair<const Key, Value> * operator->() const noexcept {
                return &(*e);
            }

            iterator & operator++() noexcept {
                e++;
                if (e == s->end()) {
                    s++;
                    e = s->begin();
                }
                return (*this);
            }

            const iterator & operator++(int) noexcept {
                return ++(*this);
            }

        };

        iterator find(const Key & key) {
            for (auto r = map.rbegin(); r != map.rend(); r++) {
                auto m = r->find(key);
                if (m != r->end()) {
                    return iterator(*this, r, m);
                }
            }
            return this->end();
        }

        iterator begin() {
            if (map.size() == 0) {
                return iterator(*this, map.rbegin());
            } else {
                return iterator(*this, map.rbegin(), map.rbegin()->begin());
            }
        }

        iterator end() {
            if (map.size() == 0) {
                return iterator(*this, map.rend());
            } else {
                return iterator(*this, map.rend(), map.rend()->begin());
            }
        }
    };

    template <typename F>
    auto recursive(F f) {
        // auto de suiron &  forwarding transfer
        // origin: http://ornew.net/cxx-recusive-lambda
        return [f] (auto &&... a) { return f(f, std::forward<decltype(a)>(a)...); };
    }

    // workaround...
    template <typename T>
    struct tuple_size : std::integral_constant<std::size_t,
                                               std::tuple_size<typename std::remove_reference_t<T>>
                                               ::value> { };

    template <std::size_t index, std::size_t end, bool isEnd = (index == end)>
    struct for_each_tuple_impl;

    template <std::size_t index, std::size_t end>
    struct for_each_tuple_impl<index, end, false> {
        template<typename Tuple, typename F>
        static void Execute(Tuple && tuple, F f) {
            f(std::get<index>(std::forward<Tuple>(tuple)));
            for_each_tuple_impl<index + 1,end>::Execute(std::forward<Tuple>(tuple), f);
        }
    };

    template <std::size_t index, std::size_t end>
    struct for_each_tuple_impl<index, end, true> {
        template<typename Tuple, class F>
        static void Execute(Tuple && tuple, F f) { /* base */ }
    };

    template <typename Tuple, typename F>
    void for_each_tuple(Tuple && tuple, F f) {
        for_each_tuple_impl<0, tuple_size<Tuple>::value>::Execute(std::forward<Tuple>(tuple), f);
    }

    template <typename T>
    struct base {
        using type = T;
        T value;
        base(const T & v) : value(v) { }
        base(T && v) : value(std::forward<T>(v)) { }
    };

    template <>
    struct base<void> {
        base() { }
    };

    namespace type {

        struct unit : base<void> { unit() { } };
        struct integer : base<void> { integer() { } };
        struct floating_point : base<void> { floating_point() { } };
        struct boolean : base<void> { boolean() { } };
        struct closure : base<void> { closure() { } };
        struct function;
        struct closure;
        struct tuple;
        struct array;
        struct variable;

        using type_variant = variant_t<unit,
                                       integer,
                                       floating_point,
                                       boolean,
                                       function,
                                       closure,
                                       tuple,
                                       array,
                                       variable>;
        using type_t = type_variant::shared_ptr;

        type_t get_unit();
        type_t get_integer();
        type_t get_floating_point();
        type_t get_boolean();

        // return, arg, free-variable
        struct function : base<std::tuple<type_t, std::vector<type_t>>> {
            bool is_closure;
            explicit function(type && v, bool c = true) : base(std::move(v)), is_closure(c) { }
        };

        struct tuple : base<std::vector<type_t>> {
            explicit tuple(type && v) : base(std::move(v)) { }
        };

        struct array : base<type_t> {
            explicit array(type_t && v) : base(std::move(v)) { }
        };

        struct variable : base<std::shared_ptr<std::shared_ptr<type_t>>> {
            explicit variable(type && v = std::make_shared<std::shared_ptr<type_t>>(nullptr)) : base(std::move(v)) { }
        };

    }

    namespace value {

        struct unit : base<void> { unit() { } };

        struct integer : base<const int> {
            explicit integer(int v = 0) : base(v) { }
        };

        struct floating_point : base<const double> {
            explicit floating_point(double v = 0) : base(v) { }
        };

        struct boolean : base<const bool> {
            explicit boolean(bool v = true) : base(v) { }
        };

        std::shared_ptr<unit> get_const_unit();
        std::shared_ptr<integer> get_const_integer(int value);
        std::shared_ptr<floating_point> get_const_floating_point(double value);
        std::shared_ptr<boolean> get_const_boolean(bool value);
        std::shared_ptr<boolean> get_const_true();
        std::shared_ptr<boolean> get_const_false();

        struct identifier : base<std::tuple<std::string, type::type_t>> {
            bool is_external;
            bool is_global;
            explicit identifier(std::string v = "", bool e = false, bool g = false) : base(std::make_tuple(v, std::make_shared<mcc::type::variable>())), is_external(e), is_global(g) { }
            template <typename T>
            identifier(std::string v, T && t, bool e = false, bool g = false) : base(std::make_tuple(v, std::forward<T>(t))), is_external(e), is_global(g) { }
        };

        struct external : base<std::tuple<std::shared_ptr<identifier>, std::string>> {
            explicit external(type && v) : base(std::move(v)) { }
        };

    }

    namespace operand {
        // unary
        struct op_neg {
            static constexpr char c_str[] = "-";
            static type::type_t result_type() { return type::get_integer(); }
        };
        struct op_fneg {
            static constexpr char c_str[] = "-.";
            static type::type_t result_type() { return type::get_floating_point(); }
        };
        struct op_not {
            static constexpr char c_str[] = "~";
            static type::type_t result_type() { return type::get_boolean(); }
        };
        struct op_le {
            static constexpr char c_str[] = "<=";
            static type::type_t result_type() { return type::get_boolean(); }
        };
        struct op_eq {
            static constexpr char c_str[] = "=";
            static type::type_t result_type() { return type::get_boolean(); }
        };
        struct op_add {
            static constexpr char c_str[] = "+";
            static type::type_t result_type() { return type::get_integer(); }
        };
        struct op_sub {
            static constexpr char c_str[] = "-";
            static type::type_t result_type() { return type::get_integer(); }
        };
        struct op_mul {
            static constexpr char c_str[] = "*";
            static type::type_t result_type() { return type::get_integer(); }
        };
        struct op_div {
            static constexpr char c_str[] = "/";
            static type::type_t result_type() { return type::get_integer(); }
        };
        struct op_fadd {
            static constexpr char c_str[] = "+.";
            static type::type_t result_type() { return type::get_floating_point(); }
        };
        struct op_fsub {
            static constexpr char c_str[] = "-.";
            static type::type_t result_type() { return type::get_floating_point(); }
        };
        struct op_fmul {
            static constexpr char c_str[] = "*.";
            static type::type_t result_type() { return type::get_floating_point(); }
        };
        struct op_fdiv {
            static constexpr char c_str[] = "/.";
            static type::type_t result_type() { return type::get_floating_point(); }
        };
    }

    namespace id {
        std::string gentmp(const type::type_t & t);
        std::string genid(const std::string & s);
    }

    namespace env {
        using env_t = std::unordered_map<std::string, type::type_t>;
        using map_t = std::unordered_map<std::string, std::string>;
    }

    namespace parser {
        using value::unit;
        using value::integer;
        using value::floating_point;
        using value::boolean;
        using value::identifier;
        using value::external;
        using namespace operand;

        struct branch;
        struct let;
        struct let_tuple;
        struct let_rec;
        struct put;
        struct get;
        struct tuple;
        struct equal;
        struct less;
        template<typename Op> struct unary;
        template<typename Op> struct binary;
        struct array;
        struct app;

        using ast = variant_t<unit,
                              integer,
                              floating_point,
                              boolean,
                              identifier,
                              unary<op_neg>,
                              unary<op_fneg>,
                              unary<op_not>,
                              binary<op_add>,
                              binary<op_sub>,
                              binary<op_mul>,
                              binary<op_div>,
                              binary<op_fadd>,
                              binary<op_fsub>,
                              binary<op_fmul>,
                              binary<op_fdiv>,
                              binary<op_eq>,
                              binary<op_le>,
                              let,
                              let_rec,
                              let_tuple,
                              app,
                              tuple,
                              array,
                              get,
                              put,
                              branch>::shared_ptr;

        template <typename Op>
        struct unary : base<ast> {
            explicit unary(type && v) : base(std::forward<type>(v)) { }
        };

        template <typename Op>
        struct binary : base<std::tuple<ast, ast>> {
            explicit binary(type && v) : base(std::move(v)) { }
        };

        struct branch : base<std::tuple<ast, ast, ast>> {
            explicit branch(type && v) : base(std::move(v)) { }
        };

        struct let : base<std::tuple<sptr<identifier>, ast, ast>> {
            explicit let(type && v) : base(std::move(v)) { }
        };

        struct let_tuple : base<std::tuple<std::vector<sptr<identifier>>, ast, ast>> {
            explicit let_tuple(type && v) : base(std::move(v)) { }
        };

        struct let_rec : base<std::tuple<sptr<identifier>, std::vector<sptr<identifier>>, ast, ast>> {
            explicit let_rec(type && v) : base(std::move(v)) { }
        };

        struct put : base<std::tuple<ast, ast, ast>> {
            explicit put(type && v) : base(std::move(v)) { }
        };

        struct get : base<std::tuple<ast, ast>> {
            explicit get(type && v) : base(std::move(v)) { }
        };

        struct array : base<std::tuple<ast, ast>> {
            explicit array(type && v) : base(std::move(v)) { }
        };

        struct tuple : base<std::vector<ast>> {
            explicit tuple(type && v) : base(std::move(v)) { }
        };

        struct app : base<std::tuple<ast, std::vector<ast>>> {
            explicit app(type && v) : base(std::move(v)) { }
        };

        struct global : base<std::tuple<sptr<identifier>, ast>> {
            explicit global(type && v) : base(std::move(v)) { }
        };

        struct global_tuple : base<std::tuple<std::vector<sptr<identifier>>, ast>> {
            explicit global_tuple(type && v) : base(std::move(v)) { }
        };

        struct global_rec : base<std::tuple<sptr<identifier>, std::vector<sptr<identifier>>, ast>> {
            explicit global_rec(type && v) : base(std::move(v)) { }
        };

        using toplevel_t = std::variant<ast,
                                        std::shared_ptr<external>,
                                        std::shared_ptr<global>,
                                        std::shared_ptr<global_rec>,
                                        std::shared_ptr<global_tuple>>;

        struct module {
            std::string module_name;
            type::type_t module_type;
            std::vector<toplevel_t> value;
        };

    }

    namespace knormal {

        using value::unit;
        using value::integer;
        using value::floating_point;
        using value::boolean;
        using value::identifier;
        using value::external;
        using namespace operand;

        // neg, fneg
        template <typename Op>
        struct unary : base<sptr<identifier>> {
            // op, arg
            explicit unary(const type & v) : base(v) { }
            explicit unary(type && v) : base(v) { }
        };
        // add, sub, (mul, div), fadd, fsub, fmul, fdiv, compair
        template <typename Op>
        struct binary : base<std::tuple<sptr<identifier>, sptr<identifier>>> {
            // op, arg1, arg2
            explicit binary(const type & v) : base(v) { }
            explicit binary(type && v) : base(v) { }
        };
        struct array : base<std::tuple<sptr<identifier>, sptr<identifier>>> {
            explicit array(const type & v) : base(v) { }
            explicit array(type && v) : base(v) { }
        };
        struct tuple : base<std::vector<sptr<identifier>>> {
            explicit tuple(const type & v) : base(v) { }
            explicit tuple(type && v) : base(std::move(v)) { }
        };
        struct get : base<std::tuple<sptr<identifier>, sptr<identifier>>> {
            explicit get(const type & v) : base(v) { }
            explicit get(type && v) : base(v) { }
        };
        struct put : base<std::tuple<sptr<identifier>, sptr<identifier>, sptr<identifier>>> {
            explicit put(const type & v) : base(v) { }
            explicit put(type && v) : base(v) { }
        };
        struct app : base<std::tuple<sptr<identifier>, std::vector<sptr<identifier>>>> {
            explicit app(type && v) : base(std::move(v)) { }
        };

        struct branch;
        struct let;
        struct let_rec;
        struct let_tuple;

        using ast = mcc::variant_t<unit,
                                   integer,
                                   floating_point,
                                   boolean,
                                   identifier,
                                   unary<op_neg>,
                                   unary<op_fneg>,
                                   unary<op_not>,
                                   binary<op_add>,
                                   binary<op_sub>,
                                   binary<op_mul>,
                                   binary<op_div>,
                                   binary<op_fadd>,
                                   binary<op_fsub>,
                                   binary<op_fmul>,
                                   binary<op_fdiv>,
                                   binary<op_eq>,
                                   binary<op_le>,
                                   let,
                                   let_rec,
                                   let_tuple,
                                   app,
                                   tuple,
                                   array,
                                   get,
                                   put,
                                   branch>::shared_ptr;

        // branching & recursion
        struct branch : base<std::tuple<sptr<identifier>, ast, ast>> {
            explicit branch(type && v) : base(std::move(v)) { }
        };
        struct let : base<std::tuple<sptr<identifier>, ast, ast>> {
            explicit let(type && v) : base(std::move(v)) { }
        };
        struct let_rec : base<std::tuple<sptr<identifier>, std::vector<sptr<identifier>>, ast, ast>> {
            explicit let_rec(type && v) : base(std::move(v)) { }
        };
        struct let_tuple : base<std::tuple<std::vector<sptr<identifier>>, sptr<identifier>, ast>> {
            explicit let_tuple(type && v) : base(std::move(v)) { }
        };
        struct global : base<std::tuple<sptr<identifier>, ast>> {
            explicit global(type && v) : base(std::move(v)) { }
        };
        struct global_tuple : base<std::tuple<std::vector<sptr<identifier>>, ast>> {
            explicit global_tuple(type && v) : base(std::move(v)) { }
        };
        struct global_rec : base<std::tuple<sptr<identifier>, std::vector<sptr<identifier>>, std::vector<sptr<identifier>>, ast>> {
            explicit global_rec(type && v) : base(std::move(v)) { }
        };

        using toplevel_t = std::variant<ast,
                                        std::shared_ptr<external>,
                                        std::shared_ptr<global>,
                                        std::shared_ptr<global_rec>,
                                        std::shared_ptr<global_tuple>>;

        struct module {
            std::string module_name;
            type::type_t module_type;
            std::vector<toplevel_t> value;
        };

    }

    namespace closure {

        using namespace operand;

        using value::unit;
        using value::integer;
        using value::floating_point;
        using value::boolean;
        using value::identifier;
        using value::external;

        using knormal::unary;
        using knormal::binary;
        using knormal::array;
        using knormal::tuple;
        using knormal::get;
        using knormal::put;
        using knormal::app;

        using knormal::branch;
        using knormal::let;
        using knormal::let_rec;
        using knormal::let_tuple;

        using knormal::ast;

        using knormal::global;
        using knormal::global_tuple;
        using knormal::global_rec;

        using knormal::toplevel_t;
        using knormal::module;

    }

}

#endif
