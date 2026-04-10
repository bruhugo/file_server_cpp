CC = g++
CFLAGS = -Wall -g -Iinclude -I../basic/include

TARGET = build/main
SRCS = $(wildcard src/*.cpp) $(wildcard ../basic/src/*.cpp)
OBJECTS = $(patsubst src/%.cpp, build/%.o, $(SRCS))

$(TARGET): $(OBJECTS) | build
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ 

build/%.o: src/%.cpp | build 
	$(CC) $(CFLAGS) -c $< -o $@

build: 
	mkdir build

run: $(TARGET)
	./$(TARGET)

debug: $(TARGET)
	./$(TARGET) DEBUG

remove: 
	rm -r build