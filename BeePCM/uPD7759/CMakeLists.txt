set(UPD7759_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(UPD7759_SOURCES
	upd7759.cpp)

set(UPD7759_HEADERS
	upd7759.h)

add_library(upd7759 STATIC ${UPD7759_SOURCES} ${UPD7759_HEADERS})
target_include_directories(upd7759 PUBLIC
	${UPD7759_INCLUDE_DIR})