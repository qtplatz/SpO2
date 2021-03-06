SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)

SET(CMAKE_C_COMPILER   /opt/raspi/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf-gcc)
SET(CMAKE_CXX_COMPILER /opt/raspi/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf-g++)
#SET(CMAKE_AR           arm-linux-gnueabihf-ar)
#SET(CMAKE_LINKER       arm-linux-gnueabihf-ld)
#SET(CMAKE_NM           arm-linux-gnueabihf-nm)
#SET(CMAKE_OBJCOPY      arm-linux-gnueabihf-objcopy)
#SET(CMAKE_OBJDUMP      arm-linux-gnueabihf-objdump)
#SET(CMAKE_STRIP        arm-linux-gnueabihf-strip)
#SET(CMAKE_RANLIB       arm-linux-gnueabihf-ranlib)

# where is the target environment
get_filename_component (_dir "${CMAKE_CURRENT_LIST_FILE}" PATH)
SET(CMAKE_FIND_ROOT_PATH /opt/raspi/arm-linux-gnueabihf /mnt/raspi-rootfs ${_dir}/.. )

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
