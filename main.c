#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "zhp.h"

int
main(void)
{
	struct template t;
	struct page p;

	char path[1024];
	char method[8];
	char modified[32];

	if (pledge("stdio rpath flock unveil", NULL) == -1)
		err(1, "pledge");

	if (unveil("./tpls", "r") == -1)
		err(1, "unveil ./tpls");
	if (unveil("./main.db", "r") == -1)
		err(1, "unveil ./main.db");
	if (unveil(NULL, NULL) == -1)
		err(1, "unveil");

	if (cgi_path(path, sizeof(path)) == -1)
		http_error(400);

	if (cgi_method(method, sizeof(method)) == -1)
		http_error(400);

	if (fetch_page(path, &t, &p) == -1)
		http_error(404);

	if (read_template(&t) == -1)
		http_error(404);

	t.size = include_tag(&t.data, t.size, "{{include('", "')}}");

	str_replace(&t.data, t.size, "{{path}}", p.path);
	str_replace(&t.data, t.size, "{{title}}", p.title);
	str_replace(&t.data, t.size, "{{keywords}}", p.keywords);
	str_replace(&t.data, t.size, "{{description}}", p.description);
	str_replace(&t.data, t.size, "{{content}}", p.content);

	memset(modified, 0, sizeof(modified));
	http_date(p.modified, modified, sizeof(modified));
	str_replace(&t.data, t.size, "{{modified}}", modified);

	response_headers(modified);
	if (strncmp(method, "HEAD", sizeof(method)) != 0)
		printf("%s", t.data);

	free(t.data);
	return 0;
}
