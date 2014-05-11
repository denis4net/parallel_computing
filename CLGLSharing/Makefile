PREFIX := obj
OBJS := $(patsubst %.cpp,$(PREFIX)/%.o, $(wildcard *.cpp)) 
BIN := openclgl

CXX ?= g++
CXXFLAGS += -std=c++11 -g 
LFLAGS += -lpng -lX11 -ldl -lglut -lGLEW -lGLU -lOpenCL -lGL 

all: $(PREFIX)/$(BIN)

$(PREFIX)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(PREFIX)/$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LFLAGS) $(OBJS) -o $@ 

clean:
	rm -rf $(PREFIX)/*.o

