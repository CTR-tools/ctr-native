# Shipped catalogs are managed resources. User overrides under assets are never touched.
file(GLOB catalogs LIST_DIRECTORIES FALSE RELATIVE "${SOURCE}/resources/text"
    "${SOURCE}/resources/text/*/expanded.toml" "${SOURCE}/resources/text/*/retail.toml")
foreach(catalog IN LISTS catalogs)
    set(input "${SOURCE}/resources/text/${catalog}")
    set(output "${DESTINATION}/resources/text/${catalog}")
    if(NOT input STREQUAL output)
        get_filename_component(directory "${output}" DIRECTORY)
        file(MAKE_DIRECTORY "${directory}")
        configure_file("${input}" "${output}" COPYONLY)
    endif()
endforeach()
