build:
	mkdir out
	gcc process_generator.c -o "out/process_generator.out"
	gcc clk.c -o "out/clk.out"
	gcc scheduler.c -o "out/scheduler.out"
	gcc process.c -o "out/process.out"
	gcc test_generator.c -o "out/test_generator.out"

clean:
	rm -f -r out
	rm -f processes.txt

all: clean build

run:
	./out/process_generator.out

generate:
	./out/test_generator.out

test:

	gcc test.c -o "out/test.out"
	./out/test.out