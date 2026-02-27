# Dedicated EMAIL Message Area — Implementation Specification

**MaximusNG Feature Proposal**
**Author:** Kevin Morgan (Limping Ninja)
**Date:** 2025-02-23
**Status:** Draft Specification

---

## Table of Contents

1. [Overview](#1-overview)
2. [Design Principles](#2-design-principles)
3. [Flag Definition](#3-flag-definition)
4. [TOML Configuration](#4-toml-configuration)
5. [Startup Singleton Validation](#5-startup-singleton-validation)
6. [Global Email Area Handle](#6-global-email-area-handle)
7. [Area Listing Exclusion](#7-area-listing-exclusion)
8. [Browse Scan Exclusion](#8-browse-scan-exclusion)
9. [New Function: Msg_CheckEmail](#9-new-function-msg_checkemail)
10. [Supporting Functions](#10-supporting-functions)
11. [Menu Command Registration](#11-menu-command-registration)
12. [Display MCI Code](#12-display-mci-code)
13. [Interaction with Msg_Checkmail](#13-interaction-with-msg_checkmail)
14. [QWK Integration (Conference 0)](#14-qwk-integration-conference-0)
15. [File-by-File Change Summary](#15-file-by-file-change-summary)
16. [Future Extensions](#16-future-extensions)

---

## 1. Overview

Maximus currently has no dedicated "email" or "inbox" concept. Private
messages are scattered across any area with `MA_PVT` set, and the only
way to find them is `Msg_Checkmail`, which brute-force scans **every**
accessible area on the system.

This specification introduces a singleton **EMAIL** message area: a
specially-flagged area that serves as the system's dedicated private
mailbox. It is:

- **Enforced as a singleton** — exactly one area must have the flag
- **Excluded from normal area listings** — not shown in area selection
- **Excluded from normal browse scans** — `Msg_Checkmail` skips it
- **Accessed via dedicated functions** — `Msg_CheckEmail`, inbox listing,
  compose-to-email, reply-in-email

The standard `Msg_Checkmail` continues to exist unchanged — it still
scans all areas for messages addressed to the user (public and/or
private, sysop's discretion). The **EMAIL area** is the primary private
mail experience, checked at login before the general mail scan.

### Typical Login Flow

```
Login → Bulletins → Msg_CheckEmail → [act on email] → Msg_Checkmail → Main Menu
```

---

## 2. Design Principles

1. **Minimal disruption** — existing `Msg_Checkmail`, `Msg_Browse`,
   `ListMsgAreas`, and all browse infrastructure remain untouched in
   behavior. Changes are additive.

2. **Single source of truth** — the `MA2_EMAIL` flag on the area record
   is the only marker. No separate config file, no magic area name.

3. **Implied attributes** — `MA2_EMAIL` forces `MA_PVT` on and `MA_PUB`
   off. All email is private by definition. The system enforces this at
   area build time and at validation time.

4. **Fail-safe startup** — if no email area is configured, Maximus logs
   a fatal error and refuses to start. If more than one exists, same.

5. **Reuse existing plumbing** — `Msg_CheckEmail` uses the same
   `PushMsgArea`/`PopMsgArea` stack, the same `Msg_Browse` callback
   engine, the same `XMSG` / `MSGAPI` layer. No new message base code.

---

## 3. Flag Definition

### 3.1 New Constant

**File:** `src/max/msg/newarea.h`

Add after line 94 (`MA2_NOMCHK`):

```c
#define MA2_EMAIL   0x0002  /* Dedicated system email area (singleton) */
```

### 3.2 Style Keyword

**File:** `src/max/core/max_init.c`, function `parse_msg_style()` (~line 1844)

Add inside the `if (attribs2)` block, after the `NoMailCheck` check:

```c
if (stricmp(s, "Email") == 0)
    *attribs2 |= MA2_EMAIL;
```

### 3.3 Implied Attributes

**File:** `src/max/core/max_init.c`, function `parse_msg_style()` (~line 1857)

After the existing defaults block (`if attribs && ... MA_PUB`), add:

```c
/* EMAIL area implies private-only */
if (attribs2 && (*attribs2 & MA2_EMAIL))
{
    if (attribs)
    {
        *attribs |= MA_PVT;
        *attribs &= ~MA_PUB;
    }
}
```

This ensures that regardless of what other style keywords the sysop puts
on the area, an EMAIL area is always private-only.

---

## 4. TOML Configuration

### 4.1 Area Definition

In the message areas TOML (`areas/msg.toml` or equivalent):

```toml
[[areas.msg.area]]
name = "EMAIL"
description = "Private Email"
path = "/opt/maximus/msgbase/email"
style = ["Email", "Squish"]
acs = ""
```

The `Email` keyword in the `style` array sets `MA2_EMAIL`. The sysop
does not need to specify `Pvt` — it is implied.

### 4.2 Placement in Area File

The EMAIL area can appear anywhere in the area file. It does NOT need
to be in a division. It is invisible in normal listings regardless of
its position or division membership.

### 4.3 maxcfg Export

The nextgen export pipeline (`nextgen_export.c`) should recognize
`MA2_EMAIL` in `attribs_2` and emit `"Email"` in the style array when
writing area TOML.

---

## 5. Startup Singleton Validation

### 5.1 Location

**File:** `src/max/core/max_init.c`, in or immediately after `OpenAreas()`
(~line 1595–1656).

After `ham` (the message area file handle) is successfully opened, call
a new validation function:

```c
/* Validate EMAIL area singleton */
if (validate_email_area_singleton(ham) != 0)
{
    logit("!FATAL: EMAIL area validation failed");
    vbuf_flush();
    Local_Beep(3);
    maximus_exit(ERROR_CRITICAL);
}
```

### 5.2 Validation Function

**File:** `src/max/core/max_init.c` (new static function)

```c
/**
 * @brief Scan all message areas for exactly one MA2_EMAIL area.
 *
 * On success, caches the area name in g_email_area[].
 *
 * @param  haf  Open message area file handle
 * @return 0 on success (exactly one EMAIL area found)
 *        -1 on failure (zero or multiple EMAIL areas)
 */
static int near validate_email_area_singleton(HAF haf)
{
    HAFF haff;
    MAH ma;
    int count = 0;
    char found_name[MAX_ALEN];

    memset(&ma, 0, sizeof ma);
    found_name[0] = '\0';

    haff = AreaFileFindOpen(haf, NULL, 0);
    if (haff == NULL)
    {
        logit("!No message areas found — cannot validate EMAIL area");
        return -1;
    }

    while (AreaFileFindNext(haff, &ma, FALSE) == 0)
    {
        if (ma.ma.attribs_2 & MA2_EMAIL)
        {
            count++;
            if (count == 1)
                strnncpy(found_name, MAS(ma, name), sizeof(found_name));
        }
        DisposeMah(&ma);
        memset(&ma, 0, sizeof ma);
    }

    DisposeMah(&ma);
    AreaFileFindClose(haff);

    if (count == 0)
    {
        logit("!FATAL: No message area with Email style configured. "
              "Exactly one area must have the Email style.");
        return -1;
    }

    if (count > 1)
    {
        logit("!FATAL: %d message areas have the Email style. "
              "Only one area may be designated as Email.", count);
        return -1;
    }

    /* Cache the EMAIL area name globally */
    strnncpy(g_email_area, found_name, sizeof(g_email_area));

    if (debuglog)
        debug_log("validate_email_area_singleton: found '%s'", g_email_area);

    return 0;
}
```

---

## 6. Global Email Area Handle

### 6.1 Declaration

**File:** `src/max/core/max_v.h` (~line 373, near other `extrn char` globals)

```c
extrn char g_email_area[MAX_ALEN];  /* Area tag of the singleton EMAIL area */
```

### 6.2 Initialization

**File:** `src/max/core/max_init.c`, in the initialization block (~line 917):

```c
g_email_area[0] = '\0';
```

Populated by `validate_email_area_singleton()` at startup.

### 6.3 Access Pattern

Any code that needs to work with the email area references
`g_email_area` directly. It is always populated if Maximus started
successfully. Example:

```c
if (g_email_area[0] != '\0')
{
    /* We have a valid email area */
    if (PushMsgArea(g_email_area, &bi))
    {
        /* ... operate on email area ... */
        PopMsgArea();
    }
}
```

---

## 7. Area Listing Exclusion

The EMAIL area must not appear in normal area listings. Users access it
via dedicated email commands, not by selecting it from an area list.

### 7.1 Legacy Scroll Listing

**File:** `src/max/msg/m_area.c`, function `ListMsgAreas()` (~line 1252)

In the display loop, after the existing `MA_HIDDN` check, add an
`MA2_EMAIL` check:

```c
/* Current code (line ~1252-1255): */
if ((!div_name || ma.ma.division==this_div+1) &&
    (ma.ma.attribs & MA_HIDDN)==0 &&
    /* ADD: skip EMAIL area in normal listings */
    (ma.ma.attribs_2 & MA2_EMAIL)==0 &&
    (((ma.ma.attribs & MA_DIVBEGIN) && PrivOK(MAS(ma, acs), TRUE)) ||
     ValidMsgArea(NULL, &ma, VA_NOVAL, &bi)))
```

### 7.2 Lightbar Listing

**File:** `src/max/msg/m_area.c`, function `lb_collect_msg_areas()` (~line 785)

In the `if (show && ...)` filter, add the EMAIL exclusion:

```c
/* Current code (line ~785): */
if (show && (ma.ma.attribs & MA_HIDDN) == 0
    /* ADD: skip EMAIL area */ 
    && (ma.ma.attribs_2 & MA2_EMAIL) == 0)
```

### 7.3 Area Search (Next/Prior)

**File:** `src/max/msg/m_area.c`, function `SearchArea()` (~line 78)

In the search loop, add the EMAIL exclusion:

```c
/* Current code (line ~78-80): */
if ((ma.ma.attribs & MA_HIDDN)==0 &&
    /* ADD: skip EMAIL area in area navigation */
    (ma.ma.attribs_2 & MA2_EMAIL)==0 &&
    ValidMsgArea(NULL, &ma, VA_VAL | VA_PWD | VA_EXTONLY, pbi))
```

This prevents `+` / `-` area navigation from landing on the EMAIL area.

---

## 8. Browse Scan Exclusion

The EMAIL area must be skipped during `Msg_Checkmail`'s `BROWSE_AALL`
scan. Email has its own dedicated check function.

### 8.1 Browse_Scan_This_Area

**File:** `src/max/msg/mb_novl.c`, function `Browse_Scan_This_Area()`
(~line 479–486)

After the existing `MA2_NOMCHK` exclusion, add `MA2_EMAIL`:

```c
/* Current code (line ~481-486): */
if ((pmah->ma.attribs_2 & MA2_NOMCHK) && b->first &&
    (b->first)->attr==MSGREAD && (b->first)->flag==(SF_NOT_ATTR|SF_OR) &&
    (b->first)->where==WHERE_TO)
{
    return FALSE;   /* Skip this area if this is a mailcheck */
}

/* ADD: Always skip the EMAIL area in browse scans.
 * Email is accessed exclusively via Msg_CheckEmail(). */
if (pmah->ma.attribs_2 & MA2_EMAIL)
    return FALSE;
```

**Note:** This unconditionally skips the EMAIL area for ALL browse
operations (`BROWSE_AALL`, `BROWSE_ATAG`, etc.), not just mail checks.
The EMAIL area is never part of the normal message browsing flow. If the
user is explicitly in the EMAIL area (via `Msg_CheckEmail` or a direct
push), the browse operates on `BROWSE_ACUR` which matches `usr.msg` and
doesn't go through `Browse_Scan_This_Area`'s multi-area path.

### 8.2 Tagging

The EMAIL area should also be excluded from tag operations. Since
`Browse_Scan_This_Area` already skips it, tagged-area scans
(`BROWSE_ATAG`) will naturally skip it. But for the tag *selection* UI,
we should also exclude it — this happens via the same `ListMsgAreas`
changes in §7 above, since the tag UI uses the same area listing
function.

---

## 9. New Function: Msg_CheckEmail

### 9.1 Purpose

`Msg_CheckEmail()` is the dedicated email check function. It:

1. Pushes into the EMAIL area
2. Scans for unread messages addressed to the current user
3. Presents them for reading (using the standard browse engine)
4. Pops back to the original area

### 9.2 Implementation

**File:** `src/max/msg/m_browse.c` (add after `Msg_Checkmail()`)

```c
/**
 * @brief Check the dedicated EMAIL area for unread messages to this user.
 *
 * Pushes into g_email_area, runs a browse scan for unread messages
 * addressed to usr.name or usr.alias, presents them for reading,
 * and pops back to the caller's original area.
 *
 * This is the primary email experience, typically called at login
 * before the general Msg_Checkmail scan.
 *
 * @param menuname  Optional menu name context (may be NULL)
 */
void Msg_CheckEmail(char *menuname)
{
    SEARCH first, *s;
    BARINFO bi;
    int saved_in_mcheck;

    /* If no EMAIL area is configured, silently return */
    if (g_email_area[0] == '\0')
        return;

    /* Save and push into the EMAIL area */
    if (!PushMsgArea(g_email_area, &bi))
    {
        logit("!Msg_CheckEmail: cannot push to EMAIL area '%s'", g_email_area);
        return;
    }

    /* Build the search: unread messages addressed to us */
    Init_Search(&first);
    first.txt = strdup(usr.name);
    first.attr = MSGREAD;
    first.flag = SF_NOT_ATTR | SF_OR;
    first.where = WHERE_TO;
    first.next = NULL;

    /* Also search by alias if the user has one */
    if (*usr.alias && (first.next = s = malloc(sizeof(SEARCH))) != NULL)
    {
        Init_Search(s);
        s->txt = strdup(usr.alias);
        s->attr = MSGREAD;
        s->flag = SF_NOT_ATTR | SF_OR;
        s->where = WHERE_TO;
    }

    /* Use BROWSE_ACUR to scan only the EMAIL area, with READ mode
     * to present messages for reading, and EXACT for name matching */
    saved_in_mcheck = in_mcheck;
    in_mcheck = TRUE;

    Msg_Browse(BROWSE_ACUR | BROWSE_NEW | BROWSE_EXACT | BROWSE_READ,
               &first, menuname);

    in_mcheck = saved_in_mcheck;

    /* Pop back to the original area */
    PopMsgArea();
}
```

### 9.3 Key Design Decisions

- Uses `BROWSE_ACUR` (not `BROWSE_AALL`) — only scans the EMAIL area
- Uses `PushMsgArea`/`PopMsgArea` — restores the user's previous area
  context automatically
- Sets `in_mcheck = TRUE` — enables the "mail check reply" keyboard
  shortcut in the reader
- Reuses the same `Init_Search` / `SEARCH` structures as `Msg_Checkmail`
- Falls back gracefully if the EMAIL area can't be opened

---

## 10. Supporting Functions

### 10.1 Email_Compose — Send Email to a User

**File:** `src/max/msg/m_browse.c` (or a new `m_email.c`)

```c
/**
 * @brief Compose a new email message in the EMAIL area.
 *
 * Pushes into the EMAIL area, enters the message editor with
 * MSGPRIVATE forced on, and pops back.
 *
 * @param to  Optional recipient name. If NULL, prompts the user.
 */
void Email_Compose(char *to)
{
    BARINFO bi;
    XMSG msg;

    if (g_email_area[0] == '\0')
        return;

    if (!PushMsgArea(g_email_area, &bi))
    {
        logit("!Email_Compose: cannot push to EMAIL area '%s'", g_email_area);
        return;
    }

    memset(&msg, 0, sizeof msg);

    /* Force private */
    msg.attr |= MSGPRIVATE | MSGLOCAL;

    /* Set "From:" to the user's name (respecting area's name policy) */
    if (mah.ma.attribs & MA_ALIAS && *usr.alias)
        strcpy(msg.from, usr.alias);
    else
        strcpy(msg.from, usr.name);

    /* Set "To:" if provided */
    if (to && *to)
        strnncpy(msg.to, to, XMSG_TO_SIZE);

    /* Enter the editor — GetMsgAttr handles the header prompts */
    if (GetMsgAttr(&msg, &mah, MAS(mah, name), 0, MsgGetHighMsg(sq)) != -1)
    {
        int rc = Editor(&msg, NULL, 0L, NULL, NULL);

        display_line = display_col = 1;

        if (rc != ABORT && rc != LOCAL_EDIT)
        {
            if (!((last_maxed ? num_lines : linenum) == 1 &&
                  screen[1] && screen[1][1] == '\0'))
            {
                SaveMsg(&msg, NULL, FALSE, 0, FALSE, &mah,
                        MAS(mah, name), sq, NULL, NULL, TRUE);
            }
        }
    }

    PopMsgArea();
}
```

### 10.2 Email_Inbox — List All Email

**File:** `src/max/msg/m_browse.c` (or `m_email.c`)

```c
/**
 * @brief List all messages in the EMAIL area addressed to this user.
 *
 * Shows both read and unread email in a list format. Unlike
 * Msg_CheckEmail (which only shows unread), this shows everything.
 *
 * @param menuname  Optional menu name context (may be NULL)
 */
void Email_Inbox(char *menuname)
{
    SEARCH first, *s;
    BARINFO bi;

    if (g_email_area[0] == '\0')
        return;

    if (!PushMsgArea(g_email_area, &bi))
        return;

    Init_Search(&first);
    first.txt = strdup(usr.name);
    first.flag = SF_OR;
    first.where = WHERE_TO;
    first.next = NULL;

    if (*usr.alias && (first.next = s = malloc(sizeof(SEARCH))) != NULL)
    {
        Init_Search(s);
        s->txt = strdup(usr.alias);
        s->flag = SF_OR;
        s->where = WHERE_TO;
    }

    /* BROWSE_ACUR + BROWSE_FROM (all messages, not just new) +
     * BROWSE_LIST (list mode) + BROWSE_EXACT (exact name match) */
    Msg_Browse(BROWSE_ACUR | BROWSE_FROM | BROWSE_EXACT | BROWSE_LIST,
               &first, menuname);

    PopMsgArea();
}
```

### 10.3 Email_Reply — Reply to Email

No new function needed. The existing `Msg_Reply()` works correctly when
the user is in the EMAIL area (via `Msg_CheckEmail` or `Email_Inbox`).
Since `Msg_CheckEmail` uses `BROWSE_ACUR` and the read mode, the
standard reply key (`R`) already works — it composes a reply in the
current area, which is the EMAIL area.

The `chkmail_reply` flag (set in `mb_read.c`) ensures lastread pointers
aren't corrupted during mail-check replies. This mechanism works
identically for `Msg_CheckEmail` since we set `in_mcheck = TRUE`.

### 10.4 Email_AreaName — Get Email Area Name

Simple accessor for code that needs to reference the email area:

```c
/**
 * @brief Return the name of the system's EMAIL area.
 *
 * @return Pointer to g_email_area (never NULL, but may be empty
 *         if called before startup validation).
 */
const char *Email_AreaName(void)
{
    return g_email_area;
}
```

### 10.5 IsEmailArea — Test if an Area is the EMAIL Area

Utility for code that needs to test dynamically:

```c
/**
 * @brief Test whether the given area name is the EMAIL area.
 *
 * @param aname  Area name to test
 * @return TRUE if aname matches g_email_area, FALSE otherwise
 */
int IsEmailArea(const char *aname)
{
    if (g_email_area[0] == '\0' || aname == NULL)
        return FALSE;
    return eqstri((char *)aname, g_email_area);
}
```

---

## 11. Menu Command Registration

### 11.1 Option Enum

**File:** `src/max/core/option.h` (~line 51)

Add new option values at the end of `MSG_BLOCK` (before `FILE_BLOCK`):

```c
MSG_BLOCK=400, same_direction, read_next, read_previous,
               enter_message, msg_reply, read_nonstop,
               read_original, read_reply, msg_list, msg_scan,
               msg_inquir, msg_kill, msg_hurl, forward, msg_upload,
               xport, read_individual, msg_checkmail, msg_change,
               msg_tag, msg_browse, msg_current, msg_edit_user,
               msg_upload_qwk, msg_toggle_kludges, msg_unreceive,
               msg_restrict, msg_area, msg_track, msg_dload_attach,
               msg_reply_area, msg_listtest,
               msg_checkemail, msg_email_compose, msg_email_inbox,
```

### 11.2 Menu Registration Table

**File:** `src/max/core/max_rmen.c` (~line 259)

Add entries to the menu name table:

```c
{"msg_checkemail", msg_checkemail},
{"msg_email_compose", msg_email_compose},
{"msg_email_inbox", msg_email_inbox},
```

### 11.3 SILT Table (Legacy Compatibility)

**File:** `src/max/core/option.h` (~line 101, inside `#ifdef SILT`)

Add to `silt_table[]`:

```c
{msg_checkemail,    "msg_checkemail"},
{msg_email_compose, "msg_email_compose"},
{msg_email_inbox,   "msg_email_inbox"},
```

### 11.4 Command Dispatch

**File:** `src/max/msg/m_intrin.c`, function `Exec_Msg()` (~line 93)

Add cases before the `default:`:

```c
case msg_checkemail:    Msg_CheckEmail(menuname);       break;
case msg_email_compose: Email_Compose(arg);             break;
case msg_email_inbox:   Email_Inbox(menuname);          break;
```

---

## 12. MECCA Display Code — Not Implemented

The `0x17`-prefixed MECCA control codes in `disp_max.c` are compiled
binary sequences from `.mec` source files. All single-letter codes are
already allocated by the legacy MECCA command set.

**Decision:** No MECCA display code is added for `Msg_CheckEmail`.
Email check is invoked exclusively through:

- **Menu commands:** `msg_checkemail`, `msg_email_compose`, `msg_email_inbox`
- **MEX scripts:** via future MEX intrinsic bindings

This is consistent with how email should be triggered — from menu
definitions and programmatic control, not from static display files.

---

## 13. Interaction with Msg_Checkmail

### 13.1 Msg_Checkmail Unchanged

`Msg_Checkmail()` in `m_browse.c:681-703` is **not modified**. It
continues to scan all areas (`BROWSE_AALL`) for messages addressed to
the user. The only behavioral change is that the EMAIL area is
**skipped** during this scan, because `Browse_Scan_This_Area()` now
returns `FALSE` for `MA2_EMAIL` areas (§8.1).

### 13.2 Why Both Functions Coexist

| Function | Purpose | Scope | When Called |
|----------|---------|-------|-------------|
| `Msg_CheckEmail` | Check dedicated private mailbox | EMAIL area only (`BROWSE_ACUR`) | Login flow, `msg_checkemail` menu command, MEX |
| `Msg_Checkmail` | Scan for messages directed to user | All areas except EMAIL (`BROWSE_AALL`) | Post-login, MECCA `*C` code, `msg_checkmail` menu command |

The two functions address different user expectations:

- **"Do I have private email?"** → `Msg_CheckEmail` (fast, one area)
- **"Did anyone reply to me in any area?"** → `Msg_Checkmail` (thorough, all areas)

### 13.3 No Double-Display

Because `Browse_Scan_This_Area` skips `MA2_EMAIL` areas, messages in the
EMAIL area are **never** shown by `Msg_Checkmail`. There is no risk of
the user seeing the same email twice.

---

## 14. QWK Integration (Conference 0)

The EMAIL area maps to **QWK conference 0** in the QWK networking spec.

### 14.1 QWK Download (mb_qwk.c)

When building a QWK packet, the EMAIL area is handled specially:

- Conference number = 0 (hardcoded, not assigned by `InsertAkh`)
- NDX file = `000.NDX`
- Only messages addressed to the downloading user are included (same
  filter as `Msg_CheckEmail`)

### 14.2 QWK Upload (mb_qwkup.c)

When tossing a REP packet:

- Messages with conference number 0 are tossed into `g_email_area`
- No need for area-name lookup — conference 0 always maps to the EMAIL area

### 14.3 QWK Network (qwknet)

- Inbound netmail (routed or direct) → delivered to `g_email_area`
- Outbound netmail → packed from `g_email_area` into conference 0
- NETFLAGS.DAT byte 0 = 1 (conference 0 is always "networked" for netmail)

---

## 15. File-by-File Change Summary

### New Functions

| Function | File | Description |
|----------|------|-------------|
| `Msg_CheckEmail()` | `src/max/msg/m_browse.c` | Dedicated email check — push to EMAIL area, browse unread, pop |
| `Email_Compose()` | `src/max/msg/m_browse.c` or new `m_email.c` | Compose private email — push to EMAIL area, editor, pop |
| `Email_Inbox()` | `src/max/msg/m_browse.c` or new `m_email.c` | List all email — push to EMAIL area, list browse, pop |
| `Email_AreaName()` | `src/max/msg/m_browse.c` or new `m_email.c` | Return `g_email_area` |
| `IsEmailArea()` | `src/max/msg/m_browse.c` or new `m_email.c` | Test if an area name is the EMAIL area |
| `validate_email_area_singleton()` | `src/max/core/max_init.c` | Startup validation — scan for exactly one `MA2_EMAIL` area |

### Modified Files

| File | Section | Change |
|------|---------|--------|
| `src/max/msg/newarea.h` | Line ~94 | Add `#define MA2_EMAIL 0x0002` |
| `src/max/core/max_init.c` | `parse_msg_style()` | Add `"Email"` keyword → `MA2_EMAIL`, implied attributes |
| `src/max/core/max_init.c` | `OpenAreas()` | Call `validate_email_area_singleton()` |
| `src/max/core/max_init.c` | Init block | Initialize `g_email_area[0] = '\0'` |
| `src/max/core/max_v.h` | Globals | Add `extrn char g_email_area[MAX_ALEN]` |
| `src/max/core/option.h` | `MSG_BLOCK` enum | Add `msg_checkemail`, `msg_email_compose`, `msg_email_inbox` |
| `src/max/core/option.h` | `silt_table[]` | Add string mappings for new options |
| `src/max/core/max_rmen.c` | Menu table | Add `{"msg_checkemail", msg_checkemail}` etc. |
| `src/max/core/protod.h` | Prototypes | Add `Msg_CheckEmail()`, `Email_Compose()`, `Email_Inbox()`, `Email_AreaName()`, `IsEmailArea()` |
| `src/max/msg/m_intrin.c` | `Exec_Msg()` | Add `case msg_checkemail:` etc. |
| `src/max/msg/m_browse.c` | After `Msg_Checkmail()` | Add `Msg_CheckEmail()` implementation |
| `src/max/msg/mb_novl.c` | `Browse_Scan_This_Area()` | Add `MA2_EMAIL` exclusion |
| `src/max/msg/m_area.c` | `ListMsgAreas()` | Add `MA2_EMAIL` exclusion in display filter |
| `src/max/msg/m_area.c` | `lb_collect_msg_areas()` | Add `MA2_EMAIL` exclusion |
| `src/max/msg/m_area.c` | `SearchArea()` | Add `MA2_EMAIL` exclusion |
| `src/max/display/disp_max.c` | Display codes | Add `'E'` case for `Msg_CheckEmail()` |
| `src/apps/maxcfg/.../nextgen_export.c` | Area export | Recognize `MA2_EMAIL` → emit `"Email"` style keyword |

### Optionally New File

| File | Description |
|------|-------------|
| `src/max/msg/m_email.c` | Dedicated email functions (if we prefer not to grow `m_browse.c`) |

If `m_email.c` is created, it would contain `Email_Compose()`,
`Email_Inbox()`, `Email_AreaName()`, and `IsEmailArea()`.
`Msg_CheckEmail()` should stay in `m_browse.c` alongside
`Msg_Checkmail()` for consistency.

---

## 16. Future Extensions

### 16.1 Email Notification at Login

Display a count of unread email before entering the read loop:

```
You have 3 unread email messages.
```

This could be a fast count (iterate EMAIL area, count unread messages
addressed to us) without entering the full browse engine. Add as a
lang string: `email_count = "You have |!1 unread email message(s).\n"`

### 16.2 Email-Specific Display Files

Support `email_header.ans`, `email_reader.ans` etc. for custom email
presentation separate from the general message reader.

### 16.3 Email Forwarding to Area

Allow users to forward an email to a public area, or forward a public
message to someone's email. The existing `Msg_Forward()` could be
extended to detect when the target area is the EMAIL area and force
`MSGPRIVATE`.

### 16.4 Sysop Email Commands

- `Email_SendToAll()` — compose an email to all users (mass mailing)
- `Email_Purge()` — clean up old read email beyond a configurable age

### 16.5 MEX Integration

Expose email functions to MEX scripts:

```c
msg_checkemail();       /* Check email from MEX */
email_compose("User");  /* Send email to a user */
email_inbox();          /* Show inbox from MEX */
email_area_name();      /* Get email area tag */
is_email_area("NAME");  /* Test if area is email */
```
