/*
 * Copyright (c) 2014 mes3hacklab
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> /* needed for off_t */

#define BLOCK_SIZE (64 * 1024)

/* needed to use fseeko and ftello */
#define _LARGEFILE_SOURCE

/* error types */
#define TOO_FEW_ARGS_ERROR          1
#define CANT_OPEN_FILE_ERROR        2
#define KEY_FILE_TOO_SMALL_ERROR    3

/* which file has an error */
#define ORIGIN_FILE_ERROR           1
#define KEY_FILE_ERROR              2
#define OUT_FILE_ERROR              3

char* origin_file_name;
char* key_file_name;
char* out_file_name;

void usage(void) {
    printf("Usage: otp ORIGIN_FILE KEY_FILE OUT_FILE\n");
}

void print_error_and_exit(int error_type, int error_file) {
    char* problem;
    char* problem_file_name;

    switch (error_type) {
    case CANT_OPEN_FILE_ERROR:
        problem = "can't open file";
        break;
    case KEY_FILE_TOO_SMALL_ERROR:
        problem = "key file too small";
        break;
    }

    switch (error_file) {
    case ORIGIN_FILE_ERROR:
        problem_file_name = origin_file_name;
        break;
    case KEY_FILE_ERROR:
        problem_file_name = key_file_name;
        break;
    case OUT_FILE_ERROR:
        problem_file_name = out_file_name;
        break;
    }

    fprintf(stderr, "Error: %s: %s\n", problem, problem_file_name);
    exit(error_type);
}

void parse_command_line(int argc, char** argv) {
    if (argc < 4) {
        usage();
        exit(TOO_FEW_ARGS_ERROR);
    }

    origin_file_name = argv[1];
    key_file_name = argv[2];
    out_file_name = argv[3];
}

void apply_pad(FILE* origin_file, FILE* key_file, FILE* out_file) {
    char* origin_buf = (char*)malloc(BLOCK_SIZE);
    char* key_buf = (char*)malloc(BLOCK_SIZE);
    char* res_buf = (char*)malloc(BLOCK_SIZE);

    do {
        int i;
        int origin_bytes_read = fread(origin_buf, 1, BLOCK_SIZE, origin_file);
        fread(key_buf, 1, origin_bytes_read, key_file);

        for(i = 0; i < origin_bytes_read; i++) {
            res_buf[i] = origin_buf[i] ^ key_buf[i];
        }

        fwrite(res_buf, 1, origin_bytes_read, out_file);
    } while(!feof(origin_file));

    free(origin_buf);
    free(key_buf);
    free(res_buf);
}

void delete_used_key_and_close(FILE* key_file, off_t bytes_to_delete) {
    FILE* temp_file = tmpfile();
    char* buffer = (char*)malloc(BLOCK_SIZE);

    fseeko(key_file, bytes_to_delete, SEEK_SET);

    do {
        int key_bytes_read = fread(buffer, 1, BLOCK_SIZE, key_file);
        fwrite(buffer, 1, key_bytes_read, temp_file);
    } while (!feof(key_file));

    fclose(key_file);
    key_file = fopen(key_file_name, "w");
    fseeko(temp_file, 0, SEEK_SET);

    do {
        int temp_bytes_read = fread(buffer, 1, BLOCK_SIZE, temp_file);
        fwrite(buffer, 1, temp_bytes_read, key_file);
    } while (!feof(temp_file));

    fclose(temp_file);
    fclose(key_file);

    free(buffer);
}

int main(int argc, char** argv) {
    /* file pointers */
    FILE* origin_file;
    FILE* key_file;
    FILE* out_file;

    /* file sizes */
    off_t origin_file_size;
    off_t key_file_size;

    /* error codes */
    int error_code = 0;
    int error_file;

    parse_command_line(argc, argv);

    origin_file = fopen(origin_file_name, "r");
    key_file = fopen(key_file_name, "r");
    out_file = fopen(out_file_name, "w");

    if (origin_file == NULL || key_file == NULL || out_file == NULL) {
        error_code = CANT_OPEN_FILE_ERROR;
        if (origin_file == NULL) {
            error_file = ORIGIN_FILE_ERROR;
        } else if (key_file == NULL) {
            error_file = KEY_FILE_ERROR;
        } else if (out_file == NULL) {
            error_file = OUT_FILE_ERROR;
        }

        if(origin_file != NULL) {
            fclose(origin_file);
        }
        if(key_file != NULL) {
            fclose(key_file);
        }
        if(out_file != NULL) {
            fclose(out_file);
        }

        print_error_and_exit (error_code, error_file);
    }

    /* check if keyfile is big enough */
    fseeko(origin_file, 0, SEEK_END);
    fseeko(key_file, 0, SEEK_END);
    origin_file_size = ftello(origin_file);
    key_file_size = ftello(key_file);

    if (key_file_size < origin_file_size) {
        fclose(origin_file);
        fclose(key_file);
        fclose(out_file);

        print_error_and_exit(KEY_FILE_TOO_SMALL_ERROR, KEY_FILE_ERROR);
    }

    /* restore files' positions */
    fseeko(origin_file, 0, SEEK_SET);
    fseeko(key_file, 0, SEEK_SET);

    /* encrypt/decrypt */
    apply_pad(origin_file, key_file, out_file);

    /* delete the used part of the key */
    delete_used_key_and_close(key_file, origin_file_size);

    /* finally close files */
    fclose(origin_file);
    fclose(out_file);
}

