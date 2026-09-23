target_sources ( ${ProjectName} PRIVATE
    "ain_lut.c"
    "can_cfg.c"
    "crc_calc.c"
    "error.c"
    "prog_ctrl.c"
    "storage.c"
    "syscalls.c"
    "terminal.c"
    "timing.c"
    "usr_303.c"
)
    
target_include_directories ( ${ProjectName} PUBLIC ".")

