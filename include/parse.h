#ifndef _PARSE_H_
#define _PARSE_H_

void default_hndl(void *data, const char *s, int len);
void start_hndl(void *data, const char *el, const char **attr);
void title_hndl(void *data, const char *txt, int txtlen);
void char_hndl(void *data, const char *txt, int txtlen);
void end_hndl(void *data, const char *el);
void proc_hndl(void *data, const char *target, const char *pidata);
int unknown_hndl(void *encodingHandlerData,
		 const XML_Char *name,
		 XML_Encoding *info);

bool iswhitespace(u8 c);

#endif
