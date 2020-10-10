all:
	gcc server.c -o server -pthread
	gcc client.c -o client -pthread

Server:
	gcc server.c -o server -pthread

Client:
	gcc client.c -o client -pthread
