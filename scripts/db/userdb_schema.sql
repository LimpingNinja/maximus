-- SQLite schema for Maximus user database (Milestone 1)
--
-- Full column-based schema mapping all fields from struct _usr
-- This enables proper SQL queries, reporting, and schema evolution.
--
-- Versioning:
-- - Uses PRAGMA user_version for schema version.

PRAGMA user_version = 1;
PRAGMA foreign_keys = ON;

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
