all: server

spec : spec.proto
	protoc-c --c_out=. spec.proto

clean:
	rm -f parser.tab.c parser.tab.h parser.output lex.yy.c server *.o

server: server.c spec
	gcc server.c spec.pb-c.c -o server -lprotobuf-c