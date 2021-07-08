set (PROJECT_NAME tick_strategy)

set (Demo_VERSION_MAJOR 1)
set (Demo_VERSION_MINOR 0)


# 加入一个配置头文件，用于处理 CMake 对源码的设置
configure_file (
  "config.h.in"
  "config.h"
  )


file(GLOB_RECURSE HEADER include/*.h include/*.hpp)
file(GLOB_RECURSE SOURCE src/*.cpp)

link_directories(
${lib_CTPAPI_path}
${includePath_Curl}

)
INCLUDE_DIRECTORIES(SYSTEM
${includePath_Curl}
${includePath_Json}
${includePath_libpqxx}
${includePath_CTPAPI}
)
add_definitions(
-DBUILDING_LIBCURL
)

INCLUDE(../app.cmake)

target_link_libraries(${PROJECT_NAME}  libthostmduserapi_se.so libthosttraderapi_se.so )

