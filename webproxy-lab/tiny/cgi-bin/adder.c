/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void)
{
  char *buf, *p;                                                              // QUERY_STRING을 가리키는 포인터, 문자열 파싱용 포인터
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];                        // 첫 번째/두 번째 인자 문자열 저장, 최종 HTML 응답 내용
  int n1 = 0, n2 = 0;                                                         // 변환된 정수 값

  /* Extract the two arguments */
  if ((buf = getenv("QUERY_STRING")) != NULL)                                 // QUERY_STRING 가져오기 (예: "num1=3&num2=5")
  {
    p = strchr(buf, '&');                                                     // '&' 위치 찾기 (num1=3&num2=5 → '&' 기준으로 분리)
    *p = '\0';                                                                // '&'를 '\0'으로 바꿔 문자열을 두 부분으로 나눔
    strcpy(arg1, buf);
    strcpy(arg2, p + 1);
    n1 = atoi(strchr(arg1, '=') + 1);
    n2 = atoi(strchr(arg2, '=') + 1);
  }

  /* Make the response body */
  sprintf(content, "QUERY_STRING=%s\r\n<p>", buf);                             // 원래의 QUERY_STRING 값 출력
  sprintf(content + strlen(content), "Welcome to add.com: ");
  sprintf(content + strlen(content), "THE Internet addition portal.\r\n<p>");
  sprintf(content + strlen(content), "The answer is: %d + %d = %d\r\n<p>",
          n1, n2, n1 + n2);                                                    // 실제 덧셈 결과 출력
  sprintf(content + strlen(content), "Thanks for visiting!\r\n");

  /* Generate the HTTP response */
  printf("Content-type: text/html\r\n");                                       // HTTP 헤더: 콘텐츠 타입
  printf("Content-length: %d\r\n", (int)strlen(content));                      // HTTP 헤더: 콘텐츠 길이
  printf("\r\n");                                                              // 헤더와 본문 사이 공백 라인
  printf("%s", content);                                                       // 본문 출력 (HTML)
  fflush(stdout);                                                              // 출력 버퍼 비우기
  
  exit(0);
}
/* $end adder */
