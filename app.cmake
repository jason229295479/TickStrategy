file(GLOB SOURCE
    "./src/*.cpp"
)
file(GLOB SOURCE_CC
	"./src/*.cc*")
file(GLOB SOURCE_C
    "./src/*.c"
)
file(GLOB SOURCE_INTER_C
    "./inter/*.c"
)
file(GLOB SOURCE_INTER_H
    "./inter/*.H"
)
file(GLOB HEAD
    "./include/*.h"
    "./include/*.hpp"
)

INCLUDE_DIRECTORIES(SYSTEM

${includePath_rapidjson}
${includePath_libpqxx}


${includePath_CTPAPI}

${includePath_boost}
${includePath_Curl}
${includePath_Json}

${includePath_CommUtil}
)

INCLUDE(../func.cmake)
build_file_tree()
init_include_path()

SET(EXECUTABLE_OUTPUT_PATH ../../apps/)
add_executable(${PROJECT_NAME} ${SOURCE_TREE} ${HEAD} ${SOURCE})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} 
	libjsoncpp.so 
	libcurl.so  
	libpq.so
	libpqxx.so
    libpthread.so
	)
target_link_libraries(${PROJECT_NAME})

