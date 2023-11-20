idf_component_register(SRCS "webserver.c" "attack.c" "main.c" "scan_wifi.c"
                    INCLUDE_DIRS "."
		    EMBED_FILES webpages/index.html)
target_link_libraries(${COMPONENT_LIB} -Wl,-zmuldefs)