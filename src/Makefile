# Makefile para la elaboración de un servidor con sockets.
# Opciones:
# * make: compila
# * make run: compila y ejecuta
# * make clean: borra los archivos binarios
# * make clear: borra los archivos binarios

all: ftp_server


ftp_server: ClientConnection.cpp FTPServer.cpp ftp_server.cpp
	g++ -g -std=gnu++0x  ClientConnection.cpp FTPServer.cpp ftp_server.cpp -o ftp_server -lpthread -lstdc++fs

clean:
	$(RM) ftp_server *~

run: clean ftp_server
	./ftp_server

clear: clean
