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

#include <stdbool.h> 

struct template {
	char file[32];
	char *data;
	size_t size;
};

struct user {
  int id;
  char *username;
  char *password;
  char *nickname;
};

struct page {
  int id;
  time_t created;
  time_t modified;
  bool published;
  bool norobots;
  struct user author;
  char path[64];
  char template[32];
  char title[64];
  char keywords[64];
  char description[256];
  char content[4096];
};

size_t http_date(time_t, char *, size_t);

int cgi_path(char *, int len);
int cgi_method(char *, int len);
int allowed_method(char *, int len);
void http_error(int);
void http_redirect(char *);
void response_headers(char *);
int fetch_page(char *, struct template *, struct page *);
int read_template(struct template *);
int str_replace(char **, size_t *, char *, char *);
int include_tag(char **, size_t *, char *, char *);
