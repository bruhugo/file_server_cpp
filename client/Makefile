CC = g++
CFLAGS = -Wall -g -Iinclude -I../basic/include

TARGET = build/main

# Only include existing .cpp files
SRCS = src/main.cpp src/Connection.cpp ../basic/src/Types.cpp
OBJECTS = $(SRCS:src/%.cpp=build/%.o)

$(TARGET): $(OBJECTS) | build
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ 


# Pattern rule for client sources
build/%.o: src/%.cpp | build
	$(CC) $(CFLAGS) -c $< -o $@

# Pattern rule for basic sources
build/%.o: ../basic/src/%.cpp | build
	$(CC) $(CFLAGS) -c $< -o $@

build: 
	mkdir build

run: $(TARGET)
	./$(TARGET)

debug: $(TARGET)
	./$(TARGET) DEBUG