# Dungeon Escape - Style Guide

This is a quick reference to keep our code consistent.

---

## 1. Naming Conventions
- **Classes:** PascalCase → `Player`, `Room`, `CommandParser`
- **Methods:** camelCase → `addItem()`, `getStatus()`
- **Member variables:** camelCase → `roomDescription`, `playerLocation`
- **Constants:** ALL_CAPS → `MAX_ROOMS`, `DEFAULT_HEALTH`

---

## 2. Braces & Indentation
- **Brace style:** K&R  
  ```cpp
  if (condition) {
      doThing();
  } else {
      otherThing();
  }
  ```
- **Indentation:** 4 spaces, no tabs

---

## 3. File Organization
- One class per `.hpp/.cpp` pair
- Use `#pragma once` in headers
- Keep `main()` in `Game.cpp` only

---

## 4. Comments & Documentation
- Short comment above each class and method  
- Use `//` for quick inline notes, `/* ... */` for larger blocks

---

## 5. Git Workflow
- New feature → new branch (`feature-parser`, `feature-room-system`)
- Use pull requests / reviews before merging
- Commit messages: short and descriptive →  
  ✅ "Add inventory system"  
  ❌ "stuff"

---
