# ===========================================================================
# CMake configuration
# ===========================================================================

# ===========================================================================
# External requirements
# ===========================================================================

# ===========================================================================
# Targets
# ===========================================================================

# ---------------------------------------------------------------------------
# Conditionnal settings of solver source files
# ---------------------------------------------------------------------------
list(APPEND Solver_sources
	${CMAKE_CURRENT_LIST_DIR}/SolverFactory.cpp
)

# XPRESS
IF( XPRESS )
	list(APPEND Solver_sources
		${CMAKE_CURRENT_LIST_DIR}/SolverXpress.cpp
	)
ENDIF( XPRESS )

#Clp - CBc
IF(COIN_OR)
	list(APPEND Solver_sources
		${CMAKE_CURRENT_LIST_DIR}/SolverCbc.cpp
		${CMAKE_CURRENT_LIST_DIR}/SolverClp.cpp
		${CMAKE_CURRENT_LIST_DIR}/COIN_common_functions.cpp
	)
ENDIF(COIN_OR)


# ---------------------------------------------------------------------------
# Solver library creation and conditionnal link to internal solvers libraries
# ---------------------------------------------------------------------------
add_library (solvers STATIC
	${Solver_sources}
)
get_target_property(xpansion_interfaces_path xpansion_interfaces INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories (solvers
		PUBLIC
			${CMAKE_CURRENT_SOURCE_DIR}/include
		PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}
		public
		${xpansion_interfaces_path}	
)


# XPRESS
if(XPRESS)
	target_link_libraries (solvers
		PUBLIC
			XPRESS::XPRESS
	)
endif()

#CLP-CBC
if(COIN_OR)
	target_link_libraries (solvers
		PUBLIC
			Coin::Clp
			Coin::Cgl
			Coin::CoinUtils
			Coin::Osi
			Coin::Cbc
	)
endif()
