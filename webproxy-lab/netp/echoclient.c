#include "csapp.h"

int main(int argc, char **argv)
{
    int clientfd;                                                       // 서버와 통신할 소켓 파일 디스크립터
    char *host, *port, buf[MAXLINE];                                    // 서버 호스트 이름, 포트 번호, I/O 버퍼
    rio_t rio;                                                          // robust I/O를 위한 구조체

    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <host> <port> \n", argv[0]);
        exit(0);
    }
    host = argv[1];                                                     
    port = argv[2];

    clientfd = Open_clientfd(host, port);                               // 서버와 연결하는 소켓 생성(TCP)
    Rio_readinitb(&rio, clientfd);                                      // rio 구조체 초기화: clientfd와 연결

    while (Fgets(buf, MAXLINE, stdin) != NULL)                          // 표준 입력(stdin)에서 한 줄씩 읽어서 서버로 전송
    {
        Rio_writen(clientfd, buf, strlen(buf));                         // 입력받은 문자열을 서버에 전송
        Rio_readlineb(&rio, buf, MAXLINE);                              // 서버가 보낸 응답 한 줄 읽기
        Fputs(buf, stdout);                                             // 읽은 응답을 표준 출력에 출력
    }
    
    Close(clientfd);                                                    // 서버와의 연결 닫기
    exit(0);
}