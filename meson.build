project('tinydhcpd', 'cpp', 
    default_options : ['cpp_std=c++20','cpp_args=-Wall -Wextra -Werror=return-type', 'b_lto=true'])

#this should already be present on the host and be automatically linked if you're doing a native build
#but during cross-compile I need to manually specify linking against gcc_s
gcc_s_dependency = meson.get_compiler('cpp').find_library('gcc_s')
libconfig_dependency = dependency('libconfig++')
executable('tinydhcpd', 'src/main.cpp', 'src/daemon.cpp', 'src/socket.cpp', 'src/datagram.cpp',
    dependencies: [gcc_s_dependency, libconfig_dependency])
