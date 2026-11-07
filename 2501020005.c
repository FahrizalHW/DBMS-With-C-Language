#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdint.h>

typedef struct {
    uint32_t nim;   
    uint32_t nama;  
} Rec;

static char  *arena = NULL;
static size_t arena_len = 0, arena_cap = 0;
static Rec   *recs = NULL;
static size_t n_rec = 0, rec_cap = 0;
static const char *path;
static int disk_needs_nl = 0;   

static void die(const char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
    exit(1);
}

static void load_file(void) {
    FILE *f = fopen(path, "rb");
    if (!f) {                                   
        f = fopen(path, "wb");
        if (!f) die("tidak bisa membuat file data.");
        fclose(f);
        printf("File %s belum ada, file baru dibuat.\n", path);
        arena_cap = 1 << 20;
        rec_cap = 1024;
        arena = malloc(arena_cap);
        recs = malloc(rec_cap * sizeof(Rec));
        if (!arena || !recs) die("memori tidak cukup.");
        return;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    if (sz < 0 || (unsigned long)sz > 0xF0000000UL) die("file terlalu besar (maks ~3,5 GB).");

    arena_cap = (size_t)sz + (1 << 20);
    arena = malloc(arena_cap);
    if (!arena) die("memori tidak cukup untuk memuat file.");
    arena_len = fread(arena, 1, (size_t)sz, f);
    fclose(f);

    if (arena_len > 0 && arena[arena_len - 1] != '\n') {
        arena[arena_len++] = '\n';
        disk_needs_nl = 1;
    }

    size_t lines = 0;
    char *p = arena, *end = arena + arena_len;
    while ((p = memchr(p, '\n', (size_t)(end - p))) != NULL) { lines++; p++; }

    rec_cap = lines + 1024;
    recs = malloc(rec_cap * sizeof(Rec));
    if (!recs) die("memori tidak cukup untuk index.");

    char *line = arena;
    while (line < end) {
        char *nl = memchr(line, '\n', (size_t)(end - line));
        if (!nl) break;
        char *e = nl;
        if (e > line && e[-1] == '\r') e--;
        *e = '\0';
        *nl = '\0';
        char *comma = memchr(line, ',', (size_t)(e - line));
        if (comma) {
            *comma = '\0';
            recs[n_rec].nim  = (uint32_t)(line - arena);
            recs[n_rec].nama = (uint32_t)(comma + 1 - arena);
            n_rec++;
        }
        line = nl + 1;
    }
    printf("Data dimuat: %zu record.\n", n_rec);
}

/* ---------- tambah ---------- */
static void add_record(const char *nim, const char *nama) {
    size_t need = strlen(nim) + strlen(nama) + 2;
    if (arena_len + need + 1 > arena_cap) {
        size_t nc = arena_cap * 2;
        while (nc < arena_len + need + 1) nc *= 2;
        if (nc > 0xF0000000UL) die("arena melebihi batas 4 GB.");
        char *t = realloc(arena, nc);
        if (!t) die("memori tidak cukup.");
        arena = t; arena_cap = nc;
    }
    if (n_rec == rec_cap) {
        rec_cap *= 2;
        Rec *t = realloc(recs, rec_cap * sizeof(Rec));
        if (!t) die("memori tidak cukup.");
        recs = t;
    }
    recs[n_rec].nim = (uint32_t)arena_len;
    memcpy(arena + arena_len, nim, strlen(nim) + 1);
    arena_len += strlen(nim) + 1;
    recs[n_rec].nama = (uint32_t)arena_len;
    memcpy(arena + arena_len, nama, strlen(nama) + 1);
    arena_len += strlen(nama) + 1;
    n_rec++;

    FILE *f = fopen(path, "ab");
    if (!f) { printf("Gagal menulis ke file.\n"); return; }
    if (disk_needs_nl) { fputc('\n', f); disk_needs_nl = 0; }
    fprintf(f, "%s,%s\n", nim, nama);
    fclose(f);
}

/* ---------- cari ---------- */
static int contains_ci(const char *h, const char *lo, size_t nl) {
    if (nl == 0) return 1;
    char first = lo[0];
    for (; *h; h++) {
        if ((char)tolower((unsigned char)*h) != first) continue;
        size_t i = 1;
        while (i < nl && h[i] && (char)tolower((unsigned char)h[i]) == lo[i]) i++;
        if (i == nl) return 1;
    }
    return 0;
}

static void do_search(int by_nim, const char *value) {
    clock_t t0 = clock();
    size_t vl = strlen(value);
    char *lo = malloc(vl + 1);
    if (!lo) die("memori tidak cukup.");
    for (size_t i = 0; i <= vl; i++) lo[i] = (char)tolower((unsigned char)value[i]);

    printf("Result:\n");
    size_t found = 0;
    for (size_t i = 0; i < n_rec; i++) {
        const char *nim = arena + recs[i].nim;
        const char *nama = arena + recs[i].nama;
        if (contains_ci(by_nim ? nim : nama, lo, vl)) {
            printf("%s %s\n", nim, nama);
            found++;
        }
    }
    free(lo);
    double sec = (double)(clock() - t0) / CLOCKS_PER_SEC;
    if (found == 0) printf("(tidak ditemukan)\n");
    printf("Total time %.3f seconds\n", sec);
}

/* ---------- util parsing ---------- */
static char *skip_sp(char *s) { while (*s == ' ' || *s == '\t') s++; return s; }
static void rtrim(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == ' ' || s[n-1] == '\t' || s[n-1] == '\r' || s[n-1] == '\n')) s[--n] = '\0';
}
static char *next_token(char **s) {         
    char *p = skip_sp(*s);
    if (!*p) { *s = p; return NULL; }
    char *start = p;
    while (*p && *p != ' ' && *p != '\t') p++;
    if (*p) *p++ = '\0';
    *s = p;
    return start;
}
static int all_digits(const char *s) {
    if (!*s) return 0;
    for (; *s; s++) if (!isdigit((unsigned char)*s)) return 0;
    return 1;
}

static void print_help(void) {
    printf("Available command:\n");
    printf("help - display this available\n");
    printf("berhenti - exit and return to Operating System\n");
    printf("cari xxxx yyyy - search xxxx by yyyyy value\n");
    printf("tambah xxxx yyyy - add new data xxxx and yyyyy\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Pemakaian: %s namafile.dat\n", argv[0]);
        return 1;
    }
    path = argv[1];
    setvbuf(stdout, NULL, _IOFBF, 1 << 16);
    load_file();

    char buf[2048];
    for (;;) {
        printf("Your command: ");
        fflush(stdout);
        if (!fgets(buf, sizeof buf, stdin)) break;        
        rtrim(buf);
        char *rest = buf;
        char *cmd = next_token(&rest);
        if (!cmd) continue;                                
        rtrim(rest);

        if (strcmp(cmd, "berhenti") == 0) break;
        else if (strcmp(cmd, "help") == 0) print_help();
        else if (strcmp(cmd, "tambah") == 0) {
            char *nim = next_token(&rest);
            char *nama = skip_sp(rest);
            if (!nim || !all_digits(nim) || !*nama) {
                printf("Parameter error. tambah xxxx yyyy where xxxx is number and yyy is string\n");
            } else {
                for (char *c = nama; *c; c++) if (*c == ',') *c = ' ';
                add_record(nim, nama);
            }
        }
        else if (strcmp(cmd, "cari") == 0) {
            char *field = next_token(&rest);
            char *val = skip_sp(rest);
            if (!field || !*val || (strcmp(field, "NIM") && strcmp(field, "NAMA"))) {
                printf("Syntax error. Should only\ncari xxx yyy\nWhere xxx is NIM or NAMA, and yyy is the search value\n");
            } else {
                do_search(strcmp(field, "NIM") == 0, val);
            }
        }
        else printf("Command not recognised. Please use help to display list of command.\n");
    }
    free(arena);
    free(recs);
    return 0;
}