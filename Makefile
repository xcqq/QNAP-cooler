CFLAGS=-Werror -Iinclude/
LD_FLAGS=-Llib/ -ldl

.IGNORE: clean

all: fan_controller

fan_controller: *.c
	$(CC) -o $@ $^ $(LD_FLAGS) $(CFLAGS)

clean:
	@rm fan_controller
