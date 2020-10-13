#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "uthash.h"

char *str_cat(char *str1, char*str2) {
    char *new_str;
    if ((new_str = malloc(strlen(str1) + strlen(str2) + 1)) != NULL) {
        new_str[0] = '\0';   // ensures the memory is an empty string
        strcat(new_str, str1);
        strcat(new_str, str2);
    } else {
        fprintf(stderr, "%s\n", "malloc failed!");
        exit(1);
    }
    return new_str;
}

char *trim_whitespace(char *str) {
  char *end;

  while (isspace((unsigned char)*str)) str++;
  if (*str == 0) return str;
  end = str + strlen(str) - 1;
  while (end > str && isspace((unsigned char)*end)) end--;
  end[1] = '\0';

  return str;
}

char *substr(char *s, int n)  {
   char *new_s = malloc(sizeof(char) * n + 1);
   strncpy(new_s, s, n);
   new_s[n] = '\0';
   return new_s;
}

void print_array(int a[], int stop) {
    printf("%c", '[');
    int i = 0;
    while (a[i] != stop) {
        printf("%d", a[i]);
        if (a[++i] != stop) printf("%s", ", ");
    }
    printf("%c", ']');
}

struct Factor {
   char type[8];
   int out;
   int inp[2];
   UT_hash_handle hh;
};

struct Factor *factors = NULL;  // dict

void add_factor(struct Factor *factor) {
    HASH_ADD_INT(factors, out, factor);
}

struct Factor *find_factor(int out) {
    struct Factor *f;
    HASH_FIND_INT(factors, &out, f);
    return f;
}

struct Config  {
    char hash_algo[64];
    int difficulty;
    int num_input_bits;
    int num_factors;
    int observed_indices[4096];
};

struct Config parse_config(char *config_file) {
    FILE *fp;
    if ((fp = fopen(config_file, "r")) == NULL) {
        fprintf(stderr, "Error opening config file: %s\n", config_file);
        exit(1);
    }

    struct Config config;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    while ((read = getline(&line, &len, fp)) != -1) {
        char *key = strtok(line, ":");
        if (strcmp(key, "num_useful_factors") == 0) {
            config.num_factors = atoi(strtok(NULL, ":"));
        } else if (strcmp(key, "difficulty") == 0) {
            config.difficulty = atoi(strtok(NULL, ":"));
        } else if (strcmp(key, "hash") == 0) {
            strcpy(config.hash_algo, trim_whitespace(strtok(NULL, ";")));
        } else if (strcmp(key, "num_input_bits") == 0) {
            config.num_input_bits = atoi(strtok(NULL, ":"));
        } else if (strcmp(key, "observed_rv_indices") == 0) {
            int i = 0;
            (void)strtok(NULL, "[");
            char *s = strtok(NULL, ",");
            while (s != NULL) {
                char *trimmed = trim_whitespace(s);
                if (trimmed[strlen(trimmed) - 1] == ']') {
                    char *num = substr(trimmed, strlen(trimmed) - 1);
                    config.observed_indices[i++] = atoi(num);
                    free(num);
                } else {
                    config.observed_indices[i++] = atoi(trimmed);
                }
                s = strtok(NULL, ",");
            }
            config.observed_indices[i] = -1;
        }
    }

    fclose(fp);
    return config;
}

struct Factor *create_factor(char s[]) {
    struct Factor *factor;
    if ((factor = malloc(sizeof(struct Factor))) == NULL) {
        fprintf(stderr, "%s\n", "malloc failed!");
        exit(1);
    }

    strcpy(factor->type, strtok(s, ";"));
    factor->out = atoi(strtok(NULL, ";"));
    if (strcmp(factor->type, "INV") == 0) {
        factor->inp[0] = atoi(strtok(NULL, ";"));
        factor->inp[1] = -1;
    } else if (strcmp(factor->type, "SAME") == 0) {
        factor->inp[0] = atoi(strtok(NULL, ";"));
        factor->inp[1] = -1;
    } else if (strcmp(factor->type, "AND") == 0) {
        factor->inp[0] = atoi(strtok(NULL, ";"));
        factor->inp[1] = atoi(strtok(NULL, ";"));
    } else {
        factor->inp[0] = -1;
        factor->inp[1] = -1;
    }
    return factor;
}

void parse_factors(char *factor_file) {
    FILE *fp;
    if ((fp = fopen(factor_file, "r")) == NULL) {
        fprintf(stderr, "Error opening factor file: %s\n", factor_file);
        exit(1);
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    while ((read = getline(&line, &len, fp)) != -1) {
        struct Factor *factor = create_factor(line);
        add_factor(factor);
    }
    fclose(fp);
}

void print_factor(struct Factor *factor) {
    printf("%s: output %d", factor->type, factor->out);
    if (strcmp(factor->type, "INV") == 0) {
        printf(", input %d\n", factor->inp[0]);
    } else if (strcmp(factor->type, "SAME") == 0) {
        printf(", input %d\n", factor->inp[0]);
    } else if (strcmp(factor->type, "AND") == 0) {
        printf(", inputs %d, %d\n", factor->inp[0], factor->inp[1]);
    } else {
        printf("%s", "\n");
    }
}

void print_config(struct Config config) {
    printf("%s\n", "config file:");
    printf("\thash algorithm: %s\n", config.hash_algo);
    printf("\tdifficulty: %d\n", config.difficulty);
    printf("\tnumber of input bits: %d\n", config.num_input_bits);
    printf("\tnumber of factors: %d\n", config.num_factors);
    printf("\tobserved indices: ");
    print_array(config.observed_indices, -1);
    printf("%s", "\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "%s\n", "Dataset must be given as an argument");
        return 1;
    }

    // Derive the path to the factor file and config file
    char *factor_file;
    char *config_file;
    if (argv[1][strlen(argv[1]) - 1] == '/') {
        factor_file = str_cat(argv[1], "factors.txt");
        config_file = str_cat(argv[1], "params.yaml");
    } else {
        factor_file = str_cat(argv[1], "/factors.txt");
        config_file = str_cat(argv[1], "/params.yaml");
    }

    struct Config config = parse_config(config_file);
    print_config(config);
    parse_factors(factor_file);

    // Clean-up
    free(factor_file);
    free(config_file);

    printf("%s\n", "Done.");
    return 0;
}
