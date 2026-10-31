# Right half: tell the left half (and its screen) whether it is plugged in.
if(CONFIG_SHIELD_TOUCAN_RIGHT)
  zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)
  zephyr_library_sources(usb_power_report.c)
endif()
