/*
 * Copyright (c) 2022 Jesper Wallin <jesper@ifconfig.se>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

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
