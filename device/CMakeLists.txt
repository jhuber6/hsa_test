add_executable(image kernel.c)
target_compile_options(image PRIVATE --target=amdgcn-amd-amdhsa -mcpu=native)
target_link_options(image PRIVATE --target=amdgcn-amd-amdhsa -mcpu=native)
