if(CONFIG_AUDIO_VS1053)
  zephyr_library_named(vs1053)
  zephyr_library_sources(vs1053.c)
  # Export public header to all targets in the build
  zephyr_include_directories(.)
endif()
