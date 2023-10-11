#include <stdio.h>
#include <stdlib.h>
#include <libcmt/keccak.h>

int main(int argc, char *argv[])
{
	size_t n = 64;
	if (argc > 1)
		sscanf(argv[1], "--n=%zu", &n);

	printf("/** Precomputed merkle roots for various tree heights */\n"
	       "#ifndef CMT_MERKLE_TABLE_H\n"
	       "#define CMT_MERKLE_TABLE_H\n"
	       "#include <libcmt/merkle.h>\n");
	printf("const uint8_t cmt_merkle_table[CMT_MERKLE_MAX_DEPTH][CMT_KECCAK_LENGTH] = {\n");

	uint8_t md[CMT_KECCAK_LENGTH] = {0};
	cmt_keccak_data(0, NULL, md);

	for (size_t i=0; i<n; ++i) {
		printf("\t{");
		for (int i=0; i<CMT_KECCAK_LENGTH; ++i) {
			printf("0x%02x%s", md[i], i+1 == CMT_KECCAK_LENGTH? "" : ",");
		}
		printf("},\n");

		CMT_KECCAK_DECL(c);
		cmt_keccak_update(c, CMT_KECCAK_LENGTH, md);
		cmt_keccak_update(c, CMT_KECCAK_LENGTH, md);
		cmt_keccak_final(c, md);
	}
	printf("};\n");
	printf("#endif /* CMT_MERKLE_TABLE_H */\n");

	return 0;
}
