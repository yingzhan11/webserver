
NAME := server

COMPILER := c++
FLAGS := -std=c++11 # -Wall -Wextra -Werror

# Source Files
SRCS := main.cpp webserver.cpp \
        threadpool/threadpool.cpp \
        config/Config.cpp \
		http/http_request.cpp


OBJS_DIR := objs

OBJS := $(SRCS:.cpp=.o)
TARGETS := $(addprefix $(OBJS_DIR)/, $(OBJS))

# Ensure subdirectories exist
OBJS_DIRS := $(sort $(dir $(TARGETS)))

# Default Target
all: $(NAME)

$(OBJS_DIR)/%.o: %.cpp | $(OBJS_DIRS)
	$(COMPILER) $(FLAGS) -c $< -o $@

# Create required directories
$(OBJS_DIRS):
	mkdir -p $@

# Linking final executable
$(NAME): $(TARGETS)
	$(COMPILER) $(FLAGS) -o $(NAME) $(TARGETS)

# Clean up build files
clean:
	rm -rf $(OBJS_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

# Mark these targets as "phony" (they do not produce files)
.PHONY: all clean fclean re
