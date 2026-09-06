/*
 * comm-tcp-server.cc
 */

#include "Message.h"

#include "rocket/Process.h"
#include "rocket/cl/cl.h"

#include <boost/asio.hpp>

namespace asio = boost::asio;

using namespace rocket;
using namespace rocket::unicode;
using namespace std;

using tcp = asio::ip::tcp;

namespace {

// `ParsedCommandLine` --------------------------------------------------------------------------------------

struct ParsedCommandLine {
  optional<bool> help;
  vector<u16> ports;
};

// `Session` ------------------------------------------------------------------------------------------------

struct Session : enable_shared_from_this<Session> {
  Session(tcp::socket socket, u16 port) :
    socket_(std::move(socket)),
    port_(port) {}

  void start() { read(); }

private:

  tcp::socket socket_;
  u16 port_;
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
          "Received message on port {}: {:?} ({} bytes)",
          self->port_, msg.display(), msg.size());
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
          self->socket_.shutdown(tcp::socket::shutdown_both, ignored); // NOLINT
        }
      });
  }
};

// `Listener` -----------------------------------------------------------------------------------------------

struct Listener : enable_shared_from_this<Listener> {
  Listener(asio::io_context& io, u16 port) :
    acceptor_(io, tcp::endpoint(tcp::v6(), port)),
    port_(port) {}

  void start() { accept(); }

private:

  tcp::acceptor acceptor_;
  u16 port_;

  void accept() {
    auto self = shared_from_this();
    acceptor_.async_accept(
      [self](const boost::system::error_code& ec, tcp::socket socket) {
        if (!ec) {
          make_shared<Session>(std::move(socket), self->port_)->start();
        } else {
          process.error(nio::err, 0, "Accept error on port {}: {}", self->port_, ec.message());
        }
        self->accept();
      });
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
      nio::out.println("Listening on port {} ...", port);
      listeners.push_back(std::move(listener));
    }
    asio::signal_set signals(io, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto) { io.stop(); });
    io.run();
  } catch (const exception& ex) {
    process.error(nio::err, EXIT_SERIOUS_FAILURE, "{}", what(ex));
  }

  return EXIT_SUCCESS;
}

} // namespace

// `main` ---------------------------------------------------------------------------------------------------

i32
main(i32 argc, char **argv) {
  process.init(argc, argv, "comm-tcp-server");

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
