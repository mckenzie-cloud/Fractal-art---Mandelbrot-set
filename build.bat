@echo off
g++ -std=c++2b -fopenmp .\main.cpp -I SFML\include\ -L SFML\lib\ -lsfml-main -lsfml-graphics -lsfml-system -lsfml-window -o bin\run