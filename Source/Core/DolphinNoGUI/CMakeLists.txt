add_executable(primehack-nogui
  Platform.cpp
  Platform.h
  PlatformHeadless.cpp
  MainNoGUI.cpp
)

if(X11_FOUND)
  target_sources(primehack-nogui PRIVATE PlatformX11.cpp)
  target_link_libraries(primehack-nogui PRIVATE PkgConfig::XRANDR PkgConfig::X11)
endif()

if(WIN32)
  target_sources(primehack-nogui PRIVATE PlatformWin32.cpp)
endif()

if(APPLE)
  target_sources(primehack-nogui PRIVATE PlatformMacos.mm)
  target_compile_options(primehack-nogui PRIVATE -fobjc-arc)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  target_sources(primehack-nogui PRIVATE PlatformFBDev.cpp)
endif()

if (WIN32)
  target_sources(primehack-nogui PRIVATE DolphinNoGUI.exe.manifest DolphinNoGUI.rc)

  set_target_properties(primehack-nogui PROPERTIES
    DEBUG_POSTFIX    D
    OUTPUT_NAME      PrimeHackGUI
  )
else()
  set_target_properties(primehack-nogui PROPERTIES OUTPUT_NAME primehack-nogui)
endif()

target_link_libraries(primehack-nogui
PRIVATE
  core
  uicommon
  cpp-optparse
)

if(APPLE)
  target_link_libraries(primehack-nogui
    PRIVATE
      ${APPKIT_LIBRARY}
      ${COREFOUNDATION_LIBRARY}
      ${IOK_LIBRARY}
  )
endif()

if(WIN32)
  # needed for adjusting window decorations with DwmSetWindowAttribute
  target_link_libraries(primehack-nogui PRIVATE dwmapi.lib)
endif()

if(MSVC)
  # Add precompiled header
  target_link_libraries(primehack-nogui PRIVATE use_pch)
endif()

if(USE_DISCORD_PRESENCE)
  target_compile_definitions(primehack-nogui PRIVATE -DUSE_DISCORD_PRESENCE)
endif()

set(CPACK_PACKAGE_EXECUTABLES ${CPACK_PACKAGE_EXECUTABLES} primehack-nogui)
install(TARGETS primehack-nogui RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

