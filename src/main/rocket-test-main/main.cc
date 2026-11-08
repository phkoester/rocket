/*
 * main.cc
 */

#include <rocket/Process.h>
#include <rocket/cl/cl.h>

#include <gmock/gmock.h>

#include <cstdlib>

#ifdef ROCKET_OS_WINDOWS
  #include <windows.h>
  #include <crtdbg.h>
#endif

using namespace rocket;
using namespace rocket::unicode;
using namespace std;

namespace {

// Local functions ------------------------------------------------------------------------------------------

#ifdef ROCKET_OS_WINDOWS

// Suppresses the CRT and Windows Error Reporting dialogs (e.g., "abort() has been called" in Debug builds).
// Must be done before anything else, as death-test children re-run this executable
void
suppressErrorDialogs() {
  _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
  for (const int type : { _CRT_WARN, _CRT_ERROR, _CRT_ASSERT }) {
    _CrtSetReportMode(type, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(type, _CRTDBG_FILE_STDERR);
  }
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
  _set_error_mode(_OUT_TO_STDERR);
}

#endif // ROCKET_OS_WINDOWS

} // namespace

// `main` ---------------------------------------------------------------------------------------------------

i32
main(i32 argc, char** argv) {
#ifdef ROCKET_OS_WINDOWS
  suppressErrorDialogs();
#endif

  testing::InitGoogleMock(&argc, argv);

  // Use the suggested code locale from the process
  auto testLocale = process().codeLocale();
  // GoogleTest formats numbers in internal flags (e.g., `--gtest_internal_run_death_test`, which includes
  // the parent PID) using the global locale. Digit grouping (`3,512`) breaks parsing in death-test children,
  // so use the classic numeric facet
  testLocale = locale(testLocale, locale::classic(), locale::numeric);

  process().init(argc, argv, nullopt, testLocale);

  optional<bool> help;
  const cl::OptionGroup general("General control");
  const cl::CommandLineConfig config { .usages={ "[OPTION]..." }, .otherOutput=true };
  cl::CommandLine cl({
    cl::Option::help(&general, help)
  }, {}, config);
  cl.parse(process().args());

  GTEST_FLAG_SET(death_test_style, "threadsafe");
  const i32 status = RUN_ALL_TESTS();

  process().exit(status);
}

// EOF
