NAME            =   webserv

INCLUDES		= 	includes



CXX             =   c++
CXXFLAGS        =   -Wall -Wextra -Werror -std=c++98 -MMD -MP -I$(INCLUDES) -g3 -fsanitize=address

ifeq ($(shell uname -s),Darwin)
	CXXFLAGS += -DMAC_BUILD
endif

SRC_DIR			=	srcs
SRC				= 	$(shell find $(SRC_DIR) -type f -name '*.cpp')

BUILD_DIR       =   __build__

OBJ             =  	$(SRC:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

DEP				=	$(OBJ:.o=.d)

RM              =   rm -rf

all: $(NAME)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

clean:
	$(RM) $(BUILD_DIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all

-include : $(DEP)
.PHONY: all clean fclean re
.SECONDARY: $(OBJ)
