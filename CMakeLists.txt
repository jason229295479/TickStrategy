cmake_minimum_required(VERSION 3.0)

project(TickStrategyProject)




#设置工程包含当前目录，非必须
SET(CMAKE_C_COMPILER "g++")
set(cmake_include_current_dir on)
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -march=native -O3 -pthread " )
set(CMAKE_BUILD_TYPE "Release" )





#设置头文件路径


set(includePath_rapidjson  		/home/jason/ProjectMarketService/third_party/rapidjson/include)
set(includePath_libpqxx			/usr/include)

set(includePath_CTPAPI  		/home/jason/ProjectMarketService/third_party/v6.3.15_20190220_api_tradeapi_se_linux64)
                                

set(includePath_boost 			/usr/include)
set(includePath_Curl  			/usr/include/x86_64-linux-gnu)
set(includePath_Json  			/usr/include/jsoncpp)


set(includePath_CommUtil 		/home/jason/ProjectMarketService/third_party/CommUtil/include)


INCLUDE_DIRECTORIES(SYSTEM

${includePath_rapidjson}
${includePath_libpqxx}


${includePath_CTPAPI}

${includePath_boost}
${includePath_Curl}
${includePath_Json}

${includePath_CommUtil}

)


#设置库文件路径
set(lib_boost_path 		/usr/lib)
set(lib_Json_path 		/usr/lib/x86_64-linux-gnu)
set(lib_Curl_path 		/usr/lib/x86_64-linux-gnu)
set(lib_libpqxx_path 	/usr/lib/x86_64-linux-gnu)


set(lib_CTPAPI_path 	/home/jason/ProjectMarketService/third_party/v6.3.15_20190220_api_tradeapi_se_linux64)


link_directories(
${lib_boost_path}

${lib_CTPAPI_path}
${lib_Json_path}
${lib_Curl_path}

)
add_definitions(
-DUNICODE
-D_CRT_SECURE_NO_DEPRECATE
-D_CRT_NON_CONFORMING_SWPRINTFS
-D_SCL_SECURE_NO_WARNINGS
-DWIN32_LEAN_AND_MEAN
)

add_subdirectory(tick_strategy)

