# Symulacja Akwarium 3D
**Status:** Work In Progress

Projekt to interaktywna symulacja akwarium 3D napisana w języku C++ z wykorzystaniem biblioteki **OpenGL**. Głównym elementem projektu jest implementacja algorytmu stadnego (Boids) sterującego zachowaniem ryb, a także zaawansowane efekty wizualne takie jak załamywanie światła na wodzie, mapowanie normalnych czy dynamiczne oświetlenie.

### Główne funkcjonalności
* **Algorytm Boids:** Ryby poruszają się w stadzie, reagując na siebie (separacja, wyrównanie, spójność) oraz unikając.
* **Shadery:**
  * **Woda:** Symulacja falowania (funkcje trygonometryczne), refrakcja (zniekształcenia obrazu tła) i odbicia otoczenia (Skybox) z użyciem zjawiska Fresnela.
  * **Szkło:** Półprzezroczystość z odbiciami bazującymi na kącie patrzenia.
  * **Tekstury:** Wykorzystanie Normal Mappingu do detali dna oraz modeli.
* **Dynamiczne Oświetlenie (UBO):** Kierunkowy reflektor punktowy naśladujący oświetlenie akwarium oraz pulsujące, punktowe światło (Chest Glow) bijące ze skrzyni.

### Sterowanie
* **W, S, A, D** - Poruszanie kamerą
* **Myszka** - Obrót kamery
* **Strzałki (Góra/Dół/Lewo/Prawo)** - Zmiana pozycji głównego źródła światła
* **ESC** - Wyjście z aplikacji

### Technologie / Zależności
* **C++**
* **OpenGL 3.3 Core**
* **GLFW** (obsługa okna i wejścia)
* **GLEW** (ładowanie rozszerzeń OpenGL)
* **GLM** (matematyka wektorów i macierzy)
* **stb_image** (ładowanie tekstur)
