CC = g++
CFLAGS = -Wall -std=c++17 -O3
EXEC_NAME = hnStat
OBJ_FILES = main.o

all: $(EXEC_NAME)

clean:
	rm $(EXEC_NAME) $(OBJ_FILES)

$(EXEC_NAME): $(OBJ_FILES)
	$(CC) -o $(EXEC_NAME) $(OBJ_FILES)

%.o: %.cpp fixed_size_pq.h
	$(CC) $(CFLAGS) -o $@ -c $<
