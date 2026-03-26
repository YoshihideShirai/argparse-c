add_library(ap_warnings INTERFACE)

option(AP_ENABLE_C_WARN_CONVERSION "Enable -Wconversion for C sources" OFF)
option(AP_ENABLE_C_WARN_SIGN_CONVERSION "Enable -Wsign-conversion for C sources" OFF)
option(AP_ENABLE_C_WARN_SHADOW "Enable -Wshadow for C sources" OFF)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(ap_warnings INTERFACE
    -Wall
    -Wextra
    -Wpedantic
  )

  if(AP_ENABLE_C_WARN_CONVERSION)
    target_compile_options(ap_warnings INTERFACE
      $<$<COMPILE_LANGUAGE:C>:-Wconversion>
    )
  endif()

  if(AP_ENABLE_C_WARN_SIGN_CONVERSION)
    target_compile_options(ap_warnings INTERFACE
      $<$<COMPILE_LANGUAGE:C>:-Wsign-conversion>
    )
  endif()

  if(AP_ENABLE_C_WARN_SHADOW)
    target_compile_options(ap_warnings INTERFACE
      $<$<COMPILE_LANGUAGE:C>:-Wshadow>
    )
  endif()
endif()
