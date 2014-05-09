PREFIX := obj
OBJS := $(patsubst %.cpp,$(PREFIX)/%.o, $(wildcard *.cpp)) 
BIN := clsobel-mpi

CXX := mpic++
CXXFLAGS += -std=c++11 -g
LFLAGS += -L/opt/AMDAPP/SDK/lib -lpng -lX11 -ldl -lglut -lGLEW -lGLU -lOpenCL -lGL 

all: $(PREFIX)/$(BIN)

$(PREFIX)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(PREFIX)/$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LFLAGS) $(OBJS) -o $@ 

run:
	time /usr/bin/mpirun -n 4 $(PREFIX)/$(BIN)

clean:
	rm -rf $(PREFIX)/*.o

