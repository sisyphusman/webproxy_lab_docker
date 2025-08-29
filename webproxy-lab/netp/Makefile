# Makefile for Echo client/server with csapp.{c,h}

# 사용할 컴파일러 지정
CC      = gcc

# 컴파일 옵션
# -Wall    : 모든 경고 메시지 활성화
# -Wextra  : 추가적인 경고 메시지 활성화
# -O2      : 최적화 레벨 2
# -g       : 디버깅 심볼 포함 (gdb 사용 가능)
CFLAGS  = -Wall -Wextra -O2 -g

# 링크 시 필요한 라이브러리
# -lpthread : POSIX 스레드 라이브러리
LIBS    = -lpthread

# CSAPP 지원 파일 (라이브러리 함수 구현부와 헤더)
CSAPP_SRC = csapp.c
CSAPP_HDR = csapp.h

# 빌드할 실행 파일 목록
TARGETS = echoclient echoserver

# 기본 타겟: make만 입력하면 echoclient와 echoserver 모두 빌드됨
all: $(TARGETS)

# echoclient 빌드 규칙
# echoclient.c와 csapp.c를 컴파일하여 echoclient 실행 파일 생성
echoclient: echoclient.c $(CSAPP_SRC) $(CSAPP_HDR)
	$(CC) $(CFLAGS) -o $@ echoclient.c $(CSAPP_SRC) $(LIBS)

# echoserver 빌드 규칙
# echoserver.c와 csapp.c를 컴파일하여 echoserver 실행 파일 생성
echoserver: echoserver.c $(CSAPP_SRC) $(CSAPP_HDR)
	$(CC) $(CFLAGS) -o $@ echoserver.c $(CSAPP_SRC) $(LIBS)

# clean 규칙
# 생성된 실행 파일과 오브젝트 파일 삭제
clean:
	rm -f $(TARGETS) *.o