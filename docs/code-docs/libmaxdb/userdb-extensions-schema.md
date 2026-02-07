# User Database Extensions Schema

## Overview
Extensions to `user.db` for enhanced user tracking, preferences, achievements, and session management.

## Schema Version
- Base schema (users table): v1
- Extensions: v2 (adds 5 new tables)

## New Tables

### 1. caller_sessions
Replaces `callers.bbs` with structured session history.

```sql
CREATE TABLE IF NOT EXISTS caller_sessions (
    session_id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    
    -- Session timing
    login_time INTEGER NOT NULL,           -- Unix timestamp
    logoff_time INTEGER,                   -- Unix timestamp (NULL if still connected)
    
    -- Session context
    task INTEGER NOT NULL,                 -- Node number
    flags INTEGER DEFAULT 0,               -- CALL_* flags from callinfo.h
    
    -- User state at login/logoff
    logon_priv INTEGER,
    logoff_priv INTEGER,
    logon_xkeys INTEGER,
    logoff_xkeys INTEGER,
    
    -- Activity counters
    calls INTEGER DEFAULT 0,               -- User's call count at login
    files_up INTEGER DEFAULT 0,
    files_dn INTEGER DEFAULT 0,
    kb_up INTEGER DEFAULT 0,
    kb_dn INTEGER DEFAULT 0,
    msgs_read INTEGER DEFAULT 0,
    msgs_posted INTEGER DEFAULT 0,
    paged INTEGER DEFAULT 0,
    time_added INTEGER DEFAULT 0,          -- Minutes added during session
    
    -- Metadata
    created_at INTEGER NOT NULL DEFAULT (unixepoch()),
    
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_caller_sessions_user 
    ON caller_sessions(user_id, login_time DESC);
CREATE INDEX IF NOT EXISTS idx_caller_sessions_time 
    ON caller_sessions(login_time DESC);
```

### 2. user_preferences
Extensible key-value store for user settings beyond core fields.

```sql
CREATE TABLE IF NOT EXISTS user_preferences (
    pref_id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    
    -- Preference data
    pref_key TEXT NOT NULL,                -- e.g., "theme.color", "hotkey.f1", "filter.msg_from"
    pref_value TEXT,                       -- JSON or simple string value
    pref_type TEXT DEFAULT 'string',       -- 'string', 'int', 'bool', 'json'
    
    -- Metadata
    created_at INTEGER NOT NULL DEFAULT (unixepoch()),
    updated_at INTEGER NOT NULL DEFAULT (unixepoch()),
    
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    UNIQUE(user_id, pref_key)
);

CREATE INDEX IF NOT EXISTS idx_user_preferences_lookup 
    ON user_preferences(user_id, pref_key);
```

**Example preferences:**
- `saved_file_list.downloads`: JSON array of tagged files
- `message_filter.ignore_from`: Comma-separated user names
- `door_game.lord.save_slot`: Game save data
- `theme.ansi_colors`: Custom color scheme
- `hotkey.custom.f10`: Custom menu command

### 3. achievement_types
Meta-table defining available achievements.

```sql
CREATE TABLE IF NOT EXISTS achievement_types (
    achievement_type_id INTEGER PRIMARY KEY AUTOINCREMENT,
    
    -- Achievement definition
    badge_code TEXT NOT NULL UNIQUE,       -- e.g., "FIRST_POST", "100_UPLOADS", "VETERAN_1Y"
    badge_name TEXT NOT NULL,              -- Display name: "First Post"
    badge_desc TEXT,                       -- Description: "Posted your first message"
    
    -- Award criteria
    threshold_type TEXT NOT NULL,          -- 'count', 'tenure', 'special'
    threshold_value INTEGER,               -- For count/tenure types
    
    -- Display
    badge_icon TEXT,                       -- ANSI art or emoji
    badge_color INTEGER,                   -- ANSI color code
    display_order INTEGER DEFAULT 0,
    
    -- Metadata
    enabled INTEGER DEFAULT 1,
    created_at INTEGER NOT NULL DEFAULT (unixepoch())
);

CREATE UNIQUE INDEX IF NOT EXISTS idx_achievement_types_code 
    ON achievement_types(badge_code);
```

**Threshold types:**
- `count`: Numeric threshold (e.g., 100 uploads)
- `tenure`: Time-based (e.g., 365 days member)
- `special`: Logic-based (e.g., "Uploaded on Christmas")

### 4. user_achievements
User-earned achievements.

```sql
CREATE TABLE IF NOT EXISTS user_achievements (
    achievement_id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    achievement_type_id INTEGER NOT NULL,
    
    -- Award details
    earned_at INTEGER NOT NULL DEFAULT (unixepoch()),
    metadata_json TEXT,                    -- Optional: context about how earned
    
    -- Display
    displayed INTEGER DEFAULT 0,           -- Has user seen notification?
    pinned INTEGER DEFAULT 0,              -- User wants to display this badge
    
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (achievement_type_id) REFERENCES achievement_types(achievement_type_id) ON DELETE CASCADE,
    UNIQUE(user_id, achievement_type_id)   -- Can only earn each achievement once
);

CREATE INDEX IF NOT EXISTS idx_user_achievements_user 
    ON user_achievements(user_id, earned_at DESC);
CREATE INDEX IF NOT EXISTS idx_user_achievements_type 
    ON user_achievements(achievement_type_id);
```

### 5. user_sessions
Token-based session management for file downloads and future web interface.

```sql
CREATE TABLE IF NOT EXISTS user_sessions (
    session_id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    
    -- Token data
    token_hash TEXT NOT NULL UNIQUE,       -- SHA256 of random token
    token_type TEXT NOT NULL,              -- 'download', 'web', 'api'
    
    -- Session context
    created_at INTEGER NOT NULL DEFAULT (unixepoch()),
    expires_at INTEGER NOT NULL,           -- Unix timestamp
    last_used_at INTEGER,                  -- Track activity
    
    -- Client info
    ip_address TEXT,
    user_agent TEXT,
    
    -- Scope/permissions
    scope_json TEXT,                       -- JSON: {"files": ["file1.zip"], "areas": ["FILES"]}
    max_uses INTEGER DEFAULT 1,            -- NULL = unlimited, >0 = limited uses
    use_count INTEGER DEFAULT 0,
    
    -- Status
    revoked INTEGER DEFAULT 0,
    revoked_at INTEGER,
    revoked_reason TEXT,
    
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE UNIQUE INDEX IF NOT EXISTS idx_user_sessions_token 
    ON user_sessions(token_hash);
CREATE INDEX IF NOT EXISTS idx_user_sessions_user 
    ON user_sessions(user_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_user_sessions_expires 
    ON user_sessions(expires_at) WHERE revoked = 0;
```

**Token types:**
- `download`: Single-file or batch download link
- `web`: Web interface session (future)
- `api`: API access token (future)

**Use cases:**
1. User selects "Links Only" for file download
2. System generates token: `https://bbs.example.com/dl?t=abc123...`
3. Token stored with `scope_json = {"files": ["file1.zip"], "max_uses": 1}`
4. User clicks link, system validates token, serves file, increments `use_count`
5. Token auto-expires after 24 hours or max_uses reached

## Migration Path

### From callers.bbs
```c
// Read existing callers.bbs records
// For each record:
//   - Map to caller_sessions table
//   - Preserve all fields
//   - Set session_id = auto-increment
//   - Keep original timestamps
```

### Seed achievement_types
```sql
-- Example achievements
INSERT INTO achievement_types (badge_code, badge_name, badge_desc, threshold_type, threshold_value) VALUES
('FIRST_LOGIN', 'First Login', 'Logged in for the first time', 'special', NULL),
('FIRST_POST', 'First Post', 'Posted your first message', 'count', 1),
('100_POSTS', 'Centurion', 'Posted 100 messages', 'count', 100),
('100_UPLOADS', 'Contributor', 'Uploaded 100 files', 'count', 100),
('VETERAN_1Y', 'Veteran (1 Year)', 'Member for 1 year', 'tenure', 365),
('VETERAN_5Y', 'Veteran (5 Years)', 'Member for 5 years', 'tenure', 1825);
```

## API Functions (libmaxdb)

### Caller Sessions
```c
int maxdb_session_create(MaxDB *db, int user_id, int task, MaxDBSession **session);
int maxdb_session_update(MaxDB *db, int session_id, MaxDBSession *session);
int maxdb_session_close(MaxDB *db, int session_id);
MaxDBSession** maxdb_session_list_by_user(MaxDB *db, int user_id, int limit, int *count);
```

### User Preferences
```c
int maxdb_pref_set(MaxDB *db, int user_id, const char *key, const char *value, const char *type);
char* maxdb_pref_get(MaxDB *db, int user_id, const char *key);
int maxdb_pref_delete(MaxDB *db, int user_id, const char *key);
```

### Achievements
```c
int maxdb_achievement_type_create(MaxDB *db, MaxDBAchievementType *type);
int maxdb_achievement_award(MaxDB *db, int user_id, const char *badge_code, const char *metadata_json);
MaxDBAchievement** maxdb_achievement_list_by_user(MaxDB *db, int user_id, int *count);
int maxdb_achievement_check_earned(MaxDB *db, int user_id, const char *badge_code);
```

### User Sessions/Tokens
```c
int maxdb_token_create(MaxDB *db, int user_id, const char *token_type, int expires_in_seconds, const char *scope_json, char **token_out);
int maxdb_token_validate(MaxDB *db, const char *token, MaxDBToken **token_out);
int maxdb_token_use(MaxDB *db, const char *token);
int maxdb_token_revoke(MaxDB *db, const char *token, const char *reason);
int maxdb_token_cleanup_expired(MaxDB *db);
```

## Integration Points

### callinfo.c
- `ci_init()`: Create session record
- `ci_save()`: Update session record with final stats
- Replace file I/O with `maxdb_session_*()` calls

### File Transfer
- Generate download token when user selects "Links Only"
- Validate token on download request
- Track downloads via `use_count`

### Future Web Interface
- Use `user_sessions` for web login
- Share session state between telnet and web
- Unified user experience

## Benefits

1. **Caller Sessions**: Structured queries, reporting, no file size limits
2. **User Preferences**: Extensible without schema changes, per-user customization
3. **Achievements**: Gamification, user engagement, leaderboards
4. **User Sessions**: Secure file sharing, future web interface, token-based auth

## Schema Version Management

Update `userdb_schema.sql` to v2:
```sql
PRAGMA user_version = 2;
```

Add migration in `db_init.c`:
```c
if (current_version == 1 && target_version >= 2) {
    // Create new tables
    // Migrate callers.bbs if exists
    // Seed achievement_types
    // Set version to 2
}
```
