#include "csapp.h"

void echo(int connfd)                                                                                // 클라이언트가 보낸 데이터를 읽어서 그대로 돌려주는 함수
{                                     
    size_t n;                                                                                        // 읽은 바이트 수
    char buf[MAXLINE];                                                                               // I/O 버퍼
    rio_t rio;                                                                                       // robust I/O 구조체 (버퍼링된 I/O 관리)
                                     
    Rio_readinitb(&rio, connfd);                                                                     // rio 구조체 초기화 (connfd 소켓과 연결)
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0)                                              // 클라이언트로부터 한 줄씩 읽고 -> 다시 그대로 돌려줌
    {                                     
        printf("server received %d bytes\n", (int)n);                                                // 몇 바이트 읽었는지 출력
        Rio_writen(connfd, buf, n);                                                                  // 받은 데이터를 그대로 클라이언트에 전송
    }                                     
}                                     
                                     
int main(int argc, char** argv)                                                                      // 서버 실행 -> 클라이언트와 연결 수립 -> echo 처리
{                                     
    int listenfd, connfd;                                                                            // 리스닝 소켓, 연결 소켓
    socklen_t clientlen;                                                                             // 클라이언트 주소 구조체 크기
    struct sockaddr_storage clientaddr;                                                              // 클라이언트 주소
    char client_hostname[MAXLINE], client_port[MAXLINE];                                             // 클라이언트의 호스트/포트 저장용 문자열
                                     
    if (argc != 2)                                     
    {                                     
        fprintf(stderr, "usage: %s <port>\n", argv[0]);                                     
        exit(0);                                     
    }                                     
                                     
    listenfd = Open_listenfd(argv[1]);                                                               // 지정된 포트 번호로 서버 소켓 생성
    
    while (1)
    {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);                                    // 클라이언트 연결 수락
        Getnameinfo((SA*)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0); // 클라이언트의 호스트 이름과 포트 번호를 문자열로 변환
        printf("Connected to (%s, %s)\n", client_hostname, client_port);                             // 연결된 클라이언트 정보 출력
        echo(connfd);                                                                                // 클라이언트와 데이터 송수신
        Close(connfd);
    }
    
    exit(0);
}