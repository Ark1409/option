#ifndef OPTION_OPTIONAL_HPP_
#define OPTION_OPTIONAL_HPP_ 1

#if __cplusplus >= 202002L
#define OPTION_CXX20_CONSTEXPR constexpr
#define OPTION_CXX20_USE_EXPLICIT(...) explicit(__VA_ARGS__)
#else
#define OPTION_CXX20_CONSTEXPR
#define OPTION_CXX20_USE_EXPLICIT(...) explicit
#endif

#if __cplusplus >= 201402L
#define OPTION_CXX14_CONSTEXPR constexpr
#else
#define OPTION_CXX14_CONSTEXPR
#endif

#include <cstring>
#include <climits>
#include <memory>
#include <type_traits>
#include <initializer_list>

#if __cplusplus >= 201703L
#include <optional>
#include <string_view>
#include <functional>
#include <optional>
#endif

namespace option {
    constexpr struct in_place_t { constexpr explicit in_place_t() noexcept = default; } in_place;
    constexpr struct nullopt_t { constexpr explicit nullopt_t() noexcept = default; } nullopt;

    template<typename T>
    struct optional_traits {};

    namespace detail {
        template<class T>
        using remove_cvref_t = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

        template<class T>
        using remove_cv_t = typename std::remove_cv<T>::type;

        template<class T>
        using remove_reference_t = typename std::remove_reference<T>::type;

        template<class...>
        struct disjunction : std::false_type {};

        template<class B1>
        struct disjunction<B1> : B1 {};

        template<class B1, class... Bn>
        struct disjunction<B1, Bn...>
            : std::conditional<bool(B1::value), B1, disjunction<Bn...>>::type {};

        template<class T, class W>
        struct converts_from_any_cvref :
            disjunction<std::is_constructible<T, W&>, std::is_convertible<W&, T>,
                               std::is_constructible<T, W>, std::is_convertible<W, T>,
                               std::is_constructible<T, const W&>, std::is_convertible<const W&, T>,
                               std::is_constructible<T, const W>, std::is_convertible<const W, T>> {};

        template<typename...>
        struct void_t;

        template<>
        struct void_t<> { typedef void type; };

        template<class A, class... Rest>
        struct void_t<A, Rest...> : void_t<Rest...> { };

        template<typename C, typename Object>
        struct is_derived_object : disjunction<std::is_same<C, remove_cvref_t<Object>>, std::is_base_of<C, remove_cvref_t<Object>>> {};

        template<class Fn, class... ArgTypes>
        struct is_nothrow_invocable;

        template<typename F, typename... Args, typename std::enable_if<!std::is_member_pointer<remove_cvref_t<F>>::value>::type* = nullptr>
        OPTION_CXX14_CONSTEXPR auto invoke(F&& f, Args&&... args) noexcept(noexcept(std::forward<F>(f)(std::forward<Args>(args)...))) -> decltype(std::forward<F>(f)(std::forward<Args>(args)...)) {
            return std::forward<F>(f)(std::forward<Args>(args)...);
        }

        template<typename C, typename Pointed, typename Object, typename... Args,
            typename std::enable_if<std::is_function<Pointed>::value>::type* = nullptr,
            typename std::enable_if<is_derived_object<C, Object>::value>::type* = nullptr
                >
        OPTION_CXX14_CONSTEXPR auto invoke(Pointed C::* member, Object&& object, Args&&... args) noexcept(noexcept((std::forward<Object>(object) .* member) (std::forward<Args>(args)...))) -> decltype((std::forward<Object>(object) .* member) (std::forward<Args>(args)...)) {
            return (std::forward<Object>(object) .* member) (std::forward<Args>(args)...);
        }

        template<typename C, typename Pointed, typename Object, typename... Args,
            typename std::enable_if<std::is_function<Pointed>::value>::type* = nullptr
                >
        OPTION_CXX14_CONSTEXPR auto invoke(Pointed C::* member, std::reference_wrapper<Object> object, Args&&... args) noexcept(noexcept((object.get() .* member)(std::forward<Args>(args)...))) -> decltype((object.get() .* member)(std::forward<Args>(args)...)) {
            return (object.get() .* member)(std::forward<Args>(args)...);
        }

        template<typename C, typename Pointed, typename Object, typename... Args,
            typename std::enable_if<std::is_function<Pointed>::value>::type* = nullptr,
            typename std::enable_if<!is_derived_object<C, Object>::value>::type* = nullptr
                >
        OPTION_CXX14_CONSTEXPR auto invoke(Pointed C::* member, Object&& object, Args&&... args) noexcept(noexcept(((*std::forward<Object>(object)) .* member)(std::forward<Args>(args)...))) -> decltype(((*std::forward<Object>(object)) .* member)(std::forward<Args>(args)...)) {
            return ((*std::forward<Object>(object)) .* member)(std::forward<Args>(args)...);
        }

        template<typename C, typename Pointed, typename Object,
            typename std::enable_if<std::is_object<Pointed>::value>::type* = nullptr,
            typename std::enable_if<is_derived_object<C, Object>::value>::type* = nullptr
                >
        OPTION_CXX14_CONSTEXPR auto invoke(Pointed C::* member, Object&& object) noexcept(noexcept(std::forward<Object>(object) .* member)) -> decltype(std::forward<Object>(object) .* member) {
            return std::forward<Object>(object) .* member;
        }

        template<typename C, typename Pointed, typename Object,
            typename std::enable_if<std::is_object<Pointed>::value>::type* = nullptr
                >
        OPTION_CXX14_CONSTEXPR auto invoke(Pointed C::* member, std::reference_wrapper<Object> object) noexcept(noexcept(object.get() .* member)) -> decltype(std::forward<Object>(object) .* member) {
            return object.get() .* member;
        }

        template<typename C, typename Pointed, typename Object,
            typename std::enable_if<std::is_object<Pointed>::value>::type* = nullptr,
            typename std::enable_if<!is_derived_object<C, Object>::value>::type* = nullptr
                >
        OPTION_CXX14_CONSTEXPR auto invoke(Pointed C::* member, Object&& object) noexcept(noexcept((*std::forward<Object>(object)) .* member)) -> decltype((*std::forward<Object>(object)) .* member) {
            return (*std::forward<Object>(object)) .* member;
        }

        template<class, class F, class... Args>
        struct invoke_result_impl {
        };

        template<class F, class... Args>
        struct invoke_result_impl<typename void_t<decltype(invoke(std::declval<F>(), std::declval<Args>()...))>::type, F, Args...> {
            using type = decltype(invoke(std::declval<F>(), std::declval<Args>()...));
        };

        template<class F, class... Args>
        using invoke_result = invoke_result_impl<void, F, Args...>;

        template<class F, class... Args>
        using invoke_result_t = typename invoke_result<F, Args...>::type;

        template<class Fn, class... ArgTypes>
        struct is_nothrow_invocable : std::integral_constant<bool, noexcept(invoke(std::declval<Fn>(), std::declval<ArgTypes>()...))> {};

        template<typename U>
        struct specialized_optional;


        template<typename T, typename Sub, class = void>
        struct optional_base_helper_cc {
            constexpr optional_base_helper_cc() noexcept = default;
            constexpr optional_base_helper_cc(const optional_base_helper_cc&) noexcept = delete;
            constexpr optional_base_helper_cc(optional_base_helper_cc&&) noexcept = default;
        };

        template<typename T, typename Sub>
        struct optional_base_helper_cc<T, Sub, typename std::enable_if<std::is_copy_constructible<T>::value && !std::is_trivially_copy_constructible<T>::value>::type> {
            constexpr optional_base_helper_cc() noexcept = default;
            OPTION_CXX14_CONSTEXPR optional_base_helper_cc(const optional_base_helper_cc& other) noexcept(std::is_nothrow_copy_constructible<T>::value) {
                if (static_cast<const Sub&>(other).has_value())
                    static_cast<Sub*>(this)->emplace(static_cast<const Sub&>(other).value());
            }

            constexpr optional_base_helper_cc(optional_base_helper_cc&& other) noexcept = default;
        };

        template<typename T, typename Sub>
        struct optional_base_helper_cc<T, Sub, typename std::enable_if<std::is_trivially_copy_constructible<T>::value>::type> { };

        template<typename T, typename Sub, class = void>
        struct optional_base_helper_mc {
            constexpr optional_base_helper_mc() noexcept = default;
            constexpr optional_base_helper_mc(const optional_base_helper_mc&) noexcept = default;
            constexpr optional_base_helper_mc(optional_base_helper_mc&&) noexcept = delete;
        };

        template<typename T, typename Sub>
        struct optional_base_helper_mc<T, Sub, typename std::enable_if<std::is_move_constructible<T>::value && !std::is_trivially_move_constructible<T>::value>::type> {
            constexpr optional_base_helper_mc() noexcept = default;
            constexpr optional_base_helper_mc(const optional_base_helper_mc&) noexcept = default;
            OPTION_CXX14_CONSTEXPR optional_base_helper_mc(optional_base_helper_mc&& other) noexcept(std::is_nothrow_move_constructible<T>::value) {
                if (static_cast<Sub&&>(other).has_value())
                    static_cast<Sub*>(this)->emplace(std::move(static_cast<Sub&&>(other).value()));
            }
        };

        template<typename T, typename Sub>
        struct optional_base_helper_mc<T, Sub, typename std::enable_if<std::is_trivially_move_constructible<T>::value>::type> { };

        template<typename T, typename Sub, class = void>
        struct optional_base_helper_ca {
            constexpr optional_base_helper_ca() noexcept = default;
            constexpr optional_base_helper_ca(const optional_base_helper_ca&) noexcept = default;
            constexpr optional_base_helper_ca(optional_base_helper_ca&&) noexcept = default;
            OPTION_CXX14_CONSTEXPR optional_base_helper_ca& operator=(const optional_base_helper_ca&) noexcept = delete;
            OPTION_CXX14_CONSTEXPR optional_base_helper_ca& operator=(optional_base_helper_ca&&) noexcept = default;
        };

        template<typename T, typename Sub>
        struct optional_base_helper_ca<T, Sub, typename std::enable_if<std::is_trivially_copy_constructible<T>::value && std::is_trivially_copy_assignable<T>::value && std::is_trivially_destructible<T>::value>::type> { };

        template<typename T, typename Sub>
        struct optional_base_helper_ca<T, Sub, typename std::enable_if<std::is_copy_constructible<T>::value && std::is_copy_assignable<T>::value
                    && !(std::is_trivially_copy_constructible<T>::value && std::is_trivially_copy_assignable<T>::value && std::is_trivially_destructible<T>::value)>::type> {
            constexpr optional_base_helper_ca() noexcept = default;
            constexpr optional_base_helper_ca(const optional_base_helper_ca&) noexcept = default;
            constexpr optional_base_helper_ca(optional_base_helper_ca&&) noexcept = default;

            OPTION_CXX14_CONSTEXPR optional_base_helper_ca& operator=(const optional_base_helper_ca& other) noexcept(std::is_nothrow_copy_constructible<T>::value && std::is_nothrow_copy_assignable<T>::value) {
                if (!static_cast<const Sub&>(other).has_value()) {
                    static_cast<Sub*>(this)->reset();
                } else if (!static_cast<Sub*>(this)->has_value()) {
                    static_cast<Sub*>(this)->emplace(static_cast<const Sub&>(other).value());
                } else {
                    static_cast<Sub*>(this)->value() = static_cast<const Sub&>(other).value();
                }
                return *this;
            }
            OPTION_CXX14_CONSTEXPR optional_base_helper_ca& operator=(optional_base_helper_ca&&) noexcept = default;
        };

        template<typename T, typename Sub, class = void>
        struct optional_base_helper_ma {
            constexpr optional_base_helper_ma() noexcept = default;
            constexpr optional_base_helper_ma(const optional_base_helper_ma&) noexcept = default;
            constexpr optional_base_helper_ma(optional_base_helper_ma&&) noexcept = default;
            OPTION_CXX14_CONSTEXPR optional_base_helper_ma& operator=(const optional_base_helper_ma&) noexcept = default;
            OPTION_CXX14_CONSTEXPR optional_base_helper_ma& operator=(optional_base_helper_ma&&) noexcept = delete;
        };

        template<typename T, typename Sub>
        struct optional_base_helper_ma<T, Sub, typename std::enable_if<std::is_trivially_move_constructible<T>::value && std::is_trivially_move_assignable<T>::value && std::is_trivially_destructible<T>::value>::type> { };

        template<typename T, typename Sub>
        struct optional_base_helper_ma<T, Sub, typename std::enable_if<std::is_move_constructible<T>::value && std::is_move_assignable<T>::value
                    && !(std::is_trivially_move_constructible<T>::value && std::is_trivially_move_assignable<T>::value && std::is_trivially_destructible<T>::value)>::type> {
            constexpr optional_base_helper_ma() noexcept = default;
            constexpr optional_base_helper_ma(const optional_base_helper_ma&) noexcept = default;
            constexpr optional_base_helper_ma(optional_base_helper_ma&&) noexcept = default;
            OPTION_CXX14_CONSTEXPR optional_base_helper_ma& operator=(const optional_base_helper_ma&) noexcept = default;
            OPTION_CXX14_CONSTEXPR optional_base_helper_ma& operator=(optional_base_helper_ma&& other) noexcept(std::is_nothrow_move_constructible<T>::value && std::is_nothrow_move_assignable<T>::value) {
                if (!static_cast<Sub&>(other).has_value()) {
                    static_cast<Sub*>(this)->reset();
                } else if (!static_cast<Sub*>(this)->has_value()) {
                    static_cast<Sub*>(this)->emplace(std::move(static_cast<Sub&>(other).value()));
                } else {
                    static_cast<Sub*>(this)->value() = std::move(static_cast<Sub&>(other).value());
                }
                return *this;
            }
        };


        template<typename T, typename Sub, class = void>
        struct optional_base_helper_de {
            OPTION_CXX20_CONSTEXPR ~optional_base_helper_de() noexcept {
                static_cast<Sub*>(this)->reset();
            }
        };

        template<typename T, typename Sub>
        struct optional_base_helper_de<T, Sub, typename std::enable_if<std::is_trivially_destructible<T>::value>::type> { };

        template<typename T, class = void>
        struct optional_base_data {
            bool valid{false};
            alignas(T) char value[sizeof(T)]{};
        };

        template<typename T>
        struct optional_base_data<T, typename void_t<decltype(optional_traits<typename std::remove_const<T>::type>::empty())>::type> {
            using traits_type = optional_traits<typename std::remove_const<T>::type>;
            using data_type = typename std::remove_reference<decltype(traits_type::empty())>::type;
            data_type value{traits_type::empty()};
        };

        template<typename T, template<typename, typename> class SubT>
        class optional_base_helper : public optional_base_helper_cc<T, SubT<T, void>>, public optional_base_helper_mc<T, SubT<T, void>>, public optional_base_helper_ca<T, SubT<T, void>>, public optional_base_helper_ma<T, SubT<T, void>>, public optional_base_helper_de<T, SubT<T, void>> {
        private:
            using Sub = SubT<T, void>;
        public:
            using value_type = T;
            using pointer = typename std::remove_reference<T>::type*;
            using const_pointer = const typename std::remove_reference<T>::type*;
            using iterator = pointer;
            using const_iterator = const_pointer;
        public:
            constexpr optional_base_helper() noexcept = default;
            constexpr optional_base_helper(nullopt_t) noexcept : optional_base_helper() {}
#if __cplusplus >= 201703L
            constexpr optional_base_helper(std::nullopt_t) noexcept : optional_base_helper() {}
#endif

            OPTION_CXX14_CONSTEXPR optional_base_helper(const optional_base_helper&) noexcept(noexcept(optional_base_helper_cc<T, Sub>(std::declval<optional_base_helper_cc<T, Sub> const&>()))) = default;
            OPTION_CXX14_CONSTEXPR optional_base_helper(optional_base_helper&&) noexcept(noexcept(optional_base_helper_mc<T, Sub>(std::declval<optional_base_helper_mc<T, Sub>>()))) = default;

            template<class U, typename std::enable_if<std::is_constructible<T, const U&>::value
                && (std::is_same<typename std::remove_cv<T>::type, bool>::value || !converts_from_any_cvref<T, optional_base_helper<U, SubT>>::value)>::type* = nullptr>
            OPTION_CXX14_CONSTEXPR OPTION_CXX20_USE_EXPLICIT(!std::is_convertible_v<const U&, T>) optional_base_helper(const optional_base_helper<U, SubT>& other) noexcept(std::is_nothrow_constructible<T, const U&>::value) {
                const bool other_valid = other.has_value();
                if (other_valid)
                    emplace(other.value());
            }

            template<class U, typename std::enable_if<std::is_constructible<T, const U&>::value
                && (std::is_same<typename std::remove_cv<T>::type, bool>::value || !converts_from_any_cvref<T, optional_base_helper<U, SubT>>::value)>::type* = nullptr>
            OPTION_CXX14_CONSTEXPR OPTION_CXX20_USE_EXPLICIT(!std::is_convertible_v<U&&, T>) optional_base_helper(optional_base_helper<U, SubT>&& other) noexcept(std::is_nothrow_constructible<T, U&&>::value) {
                const bool other_valid = other.has_value();
                if (other_valid)
                    emplace(std::move(other.value()));
            }

            template<typename... Args, typename std::enable_if<std::is_constructible<T, Args...>::value>::type* = nullptr>
            OPTION_CXX14_CONSTEXPR explicit optional_base_helper(in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible<T, Args...>::value) {
                emplace(std::forward<Args>(args)...);
            }

#if __cplusplus >= 201703L
            template<typename... Args, std::enable_if_t<std::is_constructible<T, Args...>::value>* = nullptr>
            OPTION_CXX14_CONSTEXPR explicit optional_base_helper(std::in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible<T, Args...>::value) {
                emplace(std::forward<Args>(args)...);
            }
#endif

            template<typename U, typename... Args, typename std::enable_if<std::is_constructible<T, std::initializer_list<U>&, Args...>::value>::type* = nullptr>
            OPTION_CXX14_CONSTEXPR explicit optional_base_helper(in_place_t, std::initializer_list<U> ilist, Args&&... args) noexcept(std::is_nothrow_constructible<T, std::initializer_list<U>&, Args...>::value) {
                emplace(ilist, std::forward<Args>(args)...);
            }

#if __cplusplus >= 201703L
            template<typename U, typename... Args, std::enable_if_t<std::is_constructible<T, std::initializer_list<U>&, Args...>::value>* = nullptr>
            OPTION_CXX14_CONSTEXPR explicit optional_base_helper(std::in_place_t, std::initializer_list<U> ilist, Args&&... args) noexcept(std::is_nothrow_constructible<T, std::initializer_list<U>&, Args...>::value) {
                emplace(ilist, std::forward<Args>(args)...);
            }
#endif

        public:
            template<class U = typename std::remove_cv<T>::type, typename std::enable_if<std::is_constructible<T, U&&>::value
                &&
#if __cplusplus >= 202002L
                !(std::is_same<std::remove_cvref_t<U>, in_place_t>::value ||
                  std::is_same<std::remove_cvref_t<U>, optional_base_helper>::value ||
                  std::is_same<std::remove_cvref_t<U>, Sub>::value ||
                  std::is_same<std::remove_cvref_t<U>, std::in_place_t>::value)
#else
                !(std::is_same<typename std::decay<U>::type, in_place_t>::value ||
                  std::is_same<typename std::decay<U>::type, optional_base_helper>::value ||
                  std::is_same<typename std::decay<U>::type, Sub>::value
#if __cplusplus >= 201703L
                  || std::is_same<typename std::decay<U>::type, std::in_place_t>::value
#endif
                  )
#endif
                &&
#if __cplusplus >= 202002L
                !(std::is_same<std::remove_cv_t<T>, bool>::value && specialized_optional<std::remove_cvref_t<U>>::value)
#else
                !(std::is_same<typename std::remove_cv<T>::type, bool>::value && specialized_optional<typename std::decay<U>::type>::value)
#endif
                >::type* = nullptr>
            OPTION_CXX14_CONSTEXPR OPTION_CXX20_USE_EXPLICIT(!std::is_convertible<U&&, T>::value) optional_base_helper(U&& value) noexcept(std::is_nothrow_constructible<T, U&&>::value) {
                emplace(std::forward<U>(value));
            }

        public:
            OPTION_CXX14_CONSTEXPR Sub& operator=(nullopt_t) noexcept {
                reset();
                return *static_cast<Sub*>(this);
            }

#if __cplusplus >= 201703L
            OPTION_CXX14_CONSTEXPR Sub& operator=(std::nullopt_t) noexcept {
                reset();
                return *static_cast<Sub*>(this);
            }
#endif

            OPTION_CXX14_CONSTEXPR optional_base_helper& operator=(const optional_base_helper& other) noexcept(noexcept(optional_base_helper_ca<T, Sub>::operator=(std::declval<optional_base_helper_ca<T, Sub> const&>()))) = default;
            OPTION_CXX14_CONSTEXPR optional_base_helper& operator=(optional_base_helper&& other) noexcept(noexcept(optional_base_helper_ma<T, Sub>::operator=(std::declval<optional_base_helper_ma<T, Sub>>()))) = default;

            template<class U = typename std::remove_cv<T>::type, typename std::enable_if<
#if __cplusplus >= 202002L
                !(std::is_same<std::remove_cvref_t<U>, optional_base_helper>::value ||
                  std::is_same<std::remove_cvref_t<U>, Sub>::value
                 )
#else
                !(std::is_same<typename std::decay<U>::type, optional_base_helper>::value ||
                  std::is_same<typename std::decay<U>::type, Sub>::value
                 )
#endif
                && std::is_constructible<T, U>::value
                && std::is_assignable<T&, U>::value
                && (!std::is_scalar<T>::value || std::is_same<typename std::decay<U>::type, T>::value)
                >::type* = nullptr>
            OPTION_CXX14_CONSTEXPR optional_base_helper& operator=(U&& value) noexcept(std::is_nothrow_constructible<T, U>::value && std::is_nothrow_assignable<T&, U>::value) {
                if (has_value()) {
                    this->value() = std::forward<U>(value);
                } else {
                    emplace(std::forward<U>(value));
                }
                return *this;
            }

            template<class U, typename std::enable_if<
                !(std::is_constructible<T, optional_base_helper<U, SubT>&>::value &&
                  std::is_constructible<T, const optional_base_helper<U, SubT>&>::value &&
                  std::is_constructible<T, optional_base_helper<U, SubT>&&>::value &&
                  std::is_constructible<T, const optional_base_helper<U, SubT>&&>::value &&
                  std::is_convertible<optional_base_helper<U, SubT>&, T>::value &&
                  std::is_convertible<const optional_base_helper<U, SubT>&, T>::value &&
                  std::is_convertible<optional_base_helper<U, SubT>&&, T>::value &&
                  std::is_convertible<const optional_base_helper<U, SubT>&&, T>::value &&
                  std::is_assignable<T&, optional_base_helper<U, SubT>&>::value &&
                  std::is_assignable<T&, const optional_base_helper<U, SubT>&>::value &&
                  std::is_assignable<T&, optional_base_helper<U, SubT>&&>::value &&
                  std::is_assignable<T&, const optional_base_helper<U, SubT>&&>::value)
                &&
                (std::is_constructible<T, const U&>::value && std::is_assignable<T&, const U&>::value)
                >::type* = nullptr>
            OPTION_CXX14_CONSTEXPR optional_base_helper& operator=(const optional_base_helper<U, SubT>& other) noexcept(std::is_nothrow_constructible<T, const U&>::value && std::is_nothrow_assignable<T, const U&>::value) {
                if (!other.has_value()) {
                    reset();
                } else if (!has_value()) {
                    emplace(other.value());
                } else {
                    value() = other.value();
                }
                return *this;
            }

            template<class U, typename std::enable_if<
                !(std::is_constructible<T, optional_base_helper<U, SubT>&>::value &&
                  std::is_constructible<T, const optional_base_helper<U, SubT>&>::value &&
                  std::is_constructible<T, optional_base_helper<U, SubT>&&>::value &&
                  std::is_constructible<T, const optional_base_helper<U, SubT>&&>::value &&
                  std::is_convertible<optional_base_helper<U, SubT>&, T>::value &&
                  std::is_convertible<const optional_base_helper<U, SubT>&, T>::value &&
                  std::is_convertible<optional_base_helper<U, SubT>&&, T>::value &&
                  std::is_convertible<const optional_base_helper<U, SubT>&&, T>::value &&
                  std::is_assignable<T&, optional_base_helper<U, SubT>&>::value &&
                  std::is_assignable<T&, const optional_base_helper<U, SubT>&>::value &&
                  std::is_assignable<T&, optional_base_helper<U, SubT>&&>::value &&
                  std::is_assignable<T&, const optional_base_helper<U, SubT>&&>::value)
                &&
                (std::is_constructible<T, U>::value && std::is_assignable<T&, U>::value)
                >::type* = nullptr>
            OPTION_CXX14_CONSTEXPR optional_base_helper& operator=(optional_base_helper<U, SubT>&& other) noexcept(std::is_nothrow_constructible<T, U>::value && std::is_nothrow_assignable<T, U>::value) {
                if (!other.has_value()) {
                    reset();
                } else if (!has_value()) {
                    emplace(std::move(other.value()));
                } else {
                    value() = std::move(other.value());
                }
                return *this;
            }

            OPTION_CXX14_CONSTEXPR void swap(optional_base_helper& other) noexcept(std::is_nothrow_move_constructible<T>::value) {
                if (!other.has_value()) {
                    if (!has_value()) return;
                    other.emplace(std::move(value()));
                    reset();
                    return;
                }
                if (!has_value()) {
                    return other.swap(*this);
                }
                using std::swap;
                swap(this->value(), other.value());
            }

            template<class U = typename std::remove_cv<T>::type, typename std::enable_if<std::is_copy_constructible<T>::value && std::is_convertible<U&&, T>::value>::type* = nullptr>
            OPTION_CXX14_CONSTEXPR T value_or(U&& default_value) const& noexcept(std::is_nothrow_constructible<T, U>::value) {
                return has_value() ? **this : static_cast<T>(std::forward<U>(default_value));
            }

            template<class U = typename std::remove_cv<T>::type, typename std::enable_if<std::is_move_constructible<T>::value && std::is_convertible<U&&, T>::value>::type* = nullptr>
            OPTION_CXX14_CONSTEXPR T value_or(U&& default_value) && noexcept(std::is_nothrow_constructible<T, U>::value) {
                return has_value() ? std::move(**this) : static_cast<T>(std::forward<U>(default_value));
            }
        private:
            OPTION_CXX14_CONSTEXPR value_type& value() noexcept { return static_cast<Sub*>(this)->value(); }
            OPTION_CXX14_CONSTEXPR const value_type& value() const noexcept { return static_cast<const Sub*>(this)->value(); }

            OPTION_CXX14_CONSTEXPR value_type& operator*() noexcept { return static_cast<Sub*>(this)->operator*(); }
            OPTION_CXX14_CONSTEXPR const value_type& operator*() const noexcept { return static_cast<const Sub*>(this)->operator*(); }

            OPTION_CXX14_CONSTEXPR pointer operator->() noexcept { return static_cast<Sub*>(this)->operator->(); }
            OPTION_CXX14_CONSTEXPR const_pointer operator->() const noexcept { return static_cast<const Sub*>(this)->operator->(); }

            OPTION_CXX14_CONSTEXPR operator bool() const noexcept(noexcept(has_value())) { return has_value(); }

            OPTION_CXX14_CONSTEXPR bool has_value() const { return static_cast<const Sub*>(this)->has_value(); }

            template<typename... Args>
            OPTION_CXX14_CONSTEXPR value_type& emplace(Args&&... args) noexcept(std::is_nothrow_constructible<T, Args...>::value) {
                return static_cast<Sub*>(this)->template emplace<Args...>(std::forward<Args>(args)...);
            }

            template<typename U, typename... Args, typename std::enable_if<!std::is_reference<U>::value>::type* = nullptr, typename std::enable_if<std::is_constructible<T, std::initializer_list<U>&, Args&&...>::value>::type* = nullptr>
            OPTION_CXX14_CONSTEXPR value_type& emplace(std::initializer_list<U> ilist, Args&&... args) noexcept(std::is_nothrow_constructible<T, Args...>::value) {
                return static_cast<Sub*>(this)->template emplace<U, Args...>(std::move(ilist), std::forward<Args>(args)...);
            }

            OPTION_CXX14_CONSTEXPR void reset() noexcept { static_cast<Sub*>(this)->reset(); }

        public:
            template<class F>
            OPTION_CXX14_CONSTEXPR SubT<remove_cv_t<invoke_result_t<F, T&>>, void> transform(F&& f) & noexcept(is_nothrow_invocable<F, T&>::value) {
                if (!has_value()) return nullopt;
                return SubT<remove_cv_t<invoke_result_t<F, T&>>, void>{in_place, invoke(std::forward<F>(f), **this)};
            }

            template<class F>
            OPTION_CXX14_CONSTEXPR SubT<remove_cv_t<invoke_result_t<F, T const&>>, void> transform(F&& f) const& noexcept(is_nothrow_invocable<F, T const&>::value) {
                if (!has_value()) return nullopt;
                return SubT<remove_cv_t<invoke_result_t<F, T const&>>, void>{in_place, invoke(std::forward<F>(f), **this)};
            }

            template<class F>
            OPTION_CXX14_CONSTEXPR SubT<remove_cv_t<invoke_result_t<F, T>>, void> transform(F&& f) && noexcept(is_nothrow_invocable<F, T>::value) {
                if (!has_value()) return nullopt;
                return SubT<remove_cv_t<invoke_result_t<F, T>>, void>{in_place, invoke(std::forward<F>(f), std::move(**this))};
            }

            template<class F>
            OPTION_CXX14_CONSTEXPR SubT<remove_cv_t<invoke_result_t<F, T const>>, void> transform(F&& f) const&& noexcept(is_nothrow_invocable<F, T const>::value) {
                if (!has_value()) return nullopt;
                return SubT<remove_cv_t<invoke_result_t<F, T const>>, void>{in_place, invoke(std::forward<F>(f), std::move(**this))};
            }

            template <class F>
            OPTION_CXX14_CONSTEXPR remove_cv_t<remove_reference_t<invoke_result_t<F, T&>>> and_then(F&& f) & noexcept(is_nothrow_invocable<F, T&>::value) {
                if (*this)
                    return invoke(std::forward<F>(f), value());
                else
                    return remove_cv_t<remove_reference_t<invoke_result_t<F, T &>>>{};
            }

            template <class F>
            OPTION_CXX14_CONSTEXPR remove_cv_t<remove_reference_t<invoke_result_t<F, const T&>>> and_then(F&& f) const & noexcept(is_nothrow_invocable<F, const T&>::value) {
                if (*this)
                    return invoke(std::forward<F>(f), value());
                else
                    return remove_cv_t<remove_reference_t<invoke_result_t<F, const T &>>>{};
            }

            template <class F>
            OPTION_CXX14_CONSTEXPR remove_cv_t<remove_reference_t<invoke_result_t<F, T>>> and_then(F&& f) && noexcept(is_nothrow_invocable<F, T>::value) {
                if (*this)
                    return invoke(std::forward<F>(f), std::move(value()));
                else
                    return remove_cv_t<remove_reference_t<invoke_result_t<F, T>>>{};
            }

            template <class F>
            OPTION_CXX14_CONSTEXPR remove_cv_t<remove_reference_t<invoke_result_t<F, T>>> and_then(F&& f) const && noexcept(is_nothrow_invocable<F, T>::value) {
                if (*this)
                    return invoke(std::forward<F>(f), std::move(value()));
                else
                    return remove_cv_t<remove_reference_t<invoke_result_t<F, const T>>>{};
            }

            template<class F>
            OPTION_CXX14_CONSTEXPR Sub or_else(F&& f) const& noexcept(noexcept(std::forward<F>(f)())) { return *this ? *static_cast<const Sub*>(this) : std::forward<F>(f)(); }

            template<class F>
            OPTION_CXX14_CONSTEXPR Sub or_else(F&& f) && noexcept(noexcept(std::forward<F>(f)())) { return *this ? std::move(*static_cast<Sub*>(this)) : std::forward<F>(f)(); }
        protected:
            optional_base_data<T> m_data;

            template<typename U, template<typename, typename> class SubU>
            friend class optional_base_helper;
        };

        template<typename T, class = void>
        class optional_base : public optional_base_helper<T, optional_base> {
        public:
            using value_type = typename optional_base_helper<T, optional_base>::value_type;
            using pointer = typename optional_base_helper<T, optional_base>::pointer;
            using const_pointer = typename optional_base_helper<T, optional_base>::const_pointer;
            using iterator = typename optional_base_helper<T, optional_base>::iterator;
            using const_iterator = typename optional_base_helper<T, optional_base>::const_iterator;
        public:
            using optional_base_helper<T, optional_base>::optional_base_helper;

            value_type& value() noexcept { return *reinterpret_cast<pointer>(this->m_data.value); }
            const value_type& value() const noexcept { return *reinterpret_cast<const_pointer>(this->m_data.value); }

            value_type& operator*() noexcept { return value(); }
            const value_type& operator*() const noexcept { return value(); }

            pointer operator->() noexcept { return reinterpret_cast<pointer>(this->m_data.value); }
            const_pointer operator->() const noexcept { return reinterpret_cast<const_pointer>(this->m_data.value); }

            constexpr operator bool() const noexcept { return has_value(); }

            constexpr bool has_value() const noexcept { return this->m_data.valid; }

            template<typename... Args>
            value_type& emplace(Args&&... args) noexcept(std::is_nothrow_constructible<T, Args...>::value) {
                reset();
                new (this->m_data.value) T(std::forward<Args>(args)...);
                this->m_data.valid = true;
                return value();
            }

            template<typename U, typename... Args, typename std::enable_if<!std::is_reference<U>::value>::type* = nullptr, typename std::enable_if<std::is_constructible<T, std::initializer_list<U>&, Args&&...>::value>::type* = nullptr>
            value_type& emplace(std::initializer_list<U> ilist, Args&&... args) noexcept(std::is_nothrow_constructible<T, Args...>::value) {
                reset();
                new (this->m_data.value) T(ilist, std::forward<Args>(args)...);
                this->m_data.valid = true;
                return value();
            }

            OPTION_CXX14_CONSTEXPR void reset() noexcept {
                if (this->m_data.valid)
                    value().T::~T();
                this->m_data.valid = false;
            }

            iterator begin() noexcept { return reinterpret_cast<pointer>(this->m_data.value); }
            const_iterator begin() const noexcept { return reinterpret_cast<const_pointer>(this->m_data.value); }

            iterator end() noexcept { return reinterpret_cast<pointer>(this->m_data.value) + this->m_data.valid; }
            const_iterator end() const noexcept { return reinterpret_cast<const_pointer>(this->m_data.value) + this->m_data.valid; }
        };


        template<typename T>
        class optional_base<T, typename void_t<decltype(optional_traits<typename std::remove_const<T>::type>::empty())>::type>
            : public optional_base_helper<T, optional_base> {
          public:
              using value_type = typename optional_base_helper<T, optional_base>::value_type;
              using pointer = typename optional_base_helper<T, optional_base>::pointer;
              using const_pointer = typename optional_base_helper<T, optional_base>::const_pointer;
              using iterator = typename optional_base_helper<T, optional_base>::iterator;
              using const_iterator = typename optional_base_helper<T, optional_base>::const_iterator;
          private:
              using traits_type = optional_traits<typename std::remove_const<T>::type>;
              using data_type = typename std::remove_reference<decltype(traits_type::empty())>::type;

              template<class, class U, typename P, typename... Args>
              struct constructor {
                  static constexpr std::is_nothrow_constructible<U, Args...> noexcept_cond{};
                  void operator()(P* p, Args&&... args) const noexcept(std::is_nothrow_constructible<U, Args...>::value) {
                      new (p) U(std::forward<Args>(args)...);
                  }
              };

              template<class U, typename P, typename... Args>
              struct constructor<typename void_t<decltype(optional_base<U>::traits_type::construct(std::declval<P*>(), std::declval<Args>()...))>::type, U, P, Args...> {
                  static constexpr std::integral_constant<bool, noexcept(optional_traits<U>::construct(std::declval<P*>(), std::declval<Args>()...))> noexcept_cond{};
                  OPTION_CXX14_CONSTEXPR void operator()(P* p, Args&&... args) const noexcept(noexcept(optional_base<U>::traits_type::construct(std::declval<P*>(), std::declval<Args>()...))) {
                      traits_type::construct(p, std::forward<Args>(args)...);
                  }
              };
          public:
              using optional_base_helper<T, optional_base>::optional_base_helper;

              OPTION_CXX14_CONSTEXPR value_type& value() noexcept(noexcept(value_impl(void_t<void>{}))) { return value_impl(void_t<void>{}); }
              OPTION_CXX14_CONSTEXPR const value_type& value() const noexcept(noexcept(value_impl(void_t<void>{}))) { return value_impl(void_t<void>{}); }

              OPTION_CXX14_CONSTEXPR value_type& operator*() noexcept(noexcept(value())) { return value(); }
              OPTION_CXX14_CONSTEXPR const value_type& operator*() const noexcept(noexcept(value())) { return value(); }

              OPTION_CXX14_CONSTEXPR pointer operator->() noexcept(noexcept(has_value()) && noexcept(value())) { return has_value() ? &value() : nullptr; }
              OPTION_CXX14_CONSTEXPR const_pointer operator->() const noexcept(noexcept(has_value()) && noexcept(value())) { return has_value() ? &value() : nullptr; }

              OPTION_CXX14_CONSTEXPR operator bool() const noexcept(noexcept(has_value())) { return has_value(); }
              OPTION_CXX14_CONSTEXPR bool has_value() const noexcept(noexcept(has_value_impl(void_t<void, void>{}))) { return has_value_impl(void_t<void, void>{}); }

              OPTION_CXX14_CONSTEXPR void reset() noexcept { reset_impl(void_t<void>{}); }

              OPTION_CXX14_CONSTEXPR iterator begin() noexcept(noexcept(operator->())) { return operator->(); }
              OPTION_CXX14_CONSTEXPR const_iterator begin() const noexcept(noexcept(operator->())) { return operator->(); }

              OPTION_CXX14_CONSTEXPR iterator end() noexcept(noexcept(operator->()) && noexcept(has_value())) { return operator->() + has_value(); }
              OPTION_CXX14_CONSTEXPR const_iterator end() const noexcept(noexcept(operator->()) && noexcept(has_value())) { return operator->() + has_value(); }

              template<typename... Args>
              OPTION_CXX14_CONSTEXPR value_type& emplace(Args&&... args) noexcept(constructor<void, T, data_type, Args...>::noexcept_cond.value) {
                  reset();
                  constructor<void, T, data_type, Args...>{}(&this->m_data.value, std::forward<Args>(args)...);
                  return value();
              }

              template<typename U, typename... Args, typename std::enable_if<!std::is_reference<U>::value>::type* = nullptr, typename std::enable_if<std::is_constructible<T, std::initializer_list<U>&, Args&&...>::value>::type* = nullptr>
              OPTION_CXX14_CONSTEXPR value_type& emplace(std::initializer_list<U> ilist, Args&&... args) noexcept(constructor<void, T, data_type, Args...>::noexcept_cond.value) {
                  reset();
                  constructor<void, T, data_type, std::initializer_list<U>, Args...>{}(&this->m_data.value, ilist, std::forward<Args>(args)...);
                  return value();
              }

          private:
              template<typename U = T>
              void reset_impl(void_t<>) noexcept {
                  if (has_value()) {
                      this->m_data.value.data_type::~data_type();
                      new (&this->m_data.value) data_type(traits_type::empty());
                  }
              }

              template<class U = T, typename void_t<decltype(optional_base<U>::traits_type::destruct(std::declval<data_type&>()))>::type* = nullptr>
              void reset_impl(void_t<void>) noexcept {
                  if (has_value()) {
                      traits_type::destruct(this->m_data.value);
                      new (&this->m_data.value) data_type(traits_type::empty());
                  }
              }

              template<typename U = T>
              OPTION_CXX14_CONSTEXPR value_type& value_impl(void_t<>) noexcept { return static_cast<value_type&>(this->m_data.value); }

              template<typename U = T>
              OPTION_CXX14_CONSTEXPR const value_type& value_impl(void_t<>) const noexcept { return static_cast<const value_type&>(this->m_data.value); }

              template<class U = T, typename void_t<decltype(optional_base<U>::traits_type::convert(std::declval<data_type&>()))>::type* = nullptr>
              OPTION_CXX14_CONSTEXPR value_type& value_impl(void_t<void>) noexcept(noexcept(traits_type::convert(std::declval<data_type&>()))) {
                  return traits_type::convert(this->m_data.value);
              }

              template<class U = T, typename void_t<decltype(optional_base<U>::traits_type::convert(std::declval<const data_type&>()))>::type* = nullptr>
              OPTION_CXX14_CONSTEXPR const value_type& value_impl(void_t<void>) const noexcept(noexcept(traits_type::convert(std::declval<const data_type&>()))) {
                  return traits_type::convert(this->m_data.value);
              }

              template<typename U = T>
              OPTION_CXX14_CONSTEXPR bool has_value_impl(void_t<>) const noexcept(noexcept(!(std::declval<const typename optional_base<U>::data_type&>() == std::declval<typename optional_base<U>::data_type>()))) {
                  return !(this->m_data.value == traits_type::empty());
              }

              template<class U = T, typename void_t<decltype(std::declval<const typename optional_base<U>::data_type&>() != std::declval<typename optional_base<U>::data_type>())>::type* = nullptr>
              OPTION_CXX14_CONSTEXPR bool has_value_impl(void_t<void>) const noexcept(noexcept(std::declval<const typename optional_base<U>::data_type&>() != std::declval<typename optional_base<U>::data_type>())) {
                  return this->m_data.value != traits_type::empty();
              }

              template<class U = T, typename void_t<decltype(optional_base<U>::traits_type::is_empty(std::declval<const data_type&>()))>::type* = nullptr>
              OPTION_CXX14_CONSTEXPR bool has_value_impl(void_t<void, void>) const noexcept(noexcept(!traits_type::is_empty(std::declval<const data_type&>()))) {
                  return !traits_type::is_empty(this->m_data.value);
              }
        };

        template<typename>
        struct specialized_optional : std::false_type {};

        template<typename U>
        struct specialized_optional<optional_base<U>> : std::true_type {};

        template<typename U, template<typename, typename> class Dummy>
        struct specialized_optional<optional_base_helper<U, Dummy>> : std::true_type {};

        template<typename T, typename U>
        OPTION_CXX14_CONSTEXPR bool operator==(const optional_base<T>& left, const optional_base<U>& right) {
          const bool left_valid = left.has_value();
          if (left_valid != right.has_value())
            return false;
          return !left_valid || left.value() == right.value();
        }

        template<typename T, typename U>
        OPTION_CXX14_CONSTEXPR bool operator!=(const optional_base<T>& left, const optional_base<U>& right) {
          const bool left_valid = left.has_value();
          if (left_valid != right.has_value())
            return true;
          return left_valid && left.value() != right.value();
        }

        template<typename T, typename U>
        OPTION_CXX14_CONSTEXPR bool operator<(const optional_base<T>& lhs, const optional_base<U>& rhs) {
            return !rhs ? false : (!lhs ? true : *lhs < *rhs);
        }

        template<typename T, typename U>
        OPTION_CXX14_CONSTEXPR bool operator<=(const optional_base<T>& lhs, const optional_base<U>& rhs) {
            return !lhs ? true : (!rhs ? false : *lhs <= *rhs);
        }

        template<typename T, typename U>
        OPTION_CXX14_CONSTEXPR bool operator>(const optional_base<T>& lhs, const optional_base<U>& rhs) {
            return !lhs ? false : (!rhs ? true : *lhs > *rhs);
        }

        template<typename T, typename U>
        OPTION_CXX14_CONSTEXPR bool operator>=(const optional_base<T>& lhs, const optional_base<U>& rhs) {
            return !rhs ? true : (!lhs ? false : *lhs >= *rhs);
        }

#if __cplusplus >= 202002L
        template<class T, std::three_way_comparable_with<T> U>
        constexpr std::compare_three_way_result_t<T, U> operator<=>(const optional_base<T>& lhs, const optional_base<U>& rhs) {
            const bool left_valid = bool(lhs);
            const bool right_valid = bool(lhs);
            return left_valid && right_valid ? *lhs <=> *rhs : left_valid <=> right_valid;
        }
#endif

        template<class T>
        OPTION_CXX14_CONSTEXPR bool operator==(const optional_base<T>& opt, nullopt_t) noexcept { return !opt; }

        template<class T>
        OPTION_CXX14_CONSTEXPR bool operator==(nullopt_t, const optional_base<T>& opt) noexcept { return !opt; }

#if __cplusplus >= 201703L
        template<class T>
        constexpr bool operator==(const optional_base<T>& opt, std::nullopt_t) noexcept { return !opt; }

        template<class T>
        constexpr bool operator==(std::nullopt_t, const optional_base<T>& opt) noexcept { return !opt; }
#endif

        template<class T>
        OPTION_CXX14_CONSTEXPR bool operator!=(const optional_base<T>& opt, nullopt_t) noexcept { return opt; }

        template<class T>
        OPTION_CXX14_CONSTEXPR bool operator!=(nullopt_t, const optional_base<T>& opt) noexcept { return opt; }

#if __cplusplus >= 201703L
        template<class T>
        constexpr bool operator!=(const optional_base<T>& opt, std::nullopt_t) noexcept { return opt; }

        template<class T>
        constexpr bool operator!=(std::nullopt_t, const optional_base<T>& opt) noexcept { return opt; }
#endif

        template<class T>
        OPTION_CXX14_CONSTEXPR bool operator<(const optional_base<T>&, nullopt_t) noexcept { return false; }

        template<class T>
        OPTION_CXX14_CONSTEXPR bool operator<(nullopt_t, const optional_base<T>& opt) noexcept { return opt; }

#if __cplusplus >= 201703L
        template<class T>
        constexpr bool operator<(const optional_base<T>&, std::nullopt_t) noexcept { return false; }

        template<class T>
        constexpr bool operator<(std::nullopt_t, const optional_base<T>& opt) noexcept { return opt; }
#endif

        template<class T>
        OPTION_CXX14_CONSTEXPR bool operator<=(const optional_base<T>& opt, nullopt_t) noexcept { return !opt; }

        template<class T>
        OPTION_CXX14_CONSTEXPR bool operator<=(nullopt_t, const optional_base<T>& opt) noexcept { return !opt; }

#if __cplusplus >= 201703L
        template<class T>
        constexpr bool operator<=(const optional_base<T>& opt, std::nullopt_t) noexcept { return !opt; }

        template<class T>
        constexpr bool operator<=(std::nullopt_t, const optional_base<T>& opt) noexcept { return !opt; }
#endif

        template<class T>
        OPTION_CXX14_CONSTEXPR bool operator>(const optional_base<T>& opt, nullopt_t) noexcept { return opt; }

        template<class T>
        OPTION_CXX14_CONSTEXPR bool operator>(nullopt_t, const optional_base<T>&) noexcept { return false; }

#if __cplusplus >= 201703L
        template<class T>
        constexpr bool operator>(const optional_base<T>& opt, std::nullopt_t) noexcept { return opt; }

        template<class T>
        constexpr bool operator>(std::nullopt_t, const optional_base<T>&) noexcept { return false; }
#endif

        template<class T>
        OPTION_CXX14_CONSTEXPR bool operator>=(const optional_base<T>&, nullopt_t) noexcept { return true; }

        template<class T>
        OPTION_CXX14_CONSTEXPR bool operator>=(nullopt_t, const optional_base<T>& opt) noexcept { return !opt; }

#if __cplusplus >= 201703L
        template<class T>
        constexpr bool operator>=(const optional_base<T>&, std::nullopt_t) noexcept { return true; }

        template<class T>
        constexpr bool operator>=(std::nullopt_t, const optional_base<T>& opt) noexcept { return !opt; }
#endif

#if __cplusplus >= 202002L
        template<class T>
        constexpr std::strong_ordering operator<=>(const optional_base<T>& opt, std::nullopt_t) noexcept { return opt.has_value() <=> false; }
#endif

        template<class T, class U, typename std::enable_if<
#if __cplusplus >= 202603L
            !specialized_optional<U>::value
#else
            true
#endif
            >::type* = nullptr
        >
        OPTION_CXX14_CONSTEXPR bool operator==(const optional_base<T>& opt, const U& value) { return opt.has_value() ? *opt == value : false; }

        template<class T, class U, typename std::enable_if<
#if __cplusplus >= 202603L
            !specialized_optional<U>::value
#else
            true
#endif
            >::type* = nullptr
        >
        OPTION_CXX14_CONSTEXPR bool operator==(const U& value, const optional_base<T>& opt) { return opt.has_value() ? value == *opt : false; }

        template<class T, class U, typename std::enable_if<
#if __cplusplus >= 202603L
            !specialized_optional<U>::value
#else
            true
#endif
            >::type* = nullptr
        >
        OPTION_CXX14_CONSTEXPR bool operator!=(const optional_base<T>& opt, const U& value) { return opt.has_value() ? *opt != value : true; }

        template<class T, class U, typename std::enable_if<
#if __cplusplus >= 202603L
            !specialized_optional<U>::value
#else
            true
#endif
            >::type* = nullptr
        >
        OPTION_CXX14_CONSTEXPR bool operator!=(const U& value, const optional_base<T>& opt) { return opt.has_value() ? value != *opt : true; }

        template<class T, class U, typename std::enable_if<
#if __cplusplus >= 202603L
            !specialized_optional<U>::value
#else
            true
#endif
            >::type* = nullptr
        >
        OPTION_CXX14_CONSTEXPR bool operator<(const optional_base<T>& opt, const U& value) { return opt.has_value() ? *opt < value : true; }

        template<class T, class U, typename std::enable_if<
#if __cplusplus >= 202603L
            !specialized_optional<U>::value
#else
            true
#endif
            >::type* = nullptr
        >
        OPTION_CXX14_CONSTEXPR bool operator<(const U& value, const optional_base<T>& opt) { return opt.has_value() ? value < *opt : false; }

        template<class T, class U, typename std::enable_if<
#if __cplusplus >= 202603L
            !specialized_optional<U>::value
#else
            true
#endif
            >::type* = nullptr
        >
        OPTION_CXX14_CONSTEXPR bool operator<=(const optional_base<T>& opt, const U& value) { return opt.has_value() ? *opt <= value : true; }

        template<class T, class U, typename std::enable_if<
#if __cplusplus >= 202603L
            !specialized_optional<U>::value
#else
            true
#endif
            >::type* = nullptr
        >
        OPTION_CXX14_CONSTEXPR bool operator<=(const U& value, const optional_base<T>& opt) { return opt.has_value() ? value <= *opt : false; }

        template<class T, class U, typename std::enable_if<
#if __cplusplus >= 202603L
            !specialized_optional<U>::value
#else
            true
#endif
            >::type* = nullptr
        >
        OPTION_CXX14_CONSTEXPR bool operator>(const optional_base<T>& opt, const U& value) { return opt.has_value() ? *opt > value : false; }

        template<class T, class U, typename std::enable_if<
#if __cplusplus >= 202603L
            !specialized_optional<U>::value
#else
            true
#endif
            >::type* = nullptr
        >
        OPTION_CXX14_CONSTEXPR bool operator>(const U& value, const optional_base<T>& opt) { return opt.has_value() ? value > *opt : true; }

        template<class T, class U, typename std::enable_if<
#if __cplusplus >= 202603L
            !specialized_optional<U>::value
#else
            true
#endif
            >::type* = nullptr
        >
        OPTION_CXX14_CONSTEXPR bool operator>=(const optional_base<T>& opt, const U& value) { return opt.has_value() ? *opt >= value : false; }

        template<class T, class U, typename std::enable_if<
#if __cplusplus >= 202603L
            !specialized_optional<U>::value
#else
            true
#endif
            >::type* = nullptr
        >
        OPTION_CXX14_CONSTEXPR bool operator>=(const U& value, const optional_base<T>& opt) { return opt.has_value() ? value >= *opt : false; }

#if __cplusplus >= 202002L
        template<class T, std::three_way_comparable_with<T> U, typename std::enable_if<
#if __cplusplus >= 202603L
            !specialized_optional<U>::value
#else
            true
#endif
            >::type* = nullptr
        >
        constexpr std::compare_three_way_result_t<T, U> operator<=>(const optional_base<T>& opt, const U& value) {
            return opt.has_value() ? *opt <=> value : std::strong_ordering::less;
        }
#endif

        template<class T, typename std::enable_if<std::is_move_constructible<T>::value
#if __cplusplus >= 201703L
            && std::is_swappable<T>::value
#endif
            >::type* = nullptr>
        void swap(detail::optional_base<T> &lhs,
                  detail::optional_base<T> &rhs) noexcept(noexcept(lhs.swap(rhs))) {
            lhs.swap(rhs);
        }
    }

    template<typename T>
    struct optional_traits<T&> {
      static constexpr T* empty() noexcept { return {}; }

      template<typename U = T>
      static void construct(T** p, U&& value) noexcept {
          new (p) T*(std::addressof(value));
      }

      static constexpr T& convert(T* p) noexcept { return *p; }

      template<class U = T, typename std::enable_if<!std::is_const<U>::value>::type* = nullptr>
      static constexpr const T& convert(const T* p) noexcept { return *p; }
    };

    namespace detail {
        struct option_bool_data_type { alignas(bool) unsigned char buf[sizeof(bool)]{}; };
    }

    template<>
    struct optional_traits<bool> {
#if __cplusplus >= 201703L
        static inline bool did_init{false};
        static inline detail::option_bool_data_type data_true{};
        static inline detail::option_bool_data_type data_false{};
        static inline detail::option_bool_data_type data_empty{};
#else
        static bool did_init;
        static detail::option_bool_data_type data_true;
        static detail::option_bool_data_type data_false;
        static detail::option_bool_data_type data_empty;
#endif

        inline static void init_empty() noexcept {
            if (did_init) {
                return;
            }

            new (data_true.buf) bool(true);
            new (data_false.buf) bool(false);

            while (std::memcmp(&data_empty, &data_true, sizeof(data_empty)) == 0 || std::memcmp(&data_empty, &data_false, sizeof(data_empty)) == 0) {
                int c = 1;
                for (std::size_t i = 0; c != 0 && i < sizeof(data_empty.buf); i++) {
                    if (data_empty.buf[i] == (1ULL << CHAR_BIT) - 1) {
                        data_empty.buf[i] = 0;
                        c = 1;
                    } else {
                        data_empty.buf[i]++;
                        c = 0;
                    }
                }
            }

            did_init = true;
        }

        inline static detail::option_bool_data_type empty() noexcept {
            init_empty();
            return data_empty;
        }

        inline static void construct(detail::option_bool_data_type* p, bool b) noexcept {
            new (p->buf) bool(b);
        }

        inline static bool& convert(detail::option_bool_data_type& d) noexcept { return *reinterpret_cast<bool*>(&d); }
        inline static const bool& convert(const detail::option_bool_data_type& d) noexcept { return *reinterpret_cast<const bool*>(&d); }

        inline static bool is_empty(const detail::option_bool_data_type& d) noexcept {
            init_empty();
            return std::memcmp(&d, &data_empty, sizeof(d)) == 0;
        }
    };

    template<typename T>
    using optional = detail::optional_base<T>;

    template<class T>
    OPTION_CXX14_CONSTEXPR optional<typename std::decay<T>::type> make_optional(T&& value) noexcept(noexcept(optional<typename std::decay<T>::type>(std::forward<T>(value)))) {
        return optional<typename std::decay<T>::type>(std::forward<T>(value));
    }

    template<class T, class... Args, typename std::enable_if<std::is_constructible<T, Args...>::value>::type* = nullptr>
    OPTION_CXX14_CONSTEXPR optional<T> make_optional(Args&&... args) noexcept(noexcept(optional<T>(in_place, std::forward<Args>(args)...))) {
        return optional<T>(in_place, std::forward<Args>(args)...);
    }

    template<class T, class U, class... Args, typename std::enable_if<!std::is_reference<U>::value>::type* = nullptr,
        typename std::enable_if<std::is_constructible<T, std::initializer_list<U>&, Args...>::value>::type* = nullptr>
    OPTION_CXX14_CONSTEXPR optional<T> make_optional(std::initializer_list<U> il, Args&&... args) noexcept(noexcept(optional<T>(in_place, il, std::forward<Args>(args)...))) {
        return optional<T>(in_place, il, std::forward<Args>(args)...);
    }
}

template<typename T>
struct std::hash<option::optional<T>> {
    OPTION_CXX14_CONSTEXPR std::size_t operator()(const option::optional<T>& o) const {
        if (!o.has_value()) return 0;
        return std::hash<typename std::remove_const<T>::type>()(o.value());
    }
};
#endif
