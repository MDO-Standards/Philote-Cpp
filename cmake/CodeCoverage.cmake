# CodeCoverage.cmake
# Provides a function to enable code coverage on targets

function(enable_coverage TARGET_NAME)
    if(ENABLE_COVERAGE)
        # GCC and Clang
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${TARGET_NAME} PRIVATE
                $<$<CONFIG:Debug>:-g -O0 --coverage -fprofile-arcs -ftest-coverage>
            )
            # Both GCC and Clang need link flags
            target_link_options(${TARGET_NAME} PRIVATE
                $<$<CONFIG:Debug>:--coverage>
            )
        # MSVC
        elseif(MSVC)
            # MSVC doesn't have built-in coverage, would need external tools
            message(WARNING "Code coverage for MSVC requires external tools")
        endif()
    endif()
endfunction()
