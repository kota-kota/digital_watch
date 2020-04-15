cmake_minimum_required(VERSION 2.8)

#プロジェクト名
set(PROJECT_NAME "DigitalWatch")
project(${PROJECT_NAME})

enable_language(CXX)
set(CMAKE_CXX_STANDARD 11) # C++14...
set(CMAKE_CXX_STANDARD_REQUIRED ON) #...is required...
set(CMAKE_CXX_EXTENSIONS OFF) #...without compiler extensions like gnu++11

#ソース
set(SOURCES
	${CMAKE_SOURCE_DIR}/source/DWMain.cpp
	${CMAKE_SOURCE_DIR}/source/DWMain.hpp
	${CMAKE_SOURCE_DIR}/source/DWType.hpp
	${CMAKE_SOURCE_DIR}/source/DWUtility.cpp
	${CMAKE_SOURCE_DIR}/source/DWUtility.hpp
	${CMAKE_SOURCE_DIR}/source/main_win32.cpp
)
#インクルードパス
set(INCLUDE_PATH
	${CMAKE_SOURCE_DIR}/library/include/freetype
	${CMAKE_SOURCE_DIR}/library/include/libpng
	${CMAKE_SOURCE_DIR}/library/include/zlib
)

message(STATUS "*** dump start cmake variables ***")
get_cmake_property(_variableNames VARIABLES)
foreach(_variableName ${_variableNames})
        message(STATUS "${_variableName}=${${_variableName}}")
endforeach()
message(STATUS "*** dump end ***")

#ターゲット毎の設定
if(MSVC)
	#Windows用の設定
	message(STATUS "* Target -> Windows")

	#ライブラリパス
	set(LIBRARY_PATH ${CMAKE_SOURCE_DIR}/library/lib/${CMAKE_VS_PLATFORM_NAME})
	#出力ディレクトリ
	set(OUTPUT_PATH "${CMAKE_BINARY_DIR}/${CMAKE_VS_PLATFORM_NAME}")

	#Force to always compile with W4
	if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
	endif()

	#ZERO_CHECKプロジェクトを生成しない
	set(CMAKE_SUPPRESS_REGENERATION true)
	#フィルタ機能を有効にする
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)
	#スタートアッププロジェクトを設定
	set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
	#フィルタ
	source_group(TREE ${CMAKE_SOURCE_DIR}/source PREFIX "\\source" FILES ${SOURCES})
else()
	#その他
	message(STATUS "* Target -> Unknown")

	#なし
endif()

#コンフィギュレーション設定
set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
#インクルードパス設定
include_directories(${INCLUDE_PATH})
#ライブラリパス設定
link_directories(${LIBRARY_PATH})
#出力ディレクトリ設定
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_PATH}/Debug")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_PATH}/Release")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_PATH}/Debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_PATH}/Release")
#実行ファイル
add_executable(${PROJECT_NAME} ${SOURCES})
#リンク
target_link_libraries(${PROJECT_NAME} opengl32.lib freetype28MT.lib libpng16.lib zlib.lib)

add_custom_command(
  TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/image/ $<TARGET_FILE_DIR:${PROJECT_NAME}>/image
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/image/ ${CMAKE_BINARY_DIR}/image
  COMMENT "Copy image files"
)

message(STATUS "* Project -> ${PROJECT_NAME}")
message(STATUS "* Sources -> ${SOURCES}")
message(STATUS "* Include path -> ${INCLUDE_PATH}")
message(STATUS "* Library path -> ${LIBRARY_PATH}")
message(STATUS "* Output path -> ${OUTPUT_PATH}")
message(STATUS "* CMAKE_CXX_FLAGS -> ${CMAKE_CXX_FLAGS}")
