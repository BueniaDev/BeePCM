set(SEGAPCM_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(SEGAPCM_SOURCES
	segapcm.cpp)

set(SEGAPCM_HEADERS
	segapcm.h)

add_library(segapcm STATIC ${SEGAPCM_SOURCES} ${SEGAPCM_HEADERS})
target_include_directories(segapcm PUBLIC
	${SEGAPCM_INCLUDE_DIR})