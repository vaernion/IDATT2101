#include <stdio.h>

void hanoi(int n, char fra, char hjelp, char til) {
  if (n <= 1) {
    printf("%c->%c\n", fra, til);
  } 
  else {
    hanoi(n-1, fra, til, hjelp);
    printf("%c->%c\n", fra, til);
    hanoi(n-1, hjelp, fra, til);
  }
}

int main (int argc, char **argv) {
  int n = atoi(argv[1]);
  printf("Tårnet i Hanoi\n.");
  printf("Flytte ringene fra pinne A til pinne C, ved hjelp av pinne B:\n");
  hanoi(n, 'A', 'B', 'C');
}
