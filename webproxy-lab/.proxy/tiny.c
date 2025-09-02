/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 
Main() : 클라이언트 연결 수락 → doit() 호출
doit() : 요청 파싱 후 정적/동적 분기
serve_static() : 정적 파일 전송
serve_dynamic() : CGI 실행 후 출력 전송
clienterror() : 에러 시 HTML 페이지 전송

curl "http://localhost:33404/cgi-bin/adder?n1=7&n2=3"
HEAD /home.html HTTP/1.0
GET /home.html HTTP/1.0
*/

#include "csapp.h"

void doit(int fd)
{
  int is_static, is_header = 0;                                                                                      // 정적/동적 컨텐츠 구분 플래그
  struct stat sbuf;                                                                                   // 파일 상태 정보 구조체
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;                                                                                          // Robust I/O 구조체

  Rio_readinitb(&rio, fd);                                                                            // rio 버퍼 초기화
  Rio_readlineb(&rio, buf, MAXLINE);                                                                  // 요청 라인의 첫 줄 읽기
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);                                                      // "GET /index.html HTTP/1.0" 파싱
  
  if (!strcasecmp(method, "HEAD"))
  {
    is_header = 1;
  }
  else if (strcasecmp(method, "GET"))                                                                      // Tiny 서버는 GET 메서드만 처리
  {
    clienterror(fd, method, "501", "Not implemented", 
                "Tiny does not implement this method");
    
    return;
  }

  read_requesthdrs(&rio);                                                                             // 나머지 요청 헤더는 무시

  is_static = parse_uri(uri, filename, cgiargs);                                                      // URI 파싱 -> 정적/동적 구분
  
  if (stat(filename, &sbuf) < 0)                                                                      // 요청한 파일이 실제 존재하는지 확인
  {
    clienterror(fd, filename, "404", "Not found",
                "Tiny couldn't find this file");
    
    return;
  }

  if (is_static)                                                                                       // 정적 컨텐츠 처리 
  {
    if (!(S_ISREG(sbuf.st_mode))|| !(S_IRUSR & sbuf.st_mode))                                          // 동적 컨텐츠 처리(CGI 프로그램 실행)
    {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't read this file");
      
      return;
    }
    serve_static(fd, filename, sbuf.st_size, is_header);
  }
  else
  {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't run the CGI program");

      return;
    }
    serve_dynamic(fd, filename, cgiargs);
  }
}

void read_requesthdrs(rio_t *rp)                                                                      // 요청 헤더를 읽어서 출력(실제 처리는 하지 않음)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);                                                                    // 첫 줄 읽기
  while(strcmp(buf, "\r\n"))                                                                          // 빈 줄이 나올 때까지 반복
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);                                                                                // 헤더 내용 출력
  }
  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs)                                               // URI를 정적/동적으로 구분
{
  char* ptr;

  if (!strstr(uri, "cgi-bin"))                                                                        // 정적 컨텐츠
  {
    strcpy(cgiargs, "");                                                                              // CGI 인자 없음
    strcpy(filename, ".");                                                                            // 상대 경로 시작
    strcat(filename, uri);                                                                            // ./index.html 형태
    
    if (uri[strlen(uri)-1] == '/')                                                                    // URI가 "/"로 끝나면 기본 페이지 home.html 제공
    {
      strcat(filename, "home.html");
    }  

    return 1;
  }
  else                                                                                                // 동적 컨텐츠
  {
    ptr = index(uri, '?');                                                                            // CGI 인자 시작점 찾기

    if (ptr)
    {
      strcpy(cgiargs, ptr + 1);                                                                       // "?" 이후로 인자로 저장
      *ptr = '\0';                                                                                    // "?" 기준으로 URI 자르기
    }
    else
    {
      strcpy(cgiargs, "");                                                                            // 인자가 없는 경우
    }
    strcpy(filename, ".");                                                                            
    strcat(filename, uri);                                                                            // 실행할 CGI 파일 이름

    return 0;
  }
}

void serve_static(int fd, char *filename, int filesize, int is_header)                                // 정적 컨텐츠 전송
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXLINE];

  get_filetype(filename, filetype);                                                                   // 파일 확장자로 MIME 타입 결정

  sprintf(buf, "HTTP/1.0 200 OK\r\n");                                                                // 응답 헤더 작성 
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  if (is_header == 1)
  {
    return;
  }

  srcfd = Open(filename, O_RDONLY, 0);                                                                  // 파일 열기
  
  srcp = (char *)Malloc(filesize);
  Rio_readn(srcfd, srcp, filesize);
  
  // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);                                        // 파일 내용을 가상 메모리에 매핑

  Close(srcfd);
  Rio_writen(fd, srcp, filesize);                                                                       // 매핑된 메모리를 클라이언트 소켓에 그대로 write
  // Munmap(srcp, filesize);                                                                            // 매핑 해제
  Free(srcp);
}

void get_filetype(char *filename, char *filetype)                                                    // 파일 확장자에 따라 MIME 타입 지정
{
  if (strstr(filename, ".html"))
  {
    strcpy(filetype, "text/html");
  }
  else if (strstr(filename, ".gif"))
  {
    strcpy(filetype, "image/gif");
  }
  else if (strstr(filename, ".png"))
  {
    strcpy(filetype, "image/png");
  }
  else if (strstr(filename, ".jpg"))
  {
    strcpy(filetype, "image/jpeg");
  }
  else if (strstr(filename, ".mp4"))
  {
    strcpy(filetype, "video/mp4");
  }
  else
  {
    strcpy(filetype, "text/plain");
  }
}

void serve_dynamic(int fd, char *filename, char *cgiargs)                                             // CGI 프로그램 실행 후 결과 전송
{
  char buf[MAXLINE], *emptylist[] = { NULL };

  sprintf(buf, "HTTP/1.0 200 OK\r\n");                                                                // 응답 헤더 전송
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (Fork() == 0)                                                                                    // 자식 프로세스 생성
  {
    setenv("QUERY_STRING", cgiargs, 1);                                                               // CGI 인자 설정
    Dup2(fd, STDOUT_FILENO);                                                                          // 표준 출력 -> 클라이언트 소켓
    Execve(filename, emptylist, environ);                                                             // CGI 프로그램 실행
  }

  Wait(NULL);                                                                                         // 부모는 자식 종료 대기
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg,                                   // 에러 발생 시 HTML 에러 페이지 전송
                 char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  sprintf(body, "<html><title>Tiny Error</title>");                                                   // 에러 메시지 body 구성
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);
  
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);                                               // HTTP 응답 전송
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

int main(int argc, char **argv)                                                                       // main Tiny 서버 메인 루프
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2)                                                                                      // 포트 번호 확인
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);                                                                  // 듣기 소켓 생성

  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen); // line:netp:tiny:accept                                             // 연결 수락
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);  // line:netp:tiny:doit                                                             // 요청 처리
    Close(connfd); // line:netp:tiny:close                                                            // 연결 종료
  }
}