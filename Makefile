CC = gcc
CFLAGS = -Wall -g
#빌드할 파일 이름
TARGETS = server client 

#make 명령시 all 실행
all: $(TARGETS) 

#컴파일
server: server.c banking.h
	$(CC) $(CFLAGS) -o server server.c 

client: client.c banking.h
	$(CC) $(CFLAGS) -o client client.c

#server,client 실행 파일과 .dat 파일과 .log 파일 삭제
clean:
	rm -f $(TARGETS)
	rm -rf *.dat *.log
#파일 이름 충돌 방지
.PHONY: all clean

