set(MULTIPCM_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(MULTIPCM_SOURCES
	multipcm.cpp)

set(MULTIPCM_HEADERS
	multipcm.h)

add_library(multipcm STATIC ${MULTIPCM_SOURCES} ${MULTIPCM_HEADERS})
target_include_directories(multipcm PUBLIC
	${MULTIPCM_INCLUDE_DIR})