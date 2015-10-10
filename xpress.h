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

struct PushVar
{
    std::string name;
    PushVar(const std::string &name) : name(std::move(name)) {}
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

using Instruction = boost::variant<PushConst,PushVar,UnaryOp,BinaryOp> ;


template <typename T>
struct ToDouble {
    using type = double;
};

double eval(const std::vector<Instruction> &instructions, const std::map<std::string,double> &parameters);


template<int ...> struct seq {};

template<int N, int ...S> struct gen_seq : gen_seq<N-1, N-1, S...> {};

template<int ...S> struct gen_seq<0, S...>{ typedef seq<S...> type; };


template<typename ...S>
struct Functor
{
    std::vector<Instruction> instructions;
    std::vector<std::string> parameters;

    Functor(std::vector<Instruction> &&instructions,
            std::vector<std::string> &&parameters)
        : instructions(std::move(instructions))
        , parameters(std::move(parameters))
    { }

    double operator()(typename ToDouble<S>::type... args)
    {
        return eval(instructions, make_arguments(args..., typename gen_seq<sizeof...(args)>::type()));
    }

    template<int I>
    std::pair<std::string, double> make_arg(double value) {
        return std::make_pair(parameters[I], value);
    }

    template<int ...I>
    std::map<std::string,double> make_arguments(typename ToDouble<S>::type... args,  seq<I...> ) {
        std::map<std::string,double> result = { make_arg<I>(args)... };
        return result;
    }
};


std::vector<Instruction> compile(const std::string &text);

} //namespace detail

template<typename ...S>
std::function<double(typename detail::ToDouble<S>::type...)> parse(const char *text, S ...var)
{
    return detail::Functor<S...>( detail::compile(text), { var... } );
}

template<typename ...S>
std::function<double(typename detail::ToDouble<S>::type...)> parse(const std::string &text, S ...var)
{
    return detail::Functor<S...>( detail::compile(text), { var... } );
}


} // namespace gesser
