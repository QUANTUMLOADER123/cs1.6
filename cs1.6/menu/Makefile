# Сборка headless-рендерера меню (Linux, без окна, без GPU)
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -I. -Iinclude
LDFLAGS  ?=

SRC = imgui.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp \
      headless/main.cpp
OBJ = $(SRC:.cpp=.o)

menu.png: $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

# заголовки — зависимости, иначе make не пересобирает при их изменении
HDR = $(wildcard include/*.h) imgui.h imgui_internal.h

%.o: %.cpp $(HDR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) headless_menu_marker

.PHONY: clean
