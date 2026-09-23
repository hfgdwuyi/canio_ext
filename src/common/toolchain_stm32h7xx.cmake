set(CMAKE_SYSTEM_NAME Generic)
# Path to toolchain
find_program(ARM_GCC_EXECUTABLE arm-none-eabi-gcc REQUIRED)
get_filename_component(ARM_TOOLCHAIN_BIN_DIR "${ARM_GCC_EXECUTABLE}" DIRECTORY)
find_program(ARM_GXX_EXECUTABLE arm-none-eabi-g++ HINTS "${ARM_TOOLCHAIN_BIN_DIR}" NO_DEFAULT_PATH REQUIRED)
find_program(ARM_AR_EXECUTABLE arm-none-eabi-ar HINTS "${ARM_TOOLCHAIN_BIN_DIR}" NO_DEFAULT_PATH REQUIRED)
find_program(ARM_RANLIB_EXECUTABLE arm-none-eabi-ranlib HINTS "${ARM_TOOLCHAIN_BIN_DIR}" NO_DEFAULT_PATH REQUIRED)
find_program(ARM_SIZE_EXECUTABLE arm-none-eabi-size HINTS "${ARM_TOOLCHAIN_BIN_DIR}" NO_DEFAULT_PATH REQUIRED)
find_program(ARM_OBJCOPY_EXECUTABLE arm-none-eabi-objcopy HINTS "${ARM_TOOLCHAIN_BIN_DIR}" NO_DEFAULT_PATH REQUIRED)

# Set compilers
if (UNIX)
    set(PYTHON python3)
    set(SRECORD srec_cat)
endif (UNIX)
if (WIN32)
    set(PYTHON python)
    find_program(SRECORD_EXECUTABLE
        NAMES srec_cat.exe srec_cat
        HINTS
            "C:/Install/srecord"
            "$ENV{SRECORD_ROOT}"
            "${CMAKE_SOURCE_DIR}/../../tools/srecord"
    )
    if (NOT SRECORD_EXECUTABLE)
        message(FATAL_ERROR "srec_cat not found. Install SRecord and add it to PATH, or install it at C:/Install/srecord, or set SRECORD_ROOT.")
    endif()
    set(SRECORD "${SRECORD_EXECUTABLE}")
endif (WIN32)
set(CMAKE_C_COMPILER "${ARM_GCC_EXECUTABLE}")
set(CMAKE_CXX_COMPILER "${ARM_GXX_EXECUTABLE}")
set(CMAKE_ASM_COMPILER "${ARM_GCC_EXECUTABLE}")
set(CMAKE_AR "${ARM_AR_EXECUTABLE}")
set(CMAKE_RANLIB "${ARM_RANLIB_EXECUTABLE}")

# Platform dependent flags
set(CMAKE_C_FLAGS "-mcpu=cortex-m7 -mthumb -mlittle-endian " CACHE STRING "C Compiler Base Flags")
set(CMAKE_CXX_FLAGS "-mcpu=cortex-m7 -mthumb -mlittle-endian " CACHE STRING "C++ Compiler Base Flags")
set(CMAKE_ASM_FLAGS "-mcpu=cortex-m7 -mthumb -mlittle-endian  -x assembler-with-cpp" CACHE STRING "ASM Compiler Base Flags")
set(CMAKE_LINK_FLAGS "-mcpu=cortex-m7 -mthumb -mlittle-endian " CACHE STRING "Linker Base Flags")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Size tool
set(CMAKE_SIZE "${ARM_SIZE_EXECUTABLE}")
# Object copy tool - to make hex and bin from elf
set(CMAKE_OBJCOPY "${ARM_OBJCOPY_EXECUTABLE}")

# Real executable won't be built, let's check for static library
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")