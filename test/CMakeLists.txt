# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(sawdTests LANGUAGES CXX)

# ---- Dependencies ----

find_package(Catch2 REQUIRED)
include(Catch)

# ---- Tests ----

add_executable(sawd_test source/sawd_test.cpp)
target_link_libraries(
    sawd_test PRIVATE
    sawd_lib
    Catch2::Catch2WithMain
)
target_compile_features(sawd_test PRIVATE cxx_std_20)

catch_discover_tests(sawd_test)

# ---- End-of-file commands ----

add_folders(Test)
