CXX = g++
CXXFLAGS = -std=c++17
TARGET = proxy_server
SRCS = Server.cpp ThreadPool.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
    $(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

run: $(TARGET)
    ./$(TARGET)

clean:
    rm -f $(TARGET)
