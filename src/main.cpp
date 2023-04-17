#include <arpa/inet.h>
#include <getopt.h>
#include <libconfig.h++>

#include <csignal>
#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "configuration.hpp"
#include "daemon.hpp"
#include "log/logger.hpp"
#include "log/stdout_logsink.hpp"
#include "socket.hpp"
#include "src/log/syslog_logsink.hpp"
#include "string-format.hpp"
#include "version.hpp"

#ifdef HAVE_SYSTEMD
#include "log/systemd_logsink.hpp"
#endif

#ifndef PROGRAM_VERSION
#define PROGRAM_VERSION "debug"
#endif

const char ADDRESS_TAG = 'a';
const char IFACE_TAG = 'i';
const char CONFIG_FILE_TAG = 'c';
const char FOREGROUND_TAG = 'f';
const char VERBOSE_TAG = 'v';
#ifdef HAVE_SYSTEMD
const char SYSTEMD_DAEMON_TAG = 'n';
#endif
const char SYSV_DAEMON_TAG = 'o';

const struct option long_options[] = {
    {"address", required_argument, nullptr, ADDRESS_TAG},
    {"interface", required_argument, nullptr, IFACE_TAG},
    {"configfile", required_argument, nullptr, CONFIG_FILE_TAG},
    {"foreground", no_argument, nullptr, FOREGROUND_TAG},
    {"debug", no_argument, nullptr, VERBOSE_TAG},
#ifdef HAVE_SYSTEMD
    {"systemd", no_argument, nullptr, SYSTEMD_DAEMON_TAG},
#endif
    {"sysv", no_argument, nullptr, SYSV_DAEMON_TAG},
    {nullptr, 0, nullptr, 0}};

int main(int argc, char *const argv[]) {
  std::signal(SIGINT, tinydhcpd::sighandler);
  std::signal(SIGHUP, tinydhcpd::sighandler);

  tinydhcpd::ProgramConfiguration optval = {
      .address = {.s_addr = INADDR_ANY},
      .interface = "",
      .confpath = "/etc/tinydhcpd/tinydhcpd.conf",
      .lease_file_path = "/var/lib/tinydhcpd/leases",
      .foreground = false,
#ifdef HAVE_SYSTEMD
      .daemon_type = tinydhcpd::DAEMON_TYPE::SYSTEMD,
#else
      .daemon_type = tinydhcpd::DAEMON_TYPE::SYSV,
#endif
      .subnet_config = {}};

  int opt;
  while ((opt = getopt_long(argc, argv, "a:i:c:fv", long_options, nullptr)) !=
         -1) {
    switch (opt) {
    case ADDRESS_TAG:
      inet_aton(optarg, &(optval.address));
      break;

    case IFACE_TAG:
      optval.interface = optarg;
      break;

    case CONFIG_FILE_TAG:
      optval.confpath = optarg;
      break;

    case FOREGROUND_TAG:
      optval.foreground = true;
      break;

    case VERBOSE_TAG:
      tinydhcpd::current_global_log_level = tinydhcpd::Level::DEBUG;
      break;

    case SYSV_DAEMON_TAG:
      optval.daemon_type = tinydhcpd::DAEMON_TYPE::SYSV;
      break;

#ifdef HAVE_SYSTEMD
    case SYSTEMD_DAEMON_TAG:
      optval.daemon_type = tinydhcpd::DAEMON_TYPE::SYSTEMD;
      break;
#endif

    default:
      break;
    }
  }

  if (optval.foreground) {
    tinydhcpd::global_logger.reset(
        new tinydhcpd::Logger(*(new tinydhcpd::StdoutLogSink())));
  } else if (optval.daemon_type == tinydhcpd::DAEMON_TYPE::SYSV) {
    tinydhcpd::global_logger.reset(
        new tinydhcpd::Logger(*(new tinydhcpd::SyslogLogSink())));
  }
#ifdef HAVE_SYSTEMD
  else {
    tinydhcpd::global_logger.reset(
        new tinydhcpd::Logger(*(new tinydhcpd::SystemdLogSink(std::cerr))));
  }
#endif
  tinydhcpd::LOG_INFO(
      tinydhcpd::string_format("tinydhcpd %s starting...", PROGRAM_VERSION));

  try {
    tinydhcpd::LOG_INFO(
        std::string("Loading config from file ").append(optval.confpath));
    tinydhcpd::parse_configuration(optval);
  } catch (libconfig::ParseException &pex) {
    tinydhcpd::LOG_FATAL(tinydhcpd::string_format(
        "Failed to parse config file %s at line %d! Error: %s", pex.getFile(),
        pex.getLine(), pex.getError()));
    std::exit(EXIT_FAILURE);
  } catch (libconfig::FileIOException &fex) {
    std::ostringstream os;
    os << "Error reading file " << optval.confpath << ".";
    if (!std::filesystem::exists(optval.confpath)) {
      os << " The file does not exist.";
    }
    tinydhcpd::LOG_FATAL(os.str());
    std::exit(EXIT_FAILURE);
  } catch (std::invalid_argument &ex) {
    tinydhcpd::LOG_FATAL(ex.what());
  }

  tinydhcpd::Daemon daemon(optval.address, optval.interface,
                           optval.subnet_config, optval.lease_file_path);
  tinydhcpd::LOG_INFO("Initialization finished");

  try {
    if (!optval.foreground) {
      daemon.daemonize(optval.daemon_type);
    } else {
      tinydhcpd::LOG_INFO("Running in foreground.");
    }
    std::signal(SIGTERM, tinydhcpd::sighandler);
    daemon.main_loop();
    daemon.write_leases();
  } catch (std::runtime_error &e) {
    tinydhcpd::LOG_FATAL(e.what());
    std::exit(EXIT_FAILURE);
  }
  tinydhcpd::LOG_INFO("Finished");
  return 0;
}
