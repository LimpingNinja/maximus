/*
 * ctl_parse.c — Legacy .CTL file parser
 *
 * Copyright 2026 by Kevin Morgan.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ctl_parse.h"
#include "prog.h"

/** @brief Trim leading and trailing whitespace in-place. */
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

/** @brief Check if a line starts with the given keyword (case-insensitive). */
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

/** @brief Extract the value portion following a keyword on a line. */
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

/**
 * @brief Search a CTL file for a keyword and extract its value.
 *
 * @param ctl_path   Path to the .CTL file.
 * @param keyword    Keyword to search for (case-insensitive).
 * @param value_buf  Buffer to receive the value string.
 * @param value_sz   Size of value_buf.
 * @return true if the keyword was found.
 */
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

/**
 * @brief Search a CTL file for a boolean keyword (presence = true, "No" prefix = false).
 *
 * @param ctl_path  Path to the .CTL file.
 * @param keyword   Keyword to search for.
 * @param value     Receives the boolean result.
 * @return true if the keyword or its negation was found.
 */
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
