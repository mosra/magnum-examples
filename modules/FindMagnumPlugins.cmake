# Find MagnumPlugins - Magnum plugins handling module for CMake
#
# This module depends on Magnum and additionally defines:
#
# MAGNUM_PLUGINS_FOUND          - True if Magnum plugins were found
#

find_package(Magnum REQUIRED)

if (NOT MAGNUMPLUGINS_FOUND)
    # Paths
    find_path(_MAGNUM_PLUGINS_INCLUDE_DIR
        NAMES ColladaImporter
        PATH_SUFFIXES Magnum/Plugins
    )

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args("MagnumPlugins" DEFAULT_MSG
        _MAGNUM_PLUGINS_INCLUDE_DIR
    )

endif()
