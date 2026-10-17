# DSLib - Smart Hostel Lost & Found Verification System

**Team ClaimX (Team #20), Graphic Era (Deemed to be) University, Dehradun**
Project code: DSCPP-III-2026-T020

| Role | Name | Student ID |
|------|------|-----------|
| Team Lead | Divyansh Mishra | 2510010426 |
| Member | Sharmistha Anand | 2510350057 |
| Member | Sudiksha Deshwal | 2510012126 |
| Member | Anushka Yadav | 2510012180 |

Repository: https://github.com/divyansh1503/ClaimX

---

## 1. What is this?

Hostels usually handle lost belongings through WhatsApp groups or notice boards. There is no record, no way to know if an item was already found, and anyone who repeats the posted description can claim it.

DSLib is a console program (C++17, standard library only) that fixes this:

1. Students **report lost items**; finders **report found items** separately.
2. Each report carries one **private detail** (e.g. "red rubber band on the charger"). Found items also carry a **follow-up question**.
3. The system **ranks likely matches** using Levenshtein distance, so descriptions do not have to be worded the same way.
4. To claim an item, the student answers the follow-up question. If the answer lines up with the private detail the claim goes to the admin as **PENDING**. If not, it is **FLAGGED** for manual review - never rejected automatically.
5. The admin approves or rejects claims (ID cards and keys first). Every decision is kept in a **reviewable log**, and the last decision can be undone.
6. Everything is saved to text files between sessions.

---

## 2. Features

- Report lost / found items (duplicate or empty IDs are rejected)
- Search by ID (hash table)
- Sorted views (merge sort, quick sort) and binary search by ID
- View items by priority: High (ID card, key), Medium (wallet, phone, laptop, charger...), Low (everything else)
- Match suggestions with a score (exact matching in Part 1, Levenshtein in Part 2)
- Prefix search on words in reports (trie)
- Browse by category or location (hash index written in C)
- Claim submission with private-detail answer check
- Admin panel: claim queue, approve/reject, decision log, undo, sorted views, statistics
- File persistence for items, claims and decisions

---

## 3. Data structures used

| Structure | Where | Purpose | Main cost |
|-----------|-------|---------|-----------|
| Singly linked list (own, tail pointer) | `ItemLinkedList.h` | Stores and owns all item records | add O(1), find O(n) |
| Hash table (`unordered_map`) | `LostFoundSystem.h` | Search by item ID | O(1) average |
| Hash table (own, in C) | `c_ds/hashindex.c` | Category and location index | O(1) average |
| Trie | `Trie.h` | Prefix search on words | O(length of prefix + results) |
| Priority queue | `LostFoundSystem.h`, `ClaimManager.h` | Priority view; claim queue (ID cards/keys first, then oldest) | push/pop O(log n) |
| Stack (own, in C) | `c_ds/stack.c` | Admin decision log + undo (LIFO) | push/pop O(1) |
| Merge sort (own) | `Sorting.h` | Sorted view by priority (stable) | O(n log n) |
| Quick sort (own) | `Sorting.h` | Sorted views by name / ID | O(n log n) average, O(n^2) worst |
| Binary search (own) | `Sorting.h` | Find an item in the ID-sorted list | O(log n) |
| Levenshtein distance | `Levenshtein.h` | Fuzzy matching of fields and answers | O(n*m) |

**C and C++ mix:** the stack and hash index are written in C (structs, pointers, `malloc`/`free`) and called from the C++ classes. The C++ side uses OOP: `Item` is an abstract base class, `LostItem` and `FoundItem` inherit from it.

### Matching rules
- Each of 5 fields (category, name, colour, location, description) counts only if similarity >= **0.60**.
- Score = average of the field values x 100.
- A found item is suggested when the score is >= **45%**.
- Private-detail agreement is shown next to the suggestion and breaks ties.

---

## 4. Project structure

```
DSLib/
  include/
    Utils.h            helpers (string cleanup, safe input)
    Item.h             Item (abstract), LostItem, FoundItem
    ItemLinkedList.h   own linked list
    FileManager.h      save / load items.txt
    MatchingEngine.h   match scoring
    LostFoundSystem.h  menu + core features
    Levenshtein.h      edit distance + similarity
    Trie.h             prefix search
    Sorting.h          merge sort
    ItemIndex.h        C++ wrapper for the C hash index
    Claim.h            claim record
    ClaimManager.h     claims, admin panel, log, statistics
  c_ds/
    stack.h / stack.c            stack in C
    hashindex.h / hashindex.c    hash table in C
  src/
    main.cpp
  data/                items.txt, claims.txt, decisions.txt (created automatically)
  docs/
  Makefile
  README.md
```

---

## 5. Build and run

Requirements: `g++` and `gcc` with C++17 / C11 support, and `make`.

```bash
make          # builds ./dslib
./dslib       # run the program
make clean    # remove build files
```

If `make` says "missing separator", the recipe lines in the Makefile must start with a **tab**, not spaces.

Without `make`:

```bash
gcc -std=c11 -Ic_ds -c c_ds/stack.c -o stack.o
gcc -std=c11 -Ic_ds -c c_ds/hashindex.c -o hashindex.o
g++ -std=c++17 -Iinclude -Ic_ds src/main.cpp stack.o hashindex.o -o dslib
```

---

## 6. How to use

### Student menu
| Option | Action |
|--------|--------|
| 1 / 2 | Report a lost / found item (asks for the private detail; found items also ask for a follow-up question) |
| 3 | Search item by ID |
| 4 | View items by priority |
| 5 | Suggest matches for a lost item |
| 6 | List all items |
| 7 | Sorted views (priority / name / ID) and binary search by ID |
| 8 | Claim a found item |
| 9 | Search by word prefix (trie) |
| 10 | Browse by category or location |
| 11 | Admin panel |
| 0 | Save and exit |

### Admin panel
Demo login: **username `admin`, password `geu@2026`** (hard-coded for the demo only).

1. View claim queue (priority order)
2. Process next claim (approve / reject / skip)
3. View decision log (newest first)
4. Undo last decision
5. Sorted views (items by priority / name / ID, claims by answer match) and binary search by ID
6. Statistics dashboard

### Example run
1. Report lost item `L001` (Black Backpack, private detail "red rubber band on zip").
2. Report found item `F001` (Black Bagpack, private detail "red rubber band", question "What is tied on the zip?").
3. Option 5 on `L001` suggests `F001` with a high score ("private details agree").
4. Option 8: claimant answers "red rubber-band" -> claim is PENDING.
5. Option 11 -> approve it -> item becomes CLAIMED and the decision appears in the log.

---

## 7. Data files

All files are tab-separated text in `data/`.

- `items.txt`: type, id, name, category, colour, location, description, priority, reporter, private detail, question, status
- `claims.txt`: id, item id, claimant, answer, status, match %, sequence, admin note
- `decisions.txt`: claim id, item id, claimant, admin, approved (1/0), time, note (oldest first)

Tabs typed into any field are converted to spaces so they cannot break the file format.

---

## 8. Testing status

| Test | Result |
|------|--------|
| Report lost and found item, saved to `items.txt` | Pass |
| Duplicate or empty ID rejected | Pass |
| Search by ID, priority view (High before Medium and Low) | Pass |
| Exact and fuzzy match suggestion (spelling, plurals, spacing) | Pass |
| Reload after restart (items, claims, decisions) | Pass |
| Prefix search and category/location browse | Pass |
| Matching answer -> PENDING; mismatched answer -> FLAGGED | Pass |
| Claim queue order (ID cards and keys first) | Pass |
| Approve, reject, log, undo | Pass |
| Memory check with AddressSanitizer | No errors |

Test cases for the report: *successful match*, *mismatched claim*, *no match found*.

---

## 9. Known limitations

- "bag" and "backpack" are not recognised as the same word (too different by edit distance); the private-detail check covers this case.
- The category index treats "bag" and "bags" as different keys.
- The 0.60 and 45% thresholds were chosen by trying examples and need more testing.
- The admin password is hard-coded (demo only).
- All records are rewritten to the file after every change; fine for a hostel, not for large data.
