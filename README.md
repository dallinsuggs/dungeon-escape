# dungeon-escape

Text adventure set inside a dungeon

\# Dungeon Escape



\*\*Text adventure set inside a dungeon.\*\*



---



\## 📖 Design Document



\### Summary

The player has been locked in a dungeon and must escape.



---



\## 🎯 Core Scope

The timeframe is \*\*2–4 weeks\*\*, so the game will be relatively simple:



\- \*\*Game world:\*\* 5–8 “rooms” or locations  

\- \*\*Player state:\*\* inventory to hold items, possibly health, etc.  

\- \*\*Interactions:\*\* picking up (and maybe dropping) items, moving between rooms, puzzles, maybe an enemy encounter  

\- \*\*Ending:\*\* 1–2 possible endings depending on choices  



---



\## 🏗️ Classes and Structure



\## 📂 Classes and Responsibilities



| Class          | Responsibilities                          | Who Owns It? \*(Speculative)\* |

|----------------|-------------------------------------------|-------------------------------|

| \*\*Game\*\*       | Main loop, input/output, game state manager | Dallin                        |

| \*\*Player\*\*     | Inventory, location, stats                | Both                          |

| \*\*Room\*\*       | Name, description, items, connected rooms | Nathan                        |

| \*\*Item\*\*       | Name, description, effect                 | Nathan                        |

| \*\*CommandParser\*\* | Parse player input into actions        | Dallin                        |

| \*\*FileManager\*\* \*(optional)\* | Save/load game              | Both                          |





---



\## 🌱 GitHub Workflow

\- Use \*\*branches for features\*\* (e.g., `room`, `inventory`, `parser`) to avoid overwriting each other’s work.  

\- \*\*Merge after review\*\* to maintain stability.  



---



\## 🥅 Initial Objectives

1\. Hardcode a few rooms and items  

2\. Implement movement between rooms (north, south, etc.)  

3\. Add item pickup/drop and inventory  

4\. Add simple puzzles or branching endings  

5\. \*(Optional)\* Add save/load functionality  



---



\## 🧪 Testing \& Validation

\- Test movement between rooms  

\- Test picking up and dropping items  

\- Test player state (inventory count, flags)  

\- Possibly write `tests.cpp` to simulate a short “playthrough” automatically  



---



\## 🗺️ Game World Layout

With 5–8 rooms we’ll keep things manageable but interesting. Example layout:  



\- \*\*Entrance Area\*\* – starting point, maybe a locked door puzzle  

\- \*\*Guard Room\*\* – could have a key or item  

\- \*\*Armory\*\* – contains an item or hint  

\- \*\*Library/Office\*\* – puzzle to read a code or riddles  

\- \*\*Prison Cell\*\* – optional mini challenge  



---



\## 🔑 Items \& Puzzles

\### Items

\- Key  

\- Torch  

\- Scroll  

\- Lever  

\- Rock  



\### Puzzles

\- Combine items (e.g., \*use key on door\*)  

\- Solve riddles in text  

\- Move items to trigger hidden passages  

\- Each puzzle should unlock the next step to keep progression flowing  



---



\## 👤 Player \& Game State

\*\*Player attributes:\*\*

\- Current room  

\- Inventory (vector of items)  

\- \*(Optional)\* Flags like “torch lit” or “found secret lever”  



\*\*Game state management:\*\*

\- Track solved puzzles, used items, and unlocked rooms  

\- End the game when the player reaches the exit room  



---



