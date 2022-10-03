
#ifndef LIBROXML_CONFIG_H
#define LIBROXML_CONFIG_H

#define CONFIG_XML_THREAD_SAFE		1
#define CONFIG_XML_SMALL_INPUT_FILE	1

#define CONFIG_XML_FLOAT		1
#define CONFIG_XML_SMALL_BUFFER		1

#define FILE	int

#define fopen(...) (0);
#define fdopen(...) (0);
#define fwrite(...) do { } while(0);
#define fflush(...) do { } while(0);
#define fseek(...) (0)
#define ftell(...) (0)
#define fread(...) (0)
#define fwrite(...) do { } while(0);
#define fclose(...) do { } while(0);
#define dup(...) (0);
#define vfprintf(...) do { } while(0);

#endif /* LIBROXML_CONFIG_H */

