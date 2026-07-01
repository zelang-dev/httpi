set(TARGET_LIST
 map_insert
 slice_ref
 fetch_parse
 )
foreach (TARGET ${TARGET_LIST})
    add_executable(${TARGET} ${TARGET}.c )
    target_link_libraries(${TARGET} ${SUB_PROJECT})
endforeach()

if (BUILD_HTTPI_APP)
	add_executable(gui_skeleton gui_skeleton.c)
	target_link_libraries(gui_skeleton ${SUB_PROJECT} gui)
endif()
