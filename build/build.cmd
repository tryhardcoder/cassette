
:: build src
g++ -c src/base/baseImpl.cpp -Iexternal -Isrc -o out/base.o -g
g++ -c src/main.cpp -Iexternal -Isrc -o out/main.o -g
g++ -c src/blue/blue.cpp -Iexternal -Isrc -o out/blue.o -g
g++ -c src/engine.cpp -Iexternal -Isrc -o out/engine.o -g


:: build final exe
:: What the actual fuck, https://discourse.glfw.org/t/linking-glfw-on-the-command-line-not-working/1730/9
g++ out/base.o out/engine.o out/blue.o out/main.o out/stb_image.o out/stb_truetype.o out/gl.o -Lexternal\glfw -lglfw3 -lopengl32 -lkernel32 -luser32 -lgdi32 -lws2_32 -o out/game -g


