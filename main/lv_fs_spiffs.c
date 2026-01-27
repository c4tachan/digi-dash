#include "lvgl.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    FILE *fp;
} spiffs_file_t;

static void * spiffs_open_cb(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode)
{
    LV_UNUSED(drv);
    char fullpath[256];
    snprintf(fullpath, sizeof(fullpath), "/spiffs%s", path);

    const char *fmode = (mode == LV_FS_MODE_WR) ? "wb" : (mode == LV_FS_MODE_RD) ? "rb" : "rb+";
    FILE *fp = fopen(fullpath, fmode);
    if (!fp) return NULL;
    spiffs_file_t *f = lv_malloc(sizeof(spiffs_file_t));
    if (!f) {
        fclose(fp);
        return NULL;
    }
    f->fp = fp;
    return f;
}

static lv_fs_res_t spiffs_close_cb(lv_fs_drv_t *drv, void *file_p)
{
    LV_UNUSED(drv);
    spiffs_file_t *f = (spiffs_file_t *)file_p;
    if (f && f->fp) fclose(f->fp);
    lv_free(f);
    return LV_FS_RES_OK;
}

static lv_fs_res_t spiffs_read_cb(lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br)
{
    LV_UNUSED(drv);
    spiffs_file_t *f = (spiffs_file_t *)file_p;
    if (!f->fp) return LV_FS_RES_UNKNOWN;
    size_t r = fread(buf, 1, btr, f->fp);
    if (br) *br = (uint32_t)r;
    if (r < btr && ferror(f->fp)) return LV_FS_RES_UNKNOWN;
    return LV_FS_RES_OK;
}

static lv_fs_res_t spiffs_seek_cb(lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence)
{
    LV_UNUSED(drv);
    spiffs_file_t *f = (spiffs_file_t *)file_p;
    int w = (whence == LV_FS_SEEK_SET) ? SEEK_SET : (whence == LV_FS_SEEK_CUR) ? SEEK_CUR : SEEK_END;
    if (fseek(f->fp, (long)pos, w) != 0) return LV_FS_RES_UNKNOWN;
    return LV_FS_RES_OK;
}

static lv_fs_res_t spiffs_tell_cb(lv_fs_drv_t *drv, void *file_p, uint32_t *pos)
{
    LV_UNUSED(drv);
    spiffs_file_t *f = (spiffs_file_t *)file_p;
    long p = ftell(f->fp);
    if (p < 0) return LV_FS_RES_UNKNOWN;
    if (pos) *pos = (uint32_t)p;
    return LV_FS_RES_OK;
}

void lv_fs_spiffs_init(void)
{
    static lv_fs_drv_t drv;
    lv_fs_drv_init(&drv);
    drv.letter = 'S';
    drv.open_cb = spiffs_open_cb;
    drv.close_cb = spiffs_close_cb;
    drv.read_cb = spiffs_read_cb;
    drv.seek_cb = spiffs_seek_cb;
    drv.tell_cb = spiffs_tell_cb;
    lv_fs_drv_register(&drv);
}
