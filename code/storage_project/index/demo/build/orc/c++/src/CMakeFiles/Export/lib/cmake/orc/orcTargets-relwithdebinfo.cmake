#----------------------------------------------------------------
# Generated CMake target import file for configuration "RELWITHDEBINFO".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "orc::orc" for configuration "RELWITHDEBINFO"
set_property(TARGET orc::orc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(orc::orc PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "CXX"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/liborc.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS orc::orc )
list(APPEND _IMPORT_CHECK_FILES_FOR_orc::orc "${_IMPORT_PREFIX}/lib/liborc.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
