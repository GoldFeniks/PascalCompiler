#include "syntax_analyzer.hpp"

using namespace pascal_compiler::syntax_analyzer;
using namespace tree;

syntax_error::syntax_error(const std::string& message, const tree_node::position_type position) {
    message_ = str(boost::format("(%1%, %2%) Syntax error: %3%") % 
        position.first % position.second % message);
}

syntax_analyzer& syntax_analyzer::operator=(
    syntax_analyzer&& other) noexcept {
    std::swap(tokenizer_, other.tokenizer_);
    swap(root_, other.root_);
    return *this;
}

tree_node_p syntax_analyzer::parse_simple_expression() {
    return parse_operation<&syntax_analyzer::parse_expression, simple_expression_operators>();
}

tree_node_p syntax_analyzer::parse_expression() {
    return parse_operation<&syntax_analyzer::parse_factor, expression_operators>();
}

tree_node_p syntax_analyzer::parse_term() {
    return parse_operation<&syntax_analyzer::parse_factor, term_operators>();
}

tree_node_p syntax_analyzer::parse_factor() {
    auto token = tokenizer_.current(); tokenizer_.next();
    switch (token->get_sub_type()) {
    case pascal_compiler::tokenizer::token::sub_types::identifier:
    {
        auto decl = find_declaration(token);
        const auto var = std::make_shared<variable_node>(token->get_string_value(), token->get_position(), decl.first, decl.second);
        switch(var->type()->category()) {
        case type::type_category::function:
            return parse_function_call(var);
        case type::type_category::array:
            return parse_index(var);
        case type::type_category::record:
            return parse_field_access(var);
        default:
            return var;
        }
    }
    case pascal_compiler::tokenizer::token::sub_types::integer_const:
        return std::make_shared<constant_node>(token->get_string(), integer, token->get_value(), token->get_position());
    case pascal_compiler::tokenizer::token::sub_types::real_const:
        return std::make_shared<constant_node>(token->get_string(), real, token->get_value(), token->get_position());
    case pascal_compiler::tokenizer::token::sub_types::char_const:
        return std::make_shared<constant_node>(token->get_string(), symbol, token->get_value(), token->get_position());
    case pascal_compiler::tokenizer::token::sub_types::open_parenthesis:
    {
        const auto node = parse_expression();
        require(tokenizer_.current(), pascal_compiler::tokenizer::token::sub_types::close_parenthesis);
        tokenizer_.next();
        return node;
    }
    case pascal_compiler::tokenizer::token::sub_types::plus:
    case pascal_compiler::tokenizer::token::sub_types::minus:
    case pascal_compiler::tokenizer::token::sub_types::not:
        return std::make_shared<operation_node>(token, parse_factor());
    default:
        throw unexpected_token(token, pascal_compiler::tokenizer::token::sub_types::identifier);
    }
}

tree_node_p syntax_analyzer::parse_function_call(const tree_node_p node) {
    const auto node_type = get_type(node);
    require(node_type, type::type_category::function, node->position());
    const auto func = std::dynamic_pointer_cast<function_type>(node_type);
    const auto result = std::make_shared<call_node>(node->position(), node, parse_actual_parameter_list(func));
    switch(func->return_type()->category()) {
    case type::type_category::array:
        return parse_index(result);
    case type::type_category::record:
        return parse_field_access(result);
    default:
        return result;
    }
}

tree_node_p syntax_analyzer::parse_index(tree_node_p node) {
    auto token = tokenizer_.current();
    auto node_type = get_type(node);
    while (token->get_sub_type() == pascal_compiler::tokenizer::token::sub_types::open_bracket) {
        require(node_type, type::type_category::array, token->get_position());
        const auto a = std::dynamic_pointer_cast<array_type>(node_type);
        tokenizer_.next();
        node = std::make_shared<index_node>(token->get_position(), node, a->element_type(), parse_expression());
        require(tokenizer_.current(), pascal_compiler::tokenizer::token::sub_types::close_bracket);
        node_type = a->element_type();
        token = tokenizer_.next();
    }
    if (node_type->category() == type::type_category::record)
        return parse_field_access(node);
    return node;
}

tree_node_p syntax_analyzer::parse_field_access(tree_node_p node) {
    auto token = tokenizer_.current();
    auto node_type = get_type(node);
    while (token->get_sub_type() == pascal_compiler::tokenizer::token::sub_types::dot) {
        require(node_type, type::type_category::record, token->get_position());
        const auto position = token->get_position();
        token = tokenizer_.next();
        require(token, pascal_compiler::tokenizer::token::sub_types::identifier);
        const auto r = std::dynamic_pointer_cast<record_type>(node_type);
        const auto it = r->fields().table().find(token->get_string_value());
        if (it != r->fields().table().end())
            node = std::make_shared<field_access_node>(position, node,
                std::make_shared<variable_node>(token->get_value_string(), token->get_position(),
                    it->second.first, it->second.second));
        else
            throw field_not_found(token);
        node_type = it->second.first;
    }
    if (node_type->category() == type::type_category::array)
        return parse_index(node);
    return node;
}

tree_node_p syntax_analyzer::parse_actual_parameter_list(const function_type_p& type) {
    auto result = 
        std::make_shared<tree_node>("p_list", tree_node::node_category::null, tokenizer_.current()->get_position());
    std::vector<tree_node_p> args;
    parse_expressions_list(args);
    size_t i = 0;
    for (; i < args.size(); ++i) {
        if (i == type->parameters().size())
            throw syntax_error("Illegal parameters count", args[i]->position());
        const auto tl = type->parameters().get_type(i), tr = get_type(args[i]);
        require_types_compatibility(tl, tr, result->position());
        if (tl != tr)
            result->push_back(std::make_shared<cast_node>(tl, args[i], args[i]->position()));
        else
            result->push_back(args[i]);
    }
    if (i != type->parameters().size() && type->parameters()[i].second)
        throw syntax_error("Illegal parameters count", args.back()->position());
    return result;
}

tree_node_p syntax_analyzer::parse_block() {
    parse_declaration_part();
    return parse_compound_statement();
}

tree_node_p syntax_analyzer::parse_typed_const(const type_p& type) {
    const auto result_type = type->base_type();
    if (result_type->is_scalar())
        return parse_expression();
    auto token = tokenizer_.current();
    require(token, pascal_compiler::tokenizer::token::sub_types::open_parenthesis);
    auto result = std::make_shared<typed_constant_node>(token->get_position(), result_type);
    if (result_type->is_category(type::type_category::array)) {
        const auto a = std::dynamic_pointer_cast<array_type>(result_type);
        const auto base_type = a->element_type()->base_type();
        for (auto i = a->min(); i <= a->max(); ++i) {
            token = tokenizer_.next();
            auto c = parse_typed_const(base_type);
            require_types_compatibility(base_type, get_type(c), token->get_position());
            if (base_type != get_type(c))
                c = std::make_shared<cast_node>(base_type, c, token->get_position());
            i == a->max() 
                ? require(tokenizer_.current(), pascal_compiler::tokenizer::token::sub_types::close_parenthesis) 
                : require(tokenizer_.current(), pascal_compiler::tokenizer::token::sub_types::comma);  
            result->push_back(c);
        }
        tokenizer_.next();
        return result;
    }
    token = tokenizer_.next();
    const auto r = std::dynamic_pointer_cast<record_type>(result_type);
    for (const auto it : r->fields().vector()) {
        require(token, pascal_compiler::tokenizer::token::sub_types::identifier);
        require(tokenizer_.next(), pascal_compiler::tokenizer::token::sub_types::colon);
        if (it.first != token->get_string_value())
            throw syntax_error("Illegal initialization order", token->get_position());
        tokenizer_.next();
        const auto base_type = it.second.first->base_type();
        auto c = parse_typed_const(base_type);
        require_types_compatibility(base_type, get_type(c), token->get_position());
        if (base_type != get_type(c))
            c = std::make_shared<cast_node>(base_type, c, token->get_position());
        result->push_back(c);
        if (tokenizer_.current()->get_sub_type() == pascal_compiler::tokenizer::token::sub_types::close_parenthesis)
            break;
        require(pascal_compiler::tokenizer::token::sub_types::semicolon);
        token = tokenizer_.next();
    }
    require(tokenizer_.current(), pascal_compiler::tokenizer::token::sub_types::close_parenthesis);
    tokenizer_.next();
    return result;
}

tree_node_p syntax_analyzer::parse_statements() {
    auto result = std::make_shared<tree_node>("statements", tree_node::node_category::null, tokenizer_.current()->get_position());
    result->push_back(parse_statement());
    while (tokenizer_.current()->get_sub_type() == pascal_compiler::tokenizer::token::sub_types::semicolon) {
        tokenizer_.next();
        result->push_back(parse_statement());
    }
    return result;
}

tree_node_p syntax_analyzer::parse_statement() {
    const auto token = tokenizer_.current();
    switch (token->get_sub_type()) {
    case pascal_compiler::tokenizer::token::sub_types::begin:
        return parse_compound_statement();
    case pascal_compiler::tokenizer::token::sub_types::if_op:
        return parse_if_statement();
    case pascal_compiler::tokenizer::token::sub_types::while_op:
        return parse_while_statement();
    case pascal_compiler::tokenizer::token::sub_types::for_op:
        return parse_for_statement();
    case pascal_compiler::tokenizer::token::sub_types::repeat:
        return parse_repeat_statement();
    case pascal_compiler::tokenizer::token::sub_types::break_op:
        tokenizer_.next();
        return std::make_shared<tree_node>("break", tree_node::node_category::null, token->get_position());
    case pascal_compiler::tokenizer::token::sub_types::continue_op:
        tokenizer_.next();
        return std::make_shared<tree_node>("continue", tree_node::node_category::null, token->get_position());
    case pascal_compiler::tokenizer::token::sub_types::read:
        tokenizer_.next();
        return parse_read_statement();
    case pascal_compiler::tokenizer::token::sub_types::write:
        tokenizer_.next();
        return parse_write_statement();
    case pascal_compiler::tokenizer::token::sub_types::identifier:
    {
        const auto type = find_declaration(token).first;
        if (type->category() == type::type_category::function) {
            tokenizer_.next();
            return parse_function_call(std::make_shared<variable_node>(token->get_string_value(), token->get_position(), type));
        }
        return parse_assignment_statement();
    }
    default:
        break;
    }
    return nullptr;
}

tree_node_p syntax_analyzer::parse_compound_statement() {
    require(tokenizer_.current(), pascal_compiler::tokenizer::token::sub_types::begin);
    const auto result = parse_statements();
    require(tokenizer_.current(), pascal_compiler::tokenizer::token::sub_types::end);
    tokenizer_.next();
    return result;
}

tree_node_p syntax_analyzer::parse_if_statement() {
    auto result = std::make_shared<tree_node>("if", tree_node::node_category::null, tokenizer_.current()->get_position());
    tokenizer_.next();
    //TODO parse condition
    const auto cond = parse_expression();
    require(get_type(cond), type::type_category::integer, tokenizer_.current()->get_position());
    result->push_back(cond);
    require(tokenizer_.current(), pascal_compiler::tokenizer::token::sub_types::then);
    tokenizer_.next();
    result->push_back(parse_statement());
    if (tokenizer_.current()->get_sub_type() == pascal_compiler::tokenizer::token::sub_types::else_op) {
        tokenizer_.next();
        result->push_back(parse_statement());
    }
    return result;
}

tree_node_p syntax_analyzer::parse_while_statement() {
    auto result = std::make_shared<tree_node>("while", tree_node::node_category::null, tokenizer_.current()->get_position());
    tokenizer_.next();
    //TODO parse condition
    const auto cond = parse_expression();
    require(get_type(cond), type::type_category::integer, tokenizer_.current()->get_position());
    result->push_back(cond);
    require(tokenizer_.current(), pascal_compiler::tokenizer::token::sub_types::do_op);
    tokenizer_.next();
    result->push_back(parse_statement());
    return result;
}

tree_node_p syntax_analyzer::parse_for_statement() {
    auto result = std::make_shared<tree_node>("for", tree_node::node_category::null, tokenizer_.current()->get_position());
    auto token = tokenizer_.next();
    require(token, pascal_compiler::tokenizer::token::sub_types::identifier);
    const auto var = find_declaration(token).first;
    require(var, type::type_category::integer, token->get_position());
    result->push_back(std::make_shared<variable_node>(token->get_string_value(), token->get_position(), var));
    require(tokenizer_.next(), pascal_compiler::tokenizer::token::sub_types::assign);
    tokenizer_.next();
    auto expr = parse_expression();
    require(get_type(expr), type::type_category::integer, expr->position());
    result->push_back(expr);
    token = tokenizer_.current();
    if (token->get_sub_type() != pascal_compiler::tokenizer::token::sub_types::downto)
        require(token, pascal_compiler::tokenizer::token::sub_types::to);
    result->push_back(std::make_shared<tree_node>(token->get_value_string(), tree_node::node_category::null, token->get_position()));
    tokenizer_.next();
    expr = parse_expression();
    require(get_type(expr), type::type_category::integer, expr->position());
    result->push_back(expr);
    require(tokenizer_.current(), pascal_compiler::tokenizer::token::sub_types::do_op);
    tokenizer_.next();
    result->push_back(parse_statement());
    return result;
}

tree_node_p syntax_analyzer::parse_repeat_statement() {
    auto result = std::make_shared<tree_node>("repeat", tree_node::node_category::null, 
        tokenizer_.current()->get_position(), parse_statements());
    tokenizer_.next();
    require(tokenizer_.current(), pascal_compiler::tokenizer::token::sub_types::until);
    tokenizer_.next();
    //TODO parse condition
    const auto cond = parse_expression();
    require(get_type(cond), type::type_category::integer, tokenizer_.current()->get_position());
    result->push_back(cond);
    return result;
}

tree_node_p syntax_analyzer::parse_write_statement() {
    auto result = std::make_shared<tree_node>("write", tree_node::node_category::null, tokenizer_.current()->get_position());
    auto token = tokenizer_.next();
    require(token, pascal_compiler::tokenizer::token::sub_types::open_parenthesis);
    token = tokenizer_.next();
    while (token->get_sub_type() != pascal_compiler::tokenizer::token::sub_types::close_parenthesis) {
        if (token->get_sub_type() == pascal_compiler::tokenizer::token::sub_types::string_const) {
            result->push_back(std::make_shared<tree_node>(
                token->get_string_value(), tree_node::node_category::null, token->get_position()));
            token = tokenizer_.next();
        }
        else {
            result->push_back(parse_expression());
            token = tokenizer_.current();
        }
        if (token->get_sub_type() == pascal_compiler::tokenizer::token::sub_types::close_parenthesis)
            break;
        require(token, pascal_compiler::tokenizer::token::sub_types::comma);
        token = tokenizer_.next();
    }
    return result;
}

tree_node_p syntax_analyzer::parse_read_statement() {
    auto result = std::make_shared<tree_node>("read", tree_node::node_category::null, tokenizer_.current()->get_position());
    std::vector<tree_node_p> exprs;
    parse_expressions_list(exprs);
    for (const auto it : exprs) {
        const auto result_type = get_type(it);
        if (result_type->category() == type::type_category::modified && 
            std::dynamic_pointer_cast<modified_type>(result_type)->modificator() == modified_type::modificator_type::constant)
            throw syntax_error("Can't read to const variable", it->position());
        if (!result_type->is_scalar())
            throw syntax_error("Can't read to non scalar variable", it->position());
        result->push_back(it);
    }
    return result;
}

tree_node_p syntax_analyzer::parse_assignment_statement() {
    auto token = tokenizer_.current();
    auto result_type = find_declaration(token).first;
    tree_node_p node = std::make_shared<variable_node>(token->get_string_value(), token->get_position(), result_type);
    token = tokenizer_.next();
    switch (result_type->category()) {
    case type::type_category::array:
        node = parse_index(node);
        break;
    case type::type_category::record:
        node = parse_field_access(node);
        break;
    case type::type_category::modified:
        if (std::dynamic_pointer_cast<modified_type>(result_type)->modificator() == modified_type::modificator_type::constant)
            throw syntax_error("Can't read to const variable", token->get_position());
    default:
        break;
    }
    token = tokenizer_.current();
    if (token->get_sub_type() == pascal_compiler::tokenizer::token::sub_types::assign        ||
        token->get_sub_type() == pascal_compiler::tokenizer::token::sub_types::plus_assign   ||
        token->get_sub_type() == pascal_compiler::tokenizer::token::sub_types::minus_assign  ||
        token->get_sub_type() == pascal_compiler::tokenizer::token::sub_types::mult_assign   ||
        token->get_sub_type() == pascal_compiler::tokenizer::token::sub_types::divide_assign) {
        tokenizer_.next();
        auto expr = parse_expression();
        require_types_compatibility(result_type, get_type(expr), expr->position());
        if (result_type != get_type(expr))
            expr = std::make_shared<cast_node>(result_type, expr, expr->position());
        return std::make_shared<tree_node>(token->get_string_value(), 
            tree_node::node_category::null, token->get_position(), node, expr);
    }
    return node;
}

type_p syntax_analyzer::parse_type(const std::string& name) {
    auto token = tokenizer_.current();
    switch (token->get_sub_type()) {
    case pascal_compiler::tokenizer::token::sub_types::identifier:
    {
        tokenizer_.next();
        auto type = find_declaration(token);
        require(type.first, type::type_category::alias, token->get_position());
        if (!name.length())
            return std::make_shared<alias_type>(name, type.first);
        return type.first;
    }
    case pascal_compiler::tokenizer::token::sub_types::array:
    {
        require(tokenizer_.next(), pascal_compiler::tokenizer::token::sub_types::open_bracket);
        tokenizer_.next();
        const auto from = parse_expression();
        require_constant(from);
        require(get_type(from), type::type_category::integer, from->position());
        require(tokenizer_.current(), pascal_compiler::tokenizer::token::sub_types::range);
        tokenizer_.next();
        const auto to = parse_expression();
        require_constant(to);
        require(get_type(to), type::type_category::integer, to->position());
        require(tokenizer_.current(), pascal_compiler::tokenizer::token::sub_types::close_bracket);
        require(tokenizer_.next(), pascal_compiler::tokenizer::token::sub_types::of);
        tokenizer_.next();
        return std::make_shared<array_type>(name,
            std::static_pointer_cast<constant_node>(from)->get_value<long long>(),
            std::static_pointer_cast<constant_node>(to)->get_value<long long>(),
            parse_type());
    }
    case pascal_compiler::tokenizer::token::sub_types::record:
    {
        token = tokenizer_.next();
        auto type = std::make_shared<record_type>(name);
        while (token->get_sub_type() == pascal_compiler::tokenizer::token::sub_types::identifier) {
            require(tokenizer_.next(), pascal_compiler::tokenizer::token::sub_types::colon);
            tokenizer_.next();
            type->add_field(token->get_string_value(), parse_type());
            token = tokenizer_.current();
            if (token->get_sub_type() != pascal_compiler::tokenizer::token::sub_types::end) {
                require(tokenizer_.current(), pascal_compiler::tokenizer::token::sub_types::semicolon);
                token = tokenizer_.next();
            }
        }
        require(token, pascal_compiler::tokenizer::token::sub_types::end);
        tokenizer_.next();
    }
    default:
        throw unexpected_token(token, pascal_compiler::tokenizer::token::sub_types::identifier);
    }
}

void syntax_analyzer::parse_program() {
    tables_.push_back(symbols_table());
    tables_.back().add("integer", std::make_shared<alias_type>("integer", integer));
    tables_.back().add("real", std::make_shared<alias_type>("real", real));
    tables_.back().add("symbol", std::make_shared<alias_type>("symbol", symbol));

    auto token = tokenizer_.current();
    require(token, pascal_compiler::tokenizer::token::sub_types::program);
    token = tokenizer_.next();
    require(token, pascal_compiler::tokenizer::token::sub_types::identifier);
    require(tokenizer_.next(), pascal_compiler::tokenizer::token::sub_types::semicolon);
    tokenizer_.next();
    tables_.push_back(symbols_table());
    const auto block = parse_block();
    tables_.back().add(token->get_string_value(), 
        std::make_shared<function_type>(token->get_string_value(), symbols_table(), tables_.back()), block);
    require(tokenizer_.current(), pascal_compiler::tokenizer::token::sub_types::dot);
    tables_.pop_back();
    tokenizer_.next();
}

void syntax_analyzer::parse_expressions_list(std::vector<tree_node_p>& list) {
    auto token = tokenizer_.current();
    while (token->get_sub_type() != pascal_compiler::tokenizer::token::sub_types::close_parenthesis) {
        list.push_back(parse_expression());
        if (tokenizer_.current()->get_sub_type() == pascal_compiler::tokenizer::token::sub_types::close_parenthesis)
            break;
        require(pascal_compiler::tokenizer::token::sub_types::comma);
        token = tokenizer_.next();
    }
}

void syntax_analyzer::parse_identifier_list(std::vector<std::string>& names, symbols_table& table) {
    auto token = tokenizer_.current();
    do {
        require(token, pascal_compiler::tokenizer::token::sub_types::identifier);
        table.add(token->get_string_value(), nullptr);
        names.push_back(token->get_string_value());
        token = tokenizer_.next();
    } while ((token = tokenizer_.next())->get_sub_type() == pascal_compiler::tokenizer::token::sub_types::comma);
}

void syntax_analyzer::parse_declaration_part() {
    while (true) {
        switch (tokenizer_.current()->get_sub_type()) {
        case pascal_compiler::tokenizer::token::sub_types::type:
            tokenizer_.next();
            parse_type_declaration();
            break;
        case pascal_compiler::tokenizer::token::sub_types::var:
            tokenizer_.next();
            parse_var_declaration();
            break;
        case pascal_compiler::tokenizer::token::sub_types::const_op:
            tokenizer_.next();
            parse_const_declaration();
            break;
        case pascal_compiler::tokenizer::token::sub_types::procedure:
            tokenizer_.next();
            parse_procedure_declaration();
            break;
        case pascal_compiler::tokenizer::token::sub_types::function:
            tokenizer_.next();
            parse_function_declaration();
            break;
        default:
            return;
        }
    }
}

void syntax_analyzer::parse_type_declaration() {
    auto token = tokenizer_.current();
    while (token->get_sub_type() == pascal_compiler::tokenizer::token::sub_types::identifier) {
        require(tokenizer_.next(), pascal_compiler::tokenizer::token::sub_types::equal);
        tokenizer_.next();
        const auto type = parse_type(token->get_string_value());
        tables_.back().add(token->get_string_value(), type);
        require(tokenizer_.current(), pascal_compiler::tokenizer::token::sub_types::semicolon);
        token = tokenizer_.next();
    }
}

void syntax_analyzer::parse_var_declaration() {
    auto token = tokenizer_.current();
    while (token->get_sub_type() == pascal_compiler::tokenizer::token::sub_types::identifier) {
        std::vector<std::string> names;
        parse_identifier_list(names, tables_.back());
        require(tokenizer_.current(), pascal_compiler::tokenizer::token::sub_types::colon);
        tokenizer_.next();
        auto result_type = parse_type()->base_type();
        for (const auto it : names)
            tables_.back().change(it, make_pair(result_type, nullptr));
        token = tokenizer_.current();
        if (token->get_sub_type() == pascal_compiler::tokenizer::token::sub_types::equal) {
            if (names.size() > 1)
                throw syntax_error("Only 1 variable can be initialized", token->get_position());
            tokenizer_.next();
            const auto c = parse_typed_const(result_type);
            require_types_compatibility(result_type, get_type(c), token->get_position());
            tables_.back().change_last(make_pair(result_type, c));
        }
        require(tokenizer_.current(), pascal_compiler::tokenizer::token::sub_types::semicolon);
        token = tokenizer_.next();
    }
}

void syntax_analyzer::parse_const_declaration() {
    auto token = tokenizer_.current();
    while (token->get_sub_type() == pascal_compiler::tokenizer::token::sub_types::identifier) {
        type_p result_type = nullptr;
        tree_node_p value = nullptr;
        if (tokenizer_.next()->get_sub_type() == pascal_compiler::tokenizer::token::sub_types::colon) {
            tokenizer_.next();
            result_type = parse_type();
            require(tokenizer_.current(), pascal_compiler::tokenizer::token::sub_types::equal);
            tokenizer_.next();
            value = parse_typed_const(result_type);
        }
        else {
            require(tokenizer_.current(), pascal_compiler::tokenizer::token::sub_types::equal);
            tokenizer_.next();
            value = parse_expression();
            require_constant(value);
            result_type = get_type(value);
        }
        tables_.back().add(token->get_string_value(), 
            std::make_shared<modified_type>(modified_type::modificator_type::constant, result_type), value);
        require(tokenizer_.current(), pascal_compiler::tokenizer::token::sub_types::semicolon);
        token = tokenizer_.next();
    }
}

void syntax_analyzer::parse_function_declaration() {
    const auto token = tokenizer_.current();
    require(token, pascal_compiler::tokenizer::token::sub_types::identifier);
    tables_.push_back(symbols_table());
    tokenizer_.next();
    parse_formal_parameter_list();
    require(tokenizer_.current(), pascal_compiler::tokenizer::token::sub_types::colon);
    const auto rt = tokenizer_.next();
    require(rt, pascal_compiler::tokenizer::token::sub_types::identifier);
    const auto result_type = find_declaration(rt).first;
    require(result_type, type::type_category::alias, rt->get_position());
    require(tokenizer_.next(), pascal_compiler::tokenizer::token::sub_types::semicolon);
    tokenizer_.next();
    tables_.push_back(symbols_table());
    tables_.back().add("result", result_type);
    const auto block = parse_block();
    const auto vars = tables_.back(); tables_.pop_back();
    const auto args = tables_.back(); tables_.pop_back();
    tables_.back().add(token->get_string_value(), 
        std::make_shared<function_type>(token->get_string_value(), args, vars, result_type), block);
}

void syntax_analyzer::parse_procedure_declaration() {
    const auto token = tokenizer_.current();
    require(token, pascal_compiler::tokenizer::token::sub_types::identifier);
    tables_.push_back(symbols_table());
    tokenizer_.next();
    parse_formal_parameter_list();
    require(tokenizer_.next(), pascal_compiler::tokenizer::token::sub_types::semicolon);
    tokenizer_.next();
    tables_.push_back(symbols_table());
    tables_.back().add("result", nil);
    const auto block = parse_block();
    const auto vars = tables_.back(); tables_.pop_back();
    const auto args = tables_.back(); tables_.pop_back();
    tables_.back().add(token->get_string_value(),
        std::make_shared<function_type>(token->get_string_value(), args, vars), block);
}

void syntax_analyzer::parse_formal_parameter_list() {
    if (tokenizer_.current()->get_sub_type() != pascal_compiler::tokenizer::token::sub_types::open_parenthesis)
        return;
    auto token = tokenizer_.next();
    auto end = false;
    while (token->get_sub_type() != pascal_compiler::tokenizer::token::sub_types::close_parenthesis) {
        auto last = false;
        std::vector<std::string> names;
        parse_identifier_list(names, tables_.back());
        require(tokenizer_.current(), pascal_compiler::tokenizer::token::sub_types::colon);
        require(tokenizer_.next(), pascal_compiler::tokenizer::token::sub_types::identifier);
        auto result_type = find_declaration(tokenizer_.current()).first;
        require(result_type, type::type_category::alias, tokenizer_.current()->get_position());
        switch (token->get_sub_type()) {
        case pascal_compiler::tokenizer::token::sub_types::var:
            result_type = std::make_shared<modified_type>(modified_type::modificator_type::var, result_type);
            break;
        case pascal_compiler::tokenizer::token::sub_types::const_op:
            result_type = std::make_shared<modified_type>(modified_type::modificator_type::constant, result_type);
            break;
        default:
            break;
        }
        if (tokenizer_.next()->get_sub_type() == pascal_compiler::tokenizer::token::sub_types::equal) {
            if (names.size() > 1)
                throw syntax_error("Only 1 variable can be initialized", tokenizer_.current()->get_position());
            if (token->get_sub_type() == pascal_compiler::tokenizer::token::sub_types::var)
                throw syntax_error("Can't initialize value passed by reference", token->get_position());
            tokenizer_.next();
            const auto value = parse_expression();
            require_constant(value);
            require_types_compatibility(result_type, get_type(value), tokenizer_.current()->get_position());
            tables_.back().change_last(make_pair(result_type, value));
            end = true;
            last = true;
        }
        else 
            for (const auto it : names)
                tables_.back().change(it, make_pair(result_type, nullptr));
        if (end && !last)
            throw syntax_error("Default parameter should be last", tokenizer_.current()->get_position());
        if (tokenizer_.current()->get_sub_type() == pascal_compiler::tokenizer::token::sub_types::close_parenthesis)
            break;
        require(tokenizer_.current(), pascal_compiler::tokenizer::token::sub_types::semicolon);
        token = tokenizer_.next();
    }
    tokenizer_.next();
}

const symbols_table::symbol_t& syntax_analyzer::find_declaration(const pascal_compiler::tokenizer::token_p& token) {
    const auto name = token->get_string_value();
    for (auto& it : tables_) {
        symbols_table::table_t::const_iterator result;
        if ((result = it.table().find(name)) != it.table().end())
            return result->second;
    }
    throw declaration_not_found(token);
}

void syntax_analyzer::require(const pascal_compiler::tokenizer::token_p& token, const pascal_compiler::tokenizer::token::sub_types type) {
    if (token->get_sub_type() != type)
        throw unexpected_token(token, type);
}

void syntax_analyzer::require(const pascal_compiler::tokenizer::token::sub_types type) const {
    require(tokenizer_.current(), type);
}

void syntax_analyzer::require(const type_p& type, const type::type_category category, 
    const tree_node::position_type& position) {
    if (!type || type->category() != category)
        throw unexpected_type(type, category, position);
}

void syntax_analyzer::require_types_compatibility(const type_p& left, const type_p& right,
    const tree_node::position_type& position) {
    if (!left->is_category(type::type_category::alias) && !right->is_category(type::type_category::alias) &&
        (left == right ||
        left->is_category(type::type_category::real) && right->is_category(type::type_category::integer)))
        return;
    throw syntax_error(incompatible_types(left, right).what(), position);
}

void syntax_analyzer::require_constant(const tree_node_p& node) {
    if (node->category() != tree_node::node_category::constant)
        throw syntax_error("Illegal expression", node->position());
}

const tree_node_p syntax_analyzer::get_constant(const tree_node_p& node) {
    if (node->category() == tree_node::node_category::constant)
        return node;
    if (node->category() == tree_node::node_category::variable) {
        const auto var = std::static_pointer_cast<variable_node>(node);
        if (var->type()->category() == type::type_category::modified &&
            std::dynamic_pointer_cast<modified_type>(var->type())->modificator() == modified_type::modificator_type::constant &&
            var->value()->category() == tree_node::node_category::constant)
            return var->value();
        return var;
    }
    return node;
}
