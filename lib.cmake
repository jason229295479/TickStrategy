file(GLOB SOURCE
    "./src/*.cpp" 		
)
file(GLOB SOURCE_C
    "./src/*.c" 		
)
file(GLOB SOURCE_CC
	"./src/*.cc"
)
file(GLOB HEADER
    "./include/*.h"
    "./include/*.hpp"
)

INCLUDE(../func.cmake)
build_file_tree()
init_include_path()

INCLUDE_DIRECTORIES(${BASICLIBS_INCLUDE_PATH} ${PROJECT_INCLUDE_PATH})
SET(LIBRARY_OUTPUT_PATH  /home/jason/df_quant_md/mdServer/GlobalObjData/lib)

#ADD_LIBRARY(${PROJECT_NAME} STATIC ${HEADER} ${SOURCE} ${SOURCE_C} ${SOURCE_CC})
ADD_LIBRARY(${PROJECT_NAME} SHARED ${HEADER} ${SOURCE} ${SOURCE_C} ${SOURCE_CC})

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
