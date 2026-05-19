/*
 * comm-local-client.cc
 */

#include "Message.h"

#include "rocket/Process.h"
#include "rocket/cl/cl.h"
#include "rocket/filesystem/filesystem.h"

#include <boost/asio.hpp>

namespace asio = boost::asio;

using namespace rocket;
using namespace rocket::unicode;
using namespace std;

using local = asio::local::stream_protocol;

namespace {

// #ParsedCommandLine ---------------------------------------------------------------------------------------

struct ParsedCommandLine {
  optional<bool> help;
  u16 port;
  string size;
};

// Local functions ------------------------------------------------------------------------------------------

i32
run(const ParsedCommandLine& pcl) {
  try {
    asio::io_context io;
    local::socket socket(io);
    const auto path = rocket::filesystem::systemTempDir() / fmt::format("comm-local-{}.sock", pcl.port);
    socket.connect(local::endpoint(path.string()));

    {
      u64 size = comm::Message::parseSize(pcl.size);
      comm::Message request(size);
      nio::out.println(
        "Sending request {:?} ({} bytes) to {} ...",
        request.display(), request.size(), path);
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
    socket.shutdown(local::socket::shutdown_both, ignored); // NOLINT
  } catch (const exception& ex) {
    process.error(nio::err, EXIT_SERIOUS_FAILURE, "{}", what(ex));
  }

  return EXIT_SUCCESS;
}

} // namespace

// #main ----------------------------------------------------------------------------------------------------

i32
main(i32 argc, char **argv) {
  process.init(argc, argv, "comm-local-client");

  ParsedCommandLine pcl;

  const cl::OptionGroup general("General control");
  const cl::CommandLineConfig config { .usages={ "OPTION... SIZE" } };
  cl::CommandLine cl({
    cl::Option::help(&general, pcl.help),
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
