# Compile static library
add_compile_options(
  $<$<CONFIG:>:/MT>
  $<$<CONFIG:Debug>:/MTd>
  $<$<CONFIG:Release>:/MT>
)

set(CompilerFlags
  CMAKE_CXX_FLAGS
  CMAKE_CXX_FLAGS_DEBUG
  CMAKE_CXX_FLAGS_RELEASE
  CMAKE_CXX_FLAGS_MINSIZEREL
  CMAKE_CXX_FLAGS_RELWITHDEBINFO
#  CMAKE_C_FLAGS
#  CMAKE_C_FLAGS_DEBUG
#  CMAKE_C_FLAGS_RELEASE
#  CMAKE_C_FLAGS_MINSIZEREL
#  CMAKE_C_FLAGS_RELWITHDEBINFO
)

foreach(flag_var ${CompilerFlags})
  if(${flag_var} MATCHES "/MD")
    string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
  elseif(${flag_var} MATCHES "/MDd")
    string(REGEX REPLACE "/MDd" "/MTd" ${flag_var} "${${flag_var}}")
  endif()
endforeach()
