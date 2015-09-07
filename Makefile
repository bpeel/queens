CFLAGS = -g -Wall
CC = gcc

SOURCES = \
	queens.c \
	$(NULL)

OBJS = $(SOURCES:.c=.o)

.c.o :
	$(CC) $(CFLAGS) -c -o $@ $<

queens : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -lm
