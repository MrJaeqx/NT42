all:
	gcc main.cpp -g -lcurl -lexpat -o weather.out

clean:
	rm -f weather.out
	rm -f test.out
