set(ProgramFilesX86 "ProgramFiles(x86)")
set(ProgramFilesX86 "$ENV{${ProgramFilesX86}}")

find_program(clang_format NAMES clang-format PATHS
  $ENV{VCPKG_ROOT}/triplets/toolchain/llvm/bin
  $ENV{ProgramW6432}/LLVM/bin
  $ENV{ProgramFiles}/LLVM/bin
  ${ProgramFilesX86}/LLVM/bin)

if(NOT clang_format)
  message(FATAL_ERROR "Could not find executable: clang-format")
endif()

file(GLOB_RECURSE sources include/*.hpp include/*.h src/*.hpp src/*.cpp src/*.h src/*.c)

execute_process(COMMAND "${clang_format}" -i ${sources})
