#include <stdio.h>
#include <stdlib.h>

int rekkesum(int n) {
  if (n == 1) return 1;
  return rekkesum(n-1) + n;
} 

int main(int argc, char **argv) {
  printf("%i\n", rekkesum(atoi(argv[1])));
	return 0;
}
