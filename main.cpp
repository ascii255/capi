#include <cstdlib>
#include <print>
#include <string_view>

#include <expect/expect.hpp>

#include <capi/unique_flgd.hpp>
#include <capi/unique_id.hpp>
#include <capi/unique_res.hpp>
#include <capi/unique_sys.hpp>

int main(int argc, char** argv) {
  const auto run_all = [] {
    capi::testing::run_unique_flgd_tests();
    capi::testing::run_unique_id_tests();
    capi::testing::run_unique_res_tests();
    capi::testing::run_unique_sys_tests();
  };

  if (argc < 2) {
    run_all();
    return EXIT_SUCCESS;
  }

  const std::string_view selector { argv[1] };

  if (selector == "unique_id") {
    capi::testing::run_unique_id_tests();
    return EXIT_SUCCESS;
  }

  if (selector == "unique_flgd") {
    capi::testing::run_unique_flgd_tests();
    return EXIT_SUCCESS;
  }

  if (selector == "unique_res") {
    capi::testing::run_unique_res_tests();
    return EXIT_SUCCESS;
  }

  if (selector == "unique_sys") {
    capi::testing::run_unique_sys_tests();
    return EXIT_SUCCESS;
  }

  if (selector == "all") {
    run_all();
    return EXIT_SUCCESS;
  }

  std::println(stderr, "unknown selector: {}", selector);

  return EXIT_FAILURE;
}
