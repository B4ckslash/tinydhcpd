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
#include "string-format.hpp"

#ifdef HAVE_SYSTEMD
#include "log/systemd_logsink.hpp"
#endif

#ifndef PROGRAM_VERSION
#define PROGRAM_VERSION "debug"
#endif

std::unique_ptr<tinydhcpd::LogSink> tinydhcpd::global_sink;

const char ADDRESS_TAG = 'a';
const char IFACE_TAG = 'i';
const char CONFIG_FILE_TAG = 'c';

const struct option long_options[] = {
    {"address", required_argument, nullptr, ADDRESS_TAG},
    {"interface", required_argument, nullptr, IFACE_TAG},
    {"configfile", required_argument, nullptr, CONFIG_FILE_TAG},
    {nullptr, 0, nullptr, 0}};

void sighandler(int signum) {
  LOG_TRACE("Caught signal " + std::to_string(signum));
  tinydhcpd::last_signal = signum;
}

int main(int argc, char *const argv[]) {
  std::signal(SIGINT, sighandler);
  std::signal(SIGTERM, sighandler);
  std::signal(SIGHUP, sighandler);

#ifdef HAVE_SYSTEMD
  tinydhcpd::global_sink.reset(new tinydhcpd::SystemdLogSink(std::cerr));
#else
  tinydhcpd::global_sink.reset(new tinydhcpd::StdoutLogsink());
#endif

  LOG_INFO(
      tinydhcpd::string_format("tinydhcpd %s starting...", PROGRAM_VERSION));

  tinydhcpd::ProgramConfiguration optval = {
      .address = {.s_addr = INADDR_ANY},
      .interface = "",
      .confpath = "/etc/tinydhcpd/tinydhcpd.conf",
      .lease_file_path = "/var/lib/tinydhcpd/leases",
      .subnet_config = {}};

  int opt;
  while ((opt = getopt_long(argc, argv, "a:i:c:d:", long_options, nullptr)) !=
         -1) {
    switch (opt) {
    case ADDRESS_TAG:
      LOG_INFO(std::string("Address from cmdline: ").append(optarg));
      inet_aton(optarg, &(optval.address));
      break;

    case IFACE_TAG:
      LOG_INFO(std::string("Interface from cmdline: ").append(optarg));
      optval.interface = optarg;
      break;

    case CONFIG_FILE_TAG:
      LOG_INFO(std::string("Using config file: ").append(optarg));
      optval.confpath = optarg;
      break;

    default:
      break;
    }
  }

  try {
    tinydhcpd::parse_configuration(optval);
  } catch (libconfig::ParseException &pex) {
    LOG_FATAL(tinydhcpd::string_format(
        "Failed to parse config file %s at line %d! Error: %s", pex.getFile(),
        pex.getLine(), pex.getError()));
    exit(EXIT_FAILURE);
  } catch (libconfig::FileIOException &fex) {
    std::ostringstream os;
    os << "Error reading file " << optval.confpath << ".";
    if (!std::filesystem::exists(optval.confpath)) {
      os << " The file does not exist.";
    }
    LOG_FATAL(os.str());
    exit(EXIT_FAILURE);
  } catch (std::invalid_argument &ex) {
    LOG_FATAL(ex.what());
  }

  tinydhcpd::Daemon daemon(optval.address, optval.interface,
                           optval.subnet_config, optval.lease_file_path);
  LOG_INFO("Initialization finished");
  daemon.main_loop();
  daemon.write_leases();
  LOG_INFO("Finished");
  return 0;
}
