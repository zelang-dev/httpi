set(TARGET_LIST
 map_insert
 slice_ref
 fetch_parse
 )
foreach (TARGET ${TARGET_LIST})
    add_executable(${TARGET} ${TARGET}.c )
    target_link_libraries(${TARGET} ${SUB_PROJECT})
endforeach()

if(APPLE)
	file(GLOB SKELETON_RES resources/skeleton*.*)
	set_source_files_properties(${SKELETON_RES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
	add_executable(gui_skeleton MACOSX_BUNDLE gui_skeleton.c ${SKELETON_RES})
	#set(CPACK_BUNDLE_NAME "Skeleton")
	#set(CPACK_BUNDLE_PLIST resources/info.plist)
	#set(CPACK_BUNDLE_ICON resources/skeleton_64x64.png)
else()
	if(WIN32)
		add_executable(gui_skeleton gui_skeleton.c resources/res.rc)
	else()
		add_executable(gui_skeleton gui_skeleton.c)
	endif()
endif()
target_link_libraries(gui_skeleton ${SUB_PROJECT} gui)
