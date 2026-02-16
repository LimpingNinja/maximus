# Semantic Theme Color Migration — english.toml Proposal

This document proposes replacing hardcoded `|##` color codes in `english.toml`
with semantic `|xx` theme codes. Organized into batches by reasoning.

**Theme slot reference:**
| Code | Slot          | Default |
|------|---------------|---------|
| `tx` | text          | `\|07`   |
| `hi` | highlight     | `\|15`   |
| `pr` | prompt        | `\|14`   |
| `in` | input         | `\|15`   |
| `tf` | textbox_fg    | `\|15`   |
| `tb` | textbox_bg    | `\|17`   |
| `hd` | heading       | `\|11`   |
| `lf` | lightbar_fg   | `\|15`   |
| `lb` | lightbar_bg   | `\|17`   |
| `er` | error         | `\|12`   |
| `wn` | warning       | `\|14`   |
| `ok` | success       | `\|10`   |
| `dm` | dim           | `\|08`   |
| `fi` | file_info     | `\|03`   |
| `sy` | sysop         | `\|13`   |
| `qt` | quote         | `\|09`   |
| `br` | border        | `\|01`   |
| `cd` | default       | `\|07`   |

---

## Batch 1: Standalone `_col` Variables

These are pure color-string variables consumed by C code to set colors
programmatically. They are the ideal candidates for theme codes — the
C code already reads them as strings and applies them.

### [global] section (lines 192–194)

| Variable         | Current | Proposed | Reasoning                            |
|------------------|---------|----------|--------------------------------------|
| `menu_name_col`  | `\|14`   | `\|pr`    | Menu option names are prompts        |
| `menu_high_col`  | `\|14`   | `\|hi`    | Highlighted menu hotkey              |
| `menu_opt_col`   | `\|07`   | `\|tx`    | Normal menu text                     |

### [f_area] section — DEFERRED

| Variable           | Current   | Proposed | Reasoning                          |
|--------------------|-----------|----------|------------------------------------|
| `file_name_col`    | `\|14`     | `\|pr`    | File names highlighted as prompt   |
| `file_size_col`    | `\|05`     | `\|dm`    | File size is secondary info        |
| `file_date_col`    | `\|02`     | `\|dm`    | File date is secondary info        |
| `file_desc_col`    | `\|03`     | `\|fi`    | File descriptions                  |
| `file_found_col`   | `\|14`     | `\|ok`    | "Found" is a positive indicator    |
| `file_offline_col` | `\|04`     | `\|er`    | Offline = error/unavailable        |
| `file_new_col`     | `\|03\|24`| `\|ok\|24` | New file = success + blink bg   |

> **Deferred.** These are domain-specific (file area colors). Only `[global]`
> standalone `_col` variables are in scope for the initial migration pass.
> These may be converted in a later pass once the theme system is proven.

### [m_area] section — DEFERRED

| Variable            | Current    | Proposed   | Reasoning                        |
|---------------------|------------|------------|----------------------------------|
| `msg_text_col`      | `\|03`      | `\|fi`      | Message body text (info-level)   |
| `msg_quote_col`     | `\|07`      | `\|qt`      | Quoted text                      |
| `msg_kludge_col`    | `\|13`      | `\|sy`      | Kludge lines = sysop/technical   |
| `msg_from_col`      | `\|03`      | `\|fi`      | "From:" label                    |
| `msg_from_txt_col`  | `\|14`      | `\|hi`      | From: value (emphasized)         |
| `msg_attr_col`      | `\|10`      | `\|ok`      | Message attributes (green)       |
| `msg_to_col`        | `\|03`      | `\|fi`      | "To:" label                      |
| `msg_to_txt_col`    | `\|14`      | `\|hi`      | To: value (emphasized)           |
| `msg_date_col`      | `\|10`      | `\|ok`      | Date value                       |
| `msg_subj_col`      | `\|03`      | `\|fi`      | "Subject:" label                 |
| `msg_subj_txt_col`  | `\|14`      | `\|hi`      | Subject: value (emphasized)      |
| `msg_addr_col`      | `\|03`      | `\|fi`      | Address label                    |
| `msg_locus_col`     | `\|09`      | `\|qt`      | Locus / origin info              |
| `fsr_border_col`    | `\|11\|17` | `\|br\|tb` | FSR border (border + bg)        |
| `fsr_msginfo_col`   | `\|14\|17` | `\|pr\|tb` | FSR msg info (prompt + bg)      |
| `fsr_addr_col`      | `\|14\|17` | `\|pr\|tb` | FSR address (prompt + bg)       |

> **Deferred.** These are domain-specific (reader colors). Mapping them to
> generic theme slots means changing the reader theme changes ALL info-display
> text. May revisit with dedicated reader theme slots later. Only `[global]`
> standalone `_col` variables are in scope for the initial migration pass.

---

## Batch 2: Error Messages (`|12` → `|er`)

All strings that start with `|12` and contain error/failure text. These are
user-facing error messages and should use the semantic error slot.

**Rule: `|12` at start of error message text → `|er`**

### [global]
- `file_no_wc` — `|12This command does not support wildcards.` → `|erThis command...`
- `rip_disabled` — contains `|15` intro then error text (mixed — leave as-is or split)

### [f_area]
- `badupload` — `|12\nThe filename '...' is not acceptible...` → `|er\nThe filename...`

### [m_area]
- `err_entering_msg_area` — `|15\nError! Cannot enter...` → `|er\nError! Cannot enter...`
- `no_files_attached` — `|12There are no unreceived file attaches...` → `|erThere are no...`

### [m_browse]
- `qwk_invalid_area` — `|12\nInvalid area.  |14(To=...` → `|er\nInvalid area.  |pr(To=...`
- `err_receive_rep` — `|12Error receiving...` → `|erError receiving...`
- `err_decompr_rep` — `|12Error decompressing...` → `|erError decompressing...`
- `unknown_compr` — `|12Unknown compression type.` → `|erUnknown compression type.`
- `err_toss_qwk` — `|12Error tossing from...` → `|erError tossing from...`
- `qwk_too_many` — `|12\nWarning! A maximum of...` → `|wn\nWarning! A maximum of...`
  (This is actually a warning, not an error — use `|wn`)
- `qwk_msg_skipped` — `|12\nMessage skipped.` → `|er\nMessage skipped.`
- `err_receive_attach` — `|12Error receiving attached file(s)!` → `|erError receiving...`
- `err_compr_attach` — `|12Error compressing file(s)!` → `|erError compressing...`
- `err_decompr_attach` — `|12Error decompressing attached file(s)!` → `|erError decompressing...`
- `no_attach_files` — `|12No file(s) found to send!` → `|erNo file(s) found...`
- `attach_notavail` — `|12Sorry - the attached file(s) are no longer available.` → `|erSorry...`

### [max_ued]
- `cant_change_field` — `|12You cannot change that field.\n|15` → `|erYou cannot change...\n|hi`

### [max_chat]
- `ch_off_abnormally` — `|12|!1's|13 session was terminated abnormally.` → `|er|!1's|sy session...`

### [mexbank]
- `mb_cfgfile` — `|12MEXBank configuration file error...` → `|erMEXBank configuration...`
- `mb_cfgerr` — `|12Error in MEXBank configuration...` → `|erError in MEXBank...`
- `mb_fatal` — `|12A fatal error occurred...` → `|erA fatal error occurred...`
- `mb_invalid` — `|12Use 'D' for D)eposit...` → `|erUse 'D' for D)eposit...`
- `mb_nottorb` — `|12Use 'T' for T)ime...` → `|erUse 'T' for T)ime...`
- `mb_toomuch` — `|12Sorry, the amount you entered exceeds...` → `|erSorry, the amount...`

### [mexchat]
- `chat_needremote` — `|12\nMEXChat can only be used with a remote user.` → `|er\nMEXChat can only...`

### [global]
- `invalid_pwd` — `|24\n\aINVALID PASSWORD\a\n|03` → `|er\n\aINVALID PASSWORD\a\n|cd`
  **Decision: Use `|er` alone.** The `\a` bell already provides the alarm;
  blink bg is a DOS-era visual cue that modern terminals handle inconsistently.

**Total: ~25 strings. Straightforward mechanical replacement.**

---

## Batch 3: Prompts and Instructions (`|14` → `|pr`)

Strings where `|14` (yellow) introduces a user-facing prompt, instruction,
or label the user is expected to read and respond to.

**Rule: `|14` at the start of prompt/instruction text → `|pr`**

### [global]
- `please_wait` — `|14\nPlease wait...` → `|pr\nPlease wait...`
- `achg_help` — `|14For more areas, type "|10..|14"...` → `|prFor more areas, type "|ok..|pr"...`
- `menu_end` — `|14]: ` → `|pr]: `
- `pad_zone` — `|14ZONE  ` → `|prZONE  `
- `pad_region` — `|14REGION` → `|prREGION`
- `pad_net` — `|14NET   ` → `|prNET   `
- `pad_none` — `|14      ` → `|pr      `
- `list_option` — `|14  |!1d|15) |!2\n` → `|pr  |!1d|hi) |!2\n`
- `type_keys_to_toggle` — `|14\nSysOp: Type letters/numbers...` → `|pr\nSysOp: Type letters...`
- `ck_for_help` — `|14|17Press Control-N for help.` → `|pr|tbPress Control-N for help.`

### [f_area]
- `bytes_for_ul` — `...are |14|!1|15 bytes available...` → `...are |pr|!1|hi bytes available...`
- `checking_ul` — `|14\nPlease wait.  Verifying |!1...` → `|pr\nPlease wait.  Verifying |!1...`
- `pause_msg` — `|14Hit <enter> (or wait |!1u seconds)...` → `|prHit <enter> (or wait |!1u seconds)...`
- `avail_proto` — `|14\nAvailable protocols:\n\n` → `|pr\nAvailable protocols:\n\n`
- `how_dl` — `|14Type "/q" on a blank line...` → `|prType "/q" on a blank line...`
- `down_fnam` — `|14\n\nFile: |10` → `|pr\n\nFile: |ok`
- `down_fsiz` — `|14Size: |15|!1l bytes...` → `|prSize: |hi|!1l bytes...`
- `down_ftim` — `|14Time: |15|!1d minutes...` → `|prTime: |hi|!1d minutes...`
- `down_fmode` — `|14Mode: |15|!1\n\n` → `|prMode: |hi|!1\n\n`
- `file_stats` — `|14(|!1u) |13$R12|!2 |10(...` → `|pr(|!1u) |sy$R12|!2 |ok(...`
- `file_tag_total` — `|14\nTotal: |10|!1l bytes...` → `|pr\nTotal: |ok|!1l bytes...`
- `file_untagged` — `|14File |13|!1 |14detagged.\n` → `|prFile |sy|!1 |prdetagged.\n`

### [max_bor] / editor
- `editl1` — `|14\nType what you want to put into the line\n` → `|pr\nType what you want...`
- `blfmt1` — `|07$L02|!1d: |14|!2` → `|tx$L02|!1d: |pr|!2`
- `edlist_quit` — `|14Q|07)uit\n\n|15` → `|prQ|tx)uit\n\n|hi`

### [m_area]
- `msg_menu_hdr` — `|10Msg.area |13` → `|okMsg.area |sy`
- `msg_enter_prompt1` — `|02Press <enter> for the |10` → `|dmPress <enter> for the |ok`
- `msg_enter_prompt2` — `|02 message.\n\n` → `|dm message.\n\n`

### [m_browse]
- `wait_doing_compr` — `|14Compressing mail.  Please wait...` → `|prCompressing mail.  Please wait...`
- `qwk_update_lr` — `|14Please wait.  Updating lastread pointers...` → `|prPlease wait.  Updating...`
- `tossing_rep_packet` — `|14\nTossing .REP packet |!1...` → `|pr\nTossing .REP packet |!1...`
- `wait_compr_attach` — `\r|14Compressing file(s).  Please wait...` → `\r|prCompressing file(s)...`
- `wait_decompr_attach` — `\r|14Decompressing file(s).  Please wait...` → `\r|prDecompressing file(s)...`
- `qwk_max_area` — `|14MaxArea: |!1\n` → `|prMaxArea: |!1\n`
- `qwk_pack_start` — `|14\nArea Name  Description...` → `|pr\nArea Name  Description...`

### [mexbank]
- Many `|14  Label |07.........|10 : |11` patterns → `|pr  Label |tx.........|ok : |hd`
  (mb_balance_kb, mb_balance_time, mb_left_time, mb_left_call, mb_left_kb,
   mb_max_dep_kb, mb_max_dep_time, mb_max_wdraw_kb, mb_max_wdraw_time)
- `mb_prompt` — `|14\nD|07)eposit, |14W|07)ithdraw or |14Q|07)uit` →
  `|pr\nD|tx)eposit, |prW|tx)ithdraw or |prQ|tx)uit`

**Total: ~35-40 strings.**

---

## Batch 4: Highlighted / Emphasized Text (`|15` → `|hi`)

Strings where `|15` (white) marks important or emphasized text.

**Rule: `|15` at start of important/emphatic user message → `|hi`**

### [global]
- `dontunderstand` — `|15\nI don't understand "|!1c".\n\n` → `|hi\nI don't understand...`
- `searchingfor` — `|15\nSearching for "|!1".\n|!2c` → `|hi\nSearching for...`
- `err999_1` — `|15\nError! Files cannot be uploaded in local mode.` →
  `|er\nError! Files cannot be uploaded...` (actually an error)
- `err999_2` — `Press ENTER to continue ` — no color, leave as-is
- `unavailable` — `|15Sorry! That option is not available...` → `|hiSorry! That option...`
- `userdoesntexist` — `|15That user doesn't exist!\n` → `|erThat user doesn't exist!\n`
  (actually an error)
- `areadoesntexist` — `|15\nThat area doesn't exist!\n` → `|er\nThat area doesn't exist!\n`
  (actually an error)
- `tnx4ul` — `|15\nThanks for the upload, |!1.\n` → `|hi\nThanks for the upload, |!1.\n`

### [max_bor]
- `not_reply` — `|15\nMessage is not a reply.\n` → `|er\nMessage is not a reply.\n`
- `e_numch` — `|10You can put as many as |!1d characters...` → `|okYou can put as many as...`
- `happy` — `|15|17\aDon't worry, be happy!...` — Easter egg, leave as-is
- `max_status` — `|14|17[K^Z=save|15|17  To:...` → `|pr|tb[K^Z=save|hi|tb  To:...`

### [m_area]
- `msg_nomsgs` — `|15There are no messages in this area.` → `|hiThere are no messages...`
- `msg_thereare1` — `|10There are ` → `|okThere are `
- `where_to_reply` — `|15Reply to which area number...` → `|hiReply to which area...`
- `atag_questmore` — `|15Areas to tag [..., |14<enter>|15 ...` → `|hiAreas to tag [..., |pr<enter>|hi ...`

### [max_main]
- `bibi` — `|15Bye |!1; thanks for calling...` → `|hiBye |!1; thanks for calling...`

### [m_browse]
- `rep_to_send` — `|15\nPlease begin uploading |!1 now.` → `|hi\nPlease begin uploading |!1 now.`
- `recv_attach_local` — `|15Enter destination directory...` → `|hiEnter destination directory...`
- `really_abort_attach` — `|15Do you want to abort the file attach` → `|hiDo you want to abort...`
- `browse_select_s` — `|15Select: |03` → `|hiSelect: |fi`
- `mb_trxok` — `|15\n\nTRANSACTION ACCEPTED - KAAA-CHING!\n\n` → `|ok\n\nTRANSACTION ACCEPTED...`
  (actually success — use `|ok`)
- `mb_goodbye` — `|CL|15\n\nGoodbye. Thanks for using MEXBank!\n` → `|CL|hi\n\nGoodbye...`

### [max_ued]
- `ued_nodel_current` — `|15\nYou can't delete the current user!\n` → `|er\nYou can't delete...`
  (actually an error)

**Total: ~20-25 strings. Some `|15` strings are actually errors (reclassify to `|er`).**

---

## Batch 5: Success / Positive Text (`|10` → `|ok`)

Strings where `|10` (light green) indicates success, confirmation, or
positive status.

**Rule: `|10` marking success/positive info → `|ok`**

### [global]
- `stat_hdr` — `|CL|10Your statistics for ` → `|CL|okYour statistics for `
- `time_added_for_ul` — `|10\n|!1l:$l020|!2l|14 minutes added...` → `|ok\n|!1l:...|pr minutes added...`

### [f_area]
- `file_menu_hdr` — `|10\n\nFile area |13` → `|ok\n\nFile area |sy`
- `down_fnam` — `|14\n\nFile: |10` → `|pr\n\nFile: |ok`

### [m_area]
- `msg_thereare1` — `|10There are ` → `|okThere are `

### [max_chat]
- `ch_youare` — `|14\nYou are currently |10` → `|pr\nYou are currently |ok`
- `ch_availforchat` — `AVAILABLE|14 for chat.\n\n` → `AVAILABLE|pr for chat.\n\n`

**Total: ~10 strings.**

---

## Batch 6: Section Headers (`|14|17` → `|hd|tb` or `|pr|tb`)

Strings that set yellow-on-blue for prominent section headers.

**Rule: `|14|17` section header → `|hd|tb`**

### [f_area]
- `file_sect_hdr` — `|CL|14|17[KThe FILES Section|07` → `|CL|hd|tb[KThe FILES Section|tx`

### [m_area]
- `msg_sect` — `|CL|14|17[KThe MESSAGE Section|07\n` → `|CL|hd|tb[KThe MESSAGE Section|tx\n`

### [mexbank]
- `mexbank_hdr` — `|CL|14|17[KMEXBank:...|07\n\n\n` → `|CL|hd|tb[KMEXBank:...|tx\n\n\n`

### [max_bor] (MaxEd status bar)
- `max_status` — `|14|17[K^Z=save|15|17  To:...` → `|pr|tb[K^Z=save|hi|tb  To:...`
- `status_insert` — `|13|17 Insert` → `|sy|tb Insert`
- `max_not_reply` — `|14|17[K\aNot a reply` → `|pr|tb[K\aNot a reply`
- `max_no_understand` — `|15|17[K\aI don't understand.  |!1` → `|hi|tb[K\aI don't understand.  |!1`

### [m_browse]
- `browse_rbox_top` — `|11|17[K...` → `|br|tb[K...` (border on blue bg)

**Total: ~10 strings.**

---

## Batch 7: Info / Body Text (`|07` → `|tx`, `|03` → `|fi`)

Strings where `|07` or `|03` is used for normal descriptive text.

**Rule: `|07` as body/label text → `|tx`; `|03` as info text → `|fi`**

### [global]
- `stat_minutes` — `|07 minutes` → `|tx minutes`
- `stat_file` — `|07 files` → `|tx files`
- `stat_kb` — `|07 kb` → `|tx kb`
- `stat_cents` — `|07 cents` → `|tx cents`
- `stat_time_online` — `|07 Time on line...   |03` → `|tx Time on line...   |fi`
- `stat_time_remain` — same pattern → `|tx ... |fi`
- `stat_time_prev` — same pattern
- `stat_time_calls` — same pattern
- `stat_file_ul` through `stat_pwd_chg` — all follow `|07 Label...  |03` → `|tx Label...  |fi`
- `stat_comma` — `, |03` → `, |fi`

### [max_bor]
- `bed_to` — `|07\nTo: |14|!1\n\n` → `|tx\nTo: |pr|!1\n\n`
- `bfrom` — `|07\nFrom: |14|!1\n\n` → `|tx\nFrom: |pr|!1\n\n`
- `bsubj` — `|07\nSubject: |14|!1\n\n` → `|tx\nSubject: |pr|!1\n\n`
- `end_widget` — `|15-end-|!1` → `|hi-end-|!1`
- `end_widget2` — `|15-end-|07` → `|hi-end-|tx`

### [f_area]
- `file_menu_sep` — `|07 ... |03` → `|tx ... |fi`
- `list_from` — `|03   From: |14|!1\n` → `|fi   From: |pr|!1\n`
- `list_to` — `|03     To: |14|!1\n` → `|fi     To: |pr|!1\n`
- `list_subj` — `|03Subject: |14|!1\n\n` → `|fiSubject: |pr|!1\n\n`

### [m_area]
- `msg_menu_sep` — `|07 ... |03` → `|tx ... |fi`
- `reader_pos_text` — `[Y07[X01|03|03[K` → `[Y07[X01|fi|fi[K`

**Total: ~25-30 strings.**

---

## Batch 8: Strings to LEAVE UNCHANGED

These should NOT be converted to theme codes:

### Log messages (not user-facing)
All strings prefixed with `!`, `:`, `>`, `%%`, `~` are log-only:
- `log_err_compr`, `log_qwk_bad_date`, `log_mcp_err_1`, etc.
- `lxferaborted` (uses `~` prefix)

### Plain text / no color codes
- `bto`, `bfromsp`, `bsubject`, `word_not_found`, `noroom`, etc.
- `yes`, `no`, `Yes`, `No`, `Tag`, key lists (`"ALDC?Q"`)
- Ordinal suffixes (`ordinal_th`, `ordinal_st`, etc.)
- Message attribute labels (`msg_attr0` through `msg_attr15`)

### Parameterized color strings
These pass color as a `|!N` parameter — the caller controls the color:
- `mfrom`, `mto`, `msubj`, `mdate` — parameters like `from_label_col`
- `trk_msg_info` — `|!1Status:...` where `|!1` = color
- `srchng` — `|!1cSearching:...` where `|!1` = color code number
- `quote_format`, `norm_format` — `|!2` = quote_col parameter

### Cursor/layout strings with color
These embed colors in positional cursor layouts (reader header):
- `reader_pos_attr` — `[Y02[X40|14|17` (FSR position)
- `reader_pos_from` — `[Y03[X07` (no inline color in text portion)
- These are tightly coupled to FSR layout; leave as-is for now.

### RIP sequences
All `rip = "..."` values — never touch these.

### User editor (ued_ss*) box-drawing strings
The `ued_ss1` through `ued_ss22` strings are a fixed-width TUI layout with
`|14`, `|07`, `|10` colors embedded in box-drawing. These could be converted
but the layout is fragile — recommend leaving as-is or converting as a
separate dedicated pass.

### Special-purpose strings
- `line_noise` — random bytes, not a color
- `maxed_init` — editor init escape sequence
- `br_msg_pvt` — `|04-` (single char private marker)

---

## Batch 9: MEXBank Label/Value Pattern

The MEXBank strings all follow a consistent pattern:
```
|14  Label text |07.........|10 : |11
```
→
```
|pr  Label text |tx.........|ok : |hd
```

Applies to: `mb_balance_kb`, `mb_balance_time`, `mb_left_time`,
`mb_left_call`, `mb_left_kb`, `mb_max_dep_kb`, `mb_max_dep_time`,
`mb_max_wdraw_kb`, `mb_max_wdraw_time`

Also: `mb_kb` (`|07 kb` → `|tx kb`), `mb_mins` (`|07 mins` → `|tx mins`)

---

## Batch 10: Heading Text (`|14` section tab names → `|hd`)

Some `|14` strings are section tab headings rather than prompts:
- `stat_time` — `|14TIME\n` → `|hdTIME\n`
- `stat_files` — `|14FILES\n` → `|hdFILES\n`
- `stat_matrix` — `|14MATRIX\n` → `|hdMATRIX\n`
- `stat_subscrip` — `|14SUBSCRIPTION\n` → `|hdSUBSCRIPTION\n`
- `stat_misc` — `|14MISCELLANEOUS\n` → `|hdMISCELLANEOUS\n`

---

## Batch 11: Tracking Section

The `[track]` section uses `|15` for table headers and `|14` for data rows:
- `trk_rep_hdr_graph` — `|CL|15ID...` → `|CL|hiID...` (table header)
- `trk_list_hdr` — `|15\nID   Owner\n...\n|14` → `|hi\nID   Owner\n...\n|pr`

---

## Batch 12: Misc Reclassifications

Some `|15` strings that are really errors (not just emphasis):
- `err999_1` — error about local mode uploads → `|er`
- `userdoesntexist` — "That user doesn't exist!" → `|er`
- `areadoesntexist` — "That area doesn't exist!" → `|er`
- `not_reply` — "Message is not a reply." → `|er`
- `ued_nodel_current` — "You can't delete the current user!" → `|er`

Some `|15` strings that are really success:
- `mb_trxok` — "TRANSACTION ACCEPTED" → `|ok`

---

## Batch 13: Login & Input Prompts (`|pr`...`|in`)

Any prompt where the user types input after it should be wrapped with `|pr`
at the start (prompt color) and `|in` at the end (input/echo color).

**Rule: Add `|pr` prefix and `|in` suffix to user-input prompts**

### [global] — Login/Registration

These currently have **no color codes at all**:

- `enter_city` — `"Please enter your city and state/province: "` → `"|prPlease enter your city and state/province: |in"`
- `enter_phone` — `"Please enter your phone number [(xxx) yyy-zzzz]: "` → `"|prPlease enter your phone number [(xxx) yyy-zzzz]: |in"`
- `enter_name` — `"Please enter your alias: "` → `"|prPlease enter your alias: |in"`
- `usr_pwd` — `"Password: "` → `"|prPassword: |in"`
- `select_p` — `"Select: "` → `"|prSelect: |in"`
- `tryagain` — `"Try again: "` → `"|prTry again: |in"`
- `bar_access` — `"Access code: "` → `"|prAccess code: |in"`

### [max_init] — Login Flow

- `what_first_name` — `"What is your name|!1: "` → `"|prWhat is your name|!1: |in"`
- `what_last_name` — `"What is your LAST name|!1: "` → `"|prWhat is your LAST name|!1: |in"`
- `get_pwd1` — `"Please enter the password you wish to use: "` → `"|prPlease enter the password you wish to use: |in"`
- `check_pwd2` — `"Please re-enter your password for verification: "` → `"|prPlease re-enter your password for verification: |in"`

### [max_chng] — Settings Prompts

- `current_pwd` — `"Current password: "` → `"|prCurrent password: |in"`
- `help_prompt` — `"Help Level: "` → `"|prHelp Level: |in"`
- `num_nulls` — `"Nulls (0-200): "` → `"|prNulls (0-200): |in"`
- `mon_width` — `"Monitor width (20-132): "` → `"|prMonitor width (20-132): |in"`
- `top_num` — `"Please type the number at the TOP of your display: "` → `"|prPlease type the number at the TOP of your display: |in"`
- `ask_dob` — `"|15\nEnter your date of birth [mm-dd-yyyy]: "` → `"|hi\n|prEnter your date of birth [mm-dd-yyyy]: |in"`

### [max_chat] — Chat Prompts

- `ch_node_to_page` — `"Please enter the node number to page (?=list): "` → `"|prPlease enter the node number to page (?=list): |in"`
- `ch_enter_cb` — `"Please enter CB channel number (1-255): "` → `"|prPlease enter CB channel number (1-255): |in"`
- `ch_enter_node` — `"Please enter node number to chat with (?=list): "` → `"|prPlease enter node number to chat with (?=list): |in"`

### [f_area] — File Prompts

- `loc_file` — `"Enter the text to find: "` → `"|prEnter the text to find: |in"`
- `fname_mask` — `"Filename mask: "` → `"|prFilename mask: |in"`
- `file_ul` — `"File to upload? "` → `"|prFile to upload? |in"`
- `file_to_kill` — `"File to kill? "` → `"|prFile to kill? |in"`
- `hurl_what` — `"Hurl what? "` → `"|prHurl what? |in"`
- `type_which` — `"Display which file? "` → `"|prDisplay which file? |in"`
- `contents_of` — `"Display the contents of which archive? "` → `"|prDisplay the contents of which archive? |in"`
- `full_ovr_path` — `"Enter FULL override path: "` → `"|prEnter FULL override path: |in"`
- `file_prmpt` — `"File area [Area, ...]: "` → `"|prFile area [Area, \"|!1c\"=Prior, \"|!2c\"=Next, \"|!3c\"=List]: |in"`

### [max_bor] — Editor Prompts

- `line_del_from` — → `"|prDelete FROM which line number: |in"`
- `line_del_to` — → `"|prDelete TO which line number: |in"`
- `ins_bef` — → `"|prInsert a line BEFORE which line: |in"`
- `line_edit_num` — → `"|prType the line number you wish to change: |in"`
- `rep_what` — → `"|prReplace what: |in"`
- `new_st` — → `"|prNew: |in"`
- `qstart` — → `"|prStart quoting FROM line#...: |in"`
- `qend` — → `"|prEnd quoting AT line#: |in"`
- `import_file` — → `"|prFile (specify a FULL path): |in"`

### [m_area] — Message Prompts

- `msg_prmpt` — `"Message area [Area, ...]: "` → `"|prMessage area [Area, \"|!1c\"=Prior, \"|!2c\"=Next, \"|!3c\"=List]: |in"`
- `naddr` — `"Network address: "` → `"|prNetwork address: |in"`
- `new_subj` — `"New subject: |14"` → `"|prNew subject: |in"` (was `|14` for input echo)
- `n_from` — `"   From: |14"` → `"|pr   From: |in"` (was `|14`)
- `eto` — `"|03     To: |14"` → `"|pr     To: |in"` (was `|03`/`|14`)
- `kill_which` — → `"|prKill which (\"=\" for current)? |in"`
- `fwd_which` — → `"|prForward which message (\"=\" for current)? |in"`
- `get_route_file` — → `"|prRoute file (use a FULL path): |in"`
- `hurl_which` — → `"|prHurl which message (\"=\" for current)? |in"`
- `xport_which` — → `"|prExport which message (\"=\" for current)? |in"`
- `xport_where` — → `"|prExport to where (Specify a FULL path): |in"`

### [m_browse] — Browse/Search Prompts

- `br_text_to_search` — → `"|prText to search for: |in"`
- `br_ty_from` — `"\nStart displaying at which msg#...: "` → `"\n|prStart displaying at which msg#...: |in"`
- `qwk_local_rep` — → `"|prEnter path/name of .REP file: |in"`
- `br_which_areas` — → `"|prSearch which areas (use \"*\" as wildcard): |in"`

### [max_ued] — User Editor Prompts

- `ued_find_who` — → `"|prType the name of the user you wish to find: |in"`
- `ued_sex_prompt` — → `"|prM)ale, F)emale, or N)ot Given? |in"`
- `ued_group_prompt` — → `"|prGroup number: |in"`
- `ued_getpriv` — `"[KNew priv level (# or priv key): "` → `"[K|prNew priv level (# or priv key): |in"`

### [track] — Tracking Prompts

- `trk_enter_cmt` — → `"|prEnter new comment: |in"`
- `trk_new_owner` — → `"|prName of new owner: |in"`
- `trk_new_alias` — → `"|prEnter four-character alias: |in"`
- `trk_remalias_which` — → `"|prRemove which alias (?=list): |in"`

### Select prompts with existing color

These already have `|15`...`|03` pairs that should become `|pr`...`|in`:

- `w_select_c` (L121) — `"|15Select: |03"` → `"|prSelect: |in"`
- `browse_select_s` (L1188) — `"|15Select: |03"` → `"|prSelect: |in"`

**Total: ~60-65 strings.**

---

## Batch 14: Additional Missed Strings (Found on Re-Read)

Strings not covered in Batches 1–12 that should be converted:

### Warning-tone prompts (`|03` → `|wn`)
- `pls_rsp` (L400) — `"|03\n\aPlease respond: "` → `"|wn\n\aPlease respond: "`
- `min5_left` — `"|03\n\aYOU HAVE ONLY 5 MIN. LEFT.\a\n"` → `"|wn\n\aYOU HAVE ONLY 5 MIN. LEFT.\a\n"`
- `almost_up` — `"|03\n\aTIME ALMOST UP.\a\n"` → `"|wn\n\aTIME ALMOST UP.\a\n"`
- `time_up` — `"|03\n\aTIME LIMIT.\a\n"` → `"|wn\n\aTIME LIMIT.\a\n"`

### Menu option patterns (`|14 X|07)label` → `|pr X|tx)label`)
- `ask_sex` (L861) — `"|15\nPlease enter your gender:\n\n|14  M|07)ale\n|14  F|07)emale\n\n"` → `"|hi\n|prPlease enter your gender:\n\n|pr  M|tx)ale\n|pr  F|tx)emale\n\n"`
- `help_menu` (L836) — All `|14X|07)....|15desc` → `|prX|tx)....|hidesc`
- `mb_prompt` (L1384) — `"|14\nD|07)eposit, |14W|07)ithdraw or |14Q|07)uit"` → `"|pr\nD|tx)eposit, |prW|tx)ithdraw or |prQ|tx)uit"`

### Help/info text (`|14`...`|10` → `|pr`...`|ok`)
- `achg_help` (L156) — `"|14For more areas, type \"|10..\""` → `"|prFor more areas, type \"|ok..\""` etc.
- `how_dl` (L467) — all `|14` instruction text → `|pr`
- `note_helpnf` (L441) — `"\nType \"?\" for help...\n\n"` → `"|pr\nType \"?\" for help...\n\n"`

### `|15` that should be `|hi` (additional finds)
- `chat_start` — `"|15\nCHAT: start\n"` → `"|hi\nCHAT: start\n"`
- `chat_end` — `"|15\nCHAT: end\n"` → `"|hi\nCHAT: end\n"`
- `ch_enter_chat` — `"|15\nEntering chat mode.  "` → `"|hi\nEntering chat mode.  "`
- `tnx4ul` — `"|15\nThanks for the upload, |!1.\n"` → `"|ok\nThanks for the upload, |!1.\n"` (really success)
- `recv_attach_local` — `"|15Enter destination directory..."` → `"|prEnter destination directory...|in"`

### `|12` that should be `|er` (additional finds)
- `cant_change_field` — `"|12You cannot change that field.\n|15"` → `"|erYou cannot change that field.\n|hi"`
- `hurl_cant` — `"|12Can't move msg.\n|15"` → `"|erCan't move msg.\n|hi"`
- `file_no_wc` — `"|12This command does not support wildcards.\n"` → `"|erThis command does not support wildcards.\n"`
- `qwk_toomany` — `"|12Too many areas selected...\n"` → `"|erToo many areas selected...\n"`
- `err_compr_mail` — `"|12Error compressing messages...\n|15"` → `"|erError compressing messages...\n|hi"`
- `trk_admin_noaccess` — `"|12You do not have access...\n"` → `"|erYou do not have access...\n"`

### `|10` that should be `|ok` (additional finds)
- `stat_hdr` — `"|CL|10Your statistics for "` → `"|CL|okYour statistics for "`
- `file_menu_hdr` — `"|10\n\nFile area |13"` → `"|ok\n\nFile area |sy"`
- `msg_menu_hdr` — `"|10Msg.area |13"` → `"|okMsg.area |sy"`
- `msg_thereare1` — `"|10There are "` → `"|okThere are "`

**Total: ~30-35 additional strings.**

---

## Summary Statistics

| Theme Code | Approx. Strings | Replaces            |
|------------|-----------------|---------------------|
| `\|tx`      | 30-35           | `\|07` body text     |
| `\|hi`      | 20-25           | `\|15` emphasis      |
| `\|pr`      | 95-105          | `\|14` prompts (incl. Batch 13 input prompts) |
| `\|in`      | 60-65           | Input echo (new — Batch 13 suffix) |
| `\|er`      | 30-35           | `\|12` errors        |
| `\|ok`      | 15-20           | `\|10` success       |
| `\|fi`      | 15-20           | `\|03` info text     |
| `\|hd`      | 8-10            | `\|14` headings, `\|11` |
| `\|sy`      | 5-8             | `\|13` sysop/kludge  |
| `\|qt`      | 2-3             | `\|09` quotes        |
| `\|br`      | 2-3             | `\|01`/`\|11` borders  |
| `\|tb`      | 8-10            | `\|17` background    |
| `\|dm`      | 3-5             | `\|02`/`\|05` dim     |
| `\|wn`      | 5-6             | `\|03`/`\|12` warnings |
| Skip       | ~200+           | Logs, plain, params |

**Estimated: ~240-260 strings modified out of ~400 with color codes (or no codes).**
Batch 13 alone adds ~60-65 new prompt strings that previously had no color.

---

## Color Reset Convention (`|cd`)

All strings that apply a theme color **must** end with `|cd` (reset to theme
default) so the color doesn't bleed into subsequent output.

**Exceptions — `|cd` is NOT needed when:**
- The string ends with `|in` (input echo color takes over for user typing)
- The string explicitly chains into another known color (e.g., `|pr...|hi...`)
- The string is a standalone `_col` variable (consumed as a raw color value)

**Example:**
```
# Before (bleeds yellow into next output):
please_wait = { text = "|14\nPlease wait..." }

# After (resets to theme default):
please_wait = { text = "|pr\nPlease wait...|cd" }

# Input prompt (no |cd — |in takes over):
usr_pwd = { text = "|prPassword: |in" }
```

---

## Implementation Strategy — Option C: Hybrid with Delta Architecture

**Decision: Option C (Hybrid).** Apply theme codes via a one-time batch edit
stored in the delta file, AND add `|cd` resets. Future strings use `|xx`
codes by convention.

### Source Files

- **`resources/lang/english.toml`** — Stock. Pure mechanical `.mad` conversion
  output. Never hand-edited. This is the baseline for anyone migrating their
  own `english.mad`.

- **`resources/lang/delta_english.toml`** — All hand-crafted overrides. Two
  tiers of content, distinguished by section headers:

#### Tier 1: Always-Apply (`@merge` params)

The existing `# @merge` entries (param metadata: name, type, desc, default).
These are applied to ANY `english.toml` — including user-migrated files —
because they add structural metadata without changing visible behavior.

```toml
# @merge
bad_pwd2 = { params = [{name = "got", type = "string", ...}] }
```

#### Tier 2: MaximusNG Theme Changes (`[maximusng-*]` sections)

New breakaway sections, one per heap, containing the theme color replacements
from Batches 1–14. These are **only** applied to the stock `english.toml`,
NOT to user-migrated files (since migrated `.mad` files have their own color
choices).

```toml
[maximusng-global]
# Batch 1: standalone _col variables
menu_name_col = { text = "|pr" }
menu_high_col = { text = "|hi" }
menu_opt_col  = { text = "|tx" }

# Batch 2: error messages
file_no_wc = { text = "|erThis command does not support wildcards.|cd" }
invalid_pwd = { text = "|er\n\aINVALID PASSWORD\a\n|cd" }

# Batch 3: prompts
please_wait = { text = "|pr\nPlease wait...|cd" }

# Batch 13: input prompts
usr_pwd = { text = "|prPassword: |in" }
enter_city = { text = "|prPlease enter your city and state/province: |in" }

[maximusng-f_area]
# Batch 2: error messages
badupload = { text = "|er\nThe filename '|!1' is not acceptable...|cd" }

# Batch 3: prompts
checking_ul = { text = "|pr\nPlease wait.  Verifying |!1...|cd" }

[maximusng-m_area]
# Batch 3: prompts / Batch 13: input prompts
msg_prmpt = { text = "|prMessage area [...]: |in" }

[maximusng-m_browse]
# Batch 2: error messages
qwk_invalid_area = { text = "|er\nInvalid area.  |pr(To=...|cd" }

[maximusng-max_bor]
# Batch 6: section headers
max_status = { text = "|pr|tb[K^Z=save|hi|tb  To:..." }

[maximusng-max_chat]
# Batch 5: success
ch_youare = { text = "|pr\nYou are currently |ok" }

[maximusng-max_init]
# Batch 13: login prompts
what_first_name = { text = "|prWhat is your name|!1: |in" }

[maximusng-max_chng]
# Batch 13: settings prompts
current_pwd = { text = "|prCurrent password: |in" }

[maximusng-max_ued]
# Batch 12: reclassified errors
ued_nodel_current = { text = "|er\nYou can't delete the current user!\n|cd" }

[maximusng-mexbank]
# Batch 9: label/value pattern
mb_balance_kb = { text = "|pr  KB balance |tx.........|ok : |hd" }

[maximusng-track]
# Batch 11: tracking headers
trk_rep_hdr_graph = { text = "|CL|hiID..." }
```

*(Above are representative examples — full entries will be generated per batch.)*

### Build Pipeline

1. Start with stock `resources/lang/english.toml`
2. Apply `delta_english.toml`:
   - **Tier 1** (`@merge`): merge `params` into matching keys
   - **Tier 2** (`[maximusng-*]`): replace full entries in corresponding
     `[*]` sections (e.g., `[maximusng-global]` keys → `[global]` section)
3. Output: working `english.toml` for the build

### Ship Pipeline

1. Pre-merge stock + all deltas → ship as the default `english.toml`
2. Also ship `delta_english.toml` alongside (for migration use)

### User Migration Path

When a user converts their own `english.mad`:

1. `maxcfg --convert-lang` produces a fresh `english.toml` from their `.mad`
2. Apply **Tier 1 only** (`@merge` params) from `delta_english.toml` — this
   gives them param metadata (name, type, desc, default) without overwriting
   their custom colors
3. **Skip `[maximusng-*]` sections** — their `.mad` had their own color
   choices, and those are preserved in the conversion output

This means users who migrate get structural improvements (param metadata)
but keep their own color palette. If they later want the MaximusNG theme,
they can opt in by re-running the delta apply with the `--apply-theme` flag
(or manually copying the `[maximusng-*]` values).
