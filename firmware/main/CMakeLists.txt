idf_component_register(
    SRCS main.c canbus.c ui.c ecus/rusefi/database.c ecus/rusefi/update.c
    INCLUDE_DIRS . ecus/rusefi)