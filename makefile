# Makefile pour le programme

# Nom du compilateur
CC=gcc

# Options de compilation
CFLAGS=-Wall -Wextra -g

# Liste des fichiers source
SRCS=main.c fonctions.c

# Liste des fichiers objets générés
OBJS=$(SRCS:.c=.o)

# Nom de l'exécutable généré
EXEC=my_program

# Règle de compilation
all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(EXEC)

# Règle de nettoyage
clean:
	rm -f $(EXEC) $(OBJS)

# Règle pour générer le fichier de configuration Doxygen
Doxyfile:
	doxygen -g

# Règle pour la génération de la documentation Doxygen
doc:
	doxygen Doxyfile


