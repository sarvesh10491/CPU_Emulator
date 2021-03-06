# the compiler: g++ for C++ program
CC = g++
# LIBS = -pthread -lm -lrt
CFLAGS  = -Wall
# DEPS = queue_ops.h
TARGET = main_cpu

all: $(TARGET)

$(TARGET): $(TARGET).cpp $(DEPS)
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).cpp $(LIBS)

clean:
	$(RM) $(TARGET)