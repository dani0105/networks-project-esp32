idf_component_register(SRCS "main.c" "main.h" INCLUDE_DIRS ".")

# crea la particion para guardar archivos
# spiffs_create_partition_image(storage ../data FLASH_IN_PROJECT)