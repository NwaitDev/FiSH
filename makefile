CC=gcc
CFLAGS=-Wextra -std=c99 -g 
LDFLAGS=-g
TARGET=fish cmdline_test

all: $(TARGET)

#règles de compilation séparée des .c
# $< -> première dépendance (c'est à dire fish.c)
# $@ -> cible (c'est à dire fish.o)
fish.o: fish.c cmdline.h
	$(CC) $(CFLAGS) -c $< -o $@ 

cmdline.o: cmdline.c cmdline.h
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

cmdline_test.o: cmdline_test.c cmdline.h
	$(CC) $(CFLAGS) -c $< -o $@ 


# création de la bibliothèque dynamique partagée
libcmdline.so: cmdline.o 
	$(CC) -shared $^ -o libcmdline.so

#règle d'édition de lien
#$^ correspond à toutes les dépendances
fish: fish.o libcmdline.so
	$(CC) $(LDFLAGS) -L${PWD} $< -lcmdline -o $@
	
cmdline_test: cmdline_test.o libcmdline.so
	$(CC) $(LDFLAGS) -L${PWD} $< -lcmdline -o $@

clean:
	rm -f *.o *.so

mrproper: clean
	rm $(TARGET) 
