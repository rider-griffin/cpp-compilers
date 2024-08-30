/*
 * Copyright (C) Rida Bazzi, 2019
 *
 * Do not share this file with anyone
 */
#ifndef __PARSER_H__
#define __PARSER_H__

#include <string>
#include "lexer.h"
#include <vector>
#include <memory>

class Parser {
  public:
    void ConsumeAllInput();
    void parse_input();
    void parse_program();
    void parse_poly_decl_section();
    void parse_poly_decl();
    void parse_polynomial_header(std::shared_ptr<struct poly_declaration> poly_decl);
    void parse_polynomial_body(std::shared_ptr<struct poly_declaration> poly_decl);
    Token parse_polynomial_name();
    void parse_id_list(std::shared_ptr<struct poly_declaration> poly_decl);
    void parse_inputs();
    void parse_start();
    void parse_statement_list(std::shared_ptr<struct statement> stmt);
    void parse_statement(std::shared_ptr<struct statement> stmt);
    std::shared_ptr<struct stmt> parse_input_statement();
    void parse_poly_evaluation_statement(std::shared_ptr<struct statement> stmt);
    void parse_term_list(std::shared_ptr<struct poly_declaration> poly_decl);
    void parse_argument(std::shared_ptr<struct statement> stmt);
    void parse_polynomial_evaluation(std::shared_ptr<struct statement> stmt);
    void parse_argument_list(std::shared_ptr<struct statement> stmt);
    int parse_coefficient();
    void parse_term(std::shared_ptr<struct poly_declaration> poly_decl);
    void parse_add_operator();
    void parse_monomial_list(std::shared_ptr<struct term> newTerm);
    void parse_monomial(std::shared_ptr<struct term> newTerm);
    int parse_exponent();
    
    

  private:
    LexicalAnalyzer lexer;
    void syntax_error();
    Token expect(TokenType expected_type);
};

#endif

