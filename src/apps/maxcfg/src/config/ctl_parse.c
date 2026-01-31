/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * ctl_parse.c - CTL file parser for reading configuration values
 *
 * Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ctl_parse.h"
#include "prog.h"

/* Helper to trim leading/trailing whitespace */
static char *trim_whitespace(char *str)
{
    char *end;
    
    /* Trim leading space */
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) return str;
    
    /* Trim trailing space */
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    
    return str;
}

/* Helper to check if line starts with keyword (case-insensitive) */
static bool line_starts_with_keyword(const char *line, const char *keyword)
{
    size_t kw_len = strlen(keyword);
    
    /* Skip leading whitespace */
    while (isspace((unsigned char)*line)) line++;
    
    /* Compare keyword (case-insensitive) */
    if (strncasecmp(line, keyword, kw_len) != 0) {
        return false;
    }
    
    /* Must be followed by whitespace or end of string */
    const char *after = line + kw_len;
    return (*after == '\0' || isspace((unsigned char)*after));
}

/* Helper to extract value after keyword */
static char *extract_value_after_keyword(char *line, const char *keyword)
{
    size_t kw_len = strlen(keyword);
    
    /* Skip leading whitespace */
    while (isspace((unsigned char)*line)) line++;
    
    /* Skip keyword */
    line += kw_len;
    
    /* Skip whitespace after keyword */
    while (isspace((unsigned char)*line)) line++;
    
    return trim_whitespace(line);
}

bool ctl_parse_keyword_from_file(const char *ctl_path, const char *keyword, char *value_buf, size_t value_sz)
{
    if (!ctl_path || !keyword || !value_buf || value_sz == 0) {
        return false;
    }
    
    FILE *fp = fopen(ctl_path, "r");
    if (!fp) {
        return false;
    }
    
    char line[1024];
    bool found = false;
    bool in_system_section = false;
    bool in_session_section = false;
    
    while (fgets(line, sizeof(line), fp)) {
        /* Remove newline */
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        /* Track sections */
        if (strstr(line, "System Section") || strstr(line, "Begin System")) {
            in_system_section = true;
            in_session_section = false;
            continue;
        }
        if (strstr(line, "Session Section") || strstr(line, "Begin Session")) {
            in_session_section = true;
            in_system_section = false;
            continue;
        }
        if (strstr(line, "End System") || strstr(line, "End Session")) {
            in_system_section = false;
            in_session_section = false;
            continue;
        }
        
        /* Skip comments and empty lines */
        char *trimmed = trim_whitespace(line);
        if (trimmed[0] == '\0' || trimmed[0] == '%' || trimmed[0] == ';') {
            continue;
        }
        
        /* Check if this line matches our keyword */
        if (line_starts_with_keyword(trimmed, keyword)) {
            char *value = extract_value_after_keyword(trimmed, keyword);
            
            /* Copy value to buffer */
            strncpy(value_buf, value, value_sz - 1);
            value_buf[value_sz - 1] = '\0';
            
            found = true;
            break;
        }
    }
    
    fclose(fp);
    return found;
}

bool ctl_parse_boolean_from_file(const char *ctl_path, const char *keyword, bool *value)
{
    if (!ctl_path || !keyword || !value) {
        return false;
    }
    
    FILE *fp = fopen(ctl_path, "r");
    if (!fp) {
        return false;
    }
    
    char line[1024];
    bool found = false;
    *value = false;  /* Default to false if not found */
    
    while (fgets(line, sizeof(line), fp)) {
        /* Remove newline */
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        /* Skip comments and empty lines */
        char *trimmed = trim_whitespace(line);
        if (trimmed[0] == '\0' || trimmed[0] == '%' || trimmed[0] == ';') {
            continue;
        }
        
        /* Check if this line matches our keyword */
        if (line_starts_with_keyword(trimmed, keyword)) {
            *value = true;
            found = true;
            break;
        }
        
        /* Check for negated form (e.g., "No Snoop") */
        char negated[256];
        snprintf(negated, sizeof(negated), "No %s", keyword);
        if (line_starts_with_keyword(trimmed, negated)) {
            *value = false;
            found = true;
            break;
        }
    }
    
    fclose(fp);
    return found;
}
