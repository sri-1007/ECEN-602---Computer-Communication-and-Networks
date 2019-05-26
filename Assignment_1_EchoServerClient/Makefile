default: echos echo

echos.o: echos.c
	gcc -g -c echos.c -o echos.o
echos: echos.o
	gcc echos.o -o echos
echo.o: echo.c
	gcc -g -c echo.c -o echo.o
echo: echo.o
	gcc echo.o -o echo

clean:
	rm -rf echos echo *.o
