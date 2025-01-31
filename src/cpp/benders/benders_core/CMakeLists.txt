# ===========================================================================
# CMake configuration
# ===========================================================================

# ===========================================================================
# Targets
# ===========================================================================

find_package(TBB REQUIRED CONFIG)
if (TBB_VERSION_MAJOR VERSION_LESS "2018")
	message(FATAL_ERROR "Require tbb 2018 or higher.")
endif()
if (TBB_VERSION_MAJOR VERSION_GREATER "2020")
	message(FATAL_ERROR "Require tbb 2018 to 2020.")
endif()

add_library (benders_core STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/SubproblemWorker.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/SimulationOptions.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/BendersBase.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/WorkerMaster.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/SubproblemCut.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/common.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/BendersStructsDatas.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Worker.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/LastIterationWriter.cpp 
	${CMAKE_CURRENT_SOURCE_DIR}/LastIterationReader.cpp 
	${CMAKE_CURRENT_SOURCE_DIR}/LastIterationPrinter.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/StartUp.cpp)

get_target_property(JSON_INC_PATH jsoncpp_lib INTERFACE_INCLUDE_DIRECTORIES)



target_include_directories (benders_core
		PUBLIC
			${CMAKE_CURRENT_SOURCE_DIR}/include
            ${JSON_INC_PATH}/jsoncpp
)

target_link_libraries (benders_core
	PUBLIC
		helpers
		output_core
		solvers
		glog::glog
		TBB::tbb
		jsoncpp_lib
)

add_library (${PROJECT_NAME}::benders_core ALIAS benders_core)