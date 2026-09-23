target_sources ( ${ProjectName} PRIVATE
    "bsp_ain.c"
    "bsp_aout.c"
    "bsp_board.c"
    "bsp_dio.c"
    "bsp_led.c"
    "bsp_pwm.c"
    "bsp_wtdg.c"
)
    
target_include_directories ( ${ProjectName} PUBLIC ".")

