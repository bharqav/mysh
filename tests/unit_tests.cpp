#include "../executor/Executor.hpp"
#include "../lexer/Lexer.hpp"
#include "../parser/Parser.hpp"
#include "../utils/Env.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

int runLine(const std::string& line, Environment& env) {
    auto tokens = Lexer::tokenize(line, env);
    auto program = Parser::parse(tokens);
    return Executor::execute(program, env);
}

void testLexerAssignments() {
    Environment env;
    auto tokens = Lexer::tokenize("A=1 B=2 echo $A$B", env);
    require(tokens.size() == 4, "expected 4 tokens for assignment+command line");
    require(tokens[0].type == TokenType::ASSIGNMENT, "token 0 should be assignment");
    require(tokens[1].type == TokenType::ASSIGNMENT, "token 1 should be assignment");
    require(tokens[2].type == TokenType::WORD, "token 2 should be word");
    require(tokens[3].type == TokenType::WORD, "token 3 should be word");
    require(tokens[3].value == "12", "assignment overlay expansion should resolve to 12");
}

void testParserPrecedence() {
    Environment env;
    int status = runLine("true || false && false", env);
    require(status == 0, "OR/AND precedence should evaluate as true || (false && false)");
}

void testPipelineExitStatus() {
    Environment env;
    int status = runLine("false | true", env);
    require(status == 0, "pipeline status should follow rightmost command (false | true => 0)");

    status = runLine("true | false", env);
    require(status == 1, "pipeline status should follow rightmost command (true | false => 1)");
}

void testCommandSubstitution() {
    Environment env;
    int status = runLine("echo $(echo inner)", env);
    require(status == 0, "command substitution should execute successfully");
}

void testSubshellScoping() {
    Environment env;
    runLine("export OUTER=stay", env);
    int status = runLine("(export OUTER=changed)", env);
    require(status == 0, "subshell should execute");
    require(env.get("OUTER") == "stay", "subshell changes must not mutate parent env");
}

void testParserErrors() {
    Environment env;
    bool threw = false;
    try {
        auto tokens = Lexer::tokenize("echo a | | wc", env);
        (void)Parser::parse(tokens);
    } catch (const std::exception&) {
        threw = true;
    }
    require(threw, "parser should throw on malformed pipeline");
}

} // namespace

int main() {
    try {
        testLexerAssignments();
        testParserPrecedence();
        testPipelineExitStatus();
        testCommandSubstitution();
        testSubshellScoping();
        testParserErrors();
        std::cout << "unit tests passed\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "unit test failed: " << ex.what() << "\n";
        return 1;
    }
}
