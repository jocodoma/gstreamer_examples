cmake_minimum_required (VERSION 2.8)
project (gstMain)

string(ASCII 27 Esc)
set(RED "${Esc}[1;31m")
set(GREEN "${Esc}[1;32m")
set(RESET_COLOR "${Esc}[m")

# find_package(PkgConfig)
# if(PkgConfig_FOUND)
# 	# pkg_check_modules(GST REQUIRED gstreamer-0.10>=0.10.36)
# 	pkg_check_modules(GST QUIET gstreamer-0.10>=0.10.36)
# else()
# 	message(FATAL_ERROR "${RED}PkgConfig NOT found!${RESET_COLOR}")
# endif()

# if(GST_FOUND)
#     message(STATUS "${GREEN}Gstreamer-0.10 is found!${RESET_COLOR}")
#     message(STATUS "GST_INC:    ${GST_INCLUDE_DIRS}")
#     message(STATUS "GST_LIBS:   ${GST_LIBRARIES}")
#     message(STATUS "GST_CFLAGS: ${GST_CFLAGS}")
# else()
#     message(FATAL_ERROR "${RED}Gstreamer-0.10 is NOT found!${RESET_COLOR}")
# endif()

set(GST_LIBS "-pthread -lgstapp-0.10 -lgstbase-0.10 -lgstreamer-0.10 -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lxml2 -lglib-2.0")
set(CMAKE_C_FLAGS "-pthread -I/usr/include/gstreamer-0.10 -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/libxml2")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O2")

add_executable(gstMain gstMain.c)
target_link_libraries (gstMain ${GST_LIBS})
