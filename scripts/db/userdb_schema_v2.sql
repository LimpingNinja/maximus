-- SQLite schema for Maximus user database (Milestone 2)
--
-- Full column-based schema mapping all fields from struct _usr
-- plus extensions for caller sessions, preferences, achievements, and tokens.
--
-- Versioning:
-- - Uses PRAGMA user_version for schema version.

PRAGMA user_version = 2;
PRAGMA foreign_keys = ON;

-- Base users table (from v1)
CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY,
    
    -- Identity
    name TEXT NOT NULL COLLATE NOCASE,
    city TEXT,
    alias TEXT COLLATE NOCASE,
    phone TEXT,
    dataphone TEXT,
    
    -- Authentication
    pwd BLOB,
    pwd_encrypted INTEGER DEFAULT 0,
    
    -- Demographics
    dob_year INTEGER,
    dob_month INTEGER,
    dob_day INTEGER,
    sex INTEGER DEFAULT 0,
    
    -- Access control
    priv INTEGER DEFAULT 0,
    xkeys INTEGER DEFAULT 0,
    
    -- Expiry settings
    xp_priv INTEGER DEFAULT 0,
    xp_date_date INTEGER,
    xp_date_time INTEGER,
    xp_mins INTEGER DEFAULT 0,
    xp_flag INTEGER DEFAULT 0,
    
    -- Statistics
    times INTEGER DEFAULT 0,
    call INTEGER DEFAULT 0,
    msgs_posted INTEGER DEFAULT 0,
    msgs_read INTEGER DEFAULT 0,
    nup INTEGER DEFAULT 0,
    ndown INTEGER DEFAULT 0,
    ndowntoday INTEGER DEFAULT 0,
    up INTEGER DEFAULT 0,
    down INTEGER DEFAULT 0,
    downtoday INTEGER DEFAULT 0,
    
    -- Session info (stored as SCOMBO date/time pairs)
    ludate_date INTEGER,
    ludate_time INTEGER,
    date_1stcall_date INTEGER,
    date_1stcall_time INTEGER,
    date_pwd_chg_date INTEGER,
    date_pwd_chg_time INTEGER,
    date_newfile_date INTEGER,
    date_newfile_time INTEGER,
    time INTEGER DEFAULT 0,
    time_added INTEGER DEFAULT 0,
    timeremaining INTEGER DEFAULT 0,
    
    -- Preferences
    video INTEGER DEFAULT 0,
    lang INTEGER DEFAULT 0,
    width INTEGER DEFAULT 80,
    len INTEGER DEFAULT 24,
    help INTEGER DEFAULT 0,
    nulls INTEGER DEFAULT 0,
    def_proto INTEGER DEFAULT 0,
    compress INTEGER DEFAULT 0,
    
    -- Message/File area tracking
    lastread_ptr INTEGER DEFAULT 0,
    msg TEXT,
    files TEXT,
    
    -- Credits/Points
    credit INTEGER DEFAULT 0,
    debit INTEGER DEFAULT 0,
    point_credit INTEGER DEFAULT 0,
    point_debit INTEGER DEFAULT 0,
    
    -- Flags
    bits INTEGER DEFAULT 0,
    bits2 INTEGER DEFAULT 0,
    delflag INTEGER DEFAULT 0,
    
    -- Misc
    grp INTEGER DEFAULT 0,
    extra INTEGER DEFAULT 0,
    
    -- Bookkeeping
    created_at_unix INTEGER NOT NULL DEFAULT (unixepoch()),
    updated_at_unix INTEGER NOT NULL DEFAULT (unixepoch())
);

CREATE UNIQUE INDEX IF NOT EXISTS users_name_idx 
    ON users(name COLLATE NOCASE);

CREATE INDEX IF NOT EXISTS users_alias_idx 
    ON users(alias COLLATE NOCASE);

-- Caller sessions (replaces callers.bbs)
CREATE TABLE IF NOT EXISTS caller_sessions (
    session_id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    
    -- Session timing
    login_time INTEGER NOT NULL,
    logoff_time INTEGER,
    
    -- Session context
    task INTEGER NOT NULL,
    flags INTEGER DEFAULT 0,
    
    -- User state at login/logoff
    logon_priv INTEGER,
    logoff_priv INTEGER,
    logon_xkeys INTEGER,
    logoff_xkeys INTEGER,
    
    -- Activity counters
    calls INTEGER DEFAULT 0,
    files_up INTEGER DEFAULT 0,
    files_dn INTEGER DEFAULT 0,
    kb_up INTEGER DEFAULT 0,
    kb_dn INTEGER DEFAULT 0,
    msgs_read INTEGER DEFAULT 0,
    msgs_posted INTEGER DEFAULT 0,
    paged INTEGER DEFAULT 0,
    time_added INTEGER DEFAULT 0,
    
    -- Metadata
    created_at INTEGER NOT NULL DEFAULT (unixepoch()),
    
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_caller_sessions_user 
    ON caller_sessions(user_id, login_time DESC);
CREATE INDEX IF NOT EXISTS idx_caller_sessions_time 
    ON caller_sessions(login_time DESC);

-- User preferences (extensible key-value store)
CREATE TABLE IF NOT EXISTS user_preferences (
    pref_id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    
    -- Preference data
    pref_key TEXT NOT NULL,
    pref_value TEXT,
    pref_type TEXT DEFAULT 'string',
    
    -- Metadata
    created_at INTEGER NOT NULL DEFAULT (unixepoch()),
    updated_at INTEGER NOT NULL DEFAULT (unixepoch()),
    
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    UNIQUE(user_id, pref_key)
);

CREATE INDEX IF NOT EXISTS idx_user_preferences_lookup 
    ON user_preferences(user_id, pref_key);

-- Achievement types (meta-table)
CREATE TABLE IF NOT EXISTS achievement_types (
    achievement_type_id INTEGER PRIMARY KEY AUTOINCREMENT,
    
    -- Achievement definition
    badge_code TEXT NOT NULL UNIQUE,
    badge_name TEXT NOT NULL,
    badge_desc TEXT,
    
    -- Award criteria
    threshold_type TEXT NOT NULL,
    threshold_value INTEGER,
    
    -- Display
    badge_icon TEXT,
    badge_color INTEGER,
    display_order INTEGER DEFAULT 0,
    
    -- Metadata
    enabled INTEGER DEFAULT 1,
    created_at INTEGER NOT NULL DEFAULT (unixepoch())
);

CREATE UNIQUE INDEX IF NOT EXISTS idx_achievement_types_code 
    ON achievement_types(badge_code);

-- User achievements
CREATE TABLE IF NOT EXISTS user_achievements (
    achievement_id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    achievement_type_id INTEGER NOT NULL,
    
    -- Award details
    earned_at INTEGER NOT NULL DEFAULT (unixepoch()),
    metadata_json TEXT,
    
    -- Display
    displayed INTEGER DEFAULT 0,
    pinned INTEGER DEFAULT 0,
    
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (achievement_type_id) REFERENCES achievement_types(achievement_type_id) ON DELETE CASCADE,
    UNIQUE(user_id, achievement_type_id)
);

CREATE INDEX IF NOT EXISTS idx_user_achievements_user 
    ON user_achievements(user_id, earned_at DESC);
CREATE INDEX IF NOT EXISTS idx_user_achievements_type 
    ON user_achievements(achievement_type_id);

-- User sessions/tokens (for file downloads and future web interface)
CREATE TABLE IF NOT EXISTS user_sessions (
    session_id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    
    -- Token data
    token_hash TEXT NOT NULL UNIQUE,
    token_type TEXT NOT NULL,
    
    -- Session context
    created_at INTEGER NOT NULL DEFAULT (unixepoch()),
    expires_at INTEGER NOT NULL,
    last_used_at INTEGER,
    
    -- Client info
    ip_address TEXT,
    user_agent TEXT,
    
    -- Scope/permissions
    scope_json TEXT,
    max_uses INTEGER DEFAULT 1,
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

-- Seed default achievement types
INSERT OR IGNORE INTO achievement_types (badge_code, badge_name, badge_desc, threshold_type, threshold_value, display_order) VALUES
('FIRST_LOGIN', 'First Login', 'Logged in for the first time', 'special', NULL, 1),
('FIRST_POST', 'First Post', 'Posted your first message', 'count', 1, 2),
('10_POSTS', 'Chatterer', 'Posted 10 messages', 'count', 10, 3),
('100_POSTS', 'Centurion', 'Posted 100 messages', 'count', 100, 4),
('1000_POSTS', 'Prolific', 'Posted 1000 messages', 'count', 1000, 5),
('FIRST_UPLOAD', 'First Upload', 'Uploaded your first file', 'count', 1, 6),
('10_UPLOADS', 'Sharer', 'Uploaded 10 files', 'count', 10, 7),
('100_UPLOADS', 'Contributor', 'Uploaded 100 files', 'count', 100, 8),
('FIRST_DOWNLOAD', 'First Download', 'Downloaded your first file', 'count', 1, 9),
('VETERAN_30D', 'Regular (30 Days)', 'Member for 30 days', 'tenure', 30, 10),
('VETERAN_1Y', 'Veteran (1 Year)', 'Member for 1 year', 'tenure', 365, 11),
('VETERAN_5Y', 'Elder (5 Years)', 'Member for 5 years', 'tenure', 1825, 12);
