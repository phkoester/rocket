/*
 * comm-tcp-client.cc
 */

#include "rocket/Process.h"
#include "rocket/cl/cl.h"
#include "rocket/math/random/random.h"

#include <boost/asio.hpp>

namespace asio = boost::asio;

using namespace rocket;
using namespace rocket::unicode;
using namespace std;

using tcp = asio::ip::tcp;

namespace {

// #ParsedCommandLine ---------------------------------------------------------------------------------------

struct ParsedCommandLine {
  optional<bool> help;
  optional<string> host = "localhost";
  u16 port;
};

// Local functions ------------------------------------------------------------------------------------------

i32
run(const ParsedCommandLine& pcl) {
  try {
    asio::io_context io;
    tcp::resolver resolver(io);
    auto endpoints = resolver.resolve(pcl.host.value(), to_string(pcl.port));
    tcp::socket socket(io);
    asio::connect(socket, endpoints);

    auto gen = math::random::gen();
    const string code = math::random::hex(gen, 8);
    nio::out.println("Sending random code {:?} to {}:{} ...", code, pcl.host.value(), pcl.port);
    const string payload = code + "\n";
    asio::write(socket, asio::buffer(payload));
    asio::streambuf buffer;
    boost::system::error_code ec;
    asio::read_until(socket, buffer, '\n', ec);
    if (ec && ec != asio::error::eof) {
      throw boost::system::system_error(ec);
    }
    istream is(&buffer);
    string reply;
    getline(is, reply);
    nio::out.println("Received reply: {:?}", reply);
    boost::system::error_code ignored;
    socket.shutdown(tcp::socket::shutdown_both, ignored); // NOLINT
  } catch (const exception& ex) {
    process.error(nio::err, EXIT_SERIOUS_FAILURE, "{}", what(ex));
  }

  return EXIT_SUCCESS;
}

} // namespace

// #main ----------------------------------------------------------------------------------------------------

i32
main(i32 argc, char **argv) {
  process.init(argc, argv, "comm-tcp-client");

  ParsedCommandLine pcl;

  const cl::OptionGroup general("General control");
  const cl::CommandLineConfig config { .usages={ "OPTION..." } };
  cl::CommandLine cl({
    cl::Option::help(&general, pcl.help),
    cl::Option::custom({
      .description="the host to connect to",
      .group=&general,
      .name="host",
      .shortName="h"_c
    }, pcl.host),
    cl::Option::custom({
      .description="a port number",
      .group=&general,
      .name="port",
      .shortName="p"_c
    }, pcl.port),
  }, {}, config);

  cl.parse(process.args());
  const i32 exitCode = run(pcl);
  nio::out.println("Exiting ({}) ...", exitCode);
  process.exit(exitCode);
}

// EOF
