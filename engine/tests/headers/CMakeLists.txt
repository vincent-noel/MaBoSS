# ---------------------------------------------------------------------------
# Header self-containment check
#
# Every engine header must compile on its own, without relying on whatever a
# .cc file happened to include before it. For each header this generates a
# one-line translation unit that includes only that header, and compiles it
# with the library's own flags (MAXNODES, bitset variant, SBML, MPI, ...), so
# each test configuration checks the headers the way it builds them.
#
# Nothing is linked or run: a header that is not self-contained fails the
# build of the tests themselves.
# ---------------------------------------------------------------------------

set(MABOSS_HEADER_ROOT ${PROJECT_SOURCE_DIR}/engine/src)

# In a git checkout, only tracked headers are checked, so that untracked
# work-in-progress files in the source tree do not break the build (the list
# is then refreshed when CMake reconfigures). Elsewhere -- release tarballs,
# conda builds -- every header on disk is.
set(MABOSS_HEADERS)
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS ${PROJECT_SOURCE_DIR}/.git)
  execute_process(COMMAND ${GIT_EXECUTABLE} ls-files -- "*.h"
                  WORKING_DIRECTORY ${MABOSS_HEADER_ROOT}
                  OUTPUT_VARIABLE tracked_headers
                  RESULT_VARIABLE git_result
                  OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
  if(git_result EQUAL 0 AND tracked_headers)
    string(REPLACE "\n" ";" MABOSS_HEADERS "${tracked_headers}")
  endif()
endif()
if(NOT MABOSS_HEADERS)
  file(GLOB_RECURSE MABOSS_HEADERS RELATIVE ${MABOSS_HEADER_ROOT} CONFIGURE_DEPENDS
       ${MABOSS_HEADER_ROOT}/*.h)
endif()

# The SBML and SED-ML headers need those libraries' headers.
if(NOT SBML)
  list(FILTER MABOSS_HEADERS EXCLUDE REGEX "^sbml/")
endif()
if(NOT SEDML)
  list(FILTER MABOSS_HEADERS EXCLUDE REGEX "^sedml/")
endif()

set(MABOSS_HEADER_CHECK_SOURCES)
foreach(header ${MABOSS_HEADERS})
  string(MAKE_C_IDENTIFIER ${header} stem)
  set(source ${CMAKE_CURRENT_BINARY_DIR}/${stem}.cc)
  file(GENERATE OUTPUT ${source} CONTENT "#include \"${header}\"\n")
  list(APPEND MABOSS_HEADER_CHECK_SOURCES ${source})
endforeach()

add_library(maboss_header_check OBJECT ${MABOSS_HEADER_CHECK_SOURCES})
target_link_libraries(maboss_header_check PRIVATE ${MABOSS_LIBNAME}-static)
