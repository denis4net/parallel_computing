PREFIX := obj
OBJS := $(patsubst %.cpp,$(PREFIX)/%.o, $(wildcard *.cpp)) 

CXX ?= g++
CXXFLAGS += -std=c++11 -g
LFLAGS += -lpng -lOpenCL -L/opt/AMDAPP/SDK/lib -lGL
all: $(PREFIX)/medianfilter

$(PREFIX)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(PREFIX)/medianfilter: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LFLAGS) $(OBJS) -o $@ 

run: $(PREFIX)/medianfilter
	sudo $< filtermedian_local.cl image.png 5 gpu

clean:
	rm -rf $(PREFIX)/*.o

