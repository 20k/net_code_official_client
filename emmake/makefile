#
# Makefile to use with emscripten
# See https://emscripten.org/docs/getting_started/downloads.html
# for installation instructions. This Makefile assumes you have
# loaded emscripten's environment.
#
# Running `make` will produce three files:
#  - example_emscripten.html
#  - example_emscripten.js
#  - example_emscripten.wasm
#
# All three are needed to run the demo.

.SUFFIXES:

CC = emcc
CXX = em++

EXE = index.html
SOURCES = ../main.cpp ../auto_handlers.cpp ../copy_handler.cpp ../editable_string.cpp ../font_cfg.cpp ../format_cache.cpp ../imgui_ui_components.cpp ../local_commands.cpp ../serialisables.cpp ../steam_api.cpp ../string_helpers.cpp ../tokeniser.cpp ../auth_manager.cpp
SOURCES += ../deps/imgui.cpp ../deps/imgui_draw.cpp ../deps/imgui_widgets.cpp
SOURCES += ../deps/examples/imgui_impl_opengl3.cpp ../deps/examples/imgui_impl_sdl.cpp ../deps/examples/imgui_impl_glfw.cpp
SOURCES += ../deps/misc/freetype/imgui_freetype.cpp ../deps/misc/cpp/imgui_stdlib.cpp
SOURCES += ../deps/toolkit/base_serialisables.cpp ../deps/toolkit/clock.cpp ../deps/toolkit/opencl.cpp ../deps/toolkit/render_window.cpp ../deps/toolkit/render_window_sdl2.cpp ../deps/toolkit/render_window_glfw.cpp ../deps/toolkit/stacktrace.cpp ../deps/toolkit/texture.cpp ../deps/toolkit/fs_helpers.cpp ../deps/toolkit/clipboard.cpp
SOURCES += ../deps/networking/beast_compilation_unit.cpp ../deps/networking/networking.cpp ../deps/networking/serialisable.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
UNAME_S := $(shell uname -s)

#EMS = -s USE_SDL=2 -s WASM=1
EMS = -s WASM=1 -s USE_FREETYPE=1 -s USE_WEBGL2=1 -s USE_BOOST_HEADERS=1 -s USE_PTHREADS=1 -s WASM_MEM_MAX=1073741824 -s USE_SDL=2 -s USE_GLFW=3
#EMS += -fno-rtti -DEMSCRIPTEN_HAS_UNBOUND_TYPE_NAMES=0
#EMS += --profiling
#EMS += -s ALLOW_MEMORY_GROWTH=1
#EMS += -s PROXY_TO_PTHREAD=1
EMS += -s TOTAL_MEMORY=1073741824
#EMS += -s DISABLE_EXCEPTION_CATCHING=2 -s EXCEPTION_CATCHING_WHITELIST=["client_thread_tcp"]
EMS += -s NO_EXIT_RUNTIME=0 
EMS += -s ASYNCIFY=1
#EMS += -flto


#EMS += -s ASSERTIONS=1
#EMS += -s NO_FILESYSTEM=1 -DIMGUI_DISABLE_FILE_FUNCTIONS
# Uncomment next line to fix possible rendering bugs with emscripten version older then 1.39.0 (https://github.com/ocornut/imgui/issues/2877)
#EMS += -s BINARYEN_TRAP_MODE=clamp
#EMS += -s SAFE_HEAP=1    ## Adds overhead

CPPFLAGS = -I../deps -I../deps/imgui -I../deps/imtui/include
#CPPFLAGS += -g
CPPFLAGS += -Wall -Wformat -Oz -std=c++20
CPPFLAGS += -DSDL_MAIN_HANDLED
CPPFLAGS += $(EMS)
LIBS = $(EMS)
LDFLAGS = --shell-file index_in.html -s USE_PTHREADS=1 -s SOCKET_DEBUG=0
LDFLAGS += -s WEBSOCKET_URL=wss:// -lidbfs.js -lglfw3
LDFLAGS += -s ASYNCIFY=1 -Oz
#LDFLAGS += -flto 
#LDFLAGS += -s DISABLE_EXCEPTION_CATCHING=2 -s EXCEPTION_CATCHING_WHITELIST=["client_thread_tcp"]
#LDFLAGS += -s ASYNCIFY_IGNORE_INDIRECT=1
#LDFLAGS += -s ASYNCIFY=1 -O3 --closure 1 --llvm-lto 3
#LDFLAGS = --shell-file index_in.html -lglfw3 -s USE_PTHREADS=1 -s WEBSOCKET_URL=wss://

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

%.o:%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<
    
%.o:../%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<


%.o:../deps/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

%.o:../deps/imgui/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<
    
%.o:../deps/toolkit/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

%.o:../deps/networking/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

%.o:../deps/imtui/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<
    
%.o:../deps/imgui/examples/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<
    
%.o:../deps/imgui/misc/freetype/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

%.o:../deps/imgui/misc/cpp/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(EXE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(LIBS) $(LDFLAGS)

clean:
	rm -f $(EXE) $(OBJS) *.js *.wasm *.wasm.pre