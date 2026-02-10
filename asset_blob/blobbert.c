#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "../rtypes.h"

// 64 utf-8 seems reasonable, or ascii?
//#define ASSET_NAME_MAX_LENGTH 64

// just go 0 to 27
typedef enum AssetType {
    // image/texture
    PNG, BMP, TGA, JPG, GIF,
    QOI, PSD, DDS, HDR, KTX,
    ASTC, PKM, PVR,
    // fonts
    FNT, TTF, OTF,
    // models/meshes
    OBJ, IQM, GLTF, VOX, M3D,
    // audio
    WAV, OGG, MP3, FLAC, XM, 
    MOD, QOA
} AssetType;


typedef struct AssetHeader {
    u32     magic;          // magic bytes for validity
    u16     version;        // version of the format?
    u16     count;          // number of assets
    u32     toc_offset;     // where table of contents starts
    u32     str_offset;     // where of string table starts
    u32     data_offset;    // where data starts
    u32     checksum;       // do we need this
    // padding
} AssetHeader;

// header length = 0..toc_offset - 1
// toc length = toc_offset..str_offset - 1
// data length = str_offset..end of file? 

typedef struct Toc {
    u32         start;
    u32         len;
    u32         name_loc;
    u8          type;
    // padding
    u8          _pad[3];
} Toc;

// hmm what is the correct approach to strings?
// fixed size? simplifies things tbh
// 
typedef struct StringTable {
    u16         len;
    char        *path;
} StringTable;


// of course this is just how to hold it in memory while constructing it
// the blob itself isn't made of pointers
typedef struct AssetBlob {
    AssetHeader     header;
    Toc             *toc;
    StringTable     *names;
    // or list of 
    // char *names[ASSET_NAME_MAX_LENGTH];
    u8              *assets;
} AssetBlob;

// directory ---------------------------------------------------------------- /
typedef struct FileList {
    char **paths;
    int count;
    int capacity;
} FileList;

void initFileList(FileList *list) {
    list->capacity = 16;
    list->count = 0;
    list->paths = malloc(list->capacity * sizeof(char *));
}

void addPath(FileList *list, const char *path) {
    if (list->count == list->capacity) {
        list->capacity *= 2;
        list->paths = realloc(list->paths, list->capacity * sizeof(char *));
    }
    list->paths[list->count] = strdup(path);
    list->count++;
}

void freeFileList(FileList *list) {
    for (int i = 0; i < list->count; i++) {
        free(list->paths[i]);
    }
    free(list->paths);
}

void collectFiles(const char *path, FileList *list) {
    struct dirent *de;
    DIR *dr = opendir(path);

    if (dr == NULL) {
        printf("Could not open directory: %s\n", path);
        return;
    }

    while ((de = readdir(dr)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;

        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, de->d_name);

        struct stat st;
        if (stat(fullPath, &st) == 0 && S_ISDIR(st.st_mode)) {
            collectFiles(fullPath, list);
        } else {
            addPath(list, fullPath);
        }
    }

    closedir(dr);
}

struct stat st;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Please enter a single directory path. ./cmd dir");
        return 1;
    }

    printf("Input directory is: %s\n", argv[1]);
    
    FileList files;
    
    initFileList(&files);
    collectFiles(argv[1], &files);

    int all_string_length = 0;
    int offset = 0;
    char* all_string = NULL;

    for (int i = 0; i < files.count; i++) {
        char* input_path = files.paths[i];
        int n = strlen(input_path) + 1;

        all_string_length += n;
        all_string = realloc(all_string, all_string_length);
        // should figure out diff between strncpy and memcpy
        memcpy(all_string + offset, input_path, n);
        offset += n;
        //i want to copy the string into the big string
        //basically a string blob, concantenated 
        //each file path null terminated
        printf("%s\n", input_path);
       

        stat(input_path, &st);
        u64 file_size = st.st_size;
        printf("%d\n", file_size);
        printf("test\n");


        //FILE* f = fopen(input_path, "r");
        //printf("%s\n", f);
        //fclose(f);
    }

    printf("%d\n", all_string_length);

    for (int i = 0; i < all_string_length; i++) {
        if (all_string[i] == '\0')
            printf("\\0");
        else
            printf("%c", all_string[i]);
    }
    printf("\n");

    freeFileList(&files);
    return 0;
}
