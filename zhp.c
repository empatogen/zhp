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
#include <sqlite3.h>
#include <time.h>

#include <sys/types.h>

#include "zhp.h"

int cgi_path(char *path, int len)
{
	char *env;

	if ((env = getenv("PATH_INFO")) == NULL)
		return -1;

	snprintf(path, len, "%s", env);
	return 0;
}


int cgi_method(char *method, int len)
{
	char *env;

	if ((env = getenv("REQUEST_METHOD")) == NULL)
		return -1;

	snprintf(method, len, "%s", env);
	return 0;
}


void
http_error(int http_code)
{
	switch (http_code) {
		case 400:
			printf("Status: 400 Bad Request\r\n");
			printf("Content-type: text/html\r\n\r\n");
			printf("<!DOCTYPE html><html><body><h1>400 Bad Request</h1></body></html>");
			break;
		case 404:
			printf("Status: 404 Not Found\r\n");
			printf("Content-type: text/html\r\n\r\n");
			printf("<!DOCTYPE html><html><body><h1>404 Not Found</h1></body></html>");
			break;
		case 405:
			printf("Status: 405 Method Not Allowed\r\n");
			printf("Content-type: text/html\r\n\r\n");
			printf("<!DOCTYPE html><html><body><h1>405 Method Not Allowed</h1></body></html>");
			break;
		default:
			printf("Status: 500 'Internal Server Error'\r\n");
			printf("Content-type: text/html\r\n\r\n");
			printf("<!DOCTYPE html><html><body><h1>500 Internal Server Error</h1></body></html>");
	}
	exit(1);
}


size_t
http_date(time_t t, char *date, size_t len)
{
	struct tm *tm = gmtime(&t);
	return strftime(date, len, "%a, %d %b %Y %H:%M:%S GMT", tm);
}


void
response_headers(char *modified)
{
	printf("Status: 200 OK\r\n");
	printf("Last-Modified: %s\r\n", modified);
	printf("Content-Type: text/html\r\n");
	printf("\r\n");
}


int
fetch_page(char *path, struct template *t, struct page *p)
{
	sqlite3 *db;
	sqlite3_stmt *res;
	int rc;
	char *sql = "SELECT * FROM pages WHERE path=? LIMIT 1";

	rc = sqlite3_open_v2("./main.db", &db,
	  SQLITE_OPEN_READONLY | SQLITE_OPEN_SHAREDCACHE, NULL);
	if (rc != SQLITE_OK) {
		sqlite3_close(db);
		return -1;
	}

	if (sqlite3_prepare_v2(db, sql, -1, &res, 0) != SQLITE_OK) {
		sqlite3_close(db);
		return -1;
	}

	if (sqlite3_bind_text(res, 1, path, -1, NULL) != SQLITE_OK) {
		sqlite3_close(db);
		return -1;
	}

	if (sqlite3_step(res) != SQLITE_ROW)
		return -1;

	p->id = sqlite3_column_int(res, 0);
	p->created = sqlite3_column_int(res, 1);
	p->modified = sqlite3_column_int(res, 2);
	p->published = sqlite3_column_int(res, 3);
	p->norobots = sqlite3_column_int(res, 4);
	p->author.id = sqlite3_column_int(res, 5);
	snprintf(p->path, sizeof(p->path), "%s", sqlite3_column_text(res, 6));
	snprintf(p->template, sizeof(p->template), "%s", sqlite3_column_text(res, 7));
	snprintf(p->title, sizeof(p->title), "%s", sqlite3_column_text(res, 8));
	snprintf(p->keywords, sizeof(p->keywords), "%s", sqlite3_column_text(res, 9));
	snprintf(p->description, sizeof(p->description), "%s", sqlite3_column_text(res, 10));
	snprintf(p->content, sizeof(p->content), "%s", sqlite3_column_text(res, 11));

	sqlite3_finalize(res);
	sqlite3_close(db);

	return snprintf(t->file, sizeof(t->file), "tpls/%s.tpl", p->template);
}


size_t
str_replace(char **source, size_t len, char *search, char *replace)
{
	char *src;
	size_t relen = strlen(replace);
	size_t selen = strlen(search);
	size_t solen = strlen(*source);

	while ((src = strstr(*source, search)) != NULL) {
		if (len < solen + (relen - selen) + 1) {
			len *= 2;
			if ((*source = realloc(*source, len)) == NULL)
				return 0;
			src = strstr(*source, search);
		}
		memmove(
			src + relen,
			src + selen,
			(strlen(src) - selen) + 1
		);
		memcpy(src, replace, relen);
	}

	return len;
}


int
read_template(struct template *t)
{
	FILE *fh;

	if ((fh = fopen(t->file, "r")) == NULL) {
		printf("unable to open template: %s\n", t->file);
		return -1;
	}

	fseek(fh, 0L, SEEK_END);
	t->size = ftell(fh);
	rewind(fh);

	if ((t->data = malloc(t->size+1)) == NULL)
		return -1;

	memset(t->data, '\0', t->size+1);
	if (fread(t->data, t->size, 1, fh) == 0)
		return -1;
	fclose(fh);

	return 0;
}


size_t include_tag(char **body, size_t len, char *tag_start, char *tag_end)
{
	char *inc;
	char tag[64];
	char tpl[32];
	struct template t;
	int i;

	while ((inc = strstr(*body, tag_start)) != NULL) {
		inc += 11;
		for (i=0; inc[i] != '\'' && inc[i] != '\0' && i<32; i++) {
			tpl[i] = inc[i];
		}
		tpl[i] = '\0';

		if (snprintf(tag, sizeof(tag), "%s%s%s", tag_start, tpl, tag_end) == -1)
			http_error(500);
		if (snprintf(t.file, sizeof(t.file), "tpls/%s", tpl) == -1)
			http_error(500);
		if (read_template(&t) == -1) {
			len = str_replace(&(*body), len, tag, "");
			continue;
		}

		len = str_replace(&(*body), len, tag, t.data);
		free(t.data);
	}
	if (len == 0)
		http_error(500);
	return len;
}
