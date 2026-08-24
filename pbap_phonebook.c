#define _GNU_SOURCE
#include "pbap_phonebook.h"
#include "state.h"
#include "app_log.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int join_path_checked(char *dst,size_t dst_size,const char *a,const char *b){
    if(!dst||dst_size==0||!a||!b)return -1;
    size_t al=strlen(a),bl=strlen(b);
    size_t sep=(al>0&&a[al-1]!='/')?1:0;
    if(al+sep+bl+1>dst_size){dst[0]='\0';return -1;}
    memcpy(dst,a,al);
    size_t pos=al;
    if(sep)dst[pos++]='/';
    memcpy(dst+pos,b,bl);
    dst[pos+bl]='\0';
    return 0;
}

static char phonebook_dir[512];

static int ensure_dir(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0)
        return S_ISDIR(st.st_mode) ? 0 : -ENOTDIR;
    if (mkdir(path, 0755) == 0 || errno == EEXIST)
        return 0;
    return -errno;
}

static int ensure_phonebook_dirs(void)
{
    const char *home = getenv("HOME");
    if (!home || !home[0]) home = "/home/ark";

    char base[512], telecom[512];
    snprintf(base, sizeof(base), "%s/phonebook", home);
    if(join_path_checked(telecom,sizeof(telecom),base,"telecom")!=0)return -1;
    if(join_path_checked(phonebook_dir,sizeof(phonebook_dir),telecom,"pb")!=0)return -1;

    int rc = ensure_dir(base); if (rc < 0) return rc;
    rc = ensure_dir(telecom); if (rc < 0) return rc;
    return ensure_dir(phonebook_dir);
}

static void cleanup_old_vcards(void)
{
    DIR *d = opendir(phonebook_dir);
    if (!d) return;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.') continue;
        size_t n = strlen(e->d_name);
        if (n < 5 || strcmp(e->d_name + n - 4, ".vcf")) continue;
        char path[640];
        snprintf(path, sizeof(path), "%s/%s", phonebook_dir, e->d_name);
        unlink(path);
    }
    closedir(d);
}

static void vcard_escape(const char *src, char *dst, size_t dst_size)
{
    size_t n = 0;
    if (!src || !dst || dst_size == 0) return;
    for (; *src && n + 1 < dst_size; src++) {
        const char *rep = NULL;
        char c = *src;
        if (c == '\\') rep = "\\\\";
        else if (c == ';') rep = "\\;";
        else if (c == ',') rep = "\\,";
        else if (c == '\r' || c == '\n') rep = "\\n";
        if (rep) {
            for (const char *p = rep; *p && n + 1 < dst_size; p++) dst[n++] = *p;
        } else dst[n++] = c;
    }
    dst[n] = '\0';
}

static int write_vcard(unsigned int id, const char *name)
{
    char path[640], escaped[1024];
    snprintf(path, sizeof(path), "%s/%u.vcf", phonebook_dir, id);
    vcard_escape(name, escaped, sizeof(escaped));

    FILE *fp = fopen(path, "w");
    if (!fp) return -errno;
    fprintf(fp,
        "BEGIN:VCARD\r\n"
        "VERSION:3.0\r\n"
        "N:%s;;;;\r\n"
        "FN:%s\r\n"
        "TEL;TYPE=CELL:%u\r\n"
        "END:VCARD\r\n",
        escaped, escaped, id);
    if (fclose(fp) != 0) return -errno;
    chmod(path, 0644);
    return 0;
}

int pbap_phonebook_sync(char book_names[][256], char book_paths[][512], int book_count)
{
    int rc = ensure_phonebook_dirs();
    if (rc < 0) {
        app_logf("PBAP Telefonbuch: Verzeichnisfehler %d", rc);
        return rc;
    }

    cleanup_old_vcards();
    int written = 0;
    for (int i = 0; i < book_count; i++) {
        unsigned int id = ensure_book_dial_id(book_paths[i]);
        if (id < 1001) continue;
        rc = write_vcard(id, book_names[i]);
        if (rc < 0) {
            app_logf("PBAP vCard Fehler: %u %s (%d)", id, book_names[i], rc);
            continue;
        }
        written++;
    }
    app_logf("PBAP Telefonbuch: %d vCards in %s", written, phonebook_dir);
    return written;
}

const char *pbap_phonebook_directory(void)
{
    return phonebook_dir;
}
