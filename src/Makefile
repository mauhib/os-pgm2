.PHONY: clean run profile test
CFLAGS=-Wall -g -lpthread -lrt
TARGET=pgm2
ARGS = out/producer_time.txt out/producer_items.txt out/consumer_time.txt out/consumer_items.txt

$(TARGET): $(TARGET).c

clean:
	rm -f *.o out/* gmon.out prof_output

run:
	./$(TARGET) $(ARGS)

profile:
	$(CC) $(CFLAGS) -pg $(TARGET).c -o $(TARGET)
	make run
	gprof $(TARGET) gmon.out > prof_output
	rm -f gmon.out $(TARGET)

test:
	make
	valgrind ./$(TARGET) $(ARGS)
