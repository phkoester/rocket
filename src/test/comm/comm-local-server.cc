/*
 * comm-local-server.cc
 */

#include "Message.h"

#include "rocket/Process.h"
#include "rocket/cl/cl.h"
#include "rocket/filesystem/filesystem.h"

#include <boost/asio.hpp>

namespace asio = boost::asio;
namespace fs = std::filesystem;

using namespace rocket;
using namespace rocket::unicode;
using namespace std;

using local = asio::local::stream_protocol;

namespace {

// #ParsedCommandLine ---------------------------------------------------------------------------------------

struct ParsedCommandLine {
  optional<bool> help;
  vector<u16> ports;
};

// #Session -------------------------------------------------------------------------------------------------

struct Session : enable_shared_from_this<Session> {
  Session(local::socket socket, fs::path path) :
    socket_(std::move(socket)),
    path_(std::move(path)) {}

  void start() { read(); }

private:

  local::socket socket_;
  fs::path path_;
  asio::streambuf buffer_;

  void read() {
    auto self = shared_from_this();
    asio::async_read_until(
      socket_, buffer_, '\n',
      [self](const boost::system::error_code& ec, u64) {
        if (ec) {
          return;
        }
        comm::Message msg;
        {
          istream is(&self->buffer_);
          getline(is, msg.payload());
        }
        nio::out.println(
          "Received message on {}: {:?} ({} bytes)",
          self->path_, msg.display(), msg.size());
        msg.payload().push_back('\n');
        self->write(msg.payload());
      });
  }

  void write(const string& msg) {
    auto self = shared_from_this();
    auto payload = make_shared<string>(msg);
    asio::async_write(
      socket_, asio::buffer(*payload),
      [self, payload](const boost::system::error_code& ec, u64) {
        if (!ec) {
          boost::system::error_code ignored;
          self->socket_.shutdown(local::socket::shutdown_both, ignored); // NOLINT
        }
      });
  }
};

// #Listener ------------------------------------------------------------------------------------------------

struct Listener : enable_shared_from_this<Listener> {
  Listener(asio::io_context& io, u16 port) :
    acceptor_(io),
    path_(rocket::filesystem::systemTempDir() / fmt::format("comm-local-{}.sock", port)) {
    acceptor_.open();
    acceptor_.bind(local::endpoint(path_.string()));
    acceptor_.listen();
  }

  void close() {
    boost::system::error_code ignored;
    acceptor_.close(ignored); // NOLINT
    remove();
  }

  const fs::path& path() const { return path_; }

  void start() { accept(); }

private:

  local::acceptor acceptor_;
  fs::path path_;

  void accept() {
    auto self = shared_from_this();
    acceptor_.async_accept(
      [self](const boost::system::error_code& ec, local::socket socket) {
        if (!ec) {
          make_shared<Session>(std::move(socket), self->path_)->start();
        } else if (ec != asio::error::operation_aborted) {
          process.error(nio::err, 0, "Accept error on {}: {}", self->path_, ec.message());
        }
        if (self->acceptor_.is_open()) {
          self->accept();
        }
      });
  }

  void remove() {
    std::error_code ignored;
    fs::remove(path_, ignored);
  }
};

// Local functions ------------------------------------------------------------------------------------------

i32
run(const ParsedCommandLine& pcl) {
  try {
    asio::io_context io;
    vector<shared_ptr<Listener>> listeners;
    for (const auto port : pcl.ports) {
      auto listener = make_shared<Listener>(io, port);
      listener->start();
      nio::out.println("Listening on {} ...", listener->path());
      listeners.push_back(std::move(listener));
    }
    asio::signal_set signals(io, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto) {
      for (const auto& listener : listeners) {
        listener->close();
      }
      io.stop();
    });
    io.run();
  } catch (const exception& ex) {
    process.error(nio::err, EXIT_SERIOUS_FAILURE, "{}", what(ex));
  }

  return EXIT_SUCCESS;
}

} // namespace

// #main ----------------------------------------------------------------------------------------------------

i32
main(i32 argc, char **argv) {
  process.init(argc, argv, "comm-local-server");

  ParsedCommandLine pcl;

  const cl::OptionGroup general("General control");
  const cl::CommandLineConfig config { .usages={ "OPTION..." } };
  cl::CommandLine cl({
    cl::Option::help(&general, pcl.help),
    cl::Option::custom({
      .description="a port number",
      .group=&general,
      .name="port",
      .shortName="p"_c
    }, pcl.ports),
  }, {}, config);

  cl.parse(process.args());
  const i32 exitCode = run(pcl);
  nio::out.println("Exiting ({}) ...", exitCode);
  process.exit(exitCode);
}

// EOF
