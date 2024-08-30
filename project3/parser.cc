#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <iostream>
#include <memory>
#include "compiler.h"
#include "parser.h"

using namespace std;

struct location
{
    string variableName;
    int index;
};

std::vector<std::shared_ptr<struct location>> locationTable = std::vector<std::shared_ptr<struct location>>();
struct InstructionNode *firstInstruction = NULL;

struct InstructionNode *finalLastCase = NULL;

void Parser::syntax_error()
{
    cout << "SYNTAX ERROR !&%!\n";
    exit(1);
}

Token Parser::expect(TokenType expected_type)
{
    Token t = lexer.GetToken();
    if (t.token_type != expected_type)
        syntax_error();
    return t;
}

void Parser::addInstructionNode(struct InstructionNode *instr)
{
    if (firstInstruction == NULL)
    {
        firstInstruction = instr;
    }
    else
    {
        struct InstructionNode *temp = firstInstruction;
        while (temp->next != NULL)
        {
            temp = temp->next;
        }
        temp->next = instr;
    }
}

//returns a variable's location in the memory array
int findLocation(string variable)
{
    for (int i = 0; i < locationTable.size(); i++)
    {
        if (variable.compare(locationTable[i]->variableName) == 0)
        {
            return locationTable[i]->index;
        }
    }

    return -1;
}

void Parser::parse_program()
{
    //Responsible for returning the first instruction (head)
    parse_varsection();
    parse_body();
    parse_inputs();
    expect(END_OF_FILE);
}

void Parser::parse_varsection()
{
    parse_id_list();
    expect(SEMICOLON);
}

void Parser::parse_id_list()
{
    //create location node in locationTable, assign memory
    Token id = expect(ID);
    std::shared_ptr<struct location> newLocation = std::make_shared<struct location>();
    newLocation->variableName = id.lexeme;
    newLocation->index = next_available;
    locationTable.push_back(newLocation);
    mem[next_available] = 0;
    next_available++;

    Token t = lexer.peek(1);
    if (t.token_type == COMMA)
    {
        expect(COMMA);
        parse_id_list();
    }
}

void Parser::parse_body()
{
    expect(LBRACE);
    parse_stmt_list();
    expect(RBRACE);
}

void Parser::parse_stmt_list()
{
    parse_stmt();
    Token t = lexer.peek(1);
    if (t.token_type != RBRACE)
    {
        parse_stmt_list();
    }
}

void Parser::parse_stmt()
{
    Token t = lexer.peek(1);
    if (t.token_type == ID)
    {
        parse_assign_stmt();
    }
    else if (t.token_type == WHILE)
    {
        parse_while_stmt();
    }
    else if (t.token_type == IF)
    {
        parse_if_stmt();
    }
    else if (t.token_type == SWITCH)
    {
        struct InstructionNode *afterSwitchNoOp = new InstructionNode();
        afterSwitchNoOp->type = NOOP;
        afterSwitchNoOp->next = NULL;
        parse_switch_stmt(afterSwitchNoOp);
        addInstructionNode(afterSwitchNoOp);
    }
    else if (t.token_type == FOR)
    {
        parse_for_stmt();
    }
    else if (t.token_type == OUTPUT)
    {
        parse_output_stmt();
    }
    else
    {
        parse_input_stmt();
    }
}

struct InstructionNode *Parser::parse_assign_stmt()
{
    Token var = expect(ID);
    expect(EQUAL);
    //create instr node
    struct InstructionNode *assignmentNode = new InstructionNode();
    assignmentNode->type = ASSIGN;
    assignmentNode->next = NULL;
    assignmentNode->assign_inst.left_hand_side_index = findLocation(var.lexeme);

    Token t = lexer.peek(2);
    if (t.token_type == PLUS || t.token_type == MINUS || t.token_type == MULT || t.token_type == DIV)
    {
        parse_expr(assignmentNode);
    }
    else
    {
        //Make operator OPERATOR NONE
        Token prim = parse_primary();
        assignmentNode->assign_inst.operand1_index = findLocation(prim.lexeme);
        assignmentNode->assign_inst.op = OPERATOR_NONE;
    }
    expect(SEMICOLON);
    //add instr node
    addInstructionNode(assignmentNode);
    return assignmentNode;
}

void Parser::parse_expr(struct InstructionNode *assignmentNode)
{
    Token prim1 = parse_primary();
    assignmentNode->assign_inst.operand1_index = findLocation(prim1.lexeme);

    Token t = parse_op();
    if (t.token_type == PLUS)
    {
        assignmentNode->assign_inst.op = OPERATOR_PLUS;
    }
    else if (t.token_type == MINUS)
    {
        assignmentNode->assign_inst.op = OPERATOR_MINUS;
    }
    else if (t.token_type == MULT)
    {
        assignmentNode->assign_inst.op = OPERATOR_MULT;
    }
    else if (t.token_type == DIV)
    {
        assignmentNode->assign_inst.op = OPERATOR_DIV;
    }

    Token prim2 = parse_primary();
    assignmentNode->assign_inst.operand2_index = findLocation(prim2.lexeme);
}

Token Parser::parse_primary()
{
    //needs to return ID or NUM
    Token primary;
    Token t = lexer.peek(1);
    if (t.token_type == ID)
    {
        primary = expect(ID);
    }
    else
    {
        //register the constant
        primary = expect(NUM);
        std::shared_ptr<struct location> newLocation = std::make_shared<struct location>();
        newLocation->variableName = primary.lexeme;
        newLocation->index = next_available;
        locationTable.push_back(newLocation);
        mem[next_available] = stoi(primary.lexeme);
        next_available++;
    }
    return primary;
}

Token Parser::parse_op()
{
    Token op;
    Token t = lexer.peek(1);
    if (t.token_type == PLUS)
    {
        op = expect(PLUS);
    }
    else if (t.token_type == MINUS)
    {
        op = expect(MINUS);
    }
    else if (t.token_type == MULT)
    {
        op = expect(MULT);
    }
    else
    {
        op = expect(DIV);
    }
    return op;
}

void Parser::parse_output_stmt()
{
    expect(OUTPUT);
    Token var = expect(ID);
    struct InstructionNode *outputNode = new InstructionNode();
    outputNode->type = OUT;
    outputNode->output_inst.var_index = findLocation(var.lexeme);
    outputNode->next = NULL;
    addInstructionNode(outputNode);
    expect(SEMICOLON);
}

void Parser::parse_input_stmt()
{
    expect(INPUT);
    Token var = expect(ID);
    struct InstructionNode *inputNode = new InstructionNode();
    inputNode->type = IN;
    inputNode->input_inst.var_index = findLocation(var.lexeme);
    inputNode->next = NULL;
    addInstructionNode(inputNode);
    expect(SEMICOLON);
}

void Parser::parse_while_stmt()
{
    expect(WHILE);
    struct InstructionNode *whileNode = new InstructionNode();
    whileNode->type = CJMP;
    whileNode->next = NULL;
    parse_condition(whileNode);
    //add the while node
    addInstructionNode(whileNode);
    parse_body();
    //create jmp node of type JMP
    struct InstructionNode *jmpNode = new InstructionNode();
    jmpNode->type = JMP;
    jmpNode->jmp_inst.target = whileNode;
    jmpNode->next = NULL;
    addInstructionNode(jmpNode);
    struct InstructionNode *noOpNode = new InstructionNode();
    noOpNode->type = NOOP;
    noOpNode->next = NULL;
    whileNode->cjmp_inst.target = noOpNode;
    addInstructionNode(noOpNode);
}

void Parser::parse_if_stmt()
{
    expect(IF);
    //create new node
    struct InstructionNode *ifNode = new InstructionNode();
    ifNode->type = CJMP;
    ifNode->next = NULL;

    //pass into condition and assign nodeInfo there
    parse_condition(ifNode);
    //add the if statement node
    addInstructionNode(ifNode);
    parse_body();
    //add NO-OP to LIST
    //No-OP node -> next = null
    //ifNode->target = NO OP node
    struct InstructionNode *noOpNode = new InstructionNode();
    noOpNode->type = NOOP;
    noOpNode->next = NULL;
    ifNode->cjmp_inst.target = noOpNode;
    //add NoOp to the list
    addInstructionNode(noOpNode);
}

void Parser::parse_condition(struct InstructionNode *instrNode)
{
    Token prim1 = parse_primary();
    instrNode->cjmp_inst.operand1_index = findLocation(prim1.lexeme);

    //parse_relop returns a token operator and sets the node's conditional_op
    Token cond_op = parse_relop();
    if (cond_op.token_type == GREATER)
    {
        instrNode->cjmp_inst.condition_op = CONDITION_GREATER;
    }
    else if (cond_op.token_type == LESS)
    {
        instrNode->cjmp_inst.condition_op = CONDITION_LESS;
    }
    else if (cond_op.token_type == NOTEQUAL)
    {
        instrNode->cjmp_inst.condition_op = CONDITION_NOTEQUAL;
    }

    Token prim2 = parse_primary();
    instrNode->cjmp_inst.operand2_index = findLocation(prim2.lexeme);
}

Token Parser::parse_relop()
{
    Token cond_op;
    Token t = lexer.peek(1);
    if (t.token_type == GREATER)
    {
        cond_op = expect(GREATER);
    }
    else if (t.token_type == LESS)
    {
        cond_op = expect(LESS);
    }
    else
    {
        cond_op = expect(NOTEQUAL);
    }
    return cond_op;
}

void Parser::parse_for_stmt()
{
    expect(FOR);
    expect(LPAREN);
    struct InstructionNode *forNode = new InstructionNode();
    forNode->type = CJMP;
    forNode->next = NULL;
    //this assignment statement only happens once, and parse_assignment is added to instr list
    parse_assign_stmt();
    //condition is evaluated by the compiler
    parse_condition(forNode);
    expect(SEMICOLON);
    addInstructionNode(forNode);
    //we need to store the assignment statement here to evaluate AFTER the body of the for loop is finished
    struct InstructionNode *secondAssignment = parse_assign_stmt();
    //Remove it from the list
    struct InstructionNode *tempHead = firstInstruction;
    if (tempHead->next != NULL)
    {
        while (tempHead->next->next != NULL)
        {
            tempHead = tempHead->next;
        }
        tempHead->next = NULL;
    }
    expect(RPAREN);
    parse_body();
    addInstructionNode(secondAssignment);
    struct InstructionNode *jmpNode = new InstructionNode();
    jmpNode->type = JMP;
    jmpNode->jmp_inst.target = forNode;
    jmpNode->next = NULL;
    addInstructionNode(jmpNode);
    struct InstructionNode *noOpNode = new InstructionNode();
    noOpNode->type = NOOP;
    noOpNode->next = NULL;
    forNode->cjmp_inst.target = noOpNode;
    addInstructionNode(noOpNode);
}

void Parser::parse_switch_stmt(struct InstructionNode *afterSwitchNoOp)
{
    expect(SWITCH);
    //the variable we are comparing to
    Token comparable = expect(ID);
    expect(LBRACE);
    //create currentCase, pass into caseList
    struct InstructionNode *currentCase = new InstructionNode();
    struct InstructionNode *lastCase = parse_case_list(comparable, currentCase, afterSwitchNoOp);
    lastCase->next->jmp_inst.target = afterSwitchNoOp;
    Token t = lexer.peek(1);
    if (t.token_type == DEFAULT)
    {
        struct InstructionNode *defaultNoOp = new InstructionNode();
        defaultNoOp->type = NOOP;
        defaultNoOp->next = NULL;
        addInstructionNode(defaultNoOp);
        lastCase->next->jmp_inst.target = defaultNoOp;
        parse_default_case(defaultNoOp);
    }
    expect(RBRACE);
}

struct InstructionNode *Parser::parse_case_list(Token switchCompare, struct InstructionNode *currentCase, struct InstructionNode *afterSwtichNoOp)
{
    //have this return a caseNode
    struct InstructionNode *caseNode = parse_case(switchCompare, currentCase, afterSwtichNoOp);
    finalLastCase = caseNode;

    Token t = lexer.peek(1);
    if (t.token_type == CASE)
    {
        //if there is a next case, then the previous case needs to jump to the next case
        struct InstructionNode *nextCase = new InstructionNode();
        caseNode->next->jmp_inst.target = nextCase;
        parse_case_list(switchCompare, nextCase, afterSwtichNoOp);
    }
    //returns the last case in the list of cases
    return finalLastCase;
}

struct InstructionNode *Parser::parse_case(Token switchCompare, struct InstructionNode *currentCase, struct InstructionNode *afterSwtichNoOp)
{
    //Here we build an IF instruction node
    expect(CASE);
    Token num = expect(NUM);
    //grab the indexes of both Num and switchCompare
    //Num doesnt exist yet in our memory or locationTable, though. Crap
    std::shared_ptr<struct location> newLocation = std::make_shared<struct location>();
    newLocation->variableName = num.lexeme;
    newLocation->index = next_available;
    locationTable.push_back(newLocation);
    mem[next_available] = stoi(num.lexeme);
    next_available++;

    //create IF node
    currentCase->type = CJMP;
    currentCase->next = NULL;
    currentCase->cjmp_inst.operand1_index = findLocation(switchCompare.lexeme);
    currentCase->cjmp_inst.operand2_index = findLocation(num.lexeme);
    currentCase->cjmp_inst.condition_op = CONDITION_NOTEQUAL;
    //how are we supposed to assign the operator if theres no EQUAL UGHHHHH
    //create NoOp and JMP node
    struct InstructionNode *noOp = new InstructionNode();
    struct InstructionNode *jmpNode = new InstructionNode();
    noOp->type = NOOP;
    noOp->next = NULL;
    jmpNode->type = JMP;
    jmpNode->next = NULL;
    //jmpNode->target = nextCaseNode
    currentCase->cjmp_inst.target = noOp;
    addInstructionNode(currentCase);
    addInstructionNode(jmpNode);
    addInstructionNode(noOp);
    expect(COLON);
    parse_body();
    //add jmp node that points to last no-OP that appears after default
    struct InstructionNode *jmpNodeAfterBody = new InstructionNode();
    jmpNodeAfterBody->type = JMP;
    jmpNodeAfterBody->next = NULL;
    jmpNodeAfterBody->jmp_inst.target = afterSwtichNoOp;
    addInstructionNode(jmpNodeAfterBody);
    return currentCase;
}

void Parser::parse_default_case(struct InstructionNode *defaultNoOP)
{
    expect(DEFAULT);
    expect(COLON);
    parse_body();
}

void Parser::parse_inputs()
{
    parse_num_list();
}

void Parser::parse_num_list()
{
    Token num = expect(NUM);
    //push back inputs into input list
    inputs.push_back(stoi(num.lexeme));

    Token t = lexer.peek(1);
    if (t.token_type == NUM)
    {
        parse_num_list();
    }
}

struct InstructionNode *parse_generate_intermediate_representation()
{
    Parser parser;
    //returns the parsing, parse_program returns head of the instructions list
    parser.parse_program();
    return firstInstruction;
}
