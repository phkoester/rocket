/*
 * comm-local-client.cc
 */

#include "rocket/Process.h"
#include "rocket/cl/cl.h"
#include "rocket/math/random/random.h"

#include <boost/asio.hpp>

#include <filesystem>

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
  u16 port;
};

// Local functions ------------------------------------------------------------------------------------------

i32
run(const ParsedCommandLine& pcl) {
  try {
    asio::io_context io;
    local::socket socket(io);
    fs::path path = fs::temp_directory_path() / fmt::format("comm-local-{}.sock", pcl.port);
    socket.connect(local::endpoint(path.string()));

    auto gen = math::random::gen();
    const string code = math::random::hex(gen, 8);
    nio::out.println("Sending random code {:?} to {} ...", code, path);
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
  const cl::CommandLineConfig config { .usages={ "OPTION..." } };
  cl::CommandLine cl({
    cl::Option::help(&general, pcl.help),
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
