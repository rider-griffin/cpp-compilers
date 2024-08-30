/*
 * Copyright (C) Mohsen Zohrevandi, 2017
 *               Rida Bazzi 2019
 * Do not share this file with anyone
 */
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include "lexer.h"
#include <algorithm>
#include <vector>
#include <memory>
#include <string>

using namespace std;

LexicalAnalyzer lexer;


//rule struct
struct rule
{
    string LHS;
    std::vector<string> RHS;
    bool hasEpsilon;
};

struct predictive
{
    string LHS;
    std::vector<std::vector<string>> set;
    std::vector<string> unionVec;
    std::vector<string> intersectionVec;
};

//grammar vector
std::vector<std::shared_ptr<struct rule>> ruleTable = std::vector<std::shared_ptr<struct rule>>();

//task 1 
std::vector<string> nonTerminals = std::vector<string>();
std::vector<string> orderTerminals = std::vector<string>();
std::vector<string> orderNonTerminals = std::vector<string>();

//task 2
std::vector<string> initialGen = std::vector<string>();
std::vector<std::shared_ptr<struct rule>> eliminatedUseless = std::vector<std::shared_ptr<struct rule>>();
bool grammarHasEpsilon;

//task 3
std::vector<std::shared_ptr<struct rule>> firstSetList = std::vector<std::shared_ptr<struct rule>>();

//task 4
std::vector<std::shared_ptr<struct rule>> followSetList = std::vector<std::shared_ptr<struct rule>>();

//task 5
bool grammarHasUseless;
std::vector<std::shared_ptr<struct predictive>> predictiveTable = std::vector<std::shared_ptr<struct predictive>>();

void syntax_error()
{
    cout << "SYNTAX ERROR !!!\n";
    exit(1);
}

Token expect(TokenType expected_type)
{
    Token t = lexer.GetToken();
    if (t.token_type != expected_type)
        syntax_error();
    return t;
}

// read grammar
void Parse_Id_List(std::shared_ptr<struct rule> newRule)
{
    Token symbol = expect(ID);
    newRule->RHS.push_back(symbol.lexeme);
    Token t = lexer.peek(1);
    if(t.token_type == ID)
    {
        Parse_Id_List(newRule);
    }
}

void Parse_RHS(std::shared_ptr<struct rule> newRule)
{
    Token t = lexer.peek(1);
    if(t.token_type == ID)
    {
        Parse_Id_List(newRule);
    }
    else if(t.token_type == STAR)
    {
        //noting here that the grammar has an epsilon in the set of rules, used for task 2
        grammarHasEpsilon = true;
        return;
    }
    else
    {
        syntax_error();   
    }

}

//make a new rule struct here, pass the rule into parse_RHS, fill the RHS within RHS
std::shared_ptr<struct rule> Parse_Rule()
{
    //grab LHS symbol
    std::shared_ptr<struct rule> newRule = std::make_shared<struct rule>();
    newRule->RHS = std::vector<string>();

    Token nt = expect(ID);
    newRule->LHS = nt.lexeme;

    expect(ARROW);
    Parse_RHS(newRule);
    expect(STAR);
    return newRule;
}

void Parse_Rule_List()
{
    //parse_rule returns a rule that must be pushed back into the global grammar vector
    std::shared_ptr<struct rule> rule = Parse_Rule();
    ruleTable.push_back(rule);
    Token t = lexer.peek(1);
    if(t.token_type == ID)
    {
        Parse_Rule_List();
    }
    
}

void ReadGrammar()
{
    Parse_Rule_List();
    expect(HASH);
    expect(END_OF_FILE);    
}



// Task 1
void printTerminalsAndNoneTerminals(bool print)
{
    //create nt vector

    //store nt in nt vector, check if its in there first tho
    for(int i = 0; i < ruleTable.size(); i++)
    {
        string nt = ruleTable[i]->LHS;
        if(std::find(nonTerminals.begin(), nonTerminals.end(), nt) != nonTerminals.end())
        {
            continue;
        }
        else
        {
            nonTerminals.push_back(nt);
        }

    }

    for(int i = 0; i<ruleTable.size();i++)
    {
        string nonTerm = ruleTable[i]->LHS;
        if(std::find(orderNonTerminals.begin(), orderNonTerminals.end(), nonTerm) == orderNonTerminals.end())
        {
            //couldn't find it, so push it
            orderNonTerminals.push_back(nonTerm);
        }
        //move on to RHS
        for(int j = 0; j < ruleTable[i]->RHS.size(); j++)
        {
            string id = ruleTable[i]->RHS[j];
            //search for id in the unordered nonterminals list. If found, add to ordered NT. Else, add to ordered T
            if(std::find(nonTerminals.begin(),nonTerminals.end(), id) != nonTerminals.end())
            {
                //found it, now check if its in the orderedNT list already
                if(std::find(orderNonTerminals.begin(), orderNonTerminals.end(), id) == orderNonTerminals.end())
                {
                    //cant find it in ordered NT list, so add it
                    orderNonTerminals.push_back(id);
                }
            }
            else
            {
                //didnt find it, so search for it in orderedTerminal list
                if(std::find(orderTerminals.begin(), orderTerminals.end(), id) == orderTerminals.end())
                {
                    //cant find it in the ordered Terminals list, so add it
                    orderTerminals.push_back(id);
                }
            }
        }
    }

    if(print)
    {
        for(int k = 0; k < orderTerminals.size(); k++)
        {
            cout << orderTerminals[k] << " ";
        }

        for(int i = 0; i < orderNonTerminals.size(); i++)
        {
            cout << orderNonTerminals[i] << " ";
        }
    }

}

// Task 2
void RemoveUselessSymbols(bool printUseless)
{
    //store ordered terminal vector into generating set
    bool print = false;
    printTerminalsAndNoneTerminals(print);
    for(int i = 0; i<orderTerminals.size(); i++)
    {
        initialGen.push_back(orderTerminals[i]);
    }

    //use bool grammarHasEpsilon to determine if epsilon is actually in generating set, use it as check
    //If RHS.size() == 0 && grammarHasEpsilon = true, then add LHS to gen set
    bool changedSet = true;
    while(changedSet)
    {
        changedSet = false;
        for(int i = 0; i < ruleTable.size(); i++)
        {
            string lhs = ruleTable[i]->LHS;
            //check if lhs is in generating
            if(std::find(initialGen.begin(),initialGen.end(), lhs) == initialGen.end())
            {
                //couldnt find lhs within generating set
                //need to search through right hand side now

                //check if the RHS is an epsilon
                bool ruleHasEpsilon = false;
                if(ruleTable[i]->RHS.size()==0)
                {
                    ruleHasEpsilon = true;
                }

                bool allRHSInGenerating = true;
                for(int j = 0; j < ruleTable[i]->RHS.size(); j++)
                {
                    string rhs_symbol = ruleTable[i]->RHS[j];
                    //look for the rhs_symbol within generating set
                    if(std::find(initialGen.begin(),initialGen.end(), rhs_symbol) == initialGen.end())
                    {
                        //couldn't find it in generating
                        allRHSInGenerating = false;
                    }
                }

                if(allRHSInGenerating || ruleHasEpsilon)
                {
                    initialGen.push_back(lhs);
                    changedSet = true;
                }
            }
        }

    }

    //holy crap it worked first try
    //now we traverse every symbol in every rule. If all the symbols are in generating set, then add the rule to new vector
    std::vector<std::shared_ptr<struct rule>> generatingRules = std::vector<std::shared_ptr<struct rule>>();
    for(int i = 0; i < ruleTable.size(); i++)
    {
        bool LHS_is_in = false;
        bool RHS_is_in = true;
        string lhs_sym = ruleTable[i]->LHS;
        if(std::find(initialGen.begin(),initialGen.end(), lhs_sym) != initialGen.end())
        {
            //found it
            LHS_is_in = true;
        }
        for(int j = 0; j < ruleTable[i]->RHS.size(); j++)
        {
            string rhs_sym = ruleTable[i]->RHS[j];
            if(std::find(initialGen.begin(),initialGen.end(), rhs_sym) == initialGen.end())
            {
                //couldn't find it in generating set
                RHS_is_in = false;
            }
        }

        //both sides must be true to add rule to set
        if(LHS_is_in && RHS_is_in)
        {
            generatingRules.push_back(ruleTable[i]);
        }
    }

    //all rules do not generate
    if(generatingRules.size()==0)
    {
        return;
    }

    //Reachable
    //make a new reachable vector, put start symbol LHS into reachable vector, start changed = false
    //starting from first rule, if LHS is in set reachable, add all symbols in RHS to reachable (check if already there first), make changed = true
    std::vector<string> reachable = std::vector<string>();
    string startSymbol = ruleTable[0]->LHS;
    reachable.push_back(startSymbol);

    bool changed = true;
    while(changed)
    {
        changed = false;
        for(int i = 0; i < generatingRules.size(); i ++)
        {
            string lhs = generatingRules[i]->LHS;
            if(std::find(reachable.begin(), reachable.end(), lhs) != reachable.end())
            {
                //found LHS within reachable;
                //check if each symbol is in reachable set already. If it is, go to next symbol, else add it to reachable
                    for(int j = 0; j < generatingRules[i]->RHS.size();j++)
                    {
                        string rhs = generatingRules[i]->RHS[j];
                        if(std::find(reachable.begin(),reachable.end(), rhs) == reachable.end())
                        {                  
                            //didn't find it, add
                            reachable.push_back(rhs);
                            changed = true;
                        }

                    }
            }
        }
    }


    //now reachable is filled completely, so we need to copy the rules to the new vector
    //go thru each rule, if LHS is inside reachable, push_back to new vector
    for(int i = 0; i < generatingRules.size(); i++)
    {
        string lhs = generatingRules[i]->LHS;
        if(std::find(reachable.begin(),reachable.end(), lhs) != reachable.end())
        {
            //found the LHS within reachable, add rule to new vector
            eliminatedUseless.push_back(generatingRules[i]);
        }
    }

    if(printUseless)
    {
        //now we gotta print just to make sure that we got everything right on rule elimination
        for(int i = 0; i < eliminatedUseless.size(); i++)
        {
            cout << eliminatedUseless[i]->LHS << " -> ";
            if(eliminatedUseless[i]->RHS.size()==0)
            {
                cout << "#";
            }
            else
            {
                for(int j = 0; j < eliminatedUseless[i]->RHS.size(); j++)
                {
                    cout << eliminatedUseless[i]->RHS[j] << " ";
                }
            }

            cout << endl;
        }
    }
    


}

//Task 3 and 4 Helper Function
int returnIndexOfSymbol(string symbol, std::vector<std::shared_ptr<struct rule>> set)
{
    int index = -1;
    for(int i = 0; i < set.size(); i++)
    {
        if(symbol == set[i]->LHS)
        {
            //return its index
            index = i;
            break;
        }
    }
    return index;
}

// Task 3
void CalculateFirstSets(bool printing)
{
    //now we need to calculate the ordered t/nts
    bool print = false;
    printTerminalsAndNoneTerminals(print);
    //add epsilon to the firstSetList only if the grammar has an epsilon
    if(grammarHasEpsilon)
    {
        std::shared_ptr<struct rule> epsilon = std::make_shared<struct rule>();
        epsilon->RHS = std::vector<string>();
        epsilon->LHS = "#";
        epsilon->RHS.push_back("#");
        firstSetList.push_back(epsilon);
    }

    //now we push in the terminals
    for(int i = 0; i < orderTerminals.size(); i++)
    {
        std::shared_ptr<struct rule> terminalSet = std::make_shared<struct rule>();
        terminalSet->RHS = std::vector<string>();
        terminalSet->LHS = orderTerminals[i];
        terminalSet->RHS.push_back(orderTerminals[i]);
        firstSetList.push_back(terminalSet);
    }

    //now push the non-terminals
    for(int i = 0; i < orderNonTerminals.size(); i++)
    {
        std::shared_ptr<struct rule> nonterminalSet = std::make_shared<struct rule>();
        //initialize RHS vector, nothing in it so far
        nonterminalSet->RHS = std::vector<string>();
        nonterminalSet->LHS = orderNonTerminals[i];
        firstSetList.push_back(nonterminalSet);
    }

    bool setContainsEpsilon;
    bool changedSet = true;
    while(changedSet)
    {
        changedSet = false;
        //for every grammar rule in original rule table
        for(int i = 0; i < ruleTable.size(); i++)
        {
            //store the LHS symbol and return it's index in the firstSet vector
            string nonTerminal = ruleTable[i]->LHS;
            int nonTermIndex = returnIndexOfSymbol(nonTerminal, firstSetList);
            //if RHS is just epsilon then we add epsilon to the first of the LHS (Rule 5), go to next rule
            if(ruleTable[i]->RHS.size()==0)
            {
                //search for epsilon first
                if(std::find(firstSetList[nonTermIndex]->RHS.begin(),firstSetList[nonTermIndex]->RHS.end(), "#") == firstSetList[nonTermIndex]->RHS.end())
                {
                    //didn't find it
                    firstSetList[nonTermIndex]->RHS.push_back("#");
                    changedSet = true;
                    continue; 
                }
            }
            else
            {
                int lastElementIndex = ruleTable[i]->RHS.size() - 1;
                //RHS is 1 or more symbols
                for(int j = 0; j < ruleTable[i]->RHS.size();j++)
                {
                    setContainsEpsilon = false;
                    string symbol = ruleTable[i]->RHS[j];
                    int symbolIndex = returnIndexOfSymbol(symbol, firstSetList);
                    //add the FIRST(symbol) - epsilon to the FIRST(LHS), if its not already in there
                    //traverse the RHS of the symbol in question and add its contents, if not there
                    for(int k = 0; k < firstSetList[symbolIndex]->RHS.size(); k++)
                    {
                        string setContent = firstSetList[symbolIndex]->RHS[k];
                        if(setContent.compare("#")==0)
                        {
                            setContainsEpsilon = true;
                        }
                        if(std::find(firstSetList[nonTermIndex]->RHS.begin(),firstSetList[nonTermIndex]->RHS.end(), setContent) == firstSetList[nonTermIndex]->RHS.end())
                        {
                            //didn't find it, so add, Also dont add epsilon
                            if(setContent.compare("#")!=0)
                            {
                                firstSetList[nonTermIndex]->RHS.push_back(setContent);
                                changedSet = true;
                            }
                        }
                    }

                    //continue the forloop with j if setContainsEpsilon is true.
                    if(setContainsEpsilon)
                    {
                        //check if we are at the end
                        if(j == lastElementIndex)
                        {
                            //this says that all elements leading up to last contain epsilon, and last contains, epsilon, then add epsilon if not there already
                            if(std::find(firstSetList[nonTermIndex]->RHS.begin(),firstSetList[nonTermIndex]->RHS.end(), "#") == firstSetList[nonTermIndex]->RHS.end())
                            {
                                //couldn't find, add epsilon and break
                                firstSetList[nonTermIndex]->RHS.push_back("#");
                                changedSet = true;
                                break;
                            }
                        }
                        else
                        {
                            continue;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }

    }

    //algo works, just need to reorder the first set vectors into correct order
    for(int i = 0; i < firstSetList.size(); i++)
    {
        //only order the nonterminal first set lists
        if(std::find(orderNonTerminals.begin(),orderNonTerminals.end(), firstSetList[i]->LHS) != orderNonTerminals.end())
        {
            std::vector<string> temp = std::vector<string>();
                if(std::find(firstSetList[i]->RHS.begin(),firstSetList[i]->RHS.end(), "#") != firstSetList[i]->RHS.end())
                {
                    //if epsilon found in it's set, push it back 
                    firstSetList[i]->hasEpsilon = true;
                    temp.push_back("#");
                }
                //temp now has epsilon first. Now we must print the terminals the set contains in accordance to orderTerminals
                for(int k = 0; k<orderTerminals.size(); k++)
                {
                    string term = orderTerminals[k];
                    if(std::find(firstSetList[i]->RHS.begin(),firstSetList[i]->RHS.end(), term) != firstSetList[i]->RHS.end())
                    {
                        temp.push_back(term);
                    }
                }
            firstSetList[i]->RHS = temp;
        }
    }


    //test
    if(printing)
    {
        for(int i = 0; i < firstSetList.size(); i++)
        {
        if(std::find(orderNonTerminals.begin(),orderNonTerminals.end(), firstSetList[i]->LHS) != orderNonTerminals.end())
        {
            cout << "FIRST(" << firstSetList[i]->LHS << ") = { ";
            int lastIndex = firstSetList[i]->RHS.size() - 1;
            if(firstSetList[i]->RHS.size()==0)
            {
                cout << "}";
            }
            for(int j = 0; j < firstSetList[i]->RHS.size(); j++)
            {
                if(lastIndex == j)
                {
                    cout << firstSetList[i]->RHS[j] << " }";
                }
                else
                {
                    cout << firstSetList[i]->RHS[j] << ", ";
                }
            }
            cout << endl;
        }
        
        }
    }
    
}


// Task 4
void CalculateFollowSets(bool print)
{
    bool printing = false;
    //calc first sets without printing
    CalculateFirstSets(printing);
    //add only non-terminals to the followSetList
    for(int i = 0; i < orderNonTerminals.size(); i++)
    {
        std::shared_ptr<struct rule> nonterminalSet = std::make_shared<struct rule>();
        //initialize RHS vector, nothing in it so far
        nonterminalSet->RHS = std::vector<string>();
        nonterminalSet->LHS = orderNonTerminals[i];
        followSetList.push_back(nonterminalSet);
    }

    //Rule 1
    followSetList[0]->RHS.push_back("$");

    //first pass, rules 4 and 5
    for(int i = 0; i < ruleTable.size(); i ++)
    {
        if(ruleTable[i]->RHS.size() <= 1)
        {
            //if rule goes to epsilon or has only 1 element, continue on to the next rule
            continue;
        }
        //RHS.size() - 1 because we are dealing with j+1, avoid seg fault
        int lastElementIndex = ruleTable[i]->RHS.size() - 1;
        for(int j = 0; j < ruleTable[i]->RHS.size() - 1; j++)
        {
            //we only add to nonterminal follow sets, so always check if current element is nt or not
            string currentElement = ruleTable[i]->RHS[j];
            if(std::find(orderNonTerminals.begin(),orderNonTerminals.end(), currentElement) == orderNonTerminals.end())
            {
                //can't find it in the NT list, so continue with the iteration
                continue;
            }
            string nextElement = ruleTable[i]->RHS[j+1];
            //rule 4, add FIRST(nextElement) to FOLLOW(currentElement)
            //first need Indexes for both sets
            bool setContainsEpsilon = false;
            int indexOfNextElement = returnIndexOfSymbol(nextElement, firstSetList);
            int indexOfCurrentElement = returnIndexOfSymbol(currentElement, followSetList);
            for(int k = 0; k < firstSetList[indexOfNextElement]->RHS.size(); k++)
            {
                string setContent = firstSetList[indexOfNextElement]->RHS[k];
                if(setContent.compare("#")==0)
                {
                    setContainsEpsilon = true;
                }
                //if the FIRST set element is not already in FOLLOW(NT), and is not epsilon, then add
                if(std::find(followSetList[indexOfCurrentElement]->RHS.begin(),followSetList[indexOfCurrentElement]->RHS.end(), setContent) == followSetList[indexOfCurrentElement]->RHS.end())
                {
                    //didn't find it, so add, Also dont add epsilon
                    if(setContent.compare("#")!=0)
                    {
                        followSetList[indexOfCurrentElement]->RHS.push_back(setContent);
                    }
                }
            }

            //if the set contains epsilon, then we need to evaluate rule 5
            if(setContainsEpsilon)
            {
                //rule 5 needs to add j+2, j+3, j+n to the currentElement if they all contain epsilons
                if(j != lastElementIndex - 1)
                {
                    //start at the index to the right of nextElement
                    for(int m = j + 2; m <= lastElementIndex; m++)
                    {
                        bool newSetContainsEps = false;
                        //add FIRST(m) to FOLLOW (currentElement)
                        string mElement = ruleTable[i]->RHS[m];
                        int indexOfmElement = returnIndexOfSymbol(mElement, firstSetList);
                        //traverse this elements' FIRST set and check if it has epsilon
                        for(int h = 0; h < firstSetList[indexOfmElement]->RHS.size(); h++)
                        {
                            string content = firstSetList[indexOfmElement]->RHS[h];
                            if(content.compare("#")==0)
                            {
                                newSetContainsEps = true;
                            }
                            //avoid adding duplicates
                            if(std::find(followSetList[indexOfCurrentElement]->RHS.begin(),followSetList[indexOfCurrentElement]->RHS.end(), content) == followSetList[indexOfCurrentElement]->RHS.end())
                            {
                                if(content.compare("#")!=0)
                                {
                                    //add content
                                    followSetList[indexOfCurrentElement]->RHS.push_back(content);
                                }
                            }
                        }
                        if(newSetContainsEps)
                        {
                            continue;
                        }
                        else
                        {
                            break;
                        }
                        
                    }
                }
                else
                {
                    //continue once we hit second to last element
                    continue;
                }
            }
        }
    }

    //first pass works, now onto rules 2 and 3
    bool changed = true;
    while(changed)
    {
        changed = false;
        for(int i = 0; i < ruleTable.size(); i++)
        {
            if(ruleTable[i]->RHS.size() == 0)
            {
                //if rule goes to epsilon, skip
                continue;
            }

            //store LHS into lhs
            string lhs = ruleTable[i]->LHS;
            int indexOfLHS = returnIndexOfSymbol(lhs, followSetList);
            //start at the very end
            int lastElementRHS = ruleTable[i]->RHS.size()-1;
            string lastElement = ruleTable[i]->RHS[lastElementRHS];
            //check if element at j is nonterminal
            if(std::find(orderNonTerminals.begin(),orderNonTerminals.end(), lastElement) != orderNonTerminals.end())
            {
                //found it in the NT list, so add FOLLOW(LHS) to FOLLOW(lastElement), (RULE 2)
                bool containsEpsilon = false;
                int indexOfLastElement = returnIndexOfSymbol(lastElement, followSetList);
                for(int k = 0; k < followSetList[indexOfLHS]->RHS.size(); k++)
                {
                    string content = followSetList[indexOfLHS]->RHS[k];
                    if(std::find(followSetList[indexOfLastElement]->RHS.begin(),followSetList[indexOfLastElement]->RHS.end(), content) == followSetList[indexOfLastElement]->RHS.end())
                    {
                        //couldnt find the content inside of follow(lastElement), so add it and make change = true
                        followSetList[indexOfLastElement]->RHS.push_back(content);
                        changed = true;
                    }
                }
                //Now we must check if epsilon is in the first(lastElement) (RULE 5)
                int indexOfLastInFirst = returnIndexOfSymbol(lastElement, firstSetList);
                bool firstContainsEps = false;
                //check if FIRST(lastElement) contains epsilon
                for(int k = 0; k < firstSetList[indexOfLastInFirst]->RHS.size(); k++)
                {
                    string terminal = firstSetList[indexOfLastInFirst]->RHS[k];
                    if(terminal.compare("#")==0)
                    {
                        firstContainsEps = true;
                    }
                }

                //apply Rule 3 when last and subsequent elements contain epsilon
                if(firstContainsEps)
                {
                    //idk about the condition on this for loop
                    for(int n = lastElementRHS - 1; n >= 0; n--)
                    {
                        bool newSetHasEps = false;
                        //check if the nth element is NT
                        string symbol = ruleTable[i]->RHS[n];
                        if(std::find(orderNonTerminals.begin(),orderNonTerminals.end(), symbol) != orderNonTerminals.end())
                        {
                            //found the symbol in NT list
                            //add FOLLOW(LHS) to FOLLOW(symbol)
                            int indexOfSymbol = returnIndexOfSymbol(symbol, followSetList);
                            for(int m = 0; m < followSetList[indexOfLHS]->RHS.size(); m++)
                            {
                                string terminal = followSetList[indexOfLHS]->RHS[m];
                                //check the terminal at m FOLLOW(LHS), search if it exists in FOLLOW(symbol), then add
                                if(std::find(followSetList[indexOfSymbol]->RHS.begin(),followSetList[indexOfSymbol]->RHS.end(), terminal) == followSetList[indexOfSymbol]->RHS.end())
                                {
                                    //couldnt find the content inside of follow(lastElement), so add it and make change = true
                                    followSetList[indexOfSymbol]->RHS.push_back(terminal);
                                    changed = true;
                                }
                            }

                            //check if FIRST(symbol) has epsilon. If it does, continue this loop, If it does not, break
                            int firstSetSymbol = returnIndexOfSymbol(symbol, firstSetList);
                            for(int m = 0; m < firstSetList[firstSetSymbol]->RHS.size(); m++)
                            {
                                string terminal = firstSetList[firstSetSymbol]->RHS[m];
                                if(terminal.compare("#")==0)
                                {
                                    newSetHasEps = true;
                                }
                            }
                            
                            if(newSetHasEps)
                            {
                                continue;
                            }
                            else
                            {
                                break;
                            }
                        }
                        else
                        {
                            //is a terminal, break the for loop
                            break;
                        }
                    }
                }
            }
            else
            {
                //did not find the last element in NT list, so move to next iteration
                continue;
            }
        }
    }



    //reorder
    for(int i = 0; i < followSetList.size(); i++)
    {
        std::vector<string> temp = std::vector<string>();
        if(std::find(followSetList[i]->RHS.begin(),followSetList[i]->RHS.end(), "$") != followSetList[i]->RHS.end())
        {
            //if $ found in it's set, push it back 
            temp.push_back("$");
        }
        //temp now has $ first. Now we must print the terminals the set contains in accordance to orderTerminals
        for(int k = 0; k<orderTerminals.size(); k++)
        {
            string term = orderTerminals[k];
            if(std::find(followSetList[i]->RHS.begin(),followSetList[i]->RHS.end(), term) != followSetList[i]->RHS.end())
            {
                temp.push_back(term);
            }
        }
        followSetList[i]->RHS = temp;
    }

    if(print)
    {
        //print
        for(int i = 0; i < followSetList.size(); i++)
        {

            cout << "FOLLOW(" << followSetList[i]->LHS << ") = { ";
            int lastIndex = followSetList[i]->RHS.size() - 1;
            if(followSetList[i]->RHS.size()==0)
            {
                cout << "}";
            }
            for(int j = 0; j < followSetList[i]->RHS.size(); j++)
            {
                if(lastIndex == j)
                {
                    cout << followSetList[i]->RHS[j] << " }";
                }
                else
                {
                    cout << followSetList[i]->RHS[j] << ", ";
                }
            }
            cout << endl;
        }
    }
    
}

// Task 5 Helper
int returnIndexOfNT(string symbol, std::vector<std::shared_ptr<struct predictive>> set)
{
    int index = -1;
    for(int i = 0; i < set.size(); i++)
    {
        if(symbol == set[i]->LHS)
        {
            //return its index
            index = i;
            break;
        }
    }
    return index;
}

// Task 5
void CheckIfGrammarHasPredictiveParser()
{
    bool printUseless = false;
    RemoveUselessSymbols(printUseless);
    //first check if the grammar has useless symbols. If it does, cout NO
    //we can do this by comparing eliminatedUseless with the original ruleTable. If the sizes differ, it mean a rule was eliminated
    if(ruleTable.size()!=eliminatedUseless.size())
    {
        cout << "NO" << endl;
        return;
    }

    //call these functions only if the grammar does not have any useless rules
    bool print = false;
    CalculateFirstSets(print);
    CalculateFollowSets(print);

    bool condition1 = false;
    bool condition2 = false;
    //we now must evaluate each condition for predictive parsing
    //for each grammar rule, we need to determine how many FIRST sets we must compare in respect to each nonTerimnal
    //we can store this representation inside a new predictiveTable
    for(int i = 0; i < orderNonTerminals.size(); i++)
    {
        string nt = orderNonTerminals[i];
        std::shared_ptr<struct predictive> nonTermRule = std::make_shared<struct predictive>();
        nonTermRule->set = std::vector<std::vector<string>>();
        nonTermRule->LHS = nt;
        predictiveTable.push_back(nonTermRule);
    }

    for(int i = 0; i < ruleTable.size(); i++)
    {
        //make a new set vector
        std::vector<string> rhsFirstSet = std::vector<string>();

        string nt = ruleTable[i]->LHS;
        //lookup where nt is in the predictiveTable
        int whereIsNT = returnIndexOfNT(nt, predictiveTable);
        //if the first symbol is just a terminal, then we can add the terminal itself to the rhsFirstSet and call it a day
        if(ruleTable[i]->RHS.size()==0)
        {
            rhsFirstSet.push_back("#");
            predictiveTable[whereIsNT]->set.push_back(rhsFirstSet);
            continue;
        }

        string symbol = ruleTable[i]->RHS[0];
        if(std::find(orderTerminals.begin(), orderTerminals.end(), symbol)!=orderTerminals.end())
        {
            //push back the symbol itself since FIRST(terminal) is just the terminal itself
            rhsFirstSet.push_back(symbol);
            predictiveTable[whereIsNT]->set.push_back(rhsFirstSet);
            //move to next rule
            continue;
        }

        int lastElementIndex = ruleTable[i]->RHS.size() - 1;
        //if first symbol on RHS is not a terminal or epsilon
        for(int j = 0; j < ruleTable[i]->RHS.size(); j++)
        {
            symbol = ruleTable[i]->RHS[j];
            //look up symbol in it firstSets and traverse it's RHS
            int whereIsSymbol = returnIndexOfSymbol(symbol, firstSetList);
            bool setHasEps = firstSetList[whereIsSymbol]->hasEpsilon;
            //when do we add epsilon? to the rhsSetVector? if the next value is a terminal, dont add.
            for(int k = 0; k < firstSetList[whereIsSymbol]->RHS.size(); k++)
            {
                string content = firstSetList[whereIsSymbol]->RHS[k];
                //dont add duplicates
                if(std::find(rhsFirstSet.begin(),rhsFirstSet.end(), content) == rhsFirstSet.end())
                {
                    if(content.compare("#")!=0)
                    {
                        rhsFirstSet.push_back(content);
                    }
                }
            }
            if(setHasEps)
            {
                if(j == lastElementIndex)
                {
                    if(std::find(rhsFirstSet.begin(),rhsFirstSet.end(), "#") == rhsFirstSet.end())
                    {
                        rhsFirstSet.push_back("#");
                        break;
                    }
                }
                else
                {
                    continue;
                }
            }
            else
            {
                break;
            }
        }

        //now we need to push back the set to the RHS
        predictiveTable[whereIsNT]->set.push_back(rhsFirstSet);
    }

    //our structure build works. Now we need to go thru each element in the predictiveTable
    //if the set.size() = 1, then we can automatically pass the condition
    //need a vector of strings to check if each condition of each rule passes
    std::vector<string> condition1Vector = std::vector<string>();
    std::vector<string> condition2Vector = std::vector<string>();
    for(int i = 0; i < predictiveTable.size(); i++)
    {
        if(predictiveTable[i]->set.size()==1)
        {
            //auto pass condition1 for the rule because the Rule only has one FIRST set, skip to next rule
            condition1Vector.push_back("t");
            continue;
        }

        //this is our unionVec we need to check for each NT in predictive table. Initialize
        predictiveTable[i]->unionVec = std::vector<string>();
        predictiveTable[i]->intersectionVec = std::vector<string>();

        //now we need to find disjoint sets for condition 1
        for(int j = 0; j < predictiveTable[i]->set.size(); j++)
        {
            //we could take all the set vectors, dump them into a single vector, then check for duplicate values within that vector
            //if the vector has duplicate values, then we know that the intersection is not empty, and therefore pushback condition1.pushback(f), otherwise true
            for(int k = 0; k < predictiveTable[i]->set[j].size(); k++)
            {
                string content = predictiveTable[i]->set[j][k];
                predictiveTable[i]->unionVec.push_back(content);
            }
        }

        //sort the union vector
        sort(predictiveTable[i]->unionVec.begin(), predictiveTable[i]->unionVec.end());
        //now we have a union vector and need to check it for duplicate values. If dupe vals, pushback f. else, pushback t
        for(int j = 1; j < predictiveTable[i]->unionVec.size(); j++)
        {
            //if the next and current are the same, we have a duplicate
            string next = predictiveTable[i]->unionVec[j];
            string prev = predictiveTable[i]->unionVec[j - 1];
            if(prev.compare(next)==0)
            {
                predictiveTable[i]->intersectionVec.push_back(next);
            }
        }

        //if the predictiveTable->intersec.size() == 0, push back true to condition 1. else, push back false
        if(predictiveTable[i]->intersectionVec.size()==0)
        {
            condition1Vector.push_back("t");
        }
        else
        {
            condition1Vector.push_back("f");    
        }

        //condition2
        //going to resuse unionVec and intersectionVec 
        std::vector<string> blank = std::vector<string>();
        predictiveTable[i]->unionVec = blank;
        predictiveTable[i]->intersectionVec = blank;

        //determine if we need to even apply condition two. If eps is NOT in first set of NT, then condition2 auto passes
        string nt = predictiveTable[i]->LHS;
        int whereIsNTfirst = returnIndexOfSymbol(nt, firstSetList);
        int whereisNTfollow = returnIndexOfSymbol(nt, followSetList);
        bool firstSetHasEps = firstSetList[whereIsNTfirst]->hasEpsilon;
        if(firstSetHasEps)
        {
            //calculate if FIRST(NT) intersects with FOLLOW(NT)
            //iterate through the FIRST(NT) and add it to unionVec
            for(int j = 0; j < firstSetList[whereIsNTfirst]->RHS.size(); j++)
            {
                predictiveTable[i]->unionVec.push_back(firstSetList[whereIsNTfirst]->RHS[j]);
            }
            //iterate through the FOLLOW(NT) and add it to the unionVec
            for(int j = 0; j < followSetList[whereisNTfollow]->RHS.size();j++)
            {
                predictiveTable[i]->unionVec.push_back(followSetList[whereisNTfollow]->RHS[j]);
            }
            
            //sort
            sort(predictiveTable[i]->unionVec.begin(), predictiveTable[i]->unionVec.end());
            for(int j = 1; j < predictiveTable[i]->unionVec.size(); j++)
            {
                //if the next and current are the same, we have a duplicate
                string next = predictiveTable[i]->unionVec[j];
                string prev = predictiveTable[i]->unionVec[j - 1];
                if(prev.compare(next)==0)
                {
                    predictiveTable[i]->intersectionVec.push_back(next);
                }
            }
            //if intersection is size 0, condition2 passes
            if(predictiveTable[i]->intersectionVec.size() == 0)
            {
                condition2Vector.push_back("t");
            }
            else
            {
                condition2Vector.push_back("f");   
            }
        }
        else
        {
            //auto passes condition2 if set doesn't have epsilon
            condition2Vector.push_back("t");   
        }

    }



    //condition1
    if(std::find(condition1Vector.begin(), condition1Vector.end(), "f") == condition1Vector.end())
    {
        //if it cant find any falses for condition1Vector, set condition1 = true
        condition1 = true;
    }

    if(std::find(condition2Vector.begin(), condition2Vector.end(), "f") == condition2Vector.end())
    {
        //if it cant find any falses for condition1Vector, set condition2 = true
        condition2 = true;
    }

    if(condition1 && condition2)
    {
        cout << "YES" <<endl;
    }
    else
    {
        cout << "NO" << endl;
    }

}
    
int main (int argc, char* argv[])
{
    int task;

    if (argc < 2)
    {
        cout << "Error: missing argument\n";
        return 1;
    }

    /*
       Note that by convention argv[0] is the name of your executable,
       and the first argument to your program is stored in argv[1]
     */

    task = atoi(argv[1]);
    
    ReadGrammar();  // Reads the input grammar from standard input
                    // and represent it internally in data structures
                    // ad described in project 2 presentation file

    switch (task) {
        case 1: 
        {
            bool printing = true;
            printTerminalsAndNoneTerminals(printing);
        }
        break;

        case 2:
        {
            bool printing = true;
            RemoveUselessSymbols(printing);
        } 
        break;
        case 3: 
        {
            bool printing = true;
            CalculateFirstSets(printing);
        }
        break;
        case 4:
        {
            bool printing = true;
            CalculateFollowSets(printing);
        } 
        break;
        case 5: CheckIfGrammarHasPredictiveParser();
            break;

        default:
            cout << "Error: unrecognized task number " << task << "\n";
            break;
    }
    return 0;
}

