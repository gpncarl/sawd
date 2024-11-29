install(
    TARGETS sawd_exe
    RUNTIME COMPONENT sawd_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
