CC = gcc
CFLAGS = -Wall -pthread -g

SRCS = server.c queue.c threadpool.c task.c user.c
OBJS = $(SRCS:.c=.o)
TARGET = server

all: $(TARGET) client

$(TARGET): $(OBJS)
    $(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

client: client.c
    $(CC) $(CFLAGS) -o client client.c

%.o: %.c
    $(CC) $(CFLAGS) -c $< -o $@

clean:
    rm -f $(OBJS) $(TARGET) client
