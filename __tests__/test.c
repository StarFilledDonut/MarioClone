#include <stdio.h>

typedef struct {
  char *name;
  u_short age, life, defense, attack;
} Character;

int main(void) {
  Character main = { "Kelly", 16, 5, 8, 4 };
  u_short arr[] = { 1, 2, 3, 4, 5 };
  printf("Name of the character is: %s", main.name);
  for (u_short i = 0; i < sizeof(arr) / sizeof(arr[0]); i++) {
    printf("%hu\n", arr[i]);
  }
  return 1;
}
