#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

// Proxy 서버에서 클라이언트 요청의 URI를 파싱하는 함수
// 입력: "http://host[:port]/path"
// 출력: host, port, path를 각각 문자열로 반환
// 성공 시 0, 실패 시 -1
static int parse_uri_proxy(const char *uri, char *host, char *port, char *path)
{
  char hostport[MAXLINE]; // host[:port] 임시 저장
  size_t len;

  // 프록시가 다루는 요청은 반드시 "http://"로 시작해야 함
  if (strncasecmp(uri, "http://", 7) != 0)
  {
    return -1;
  }

  const char *p = uri + 7;            // p: "host" 시작 위치, "http://" 다음부터 host[:port] 시작
  const char *slash = strchr(p, '/'); // '/' (path 시작) 위치 찾기, 없으면 문자열 끝까지

  if (!slash)
  {
    slash = p + strlen(p);
  }

  len = slash - p; // host[:port] 부분의 길이 계산
  if (len >= sizeof(hostport))
  {
    return -1;
  }

  memcpy(hostport, p, len); // host[:port] 문자열 복사
  hostport[len] = '\0';

  char *colon = strchr(hostport, ':'); // host:port 구분하는 ':' 위치, ':' 있는 경우 → host와 port 분리
  if (colon)
  {
    *colon = '\0'; // ':' 자리에 널 문자 넣어 host 문자열 종료
    strcpy(host, hostport);
    strcpy(port, colon + 1);
  }
  else
  {
    strcpy(host, hostport); // ':' 없으면 포트는 기본값 80
    strcpy(port, "80");
  }

  if (*slash == '\0') // '/' 가 없으면 path는 "/"로, 있으면 그 이후 부분이 path
  {
    strcpy(path, "/");
  }
  else
  {
    strcpy(path, slash);
  }

  return 0;
}

void *thread(void *vargp)
{
  int connfd = *((int *)vargp);
  free(vargp);
  pthread_detach(pthread_self());

  doit(connfd);
  Close(connfd);
  return NULL;
}

void doit(int clientfd)
{
  rio_t crio;
  char line[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];

  Rio_readinitb(&crio, clientfd);
  if (!Rio_readlineb(&crio, line, MAXLINE))
  {
    return;
  }

  printf("Request: %s", line);
  sscanf(line, "%s %s %s", method, uri, version);

  if (strcasecmp(method, "GET"))
  {
    clienterror(clientfd, method, "501", "Not Implemented",
                "Proxy supports only GET/HEAD");
    return;
  }

  // absolute-form URI만 지원
  char host[MAXLINE], port[16], path[MAXLINE];
  if (parse_uri_proxy(uri, host, port, path) < 0)
  {
    clienterror(clientfd, uri, "400", "Bad Request",
                "Absolute-form URI required");
    return;
  }

  // 나머지 클라이언트 헤더는 읽어서 버린다(빈 줄까지)
  for (;;)
  {
    if (!Rio_readlineb(&crio, line, sizeof(line)))
    {
      return;
    }
    if (!strcmp(line, "\r\n"))
    {
      break;
    }
  }

  // 원서버 연결
  int serverfd = Open_clientfd(host, port);
  if (serverfd < 0)
  {
    clienterror(clientfd, host, "502", "Bad Gateway", "Cannot connect to origin");
    return;
  }

  // 요청라인/헤더를 아주 간단히 재작성해서 전송
  char out[MAXBUF];
  int n = 0;
  n += snprintf(out + n, sizeof(out) - n, "GET %s HTTP/1.0\r\n", path);
  n += snprintf(out + n, sizeof(out) - n, "Host: %s", host);

  if (strcmp(port, "80"))
  {
    n += snprintf(out + n, sizeof(out) - n, ":%s", port);
  }

  n += snprintf(out + n, sizeof(out) - n, "\r\n");
  n += snprintf(out + n, sizeof(out) - n, "%s", user_agent_hdr);
  n += snprintf(out + n, sizeof(out) - n, "Connection: close\r\n");
  n += snprintf(out + n, sizeof(out) - n, "Proxy-Connection: close\r\n");
  n += snprintf(out + n, sizeof(out) - n, "\r\n");
  Rio_writen(serverfd, out, n);

  // 응답을 그대로 릴레이
  rio_t srio;
  Rio_readinitb(&srio, serverfd);
  char buf[MAXBUF];
  ssize_t r;

  while ((r = Rio_readnb(&srio, buf, sizeof(buf))) > 0)
  {
    Rio_writen(clientfd, buf, r);
  }

  Close(serverfd);
}

void read_requesthdrs(rio_t *rp) // 요청 헤더를 읽어서 출력(실제 처리는 하지 않음)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE); // 첫 줄 읽기
  while (strcmp(buf, "\r\n"))      // 빈 줄이 나올 때까지 반복
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf); // 헤더 내용 출력
  }
  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs) // URI를 정적/동적으로 구분
{
  char *ptr;

  if (!strstr(uri, "cgi-bin")) // 정적 컨텐츠
  {
    strcpy(cgiargs, "");   // CGI 인자 없음
    strcpy(filename, "."); // 상대 경로 시작
    strcat(filename, uri); // ./index.html 형태

    if (uri[strlen(uri) - 1] == '/') // URI가 "/"로 끝나면 기본 페이지 home.html 제공
    {
      strcat(filename, "home.html");
    }

    return 1;
  }
  else // 동적 컨텐츠
  {
    ptr = index(uri, '?'); // CGI 인자 시작점 찾기

    if (ptr)
    {
      strcpy(cgiargs, ptr + 1); // "?" 이후로 인자로 저장
      *ptr = '\0';              // "?" 기준으로 URI 자르기
    }
    else
    {
      strcpy(cgiargs, ""); // 인자가 없는 경우
    }
    strcpy(filename, ".");
    strcat(filename, uri); // 실행할 CGI 파일 이름

    return 0;
  }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, // 에러 발생 시 HTML 에러 페이지 전송
                 char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  sprintf(body, "<html><title>Tiny Error</title>"); // 에러 메시지 body 구성
  sprintf(body, "%s<body bgcolor="
                "ffffff"
                ">\r\n",
          body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg); // HTTP 응답 전송
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

// 메인 스레드: Open_listenfd(port) → 무한루프 Accept() → 새 커넥션마다 스레드 생성
int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) // 포트 번호 확인
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);

  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);

    printf("Accepted connection from (%s, %s)\n", hostname, port);

    int *connfdp = malloc(sizeof(int));
    *connfdp = connfd;

    pthread_t tid;
    pthread_create(&tid, NULL, thread, connfdp);
  }

  printf("%s", user_agent_hdr);
  return 0;
}

// telnet localhost 4500
// GET http://localhost:8000/home.html HTTP/1.0
// Host: localhost:8000