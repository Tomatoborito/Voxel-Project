# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

if(EXISTS "E:/.codingshit/traktorprojekt - Copy/_deps/highway-subbuild/highway-populate-prefix/src/highway-populate-stamp/highway-populate-gitclone-lastrun.txt" AND EXISTS "E:/.codingshit/traktorprojekt - Copy/_deps/highway-subbuild/highway-populate-prefix/src/highway-populate-stamp/highway-populate-gitinfo.txt" AND
  "E:/.codingshit/traktorprojekt - Copy/_deps/highway-subbuild/highway-populate-prefix/src/highway-populate-stamp/highway-populate-gitclone-lastrun.txt" IS_NEWER_THAN "E:/.codingshit/traktorprojekt - Copy/_deps/highway-subbuild/highway-populate-prefix/src/highway-populate-stamp/highway-populate-gitinfo.txt")
  message(VERBOSE
    "Avoiding repeated git clone, stamp file is up to date: "
    "'E:/.codingshit/traktorprojekt - Copy/_deps/highway-subbuild/highway-populate-prefix/src/highway-populate-stamp/highway-populate-gitclone-lastrun.txt'"
  )
  return()
endif()

# Even at VERBOSE level, we don't want to see the commands executed, but
# enabling them to be shown for DEBUG may be useful to help diagnose problems.
cmake_language(GET_MESSAGE_LOG_LEVEL active_log_level)
if(active_log_level MATCHES "DEBUG|TRACE")
  set(maybe_show_command COMMAND_ECHO STDOUT)
else()
  set(maybe_show_command "")
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E rm -rf "E:/.codingshit/traktorprojekt - Copy/_deps/highway-src"
  RESULT_VARIABLE error_code
  ${maybe_show_command}
)
if(error_code)
  message(FATAL_ERROR "Failed to remove directory: 'E:/.codingshit/traktorprojekt - Copy/_deps/highway-src'")
endif()

# try the clone 3 times in case there is an odd git clone issue
set(error_code 1)
set(number_of_tries 0)
while(error_code AND number_of_tries LESS 3)
  execute_process(
    COMMAND "E:/appsunds/Git/cmd/git.exe"
            clone --no-checkout --config "advice.detachedHead=false" "https://github.com/google/highway.git" "highway-src"
    WORKING_DIRECTORY "E:/.codingshit/traktorprojekt - Copy/_deps"
    RESULT_VARIABLE error_code
    ${maybe_show_command}
  )
  math(EXPR number_of_tries "${number_of_tries} + 1")
endwhile()
if(number_of_tries GREATER 1)
  message(NOTICE "Had to git clone more than once: ${number_of_tries} times.")
endif()
if(error_code)
  message(FATAL_ERROR "Failed to clone repository: 'https://github.com/google/highway.git'")
endif()

execute_process(
  COMMAND "E:/appsunds/Git/cmd/git.exe"
          checkout "master" --
  WORKING_DIRECTORY "E:/.codingshit/traktorprojekt - Copy/_deps/highway-src"
  RESULT_VARIABLE error_code
  ${maybe_show_command}
)
if(error_code)
  message(FATAL_ERROR "Failed to checkout tag: 'master'")
endif()

set(init_submodules TRUE)
if(init_submodules)
  execute_process(
    COMMAND "E:/appsunds/Git/cmd/git.exe" 
            submodule update --recursive --init 
    WORKING_DIRECTORY "E:/.codingshit/traktorprojekt - Copy/_deps/highway-src"
    RESULT_VARIABLE error_code
    ${maybe_show_command}
  )
endif()
if(error_code)
  message(FATAL_ERROR "Failed to update submodules in: 'E:/.codingshit/traktorprojekt - Copy/_deps/highway-src'")
endif()

# Complete success, update the script-last-run stamp file:
#
execute_process(
  COMMAND ${CMAKE_COMMAND} -E copy "E:/.codingshit/traktorprojekt - Copy/_deps/highway-subbuild/highway-populate-prefix/src/highway-populate-stamp/highway-populate-gitinfo.txt" "E:/.codingshit/traktorprojekt - Copy/_deps/highway-subbuild/highway-populate-prefix/src/highway-populate-stamp/highway-populate-gitclone-lastrun.txt"
  RESULT_VARIABLE error_code
  ${maybe_show_command}
)
if(error_code)
  message(FATAL_ERROR "Failed to copy script-last-run stamp file: 'E:/.codingshit/traktorprojekt - Copy/_deps/highway-subbuild/highway-populate-prefix/src/highway-populate-stamp/highway-populate-gitclone-lastrun.txt'")
endif()
