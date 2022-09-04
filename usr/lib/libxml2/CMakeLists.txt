include_directories(
        include
)

add_library(xml2 STATIC 
	SAX.c xmlmodule.c
	)

target_link_libraries(xml2 c)

