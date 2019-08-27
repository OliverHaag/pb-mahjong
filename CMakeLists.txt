project(pb-mahjong)
cmake_minimum_required(VERSION 2.6.0)

get_filename_component(TOOLCHAIN_DIR ${CMAKE_TOOLCHAIN_FILE} DIRECTORY)
get_filename_component(TOOLCHAIN_DIR ${TOOLCHAIN_DIR} DIRECTORY)
get_filename_component(TOOLCHAIN_DIR ${TOOLCHAIN_DIR} DIRECTORY)

find_package(Freetype REQUIRED)
find_package(Inkview REQUIRED)
set(Python_ADDITIONAL_VERSIONS 3)
find_package(PythonInterp REQUIRED)

file(GLOB CHIP_SVG_FILES ${CMAKE_SOURCE_DIR}/images/chip_*.svg)
foreach(CHIP_SVG_FILE ${CHIP_SVG_FILES})
	get_filename_component(CHIP_BMP_FILE ${CHIP_SVG_FILE} NAME_WE)
	set(CHIP_BMP_FILE "${CMAKE_SOURCE_DIR}/images/${CHIP_BMP_FILE}.bmp")
	list(APPEND CHIP_BMP_FILES ${CHIP_BMP_FILE})
	add_custom_command(
		OUTPUT ${CHIP_BMP_FILE}
		COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/images/convert_chip.py ${CHIP_SVG_FILE} ${CHIP_BMP_FILE}
		DEPENDS ${CMAKE_SOURCE_DIR}/images/convert_chip.py ${CHIP_SVG_FILE})
endforeach()

add_custom_command(
	OUTPUT ${CMAKE_SOURCE_DIR}/images/background.c
	COMMAND ${TOOLCHAIN_DIR}/usr/bin/pbres -c ${CMAKE_SOURCE_DIR}/images/background.c -2 ${CMAKE_SOURCE_DIR}/images/background.bmp
	DEPENDS ${CMAKE_SOURCE_DIR}/images/background.bmp)
add_custom_command(
	OUTPUT ${CMAKE_SOURCE_DIR}/images/chip.c
	COMMAND ${TOOLCHAIN_DIR}/usr/bin/pbres -c ${CMAKE_SOURCE_DIR}/images/chip.c -4 ${CMAKE_SOURCE_DIR}/images/chip_*.bmp
	DEPENDS ${CHIP_BMP_FILES})
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

include_directories(${CMAKE_SOURCE_DIR}/src ${FREETYPE_INCLUDE_DIRS} ${INKVIEW_INCLUDE_DIR})
target_link_libraries(pb-mahjong.app ${FREETYPE_LIBRARIES} ${INKVIEW_LIBRARIES})

if(NOT DEFINED INSTALL_DIR)
	set(INSTALL_DIR ${CMAKE_SOURCE_DIR}/install)
endif()
install(TARGETS pb-mahjong.app DESTINATION ${INSTALL_DIR}/applications)
file(GLOB MAP_FILES ${CMAKE_SOURCE_DIR}/maps/*.map)
install(FILES ${MAP_FILES} DESTINATION ${INSTALL_DIR}/system/config/pb-mahjong)
install(FILES LICENSE.md README.md DESTINATION ${INSTALL_DIR})
if(DEFINED REMOTE_INSTALL_DIR)
	if("${REMOTE_INSTALL_TOOL}" STREQUAL "rsync")
		message("Remote installation to ${REMOTE_INSTALL_DIR} using rsync")
		install(CODE "execute_process(COMMAND /usr/bin/rsync -hrltv ${INSTALL_DIR}/applications ${INSTALL_DIR}/system ${REMOTE_INSTALL_DIR})")
	else()
		message("Remote installation to ${REMOTE_INSTALL_DIR} using scp")
		install(CODE "execute_process(COMMAND /usr/bin/scp -r ${INSTALL_DIR}/applications ${INSTALL_DIR}/system ${REMOTE_INSTALL_DIR})")
	endif()
else()
	message("Local installation to ${INSTALL_DIR}")
endif()

execute_process(COMMAND /bin/sh -c "git tag -l --points-at HEAD | sed -n 's/^v\\([0-9]\\+\\.[0-9]\\+\\.[0-9]\\+\\)/\\1/p' | tail -n1 | tr -d '\n'" OUTPUT_VARIABLE GIT_VERSION)
if(NOT "${GIT_VERSION}" STREQUAL "")
	message("Version ${GIT_VERSION}")
	add_custom_target(
		buildpackage
		BYPRODUCTS ${CMAKE_SOURCE_DIR}/pb-mahjong-${GIT_VERSION}.tar.xz
		COMMAND XZ_OPT=-9 tar -C "${INSTALL_DIR}" --lzma -cf "${CMAKE_SOURCE_DIR}/pb-mahjong-${GIT_VERSION}.tar.xz" applications system LICENSE.md README.md)
endif()
