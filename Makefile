# **************************************************************************** #
#                                                                              #
#    PS_TESTER - The Ultimate Push_swap Tester                                 #
#                                                                              #
#    by mjabri                                                                 #
#                                                                              #
# **************************************************************************** #

NAME		= ps_tester
SRC			= push_swap_ultimate_tester.cpp
CXX			= g++
CXXFLAGS	= -std=c++17 -O3 -pthread -Wall -Wextra

# Colors
GREEN		= \e[0;32m
CYAN		= \e[0;36m
YELLOW		= \e[0;33m
RED			= \e[0;31m
RESET		= \e[0m

all: $(NAME)

$(NAME): $(SRC)
	@printf "$(CYAN)Compiling $(NAME)...$(RESET)\n"
	@$(CXX) $(CXXFLAGS) $(SRC) -o $(NAME)
	@printf "$(GREEN)✓ $(NAME) compiled successfully!$(RESET)\n"
	@printf "\n$(YELLOW)Usage:$(RESET) ./ps_tester <path_to_push_swap>\n"

clean:
	@printf "$(YELLOW)Cleaning log files...$(RESET)\n"
	@rm -f trace.log errors.txt report.html
	@rm -f full_output.txt output2.txt output*.txt
	@printf "$(GREEN)✓ Logs cleaned!$(RESET)\n"

fclean: clean
	@printf "$(RED)Removing $(NAME)...$(RESET)\n"
	@rm -f $(NAME)
	@printf "$(GREEN)✓ Full clean done!$(RESET)\n"

re: fclean all

# Help
help:
	@printf "$(CYAN)PS_TESTER Makefile$(RESET)\n"
	@printf "\n"
	@printf "$(GREEN)make$(RESET)        - Compile the tester\n"
	@printf "$(GREEN)make clean$(RESET)  - Remove log files (trace.log, errors.txt, output*.txt)\n"
	@printf "$(GREEN)make fclean$(RESET) - Remove logs + binary\n"
	@printf "$(GREEN)make re$(RESET)     - Recompile\n"
	@printf "\n"
	@printf "$(YELLOW)Usage:$(RESET)\n"
	@printf "  ./ps_tester <push_swap_path> [checker_path] [options]\n"
	@printf "\n"
	@printf "$(YELLOW)Options:$(RESET)\n"
	@printf "  --quick        Quick mode (fewer iterations)\n"
	@printf "  --no-valgrind  Skip memory leak tests\n"
	@printf "  --stress       Extra stress tests\n"
	@printf "  --html         Generate HTML report\n"

.PHONY: all clean fclean re help
