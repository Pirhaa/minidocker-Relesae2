CC = gcc
CFLAGS = -Wall -Wextra -g
SRC = src/main.c src/monitor.c src/container.c src/cgroup.c src/image.c src/overlay.c

minidocker: $(SRC)
	$(CC) $(CFLAGS) -o minidocker $(SRC)

clean:
	rm -f minidocker
