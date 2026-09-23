# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.20.0)

if(POLICY CMP0175)
    cmake_policy(SET CMP0175 OLD)
endif()

set(CMAKE_TOOLCHAIN_FILE ../common/toolchain_stm32h7xx.cmake)

set(ProjectName "bootloader")

project(${ProjectName} CXX C ASM)

# Project defines
add_compile_definitions(STM32H750xx CORE_CM7 USE_FULL_LL_DRIVER DATA_IN_D2_SRAM USE_HAL_DRIVER USE_USB_FS BOOTLOADER)

set_property(GLOBAL PROPERTY CXX_STANDARD 17)
set_property(GLOBAL PROPERTY C_STANDARD 11)

# Project compile options
add_compile_options( 
    -ffunction-sections 
    -fdata-sections 
    -Wall 
    -Wextra 
    -Wno-format
    -ggdb
    -O1
)


if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_options(-g3 -O0)
    add_compile_definitions(GLOBAL_DEBUG_ENABLED)
elseif(CMAKE_BUILD_TYPE MATCHES RTT)
    add_compile_options(-g3 -O0)
    add_compile_definitions(GLOBAL_DEBUG_ENABLED RTT_DEBUG)
elseif(CMAKE_BUILD_TYPE MATCHES ITM)
    add_compile_options(-g3 -O0)
    add_compile_definitions(GLOBAL_DEBUG_ENABLED ITM_DEBUG)
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    add_compile_options(-O3)
endif()

add_executable(${ProjectName} 
    "main/main.c"
    "main/nmtslave.c"
    "main/usr_301.c"
    "main/stm32h7xx_it.c"
    "main/update.c"
    "core/sysinit.c"
    "core/startup_stm32h750xx.s"
    "obj_dic/co_init.c"
    "obj_dic/objects.c"
    "diag/selftest.c"
    "diag/ram_test.c"
    #USB Protocol
    "usbd/usbd_cdc_interface.c"
    "usbd/usbd_cdc.c"
    "usbd/usbd_conf.c"
    "usbd/usbd_desc.c"
    #Canopen Protocol
    "../import/canopen/drivers/stm32_h7/cpu.c"
    "../import/canopen/drivers/stm32_h7/init_stm32_h7.c"
    "../import/canopen/drivers/shar_src/can_stm32_fdcan.c"
    "../import/canopen/drivers/shar_src/cdriver.c"
    "../import/canopen/drivers/shar_src/cpu_stm32_h7.c"
    "../import/canopen/library/source/access_o.c"
    "../import/canopen/library/source/cfg_man.c"
    "../import/canopen/library/source/cmscodec.c"
    "../import/canopen/library/source/cmsmain.c"
    "../import/canopen/library/source/co_debug.c"
    #"../import/canopen/library/source/dynmem.c"
    "../import/canopen/library/source/emerg.c"
    "../import/canopen/library/source/globvars.c"
    #"../import/canopen/library/source/guard.c"
    "../import/canopen/library/source/heartbt.c"
    "../import/canopen/library/source/led.c"
    "../import/canopen/library/source/lme.c"
    #"../import/canopen/library/source/lss.c"
    "../import/canopen/library/source/nmt.c"
    "../import/canopen/library/source/nmterr.c"
    "../import/canopen/library/source/nmtstart.c"
    "../import/canopen/library/source/nmt_m.c"
    "../import/canopen/library/source/nmt_s.c"
    "../import/canopen/library/source/pdo.c"
    "../import/canopen/library/source/sdo.c"
    "../import/canopen/library/source/sdoblock.c"
    "../import/canopen/library/source/sdoclien.c"
    "../import/canopen/library/source/sdomain.c"
    "../import/canopen/library/source/sdoserv.c"
    "../import/canopen/library/source/set_com.c"
    #"../import/canopen/library/source/slave_ut.c"
    "../import/canopen/library/source/sync.c"
    #"../import/canopen/library/source/time.c"
    "../import/canopen/library/source/timer.c"
    "../import/canopen/library/source/utility.c"

    # STM32H7 Cube HAL
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c"
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.c"
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_crc.c"
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_crc_ex.c"
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma.c"
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_iwdg.c"
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_fdcan.c"
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash.c"
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash_ex.c"
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.c"
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c.c"
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c_ex.c"
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.c"
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.c"
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_qspi.c"
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.c"
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc_ex.c"
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi.c"
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi_ex.c"
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim.c"
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim_ex.c"
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart.c"
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart_ex.c"
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pcd.c"
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pcd_ex.c"
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_usb.c"
    "../import/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_comp.c"
    "../import/STM32_USB_Device_Library/Core/Src/usbd_core.c"
    "../import/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c"
    "../import/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c"
    #qspi chip
    "../external/w25xx_qspi.c"
    #eeprom chip
    "../external/at24xx.c"
)

target_include_directories(${ProjectName} PUBLIC
    "main"
    "core"
    "obj_dic"
    "diag"
    "usbd"
    "../import/canopen/drivers/stm32_h7"
    "../import/canopen/drivers/shar_inc"
    "../import/canopen/library/include"
    "../import/STM32H7xx_HAL_Driver/Inc"
    "../import/STM32_USB_Device_Library/Core/Inc"
    "../external"
)

#Add RTT sources, if necessary
if (CMAKE_BUILD_TYPE MATCHES RTT)
    add_subdirectory(../import/rtt rtt)
endif()


# Add subdirectories
add_subdirectory(../bsp bsp)
add_subdirectory(../common common)
add_subdirectory(../debug debug)
#add_subdirectory(../external external)

# Include CMSIS for all targets and libraries
include_directories(
    "../import/CMSIS/Include"
    "../import/CMSIS/Device/ST/STM32H7xx/Include"
)


set(LD_FILE ${CMAKE_SOURCE_DIR}/bootloader.ld)
# Project linker options
target_link_options(${ProjectName} PUBLIC 
    -T ${CMAKE_BINARY_DIR}/${ProjectName}.ld
    --specs=nano.specs 
    --specs=nosys.specs  
    -Wl,--gc-sections 
    -Wl,-Map,${CMAKE_BINARY_DIR}/${ProjectName}.map
    -Wl,--no-warn-rwx-segment
)

 
# Elf output file
set_target_properties(${ProjectName} PROPERTIES SUFFIX ".elf")

# Preprocess linker file
add_custom_command(TARGET ${ProjectName} PRE_LINK
                COMMAND ${CMAKE_C_COMPILER}
                ARGS -E -P -x c -I${CMAKE_SOURCE_DIR}/../common ${LD_FILE} >${CMAKE_BINARY_DIR}/${ProjectName}.ld
                DEPENDS ${LD_FILE}
)
# Make binary
add_custom_target(${ProjectName}-binary ALL DEPENDS ${ProjectName})
add_custom_command(TARGET ${ProjectName}-binary POST_BUILD
                COMMAND ${CMAKE_OBJCOPY}
                ARGS --gap-fill 0xFF -O ihex ${CMAKE_BINARY_DIR}/${ProjectName}.elf ${CMAKE_BINARY_DIR}/${ProjectName}_org.hex
                DEPENDS ${ProjectName})

add_custom_command(TARGET ${ProjectName}-binary POST_BUILD
                COMMAND ${SRECORD}
                ARGS  ${CMAKE_BINARY_DIR}/${ProjectName}_org.hex -intel -bit-reverse -crc32-b-e 0x100000 -crop 0x100000 0x100004 -NOT -bit-reverse --offset -0x100000 -o ${CMAKE_BINARY_DIR}/checksum.bin -binary
                DEPENDS ${CMAKE_BINARY_DIR}/${ProjectName}_org.hex) 

add_custom_command(TARGET ${ProjectName}-binary POST_BUILD
                COMMAND ${SRECORD}
                ARGS  ${CMAKE_BINARY_DIR}/${ProjectName}_org.hex -intel ${CMAKE_BINARY_DIR}/checksum.bin -binary -offset -maximum-addr ${CMAKE_BINARY_DIR}/${ProjectName}_org.hex -intel -o ${CMAKE_BINARY_DIR}/${ProjectName}.hex -intel
                DEPENDS ${CMAKE_BINARY_DIR}/checksum.bin) 
                
add_custom_command(TARGET ${ProjectName}-binary POST_BUILD
                COMMAND ${SRECORD}
                ARGS  ${CMAKE_BINARY_DIR}/${ProjectName}.hex -intel -offset - -minimum-addr ${CMAKE_BINARY_DIR}/${ProjectName}.hex -intel -o ${CMAKE_BINARY_DIR}/${ProjectName}.bin -binary
                DEPENDS ${CMAKE_BINARY_DIR}/${ProjectName}.hex)
                       
# Print size
add_custom_target(${ProjectName}-size ALL DEPENDS ${ProjectName})
add_custom_command(TARGET ${ProjectName}-size POST_BUILD COMMAND ${CMAKE_SIZE} ${ProjectName}.elf)



