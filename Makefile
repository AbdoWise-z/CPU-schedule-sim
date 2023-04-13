build:
	gcc process_generator.c -o "out/process_generator.out"
	gcc clk.c -o "out/clk.out"
	gcc scheduler.c -o "out/scheduler.out"
	gcc process.c -o "out/process.out"
	gcc test_generator.c -o "out/test_generator.out"

clean:
	rm -f "./out/*.out"  processes.txt

all: clean build

run:
	./out/process_generator.out

test:
	./out/test_generator.out