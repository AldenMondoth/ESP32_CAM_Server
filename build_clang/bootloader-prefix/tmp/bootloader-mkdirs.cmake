# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/atm/esp/v6.1/esp-idf/components/bootloader/subproject"
  "/home/atm/projects/esp32_projs/webcam/build_clang/bootloader"
  "/home/atm/projects/esp32_projs/webcam/build_clang/bootloader-prefix"
  "/home/atm/projects/esp32_projs/webcam/build_clang/bootloader-prefix/tmp"
  "/home/atm/projects/esp32_projs/webcam/build_clang/bootloader-prefix/src/bootloader-stamp"
  "/home/atm/projects/esp32_projs/webcam/build_clang/bootloader-prefix/src"
  "/home/atm/projects/esp32_projs/webcam/build_clang/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/atm/projects/esp32_projs/webcam/build_clang/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/atm/projects/esp32_projs/webcam/build_clang/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
