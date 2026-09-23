target_sources ( ${ProjectName} PRIVATE
    "at24xx.c"
    "ism330dlc.c"
    "ism330dlc_reg.c"
    "tlc591x.c"
    "tmp75.c"
    "w25xx_qspi.c"
    "MadgwickAHRS.c"
)
    
target_include_directories ( ${ProjectName} PUBLIC ".")

