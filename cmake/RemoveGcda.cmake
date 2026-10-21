# Delete gcov counters below BINARY_DIR so each coverage run measures only its own test execution.
# Usage: cmake -DBINARY_DIR=<dir> -P RemoveGcda.cmake
file(GLOB_RECURSE gcda_files "${BINARY_DIR}/*.gcda")
if(gcda_files)
    file(REMOVE ${gcda_files})
endif()
