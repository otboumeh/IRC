NAME = ft_IRC
CC = c++

INCLUDES = -I.
CFLAGS = -Wall -Wextra -Werror -std=c++98 $(INCLUDES)



SRCS = $(shell find . -name "*.cpp")


OBJS = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -f $(OBJS)

fclean:
	@rm -f $(NAME)
	@rm -f $(OBJS)

re: fclean all

.PHONY: all clean fclean re
