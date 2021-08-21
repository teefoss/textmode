TARGET	= libtextmode.a
CFLAGS	= -Wall -Wextra -Werror -Wshadow -g
LIBS	= -lSDL2

OBJ=text.o sound.o color.o console.o screen.o

$(TARGET): $(OBJ)
	ar rcs $@ $^

test: $(OBJ) test.o
	cc $^ -o $@ $(LIBS) && ./$@

%.o: %.c
	cc -o $@ -c $< $(CFLAGS)

.PHONY: clean
clean:
	@rm -rf *.o $(TARGET) test
