CXX = g++
CXXFLAGS = -std=c++14 -Wall -pthread
SRC = main.cpp Server.cpp ThreadPool.cpp
OBJ = $(SRC:.cpp=.o)
EXEC = server

# Default target to build the server
all: $(EXEC)

$(EXEC): $(OBJ)
	$(CXX) $(OBJ) -o $(EXEC)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f $(OBJ) $(EXEC)
