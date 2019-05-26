default: chats chatc

chats.o: chats.c
	gcc -g -c chats.c -o chats.o
chats: chats.o
	gcc chats.o -o chats
chatc.o: chatc.c
	gcc -g -c chatc.c -o chatc.o
chatc: chatc.o
	gcc chatc.o -o chatc

clean:
	rm -rf chats chatc *.o
