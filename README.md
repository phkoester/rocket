# Rocket

Rocket is an experimental all-purpose library written in C++.

## Environment Variables

| Name                   | Type     | Description
| :--------------------- | :------- | :----------
| `ROCKET_EXIT`          | `bool`   | If set to `1`, `std::exit` is called rather than `std::quick_exit`.
| `ROCKET_LOG_FMT`       | `string` | Default log format.
| `ROCKET_QUICK_EXIT`    | `bool`   | If set to `1`, `std::quick_exit` is called rather than `std::exit`.
| `ROCKET_TEST_TERMINAL` | `bool`   | If set to `1`, some unit tests and benches may perform additional terminal tests and produce some extra terminal output.
