CC = gcc
CFLAGS = -Wall -pthread -ljson-c -Ifeatures -Icore -Iroutes

all: server

server: main.o core/sse.o features/user.o features/room.o features/game.o features/leaderboard.o utils/utils.o utils/database.o core/server.o routes/auth_routes.o middleware/cookies.o routes/api_routes.o handler/http_handler.o routes/room_routes.o
	$(CC) -o server main.o core/sse.o features/user.o features/room.o features/game.o features/leaderboard.o utils/utils.o utils/database.o core/server.o routes/auth_routes.o middleware/cookies.o routes/api_routes.o handler/http_handler.o routes/room_routes.o $(CFLAGS)

main.o: main.c
	$(CC) -c main.c -o main.o $(CFLAGS)

core/server.o: core/server.c core/server.h
	$(CC) -c core/server.c -o core/server.o $(CFLAGS)

core/sse.o: core/sse.c core/sse.h
	$(CC) -c core/sse.c -o core/sse.o $(CFLAGS)

features/user.o: features/user.c features/user.h
	$(CC) -c features/user.c -o features/user.o $(CFLAGS)

features/room.o: features/room.c features/room.h
	$(CC) -c features/room.c -o features/room.o $(CFLAGS)

features/game.o: features/game.c features/game.h
	$(CC) -c features/game.c -o features/game.o $(CFLAGS)

features/leaderboard.o: features/leaderboard.c features/leaderboard.h
	$(CC) -c features/leaderboard.c -o features/leaderboard.o $(CFLAGS)

utils/utils.o: utils/utils.c utils/utils.h
	$(CC) -c utils/utils.c -o utils/utils.o $(CFLAGS)

utils/database.o: utils/database.c utils/database.h
	$(CC) -c utils/database.c -o utils/database.o $(CFLAGS)

routes/auth_routes.o: routes/auth_routes.c routes/auth_routes.h
	$(CC) -c routes/auth_routes.c -o routes/auth_routes.o $(CFLAGS)

routes/api_routes.o: routes/api_routes.c routes/api_routes.h
	$(CC) -c routes/api_routes.c -o routes/api_routes.o $(CFLAGS)

routes/room_routes.o: routes/room_routes.c routes/room_routes.h
	$(CC) -c routes/room_routes.c -o routes/room_routes.o $(CFLAGS)

middleware/cookies.o: middleware/cookies.c middleware/cookies.h
	$(CC) -c middleware/cookies.c -o middleware/cookies.o $(CFLAGS)

handler/http_handler.o: handler/http_handler.c handler/http_handler.h
	$(CC) -c handler/http_handler.c -o handler/http_handler.o $(CFLAGS)

clean:
	rm -f main.o core/*.o features/*.o utils/*.o routes/*.o server middleware/*.o handler/*.o
