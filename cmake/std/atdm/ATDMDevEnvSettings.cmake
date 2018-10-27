###############################################################################
#
#              Standard ATDM Configuration of Trilnos
#
###############################################################################

INCLUDE("${CMAKE_CURRENT_LIST_DIR}/utils/ATDMDevEnvUtils.cmake")
#
# A) Assert the right env vars are set and set local defaults
#

IF (NOT "$ENV{ATDM_CONFIG_COMPLETED_ENV_SETUP}" STREQUAL "TRUE")
  MESSAGE(FATAL_ERROR
    "Error, env var ATDM_CONFIG_COMPLETED_ENV_SETUP not set to true!"
    "  Check load-env.sh output for error loading the env!"
    ) 
ENDIF()

ATDM_SET_ATDM_VAR_FROM_ENV_AND_DEFAULT(ENABLE_SPARC_SETTINGS OFF)

ASSERT_DEFINED(ENV{ATDM_CONFIG_JOB_NAME})

ASSERT_DEFINED(ENV{ATDM_CONFIG_BUILD_COUNT})
ASSERT_DEFINED(ENV{ATDM_CONFIG_CTEST_PARALLEL_LEVEL})

ASSERT_DEFINED(ENV{ATDM_CONFIG_KNOWN_SYSTEM_NAME})
ASSERT_DEFINED(ENV{ATDM_CONFIG_COMPILER})
ASSERT_DEFINED(ENV{ATDM_CONFIG_BUILD_TYPE})
ASSERT_DEFINED(ENV{ATDM_CONFIG_BLAS_LIBS})
ASSERT_DEFINED(ENV{ATDM_CONFIG_LAPACK_LIBS})
ASSERT_DEFINED(ENV{BOOST_ROOT})
ASSERT_DEFINED(ENV{HDF5_ROOT})
ASSERT_DEFINED(ENV{NETCDF_ROOT})

IF (ATDM_ENABLE_SPARC_SETTINGS)
  ASSERT_DEFINED(ENV{ATDM_CONFIG_BINUTILS_LIBS})
  ASSERT_DEFINED(ENV{METIS_ROOT})
  ASSERT_DEFINED(ENV{PARMETIS_ROOT})
  ASSERT_DEFINED(ENV{PNETCDF_ROOT})
  ASSERT_DEFINED(ENV{CGNS_ROOT})
  ASSERT_DEFINED(ENV{ATDM_CONFIG_SUPERLUDIST_INCLUDE_DIRS})
  ASSERT_DEFINED(ENV{ATDM_CONFIG_SUPERLUDIST_LIBS})
ENDIF()

ATDM_SET_ATDM_VAR_FROM_ENV_AND_DEFAULT(CMAKE_JOB_POOL_LINK "")

ATDM_SET_ATDM_VAR_FROM_ENV_AND_DEFAULT(USE_OPENMP OFF)
ATDM_SET_ATDM_VAR_FROM_ENV_AND_DEFAULT(USE_PTHREADS OFF)
ATDM_SET_ATDM_VAR_FROM_ENV_AND_DEFAULT(USE_CUDA OFF)

SET(ATDM_INST_SERIAL OFF)
SET(ATDM_KOKKOS_SERIAL OFF)
IF (ATDM_USE_OPENMP)
  SET(ATDM_NODE_TYPE OPENMP)
ELSEIF (ATDM_USE_PTHREADS)
  SET(ATDM_NODE_TYPE THREAD)
ELSEIF (ATDM_USE_CUDA)
  SET(ATDM_NODE_TYPE CUDA)
  SET(ATDM_INST_SERIAL ON)
ELSE()
  SET(ATDM_NODE_TYPE SERIAL)
  SET(ATDM_INST_SERIAL ON)
  SET(ATDM_KOKKOS_SERIAL ON)
ENDIF()

# ATDM_CMAKE_BUILD_TYPE and ATDM_BOUNDS_CHECK
IF ("$ENV{ATDM_CONFIG_BUILD_TYPE}" STREQUAL "RELEASE_DEBUG")
  SET(ATDM_CMAKE_BUILD_TYPE RELEASE)
  SET(ATDM_BOUNDS_CHECK ON)
ELSEIF ("$ENV{ATDM_CONFIG_BUILD_TYPE}" STREQUAL "RELEASE")
  SET(ATDM_CMAKE_BUILD_TYPE RELEASE)
  SET(ATDM_BOUNDS_CHECK OFF)
ELSEIF ("$ENV{ATDM_CONFIG_BUILD_TYPE}" STREQUAL "DEBUG")
  SET(ATDM_CMAKE_BUILD_TYPE DEBUG)
  SET(ATDM_BOUNDS_CHECK ON)
ELSE()
  MESSAGE(FATAL_ERROR
    "Error, env var ATDM_CONFIG_BUILD_TYPE=$ENV{ATDM_CONFIG_BUILD_TYPE} is"
    " set to an illegal value!  Only the values 'RELEASE_DEBUG', 'RELEASE',"
    " and 'DEBUG' are allowed!"
    )
ENDIF()

ATDM_SET_ATDM_VAR_FROM_ENV_AND_DEFAULT(SHARED_LIBS OFF)
ATDM_SET_ATDM_VAR_FROM_ENV_AND_DEFAULT(CMAKE_BUILD_WITH_INSTALL_RPATH OFF)

ATDM_SET_ATDM_VAR_FROM_ENV_AND_DEFAULT(USE_HWLOC OFF)
iF (ATDM_USE_HWLOC AND "$ENV{ATDM_CONFIG_HWLOC_LIBS}" STREQUAL "")
  MESSAGE(FATAL_ERROR
    "Error, env var USE_HWLOC=$ATDM_USE_HWLOC but env var"
    " HWLOC_LIBS is not set!")
ENDIF()

# If ATDM_CONFIG_KOKKOS_ARCH is set to empty, don't append the KOKKOS_ARCH
# name.  This makes sense for platforms like 'rhel'" that may involve
# different CPU architectures where it would not make sense to try to set a
# specific KOKKOS_ARCH value.
SET(ATDM_CONFIG_KOKKOS_ARCH_JOB_NAME_KEYS $ENV{ATDM_CONFIG_KOKKOS_ARCH})
IF(ATDM_CONFIG_KOKKOS_ARCH_JOB_NAME_KEYS)
  STRING(TOUPPER ${ATDM_CONFIG_KOKKOS_ARCH_JOB_NAME_KEYS}
    ATDM_CONFIG_KOKKOS_ARCH_JOB_NAME_KEYS)
  STRING(REPLACE , - ATDM_CONFIG_KOKKOS_ARCH_JOB_NAME_KEYS
    ${ATDM_CONFIG_KOKKOS_ARCH_JOB_NAME_KEYS})
  SET(ATDM_CONFIG_KOKKOS_ARCH_JOB_NAME_KEYS
    -${ATDM_CONFIG_KOKKOS_ARCH_JOB_NAME_KEYS})
ENDIF()

SET(ATDM_JOB_NAME_KEYS_STR
  "$ENV{ATDM_CONFIG_COMPILER}-$ENV{ATDM_CONFIG_BUILD_TYPE}-${ATDM_NODE_TYPE}${ATDM_CONFIG_KOKKOS_ARCH_JOB_NAME_KEYS}")
PRINT_VAR(ATDM_JOB_NAME_KEYS_STR)

ATDM_SET_ATDM_VAR_FROM_ENV_AND_DEFAULT(MPI_EXEC mpiexec)
ATDM_SET_ATDM_VAR_FROM_ENV_AND_DEFAULT(MPI_EXEC_NUMPROCS_FLAG -np)

#
# B) Look for tweaks file(s) for this build and load the file(s) if it exists
# 

SET(ATDM_TWEAKS_FILE_DEFAULT_DEFAULT
  "${CMAKE_CURRENT_LIST_DIR}/$ENV{ATDM_CONFIG_KNOWN_SYSTEM_NAME}/tweaks/${ATDM_JOB_NAME_KEYS_STR}.cmake")
IF (EXISTS "${ATDM_TWEAKS_FILE_DEFAULT_DEFAULT}")
  SET(ATDM_TWEAKS_FILES_DEFAULT "${ATDM_TWEAKS_FILE_DEFAULT_DEFAULT}")
ELSE()
  SET(ATDM_TWEAKS_FILES_DEFAULT "")
ENDIF()

ADVANCED_SET(ATDM_TWEAKS_FILES "${ATDM_TWEAKS_FILES_DEFAULT}"
  CACHE FILEPATH
  "Extra *.cmake file to include to define tweaks for this ATDM build ${ATDM_JOB_NAME_KEYS_STR}"
  )
PRINT_VAR(ATDM_TWEAKS_FILES)

FOREACH(ATDM_TWEAKS_FILE ${ATDM_TWEAKS_FILES})
  MESSAGE("-- " "Including ATDM build tweaks file ${ATDM_TWEAKS_FILE} ...")
  TRIBITS_TRACE_FILE_PROCESSING(PROJECT  INCLUDE "${ATDM_TWEAKS_FILE}")
  INCLUDE("${ATDM_TWEAKS_FILE}")
ENDFOREACH()

#
# C) Set the compilers
#

# All ATDM builds of Trilinos are MPI builds!
ATDM_SET_ENABLE(TPL_ENABLE_MPI ON)

ASSERT_DEFINED(ENV{MPICC})
ASSERT_DEFINED(ENV{MPICXX})
ASSERT_DEFINED(ENV{MPIF90})

ATDM_SET_CACHE(CMAKE_C_COMPILER "$ENV{MPICC}" CACHE FILEPATH)
ATDM_SET_CACHE(CMAKE_CXX_COMPILER "$ENV{MPICXX}" CACHE FILEPATH)
ATDM_SET_CACHE(CMAKE_Fortran_COMPILER "$ENV{MPIF90}" CACHE FILEPATH)
ATDM_SET_ENABLE(Trilinos_ENABLE_Fortran ${ATDM_ENABLE_SPARC_SETTINGS})

IF (NOT "$ENV{ATDM_CONFIG_OPENMP_FORTRAN_FLAGS}" STREQUAL "")
  ATDM_SET_CACHE(OpenMP_Fortran_FLAGS "$ENV{ATDM_CONFIG_OPENMP_FORTRAN_FLAGS}"
    CACHE STRING)
  ATDM_SET_CACHE(OpenMP_Fortran_LIB_NAMES "$ENV{ATDM_CONFIG_OPENMP_FORTRAN_LIB_NAMES}"
   CACHE STRING)
  ATDM_SET_CACHE(OpenMP_gomp_LIBRARY "$ENV{ATDM_CONFIG_OPENMP_GOMP_LIBRARY}"
   CACHE STRING)
ENDIF()

#
# D) Set up basic compiler flags, link flags etc.
#

ATDM_SET_CACHE(BUILD_SHARED_LIBS "${ATDM_SHARED_LIBS}" CACHE BOOL)
ATDM_SET_CACHE(CMAKE_BUILD_TYPE "${ATDM_CMAKE_BUILD_TYPE}" CACHE STRING)
ATDM_SET_CACHE(CMAKE_C_FLAGS "$ENV{EXTRA_C_FLAGS}" CACHE STRING)
ATDM_SET_CACHE(CMAKE_CXX_FLAGS "$ENV{EXTRA_CXX_FLAGS}" CACHE STRING)

#
# E) Set up other misc options
#

# Currently, EMPIRE configures of Trilinos have this enabled by default.  But
# really we should elevate every subpackage that ATDM uses to Primary Tested.
# That is the right solution.
ATDM_SET_ENABLE(Trilinos_ENABLE_SECONDARY_TESTED_CODE ON)

# Other various options
ATDM_SET_CACHE(CTEST_BUILD_FLAGS "-j$ENV{ATDM_CONFIG_BUILD_COUNT}" CACHE STRING)
ATDM_SET_CACHE(CMAKE_JOB_POOL_LINK "${ATDM_CMAKE_JOB_POOL_LINK}" CACHE STRING)
ATDM_SET_CACHE(CTEST_PARALLEL_LEVEL "$ENV{ATDM_CONFIG_CTEST_PARALLEL_LEVEL}" CACHE STRING)
ATDM_SET_CACHE(CMAKE_SKIP_RULE_DEPENDENCY ON CACHE BOOL)
ATDM_SET_CACHE(CMAKE_BUILD_WITH_INSTALL_RPATH ${ATDM_CMAKE_BUILD_WITH_INSTALL_RPATH} CACHE BOOL)
ATDM_SET_CACHE(CMAKE_VERBOSE_MAKEFILE OFF CACHE BOOL)
ATDM_SET_CACHE(TPL_FIND_SHARED_LIBS ${ATDM_SHARED_LIBS} CACHE BOOL)
ATDM_SET_CACHE(MPI_EXEC ${ATDM_MPI_EXEC} CACHE FILEPATH)
ATDM_SET_CACHE(MPI_EXEC_PRE_NUMPROCS_FLAGS "$ENV{ATDM_CONFIG_MPI_PRE_FLAGS}" CACHE STRING)
ATDM_SET_CACHE(MPI_EXEC_NUMPROCS_FLAG "${ATDM_MPI_EXEC_NUMPROCS_FLAG}" CACHE STRING)
ATDM_SET_CACHE(MPI_EXEC_POST_NUMPROCS_FLAGS "$ENV{ATDM_CONFIG_MPI_POST_FLAG}" CACHE STRING)
ATDM_SET_CACHE(Trilinos_VERBOSE_CONFIGURE OFF CACHE BOOL)
ATDM_SET_CACHE(Trilinos_ENABLE_DEBUG OFF CACHE BOOL)
ATDM_SET_CACHE(Trilinos_ENABLE_EXPLICIT_INSTANTIATION ON CACHE BOOL)
ATDM_SET_CACHE(Trilinos_ENABLE_INSTALL_CMAKE_CONFIG_FILES ON CACHE BOOL)
ATDM_SET_CACHE(Trilinos_ENABLE_DEVELOPMENT_MODE OFF CACHE BOOL)
ATDM_SET_CACHE(Trilinos_ASSERT_MISSING_PACKAGES ON CACHE BOOL)
ATDM_SET_CACHE(Trilinos_ENABLE_OpenMP "${ATDM_USE_OPENMP}" CACHE BOOL)
IF (ATDM_ENABLE_SPARC_SETTINGS)
  ATDM_SET_CACHE(Kokkos_ENABLE_Serial "${ATDM_KOKKOS_SERIAL}" CACHE BOOL)
ENDIF()
ATDM_SET_CACHE(Kokkos_ENABLE_OpenMP "${ATDM_USE_OPENMP}" CACHE BOOL)
ATDM_SET_CACHE(Kokkos_ENABLE_Pthread "${ATDM_USE_PTHREADS}" CACHE BOOL)
ATDM_SET_CACHE(Kokkos_ENABLE_Cuda_UVM "${ATDM_USE_CUDA}" CACHE BOOL)
ATDM_SET_CACHE(Kokkos_ENABLE_Cuda_Relocatable_Device_Code OFF CACHE BOOL)
ATDM_SET_CACHE(Kokkos_ENABLE_CXX11_DISPATCH_LAMBDA ON CACHE BOOL)
ATDM_SET_CACHE(Kokkos_ENABLE_Cuda_Lambda "${ATDM_USE_CUDA}" CACHE BOOL)
ATDM_SET_CACHE(Kokkos_ENABLE_Debug_Bounds_Check "${ATDM_BOUNDS_CHECK}" CACHE BOOL)
ATDM_SET_CACHE(Kokkos_ENABLE_Profiling OFF CACHE BOOL)
ATDM_SET_CACHE(KOKKOS_ENABLE_DEBUG "${ATDM_BOUNDS_CHECK}" CACHE BOOL)
ATDM_SET_CACHE(KOKKOS_ARCH "$ENV{ATDM_CONFIG_KOKKOS_ARCH}" CACHE STRING)
ATDM_SET_CACHE(EpetraExt_ENABLE_HDF5 OFF CACHE BOOL)
ATDM_SET_CACHE(Panzer_ENABLE_FADTYPE "Sacado::Fad::DFad<RealType>" CACHE STRING)
ATDM_SET_CACHE(Phalanx_KOKKOS_DEVICE_TYPE "${ATDM_NODE_TYPE}" CACHE STRING)
ATDM_SET_CACHE(Phalanx_SHOW_DEPRECATED_WARNINGS OFF CACHE BOOL)
ATDM_SET_CACHE(Tpetra_INST_CUDA "${ATDM_USE_CUDA}" CACHE BOOL)
ATDM_SET_CACHE(Tpetra_INST_SERIAL "${ATDM_INST_SERIAL}" CACHE BOOL)
ATDM_SET_CACHE(DART_TESTING_TIMEOUT 600 CACHE STRING)
IF (ATDM_ENABLE_SPARC_SETTINGS)
  ATDM_SET_CACHE(Zoltan_ENABLE_ULLONG_IDS ON CACHE BOOL)
  ATDM_SET_CACHE(Anasazi_ENABLE_RBGen ON CACHE BOOL)
  ATDM_SET_CACHE(Amesos2_ENABLE_Epetra OFF CACHE BOOL)
ENDIF()

#
# F) TPL locations and enables
#
# Since this is special ATDM configuration of Trilinos, it makes sense to go
# ahead and enable all of the TPLs by default that are used by the ATDM
# applications.
#

# CUDA
ATDM_SET_ENABLE(TPL_ENABLE_CUDA "${ATDM_USE_CUDA}")

# CUSPARSE
ATDM_SET_ENABLE(TPL_ENABLE_CUSPARSE "${ATDM_USE_CUDA}")

# Pthread
ATDM_SET_ENABLE(TPL_ENABLE_Pthread OFF)

# BinUtils
ATDM_SET_ENABLE(TPL_ENABLE_BinUtils ${ATDM_ENABLE_SPARC_SETTINGS})
ATDM_SET_ENABLE(TPL_BinUtils_LIBRARIES "$ENV{ATDM_CONFIG_BINUTILS_LIBS}")

# BLAS
ATDM_SET_ENABLE(TPL_ENABLE_BLAS ON)
ATDM_SET_CACHE(TPL_BLAS_LIBRARIES "$ENV{ATDM_CONFIG_BLAS_LIBS}" CACHE FILEPATH)

# Boost
ATDM_SET_ENABLE(TPL_ENABLE_Boost ON)
ATDM_SET_CACHE(Boost_INCLUDE_DIRS "$ENV{BOOST_ROOT}/include" CACHE FILEPATH)
ATDM_SET_CACHE(TPL_Boost_LIBRARIES
   "$ENV{BOOST_ROOT}/lib/libboost_program_options.so;$ENV{BOOST_ROOT}/lib/libboost_system.so"
  CACHE FILEPATH)

# BoostLib
ATDM_SET_ENABLE(TPL_ENABLE_BoostLib ON)
ATDM_SET_CACHE(BoostLib_INCLUDE_DIRS "$ENV{BOOST_ROOT}/include" CACHE FILEPATH)
ATDM_SET_CACHE(TPL_BoostLib_LIBRARIES
  "$ENV{BOOST_ROOT}/lib/libboost_program_options.a;$ENV{BOOST_ROOT}/lib/libboost_system.a"
   CACHE FILEPATH)

# METIS
ATDM_SET_ENABLE(TPL_ENABLE_METIS ${ATDM_ENABLE_SPARC_SETTINGS})
ATDM_SET_CACHE(METIS_INCLUDE_DIRS "$ENV{METIS_ROOT}/include" CACHE FILEPATH)
ATDM_SET_CACHE(METIS_LIBRARY_DIRS "$ENV{METIS_ROOT}/lib" CACHE FILEPATH)

# ParMETIS
ATDM_SET_ENABLE(TPL_ENABLE_ParMETIS ${ATDM_ENABLE_SPARC_SETTINGS})
ATDM_SET_CACHE(ParMETIS_INCLUDE_DIRS "$ENV{PARMETIS_ROOT}/include" CACHE FILEPATH)
ATDM_SET_CACHE(ParMETIS_LIBRARY_DIRS "$ENV{PARMETIS_ROOT}/lib" CACHE FILEPATH)

# HWLOC
ATDM_SET_ENABLE(TPL_ENABLE_HWLOC ${ATDM_USE_HWLOC})
ATDM_SET_CACHE(TPL_HWLOC_LIBRARIES "$ENV{ATDM_CONFIG_HWLOC_LIBS}" CACHE FILEPATH)

# LAPACK
ATDM_SET_ENABLE(TPL_ENABLE_LAPACK ON)
ATDM_SET_CACHE(TPL_LAPACK_LIBRARIES "$ENV{ATDM_CONFIG_LAPACK_LIBS}" CACHE FILEPATH)

# CGNS
ATDM_SET_ENABLE(TPL_ENABLE_CGNS ${ATDM_ENABLE_SPARC_SETTINGS})
ATDM_SET_CACHE(CGNS_INCLUDE_DIRS "$ENV{CGNS_ROOT}/include" CACHE FILEPATH)
ATDM_SET_CACHE(CGNS_LIBRARY_DIRS "$ENV{CGNS_ROOT}/lib" CACHE FILEPATH)

# HDF5
IF (ATDM_ENABLE_SPARC_SETTINGS)
  # SPARC ATDM Trilinos configuration does not actually enable HDF5 TPL!
  ATDM_SET_ENABLE(TPL_ENABLE_HDF5 OFF)
ELSE()
  ATDM_SET_ENABLE(TPL_ENABLE_HDF5 ON)
  ATDM_SET_CACHE(HDF5_INCLUDE_DIRS "$ENV{HDF5_ROOT}/include" CACHE FILEPATH)
  IF ("$ENV{ATDM_CONFIG_HDF5_LIBS}" STREQUAL "")
    MESSAGE(FATAL_ERROR "Error: ToDo: Implement default HDF5 libs")
    #SET(HDF5_LIBS
    #  "$ENV{HDF5_ROOT}/lib/libhdf5_hl.a;$ENV{HDF5_ROOT}/lib/libhdf5.a;-lz;-ldl")
  ENDIF()
  ATDM_SET_CACHE(TPL_HDF5_LIBRARIES "$ENV{ATDM_CONFIG_HDF5_LIBS}" CACHE FILEPATH)
ENDIF()

# Netcdf
ATDM_SET_ENABLE(TPL_ENABLE_Netcdf ON)
IF (ATDM_ENABLE_SPARC_SETTINGS)
  # SPARC ATDM Trilinos configuration has SEACAS-customized FindNetCDF.cmake
  # module recurrsively find PNetCDF and HDF5 internally!
  ATDM_SET_CACHE(HDF5_ROOT "$ENV{HDF5_ROOT}" CACHE FILEPATH)
  ATDM_SET_CACHE(PNetCDF_ROOT "$ENV{PNETCDF_ROOT}" CACHE FILEPATH)
  ATDM_SET_CACHE(NetCDF_ROOT "$ENV{NETCDF_ROOT}" CACHE FILEPATH)
ELSE()
  IF ("$ENV{ATDM_CONFIG_NETCDF_LIBS}" STREQUAL "")
    MESSAGE(FATAL_ERROR "Error: ToDo: Implement default Netcdf libs")
    #SET(NETCDF_LIBS
    #  "$ENV{NETCDF_ROOT}/lib/libnetcdf.a;$ENV{HDF5_ROOT}/lib/libhdf5_hl.a;$ENV{HDF5_ROOT}/lib/libhdf5.a;-lz;-ldl")
  ENDIF()
  ATDM_SET_CACHE(Netcdf_INCLUDE_DIRS "$ENV{NETCDF_ROOT}/include" CACHE FILEPATH)
  ATDM_SET_CACHE(TPL_Netcdf_LIBRARIES "$ENV{ATDM_CONFIG_NETCDF_LIBS}" CACHE FILEPATH)
ENDIF()

# SuperLUDist
ATDM_SET_ENABLE(TPL_ENABLE_SuperLUDist ${ATDM_ENABLE_SPARC_SETTINGS})
ATDM_SET_CACHE(SuperLUDist_INCLUDE_DIRS "$ENV{ATDM_CONFIG_SUPERLUDIST_INCLUDE_DIRS}" CACHE FILEPATH)
ATDM_SET_CACHE(TPL_SuperLUDist_LIBRARIES "$ENV{ATDM_CONFIG_SUPERLUDIST_LIBS}" CACHE FILEPATH)

# DLlib
ATDM_SET_CACHE(TPL_DLlib_LIBRARIES "-ldl" CACHE FILEPATH)
# NOTE: Not clear why you need this since the TPL DLlib is not explicilty
# enabled anywhere in the EM-Plasma/BuildScripts files.xsxs

#
# G) Test Disables
#
# There are some tests that have to be disabled for a braod set of builds
# for example, if all openmp builds are failing a certain test then it 
# makes more sense to disbale it once in this file instead of in every openmp
# buid's tweaks file
#


#
# H) ATDM env config install hooks
#
# Install just enough to allow loading the exact matching env and nothing
# else!
#

IF (COMMAND INSTALL)

SET(ATDM_CONFIG_SCRIPTS_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/share/atdm-trilinos)

  INSTALL( FILES ${CMAKE_CURRENT_LIST_DIR}/load-env.sh
    DESTINATION ${ATDM_CONFIG_SCRIPTS_INSTALL_DIR} )
  
  INSTALL( DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/utils
    DESTINATION ${ATDM_CONFIG_SCRIPTS_INSTALL_DIR}
    PATTERN "*.cmake" EXCLUDE )
  
  INSTALL( DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/$ENV{ATDM_CONFIG_KNOWN_SYSTEM_NAME}
    DESTINATION ${ATDM_CONFIG_SCRIPTS_INSTALL_DIR}
    PATTERN "*.cmake" EXCLUDE )
  
  SET( ATDM_JOB_NAME $ENV{ATDM_CONFIG_JOB_NAME} )
  
  SET( ATDM_INSTALLED_ENV_LOAD_SCRIPT_NAME load_matching_env.sh
    CACHE STRING
    "Name of script installed in <CMAKE_INSTALL_PREFIX> to source to load matching env." )
  
  SET( ATDM_TRILINOS_INSTALL_PREFIX_ENV_VAR_NAME  ATDM_TRILINOS_INSTALL_PREFIX
    CACHE STRING
    "Name of env var set to <CMAKE_INSTALL_PREFIX> set in installed script <ATDM_INSTALLED_ENV_LOAD_SCRIPT_NAME>." )
  
  CONFIGURE_FILE( ${CMAKE_CURRENT_LIST_DIR}/load_matching_env.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/load_matching_env.sh @ONLY )
  
  INSTALL( FILES ${CMAKE_CURRENT_BINARY_DIR}/load_matching_env.sh
    DESTINATION ${CMAKE_INSTALL_PREFIX}
    RENAME ${ATDM_INSTALLED_ENV_LOAD_SCRIPT_NAME} )

  INSTALL( PROGRAMS ${${PROJECT_NAME}_SOURCE_DIR}/packages/kokkos/bin/nvcc_wrapper
    DESTINATION ${ATDM_CONFIG_SCRIPTS_INSTALL_DIR} )

ENDIF()
