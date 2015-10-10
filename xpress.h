#pragma once

#include <vector>
#include <map>
#include <functional>

#include <boost/variant/variant.hpp>

namespace xpress {

namespace detail {

struct PushConst
{
    double value;
    PushConst(double value) : value(value) {}
};

struct PushParam
{
    std::size_t index;
    PushParam(std::size_t index) : index(index) {}
};

using UnFunc = double (*)(double);
using BiFunc = double (*)(double,double);

struct UnaryOp
{
    UnFunc op;
    UnaryOp(UnFunc op) : op(op) {}
};

struct BinaryOp
{
    BiFunc op;
    BinaryOp(BiFunc op) : op(op) {}
};

using Instruction = boost::variant<PushConst,PushParam,UnaryOp,BinaryOp> ;


template <typename T>
struct ToDouble {
    using type = double;
};

double eval(const std::vector<Instruction> &instructions, const std::vector<double> &parameters);

template<typename ...D>
struct Functor
{
    std::vector<Instruction> instructions;

    Functor(std::vector<Instruction> &&instructions) : instructions(std::move(instructions)) { }

    double operator()(D... args)
    {
        return eval(instructions, { args... });
    }
};


std::vector<Instruction> compile(const std::string &text, std::vector<std::string> &&parameters);

} //namespace detail

template<typename ...S>
std::function<double(typename detail::ToDouble<S>::type...)> parse(const std::string &text, S ...var)
{
    return detail::Functor<typename detail::ToDouble<S>::type...>( detail::compile(text, { var... }) );
}


} // namespace gesser
