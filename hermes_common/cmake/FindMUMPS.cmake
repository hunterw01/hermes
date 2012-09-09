#
# MUMPS
#
# set WITH_MUMPS to YES to enable MUMPS support
# set MUMPS_ROOT to point to the directory containing your MUMPS library
#

SET(MUMPS_INCLUDE_SEARCH_PATH
	${MUMPS_ROOT}/include
	/usr/include
	/usr/local/include/
)

SET(MUMPS_LIB_SEARCH_PATH
	${MUMPS_ROOT}/lib
	/usr/lib64
	/usr/lib
	/usr/local/lib/
)

FIND_PATH(MUMPS_INCLUDE_PATH        mumps_c_types.h      ${MUMPS_INCLUDE_SEARCH_PATH})

FIND_LIBRARY(MUMPS_PORD_LIBRARY     pord                 ${MUMPS_LIB_SEARCH_PATH})

IF(NOT WITH_MPI)
  FIND_LIBRARY(MUMPS_COMMON_LIBRARY   mumps_common_seq      ${MUMPS_LIB_SEARCH_PATH})
  
  IF(H1D_REAL OR H2D_REAL OR H3D_REAL)
    FIND_LIBRARY(MUMPSD_MPISEQ_LIBRARY dmumps_seq           ${MUMPS_ROOT}/libseq)
    FIND_PATH(MUMPSD_SEQ_INCLUDE_PATH  mpi.h                ${MUMPS_ROOT}/libseq)
  
    SET(MUMPS_INCLUDE_PATH ${MUMPS_INCLUDE_PATH} ${MUMPSD_SEQ_INCLUDE_PATH})
    LIST(APPEND REQUIRED_LIBRARIES "MUMPSD_MPISEQ_LIBRARY")
  ENDIF(H1D_REAL OR H2D_REAL OR H3D_REAL)
  IF(H1D_COMPLEX OR H2D_COMPLEX OR H3D_COMPLEX)
    FIND_LIBRARY(MUMPSZ_MPISEQ_LIBRARY zmumps_seq           ${MUMPS_ROOT}/libseq)
    FIND_PATH(MUMPSZ_SEQ_INCLUDE_PATH  mpi.h                ${MUMPS_ROOT}/libseq)
  
    SET(MUMPS_INCLUDE_PATH ${MUMPS_INCLUDE_PATH} ${MUMPSZ_SEQ_INCLUDE_PATH})
    LIST(APPEND REQUIRED_LIBRARIES "MUMPSZ_MPISEQ_LIBRARY")
  ENDIF(H1D_COMPLEX OR H2D_COMPLEX OR H3D_COMPLEX)
ELSE(NOT WITH_MPI)
  FIND_LIBRARY(MUMPS_COMMON_LIBRARY   mumps_common         ${MUMPS_LIB_SEARCH_PATH})

  IF(H1D_REAL OR H2D_REAL OR H3D_REAL)
    FIND_LIBRARY(MUMPSD_LIBRARY dmumps ${MUMPS_LIB_SEARCH_PATH})
    LIST(APPEND REQUIRED_REAL_LIBRARIES "MUMPSD_LIBRARY")
  ENDIF(H1D_REAL OR H2D_REAL OR H3D_REAL)
  IF(H1D_COMPLEX OR H2D_COMPLEX OR H3D_COMPLEX)
    FIND_LIBRARY(MUMPSZ_LIBRARY zmumps ${MUMPS_LIB_SEARCH_PATH})
    LIST(APPEND REQUIRED_CPLX_LIBRARIES "MUMPSZ_LIBRARY")
  ENDIF(H1D_COMPLEX OR H2D_COMPLEX OR H3D_COMPLEX)
ENDIF(NOT WITH_MPI)
LIST(APPEND REQUIRED_LIBRARIES "MUMPS_COMMON_LIBRARY" "MUMPS_PORD_LIBRARY")

SET(REQUIRED_REAL_LIBRARIES ${REQUIRED_LIBRARIES})
SET(REQUIRED_CPLX_LIBRARIES ${REQUIRED_LIBRARIES})

# Test if all the required libraries have been found. If they haven't, end with fatal error...
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MUMPS DEFAULT_MSG ${REQUIRED_REAL_LIBRARIES} ${REQUIRED_CPLX_LIBRARIES} MUMPS_INCLUDE_PATH) 

# ...if they have, append them all to the MUMPS_{REAL/CPLX}_LIBRARIES variable.
IF(H1D_REAL OR H2D_REAL OR H3D_REAL)
  FOREACH(_LIB ${REQUIRED_REAL_LIBRARIES})
    LIST(APPEND MUMPS_REAL_LIBRARIES ${${_LIB}})
  ENDFOREACH(_LIB ${REQUIRED_REAL_LIBRARIES})
ENDIF(H1D_REAL OR H2D_REAL OR H3D_REAL)
IF(H1D_COMPLEX OR H2D_COMPLEX OR H3D_COMPLEX)
  FOREACH(_LIB ${REQUIRED_CPLX_LIBRARIES})
    LIST(APPEND MUMPS_CPLX_LIBRARIES ${${_LIB}})
  ENDFOREACH(_LIB ${REQUIRED_CPLX_LIBRARIES})
ENDIF(H1D_COMPLEX OR H2D_COMPLEX OR H3D_COMPLEX)

# Finally, set MUMPS_INCLUDE_DIR to point to the MUMPS include directory.
SET(MUMPS_INCLUDE_DIR ${MUMPS_INCLUDE_DIR} ${MUMPS_INCLUDE_PATH})