project('tinydhcpd', 'cpp', 
    default_options : ['cpp_std=c++20', 'b_lto=true'])

args = ['-Wall', '-Wextra', '-Werror=return-type']

if meson.is_cross_build()
  libc_type = meson.get_external_property('libc', 'glibc')
  if libc_type == 'musl' or libc_type == 'MUSL'
    args += '-D__MUSL__'
  endif
else
  result = run_command('tools/check_if_musl.sh', meson.get_compiler('cpp').get_id(), check: true)
  musl_opts = result.stdout().strip()
  if musl_opts != ''
    args += musl_opts
  endif
endif

libconfig_dependency = dependency('libconfig++')
executable('tinydhcpd', 'src/main.cpp', 'src/daemon.cpp', 'src/socket.cpp', 'src/datagram.cpp', 'src/configuration.cpp', 'src/bytemanip.cpp',
    cpp_args: args,
    dependencies: [libconfig_dependency])
