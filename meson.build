project('tinydhcpd', 'cpp', 
    default_options : ['cpp_std=c++20', 'b_lto=true'],
    version: '0.0.1')

args = ['-Wall', '-Wextra', '-Werror=return-type']
dependencies = [dependency('libconfig++')]

if meson.is_cross_build()
  libc_type = meson.get_external_property('libc', 'glibc').to_lower()
  if libc_type == 'musl' or libc_type == 'musl-libc'
    args += '-D__MUSL__'
  endif
else
  result = run_command('tools/check_if_musl.sh', meson.get_compiler('cpp').get_id(), check: true)
  musl_opts = result.stdout().strip()
  if musl_opts != ''
    args += musl_opts
  endif
endif

if get_option('use_systemd')
  dependencies += dependency('libsystemd')
  args += '-DHAVE_SYSTEMD'
endif

executable('tinydhcpd', 'src/main.cpp', 'src/daemon.cpp', 'src/socket.cpp', 'src/datagram.cpp', 'src/configuration.cpp', 'src/bytemanip.cpp', 'src/logger.cpp',
    cpp_args: args,
    dependencies: dependencies)
