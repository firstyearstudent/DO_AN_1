idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES AD8232 ADS1115 MAX30102 INMP441 logger sensor_config)