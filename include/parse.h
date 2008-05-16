#ifndef _parse_h
#define _parse_h

#include <expat.h>
#include "main.h"
#include "Book.h"

typedef enum {
	TAG_ANCHOR,
	TAG_BR,TAG_BODY,
	TAG_DIV,TAG_DT,
	TAG_H1,TAG_H2,TAG_H3,TAG_H4,TAG_H5,TAG_H6,TAG_HTML,TAG_HEAD,
	TAG_NONE,
	TAG_OL,
	TAG_P,TAG_PRE,
	TAG_SCRIPT,TAG_STYLE,
	TAG_TD,TAG_TITLE,
	TAG_STRONG,TAG_EM,
	TAG_UL,TAG_UNKNOWN
} context_t;

typedef struct {
	context_t stack[16];
	u8 stacksize;
	Book *book;
	page_t *page;
	FT_Vector pen;
} parsedata_t;

void default_hndl(void *data, const char *s, int len);
void start_hndl(void *data, const char *el, const char **attr);
void char_hndl(void *data, const char *txt, int txtlen);
void end_hndl(void *data, const char *el);
void proc_hndl(void *data, const char *target, const char *pidata);
int unknown_hndl(void *encodingHandlerData,
					const XML_Char *name,
					XML_Encoding *info);
void prefs_start_hndl(void *data, const char *el, const char **attr);
void title_start_hndl(void *data, const char *el, const char **attr);
void title_char_hndl(void *data, const char *txt, int txtlen);
void title_end_hndl(void *data, const char *el);

bool iswhitespace(u8 c);

void parse_init(parsedata_t *data);
void parse_printerror(XML_Parser p);
bool parse_pagefeed(parsedata_t *data, page_t *page);

#endif
