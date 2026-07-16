#if __cplusplus < 201703L
#include <option/optional.hpp>

namespace option {
    bool option::optional_traits<bool>::did_init{};
    option::detail::option_bool_data_type option::optional_traits<bool>::data_true{};
    option::detail::option_bool_data_type option::optional_traits<bool>::data_false{};
    option::detail::option_bool_data_type option::optional_traits<bool>::data_empty{};
}
#endif
