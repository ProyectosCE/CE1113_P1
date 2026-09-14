set(CMAKE_SYSTEM_NAME linux)
set(CMAKE_SYSTEM_PROCESSOR arm)


set(CMAKE_C_COMPILER arm-poky-linux-gnueabi-gcc)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mthumb -mfpu=neon-vfpv4 -mfloat-abi=hard -mcpu=cortex-a7 -fstack-protector-strong -O2 -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security -Werror=format-security -D_TIME_BITS=64 -D_FILE_OFFSET_BITS=64 --sysroot=/opt/poky/5.0.19/sysroots/cortexa7t2hf-neon-vfpv4-poky-linux-gnueabi")
