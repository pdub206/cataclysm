#include "conf.h"
#include "sysdep.h"
#include "toml_utils.h"

static void toml_write_escaped(FILE *fp, const char *value)
{
  const unsigned char *p = (const unsigned char *)value;

  while (p && *p) {
    switch (*p) {
    case '\\':
      fputs("\\\\", fp);
      break;
    case '"':
      fputs("\\\"", fp);
      break;
    case '\n':
      fputs("\\n", fp);
      break;
    case '\r':
      fputs("\\r", fp);
      break;
    case '\t':
      fputs("\\t", fp);
      break;
    default:
      if (*p < 0x20) {
        fprintf(fp, "\\u%04x", (unsigned int)*p);
      } else {
        fputc(*p, fp);
      }
      break;
    }
    p++;
  }
}

void toml_write_string(FILE *fp, const char *value)
{
  fputc('"', fp);
  if (value && *value)
    toml_write_escaped(fp, value);
  fputc('"', fp);
}

void toml_write_kv_string(FILE *fp, const char *key, const char *value)
{
  fprintf(fp, "%s = ", key);
  toml_write_string(fp, value);
  fputc('\n', fp);
}

void toml_write_kv_string_opt(FILE *fp, const char *key, const char *value)
{
  if (!value || !*value)
    return;
  toml_write_kv_string(fp, key, value);
}
