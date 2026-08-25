NAME		= ft_vox
CC		= c++
CFLAGS	= -std=c++23 -Wall -Wextra -O2

SRC_DIR		= src
OBJ_DIR		= obj
INC_DIR		= include
LIBS_DIR	= libs

SRCS		= $(wildcard $(SRC_DIR)/*.cpp)
OBJS		= $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

INCS		= -I$(INC_DIR) -I$(LIBS_DIR) $(shell pkg-config --cflags glfw3 glew 2>/dev/null)
LIBS		= $(shell pkg-config --libs glfw3 glew 2>/dev/null || echo "-lglfw -lGLEW") -lGL

all: deps $(NAME)

deps:
	@if [ ! -d $(LIBS_DIR)/glm ] || [ ! -f $(LIBS_DIR)/stb_image.h ]; then \
		bash scripts/install_deps.sh; \
	fi

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all deps clean fclean re