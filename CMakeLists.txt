project(pb-mahjong)
cmake_minimum_required(VERSION 2.6.0)

set(TOOLCHAIN_PATH ${CMAKE_FIND_ROOT_PATH}/../..)

find_package(Freetype REQUIRED)

add_custom_command(
	OUTPUT ${CMAKE_SOURCE_DIR}/images/background.c
	COMMAND ${TOOLCHAIN_PATH}/bin/pbres -c ${CMAKE_SOURCE_DIR}/images/background.c -2 ${CMAKE_SOURCE_DIR}/images/background.bmp)
add_custom_command(
	OUTPUT ${CMAKE_SOURCE_DIR}/images/chip.c
	COMMAND ${TOOLCHAIN_PATH}/bin/pbres -c ${CMAKE_SOURCE_DIR}/images/chip.c -4 ${CMAKE_SOURCE_DIR}/images/chip_*.bmp)

add_executable(pb-mahjong.app
	${CMAKE_SOURCE_DIR}/src/bitmaps.c
	${CMAKE_SOURCE_DIR}/src/board.c
	${CMAKE_SOURCE_DIR}/src/common.c
	${CMAKE_SOURCE_DIR}/src/geometry.c
	${CMAKE_SOURCE_DIR}/src/main.c
	${CMAKE_SOURCE_DIR}/src/maps.c
	${CMAKE_SOURCE_DIR}/src/menu.c
	${CMAKE_SOURCE_DIR}/src/messages.c
	${CMAKE_SOURCE_DIR}/images/background.c
	${CMAKE_SOURCE_DIR}/images/chip.c)

include_directories(${CMAKE_SOURCE_DIR}/src ${FREETYPE_INCLUDE_DIRS})
target_link_libraries(pb-mahjong.app pthread inkview freetype)

install(TARGETS pb-mahjong.app DESTINATION bin)
