set(OKIM6295_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(OKIM6295_SOURCES
	okim6295.cpp)

set(OKIM6295_HEADERS
	okim6295.h)

add_library(okim6295 STATIC ${OKIM6295_SOURCES} ${OKIM6295_HEADERS})
target_include_directories(okim6295 PUBLIC
	${OKIM6295_INCLUDE_DIR})