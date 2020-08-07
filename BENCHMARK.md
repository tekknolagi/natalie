The reason I won't merge this branch:

```
git checkout master
make clean build
bin/natalie -c fib1 examples/fib.rb

git checkout std-map
make clean build
bin/natalie -c fib2 examples/fib.rb

time ./fib1
75025
./fib1  0.53s user 0.00s system 99% cpu 0.537 total
time ./fib1
75025
./fib1  0.53s user 0.00s system 99% cpu 0.534 total
time ./fib1
75025
./fib1  0.53s user 0.00s system 99% cpu 0.540 total

time ./fib2
75025
./fib2  1.41s user 0.00s system 93% cpu 1.502 total
time ./fib2
75025
./fib2  1.41s user 0.00s system 99% cpu 1.422 total
time ./fib2
75025
./fib2  1.41s user 0.00s system 99% cpu 1.424 total
```
