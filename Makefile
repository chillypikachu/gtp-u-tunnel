CXX = g++

CXXFLAGS = -std=c++20 -Wall -Wextra -O3

TARGET = tun

SOURCES = src/tun.cpp \
          src/ue_session.cpp \
          src/raw_socket.cpp \
          src/gtpu.cpp \
          src/gtpu_socket.cpp \
          src/packet_handlers.cpp

OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -Isrc/include -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean