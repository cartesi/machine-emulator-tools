#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "yield-driver.h"

int main(int argc, char *argv[])
{
	float progress = 0.0;

	cmt_yield_driver_t Y[1];
	if ((argc > 1) && (sscanf(argv[1], "--progress=%f\n", &progress) == 1)) {
		if (cmt_yield_driver_init(Y))
			return EXIT_FAILURE;
		if (cmt_yield_driver_progress(Y, progress * 10))
			return EXIT_FAILURE;
		cmt_yield_driver_fini(Y);
	} else {
		fprintf(stderr, "usage: %s --progress=<float>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	return EXIT_SUCCESS;
}
