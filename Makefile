CC=gcc
CFLAGS=-W

TARGET=sendaltec
SOURCES=sendaltec.c

all: $(TARGET)

sendaltec: $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)
	

