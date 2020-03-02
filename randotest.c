#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>

long getMicrotime(){
	struct timeval currentTime;
	gettimeofday(&currentTime, NULL);
	return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
} 

int main(int argc, char **argv)
{
  if (argc < 2)
    {
      printf("USAGE: %s loop-iterations\n", argv[0]);
      return 1;
    }

  int iterations = atoi(argv[1]);

  long start, end;

  start = getMicrotime();

  for (int i = 0; i < iterations; i++)
    {
    }

  end = getMicrotime();

  printf("Start: %d End: %d\n", start, end);
  printf("%ld\n", (end - start));

  return 0;
}