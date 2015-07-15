include_directories(${PROJECT_SOURCE_DIR}/include)
aux_source_directory(./ SRC_LIST)
add_library(mycpp SHARED ${SRC_LIST})
install(TARGETS mycpp 
	LIBRARY DESTINATION lib
	)
