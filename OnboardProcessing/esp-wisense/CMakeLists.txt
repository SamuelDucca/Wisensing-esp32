idf_component_register(SRCS src/esp_wisense.c src/esp_wisense_connection.c src/inference.cc
                       INCLUDE_DIRS include
                       PRIV_INCLUDE_DIRS private
                       REQUIRES esp_wifi
                       PRIV_REQUIRES esp_ringbuf main)
