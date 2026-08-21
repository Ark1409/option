# option

A header-only C++11-compatible extended `optional` class.

## Features
- Backporting of all `std::optional` functions to C++11
- Support for `optional<T&>`
- Allows for custom data representations of user-defined types
  - `sizeof(optional<T&>) == sizeof(T*)`
  - `sizeof(optional<bool>) == sizeof(bool)`<sup>1</sup>

## Example

```cpp
#include <option/optinal.hpp>
#include <string>

option::optional<int> empty{};
option::optional<int> also_empty{option::nullopt};

if (!also_empty.has_value()) {
    std::cout << "Nothing in it\n";
}

if (empty == also_empty) {
    std::cout << "Both empty\n";
}

option::optional<int> guess{3};
option::optional<bool> found_guess{guess == 3};

if (found_guess) {
    std::cout << "Guessed: " << *found_guess << ", same as " << found_guess.value() << std::endl;

    auto s = guess.and_then([] (bool&) { return std::string{"Holds value"}; });
    std::cout << s << std::endl;
}
```

## Optimizing for your type
If there exists a representation of your type that is guaranteed to be invalid for the sake of your use case, the
representation of that value can be used to encode an empty `optional`.

```cpp
struct A {
    void* n; // Assume can never be nullptr
};

inline bool operator==(A a, A b) noexcept {
    return a.n == b.n;
}

template<>
struct option::optional_traits<A> {
    struct data_type {
        constexpr data_type() = default; // Constructs empty representation

        constexpr data_type(option::in_place_t, void* p) noexcept : a{p} { }

        inline operator A&() noexcept { return a; }
        constexpr operator const A&() const noexcept { return a; }

        constexpr bool empty() const noexcept { return a == A{nullptr}; }

        A a{nullptr};
    };
};

struct B {
    void* n;
};

static_assert(sizeof(option::optional<A>) == sizeof(A));
static_assert(sizeof(option::optional<B>) > sizeof(option::optional<A>));
```

In this case, `option::optional<A>` will use the empty representation given by
`option::optional_traits<A>::data_type::data_type()`, reducing the size the `optional`.

If that's too much boilerplate for you, you can use the mixin `option::optional_traits_data<TSelf, T>` to implement most
of the functionality for equality comparable types:

```cpp
template<>
struct option::optional_traits<A> {
    struct data_type : option::optional_traits_data<data_type, A> {
        // Choose your empty representation
        constexpr data_type() noexcept : optional_traits_data(option::in_place, nullptr) { }

        using optional_traits_data::optional_traits_data;
    };
};
```

The struct will use `operator!=(const A&, const A&)` or `operator==(const A&, const A&)` for checking emptiness. If both are
defined, chooses the former.

### Detailed requirements
`optional<T>` will look for the nested type `data_type` in the specialization `optional_traits<T>` and will use the
following functions to implement the class' functionality:
- `data_type::data_type() noexcept` - Constructs an empty representation.
- `data_type::data_type(option::in_place_t, Args&&...)` - Constructs a representation in-place from forwarded arguments.
- `bool data_type::empty() const` - Checks stored value for emptiness.
- `T& data_type::operator T&()` - Converts the stored value into a `T&`. Required if `!std::is_convertible_v<data_type&, T&>`.
- `const T& data_type::operator const T&() const` - Same but `const` version.
- `void optional_traits<T>::make_empty(data_type*)` - `static` function that transforms a valid value into an empty
  representation. Can be used to make `reset` `constexpr` if necessary. This function is optional.

Notice how the type stored in `data_type` need not match the type `T` of the `optional` itself due to the implicit
conversion.

# Building
With a configured C++ compiler, you can build from source as follows:
```sh
cmake --preset default-release
cmake --build --preset default

# Optionally install
cmake --install build --prefix=/usr/local
```

The library itself exposes the CMake target `option::option` for linking.

Alternatively, just grab `include/option/optional.hpp` if using the library as header-only.

<sup>1</sup> To enable this optimization, you must define `OPTION_OPTIONAL_BOOL`.
In this case, `optional_traits<bool>` uses (`thread_local static`) data which requires C++17 to be inlined; lower C++ versions need linking.
