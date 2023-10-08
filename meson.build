project('tinydhcpd', 'cpp',
    default_options : ['cpp_std=c++20', 'b_lto=true', 'prefix=/usr'],
    version: '0.0.1')

args = ['-Wall', '-Wextra', '-Werror=return-type']
dependencies = [dependency('libconfig++'), dependency('libcap')]

version_header = vcs_tag(
                 command: ['tools/getversion.sh', meson.source_root()],
                 input: 'templates/version.hpp.in',
                 output: 'version.hpp' 
                 )

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
  libsystemd_dep = dependency('libsystemd', required: false, 
    not_found_message: 'Option use_systemd is set to true, but libsystemd was not found on this system. Perhaps you meant to configure with \'-Duse_systemd=false?\'')
  if libsystemd_dep.found()
    args += '-DHAVE_SYSTEMD'
    dependencies += libsystemd_dep
    service_cfg = configuration_data({
        'binary_path': get_option('prefix') / get_option('bindir'),
        'config_path': get_option('sysconfdir')
        })
    configure_file(
      configuration: service_cfg,
      input: 'templates/tinydhcpd@.service.in',
      output: 'tinydhcpd@.service',
      install: true,
      install_dir: '/lib/systemd/system/'
      )
  endif
endif

if get_option('debug')
  args += '-DENABLE_TRACE'  
endif

executable('tinydhcpd', 'src/main.cpp', 'src/daemon.cpp', 'src/socket.cpp', 'src/datagram.cpp', 'src/configuration.cpp', 'src/bytemanip.cpp', 'src/log/logger.cpp', 'src/log/syslog_buffer.cpp', version_header,
    cpp_args: args,
    dependencies: dependencies,
    install: true)
