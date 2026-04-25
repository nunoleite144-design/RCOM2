CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11
TARGET = download
SRC = src/download.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run-example: $(TARGET)
	./$(TARGET) ftp://anonymous:anonymous@mirrors.up.pt/debian/README.html

clean:
	rm -f $(TARGET) *.o
