all: StandardCollision

StandardCollision: StandardCollision.c
	gcc -o StandardCollision StandardCollision.c -lSDL2 -g

clean:
	rm StandardCollision