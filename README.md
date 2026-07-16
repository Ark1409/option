# option

A C++11-compatible extended `optional` class.  
Header-only if compiling for C++17 or greater<sup>1</sup>.

## Features
- Backporting of all `std::optional` functions to C++11
- Support for `optional<T&>`
- Allows for custom data representations of user-defined types
  - `sizeof(optional<bool>) == sizeof(bool)`
  - `sizeof(optional<T&>) == sizeof(T*)`

## Example

```cpp
#include <option/optinal.hpp>

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
}
```

## Optimizing for your type
If there exists a representation of your type that is guaranteed to be invalid for the sake of your use case
(e.g. violated invariants), the representation of that value can be used to encode an empty `optional`.

```cpp
struct S {
    std::uint64_t n; // Assume can never be 3
};

inline bool operator==(S a, S b) noexcept {
    return a.n == b.n;
}

template<>
struct option::optional_traits<S> {
    static constexpr S empty() noexcept { return {3}; }
};

static_assert(sizeof(option::optional<S>) == sizeof(S));
```

In this case, `option::optional<S>` will use the empty representation given by `option::optional_traits<S>::empty()`,
reducing the size the `optional`.

### Detailed requirements
`optional<T>` will look for the function `empty` in the specialization `optional_traits<T>`, and if found will use the
following `static` functions to implement the class' functionality:
- `U optional_traits<T>::empty() noexcept` - Empty optional representation; required.
- `bool optional_traits<T>::is_empty(const U& u)` - Checks stored value for emptiness. Defaults to `operator!=(u, empty())` or `!operator==(u, empty())` (tried sequentially).
- `void optional_traits<T>::construct(U* u, Args&&... args)` - Constructs into `u` based on forwarded arguments `args`.
  Defaults to `new (u) T(std::forward<Args>(args)...)`.
- `void optional_traits<T>::destruct(U& u) noexcept` - Destroys a stored value. Defaults to `u.U::~U()`.
- `T& optional_traits<T>::convert(U& u)` - Converts the stored value into a `T&`. Required if `!std::is_convertible_v<U&, T&>`.
- `const T& optional_traits<T>::convert(const U& u)` - Same but `const` version.

Notice how the underlying type `U` need not match the type `T` of the `optional` itself.

<sup>1</sup> `optional_traits<bool>` requires (`static`) data and can only be inlined in C++17.
