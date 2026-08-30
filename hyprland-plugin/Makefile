CXX ?= g++
CXXFLAGS := -Wall -Wextra -std=c++26 -fPIC -O2 $(shell pkg-config --cflags hyprland pixman-1 hyprutils hyprlang aquamarine)
LDFLAGS := -shared

TARGET := minimize-hook.so
SRCS := main.cpp
OBJS := $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
