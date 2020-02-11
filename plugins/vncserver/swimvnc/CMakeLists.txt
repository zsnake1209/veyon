include(BuildPlugin)

ADD_DEFINITIONS(-D_WIN32_WINNT=0x0600)

# TODO: use compiled libvncserver library
set(libvncserver_SOURCES
	${libvncserver_DIR}/libvncserver/auth.c
	${libvncserver_DIR}/libvncserver/cargs.c
	${libvncserver_DIR}/libvncserver/corre.c
	${libvncserver_DIR}/libvncserver/cursor.c
	${libvncserver_DIR}/libvncserver/cutpaste.c
	${libvncserver_DIR}/libvncserver/draw.c
	${libvncserver_DIR}/libvncserver/font.c
	${libvncserver_DIR}/libvncserver/hextile.c
	${libvncserver_DIR}/libvncserver/httpd.c
	${libvncserver_DIR}/libvncserver/main.c
	${libvncserver_DIR}/libvncserver/rfbregion.c
	${libvncserver_DIR}/libvncserver/rfbserver.c
	${libvncserver_DIR}/libvncserver/rre.c
	${libvncserver_DIR}/libvncserver/scale.c
	${libvncserver_DIR}/libvncserver/selbox.c
	${libvncserver_DIR}/libvncserver/sockets.c
	${libvncserver_DIR}/libvncserver/stats.c
	${libvncserver_DIR}/libvncserver/translate.c
	${libvncserver_DIR}/libvncserver/ultra.c
	${libvncserver_DIR}/libvncserver/zlib.c
	${libvncserver_DIR}/libvncserver/zrle.c
	${libvncserver_DIR}/libvncserver/zrleoutstream.c
	${libvncserver_DIR}/libvncserver/zrlepalettehelper.c
	${libvncserver_DIR}/libvncserver/tight.c
	${libvncserver_DIR}/common/crypto_openssl.c
	${libvncserver_DIR}/common/d3des.c
	${libvncserver_DIR}/common/turbojpeg.c
	${libvncserver_DIR}/common/vncauth.c
)

set(swimvnc_SOURCES
	DeskDupEngine.cpp
	DeskDupEngine.h
	SwimVncServer.cpp
	SwimVncServer.h
	SwimVncConfiguration.h
	)

set_source_files_properties(${libvncserver_SOURCES} PROPERTIES
	COMPILE_FLAGS "-Wno-unused-result -Wno-parentheses -Wno-unused-function -Wno-unused-variable -Wno-deprecated-declarations -Wno-address -Wno-format -Wno-strict-aliasing -Wno-user-defined-warnings"
	COTIRE_EXCLUDED TRUE
)

build_plugin(swim-vnc-server
	${swimvnc_SOURCES}
	${libvncserver_SOURCES}
)

target_compile_options(swim-vnc-server PRIVATE "-Wno-parentheses")

target_include_directories(swim-vnc-server PRIVATE
	${libvncserver_DIR}/libvncserver
	${libvncserver_DIR}/common
)
