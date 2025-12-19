1. Choosing the Console → Atari ST

There were three possible options: Amiga 500, Macintosh Plus, and Atari ST.
At first, I chose the Macintosh Plus because I liked its hardware design. However, since this project focuses on game production, I decided to switch to the Atari ST, a console that left a strong mark on game history—whether positive or negative.

⸻

2. Setting Up the Development Environment (Mac-based)

2.1 Hatari (Emulator)
First, I installed an emulator. Since transferring the project to real hardware takes a lot of time, testing within the development environment was essential.
The Atari ST emulator I used is called Hatari. It is still actively updated and easy to use.

2.2 Installing the Compiler
This was the most difficult step of the entire project. I almost changed consoles again, but eventually succeeded.

To run code on the Atari ST, it must be compiled into a .prg format. This required a compatible compiler. Because I worked on macOS, I needed a Mac-compatible compiler. However, most available compilers were designed for Windows, so finding and installing the right one involved many trials and errors.

Details about installing the compiler are explained in the Guideline section.

⸻

3. Deciding the Game Concept

3.1 Concept and Conditions
There were two elements I strongly wanted to include in the game:

1) Something modern that did not exist at the time
One of the most interesting aspects of creating a retro game today is the mismatch between eras. I believed that displaying modern elements—things that did not exist back then—within the Atari’s 16-color limitation and on a CRT monitor would create an appealing sense of contrast.

2) Personal experience
This is a condition I apply to all my work. I believe that the more personal a story is, the more engaging it becomes. In a world already full of games, originality is essential, and personal experiences, narratives, and thoughts can provide that uniqueness.

⸻

3.2 Game Plan
Title: 박우성의 대모험 (The Great Adventure of Woosung Park)

I chose a cliché, old-school game title on purpose. 
Additionally, since the Atari was never officially released in South Korea and was a very unfamiliar machine there, I thought it would be interesting to include Korean text. 

Genre: Shooting Action

Synopsis:
Woosung Park from South Korea decides to go to France as an exchange student at ESAD Orléans. To do so, he must fight against countless documents and procedures, including French visa paperwork. Using ChatGPT, he battles and clears these administrative obstacles.

Gameplay:
Enemies (paperwork) approach from all directions. The player shoots GPT projectiles to eliminate them. The game is cleared after defeating 30 enemies (documents).

⸻

4. Development Process

4.1 Programming Language (C)
The programming language used for this project is C.

4.2 Image Conversion and Implementation
Naturally, the Atari ST cannot read image files directly. All in-game graphics were converted into C data and embedded directly into the code.

To achieve this, I created a PNG-to-C converter using Python.
Since the Atari ST can display only 16 colors per screen, all PNG images were designed within this color limitation before conversion.
