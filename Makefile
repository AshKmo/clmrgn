bin/clmrgn : main.c
	gcc -o bin/clmrgn -lm main.c

clean :
	rm -f bin/clmrgn

debug : main.c
	gcc -o bin/clmrgn -g -lm main.c
