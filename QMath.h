#pragma once
#include <string>
#include <map>
#include <cmath>

namespace QMath
{
    bool isNumber(const std::string& numString);
    
	class Expression
	{

	public:
		virtual ~Expression();

		bool operator!= (const Expression &b);
		virtual bool operator== (const Expression &b) = 0;
		virtual std::string toString(bool showParentheses = false) = 0;
		virtual Expression* copyTree() = 0;
		virtual double evaluate() = 0;
		virtual Expression* simplify();
		virtual Expression* differentiate(char diffOperator = 'x') = 0;
		virtual bool isConstant() = 0;
		virtual bool isAtomic();
		virtual void substitute(const std::map<char, double>& varMap) = 0;
		virtual unsigned char precedence() = 0;
		virtual bool isCommutative();
		void substitute(char var, double value);
        
        static Expression* parse(const std::string& input, bool validateAndRectify = true);

	private:
        struct ExpressionData
        {
        public:
            enum Type
            {
                Operator = 0,
                Function = 1
            };
            
            std::string name;
            Type type;
            
            ExpressionData(std::string name, Type type);
        };
        
        static Expression* parseBaseCase(const std::string& input);
        static Expression* parseOperator(const std::string& inputLeft, const std::string& inputRight, const ExpressionData& operatorData);
        static Expression* parseFunction(const std::string& inputRight, const ExpressionData& operatorData);
        static std::string cleanseParseInput(std::string input);
        static unsigned char determineScopeDepth(const std::string& input, int expansionCenter);
	};

	class Operator : public Expression
	{
	public:
		Operator(Expression *left, Expression *right);
        Operator(double left, Expression *right);
        Operator(Expression *left, double right);
        Operator(double left, double right);
		~Operator();

		bool operator== (const Expression &b);
		bool isConstant();
		void substitute(const std::map<char, double>& varMap);
		void determineParentheses(bool& left, bool& right);
		void toStringOperands(std::string& left, std::string& right);

		template<typename T>
		static Expression* factoriseLinear(Expression *left, Expression *right);

		Expression* getLeftOperand();
		Expression* getRightOperand();

	protected:
		Expression *leftOperand;
		Expression *rightOperand;
	};

	class Add : public Operator
	{
	public:
		using Operator::Operator;

		std::string toString(bool showParentheses = false);
		Add* copyTree();
		double evaluate();
		Add* differentiate(char diffOperator);
		Expression* simplify();
		unsigned char precedence();
	};

	class Subtract : public Operator
	{
	public:
		using Operator::Operator;

		std::string toString(bool showParentheses = false);
		Subtract* copyTree();
		double evaluate();
		Subtract* differentiate(char diffOperator);
		Expression* simplify();
		unsigned char precedence();
		bool isCommutative();
	};

	class Multiply : public Operator
	{
	public:
		using Operator::Operator;

		std::string toString(bool showParentheses = false);
		Multiply* copyTree();
		double evaluate();
		Add* differentiate(char diffOperator);
		Expression* simplify();
		unsigned char precedence();
        bool isAtomic();
	};

	class Divide : public Operator
	{
	public:
		using Operator::Operator;

		std::string toString(bool showParentheses = false);
		Divide* copyTree();
		double evaluate();
		Divide* differentiate(char diffOperator);
		Expression* simplify();
		unsigned char precedence();
		bool isCommutative();
	};

	class Exponent : public Operator
	{
	public:
		using Operator::Operator;

		std::string toString(bool showParentheses = false);
		Exponent* copyTree();
		double evaluate();
		Multiply* differentiate(char diffOperator);
		Expression* simplify();
		unsigned char precedence();
		bool isCommutative();
	};
    
    class Differential : public Operator
    {
    public:
        using Operator::Operator;
        Differential(Expression *left, Expression *right, unsigned char order);
        
        std::string toString(bool showParentheses = false);
        Differential* copyTree();
        double evaluate();
        Expression* differentiate(char diffOperator);
        unsigned char precedence();
        bool isCommutative();
        
    private:
        unsigned char order = 1;
    };
    
    class Log : public Operator
    {
    public:
        Log(Expression *left, Expression *right);
        
        std::string toString(bool showParentheses = false);
        Log* copyTree();
        double evaluate();
        Expression* differentiate(char diffOperator);
        unsigned char precedence();
        bool isCommutative();
        Expression* simplify();
        
    private:
        bool isNatural;
        bool is10;
        unsigned char order = 1;
    };

	class Number : public Expression
	{
	public:
		Number(double value);
		operator double() const { return value; }

		bool operator== (const Expression &b);
		std::string toString(bool showParentheses = false);
		Number* copyTree();
		double evaluate();
		bool isConstant();
		Number* differentiate(char diffOperator);
		bool isAtomic();
		void substitute(const std::map<char, double>& varMap);
		unsigned char precedence();

	private:
		double value;
	};

	class Variable : public Expression
	{
	public:
		Variable(char var);
		Variable(char var, double value);

		bool operator== (const Expression &b);
		std::string toString(bool showParentheses = false);
		Variable* copyTree();
		double evaluate();
		bool isConstant();
		Expression* differentiate(char diffOperator);
		char charID();
		bool isAtomic();
		void substitute(const std::map<char, double>& varMap);
		unsigned char precedence();

	protected:
		char var;
		double value;
	};

	class Constant : public Variable
	{
    public:
        Constant(char var);
        Constant(char var, double value);
        
		Number* differentiate(char diffOperator);
        Constant* copyTree();
	};

	class Func : public Expression
	{
	public:
		Func(Expression *operand);
		~Func();

		bool operator== (const Expression &b);
        bool isAtomic();
		bool isConstant();
		void substitute(const std::map<char, double>& varMap);
		unsigned char precedence();

	protected:
		Expression *operand;
	};

	class Sin : public Func
	{
	public:
		using Func::Func;

		std::string toString(bool showParentheses = false);
		Sin* copyTree();
		double evaluate();
		Multiply* differentiate(char diffOperator);
		Sin* simplify();
	};

	class Cos : public Func
	{
	public:
		using Func::Func;

		std::string toString(bool showParentheses = false);
		Cos* copyTree();
		double evaluate();
		Multiply* differentiate(char diffOperator);
		Cos* simplify();
	};
    
    class Tan : public Func
    {
    public:
        using Func::Func;
        
        std::string toString(bool showParentheses = false);
        Tan* copyTree();
        double evaluate();
        Multiply* differentiate(char diffOperator);
        Tan* simplify();
    };
}
