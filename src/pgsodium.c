#include "pgsodium.h"

PG_MODULE_MAGIC;

bytea* pgsodium_secret_key;

void _PG_init(void)
{
	FILE* fp;
	char* secret_buf;
	size_t secret_len = 0;
	size_t char_read;
	char* path;

	char sharepath[MAXPGPATH];

	if (sodium_init() == -1)
	{
		elog(ERROR, "_PG_init: sodium_init() failed cannot initialize pgsodium");
		return;
	}

	if (process_shared_preload_libraries_in_progress)
	{
		get_share_path(my_exec_path, sharepath);
		path = (char*) palloc(MAXPGPATH);
		snprintf(
			path,
			MAXPGPATH,
			"%s/extension/%s",
			sharepath,
			PG_GETKEY_EXEC);

		if (access(path, F_OK) == -1)
			return;

		if ((fp = popen(path, "r")) == NULL)
		{
			fprintf(stderr,
					"%s: could not launch shell command from\n",
					path);
			proc_exit(1);
		}

		char_read = getline(&secret_buf, &secret_len, fp);
		if (secret_buf[char_read-1] == '\n')
			secret_buf[char_read-1] = '\0';

		secret_len = strlen(secret_buf);

		if (secret_len != 64)
		{
			fprintf(stderr, "invalid secret key\n");
			proc_exit(1);
		}

		if (pclose(fp) != 0)
		{
			fprintf(stderr, "%s: could not close shell command\n",
					PG_GETKEY_EXEC);
			proc_exit(1);
		}
		pgsodium_secret_key = palloc(crypto_sign_SECRETKEYBYTES + VARHDRSZ);
		hex_decode(secret_buf, secret_len, VARDATA(pgsodium_secret_key));
		sodium_mlock(pgsodium_secret_key, crypto_sign_SECRETKEYBYTES + VARHDRSZ);
		memset(secret_buf, 0, secret_len);
		free(secret_buf);
	}
}
