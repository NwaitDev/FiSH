CC=gcc
CFLAGS=-Wall -std=c99 -g
LDFLAGS=-g
TARGET=fish cmdline_test

all: $(TARGET)

#règles de compilation séparée des .c
# $< -> première dépendance (c'est à dire fish.c)
# $@ -> cible (c'est à dire fish.o)
fish.o: fish.c
	$(CC) $(CFLAGS) -c $< -o $@ 

cmdline.o: cmdline.c cmdline.h
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

libcmdline.so: cmdline.o
	$(CC) $(LDFLAGS) -o $@ -shared $< 

cmdline_test.o: cmdline_test.c
	$(CC) $(CFLAGS) -c $< -o $@ 

#règle d'édition de lien
#$^ correspond à toutes les dépendances
fish: fish.o libcmdline.so 
	$(CC) $(LDFLAGS) $^ -o $@
	
cmdline_test: cmdline_test.o libcmdline.so
	$(CC) $(LDFLAGS) $^ -o $@

clean:
	rm -f *.o

mrproper: clean
	rm $(TARGET) *.so
