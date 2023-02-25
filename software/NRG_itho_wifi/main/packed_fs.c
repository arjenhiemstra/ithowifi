#if defined MG_ENABLE_PACKED_FS && MG_ENABLE_PACKED_FS == 1

#include <stddef.h>
#include <string.h>
#include <time.h>

#include "webroot/index_html_gz.h"
#include "webroot/edit_html_gz.h"
#include "webroot/controls_js_gz.h"
#include "webroot/pure_min_css_gz.h"
#include "webroot/zepto_min_js_gz.h"
#include "webroot/favicon_png_gz.h"
static const struct packed_file
{
    const char *name;
    const unsigned char *data;
    size_t size;
    time_t mtime;
} packed_files[] = {
    {"/web_root/", index_html_gz, sizeof(index_html_gz), 1660216320},
    {"/web_root/index.html.gz", index_html_gz, sizeof(index_html_gz), 1660216320},
    {"/web_root/controls.js.gz", controls_js_gz, sizeof(controls_js_gz), 1660216320},
    {"/web_root/edit.html.gz", edit_html_gz, sizeof(edit_html_gz), 1660216320},
    {"/web_root/favicon.png.gz", favicon_png_gz, sizeof(favicon_png_gz), 1660216320},
    {"/web_root/pure-min.css.gz", pure_min_css_gz, sizeof(pure_min_css_gz), 1660216320},
    {"/web_root/zepto.min.js.gz", zepto_min_js_gz, sizeof(zepto_min_js_gz), 1660216320},
    {NULL, NULL, 0, 0}};

static int scmp(const char *a, const char *b)
{
    while (*a && (*a == *b))
        a++, b++;
    return *(const unsigned char *)a - *(const unsigned char *)b;
}
const char *mg_unlist(size_t no);
const char *mg_unlist(size_t no)
{
    return packed_files[no].name;
}
const char *mg_unpack(const char *path, size_t *size, time_t *mtime);
const char *mg_unpack(const char *name, size_t *size, time_t *mtime)
{
    const struct packed_file *p;
    for (p = packed_files; p->name != NULL; p++)
    {
        if (scmp(p->name, name) != 0)
            continue;
        if (size != NULL)
            *size = p->size - 1;
        if (mtime != NULL)
            *mtime = p->mtime;
        return (const char *)p->data;
    }
    return NULL;
}

#endif //#if defined MG_ENABLE_PACKED_FS && MG_ENABLE_PACKED_FS == 1
