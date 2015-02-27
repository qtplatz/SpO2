# arch-config.cmake

include(CheckSymbolExists)

if( WIN32 )

  check_symbol_exists( "_M_AMD64" "" RTC_ARCH_X64 )
  if ( NOT RTC_ARCH_X64 )
    check_symbol_exists( "_M_IX86" "" RTC_ARCH_X86 )
  endif( NOT RTC_ARCH_X64 )

  check_symbol_exists( "__i386__" "" RTC_ARCH_X86 )
  check_symbol_exists( "__x86_64__" "" RTC_ARCH_X64 )

else( WIN32 )

  check_symbol_exists( "__arm__" "" RTC_ARCH_ARM )

endif()

