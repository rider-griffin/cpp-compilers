/*
 * Copyright (C) Rida Bazzi, 2020
 *
 * Do not share this file with anyone
 *
 * Do not post this file or derivatives of
 * of this file online
 *
 */
#include <iostream>
#include <cstdlib>
#include "parser.h"
#include <algorithm>
#include <memory>
#include <string>

using namespace std;
//globals
int next_available = 0;
struct stmt
{
    string variableName;
    int loc;
};

struct poly_declaration
{
    string name;
    int line_number;
    std::vector<string> parameters;
    std::shared_ptr<struct term> term;
};

struct term
{
    int coef;
    std::shared_ptr<struct monomial> mono;
    string addoperator;
    std::shared_ptr<struct term> next;
};

struct monomial
{
    int index;
    int power;
    bool notFound;
    int line_number;
    std::shared_ptr<struct monomial> next;
};

struct statement
{
    string stmt_type;
    std::shared_ptr<struct poly_eval> poly;
    int variable;
    std::shared_ptr<struct statement> next;
};

struct poly_eval
{
    int poly_index;
    int line_number;
    bool notFound;
    std::shared_ptr<struct arguments> args;
};

struct arguments
{
    string type;
    int value;
    int symbol_index;
    std::shared_ptr<struct poly_eval> poly;
};



std::shared_ptr<struct statement> stmtHead = std::make_shared<struct statement>();
std::vector<std::shared_ptr<struct stmt>> symbolTable = std::vector<std::shared_ptr<struct stmt>>();
std::vector<int> inputNumbers = std::vector<int>();
std::vector<std::shared_ptr<struct poly_declaration>> poly_declarations_vector = std::vector<std::shared_ptr<struct poly_declaration>>();





// Parsing
void Parser::syntax_error()
{
    cout << "SYNTAX ERROR !&%!\n";
    exit(1);
}

// this function gets a token and checks if it is
// of the expected type. If it is, the token is
// returned, otherwise, synatx_error() is generated
// this function is particularly useful to match
// terminals in a right hand side of a rule.
// Written by Mohsen Zohrevandi
Token Parser::expect(TokenType expected_type)
{
    Token t = lexer.GetToken();
    if (t.token_type != expected_type)
        syntax_error();
    return t;
    
}

void Parser::parse_input()
{
    //input -> program  inputs
    Token t;
    parse_program();
    parse_inputs();
    t = lexer.GetToken();
    if(t.token_type != END_OF_FILE)
    {
        syntax_error();
    }
    
}


void Parser::parse_program()
{
    //parse_program -> poly_decl_section  start
    parse_poly_decl_section();
    parse_start();
}


void Parser::parse_poly_decl_section()
{
    //poly_decl_section -> poly_decl
    //poly_decl_section -> poly_decl  poly_decl_section
    parse_poly_decl();
    Token t = lexer.peek(1);
    if(t.token_type == POLY)
    {
        parse_poly_decl_section();
    }
}


void Parser::parse_poly_decl()
{
    //poly_decl -> POLY polynomial_header EQUAL polynomial_body SEMICOLON
    expect(POLY);
    //new poly decl
    std::shared_ptr<struct poly_declaration> poly_decl = std::make_shared<struct poly_declaration>();
    poly_declarations_vector.push_back(poly_decl);
    //set term to null so we can attach terms to it later
    poly_decl->term = NULL;
    parse_polynomial_header(poly_decl);
    expect(EQUAL);
    parse_polynomial_body(poly_decl);
    expect(SEMICOLON);
}


void Parser::parse_polynomial_header(std::shared_ptr<struct poly_declaration> poly_decl)
{
    //polynomial_header -> polynomial_name
    //polynomial_header -> polynomial_name  LPAREN  id_list  RPAREN
    poly_decl->parameters = std::vector<string>();
    Token polyname = parse_polynomial_name();
    poly_decl->name = polyname.lexeme;
    poly_decl->line_number = polyname.line_no;
    Token t = lexer.peek(1);
    if(t.token_type == LPAREN)
    {
        expect(LPAREN);
        parse_id_list(poly_decl);
        expect(RPAREN);
    }
    else
    {
        poly_decl->parameters.push_back("x");
        return;
    }
}


void Parser::parse_polynomial_body(std::shared_ptr<struct poly_declaration> poly_decl)
{
    //polynomial_body -> term_list
    parse_term_list(poly_decl);
}


Token Parser::parse_polynomial_name()
{
    //polynomial_name -> ID
    Token id = expect(ID);
    return id;
}


void Parser::parse_term_list(std::shared_ptr<struct poly_declaration> poly_decl)
{
    //term_list -> term
    //term_list -> term  add_operator  term_list

    parse_term(poly_decl);
    Token t = lexer.peek(1);
    if((t.token_type == PLUS) || (t.token_type == MINUS))
    {
        parse_add_operator();
        parse_term_list(poly_decl);
    }
}


void Parser::parse_term(std::shared_ptr<struct poly_declaration> poly_decl)
{
    //term -> monomial_list
    //term -> coefficient  monomial_list
    //term -> coefficient

    //create a new term node.
    std::shared_ptr<struct term> newTerm = std::make_shared<struct term>();
    newTerm->next = NULL;
    //if term list is empty, attach the newly created term node to it
    //if theres something already there, then traverse the list of terms to find the end of the list, then attach
    if(poly_decl->term == NULL)
    {   
        //change the first term from null and instead have it point to newTerm, which will be the head
        poly_decl->term = newTerm;
    }
    else
    {
        //if theres something there, we need to 
        //traverse list of terms
        std::shared_ptr<struct term> temp = poly_decl->term;

        while(temp->next != NULL)
        {
            temp = temp->next;
        }
        //attach newTerm to the end of the term list
        temp->next = newTerm;
    }
    
    Token t = lexer.peek(1);
    if(t.token_type == NUM)
    {
        int coefficient = parse_coefficient();
        newTerm->coef = coefficient;

        t = lexer.peek(1);
        if(t.token_type == ID)
        {
           parse_monomial_list(newTerm);
        }
    }
    else if(t.token_type == ID)
    {
        //if no coefficient given, then it is implied 1
        newTerm->coef = 1;
        parse_monomial_list(newTerm);
    }
    else
    {
        syntax_error();
    }
    
}


void Parser::parse_monomial_list(std::shared_ptr<struct term> newTerm)
{
    //monomial_list -> monomial
    //monomial_list -> monomial monomial_list

    parse_monomial(newTerm);
    Token t = lexer.peek(1);
    if(t.token_type == ID)
    {
        parse_monomial_list(newTerm);   
    }


}


void Parser::parse_monomial(std::shared_ptr<struct term> newTerm)
{
    //monomial -> ID
    //monomial -> ID  exponent
    
    //need to create the monomial struct pointer
    std::shared_ptr<struct monomial> newMonomial = std::make_shared<struct monomial>();
    //set its next value to NULL
    newMonomial->next = NULL;
    newMonomial->notFound = true;
    
    //check for existing monomial structs
    if(newTerm->mono == NULL)
    {
        newTerm->mono = newMonomial;
    }
    else
    {
        std::shared_ptr<struct monomial> temp = newTerm->mono;
        while(temp->next != NULL)
        {
            temp = temp->next;
        }
        //attach to end of the list
        temp->next = newMonomial;
    }
    
    
    Token id = expect(ID);
    newMonomial->line_number = id.line_no;
    //lookup the id and store its index
    //we know that the latest update to the vector is the poly_decl we want to traverse
        for(int j = 0; j < poly_declarations_vector.back()->parameters.size(); j++)
        {
            //what happens when we dont find it's lexeme in the parameter list? We need to store its line no. and a value called notfound
            if(id.lexeme.compare(poly_declarations_vector.back()->parameters[j])==0)
            {
                //note that we are listing parameters as (x,y,z) as 1,2,3 for index
                
                newMonomial->index = j + 1;
                newMonomial->notFound = false;

            }
        }

    Token t = lexer.peek(1);
    if(t.token_type == POWER)
    {
        int power = parse_exponent();
        newMonomial->power = power;

    }
    else
    {
        //if power is not returned then we assume its power is 1
        newMonomial->power = 1;

    }
    
}


void Parser::parse_id_list(std::shared_ptr<struct poly_declaration> poly_decl)
{
    //id_list -> ID
    //id_list -> ID  COMMA  id_list
    
    Token id = expect(ID);
    poly_decl->parameters.push_back(id.lexeme);


    Token t = lexer.peek(1);
    if (t.token_type == COMMA)
    {
        expect(COMMA);
        parse_id_list(poly_decl);
    }

}


void Parser::parse_start()
{
    //start -> START  statement_list
    expect(START);
    stmtHead = NULL;
    parse_statement_list(stmtHead);
}


void Parser::parse_statement_list(std::shared_ptr<struct statement> stmt)
{
    //statement_list -> statement
    //statement_list -> statement  statement_list;

    //create the statement_node
    std::shared_ptr<struct statement> newStatement = std::make_shared<struct statement>();
    newStatement->next = NULL;

    if(stmt == NULL)
    {
        stmt = newStatement;
    }
    else
    {
        std::shared_ptr<struct statement> temp = newStatement;
        while(temp->next != NULL)
        {
            temp = temp->next;
        }
        temp->next = newStatement;
    }

    parse_statement(newStatement);
    Token t = lexer.peek(1);
    if((t.token_type == INPUT)||(t.token_type == ID))
    {
        parse_statement_list(newStatement);
    }    
    
}


int Parser::parse_exponent()
{
    //exponent -> POWER NUM
    expect(POWER);
    Token t = expect(NUM);
    int power = stoi(t.lexeme);
    return power;
}


void Parser::parse_statement(std::shared_ptr<struct statement> stmt)
{
    //statement -> input_statement
    //statement -> poly_evaluation_statement
    Token t = lexer.peek(1);
    if(t.token_type == INPUT)
    {
        std::shared_ptr<struct stmt> st = parse_input_statement();
        stmt->stmt_type = "INPUT";
        stmt->poly = NULL;
        stmt->variable = st->loc;
    }
    else if(t.token_type == ID)
    {
        parse_poly_evaluation_statement(stmt); 
    }
    else
    {
        syntax_error();
    }
    
}

//create symbol table for the input statement variables and allocate memory for them
std::shared_ptr<struct stmt> Parser::parse_input_statement()
{
    //input_statement -> INPUT  ID  SEMICOLON
    std::shared_ptr<struct stmt> st =  std::make_shared<struct stmt>();

    expect(INPUT);
    Token t = expect(ID);
    for(int i = 0; i < symbolTable.size(); i++)
    {
        if(t.lexeme.compare(symbolTable[i]->variableName) == 0)
        {
            expect(SEMICOLON);
            st->loc = symbolTable[i]->loc;
            return st;
        }
    }

    
    st->variableName = t.lexeme;
    st->loc = next_available;
    symbolTable.push_back(st);
    next_available++;
    expect(SEMICOLON);
    return st;
}


void Parser::parse_poly_evaluation_statement(std::shared_ptr<struct statement> stmt)
{
    //poly_evaluation_statement -> polynomial_evaluation  SEMICOLON
    stmt->stmt_type="POLYEVAL";
    parse_polynomial_evaluation(stmt);
    expect(SEMICOLON);
}

void Parser::parse_polynomial_evaluation(std::shared_ptr<struct statement> stmt)
{
    //polynomial_evaluation -> polynomial_name  LPAREN argument_list RPAREN
    //create a new poly_eval node
    std::shared_ptr<struct poly_eval> poly_eval = std::make_shared<struct poly_eval>();

    if(stmt->poly == NULL)
    {   
        stmt->poly = poly_eval;
    }

    //name exists in the poly_decl_vector
    Token name = parse_polynomial_name();
    poly_eval->line_number = name.line_no;
    poly_eval->notFound = true;

    for(int i = 0; i < poly_declarations_vector.size();i++)
    {
        if(name.lexeme.compare(poly_declarations_vector[i]->name)==0)
        {
            poly_eval->poly_index = i;
            poly_eval->notFound = false;
        }
    }

    Token t = lexer.peek(1);
    if(t.token_type == LPAREN)
    {
        expect(LPAREN);
        parse_argument_list(stmt);
        expect(RPAREN);
    }
    else
    {
        syntax_error();
    }
    
}

void Parser::parse_argument_list(std::shared_ptr<struct statement> stmt)
{
    //argument_list -> argument
    //argument_list -> argument COMMA argument_list
    parse_argument(stmt);
    Token t = lexer.peek(1);
    if(t.token_type == COMMA)
    {
        expect(COMMA);
        parse_argument_list(stmt);
    }

}

void Parser::parse_argument(std::shared_ptr<struct statement> stmt)
{
    //argument -> ID | NUM | polynomial_evaluation
    Token t = lexer.peek(1);
    if(t.token_type == ID)
    {
        t = lexer.peek(2);
        if(t.token_type == LPAREN)
        {
            parse_polynomial_evaluation(stmt);
        }
        else
        {
            expect(ID);
        }
        
    }
    else if(t.token_type == NUM)
    {
        expect(NUM);
    }
    else
    {
        syntax_error();
    }
    
    
}

void Parser::parse_inputs()
{
    //inputs -> NUM
    //inputs -> NUM inputs
    Token num = expect(NUM);
    inputNumbers.push_back(stoi(num.lexeme));
    Token t = lexer.peek(1);
    if(t.token_type == NUM)
    {
        parse_inputs();
    }

}


int Parser::parse_coefficient()
{
    //coefficient -> NUM
    Token t = expect(NUM);
    int num = stoi(t.lexeme);
    return num;

}


void Parser::parse_add_operator()
{
    //add_operator -> PLUS
    //add_operator -> MINUS
    Token t = lexer.peek(1);
    if(t.token_type == PLUS)
    {
        expect(PLUS);
    }
    else if(t.token_type == MINUS)
    {
        expect(MINUS);
    }
    else
    {
        syntax_error();
    }
}


int main()
{
	Parser parser;
	parser.parse_input();

    //Error Code 1
    bool noErrors1 = false;
    std::vector<int> ec1_line_nums;
    for(int i = 0; i < poly_declarations_vector.size(); i++)
    {
        string tempName = poly_declarations_vector[i]->name;
        int tempLine = poly_declarations_vector[i]->line_number;
        
        for(int j = i+1; j < poly_declarations_vector.size(); j++)
        {
            if(tempName.compare(poly_declarations_vector[j]->name)==0)
            {
                ec1_line_nums.push_back(tempLine);
                ec1_line_nums.push_back(poly_declarations_vector[j]->line_number);
            }
        }
    }
    if(ec1_line_nums.size() == 0)
    {
        noErrors1 = true;
    }
    else
    {
        //lets get rid of duplicate values
        sort(ec1_line_nums.begin(), ec1_line_nums.end());
        ec1_line_nums.erase(unique(ec1_line_nums.begin(), ec1_line_nums.end()), ec1_line_nums.end());
        string errors = "";
        for(int k = 0; k<ec1_line_nums.size(); k++)
        {
            string line = to_string(ec1_line_nums[k]);
            errors = errors + line + " ";
        }
        cout << "Error Code 1: " << errors << endl;
    }


    //Error Code 2
    bool noErrors2 = false;
    if(noErrors1)
    {
        std::vector<int> ec2_line_nums;
        
        //first we will have to traverse the poly_decl_vector one by one
        //then we need to traverse each body struct at the index of poly_decl_vector
        //then we need to traverse each body structs' monomial list... if there is any notfounds, store the line number
        for(int i = 0; i < poly_declarations_vector.size(); i++)
        {
            while(poly_declarations_vector[i]->term !=NULL)
            {
                //traverse each monomial list in each term 
                while(poly_declarations_vector[i]->term->mono!= NULL)
                {
                   if(poly_declarations_vector[i]->term->mono->notFound)
                   {
                       ec2_line_nums.push_back(poly_declarations_vector[i]->term->mono->line_number);
                   }

                   poly_declarations_vector[i]->term->mono = poly_declarations_vector[i]->term->mono->next;
                }
                poly_declarations_vector[i]->term = poly_declarations_vector[i]->term->next;
            }
        }
        if(ec2_line_nums.size() == 0)
        {
            noErrors2 = true;
        }
        else
        {
            cout << "Error Code 2: "; 
            for(int l = 0; l<ec2_line_nums.size(); l++)
            {
                cout << to_string(ec2_line_nums[l]) << " ";
            }
        }
    }



    //Error code 3
    if(noErrors2)
    {

    }


    

}
