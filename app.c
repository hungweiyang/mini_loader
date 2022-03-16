#include <stdio.h>

int add5(int i)
{
  return i+5;
}

int add10(int i)
{
  i = add5(i);
  return add5(i);
}

const char *get_hello(void)
{
    return "Hello, world!";
}

void print_hello(void)
{
    printf("Hello, world by printf()!");
}