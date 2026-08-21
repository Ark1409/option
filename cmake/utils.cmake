function(add_warnings target)
    if(MSVC)
        target_compile_options("${target}" PRIVATE /W4)
    else()
        target_compile_options("${target}" PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

function(add_werror target)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24")
        set_target_properties("${target}" PROPERTIES
            COMPILE_WARNING_AS_ERROR TRUE
            LINK_WARNING_AS_ERROR TRUE # CMAKE >= 4.0
        )
    else()
        if(MSVC)
            target_compile_options("${target}" PRIVATE /WX)
        else()
            target_compile_options("${target}" PRIVATE -Werror)
        endif()
    endif()
endfunction()
