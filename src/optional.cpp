#ifdef OPTION_OPTIONAL_BOOL
#if __cplusplus < 201703L
#include <option/optional.hpp>

namespace option {
    thread_local bool option::optional_traits<bool>::data_type::did_init{};
    thread_local optional_traits<bool>::data_type::U optional_traits<bool>::data_type::data_true{true};
    thread_local optional_traits<bool>::data_type::U optional_traits<bool>::data_type::data_false{false};
    thread_local optional_traits<bool>::data_type::U optional_traits<bool>::data_type::data_empty{};
}
#endif
#endif
