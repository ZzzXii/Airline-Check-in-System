.phony all:
all: ACS 

ACS: ACS.c
	
	gcc -Wall  ACS.c -o ACS  -pthread



.PHONY clean:
clean:
	-rm -rf ACS