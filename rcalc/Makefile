MODE     := dev
ifeq ($(MODE), dev)
	OFLAGS := -g -O0 -DDEBUG
else
	OFLAGS := -O3 -march=native
endif
CPPFLAGS :=
CXXFLAGS   := -std=c++17 -pedantic -Wall -Wno-unused-function $(OFLAGS)
CXFLAGS   := -std=c++17 -pedantic -Wall $(OFLAGS) -fno-exceptions -fno-rtti -fno-unwind-tables -fno-asynchronous-unwind-tables -fvisibility=hidden -fno-semantic-interposition -fno-math-errno -fno-trapping-math
LDFLAGS  :=

CC = g++

TARGET_EXE := rcalc
BUILD_DIR := build
SRC_DIR   := src

SRC := src/main.cpp
OBJ := $(addprefix $(BUILD_DIR)/, $(notdir $(SRC:.cpp=.o)))

all: $(BUILD_DIR) $(BUILD_DIR)/$(TARGET_EXE)

$(BUILD_DIR)/$(TARGET_EXE): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -r $(BUILD_DIR)

.PHONY: all clean
