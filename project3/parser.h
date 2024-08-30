#include <string>
#include "lexer.h"
#include <vector>
#include <memory>

class Parser
{
public:
  void parse_program();
  void parse_varsection();
  void parse_id_list();
  void parse_body();
  void parse_stmt_list();
  void parse_stmt();
  struct InstructionNode *parse_assign_stmt();
  void parse_expr(struct InstructionNode *assignmentNode);
  Token parse_primary();
  Token parse_op();
  void parse_output_stmt();
  void parse_input_stmt();
  void parse_while_stmt();
  void parse_if_stmt();
  void parse_condition(struct InstructionNode *instrNode);
  Token parse_relop();
  void parse_switch_stmt(struct InstructionNode *afterSwitchNoOp);
  void parse_for_stmt();
  struct InstructionNode *parse_case_list(Token switchCompare, struct InstructionNode *currentCase, struct InstructionNode *afterSwitchNoOp);
  struct InstructionNode *parse_case(Token switchCompare, struct InstructionNode *currentCase, struct InstructionNode *afterSwitchNoOp);
  void parse_default_case(struct InstructionNode *defaultNoOP);
  void parse_inputs();
  void parse_num_list();
  void addInstructionNode(struct InstructionNode *instr);

private:
  LexicalAnalyzer lexer;
  void syntax_error();
  Token expect(TokenType expected_type);
};
