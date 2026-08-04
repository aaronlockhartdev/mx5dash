idf_component_register(
    SRCS main.c canbus.c ui.c screens/gauge.c
    INCLUDE_DIRS . ecus screens)

if(CONFIG_ECU_RUSEFI)
    target_sources(${COMPONENT_LIB} PRIVATE ecus/rusefi/database.c ecus/rusefi/update.c)
endif()