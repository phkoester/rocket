/*
 * comm-tcp-client.cc
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

// #ParsedCommandLine ---------------------------------------------------------------------------------------

struct ParsedCommandLine {
  optional<bool> help;
  optional<string> host = "localhost";
  u16 port = 0;
  string size;
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

    {
      const u64 size = comm::Message::parseSize(pcl.size);
      comm::Message request(size);
      nio::out.println(
        "Sending request {:?} ({} bytes) to {}:{} ...",
        request.display(), request.size(), pcl.host.value(), pcl.port);
      request.payload().push_back('\n');
      asio::write(socket, asio::buffer(request.payload()));
    }

    asio::streambuf buffer;
    boost::system::error_code ec;
    asio::read_until(socket, buffer, '\n', ec);
    if (ec && ec != asio::error::eof) {
      throw boost::system::system_error(ec);
    }
    istream is(&buffer);
    comm::Message reply;
    getline(is, reply.payload());
    nio::out.println("Received reply: {:?} ({} bytes)", reply.display(), reply.size());
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
  const cl::CommandLineConfig config { .usages={ "OPTION... SIZE" } };
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
  }, {
    cl::Parameter::make({
      .description="the message size",
      .name="SIZE"
    }, pcl.size)
  }, config);

  cl.parse(process.args());
  const i32 exitCode = run(pcl);
  nio::out.println("Exiting ({}) ...", exitCode);
  process.exit(exitCode);
}

// EOF
