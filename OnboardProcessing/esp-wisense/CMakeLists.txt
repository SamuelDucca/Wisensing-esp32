set(srcs "src/esp_wisense_connection.c")

if(NOT WISENSE_CONNECTION_ONLY)
  list(APPEND srcs "src/esp_wisense.c" "src/inference.cc")
endif()

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS include
                       PRIV_INCLUDE_DIRS private
                       REQUIRES esp_wifi
                       PRIV_REQUIRES esp_ringbuf main)
