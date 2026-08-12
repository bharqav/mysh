#include "Parser.hpp"

#include <sstream>
#include <stdexcept>

namespace {
const char* tokenName(TokenType t) {
    switch (t) {
    case TokenType::WORD:
        return "word";
    case TokenType::ASSIGNMENT:
        return "assignment";
    case TokenType::PIPE:
        return "|";
    case TokenType::AND_IF:
        return "&&";
    case TokenType::OR_IF:
        return "||";
    case TokenType::SEMI:
        return ";";
    case TokenType::AMPERSAND:
        return "&";
    case TokenType::LPAREN:
        return "(";
    case TokenType::RPAREN:
        return ")";
    case TokenType::REDIRECT_IN:
        return "<";
    case TokenType::REDIRECT_OUT:
        return ">";
    case TokenType::APPEND:
        return ">>";
    case TokenType::HEREDOC:
        return "<<";
    default:
        return "?";
    }
}

std::runtime_error syntaxError(const std::string& message, const Token* tok) {
    std::ostringstream oss;
    oss << "parse error: " << message;
    if (tok != nullptr) {
        oss << " near '" << tokenName(tok->type);
        if (!tok->value.empty()) {
            oss << " " << tok->value;
        }
        oss << "'";
    }
    return std::runtime_error(oss.str());
}

RedirectionType mapTokenToRedir(TokenType t) {
    switch (t) {
    case TokenType::REDIRECT_IN:
        return RedirectionType::In;
    case TokenType::REDIRECT_OUT:
        return RedirectionType::Out;
    case TokenType::APPEND:
        return RedirectionType::Append;
    case TokenType::HEREDOC:
        return RedirectionType::Heredoc;
    default:
        throw std::runtime_error("internal parse error: invalid redirection token");
    }
}

Assignment parseAssignment(const std::string& token) {
    std::size_t eq = token.find('=');
    if (eq == std::string::npos || eq == 0) {
        throw std::runtime_error("internal parse error: invalid assignment token");
    }
    Assignment assignment;
    assignment.key = token.substr(0, eq);
    assignment.value = token.substr(eq + 1);
    return assignment;
}

class ParserImpl {
public:
    explicit ParserImpl(const std::vector<Token>& tokens) : tokens_(tokens), pos_(0) {}

    ExprPtr parseExpression() {
        return parseSequence();
    }

    bool atEnd() const {
        return pos_ >= tokens_.size();
    }

    const Token* currentToken() const {
        if (atEnd()) {
            return nullptr;
        }
        return &tokens_[pos_];
    }

private:
    ExprPtr parseSequence() {
        ExprPtr left = parseOr();
        while (match(TokenType::SEMI)) {
            auto node = std::make_unique<Expr>();
            node->kind = ExprKind::Sequence;
            node->left = std::move(left);
            node->right = parseOr();
            left = std::move(node);
        }
        return left;
    }

    ExprPtr parseOr() {
        ExprPtr left = parseAnd();
        while (match(TokenType::OR_IF)) {
            auto node = std::make_unique<Expr>();
            node->kind = ExprKind::Or;
            node->left = std::move(left);
            node->right = parseAnd();
            left = std::move(node);
        }
        return left;
    }

    ExprPtr parseAnd() {
        ExprPtr left = parseTerm();
        while (match(TokenType::AND_IF)) {
            auto node = std::make_unique<Expr>();
            node->kind = ExprKind::And;
            node->left = std::move(left);
            node->right = parseTerm();
            left = std::move(node);
        }
        return left;
    }

    ExprPtr parseTerm() {
        std::vector<Assignment> assignments;
        while (!atEnd() && tokens_[pos_].type == TokenType::ASSIGNMENT) {
            assignments.push_back(parseAssignment(tokens_[pos_].value));
            ++pos_;
        }

        if (match(TokenType::LPAREN)) {
            ExprPtr grouped = parseExpression();
            consume(TokenType::RPAREN, "expected ')' after group");
            auto node = std::make_unique<Expr>();
            node->kind = ExprKind::Subshell;
            node->left = std::move(grouped);
            node->assignments = std::move(assignments);
            return node;
        }
        return parsePipeline(std::move(assignments));
    }

    ExprPtr parsePipeline(std::vector<Assignment> assignments = {}) {
        Pipeline pipeline;
        Command current;

        if (!assignments.empty()) {
            current.assignments = std::move(assignments);
        }

        while (!atEnd()) {
            const Token& tok = tokens_[pos_];
            if (tok.type == TokenType::ASSIGNMENT && current.args.empty()) {
                current.assignments.push_back(parseAssignment(tok.value));
                ++pos_; 
                continue;
            }
            if (tok.type == TokenType::WORD) {
                current.args.push_back(tok.value);
                ++pos_;
                continue;
            }
            if (tok.type == TokenType::REDIRECT_IN || tok.type == TokenType::REDIRECT_OUT ||
                tok.type == TokenType::APPEND || tok.type == TokenType::HEREDOC) {
                ++pos_;
                if (atEnd() || tokens_[pos_].type != TokenType::WORD) {
                    const Token* near = atEnd() ? nullptr : &tokens_[pos_];
                    throw syntaxError("expected file or delimiter after redirection", near);
                }
                current.redirections.push_back({mapTokenToRedir(tok.type), tokens_[pos_].value});
                ++pos_;
                continue;
            }
            if (tok.type == TokenType::PIPE) {
                if (current.args.empty()) {
                    throw syntaxError("missing command before pipe", &tok);
                }
                pipeline.commands.push_back(current);
                current = Command{};
                ++pos_;
                continue;
            }
            if (tok.type == TokenType::SEMI || tok.type == TokenType::AND_IF || tok.type == TokenType::OR_IF ||
                tok.type == TokenType::RPAREN) {
                break;
            }
            break;
        }

        if (!current.args.empty() || !current.assignments.empty()) {
            pipeline.commands.push_back(current);
        }
        if (pipeline.commands.empty()) {
            const Token* near = atEnd() ? nullptr : &tokens_[pos_];
            throw syntaxError("expected command", near);
        }

        bool background = false;
        if (match(TokenType::AMPERSAND)) {
            background = true;
        }

        auto expr = std::make_unique<Expr>();
        expr->kind = ExprKind::Pipeline;
        expr->pipeline = std::move(pipeline);
        expr->runInBackground = background;
        return expr;
    }

    bool match(TokenType type) {
        if (!atEnd() && tokens_[pos_].type == type) {
            ++pos_;
            return true;
        }
        return false;
    }

    void consume(TokenType type, const char* message) {
        if (atEnd() || tokens_[pos_].type != type) {
            const Token* near = atEnd() ? nullptr : &tokens_[pos_];
            throw syntaxError(message, near);
        }
        ++pos_;
    }

    const std::vector<Token>& tokens_;
    std::size_t pos_;
};
} // namespace

ExprPtr Parser::parse(const std::vector<Token>& tokens) {
    ParserImpl parser(tokens);
    ExprPtr root = parser.parseExpression();
    if (!parser.atEnd()) {
        throw syntaxError("unexpected trailing tokens", parser.currentToken());
    }
    return root;
}
