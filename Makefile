CLIENT_DIR=client
SERVER_DIR=server

.ONESHELL:

all: client server

spec : spec.proto
	protoc-c --c_out=. spec.proto

run_client: client
	./$(CLIENT_DIR)/client

run_server: server
	./$(SERVER_DIR)/server
clean: clean_server clean_client
	rm -f spec.pb-c.*

clean_client:
	cd $(CLIENT_DIR)
	rm -f parser.tab.c parser.tab.h parser.output lex.yy.c client
	cd ..

clean_server:
	rm -f $(SERVER_DIR)/server

parser: $(CLIENT_DIR)/parser.y
	cd $(CLIENT_DIR)
	bison -d parser.y
	cd ..

lexer: $(CLIENT_DIR)/lexer.l
	cd $(CLIENT_DIR)
	flex lexer.l
	cd ..

client: parser lexer $(CLIENT_DIR)/ast.c $(CLIENT_DIR)/printer.c $(CLIENT_DIR)/converter.c  $(CLIENT_DIR)/main.c spec
	cd $(CLIENT_DIR)
	gcc parser.tab.c lex.yy.c ast.c printer.c converter.c main.c ../spec.pb-c.c -o client -lprotobuf-c
	cd ..

server: $(SERVER_DIR)/src/schema.c $(SERVER_DIR)/src/page.c $(SERVER_DIR)/src/query.c $(SERVER_DIR)/main.c spec
	cd $(SERVER_DIR)
	gcc src/schema.c src/page.c src/query.c main.c -o server -lprotobuf-c
	cd ..

.PHONY: client server spec