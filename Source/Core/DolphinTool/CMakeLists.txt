add_executable(primehack-tool
  ToolHeadlessPlatform.cpp
  ExtractCommand.cpp
  ExtractCommand.h
  ConvertCommand.cpp
  ConvertCommand.h
  VerifyCommand.cpp
  VerifyCommand.h
  HeaderCommand.cpp
  HeaderCommand.h
  ToolMain.cpp
)

if (WIN32)
  target_sources(primehack-tool PRIVATE DolphinTool.exe.manifest DolphinTool.rc)

  set_target_properties(primehack-tool PROPERTIES
    DEBUG_POSTFIX    D
    OUTPUT_NAME      PrimeHackTool
  )
endif()

target_link_libraries(primehack-tool
PRIVATE
  discio
  uicommon
  cpp-optparse
  fmt::fmt
)

if(MSVC)
  # Add precompiled header
  target_link_libraries(primehack-tool PRIVATE use_pch)
endif()

set(CPACK_PACKAGE_EXECUTABLES ${CPACK_PACKAGE_EXECUTABLES} primehack-tool)
install(TARGETS primehack-tool RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
