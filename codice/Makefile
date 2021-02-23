CC = gcc
OTP = -pthread

all: 
	$(CC) -Wall -Wextra server.c -o server $(OTP)
	$(CC) -Wall -Wextra client.c -o client $(OTP)

server:
	$(CC) -Wall -Wextra server.c -o server $(OTP)

client:
	$(CC) -Wall -Wextra client.c -o client $(OTP)

clean:
	rm client server