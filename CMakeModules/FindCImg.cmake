# - Find CImg
# Find CImg header.
#
#  CImg_INCLUDE_DIRS - where to find CImg uncludes.
#  CImg_FOUND        - True if CImg found.

# Look for the header file.
FIND_PATH(CImg_INCLUDE_DIRS 
    NAMES CImg.h
)

find_library(x11_LIBRARY
    NAMES X11
    PATHS ${CIMG_PKGCONF_LIBRARY_DIRS}
)
find_library(pthread_LIBRARY
    NAMES pthread
    PATHS ${CIMG_PKGCONF_LIBRARY_DIRS}
)
set(CImg_LIBRARIES ${x11_LIBRARY} ${pthread_LIBRARY})

# handle the QUIETLY and REQUIRED arguments and set TBB_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CImg DEFAULT_MSG CImg_INCLUDE_DIRS CImg_LIBRARIES)
MARK_AS_ADVANCED(CImg_INCLUDE_DIRS)
