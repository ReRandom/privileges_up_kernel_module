#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{
	printf("pid: %d\n", getpid());
	while(1)
	{
		FILE* f = fopen("/etc/shadow", "r");
		if(f)
		{
			printf("ok\n");
			fclose(f);
			return 0;
		}
		sleep(1);
	}
}
