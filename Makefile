PWD = $(shell pwd)
CC = gcc
UTILS_LIBS = -L ./ -lzlutils
UV_LIBS = -luv -lpthread
LUA_LIBS = -L ./ -llua -ldl -lm
CARGS_LIBS = -L ./cargs -lcargs 
SSL_LIBS = -lssl
CRYPTO_LIBS = -lcrypto
SESSION_LIBS = -L ./ -l$(SESSION_NAME)
LOCAL_INCS = -I include/
CARGS_INCS = -I cargs/include/
INCS = $(LOCAL_INCS) $(CARGS_INCS)
FLAG = -fPIC -shared
DEBUG = -D=DEBUG
DEBUG_SOURCES = debug/console.c 
HTTP_SOURCES = http/config.c \
			   http/interface.c \
			   http/request.c \
			   http/response.c \
			   http/web_gateway.c \
			   http/websocket.c
LUA_ENGINE_SOURCES = lua_engine/call.c \
					 lua_engine/router.c \
					 lua_engine/request_wrap.c \
					 lua_engine/response_wrap.c \
					 lua_engine/session_wrap.c 
SERVER_SOURCES = server/http.c \
				 server/https.c \
				server/main.c
UTILS_SOURCES = utils/kv_param.c \
			   utils/linked_list.c \
			   utils/rbtree.c \
			   utils/sha1.c \
			   utils/base64.c
SESSION_SOURCES = session/session_storage.c \
				  session/session_handler.c

SOURCES = $(DEBUG_SOURCES) \
		  $(HTTP_SOURCES) \
		  $(LUA_ENGINE_SOURCES) \
		  $(SERVER_SOURCES)

SERVER_NAME = zlong
SESSION_NAME = zlsession
UTILS_NAME = zlutils

BUILD_MODULE_OBJS = for source in $^; do \
						$(CC) $(DEBUG) $(INCS) -c $$source -o `echo $$source | sed s/.c$$/.o/g`; \
					done

CLEAN_MODULE_OBJS = for source in `echo $^ | awk '{gsub(/\.c( |$$)/,".o ",$$0);print $$0}'`; do \
						if [[ -e "$$source" ]]; then \
							rm "$$source"; \
						fi; \
					done

LUA_MODULES = request response websocket_frame

BUILD_LUA_MODULE = $(CC) $(DEBUG) $(INCS) $(FLAG) lua_modules/$^.c \
				   $(UTILS_LIBS) $(LUA_LIBS) $(SESSION_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS) \
				   -o $^.so
CLEAN_LUA_MODULE = if [[ -e "$^.so" ]]; then \
				   		rm "$^.so"; \
				   fi;

all: build_cargs \
	build_zlutils \
	build_zlsession \
	link_objs \
	build_lua_module
	ldconfig

clean: clean_cargs \
	clean_obj \
	clean_lua_module \
	clean_zlsession \
	clean_zlutils
	if [[ -e "$(SERVER_NAME)" ]]; then \
		rm "$(SERVER_NAME)"; \
	fi

build_lua_module: 

clean_lua_module: 

build_cargs:
	git submodule update --init --recursive
	cd cargs && $(MAKE)

clean_cargs:
	cd cargs && $(MAKE) clean

link_objs: build_objs
	$(CC) $(DEBUG) `echo $(SOURCES) | awk '{gsub(/\.c( |$$)/,".o ",$$0);print $$0}'` \
		$(UTILS_LIBS) $(UV_LIBS) $(LUA_LIBS) $(CARGS_LIBS) $(SESSION_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS) \
		-o $(SERVER_NAME)

build_zlsession:
	$(CC) $(DEBUG) $(FLAG) $(LOCAL_INCS) -o lib$(SESSION_NAME).so $(SESSION_SOURCES) $(UTILS_SOURCES) $(UV_LIBS)

clean_zlsession:
	if [[ -e "lib$(SESSION_NAME).so" ]]; then \
		rm "lib$(SESSION_NAME).so"; \
	fi

build_zlutils:
	$(CC) $(DEBUG) $(FLAG) $(LOCAL_INCS) -o lib$(UTILS_NAME).so $(UTILS_SOURCES)

clean_zlutils:
	if [[ -e "lib$(UTILS_NAME).so" ]]; then \
		rm "lib$(UTILS_NAME).so"; \
	fi

build_objs: build_debug \
	build_http \
	build_lua_engine \
	build_server

clean_obj: clean_debug \
	clean_http \
	clean_lua_engine \
	clean_server

build_debug: $(DEBUG_SOURCES)
	$(BUILD_MODULE_OBJS)

clean_debug: $(DEBUG_SOURCES)
	$(CLEAN_MODULE_OBJS)

build_http: $(HTTP_SOURCES)
	$(BUILD_MODULE_OBJS)

clean_http: $(HTTP_SOURCES)
	$(CLEAN_MODULE_OBJS)

build_lua_engine: $(LUA_ENGINE_SOURCES)
	$(BUILD_MODULE_OBJS)

clean_lua_engine: $(LUA_ENGINE_SOURCES)
	$(CLEAN_MODULE_OBJS)

build_server: $(SERVER_SOURCES)
	$(BUILD_MODULE_OBJS)

clean_server: $(SERVER_SOURCES)
	$(CLEAN_MODULE_OBJS)

.PHONY: $(LUA_MODULES) clean

