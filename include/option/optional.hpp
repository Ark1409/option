#ifndef OPTION_OPTIONAL_HPP_
#define OPTION_OPTIONAL_HPP_ 1

#if __cplusplus >= 201402L
#    define OPTION_CXX14_CONSTEXPR constexpr
#else
#    define OPTION_CXX14_CONSTEXPR
#endif

#if __cplusplus >= 202002L
#    define OPTION_CXX20_CONSTEXPR constexpr
#    define OPTION_USE_EXPLICIT(...) explicit(__VA_ARGS__)
#else
#    define OPTION_CXX20_CONSTEXPR
#    define OPTION_USE_EXPLICIT(...) explicit
#endif

#include <cstring>
#include <climits>
#include <type_traits>
#include <initializer_list>
#include <utility>
#include <new>
#include <memory>

#if __cplusplus >= 201703L
#    include <optional>
#    include <string_view>
#    include <functional>
#endif

namespace option {
    constexpr struct in_place_t {
        constexpr explicit in_place_t() noexcept = default;
    } in_place;

    constexpr struct nullopt_t {
        constexpr explicit nullopt_t() noexcept = default;
    } nullopt;

    template<typename>
    struct optional_traits;

    namespace detail {
        template<typename T>
        class optional_base;

        template<typename...>
        struct void_t;

        template<>
        struct void_t<> {
            typedef void type;
        };

        template<class A, class... Rest>
        struct void_t<A, Rest...> : void_t<Rest...> {};

        namespace detail2 {
            using std::swap;

            template<class T, class = void>
            struct is_swappable : std::false_type {};

            template<class T>
            struct is_swappable<T, typename void_t<decltype(swap(std::declval<T&>(), std::declval<T&>()))>::type> : std::true_type {};

            template<class T, class = void>
            struct is_nothrow_swappable : std::false_type {};

            template<class T>
            struct is_nothrow_swappable<T, typename void_t<decltype(swap(std::declval<T&>(), std::declval<T&>()))>::type>
                : std::integral_constant<bool, noexcept(swap(std::declval<T&>(), std::declval<T&>()))> {};
        }

        template<typename T>
        using is_swappable = detail2::is_swappable<T>;

        template<typename T>
        using is_nothrow_swappable = detail2::is_nothrow_swappable<T>;

        template<class T>
        struct add_const_pointer {
            using type = typename std::add_pointer<const T>;
        };

        template<class T>
        struct add_const_pointer<T&> {
            using type = typename std::add_pointer<const T&>;
        };

        template<class T>
        struct add_const_pointer<T&&> {
            using type = typename std::add_pointer<const T&&>;
        };

        template<class T>
        struct embed_ref_const { using type = const T; };

        template<class T>
        struct embed_ref_const<T&> { using type = const T&; };

        template<class T>
        struct embed_ref_const<T&&> { using type = const T&&; };

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
        struct disjunction<B1, Bn...> : std::conditional<bool(B1::value), B1, disjunction<Bn...>>::type {};

        template<class...>
        struct conjunction : std::true_type {};

        template<class B1>
        struct conjunction<B1> : B1 {};

        template<class B1, class... Bn>
        struct conjunction<B1, Bn...> : std::conditional<bool(B1::value), conjunction<Bn...>, B1>::type {};

        template<class B>
        struct not_t : std::integral_constant<bool, !B::value> {};

        template<class T, class W>
        struct converts_from_any_cvref
            : disjunction<std::is_constructible<T, W&>, std::is_convertible<W&, T>, std::is_constructible<T, W>, std::is_convertible<W, T>,
                  std::is_constructible<T, const W&>, std::is_convertible<const W&, T>, std::is_constructible<T, const W>,
                  std::is_convertible<const W, T>> {};


        template<typename C, typename Object>
        struct is_derived_object : disjunction<std::is_same<C, remove_cvref_t<Object>>, std::is_base_of<C, remove_cvref_t<Object>>> {};

        template<typename T, typename... Args>
        inline OPTION_CXX20_CONSTEXPR T* construct_at(T* t, Args&&... args) noexcept(std::is_nothrow_constructible<T, Args...>::value) {
#if __cplusplus >= 202002L
            return std::construct_at(t, std::forward<Args>(args)...)
#else
            return ::new (static_cast<void*>(t)) T(std::forward<Args>(args)...);
#endif
        }

        template<typename T, std::size_t N>
        inline OPTION_CXX20_CONSTEXPR T* construct_at(T (*t)[N]) noexcept(std::is_nothrow_default_constructible<T>::value) {
#if __cplusplus >= 202002L
            return std::construct_at(t);
#else
            return ::new (static_cast<void*>(t)) T[1]();
#endif
        }

        template<typename T>
        inline OPTION_CXX20_CONSTEXPR void destroy_at(T* t) noexcept(std::is_nothrow_destructible<T>::value) {
#if __cplusplus >= 202002L
            return std::destroy_at(t);
#else
            return t->~T();
#endif
        }

        template<typename T, std::size_t N>
        inline OPTION_CXX20_CONSTEXPR void destroy_at(T (*t)[N]) noexcept(std::is_nothrow_destructible<T>::value) {
#if __cplusplus >= 202002L
            return std::destroy_at(t);
#else
            for (auto& p : t) { (destroy_at)(std::addressof(p)); }
#endif
        }

        template<typename F, typename... Args, typename std::enable_if<!std::is_member_pointer<remove_cvref_t<F>>::value>::type* = nullptr>
        OPTION_CXX14_CONSTEXPR auto invoke(F&& f, Args&&... args) noexcept(noexcept(std::forward<F>(f)(std::forward<Args>(args)...)))
            -> decltype(std::forward<F>(f)(std::forward<Args>(args)...)) {
            return std::forward<F>(f)(std::forward<Args>(args)...);
        }

        template<typename C, typename Pointed, typename Object, typename... Args,
            typename std::enable_if<std::is_function<Pointed>::value>::type* = nullptr,
            typename std::enable_if<is_derived_object<C, Object>::value>::type* = nullptr>
        OPTION_CXX14_CONSTEXPR auto invoke(Pointed C::* member, Object&& object, Args&&... args)
            noexcept(noexcept((std::forward<Object>(object).*member)(std::forward<Args>(args)...)))
                -> decltype((std::forward<Object>(object).*member)(std::forward<Args>(args)...)) {
            return (std::forward<Object>(object).*member)(std::forward<Args>(args)...);
        }

        template<typename C, typename Pointed, typename Object, typename... Args,
            typename std::enable_if<std::is_function<Pointed>::value>::type* = nullptr>
        OPTION_CXX14_CONSTEXPR auto invoke(Pointed C::* member, std::reference_wrapper<Object> object, Args&&... args)
            noexcept(noexcept((object.get().*member)(std::forward<Args>(args)...)))
                -> decltype((object.get().*member)(std::forward<Args>(args)...)) {
            return (object.get().*member)(std::forward<Args>(args)...);
        }

        template<typename C, typename Pointed, typename Object, typename... Args,
            typename std::enable_if<std::is_function<Pointed>::value>::type* = nullptr,
            typename std::enable_if<!is_derived_object<C, Object>::value>::type* = nullptr>
        OPTION_CXX14_CONSTEXPR auto invoke(Pointed C::* member, Object&& object, Args&&... args)
            noexcept(noexcept(((*std::forward<Object>(object)).*member)(std::forward<Args>(args)...)))
                -> decltype(((*std::forward<Object>(object)).*member)(std::forward<Args>(args)...)) {
            return ((*std::forward<Object>(object)).*member)(std::forward<Args>(args)...);
        }

        template<typename C, typename Pointed, typename Object, typename std::enable_if<std::is_object<Pointed>::value>::type* = nullptr,
            typename std::enable_if<is_derived_object<C, Object>::value>::type* = nullptr>
        OPTION_CXX14_CONSTEXPR auto invoke(Pointed C::* member, Object&& object) noexcept(noexcept(std::forward<Object>(object).*member))
            -> decltype(std::forward<Object>(object).*member) {
            return std::forward<Object>(object).*member;
        }

        template<typename C, typename Pointed, typename Object, typename std::enable_if<std::is_object<Pointed>::value>::type* = nullptr>
        OPTION_CXX14_CONSTEXPR auto invoke(Pointed C::* member, std::reference_wrapper<Object> object)
            noexcept(noexcept(object.get().*member)) -> decltype(object.get().*member) {
            return object.get().*member;
        }

        template<typename C, typename Pointed, typename Object, typename std::enable_if<std::is_object<Pointed>::value>::type* = nullptr,
            typename std::enable_if<!is_derived_object<C, Object>::value>::type* = nullptr>
        OPTION_CXX14_CONSTEXPR auto invoke(Pointed C::* member, Object&& object) noexcept(noexcept((*std::forward<Object>(object)).*member))
            -> decltype((*std::forward<Object>(object)).*member) {
            return (*std::forward<Object>(object)).*member;
        }

        template<class, class F, class... Args>
        struct invoke_result_impl {};

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

        template<typename>
        struct specialized_optional : std::false_type {};

        template<typename U>
        struct specialized_optional<optional_base<U>> : std::true_type {};

        template<typename T>
        class optional_base {
        private:
            using traits_type = optional_traits<detail::remove_cv_t<T>>;
            using data_type = typename traits_type::data_type;

        public:
            using value_type = T;
            using const_value_type = const value_type;
            using reference = value_type&;
            using const_reference = const value_type&;
            using pointer = typename std::add_pointer<value_type>::type;
            using const_pointer = typename std::add_pointer<const value_type>::type;
            using iterator = pointer;
            using const_iterator = const_pointer;

        public:
            constexpr optional_base() noexcept = default;

            constexpr optional_base(nullopt_t) noexcept {}
#if __cplusplus >= 201703L
            constexpr optional_base(std::nullopt_t) noexcept {}
#endif

            template<class U, typename std::enable_if<std::is_constructible<T, const U&>::value
                                                      && (std::is_same<typename std::remove_cv<T>::type, bool>::value
                                                          || !converts_from_any_cvref<T, optional_base<U>>::value)>::type* = nullptr>
            OPTION_CXX14_CONSTEXPR OPTION_USE_EXPLICIT(!std::is_convertible_v<const U&, T>) optional_base(const optional_base<U>& other)
                noexcept(std::is_nothrow_constructible<T, const U&>::value) {
                const bool other_valid = other.has_value();
                if (other_valid) emplace(other.value());
            }

            template<class U, typename std::enable_if<std::is_constructible<T, U>::value
                                                      && (std::is_same<typename std::remove_cv<T>::type, bool>::value
                                                          || !converts_from_any_cvref<T, optional_base<U>>::value)>::type* = nullptr>
            OPTION_CXX14_CONSTEXPR OPTION_USE_EXPLICIT(!std::is_convertible_v<U&&, T>) optional_base(optional_base<U>&& other)
                noexcept(std::is_nothrow_constructible<T, U&&>::value) {
                const bool other_valid = other.has_value();
                if (other_valid) emplace(std::move(other.value()));
            }

            template<typename... Args, typename std::enable_if<std::is_constructible<T, Args...>::value>::type* = nullptr>
            OPTION_CXX14_CONSTEXPR explicit optional_base(in_place_t, Args&&... args)
                noexcept(std::is_nothrow_constructible<T, Args...>::value) {
                emplace(std::forward<Args>(args)...);
            }

#if __cplusplus >= 201703L
            template<typename... Args, std::enable_if_t<std::is_constructible<T, Args...>::value>* = nullptr>
            OPTION_CXX14_CONSTEXPR explicit optional_base(std::in_place_t, Args&&... args)
                noexcept(std::is_nothrow_constructible<T, Args...>::value) {
                emplace(std::forward<Args>(args)...);
            }
#endif

            template<typename U, typename... Args,
                typename std::enable_if<std::is_constructible<T, std::initializer_list<U>&, Args...>::value>::type* = nullptr>
            OPTION_CXX14_CONSTEXPR explicit optional_base(in_place_t, std::initializer_list<U> ilist, Args&&... args)
                noexcept(std::is_nothrow_constructible<T, std::initializer_list<U>&, Args...>::value) {
                emplace(ilist, std::forward<Args>(args)...);
            }

#if __cplusplus >= 201703L
            template<typename U, typename... Args,
                std::enable_if_t<std::is_constructible<T, std::initializer_list<U>&, Args...>::value>* = nullptr>
            OPTION_CXX14_CONSTEXPR explicit optional_base(std::in_place_t, std::initializer_list<U> ilist, Args&&... args)
                noexcept(std::is_nothrow_constructible<T, std::initializer_list<U>&, Args...>::value) {
                emplace(ilist, std::forward<Args>(args)...);
            }
#endif

        public:
            template<class U = typename std::remove_cv<T>::type,
                typename std::enable_if<
                    std::is_constructible<T, U&&>::value &&
#if __cplusplus >= 202002L
                    !(std::is_same<std::remove_cvref_t<U>, in_place_t>::value || std::is_same<std::remove_cvref_t<U>, optional_base>::value
                        || std::is_same<std::remove_cvref_t<U>, std::in_place_t>::value)
#else
                    !(std::is_same<typename std::decay<U>::type, in_place_t>::value
                        || std::is_same<typename std::decay<U>::type, optional_base>::value
#    if __cplusplus >= 201703L
                        || std::is_same<typename std::decay<U>::type, std::in_place_t>::value
#    endif
                        )
#endif
                    &&
#if __cplusplus >= 202002L
                    !(std::is_same<std::remove_cv_t<T>, bool>::value && specialized_optional<std::remove_cvref_t<U>>::value)
#else
                    !(std::is_same<typename std::remove_cv<T>::type, bool>::value
                        && specialized_optional<typename std::decay<U>::type>::value)
#endif
                >::type* = nullptr>
            OPTION_CXX14_CONSTEXPR OPTION_USE_EXPLICIT(!std::is_convertible<U&&, T>::value) optional_base(U&& value)
                noexcept(std::is_nothrow_constructible<T, U&&>::value) {
                emplace(std::forward<U>(value));
            }

        public:
            OPTION_CXX14_CONSTEXPR optional_base& operator=(nullopt_t) noexcept {
                reset();
                return *this;
            }

#if __cplusplus >= 201703L
            OPTION_CXX14_CONSTEXPR optional_base& operator=(std::nullopt_t) noexcept {
                reset();
                return *this;
            }
#endif

            template<class U = typename std::remove_cv<T>::type,
                typename std::enable_if<
#if __cplusplus >= 202002L
                    !(std::is_same<std::remove_cvref_t<U>, optional_base>::value)
#else
                    !(std::is_same<typename std::decay<U>::type, optional_base>::value)
#endif
                    && std::is_constructible<T, U>::value && std::is_assignable<T&, U>::value
                    && (!std::is_scalar<T>::value || std::is_same<typename std::decay<U>::type, T>::value)
                >::type* = nullptr>
            OPTION_CXX14_CONSTEXPR optional_base& operator=(U&& value)
                noexcept(std::is_nothrow_constructible<T, U>::value && std::is_nothrow_assignable<T&, U>::value) {
                if (has_value()) {
                    this->value() = std::forward<U>(value);
                } else {
                    emplace(std::forward<U>(value));
                }
                return *this;
            }

            template<class U,
                typename std::enable_if<
                    !(std::is_constructible<T, optional_base<U>&>::value && std::is_constructible<T, const optional_base<U>&>::value
                        && std::is_constructible<T, optional_base<U>&&>::value && std::is_constructible<T, const optional_base<U>&&>::value
                        && std::is_convertible<optional_base<U>&, T>::value && std::is_convertible<const optional_base<U>&, T>::value
                        && std::is_convertible<optional_base<U>&&, T>::value && std::is_convertible<const optional_base<U>&&, T>::value
                        && std::is_assignable<T&, optional_base<U>&>::value && std::is_assignable<T&, const optional_base<U>&>::value
                        && std::is_assignable<T&, optional_base<U>&&>::value && std::is_assignable<T&, const optional_base<U>&&>::value)
                    && (std::is_constructible<T, const U&>::value && std::is_assignable<T&, const U&>::value)
                >::type* = nullptr>
            OPTION_CXX14_CONSTEXPR optional_base& operator=(const optional_base<U>& other)
                noexcept(std::is_nothrow_constructible<T, const U&>::value && std::is_nothrow_assignable<T, const U&>::value) {
                if (!other.has_value()) {
                    reset();
                } else if (!has_value()) {
                    emplace(other.value());
                } else {
                    value() = other.value();
                }
                return *this;
            }

            template<class U,
                typename std::enable_if<
                    !(std::is_constructible<T, optional_base<U>&>::value && std::is_constructible<T, const optional_base<U>&>::value
                        && std::is_constructible<T, optional_base<U>&&>::value && std::is_constructible<T, const optional_base<U>&&>::value
                        && std::is_convertible<optional_base<U>&, T>::value && std::is_convertible<const optional_base<U>&, T>::value
                        && std::is_convertible<optional_base<U>&&, T>::value && std::is_convertible<const optional_base<U>&&, T>::value
                        && std::is_assignable<T&, optional_base<U>&>::value && std::is_assignable<T&, const optional_base<U>&>::value
                        && std::is_assignable<T&, optional_base<U>&&>::value && std::is_assignable<T&, const optional_base<U>&&>::value)
                    && (std::is_constructible<T, U>::value && std::is_assignable<T&, U>::value)
                >::type* = nullptr>
            OPTION_CXX14_CONSTEXPR optional_base& operator=(optional_base<U>&& other)
                noexcept(std::is_nothrow_constructible<T, U>::value && std::is_nothrow_assignable<T, U>::value) {
                if (!other.has_value()) {
                    reset();
                } else if (!has_value()) {
                    emplace(std::move(other.value()));
                } else {
                    value() = std::move(other.value());
                }
                return *this;
            }

            template<class U = typename std::remove_cv<T>::type,
                typename std::enable_if<std::is_copy_constructible<T>::value && std::is_convertible<U&&, T>::value>::type* = nullptr>
            constexpr T value_or(U&& default_value) const& noexcept(std::is_nothrow_constructible<T, U>::value) {
                return has_value() ? **this : static_cast<T>(std::forward<U>(default_value));
            }

            template<class U = typename std::remove_cv<T>::type,
                typename std::enable_if<std::is_move_constructible<T>::value && std::is_convertible<U&&, T>::value>::type* = nullptr>
            OPTION_CXX14_CONSTEXPR T value_or(U&& default_value) && noexcept(std::is_nothrow_constructible<T, U>::value) {
                return has_value() ? std::move(**this) : static_cast<T>(std::forward<U>(default_value));
            }

        public:
            template<class F>
            OPTION_CXX14_CONSTEXPR optional_base<remove_cv_t<invoke_result_t<F, T&>>> transform(F&& f) & noexcept(
                is_nothrow_invocable<F, T&>::value) {
                return !has_value() ? nullopt :
                                      optional_base<remove_cv_t<invoke_result_t<F, T&>>>{in_place, invoke(std::forward<F>(f), **this)};
            }

            template<class F>
            constexpr optional_base<remove_cv_t<invoke_result_t<F, const T&>>> transform(F&& f) const& noexcept(
                is_nothrow_invocable<F, const T&>::value) {
                return !has_value() ?
                           nullopt :
                           optional_base<remove_cv_t<invoke_result_t<F, const T&>>>{in_place, invoke(std::forward<F>(f), **this)};
            }

            template<class F>
            OPTION_CXX14_CONSTEXPR optional_base<remove_cv_t<invoke_result_t<F, T>>> transform(F&& f) && noexcept(
                is_nothrow_invocable<F, T>::value) {
                return !has_value() ?
                           nullopt :
                           optional_base<remove_cv_t<invoke_result_t<F, T>>>{in_place, invoke(std::forward<F>(f), std::move(**this))};
            }

            template<class F>
            constexpr optional_base<remove_cv_t<invoke_result_t<F, const T>>> transform(F&& f) const&& noexcept(
                is_nothrow_invocable<F, const T>::value) {
                return !has_value() ?
                           nullopt :
                           optional_base<remove_cv_t<invoke_result_t<F, const T>>>{in_place, invoke(std::forward<F>(f), std::move(**this))};
            }

            template<class F>
            OPTION_CXX14_CONSTEXPR remove_cvref_t<invoke_result_t<F, T&>> and_then(F&& f) & noexcept(is_nothrow_invocable<F, T&>::value) {
                return *this ? invoke(std::forward<F>(f), value()) : remove_cvref_t<invoke_result_t<F, T&>>{};
            }

            template<class F>
            constexpr remove_cvref_t<invoke_result_t<F, const T&>> and_then(F&& f) const& noexcept(
                is_nothrow_invocable<F, const T&>::value) {
                return *this ? invoke(std::forward<F>(f), value()) : remove_cvref_t<invoke_result_t<F, const T&>>{};
            }

            template<class F>
            OPTION_CXX14_CONSTEXPR remove_cvref_t<invoke_result_t<F, T>> and_then(F&& f) && noexcept(is_nothrow_invocable<F, T>::value) {
                return *this ? invoke(std::forward<F>(f), std::move(value())) : remove_cvref_t<invoke_result_t<F, T>>{};
            }

            template<class F>
            constexpr remove_cvref_t<invoke_result_t<F, T>> and_then(F&& f) const&& noexcept(is_nothrow_invocable<F, T>::value) {
                return *this ? invoke(std::forward<F>(f), std::move(value())) : remove_cvref_t<invoke_result_t<F, const T>>{};
            }

            template<class F>
            OPTION_CXX14_CONSTEXPR optional_base or_else(F&& f) && noexcept(noexcept(std::forward<F>(f)())) {
                return *this ? std::move(*static_cast<optional_base*>(this)) : std::forward<F>(f)();
            }

            template<class F>
            constexpr optional_base or_else(F&& f) const& noexcept(noexcept(std::forward<F>(f)())) {
                return *this ? *static_cast<const optional_base*>(this) : std::forward<F>(f)();
            }

        public:
            OPTION_CXX14_CONSTEXPR reference value() noexcept { return this->m_data; }

            constexpr const_reference value() const noexcept { return this->m_data; }

            OPTION_CXX14_CONSTEXPR reference operator*() noexcept { return value(); }

            const_reference operator*() const noexcept { return value(); }

            OPTION_CXX14_CONSTEXPR pointer operator->() noexcept { return has_value() ? &value() : nullptr; }

            constexpr const_pointer operator->() const noexcept { return has_value() ? &value() : nullptr; }

            constexpr operator bool() const noexcept(noexcept(has_value())) { return has_value(); }

            constexpr bool has_value() const noexcept(noexcept(!this->m_data.empty())) { return !this->m_data.empty(); }

            template<class U = T, typename std::enable_if<std::is_move_constructible<data_type>::value && is_swappable<data_type>::value,
                                      typename std::remove_reference<U>::type>* = nullptr>
            OPTION_CXX14_CONSTEXPR void swap(optional_base& other)
                noexcept(std::is_nothrow_move_constructible<data_type>::value && is_nothrow_swappable<data_type>::value) {
                if (this == &other) return;
                using std::swap;
                swap(this->m_data, other.m_data);
            }

            template<class U = T, typename... Args,
                typename std::enable_if<std::is_trivially_move_assignable<data_type>::value,
                    typename std::remove_reference<U>::type>::type* = nullptr>
            OPTION_CXX14_CONSTEXPR reference emplace(Args&&... args)
                noexcept(std::is_nothrow_constructible<data_type, in_place_t, Args...>::value) {
                this->m_data = data_type(in_place, std::forward<Args>(args)...);
                return value();
            }

            template<class U = T, typename... Args,
                typename std::enable_if<!std::is_trivially_move_assignable<data_type>::value,
                    typename std::remove_reference<U>::type>::type* = nullptr,
                typename std::enable_if<std::is_nothrow_constructible<data_type, in_place_t, Args...>::value>::type* = nullptr>
            OPTION_CXX20_CONSTEXPR reference emplace(Args&&... args) noexcept {
                this->m_data.~data_type();
                (construct_at)(&this->m_data, in_place, std::forward<Args>(args)...);
                return value();
            }

            template<class U = T, typename... Args,
                typename std::enable_if<!std::is_trivially_move_assignable<data_type>::value,
                    typename std::remove_reference<U>::type>::type* = nullptr,
                typename std::enable_if<!std::is_nothrow_constructible<data_type, in_place_t, Args...>::value>::type* = nullptr>
            OPTION_CXX20_CONSTEXPR reference emplace(Args&&... args) {
                this->m_data.~data_type();
                try {
                    (construct_at)(&this->m_data, in_place, std::forward<Args>(args)...);
                } catch (...) {
                    (construct_at)(&this->m_data);
                    throw;
                }
                return value();
            }

            OPTION_CXX14_CONSTEXPR void reset() noexcept { reset_impl(void_t<>{}); }

            OPTION_CXX14_CONSTEXPR iterator begin() noexcept(noexcept(operator->())) { return operator->(); }

            constexpr const_iterator begin() const noexcept(noexcept(operator->())) { return operator->(); }

            OPTION_CXX14_CONSTEXPR iterator end() noexcept(noexcept(operator->()) && noexcept(has_value())) {
                return operator->() + has_value();
            }

            constexpr const_iterator end() const noexcept(noexcept(operator->()) && noexcept(has_value())) {
                return operator->() + has_value();
            }

        private:
            template<class W>
            OPTION_CXX20_CONSTEXPR void reset_impl(W) noexcept(noexcept(has_value()) && std::is_nothrow_default_constructible<data_type>::value) {
                if (has_value()) {
                    this->m_data.~data_type();
                    (construct_at)(&this->m_data);
                }
            }

            template<class U = optional_base,
                typename void_t<decltype(typename U::traits_type::make_empty(std::declval<typename U::data_type*>()))>::type* = nullptr>
            OPTION_CXX14_CONSTEXPR void reset_impl(void_t<>)
                noexcept(noexcept(typename U::traits_type::make_empty(std::declval<typename U::data_type*>()))) {
                traits_type::make_empty(&this->m_data);
            }

        private:
            data_type m_data;
        };

        namespace detail2 {
            template<typename T, bool = std::is_destructible<T>::value, bool = std::is_trivially_destructible<T>::value>
            struct option_data_d;

            template<typename T>
            struct option_data_d<T, true, true> {
                constexpr option_data_d() noexcept = default;

                template<typename... Args>
                constexpr option_data_d(in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible<T, Args...>::value)
                    : d(in_place, std::forward<Args>(args)...) {}

                OPTION_CXX14_CONSTEXPR void clear() noexcept { d.valid = false; }

                struct data {
                    constexpr data() noexcept = default;

                    template<typename... Args>
                    constexpr data(in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible<T, Args...>::value)
                        : valid{true}, u(in_place, std::forward<Args>(args)...) {}

                    bool valid{false};

                    union U {
                        constexpr U() noexcept = default;

                        template<typename... Args>
                        constexpr U(in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible<T, Args...>::value)
                            : value(std::forward<Args>(args)...) {}

                        T value;
                        unsigned char dummy{};
                    } u{};
                } d;
            };

            template<typename T>
            struct option_data_d<T, true, false> {
                constexpr option_data_d() noexcept = default;

                template<typename... Args>
                constexpr option_data_d(in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible<T, Args...>::value)
                    : d(in_place, std::forward<Args>(args)...) {}

                OPTION_CXX20_CONSTEXPR ~option_data_d() noexcept { clear(); }

                OPTION_CXX20_CONSTEXPR void clear() noexcept {
                    if (this->d.valid) {
                        (destroy_at)(&this->d.u.value);
                        this->d.valid = false;
                    }
                }

                struct data {
                    constexpr data() noexcept = default;

                    template<typename... Args>
                    constexpr data(in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible<T, Args...>::value)
                        : valid{true}, u(in_place, std::forward<Args>(args)...) {}

                    bool valid{false};

                    union U {
                        constexpr U() noexcept = default;

                        template<typename... Args>
                        constexpr U(in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible<T, Args...>::value)
                            : value(std::forward<Args>(args)...) {}

                        OPTION_CXX20_CONSTEXPR ~U() noexcept { }

                        T value;
                        unsigned char dummy{};
                    } u{};
                } d;
            };

            template<typename T, bool TD>
            struct option_data_d<T, false, TD> {
                constexpr option_data_d() noexcept = default;

                template<typename... Args>
                constexpr option_data_d(in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible<T, Args...>::value)
                    : d(in_place, std::forward<Args>(args)...) {}

                OPTION_CXX20_CONSTEXPR void clear() noexcept {
                    if (this->d.valid) {
                        (destroy_at)(&this->d.u.value);
                        this->d.valid = false;
                    }
                }

                struct data {
                    constexpr data() noexcept = default;

                    template<typename... Args>
                    constexpr data(in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible<T, Args...>::value)
                        : valid{true}, u(in_place, std::forward<Args>(args)...) {}

                    bool valid{false};

                    union U {
                        constexpr U() noexcept = default;

                        template<typename... Args>
                        constexpr U(in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible<T, Args...>::value)
                            : value(std::forward<Args>(args)...) {}

                        T value;
                        unsigned char dummy{};
                    } u{};
                } d;
            };

            template<typename T, bool = std::is_copy_constructible<T>::value, bool = std::is_trivially_copy_constructible<T>::value>
            struct option_data_cc;

            template<typename T>
            struct option_data_cc<T, true, true> : option_data_d<T> {
                using option_data_d<T>::option_data_d;
            };

            template<typename T>
            struct option_data_cc<T, true, false> : option_data_d<T> {
                using option_data_d<T>::option_data_d;

                constexpr option_data_cc() noexcept = default;

                OPTION_CXX20_CONSTEXPR option_data_cc(const option_data_cc& other) noexcept(std::is_nothrow_copy_constructible<T>::value) {
                    if (other.d.valid) {
                        (construct_at)(&this->d.u, in_place, other.d.u.value);
                        this->d.valid = true;
                    }
                }

                constexpr option_data_cc(option_data_cc&&) = default;

                OPTION_CXX14_CONSTEXPR option_data_cc& operator=(const option_data_cc&) = default;
                OPTION_CXX14_CONSTEXPR option_data_cc& operator=(option_data_cc&&) = default;
            };

            template<typename T, bool TCC>
            struct option_data_cc<T, false, TCC> : option_data_d<T> {
                using option_data_d<T>::option_data_d;

                constexpr option_data_cc() noexcept = default;
                constexpr option_data_cc(const option_data_cc& other) = delete;
                constexpr option_data_cc(option_data_cc&&) = default;
                OPTION_CXX14_CONSTEXPR option_data_cc& operator=(const option_data_cc&) = default;
                OPTION_CXX14_CONSTEXPR option_data_cc& operator=(option_data_cc&&) = default;
            };

            template<typename T, bool = std::is_move_constructible<T>::value, bool = std::is_trivially_move_constructible<T>::value>
            struct option_data_mc;

            template<typename T>
            struct option_data_mc<T, true, true> : option_data_cc<T> {
                using option_data_cc<T>::option_data_cc;
            };

            template<typename T>
            struct option_data_mc<T, true, false> : option_data_cc<T> {
                using option_data_cc<T>::option_data_cc;

                constexpr option_data_mc() noexcept = default;
                constexpr option_data_mc(const option_data_mc& other) = default;

                OPTION_CXX20_CONSTEXPR option_data_mc(option_data_mc&& other) noexcept(std::is_nothrow_move_constructible<T>::value) {
                    if (other.d.valid) {
                        (construct_at)(&this->d.u, in_place, std::move(other.d.u.value));
                        this->d.valid = true;
                    }
                }

                OPTION_CXX14_CONSTEXPR option_data_mc& operator=(const option_data_mc&) = default;
                OPTION_CXX14_CONSTEXPR option_data_mc& operator=(option_data_mc&&) = default;
            };

            template<typename T, bool TMC>
            struct option_data_mc<T, false, TMC> : option_data_cc<T> {
                using option_data_cc<T>::option_data_cc;

                constexpr option_data_mc() noexcept = default;
                constexpr option_data_mc(const option_data_mc& other) = default;

                constexpr option_data_mc(option_data_mc&& other) = delete;

                OPTION_CXX14_CONSTEXPR option_data_mc& operator=(const option_data_mc&) = default;
                OPTION_CXX14_CONSTEXPR option_data_mc& operator=(option_data_mc&&) = default;
            };

            template<typename T, bool = std::is_copy_assignable<T>::value, bool = std::is_trivially_copy_assignable<T>::value>
            struct option_data_ca;

            template<typename T>
            struct option_data_ca<T, true, true> : option_data_mc<T> {
                using option_data_mc<T>::option_data_mc;
            };

            template<typename T>
            struct option_data_ca<T, true, false> : option_data_mc<T> {
                using option_data_mc<T>::option_data_mc;

                constexpr option_data_ca() noexcept = default;

                constexpr option_data_ca(const option_data_ca& other) = default;
                constexpr option_data_ca(option_data_ca&& other) = default;

                OPTION_CXX20_CONSTEXPR option_data_ca& operator=(const option_data_ca& other)
                    noexcept(std::is_nothrow_copy_constructible<T>::value && std::is_nothrow_copy_assignable<T>::value) {
                    if (other.d.valid) {
                        if (this->d.valid) {
                            this->d.u.value = other.d.u.value;
                        } else {
                            (construct_at)(&this->d.u, in_place, other.d.u.value);
                            this->d.valid = true;
                        }
                    } else {
                        this->clear();
                    }
                }

                OPTION_CXX14_CONSTEXPR option_data_ca& operator=(option_data_ca&&) = default;
            };

            template<typename T, bool TCA>
            struct option_data_ca<T, false, TCA> : option_data_mc<T> {
                using option_data_mc<T>::option_data_mc;

                constexpr option_data_ca() noexcept = default;

                constexpr option_data_ca(const option_data_ca& other) = default;
                constexpr option_data_ca(option_data_ca&& other) = default;

                OPTION_CXX14_CONSTEXPR option_data_ca& operator=(const option_data_ca& other) = delete;
                OPTION_CXX14_CONSTEXPR option_data_ca& operator=(option_data_ca&&) = default;
            };

            template<typename T, bool = std::is_copy_assignable<T>::value, bool = std::is_trivially_copy_assignable<T>::value>
            struct option_data_ma;

            template<typename T>
            struct option_data_ma<T, true, true> : option_data_ca<T> {
                using option_data_ca<T>::option_data_ca;
            };

            template<typename T>
            struct option_data_ma<T, true, false> : option_data_ca<T> {
                using option_data_ca<T>::option_data_ca;

                constexpr option_data_ma() = default;

                constexpr option_data_ma(const option_data_ma& other) = default;
                constexpr option_data_ma(option_data_ma&& other) = default;

                OPTION_CXX14_CONSTEXPR option_data_ma& operator=(const option_data_ma&) = default;

                OPTION_CXX20_CONSTEXPR option_data_ma& operator=(option_data_ma&& other)
                    noexcept(std::is_nothrow_move_constructible<T>::value && std::is_nothrow_move_assignable<T>::value) {
                    if (other.d.valid) {
                        if (this->d.valid) {
                            this->d.u.value = std::move(other.d.u.value);
                        } else {
                            (construct_at)(&this->d.u, in_place, std::move(other.d.u.value));
                            this->d.valid = true;
                        }
                    } else {
                        this->clear();
                    }
                }
            };

            template<typename T, bool TMA>
            struct option_data_ma<T, false, TMA> : option_data_ca<T> {
                using option_data_ca<T>::option_data_ca;

                constexpr option_data_ma() = default;

                constexpr option_data_ma(const option_data_ma& other) = default;
                constexpr option_data_ma(option_data_ma&& other) = default;

                OPTION_CXX14_CONSTEXPR option_data_ma& operator=(const option_data_ma&) = default;
                OPTION_CXX14_CONSTEXPR option_data_ma& operator=(option_data_ma&& other) = delete;
            };

            template<typename T>
            struct option_data_type : option_data_ma<T> {
                using option_data_ma<T>::option_data_ma;
                constexpr option_data_type() noexcept = default;

                OPTION_CXX14_CONSTEXPR operator T&() noexcept { return this->d.u.value; }

                constexpr operator const T&() const noexcept { return this->d.u.value; }

                constexpr bool empty() const noexcept { return !this->d.valid; }
            };

            template<class T, typename std::enable_if<std::is_move_constructible<T>::value && is_swappable<T>::value>::type* = nullptr>
            OPTION_CXX20_CONSTEXPR void swap(option_data_type<T>& lhs, option_data_type<T>& rhs)
                noexcept(std::is_nothrow_move_constructible<T>::value && is_nothrow_swappable<T>::value) {
                if (!lhs.d.valid) {
                    if (!rhs.d.valid) return;
                    return swap(rhs, lhs);
                }
                if (!rhs.d.valid) {
                    (construct_at)(&rhs.d.u, in_place, std::move(lhs.d.u.value));
                    lhs.clear();
                    return;
                }

                return swap(lhs.d.u.value, rhs.d.u.value);
            }
        }

        template<typename T, typename U>
        OPTION_CXX14_CONSTEXPR bool operator==(const optional_base<T>& left, const optional_base<U>& right) {
            const bool left_valid = left.has_value();
            if (left_valid != right.has_value()) return false;
            return !left_valid || left.value() == right.value();
        }

        template<typename T, typename U>
        OPTION_CXX14_CONSTEXPR bool operator!=(const optional_base<T>& left, const optional_base<U>& right) {
            const bool left_valid = left.has_value();
            if (left_valid != right.has_value()) return true;
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
        OPTION_CXX14_CONSTEXPR bool operator==(const optional_base<T>& opt, nullopt_t) noexcept {
            return !opt;
        }

        template<class T>
        OPTION_CXX14_CONSTEXPR bool operator==(nullopt_t, const optional_base<T>& opt) noexcept {
            return !opt;
        }

#if __cplusplus >= 201703L
        template<class T>
        constexpr bool operator==(const optional_base<T>& opt, std::nullopt_t) noexcept {
            return !opt;
        }

        template<class T>
        constexpr bool operator==(std::nullopt_t, const optional_base<T>& opt) noexcept {
            return !opt;
        }
#endif

        template<class T>
        OPTION_CXX14_CONSTEXPR bool operator!=(const optional_base<T>& opt, nullopt_t) noexcept {
            return opt;
        }

        template<class T>
        OPTION_CXX14_CONSTEXPR bool operator!=(nullopt_t, const optional_base<T>& opt) noexcept {
            return opt;
        }

#if __cplusplus >= 201703L
        template<class T>
        constexpr bool operator!=(const optional_base<T>& opt, std::nullopt_t) noexcept {
            return opt;
        }

        template<class T>
        constexpr bool operator!=(std::nullopt_t, const optional_base<T>& opt) noexcept {
            return opt;
        }
#endif

        template<class T>
        OPTION_CXX14_CONSTEXPR bool operator<(const optional_base<T>&, nullopt_t) noexcept {
            return false;
        }

        template<class T>
        OPTION_CXX14_CONSTEXPR bool operator<(nullopt_t, const optional_base<T>& opt) noexcept {
            return opt;
        }

#if __cplusplus >= 201703L
        template<class T>
        constexpr bool operator<(const optional_base<T>&, std::nullopt_t) noexcept {
            return false;
        }

        template<class T>
        constexpr bool operator<(std::nullopt_t, const optional_base<T>& opt) noexcept {
            return opt;
        }
#endif

        template<class T>
        OPTION_CXX14_CONSTEXPR bool operator<=(const optional_base<T>& opt, nullopt_t) noexcept {
            return !opt;
        }

        template<class T>
        OPTION_CXX14_CONSTEXPR bool operator<=(nullopt_t, const optional_base<T>& opt) noexcept {
            return !opt;
        }

#if __cplusplus >= 201703L
        template<class T>
        constexpr bool operator<=(const optional_base<T>& opt, std::nullopt_t) noexcept {
            return !opt;
        }

        template<class T>
        constexpr bool operator<=(std::nullopt_t, const optional_base<T>& opt) noexcept {
            return !opt;
        }
#endif

        template<class T>
        OPTION_CXX14_CONSTEXPR bool operator>(const optional_base<T>& opt, nullopt_t) noexcept {
            return opt;
        }

        template<class T>
        OPTION_CXX14_CONSTEXPR bool operator>(nullopt_t, const optional_base<T>&) noexcept {
            return false;
        }

#if __cplusplus >= 201703L
        template<class T>
        constexpr bool operator>(const optional_base<T>& opt, std::nullopt_t) noexcept {
            return opt;
        }

        template<class T>
        constexpr bool operator>(std::nullopt_t, const optional_base<T>&) noexcept {
            return false;
        }
#endif

        template<class T>
        OPTION_CXX14_CONSTEXPR bool operator>=(const optional_base<T>&, nullopt_t) noexcept {
            return true;
        }

        template<class T>
        OPTION_CXX14_CONSTEXPR bool operator>=(nullopt_t, const optional_base<T>& opt) noexcept {
            return !opt;
        }

#if __cplusplus >= 201703L
        template<class T>
        constexpr bool operator>=(const optional_base<T>&, std::nullopt_t) noexcept {
            return true;
        }

        template<class T>
        constexpr bool operator>=(std::nullopt_t, const optional_base<T>& opt) noexcept {
            return !opt;
        }
#endif

#if __cplusplus >= 202002L
        template<class T>
        constexpr std::strong_ordering operator<=>(const optional_base<T>& opt, std::nullopt_t) noexcept {
            return opt.has_value() <=> false;
        }
#endif

        template<class T, class U,
            typename std::enable_if<
#if __cplusplus >= 202603L
                !specialized_optional<U>::value
#else
                true
#endif
            >::type* = nullptr>
        OPTION_CXX14_CONSTEXPR bool operator==(const optional_base<T>& opt, const U& value) {
            return opt.has_value() ? *opt == value : false;
        }

        template<class T, class U,
            typename std::enable_if<
#if __cplusplus >= 202603L
                !specialized_optional<U>::value
#else
                true
#endif
            >::type* = nullptr>
        OPTION_CXX14_CONSTEXPR bool operator==(const U& value, const optional_base<T>& opt) {
            return opt.has_value() ? value == *opt : false;
        }

        template<class T, class U,
            typename std::enable_if<
#if __cplusplus >= 202603L
                !specialized_optional<U>::value
#else
                true
#endif
            >::type* = nullptr>
        OPTION_CXX14_CONSTEXPR bool operator!=(const optional_base<T>& opt, const U& value) {
            return opt.has_value() ? *opt != value : true;
        }

        template<class T, class U,
            typename std::enable_if<
#if __cplusplus >= 202603L
                !specialized_optional<U>::value
#else
                true
#endif
            >::type* = nullptr>
        OPTION_CXX14_CONSTEXPR bool operator!=(const U& value, const optional_base<T>& opt) {
            return opt.has_value() ? value != *opt : true;
        }

        template<class T, class U,
            typename std::enable_if<
#if __cplusplus >= 202603L
                !specialized_optional<U>::value
#else
                true
#endif
            >::type* = nullptr>
        OPTION_CXX14_CONSTEXPR bool operator<(const optional_base<T>& opt, const U& value) {
            return opt.has_value() ? *opt < value : true;
        }

        template<class T, class U,
            typename std::enable_if<
#if __cplusplus >= 202603L
                !specialized_optional<U>::value
#else
                true
#endif
            >::type* = nullptr>
        OPTION_CXX14_CONSTEXPR bool operator<(const U& value, const optional_base<T>& opt) {
            return opt.has_value() ? value < *opt : false;
        }

        template<class T, class U,
            typename std::enable_if<
#if __cplusplus >= 202603L
                !specialized_optional<U>::value
#else
                true
#endif
            >::type* = nullptr>
        OPTION_CXX14_CONSTEXPR bool operator<=(const optional_base<T>& opt, const U& value) {
            return opt.has_value() ? *opt <= value : true;
        }

        template<class T, class U,
            typename std::enable_if<
#if __cplusplus >= 202603L
                !specialized_optional<U>::value
#else
                true
#endif
            >::type* = nullptr>
        OPTION_CXX14_CONSTEXPR bool operator<=(const U& value, const optional_base<T>& opt) {
            return opt.has_value() ? value <= *opt : false;
        }

        template<class T, class U,
            typename std::enable_if<
#if __cplusplus >= 202603L
                !specialized_optional<U>::value
#else
                true
#endif
            >::type* = nullptr>
        OPTION_CXX14_CONSTEXPR bool operator>(const optional_base<T>& opt, const U& value) {
            return opt.has_value() ? *opt > value : false;
        }

        template<class T, class U,
            typename std::enable_if<
#if __cplusplus >= 202603L
                !specialized_optional<U>::value
#else
                true
#endif
            >::type* = nullptr>
        OPTION_CXX14_CONSTEXPR bool operator>(const U& value, const optional_base<T>& opt) {
            return opt.has_value() ? value > *opt : true;
        }

        template<class T, class U,
            typename std::enable_if<
#if __cplusplus >= 202603L
                !specialized_optional<U>::value
#else
                true
#endif
            >::type* = nullptr>
        OPTION_CXX14_CONSTEXPR bool operator>=(const optional_base<T>& opt, const U& value) {
            return opt.has_value() ? *opt >= value : false;
        }

        template<class T, class U,
            typename std::enable_if<
#if __cplusplus >= 202603L
                !specialized_optional<U>::value
#else
                true
#endif
            >::type* = nullptr>
        OPTION_CXX14_CONSTEXPR bool operator>=(const U& value, const optional_base<T>& opt) {
            return opt.has_value() ? value >= *opt : false;
        }

#if __cplusplus >= 202002L
        template<class T, std::three_way_comparable_with<T> U,
            typename std::enable_if<
#    if __cplusplus >= 202603L
                !specialized_optional<U>::value
#    else
                true
#    endif
            >::type* = nullptr>
        constexpr std::compare_three_way_result_t<T, U> operator<=>(const optional_base<T>& opt, const U& value) {
            return opt.has_value() ? *opt <=> value : std::strong_ordering::less;
        }
#endif

        template<class T, typename std::enable_if<std::is_move_constructible<T>::value && is_swappable<T>::value>::type* = nullptr>
        void swap(detail::optional_base<T>& lhs, detail::optional_base<T>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
            lhs.swap(rhs);
        }
    }

    template<typename T>
    using optional = detail::optional_base<T>;

    namespace detail {
        template<typename TSelf, typename T,  class = void>
        struct optional_traits_data {
            template<typename... Args, typename std::enable_if<std::is_constructible<T, Args...>::value>::type* = nullptr>
            constexpr optional_traits_data(in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible<T, Args...>::value)
                : t(std::forward<Args>(args)...) {}

            OPTION_CXX14_CONSTEXPR inline operator T&() noexcept { return t; }

            constexpr operator const T&() const noexcept { return t; }

            constexpr bool empty() const noexcept { return !(t == static_cast<const optional_traits_data&>(TSelf()).t); }

            T t;
        };

        template<typename TSelf, typename T>
        struct optional_traits_data<TSelf, T, typename void_t<decltype(std::declval<const T&>() != std::declval<const T&>())>::type> {
            template<typename... Args, typename std::enable_if<std::is_constructible<T, Args...>::value>::type* = nullptr>
            constexpr optional_traits_data(in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible<T, Args...>::value)
                : t(std::forward<Args>(args)...) {}

            OPTION_CXX14_CONSTEXPR inline operator T&() noexcept { return t; }

            constexpr operator const T&() const noexcept { return t; }

            constexpr bool empty() const noexcept { return t != static_cast<const optional_traits_data&>(TSelf()).t; }

            T t;
        };
    }

    template<typename T, typename TSelf>
    using optional_traits_data = detail::optional_traits_data<T, TSelf>;

    template<typename T>
    struct optional_traits {
        using data_type = detail::detail2::option_data_type<T>;

        static OPTION_CXX14_CONSTEXPR void make_empty(data_type* p) noexcept { p->clear(); }
    };

    template<typename T>
    struct optional_traits<T&> {
        struct data_type {
            constexpr data_type() noexcept = default;

            constexpr data_type(in_place_t, T& t) noexcept : value{&t} {}

            constexpr operator T&() const noexcept { return *value; }

            constexpr bool empty() const noexcept { return value == nullptr; }

            T* value{};
        };

        static inline OPTION_CXX14_CONSTEXPR void make_empty(data_type* p) noexcept { p->value = nullptr; }
    };

#ifdef OPTION_OPTIONAL_BOOL
    template<>
    struct optional_traits<bool> {
        struct data_type {
            data_type() noexcept = default;

            constexpr data_type(in_place_t, bool b) noexcept : u{b} {}

            OPTION_CXX14_CONSTEXPR operator bool&() noexcept { return u.b; }

            constexpr operator const bool&() const noexcept { return u.b; }

            bool empty() const noexcept {
                for (std::size_t i = 0; i < sizeof(bool); i++) {
                    if (*static_cast<const unsigned char*>(static_cast<const void*>(&u)) + i
                        != *static_cast<const unsigned char*>(static_cast<const void*>(&data_empty)) + i)
                        return false;
                }
                return true;
            }

            union U {
                constexpr U() noexcept = default;
#if __cplusplus <= 201103L
                constexpr U(bool b) noexcept : b{b} {}
#endif
                bool b;
                alignas(bool) unsigned char buf[sizeof(bool)]{};
            } u{init_empty()};

            inline static U init_empty() noexcept {
                if (did_init) { return data_empty; }

                data_empty = U{};

                while (std::memcmp(data_empty.buf, &data_true.b, sizeof(data_empty.buf)) == 0
                       || std::memcmp(data_empty.buf, &data_false.b, sizeof(data_empty.buf)) == 0) {
                    unsigned c = 1;
                    for (std::size_t i = 0; c != 0 && i < sizeof(data_empty.buf); i++) {
                        if (data_empty.buf[i] == (1 << CHAR_BIT) - 1) {
                            data_empty.buf[i] = 0;
                            c = 1;
                        } else {
                            data_empty.buf[i]++;
                            c = 0;
                        }
                    }
                }

                did_init = true;
                return data_empty;
            }
#if __cplusplus >= 201703L
            static thread_local inline bool did_init{false};
            static thread_local inline U data_true{true};
            static thread_local inline U data_false{false};
            static thread_local inline U data_empty{{}};
#else
            static thread_local bool did_init;
            static thread_local U data_true;
            static thread_local U data_false;
            static thread_local U data_empty;
#endif
        };

        static inline void make_empty(data_type* p) noexcept { p->u = data_type::init_empty(); }
    };
#endif

    template<class T>
    OPTION_CXX14_CONSTEXPR optional<typename std::decay<T>::type> make_optional(T&& value)
        noexcept(noexcept(optional<typename std::decay<T>::type>(std::forward<T>(value)))) {
        return optional<typename std::decay<T>::type>(std::forward<T>(value));
    }

    template<class T, class... Args, typename std::enable_if<std::is_constructible<T, Args...>::value>::type* = nullptr>
    OPTION_CXX14_CONSTEXPR optional<T> make_optional(Args&&... args)
        noexcept(noexcept(optional<T>(in_place, std::forward<Args>(args)...))) {
        return optional<T>(in_place, std::forward<Args>(args)...);
    }

    template<class T, class U, class... Args, typename std::enable_if<!std::is_reference<U>::value>::type* = nullptr,
        typename std::enable_if<std::is_constructible<T, std::initializer_list<U>&, Args...>::value>::type* = nullptr>
    OPTION_CXX14_CONSTEXPR optional<T> make_optional(std::initializer_list<U> il, Args&&... args)
        noexcept(noexcept(optional<T>(in_place, il, std::forward<Args>(args)...))) {
        return optional<T>(in_place, il, std::forward<Args>(args)...);
    }

    namespace detail {
        template<typename T, typename void_t<decltype(std::hash<typename std::remove_const<T>::type>()(
                                 std::declval<typename optional<T>::value_type>()))>::type* = nullptr>
        struct hash_helper {
            OPTION_CXX14_CONSTEXPR std::size_t operator()(const option::optional<T>& o) const
                noexcept(noexcept(std::hash<typename std::remove_const<T>::type>()(std::declval<typename optional<T>::value_type>()))) {
                using value_type = typename option::optional<T>::value_type;
#if __cplusplus >= 201703L
                if (!o.has_value()) return std::hash<std::optional<value_type>>()({std::nullopt});
#else
                if (!o.has_value()) return 0;
#endif
                return std::hash<typename std::remove_const<value_type>::type>()(o.value());
            }
        };
    }
}

template<typename T>
struct std::hash<option::optional<T>> : option::detail::hash_helper<T> {};

#endif
