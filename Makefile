PWD = $(shell pwd)
CC = gcc
LIBS = -L ./ -llua -luv -ldl -lm -lpthread -L ./cargs -lcargs
INCS = -I include/ -I cargs/include/
FLAG = -fPIC -shared
DEBUG_SOURCES = debug/console.c 
HTTP_SOURCES = http/config.c \
			   http/interface.c \
			   http/request.c \
			   http/response.c \
			   http/web_gateway.c 
LUA_ENGINE_SOURCES = lua_engine/call.c \
					 lua_engine/router.c
SERVER_SOURCES = server/http.c \
				server/main.c
UTILS_SOURCES = utils/kv_param.c \
			   utils/linked_list.c \
			   utils/rbtree.c

SOURCES = $(DEBUG_SOURCES) \
		  $(HTTP_SOURCES) \
		  $(LUA_ENGINE_SOURCES) \
		  $(SERVER_SOURCES) \
		  $(UTILS_SOURCES)

SERVER_NAME = zlong

BUILD_MODULE_OBJS = for source in $^; do \
						$(CC) $(INCS) -c $$source -o `echo $$source | sed s/.c$$/.o/g`; \
					done

CLEAN_MODULE_OBJS = for source in `echo $^ | awk '{gsub(/\.c( |$$)/,".o ",$$0);print $$0}'`; do \
						if [[ -e "$$source" ]]; then \
							rm "$$source"; \
						fi; \
					done

LUA_MODULES = request response

BUILD_LUA_MODULE = $(CC) $(INCS) $(FLAG) $(UTILS_SOURCES) lua_modules/$^.c $(LIBS) -o $^.so
CLEAN_LUA_MODULE = if [[ -e "$^.so" ]]; then \
				   		rm "$^.so"; \
				   fi;

all: cargs_build \
	link_objs \
	build_lua_module

clean: clean_obj \
	clean_lua_module
	if [[ -e "$(SERVER_NAME)" ]]; then \
		rm "$(SERVER_NAME)"; \
	fi

build_lua_module: build_lua_module_request \
	build_lua_module_response

clean_lua_module: clean_lua_module_request \
	clean_lua_module_response

build_lua_module_request: request
	$(BUILD_LUA_MODULE)

clean_lua_module_request: request
	$(CLEAN_LUA_MODULE)

build_lua_module_response: response
	$(BUILD_LUA_MODULE)

clean_lua_module_response: response
	$(CLEAN_LUA_MODULE)

cargs_build:
	git submodule update --init --recursive
	cd cargs && $(MAKE)

link_objs: build_objs
	$(CC) `echo $(SOURCES) | awk '{gsub(/\.c( |$$)/,".o ",$$0);print $$0}'` $(LIBS) -o $(SERVER_NAME)

build_objs: build_debug \
	build_utils \
	build_http \
	build_lua_engine \
	build_server

clean_obj: clean_debug \
	clean_http \
	clean_lua_engine \
	clean_server \
	clean_utils

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

build_utils: $(UTILS_SOURCES)
	$(BUILD_MODULE_OBJS)

clean_utils: $(UTILS_SOURCES)
	$(CLEAN_MODULE_OBJS)

.PHONY: $(LUA_MODULES) clean

