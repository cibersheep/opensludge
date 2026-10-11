cmake_minimum_required(VERSION 3.24)

project(OpenSLUDGE VERSION 2.3.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SLUDGE_GLES2 "Render with OpenGL ES 2.0 instead of desktop OpenGL" OFF)
option(SLUDGE_MOVIES "Support playing WebM movies (needs libvpx)" ON)
option(SLUDGE_BUNDLED_DEPS "Build all dependencies from source and link them statically" OFF)

include(cmake/Dependencies.cmake)

set(ENGINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source/Engine)
set(COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source/CommonCode)

set(ENGINE_SOURCES
	backdrop.cpp bg_effects.cpp builtin.cpp cursors.cpp debug.cpp
	fileset.cpp floor.cpp fonttext.cpp freeze.cpp graphics.cpp helpers.cpp
	language.cpp line.cpp loadsave.cpp main.cpp
	memwatch.cpp moreio.cpp newfatal.cpp objtypes.cpp people.cpp
	region.cpp savedata.cpp shaders.cpp sludge_gl.cpp sludger.cpp sprbanks.cpp
	sprites.cpp statusba.cpp stringy.cpp talk.cpp
	thumbnail.cpp timing.cpp transition.cpp variable.cpp
	zbuffer.cpp sound_sdl.cpp
)
if(SLUDGE_MOVIES)
	list(APPEND ENGINE_SOURCES movie.cpp libwebm/mkvparser.cpp libwebm/mkvreader.cpp)
else()
	list(APPEND ENGINE_SOURCES movie_none.cpp)
endif()
list(TRANSFORM ENGINE_SOURCES PREPEND ${ENGINE_DIR}/)
list(APPEND ENGINE_SOURCES ${COMMON_DIR}/utf8.cpp)
list(APPEND ENGINE_SOURCES ${ENGINE_DIR}/posixstuff.cpp)

set(SHADERS
	fixScaleSprite.frag fixScaleSprite.vert
	scale.frag scale_noaa.frag scale.vert
	texture.frag texture.vert
	color.frag color.vert
	yuv.frag yuv.vert
)
list(TRANSFORM SHADERS PREPEND ${ENGINE_DIR}/)

# We build the shaders into the executable.
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
add_custom_command(
	OUTPUT ${GENERATED_DIR}/shaders_embedded.h
	COMMAND ${CMAKE_COMMAND} -DOUTPUT=${GENERATED_DIR}/shaders_embedded.h "-DSHADERS=${SHADERS}"
		-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedShaders.cmake
	DEPENDS ${SHADERS} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedShaders.cmake
	COMMENT "Embedding shaders"
	VERBATIM)

add_executable(sludge-engine ${ENGINE_SOURCES} ${GENERATED_DIR}/shaders_embedded.h)

target_include_directories(sludge-engine PRIVATE ${GENERATED_DIR} ${COMMON_DIR} ${ENGINE_DIR}/libwebm)
target_link_libraries(sludge-engine PRIVATE
	sludge::sdl3 sludge::vorbis sludge::png sludge::dumb)

include(GNUInstallDirs)

if(SLUDGE_GLES2)
	target_compile_definitions(sludge-engine PRIVATE HAVE_GLES2)
	target_link_libraries(sludge-engine PRIVATE GLESv2)
else()
	find_package(OpenGL REQUIRED)
	target_link_libraries(sludge-engine PRIVATE OpenGL::GL)
endif()

if(SLUDGE_MOVIES)
	target_link_libraries(sludge-engine PRIVATE sludge::vpx)
	if(TARGET libvpx)
		add_dependencies(sludge-engine libvpx)
	endif()
endif()

# On Linux, iconv is part of the C library. On macOS, it is a separate
# library.
include(CheckFunctionExists)
check_function_exists(iconv_open HAVE_ICONV_IN_LIBC)
if(NOT HAVE_ICONV_IN_LIBC)
	find_package(Iconv REQUIRED)
	target_link_libraries(sludge-engine PRIVATE Iconv::Iconv)
endif()

install(TARGETS sludge-engine RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
