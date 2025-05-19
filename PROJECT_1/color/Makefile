CFLAGS = -Wall -g     # Flags per la compilazione (debug, warning)
LIBS = -lm            # Libreria matematica se necessario
SRCS = main.c     # File sorgente principale
OBJS = $(SRCS:.c=.o)  # Obiettivi (file .o)
BIN = rgb_exec   # Nome del binario finale

# Regola di compilazione per il binario finale
all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)
	@echo "Build completed!"

# Regola per la compilazione di ogni file sorgente
%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

# Pulizia dei file oggetto e del binario
clean:
	rm -f $(BIN)
	@echo "Cleaned up!"

# Regola per un build rapido
quick: clean $(BIN)
