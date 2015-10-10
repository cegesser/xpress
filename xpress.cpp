#include "xpress.h"

#include <boost/version.hpp>

#if BOOST_VERSION >= 103800
    #include <boost/spirit/include/classic_core.hpp>
    #include <boost/spirit/include/classic_confix.hpp>
    #include <boost/spirit/include/classic_escape_char.hpp>
    #include <boost/spirit/include/classic_multi_pass.hpp>
    #include <boost/spirit/include/classic_position_iterator.hpp>
    namespace spirit = boost::spirit::classic;
#else
    #include <boost/spirit/core.hpp>
    #include <boost/spirit/utility/confix.hpp>
    #include <boost/spirit/utility/escape_char.hpp>
    #include <boost/spirit/iterator/multi_pass.hpp>
    #include <boost/spirit/iterator/position_iterator.hpp>
    namespace spirit = boost::spirit;
#endif


#include "type_switch.h"

#include <map>
#include <vector>

using namespace xpress;
using namespace xpress::detail;

namespace {

double add(double lhs, double rhs) { return lhs+rhs; }
double sub(double lhs, double rhs) { return lhs-rhs; }
double mul(double lhs, double rhs) { return lhs*rhs; }
double div(double lhs, double rhs) { return lhs/rhs; }

double neg(double d) { return -d; }

template<typename T>
struct Emiter;

template<>
struct Emiter<PushConst>
{
    std::vector<Instruction> &instructions;

    Emiter(std::vector<Instruction> &instructions) : instructions(instructions) {}

    void operator()(double d) const {
        instructions.push_back(PushConst(d));
    }
};

template<>
struct Emiter<PushVar>
{
    std::vector<Instruction> &instructions;

    Emiter(std::vector<Instruction> &instructions) : instructions(instructions) {}

    template<typename Iterator>
    void operator()(Iterator start, Iterator end) const {
        instructions.push_back(PushVar(std::string(start, end)));
    }
};

template<>
struct Emiter<UnaryOp>
{
    UnFunc op;
    std::vector<Instruction> &instructions;

    Emiter(UnFunc op, std::vector<Instruction> &instructions)
        : op(op), instructions(instructions) {}

    template<typename Iterator>
    void operator()(Iterator, Iterator) const {
        instructions.push_back(UnaryOp(op));
    }

    template<typename T>
    void operator()(T) const {
        instructions.push_back(UnaryOp(op));
    }
};


template<>
struct Emiter<BinaryOp>
{
    BiFunc op;
    std::vector<Instruction> &instructions;

    Emiter(BiFunc op, std::vector<Instruction> &instructions)
        : op(op), instructions(instructions) {}

    template<typename Iterator>
    void operator()(Iterator, Iterator) const {
        instructions.push_back(BinaryOp(op));
    }
};

Emiter<PushConst> emit_const(std::vector<Instruction> &instructions) {
    return Emiter<PushConst>(instructions);
}

Emiter<PushVar> emit_var(std::vector<Instruction> &instructions) {
    return Emiter<PushVar>(instructions);
}

Emiter<UnaryOp> emit_op(UnFunc op, std::vector<Instruction> &instructions) {
    return Emiter<UnaryOp>(op, instructions);
}

Emiter<BinaryOp> emit_op(BiFunc op, std::vector<Instruction> &instructions) {
    return Emiter<BinaryOp>(op, instructions);
}

class Compiler : public spirit::grammar< Compiler >
{
public:

    std::vector<Instruction> *instructions = nullptr;

    template< typename ScannerT >
    class definition
    {
    public:

        definition( const Compiler& self )
        {
            std::vector<Instruction> &instructions = *self.instructions;
            using namespace spirit;
            expression = term >> *( ch_p('+') >> term [emit_op(add, instructions)]
                                  | ch_p('-') >> term [emit_op(sub, instructions)])
                       ;

            term       = factor >> *( ch_p('*') >> factor [emit_op(mul, instructions)]
                                    | ch_p('/') >> factor [emit_op(div, instructions)] )
                       ;

            factor     = primary >> *( ch_p('^') >> factor [emit_op(pow, instructions)] );

            primary    = ureal_p [emit_const(*self.instructions)]
                       | ch_p('-') >> primary [emit_op(neg, instructions)]
                       | identifier [emit_var(*self.instructions)]
                       | function
                       | sub_expression
                       ;

            sub_expression = ch_p('(') >> expression >> ch_p(')');

            function   = str_p("exp")  >> sub_expression [emit_op(std::exp,   instructions)]
                       | str_p("log")  >> sub_expression [emit_op(std::log10, instructions)]
                       | str_p("sqrt") >> sub_expression [emit_op(std::sqrt,  instructions)]
                       ;

            identifier = alpha_p >> *(alnum_p);
        }

        spirit::rule< ScannerT > term, factor, primary, expression, identifier, function, sub_expression;

        const spirit::rule< ScannerT >& start() const { return expression; }
    };

    template< class Iter_type>
    std::vector<Instruction> compile( Iter_type begin, Iter_type end)
    {
        std::vector<Instruction> result;
        instructions = &result;

        const spirit::parse_info< Iter_type > info = spirit::parse( begin, end, *this, spirit::space_p);
        if (!info.hit) {
            throw std::runtime_error("error");
        }
        instructions = nullptr;
        return result;
    }
};

inline void execute(const Instruction &instruction, const std::map<std::string,double> &arguments, std::vector<double> &stack)
{
    type_switch(instruction,
        [&stack](const PushConst &inst) {
            stack.push_back(inst.value);
        },
        [&stack, &arguments](const PushVar &inst) {
            auto iter = arguments.find(inst.name);
            stack.push_back( iter->second );
        },
        [&stack](const UnaryOp &inst) {
            double v = stack.back(); stack.pop_back();
            stack.push_back( inst.op(v) );
        },
        [&stack](const BinaryOp &inst) {
            double rhs = stack.back(); stack.pop_back();
            double lhs = stack.back(); stack.pop_back();
            stack.push_back( inst.op(lhs, rhs) );
        }
    );
}

//inline std::string to_string(const Instruction &instruction)
//{
//    return type_switch(instruction,
//        [](const PushConst &inst) {
//            return "Push " + std::to_string(inst.value);
//        },
//        [](const PushVar &inst) {
//            return "Push " + inst.name;
//        },
//        [](const UnaryOp &inst) {
//            return  std::string(" [1]");
//        },
//        [](const BinaryOp &inst) {
//            return  std::string(" [2]");
//        }
//    );
//}

} //namespace

double xpress::detail::eval(const std::vector<Instruction> &instructions, const std::map<std::string,double> &arguments)
{
    std::vector<double> stack;
    for (Instruction const &instruction : instructions)
    {
        execute(instruction, arguments, stack);
    }
    return stack.back();
}

std::vector<Instruction> xpress::detail::compile(const std::string &text)
{
    Compiler compiler;
    return compiler.compile(text.begin(), text.end());
}
