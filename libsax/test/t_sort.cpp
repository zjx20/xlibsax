#include <stdio.h>

#include <sax/c++/quicksort.h>

int compare (const void * a, const void * b, void *user)
{
  return ( *(int*)a - *(int*)b );
}

void test1()
{
  int values[] = { 40, 10, 100, 90, 20, 25 };
  qsort2(values, 6, sizeof(int), compare, 0);
  for (int n=0; n<6; n++)
     printf ("%d ",values[n]);
  printf("\n\n");
}

void test2()
{
  int values[] = { 40, 10, 100, 90, 20, 25 };
  sax::quicksort<int>(values, 6, false);
  for (int n=0; n<6; n++)
     printf ("%d ",values[n]);
  printf("\n\n");
}

void test3()
{
  int values[] = { 40, 10, 100, 90, 20, 25 };
  size_t id[6], n;
  sax::quicksort<int>(values, id, 6);
  for (n=0; n<6; n++)
     printf ("[%d:%d] ", id[n], values[id[n]]);
  printf("\n\n");
}

int main ()
{
  test3();
  test2();
  test1();
  return 0;
}

